// test_cmd_replay.c - deferred draw command replay tests
// Included from test_main.c (single TU build).

// ============================================================================
// Recording backend (prefixed crec_ to avoid test_rounding.c collision)
// ============================================================================

#define CREC_CAP 16384

typedef struct {
    WLX_Cmd_Type type;
    WLX_Rect rect;
    WLX_Color color;
    const char *text;
    float y;
} CRec_Entry;

static CRec_Entry _crec_log[CREC_CAP];
static size_t _crec_count = 0;

static void crec_reset(void) { _crec_count = 0; }

static void crec_draw_rect(WLX_Rect r, WLX_Color c) {
    if (_crec_count < CREC_CAP)
        _crec_log[_crec_count++] = (CRec_Entry){ WLX_CMD_RECT, r, c, NULL, r.y };
}

static void crec_draw_rect_lines(WLX_Rect r, float thick, WLX_Color c) {
    (void)thick;
    if (_crec_count < CREC_CAP)
        _crec_log[_crec_count++] = (CRec_Entry){ WLX_CMD_RECT_LINES, r, c, NULL, r.y };
}

static void crec_draw_rect_rounded(WLX_Rect r, float roundness, int segments, WLX_Color c) {
    (void)roundness; (void)segments;
    if (_crec_count < CREC_CAP)
        _crec_log[_crec_count++] = (CRec_Entry){ WLX_CMD_RECT_ROUNDED, r, c, NULL, r.y };
}

static void crec_draw_rect_rounded_lines(WLX_Rect r, float roundness, int segments, float thick, WLX_Color c) {
    (void)roundness; (void)segments; (void)thick;
    if (_crec_count < CREC_CAP)
        _crec_log[_crec_count++] = (CRec_Entry){ WLX_CMD_RECT_ROUNDED_LINES, r, c, NULL, r.y };
}

static void crec_draw_circle(float cx, float cy, float radius, int segments, WLX_Color c) {
    (void)segments;
    WLX_Rect r = { cx - radius, cy - radius, radius * 2, radius * 2 };
    if (_crec_count < CREC_CAP)
        _crec_log[_crec_count++] = (CRec_Entry){ WLX_CMD_CIRCLE, r, c, NULL, cy };
}

static void crec_draw_ring(float cx, float cy, float inner_r, float outer_r, int segments, WLX_Color c) {
    (void)segments; (void)inner_r;
    WLX_Rect r = { cx - outer_r, cy - outer_r, outer_r * 2, outer_r * 2 };
    if (_crec_count < CREC_CAP)
        _crec_log[_crec_count++] = (CRec_Entry){ WLX_CMD_RING, r, c, NULL, cy };
}

static void crec_draw_line(float x1, float y1, float x2, float y2, float thick, WLX_Color c) {
    (void)thick;
    WLX_Rect r = { x1, y1, x2 - x1, y2 - y1 };
    if (_crec_count < CREC_CAP)
        _crec_log[_crec_count++] = (CRec_Entry){ WLX_CMD_LINE, r, c, NULL, y1 };
}

static void crec_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)style;
    WLX_Rect r = { x, y, 0, 0 };
    if (_crec_count < CREC_CAP)
        _crec_log[_crec_count++] = (CRec_Entry){ WLX_CMD_TEXT, r, {0}, text, y };
}

static void crec_draw_texture(WLX_Texture tex, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    (void)tex; (void)src;
    if (_crec_count < CREC_CAP)
        _crec_log[_crec_count++] = (CRec_Entry){ WLX_CMD_TEXTURE, dst, tint, NULL, dst.y };
}

static void crec_begin_scissor(WLX_Rect r) {
    if (_crec_count < CREC_CAP)
        _crec_log[_crec_count++] = (CRec_Entry){ WLX_CMD_SCISSOR_BEGIN, r, {0}, NULL, r.y };
}

static void crec_end_scissor(void) {
    WLX_Rect z = {0};
    if (_crec_count < CREC_CAP)
        _crec_log[_crec_count++] = (CRec_Entry){ WLX_CMD_SCISSOR_END, z, {0}, NULL, 0 };
}

