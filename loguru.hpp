/*
Loguru logging library for C++, by Emil Ernerfeldt.
www.github.com/emilk/loguru
License: use, abuse, give credit if you like.

* Version 0.1 - 2015-03-22 - Works great on Mac.
* Version 0.2 - 2015-09-17 - Removed the only dependency.

# Usage:
LOG(INFO, "I'm hungry for some %.3f!", 3.14159);
CHECK(0 < x, "Expected positive integer, got %d", x);
Use LOG_SCOPE for temporal grouping and to measure durations.
Calling loguru::init is optional, but useful to timestamp the start of the log.

# TODO:
* Set file output with cutoff for verbosity.
* Test on Windows.
* Color print to terminal.
* Log on atexit?
* Make drop-in replacement for GLOG?
* Print arguments of failed CHECK_GE etc (stream only?)
*/

#pragma once

// Helper macro for declaring functions as having similar signature to printf.
// This allows the compiler to catch format errors at compile-time.
#define LOGURU_PRINTF_LIKE(fmtarg, firstvararg) \
		__attribute__((__format__ (__printf__, fmtarg, firstvararg)))

// Used to mark on_assertion_failed for the benefit of the static analyzer and optimizer.
#define LOGURU_NORETURN __attribute__((noreturn))

namespace loguru
{
	using Verbosity = int;

	enum NamedVerbosity : Verbosity
	{
		// Value is the verbosity level one must pass.
		// Negative numbers go to stderr and cannot be skipped.
		FATAL   = -3,
		ERROR   = -2,
		WARNING = -1,
		INFO    =  0, // Normal messages.
		SPAM    = +1  // Goes to file, but not to screen.
	};

	extern Verbosity g_verbosity; // Anything higher than this is ignored.

	// Musn't throw!
	typedef void (*log_handler_t)(void* user_data, Verbosity verbosity, const char* text);
	typedef void (*fatal_handler_t)();

	/*  Should be called from the main thread.
		Musn't be called, but it's nice if you do.
	    This will look for arguments meant for loguru and remove them.
	    Arguments meant for loguru are:
			-v n   Set verbosity level */
	void init(int& argc, char* argv[]);

	/*  Will be called right before abort().
	    This can be used to print a callstack.
	    Feel free to call LOG:ing function from this, but not FATAL ones! */
	void set_fatal_handler(fatal_handler_t handler);

	/* Will be called on each log messages that passes verbocity tests etc.
	   Useful for displaying messages on-screen in a game, for eample. */
	void add_callback(const char* id, log_handler_t callback, void* user_data);
	void remove_callback(const char* id);

	// Actual logging function. Use the LOG macro instead of calling this directly.
	void log(Verbosity verbosity, const char* file, unsigned line, const char* format, ...) LOGURU_PRINTF_LIKE(4, 5);

	// Helper class for LOG_SCOPE
	class LogScopeRAII
	{
	public:
		LogScopeRAII(Verbosity verbosity, const char* file, unsigned line, const char* format, ...) LOGURU_PRINTF_LIKE(5, 6);
		~LogScopeRAII();

	private:
		LogScopeRAII(LogScopeRAII&);
		LogScopeRAII(LogScopeRAII&&);
		LogScopeRAII& operator=(LogScopeRAII&);
		LogScopeRAII& operator=(LogScopeRAII&&);

		Verbosity   _verbosity;
		const char* _file; // Set to null if we are disabled due to verbosity
		unsigned    _line;
		long long   _start_time_ns;
		char        _name[64];
	};

	// Marked as 'noreturn' for the benefit of the static analyzer and optimizer.
	void on_assertion_failed(const char* expr, const char* file, unsigned line, const char* format, ...) LOGURU_PRINTF_LIKE(4, 5) LOGURU_NORETURN;
	void on_assertion_failed(const char* expr, const char* file, unsigned line) LOGURU_NORETURN;

	// Convenience:
	void set_thread_name(const char* name);
} // namespace loguru

// --------------------------------------------------------------------
// Macros!

// LOG(2, "Only logged if verbosity is 2 or higher: %d", some_number);
#define VLOG(verbosity, ...) if (verbosity > loguru::g_verbosity) {} else do { loguru::log(verbosity, __FILE__, __LINE__, __VA_ARGS__); } while (false)

