#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#define LOGURU_FLUSH_INTERVAL_MS 100 // Try commenting this line
#define LOGURU_WITH_STREAMS 1
#define LOGURU_REDEFINE_ASSERT 1
#define LOGURU_IMPLEMENTATION 1
#include "../loguru.hpp"

const size_t kNumIterations = 50 * 1000;
const size_t kNumRuns = 10;
const double kPi = 3.1415926535897932384626433;

static long long now_ns()
{
	using namespace std::chrono;
	return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

template<typename Function>
double time_sec(const Function& function)
{
	auto start_ns = now_ns();
	function();
	return (now_ns() - start_ns) * 1e-9;
}

template<typename Function>
void bench(const std::string& name, const Function& function)
{
	printf("%-50s ", name.c_str());
	fflush(stdout);
	std::vector<double> times;
	double sum = 0;
	for (size_t i = 0; i < kNumRuns; ++i)
	{
		times.push_back(time_sec(function) / kNumIterations);
		sum += times.back();
	}

	double mean = sum / kNumRuns;
	double std_dev_sum = 0;

	for (double time : times) {
		std_dev_sum += (time - mean) * (time - mean);
	}

	double variance = std::sqrt(std_dev_sum / kNumRuns);

	printf("%6.3f +- %.3f us per call\n", mean * 1e6, variance * 1e6);
	fflush(stdout);
}

// ----------------------------------------------------------------------------

void format_strings()
{
	for (size_t i = 0; i < kNumIterations; ++i) {
		LOG_F(WARNING, "Some long, complex message.");
	}
	loguru::flush();
}

void format_float()
{
	for (size_t i = 0; i < kNumIterations; ++i) {
		LOG_F(WARNING, "%+05.3f", kPi);
	}
	loguru::flush();
}

void stream_strings()
{
	for (size_t i = 0; i < kNumIterations; ++i) {
		LOG_S(WARNING) << "Some long, complex message.";
	}
	loguru::flush();
}

void stream_float()
{
	for (size_t i = 0; i < kNumIterations; ++i) {
		LOG_S(WARNING) << std::setfill('0') << std::setw(5) << std::setprecision(3) << kPi;
	}
	loguru::flush();
}

void raw_string_float()
{
	for (size_t i = 0; i < kNumIterations; ++i) {
		RAW_LOG_F(WARNING, "Some long, complex message.");
	}
	loguru::flush();
}

int main(int argc, char* argv[])
{
	loguru::init(argc, argv);
	loguru::add_file("loguru_bench.log", loguru::Truncate, loguru::Verbosity_INFO);

	bench("LOG_F string (unbuffered):", format_strings);
	bench("LOG_F float  (unbuffered):", format_float);
    bench("LOG_S string (unbuffered):", stream_strings);
    bench("LOG_S float  (unbuffered):", stream_float);
    bench("RAW_LOG_F    (unbuffered):", raw_string_float);
}
