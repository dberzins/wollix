// test_widget_id.c - widget id derivation pins
//
// Covers:
//   Two stateful call sites on adjacent lines inside one wlx_push_id(row)
//   loop never share an id (an unmixed combine let a line step be undone by
//   a pushed value about 64 lower); id stacks of different depth differ even
//   when they share a suffix; a nested push_id grid is collision-free; ids
//   are stable across frames and split by container scope; interaction and
//   state ids of one site agree; 0 is never handed out; and the WLX_DEBUG
//   site table reports two sites that share one key. Sites in two files
//   stay apart: a header and its .c (.m, .cc / .hh) at any line distance,
//   and two paths of equal length such as ab.c / bA.c. String scopes of
//   different strings stay apart ("ab" / "bA").

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif
#ifndef TEST_MOCK_BACKEND_H_
#include "test_mock_backend.h"
#endif

#include <stdlib.h>

// ============================================================================
// Helpers
// ============================================================================

// A state id from a synthetic call site, so the tests do not depend on this
// file's own line numbers. Every site uses one state size: a collision then
// shares a block instead of tripping the size guard, and the assertions on
// distinctness are what catch it.
static WLX_Id wid_state_at(WLX_Context *ctx, const char *file, int line) {
    return wlx_get_state_impl(ctx, sizeof(int), file, line).id;
}

static int wid_cmp(const void *a, const void *b) {
    WLX_Id x = *(const WLX_Id *)a, y = *(const WLX_Id *)b;
    return (x > y) - (x < y);
}

// Number of equal neighbours after sorting: 0 means all distinct.
static size_t wid_duplicates(WLX_Id *ids, size_t n) {
    qsort(ids, n, sizeof *ids, wid_cmp);
    size_t d = 0;
    for (size_t i = 1; i < n; i++) d += ids[i] == ids[i - 1];
    return d;
}

typedef struct {
    const char *a; int a0;
    const char *b; int b0;
} Wid_File_Pair;

// Collisions between two files: shared ids among 2000 consecutive lines of
// each (from a0 in a, from b0 in b), taken as interaction ids so no state
// block is allocated, plus one if the two first site hashes are equal.
// 0 means the two files never meet.
static size_t wid_file_pair_collisions(const Wid_File_Pair *p) {
    enum { LINES = 2000 };
    static WLX_Id ids[LINES * 2];
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);
    for (int i = 0; i < LINES; i++) {
        ids[i]         = wlx_interaction_make_id(&ctx, p->a, p->a0 + i);
        ids[LINES + i] = wlx_interaction_make_id(&ctx, p->b, p->b0 + i);
    }
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
    return wid_duplicates(ids, LINES * 2)
        + (wlx_hash_id(p->a, p->a0) == wlx_hash_id(p->b, p->b0));
}

// ============================================================================
// Tests
// ============================================================================

TEST(id_adjacent_sites_in_loop_distinct) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    enum { ROWS = 256 };
    static WLX_Id ids[ROWS * 2];
    WLX_Id a65 = 0, b4 = 0;
    for (int r = 0; r < ROWS; r++) {
        wlx_push_id(&ctx, (WLX_Id)r);
        ids[r * 2]     = wid_state_at(&ctx, "app/view.c", 120);
        ids[r * 2 + 1] = wid_state_at(&ctx, "app/view.c", 121);
        if (r == 65) a65 = ids[r * 2];
        if (r == 4)  b4  = ids[r * 2 + 1];
        wlx_pop_id(&ctx);
    }
    test_frame_end(&ctx);

    // The pair that met under the unmixed combine.
    ASSERT_TRUE(a65 != b4);
    ASSERT_EQ_INT((int)wid_duplicates(ids, ROWS * 2), 0);
    wlx_context_destroy(&ctx);
}

