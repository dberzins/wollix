// test_wasm_pool.c - unit tests for the WASM page-pool allocator
//
// The pool helpers live in wollix_wasm.h. They are pure C (only depend on
// malloc/memcpy/size_t) so they can be exercised on the desktop test runner
// even though their target is the bare-wasm32 backend.

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif
#include "wollix_wasm.h"

TEST(wasm_pool_class_clamps_to_min_and_max) {
    ASSERT_EQ_INT(wlx_wasm_pool_class(0), 0);
    ASSERT_EQ_INT(wlx_wasm_pool_class(1), 0);
    ASSERT_EQ_INT(wlx_wasm_pool_class(256), 0);
    ASSERT_EQ_INT(wlx_wasm_pool_class(257), 1);
    ASSERT_EQ_INT(wlx_wasm_pool_class(512), 1);
    ASSERT_EQ_INT(wlx_wasm_pool_class(1024), 2);
    ASSERT_EQ_INT(wlx_wasm_pool_class((size_t)1 << 21),
        WLX_WASM_POOL_CLASSES - 1);
    ASSERT_EQ_INT(wlx_wasm_pool_class_size(0), 256);
    ASSERT_EQ_INT(wlx_wasm_pool_class_size(1), 512);
}

TEST(wasm_pool_alloc_then_free_reuses_block) {
    WLX_Wasm_Pool pool;
    void *a;
    void *b;

    memset(&pool, 0, sizeof(pool));

    a = wlx_wasm_pool_alloc(200, &pool);
    ASSERT_TRUE(a != NULL);
    ASSERT_EQ_INT(pool.alloc_count, 1);
    ASSERT_EQ_INT(pool.reuse_count, 0);
    ASSERT_EQ_INT(pool.bytes_in_use, 256);
    ASSERT_EQ_INT(pool.high_water, 256);

    wlx_wasm_pool_free(a, 200, &pool);
    ASSERT_EQ_INT(pool.free_count, 1);
    ASSERT_EQ_INT(pool.bytes_in_use, 0);
    ASSERT_EQ_INT(pool.high_water, 256);

    b = wlx_wasm_pool_alloc(180, &pool);
    ASSERT_TRUE(b == a);
    ASSERT_EQ_INT(pool.alloc_count, 1);
    ASSERT_EQ_INT(pool.reuse_count, 1);
    ASSERT_EQ_INT(pool.bytes_in_use, 256);

    wlx_wasm_pool_free(b, 180, &pool);
}

TEST(wasm_pool_realloc_same_class_returns_same_pointer) {
    WLX_Wasm_Pool pool;
    void *a;
    void *b;

    memset(&pool, 0, sizeof(pool));

    a = wlx_wasm_pool_alloc(200, &pool);
    ASSERT_TRUE(a != NULL);

    b = wlx_wasm_pool_realloc(a, 200, 250, &pool);
    ASSERT_TRUE(b == a);
    ASSERT_EQ_INT(pool.alloc_count, 1);
    ASSERT_EQ_INT(pool.reuse_count, 0);
    ASSERT_EQ_INT(pool.free_count, 0);

    wlx_wasm_pool_free(b, 250, &pool);
}

TEST(wasm_pool_realloc_grows_to_next_class_and_recycles_old) {
    WLX_Wasm_Pool pool;
    unsigned char *a;
    unsigned char *b;
    unsigned char *c;
    size_t i;

    memset(&pool, 0, sizeof(pool));

    a = (unsigned char *)wlx_wasm_pool_alloc(256, &pool);
    ASSERT_TRUE(a != NULL);
    for (i = 0; i < 256; i++) a[i] = (unsigned char)(i & 0xff);

    b = (unsigned char *)wlx_wasm_pool_realloc(a, 256, 600, &pool);
    ASSERT_TRUE(b != NULL);
    ASSERT_EQ_INT(pool.alloc_count, 2);
    ASSERT_EQ_INT(pool.free_count, 1);
    for (i = 0; i < 256; i++) ASSERT_EQ_INT(b[i], (int)(i & 0xff));

    // The old 256-byte block should have been parked in class 0 free list.
    c = (unsigned char *)wlx_wasm_pool_alloc(200, &pool);
    ASSERT_TRUE((void *)c == (void *)a);
    ASSERT_EQ_INT(pool.reuse_count, 1);

    wlx_wasm_pool_free(b, 600, &pool);
    wlx_wasm_pool_free(c, 200, &pool);
}

