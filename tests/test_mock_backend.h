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

static void noop_draw_rect(WLX_Rect r, WLX_Color c, void *user) {
    (void)user;
    (void)r; (void)c;
}

static float _mock_last_border_thick = 0.0f;
static WLX_Color _mock_last_border_color = {0};
static void noop_draw_rect_lines(WLX_Rect r, float thick, WLX_Color c, void *user) {
    (void)user;
    _mock_last_border_thick = thick;
    _mock_last_border_color = c;
    (void)r;
}

static void noop_draw_rect_rounded(WLX_Rect r, float roundness, int segments, WLX_Color c, void *user) {
    (void)user;
    (void)r; (void)roundness; (void)segments; (void)c;
}

static void noop_draw_rect_rounded_lines(WLX_Rect r, float roundness, int segments, float thick, WLX_Color c, void *user) {
    (void)user;
    (void)r; (void)roundness; (void)segments; (void)thick; (void)c;
}

static int _mock_circle_call_count = 0;
static float _mock_last_circle_cx = 0;
static float _mock_last_circle_cy = 0;
static float _mock_last_circle_radius = 0;
static void noop_draw_circle(float cx, float cy, float radius, int segments, WLX_Color c, void *user) {
    (void)user;
    _mock_circle_call_count++;
    _mock_last_circle_cx = cx;
    _mock_last_circle_cy = cy;
    _mock_last_circle_radius = radius;
    (void)segments; (void)c;
}

static int _mock_ring_call_count = 0;
static float _mock_last_ring_cx = 0;
static float _mock_last_ring_cy = 0;
static float _mock_last_ring_inner_r = 0;
static float _mock_last_ring_outer_r = 0;
static void noop_draw_ring(float cx, float cy, float inner_r, float outer_r, int segments, WLX_Color c, void *user) {
    (void)user;
    _mock_ring_call_count++;
    _mock_last_ring_cx = cx;
    _mock_last_ring_cy = cy;
    _mock_last_ring_inner_r = inner_r;
    _mock_last_ring_outer_r = outer_r;
    (void)segments; (void)c;
}

static void noop_draw_line(float x1, float y1, float x2, float y2, float thick, WLX_Color c, void *user) {
    (void)user;
    (void)x1; (void)y1; (void)x2; (void)y2; (void)thick; (void)c;
}

static void noop_draw_text(const char *text, float x, float y, WLX_Text_Style style, void *user) {
    (void)user;
    (void)text; (void)x; (void)y; (void)style;
}

static void noop_draw_texture(WLX_Texture tex, WLX_Rect src, WLX_Rect dst, WLX_Color tint, void *user) {
    (void)user;
    (void)tex; (void)src; (void)dst; (void)tint;
}

static void noop_begin_scissor(WLX_Rect r, void *user) {
    (void)user;
    (void)r;
}

static void noop_end_scissor(void *user) {
    (void)user;
}

static float noop_get_frame_time(void *user) {
    (void)user;
    return 1.0f / 60.0f;  // fixed 60 fps
}

// ----------------------------------------------------------------------------
// Clipboard stub: a growable heap buffer standing in for the system clipboard
// so copy/cut/paste round-trips of any size are testable without a real
// backend (mirrors the adapters' grow-and-reuse transports). Never freed
// (test-process lifetime).
// ----------------------------------------------------------------------------
static char  *_mock_clipboard = NULL;
static size_t _mock_clipboard_cap = 0;

static const char *mock_clipboard_get(void *user) {
    (void)user;
    return _mock_clipboard != NULL ? _mock_clipboard : "";
}

static void mock_clipboard_set(const char *text, size_t len, void *user) {
    (void)user;
    if (!wlx_buf_reserve(&_mock_clipboard, &_mock_clipboard_cap, len + 1)) return;
    if (len > 0) memcpy(_mock_clipboard, text, len);
    _mock_clipboard[len] = '\0';
}

// Seed/inspect the stub clipboard from tests.
static inline void test_set_clipboard(const char *text) {
    mock_clipboard_set(text, text ? strlen(text) : 0, NULL);
}

static inline const char *test_get_clipboard(void) {
    return _mock_clipboard;
}

// ============================================================================
// Deterministic text measurement
// ============================================================================

// Simple proportional model: each character is (font_size * 0.5) wide,
// height equals font_size. Defaults to font_size=10 when style has 0.
static void mock_measure_text(const char *text, WLX_Text_Style style, float *out_w, float *out_h, void *user) {
    (void)user;
    int fs = style.font_size > 0 ? style.font_size : 10;
    size_t len = text ? strlen(text) : 0;
    if (out_w) *out_w = (float)len * (float)fs * 0.5f;
    if (out_h) *out_h = (float)fs;
}