static inline WLX_Backend crec_backend(void) {
    return (WLX_Backend){
        .draw_rect               = crec_draw_rect,
        .draw_rect_lines         = crec_draw_rect_lines,
        .draw_rect_rounded       = crec_draw_rect_rounded,
        .draw_rect_rounded_lines = crec_draw_rect_rounded_lines,
        .draw_circle             = crec_draw_circle,
        .draw_ring               = crec_draw_ring,
        .draw_line               = crec_draw_line,
        .draw_text               = crec_draw_text,
        .measure_text            = mock_measure_text,
        .draw_texture            = crec_draw_texture,
        .begin_scissor           = crec_begin_scissor,
        .end_scissor             = crec_end_scissor,
        .get_frame_time          = noop_get_frame_time,
    };
}

static inline void crec_ctx_init(WLX_Context *ctx, float w, float h) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->backend = crec_backend();
    ctx->rect = wlx_rect(0, 0, w, h);
}

static inline void crec_frame_begin(WLX_Context *ctx, int mx, int my,
                                     bool mouse_down, bool mouse_clicked) {
    crec_reset();
    memset(&_test_staged_input, 0, sizeof(_test_staged_input));
    _test_staged_input.mouse_x       = mx;
    _test_staged_input.mouse_y       = my;
    _test_staged_input.mouse_down    = mouse_down;
    _test_staged_input.mouse_clicked = mouse_clicked;
    _test_staged_input.mouse_held    = mouse_down;
    wlx_begin(ctx, ctx->rect, _test_input_handler);
}

static inline void crec_frame_end(WLX_Context *ctx) {
    while (ctx->arena.layouts.count > 0) {
        wlx_layout_end(ctx);
    }
    wlx_end(ctx);
}

// Find first recorded entry of a given type starting from `start`.
static inline int crec_find(WLX_Cmd_Type type, size_t start) {
    for (size_t i = start; i < _crec_count; i++) {
        if (_crec_log[i].type == type) return (int)i;
    }
    return -1;
}

// Find first rect with an exact color match starting from `start`.
static inline int crec_find_color(WLX_Color c, size_t start) {
    for (size_t i = start; i < _crec_count; i++) {
        if (_crec_log[i].type == WLX_CMD_RECT &&
            _crec_log[i].color.r == c.r && _crec_log[i].color.g == c.g &&
            _crec_log[i].color.b == c.b && _crec_log[i].color.a == c.a)
            return (int)i;
    }
    return -1;
}

// Find first text entry containing `substr` starting from `start`.
// NOTE: wlx_draw_text_fitted draws per-glyph, so recorded text is usually
// a single char. Use wlx_draw_text directly for multi-char matching.
static inline int crec_find_text(const char *substr, size_t start) {
    for (size_t i = start; i < _crec_count; i++) {
        if (_crec_log[i].type == WLX_CMD_TEXT && _crec_log[i].text != NULL
            && strstr(_crec_log[i].text, substr) != NULL)
            return (int)i;
    }
    return -1;
}

// ============================================================================
// Marker colors (unique per slot so we can find them by color)
// ============================================================================

static const WLX_Color MARK_A = {250, 1, 1, 255};
static const WLX_Color MARK_B = {1, 250, 1, 255};
static const WLX_Color MARK_C = {1, 1, 250, 255};
static const WLX_Color MARK_D = {250, 250, 1, 255};

// Helper: draw a colored rect at the current slot position.
// Advances the slot (widget_begin) and draws a rect to mark position.
static inline void crec_marker(WLX_Context *ctx, WLX_Color c, float h) {
    WLX_Widget_Rect wg = wlx_widget_begin(ctx, (WLX_Widget_Layout){.pos = -1, .span = 1, .height = h});
    wlx_draw_rect(ctx, wg.rect, c);
}

// ============================================================================
// 6.2 Frame-1 correctness: PX slots have correct positions from frame 1
// ============================================================================