// LOG(INFO, "Foo: %d", some_number);
#define LOG(verbosity_name, ...) VLOG((loguru::Verbosity)loguru::NamedVerbosity::verbosity_name, __VA_ARGS__)

// Used for giving a unique name to a RAII-object
#define LOGURU_GIVE_UNIQUE_NAME(arg1, arg2) LOGURU_STRING_JOIN(arg1, arg2)
#define LOGURU_STRING_JOIN(arg1, arg2) arg1 ## arg2

#define VLOG_SCOPE(verbosity, ...) loguru::LogScopeRAII LOGURU_GIVE_UNIQUE_NAME(error_context_RAII_, __LINE__){verbosity, __FILE__, __LINE__, __VA_ARGS__}

// Use to book-end a scope. Affects logging on all threads.
#define LOG_SCOPE(verbosity_name, ...) VLOG_SCOPE((loguru::Verbosity)loguru::NamedVerbosity::verbosity_name, __VA_ARGS__)
#define LOG_SCOPE_FUNCTION(verbosity_name) LOG_SCOPE(verbosity_name, __PRETTY_FUNCTION__)

#define CHECK_WITH_INFO(test, info, ...) ((test) == true) ? (void)0 :  \
    loguru::on_assertion_failed("CHECK FAILED:  " info "  ",           \
                                __FILE__, __LINE__, ##__VA_ARGS__)

/* Checked at runtime too. Will print error, then call abort_handler (if any), then 'abort'.
   Note that the test must be boolean.
   CHECK(ptr); will not compile, but CHECK(ptr != nullptr); will. */
#define CHECK(test, ...) CHECK_WITH_INFO(test, #test, ##__VA_ARGS__)

#define CHECK_NOTNULL(x, ...) CHECK_WITH_INFO((x) != nullptr, #x " != nullptr", ##__VA_ARGS__)

#define CHECK_EQ(a, b, ...) CHECK_WITH_INFO((a) == (b), #a " == " #b, ##__VA_ARGS__)
#define CHECK_NE(a, b, ...) CHECK_WITH_INFO((a) != (b), #a " != " #b, ##__VA_ARGS__)
#define CHECK_LT(a, b, ...) CHECK_WITH_INFO((a) <  (b), #a " < "  #b, ##__VA_ARGS__)
#define CHECK_GT(a, b, ...) CHECK_WITH_INFO((a) >  (b), #a " > "  #b, ##__VA_ARGS__)
#define CHECK_LE(a, b, ...) CHECK_WITH_INFO((a) <= (b), #a " <= " #b, ##__VA_ARGS__)
#define CHECK_GE(a, b, ...) CHECK_WITH_INFO((a) >= (b), #a " >= " #b, ##__VA_ARGS__)

#ifndef NDEBUG
#  define DLOG(verbosity, ...)  LOG(verbosity, __VA_ARGS__)
#  define DCHECK(test, ...)     CHECK(test, ##__VA_ARGS__)
#  define DCHECK_NOTNULL(x)     CHECK_NOTNULL(x, ##__VA_ARGS__)
#  define DCHECK_EQ(a, b, ...)  CHECK_EQ(a, b, ##__VA_ARGS__)
#  define DCHECK_NE(a, b, ...)  CHECK_NE(a, b, ##__VA_ARGS__)
#  define DCHECK_LT(a, b, ...)  CHECK_LT(a, b, ##__VA_ARGS__)
#  define DCHECK_LE(a, b, ...)  CHECK_LE(a, b, ##__VA_ARGS__)
#  define DCHECK_GT(a, b, ...)  CHECK_GT(a, b, ##__VA_ARGS__)
#  define DCHECK_GE(a, b, ...)  CHECK_GE(a, b, ##__VA_ARGS__)
#else
#  define DLOG(verbosity, ...)
#  define DCHECK(test, ...)
#  define DCHECK_NOTNULL(x, ...)
#  define DCHECK_EQ(a, b, ...)
#  define DCHECK_NE(a, b, ...)
#  define DCHECK_LT(a, b, ...)
#  define DCHECK_LE(a, b, ...)
#  define DCHECK_GT(a, b, ...)
#  define DCHECK_GE(a, b, ...)
#endif

#ifndef NDEBUG
	#undef assert
	#define assert(test) CHECK_WITH_INFO(!!(test), #test) // HACK
#else
   #define assert(test)
#endif