// Content-dependent-height variant: width as above, height equals
// font_size plus the measured byte count, so the height of a measured
// prefix differs from any fixed reference line height. Installed by
// suites that pin which measurement path filled a record's height (the
// per-unit path stores this measured height; the batched advances path
// substitutes the build's uniform line_h).
static void mock_measure_text_tall(const char *text, WLX_Text_Style style, float *out_w, float *out_h, void *user) {
    (void)user;
    int fs = style.font_size > 0 ? style.font_size : 10;
    size_t len = text ? strlen(text) : 0;
    if (out_w) *out_w = (float)len * (float)fs * 0.5f;
    if (out_h) *out_h = (float)fs + (float)len;
}

static inline void test_install_mock_tall_measure(WLX_Context *ctx) {
    ctx->backend.measure_text = mock_measure_text_tall;
}

// Cumulative advances of the same per-byte model: the advance at unit end
// k equals the mock measure of the k-byte prefix, so geometry built through
// the callback is bit-identical to per-unit prefix measuring. Opt-in via
// test_install_mock_advances so existing suites keep exercising the
// per-unit fallback path. Calls are counted so suites can assert which
// builds batched and which stayed per-unit.
static int _mock_advances_calls = 0;

static size_t mock_measure_text_advances(const char *text, size_t len,
        WLX_Text_Style style, const size_t *unit_ends, size_t unit_count,
        float *out_advances, void *user) {
    (void)user;
    (void)text; (void)len;
    _mock_advances_calls++;
    int fs = style.font_size > 0 ? style.font_size : 10;
    for (size_t i = 0; i < unit_count; i++)
        out_advances[i] = (float)unit_ends[i] * (float)fs * 0.5f;
    return unit_count;
}

static inline void test_install_mock_advances(WLX_Context *ctx) {
    ctx->backend.measure_text_advances = mock_measure_text_advances;
}

static inline void test_reset_mock_advances_calls(void) {
    _mock_advances_calls = 0;
}

static inline int test_mock_advances_calls(void) {
    return _mock_advances_calls;
}

// ============================================================================
// Cursor-shape recording stub
// ============================================================================

static WLX_Cursor_Shape _mock_last_cursor = WLX_CURSOR_ARROW;
static int _mock_cursor_calls = 0;

static void mock_set_cursor(WLX_Cursor_Shape shape, void *user) {
    (void)user;
    _mock_last_cursor = shape;
    _mock_cursor_calls++;
}

// Last shape the core pushed through set_cursor (ARROW before any push).
static inline WLX_Cursor_Shape mock_last_cursor(void) {
    return _mock_last_cursor;
}

static inline int mock_cursor_calls(void) {
    return _mock_cursor_calls;
}

static inline void test_reset_mock_cursor(void) {
    _mock_last_cursor = WLX_CURSOR_ARROW;
    _mock_cursor_calls = 0;
}

// ============================================================================
// Mock backend constructor
// ============================================================================

// Built by assignment rather than as a designated compound literal so the
// header also compiles as C++ (the C++ caller gate includes it).
static inline WLX_Backend mock_backend(void) {
    WLX_Backend b;
    memset(&b, 0, sizeof(b));
    b.contract_version        = WLX_BACKEND_CONTRACT_VERSION;
    b.user                    = NULL;
    b.draw_rect               = noop_draw_rect;
    b.draw_rect_lines         = noop_draw_rect_lines;
    b.draw_rect_rounded       = noop_draw_rect_rounded;
    b.draw_rect_rounded_lines = noop_draw_rect_rounded_lines;
    b.draw_circle             = noop_draw_circle;
    b.draw_ring               = noop_draw_ring;
    b.draw_line               = noop_draw_line;
    b.draw_text               = noop_draw_text;
    b.measure_text            = mock_measure_text;
    b.draw_texture            = noop_draw_texture;
    b.begin_scissor           = noop_begin_scissor;
    b.end_scissor             = noop_end_scissor;
    b.get_frame_time          = noop_get_frame_time;
    b.clipboard_get           = mock_clipboard_get;
    b.clipboard_set           = mock_clipboard_set;
    b.set_cursor              = mock_set_cursor;
    return b;
}

// ============================================================================
// Draw-stream recorder
// ============================================================================
//
// Records the shape of what a frame draws (rect-bounded primitives and text)
// so two widgets can be compared call-for-call. Install with
// test_stream_install(ctx), reset per frame with test_stream_reset(), and
// compare two recordings with test_stream_equal(). Colors are recorded too,
// so a hover/disabled tint difference shows up.

typedef struct {
    int       kind;     // 1 rect, 2 rect_lines, 3 rounded, 4 rounded_lines, 5 text
    WLX_Rect  rect;     // text: x, y in rect.x/rect.y, len in rect.w
    float     a, b;     // thickness / roundness extras
    WLX_Color color;
} Test_Stream_Cmd;

#define TEST_STREAM_MAX 128
typedef struct {
    Test_Stream_Cmd cmds[TEST_STREAM_MAX];
    int count;
} Test_Stream;

static Test_Stream _test_stream;