TEST(replay_frame1_positions) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 300);

    crec_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_PX(60), WLX_SLOT_PX(40)));

        crec_marker(&ctx, MARK_A, 40);
        crec_marker(&ctx, MARK_B, 60);
        crec_marker(&ctx, MARK_C, 40);

    wlx_layout_end(&ctx);
    wlx_end(&ctx);

    int ai = crec_find_color(MARK_A, 0);
    int bi = crec_find_color(MARK_B, 0);
    int ci = crec_find_color(MARK_C, 0);
    ASSERT_TRUE(ai >= 0);
    ASSERT_TRUE(bi >= 0);
    ASSERT_TRUE(ci >= 0);

    ASSERT_EQ_F(_crec_log[ai].y, 0.0f, 0.5f);
    ASSERT_EQ_F(_crec_log[bi].y, 40.0f, 0.5f);
    ASSERT_EQ_F(_crec_log[ci].y, 100.0f, 0.5f);
}

// ============================================================================
// 6.3 CONTENT convergence: after seeding, positions are correct on frame 2
// ============================================================================

TEST(replay_content_convergence) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 300);

    // Frame 1: seed CONTENT slot
    crec_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_CONTENT, WLX_SLOT_PX(40)));
        crec_marker(&ctx, MARK_A, 40);
        crec_marker(&ctx, MARK_B, 30);
        crec_marker(&ctx, MARK_C, 40);
    crec_frame_end(&ctx);

    // Frame 2: CONTENT now has measured height 30
    crec_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_CONTENT, WLX_SLOT_PX(40)));
        crec_marker(&ctx, MARK_A, 40);
        crec_marker(&ctx, MARK_B, 30);
        crec_marker(&ctx, MARK_C, 40);
    crec_frame_end(&ctx);

    int ai = crec_find_color(MARK_A, 0);
    int bi = crec_find_color(MARK_B, 0);
    int ci = crec_find_color(MARK_C, 0);
    ASSERT_TRUE(ai >= 0);
    ASSERT_TRUE(bi >= 0);
    ASSERT_TRUE(ci >= 0);

    ASSERT_EQ_F(_crec_log[ai].y, 0.0f, 0.5f);
    ASSERT_EQ_F(_crec_log[bi].y, 40.0f, 0.5f);
    ASSERT_EQ_F(_crec_log[ci].y, 70.0f, 0.5f);
}

// ============================================================================
// 6.3b Section-switch test
// ============================================================================

TEST(replay_section_switch) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 300);

    // Frames 1-2: panel A (body=100)
    for (int f = 0; f < 2; f++) {
        crec_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin_s(&ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_PX(30), WLX_SLOT_CONTENT, WLX_SLOT_PX(30)));
            crec_marker(&ctx, MARK_A, 30);
            crec_marker(&ctx, MARK_B, 100);
            crec_marker(&ctx, MARK_C, 30);
        crec_frame_end(&ctx);
    }

    // Frame 3: switch to panel B (body=50)
    crec_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(30), WLX_SLOT_CONTENT, WLX_SLOT_PX(30)));
        crec_marker(&ctx, MARK_A, 30);
        crec_marker(&ctx, MARK_B, 50);
        crec_marker(&ctx, MARK_C, 30);
    crec_frame_end(&ctx);

    int ci = crec_find_color(MARK_C, 0);
    ASSERT_TRUE(ci >= 0);
    // After switch: top(30) + body(50) = 80
    ASSERT_EQ_F(_crec_log[ci].y, 80.0f, 1.0f);
}

// ============================================================================
// 6.4 Interaction parity: button in CONTENT slot is clickable after seeding
// ============================================================================

static bool _crec_interaction_frame(WLX_Context *ctx) {
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_CONTENT));
        wlx_label(ctx, ".", .height = 40);
        bool c = wlx_button(ctx, "X", .height = 30);
    crec_frame_end(ctx);
    return c;
}

TEST(replay_interaction_frame1) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 300);

    // Frame 1: seed CONTENT
    crec_frame_begin(&ctx, 0, 0, false, false);
    _crec_interaction_frame(&ctx);

    // Frame 2: hover
    crec_frame_begin(&ctx, 200, 55, false, false);
    _crec_interaction_frame(&ctx);

    // Frame 3: press
    crec_frame_begin(&ctx, 200, 55, true, true);
    _crec_interaction_frame(&ctx);

    // Frame 4: release -> click fires
    crec_frame_begin(&ctx, 200, 55, false, false);
    bool clicked = _crec_interaction_frame(&ctx);

    ASSERT_TRUE(clicked);
}

