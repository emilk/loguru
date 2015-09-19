#include "loguru.hpp"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <unordered_map>

#ifndef _MSC_VER
	#include <pthread.h>
#endif

using namespace std::chrono;

namespace loguru
{
	struct Callback
	{
		log_handler_t callback;
		void*         user_data;
	};

	using CallbackMap = std::unordered_map<std::string, Callback>;

	const auto SCOPE_TIME_PRECISION = 3; // 3=ms, 6≈us, 9=ns

	const auto s_start_time = system_clock::now();
	int              g_verbosity       = INT_MAX;
	std::mutex       s_mutex;
	CallbackMap      s_callbacks;
	FILE*            s_err             = stderr;
	FILE*            s_out             = stdout;
	FILE*            s_file            = nullptr;
	fatal_handler_t  s_fatal_handler   = ::abort;
	bool             s_strip_file_path = true;
	std::atomic<int> s_indentation     { 0 };

	const int THREAD_NAME_WIDTH = 16;
	const char* PREAMBLE_EXPLAIN = "date       time         ( uptime  ) [ thread name/id ]                 file:line     v| ";

	// ------------------------------------------------------------------------------
	// Helpers:

	// free after use
	static char* strprintf(const char* format, va_list vlist)
	{
#ifdef _MSC_VER
		int bytes_needed = vsnprintf(nullptr, 0, format, vlist);
		CHECK(bytes_needed >= 0, "Bad string format: '%s'", format);
		char* buff = (char*)malloc(bytes_needed + 1);
		vsnprintf(str.data(), bytes_needed, format, vlist);
		return buff;
#else
		char* buff = nullptr;
		int result = vasprintf(&buff, format, vlist);
		CHECK(result >= 0, "Bad string format: '%s'", format);
		return buff;
#endif
	}

	const char* indentation(unsigned depth)
	{
		static const char* buff =
		".  .  .  .  .  .  .  .  .  .  " ".  .  .  .  .  .  .  .  .  .  "
		".  .  .  .  .  .  .  .  .  .  " ".  .  .  .  .  .  .  .  .  .  "
		".  .  .  .  .  .  .  .  .  .  " ".  .  .  .  .  .  .  .  .  .  "
		".  .  .  .  .  .  .  .  .  .  " ".  .  .  .  .  .  .  .  .  .  "
		".  .  .  .  .  .  .  .  .  .  " ".  .  .  .  .  .  .  .  .  .  ";
		depth = std::min<unsigned>(depth, 100);
		return buff + 3 * (100 - depth);
	}

	static void parse_args(int& argc, char* argv[])
	{
		CHECK_GT(argc,       0,       "Expected proper argc/argv");
		CHECK_EQ(argv[argc], nullptr, "Expected proper argc/argv");

		int arg_dest = 1;
		int out_argc = argc;

		for (int arg_it = 1; arg_it < argc; ++arg_it)
		{
			auto cmd = argv[arg_it];
			if (strncmp(cmd, "-v", 2) == 0 && !std::isalpha(cmd[2])) {
				out_argc -= 1;
				auto value_str = cmd + 2;
				if (value_str[0] == '\0') {
					// Value in separate argument
					arg_it += 1;
					value_str =  argv[arg_it];
					out_argc -= 1;
				}
				if (*value_str == '=') { value_str += 1; }
				g_verbosity = atoi(value_str);
			} else {
				argv[arg_dest++] = argv[arg_it];
			}
		}

		argc = out_argc;
		argv[argc] = nullptr;
	}

	static long long now_ns()
	{
		return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}

	// ------------------------------------------------------------------------------

