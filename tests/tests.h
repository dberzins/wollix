// tests.h - minimal test framework for wollix.h
// No dependencies beyond libc. Header-only.
//
// Usage:
//   #include "tests.h"
//
//   TEST(my_test) {
//       ASSERT_TRUE(1 + 1 == 2);
//       ASSERT_EQ_INT(4, 2 + 2);
//       ASSERT_EQ_F(3.14f, 3.14f, 0.001f);
//   }
//
//   SUITE(my_suite) {
//       RUN_TEST(my_test);
//   }
//
//   int main(void) {
//       RUN_SUITE(my_suite);
//       return test_summary();
//   }

#ifndef TESTS_H_
#define TESTS_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>  // isatty

// ============================================================================
// Internal state
// ============================================================================

static int tests_passed  = 0;
static int tests_failed  = 0;
static int tests_total   = 0;
static int tests_assertions = 0;

// Per-test tracking
static const char *_test_current_name  = NULL;
static int         _test_current_failed = 0;

// ============================================================================
// Color output (auto-detect terminal)
// ============================================================================

static inline int _test_use_color(void) {
    static int cached = -1;
    if (cached < 0) cached = isatty(STDOUT_FILENO);
    return cached;
}

#define _TEST_COLOR_GREEN  "\033[32m"
#define _TEST_COLOR_RED    "\033[31m"
#define _TEST_COLOR_YELLOW "\033[33m"
#define _TEST_COLOR_RESET  "\033[0m"

#define _TEST_GREEN(s)  (_test_use_color() ? _TEST_COLOR_GREEN  s _TEST_COLOR_RESET : s)
#define _TEST_RED(s)    (_test_use_color() ? _TEST_COLOR_RED    s _TEST_COLOR_RESET : s)
#define _TEST_YELLOW(s) (_test_use_color() ? _TEST_COLOR_YELLOW s _TEST_COLOR_RESET : s)

// ============================================================================
// Test and suite declaration
// ============================================================================

#define TEST(name) static void name(void)

#define RUN_TEST(name) do {                                                    \
    _test_current_name = #name;                                                \
    _test_current_failed = 0;                                                  \
    tests_total++;                                                             \
    name();                                                                    \
    if (_test_current_failed) {                                                \
        tests_failed++;                                                        \
        printf("  %s %s\n", _TEST_RED("[FAIL]"), #name);                       \
    } else {                                                                   \
        tests_passed++;                                                        \
        printf("  %s %s\n", _TEST_GREEN("[PASS]"), #name);                     \
    }                                                                          \
} while(0)

#define SUITE(name) static void suite_##name(void)

#define RUN_SUITE(name) do {                                                   \
    printf("\n── %s ──\n", #name);                                             \
    suite_##name();                                                            \
} while(0)

// ============================================================================
// Assertions - each one increments tests_assertions and returns from the
// test function on failure (so later asserts in the same test are skipped).
// ============================================================================

#define _TEST_FAIL(fmt, ...) do {                                              \
    fprintf(stderr, "    %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);\
    _test_current_failed = 1;                                                  \
    return;                                                                    \
} while(0)

// Boolean
#define ASSERT_TRUE(cond) do {                                                 \
    tests_assertions++;                                                        \
    if (!(cond)) {                                                             \
        _TEST_FAIL("ASSERT_TRUE failed: %s", #cond);                          \
    }                                                                          \
} while(0)

#define ASSERT_FALSE(cond) do {                                                \
    tests_assertions++;                                                        \
    if ((cond)) {                                                              \
        _TEST_FAIL("ASSERT_FALSE failed: %s", #cond);                         \
    }                                                                          \
} while(0)

// Integer equality
#define ASSERT_EQ_INT(a, b) do {                                               \
    tests_assertions++;                                                        \
    long _a = (long)(a), _b = (long)(b);                                       \
    if (_a != _b) {                                                            \
        _TEST_FAIL("ASSERT_EQ_INT: %ld != %ld", _a, _b);                      \
    }                                                                          \
} while(0)

#define ASSERT_NEQ_INT(a, b) do {                                              \
    tests_assertions++;                                                        \
    long _a = (long)(a), _b = (long)(b);                                       \
    if (_a == _b) {                                                            \
        _TEST_FAIL("ASSERT_NEQ_INT: both are %ld", _a);                       \
    }                                                                          \
} while(0)