// ============================================================================
// 6.5 Z-order / scissor ordering
// ============================================================================

TEST(replay_scissor_ordering) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 300);

    crec_frame_begin(&ctx, 0, 0, false, false);

    wlx_begin_scissor(&ctx, (WLX_Rect){0, 0, 400, 300});
    wlx_begin_scissor(&ctx, (WLX_Rect){10, 10, 200, 200});
    wlx_draw_rect(&ctx, (WLX_Rect){20, 20, 50, 50}, (WLX_Color){255, 0, 0, 255});
    wlx_end_scissor(&ctx);
    wlx_draw_rect(&ctx, (WLX_Rect){300, 0, 50, 50}, (WLX_Color){0, 255, 0, 255});
    wlx_end_scissor(&ctx);

    wlx_end(&ctx);

    ASSERT_TRUE(_crec_count >= 6);

    ASSERT_EQ_INT(_crec_log[0].type, WLX_CMD_SCISSOR_BEGIN);
    ASSERT_EQ_F(_crec_log[0].rect.w, 400.0f, 0.1f);

    ASSERT_EQ_INT(_crec_log[1].type, WLX_CMD_SCISSOR_BEGIN);
    ASSERT_EQ_F(_crec_log[1].rect.w, 200.0f, 0.1f);

    ASSERT_EQ_INT(_crec_log[2].type, WLX_CMD_RECT);
    ASSERT_EQ_INT(_crec_log[2].color.r, 255);

    ASSERT_EQ_INT(_crec_log[3].type, WLX_CMD_SCISSOR_END);

    ASSERT_EQ_INT(_crec_log[4].type, WLX_CMD_RECT);
    ASSERT_EQ_INT(_crec_log[4].color.g, 255);

    ASSERT_EQ_INT(_crec_log[5].type, WLX_CMD_SCISSOR_END);
}

// ============================================================================
// 6.6 Cascade: nested CONTENT layouts converge positions
// ============================================================================

TEST(replay_cascade_nested_content) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 600);

    // Frame 1: seed nested CONTENT
    crec_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(50), WLX_SLOT_CONTENT));
        crec_marker(&ctx, MARK_A, 50);
        wlx_layout_begin_s(&ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_PX(30), WLX_SLOT_CONTENT));
            crec_marker(&ctx, MARK_B, 30);
            wlx_layout_begin_s(&ctx, WLX_VERT,
                WLX_SIZES(WLX_SLOT_PX(20), WLX_SLOT_CONTENT));
                crec_marker(&ctx, MARK_C, 20);
                crec_marker(&ctx, MARK_D, 25);
            wlx_layout_end(&ctx);
        wlx_layout_end(&ctx);
    wlx_layout_end(&ctx);
    wlx_end(&ctx);

    // Frame 2: converged
    crec_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(50), WLX_SLOT_CONTENT));
        crec_marker(&ctx, MARK_A, 50);
        wlx_layout_begin_s(&ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_PX(30), WLX_SLOT_CONTENT));
            crec_marker(&ctx, MARK_B, 30);
            wlx_layout_begin_s(&ctx, WLX_VERT,
                WLX_SIZES(WLX_SLOT_PX(20), WLX_SLOT_CONTENT));
                crec_marker(&ctx, MARK_C, 20);
                crec_marker(&ctx, MARK_D, 25);
            wlx_layout_end(&ctx);
        wlx_layout_end(&ctx);
    wlx_layout_end(&ctx);
    wlx_end(&ctx);

    int ai = crec_find_color(MARK_A, 0);
    int bi = crec_find_color(MARK_B, 0);
    int ci = crec_find_color(MARK_C, 0);
    int di = crec_find_color(MARK_D, 0);

    ASSERT_TRUE(ai >= 0);
    ASSERT_TRUE(bi >= 0);
    ASSERT_TRUE(ci >= 0);
    ASSERT_TRUE(di >= 0);

    // A at y=0, B at y=50, C at y=80, D at y=100
    ASSERT_EQ_F(_crec_log[ai].y, 0.0f, 1.0f);
    ASSERT_EQ_F(_crec_log[bi].y, 50.0f, 1.0f);
    ASSERT_EQ_F(_crec_log[ci].y, 80.0f, 1.0f);
    ASSERT_EQ_F(_crec_log[di].y, 100.0f, 1.0f);
}