TEST(id_stack_depth_distinct) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    const char *f = "app/row.c";
    WLX_Id none = wid_state_at(&ctx, f, 10);

    wlx_push_id(&ctx, 7);
    WLX_Id s7 = wid_state_at(&ctx, f, 10);
    wlx_push_id(&ctx, 0);
    WLX_Id s70 = wid_state_at(&ctx, f, 10);
    wlx_pop_id(&ctx);
    wlx_pop_id(&ctx);

    wlx_push_id(&ctx, 0);
    WLX_Id s0 = wid_state_at(&ctx, f, 10);
    wlx_push_id(&ctx, 7);
    WLX_Id s07 = wid_state_at(&ctx, f, 10);
    wlx_pop_id(&ctx);
    wlx_pop_id(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(s07 != s7);   // [0, 7] vs [7]: the unmixed fold made these equal
    ASSERT_TRUE(s0 != none);  // [0] vs []: likewise
    ASSERT_TRUE(s70 != s7);   // [7, 0] vs [7]
    ASSERT_TRUE(s7 != none);
    ASSERT_TRUE(s07 != s70);
    wlx_context_destroy(&ctx);
}

TEST(id_nested_grid_distinct) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    enum { N = 200 };
    WLX_Id *ids = (WLX_Id *)malloc((size_t)N * N * sizeof *ids);
    ASSERT_TRUE(ids != NULL);
    size_t k = 0;
    for (int r = 0; r < N; r++) {
        wlx_push_id(&ctx, (WLX_Id)r);
        for (int c = 0; c < N; c++) {
            wlx_push_id(&ctx, (WLX_Id)c);
            ids[k++] = wid_state_at(&ctx, "app/cell.c", 40);
            wlx_pop_id(&ctx);
        }
        wlx_pop_id(&ctx);
    }
    test_frame_end(&ctx);

    size_t dups = wid_duplicates(ids, k);
    free(ids);
    ASSERT_EQ_INT((int)dups, 0);
    wlx_context_destroy(&ctx);
}

TEST(id_stable_across_frames_and_scopes) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    WLX_Id frame1, frame2, scope_a, scope_b;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_push_id(&ctx, 3);
    frame1 = wid_state_at(&ctx, "app/stable.c", 5);
    wlx_pop_id(&ctx);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .id = "scope_a");
        scope_a = wid_state_at(&ctx, "app/stable.c", 9);
    wlx_layout_end(&ctx);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .id = "scope_b");
        scope_b = wid_state_at(&ctx, "app/stable.c", 9);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_push_id(&ctx, 3);
    frame2 = wid_state_at(&ctx, "app/stable.c", 5);
    wlx_pop_id(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(frame1 == frame2);
    ASSERT_TRUE(scope_a != scope_b);
    wlx_context_destroy(&ctx);
}

TEST(id_interaction_equals_state) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Id inter_plain = wlx_interaction_make_id(&ctx, "app/same.c", 30);
    WLX_Id state_plain = wid_state_at(&ctx, "app/same.c", 30);
    wlx_push_id(&ctx, 11);
    WLX_Id inter_pushed = wlx_interaction_make_id(&ctx, "app/same.c", 30);
    WLX_Id state_pushed = wid_state_at(&ctx, "app/same.c", 30);
    wlx_pop_id(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(inter_plain == state_plain);
    ASSERT_TRUE(inter_pushed == state_pushed);
    ASSERT_TRUE(inter_plain != inter_pushed);
    wlx_context_destroy(&ctx);
}

TEST(id_zero_reserved) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    int zeros = 0;
    for (int r = 0; r < 64; r++) {
        wlx_push_id(&ctx, (WLX_Id)r);
        for (int l = 1; l <= 64; l++) {
            if (wlx_interaction_make_id(&ctx, "app/zero.c", l) == 0) zeros++;
            if (wid_state_at(&ctx, "app/zero.c", l) == 0) zeros++;
        }
        wlx_pop_id(&ctx);
    }
    test_frame_end(&ctx);

    ASSERT_EQ_INT(zeros, 0);
    wlx_context_destroy(&ctx);
}

