// test_slot_style.c - tests for uniform slot/cell decoration and per-slot/cell
// style overrides (wlx_slot_style, wlx_grid_cell, wlx_grid_cell_style, WLX_Slot_Style_Opt).
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

// ============================================================================
// Recording backend (ss_ prefix to avoid collision with test_rounding.c)
// ============================================================================

#define SS_LOG_CAP 128

static WLX_Color _ss_rects[SS_LOG_CAP];
static int _ss_rect_n = 0;
static WLX_Color _ss_lines[SS_LOG_CAP];
static int _ss_lines_n = 0;

static void ss_draw_rect(WLX_Rect r, WLX_Color c) {
    (void)r;
    if (_ss_rect_n < SS_LOG_CAP) _ss_rects[_ss_rect_n++] = c;
}

static void ss_draw_rect_lines(WLX_Rect r, float thick, WLX_Color c) {
    (void)r; (void)thick;
    if (_ss_lines_n < SS_LOG_CAP) _ss_lines[_ss_lines_n++] = c;
}

static void ss_reset(void) {
    _ss_rect_n  = 0;
    _ss_lines_n = 0;
}

static WLX_Backend ss_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_rect       = ss_draw_rect;
    b.draw_rect_lines = ss_draw_rect_lines;
    return b;
}

static int ss_count_rects(WLX_Color c) {
    int n = 0;
    for (int i = 0; i < _ss_rect_n; i++) {
        if (_ss_rects[i].r == c.r && _ss_rects[i].g == c.g &&
            _ss_rects[i].b == c.b && _ss_rects[i].a == c.a) n++;
    }
    return n;
}

static int ss_count_lines(WLX_Color c) {
    int n = 0;
    for (int i = 0; i < _ss_lines_n; i++) {
        if (_ss_lines[i].r == c.r && _ss_lines[i].g == c.g &&
            _ss_lines[i].b == c.b && _ss_lines[i].a == c.a) n++;
    }
    return n;
}

// Unique marker colors — chosen so they cannot collide with WGT_C.
static const WLX_Color SLOT_BG  = {20,  180, 20,  255};  // uniform bg
static const WLX_Color SLOT_OVR = {180, 20,  20,  255};  // per-slot override
static const WLX_Color BORD_C   = {20,  20,  180, 255};  // border
static const WLX_Color GRID_C   = {180, 20,  180, 255};  // layout border
static const WLX_Color WGT_C    = {90,  90,  90,  90};   // widget fill

static void ss_ctx_init(WLX_Context *ctx, float w, float h) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->backend = ss_backend();
    ctx->theme   = &wlx_theme_dark;
    ctx->rect    = wlx_rect(0, 0, w, h);
}

// ============================================================================
// Tests
// ============================================================================

// No slot decoration fields -> no bg/border draws for slots.
TEST(slot_style_no_decoration_no_draw) {
    ss_reset();
    WLX_Context ctx;
    ss_ctx_init(&ctx, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);

    static const WLX_Slot_Size rows[] = { WLX_SLOT_PX(50), WLX_SLOT_PX(50) };
    static const WLX_Slot_Size cols[] = { WLX_SLOT_PX(50), WLX_SLOT_PX(50) };
    wlx_grid_begin(&ctx, 2, 2, .row_sizes = rows, .col_sizes = cols);
        for (int i = 0; i < 4; i++) {
            wlx_widget(&ctx, .color = WGT_C);
        }
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(ss_count_rects(SLOT_BG),  0);
    ASSERT_EQ_INT(ss_count_rects(SLOT_OVR), 0);
    ASSERT_EQ_INT(ss_count_lines(BORD_C),   0);
    wlx_context_destroy(&ctx);
}

// Uniform slot_back_color on a grid: one bg rect drawn per cell.
TEST(slot_style_grid_uniform_bg) {
    ss_reset();
    WLX_Context ctx;
    ss_ctx_init(&ctx, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);

    static const WLX_Slot_Size rows[] = { WLX_SLOT_PX(50), WLX_SLOT_PX(50) };
    static const WLX_Slot_Size cols[] = { WLX_SLOT_PX(50), WLX_SLOT_PX(50) };
    wlx_grid_begin(&ctx, 2, 2, .row_sizes = rows, .col_sizes = cols,
        .slot_back_color = SLOT_BG);
        for (int i = 0; i < 4; i++) {
            wlx_widget(&ctx, .color = WGT_C);
        }
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(ss_count_rects(SLOT_BG), 4);
    wlx_context_destroy(&ctx);
}

// Uniform slot_border on a grid: one border rect_lines drawn per cell.
TEST(slot_style_grid_uniform_border) {
    ss_reset();
    WLX_Context ctx;
    ss_ctx_init(&ctx, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);

    static const WLX_Slot_Size rows[] = { WLX_SLOT_PX(50), WLX_SLOT_PX(50) };
    static const WLX_Slot_Size cols[] = { WLX_SLOT_PX(50), WLX_SLOT_PX(50) };
    wlx_grid_begin(&ctx, 2, 2, .row_sizes = rows, .col_sizes = cols,
        .slot_border_color = BORD_C, .slot_border_width = 1.0f);
        for (int i = 0; i < 4; i++) {
            wlx_widget(&ctx, .color = WGT_C);
        }
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(ss_count_lines(BORD_C), 4);
    wlx_context_destroy(&ctx);
}

