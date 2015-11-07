#include <iostream>

#include "loguru_example.hpp"

#define LOGURU_IMPLEMENTATION 1
#include "../loguru.hpp"

int main(int argc, char* argv[])
{
    loguru::init(argc, argv);
    LOG_F(INFO, "Hello from main.cpp!");
    complex_calculation();
    LOG_F(INFO, "main function about to end!");
}
