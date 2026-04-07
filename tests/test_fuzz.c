// test_fuzz.c - property-based fuzz tests for layout math invariants
// Verifies invariants hold over thousands of randomly generated inputs.

#include <stdlib.h>
#include <time.h>
#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

// Number of random iterations per fuzz test
#define FUZZ_ITERATIONS 5000
#define FUZZ_MAX_SLOTS  16

// Deterministic seeded RNG so failures are reproducible.
// We print the seed on failure.
static unsigned int _fuzz_seed = 0;

static void fuzz_srand(unsigned int seed) {
    _fuzz_seed = seed;
}

// xorshift32 - fast, deterministic, no libc dependency
static unsigned int fuzz_rand(void) {
    _fuzz_seed ^= _fuzz_seed << 13;
    _fuzz_seed ^= _fuzz_seed >> 17;
    _fuzz_seed ^= _fuzz_seed << 5;
    return _fuzz_seed;
}

// Random float in [lo, hi)
static float fuzz_randf(float lo, float hi) {
    return lo + (float)(fuzz_rand() % 10000) / 10000.0f * (hi - lo);
}

// Random int in [lo, hi]
static int fuzz_randi(int lo, int hi) {
    if (lo >= hi) return lo;
    return lo + (int)(fuzz_rand() % (unsigned)(hi - lo + 1));
}

// Generate a random WLX_Slot_Size
static WLX_Slot_Size fuzz_random_slot(void) {
    int kind = fuzz_randi(0, 3);
    float min_v = 0, max_v = 0;

    // 30% chance of having min and/or max constraints
    if (fuzz_randi(0, 9) < 3) {
        min_v = fuzz_randf(0, 50);
        if (fuzz_randi(0, 1)) {
            max_v = min_v + fuzz_randf(10, 200);
        }
    }

    switch (kind) {
        case 0: return (WLX_Slot_Size){ WLX_SIZE_AUTO,    0,                    min_v, max_v };
        case 1: return (WLX_Slot_Size){ WLX_SIZE_PIXELS,  fuzz_randf(1, 300),   min_v, max_v };
        case 2: return (WLX_Slot_Size){ WLX_SIZE_PERCENT, fuzz_randf(1, 100),   min_v, max_v };
        case 3: return (WLX_Slot_Size){ WLX_SIZE_FLEX,    fuzz_randf(0.1f, 10), min_v, max_v };
        default: return WLX_SLOT_AUTO;
    }
}

// ============================================================================
// Fuzz: wlx_compute_offsets invariants
// ============================================================================

// Invariant 1: offsets[0] == 0 always
// Invariant 2: offsets are monotonically non-decreasing
// Invariant 3: all slot sizes are non-negative
// Invariant 4: if min constraint, slot_size >= min (unless total is too small)
// Invariant 5: if max constraint, slot_size <= max
// Invariant 6: with NULL sizes (equal split), offsets[count] == total exactly

TEST(fuzz_offsets_monotonic) {
    unsigned int seed = (unsigned int)time(NULL) ^ 0xDEAD0001;
    fuzz_srand(seed);

    for (int iter = 0; iter < FUZZ_ITERATIONS; iter++) {
        size_t count = (size_t)fuzz_randi(1, FUZZ_MAX_SLOTS);
        float total = fuzz_randf(10.0f, 2000.0f);

        WLX_Slot_Size sizes[FUZZ_MAX_SLOTS];
        for (size_t i = 0; i < count; i++) {
            sizes[i] = fuzz_random_slot();
        }

        float offsets[FUZZ_MAX_SLOTS + 1];
        wlx_compute_offsets(offsets, count, total, total, sizes);

        // Invariant 1: starts at 0
        if (fabsf(offsets[0]) > 0.001f) {
            fprintf(stderr, "    fuzz_offsets_monotonic: offsets[0]=%.4f != 0 "
                    "(seed=%u, iter=%d, count=%zu, total=%.2f)\n",
                    (double)offsets[0], seed, iter, count, (double)total);
            _test_current_failed = 1;
            return;
        }
        tests_assertions++;

        // Invariant 2: monotonically non-decreasing
        for (size_t i = 0; i < count; i++) {
            float slot_size = offsets[i + 1] - offsets[i];
            if (slot_size < -0.01f) {
                fprintf(stderr, "    fuzz_offsets_monotonic: slot %zu has negative size %.4f "
                        "(seed=%u, iter=%d, count=%zu, total=%.2f)\n",
                        i, (double)slot_size, seed, iter, count, (double)total);
                _test_current_failed = 1;
                return;
            }
            tests_assertions++;
        }
    }
}