	void init(int& argc, char* argv[])
	{
		parse_args(argc, argv);

#ifndef _MSC_VER
		// Set thread name, unless it is already set:
		char old_thread_name[128] = {0};
		pthread_getname_np(pthread_self(), old_thread_name, sizeof(old_thread_name));
		if (old_thread_name[0] == 0)
		{
			pthread_setname_np("main thread");
		}
#endif

		// TODO: unique file name?
		//auto path = boost::filesystem::unique_path();
		//s_file = fopen(path.c_str(), "w");

		fprintf(stdout, "%s\n", PREAMBLE_EXPLAIN); fflush(stdout);
		if (s_file) {
			fprintf(s_file, "%s\n", PREAMBLE_EXPLAIN); fflush(s_file);
		}

		LOG(INFO, "argv[0]:         %s", argv[0]);
		LOG(INFO, "Verbosity level: %d", g_verbosity);
		LOG(INFO, "-----------------------------------");
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

	void set_thread_name(const char* name)
	{
		pthread_setname_np(name);
	}

	// ------------------------------------------------------------------------

	static void log_line(FILE* out, Verbosity verbosity, const char* file, unsigned line, const char* prefix, const char* message)
	{
		auto now = system_clock::now();
		time_t ms_since_epoch = duration_cast<milliseconds>(now.time_since_epoch()).count();
		time_t sec_since_epoch = ms_since_epoch / 1000;
		tm time_info;
		localtime_r(&sec_since_epoch, &time_info);

		auto uptime_ms = duration_cast<milliseconds>(now - s_start_time).count();
		auto uptime_sec = uptime_ms / 1000.0;

#ifdef _MSC_VER
		const char* thread_name = ""; // TODO
#else
		auto thread = pthread_self();
		char thread_name[THREAD_NAME_WIDTH + 1] = {0};
		pthread_getname_np(thread, thread_name, sizeof(thread_name));

		if (thread_name[0] == 0) {
			uint64_t thread_id;
			pthread_threadid_np(thread, &thread_id);
			snprintf(thread_name, sizeof(thread_name), "%16X", (unsigned)thread_id);
		}
#endif

		if (s_strip_file_path) {
			for (auto ptr = file; *ptr; ++ptr) {
				if (*ptr == '/' || *ptr == '\\') {
					file = ptr + 1;
				}
			}
		}

		char level_buff[6];
		if (verbosity <= NamedVerbosity::FATAL) {
			strcpy(level_buff, "FATL");
		} else if (verbosity == NamedVerbosity::ERROR) {
			strcpy(level_buff, "ERR");
		} else if (verbosity == NamedVerbosity::WARNING) {
			strcpy(level_buff, "WARN");
		} else {
			snprintf(level_buff, sizeof(level_buff) - 1, "% 4d", verbosity);
		}

		fprintf(out, "%04d-%02d-%02d %02d:%02d:%02d.%03ld (%8.3fs) [%-*s] %20s:%-5u %4s| %s%s%s\n",
			1900 + time_info.tm_year, 1 + time_info.tm_mon, time_info.tm_mday,
			time_info.tm_hour, time_info.tm_min, time_info.tm_sec, ms_since_epoch % 1000,
			uptime_sec,
			THREAD_NAME_WIDTH, thread_name,
			file, line, level_buff,
			indentation(s_indentation), prefix, message);
		fflush(out);
	}

	void log_to_everywhere(Verbosity verbosity, const char* file, unsigned line, const char* prefix, const char* buff)
	{
		std::lock_guard<std::mutex> lg(s_mutex);

		FILE* out = ((int)verbosity <= (int)NamedVerbosity::WARNING ? s_err : s_out);
		log_line(out, verbosity, file, line, prefix, buff);

		if (s_file) {
			log_line(s_file, verbosity, file, line, prefix, buff);
		}

		for (auto& p : s_callbacks) {
			p.second.callback(p.second.user_data, verbosity, buff);
		}

		if (verbosity == (Verbosity)NamedVerbosity::FATAL) {
			if (s_fatal_handler) {
				s_fatal_handler();
			}
			abort();
		}
	}

	void log_to_everywhere_v(Verbosity verbosity, const char* file, unsigned line, const char* prefix, const char* format, va_list vlist)
	{
		auto buff = strprintf(format, vlist);
		log_to_everywhere(verbosity, file, line, prefix, buff);
		free(buff);
	}

	void logv(Verbosity verbosity, const char* file, unsigned line, const char* format, va_list vlist)
	{
		log_to_everywhere_v(verbosity, file, line, "", format, vlist);
	}

	void log(Verbosity verbosity, const char* file, unsigned line, const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		logv(verbosity, file, line, format, vlist);
		va_end(vlist);
	}

	LogScopeRAII::LogScopeRAII(Verbosity verbosity, const char* file, unsigned line, const char* format, ...)
		: _verbosity(verbosity), _file(file), _line(line), _start_time_ns(now_ns())
	{
		if ((int)verbosity <= g_verbosity) {
			va_list vlist;
			va_start(vlist, format);
			vsnprintf(_name, sizeof(_name), format, vlist);
			log_to_everywhere(_verbosity, file, line, "{ ", _name);
			va_end(vlist);
			++s_indentation;
		} else {
			_file = nullptr;
		}
	}

	LogScopeRAII::~LogScopeRAII()
	{
		if (_file) {
			--s_indentation;
			auto duration_sec = (now_ns() - _start_time_ns) / 1e9;
			log(_verbosity, _file, _line, "} %.*f s: %s", SCOPE_TIME_PRECISION, duration_sec, _name);
		}
	}

	void on_assertion_failed(const char* expr, const char* file, unsigned line, const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		log_to_everywhere_v(NamedVerbosity::FATAL, file, line, expr, format, vlist);
		va_end(vlist);
		if (s_fatal_handler) {
			s_fatal_handler();
		}
		abort();
	}

	void on_assertion_failed(const char* expr, const char* file, unsigned line)
	{
		on_assertion_failed(expr, file, line, "");
	}
} // namespace loguru
