#include <chrono>
#include <thread>

#include "../loguru.hpp"

void sleep_ms(int ms)
{
	VLOG(3, "Sleeping for %d ms", ms);
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void test_thread_names()
{
	LOG_SCOPE_FUNCTION(INFO);

	LOG(INFO, "Hello from main thread!");

	auto a = std::thread([](){
		LOG(INFO, "Hello from nameless thread!");
	});

	auto b = std::thread([](){
		loguru::set_thread_name("renderer");
		LOG(INFO, "Hello from render thread!");
	});

	auto c = std::thread([](){
		loguru::set_thread_name("abcdefghijklmnopqrstuvwxyz");
		LOG(INFO, "Hello from thread with a very long name");
	});

	a.join();
	b.join();
	c.join();
}

void test_scopes()
{
	LOG_SCOPE_FUNCTION(INFO);

	VLOG(1, "First thing");
	VLOG(1, "Second thing");

	{
		VLOG_SCOPE(1, "Some indentation");
		VLOG(2, "Some info");
		sleep_ms(123);
	}

	sleep_ms(200);
}

void test_levels()
{
	LOG_SCOPE_FUNCTION(INFO);
	VLOG(3,      "Only visible with -v 3 or higher");
	VLOG(2,      "Only visible with -v 2 or higher");
	VLOG(1,      "Only visible with -v 1 or higher");
	VLOG(0,      "VLOG(0)");
	LOG(INFO,    "This is some INFO");
	LOG(WARNING, "This is a WARNING");
	LOG(ERROR,   "This is a serious ERROR");
}

void print_args(int argc, char* argv[])
{
	CHECK_EQ(argv[argc], nullptr);
	for (int i = 0; i < argc; ++i)
	{
		LOG(INFO, "argv[%d]: %s", i, argv[i]);
	}
}

int main(int argc, char* argv[])
{
	{
		LOG_SCOPE(INFO, "Raw arguments");
		print_args(argc, argv);
	}
	loguru::init(argc, argv);
	{
		LOG_SCOPE(INFO, "Arguments after loguru");
		print_args(argc, argv);
	}

	LOG(INFO, "Loguru test");

	test_thread_names();
	test_scopes();
	test_levels();

	DCHECK(!!(32 > 56));

	const char* ptr = 0;
	assert(ptr && "Error that was unexpected");
	CHECK_NOTNULL(ptr);
	CHECK(1 > 2, "Oh, no?");
	CHECK_EQ(3, 4, "Oh, no?");
	LOG(FATAL, "This is the end, beautiful friend");
}
