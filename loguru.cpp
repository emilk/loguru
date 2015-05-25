#include "loguru.hpp"

#include <chrono>
#include <cstdio>
#include <cstdarg>
#include <mutex>
#include <unordered_map>

#include <pthread.h>

#include <boost/date_time/posix_time/posix_time.hpp> // TODO: remove dependency

namespace loguru
{
	// ------------------------------------------------------------------------------
	// Helpers:

	// free after use
	char* strprintf(const char* format, va_list vlist)
	{
#if !defined(_MSC_VER)
		char* buff = nullptr;
		int result = vasprintf(&buff, format, vlist);
		CHECK(result >= 0, "Bad string format: '%s'", format);
		return buff;
#else
		int bytes_needed = vsnprintf(nullptr, 0, format, vlist);
		CHECK(bytes_needed >= 0, "Bad string format: '%s'", format);
		char* buff = (char*)malloc(bytes_needed + 1);
		vsnprintf(str.data(), bytes_needed, format, vlist);
		return buff;
#endif
	}

	// Returns a pointer to this many spaces (up to a reasonable max);
	const char* spaces(unsigned count)
	{
		static const char* buff =
		"          " "          " "          " "          " "          "
		"          " "          " "          " "          " "          "
		"          " "          " "          " "          " "          "
		"          " "          " "          " "          " "          ";
		count = std::min<unsigned>(count, 200);
		return buff + 200 - count;
	}

	// Returns a pointer to this many spaces (up to a reasonable max);
	const char* indentation(unsigned depth, unsigned tw)
	{
		return spaces(depth * tw);
	}

	// ------------------------------------------------------------------------------

	struct Callback
	{
		log_handler_t callback;
		void*         user_data;
	};

	using namespace std::chrono;
	typedef std::chrono::high_resolution_clock Clock;

	const auto SCOPE_TYPE = LogType::SPAM;
	const auto INDENTATION_WIDTH = 4;
	const auto SCOPE_TIME_PRECISION = 3; // 3=ms, 6≈us, 9=ns

	//const auto s_start_time = Clock::now();
	std::mutex s_mutex;
	std::unordered_map<std::string, Callback> s_callbacks;
	FILE* s_err  = stderr;
	FILE* s_out  = stdout;
	FILE* s_file = nullptr;
	fatal_handler_t s_fatal_handler = abort;
	bool s_strip_file_path = true;
	std::atomic<int> s_indentation { 0 };
	int s_log_level = INT_MAX;

	const char* PREAMBLE_EXPLAIN = "date       time         ( uptime  ) [thread name     thread id]                 file:line lvl| ";

	void init(int argc, char* argv[])
	{
		// TODO: unique file name?
		//auto path = boost::filesystem::unique_path();
		//g_file = fopen(path.c_str(), "w");

		CHECK(argc > 0, "Expected proper argc/argv");

		fprintf(stdout, "%s\n", PREAMBLE_EXPLAIN); fflush(stdout);
		if (s_file) {
			fprintf(s_file, "%s\n", PREAMBLE_EXPLAIN); fflush(s_file);
		}

		LOG(INFO, "argv[0]: %s", argv[0]);
	}

	// Will be called right before abort().
	void set_fatal_handler(fatal_handler_t handler)
	{
		s_fatal_handler = handler;
	}

	void add_callback(const char* id, log_handler_t callback, void* user_data)
	{
		s_callbacks.insert(std::make_pair(std::string(id), Callback{callback, user_data}));
	}

	void remove_callback(const char* id)
	{
		s_callbacks.erase(std::string(id));
	}

	// ------------------------------------------------------------------------