TEST(fuzz_offsets_equal_split) {
    unsigned int seed = (unsigned int)time(NULL) ^ 0xDEAD0002;
    fuzz_srand(seed);

    for (int iter = 0; iter < FUZZ_ITERATIONS; iter++) {
        size_t count = (size_t)fuzz_randi(1, FUZZ_MAX_SLOTS);
        float total = fuzz_randf(1.0f, 5000.0f);

        float offsets[FUZZ_MAX_SLOTS + 1];
        wlx_compute_offsets(offsets, count, total, total, NULL);

        // offsets[0] == 0
        if (fabsf(offsets[0]) > 0.001f) {
            fprintf(stderr, "    fuzz_offsets_equal_split: offsets[0]=%.4f "
                    "(seed=%u, iter=%d)\n", (double)offsets[0], seed, iter);
            _test_current_failed = 1;
            return;
        }
        tests_assertions++;

        // offsets[count] == total (±1px due to pixel snapping)
        if (fabsf(offsets[count] - total) > 1.0f) {
            fprintf(stderr, "    fuzz_offsets_equal_split: offsets[%zu]=%.4f != total=%.4f "
                    "(seed=%u, iter=%d)\n",
                    count, (double)offsets[count], (double)total, seed, iter);
            _test_current_failed = 1;
            return;
        }
        tests_assertions++;

        // All slots are equal (within tolerance; ±1px from pixel snapping)
        float expected_size = total / (float)count;
        for (size_t i = 0; i < count; i++) {
            float slot_size = offsets[i + 1] - offsets[i];
            if (fabsf(slot_size - expected_size) > 1.0f) {
                fprintf(stderr, "    fuzz_offsets_equal_split: slot %zu size=%.4f != expected=%.4f "
                        "(seed=%u, iter=%d)\n",
                        i, (double)slot_size, (double)expected_size, seed, iter);
                _test_current_failed = 1;
                return;
            }
            tests_assertions++;
        }

        // Monotonicity
        for (size_t i = 0; i < count; i++) {
            if (offsets[i + 1] < offsets[i] - 0.001f) {
                fprintf(stderr, "    fuzz_offsets_equal_split: non-monotonic at %zu "
                        "(seed=%u, iter=%d)\n", i, seed, iter);
                _test_current_failed = 1;
                return;
            }
            tests_assertions++;
        }
    }
}

TEST(fuzz_offsets_minmax_respected) {
    unsigned int seed = (unsigned int)time(NULL) ^ 0xDEAD0003;
    fuzz_srand(seed);

    for (int iter = 0; iter < FUZZ_ITERATIONS; iter++) {
        size_t count = (size_t)fuzz_randi(1, FUZZ_MAX_SLOTS);
        float total = fuzz_randf(50.0f, 2000.0f);

        WLX_Slot_Size sizes[FUZZ_MAX_SLOTS];
        for (size_t i = 0; i < count; i++) {
            sizes[i] = fuzz_random_slot();
        }

        float offsets[FUZZ_MAX_SLOTS + 1];
        wlx_compute_offsets(offsets, count, total, total, sizes);

        for (size_t i = 0; i < count; i++) {
            float slot_size = offsets[i + 1] - offsets[i];

            // Max constraint: slot should not exceed max (±1px from pixel snapping)
            if (sizes[i].max > 0 && slot_size > sizes[i].max + 1.0f) {
                fprintf(stderr, "    fuzz_offsets_minmax_respected: slot %zu size=%.4f > max=%.4f "
                        "(seed=%u, iter=%d, count=%zu, total=%.2f)\n",
                        i, (double)slot_size, (double)sizes[i].max, seed, iter, count, (double)total);
                _test_current_failed = 1;
                return;
            }
            tests_assertions++;

            // Min constraint: slot should be at least min
            // (exception: if total is too small to satisfy all mins, we can't guarantee this)
            if (sizes[i].min > 0 && slot_size < sizes[i].min - 1.0f) {
                // Only flag as failure if total is large enough that min should have been achievable
                float total_min = 0;
                for (size_t j = 0; j < count; j++) {
                    if (sizes[j].min > 0) total_min += sizes[j].min;
                }
                if (total >= total_min) {
                    fprintf(stderr, "    fuzz_offsets_minmax_respected: slot %zu size=%.4f < min=%.4f "
                            "(seed=%u, iter=%d, count=%zu, total=%.2f)\n",
                            i, (double)slot_size, (double)sizes[i].min, seed, iter, count, (double)total);
                    _test_current_failed = 1;
                    return;
                }
            }
            tests_assertions++;
        }
    }
}

