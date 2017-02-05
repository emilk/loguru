// Include loguru first to test that it needs no dependencies:
#define LOGURU_FILENAME_WIDTH  16
#define LOGURU_WITH_STREAMS     1
#define LOGURU_REDEFINE_ASSERT  1
#define LOGURU_USE_FMTLIB       0
#define LOGURU_WITH_FILEABS     0
#define LOGURU_IMPLEMENTATION   1
#include "../loguru.hpp"

#include <chrono>
#include <string>
#include <thread>

#include <fstream>

void the_one_where_the_problem_is(const std::vector<std::string>& v) {
	ABORT_F("Abort deep in stack trace, msg: %s", v[0].c_str());
}
void deep_abort_1(const std::vector<std::string>& v) { the_one_where_the_problem_is(v); }
void deep_abort_2(const std::vector<std::string>& v) { deep_abort_1(v); }
void deep_abort_3(const std::vector<std::string>& v) { deep_abort_2(v); }
void deep_abort_4(const std::vector<std::string>& v) { deep_abort_3(v); }
void deep_abort_5(const std::vector<std::string>& v) { deep_abort_4(v); }
void deep_abort_6(const std::vector<std::string>& v) { deep_abort_5(v); }
void deep_abort_7(const std::vector<std::string>& v) { deep_abort_6(v); }
void deep_abort_8(const std::vector<std::string>& v) { deep_abort_7(v); }
void deep_abort_9(const std::vector<std::string>& v) { deep_abort_8(v); }
void deep_abort_10(const std::vector<std::string>& v) { deep_abort_9(v); }

