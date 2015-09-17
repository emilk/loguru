# Loguru: a simple C++ logging library

## License
Use, abuse, enjoy. Give credit if you like it!

## Why another logging library?
I abhor logging libraries that `#include`'s everything from `iostream` to `windows.h` into every compilation unit in your project. Logging should be frequent in your source code, and thus as lightweight as possible. Loguru's header has *no #includes*. This means it will not slow down the compilation of your project.

Also, I love the readable logs that indented scopes give you (see separate section).

## Versions
* Version 0.1 - 2015-03-22 - Works great on Mac.

## Features:
* Small, simple library.
* Small header with no `#include`s for **fast compile times**.
  * In a test of a medium-sized project, including `loguru.hpp` instead of `glog/logging.hpp` everywhere gave about 10% speedup in compilation times.
* No dependencies.
* Print using printf-style formatting or streams.
* Compile-time checked printf-formating (on supported compilers).
* Log to any combination of file, stdout and stderr.
* Thread-safe.
* Flushes output on each call so you won't miss anything even on hard crashes.
* Prefixes each line with:
  * Date and time to millisecond precision.
  * Application uptime to millisecond precision.
  * Thread id and name.
  * File and line.
  * Log level.
  * Indentation (see *Scopes*).
* Supports assertions: `CHECK(test, msg_fmt, ...)`
* Supports abort: `LOG(FATAL, ...)`.
* Cross-platform.
* Scopes (see separate heading).
* Install handles for logging (e.g. to draw log messages on screen in a game).
* Install handles for fatal error (e.g. to print stack traces).



## Scopes
The library supports scopes for indenting the log-file. Here's an example:

``` C++
int main(int argc, char* argv[]) {
	logging::init(argc, argv);
	LOG_SCOPE_FUNCTION();
	LOG(INFO, "Doing some stuff...");
	for (int i=0; i<2; ++i) {
		LOG_SCOPE("Iteration %d", i);
		auto result = some_expensive_operation();
		if (result == BAD) {
			LOG(WARNING, "Bad result")
		}
	}
	LOG(INFO, "Time to go!");
}
```

This will output:

```
date       time         [thread name     thread id]                 file:line lvl|
2015-03-21 22:33:58.615 [main             396D9   ]             main.cpp:2      0| { main()
2015-03-21 22:33:58.877 [main             396D9   ]             main.cpp:3      0| .   Doing some stuff
2015-03-21 22:33:58.960 [main             396D9   ]             main.cpp:5      0| .   { Iteration 0
2015-03-21 22:33:58.960 [main             396D9   ]             main.cpp:5      0| .   } 0.302 s
2015-03-21 22:33:58.960 [main             396D9   ]             main.cpp:5      0| .   { Iteration 1
2015-03-21 22:33:58.960 [main             396D9   ]             main.cpp:8     -1| .   .   Bad result
2015-03-21 22:33:58.960 [main             396D9   ]             main.cpp:5      0| .   } 0.317 s
2015-03-21 22:34:59.020 [main             396D9   ]             main.cpp:11     0| .   Time to go!
2015-03-21 22:34:59.020 [main             396D9   ]             main.cpp:2      0| } 0.619 s
```

Scopes affects logging on all threads.



## Streams vs printf#
Some logging libraries only supports stream style logging, not printf-style. This means that what should be:

```
LOG(INFO, "Some float: %+05.3f", number);
```

in Glog becomes something along the lines of:

```
LOG(INFO) << "Some float: " << std::setfill('0') << std::setw(5) << std::setprecision(3) << number;
// Plus whatever else is needed to reset the stream after this.
```

Loguru allows you to use whatever style you prefer.


## Limitations and TODO
* Only prints to a single file.
* Severity not yet customizable.
* Better cross-platform support (Clang + POSIX only at the moment).
* Code needs cleanup.
* Currently depends on boost::posix_time.
* There is room for optimization.

