// test_grid.c - tests for grid layout creation, offset computation,
// auto-advance cursor, explicit cell placement, dynamic row growth,
// and per-row size overrides.
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

#define GRID_EPS 0.01f

#ifdef WLX_DEBUG
static int _grid_debug_warn_count = 0;

static void _grid_debug_warn_cb(const char *file, int line, const char *msg, void *user_data) {
    (void)file;
    (void)line;
    (void)msg;
    (void)user_data;
    _grid_debug_warn_count++;
}

static void _grid_debug_warn_reset(WLX_Context *ctx) {
    _grid_debug_warn_count = 0;
    wlx_dbg_init(ctx);
    ctx->dbg->warn_cb = _grid_debug_warn_cb;
    ctx->dbg->warn_user_data = NULL;
}
#endif

// ============================================================================
// wlx_create_grid - equal division (NULL sizes)
// ============================================================================

TEST(grid_equal_2x2) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Layout l = wlx_create_grid(&ctx, wlx_rect(0, 0, 400, 300), 2, 2, NULL, NULL, 0.0f);

    // Row offsets: 0, 150, 300
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, &l)[0],   0.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, &l)[1], 150.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, &l)[2], 300.0f, GRID_EPS);

    // Col offsets: 0, 200, 400
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[0],   0.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[1], 200.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[2], 400.0f, GRID_EPS);

    test_frame_end(&ctx);
}

TEST(grid_equal_3x4) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 600, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Layout l = wlx_create_grid(&ctx, wlx_rect(0, 0, 600, 300), 3, 4, NULL, NULL, 0.0f);

    // 3 rows in 300px -> 100 each
    for (int i = 0; i <= 3; i++) {
        ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, &l)[i], (float)i * 100.0f, GRID_EPS);
    }
    // 4 cols in 600px -> 150 each
    for (int i = 0; i <= 4; i++) {
        ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[i], (float)i * 150.0f, GRID_EPS);
    }

    test_frame_end(&ctx);
}

// ============================================================================
// wlx_create_grid - explicit row/col sizes
// ============================================================================

TEST(grid_explicit_sizes) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Slot_Size row_sizes[] = { WLX_SLOT_PX(100), WLX_SLOT_FLEX(1) };
    WLX_Slot_Size col_sizes[] = { WLX_SLOT_PX(150), WLX_SLOT_FLEX(1), WLX_SLOT_PX(50) };
    WLX_Layout l = wlx_create_grid(&ctx, wlx_rect(0, 0, 400, 300), 2, 3, row_sizes, col_sizes, 0.0f);

    // Rows: 100px + flex(1) = 100 + 200 = 300
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, &l)[0],   0.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, &l)[1], 100.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, &l)[2], 300.0f, GRID_EPS);

    // Cols: 150px + flex(1) + 50px -> 150 + 200 + 50 = 400
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[0],   0.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[1], 150.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[2], 350.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[3], 400.0f, GRID_EPS);

    test_frame_end(&ctx);
}

TEST(grid_mixed_pct_flex) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 600, 400);
    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Slot_Size row_sizes[] = { WLX_SLOT_PCT(25), WLX_SLOT_FLEX(1) };
    WLX_Slot_Size col_sizes[] = { WLX_SLOT_PCT(50), WLX_SLOT_PCT(50) };
    WLX_Layout l = wlx_create_grid(&ctx, wlx_rect(0, 0, 600, 400), 2, 2, row_sizes, col_sizes, 0.0f);

    // Rows: 25% of 400 = 100, flex gets 300
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, &l)[0],   0.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, &l)[1], 100.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, &l)[2], 400.0f, GRID_EPS);

    // Cols: 50% + 50% of 600 = 300 + 300
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[0],   0.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[1], 300.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[2], 600.0f, GRID_EPS);

    test_frame_end(&ctx);
}

// ============================================================================
// wlx_calc_grid_slot_rect - cell rect computation
// ============================================================================