// ============================================================================
// 6.7 String lifetime: arena copy preserves text across replay
// ============================================================================

TEST(replay_string_lifetime) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 300);

    crec_frame_begin(&ctx, 0, 0, false, false);
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "Dynamic_%d", 42);
        WLX_Text_Style ts = { .font_size = 16, .spacing = 2, .color = {255,255,255,255} };
        wlx_draw_text(&ctx, buf, 10.0f, 10.0f, ts);
        memset(buf, 'X', sizeof(buf));
    }
    wlx_end(&ctx);

    int idx = crec_find_text("Dynamic_42", 0);
    ASSERT_TRUE(idx >= 0);
    ASSERT_TRUE(strcmp(_crec_log[idx].text, "Dynamic_42") == 0);
}

// ============================================================================
// 6.8 Stress: 10k widgets, no crash or overflow
// ============================================================================

TEST(replay_stress_10k_widgets) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 10000);

    crec_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin_auto(&ctx, WLX_VERT, 20);
    for (int i = 0; i < 10000; i++) {
        wlx_label(&ctx, "W", .height = 20);
    }
    crec_frame_end(&ctx);

    int text_count = 0;
    for (size_t i = 0; i < _crec_count; i++) {
        if (_crec_log[i].type == WLX_CMD_TEXT) text_count++;
    }
    ASSERT_TRUE(text_count >= 10000);
    ASSERT_TRUE(_crec_count <= CREC_CAP);
}

// ============================================================================
// 6.9 Opt-out parity: deferred vs. immediate produce same positions
// ============================================================================

TEST(replay_optout_parity) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 300);

    // Deferred mode
    crec_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(50), WLX_SLOT_PX(100), WLX_SLOT_PX(50)));
        crec_marker(&ctx, MARK_A, 50);
        crec_marker(&ctx, MARK_B, 100);
        crec_marker(&ctx, MARK_C, 50);
    crec_frame_end(&ctx);

    int ai = crec_find_color(MARK_A, 0);
    int bi = crec_find_color(MARK_B, 0);
    int ci = crec_find_color(MARK_C, 0);
    ASSERT_TRUE(ai >= 0);
    ASSERT_TRUE(bi >= 0);
    ASSERT_TRUE(ci >= 0);
    float ya_def = _crec_log[ai].y;
    float yb_def = _crec_log[bi].y;
    float yc_def = _crec_log[ci].y;

    // Immediate mode
    crec_reset();
    memset(&_test_staged_input, 0, sizeof(_test_staged_input));
    wlx_begin_immediate(&ctx, ctx.rect, _test_input_handler);

    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(50), WLX_SLOT_PX(100), WLX_SLOT_PX(50)));
        crec_marker(&ctx, MARK_A, 50);
        crec_marker(&ctx, MARK_B, 100);
        crec_marker(&ctx, MARK_C, 50);
    crec_frame_end(&ctx);

    int ai2 = crec_find_color(MARK_A, 0);
    int bi2 = crec_find_color(MARK_B, 0);
    int ci2 = crec_find_color(MARK_C, 0);
    ASSERT_TRUE(ai2 >= 0);
    ASSERT_TRUE(bi2 >= 0);
    ASSERT_TRUE(ci2 >= 0);

    ASSERT_EQ_F(_crec_log[ai2].y, ya_def, 0.5f);
    ASSERT_EQ_F(_crec_log[bi2].y, yb_def, 0.5f);
    ASSERT_EQ_F(_crec_log[ci2].y, yc_def, 0.5f);
}

// ============================================================================
// 6.10 WLX_NO_RANGE sentinel: root range has WLX_NO_RANGE parent; child
//      ranges have a valid parent index.
// ============================================================================

