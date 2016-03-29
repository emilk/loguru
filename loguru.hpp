/*
Loguru logging library for C++, by Emil Ernerfeldt.
www.github.com/emilk/loguru
If you find Loguru useful, please let me know on twitter or in a mail!
Twitter: @ernerfeldt
Mail:    emil.ernerfeldt@gmail.com
Website: www.ilikebigbits.com

# License
	This software is in the public domain. Where that dedication is not
	recognized, you are granted a perpetual, irrevocable license to copy
	and modify this file as you see fit.

# Inspiration
	Much of Loguru was inspired by GLOG, https://code.google.com/p/google-glog/.
	The whole "single header" and public domain is fully due Sean T. Barrett
	and his wonderful stb libraries at https://github.com/nothings/stb.

# Version history
	* Version 0.10 - 2015-03-22 - Works great on Mac.
	* Version 0.20 - 2015-09-17 - Removed the only dependency.
	* Version 0.30 - 2015-10-02 - Drop-in replacement for most of GLOG
	* Version 0.40 - 2015-10-07 - Single-file!
	* Version 0.50 - 2015-10-17 - Improved file logging
	* Version 0.60 - 2015-10-24 - Add stack traces
	* Version 0.70 - 2015-10-27 - Signals
	* Version 0.80 - 2015-10-30 - Color logging.
	* Version 0.90 - 2015-11-26 - ABORT_S and proper handling of FATAL
	* Verison 1.00 - 2015-02-14 - ERROR_CONTEXT
	* Verison 1.10 - 2015-02-19 - -v OFF, -v INFO etc
	* Verison 1.11 - 2015-02-20 - textprintf vs strprintf
	* Verison 1.12 - 2015-02-22 - Remove g_alsologtostderr
	* Verison 1.13 - 2015-02-29 - ERROR_CONTEXT as linked list
	* Verison 1.20 - 2015-03-19 - Add get_thread_name()
	* Verison 1.21 - 2015-03-20 - Minor fixes
	* Verison 1.22 - 2015-03-29 - Fix issues with set_fatal_handler throwing an exception

# Compiling
	Just include <loguru.hpp> where you want to use Loguru.
	Then, in one .cpp file:
		#define LOGURU_IMPLEMENTATION
		#include <loguru.hpp>
	Make sure you compile with -std=c++11 -lpthread -ldl

# Usage
	#include <loguru.hpp>

	// Optional, but useful to time-stamp the start of the log.
	// Will also detect verbosity level on command line as -v.
	loguru::init(argc, argv);

	// Put every log message in "everything.log":
	loguru::add_file("everything.log", loguru::Append);

	// Only log INFO, WARNING, ERROR and FATAL to "latest_readable.log":
	loguru::add_file("latest_readable.log", loguru::Truncate, Verbosity_INFO);

	// Only show most relevant things on stderr:
	loguru::g_stderr_verbosity = 1;

	// Or just go with what Loguru suggests:
	char log_path[1024];
	loguru::suggest_log_path("~/loguru/", log_path, sizeof(log_path));
	loguru::add_file(log_path, loguru::FileMode::Truncate, loguru::Verbosity_MAX);

	LOG_SCOPE_F(INFO, "Will indent all log messages within this scope.");
	LOG_F(INFO, "I'm hungry for some %.3f!", 3.14159);
	LOG_F(2, "Will only show if verbosity is 2 or higher");
	VLOG_F(get_log_level(), "Use vlog for dynamic log level (integer in the range 0-9, inclusive)");
	LOG_IF_F(ERROR, badness, "Will only show if badness happens");
	auto fp = fopen(filename, "r");
	CHECK_F(fp != nullptr, "Failed to open file '%s'", filename);
	CHECK_GT_F(length, 0); // Will print the value of `length` on failure.
	CHECK_EQ_F(a, b, "You can also supply a custom message, like to print something: %d", a + b);

	// Each function also comes with a version prefixed with D for Debug:
	DCHECK_F(expensive_check(x)); // Only checked #if !NDEBUG
	DLOG_F("Only written in debug-builds");

	// Turn off writing to stderr:
	loguru::g_stderr_verbosity = loguru::Verbosity_OFF;

	// Turn off writing err/warn in red:
	loguru::g_colorlogtostderr = false;

	// Throw exceptions instead of aborting on CHECK fails:
	loguru::set_fatal_handler([](const loguru::Message& message){
		throw std::runtime_error(std::string(message.prefix) + message.message);
	});

	If you prefer logging with streams:

	#define LOGURU_WITH_STREAMS 1
	#include <loguru.hpp>
	...
	LOG_S(INFO) << "Look at my custom object: " << a.cross(b);
	CHECK_EQ_S(pi, 3.14) << "Maybe it is closer to " << M_PI;

	Before including <loguru.hpp> you may optionally want to define the following to 1:

	LOGURU_REDEFINE_ASSERT:
		Redefine "assert" call Loguru version (!NDEBUG only).

	LOGURU_WITH_STREAMS:
		Add support for _S versions for all LOG and CHECK functions:
			LOG_S(INFO) << "My vec3: " << x.cross(y);
			CHECK_EQ_S(a, b) << "I expected a and b to be the same!";
		This is off by default to keep down compilation times.

	LOGURU_REPLACE_GLOG:
		Make Loguru mimic GLOG as close as possible,
		including #defining LOG, CHECK, VLOG_IS_ON etc.
		LOGURU_REPLACE_GLOG implies LOGURU_WITH_STREAMS.

	You can also configure:
	loguru::g_flush_interval_ms:
		If set to zero Loguru will flush on every line (unbuffered mode).
		Else Loguru will flush outputs every g_flush_interval_ms milliseconds (buffered mode).
		The default is g_flush_interval_ms=0, i.e. unbuffered mode.

# Notes:
	* Any arguments to CHECK:s are only evaluated once.
	* Any arguments to LOG functions or LOG_SCOPE are only evaluated iff the verbosity test passes.
	* Any arguments to LOG_IF functions are only evaluated if the test passes.
*/

// Disable all warnings from gcc/clang:
#if defined(__clang__)
	#pragma clang system_header
#elif defined(__GNUC__)
	#pragma GCC system_header
#endif

#ifndef LOGURU_HAS_DECLARED_FORMAT_HEADER
#define LOGURU_HAS_DECLARED_FORMAT_HEADER

// ----------------------------------------------------------------------------

#ifndef LOGURU_SCOPE_TEXT_SIZE
	// Maximum length of text that can be printed by a LOG_SCOPE.
	// This should be long enough to get most things, but short enough not to clutter the stack.
	#define LOGURU_SCOPE_TEXT_SIZE 196
#endif

#ifndef LOGURU_CATCH_SIGABRT
	// Should Loguru catch SIGABRT to print stack trace etc?
	#define LOGURU_CATCH_SIGABRT 1
#endif

#ifndef LOGURU_REDEFINE_ASSERT
	#define LOGURU_REDEFINE_ASSERT 0
#endif

#ifndef LOGURU_WITH_STREAMS
	#define LOGURU_WITH_STREAMS 0
#endif

#ifndef LOGURU_REPLACE_GLOG
	#define LOGURU_REPLACE_GLOG 0
#endif

#if LOGURU_REPLACE_GLOG
	#undef LOGURU_WITH_STREAMS
	#define LOGURU_WITH_STREAMS 1
#endif

#if LOGURU_IMPLEMENTATION
	#undef LOGURU_WITH_STREAMS
	#define LOGURU_WITH_STREAMS 1
#endif

// --------------------------------------------------------------------
// Utility macros

#define LOGURU_CONCATENATE_IMPL(s1, s2) s1 ## s2
#define LOGURU_CONCATENATE(s1, s2) LOGURU_CONCATENATE_IMPL(s1, s2)

#ifdef __COUNTER__
#   define LOGURU_ANONYMOUS_VARIABLE(str) LOGURU_CONCATENATE(str, __COUNTER__)
#else
#   define LOGURU_ANONYMOUS_VARIABLE(str) LOGURU_CONCATENATE(str, __LINE__)
#endif

#if defined(__clang__) || defined(__GNUC__)
	// Helper macro for declaring functions as having similar signature to printf.
	// This allows the compiler to catch format errors at compile-time.
	#define LOGURU_PRINTF_LIKE(fmtarg, firstvararg) __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
	#define LOGURU_FORMAT_STRING_TYPE const char*
#elif defined(_MSC_VER)
	#define LOGURU_PRINTF_LIKE(fmtarg, firstvararg)
	#define LOGURU_FORMAT_STRING_TYPE _In_z_ _Printf_format_string_ const char*
#else
	#define LOGURU_PRINTF_LIKE(fmtarg, firstvararg)
	#define LOGURU_FORMAT_STRING_TYPE const char*
#endif

// Used to mark log_and_abort for the benefit of the static analyzer and optimizer.
#define LOGURU_NORETURN __attribute__((noreturn))

#define LOGURU_PREDICT_FALSE(x) (__builtin_expect(x,     0))
#define LOGURU_PREDICT_TRUE(x)  (__builtin_expect(!!(x), 1))

// --------------------------------------------------------------------

namespace loguru
{
	// Simple RAII ownership of a char*.
	class Text
	{
	public:
		explicit Text(char* owned_str) : _str(owned_str) {}
		~Text();
		Text(Text&& t)
		{
			_str = t._str;
			t._str = nullptr;
		}
		Text(Text& t) = delete;
		Text& operator=(Text& t) = delete;
		void operator=(Text&& t) = delete;

		const char* c_str() const { return _str; }
		bool empty() const { return _str == nullptr || *_str == '\0'; }

		char* release()
		{
			auto result = _str;
			_str = nullptr;
			return result;
		}

	private:
		char* _str;
	};