// A site hash that adds the line to a linear hash of the path lets two paths
// whose last characters differ by d meet 33 * d lines apart: 165 for .c / .h
// and .m / .h ('h' - 'c' = 'm' - 'h' = 5), 5,610 for .cc / .hh. Each pair
// starts that far apart, so every line of one file faces its partner.
TEST(id_same_stem_sites_distinct) {
    static const Wid_File_Pair pairs[] = {
        { "ui/panel.c",  166,  "ui/panel.h",  1 },
        { "ui/panel.h",  166,  "ui/panel.m",  1 },
        { "ui/panel.cc", 5611, "ui/panel.hh", 1 },
    };
    for (size_t i = 0; i < sizeof pairs / sizeof pairs[0]; i++) {
        ASSERT_EQ_INT((int)wid_file_pair_collisions(&pairs[i]), 0);
    }
}

// A djb2 walk maps two paths of equal length to one value when their
// characters differ by a 33-weighted zero sum ("ab" / "bA"); every line of
// the two files then shares an id. The second pair is two real file names
// that collide that way.
TEST(id_equal_length_paths_distinct) {
    static const Wid_File_Pair pairs[] = {
        { "ui/ab.c",           1, "ui/bA.c",           1 },
        { "cfg/BCM7XXX_PHY.h", 1, "cfg/BCM87XX_PHY.h", 1 },
    };
    for (size_t i = 0; i < sizeof pairs / sizeof pairs[0]; i++) {
        ASSERT_EQ_INT((int)wid_file_pair_collisions(&pairs[i]), 0);
    }
}

// Container .id scopes push wlx_hash_string(id). A djb2 string hash gives
// "ab" and "bA", and most short strings, the value of another string; the
// push mix keeps a collision, so the two scopes were one.
TEST(id_string_scope_distinct) {
    enum { CHARS = 95, N = CHARS * CHARS };
    static WLX_Id hashes[N];
    char s[3] = { 0 };
    size_t k = 0;
    for (int a = 0; a < CHARS; a++) {
        for (int b = 0; b < CHARS; b++) {
            s[0] = (char)(' ' + a);
            s[1] = (char)(' ' + b);
            hashes[k++] = wlx_hash_string(s);
        }
    }
    ASSERT_EQ_INT((int)wid_duplicates(hashes, N), 0);

    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);
    WLX_Id ab, ba;
    wlx_layout_begin(&ctx, 1, WLX_VERT, .id = "ab");
        ab = wid_state_at(&ctx, "app/list.c", 12);
    wlx_layout_end(&ctx);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .id = "bA");
        ba = wid_state_at(&ctx, "app/list.c", 12);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);

    ASSERT_TRUE(ab != ba);
}

#ifdef WLX_DEBUG
TEST(id_debug_cross_site_warns) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_TRUE(ctx.dbg != NULL);

    // Two sites, one key: reported once.
    int before = ctx.dbg->warn_count;
    wlx_dbg_interaction_id(&ctx, 12345, "a.c", 1);
    wlx_dbg_interaction_id(&ctx, 12345, "b.c", 2);
    wlx_dbg_interaction_id(&ctx, 12345, "b.c", 2);
    ASSERT_EQ_INT(ctx.dbg->warn_count - before, 1);

    // One site twice without a push: the existing report, still once.
    before = ctx.dbg->warn_count;
    wlx_dbg_interaction_id(&ctx, 777, "c.c", 3);
    wlx_dbg_interaction_id(&ctx, 777, "c.c", 3);
    wlx_dbg_interaction_id(&ctx, 777, "c.c", 3);
    ASSERT_EQ_INT(ctx.dbg->warn_count - before, 1);

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}
#endif

// ============================================================================
// Suite
// ============================================================================

SUITE(widget_id) {
    RUN_TEST(id_adjacent_sites_in_loop_distinct);
    RUN_TEST(id_stack_depth_distinct);
    RUN_TEST(id_nested_grid_distinct);
    RUN_TEST(id_stable_across_frames_and_scopes);
    RUN_TEST(id_interaction_equals_state);
    RUN_TEST(id_zero_reserved);
    RUN_TEST(id_same_stem_sites_distinct);
    RUN_TEST(id_equal_length_paths_distinct);
    RUN_TEST(id_string_scope_distinct);
#ifdef WLX_DEBUG
    RUN_TEST(id_debug_cross_site_warns);
#endif
}
