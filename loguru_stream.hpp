/*
This file extends loguru to enable std::stream-style logging, a la Glog.
It's in a separate include because including it everywhere will slow down compilation times.
*/

#pragma once

#include <sstream> // Adds about 38 kLoC on clang.

#include "loguru.hpp"

namespace loguru
{
	class StreamLogger : public std::stringstream
	{
	public:
		StreamLogger(Verbosity verbosity, const char* file, unsigned line) : _verbosity(verbosity), _file(file), _line(line) {}
		~StreamLogger() {
			auto str = this->str();
			log(_verbosity, _file, _line, "%s", str.c_str());
		}

	private:
		Verbosity   _verbosity;
		const char* _file;
		unsigned    _line;
	};

	// usage:  LOG_STREAM(INFO) << "Foo " << std::setprecision(10) << some_value;
	#define LOG_STREAM(verbosity_name) StreamLogger(loguru::Verbosity::verbosity_name, __FILE__, __LINE__)

#ifndef NDEBUG
#  define DLOG_STREAM(...) LOG_STREAM(__VA_ARGS__)
#else
#  define DLOG_STREAM(...)
#endif
}