// Grid border uses border_color/border_width and is independent from cell borders.
TEST(slot_style_grid_layout_border_only) {
    ss_reset();
    WLX_Context ctx;
    ss_ctx_init(&ctx, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);

    static const WLX_Slot_Size rows[] = { WLX_SLOT_PX(50), WLX_SLOT_PX(50) };
    static const WLX_Slot_Size cols[] = { WLX_SLOT_PX(50), WLX_SLOT_PX(50) };
    wlx_grid_begin(&ctx, 2, 2, .row_sizes = rows, .col_sizes = cols,
        .border_color = GRID_C, .border_width = 1.0f);
        for (int i = 0; i < 4; i++) {
            wlx_widget(&ctx, .color = WGT_C);
        }
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(ss_count_lines(GRID_C), 1);
    ASSERT_EQ_INT(ss_count_lines(BORD_C), 0);
    wlx_context_destroy(&ctx);
}

// Grid border and per-cell borders should be independently configurable.
TEST(slot_style_grid_layout_and_cell_borders) {
    ss_reset();
    WLX_Context ctx;
    ss_ctx_init(&ctx, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);

    static const WLX_Slot_Size rows[] = { WLX_SLOT_PX(50), WLX_SLOT_PX(50) };
    static const WLX_Slot_Size cols[] = { WLX_SLOT_PX(50), WLX_SLOT_PX(50) };
    wlx_grid_begin(&ctx, 2, 2, .row_sizes = rows, .col_sizes = cols,
        .border_color = GRID_C, .border_width = 1.0f,
        .slot_border_color = BORD_C, .slot_border_width = 1.0f);
        for (int i = 0; i < 4; i++) {
            wlx_widget(&ctx, .color = WGT_C);
        }
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(ss_count_lines(GRID_C), 1);
    ASSERT_EQ_INT(ss_count_lines(BORD_C), 4);
    wlx_context_destroy(&ctx);
}

// Uniform slot_back_color on a linear layout: one bg rect drawn per slot.
TEST(slot_style_linear_uniform) {
    ss_reset();
    WLX_Context ctx;
    ss_ctx_init(&ctx, 300, 150);
    test_frame_begin(&ctx, 0, 0, false, false);

    static const WLX_Slot_Size slots[] = {
        WLX_SLOT_PX(50), WLX_SLOT_PX(50), WLX_SLOT_PX(50)
    };
    wlx_layout_begin(&ctx, 3, WLX_VERT, .sizes = slots,
        .slot_back_color = SLOT_BG);
        wlx_widget(&ctx, .color = WGT_C, .height = 50);
        wlx_widget(&ctx, .color = WGT_C, .height = 50);
        wlx_widget(&ctx, .color = WGT_C, .height = 50);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(ss_count_rects(SLOT_BG), 3);
    wlx_context_destroy(&ctx);
}

// wlx_grid_cell_style: override applies to exactly one cell, others keep uniform.
TEST(slot_style_grid_cell_style) {
    ss_reset();
    WLX_Context ctx;
    ss_ctx_init(&ctx, 300, 100);
    test_frame_begin(&ctx, 0, 0, false, false);

    static const WLX_Slot_Size rows[] = { WLX_SLOT_PX(50) };
    static const WLX_Slot_Size cols[] = {
        WLX_SLOT_PX(50), WLX_SLOT_PX(50), WLX_SLOT_PX(50)
    };
    wlx_grid_begin(&ctx, 1, 3, .row_sizes = rows, .col_sizes = cols,
        .slot_back_color = SLOT_BG);
        // cell (0,0): uniform
        wlx_widget(&ctx, .color = WGT_C);
        // cell (0,1): override
        wlx_grid_cell_style(&ctx, .back_color = SLOT_OVR);
        wlx_widget(&ctx, .color = WGT_C);
        // cell (0,2): override consumed, back to uniform
        wlx_widget(&ctx, .color = WGT_C);
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(ss_count_rects(SLOT_BG),  2);
    ASSERT_EQ_INT(ss_count_rects(SLOT_OVR), 1);
    wlx_context_destroy(&ctx);
}

// wlx_grid_cell with inline back_color: decoration fields on WLX_Slot_Style_Opt.
TEST(slot_style_grid_cell_opt_decoration) {
    ss_reset();
    WLX_Context ctx;
    ss_ctx_init(&ctx, 300, 100);
    test_frame_begin(&ctx, 0, 0, false, false);

    static const WLX_Slot_Size rows[] = { WLX_SLOT_PX(50) };
    static const WLX_Slot_Size cols[] = {
        WLX_SLOT_PX(50), WLX_SLOT_PX(50), WLX_SLOT_PX(50)
    };
    wlx_grid_begin(&ctx, 1, 3, .row_sizes = rows, .col_sizes = cols,
        .slot_back_color = SLOT_BG);
        // cell (0,0): uniform
        wlx_widget(&ctx, .color = WGT_C);
        // cell (0,1): inline decoration via wlx_grid_cell
        wlx_grid_cell(&ctx, 0, 1, .back_color = SLOT_OVR);
        wlx_widget(&ctx, .color = WGT_C);
        // cell (0,2): override consumed, back to uniform
        wlx_widget(&ctx, .color = WGT_C);
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(ss_count_rects(SLOT_BG),  2);
    ASSERT_EQ_INT(ss_count_rects(SLOT_OVR), 1);
    wlx_context_destroy(&ctx);
}

