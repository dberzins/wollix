// test_backend_contract.c - the WLX_Backend v2 contract: the instance
// pointer reaches every callback, a v1 table runs through the shim, and the
// context-level style transform is applied at the backend boundary only.
// Included from test_main.c (single TU build) after test_cmd_replay.c.

// ============================================================================
// Instance pointer
// ============================================================================

static void *_bc_seen_user = NULL;
static int   _bc_user_calls = 0;

static void bc_rec_rect(WLX_Rect r, WLX_Color c, void *user) {
    (void)r; (void)c;
    _bc_seen_user = user;
    _bc_user_calls++;
}
static float bc_frame_time(void *user) {
    _bc_seen_user = user;
    _bc_user_calls++;
    return 1.0f / 60.0f;
}

// Every callback receives the table's `user` member, the void-parameter
// ones included.
TEST(backend_user_pointer_reaches_callbacks) {
    int instance = 7;
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.user = &instance;
    ctx.backend.draw_rect = bc_rec_rect;
    ctx.backend.get_frame_time = bc_frame_time;
    ASSERT_EQ_INT((int)ctx.backend.contract_version, (int)WLX_BACKEND_CONTRACT_VERSION);

    _bc_seen_user = NULL; _bc_user_calls = 0;
    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_TRUE(_bc_seen_user == &instance);          // get_frame_time in wlx_begin
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_widget(&ctx, .back_color = WLX_RGBA(1, 2, 3, 255));
    wlx_layout_end(&ctx);
    _bc_seen_user = NULL;
    test_frame_end(&ctx);                               // replay: draw_rect
    ASSERT_TRUE(_bc_seen_user == &instance);
    ASSERT_TRUE(_bc_user_calls >= 2);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// v1 shim
// ============================================================================

// A v0.8-shaped table: the same callbacks without the instance pointer.
static int _v1_rect_calls = 0, _v1_text_calls = 0, _v1_frame_calls = 0;
static void v1_draw_rect(WLX_Rect r, WLX_Color c) { (void)r; (void)c; _v1_rect_calls++; }
static void v1_draw_rect_lines(WLX_Rect r, float t, WLX_Color c) { (void)r; (void)t; (void)c; }
static void v1_draw_rect_rounded(WLX_Rect r, float ro, int seg, WLX_Color c) { (void)r; (void)ro; (void)seg; (void)c; }
static void v1_draw_rect_rounded_lines(WLX_Rect r, float ro, int seg, float t, WLX_Color c) { (void)r; (void)ro; (void)seg; (void)t; (void)c; }
static void v1_draw_line(float x1, float y1, float x2, float y2, float t, WLX_Color c) { (void)x1; (void)y1; (void)x2; (void)y2; (void)t; (void)c; }
static void v1_draw_text(const char *text, float x, float y, WLX_Text_Style st) { (void)text; (void)x; (void)y; (void)st; _v1_text_calls++; }
static void v1_measure_text(const char *text, WLX_Text_Style st, float *w, float *h) {
    int fs = st.font_size > 0 ? st.font_size : 10;
    if (w) *w = (float)(text ? strlen(text) : 0) * (float)fs * 0.5f;
    if (h) *h = (float)fs;
}
static void v1_draw_texture(WLX_Texture t, WLX_Rect s, WLX_Rect d, WLX_Color c) { (void)t; (void)s; (void)d; (void)c; }
static void v1_begin_scissor(WLX_Rect r) { (void)r; }
static void v1_end_scissor(void) {}
static float v1_get_frame_time(void) { _v1_frame_calls++; return 1.0f / 60.0f; }

TEST(backend_v1_shim_forwards_and_keeps_null_optionals) {
    static WLX_Backend_V1 v1;
    memset(&v1, 0, sizeof(v1));
    v1.draw_rect = v1_draw_rect;
    v1.draw_rect_lines = v1_draw_rect_lines;
    v1.draw_rect_rounded = v1_draw_rect_rounded;
    v1.draw_rect_rounded_lines = v1_draw_rect_rounded_lines;
    v1.draw_line = v1_draw_line;
    v1.draw_text = v1_draw_text;
    v1.measure_text = v1_measure_text;
    v1.draw_texture = v1_draw_texture;
    v1.begin_scissor = v1_begin_scissor;
    v1.end_scissor = v1_end_scissor;
    v1.get_frame_time = v1_get_frame_time;

    WLX_Context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.backend = wlx_backend_from_v1(&v1);
    ctx.rect = wlx_rect(0, 0, 400, 300);
    ASSERT_EQ_INT((int)ctx.backend.contract_version, (int)WLX_BACKEND_CONTRACT_VERSION);
    ASSERT_TRUE(ctx.backend.user == &v1);
    ASSERT_TRUE(ctx.backend.draw_circle == NULL);          // optional stays optional
    ASSERT_TRUE(ctx.backend.draw_text_slice == NULL);      // legacy text path
    ASSERT_TRUE(ctx.backend.clipboard_get == NULL);
    ASSERT_TRUE(ctx.backend.set_cursor == NULL);

    _v1_rect_calls = _v1_text_calls = _v1_frame_calls = 0;
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_button(&ctx, "shim");
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_TRUE(_v1_frame_calls >= 1);
    ASSERT_TRUE(_v1_rect_calls >= 1);
    ASSERT_TRUE(_v1_text_calls >= 1);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Style transform at the backend boundary
// ============================================================================

static int _st_draw_fs = 0, _st_measure_fs = 0, _st_advances_fs = 0;
static void st_draw_text(const char *text, float x, float y, WLX_Text_Style st, void *user) {
    (void)text; (void)x; (void)y; (void)user;
    _st_draw_fs = st.font_size;
}
static void st_measure_text(const char *text, WLX_Text_Style st, float *w, float *h, void *user) {
    (void)user;
    _st_measure_fs = st.font_size;
    mock_measure_text(text, st, w, h, NULL);
}
static size_t st_measure_advances(const char *text, size_t len, WLX_Text_Style st,
        const size_t *ends, size_t n, float *adv, void *user) {
    _st_advances_fs = st.font_size;
    return mock_measure_text_advances(text, len, st, ends, n, adv, user);
}

static WLX_Text_Style st_double(WLX_Text_Style style, void *user) {
    int *calls = (int *)user;
    if (calls) (*calls)++;
    style.font_size *= 2;
    return style;
}

// The transform is applied immediately before every text callback, and
// nowhere else: the recorded command keeps the nominal style while draw,
// measure and advances all see the transformed one.
TEST(style_transform_applies_at_boundary) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_text = st_draw_text;
    ctx.backend.measure_text = st_measure_text;
    ctx.backend.measure_text_advances = st_measure_advances;
    int calls = 0;
    wlx_set_style_transform(&ctx, st_double, &calls);

    _st_draw_fs = _st_measure_fs = _st_advances_fs = 0;
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_label(&ctx, "nominal", .font_size = 10);
    wlx_layout_end(&ctx);
    // Command buffer: nominal.
    WLX_Cmd *cmds = wlx_pool_commands(&ctx);
    int found = 0;
    for (size_t i = 0; i < ctx.arena.commands.count; i++) {
        if (cmds[i].type == WLX_CMD_TEXT) { ASSERT_EQ_INT(cmds[i].data.text.style.font_size, 10); found++; }
    }
    ASSERT_TRUE(found >= 1);
    test_frame_end(&ctx);
    // Backend: transformed, for measure (during the label) and draw (replay).
    ASSERT_EQ_INT(_st_measure_fs, 20);
    ASSERT_EQ_INT(_st_draw_fs, 20);
    ASSERT_TRUE(calls >= 2);

    // The advances path too, through the batch helper.
    WLX_Text_Measure_Args args = { .ctx = &ctx, .text = "abc", .length = 3,
                                   .style = { .font_size = 10 }, .tab_advance = 0.0f, .line_h = 10.0f };
    size_t ends[3];
    float adv[3];
    size_t first_tab = SIZE_MAX;
    size_t units = wlx_text_measure_advances_batch(&args, 0, 0.0f, 3, ends, adv, &first_tab);
    ASSERT_EQ_INT((int)units, 3);
    ASSERT_EQ_INT(_st_advances_fs, 20);
    ASSERT_EQ_F(adv[2], 3.0f * 20.0f * 0.5f, 0.001f);

    // Clearing restores nominal at the backend.
    wlx_set_style_transform(&ctx, NULL, NULL);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_label(&ctx, "nominal", .font_size = 10);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(_st_measure_fs, 10);
    ASSERT_EQ_INT(_st_draw_fs, 10);
    wlx_context_destroy(&ctx);
}

static WLX_Text_Style st_identity(WLX_Text_Style style, void *user) { (void)user; return style; }

// Setting the transform again, even to the same function, is a
// measurement-environment change: the editor's retained geometry clears.
TEST(style_transform_reset_clears_geometry) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    char buf[512];
    size_t len = gc_fill_lines(buf, sizeof(buf), 30);
    for (int i = 0; i < 3; i++) gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    WLX_Text_Geom_Store *s = &gc_index(&ctx)->geom;
    ASSERT_TRUE(s->count > 0);

    wlx_set_style_transform(&ctx, st_identity, NULL);
    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    // The store was cleared and rebuilt for the visible window only; a
    // second identical set clears it again.
    size_t after_set = s->count;
    ASSERT_TRUE(after_set > 0);
    wlx_set_style_transform(&ctx, st_identity, NULL);
    ASSERT_EQ_INT((int)s->env.transform_generation + 1, (int)ctx.style_transform_generation);
    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    ASSERT_EQ_INT((int)s->env.transform_generation, (int)ctx.style_transform_generation);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(backend_contract) {
    RUN_TEST(backend_user_pointer_reaches_callbacks);
    RUN_TEST(backend_v1_shim_forwards_and_keeps_null_optionals);
    RUN_TEST(style_transform_applies_at_boundary);
    RUN_TEST(style_transform_reset_clears_geometry);
}