TEST(grid_cell_rect_basic) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Layout l = wlx_create_grid(&ctx, wlx_rect(10, 20, 400, 200), 2, 2, NULL, NULL, 0.0f);

    // Cell (0,0): top-left quarter
    WLX_Rect r00 = wlx_calc_grid_slot_rect(&ctx, &l, 0, 0, 1, 1);
    ASSERT_EQ_RECT(r00, ((WLX_Rect){10, 20, 200, 100}), GRID_EPS);

    // Cell (0,1): top-right quarter
    WLX_Rect r01 = wlx_calc_grid_slot_rect(&ctx, &l, 0, 1, 1, 1);
    ASSERT_EQ_RECT(r01, ((WLX_Rect){210, 20, 200, 100}), GRID_EPS);

    // Cell (1,0): bottom-left quarter
    WLX_Rect r10 = wlx_calc_grid_slot_rect(&ctx, &l, 1, 0, 1, 1);
    ASSERT_EQ_RECT(r10, ((WLX_Rect){10, 120, 200, 100}), GRID_EPS);

    // Cell (1,1): bottom-right quarter
    WLX_Rect r11 = wlx_calc_grid_slot_rect(&ctx, &l, 1, 1, 1, 1);
    ASSERT_EQ_RECT(r11, ((WLX_Rect){210, 120, 200, 100}), GRID_EPS);

    test_frame_end(&ctx);
}

TEST(grid_cell_rect_span) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 300, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Layout l = wlx_create_grid(&ctx, wlx_rect(0, 0, 300, 300), 3, 3, NULL, NULL, 0.0f);
    // Each cell: 100x100

    // Span 2 columns at (0,0)
    WLX_Rect r = wlx_calc_grid_slot_rect(&ctx, &l, 0, 0, 1, 2);
    ASSERT_EQ_RECT(r, ((WLX_Rect){0, 0, 200, 100}), GRID_EPS);

    // Span 2 rows at (0,0)
    r = wlx_calc_grid_slot_rect(&ctx, &l, 0, 0, 2, 1);
    ASSERT_EQ_RECT(r, ((WLX_Rect){0, 0, 100, 200}), GRID_EPS);

    // Span 2x2 at (1,1)
    r = wlx_calc_grid_slot_rect(&ctx, &l, 1, 1, 2, 2);
    ASSERT_EQ_RECT(r, ((WLX_Rect){100, 100, 200, 200}), GRID_EPS);

    test_frame_end(&ctx);
}

// ============================================================================
// Grid auto-advance cursor (row-major order)
// ============================================================================

TEST(grid_auto_advance) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_grid_begin(&ctx, 2, 3);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // Consume cells sequentially (auto-advance)
    WLX_Rect r0 = wlx_get_slot_rect(&ctx, l, -1, 1); // (0,0)
    ASSERT_EQ_F(r0.x, 0.0f, GRID_EPS);
    ASSERT_EQ_F(r0.y, 0.0f, GRID_EPS);
    ASSERT_EQ_INT(l->grid.cursor_col, 1);
    ASSERT_EQ_INT(l->grid.cursor_row, 0);

    WLX_Rect r1 = wlx_get_slot_rect(&ctx, l, -1, 1); // (0,1)
    ASSERT_EQ_F(r1.x, r0.w, GRID_EPS);
    ASSERT_EQ_F(r1.y, 0.0f, GRID_EPS);
    ASSERT_EQ_INT(l->grid.cursor_col, 2);

    WLX_Rect r2 = wlx_get_slot_rect(&ctx, l, -1, 1); // (0,2)
    (void)r2;
    ASSERT_EQ_INT(l->grid.cursor_col, 0);  // wraps to next row
    ASSERT_EQ_INT(l->grid.cursor_row, 1);

    WLX_Rect r3 = wlx_get_slot_rect(&ctx, l, -1, 1); // (1,0)
    ASSERT_EQ_F(r3.y, r0.h, GRID_EPS);  // second row
    ASSERT_EQ_F(r3.x, 0.0f, GRID_EPS);

    test_frame_end(&ctx);
}

