## Loguru external

A comparison of loguru and GLOG.

### Keypoints
- Consolidate and stream-line the process
  - Build Gflags and Glog
  - Collect the dependent targets under one directory and use common `CMakeLists.txt` file
- Make it cross-platform (including platfoms without built-in bash, make, etc.)
  - Use CMake scripts (instead of shell/batch) wherever possible

### Try it locally
On a prompt, run the following
```
git submodule update --init
cmake -P build_and_run.cmake
```
Optionally:
- The build configuration can be overridden using `-DConfiguration=<config name>`. Use `Release` on Windows for better results.
- The CMake generator can be overridden using `-DGenerator=<generator name>`. Use `Ninja` for faster builds.

Ensure these go before `-P`.

The cmake script builds Gflags, Glog and the dependent test projects: glog_bench and glog_example.
After the build process, it runs the test projects and shows the relevant output.
#### Sample result
```
running T:/GitRepo/loguru/external/build/glog_bench;ERROR_QUIET
LOG(WARNING) << string (buffered): 29.347 +- 0.303 us per call
LOG(WARNING) << float  (buffered): 31.816 +- 0.103 us per call
LOG(WARNING) << string (unbuffered): 31.926 +- 0.139 us per call
LOG(WARNING) << float  (unbuffered): 34.603 +- 0.136 us per call
running T:/GitRepo/loguru/external/build/glog_example
I1231 17:03:14.299798  8852 glog_example.cpp:10] Hello from T:\GitRepo\loguru\external\test\glog_example.cpp!
I1231 17:03:14.301300  8852 glog_example.hpp:14] complex_calculation started
I1231 17:03:14.301300  8852 glog_example.hpp:15] Starting time machine...
W1231 17:03:14.501893  8852 glog_example.hpp:17] The flux capacitor is not getting enough power!
I1231 17:03:14.902706  8852 glog_example.hpp:19] Lighting strike!
E1231 17:03:15.303464  7036 glog_example.hpp:23] We ended up in 1985!
I1231 17:03:15.303964  8852 glog_example.hpp:25] complex_calculation stopped
I1231 17:03:15.303964  8852 glog_example.cpp:12] main function about to end!
```
There are various environment-dependent variables in the result. YMMV.

### Caveats
- The CMake generator cannot be changed between multiple runs. To change the generator, delete the `build` directory first.