TEST(fuzz_offsets_nonnegative_sizes) {
    unsigned int seed = (unsigned int)time(NULL) ^ 0xDEAD0004;
    fuzz_srand(seed);

    for (int iter = 0; iter < FUZZ_ITERATIONS; iter++) {
        size_t count = (size_t)fuzz_randi(1, FUZZ_MAX_SLOTS);
        float total = fuzz_randf(0.0f, 3000.0f);  // includes near-zero totals

        WLX_Slot_Size sizes[FUZZ_MAX_SLOTS];
        for (size_t i = 0; i < count; i++) {
            sizes[i] = fuzz_random_slot();
        }

        float offsets[FUZZ_MAX_SLOTS + 1];
        wlx_compute_offsets(offsets, count, total, total, sizes);

        for (size_t i = 0; i < count; i++) {
            float slot_size = offsets[i + 1] - offsets[i];
            if (slot_size < -0.01f) {
                fprintf(stderr, "    fuzz_offsets_nonnegative_sizes: slot %zu = %.4f < 0 "
                        "(seed=%u, iter=%d, count=%zu, total=%.2f)\n",
                        i, (double)slot_size, seed, iter, count, (double)total);
                _test_current_failed = 1;
                return;
            }
            tests_assertions++;
        }
    }
}

// ============================================================================
// Fuzz: wlx_get_align_rect invariants
// ============================================================================

TEST(fuzz_align_rect_contained) {
    unsigned int seed = (unsigned int)time(NULL) ^ 0xDEAD0005;
    fuzz_srand(seed);

    WLX_Align aligns[] = {
        WLX_ALIGN_NONE, WLX_TOP_LEFT, WLX_TOP_RIGHT, WLX_TOP_CENTER,
        WLX_BOTTOM_LEFT, WLX_BOTTOM_RIGHT, WLX_BOTTOM_CENTER,
        WLX_CENTER, WLX_LEFT, WLX_RIGHT, WLX_TOP, WLX_BOTTOM
    };
    int num_aligns = (int)wlx_array_len(aligns);

    for (int iter = 0; iter < FUZZ_ITERATIONS; iter++) {
        float px = fuzz_randf(-500, 500);
        float py = fuzz_randf(-500, 500);
        float pw = fuzz_randf(1, 1000);
        float ph = fuzz_randf(1, 1000);
        WLX_Rect parent = wlx_rect(px, py, pw, ph);

        int cw = fuzz_randi(1, (int)pw + 50);  // sometimes larger than parent
        int ch = fuzz_randi(1, (int)ph + 50);

        WLX_Align align = aligns[fuzz_randi(0, num_aligns - 1)];

        WLX_Rect r = wlx_get_align_rect(parent, cw, ch, align);

        // Result x >= parent.x
        if (r.x < parent.x - 0.01f) {
            fprintf(stderr, "    fuzz_align_rect_contained: r.x=%.2f < parent.x=%.2f "
                    "(seed=%u, iter=%d, align=%d)\n",
                    (double)r.x, (double)parent.x, seed, iter, align);
            _test_current_failed = 1;
            return;
        }
        tests_assertions++;

        // Result y >= parent.y
        if (r.y < parent.y - 0.01f) {
            fprintf(stderr, "    fuzz_align_rect_contained: r.y=%.2f < parent.y=%.2f "
                    "(seed=%u, iter=%d, align=%d)\n",
                    (double)r.y, (double)parent.y, seed, iter, align);
            _test_current_failed = 1;
            return;
        }
        tests_assertions++;

        // Result dimensions <= parent dimensions (clamped to parent)
        if (r.w > parent.w + 0.01f) {
            fprintf(stderr, "    fuzz_align_rect_contained: r.w=%.2f > parent.w=%.2f "
                    "(seed=%u, iter=%d, align=%d)\n",
                    (double)r.w, (double)parent.w, seed, iter, align);
            _test_current_failed = 1;
            return;
        }
        tests_assertions++;

        if (r.h > parent.h + 0.01f) {
            fprintf(stderr, "    fuzz_align_rect_contained: r.h=%.2f > parent.h=%.2f "
                    "(seed=%u, iter=%d, align=%d)\n",
                    (double)r.h, (double)parent.h, seed, iter, align);
            _test_current_failed = 1;
            return;
        }
        tests_assertions++;

        // Right/bottom edge within parent bounds
        if (r.x + r.w > parent.x + parent.w + 0.01f) {
            fprintf(stderr, "    fuzz_align_rect_contained: right edge %.2f > parent right %.2f "
                    "(seed=%u, iter=%d, align=%d)\n",
                    (double)(r.x + r.w), (double)(parent.x + parent.w), seed, iter, align);
            _test_current_failed = 1;
            return;
        }
        tests_assertions++;

        if (r.y + r.h > parent.y + parent.h + 0.01f) {
            fprintf(stderr, "    fuzz_align_rect_contained: bottom edge %.2f > parent bottom %.2f "
                    "(seed=%u, iter=%d, align=%d)\n",
                    (double)(r.y + r.h), (double)(parent.y + parent.h), seed, iter, align);
            _test_current_failed = 1;
            return;
        }
        tests_assertions++;
    }
}