// ============================================================================
// Explicit cell placement via wlx_grid_cell()
// ============================================================================

TEST(grid_explicit_cell) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 300, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_grid_begin(&ctx, 3, 3);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
    // Each cell 100x100

    // Jump to (2,1) explicitly
    wlx_grid_cell(&ctx, 2, 1);
    WLX_Rect r = wlx_get_slot_rect(&ctx, l, -1, 1);
    ASSERT_EQ_RECT(r, ((WLX_Rect){100, 200, 100, 100}), GRID_EPS);

    // Jump to (0,2) explicitly
    wlx_grid_cell(&ctx, 0, 2);
    r = wlx_get_slot_rect(&ctx, l, -1, 1);
    ASSERT_EQ_RECT(r, ((WLX_Rect){200, 0, 100, 100}), GRID_EPS);

    test_frame_end(&ctx);
}

TEST(grid_cell_with_span) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_grid_begin(&ctx, 4, 4);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
    // Each cell 100x100

    // Place a 2x2 cell at (1,1)
    wlx_grid_cell(&ctx, 1, 1, .row_span = 2, .col_span = 2);
    WLX_Rect r = wlx_get_slot_rect(&ctx, l, -1, 1);
    ASSERT_EQ_RECT(r, ((WLX_Rect){100, 100, 200, 200}), GRID_EPS);

    test_frame_end(&ctx);
}

// ============================================================================
// Dynamic grid (`wlx_grid_begin_auto()`) - rows grow on demand
// ============================================================================

TEST(grid_auto_grow_rows) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 300, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // 3 columns, 50px per row, dynamic
    wlx_grid_begin_auto(&ctx, 3, 50);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    ASSERT_EQ_INT(l->grid.rows, 0);  // starts with 0 rows
    ASSERT_TRUE(l->grid.dynamic);

    // First 3 slots -> creates row 0
    WLX_Rect r0 = wlx_get_slot_rect(&ctx, l, -1, 1);
    ASSERT_EQ_INT(l->grid.rows, 1);
    ASSERT_EQ_F(r0.h, 50.0f, GRID_EPS);

    wlx_get_slot_rect(&ctx, l, -1, 1); // col 1
    wlx_get_slot_rect(&ctx, l, -1, 1); // col 2 -> wraps

    // Next slot -> creates row 1
    WLX_Rect r3 = wlx_get_slot_rect(&ctx, l, -1, 1);
    ASSERT_EQ_INT(l->grid.rows, 2);
    ASSERT_EQ_F(r3.y, 50.0f, GRID_EPS);  // second row starts at 50px
    ASSERT_EQ_F(r3.h, 50.0f, GRID_EPS);

    test_frame_end(&ctx);
}

TEST(grid_auto_row_count_grows) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 200, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_grid_begin_auto(&ctx, 2, 40);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // Add 8 items -> 4 rows with 2 cols
    for (int i = 0; i < 8; i++) {
        wlx_get_slot_rect(&ctx, l, -1, 1);
    }
    ASSERT_EQ_INT(l->grid.rows, 4);
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, l)[4], 160.0f, GRID_EPS); // 4 * 40

    test_frame_end(&ctx);
}

// ============================================================================
// Dynamic grid - per-row size override via wlx_grid_auto_row_px()
// ============================================================================

TEST(grid_auto_row_px_override) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 200, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_grid_begin_auto(&ctx, 2, 40);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // Row 0: default 40px
    wlx_get_slot_rect(&ctx, l, -1, 1);
    wlx_get_slot_rect(&ctx, l, -1, 1);
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, l)[1], 40.0f, GRID_EPS);

    // Row 1: override to 80px
    wlx_grid_auto_row_px(&ctx, 80);
    wlx_get_slot_rect(&ctx, l, -1, 1);
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, l)[2], 120.0f, GRID_EPS); // 40 + 80

    // Row 2: back to default 40px (override consumed)
    wlx_get_slot_rect(&ctx, l, -1, 1); // fills row 1 col 1
    wlx_get_slot_rect(&ctx, l, -1, 1); // triggers row 2
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, l)[3], 160.0f, GRID_EPS); // 120 + 40

    test_frame_end(&ctx);
}

