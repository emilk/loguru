#include <iostream>
#include "glog_example.hpp"

int main(int argc, char* argv[])
{
    FLAGS_alsologtostderr = true;
    FLAGS_colorlogtostderr = true;
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    LOG(INFO) << "Hello from " __FILE__ "!";
    complex_calculation();
    LOG(INFO) << "main function about to end!";
}