TEST(replay_no_range_sentinel) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 300);

    crec_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(50), WLX_SLOT_PX(50)));
        crec_marker(&ctx, MARK_A, 50);
        crec_marker(&ctx, MARK_B, 50);
    wlx_layout_end(&ctx);

    wlx_end(&ctx);

    ASSERT_TRUE(ctx.arena.cmd_ranges.count >= 1);
    WLX_Cmd_Range *ranges = wlx_pool_cmd_ranges(&ctx);

    ASSERT_EQ_INT((int)(ranges[0].parent_range_idx == WLX_NO_RANGE), 1);

    for (size_t i = 1; i < ctx.arena.cmd_ranges.count; i++) {
        ASSERT_TRUE(ranges[i].parent_range_idx == WLX_NO_RANGE ||
                    (size_t)ranges[i].parent_range_idx < ctx.arena.cmd_ranges.count);
    }
}

// ============================================================================
// 6.11 Nested layout parentage: all five layout-begin entry points produce
//      valid cmd_range parent links after begin/end.
// ============================================================================

TEST(replay_nested_layout_parentage) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 600);

    crec_frame_begin(&ctx, 0, 0, false, false);

    // Root: fixed linear with five slots, one per entry point under test
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(
        WLX_SLOT_PX(80), WLX_SLOT_PX(50), WLX_SLOT_PX(80),
        WLX_SLOT_PX(80), WLX_SLOT_PX(80)));

        // 1. wlx_layout_begin (fixed linear)
        wlx_layout_begin_s(&ctx, WLX_HORZ, WLX_SIZES(WLX_SLOT_PX(100), WLX_SLOT_PX(100)));
            crec_marker(&ctx, MARK_A, 40);
            crec_marker(&ctx, MARK_B, 40);
        wlx_layout_end(&ctx);

        // 2. wlx_layout_begin_auto
        wlx_layout_begin_auto(&ctx, WLX_HORZ, 50);
            crec_marker(&ctx, MARK_C, 40);
        wlx_layout_end(&ctx);

        // 3. wlx_grid_begin
        wlx_grid_begin(&ctx, 1, 2);
            crec_marker(&ctx, MARK_A, 40);
            crec_marker(&ctx, MARK_B, 40);
        wlx_layout_end(&ctx);

        // 4. wlx_grid_begin_auto
        wlx_grid_begin_auto(&ctx, 2, 40);
            crec_marker(&ctx, MARK_C, 40);
            crec_marker(&ctx, MARK_D, 40);
        wlx_layout_end(&ctx);

        // 5. wlx_grid_begin_auto_tile
        wlx_grid_begin_auto_tile(&ctx, 80.0f, 40.0f);
            crec_marker(&ctx, MARK_A, 40);
        wlx_layout_end(&ctx);

    wlx_layout_end(&ctx);
    wlx_end(&ctx);

    // Every cmd_range with a non-root parent must point to a valid index
    WLX_Cmd_Range *ranges = wlx_pool_cmd_ranges(&ctx);
    size_t n = ctx.arena.cmd_ranges.count;
    ASSERT_TRUE(n >= 6); // root + 5 child layouts + widget ranges

    size_t root_count = 0;
    for (size_t i = 0; i < n; i++) {
        if (ranges[i].parent_range_idx == WLX_NO_RANGE) {
            root_count++;
        } else {
            ASSERT_TRUE((size_t)ranges[i].parent_range_idx < n);
        }
    }
    ASSERT_EQ_INT((int)root_count, 1);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(cmd_replay) {
    RUN_TEST(replay_frame1_positions);
    RUN_TEST(replay_content_convergence);
    RUN_TEST(replay_section_switch);
    RUN_TEST(replay_interaction_frame1);
    RUN_TEST(replay_scissor_ordering);
    RUN_TEST(replay_cascade_nested_content);
    RUN_TEST(replay_string_lifetime);
    RUN_TEST(replay_stress_10k_widgets);
    RUN_TEST(replay_optout_parity);
    RUN_TEST(replay_no_range_sentinel);
    RUN_TEST(replay_nested_layout_parentage);
}
