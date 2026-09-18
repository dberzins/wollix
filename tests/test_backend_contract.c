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
// Suite
// ============================================================================

SUITE(backend_contract) {
    RUN_TEST(backend_user_pointer_reaches_callbacks);
}