// Float equality within epsilon
#define ASSERT_EQ_F(a, b, eps) do {                                            \
    tests_assertions++;                                                        \
    float _a = (float)(a), _b = (float)(b);                                    \
    if (fabsf(_a - _b) > (float)(eps)) {                                       \
        _TEST_FAIL("ASSERT_EQ_F: %.4f != %.4f (eps=%.4f)",                    \
                   (double)_a, (double)_b, (double)(eps));                     \
    }                                                                          \
} while(0)

// WLX_Rect equality (all 4 fields within epsilon)
// Requires WLX_Rect to be defined (include wollix.h before tests.h)
#define ASSERT_EQ_RECT(a, b, eps) do {                                         \
    tests_assertions++;                                                        \
    WLX_Rect _ra = (a), _rb = (b);                                             \
    if (fabsf(_ra.x - _rb.x) > (float)(eps) ||                                \
        fabsf(_ra.y - _rb.y) > (float)(eps) ||                                \
        fabsf(_ra.w - _rb.w) > (float)(eps) ||                                \
        fabsf(_ra.h - _rb.h) > (float)(eps)) {                                \
        _TEST_FAIL("ASSERT_EQ_RECT:\n"                                        \
                   "      got  {%.2f, %.2f, %.2f, %.2f}\n"                    \
                   "      want {%.2f, %.2f, %.2f, %.2f}",                     \
                   (double)_ra.x, (double)_ra.y,                              \
                   (double)_ra.w, (double)_ra.h,                              \
                   (double)_rb.x, (double)_rb.y,                              \
                   (double)_rb.w, (double)_rb.h);                             \
    }                                                                          \
} while(0)

// WLX_Color equality (exact match, all 4 channels)
#define ASSERT_EQ_COLOR(clr_actual, clr_expect) do {                           \
    tests_assertions++;                                                        \
    WLX_Color _col_a = (clr_actual), _col_b = (clr_expect);                    \
    if (_col_a.r != _col_b.r || _col_a.g != _col_b.g ||                       \
        _col_a.b != _col_b.b || _col_a.a != _col_b.a) {                       \
        _TEST_FAIL("ASSERT_EQ_COLOR:\n"                                       \
                   "      got  {%d, %d, %d, %d}\n"                           \
                   "      want {%d, %d, %d, %d}",                            \
                   _col_a.r, _col_a.g, _col_a.b, _col_a.a,                   \
                   _col_b.r, _col_b.g, _col_b.b, _col_b.a);                  \
    }                                                                          \
} while(0)

// String equality
#define ASSERT_EQ_STR(a, b) do {                                               \
    tests_assertions++;                                                        \
    const char *_a = (a), *_b = (b);                                           \
    if (strcmp(_a, _b) != 0) {                                                 \
        _TEST_FAIL("ASSERT_EQ_STR:\n"                                         \
                   "      got  \"%s\"\n"                                      \
                   "      want \"%s\"", _a, _b);                              \
    }                                                                          \
} while(0)

// ============================================================================
// Summary - call at the end of main(), returns process exit code
// ============================================================================

static inline int test_summary(void) {
    printf("\n");
    if (tests_failed > 0) {
        printf("%s: %d passed, %d failed (%d assertions)\n",
               _TEST_RED("FAILED"), tests_passed, tests_failed, tests_assertions);
    } else {
        printf("%s: %d passed (%d assertions)\n",
               _TEST_GREEN("ALL PASSED"), tests_passed, tests_assertions);
    }
    return tests_failed > 0 ? 1 : 0;
}

#endif // TESTS_H_
