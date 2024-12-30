#pragma once

#include <ctest/internal.h>
#include <ctest/assert.h>
#include <macro_helper.h>
#include <compiler.h>

//! TODO: support fixture

#define CTEST_TESTCASE_ID(testsuite_name, testcase_name) \
    MH_CONCAT(test_, MH_CONCAT(testsuite_name, MH_CONCAT(_, testcase_name)))

#define CTEST_TESTCASE_ROUTINE(testsuite_name, testcase_name) \
    MH_CONCAT(CTEST_private_routine_, CTEST_TESTCASE_ID(testsuite_name, testcase_name))

#define TEST(testsuite_name, testcase_name)                                                 \
    void CTEST_TESTCASE_ROUTINE(testsuite_name, testcase_name)();                           \
    CONSTRUCTOR static void MH_CONCAT(CTEST_register_testcase_,                             \
                                      CTEST_TESTCASE_ID(testsuite_name, testcase_name))() { \
        CTEST_register_testcase(MH_STRINGIFY(testsuite_name), MH_STRINGIFY(testcase_name),  \
                                CTEST_TESTCASE_ROUTINE(testsuite_name, testcase_name));     \
    }                                                                                       \
    void CTEST_TESTCASE_ROUTINE(testsuite_name, testcase_name)()

void CTEST_initialize(int argc, char** argv);
int CTEST_run_all_tests() MUST_USE_RESULT;

#define RUN_ALL_TESTS() CTEST_run_all_tests()

#ifdef CTEST_MAIN
int main(int argc, char* argv[]) {
    CTEST_initialize(argc, argv);
    return RUN_ALL_TESTS();
}
#endif
