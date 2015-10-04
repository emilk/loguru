/*
Loguru logging library for C++, by Emil Ernerfeldt.
www.github.com/emilk/loguru
License: use, abuse, give credit if you like.

* Version 0.1 - 2015-03-22 - Works great on Mac.
* Version 0.2 - 2015-09-17 - Removed the only dependency.
* Version 0.3 - 2015-10-02 - Drop-in replacement for most of GLOG

# Usage:
LOG_F(INFO, "I'm hungry for some %.3f!", 3.14159);
CHECK_GT_F(x, 0);
Use LOG_SCOPE_F for temporal grouping and to measure durations.
Calling loguru::init is optional, but useful to timestamp the start of the log.

Before including <loguru.hpp> you can optionally define:

LOGURU_REDEFINE_ASSERT:
	Redefine "assert" call loguru version (!NDEBUG).
LOGURU_WITH_STREAMS:
	add support for _S versions for logging using std::ostreams.
LOGURU_REPLACE_GLOG:
	Make Loguru act like GLOG as close as possible.
	Imlies LOGURU_WITH_STREAMS.
*/

#pragma once

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

// GCC can be told that a certain branch is not likely to be taken (for
// instance, a CHECK failure), and use that information in static analysis.
// Giving it this information can help it optimize for the common case in
// the absence of better information (ie. -fprofile-arcs).
//
#define LOGURU_PREDICT_BRANCH_NOT_TAKEN(x) (__builtin_expect(x, 0))
#define LOGURU_PREDICT_FALSE(x) (__builtin_expect(x, 0))
#define LOGURU_PREDICT_TRUE(x) (__builtin_expect(!!(x), 1))