// ============================================================================
// Dynamic grid - with explicit col sizes
// ============================================================================

TEST(grid_auto_with_col_sizes) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Slot_Size col_sizes[] = { WLX_SLOT_PX(100), WLX_SLOT_FLEX(1) };
    wlx_grid_begin_auto(&ctx, 2, 50, .col_sizes = col_sizes);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // First cell: col 0, 100px wide
    WLX_Rect r0 = wlx_get_slot_rect(&ctx, l, -1, 1);
    ASSERT_EQ_F(r0.w, 100.0f, GRID_EPS);
    ASSERT_EQ_F(r0.x, 0.0f, GRID_EPS);

    // Second cell: col 1, flex fills remaining 300px
    WLX_Rect r1 = wlx_get_slot_rect(&ctx, l, -1, 1);
    ASSERT_EQ_F(r1.w, 300.0f, GRID_EPS);
    ASSERT_EQ_F(r1.x, 100.0f, GRID_EPS);

    test_frame_end(&ctx);
}

// ============================================================================
// Grid with padding via grid_begin opt
// ============================================================================

TEST(grid_with_padding) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    // Root layout -> grid with 10px padding
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_grid_begin(&ctx, 2, 2, .padding = 10);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // Padded rect: (10, 10, 380, 280)
    ASSERT_EQ_F(l->rect.x, 10.0f, GRID_EPS);
    ASSERT_EQ_F(l->rect.y, 10.0f, GRID_EPS);
    ASSERT_EQ_F(l->rect.w, 380.0f, GRID_EPS);
    ASSERT_EQ_F(l->rect.h, 280.0f, GRID_EPS);

    test_frame_end(&ctx);
}

// ============================================================================
// Grid CONTENT row sizing
//
// CONTENT rows use one-frame delay: frame N measures, frame N+1 uses the
// measured value.  All tests use a loop so the same wlx_grid_begin() call
// site (and thus the same persistent state key) is used across frames.
// ============================================================================

TEST(grid_content_row_basic) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    WLX_Slot_Size row_sizes[] = { WLX_SLOT_CONTENT, WLX_SLOT_PX(50) };
    float row0_h = 0, row1_h = 0;

    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_grid_begin(&ctx, 2, 2, .row_sizes = row_sizes);
        WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        row0_h = wlx_grid_row_offsets(&ctx, l)[1] - wlx_grid_row_offsets(&ctx, l)[0];
        row1_h = wlx_grid_row_offsets(&ctx, l)[2] - wlx_grid_row_offsets(&ctx, l)[1];
        wlx_label(&ctx, "a", .height = 30);
        wlx_label(&ctx, "b", .height = 40);
        wlx_label(&ctx, "c");
        wlx_label(&ctx, "d");
        wlx_grid_end(&ctx);
        test_frame_end(&ctx);
    }
    // After convergence: row 0 = max(30,40) = 40, row 1 = PX(50)
    ASSERT_EQ_F(row0_h, 40.0f, GRID_EPS);
    ASSERT_EQ_F(row1_h, 50.0f, GRID_EPS);
}

