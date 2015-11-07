#include <chrono>
#include <thread>

#include "../loguru.hpp"

inline void sleep_ms(int ms)
{
	VLOG_F(2, "Sleeping for %d ms", ms);
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void complex_calculation()
{
	LOG_SCOPE_F(INFO, "complex_calculation");
	LOG_F(INFO, "Starting time machine...");
	sleep_ms(200);
	LOG_F(WARNING, "The flux capacitor is not getting enough power!");
	sleep_ms(400);
	LOG_F(INFO, "Lighting strike!");
	VLOG_F(1, "Found 1.21 gigawatts...");
	sleep_ms(400);
	std::thread([](){
		loguru::set_thread_name("the past");
		LOG_F(ERROR, "We ended up in 1985!");
	}).join();
}