void sleep_ms(int ms)
{
	LOG_F(3, "Sleeping for %d ms", ms);
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void test_thread_names()
{
	LOG_SCOPE_FUNCTION(INFO);

	{
		char thread_name[17];
		loguru::get_thread_name(thread_name, sizeof(thread_name), false);
		LOG_F(INFO, "Hello from main thread ('%s')", thread_name);
	}

	auto a = std::thread([](){
		char thread_name[17];
		loguru::get_thread_name(thread_name, sizeof(thread_name), false);
		LOG_F(INFO, "Hello from nameless thread ('%s')", thread_name);
	});

	auto b = std::thread([](){
		loguru::set_thread_name("renderer");
		char thread_name[17];
		loguru::get_thread_name(thread_name, sizeof(thread_name), false);
		LOG_F(INFO, "Hello from render thread ('%s')", thread_name);
	});

	auto c = std::thread([](){
		loguru::set_thread_name("abcdefghijklmnopqrstuvwxyz");
		char thread_name[17];
		loguru::get_thread_name(thread_name, sizeof(thread_name), false);
		LOG_F(INFO, "Hello from thread with a very long name ('%s')", thread_name);
	});

	a.join();
	b.join();
	c.join();
}

void test_scopes()
{
	LOG_SCOPE_FUNCTION(INFO);

	LOG_F(INFO, "Should be indented one step");
	LOG_F(1, "First thing");
	LOG_F(1, "Second thing");

	{
		LOG_SCOPE_F(1, "Some indentation at level 1");
		LOG_F(INFO, "Should only be indented one more step iff verbosity is 1 or higher");
		LOG_F(2, "Some info");
		sleep_ms(123);
	}

	sleep_ms(64);
}

void test_levels()
{
	LOG_SCOPE_FUNCTION(INFO);
	{
		VLOG_SCOPE_F(1, "Scope with verbosity 1");
		LOG_F(3,       "Only visible with -v 3 or higher");
		LOG_F(2,       "Only visible with -v 2 or higher");
		LOG_F(1,       "Only visible with -v 1 or higher");
	}
	LOG_F(0,       "LOG_F(0)");
	LOG_F(INFO,    "This is some INFO");
	LOG_F(WARNING, "This is a WARNING");
	LOG_F(ERROR,   "This is a serious ERROR");
}

#if LOGURU_WITH_STREAMS
void test_stream()
{
	LOG_SCOPE_FUNCTION(INFO);
	LOG_S(INFO) << "Testing stream-logging.";
	LOG_S(INFO) << "First line" << std::endl << "Seconds line.";
	LOG_S(1) << "Stream-logging with verbosity 1";
	LOG_S(2) << "Stream-logging with verbosity 2";
	LOG_S(3) << "Stream-logging with verbosity 3";
	LOG_IF_S(INFO, true) << "Should be visible";
	LOG_IF_S(INFO, false) << "SHOULD NOT BE VISIBLE";
	LOG_IF_S(1, true) << "Should be visible if verbosity is at least 1";
	LOG_IF_S(1, false) << "SHOULD NOT BE VISIBLE";
	CHECK_LT_S(1, 2);
	CHECK_GT_S(3, 2) << "Weird";
}
#endif

int some_expensive_operation() { static int r=31; sleep_ms(132); return r++; }
const int BAD = 32;

int always_increasing() { static int x = 0; return x++; }

int main_test(int argc, char* argv[])
{
	loguru::init(argc, argv);
	LOG_SCOPE_FUNCTION(INFO);
	LOG_F(INFO, "Doing some stuff...");
	for (int i=0; i<2; ++i) {
		LOG_SCOPE_F(1, "Iteration %d", i);
		auto result = some_expensive_operation();
		LOG_IF_F(WARNING, result == BAD, "Bad result");
	}
	LOG_F(INFO, "Time to go!");

	return 0;
}

void test_SIGSEGV_0()
{
	LOG_F(INFO, "Intentionally writing to nullptr:");
	int* ptr = nullptr;
	*ptr = 42;
	LOG_F(FATAL, "We shouldn't get here");
}
void test_SIGSEGV_1() { test_SIGSEGV_0(); }
void test_SIGSEGV_2() { test_SIGSEGV_1(); }

void test_abort_0()
{
	LOG_F(INFO, "Calling std::abort");
	std::abort();
}
void test_abort_1() { test_abort_0(); }
void test_abort_2() { test_abort_1(); }

struct CustomType
{
	std::string contents;
};

namespace loguru {
Text ec_to_text(const CustomType* custom)
{
	return Text(strdup(custom->contents.c_str()));
}
} // namespace loguru

void test_error_contex()
{
	{ ERROR_CONTEXT("THIS SHOULDN'T BE PRINTED", "scoped"); }
	ERROR_CONTEXT("Parent thread value", 42);
	{ ERROR_CONTEXT("THIS SHOULDN'T BE PRINTED", "scoped"); }
	char parent_thread_name[17];
	loguru::get_thread_name(parent_thread_name, sizeof(parent_thread_name), false);
	ERROR_CONTEXT("Parent thread name", &parent_thread_name[0]);

	const auto parent_ec_handle = loguru::get_thread_ec_handle();

	std::thread([=]{
		loguru::set_thread_name("EC test thread");
		ERROR_CONTEXT("parent error context", parent_ec_handle);
		{ ERROR_CONTEXT("THIS SHOULDN'T BE PRINTED", "scoped"); }
		ERROR_CONTEXT("const char*",       "test string");
		ERROR_CONTEXT("integer",           42);
		ERROR_CONTEXT("float",              3.14f);
		ERROR_CONTEXT("double",             3.14);
		{ ERROR_CONTEXT("THIS SHOULDN'T BE PRINTED", "scoped"); }
		ERROR_CONTEXT("char A",            'A');
		ERROR_CONTEXT("char backslash",    '\\');
		ERROR_CONTEXT("char double-quote", '\"');
		ERROR_CONTEXT("char single-quote", '\'');
		ERROR_CONTEXT("char zero",         '\0');
		ERROR_CONTEXT("char bell",         '\b');
		ERROR_CONTEXT("char feed",         '\f');
		ERROR_CONTEXT("char newline",      '\n');
		ERROR_CONTEXT("char return",       '\r');
		ERROR_CONTEXT("char tab",          '\t');
		ERROR_CONTEXT("char x13",          '\u0013');
		{ ERROR_CONTEXT("THIS SHOULDN'T BE PRINTED", "scoped"); }
		CustomType custom{"custom_contents"};
		ERROR_CONTEXT("CustomType", &custom);
		ABORT_F("Intentional abort");
	}).join();
}

void test_hang_0()
{
	LOG_F(INFO, "Press ctrl-C to kill.");
	for(;;) {
		// LOG_F(INFO, "Press ctrl-C to break out of this infinite loop.");
	}
}
void test_hang_1() { test_hang_0(); }
void test_hang_2() { test_hang_1(); }

void throw_on_fatal()
{
	loguru::set_fatal_handler([](const loguru::Message& message){
		LOG_F(INFO, "Throwing exception...");
		throw std::runtime_error(std::string(message.prefix) + message.message);
	});
	{
		LOG_SCOPE_F(INFO, "CHECK_F throw + catch");
		try {
			CHECK_F(false, "some CHECK_F message");
		} catch (std::runtime_error& e) {
			LOG_F(INFO, "CHECK_F threw this: '%s'", e.what());
		}
	}
#if LOGURU_WITH_STREAMS
	{
		LOG_SCOPE_F(INFO, "CHECK_S throw + catch");
		try {
			CHECK_S(false) << "Some CHECK_S message";
		} catch (std::runtime_error& e) {
			LOG_F(INFO, "CHECK_S threw this: '%s'", e.what());
		}
	}
	LOG_F(INFO, "Trying an uncaught exception:");
	CHECK_S(false);
#else
	CHECK_F(false);
#endif // LOGURU_WITH_STREAMS
}

void throw_on_signal()
{
	loguru::set_fatal_handler([](const loguru::Message& message){
		LOG_F(INFO, "Throwing exception...");
		throw std::runtime_error(std::string(message.prefix) + message.message);
	});
	test_SIGSEGV_0();
}

// void die(std::ofstream& of)
// {
// 	(void)of;
// 	test_hang_2();
// }

// ----------------------------------------------------------------------------

struct CallbackTester
{
	size_t num_print = 0;
	size_t num_flush = 0;
	size_t num_close = 0;
};

void callbackPrint(void* user_data, const loguru::Message& message)
{
    printf("Custom callback: %s%s\n", message.prefix, message.message);
    reinterpret_cast<CallbackTester*>(user_data)->num_print += 1;
}

void callbackFlush(void* user_data)
{
	printf("Custom callback flush\n");
    reinterpret_cast<CallbackTester*>(user_data)->num_flush += 1;
}

void callbackClose(void* user_data)
{
	printf("Custom callback close\n");
    reinterpret_cast<CallbackTester*>(user_data)->num_close += 1;
}

void test_log_callback()
{
	CallbackTester tester;
	loguru::add_callback(
		"user_callback", callbackPrint, &tester,
		loguru::Verbosity_INFO, callbackClose, callbackFlush);
	CHECK_EQ_F(tester.num_print, 0u);
	LOG_F(INFO, "Test print");
	CHECK_EQ_F(tester.num_print, 1u);
	CHECK_EQ_F(tester.num_close, 0u);
	CHECK_EQ_F(tester.num_flush, 1u);
	loguru::flush();
	CHECK_EQ_F(tester.num_flush, 2u);
	loguru::remove_callback("user_callback");
	CHECK_EQ_F(tester.num_close, 1u);
}

#if defined _WIN32 && defined _DEBUG
#define USE_WIN_DBG_HOOK
static int winDbgHook(int reportType, char *message, int *)
{
	fprintf(stderr, "Report type: %d\nMessage: %s\n", reportType,
	                (nullptr != message ? message : "nullptr message"));
    return 1; // To prevent the Abort, Retry, Ignore dialog
}
#endif

// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
#ifdef USE_WIN_DBG_HOOK
	_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, winDbgHook);