static void _test_stream_push(int kind, WLX_Rect r, float a, float b, WLX_Color c) {
    if (_test_stream.count >= TEST_STREAM_MAX) return;
    Test_Stream_Cmd cmd = { kind, r, a, b, c };
    _test_stream.cmds[_test_stream.count++] = cmd;
}
static void _ts_rect(WLX_Rect r, WLX_Color c, void *user) {
    (void)user; _test_stream_push(1, r, 0, 0, c); }
static void _ts_rect_lines(WLX_Rect r, float t, WLX_Color c, void *user) {
    (void)user; _test_stream_push(2, r, t, 0, c); }
static void _ts_rounded(WLX_Rect r, float ro, int seg, WLX_Color c, void *user) {
    (void)user; _test_stream_push(3, r, ro, (float)seg, c); }
static void _ts_rounded_lines(WLX_Rect r, float ro, int seg, float t, WLX_Color c, void *user) {
    (void)user; (void)seg; _test_stream_push(4, r, ro, t, c); }
static void _ts_text(const char *text, float x, float y, WLX_Text_Style st, void *user) {
    (void)user;
    (void)text;
    _test_stream_push(5, wlx_rect(x, y, (float)(text ? strlen(text) : 0), (float)st.font_size), 0, 0, st.color);
}

static inline void test_stream_install(WLX_Context *ctx) {
    ctx->backend.draw_rect               = _ts_rect;
    ctx->backend.draw_rect_lines         = _ts_rect_lines;
    ctx->backend.draw_rect_rounded       = _ts_rounded;
    ctx->backend.draw_rect_rounded_lines = _ts_rounded_lines;
    ctx->backend.draw_text               = _ts_text;
}
static inline void test_stream_reset(void) { _test_stream.count = 0; }
static inline Test_Stream test_stream_take(void) {
    Test_Stream s = _test_stream;
    _test_stream.count = 0;
    return s;
}
static inline bool test_stream_equal(const Test_Stream *a, const Test_Stream *b) {
    if (a->count != b->count) return false;
    for (int i = 0; i < a->count; i++) {
        const Test_Stream_Cmd *x = &a->cmds[i], *y = &b->cmds[i];
        if (x->kind != y->kind) return false;
        if (x->rect.x != y->rect.x || x->rect.y != y->rect.y
                || x->rect.w != y->rect.w || x->rect.h != y->rect.h) return false;
        if (x->a != y->a || x->b != y->b) return false;
        if (x->color.r != y->color.r || x->color.g != y->color.g
                || x->color.b != y->color.b || x->color.a != y->color.a) return false;
    }
    return true;
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

// Begin a frame with full mouse + keyboard state plus modifier bits and OS
// auto-repeat ticks. keys_repeated and modifiers default to none in the simpler
// helpers above (the staged input is zeroed each frame).
static inline void test_frame_begin_full(WLX_Context *ctx, int mx, int my,
                                         bool mouse_down, bool mouse_clicked,
                                         bool mouse_held, float wheel_delta,
                                         const bool keys_down[WLX_KEY_COUNT],
                                         const bool keys_pressed[WLX_KEY_COUNT],
                                         const bool keys_repeated[WLX_KEY_COUNT],
                                         uint32_t modifiers,
                                         const char *text_input) {
    memset(&_test_staged_input, 0, sizeof(_test_staged_input));
    _test_staged_input.mouse_x       = mx;
    _test_staged_input.mouse_y       = my;
    _test_staged_input.mouse_down    = mouse_down;
    _test_staged_input.mouse_clicked = mouse_clicked;
    _test_staged_input.mouse_held    = mouse_held;
    _test_staged_input.wheel_delta   = wheel_delta;
    _test_staged_input.modifiers     = modifiers;
    if (keys_down) {
        memcpy(_test_staged_input.keys_down, keys_down, sizeof(_test_staged_input.keys_down));
    }
    if (keys_pressed) {
        memcpy(_test_staged_input.keys_pressed, keys_pressed, sizeof(_test_staged_input.keys_pressed));
    }
    if (keys_repeated) {
        memcpy(_test_staged_input.keys_repeated, keys_repeated, sizeof(_test_staged_input.keys_repeated));
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

// Begin a frame from a fully caller-populated input state. Widest tier:
// stages the whole struct verbatim, so tests reach every contract field
// (buttons, wheel axes) without another parameter-list helper.
static inline void test_frame_begin_input(WLX_Context *ctx, const WLX_Input_State *input) {
    _test_staged_input = *input;
    wlx_begin(ctx, ctx->rect, _test_input_handler);
}

// End the current frame.
static inline void test_frame_end(WLX_Context *ctx) {
    // Drain any open layouts so wlx_end doesn't assert
    while (ctx->arena.layouts.count > 0) {
        wlx_layout_end(ctx);
    }
    wlx_end(ctx);
}

#endif // TEST_MOCK_BACKEND_H_