// wlx_slot_style: override applies to exactly one linear slot, others keep uniform.
TEST(slot_style_linear_slot_style) {
    ss_reset();
    WLX_Context ctx;
    ss_ctx_init(&ctx, 300, 150);
    test_frame_begin(&ctx, 0, 0, false, false);

    static const WLX_Slot_Size slots[] = {
        WLX_SLOT_PX(50), WLX_SLOT_PX(50), WLX_SLOT_PX(50)
    };
    wlx_layout_begin(&ctx, 3, WLX_VERT, .sizes = slots,
        .slot_back_color = SLOT_BG);
        // slot 0: uniform
        wlx_widget(&ctx, .color = WGT_C, .height = 50);
        // slot 1: override
        wlx_slot_style(&ctx, .back_color = SLOT_OVR);
        wlx_widget(&ctx, .color = WGT_C, .height = 50);
        // slot 2: override consumed, back to uniform
        wlx_widget(&ctx, .color = WGT_C, .height = 50);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(ss_count_rects(SLOT_BG),  2);
    ASSERT_EQ_INT(ss_count_rects(SLOT_OVR), 1);
    wlx_context_destroy(&ctx);
}

// Override consumed after one slot: no uniform set, override fires once, then
// slots revert to no decoration.
TEST(slot_style_override_consumed_after_one) {
    ss_reset();
    WLX_Context ctx;
    ss_ctx_init(&ctx, 300, 100);
    test_frame_begin(&ctx, 0, 0, false, false);

    static const WLX_Slot_Size rows[] = { WLX_SLOT_PX(50) };
    static const WLX_Slot_Size cols[] = {
        WLX_SLOT_PX(50), WLX_SLOT_PX(50), WLX_SLOT_PX(50)
    };
    // No uniform slot decoration — only override for cell 1.
    wlx_grid_begin(&ctx, 1, 3, .row_sizes = rows, .col_sizes = cols);
        // cell (0,0): no decoration
        wlx_widget(&ctx, .color = WGT_C);
        // cell (0,1): override fires
        wlx_grid_cell_style(&ctx, .back_color = SLOT_OVR);
        wlx_widget(&ctx, .color = WGT_C);
        // cell (0,2): override consumed — no decoration
        wlx_widget(&ctx, .color = WGT_C);
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(ss_count_rects(SLOT_OVR), 1);  // fired exactly once
    ASSERT_EQ_INT(ss_count_rects(SLOT_BG),  0);  // uniform never set
    wlx_context_destroy(&ctx);
}

// Override beats uniform: cell with override uses override_color, not uniform.
TEST(slot_style_override_wins_over_uniform) {
    ss_reset();
    WLX_Context ctx;
    ss_ctx_init(&ctx, 200, 100);
    test_frame_begin(&ctx, 0, 0, false, false);

    static const WLX_Slot_Size rows[] = { WLX_SLOT_PX(50) };
    static const WLX_Slot_Size cols[] = { WLX_SLOT_PX(50), WLX_SLOT_PX(50) };
    wlx_grid_begin(&ctx, 1, 2, .row_sizes = rows, .col_sizes = cols,
        .slot_back_color = SLOT_BG);
        // cell (0,0): override — must NOT draw SLOT_BG for this cell
        wlx_grid_cell_style(&ctx, .back_color = SLOT_OVR);
        wlx_widget(&ctx, .color = WGT_C);
        // cell (0,1): uniform
        wlx_widget(&ctx, .color = WGT_C);
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);

    // One cell overridden, one uniform — not both SLOT_BG for cell 0
    ASSERT_EQ_INT(ss_count_rects(SLOT_OVR), 1);
    ASSERT_EQ_INT(ss_count_rects(SLOT_BG),  1);  // only cell (0,1) has uniform
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(slot_style) {
    RUN_TEST(slot_style_no_decoration_no_draw);
    RUN_TEST(slot_style_grid_uniform_bg);
    RUN_TEST(slot_style_grid_uniform_border);
    RUN_TEST(slot_style_grid_layout_border_only);
    RUN_TEST(slot_style_grid_layout_and_cell_borders);
    RUN_TEST(slot_style_linear_uniform);
    RUN_TEST(slot_style_grid_cell_style);
    RUN_TEST(slot_style_grid_cell_opt_decoration);
    RUN_TEST(slot_style_linear_slot_style);
    RUN_TEST(slot_style_override_consumed_after_one);
    RUN_TEST(slot_style_override_wins_over_uniform);
}