#endif

	if (argc > 1 && argv[1] == std::string("test"))
	{
		return main_test(argc, argv);
	}

	loguru::init(argc, argv);

	// auto verbose_type_name = loguru::demangle(typeid(std::ofstream).name());
	// loguru::add_stack_cleanup(verbose_type_name.c_str(), "std::ofstream");
	// std::ofstream os;
	// die(os);

	if (argc == 1)
	{
		loguru::add_file("latest_readable.log", loguru::Truncate, loguru::Verbosity_INFO);
		loguru::add_file("everything.log",      loguru::Append,   loguru::Verbosity_MAX);

		LOG_F(INFO, "Loguru test");
		test_thread_names();

		test_scopes();
		test_levels();
		#if LOGURU_WITH_STREAMS
		test_stream();
		#endif

		loguru::shutdown();

		LOG_F(INFO, "goes to stderr, but not to file");
	}
	else
	{
		std::string test = argv[1];
		if (test == "ABORT_F") {
			ABORT_F("ABORT_F format message");
		} else if (test == "ABORT_S") {
			ABORT_S() << "ABORT_S stream message";
		} else if (test == "assert") {
			const char* ptr = 0;
			assert(ptr && "Error that was unexpected");
		} else if (test == "LOG_F_FATAL") {
			LOG_F(FATAL, "Fatal format message");
		} else if (test == "LOG_S_FATAL") {
			LOG_S(FATAL) << "Fatal stream message";
		} else if (test == "CHECK_NOTNULL_F") {
			const char* ptr = 0;
			CHECK_NOTNULL_F(ptr);
		} else if (test == "CHECK_F") {
			CHECK_F(1 > 2);
		} else if (test == "CHECK_EQ_F") {
			CHECK_EQ_F(always_increasing(),  0);
			CHECK_EQ_F(always_increasing(),  1);
			CHECK_EQ_F(always_increasing(), 42);
		} else if (test == "CHECK_EQ_F_int") {
			int x = 42;
			CHECK_EQ_F(x, x + 1);
		} else if (test == "CHECK_EQ_F_unsigned") {
			unsigned x = 42;
			CHECK_EQ_F(x, x + 1);
		} else if (test == "CHECK_EQ_F_size_t") {
			size_t x = 42;
			CHECK_EQ_F(x, x + 1);
		} else if (test == "CHECK_EQ_F_message") {
			CHECK_EQ_F(always_increasing(),  0, "Should pass");
			CHECK_EQ_F(always_increasing(),  1, "Should pass");
			CHECK_EQ_F(always_increasing(), 42, "Should fail");
		} else if (test == "CHECK_EQ_S") {
			std::string str = "right";
			CHECK_EQ_F(str, "wrong", "Expected to fail, since `str` isn't \"wrong\" but \"%s\"", str.c_str());
		} else if (test == "CHECK_LT_S") {
			CHECK_EQ_F(always_increasing(), 0);
			CHECK_EQ_F(always_increasing(), 1);
			CHECK_EQ_F(always_increasing(), 42);
		} else if (test == "CHECK_LT_S_message") {
			CHECK_EQ_F(always_increasing(),  0, "Should pass");
			CHECK_EQ_F(always_increasing(),  1, "Should pass");
			CHECK_EQ_F(always_increasing(), 42, "Should fail!");
		} else if (test == "deep_abort") {
			deep_abort_10({"deep_abort"});
		} else if (test == "SIGSEGV") {
			test_SIGSEGV_2();
		} else if (test == "abort") {
			test_abort_2();
		} else if (test == "error_context") {
			test_error_contex();
		} else if (test == "throw_on_fatal") {
			throw_on_fatal();
		} else if (test == "throw_on_signal") {
			throw_on_signal();
		} else if (test == "callback") {
			test_log_callback();
		} else if (test == "hang") {
			loguru::add_file("hang.log", loguru::Truncate, loguru::Verbosity_INFO);
			test_hang_2();
		} else {
			LOG_F(ERROR, "Unknown test: '%s'", test.c_str());
		}
	}
}