namespace loguru
{
	// free after use
	char* strprintf(LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(1, 2);

	// Overloaded for variadic template matching.
	char* strprintf();

	using Verbosity = int;

	enum NamedVerbosity : Verbosity
	{
		// Value is the verbosity level one must pass.
		// Negative numbers go to stderr and cannot be skipped.
		FATAL   = -3, // Prefer to use ABORT_F over LOG_F(FATAL)
		ERROR   = -2,
		WARNING = -1,
		INFO    =  0, // Normal messages.
		SPAM    = +1, // Goes to file, but not to screen.
		MAX     = +9
	};

	struct Message
	{
		// You would generally print a Message by just concating the buffers without spacing.
		// Optionally, ignore preamble and indentation.
		Verbosity   verbosity;
		const char* preamble;    // Date, time, uptime, thread, file:line, verbosity.
		const char* indentation; // Just a bunch of spacing.
		const char* prefix;      // Assertion failure info goes here (or "").
		const char* message;     // User message goes here.
	};

	extern Verbosity g_verbosity;        // Anything greater than this is ignored.
	extern bool      g_alsologtostderr;  // Ignored right now. Only used for LOGURU_REPLACE_GLOG.
	extern bool      g_colorlogtostderr; // Ignored right now. Only used for LOGURU_REPLACE_GLOG.

	// May not throw!
	typedef void (*log_handler_t)(void* user_data, const Message& message);
	typedef void (*close_handler_t)(void* user_data);
	typedef void (*fatal_handler_t)();

	/*  Should be called from the main thread.
	    You don't need to call this, but it's nice if you do.
	    This will look for arguments meant for loguru and remove them.
	    Arguments meant for loguru are:
	        -v n   Set verbosity level */
	void init(int& argc, char* argv[]);

	enum FileMode { Truncate, Append };

	/*  Will log to a file at the given path.
	    `verbosity` is the cutoff, but this is applied *after* g_verbosity.
	*/
	bool add_file(const char* dir, FileMode mode, Verbosity verbosity = NamedVerbosity::MAX);

	/*  Will be called right before abort().
	    This can be used to print a callstack.
	    Feel free to call LOG:ing function from this, but not FATAL ones! */
	void set_fatal_handler(fatal_handler_t handler);

	/*  Will be called on each log messages that passes verbocity tests etc.
	    Useful for displaying messages on-screen in a game, for eample.
	    `verbosity` is the cutoff, but this is applied *after* g_verbosity.
	*/
	void add_callback(const char* id, log_handler_t callback, void* user_data,
	                  Verbosity verbosity = NamedVerbosity::MAX,
	                  close_handler_t on_close = nullptr);
	void remove_callback(const char* id);

	// Actual logging function. Use the LOG macro instead of calling this directly.
	void log(Verbosity verbosity, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(4, 5);

	// Helper class for LOG_SCOPE_F
	class LogScopeRAII
	{
	public:
		LogScopeRAII(Verbosity verbosity, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(5, 6);
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
	void log_and_abort(const char* expr, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(4, 5) LOGURU_NORETURN;
	void log_and_abort(const char* expr, const char* file, unsigned line) LOGURU_NORETURN;

	// Free after use!
	template<class T> inline char* format_value(const T&)                    { return strprintf("N/A");     }
	template<>        inline char* format_value(const char& v)               { return strprintf("%c",   v); }
	template<>        inline char* format_value(const int& v)                { return strprintf("%d",   v); }
	template<>        inline char* format_value(const unsigned& v)           { return strprintf("%u",   v); }
	template<>        inline char* format_value(const unsigned long& v)      { return strprintf("%lu",  v); }
	template<>        inline char* format_value(const unsigned long long& v) { return strprintf("%llu", v); }
	template<>        inline char* format_value(const float& v)              { return strprintf("%f",   v); }
	template<>        inline char* format_value(const double& v)             { return strprintf("%f",   v); }

	// Convenience:
	void set_thread_name(const char* name);
} // namespace loguru

// --------------------------------------------------------------------
// Utitlity macros

// Used for giving a unique name to a RAII-object
#define LOGURU_GIVE_UNIQUE_NAME(arg1, arg2) LOGURU_STRING_JOIN(arg1, arg2)
#define LOGURU_STRING_JOIN(arg1, arg2) arg1 ## arg2

// --------------------------------------------------------------------
// Logging macros

// LOG_F(2, "Only logged if verbosity is 2 or higher: %d", some_number);
#define VLOG_F(verbosity, ...) if (verbosity > loguru::g_verbosity) {} else do { loguru::log(verbosity, __FILE__, __LINE__, __VA_ARGS__); } while (false)

// LOG_F(INFO, "Foo: %d", some_number);
#define LOG_F(verbosity_name, ...) VLOG_F((loguru::Verbosity)loguru::NamedVerbosity::verbosity_name, __VA_ARGS__)

#define VLOG_SCOPE_F(verbosity, ...) loguru::LogScopeRAII LOGURU_GIVE_UNIQUE_NAME(error_context_RAII_, __LINE__){verbosity, __FILE__, __LINE__, __VA_ARGS__}

// Use to book-end a scope. Affects logging on all threads.
#define LOG_SCOPE_F(verbosity_name, ...) VLOG_SCOPE_F((loguru::Verbosity)loguru::NamedVerbosity::verbosity_name, __VA_ARGS__)
#define LOG_SCOPE_FUNCTION_F(verbosity_name) LOG_SCOPE_F(verbosity_name, __PRETTY_FUNCTION__)

// --------------------------------------------------------------------
// Check/Abort macros

// Message is optional
#define ABORT_F(...) loguru::log_and_abort("ABORT: ", __FILE__, __LINE__, __VA_ARGS__)

#define CHECK_WITH_INFO_F(test, info, ...)                                                         \
    ((test) == true) ? (void)0 : loguru::log_and_abort("CHECK FAILED:  " info "  ", __FILE__,      \
                                                       __LINE__, ##__VA_ARGS__)

/* Checked at runtime too. Will print error, then call abort_handler (if any), then 'abort'.
   Note that the test must be boolean.
   CHECK_F(ptr); will not compile, but CHECK_F(ptr != nullptr); will. */
#define CHECK_F(test, ...) CHECK_WITH_INFO_F(test, #test, ##__VA_ARGS__)

#define CHECK_NOTNULL_F(x, ...) CHECK_WITH_INFO_F((x) != nullptr, #x " != nullptr", ##__VA_ARGS__)

#define CHECK_OP_F(expr_left, expr_right, op, ...)                                                 \
	do                                                                                             \
	{                                                                                              \
		auto val_left = expr_left;                                                                 \
		auto val_right = expr_right;                                                               \
		if (!(val_left op val_right))                                                              \
		{                                                                                          \
			char* str_left = loguru::format_value(val_left);                                       \
			char* str_right = loguru::format_value(val_right);                                     \
			char* fail_info = loguru::strprintf("CHECK FAILED:  %s %s %s  (%s %s %s)  ",           \
				#expr_left, #op, #expr_right, str_left, #op, str_right);                           \
			char* user_msg = loguru::strprintf(__VA_ARGS__);                                       \
			loguru::log_and_abort(fail_info, __FILE__, __LINE__, "%s", user_msg);                  \
			/* free(user_msg);  // no need - we never get here anyway! */                          \
			/* free(fail_info); // no need - we never get here anyway! */                          \
			/* free(str_right); // no need - we never get here anyway! */                          \
			/* free(str_left);  // no need - we never get here anyway! */                          \
		}                                                                                          \
	} while (false)

#define CHECK_EQ_F(a, b, ...) CHECK_OP_F(a, b, ==, ##__VA_ARGS__)
#define CHECK_NE_F(a, b, ...) CHECK_OP_F(a, b, !=, ##__VA_ARGS__)
#define CHECK_LT_F(a, b, ...) CHECK_OP_F(a, b, < , ##__VA_ARGS__)
#define CHECK_GT_F(a, b, ...) CHECK_OP_F(a, b, > , ##__VA_ARGS__)
#define CHECK_LE_F(a, b, ...) CHECK_OP_F(a, b, <=, ##__VA_ARGS__)
#define CHECK_GE_F(a, b, ...) CHECK_OP_F(a, b, >=, ##__VA_ARGS__)

#ifndef NDEBUG
	#define DLOG_F(verbosity, ...)     LOG_F(verbosity, __VA_ARGS__)
	#define DVLOG_F(verbosity, ...)    VLOG_F(verbosity, __VA_ARGS__)
	#define DLOG_IF_F(verbosity, ...)  LOG_IF_F(verbosity, __VA_ARGS__)
	#define DVLOG_IF_F(verbosity, ...) VLOG_IF_F(verbosity, __VA_ARGS__)
	#define DCHECK_F(test, ...)        CHECK_F(test, ##__VA_ARGS__)
	#define DCHECK_NOTNULL_F(x, ...)   CHECK_NOTNULL_F(x, ##__VA_ARGS__)
	#define DCHECK_EQ_F(a, b, ...)     CHECK_EQ_F(a, b, ##__VA_ARGS__)
	#define DCHECK_NE_F(a, b, ...)     CHECK_NE_F(a, b, ##__VA_ARGS__)
	#define DCHECK_LT_F(a, b, ...)     CHECK_LT_F(a, b, ##__VA_ARGS__)
	#define DCHECK_LE_F(a, b, ...)     CHECK_LE_F(a, b, ##__VA_ARGS__)
	#define DCHECK_GT_F(a, b, ...)     CHECK_GT_F(a, b, ##__VA_ARGS__)
	#define DCHECK_GE_F(a, b, ...)     CHECK_GE_F(a, b, ##__VA_ARGS__)
#else
	#define DLOG_F(verbosity, ...)
	#define DVLOG_F(verbosity, ...)
	#define DLOG_IF_F(verbosity, ...)
	#define DVLOG_IF_F(verbosity, ...)
	#define DCHECK_F(test, ...)
	#define DCHECK_NOTNULL_F(x, ...)
	#define DCHECK_EQ_F(a, b, ...)
	#define DCHECK_NE_F(a, b, ...)
	#define DCHECK_LT_F(a, b, ...)
	#define DCHECK_LE_F(a, b, ...)
	#define DCHECK_GT_F(a, b, ...)
	#define DCHECK_GE_F(a, b, ...)
#endif

#ifdef LOGURU_REDEFINE_ASSERT
	#undef assert
	#ifndef NDEBUG
		#define assert(test) CHECK_WITH_INFO_F(!!(test), #test) // HACK
	#else
		#define assert(test)
	#endif
#endif // LOGURU_REDEFINE_ASSERT

// ----------------------------------------------------------------------------

#if LOGURU_WITH_STREAMS || LOGURU_REPLACE_GLOG

/* This file extends loguru to enable std::stream-style logging, a la Glog.
   It's an optional feature beind the LOGURU_WITH_STREAMS settings
   because including it everywhere will slow down compilation times.
*/

#include <sstream> // Adds about 38 kLoC on clang.

namespace loguru
{
	class StreamLogger : public std::ostringstream
	{
	public:
		StreamLogger(Verbosity verbosity, const char* file, unsigned line) : _verbosity(verbosity), _file(file), _line(line) {}
		~StreamLogger()
		{
			auto message = this->str();
			log(_verbosity, _file, _line, "%s", message.c_str());
		}

	private:
		Verbosity   _verbosity;
		const char* _file;
		unsigned    _line;
	};

	class AbortLogger : public std::ostringstream
	{
	public:
		AbortLogger(const char* expr, const char* file, unsigned line) : _expr(expr), _file(file), _line(line) {}
		~AbortLogger() LOGURU_NORETURN
		{
			auto message = this->str();
			loguru::log_and_abort(_expr, _file, _line, "%s", message.c_str());
		}

	private:
		const char* _expr;
		const char* _file;
		unsigned    _line;
	};

	class Voidify
	{
	public:
		Voidify() {}
		// This has to be an operator with a precedence lower than << but higher than ?:
		void operator&(const std::ostream&) {}
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
    (verbosity > loguru::g_verbosity || (cond) == false)                                           \
        ? (void)0                                                                                  \
        : loguru::Voidify() & loguru::StreamLogger(verbosity, __FILE__, __LINE__)
#define LOG_IF_S(verbosity_name, cond) VLOG_IF_S(loguru::NamedVerbosity::verbosity_name, cond)
#define VLOG_S(verbosity)              VLOG_IF_S(verbosity, true)
#define LOG_S(verbosity_name)          VLOG_S(loguru::NamedVerbosity::verbosity_name)

// -----------------------------------------------
// CHECKS:

#define CHECK_WITH_INFO_S(cond, info)                                                              \
    ((cond) == true)                                                                               \
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
#else
	#define DVLOG_IF_S(verbosity, cond)                                                     \
	    (true || verbosity > loguru::g_verbosity || (cond) == false)                        \
	        ? (void)0                                                                       \
	        : loguru::Voidify() & loguru::StreamLogger(verbosity, __FILE__, __LINE__)

	#define DLOG_IF_S(verbosity_name, cond) DVLOG_IF_S(loguru::NamedVerbosity::verbosity_name, cond)
	#define DVLOG_S(verbosity)              DVLOG_IF_S(verbosity, true)
	#define DLOG_S(verbosity_name)          DVLOG_S(loguru::NamedVerbosity::verbosity_name)
	#define DCHECK_S(cond)                  while (false) CHECK(cond)
	#define DCHECK_NOTNULL_S(x)             while (false) CHECK((x) != nullptr)
	#define DCHECK_EQ_S(a, b)               while (false) CHECK_EQ_S(a, b)
	#define DCHECK_NE_S(a, b)               while (false) CHECK_NE_S(a, b)
	#define DCHECK_LT_S(a, b)               while (false) CHECK_LT_S(a, b)
	#define DCHECK_LE_S(a, b)               while (false) CHECK_LE_S(a, b)
	#define DCHECK_GT_S(a, b)               while (false) CHECK_GT_S(a, b)
	#define DCHECK_GE_S(a, b)               while (false) CHECK_GE_S(a, b)
#endif

#if LOGURU_REPLACE_GLOG
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

	#define FLAGS_v                loguru::g_verbosity
	#define FLAGS_alsologtostderr  loguru::g_alsologtostderr
	#define FLAGS_colorlogtostderr loguru::g_colorlogtostderr

	#define VLOG_IS_ON(verbosity) ((verbosity) <= loguru::g_verbosity)
#endif // LOGURU_REPLACE_GLOG

#endif // LOGURU_WITH_STREAMS || LOGURU_REPLACE_GLOG
