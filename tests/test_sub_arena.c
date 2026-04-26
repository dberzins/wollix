// test_sub_arena.c - unit tests for allocator callbacks and sub-arena helpers

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

typedef struct {
    size_t alloc_calls;
    size_t realloc_calls;
    size_t free_calls;
    size_t last_alloc_size;
    size_t last_realloc_old_size;
    size_t last_realloc_new_size;
    size_t last_free_size;
} Test_Sub_Arena_Stats;

static void *test_sub_arena_alloc(size_t size, void *user) {
    Test_Sub_Arena_Stats *stats = (Test_Sub_Arena_Stats *)user;
    stats->alloc_calls++;
    stats->last_alloc_size = size;
    return malloc(size);
}

static void *test_sub_arena_realloc(void *ptr, size_t old_size,
    size_t new_size, void *user)
{
    Test_Sub_Arena_Stats *stats = (Test_Sub_Arena_Stats *)user;
    stats->realloc_calls++;
    stats->last_realloc_old_size = old_size;
    stats->last_realloc_new_size = new_size;
    return realloc(ptr, new_size);
}

static void test_sub_arena_free(void *ptr, size_t size, void *user) {
    Test_Sub_Arena_Stats *stats = (Test_Sub_Arena_Stats *)user;
    stats->free_calls++;
    stats->last_free_size = size;
    free(ptr);
}

TEST(sub_arena_grows_and_preserves_contents) {
    WLX_Sub_Arena arena;
    size_t base;

    wlx_sub_arena_init(&arena, sizeof(int), NULL);

    base = wlx_sub_arena_alloc(&arena, 1);
    ASSERT_EQ_INT(base, 0);
    ASSERT_EQ_INT(arena.capacity, WLX_DA_INIT_CAP);

    *wlx_sub_arena_at(&arena, int, 0) = 42;

    base = wlx_sub_arena_alloc(&arena, WLX_DA_INIT_CAP);
    ASSERT_EQ_INT(base, 1);
    ASSERT_EQ_INT(arena.count, WLX_DA_INIT_CAP + 1);
    ASSERT_EQ_INT(arena.capacity, WLX_DA_INIT_CAP * 2);
    ASSERT_EQ_INT(arena.high_water, WLX_DA_INIT_CAP + 1);
    ASSERT_EQ_INT(*wlx_sub_arena_at(&arena, int, 0), 42);

    wlx_sub_arena_destroy(&arena);
    ASSERT_TRUE(arena.items == NULL);
    ASSERT_EQ_INT(arena.count, 0);
    ASSERT_EQ_INT(arena.capacity, 0);
    ASSERT_EQ_INT(arena.high_water, 0);
}

TEST(sub_arena_reset_keeps_capacity_and_high_water) {
    WLX_Sub_Arena arena;
    size_t capacity;
    size_t high_water;

    wlx_sub_arena_init(&arena, sizeof(int), NULL);
    wlx_sub_arena_alloc(&arena, 16);

    capacity = arena.capacity;
    high_water = arena.high_water;

    wlx_sub_arena_reset(&arena);

    ASSERT_EQ_INT(arena.count, 0);
    ASSERT_EQ_INT(arena.capacity, capacity);
    ASSERT_EQ_INT(arena.high_water, high_water);

    wlx_sub_arena_destroy(&arena);
}

TEST(sub_arena_alloc_bytes_aligns_offsets) {
    WLX_Sub_Arena arena;
    size_t a;
    size_t b;
    size_t c;

    wlx_sub_arena_init(&arena, 1, NULL);

    a = wlx_sub_arena_alloc_bytes(&arena, 1, 1);
    b = wlx_sub_arena_alloc_bytes(&arena, sizeof(uint32_t), 8);
    c = wlx_sub_arena_alloc_bytes(&arena, 2, 4);

    ASSERT_EQ_INT(a, 0);
    ASSERT_EQ_INT(b, 8);
    ASSERT_EQ_INT(c, 12);
    ASSERT_EQ_INT(arena.count, 14);
    ASSERT_EQ_INT(arena.high_water, 14);

    *wlx_sub_arena_bytes_at(&arena, uint8_t, a) = 0x7f;
    *wlx_sub_arena_bytes_at(&arena, uint32_t, b) = 0x11223344u;

    ASSERT_EQ_INT(*wlx_sub_arena_bytes_at(&arena, uint8_t, a), 0x7f);
    ASSERT_EQ_INT(*wlx_sub_arena_bytes_at(&arena, uint32_t, b), 0x11223344u);
    ASSERT_TRUE((((uintptr_t)wlx_sub_arena_bytes_at(&arena, uint32_t, b)) & 7u) == 0);

    wlx_sub_arena_destroy(&arena);
}

TEST(sub_arena_custom_allocator_receives_size_metadata) {
    Test_Sub_Arena_Stats stats = {0};
    WLX_Allocator allocator = {
        .alloc = test_sub_arena_alloc,
        .realloc = test_sub_arena_realloc,
        .free = test_sub_arena_free,
        .user = &stats,
    };
    WLX_Sub_Arena arena;

    wlx_sub_arena_init(&arena, sizeof(uint32_t), &allocator);

    wlx_sub_arena_alloc(&arena, 1);
    *wlx_sub_arena_at(&arena, uint32_t, 0) = 0xAABBCCDDu;

    ASSERT_EQ_INT(stats.alloc_calls, 1);
    ASSERT_EQ_INT(stats.last_alloc_size, WLX_DA_INIT_CAP * sizeof(uint32_t));
    ASSERT_EQ_INT(stats.realloc_calls, 0);

    wlx_sub_arena_alloc(&arena, WLX_DA_INIT_CAP);

    ASSERT_EQ_INT(stats.realloc_calls, 1);
    ASSERT_EQ_INT(stats.last_realloc_old_size, WLX_DA_INIT_CAP * sizeof(uint32_t));
    ASSERT_EQ_INT(stats.last_realloc_new_size, WLX_DA_INIT_CAP * 2 * sizeof(uint32_t));
    ASSERT_EQ_INT(*wlx_sub_arena_at(&arena, uint32_t, 0), 0xAABBCCDDu);

    wlx_sub_arena_destroy(&arena);

    ASSERT_EQ_INT(stats.free_calls, 1);
    ASSERT_EQ_INT(stats.last_free_size, WLX_DA_INIT_CAP * 2 * sizeof(uint32_t));
}

SUITE(sub_arena) {
    RUN_TEST(sub_arena_grows_and_preserves_contents);
    RUN_TEST(sub_arena_reset_keeps_capacity_and_high_water);
    RUN_TEST(sub_arena_alloc_bytes_aligns_offsets);
    RUN_TEST(sub_arena_custom_allocator_receives_size_metadata);
}
