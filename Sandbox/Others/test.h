//
// Created by Npchitman on 2021/1/21.
//

#ifndef HAZELENGINE_TEST_H
#define HAZELENGINE_TEST_H
#include <iostream>
#include <fstream>
#include <sstream>
#include "opgl_test.h"

#if false
#define TEST_ASSERT(x, terminate_func) { if(!(x)) { std::cout << "Assertion Failed" << std::endl;\
                                            terminate_func; \
                                            return -1; } }
#define TEST_CORE_ASSERT(x, ...) { if(!(x)) { printf("Assertion Failed"); __debugbreak(); } }
#endif

#endif //HAZELENGINE_TEST_H
