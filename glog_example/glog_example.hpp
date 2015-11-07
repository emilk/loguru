#include <chrono>
#include <thread>

#include <glog/logging.h>

inline void sleep_ms(int ms)
{
    VLOG(2) << "Sleeping for " << ms << " ms";
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void complex_calculation()
{
	LOG(INFO) << "complex_calculation started";
	LOG(INFO) << "Starting time machine...";
	sleep_ms(200);
	LOG(WARNING) << "The flux capacitor is not getting enough power!";
	sleep_ms(400);
	LOG(INFO) << "Lighting strike!";
	VLOG(1) << "Found 1.21 gigawatts...";
	sleep_ms(400);
	std::thread([](){
		LOG(ERROR) << "We ended up in 1985!";
	}).join();
	LOG(INFO) << "complex_calculation stopped";
}