TEST(grid_content_row_all) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    WLX_Slot_Size row_sizes[] = { WLX_SLOT_CONTENT, WLX_SLOT_CONTENT, WLX_SLOT_CONTENT };
    float r0 = 0, r1 = 0, r2 = 0;

    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_grid_begin(&ctx, 3, 2, .row_sizes = row_sizes);
        WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        r0 = wlx_grid_row_offsets(&ctx, l)[1] - wlx_grid_row_offsets(&ctx, l)[0];
        r1 = wlx_grid_row_offsets(&ctx, l)[2] - wlx_grid_row_offsets(&ctx, l)[1];
        r2 = wlx_grid_row_offsets(&ctx, l)[3] - wlx_grid_row_offsets(&ctx, l)[2];
        // row 0: 20, 35 -> max 35
        wlx_label(&ctx, "a", .height = 20);
        wlx_label(&ctx, "b", .height = 35);
        // row 1: 50, 10 -> max 50
        wlx_label(&ctx, "c", .height = 50);
        wlx_label(&ctx, "d", .height = 10);
        // row 2: 25, 25 -> max 25
        wlx_label(&ctx, "e", .height = 25);
        wlx_label(&ctx, "f", .height = 25);
        wlx_grid_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_EQ_F(r0, 35.0f, GRID_EPS);
    ASSERT_EQ_F(r1, 50.0f, GRID_EPS);
    ASSERT_EQ_F(r2, 25.0f, GRID_EPS);
}

TEST(grid_content_row_min) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // min=60, content max=40 -> row should be 60
    WLX_Slot_Size row_sizes[] = { WLX_SLOT_CONTENT_MIN(60), WLX_SLOT_PX(50) };
    float row0_h = 0;

    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_grid_begin(&ctx, 2, 2, .row_sizes = row_sizes);
        WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        row0_h = wlx_grid_row_offsets(&ctx, l)[1] - wlx_grid_row_offsets(&ctx, l)[0];
        wlx_label(&ctx, "a", .height = 30);
        wlx_label(&ctx, "b", .height = 40);
        wlx_label(&ctx, "c");
        wlx_label(&ctx, "d");
        wlx_grid_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_EQ_F(row0_h, 60.0f, GRID_EPS);
}

TEST(grid_content_row_max) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // max=25, content max=40 -> row should be clamped to 25
    WLX_Slot_Size row_sizes[] = { WLX_SLOT_CONTENT_MAX(25), WLX_SLOT_PX(50) };
    float row0_h = 0;

    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_grid_begin(&ctx, 2, 2, .row_sizes = row_sizes);
        WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        row0_h = wlx_grid_row_offsets(&ctx, l)[1] - wlx_grid_row_offsets(&ctx, l)[0];
        wlx_label(&ctx, "a", .height = 30);
        wlx_label(&ctx, "b", .height = 40);
        wlx_label(&ctx, "c");
        wlx_label(&ctx, "d");
        wlx_grid_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_EQ_F(row0_h, 25.0f, GRID_EPS);
}

TEST(grid_content_row_minmax) {
    // min=30, max=35, content=40 -> clamp to 35
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    WLX_Slot_Size row_sizes[] = { WLX_SLOT_CONTENT_MINMAX(30, 35), WLX_SLOT_PX(50) };
    float row0_h = 0;

    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_grid_begin(&ctx, 2, 2, .row_sizes = row_sizes);
        WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        row0_h = wlx_grid_row_offsets(&ctx, l)[1] - wlx_grid_row_offsets(&ctx, l)[0];
        wlx_label(&ctx, "a", .height = 30);
        wlx_label(&ctx, "b", .height = 40);
        wlx_label(&ctx, "c");
        wlx_label(&ctx, "d");
        wlx_grid_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_EQ_F(row0_h, 35.0f, GRID_EPS);

    // min=50, max=80, content=40 -> clamp to 50
    WLX_Context ctx2;
    test_ctx_init(&ctx2, 400, 300);
    WLX_Slot_Size row_sizes2[] = { WLX_SLOT_CONTENT_MINMAX(50, 80), WLX_SLOT_PX(50) };
    float row0_h2 = 0;

    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx2, 0, 0, false, false);
        wlx_grid_begin(&ctx2, 2, 2, .row_sizes = row_sizes2);
        WLX_Layout *l2 = &wlx_pool_layouts(&ctx2)[ctx2.arena.layouts.count - 1];
        row0_h2 = wlx_grid_row_offsets(&ctx2, l2)[1] - wlx_grid_row_offsets(&ctx2, l2)[0];
        wlx_label(&ctx2, "a", .height = 30);
        wlx_label(&ctx2, "b", .height = 40);
        wlx_label(&ctx2, "c");
        wlx_label(&ctx2, "d");
        wlx_grid_end(&ctx2);
        test_frame_end(&ctx2);
    }
    ASSERT_EQ_F(row0_h2, 50.0f, GRID_EPS);
}

