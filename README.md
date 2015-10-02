# Loguru: a simple C++ logging library

## License
Use, abuse, enjoy. Give credit if you like it!

## Why another logging library?
I have yet to come accross a nice, light-weight logging library for C++ that does everything I want. So I made one!

In particular, I want logging that produces logs that are both human-readable and easily grep:ed. I also want to be able to hook into the logging process to print some of the more severe messages on-screen in my app (for dev-purposes).

## Features:
* Small, simple library.
	* Small header with no `#include`s for **fast compile times** (see separate heading).
	* No dependencies.
	* Cross-platform - but only tested on Mac so far :)
* Print using printf-style formatting or streams.
* Compile-time checked printf-formating (on supported compilers).
* Assertion failures are marked with 'noreturn' for the benefit of the static analyzer and optimizer.
* Verbosity levels
* Log to any combination of file, stdout and stderr.
* Support flexible multiple file outputs
	* e.g. a logfile with just the latest run at low verbosity (high readability).
	* e.g. a full logfile at highest verbosity which is appended to
* Thread-safe.
* Flushes output on each call so you won't miss anything even on hard crashes.
* Prefixes each line with:
  * Date and time to millisecond precision.
  * Application uptime to millisecond precision.
  * Thread name or id.
  * File and line.
  * Log level.
  * Indentation (see *Scopes*).
* Supports assertions: `CHECK(fp != nullptr, "Failed to open '%s'", filename)`
* Supports abort: `ABORT("Something went wrong, debug value is %d", value)`.
* Scopes (see separate heading).
* User can install callbacks for logging (e.g. to draw log messages on screen in a game).
* User can install callbacks for fatal error (e.g. to print stack traces).
* Human-readable logs which include, date/time, uptime, thread name, file/line and loglevel.
* grep:able logs:
	* Each line has all the info you need (e.g. date).
	* You can easily filter out high verbosity levels after the fact.


## No includes in loguru.h
I abhor logging libraries that `#include`'s everything from `iostream` to `windows.h` into every compilation unit in your project. Logging should be frequent in your source code, and thus as lightweight as possible. Loguru's header has *no #includes*. This means it will not slow down the compilation of your project.

In a test of a medium-sized project, including `loguru.hpp` instead of `glog/logging.hpp` everywhere gave about 10% speedup in compilation times.

### Scopes
The library supports scopes for indenting the log-file. Here's an example:

``` C++
int main(int argc, char* argv[])
{
	loguru::init(argc, argv);
	LOG_SCOPE_FUNCTION_F(INFO);
	LOG(INFO, "Doing some stuff...");
	for (int i=0; i<2; ++i) {
		VLOG_SCOPE(1, "Iteration %d", i);
		auto result = some_expensive_operation();
		if (result == BAD) {
			LOG(WARNING, "Bad result");
		}
	}
	LOG(INFO, "Time to go!");
}
```

This will output:

```
date       time         ( uptime  ) [ thread name/id ]                 file:line     v|
2015-09-19 21:13:33.264 (   0.002s) [main thread     ]           loguru.cpp:138      0| -----------------------------------
2015-09-19 21:15:35.938 (   0.001s) [main thread     ]      loguru_test.cpp:80       0| { int main(int, char **)
2015-09-19 21:15:35.938 (   0.001s) [main thread     ]      loguru_test.cpp:81       0| .  Doing some stuff...
2015-09-19 21:15:35.938 (   0.001s) [main thread     ]      loguru_test.cpp:83       1| .  { Iteration 0
2015-09-19 21:15:35.938 (   0.001s) [main thread     ]      loguru_test.cpp:83       1| .  } 0.302 s: Iteration 0
2015-09-19 21:15:35.938 (   0.001s) [main thread     ]      loguru_test.cpp:83       1| .  { Iteration 1
2015-09-19 21:15:35.938 (   0.001s) [main thread     ]      loguru_test.cpp:86    WARN| .  .  Bad result
2015-09-19 21:15:35.938 (   0.001s) [main thread     ]      loguru_test.cpp:83       1| .  } 0.317 s: Iteration 1
2015-09-19 21:15:35.938 (   0.001s) [main thread     ]      loguru_test.cpp:89       0| .  Time to go!
2015-09-19 21:15:35.938 (   0.001s) [main thread     ]      loguru_test.cpp:80       0| } 0.619 s: int main(int, char **)
```

Scopes affects logging on all threads.


### Streams vs printf#
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


### Limitations and TODO
* Better cross-platform support (only tested with clang + POSIX at the moment).
* Code needs cleanup.
* Bench against GLOG.
* Color print to terminal?
* Is writing WARN/ERR/FATL to stderr the right thing to do?
* Log on atexit?
* Add LOGURU_OVERRIDE_ASSERT_AND_ABORT for #define abort etc.
* Use __builtin_expect.
* Raw logging (no preamble or indentation).
* File logging should start with PREAMBLE_EXPLAIN etc.
* Replicate InstallFailureSignalHandler
* Default log-file with good path and name