// ============================================================================
// Fuzz: wlx_rect_intersect invariants
// ============================================================================

TEST(fuzz_rect_intersect_commutative) {
    unsigned int seed = (unsigned int)time(NULL) ^ 0xDEAD0006;
    fuzz_srand(seed);

    for (int iter = 0; iter < FUZZ_ITERATIONS; iter++) {
        WLX_Rect a = wlx_rect(fuzz_randf(-500, 500), fuzz_randf(-500, 500),
                            fuzz_randf(0, 800), fuzz_randf(0, 800));
        WLX_Rect b = wlx_rect(fuzz_randf(-500, 500), fuzz_randf(-500, 500),
                            fuzz_randf(0, 800), fuzz_randf(0, 800));

        WLX_Rect ab = wlx_rect_intersect(a, b);
        WLX_Rect ba = wlx_rect_intersect(b, a);

        // Commutative: intersect(a,b) == intersect(b,a)
        if (fabsf(ab.x - ba.x) > 0.01f || fabsf(ab.y - ba.y) > 0.01f ||
            fabsf(ab.w - ba.w) > 0.01f || fabsf(ab.h - ba.h) > 0.01f) {
            fprintf(stderr, "    fuzz_rect_intersect_commutative: not commutative "
                    "(seed=%u, iter=%d)\n", seed, iter);
            _test_current_failed = 1;
            return;
        }
        tests_assertions++;

        // Result w and h are non-negative
        if (ab.w < -0.01f || ab.h < -0.01f) {
            fprintf(stderr, "    fuzz_rect_intersect_commutative: negative dims "
                    "(seed=%u, iter=%d)\n", seed, iter);
            _test_current_failed = 1;
            return;
        }
        tests_assertions++;

        // Result is contained within both inputs (when non-zero)
        if (ab.w > 0 && ab.h > 0) {
            if (ab.x < a.x - 0.01f || ab.x < b.x - 0.01f ||
                ab.y < a.y - 0.01f || ab.y < b.y - 0.01f) {
                fprintf(stderr, "    fuzz_rect_intersect_commutative: result not contained "
                        "(seed=%u, iter=%d)\n", seed, iter);
                _test_current_failed = 1;
                return;
            }
            if (ab.x + ab.w > a.x + a.w + 0.01f || ab.x + ab.w > b.x + b.w + 0.01f ||
                ab.y + ab.h > a.y + a.h + 0.01f || ab.y + ab.h > b.y + b.h + 0.01f) {
                fprintf(stderr, "    fuzz_rect_intersect_commutative: result exceeds inputs "
                        "(seed=%u, iter=%d)\n", seed, iter);
                _test_current_failed = 1;
                return;
            }
            tests_assertions++;
        }
    }
}

// ============================================================================
// Fuzz: wlx_point_in_rect boundary invariants
// ============================================================================

TEST(fuzz_point_in_rect_boundary) {
    unsigned int seed = (unsigned int)time(NULL) ^ 0xDEAD0007;
    fuzz_srand(seed);

    for (int iter = 0; iter < FUZZ_ITERATIONS; iter++) {
        int rx = fuzz_randi(-500, 500);
        int ry = fuzz_randi(-500, 500);
        int rw = fuzz_randi(1, 800);
        int rh = fuzz_randi(1, 800);

        // Top-left corner is always inside (inclusive)
        ASSERT_TRUE(wlx_point_in_rect(rx, ry, rx, ry, rw, rh));

        // Bottom-right edge is always outside (exclusive)
        ASSERT_FALSE(wlx_point_in_rect(rx + rw, ry + rh, rx, ry, rw, rh));

        // Just inside the right/bottom edges
        ASSERT_TRUE(wlx_point_in_rect(rx + rw - 1, ry + rh - 1, rx, ry, rw, rh));

        // Just outside each edge
        ASSERT_FALSE(wlx_point_in_rect(rx - 1, ry, rx, ry, rw, rh));
        ASSERT_FALSE(wlx_point_in_rect(rx, ry - 1, rx, ry, rw, rh));
    }
}

// ============================================================================
// Suite
// ============================================================================

SUITE(fuzz) {
    RUN_TEST(fuzz_offsets_monotonic);
    RUN_TEST(fuzz_offsets_equal_split);
    RUN_TEST(fuzz_offsets_minmax_respected);
    RUN_TEST(fuzz_offsets_nonnegative_sizes);
    RUN_TEST(fuzz_align_rect_contained);
    RUN_TEST(fuzz_rect_intersect_commutative);
    RUN_TEST(fuzz_point_in_rect_boundary);
}