TEST(wasm_pool_realloc_zero_frees_block) {
    WLX_Wasm_Pool pool;
    void *a;
    void *r;

    memset(&pool, 0, sizeof(pool));
    a = wlx_wasm_pool_alloc(300, &pool);
    ASSERT_TRUE(a != NULL);

    r = wlx_wasm_pool_realloc(a, 300, 0, &pool);
    ASSERT_TRUE(r == NULL);
    ASSERT_EQ_INT(pool.free_count, 1);
    ASSERT_EQ_INT(pool.bytes_in_use, 0);
}

TEST(wasm_pool_steady_state_drives_reuse_only) {
    WLX_Wasm_Pool pool;
    void *blocks[8];
    size_t frame;
    size_t i;
    size_t baseline_alloc;

    memset(&pool, 0, sizeof(pool));

    // Warm-up: first frame populates the heap.
    for (i = 0; i < 8; i++) {
        blocks[i] = wlx_wasm_pool_alloc(200, &pool);
        ASSERT_TRUE(blocks[i] != NULL);
    }
    for (i = 0; i < 8; i++) wlx_wasm_pool_free(blocks[i], 200, &pool);
    baseline_alloc = pool.alloc_count;

    // Subsequent frames: every allocation must come from the free list.
    for (frame = 0; frame < 16; frame++) {
        for (i = 0; i < 8; i++) {
            blocks[i] = wlx_wasm_pool_alloc(200, &pool);
            ASSERT_TRUE(blocks[i] != NULL);
        }
        for (i = 0; i < 8; i++) wlx_wasm_pool_free(blocks[i], 200, &pool);
    }

    ASSERT_EQ_INT(pool.alloc_count, baseline_alloc);
    ASSERT_EQ_INT(pool.reuse_count, 8 * 16);
}

TEST(wasm_pool_via_sub_arena_recycles_on_growth) {
    WLX_Wasm_Pool pool;
    WLX_Allocator alloc;
    WLX_Sub_Arena sa;
    size_t reuse_after_destroy;

    memset(&pool, 0, sizeof(pool));
    alloc = wlx_wasm_allocator(&pool);

    wlx_sub_arena_init(&sa, sizeof(int), &alloc);

    // Force several growth steps so that realloc walks size classes.
    wlx_sub_arena_alloc(&sa, 4);
    wlx_sub_arena_alloc(&sa, 200);
    wlx_sub_arena_alloc(&sa, 4000);
    ASSERT_TRUE(pool.alloc_count >= 1);

    wlx_sub_arena_destroy(&sa);
    ASSERT_TRUE(pool.free_count >= 1);
    ASSERT_EQ_INT(pool.bytes_in_use, 0);
    reuse_after_destroy = pool.reuse_count;

    // A second sub-arena of similar size should reuse pooled blocks rather
    // than calling malloc again.
    wlx_sub_arena_init(&sa, sizeof(int), &alloc);
    wlx_sub_arena_alloc(&sa, 4);
    wlx_sub_arena_alloc(&sa, 200);
    wlx_sub_arena_alloc(&sa, 4000);
    ASSERT_TRUE(pool.reuse_count > reuse_after_destroy);

    wlx_sub_arena_destroy(&sa);
}

SUITE(wasm_pool) {
    RUN_TEST(wasm_pool_class_clamps_to_min_and_max);
    RUN_TEST(wasm_pool_alloc_then_free_reuses_block);
    RUN_TEST(wasm_pool_realloc_same_class_returns_same_pointer);
    RUN_TEST(wasm_pool_realloc_grows_to_next_class_and_recycles_old);
    RUN_TEST(wasm_pool_realloc_zero_frees_block);
    RUN_TEST(wasm_pool_steady_state_drives_reuse_only);
    RUN_TEST(wasm_pool_via_sub_arena_recycles_on_growth);
}
