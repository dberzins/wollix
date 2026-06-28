// test_slot_redistribute.c - default freeze-and-redistribute for slot min/max
// constraints, plus the debug over-allocation diagnostic.
// Included from test_main.c (single TU build).

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

#define RD_EPS 0.01f

// ============================================================================
// wlx_compute_offsets - redistribution is the default
// ============================================================================

// A capped flex hands its surplus to a flex sibling, so the boundary stays at
// total instead of leaving the gap a single-pass clamp would.
TEST(redistribute_capped_flex_shares_surplus) {
    WLX_Slot_Size sizes[] = {
        WLX_SLOT_FLEX_MAX(1, 100),
        WLX_SLOT_FLEX(1),
    };
    float off[3];
    wlx_compute_offsets(off, 2, 500.0f, 500.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0],   0.0f, RD_EPS);
    ASSERT_EQ_F(off[1], 100.0f, RD_EPS);  // frozen at max
    ASSERT_EQ_F(off[2], 500.0f, RD_EPS);  // sibling absorbs the remaining 400
}

// Topbar-shaped squeeze: a fixed logo slot plus a ranged-flex search slot. The
// flex sits inside its [96,256] range, so the boundary equals total (no overflow).
TEST(redistribute_min_squeeze_fits) {
    WLX_Slot_Size sizes[] = {
        WLX_SLOT_PX(180),
        WLX_SLOT_FLEX_MINMAX(1, 96, 256),
    };
    float off[3];
    wlx_compute_offsets(off, 2, 300.0f, 300.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0],   0.0f, RD_EPS);
    ASSERT_EQ_F(off[1], 180.0f, RD_EPS);
    ASSERT_EQ_F(off[2], 300.0f, RD_EPS);  // search = 120, within [96,256]
}

// Chained caps: two slots freeze at their max, the third flex absorbs the rest.
// Converges within count iterations.
TEST(redistribute_chained_minmax_converges) {
    WLX_Slot_Size sizes[] = {
        WLX_SLOT_FLEX_MAX(1, 80),
        WLX_SLOT_FLEX_MAX(1, 80),
        WLX_SLOT_FLEX(1),
    };
    float off[4];
    wlx_compute_offsets(off, 3, 400.0f, 400.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0],   0.0f, RD_EPS);
    ASSERT_EQ_F(off[1],  80.0f, RD_EPS);
    ASSERT_EQ_F(off[2], 160.0f, RD_EPS);
    ASSERT_EQ_F(off[3], 400.0f, RD_EPS);  // third flex absorbs 240
}

// Regression: a layout with no min/max constraints is untouched by the
// redistribute pass (the has_constraints gate skips it).
TEST(redistribute_no_constraints_unchanged) {
    WLX_Slot_Size sizes[] = {
        WLX_SLOT_PX(100),
        WLX_SLOT_FLEX(1),
        WLX_SLOT_FLEX(2),
    };
    float off[4];
    wlx_compute_offsets(off, 3, 400.0f, 400.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0],   0.0f, RD_EPS);
    ASSERT_EQ_F(off[1], 100.0f, RD_EPS);
    ASSERT_EQ_F(off[2], 200.0f, RD_EPS);  // flex(1) of remaining 300
    ASSERT_EQ_F(off[3], 400.0f, RD_EPS);  // flex(2) of remaining 300
}

// Physical floor: when a fixed slot plus a min-clamped flex exceed total, there
// is no unfrozen pool to absorb the deficit, so the boundary still overflows.
// Redistribution narrows the overflow window; it cannot remove it.
TEST(redistribute_residual_overflow_when_mins_exceed) {
    WLX_Slot_Size sizes[] = {
        WLX_SLOT_PX(200),
        WLX_SLOT_FLEX_MIN(1, 200),
    };
    float off[3];
    wlx_compute_offsets(off, 2, 300.0f, 300.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0],   0.0f, RD_EPS);
    ASSERT_EQ_F(off[1], 200.0f, RD_EPS);
    ASSERT_EQ_F(off[2], 400.0f, RD_EPS);  // 200 + min 200 > 300: still overflows
    ASSERT_TRUE(off[2] > 300.0f);
}

// ============================================================================
// Over-allocation diagnostic (WLX_DEBUG only)
// ============================================================================

#ifdef WLX_DEBUG
static int _rd_warn_count = 0;

static void _rd_warn_cb(const char *file, int line, const char *msg, void *user_data) {
    (void)file;
    (void)line;
    (void)msg;
    (void)user_data;
    _rd_warn_count++;
}

static void _rd_warn_reset(WLX_Context *ctx) {
    _rd_warn_count = 0;
    wlx_dbg_init(ctx);
    ctx->dbg->warn_cb = _rd_warn_cb;
    ctx->dbg->warn_user_data = NULL;
}

// Fixed slots that sum past the layout extent, with no clip, warn once.
TEST(slot_overflow_warns_unclipped) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _rd_warn_reset(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_HORZ,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(300), WLX_SLOT_PX(300) });
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(_rd_warn_count >= 1);
}

// The same over-allocation under .clip = true is contained, so it stays quiet.
TEST(slot_overflow_suppressed_when_clipped) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _rd_warn_reset(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_HORZ, .clip = true,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(300), WLX_SLOT_PX(300) });
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(0, _rd_warn_count);
}

// A layout whose slots fit (capped flex + flex sibling) produces no warning.
TEST(slot_overflow_quiet_when_fits) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _rd_warn_reset(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_HORZ,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX_MAX(1, 100), WLX_SLOT_FLEX(1) });
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(0, _rd_warn_count);
}
#endif // WLX_DEBUG

SUITE(slot_redistribute) {
    RUN_TEST(redistribute_capped_flex_shares_surplus);
    RUN_TEST(redistribute_min_squeeze_fits);
    RUN_TEST(redistribute_chained_minmax_converges);
    RUN_TEST(redistribute_no_constraints_unchanged);
    RUN_TEST(redistribute_residual_overflow_when_mins_exceed);
#ifdef WLX_DEBUG
    RUN_TEST(slot_overflow_warns_unclipped);
    RUN_TEST(slot_overflow_suppressed_when_clipped);
    RUN_TEST(slot_overflow_quiet_when_fits);
#endif
}
