#include <chrono>
#include <string>
#include <thread>

#define LOGURU_WITH_STREAMS 1
#define LOGURU_REDEFINE_ASSERT 1
#define LOGURU_IMPLEMENTATION 1
#include "../loguru.hpp"

void the_one_where_the_problem_is(const std::vector<std::string>& v) {
	ABORT_F("Abort deep in stack trace, msg: %s", v[0].c_str());
}
void deep_abort_1(const std::string& str) { the_one_where_the_problem_is({str}); }
void deep_abort_2(const std::string& str) { deep_abort_1(str); }
void deep_abort_3(const std::string& str) { deep_abort_2(str); }
void deep_abort_4(const std::string& str) { deep_abort_3(str); }
void deep_abort_5(const std::string& str) { deep_abort_4(str); }
void deep_abort_6(const std::string& str) { deep_abort_5(str); }
void deep_abort_7(const std::string& str) { deep_abort_6(str); }
void deep_abort_8(const std::string& str) { deep_abort_7(str); }
void deep_abort_9(const std::string& str) { deep_abort_8(str); }
void deep_abort_10(const std::string& str) { deep_abort_9(str); }

void sleep_ms(int ms)
{
	LOG_F(3, "Sleeping for %d ms", ms);
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void test_thread_names()
{
	LOG_SCOPE_FUNCTION(INFO);

	LOG_F(INFO, "Hello from main thread!");

	auto a = std::thread([](){
		LOG_F(INFO, "Hello from nameless thread!");
	});

	auto b = std::thread([](){
		loguru::set_thread_name("renderer");
		LOG_F(INFO, "Hello from render thread!");
	});

	auto c = std::thread([](){
		loguru::set_thread_name("abcdefghijklmnopqrstuvwxyz");
		LOG_F(INFO, "Hello from thread with a very long name");
	});

	a.join();
	b.join();
	c.join();
}

void test_scopes()
{
	LOG_SCOPE_FUNCTION(INFO);

	LOG_F(1, "First thing");
	LOG_F(1, "Second thing");

	{
		LOG_SCOPE_F(1, "Some indentation");
		LOG_F(2, "Some info");
		sleep_ms(123);
	}

	sleep_ms(64);
}

void test_levels()
{
	LOG_SCOPE_FUNCTION(INFO);
	LOG_F(3,       "Only visible with -v 3 or higher");
	LOG_F(2,       "Only visible with -v 2 or higher");
	LOG_F(1,       "Only visible with -v 1 or higher");
	LOG_F(0,       "LOG_F(0)");
	LOG_F(INFO,    "This is some INFO");
	LOG_F(WARNING, "This is a WARNING");
	LOG_F(ERROR,   "This is a serious ERROR");
}

void test_stream()
{
	LOG_SCOPE_FUNCTION(INFO);
	LOG_S(INFO) << "Testing stream-logging.";
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

int some_expensive_operation() { static int r=31; sleep_ms(132); return r++; }
int BAD = 32;

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

void test_hang_0()
{
	LOG_F(INFO, "Press ctrl-C to kill.");
	for(;;) {
		// LOG_F(INFO, "Press ctrl-C to break out of this infinite loop.");
	}
}
void test_hang_1() { test_hang_0(); }
void test_hang_2() { test_hang_1(); }

int main(int argc, char* argv[])
{
	if (argc > 1 && argv[1] == std::string("test"))
	{
		return main_test(argc, argv);
	}

	loguru::init(argc, argv);

	if (argc == 1)
	{
		loguru::add_file("latest_readable.log", loguru::Truncate, loguru::Verbosity_INFO);
		loguru::add_file("everything.log",      loguru::Append,   loguru::Verbosity_MAX);

		LOG_F(INFO, "Loguru test");
		test_thread_names();

		test_scopes();
		test_levels();
		test_stream();
	}
	else
	{
		std::string test = argv[1];
		if (test == "ABORT_F") {
			ABORT_F("This is the end, beautiful friend");
		} else if (test == "assert") {
			const char* ptr = 0;
			assert(ptr && "Error that was unexpected");
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
			CHECK_EQ_S(str, "wrong") << "Expected to fail, since `str` isn't \"wrong\" but \"" << str << "\"";
		} else if (test == "CHECK_LT_S") {
			CHECK_EQ_S(always_increasing(), 0);
			CHECK_EQ_S(always_increasing(), 1);
			CHECK_EQ_S(always_increasing(), 42);
		} else if (test == "CHECK_LT_S_message") {
			CHECK_EQ_S(always_increasing(),  0) << "Should pass";
			CHECK_EQ_S(always_increasing(),  1) << "Should pass";
			CHECK_EQ_S(always_increasing(), 42) << "Should fail!";
		} else if (test == "deep_abort") {
			deep_abort_10("deep_abort");
		} else if (test == "SIGSEGV") {
			test_SIGSEGV_2();
		} else if (test == "hang") {
			test_hang_2();
		} else {
			LOG_F(ERROR, "Unknown test: '%s'", test.c_str());
		}
	}
}