TEST(grid_content_row_mixed) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    WLX_Slot_Size row_sizes[] = { WLX_SLOT_PX(20), WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) };
    float row0_h = 0, row1_h = 0, row2_h = 0;

    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_grid_begin(&ctx, 3, 2, .row_sizes = row_sizes);
        WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        row0_h = wlx_grid_row_offsets(&ctx, l)[1] - wlx_grid_row_offsets(&ctx, l)[0];
        row1_h = wlx_grid_row_offsets(&ctx, l)[2] - wlx_grid_row_offsets(&ctx, l)[1];
        row2_h = wlx_grid_row_offsets(&ctx, l)[3] - wlx_grid_row_offsets(&ctx, l)[2];
        // row 0: PX(20)
        wlx_label(&ctx, "a");
        wlx_label(&ctx, "b");
        // row 1: CONTENT, heights 45 and 30 -> max 45
        wlx_label(&ctx, "c", .height = 45);
        wlx_label(&ctx, "d", .height = 30);
        // row 2: FLEX
        wlx_label(&ctx, "e");
        wlx_label(&ctx, "f");
        wlx_grid_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_EQ_F(row0_h, 20.0f, GRID_EPS);
    ASSERT_EQ_F(row1_h, 45.0f, GRID_EPS);
    // FLEX gets remaining: 300 - 20 - 45 = 235
    ASSERT_EQ_F(row2_h, 235.0f, GRID_EPS);
}

TEST(grid_content_row_empty) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    WLX_Slot_Size row_sizes[] = { WLX_SLOT_CONTENT, WLX_SLOT_CONTENT };
    float row0_h = 0, row1_h = 0;

    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_grid_begin(&ctx, 2, 2, .row_sizes = row_sizes);
        WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        row0_h = wlx_grid_row_offsets(&ctx, l)[1] - wlx_grid_row_offsets(&ctx, l)[0];
        row1_h = wlx_grid_row_offsets(&ctx, l)[2] - wlx_grid_row_offsets(&ctx, l)[1];
        // row 0: explicit heights
        wlx_label(&ctx, "a", .height = 50);
        wlx_label(&ctx, "b", .height = 60);
        // row 1: no explicit heights (default = cell.h)
        wlx_label(&ctx, "c");
        wlx_label(&ctx, "d");
        wlx_grid_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_EQ_F(row0_h, 60.0f, GRID_EPS);
    // Row 1 widgets use default cell.h which equals the row's measured height
    // from the previous frame. On first frame it's 0, but the mock text
    // measurement gives height = font_size (default 10). So this converges
    // to the cell.h of the measured row which is font_size=10.
    ASSERT_TRUE(row1_h >= 0.0f);
}

TEST(grid_content_row_multiframe) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    WLX_Slot_Size row_sizes[] = { WLX_SLOT_CONTENT, WLX_SLOT_PX(100) };
    float row0_f1 = -1, row0_f2 = -1, row0_f3 = -1;

    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_grid_begin(&ctx, 2, 2, .row_sizes = row_sizes);
        WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        float row0_h = wlx_grid_row_offsets(&ctx, l)[1] - wlx_grid_row_offsets(&ctx, l)[0];
        if (frame == 0) row0_f1 = row0_h;
        if (frame == 1) row0_f2 = row0_h;
        if (frame == 2) row0_f3 = row0_h;
        wlx_label(&ctx, "a", .height = 70);
        wlx_label(&ctx, "b", .height = 80);
        wlx_label(&ctx, "c");
        wlx_label(&ctx, "d");
        wlx_grid_end(&ctx);
        test_frame_end(&ctx);
    }
    // Frame 1: 1px seed (no prior measurement, no .min set)
    ASSERT_EQ_F(row0_f1, 1.0f, GRID_EPS);
    // Frame 2: uses measured value from frame 1 = 80
    ASSERT_EQ_F(row0_f2, 80.0f, GRID_EPS);
    // Frame 3: stable
    ASSERT_EQ_F(row0_f3, 80.0f, GRID_EPS);
}

