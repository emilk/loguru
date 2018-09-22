#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#define LOGURU_WITH_STREAMS 1
#define LOGURU_REDEFINE_ASSERT 1
#include "../loguru.cpp"

const size_t kNumRuns = 10;
const double kPi = 3.1415926535897932384626433;

static long long now_ns()
{
	using namespace std::chrono;
	return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

template<typename Function>
void bench(const std::string& name, const Function& function, size_t num_iterations)
{
	function(num_iterations); // Warm-up.

	printf("%-30s ", name.c_str());
	fflush(stdout);
	std::vector<double> times;
	double sum = 0;
	for (size_t i = 0; i < kNumRuns; ++i) {
		auto start_ns = now_ns();
		function(num_iterations);
		double total_time_sec = (now_ns() - start_ns) * 1e-9;
		times.push_back(total_time_sec / num_iterations);
		sum += times.back();
	}

	double mean = sum / kNumRuns;
	double std_dev_sum = 0;

	for (double time : times) {
		std_dev_sum += (time - mean) * (time - mean);
	}

	double variance = std::sqrt(std_dev_sum / kNumRuns);

	printf("%6.3f ± %.3f μs per call\n", mean * 1e6, variance * 1e6);
	fflush(stdout);
}

// ----------------------------------------------------------------------------

void format_strings(size_t num_iterations)
{
	for (size_t i = 0; i < num_iterations; ++i) {
		LOG_F(WARNING, "Some long, complex message.");
	}
	loguru::flush();
}

void format_float(size_t num_iterations)
{
	for (size_t i = 0; i < num_iterations; ++i) {
		LOG_F(WARNING, "%+05.3f", kPi);
	}
	loguru::flush();
}

void stream_strings(size_t num_iterations)
{
	for (size_t i = 0; i < num_iterations; ++i) {
		LOG_S(WARNING) << "Some long, complex message.";
	}
	loguru::flush();
}

void stream_float(size_t num_iterations)
{
	for (size_t i = 0; i < num_iterations; ++i) {
		LOG_S(WARNING) << std::setfill('0') << std::setw(5) << std::setprecision(3) << kPi;
	}
	loguru::flush();
}

void raw_string_float(size_t num_iterations)
{
	for (size_t i = 0; i < num_iterations; ++i) {
		RAW_LOG_F(WARNING, "Some long, complex message.");
	}
	loguru::flush();
}

void error_context(size_t num_iterations)
{
	for (size_t i = 0; i < num_iterations; ++i) {
		ERROR_CONTEXT("key", "value");
	}
}

int main(int argc, char* argv[])
{
	const size_t kNumIterations = 50 * 1000;

	loguru::init(argc, argv);
	loguru::add_file("loguru_bench.log", loguru::Truncate, loguru::Verbosity_INFO);

	bench("ERROR_CONTEXT", error_context, kNumIterations * 100);

	loguru::g_flush_interval_ms = 200;
	bench("LOG_F string (buffered):", format_strings,   kNumIterations);
	bench("LOG_F float  (buffered):", format_float,     kNumIterations);
	bench("LOG_S string (buffered):", stream_strings,   kNumIterations);
	bench("LOG_S float  (buffered):", stream_float,     kNumIterations);
	bench("RAW_LOG_F    (buffered):", raw_string_float, kNumIterations);

	loguru::g_flush_interval_ms = 0;
	bench("LOG_F string (unbuffered):", format_strings,   kNumIterations);
	bench("LOG_F float  (unbuffered):", format_float,     kNumIterations);
	bench("LOG_S string (unbuffered):", stream_strings,   kNumIterations);
	bench("LOG_S float  (unbuffered):", stream_float,     kNumIterations);
	bench("RAW_LOG_F    (unbuffered):", raw_string_float, kNumIterations);
}
