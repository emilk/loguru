/*
This file extends loguru to enable std::stream-style logging, a la Glog.
It's in a separate include because including it everywhere will slow down compilation times.
*/

#pragma once

#include "log.hpp"
#include <sstream> // Adds about 70 kLoC on clang.

namespace loguru
{
	class StreamLogger : public std::stringstream
	{
	public:
		StreamLogger(LogType type, const char* file, unsigned line) : _type(type), _file(file), _line(line) {}
		~StreamLogger() {
			auto str = this->str();
			log(_type, _file, _line, "%s", str.c_str());
		}

	private:
		LogType     _type;
		const char* _file;
		unsigned    _line;
	};

	// usage:  LOG_STREAM(loguru::LogType::INFO) << "Foo " << std::setprecision(10) << some_value;
	#define LOG_STREAM(type) StreamLogger(type, __FILE__, __LINE__)

#ifndef NDEBUG
#  define LOG_STREAM(...) LOG_STREAM(__VA_ARGS__)
#else
#  define DLOG(...)
#endif
}