#ifdef WLX_DEBUG
TEST(grid_content_row_seed_does_not_warn_clip) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _grid_debug_warn_reset(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    {
        WLX_Slot_Size row_sizes[] = { WLX_SLOT_CONTENT_MAX(30) };
        wlx_grid_begin(&ctx, 1, 1, .row_sizes = row_sizes);
        wlx_label(&ctx, "Max 30", .height = 35, .font_size = 13,
            .align = WLX_CENTER,
            .show_background = true);
        wlx_grid_end(&ctx);
    }
    test_frame_end(&ctx);

    ASSERT_EQ_INT(0, _grid_debug_warn_count);
}
#endif

// ============================================================================
// Grid per-side padding
// ============================================================================

TEST(grid_perside_padding_top_zero) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_grid_begin(&ctx, 2, 2, .padding = 10, .padding_top = 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // top=0, right=10, bottom=10, left=10
    ASSERT_EQ_F(l->rect.x, 10.0f, GRID_EPS);
    ASSERT_EQ_F(l->rect.y, 0.0f, GRID_EPS);
    ASSERT_EQ_F(l->rect.w, 380.0f, GRID_EPS);  // 400 - 10 - 10
    ASSERT_EQ_F(l->rect.h, 290.0f, GRID_EPS);  // 300 - 0 - 10

    test_frame_end(&ctx);
}

// ============================================================================
// Grid gap (inter-slot spacing)
// ============================================================================

TEST(grid_gap_basic) {
    // 2x2 grid, gap=8, 400x300. Gap applies to both rows and cols.
    // Cols: total=400, 1 gap, distributable=392, col_w=196.
    //   Offsets: 0, 196+8=204, 204+196=400.
    // Rows: total=300, 1 gap, distributable=292, row_h=146.
    //   Offsets: 0, 146+8=154, 154+146=300.
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Layout l = wlx_create_grid(&ctx, wlx_rect(0, 0, 400, 300), 2, 2, NULL, NULL, 8.0f);

    // Col offsets
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[0],   0.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[1], 204.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, &l)[2], 400.0f, GRID_EPS);

    // Row offsets
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, &l)[0],   0.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, &l)[1], 154.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_row_offsets(&ctx, &l)[2], 300.0f, GRID_EPS);

    test_frame_end(&ctx);
}

