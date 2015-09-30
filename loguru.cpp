#include "loguru.hpp"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <vector>

#ifndef _MSC_VER
	#include <pthread.h>
#endif

using namespace std::chrono;

namespace loguru
{
	struct Callback
	{
		std::string     id;
		log_handler_t   callback;
		void*           user_data;
		close_handler_t close;
	};

	using CallbackVec = std::vector<Callback>;

	const auto SCOPE_TIME_PRECISION = 3; // 3=ms, 6≈us, 9=ns

	const auto s_start_time = system_clock::now();
	int                  g_verbosity       = 9;
	std::recursive_mutex s_mutex;
	std::string          s_file_arguments;
	CallbackVec          s_callbacks;
	FILE*                s_err             = stderr;
	FILE*                s_out             = stdout;
	fatal_handler_t      s_fatal_handler   = ::abort;
	bool                 s_strip_file_path = true;
	std::atomic<int>     s_indentation     { 0 };

	const int THREAD_NAME_WIDTH = 16;
	const char* PREAMBLE_EXPLAIN = "date       time         ( uptime  ) [ thread name/id ]                 file:line     v| ";

	// ------------------------------------------------------------------------------

	void file_log(void* user_data, const Message& message)
	{
		FILE* file = reinterpret_cast<FILE*>(user_data);
		fprintf(file, "%s%s%s%s\n",
			message.preamble, message.indentation, message.prefix, message.message);
		fflush(file);
	}

	void file_close(void* user_data)
	{
		FILE* file = reinterpret_cast<FILE*>(user_data);
		fclose(file);
	}

	// ------------------------------------------------------------------------------

	// Helpers:

	// free after use
	static char* strprintf(const char* format, va_list vlist)
	{
#ifdef _MSC_VER
		int bytes_needed = vsnprintf(nullptr, 0, format, vlist);
		CHECK_F(bytes_needed >= 0, "Bad string format: '%s'", format);
		char* buff = (char*)malloc(bytes_needed + 1);
		vsnprintf(str.data(), bytes_needed, format, vlist);
		return buff;
#else
		char* buff = nullptr;
		int result = vasprintf(&buff, format, vlist);
		CHECK_F(result >= 0, "Bad string format: '%s'", format);
		return buff;
#endif
	}

	const char* indentation(unsigned depth)
	{
		static const char* buff =
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   "
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   "
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   "
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   "
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   ";
		depth = std::min<unsigned>(depth, 100);
		return buff + 4 * (100 - depth);
	}

	static void parse_args(int& argc, char* argv[])
	{
		CHECK_GT_F(argc,       0,       "Expected proper argc/argv");
		CHECK_EQ_F(argv[argc], nullptr, "Expected proper argc/argv");

		int arg_dest = 1;
		int out_argc = argc;

		for (int arg_it = 1; arg_it < argc; ++arg_it) {
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
		s_file_arguments = "";
		for (int i = 0; i < argc; ++i) {
			s_file_arguments += argv[i];
			if (i + 1 < argc) {
				s_file_arguments += " ";
			}
		}

		parse_args(argc, argv);

#ifndef _MSC_VER
		// Set thread name, unless it is already set:
		char old_thread_name[128] = {0};
		pthread_getname_np(pthread_self(), old_thread_name, sizeof(old_thread_name));
		if (old_thread_name[0] == 0) {
			pthread_setname_np("main thread");
		}
#endif

		fprintf(stdout, "%s\n", PREAMBLE_EXPLAIN); fflush(stdout);

		LOG_F(INFO, "arguments:       %s", s_file_arguments.c_str());
		LOG_F(INFO, "Verbosity level: %d", g_verbosity);
		LOG_F(INFO, "-----------------------------------");
	}

	bool add_file(const char* path)
	{
		auto file = fopen(path, "wa");
		if (!file) {
			LOG_F(ERROR, "Failed to open '%s'", path);
			return false;
		}
		add_callback(path, file_log, file, file_close);
		LOG_F(INFO, "Logging to '%s'", path);
		return true;
	}

	// Will be called right before abort().
	void set_fatal_handler(fatal_handler_t handler)
	{
		s_fatal_handler = handler;
	}

	void add_callback(const char* id, log_handler_t callback, void* user_data,
					  close_handler_t on_close)
	{
		std::lock_guard<std::recursive_mutex> lock(s_mutex);
		s_callbacks.push_back(Callback{id, callback, user_data, on_close});
	}

	void remove_callback(const char* id)
	{
		std::lock_guard<std::recursive_mutex> lock(s_mutex);
		auto it = std::find_if(begin(s_callbacks), end(s_callbacks), [&](const Callback& c) { return c.id == id; });
		if (it != s_callbacks.end()) {
			if (it->close) { it->close(it->user_data); }
			s_callbacks.erase(it);
		} else {
			LOG_F(ERROR, "Failed to locate callback with id '%s'", id);
		}
	}

	void set_thread_name(const char* name)
	{
		pthread_setname_np(name);
	}

	// ------------------------------------------------------------------------

	static void print_preamble(char* out_buff, size_t out_buff_size, Verbosity verbosity, const char* file, unsigned line)
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

		snprintf(out_buff, out_buff_size, "%04d-%02d-%02d %02d:%02d:%02d.%03ld (%8.3fs) [%-*s] %20s:%-5u %4s| ",
			1900 + time_info.tm_year, 1 + time_info.tm_mon, time_info.tm_mday,
			time_info.tm_hour, time_info.tm_min, time_info.tm_sec, ms_since_epoch % 1000,
			uptime_sec,
			THREAD_NAME_WIDTH, thread_name,
			file, line, level_buff);
	}

	void log_to_everywhere(Verbosity verbosity, const char* file, unsigned line, const char* prefix,
						   const char* buff)
	{
		char preamble_buff[128];
		print_preamble(preamble_buff, sizeof(preamble_buff), verbosity, file, line);

		auto message =
			Message{verbosity, preamble_buff, indentation(s_indentation), prefix, buff};

		std::lock_guard<std::recursive_mutex> lock(s_mutex);

		FILE* out = (verbosity <= static_cast<Verbosity>(NamedVerbosity::WARNING) ? s_err : s_out);
		fprintf(out, "%s%s%s%s\n",
			message.preamble, message.indentation, message.prefix, message.message);
		fflush(out);

		for (auto& p : s_callbacks) {
			p.callback(p.user_data, message);
		}

		if (verbosity == static_cast<Verbosity>(NamedVerbosity::FATAL)) {
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

	void log(Verbosity verbosity, const char* file, unsigned line, const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		log_to_everywhere_v(verbosity, file, line, "", format, vlist);
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

	void log_and_abort(const char* expr, const char* file, unsigned line, const char* format, ...)
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

	void log_and_abort(const char* expr, const char* file, unsigned line)
	{
		log_and_abort(expr, file, line, "");
	}
} // namespace loguru
