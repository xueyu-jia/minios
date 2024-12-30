#pragma once

#include <macro_helper.h>
#include <stdbool.h>

void CTEST_register_testcase(const char *testsuite_name, const char *testcase_name,
                             void (*routine)());
