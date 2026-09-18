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
// Suite
// ============================================================================

SUITE(backend_contract) {
    RUN_TEST(backend_user_pointer_reaches_callbacks);
    RUN_TEST(backend_v1_shim_forwards_and_keeps_null_optionals);
}