	// Like printf, but returns the formated text.
	Text textprintf(LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(1, 2);

	// Overloaded for variadic template matching.
	Text textprintf();

	using Verbosity = int;

#undef FATAL
#undef ERROR
#undef WARNING
#undef INFO
#undef MAX

	enum NamedVerbosity : Verbosity
	{
		// You may use Verbosity_OFF on g_stderr_verbosity, but for nothing else!
		Verbosity_OFF     = -9, // Never do LOG_F(OFF)

		// Prefer to use ABORT_F or ABORT_S over LOG_F(FATAL) or LOG_S(FATAL).
		Verbosity_FATAL   = -3,
		Verbosity_ERROR   = -2,
		Verbosity_WARNING = -1,

		// Normal messages. By default written to stderr.
		Verbosity_INFO    =  0,

		// Same as Verbosity_INFO in every way.
		Verbosity_0       =  0,

		// Verbosity levels 1-9 are generally not written to stderr, but are written to file.
		Verbosity_1       = +1,
		Verbosity_2       = +2,
		Verbosity_3       = +3,
		Verbosity_4       = +4,
		Verbosity_5       = +5,
		Verbosity_6       = +6,
		Verbosity_7       = +7,
		Verbosity_8       = +8,
		Verbosity_9       = +9,

		// Don not use higher verbosity levels, as that will make grepping log files harder.
		Verbosity_MAX     = +9,
	};

	struct Message
	{
		// You would generally print a Message by just concating the buffers without spacing.
		// Optionally, ignore preamble and indentation.
		Verbosity   verbosity;   // Already part of preamble
		const char* filename;    // Already part of preamble
		unsigned    line;        // Already part of preamble
		const char* preamble;    // Date, time, uptime, thread, file:line, verbosity.
		const char* indentation; // Just a bunch of spacing.
		const char* prefix;      // Assertion failure info goes here (or "").
		const char* message;     // User message goes here.
	};

	/* Everything with a verbosity equal or greater than g_stderr_verbosity will be
	written to stderr. You can set this in code or via the -v argument.
	Set to logurur::Verbosity_OFF to write nothing to stderr.
	Default is 0, i.e. only log ERROR, WARNING and INFO are written to stderr.
	*/
	extern Verbosity g_stderr_verbosity;
	extern bool      g_colorlogtostderr; // True by default.
	extern unsigned  g_flush_interval_ms; // 0 (unbuffered) by default.

	// May not throw!
	typedef void (*log_handler_t)(void* user_data, const Message& message);
	typedef void (*close_handler_t)(void* user_data);
	typedef void (*flush_handler_t)(void* user_data);

	// May throw if that's how you'd like to handle your errors.
	typedef void (*fatal_handler_t)(const Message& message);

	/*  Should be called from the main thread.
		You don't need to call this, but it's nice if you do.
		This will look for arguments meant for loguru and remove them.
		Arguments meant for loguru are:
			-v n   Set stderr verbosity level. Examples:
					   -v 3        Show verbosity level 3 and lower.
					   -v 0        Only show INFO, WARNING, ERROR, FATAL (default).
					   -v INFO     Only show INFO, WARNING, ERROR, FATAL (default).
					   -v WARNING  Only show WARNING, ERROR, FATAL.
					   -v ERROR    Only show ERROR, FATAL.
					   -v FATAL    Only show FATAL.
					   -v OFF      Turn off logging to stderr.

		Tip: You can set g_stderr_verbosity before calling loguru::init.
		That way you can set the default but have the user override it with the -v flag.
	*/
	void init(int& argc, char* argv[]);

	// What ~ will be replaced with, e.g. "/home/your_user_name/"
	const char* home_dir();

	/* Returns the name of the app as given in argv[0] but wtihout leadin path.
	   That is, if argv[0] is "../foo/app" this will return "app".
	*/
	const char* argv0_filename();

	// Returns the part of the path after the last / or \ (if any).
	const char* filename(const char* path);

	// Writes date and time with millisecond precision, e.g. "20151017_161503.123"
	void write_date_time(char* buff, unsigned buff_size);

	/* Given a prefix of e.g. "~/loguru/" this might return
	   "/home/your_username/loguru/app_name/20151017_161503.123.log"

	   where "app_name" is a sanatized version of argv[0].
	*/
	void suggest_log_path(const char* prefix, char* buff, unsigned buff_size);

	enum FileMode { Truncate, Append };

	/*  Will log to a file at the given path.
		Any logging message with a verbosity lower or equal to
		the given verbosity will be included.
		The function will create all directories in 'path' if needed.
		If path starts with a ~, it will be replaced with loguru::home_dir()
	*/
	bool add_file(const char* path, FileMode mode, Verbosity verbosity);

	/*  Will be called right before abort().
		You can for instance use this to print custom error messages, or throw an exception.
		Feel free to call LOG:ing function from this, but not FATAL ones! */
	void set_fatal_handler(fatal_handler_t handler);

	/*  Will be called on each log messages with a verbosity less or equal to the given one.
		Useful for displaying messages on-screen in a game, for example.
	*/
	void add_callback(const char* id, log_handler_t callback, void* user_data,
					  Verbosity verbosity,
					  close_handler_t on_close = nullptr,
					  flush_handler_t on_flush = nullptr);
	void remove_callback(const char* id);

	// Returns the maximum of g_stderr_verbosity and all file/custom outputs.
	Verbosity current_verbosity_cutoff();

	// Actual logging function. Use the LOG macro instead of calling this directly.
	void log(Verbosity verbosity, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(4, 5);

	// Log without any preamble or indentation.
	void raw_log(Verbosity verbosity, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(4, 5);

	// Helper class for LOG_SCOPE_F
	class LogScopeRAII
	{
	public:
		LogScopeRAII() : _file(nullptr) {} // No logging
		LogScopeRAII(Verbosity verbosity, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(5, 6);
		~LogScopeRAII();

		LogScopeRAII(LogScopeRAII&& other) = default;

	private:
		LogScopeRAII(const LogScopeRAII&) = delete;
		LogScopeRAII& operator=(const LogScopeRAII&) = delete;
		void operator=(LogScopeRAII&&) = delete;

		Verbosity   _verbosity;
		const char* _file; // Set to null if we are disabled due to verbosity
		unsigned    _line;
		bool        _indent_stderr; // Did we?
		long long   _start_time_ns;
		char        _name[LOGURU_SCOPE_TEXT_SIZE];
	};

	// Marked as 'noreturn' for the benefit of the static analyzer and optimizer.
	// stack_trace_skip is the number of extrace stack frames to skip above log_and_abort.
	LOGURU_NORETURN void log_and_abort(int stack_trace_skip, const char* expr, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(5, 6);
	LOGURU_NORETURN void log_and_abort(int stack_trace_skip, const char* expr, const char* file, unsigned line);

	// Flush output to stderr and files.
	// If g_flush_interval_ms is set to non-zero, this will be called automatically this often.
	// If not set, you do not need to call this at al.
	void flush();

	template<class T> inline Text format_value(const T&)                    { return textprintf("N/A");     }
	template<>        inline Text format_value(const char& v)               { return textprintf("%c",   v); }
	template<>        inline Text format_value(const int& v)                { return textprintf("%d",   v); }
	template<>        inline Text format_value(const unsigned int& v)       { return textprintf("%u",   v); }
	template<>        inline Text format_value(const long& v)               { return textprintf("%lu",  v); }
	template<>        inline Text format_value(const unsigned long& v)      { return textprintf("%ld",  v); }
	template<>        inline Text format_value(const long long& v)          { return textprintf("%llu", v); }
	template<>        inline Text format_value(const unsigned long long& v) { return textprintf("%lld", v); }
	template<>        inline Text format_value(const float& v)              { return textprintf("%f",   v); }
	template<>        inline Text format_value(const double& v)             { return textprintf("%f",   v); }

	/* Thread names can be set for the benefit of readable logs.
	   If you do not set the thread name, a hex id will be shown instead.
	   These thread names may or may not be the same as the system thread names,
	   depending on the system.
	   Try to limit the thread name to 15 characters or less. */
	void set_thread_name(const char* name);

	/* Returns the thread name for this thread.
	   On OSX this will return the system thread name (setable from both within and without Loguru).
	   On other systems it will return whatever you set in set_thread_name();
	   If no thread name is set, this will return a hexadecimal thread id.
	   length should be the number of bytes available in the buffer.
	   17 is a good number for length.
	   right_align_hext_id means any hexadecimal thread id will be written to the end of buffer.
	*/
	void get_thread_name(char* buffer, unsigned long long length, bool right_align_hext_id);

	/* Generates a readable stacktrace as a string.
	   'skip' specifies how many stack frames to skip.
	   For instance, the default skip (1) means:
	   don't include the call to loguru::stacktrace in the stack trace. */
	Text stacktrace(int skip = 1);

	/*  Add a string to be replaced with something else in the stack output.

		For instance, instead of having a stack trace look like this:
			0x41f541 some_function(std::basic_ofstream<char, std::char_traits<char> >&)
		You can clean it up with:
			auto verbose_type_name = loguru::demangle(typeid(std::ofstream).name());
			loguru::add_stack_cleanup(verbose_type_name.c_str(); "std::ofstream");
		So the next time you will instead see:
			0x41f541 some_function(std::ofstream&)

		`replace_with_this` must be shorter than `find_this`.
	*/
	void add_stack_cleanup(const char* find_this, const char* replace_with_this);

	// Example: demangle(typeid(std::ofstream).name()) -> "std::basic_ofstream<char, std::char_traits<char> >"
	Text demangle(const char* name);

	// ------------------------------------------------------------------------
	/*
	Not all terminals support colors, but if they do, and g_colorlogtostderr
	is set, Loguru will write them to stderr to make errors in red, etc.

	You also have the option to manually use them, via the function below.

	Note, however, that if you do, the color codes could end up in your logfile!

	This means if you intend to use them functions you should either:
		* Use them on the stderr/stdout directly (bypass Loguru).
		* Don't add file outputs to Loguru.
		* Expect some \e[1m things in your logfile.

	Usage:
		printf("%sRed%sGreen%sBold green%sClear again\n",
			   loguru::terminal_red(), loguru::terminal_green(),
			   loguru::terminal_bold(), loguru::terminal_reset());

	If the terminal at hand does not support colors the above output
	will just not have funky \e[1m things showing.
	*/

	// Do the output terminal support colors?
	bool terminal_has_color();

	// Colors
	const char* terminal_black();
	const char* terminal_red();
	const char* terminal_green();
	const char* terminal_yellow();
	const char* terminal_blue();
	const char* terminal_purple();
	const char* terminal_cyan();
	const char* terminal_light_gray();
	const char* terminal_light_red();
	const char* terminal_white();

	// Formating
	const char* terminal_bold();
	const char* terminal_underline();

	// You should end each line with this!
	const char* terminal_reset();

	// --------------------------------------------------------------------
	// Error context related:

	struct StringStream;

	// Use this in your EcEntryBase::print_value overload.
	void stream_print(StringStream& out_string_stream, const char* text);

	class EcEntryBase
	{
	public:
		EcEntryBase(const char* file, unsigned line, const char* descr);
		~EcEntryBase();
		EcEntryBase(const EcEntryBase&) = delete;
		EcEntryBase(EcEntryBase&&) = delete;
		EcEntryBase& operator=(const EcEntryBase&) = delete;
		EcEntryBase& operator=(EcEntryBase&&) = delete;

		virtual void print_value(StringStream& out_string_stream) const = 0;

		EcEntryBase* previous() const { return _previous; }

	// private:
		const char*  _file;
		unsigned     _line;
		const char*  _descr;
		EcEntryBase* _previous;
	};

	template<typename T>
	class EcEntryData : public EcEntryBase
	{
	public:
		using Printer = Text(*)(T data);

		EcEntryData(const char* file, unsigned line, const char* descr, T data, Printer&& printer)
			: EcEntryBase(file, line, descr), _data(data), _printer(printer) {}

		virtual void print_value(StringStream& out_string_stream) const override
		{
			const auto str = _printer(_data);
			stream_print(out_string_stream, str.c_str());
		}

	private:
		T   _data;
		Printer _printer;
	};

	// template<typename Printer>
	// class EcEntryLambda : public EcEntryBase
	// {
	// public:
	// 	EcEntryLambda(const char* file, unsigned line, const char* descr, Printer&& printer)
	// 		: EcEntryBase(file, line, descr), _printer(std::move(printer)) {}

	// 	virtual void print_value(StringStream& out_string_stream) const override
	// 	{
	// 		const auto str = _printer();
	// 		stream_print(out_string_stream, str.c_str());
	// 	}

	// private:
	// 	Printer _printer;
	// };

	// template<typename Printer>
	// EcEntryLambda<Printer> make_ec_entry_lambda(const char* file, unsigned line, const char* descr, Printer&& printer)
	// {
	// 	return {file, line, descr, std::move(printer)};
	// }

	template <class T>
	struct decay_char_array { using type = T; };

	template <unsigned long long  N>
	struct decay_char_array<const char(&)[N]> { using type = const char*; };

	template <class T>
	struct make_const_ptr { using type = T; };

	template <class T>
	struct make_const_ptr<T*> { using type = const T*; };

	template <class T>
	struct make_ec_type { using type = typename make_const_ptr<typename decay_char_array<T>::type>::type; };


	/* 	A stack trace gives you the names of the function at the point of a crash.
		With ERROR_CONTEXT, you can also get the values of select local variables.
		Usage:

		void process_customers(const std::string& filename)
		{
			ERROR_CONTEXT("Processing file", filename.c_str());
			for (int customer_index : ...)
			{
				ERROR_CONTEXT("Customer index", customer_index);
				...
			}
		}

		The context is in effect during the scope of the ERROR_CONTEXT.
		Use loguru::get_error_context() to get the contents of the active error contexts.

		Example result:

		------------------------------------------------
		[ErrorContext]                main.cpp:416   Processing file:    "customers.json"
		[ErrorContext]                main.cpp:417   Customer index:     42
		------------------------------------------------

		Error contexts are printed automatically on crashes, and only on crashes.
		This makes them much faster than logging the value of a variable.
	*/
	#define ERROR_CONTEXT(descr, data)                                             \
		const loguru::EcEntryData<loguru::make_ec_type<decltype(data)>::type>      \
			LOGURU_ANONYMOUS_VARIABLE(error_context_scope_)(                       \
				__FILE__, __LINE__, descr, data,                                   \
				static_cast<loguru::EcEntryData<loguru::make_ec_type<decltype(data)>::type>::Printer>(loguru::ec_to_text) ) // For better error messages

/*
	#define ERROR_CONTEXT(descr, data)                                 \
		const auto LOGURU_ANONYMOUS_VARIABLE(error_context_scope_)(    \
			loguru::make_ec_entry_lambda(__FILE__, __LINE__, descr,    \
				[=](){ return loguru::ec_to_text(data); }))
*/

	// Get a string describing the current stack of error context. Empty string if there is none.
	Text get_error_context();

	// ------------------------------------------------------------------------

	Text ec_to_text(const char* data);
	Text ec_to_text(char data);
	Text ec_to_text(int data);
	Text ec_to_text(unsigned int data);
	Text ec_to_text(long data);
	Text ec_to_text(unsigned long data);
	Text ec_to_text(long long data);
	Text ec_to_text(unsigned long long data);
	Text ec_to_text(float data);
	Text ec_to_text(double data);
	Text ec_to_text(long double data);

	/*
	You can add ERROR_CONTEXT support for your own types by overloading ec_to_text. Here's how:

	some.hpp:
		namespace loguru {
			Text ec_to_text(MySmallType data)
			Text ec_to_text(const MyBigType* data)
		} // namespace loguru

	some.cpp:
		namespace loguru {
			Text ec_to_text(MySmallType small_value)
			{
				// Called only when needed, i.e. on a crash.
				std::string str = small_value.as_string(); // Format 'small_value' here somehow.
				return Text{strdup(str.c_str())};
			}

			Text ec_to_text(const MyBigType* big_value)
			{
				// Called only when needed, i.e. on a crash.
				std::string str = big_value->as_string(); // Format 'big_value' here somehow.
				return Text{strdup(str.c_str())};
			}
		} // namespace loguru

	Any file that include some.hpp:
		void foo(MySmallType small, const MyBigType& big)
		{
			ERROR_CONTEXT("Small", small); // Copy ´small` by value.
			ERROR_CONTEXT("Big",   &big);  // `big` should not change during this scope!
			....
		}
	*/
} // namespace loguru

// --------------------------------------------------------------------
// Logging macros

// LOG_F(2, "Only logged if verbosity is 2 or higher: %d", some_number);
#define VLOG_F(verbosity, ...)                                                                     \
	(verbosity > loguru::current_verbosity_cutoff()) ? (void)0                                     \
									  : loguru::log(verbosity, __FILE__, __LINE__, __VA_ARGS__)

// LOG_F(INFO, "Foo: %d", some_number);
#define LOG_F(verbosity_name, ...) VLOG_F(loguru::Verbosity_ ## verbosity_name, __VA_ARGS__)

#define VLOG_IF_F(verbosity, cond, ...)                                                            \
	(verbosity > loguru::current_verbosity_cutoff() || (cond) == false)                            \
		? (void)0                                                                                  \
		: loguru::log(verbosity, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_IF_F(verbosity_name, cond, ...)                                                        \
	VLOG_IF_F(loguru::Verbosity_ ## verbosity_name, cond, __VA_ARGS__)

#define VLOG_SCOPE_F(verbosity, ...)                                                               \
	loguru::LogScopeRAII LOGURU_ANONYMOUS_VARIABLE(error_context_RAII_) =                  \
	((verbosity) > loguru::current_verbosity_cutoff()) ? loguru::LogScopeRAII() :                  \
	loguru::LogScopeRAII{verbosity, __FILE__, __LINE__, __VA_ARGS__}

// Raw logging - no preamble, no indentation. Slightly faster than full logging.
#define RAW_VLOG_F(verbosity, ...)                                                                  \
	(verbosity > loguru::current_verbosity_cutoff()) ? (void)0                                      \
									  : loguru::raw_log(verbosity, __FILE__, __LINE__, __VA_ARGS__)

#define RAW_LOG_F(verbosity_name, ...) RAW_VLOG_F(loguru::Verbosity_ ## verbosity_name, __VA_ARGS__)

// Use to book-end a scope. Affects logging on all threads.
#define LOG_SCOPE_F(verbosity_name, ...)                                                           \
	VLOG_SCOPE_F(loguru::Verbosity_ ## verbosity_name, __VA_ARGS__)

#define LOG_SCOPE_FUNCTION(verbosity_name) LOG_SCOPE_F(verbosity_name, __PRETTY_FUNCTION__)

// -----------------------------------------------
// ABORT_F macro. Usage:  ABORT_F("Cause of error: %s", error_str);

// Message is optional
#define ABORT_F(...) loguru::log_and_abort(0, "ABORT: ", __FILE__, __LINE__, __VA_ARGS__)

// --------------------------------------------------------------------
// CHECK_F macros:

#define CHECK_WITH_INFO_F(test, info, ...)                                                         \
	LOGURU_PREDICT_TRUE((test) == true) ? (void)0 : loguru::log_and_abort(0, "CHECK FAILED:  " info "  ", __FILE__,      \
													   __LINE__, ##__VA_ARGS__)

/* Checked at runtime too. Will print error, then call fatal_handler (if any), then 'abort'.
   Note that the test must be boolean.
   CHECK_F(ptr); will not compile, but CHECK_F(ptr != nullptr); will. */
#define CHECK_F(test, ...) CHECK_WITH_INFO_F(test, #test, ##__VA_ARGS__)

#define CHECK_NOTNULL_F(x, ...) CHECK_WITH_INFO_F((x) != nullptr, #x " != nullptr", ##__VA_ARGS__)

#define CHECK_OP_F(expr_left, expr_right, op, ...)                                                 \
	do                                                                                             \
	{                                                                                              \
		auto val_left = expr_left;                                                                 \
		auto val_right = expr_right;                                                               \
		if (! LOGURU_PREDICT_TRUE(val_left op val_right))                                          \
		{                                                                                          \
			auto str_left = loguru::format_value(val_left);                                        \
			auto str_right = loguru::format_value(val_right);                                      \
			auto fail_info = loguru::textprintf("CHECK FAILED:  %s %s %s  (%s %s %s)  ",            \
				#expr_left, #op, #expr_right, str_left.c_str(), #op, str_right.c_str());           \
			auto user_msg = loguru::textprintf(__VA_ARGS__);                                        \
			loguru::log_and_abort(0, fail_info.c_str(), __FILE__, __LINE__,                        \
								  "%s", user_msg.c_str());                                         \
		}                                                                                          \
	} while (false)

#define CHECK_EQ_F(a, b, ...) CHECK_OP_F(a, b, ==, ##__VA_ARGS__)
#define CHECK_NE_F(a, b, ...) CHECK_OP_F(a, b, !=, ##__VA_ARGS__)
#define CHECK_LT_F(a, b, ...) CHECK_OP_F(a, b, < , ##__VA_ARGS__)
#define CHECK_GT_F(a, b, ...) CHECK_OP_F(a, b, > , ##__VA_ARGS__)
#define CHECK_LE_F(a, b, ...) CHECK_OP_F(a, b, <=, ##__VA_ARGS__)
#define CHECK_GE_F(a, b, ...) CHECK_OP_F(a, b, >=, ##__VA_ARGS__)

#ifndef NDEBUG
	// Debug:
	#define DLOG_F(verbosity_name, ...)     LOG_F(verbosity_name, __VA_ARGS__)
	#define DVLOG_F(verbosity, ...)         VLOG_F(verbosity, __VA_ARGS__)
	#define DLOG_IF_F(verbosity_name, ...)  LOG_IF_F(verbosity_name, __VA_ARGS__)
	#define DVLOG_IF_F(verbosity, ...)      VLOG_IF_F(verbosity, __VA_ARGS__)
	#define DRAW_LOG_F(verbosity_name, ...) RAW_LOG_F(verbosity_name, __VA_ARGS__)
	#define DRAW_VLOG_F(verbosity, ...)     RAW_VLOG_F(verbosity, __VA_ARGS__)
	#define DCHECK_F(test, ...)             CHECK_F(test, ##__VA_ARGS__)
	#define DCHECK_NOTNULL_F(x, ...)        CHECK_NOTNULL_F(x, ##__VA_ARGS__)
	#define DCHECK_EQ_F(a, b, ...)          CHECK_EQ_F(a, b, ##__VA_ARGS__)
	#define DCHECK_NE_F(a, b, ...)          CHECK_NE_F(a, b, ##__VA_ARGS__)
	#define DCHECK_LT_F(a, b, ...)          CHECK_LT_F(a, b, ##__VA_ARGS__)
	#define DCHECK_LE_F(a, b, ...)          CHECK_LE_F(a, b, ##__VA_ARGS__)
	#define DCHECK_GT_F(a, b, ...)          CHECK_GT_F(a, b, ##__VA_ARGS__)
	#define DCHECK_GE_F(a, b, ...)          CHECK_GE_F(a, b, ##__VA_ARGS__)
#else // NDEBUG
	// Release:
	#define DLOG_F(verbosity_name, ...)
	#define DVLOG_F(verbosity, ...)
	#define DLOG_IF_F(verbosity_name, ...)
	#define DVLOG_IF_F(verbosity, ...)
	#define DRAW_LOG_F(verbosity_name, ...)
	#define DRAW_VLOG_F(verbosity, ...)
	#define DCHECK_F(test, ...)
	#define DCHECK_NOTNULL_F(x, ...)
	#define DCHECK_EQ_F(a, b, ...)
	#define DCHECK_NE_F(a, b, ...)
	#define DCHECK_LT_F(a, b, ...)
	#define DCHECK_LE_F(a, b, ...)
	#define DCHECK_GT_F(a, b, ...)
	#define DCHECK_GE_F(a, b, ...)
#endif // NDEBUG

#ifdef LOGURU_REDEFINE_ASSERT
	#undef assert
	#ifndef NDEBUG
		// Debug:
		#define assert(test) CHECK_WITH_INFO_F(!!(test), #test) // HACK
	#else
		#define assert(test)
	#endif
#endif // LOGURU_REDEFINE_ASSERT

#endif // LOGURU_HAS_DECLARED_FORMAT_HEADER

// ----------------------------------------------------------------------------
// .dP"Y8 888888 88""Yb 888888    db    8b    d8 .dP"Y8
// `Ybo."   88   88__dP 88__     dPYb   88b  d88 `Ybo."
// o.`Y8b   88   88"Yb  88""    dP__Yb  88YbdP88 o.`Y8b
// 8bodP'   88   88  Yb 888888 dP""""Yb 88 YY 88 8bodP'

#if LOGURU_WITH_STREAMS
#ifndef LOGURU_HAS_DECLARED_STREAMS_HEADER
#define LOGURU_HAS_DECLARED_STREAMS_HEADER

/* This file extends loguru to enable std::stream-style logging, a la Glog.
   It's an optional feature beind the LOGURU_WITH_STREAMS settings
   because including it everywhere will slow down compilation times.
*/

#include <cstdarg>
#include <sstream> // Adds about 38 kLoC on clang.
#include <string>

namespace loguru
{
	// Like sprintf, but returns the formated text.
	std::string strprintf(LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(1, 2);

	// Like vsprintf, but returns the formated text.
	std::string vstrprintf(LOGURU_FORMAT_STRING_TYPE format, va_list) LOGURU_PRINTF_LIKE(1, 0);

	class StreamLogger
	{
	public:
		StreamLogger(Verbosity verbosity, const char* file, unsigned line) : _verbosity(verbosity), _file(file), _line(line) {}
		~StreamLogger() noexcept(false);

		template<typename T>
		StreamLogger& operator<<(const T& t)
		{
			_ss << t;
			return *this;
		}

		// std::endl and other iomanip:s.
		StreamLogger& operator<<(std::ostream&(*f)(std::ostream&))
		{
			f(_ss);
			return *this;
		}

	private:
		Verbosity   _verbosity;
		const char* _file;
		unsigned    _line;
		std::ostringstream _ss;
	};

	class AbortLogger
	{
	public:
		AbortLogger(const char* expr, const char* file, unsigned line) : _expr(expr), _file(file), _line(line) { }
		LOGURU_NORETURN ~AbortLogger() noexcept(false);

		template<typename T>
		AbortLogger& operator<<(const T& t)
		{
			_ss << t;
			return *this;
		}

		// std::endl and other iomanip:s.
		AbortLogger& operator<<(std::ostream&(*f)(std::ostream&))
		{
			f(_ss);
			return *this;
		}

	private:
		const char*        _expr;
		const char*        _file;
		unsigned           _line;
		std::ostringstream _ss;
	};

	class Voidify
	{
	public:
		Voidify() {}
		// This has to be an operator with a precedence lower than << but higher than ?:
		void operator&(const StreamLogger&) { }
		void operator&(const AbortLogger&)  { }
	};

	/*  Helper functions for CHECK_OP_S macro.
		GLOG trick: The (int, int) specialization works around the issue that the compiler
		will not instantiate the template version of the function on values of unnamed enum type. */
	#define DEFINE_CHECK_OP_IMPL(name, op)                                                             \
		template <typename T1, typename T2>                                                            \
		inline std::string* name(const char* expr, const T1& v1, const char* op_str, const T2& v2)     \
		{                                                                                              \
			if (LOGURU_PREDICT_TRUE(v1 op v2)) { return NULL; }                                        \
			std::ostringstream ss;                                                                     \
			ss << "CHECK FAILED:  " << expr << "  (" << v1 << " " << op_str << " " << v2 << ")  ";     \
			return new std::string(ss.str());                                                          \
		}                                                                                              \
		inline std::string* name(const char* expr, int v1, const char* op_str, int v2)                 \
		{                                                                                              \
			return name<int, int>(expr, v1, op_str, v2);                                               \
		}

	DEFINE_CHECK_OP_IMPL(check_EQ_impl, ==)
	DEFINE_CHECK_OP_IMPL(check_NE_impl, !=)
	DEFINE_CHECK_OP_IMPL(check_LE_impl, <=)
	DEFINE_CHECK_OP_IMPL(check_LT_impl, < )
	DEFINE_CHECK_OP_IMPL(check_GE_impl, >=)
	DEFINE_CHECK_OP_IMPL(check_GT_impl, > )
	#undef DEFINE_CHECK_OP_IMPL

	/*  GLOG trick: Function is overloaded for integral types to allow static const integrals
		declared in classes and not defined to be used as arguments to CHECK* macros. */
	template <class T>
	inline const T&           referenceable_value(const T&           t) { return t; }
	inline char               referenceable_value(char               t) { return t; }
	inline unsigned char      referenceable_value(unsigned char      t) { return t; }
	inline signed char        referenceable_value(signed char        t) { return t; }
	inline short              referenceable_value(short              t) { return t; }
	inline unsigned short     referenceable_value(unsigned short     t) { return t; }
	inline int                referenceable_value(int                t) { return t; }
	inline unsigned int       referenceable_value(unsigned int       t) { return t; }
	inline long               referenceable_value(long               t) { return t; }
	inline unsigned long      referenceable_value(unsigned long      t) { return t; }
	inline long long          referenceable_value(long long          t) { return t; }
	inline unsigned long long referenceable_value(unsigned long long t) { return t; }
} // namespace loguru

// -----------------------------------------------
// Logging macros:

// usage:  LOG_STREAM(INFO) << "Foo " << std::setprecision(10) << some_value;
#define VLOG_IF_S(verbosity, cond)                                                                 \
	(verbosity > loguru::current_verbosity_cutoff() || (cond) == false)                                           \
		? (void)0                                                                                  \
		: loguru::Voidify() & loguru::StreamLogger(verbosity, __FILE__, __LINE__)
#define LOG_IF_S(verbosity_name, cond) VLOG_IF_S(loguru::Verbosity_ ## verbosity_name, cond)
#define VLOG_S(verbosity)              VLOG_IF_S(verbosity, true)
#define LOG_S(verbosity_name)          VLOG_S(loguru::Verbosity_ ## verbosity_name)

// -----------------------------------------------
// ABORT_S macro. Usage:  ABORT_S() << "Causo of error: " << details;

#define ABORT_S() loguru::Voidify() & loguru::AbortLogger("ABORT: ", __FILE__, __LINE__)

// -----------------------------------------------
// CHECK_S macros:

#define CHECK_WITH_INFO_S(cond, info)                                                              \
	LOGURU_PREDICT_TRUE((cond) == true)                                                            \
		? (void)0                                                                                  \
		: loguru::Voidify() & loguru::AbortLogger("CHECK FAILED:  " info "  ", __FILE__, __LINE__)

#define CHECK_S(cond) CHECK_WITH_INFO_S(cond, #cond)
#define CHECK_NOTNULL_S(x) CHECK_WITH_INFO_S((x) != nullptr, #x " != nullptr")

#define CHECK_OP_S(function_name, expr1, op, expr2)                                                \
	while (auto error_string = loguru::function_name(#expr1 " " #op " " #expr2,                    \
													 loguru::referenceable_value(expr1), #op,      \
													 loguru::referenceable_value(expr2)))          \
		loguru::AbortLogger(error_string->c_str(), __FILE__, __LINE__)

#define CHECK_EQ_S(expr1, expr2) CHECK_OP_S(check_EQ_impl, expr1, ==, expr2)
#define CHECK_NE_S(expr1, expr2) CHECK_OP_S(check_NE_impl, expr1, !=, expr2)
#define CHECK_LE_S(expr1, expr2) CHECK_OP_S(check_LE_impl, expr1, <=, expr2)
#define CHECK_LT_S(expr1, expr2) CHECK_OP_S(check_LT_impl, expr1, < , expr2)
#define CHECK_GE_S(expr1, expr2) CHECK_OP_S(check_GE_impl, expr1, >=, expr2)
#define CHECK_GT_S(expr1, expr2) CHECK_OP_S(check_GT_impl, expr1, > , expr2)

#ifndef NDEBUG
	// Debug:
	#define DVLOG_IF_S(verbosity, cond)     VLOG_IF_S(verbosity, cond)
	#define DLOG_IF_S(verbosity_name, cond) LOG_IF_S(verbosity_name, cond)
	#define DVLOG_S(verbosity)              VLOG_S(verbosity)
	#define DLOG_S(verbosity_name)          LOG_S(verbosity_name)
	#define DCHECK_S(cond)                  CHECK_S(cond)
	#define DCHECK_NOTNULL_S(x)             CHECK_NOTNULL_S(x)
	#define DCHECK_EQ_S(a, b)               CHECK_EQ_S(a, b)
	#define DCHECK_NE_S(a, b)               CHECK_NE_S(a, b)
	#define DCHECK_LT_S(a, b)               CHECK_LT_S(a, b)
	#define DCHECK_LE_S(a, b)               CHECK_LE_S(a, b)
	#define DCHECK_GT_S(a, b)               CHECK_GT_S(a, b)
	#define DCHECK_GE_S(a, b)               CHECK_GE_S(a, b)
#else // NDEBUG
	// Release:
	#define DVLOG_IF_S(verbosity, cond)                                                     \
		(true || verbosity > loguru::current_verbosity_cutoff() || (cond) == false)                        \
			? (void)0                                                                       \
			: loguru::Voidify() & loguru::StreamLogger(verbosity, __FILE__, __LINE__)

	#define DLOG_IF_S(verbosity_name, cond) DVLOG_IF_S(loguru::Verbosity_ ## verbosity_name, cond)
	#define DVLOG_S(verbosity)              DVLOG_IF_S(verbosity, true)
	#define DLOG_S(verbosity_name)          DVLOG_S(loguru::Verbosity_ ## verbosity_name)
	#define DCHECK_S(cond)                  CHECK_S(true || (cond))
	#define DCHECK_NOTNULL_S(x)             CHECK_S(true || (x) != nullptr)
	#define DCHECK_EQ_S(a, b)               CHECK_S(true || (a) == (b))
	#define DCHECK_NE_S(a, b)               CHECK_S(true || (a) != (b))
	#define DCHECK_LT_S(a, b)               CHECK_S(true || (a) <  (b))
	#define DCHECK_LE_S(a, b)               CHECK_S(true || (a) <= (b))
	#define DCHECK_GT_S(a, b)               CHECK_S(true || (a) >  (b))
	#define DCHECK_GE_S(a, b)               CHECK_S(true || (a) >= (b))
#endif // NDEBUG

#if LOGURU_REPLACE_GLOG
	#undef LOG
	#undef VLOG
	#undef LOG_IF
	#undef VLOG_IF
	#undef CHECK
	#undef CHECK_NOTNULL
	#undef CHECK_EQ
	#undef CHECK_NE
	#undef CHECK_LT
	#undef CHECK_LE
	#undef CHECK_GT
	#undef CHECK_GE
	#undef DLOG
	#undef DVLOG
	#undef DLOG_IF
	#undef DVLOG_IF
	#undef DCHECK
	#undef DCHECK_NOTNULL
	#undef DCHECK_EQ
	#undef DCHECK_NE
	#undef DCHECK_LT
	#undef DCHECK_LE
	#undef DCHECK_GT
	#undef DCHECK_GE
	#undef VLOG_IS_ON

	#define LOG            LOG_S
	#define VLOG           VLOG_S
	#define LOG_IF         LOG_IF_S
	#define VLOG_IF        VLOG_IF_S
	#define CHECK(cond)    CHECK_S(!!(cond))
	#define CHECK_NOTNULL  CHECK_NOTNULL_S
	#define CHECK_EQ       CHECK_EQ_S
	#define CHECK_NE       CHECK_NE_S
	#define CHECK_LT       CHECK_LT_S
	#define CHECK_LE       CHECK_LE_S
	#define CHECK_GT       CHECK_GT_S
	#define CHECK_GE       CHECK_GE_S
	#define DLOG           DLOG_S
	#define DVLOG          DVLOG_S
	#define DLOG_IF        DLOG_IF_S
	#define DVLOG_IF       DVLOG_IF_S
	#define DCHECK         DCHECK_S
	#define DCHECK_NOTNULL DCHECK_NOTNULL_S
	#define DCHECK_EQ      DCHECK_EQ_S
	#define DCHECK_NE      DCHECK_NE_S
	#define DCHECK_LT      DCHECK_LT_S
	#define DCHECK_LE      DCHECK_LE_S
	#define DCHECK_GT      DCHECK_GT_S
	#define DCHECK_GE      DCHECK_GE_S
	#define VLOG_IS_ON(verbosity) ((verbosity) <= loguru::current_verbosity_cutoff())

#endif // LOGURU_REPLACE_GLOG

#endif // LOGURU_WITH_STREAMS

#endif // LOGURU_HAS_DECLARED_STREAMS_HEADER

// ----------------------------------------------------------------------------
// 88 8b    d8 88""Yb 88     888888 8b    d8 888888 88b 88 888888    db    888888 88  dP"Yb  88b 88
// 88 88b  d88 88__dP 88     88__   88b  d88 88__   88Yb88   88     dPYb     88   88 dP   Yb 88Yb88
// 88 88YbdP88 88"""  88  .o 88""   88YbdP88 88""   88 Y88   88    dP__Yb    88   88 Yb   dP 88 Y88
// 88 88 YY 88 88     88ood8 888888 88 YY 88 888888 88  Y8   88   dP""""Yb   88   88  YbodP  88  Y8


/* In one of your .cpp files you need to do the following:
#define LOGURU_IMPLEMENTATION
#include <loguru.hpp>

This will define all the Loguru functions so that the linker may find them.
*/

#if defined(LOGURU_IMPLEMENTATION) && !defined(LOGURU_HAS_BEEN_IMPLEMENTED)
#define LOGURU_HAS_BEEN_IMPLEMENTED

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#ifdef _MSC_VER
	#include <direct.h>
#else
	#include <signal.h>
	#include <sys/stat.h> // mkdir
	#include <unistd.h>   // STDERR_FILENO
#endif

#ifdef __APPLE__
	#include "TargetConditionals.h"
#endif

// TODO: use defined(_POSIX_VERSION) for some of these things?

#ifdef _WIN32
	#define LOGURU_PTHREADS    0
	#define LOGURU_STACKTRACES 0
#else
	#define LOGURU_PTHREADS    1
	#define LOGURU_STACKTRACES 1
#endif

#if LOGURU_STACKTRACES
	#include <cxxabi.h>    // for __cxa_demangle
	#include <dlfcn.h>     // for dladdr
	#include <execinfo.h>  // for backtrace
#endif // LOGURU_STACKTRACES

#if LOGURU_PTHREADS
	#include <pthread.h>

	#ifdef __linux__
		/* On Linux, the default thread name is the same as the name of the binary.
		   Additionally, all new threads inherit the name of the thread it got forked from.
		   For this reason, Loguru use the pthread Thread Local Storage
		   for storing thread names on Linux. */
		#define LOGURU_PTLS_NAMES 1
	#endif
#endif

#ifndef LOGURU_PTLS_NAMES
   #define LOGURU_PTLS_NAMES 0
#endif

namespace loguru
{
	using namespace std::chrono;

	struct Callback
	{
		std::string     id;
		log_handler_t   callback;
		void*           user_data;
		Verbosity       verbosity; // Does not change!
		close_handler_t close;
		flush_handler_t flush;
		unsigned        indentation;
	};

	using CallbackVec = std::vector<Callback>;

	using StringPair     = std::pair<std::string, std::string>;
	using StringPairList = std::vector<StringPair>;

	const auto SCOPE_TIME_PRECISION = 3; // 3=ms, 6≈us, 9=ns

	const auto s_start_time = system_clock::now();

	Verbosity g_stderr_verbosity  = Verbosity_0;
	bool      g_colorlogtostderr  = true;
	unsigned  g_flush_interval_ms = 0;

	static std::recursive_mutex  s_mutex;
	static Verbosity             s_max_out_verbosity = Verbosity_OFF;
	static std::string           s_argv0_filename;
	static std::string           s_file_arguments;
	static CallbackVec           s_callbacks;
	static fatal_handler_t       s_fatal_handler   = nullptr;
	static StringPairList        s_user_stack_cleanups;
	static bool                  s_strip_file_path = true;
	static std::atomic<unsigned> s_stderr_indentation { 0 };

	// For periodic flushing:
	static std::thread* s_flush_thread   = nullptr;
	static bool         s_needs_flushing = false;

	static const bool s_terminal_has_color = [](){
		#ifdef _MSC_VER
			return false;
		#else
			if (const char* term = getenv("TERM")) {
				return 0 == strcmp(term, "cygwin")
					|| 0 == strcmp(term, "linux")
					|| 0 == strcmp(term, "screen")
					|| 0 == strcmp(term, "xterm")
					|| 0 == strcmp(term, "xterm-256color")
					|| 0 == strcmp(term, "xterm-color");
			} else {
				return false;
			}
		#endif
	}();

	const auto THREAD_NAME_WIDTH = 16;
	const auto PREAMBLE_EXPLAIN  = "date       time         ( uptime  ) [ thread name/id ]                   file:line     v| ";

	#if LOGURU_PTLS_NAMES
		static pthread_once_t s_pthread_key_once = PTHREAD_ONCE_INIT;
		static pthread_key_t  s_pthread_key_name;

		void make_pthread_key_name()
		{
			(void)pthread_key_create(&s_pthread_key_name, free);
		}
	#endif

	// ------------------------------------------------------------------------------
	// Colors

	bool terminal_has_color() { return s_terminal_has_color; }

	// Colors
	const char* terminal_black()      { return s_terminal_has_color ? "\e[30m" : ""; }
	const char* terminal_red()        { return s_terminal_has_color ? "\e[31m" : ""; }
	const char* terminal_green()      { return s_terminal_has_color ? "\e[32m" : ""; }
	const char* terminal_yellow()     { return s_terminal_has_color ? "\e[33m" : ""; }
	const char* terminal_blue()       { return s_terminal_has_color ? "\e[34m" : ""; }
	const char* terminal_purple()     { return s_terminal_has_color ? "\e[35m" : ""; }
	const char* terminal_cyan()       { return s_terminal_has_color ? "\e[36m" : ""; }
	const char* terminal_light_gray() { return s_terminal_has_color ? "\e[37m" : ""; }
	const char* terminal_white()      { return s_terminal_has_color ? "\e[37m" : ""; }
	const char* terminal_light_red()  { return s_terminal_has_color ? "\e[91m" : ""; }
	const char* terminal_dim()        { return s_terminal_has_color ? "\e[2m"  : ""; }

	// Formating
	const char* terminal_bold()       { return s_terminal_has_color ? "\e[1m" : ""; }
	const char* terminal_underline()  { return s_terminal_has_color ? "\e[4m" : ""; }

	// You should end each line with this!
	const char* terminal_reset()      { return s_terminal_has_color ? "\e[0m" : ""; }

	// ------------------------------------------------------------------------------

	void file_log(void* user_data, const Message& message)
	{
		FILE* file = reinterpret_cast<FILE*>(user_data);
		fprintf(file, "%s%s%s%s\n",
			message.preamble, message.indentation, message.prefix, message.message);
		if (g_flush_interval_ms == 0) {
			fflush(file);
		}
	}

	void file_close(void* user_data)
	{
		FILE* file = reinterpret_cast<FILE*>(user_data);
		fclose(file);
	}

	void file_flush(void* user_data)
	{
		FILE* file = reinterpret_cast<FILE*>(user_data);
		fflush(file);
	}

	// ------------------------------------------------------------------------------

	// Helpers:

	Text::~Text() { free(_str); }

	LOGURU_PRINTF_LIKE(1, 0)
	static Text vtextprintf(const char* format, va_list vlist)
	{
#ifdef _MSC_VER
		int bytes_needed = vsnprintf(nullptr, 0, format, vlist);
		CHECK_F(bytes_needed >= 0, "Bad string format: '%s'", format);
		char* buff = (char*)malloc(bytes_needed + 1);
		vsnprintf(buff, bytes_needed, format, vlist);
		return buff;
#else
		char* buff = nullptr;
		int result = vasprintf(&buff, format, vlist);
		CHECK_F(result >= 0, "Bad string format: '%s'", format);
		return Text(buff);
#endif
	}

	Text textprintf(const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		auto result = vtextprintf(format, vlist);
		va_end(vlist);
		return result;
	}

	// Overloaded for variadic template matching.
	Text textprintf()
	{
		return Text(static_cast<char*>(calloc(1, 1)));
	}

	const char* indentation(unsigned depth)
	{
		static const char buff[] =
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   "
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   "
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   "
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   "
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   ";
		static const size_t INDENTATION_WIDTH = 4;
		static const size_t NUM_INDENTATIONS = (sizeof(buff) - 1) / INDENTATION_WIDTH;
		depth = std::min<unsigned>(depth, NUM_INDENTATIONS);
		return buff + INDENTATION_WIDTH * (NUM_INDENTATIONS - depth);
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
					CHECK_LT_F(arg_it, argc, "Missing verbosiy level after -v");
					value_str = argv[arg_it];
					out_argc -= 1;
				}
				if (*value_str == '=') { value_str += 1; }

				if (strcmp(value_str, "OFF") == 0) {
					g_stderr_verbosity = Verbosity_OFF;
				} else if (strcmp(value_str, "INFO") == 0) {
					g_stderr_verbosity = Verbosity_INFO;
				} else if (strcmp(value_str, "WARNING") == 0) {
					g_stderr_verbosity = Verbosity_WARNING;
				} else if (strcmp(value_str, "ERROR") == 0) {
					g_stderr_verbosity = Verbosity_ERROR;
				} else if (strcmp(value_str, "FATAL") == 0) {
					g_stderr_verbosity = Verbosity_FATAL;
				} else {
					char* end = 0;
					g_stderr_verbosity = static_cast<int>(strtol(value_str, &end, 10));
					CHECK_F(end && *end == '\0',
						"Invalid verbosity. Expected integer, INFO, WARNING, ERROR or OFF, got '%s'", value_str);
				}
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

	// Returns the part of the path after the last / or \ (if any).
	const char* filename(const char* path)
	{
		for (auto ptr = path; *ptr; ++ptr) {
			if (*ptr == '/' || *ptr == '\\') {
				path = ptr + 1;
			}
		}
		return path;
	}

	// ------------------------------------------------------------------------------

	static void on_atexit()
	{
		LOG_F(INFO, "atexit");
		flush();
	}

	void install_signal_handlers();

	void write_hex_digit(std::string& out, unsigned num)
	{
		DCHECK_LT_F(num, 16u);
		if (num < 10u) { out.push_back(char('0' + num)); }
		else { out.push_back(char('A' + num - 10)); }
	}

	void write_hex_byte(std::string& out, uint8_t n)
	{
		write_hex_digit(out, n >> 4u);
		write_hex_digit(out, n & 0x0f);
	}

	void escape(std::string& out, const std::string& str)
	{
		for (char c : str) {
			/**/ if (c == '\a') { out += "\\a";  }
			else if (c == '\b') { out += "\\b";  }
			else if (c == '\f') { out += "\\f";  }
			else if (c == '\n') { out += "\\n";  }
			else if (c == '\r') { out += "\\r";  }
			else if (c == '\t') { out += "\\t";  }
			else if (c == '\v') { out += "\\v";  }
			else if (c == '\\') { out += "\\\\"; }
			else if (c == '\'') { out += "\\\'"; }
			else if (c == '\"') { out += "\\\""; }
			else if (c == ' ')  { out += "\\ ";  }
			else if (0 <= c && c < 0x20) { // ASCI control character:
			// else if (c < 0x20 || c != (c & 127)) { // ASCII control character or UTF-8:
				out += "\\x";
				write_hex_byte(out, static_cast<uint8_t>(c));
			} else { out += c; }
		}
	}

	void init(int& argc, char* argv[])
	{
		s_argv0_filename = filename(argv[0]);

		s_file_arguments = "";
		for (int i = 0; i < argc; ++i) {
			escape(s_file_arguments, argv[i]);
			if (i + 1 < argc) {
				s_file_arguments += " ";
			}
		}

		parse_args(argc, argv);

		#if LOGURU_PTLS_NAMES
			set_thread_name("main thread");
		#elif LOGURU_PTHREADS
			char old_thread_name[16] = {0};
			auto this_thread = pthread_self();
			pthread_getname_np(this_thread, old_thread_name, sizeof(old_thread_name));
			if (old_thread_name[0] == 0) {
				#ifdef __APPLE__
					pthread_setname_np("main thread");
				#else
					pthread_setname_np(this_thread, "main thread");
				#endif
			}
		#endif // LOGURU_PTHREADS

		if (g_stderr_verbosity >= Verbosity_INFO) {
			if (g_colorlogtostderr && s_terminal_has_color) {
				fprintf(stderr, "%s%s%s\n", terminal_reset(), terminal_dim(), PREAMBLE_EXPLAIN);
			} else {
				fprintf(stderr, "%s\n", PREAMBLE_EXPLAIN);
			}
			fflush(stderr);
		}
		LOG_F(INFO, "arguments: %s", s_file_arguments.c_str());
		LOG_F(INFO, "stderr verbosity: %d", g_stderr_verbosity);
		LOG_F(INFO, "-----------------------------------");

		install_signal_handlers();

		atexit(on_atexit);
	}

	void write_date_time(char* buff, size_t buff_size)
	{
		auto now = system_clock::now();
		long long ms_since_epoch = duration_cast<milliseconds>(now.time_since_epoch()).count();
		time_t sec_since_epoch = time_t(ms_since_epoch / 1000);
		tm time_info;
		localtime_r(&sec_since_epoch, &time_info);
		snprintf(buff, buff_size, "%04d%02d%02d_%02d%02d%02d.%03lld",
			1900 + time_info.tm_year, 1 + time_info.tm_mon, time_info.tm_mday,
			time_info.tm_hour, time_info.tm_min, time_info.tm_sec, ms_since_epoch % 1000);
	}

	const char* argv0_filename() { return s_argv0_filename.c_str(); }

	const char* home_dir()
	{
		#ifdef _WIN32
			auto user_profile = getenv("USERPROFILE");
			CHECK_F(user_profile != nullptr, "Missing USERPROFILE");
			return user_profile;
		#else // _WIN32
			auto home = getenv("HOME");
			CHECK_F(home != nullptr, "Missing HOME");
			return home;
		#endif // _WIN32
	}

	void suggest_log_path(const char* prefix, char* buff, unsigned buff_size)
	{
		if (prefix[0] == '~') {
			snprintf(buff, buff_size - 1, "%s%s", home_dir(), prefix + 1);
		} else {
			snprintf(buff, buff_size - 1, "%s", prefix);
		}

		// Check for terminating /
		size_t n = strlen(buff);
		if (n != 0) {
			if (buff[n - 1] != '/') {
				CHECK_F(n + 2 < buff_size, "Filename buffer too small");
				buff[n] = '/';
				buff[n + 1] = '\0';
			}
		}

		strncat(buff, s_argv0_filename.c_str(), buff_size - strlen(buff) - 1);
		strncat(buff, "/",                      buff_size - strlen(buff) - 1);
		write_date_time(buff + strlen(buff),    buff_size - strlen(buff));
		strncat(buff, ".log",                   buff_size - strlen(buff) - 1);
	}

	bool mkpath(const char* file_path_const)
	{
		CHECK_F(file_path_const && *file_path_const);
		char* file_path = strdup(file_path_const);
		for (char* p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
			*p = '\0';

	#ifdef _MSC_VER
			if (_mkdir(file_path) == -1) {
	#else
			if (mkdir(file_path, 0755) == -1) {
	#endif
				if (errno != EEXIST) {
					LOG_F(ERROR, "Failed to create directory '%s'", file_path);
					LOG_IF_F(ERROR, errno == EACCES,       "EACCES");
					LOG_IF_F(ERROR, errno == ENAMETOOLONG, "ENAMETOOLONG");
					LOG_IF_F(ERROR, errno == ENOENT,       "ENOENT");
					LOG_IF_F(ERROR, errno == ENOTDIR,      "ENOTDIR");
					LOG_IF_F(ERROR, errno == ELOOP,        "ELOOP");

					*p = '/';
					free(file_path);
					return false;
				}
			}
			*p = '/';
		}
		free(file_path);
		return true;
	}

	bool add_file(const char* path_in, FileMode mode, Verbosity verbosity)
	{
		char path[1024];
		if (path_in[0] == '~') {
			snprintf(path, sizeof(path) - 1, "%s%s", home_dir(), path_in + 1);
		} else {
			snprintf(path, sizeof(path) - 1, "%s", path_in);
		}

		if (!mkpath(path)) {
			LOG_F(ERROR, "Failed to create directories to '%s'", path);
		}

		const char* mode_str = (mode == FileMode::Truncate ? "w" : "a");
		auto file = fopen(path, mode_str);
		if (!file) {
			LOG_F(ERROR, "Failed to open '%s'", path);
			return false;
		}
		add_callback(path, file_log, file, verbosity, file_close, file_flush);

		if (mode == FileMode::Append) {
			fprintf(file, "\n\n\n\n\n");
		}

		fprintf(file, "arguments: %s\n", s_file_arguments.c_str());
		fprintf(file, "File verbosity level: %d\n", verbosity);
		fprintf(file, "%s\n", PREAMBLE_EXPLAIN);
		fflush(file);

		LOG_F(INFO, "Logging to '%s', mode: '%s', verbosity: %d", path, mode_str, verbosity);
		return true;
	}

	// Will be called right before abort().
	void set_fatal_handler(fatal_handler_t handler)
	{
		s_fatal_handler = handler;
	}

	void add_stack_cleanup(const char* find_this, const char* replace_with_this)
	{
		if (strlen(find_this) <= strlen(replace_with_this))
		{
			LOG_F(WARNING, "add_stack_cleanup: the replacement should be shorter than the pattern!");
			return;
		}

		s_user_stack_cleanups.push_back(StringPair(find_this, replace_with_this));
	}

	static void on_callback_change()
	{
		s_max_out_verbosity = Verbosity_OFF;
		for (const auto& callback : s_callbacks)
		{
			s_max_out_verbosity = std::max(s_max_out_verbosity, callback.verbosity);
		}
	}

	void add_callback(const char* id, log_handler_t callback, void* user_data,
					  Verbosity verbosity, close_handler_t on_close, flush_handler_t on_flush)
	{
		std::lock_guard<std::recursive_mutex> lock(s_mutex);
		s_callbacks.push_back(Callback{id, callback, user_data, verbosity, on_close, on_flush, 0});
		on_callback_change();
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
		on_callback_change();
	}

	// Returns the maximum of g_stderr_verbosity and all file/custom outputs.
	Verbosity current_verbosity_cutoff()
	{
		return g_stderr_verbosity > s_max_out_verbosity ?
			   g_stderr_verbosity : s_max_out_verbosity;
	}

	void set_thread_name(const char* name)
	{
		#if LOGURU_PTLS_NAMES
			(void)pthread_once(&s_pthread_key_once, make_pthread_key_name);
			(void)pthread_setspecific(s_pthread_key_name, strdup(name));

		#elif LOGURU_PTHREADS
			#ifdef __APPLE__
				pthread_setname_np(name);
			#else
				pthread_setname_np(pthread_self(), name);
			#endif
		#else // LOGURU_PTHREADS
			(void)name;
		#endif // LOGURU_PTHREADS
	}

#if LOGURU_PTLS_NAMES
	const char* get_thread_name_ptls()
	{
		(void)pthread_once(&s_pthread_key_once, make_pthread_key_name);
		return static_cast<const char*>(pthread_getspecific(s_pthread_key_name));
	}
#endif // LOGURU_PTLS_NAMES

	void get_thread_name(char* buffer, unsigned long long length, bool right_align_hext_id)
	{
		CHECK_NE_F(length, 0u, "Zero length buffer in get_thread_name");
		CHECK_NOTNULL_F(buffer, "nullptr in get_thread_name");
#if LOGURU_PTHREADS
		auto thread = pthread_self();
		#if LOGURU_PTLS_NAMES
			if (const char* name = get_thread_name_ptls()) {
				snprintf(buffer, length, "%s", name);
			} else {
				buffer[0] = 0;
			}
		#else
			pthread_getname_np(thread, buffer, length);
		#endif

		if (buffer[0] == 0) {
			#ifdef __APPLE__
				uint64_t thread_id;
				pthread_threadid_np(thread, &thread_id);
			#else
				uint64_t thread_id = thread;
			#endif
			if (right_align_hext_id) {
				snprintf(buffer, length, "%*X", length - 1, static_cast<unsigned>(thread_id));
			} else {
				snprintf(buffer, length, "%X", static_cast<unsigned>(thread_id));
			}
		}
#else // LOGURU_PTHREADS
		buffer[0] = 0;
#endif // LOGURU_PTHREADS

	}

	// ------------------------------------------------------------------------
	// Stack traces

#if LOGURU_STACKTRACES
	Text demangle(const char* name)
	{
		int status = -1;
		char* demangled = abi::__cxa_demangle(name, 0, 0, &status);
		Text result{status == 0 ? demangled : strdup(name)};
		return result;
	}

	template <class T>
	std::string type_name() {
		auto demangled = demangle(typeid(T).name());
		return demangled.c_str();
	}

	static const StringPairList REPLACE_LIST = {
		{ type_name<std::string>(),    "std::string"    },
		{ type_name<std::wstring>(),   "std::wstring"   },
		{ type_name<std::u16string>(), "std::u16string" },
		{ type_name<std::u32string>(), "std::u32string" },
		{ "std::__1::",                "std::"          },
		{ "__thiscall ",               ""               },
		{ "__cdecl ",                  ""               },
	};

	void do_replacements(const StringPairList& replacements, std::string& str)
	{
		for (auto&& p : replacements) {
			if (p.first.size() <= p.second.size()) {
				// On gcc, "type_name<std::string>()" is "std::string"
				continue;
			}

			size_t it;
			while ((it=str.find(p.first)) != std::string::npos) {
				str.replace(it, p.first.size(), p.second);
			}
		}
	}

	std::string prettify_stacktrace(const std::string& input)
	{
		std::string output = input;

		do_replacements(s_user_stack_cleanups, output);
		do_replacements(REPLACE_LIST, output);

		try {
			std::regex std_allocator_re(R"(,\s*std::allocator<[^<>]+>)");
			output = std::regex_replace(output, std_allocator_re, std::string(""));

			std::regex template_spaces_re(R"(<\s*([^<> ]+)\s*>)");
			output = std::regex_replace(output, template_spaces_re, std::string("<$1>"));
		} catch (std::regex_error&) {
			// Probably old GCC.
		}

		return output;
	}

	std::string stacktrace_as_stdstring(int skip)
	{
		// From https://gist.github.com/fmela/591333
		void* callstack[128];
		const auto max_frames = sizeof(callstack) / sizeof(callstack[0]);
		int num_frames = backtrace(callstack, max_frames);
		char** symbols = backtrace_symbols(callstack, num_frames);

		std::string result;
		// Print stack traces so the most relevant ones are written last
		// Rationale: http://yellerapp.com/posts/2015-01-22-upside-down-stacktraces.html
		for (int i = num_frames - 1; i >= skip; --i) {
			char buf[1024];
			Dl_info info;
			if (dladdr(callstack[i], &info) && info.dli_sname) {
				char* demangled = NULL;
				int status = -1;
				if (info.dli_sname[0] == '_') {
					demangled = abi::__cxa_demangle(info.dli_sname, 0, 0, &status);
				}
				snprintf(buf, sizeof(buf), "%-3d %*p %s + %zd\n",
						 i - skip, int(2 + sizeof(void*) * 2), callstack[i],
						 status == 0 ? demangled :
						 info.dli_sname == 0 ? symbols[i] : info.dli_sname,
						 static_cast<char*>(callstack[i]) - static_cast<char*>(info.dli_saddr));
				free(demangled);
			} else {
				snprintf(buf, sizeof(buf), "%-3d %*p %s\n",
						 i - skip, int(2 + sizeof(void*) * 2), callstack[i], symbols[i]);
			}
			result += buf;
		}
		free(symbols);

		if (num_frames == max_frames) {
			result = "[truncated]\n" + result;
		}

		if (!result.empty() && result[result.size() - 1] == '\n') {
			result.resize(result.size() - 1);
		}

		return prettify_stacktrace(result);
	}

#else // LOGURU_STACKTRACES
	Text demangle(const char* name)
	{
		return name;
	}

	std::string stacktrace_as_stdstring(int)
	{
		#warning "Loguru: No stacktraces available on this platform"
		return "";
	}

#endif // LOGURU_STACKTRACES

	Text stacktrace(int skip)
	{
		auto str = stacktrace_as_stdstring(skip + 1);
		return Text(strdup(str.c_str()));
	}

	// ------------------------------------------------------------------------

	static void print_preamble(char* out_buff, size_t out_buff_size, Verbosity verbosity, const char* file, unsigned line)
	{
		auto now = system_clock::now();
		long long ms_since_epoch = duration_cast<milliseconds>(now.time_since_epoch()).count();
		time_t sec_since_epoch = time_t(ms_since_epoch / 1000);
		tm time_info;
		localtime_r(&sec_since_epoch, &time_info);

		auto uptime_ms = duration_cast<milliseconds>(now - s_start_time).count();
		auto uptime_sec = uptime_ms / 1000.0;

		char thread_name[THREAD_NAME_WIDTH + 1] = {0};
		get_thread_name(thread_name, THREAD_NAME_WIDTH + 1, true);

		if (s_strip_file_path) {
			file = filename(file);
		}

		char level_buff[6];
		if (verbosity <= Verbosity_FATAL) {
			snprintf(level_buff, sizeof(level_buff) - 1, "FATL");
		} else if (verbosity == Verbosity_ERROR) {
			snprintf(level_buff, sizeof(level_buff) - 1, "ERR");
		} else if (verbosity == Verbosity_WARNING) {
			snprintf(level_buff, sizeof(level_buff) - 1, "WARN");
		} else {
			snprintf(level_buff, sizeof(level_buff) - 1, "% 4d", verbosity);
		}

		snprintf(out_buff, out_buff_size, "%04d-%02d-%02d %02d:%02d:%02d.%03lld (%8.3fs) [%-*s]%23s:%-5u %4s| ",
			1900 + time_info.tm_year, 1 + time_info.tm_mon, time_info.tm_mday,
			time_info.tm_hour, time_info.tm_min, time_info.tm_sec, ms_since_epoch % 1000,
			uptime_sec,
			THREAD_NAME_WIDTH, thread_name,
			file, line, level_buff);
	}

	// stack_trace_skip is just if verbosity == FATAL.
	static void log_message(int stack_trace_skip, Message& message, bool with_indentation, bool abort_if_fatal)
	{
		const auto verbosity = message.verbosity;
		std::lock_guard<std::recursive_mutex> lock(s_mutex);

		if (message.verbosity == Verbosity_FATAL) {
			auto st = loguru::stacktrace(stack_trace_skip + 2);
			if (!st.empty()) {
				RAW_LOG_F(ERROR, "Stack trace:\n%s", st.c_str());
			}

			auto ec = loguru::get_error_context();
			if (!ec.empty()) {
				RAW_LOG_F(ERROR, "%s", ec.c_str());
			}
		}

		if (with_indentation) {
			message.indentation = indentation(s_stderr_indentation);
		}

		if (verbosity <= g_stderr_verbosity) {
			if (g_colorlogtostderr && s_terminal_has_color) {
				if (verbosity > Verbosity_WARNING) {
					fprintf(stderr, "%s%s%s%s%s%s%s%s%s\n",
						terminal_reset(),
						terminal_dim(),
						message.preamble,
						message.indentation,
						terminal_reset(),
						verbosity == Verbosity_INFO ? terminal_bold() : terminal_light_gray(),
						message.prefix,
						message.message,
						terminal_reset());
				} else {
					fprintf(stderr, "%s%s%s%s%s%s%s%s\n",
						terminal_reset(),
						terminal_bold(),
						verbosity == Verbosity_WARNING ? terminal_red() : terminal_light_red(),
						message.preamble,
						message.indentation,
						message.prefix,
						message.message,
						terminal_reset());
				}
			} else {
				fprintf(stderr, "%s%s%s%s\n",
					message.preamble, message.indentation, message.prefix, message.message);
			}

			if (g_flush_interval_ms == 0) {
				fflush(stderr);
			} else {
				s_needs_flushing = true;
			}
		}

		for (auto& p : s_callbacks) {
			if (verbosity <= p.verbosity) {
				if (with_indentation) {
					message.indentation = indentation(p.indentation);
				}
				p.callback(p.user_data, message);
				if (g_flush_interval_ms > 0) {
					s_needs_flushing = true;
				}
			}
		}

		if (g_flush_interval_ms > 0 && !s_flush_thread) {
			s_flush_thread = new std::thread([](){
				for (;;) {
					if (s_needs_flushing) {
						flush();
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(g_flush_interval_ms));
				}
			});
		}

		if (message.verbosity == Verbosity_FATAL) {
			flush();

			if (s_fatal_handler) {
				s_fatal_handler(message);
				flush();
			}

			if (abort_if_fatal) {
#if LOGURU_CATCH_SIGABRT && !defined(_WIN32)
				// Make sure we don't catch our own abort:
				signal(SIGABRT, SIG_DFL);
#endif
				abort();
			}
		}
	}

	// stack_trace_skip is just if verbosity == FATAL.
	void log_to_everywhere(int stack_trace_skip, Verbosity verbosity,
						   const char* file, unsigned line,
						   const char* prefix, const char* buff)
	{
		char preamble_buff[128];
		print_preamble(preamble_buff, sizeof(preamble_buff), verbosity, file, line);
		auto message = Message{verbosity, file, line, preamble_buff, "", prefix, buff};
		log_message(stack_trace_skip + 1, message, true, true);
	}

	void log(Verbosity verbosity, const char* file, unsigned line, const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		auto buff = vtextprintf(format, vlist);
		log_to_everywhere(1, verbosity, file, line, "", buff.c_str());
		va_end(vlist);
	}

	void raw_log(Verbosity verbosity, const char* file, unsigned line, const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		auto buff = vtextprintf(format, vlist);
		auto message = Message{verbosity, file, line, "", "", "", buff.c_str()};
		log_message(1, message, false, true);
		va_end(vlist);
	}

	void flush()
	{
		std::lock_guard<std::recursive_mutex> lock(s_mutex);
		fflush(stderr);
		for (const auto& callback : s_callbacks)
		{
			callback.flush(callback.user_data);
		}
		s_needs_flushing = false;
	}

	LogScopeRAII::LogScopeRAII(Verbosity verbosity, const char* file, unsigned line, const char* format, ...)
		: _verbosity(verbosity), _file(file), _line(line)
	{
		if (verbosity <= current_verbosity_cutoff()) {
			std::lock_guard<std::recursive_mutex> lock(s_mutex);
			_indent_stderr = (verbosity <= g_stderr_verbosity);
			_start_time_ns = now_ns();
			va_list vlist;
			va_start(vlist, format);
			vsnprintf(_name, sizeof(_name), format, vlist);
			log_to_everywhere(1, _verbosity, file, line, "{ ", _name);
			va_end(vlist);

			if (_indent_stderr) {
				++s_stderr_indentation;
			}

			for (auto& p : s_callbacks) {
				if (verbosity <= p.verbosity) {
					++p.indentation;
				}
			}
		} else {
			_file = nullptr;
		}
	}

	LogScopeRAII::~LogScopeRAII()
	{
		if (_file) {
			std::lock_guard<std::recursive_mutex> lock(s_mutex);
			if (_indent_stderr && s_stderr_indentation > 0) {
				--s_stderr_indentation;
			}
			for (auto& p : s_callbacks) {
				// Note: Callback indentation cannot change!
				if (_verbosity <= p.verbosity) {
					// in unlikely case this callback is new
					if (p.indentation > 0) {
						--p.indentation;
					}
				}
			}
			auto duration_sec = (now_ns() - _start_time_ns) / 1e9;
			log(_verbosity, _file, _line, "} %.*f s: %s", SCOPE_TIME_PRECISION, duration_sec, _name);
		}
	}

	void log_and_abort(int stack_trace_skip, const char* expr, const char* file, unsigned line, const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		auto buff = vtextprintf(format, vlist);
		log_to_everywhere(stack_trace_skip + 1, Verbosity_FATAL, file, line, expr, buff.c_str());
		va_end(vlist);
		abort(); // log_to_everywhere already does this, but this makes the analyzer happy.
	}

	void log_and_abort(int stack_trace_skip, const char* expr, const char* file, unsigned line)
	{
		log_and_abort(stack_trace_skip + 1, expr, file, line, " ");
	}

	// ----------------------------------------------------------------------------
	// Streams:

	std::string vstrprintf(const char* format, va_list vlist)
	{
		auto text = vtextprintf(format, vlist);
		std::string result = text.c_str();
		return result;
	}

	std::string strprintf(const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		auto result = vstrprintf(format, vlist);
		va_end(vlist);
		return result;
	}

	#if LOGURU_WITH_STREAMS

	StreamLogger::~StreamLogger() noexcept(false)
	{
		auto message = _ss.str();
		log(_verbosity, _file, _line, "%s", message.c_str());
	}

	AbortLogger::~AbortLogger() noexcept(false)
	{
		auto message = _ss.str();
		loguru::log_and_abort(1, _expr, _file, _line, "%s", message.c_str());
	}

	#endif // LOGURU_WITH_STREAMS

	// ----------------------------------------------------------------------------
	// 888888 88""Yb 88""Yb  dP"Yb  88""Yb      dP""b8  dP"Yb  88b 88 888888 888888 Yb  dP 888888
	// 88__   88__dP 88__dP dP   Yb 88__dP     dP   `" dP   Yb 88Yb88   88   88__    YbdP    88
	// 88""   88"Yb  88"Yb  Yb   dP 88"Yb      Yb      Yb   dP 88 Y88   88   88""    dPYb    88
	// 888888 88  Yb 88  Yb  YbodP  88  Yb      YboodP  YbodP  88  Y8   88   888888 dP  Yb   88
	// ----------------------------------------------------------------------------

	struct StringStream
	{
		std::string str;
	};

	// Use this in your EcPrinter implementations.
	void stream_print(StringStream& out_string_stream, const char* text)
	{
		out_string_stream.str += text;
	}

	// ----------------------------------------------------------------------------

	using ECPtr = EcEntryBase*;

#if defined(_WIN32) || (defined(TARGET_OS_MAC) && TARGET_OS_MAC)
	#ifdef __APPLE__
		#define LOGURU_THREAD_LOCAL __thread
	#else
		#define LOGURU_THREAD_LOCAL thread_local
	#endif
	static LOGURU_THREAD_LOCAL ECPtr thread_ec_ptr = nullptr;

	ECPtr& get_thread_ec_head_ref()
	{
		return thread_ec_ptr;
	}
#else // !thread_local
	static pthread_once_t s_ec_pthread_once = PTHREAD_ONCE_INIT;
	static pthread_key_t  s_ec_pthread_key;

	void free_ec_head_ref(void* io_error_context)
	{
		delete reinterpret_cast<ECPtr*>(io_error_context);
	}

	void ec_make_pthread_key()
	{
		(void)pthread_key_create(&s_ec_pthread_key, free_ec_head_ref);
	}

	ECPtr& get_thread_ec_head_ref()
	{
		(void)pthread_once(&s_ec_pthread_once, ec_make_pthread_key);
		auto ec = reinterpret_cast<ECPtr*>(pthread_getspecific(s_ec_pthread_key));
		if (ec == nullptr) {
			ec = new ECPtr(nullptr);
			(void)pthread_setspecific(s_ec_pthread_key, ec);
		}
		return *ec;
	}
#endif // !thread_local

	// ----------------------------------------------------------------------------

	Text get_error_context()
	{
		const EcEntryBase* ec_head = get_thread_ec_head_ref();

		std::vector<const EcEntryBase*> stack;
		while (ec_head) {
			stack.push_back(ec_head);
			ec_head = ec_head->_previous;
		}
		std::reverse(stack.begin(), stack.end());

		StringStream result;
		if (!stack.empty()) {
			result.str += "------------------------------------------------\n";
			for (auto entry : stack) {
				const auto description = std::string(entry->_descr) + ":";
				auto prefix = textprintf("[ErrorContext] %23s:%-5u %-20s ",
					filename(entry->_file), entry->_line, description.c_str());
				result.str += prefix.c_str();
				entry->print_value(result);
				result.str += "\n";
			}
			result.str += "------------------------------------------------";
		}
		return Text(strdup(result.str.c_str()));
	}

	EcEntryBase::EcEntryBase(const char* file, unsigned line, const char* descr)
		: _file(file), _line(line), _descr(descr)
	{
		EcEntryBase*& ec_head = get_thread_ec_head_ref();
		_previous = ec_head;
		ec_head = this;
	}

	EcEntryBase::~EcEntryBase()
	{
		get_thread_ec_head_ref() = _previous;
	}

	// ------------------------------------------------------------------------

	Text ec_to_text(const char* value)
	{
		// Add quotes around the string to make it obvious where it begin and ends.
		// This is great for detecting erroneous leading or trailing spaces in e.g. an identifier.
		auto str = "\"" + std::string(value) + "\"";
		return Text{strdup(str.c_str())};
	}

	Text ec_to_text(char c)
	{
		// Add quotes around the character to make it obvious where it begin and ends.
		std::string str = "'";

		auto write_hex_digit = [&](unsigned num)
		{
			if (num < 10u) { str += char('0' + num); }
			else           { str += char('a' + num - 10); }
		};

		auto write_hex_16 = [&](uint16_t n)
		{
			write_hex_digit((n >> 12u) & 0x0f);
			write_hex_digit((n >>  8u) & 0x0f);
			write_hex_digit((n >>  4u) & 0x0f);
			write_hex_digit((n >>  0u) & 0x0f);
		};

		if      (c == '\\') { str += "\\\\"; }
		else if (c == '\"') { str += "\\\""; }
		else if (c == '\'') { str += "\\\'"; }
		else if (c == '\0') { str += "\\0";  }
		else if (c == '\b') { str += "\\b";  }
		else if (c == '\f') { str += "\\f";  }
		else if (c == '\n') { str += "\\n";  }
		else if (c == '\r') { str += "\\r";  }
		else if (c == '\t') { str += "\\t";  }
		else if (0 <= c && c < 0x20) {
			str += "\\u";
			write_hex_16(static_cast<uint16_t>(c));
		} else { str += c; }

		str += "'";

		return Text{strdup(str.c_str())};
	}

	#define DEFINE_EC(Type)                        \
		Text ec_to_text(Type value)                \
		{                                          \
			auto str = std::to_string(value);      \
			return Text{strdup(str.c_str())};      \
		}

	DEFINE_EC(int)
	DEFINE_EC(unsigned int)
	DEFINE_EC(long)
	DEFINE_EC(unsigned long)
	DEFINE_EC(long long)
	DEFINE_EC(unsigned long long)
	DEFINE_EC(float)
	DEFINE_EC(double)
	DEFINE_EC(long double)

	// ----------------------------------------------------------------------------

} // namespace loguru

// ----------------------------------------------------------------------------
// .dP"Y8 88  dP""b8 88b 88    db    88     .dP"Y8
// `Ybo." 88 dP   `" 88Yb88   dPYb   88     `Ybo."
// o.`Y8b 88 Yb  "88 88 Y88  dP__Yb  88  .o o.`Y8b
// 8bodP' 88  YboodP 88  Y8 dP""""Yb 88ood8 8bodP'
// ----------------------------------------------------------------------------

#ifdef _WIN32
namespace loguru {
	void install_signal_handlers()
	{
		#warning "No signal handlers on Win32"
	}
} // namespace loguru

#else // _WIN32

namespace loguru
{
	struct Signal
	{
		int         number;
		const char* name;
	};
	const Signal ALL_SIGNALS[] = {
#if LOGURU_CATCH_SIGABRT
		{ SIGABRT, "SIGABRT" },
#endif
		{ SIGBUS,  "SIGBUS"  },
		{ SIGFPE,  "SIGFPE"  },
		{ SIGILL,  "SIGILL"  },
		{ SIGINT,  "SIGINT"  },
		{ SIGSEGV, "SIGSEGV" },
		{ SIGTERM, "SIGTERM" },
	};

	void write_to_stderr(const char* data, size_t size)
	{
		auto result = write(STDERR_FILENO, data, size);
		(void)result; // Ignore errors.
	}

	void write_to_stderr(const char* data)
	{
		write_to_stderr(data, strlen(data));
	}

	void call_default_signal_handler(int signal_number)
	{
		struct sigaction sig_action;
		memset(&sig_action, 0, sizeof(sig_action));
		sigemptyset(&sig_action.sa_mask);
		sig_action.sa_handler = SIG_DFL;
		sigaction(signal_number, &sig_action, NULL);
		kill(getpid(), signal_number);
	}

	void signal_handler(int signal_number, siginfo_t*, void*)
	{
		const char* signal_name = "UNKNOWN SIGNAL";

		for (const auto& s : ALL_SIGNALS) {
			if (s.number == signal_number) {
				signal_name = s.name;
				break;
			}
		}

		// --------------------------------------------------------------------
		/* There are few things that are safe to do in a signal handler,
		   but writing to stderr is one of them.
		   So we first print out what happened to stderr so we're sure that gets out,
		   then we do the unsafe things, like logging the stack trace.
		   In practice, I've never seen any problems with doing these unsafe things in the signal handler.
		*/

		if (g_colorlogtostderr && s_terminal_has_color) {
			write_to_stderr(terminal_reset());
			write_to_stderr(terminal_bold());
			write_to_stderr(terminal_light_red());
		}
		write_to_stderr("\n");
		write_to_stderr("Loguru caught a signal: ");
		write_to_stderr(signal_name);
		write_to_stderr("\n");
		if (g_colorlogtostderr && s_terminal_has_color) {
			write_to_stderr(terminal_reset());
		}

		// --------------------------------------------------------------------

		flush();
		char preamble_buff[128];
		print_preamble(preamble_buff, sizeof(preamble_buff), Verbosity_FATAL, "", 0);
		auto message = Message{Verbosity_FATAL, "", 0, preamble_buff, "", "Signal: ", signal_name};
		try {
			log_message(1, message, false, false);
		} catch (...) {
			// This can happed due to s_fatal_handler.
			write_to_stderr("Exception caught and ignored by Loguru signal handler.\n");
		}
		flush();

		call_default_signal_handler(signal_number);
	}

	void install_signal_handlers()
	{
		struct sigaction sig_action;
		memset(&sig_action, 0, sizeof(sig_action));
		sigemptyset(&sig_action.sa_mask);
		sig_action.sa_flags |= SA_SIGINFO;
		sig_action.sa_sigaction = &signal_handler;
		for (const auto& s : ALL_SIGNALS) {
			CHECK_F(sigaction(s.number, &sig_action, NULL) != -1,
				"Failed to install handler for %s", s.name);
		}
	}
} // namespace loguru

#endif // _WIN32

#endif // LOGURU_IMPLEMENTATION
