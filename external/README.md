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
Optionally, the CMake generator can be overridden using `-DGenerator=<generator name>`.
Ensure it goes before `-P`.

The cmake script builds Gflags, Glog and the dependent test projects: glog_bench and glog_example.
After the build process, it runs the test projects and shows the relevant output.
#### Sample result
```
running T:/GitRepo/loguru/external/build/glog_bench;ERROR_QUIET
LOG(WARNING) << string (buffered): 73.695 +- 1.548 us per call
LOG(WARNING) << float  (buffered): 83.216 +- 0.986 us per call
LOG(WARNING) << string (unbuffered): 79.280 +- 2.453 us per call
LOG(WARNING) << float  (unbuffered): 87.755 +- 0.762 us per call
running T:/GitRepo/loguru/external/build/glog_example
I1231 14:49:09.042631  4020 glog_example.cpp:10] Hello from main.cpp!
I1231 14:49:09.044632  4020 glog_example.hpp:14] complex_calculation started
I1231 14:49:09.044632  4020 glog_example.hpp:15] Starting time machine...
W1231 14:49:09.245019  4020 glog_example.hpp:17] The flux capacitor is not getting enough power!
I1231 14:49:09.646394  4020 glog_example.hpp:19] Lighting strike!
E1231 14:49:10.047559 10976 glog_example.hpp:23] We ended up in 1985!
I1231 14:49:10.048560  4020 glog_example.hpp:25] complex_calculation stopped
I1231 14:49:10.048560  4020 glog_example.cpp:12] main function about to end!
```
There are various environment-dependent variables in the result. YMMV.

### Caveats
- The CMake generator cannot be changed between multiple runs. To change the generator, delete the `build` directory first.
