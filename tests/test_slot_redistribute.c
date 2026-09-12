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

// A clamped fixed slot lands within one pixel of its bound. The frame is
// overconstrained (two 300 px slots in 100 px), the AUTO slot sits at a min
// just under .5 and the first PIXELS slot is capped at a fractional max:
// redistribution used to re-read that slot's snapped span (103) and snap
// the rebuilt boundaries again, landing at 29 / 133 - a 104 px span.
TEST(redistribute_clamped_fixed_slot_lands_within_one_px_of_its_max) {
    WLX_Slot_Size sizes[] = {
        (WLX_Slot_Size){ WLX_SIZE_AUTO,   0.0f,   29.499998f, 0.0f },
        (WLX_Slot_Size){ WLX_SIZE_PIXELS, 300.0f, 0.0f,       102.001999f },
        (WLX_Slot_Size){ WLX_SIZE_PIXELS, 300.0f, 0.0f,       0.0f },
    };
    float off[4];
    wlx_compute_offsets(off, 3, 100.0f, 100.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[1], 29.0f, RD_EPS);
    ASSERT_TRUE(off[2] - off[1] <= sizes[1].max + 1.0f);
    ASSERT_EQ_F(off[2] - off[1], 103.0f, RD_EPS);
    ASSERT_EQ_F(off[3] - off[2], 300.0f, RD_EPS);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
}

// CONTENT in HORZ layouts sizes from intrinsic/explicit child widths and is
// supported: the former unsupported-orientation diagnostic must stay gone.
TEST(content_in_horz_layout_no_longer_warns) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _rd_warn_reset(&ctx);

    for (int frame = 0; frame < 2; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin(&ctx, 2, WLX_HORZ,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) });
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    ASSERT_EQ_INT(0, _rd_warn_count);
    wlx_context_destroy(&ctx);
}

// The supported orientation stays quiet.
TEST(content_in_vert_layout_no_warning) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _rd_warn_reset(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_VERT,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) });
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(0, _rd_warn_count);
    wlx_context_destroy(&ctx);
}
#endif // WLX_DEBUG

SUITE(slot_redistribute) {
    RUN_TEST(redistribute_capped_flex_shares_surplus);
    RUN_TEST(redistribute_min_squeeze_fits);
    RUN_TEST(redistribute_chained_minmax_converges);
    RUN_TEST(redistribute_no_constraints_unchanged);
    RUN_TEST(redistribute_residual_overflow_when_mins_exceed);
    RUN_TEST(redistribute_clamped_fixed_slot_lands_within_one_px_of_its_max);
#ifdef WLX_DEBUG
    RUN_TEST(slot_overflow_warns_unclipped);
    RUN_TEST(slot_overflow_suppressed_when_clipped);
    RUN_TEST(slot_overflow_quiet_when_fits);
    RUN_TEST(content_in_horz_layout_no_longer_warns);
    RUN_TEST(content_in_vert_layout_no_warning);
#endif
}
