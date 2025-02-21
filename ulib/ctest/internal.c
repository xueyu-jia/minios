#include <ctest/internal.h>
#include <compiler.h>
#include <stdio.h>
#include <list.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>

struct CTEST_testsuite_entry;

//! TODO: use hash table to speed up lookup

struct CTEST_testcase_entry {
    struct CTEST_testsuite_entry *owner;
    const char *name;
    void (*routine)();
    struct list_node self;
    bool pass;
    size_t total_errors;
};

struct CTEST_testsuite_entry {
    const char *name;
    size_t total_cases;
    list_head testcases;
    struct list_node self;
    size_t total_failed;
};

static bool CTEST_initialized = false;
static list_head CTEST_testsuites = {};
static struct CTEST_testcase_entry *CTEST_current_testcase = NULL;

static void CTEST_ensures_initialized() {
    if (CTEST_initialized) { return; }
    list_init(&CTEST_testsuites);
    CTEST_initialized = true;
}

static void CTEST_ensures_valid_name(const char *name) {
    assert(strchr(name, '_') == NULL && "underscore `_` is not allowed in testsuite/testcase name");
}

static struct CTEST_testsuite_entry *CTEST_register_testsuite(const char *name) {
    struct CTEST_testsuite_entry *testsuite = NULL;
    list_for_each(&CTEST_testsuites, testsuite, self) {
        if (strcmp(testsuite->name, name) == 0) { break; }
    }

    if (testsuite == NULL) {
        testsuite = malloc(sizeof(struct CTEST_testsuite_entry));
        testsuite->name = strdup(name);
        testsuite->total_cases = 0;
        testsuite->total_failed = 0;
        list_init(&testsuite->testcases);
        list_add_last(&testsuite->self, &CTEST_testsuites);
    }
    return testsuite;
}

void CTEST_initialize(int argc, char **argv) {
    UNUSED(argc, argv);
    //! nothing to do for now
}

void CTEST_register_testcase(const char *testsuite_name, const char *testcase_name,
                             void (*routine)()) {
    CTEST_ensures_initialized();
    CTEST_ensures_valid_name(testsuite_name);
    CTEST_ensures_valid_name(testcase_name);

    struct CTEST_testsuite_entry *testsuite = CTEST_register_testsuite(testsuite_name);

    struct CTEST_testcase_entry *testcase = NULL;
    list_for_each(&testsuite->testcases, testcase, self) {
        if (strcmp(testcase->name, testcase_name) == 0) { break; }
    }

    assert(testcase == NULL && "duplicate testcase");

    testcase = malloc(sizeof(struct CTEST_testcase_entry));
    testcase->owner = testsuite;
    testcase->name = strdup(testcase_name);
    testcase->routine = routine;
    testcase->pass = false;
    testcase->total_errors = 0;
    list_add_last(&testcase->self, &testsuite->testcases);
    ++testsuite->total_cases;
}

void CTEST_notify_assertion_failure(const char *file, int line, const char *fmt, ...) {
    if (CTEST_current_testcase == NULL) { return; }
    fprintf(stderr, "%s:%d: Failure\n", file, line);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    ++CTEST_current_testcase->total_errors;
}

static void CTEST_run_testcase(struct CTEST_testcase_entry *testcase) {
    CTEST_current_testcase = testcase;
    testcase->routine();
    testcase->pass = testcase->total_errors == 0;
    if (!testcase->pass) { ++testcase->owner->total_failed; }
    CTEST_current_testcase = NULL;
}

int CTEST_run_all_tests() {
    assert(CTEST_initialized && "testcases ran before initialization");

    size_t total_testsuite = 0;
    size_t total_testcase = 0;

    struct CTEST_testsuite_entry *testsuite = NULL;
    struct CTEST_testcase_entry *testcase = NULL;
    list_for_each(&CTEST_testsuites, testsuite, self) {
        ++total_testsuite;
        total_testcase += testsuite->total_cases;
    }

    size_t total_testsuite_ran = total_testsuite;
    size_t total_testcase_ran = total_testcase;

    enum {
        GLOBAL,
        LOCAL,
        RUN,
        OK,
        FAILED,
        PASSED,
    };
    const char *prompt[] = {
        [GLOBAL] = "[==========]", [LOCAL] = "[----------]",  [RUN] = "[ RUN      ]",
        [OK] = "[       OK ]",     [FAILED] = "[  FAILED  ]", [PASSED] = "[  PASSED  ]",
    };

    size_t total_passed = 0;
    const clock_t beg_time = clock();

    printf("%s Running %zu tests from %zu test suites\n", prompt[GLOBAL], total_testcase,
           total_testsuite);
    list_for_each(&CTEST_testsuites, testsuite, self) {
        size_t skipped = 0;
        const clock_t beg_time = clock();
        printf("%s %zu tests from %s\n", prompt[LOCAL], testsuite->total_cases, testsuite->name);
        list_for_each(&testsuite->testcases, testcase, self) {
            if (testcase->routine == NULL) {
                ++skipped;
                continue;
            }
            printf("%s %s.%s\n", prompt[RUN], testsuite->name, testcase->name);
            const clock_t beg_time = clock();
            CTEST_run_testcase(testcase);
            printf("%s %s.%s (%d ms)\n", prompt[testcase->pass ? OK : FAILED], testsuite->name,
                   testcase->name, (int)((clock() - beg_time) * 1e3 / CLOCKS_PER_SEC));
            if (testcase->pass) { ++total_passed; }
        }
        printf("%s %zu tests from %s (%d ms)\n\n", prompt[LOCAL], testsuite->total_cases,
               testsuite->name, (int)((clock() - beg_time) * 1e3 / CLOCKS_PER_SEC));
        total_testcase_ran -= skipped;
        if (skipped == testsuite->total_cases) { --total_testsuite_ran; }
    }
    printf("%s %zu tests from %zu test suites ran (%d ms)\n", prompt[GLOBAL], total_testcase_ran,
           total_testsuite_ran, (int)((clock() - beg_time) * 1e3 / CLOCKS_PER_SEC));
    printf("%s %zu tests\n", prompt[PASSED], total_passed);
    if (total_passed < total_testcase_ran) {
        list_for_each(&CTEST_testsuites, testsuite, self) {
            list_for_each(&testsuite->testcases, testcase, self) {
                if (testcase->total_errors > 0) {
                    printf("%s %s.%s\n", prompt[FAILED], testsuite->name, testcase->name);
                }
            }
        }
    }

    return 0;
}

DESTRUCTOR static void CTEST_teardown() {
    struct CTEST_testsuite_entry *testsuite = NULL;
    struct CTEST_testcase_entry *testcase = NULL;
    while (!list_empty(&CTEST_testsuites)) {
        testsuite = list_front(&CTEST_testsuites, struct CTEST_testsuite_entry, self);
        list_remove(&testsuite->self);
        while (!list_empty(&testsuite->testcases)) {
            testcase = list_front(&testsuite->testcases, struct CTEST_testcase_entry, self);
            list_remove(&testcase->self);
            free((void *)testcase->name);
            free(testcase);
        }
        free((void *)testsuite->name);
        free(testsuite);
    }
}