	void log_preamble(FILE* out, LogType type, const char* file, unsigned line, const char* prefix)
	{
		// Date:
		time_t rawtime;
		time(&rawtime);
		auto timeinfo = localtime(&rawtime);
		char date_buff[80];
		strftime(date_buff, sizeof(date_buff), "%Y-%m-%d", timeinfo);

		// Time: http://stackoverflow.com/questions/16077299/how-to-print-current-time-with-milliseconds-using-c-c11
		auto now = boost::posix_time::microsec_clock::local_time();
		auto td = now.time_of_day();
		const long hours        = td.hours();
		const long minutes      = td.minutes();
		const long seconds      = td.seconds();
		const long milliseconds = td.total_milliseconds() - ((hours * 3600 + minutes * 60 + seconds) * 1000);

		// Uptime:
		//auto end = Clock::now();
		//auto uptime_ms = duration_cast<std::chrono::milliseconds>(end - s_start_time).count();
		auto uptime_sec = (double)clock() / CLOCKS_PER_SEC;

		// Thread id/name:
		//std::stringstream ss;
		//ss << std::this_thread::get_id(); // 64 bit hex :T
		// thread_id_str = ss.str().c_str()

		uint64_t thread_id;
		pthread_threadid_np(pthread_self(), &thread_id);
		char thread_id_str[64];
		snprintf(thread_id_str, sizeof(thread_id_str) - 1, "%X", (unsigned)thread_id);

		char thread_name[128] = {0};
		pthread_getname_np(pthread_self(), thread_name, sizeof(thread_name));

		if (s_strip_file_path) {
			for (auto ptr = file; *ptr; ++ptr) {
				if (*ptr == '/' || *ptr == '\\') {
					file = ptr + 1;
				}
			}
		}

		fprintf(out, "%s %02ld:%02ld:%02ld.%03ld (%8.3fs) [%-16s %-8s] %20s:%-5u % d| %s%s",
		        date_buff, hours, minutes, seconds, milliseconds,
		        uptime_sec,
		        thread_name, thread_id_str, file, line,
		        (int)type,
		        indentation(s_indentation, INDENTATION_WIDTH), prefix);
	}

	void log_line(FILE* out, LogType type, const char* file, unsigned line, const char* prefix, const char* buff)
	{
		if (s_log_level < (int)type)
		{
			return;
		}

		log_preamble(out, type, file, line, prefix);
		fprintf(out, "%s\n", buff);
		fflush(out);
	}

	void log_with_prefix(LogType type, const char* file, unsigned line, const char* prefix, const char* format, va_list vlist)
	{
		std::lock_guard<std::mutex> lg(s_mutex);
		auto buff = strprintf(format, vlist);

		FILE* out = ((int)type <= (int)LogType::WARNING ? s_err : s_out);
		log_line(out, type, file, line, prefix, buff);

		if (s_file) {
			log_line(s_file, type, file, line, prefix, buff);
		}

		for (auto& p : s_callbacks) {
			//p.second(type, str.c_str());
			p.second.callback(p.second.user_data, type, buff);
		}

		free(buff);
	}

	void logv(LogType type, const char* file, unsigned line, const char* format, va_list vlist)
	{
		const char* prefix;
		switch (type)
		{
			case LogType::WARNING:
				prefix = "⚠ WARNING: ";
				break;

			case LogType::ERROR:
				prefix = "🔴 ERROR: ";
				break;

			case LogType::FATAL:
				prefix = "🔴 FATAL: ";
				break;

			default:
				prefix = "";
				break;
		}

		log_with_prefix(type, file, line, prefix, format, vlist);
	}

	void log(LogType type, const char* file, unsigned line, const char* prefix, const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		log_with_prefix(type, file, line, prefix, format, vlist);
		va_end(vlist);

		if (type == LogType::FATAL) {
			if (s_fatal_handler) {
				s_fatal_handler();
			}
			abort();
		}
	}

	void log(LogType type, const char* file, unsigned line, const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		logv(type, file, line, format, vlist);
		va_end(vlist);
	}

	long long now_ns()
	{
		return duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
	}

	LogScopeRAII::LogScopeRAII(const char* file, unsigned line, const char* format, ...)
		: _file(file), _line(line), _start_time_ns(now_ns())
	{
		va_list vlist;
		va_start(vlist, format);
		log_with_prefix(SCOPE_TYPE, file, line, "{ ", format, vlist);
		va_end(vlist);
		++s_indentation;
	}

	LogScopeRAII::~LogScopeRAII()
	{
		--s_indentation;
		//log_with_prefix(SCOPE_TYPE, _file, _line, "}", "");
		auto duration_sec = (now_ns() - _start_time_ns) / 1e9;
		log(SCOPE_TYPE, _file, _line, "} %.*f s", SCOPE_TIME_PRECISION, duration_sec);
	}

	void on_assertion_failed(const char* expr, const char* file, unsigned line, const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		log_with_prefix(LogType::FATAL, file, line, expr, format, vlist);
		va_end(vlist);
		if (s_fatal_handler) {
			s_fatal_handler();
		}
		abort();
	}
} // namespace loguru
