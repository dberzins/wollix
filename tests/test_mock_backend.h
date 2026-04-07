// test_mock_backend.h - no-op WLX_Backend stubs and frame simulation helpers
// Include after wollix.h (with WOLLIX_IMPLEMENTATION) and tests.h.
//
// Provides:
//   mock_backend()                 - fully populated WLX_Backend with no-op draws
//   test_ctx_init(ctx, w, h)      - zero-init a WLX_Context with mock backend
//   test_frame_begin(ctx, ...)    - begin a frame with mouse state
//   test_frame_begin_ex(ctx, ...) - begin a frame with mouse + keyboard state
//   test_frame_end(ctx)           - end a frame

#ifndef TEST_MOCK_BACKEND_H_
#define TEST_MOCK_BACKEND_H_

// ============================================================================
// No-op draw stubs
// ============================================================================

static void noop_draw_rect(WLX_Rect r, WLX_Color c) {
    (void)r; (void)c;
}

static float _mock_last_border_thick = 0.0f;
static WLX_Color _mock_last_border_color = {0};
static void noop_draw_rect_lines(WLX_Rect r, float thick, WLX_Color c) {
    _mock_last_border_thick = thick;
    _mock_last_border_color = c;
    (void)r;
}

static void noop_draw_rect_rounded(WLX_Rect r, float roundness, int segments, WLX_Color c) {
    (void)r; (void)roundness; (void)segments; (void)c;
}

static void noop_draw_line(float x1, float y1, float x2, float y2, float thick, WLX_Color c) {
    (void)x1; (void)y1; (void)x2; (void)y2; (void)thick; (void)c;
}

static void noop_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)text; (void)x; (void)y; (void)style;
}

static void noop_draw_texture(WLX_Texture tex, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    (void)tex; (void)src; (void)dst; (void)tint;
}

static void noop_begin_scissor(WLX_Rect r) {
    (void)r;
}

static void noop_end_scissor(void) {
}

static float noop_get_frame_time(void) {
    return 1.0f / 60.0f;  // fixed 60 fps
}

// ============================================================================
// Deterministic text measurement
// ============================================================================

// Simple proportional model: each character is (font_size * 0.5) wide,
// height equals font_size. Defaults to font_size=10 when style has 0.
static void mock_measure_text(const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
    int fs = style.font_size > 0 ? style.font_size : 10;
    size_t len = text ? strlen(text) : 0;
    if (out_w) *out_w = (float)len * (float)fs * 0.5f;
    if (out_h) *out_h = (float)fs;
}

// ============================================================================
// Mock backend constructor
// ============================================================================

static inline WLX_Backend mock_backend(void) {
    return (WLX_Backend){
        .draw_rect         = noop_draw_rect,
        .draw_rect_lines   = noop_draw_rect_lines,
        .draw_rect_rounded = noop_draw_rect_rounded,
        .draw_line         = noop_draw_line,
        .draw_text         = noop_draw_text,
        .measure_text      = mock_measure_text,
        .draw_texture      = noop_draw_texture,
        .begin_scissor     = noop_begin_scissor,
        .end_scissor       = noop_end_scissor,
        .get_frame_time    = noop_get_frame_time,
    };
}

// ============================================================================
// Context initialization
// ============================================================================

// Zero-initialize a WLX_Context, attach mock backend, and set the root rect.
// Call once before a sequence of test_frame_begin/end pairs.
static inline void test_ctx_init(WLX_Context *ctx, float w, float h) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->backend = mock_backend();
    ctx->rect = wlx_rect(0, 0, w, h);
}

// ============================================================================
// Frame simulation - staged input
// ============================================================================

// Internal: cached input state set before wlx_begin's input handler callback.
static WLX_Input_State _test_staged_input = {0};

static void _test_input_handler(WLX_Context *ctx) {
    ctx->input = _test_staged_input;
}

// Begin a frame with the given mouse state.
// mouse_down  = button is currently held this frame
// mouse_clicked = button transitioned to down this frame (one-shot)
static inline void test_frame_begin(WLX_Context *ctx, int mx, int my,
                                     bool mouse_down, bool mouse_clicked) {
    memset(&_test_staged_input, 0, sizeof(_test_staged_input));
    _test_staged_input.mouse_x       = mx;
    _test_staged_input.mouse_y       = my;
    _test_staged_input.mouse_down    = mouse_down;
    _test_staged_input.mouse_clicked = mouse_clicked;
    _test_staged_input.mouse_held    = mouse_down;
    wlx_begin(ctx, ctx->rect, _test_input_handler);
}

// Begin a frame with full mouse + keyboard state.
static inline void test_frame_begin_ex(WLX_Context *ctx, int mx, int my,
                                        bool mouse_down, bool mouse_clicked,
                                        bool mouse_held, float wheel_delta,
                                        const bool keys_down[WLX_KEY_COUNT],
                                        const bool keys_pressed[WLX_KEY_COUNT],
                                        const char *text_input) {
    memset(&_test_staged_input, 0, sizeof(_test_staged_input));
    _test_staged_input.mouse_x       = mx;
    _test_staged_input.mouse_y       = my;
    _test_staged_input.mouse_down    = mouse_down;
    _test_staged_input.mouse_clicked = mouse_clicked;
    _test_staged_input.mouse_held    = mouse_held;
    _test_staged_input.wheel_delta   = wheel_delta;
    if (keys_down) {
        memcpy(_test_staged_input.keys_down, keys_down, sizeof(_test_staged_input.keys_down));
    }
    if (keys_pressed) {
        memcpy(_test_staged_input.keys_pressed, keys_pressed, sizeof(_test_staged_input.keys_pressed));
    }
    if (text_input) {
        size_t len = strlen(text_input);
        if (len >= sizeof(_test_staged_input.text_input))
            len = sizeof(_test_staged_input.text_input) - 1;
        memcpy(_test_staged_input.text_input, text_input, len);
        _test_staged_input.text_input[len] = '\0';
    }
    wlx_begin(ctx, ctx->rect, _test_input_handler);
}

// End the current frame.
static inline void test_frame_end(WLX_Context *ctx) {
    // Drain any open layouts so wlx_end doesn't assert
    while (ctx->layouts.count > 0) {
        wlx_layout_end(ctx);
    }
    wlx_end(ctx);
}

#endif // TEST_MOCK_BACKEND_H_