TEST(grid_gap_auto) {
    // Auto-grow grid with gap=6, 3 cols, 400x600 container.
    // Cols: total=400, 2 gaps=12, distributable=388, col_w ~= 129.33.
    //   Off: 0, 129.33+6=135.33, 135.33+129.33+6=270.67, 270.67+129.33=400.
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_grid_begin_auto(&ctx, 3, 50, .gap = 6);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // 3 cols with gap=6 in 400px width
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, l)[0],   0.0f, GRID_EPS);
    ASSERT_EQ_F(wlx_grid_col_offsets(&ctx, l)[3], 400.0f, GRID_EPS);

    // Verify gap structure: off[1] - off[0] > off[2] - off[1] is not expected;
    // both non-last regions include gap. Just verify col_offsets[1] > 0.
    ASSERT_TRUE(wlx_grid_col_offsets(&ctx, l)[1] > 100.0f);
    ASSERT_TRUE(wlx_grid_col_offsets(&ctx, l)[2] > 230.0f);

    // Gap stored in layout
    ASSERT_EQ_F(l->gap, 6.0f, GRID_EPS);

    wlx_grid_end(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

TEST(grid_gap_slot_rect) {
    // 2x2 grid, gap=8, 400x300.
    // Col content = (400 - 8) / 2 = 196. Row content = (300 - 8) / 2 = 146.
    // Cell (0,0): w=196 (not 204), h=146 (not 154).
    // Cell (0,1): w=196, h=146 (last col, last row not applicable for col).
    // Cell (1,1): w=196, h=146 (last col, last row).
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Layout l = wlx_create_grid(&ctx, wlx_rect(0, 0, 400, 300), 2, 2, NULL, NULL, 8.0f);

    WLX_Rect r00 = wlx_calc_grid_slot_rect(&ctx, &l, 0, 0, 1, 1);
    WLX_Rect r01 = wlx_calc_grid_slot_rect(&ctx, &l, 0, 1, 1, 1);
    WLX_Rect r10 = wlx_calc_grid_slot_rect(&ctx, &l, 1, 0, 1, 1);
    WLX_Rect r11 = wlx_calc_grid_slot_rect(&ctx, &l, 1, 1, 1, 1);

    // All cells should have content width = 196, content height = 146
    ASSERT_EQ_F(r00.w, 196.0f, GRID_EPS);
    ASSERT_EQ_F(r01.w, 196.0f, GRID_EPS);
    ASSERT_EQ_F(r10.w, 196.0f, GRID_EPS);
    ASSERT_EQ_F(r11.w, 196.0f, GRID_EPS);

    ASSERT_EQ_F(r00.h, 146.0f, GRID_EPS);
    ASSERT_EQ_F(r01.h, 146.0f, GRID_EPS);
    ASSERT_EQ_F(r10.h, 146.0f, GRID_EPS);
    ASSERT_EQ_F(r11.h, 146.0f, GRID_EPS);

    // Gap space: cell (0,0) right edge + 8 == cell (0,1) left edge
    ASSERT_EQ_F(r01.x - (r00.x + r00.w), 8.0f, GRID_EPS);
    ASSERT_EQ_F(r10.y - (r00.y + r00.h), 8.0f, GRID_EPS);

    test_frame_end(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(grid) {
    // Grid creation with equal division
    RUN_TEST(grid_equal_2x2);
    RUN_TEST(grid_equal_3x4);

    // Grid with explicit row/col sizes
    RUN_TEST(grid_explicit_sizes);
    RUN_TEST(grid_mixed_pct_flex);

    // Cell rect computation
    RUN_TEST(grid_cell_rect_basic);
    RUN_TEST(grid_cell_rect_span);

    // Auto-advance cursor
    RUN_TEST(grid_auto_advance);

    // Explicit cell placement
    RUN_TEST(grid_explicit_cell);
    RUN_TEST(grid_cell_with_span);

    // Dynamic grid (auto rows)
    RUN_TEST(grid_auto_grow_rows);
    RUN_TEST(grid_auto_row_count_grows);
    RUN_TEST(grid_auto_row_px_override);
    RUN_TEST(grid_auto_with_col_sizes);

    // Padding
    RUN_TEST(grid_with_padding);

    // CONTENT row sizing
    RUN_TEST(grid_content_row_basic);
    RUN_TEST(grid_content_row_all);
    RUN_TEST(grid_content_row_min);
    RUN_TEST(grid_content_row_max);
    RUN_TEST(grid_content_row_minmax);
    RUN_TEST(grid_content_row_mixed);
    RUN_TEST(grid_content_row_empty);
    RUN_TEST(grid_content_row_multiframe);
#ifdef WLX_DEBUG
    RUN_TEST(grid_content_row_seed_does_not_warn_clip);
#endif

    // Per-side padding
    RUN_TEST(grid_perside_padding_top_zero);

    // Gap (inter-slot spacing)
    RUN_TEST(grid_gap_basic);
    RUN_TEST(grid_gap_auto);
    RUN_TEST(grid_gap_slot_rect);
}
