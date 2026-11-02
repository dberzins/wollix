// test_backend_contract.c - the WLX_Backend v2 contract: one scripted frame
// set reaches every callback with the table's instance pointer, a v1 table
// run through the shim logs the same calls as a v2 table, and the
// context-level style transform is applied at the backend boundary only.
// Included from test_main.c (single TU build) after test_cmd_replay.c.

// ============================================================================
// The recording table: every callback, one log
// ============================================================================

// One bit per v2 callback, in table order. A callback added to WLX_Backend
// fails bc_full_table until it is listed here and the script reaches it.
enum {
    BC_DRAW_RECT, BC_DRAW_RECT_LINES, BC_DRAW_RECT_ROUNDED, BC_DRAW_RECT_ROUNDED_LINES,
    BC_DRAW_CIRCLE, BC_DRAW_RING, BC_DRAW_LINE, BC_DRAW_TEXT, BC_MEASURE_TEXT,
    BC_DRAW_TEXTURE, BC_BEGIN_SCISSOR, BC_END_SCISSOR, BC_GET_FRAME_TIME,
    BC_DRAW_TEXT_SLICE, BC_MEASURE_TEXT_SLICE, BC_MEASURE_TEXT_ADVANCES,
    BC_DRAW_SHADOW, BC_DRAW_GLOW, BC_DRAW_GRADIENT_V, BC_CLIPBOARD_GET,
    BC_CLIPBOARD_SET, BC_SET_CURSOR, BC_GET_CONTENT_SCALE, BC_SET_TEXT_INPUT_AREA,
    BC_CALLBACK_COUNT
};
#define BC_V1_CALLBACK_COUNT 22   // the v1 table lacks the last two

// One call with its arguments: floats, ints (colours packed), and the text
// a text callback received, as its length and a hash of its bytes.
typedef struct {
    int      id;
    float    f[8];
    int64_t  i[5];
    size_t   len;
    uint32_t hash;
} BC_Call;

#define BC_LOG_CAP 4096
typedef struct {
    BC_Call calls[BC_LOG_CAP];
    int     count;
} BC_Log;

static uint32_t _bc_hits;           // bit per callback reached
static void    *_bc_expect_user;    // the `user` every v2 callback must see
static int      _bc_bad_user;       // calls that saw another one
static BC_Log  *_bc_log;            // NULL -> hits only
static char     _bc_copied[256];   // what clipboard_set last received

static uint32_t bc_hash(const char *s, size_t n) {
    uint32_t h = 2166136261u;
    for (size_t k = 0; s != NULL && k < n; k++) { h ^= (uint8_t)s[k]; h *= 16777619u; }
    return h;
}
static int64_t bc_rgba(WLX_Color c) {
    return (int64_t)c.r | ((int64_t)c.g << 8) | ((int64_t)c.b << 16) | ((int64_t)c.a << 24);
}

static BC_Call *bc_note(int id, void *user) {
    _bc_hits |= 1u << id;
    if (user != _bc_expect_user) _bc_bad_user++;
    static BC_Call sink;
    if (_bc_log == NULL || _bc_log->count >= BC_LOG_CAP) {
        if (_bc_log != NULL) _bc_log->count++;   // an overflow still fails the compare
        memset(&sink, 0, sizeof(sink));
        return &sink;
    }
    BC_Call *c = &_bc_log->calls[_bc_log->count++];
    memset(c, 0, sizeof(*c));
    c->id = id;
    return c;
}
static void bc_rect_args(BC_Call *c, WLX_Rect r) { c->f[0] = r.x; c->f[1] = r.y; c->f[2] = r.w; c->f[3] = r.h; }
static void bc_text_args(BC_Call *c, const char *t, size_t n, WLX_Text_Style st) {
    c->len = n; c->hash = bc_hash(t, n);
    c->i[0] = (int64_t)st.font; c->i[1] = st.font_size; c->i[2] = bc_rgba(st.color); c->i[3] = st.spacing;
}

static void bcf_draw_rect(WLX_Rect r, WLX_Color col, void *u) {
    BC_Call *c = bc_note(BC_DRAW_RECT, u); bc_rect_args(c, r); c->i[0] = bc_rgba(col);
}
static void bcf_draw_rect_lines(WLX_Rect r, float t, WLX_Color col, void *u) {
    BC_Call *c = bc_note(BC_DRAW_RECT_LINES, u); bc_rect_args(c, r); c->f[4] = t; c->i[0] = bc_rgba(col);
}
static void bcf_draw_rect_rounded(WLX_Rect r, float ro, int seg, WLX_Color col, void *u) {
    BC_Call *c = bc_note(BC_DRAW_RECT_ROUNDED, u); bc_rect_args(c, r); c->f[4] = ro; c->i[0] = seg; c->i[1] = bc_rgba(col);
}
static void bcf_draw_rect_rounded_lines(WLX_Rect r, float ro, int seg, float t, WLX_Color col, void *u) {
    BC_Call *c = bc_note(BC_DRAW_RECT_ROUNDED_LINES, u); bc_rect_args(c, r);
    c->f[4] = ro; c->f[5] = t; c->i[0] = seg; c->i[1] = bc_rgba(col);
}
static void bcf_draw_circle(float cx, float cy, float rad, int seg, WLX_Color col, void *u) {
    BC_Call *c = bc_note(BC_DRAW_CIRCLE, u); c->f[0] = cx; c->f[1] = cy; c->f[2] = rad; c->i[0] = seg; c->i[1] = bc_rgba(col);
}
static void bcf_draw_ring(float cx, float cy, float ri, float ro, int seg, WLX_Color col, void *u) {
    BC_Call *c = bc_note(BC_DRAW_RING, u); c->f[0] = cx; c->f[1] = cy; c->f[2] = ri; c->f[3] = ro;
    c->i[0] = seg; c->i[1] = bc_rgba(col);
}
static void bcf_draw_line(float x1, float y1, float x2, float y2, float t, WLX_Color col, void *u) {
    BC_Call *c = bc_note(BC_DRAW_LINE, u); c->f[0] = x1; c->f[1] = y1; c->f[2] = x2; c->f[3] = y2; c->f[4] = t;
    c->i[0] = bc_rgba(col);
}
static void bcf_draw_text(const char *t, float x, float y, WLX_Text_Style st, void *u) {
    BC_Call *c = bc_note(BC_DRAW_TEXT, u); bc_text_args(c, t, t ? strlen(t) : 0, st); c->f[0] = x; c->f[1] = y;
}
static void bcf_measure_text(const char *t, WLX_Text_Style st, float *w, float *h, void *u) {
    BC_Call *c = bc_note(BC_MEASURE_TEXT, u); bc_text_args(c, t, t ? strlen(t) : 0, st);
    mock_measure_text(t, st, w, h, NULL);
}
static void bcf_draw_texture(WLX_Texture tex, WLX_Rect s, WLX_Rect d, WLX_Color tint, void *u) {
    BC_Call *c = bc_note(BC_DRAW_TEXTURE, u);
    c->f[0] = s.x; c->f[1] = s.y; c->f[2] = s.w; c->f[3] = s.h;
    c->f[4] = d.x; c->f[5] = d.y; c->f[6] = d.w; c->f[7] = d.h;
    c->i[0] = (int64_t)tex.handle; c->i[1] = tex.width; c->i[2] = tex.height; c->i[3] = bc_rgba(tint);
}
static void bcf_begin_scissor(WLX_Rect r, void *u) { bc_rect_args(bc_note(BC_BEGIN_SCISSOR, u), r); }
static void bcf_end_scissor(void *u) { (void)bc_note(BC_END_SCISSOR, u); }
static float bcf_get_frame_time(void *u) { (void)bc_note(BC_GET_FRAME_TIME, u); return 1.0f / 60.0f; }
static void bcf_draw_text_slice(const char *t, size_t n, float x, float y, WLX_Text_Style st, void *u) {
    BC_Call *c = bc_note(BC_DRAW_TEXT_SLICE, u); bc_text_args(c, t, n, st); c->f[0] = x; c->f[1] = y;
}
static void bcf_measure_text_slice(const char *t, size_t n, WLX_Text_Style st, float *w, float *h, void *u) {
    BC_Call *c = bc_note(BC_MEASURE_TEXT_SLICE, u); bc_text_args(c, t, n, st);
    int fs = st.font_size > 0 ? st.font_size : 10;
    if (w) *w = (float)n * (float)fs * 0.5f;
    if (h) *h = (float)fs;
}
static size_t bcf_measure_text_advances(const char *t, size_t n, WLX_Text_Style st,
        const size_t *ends, size_t count, float *adv, void *u) {
    BC_Call *c = bc_note(BC_MEASURE_TEXT_ADVANCES, u); bc_text_args(c, t, n, st);
    c->i[4] = (int64_t)count;
    c->f[0] = (float)bc_hash((const char *)ends, count * sizeof(*ends));
    return mock_measure_text_advances(t, n, st, ends, count, adv, NULL);
}
static void bcf_draw_shadow(WLX_Rect r, WLX_Color col, float ox, float oy, float blur, int layers,
        float ro, int seg, void *u) {
    BC_Call *c = bc_note(BC_DRAW_SHADOW, u); bc_rect_args(c, r);
    c->f[4] = ox; c->f[5] = oy; c->f[6] = blur; c->f[7] = ro;
    c->i[0] = bc_rgba(col); c->i[1] = layers; c->i[2] = seg;
}
static void bcf_draw_glow(WLX_Rect r, WLX_Color col, float spread, int rings, float ro, int seg, void *u) {
    BC_Call *c = bc_note(BC_DRAW_GLOW, u); bc_rect_args(c, r);
    c->f[4] = spread; c->f[5] = ro; c->i[0] = bc_rgba(col); c->i[1] = rings; c->i[2] = seg;
}
static void bcf_draw_gradient_v(WLX_Rect r, WLX_Color top, WLX_Color bottom, float ro, int seg, void *u) {
    BC_Call *c = bc_note(BC_DRAW_GRADIENT_V, u); bc_rect_args(c, r);
    c->f[4] = ro; c->i[0] = bc_rgba(top); c->i[1] = bc_rgba(bottom); c->i[2] = seg;
}
static const char *bcf_clipboard_get(void *u) { (void)bc_note(BC_CLIPBOARD_GET, u); return "pasted"; }
static void bcf_clipboard_set(const char *t, size_t n, void *u) {
    BC_Call *c = bc_note(BC_CLIPBOARD_SET, u); c->len = n; c->hash = bc_hash(t, n);
    if (n >= sizeof(_bc_copied)) n = sizeof(_bc_copied) - 1;
    memcpy(_bc_copied, t, n);
    _bc_copied[n] = '\0';
}
static void bcf_set_cursor(WLX_Cursor_Shape shape, void *u) { bc_note(BC_SET_CURSOR, u)->i[0] = (int64_t)shape; }
static float bcf_get_content_scale(void *u) { (void)bc_note(BC_GET_CONTENT_SCALE, u); return 1.0f; }
static void bcf_set_text_input_area(const WLX_Text_Input_Area *a, void *u) {
    BC_Call *c = bc_note(BC_SET_TEXT_INPUT_AREA, u); bc_rect_args(c, a->line);
    c->f[4] = a->cursor; c->i[0] = a->active; c->i[1] = a->multiline; c->i[2] = a->password;
}

// A v2 table of all 24 recording callbacks.
static WLX_Backend bc_full_table(void *user) {
    WLX_Backend b;
    memset(&b, 0, sizeof(b));
    b.contract_version        = WLX_BACKEND_CONTRACT_VERSION;
    b.user                    = user;
    b.draw_rect               = bcf_draw_rect;
    b.draw_rect_lines         = bcf_draw_rect_lines;
    b.draw_rect_rounded       = bcf_draw_rect_rounded;
    b.draw_rect_rounded_lines = bcf_draw_rect_rounded_lines;
    b.draw_circle             = bcf_draw_circle;
    b.draw_ring               = bcf_draw_ring;
    b.draw_line               = bcf_draw_line;
    b.draw_text               = bcf_draw_text;
    b.measure_text            = bcf_measure_text;
    b.draw_texture            = bcf_draw_texture;
    b.begin_scissor           = bcf_begin_scissor;
    b.end_scissor             = bcf_end_scissor;
    b.get_frame_time          = bcf_get_frame_time;
    b.draw_text_slice         = bcf_draw_text_slice;
    b.measure_text_slice      = bcf_measure_text_slice;
    b.measure_text_advances   = bcf_measure_text_advances;
    b.draw_shadow             = bcf_draw_shadow;
    b.draw_glow               = bcf_draw_glow;
    b.draw_gradient_v         = bcf_draw_gradient_v;
    b.clipboard_get           = bcf_clipboard_get;
    b.clipboard_set           = bcf_clipboard_set;
    b.set_cursor              = bcf_set_cursor;
    b.get_content_scale       = bcf_get_content_scale;
    b.set_text_input_area     = bcf_set_text_input_area;
    return b;
}

// The callback members of a table, counted from its size: a member added
// to either table changes the count and fails the test that lists them.
static int bc_member_count(size_t table_size, size_t first_offset) {
    return (int)((table_size - first_offset) / sizeof(void (*)(void)));
}

// ============================================================================
// The script: one widget set per frame, reaching every callback
// ============================================================================

static char   _bc_field[64];
static char   _bc_doc[64];
static size_t _bc_doc_len;

#define BC_ROOT_W 400
#define BC_ROOT_H 340

// Ten fixed rows of the root. Every widget sits at one call site, so its
// id is the same on every frame. The size arrays are automatic: the slot
// macros are compound literals in C, which MSVC refuses in a static
// initializer.
static void bc_widgets(WLX_Context *ctx) {
    const WLX_Slot_Size rows[] = {
        WLX_SLOT_PX(30), WLX_SLOT_PX(30), WLX_SLOT_PX(40), WLX_SLOT_PX(30), WLX_SLOT_PX(30),
        WLX_SLOT_PX(30), WLX_SLOT_PX(40), WLX_SLOT_PX(30), WLX_SLOT_PX(30), WLX_SLOT_PX(40),
    };
    const WLX_Slot_Size disc_row[] = { WLX_SLOT_PX(40), WLX_SLOT_PX(40), WLX_SLOT_FLEX(1) };
    static const WLX_Texture tex = { 7, 16, 16 };
    WLX_Color fill = WLX_RGBA(40, 50, 60, 255), edge = WLX_RGBA(200, 210, 220, 255);

    wlx_layout_begin(ctx, 10, WLX_VERT, .sizes = rows, .padding = 0, .gap = 0);
    // draw_rect, draw_rect_lines
    wlx_widget(ctx, .back_color = fill, .border_color = edge, .border_width = 1);
    // draw_rect_rounded, draw_rect_rounded_lines (not square: no circle)
    wlx_widget(ctx, .back_color = fill, .border_color = edge, .border_width = 1, .roundness = 0.4f);
    // draw_circle, draw_ring: a square at roundness 1
    wlx_layout_begin(ctx, 3, WLX_HORZ, .sizes = disc_row, .padding = 0, .gap = 0);
    wlx_widget(ctx, .back_color = fill, .roundness = 1.0f);
    wlx_widget(ctx, .border_color = edge, .border_width = 2, .roundness = 1.0f);
    wlx_widget(ctx);
    wlx_layout_end(ctx);
    // draw_line: the check mark
    bool checked = true;
    wlx_checkbox(ctx, "check", &checked);
    // draw_texture
    wlx_image(ctx, tex);
    // begin_scissor, end_scissor
    wlx_layout_begin(ctx, 1, WLX_VERT, .clip = true, .padding = 0);
    wlx_label(ctx, "clipped");
    wlx_layout_end(ctx);
    // draw_shadow, draw_glow, draw_gradient_v
    wlx_widget(ctx, .back_color = fill,
        .shadow_color = WLX_RGBA(0, 0, 0, 120), .shadow_offset_y = 3,
        .glow_color = WLX_RGBA(90, 160, 255, 160),
        .gradient_top = WLX_RGBA(10, 20, 30, 255), .gradient_bottom = WLX_RGBA(70, 80, 90, 255));
    // the caret line, the clipboard, the cursor, the composition anchor: a
    // focused inputbox under the pointer
    (void)wlx_inputbox(ctx, NULL, _bc_field, sizeof(_bc_field),
        .content_padding = 4, .font_size = 10, .wrap = false, .border_width = 0);
    // the text pair
    wlx_label(ctx, "slice");
    // measure_text_advances: the editor's windowed build is the batched path
    (void)wlx_editor(ctx, "bc_doc", _bc_doc, sizeof(_bc_doc), &_bc_doc_len, .font_size = 10);
    wlx_layout_end(ctx);
}

// The inputbox's row: rows 0-6 sum to 230, and it is 30 tall.
#define BC_FIELD_X 100
#define BC_FIELD_Y 245

static void bc_frame(WLX_Context *ctx, bool click, WLX_Key_Code key, uint32_t mods) {
    bool pressed[WLX_KEY_COUNT] = {0};
    if (key != WLX_KEY_NONE) pressed[key] = true;
    test_frame_begin_full(ctx, BC_FIELD_X, BC_FIELD_Y, click, click, click, 0.0f,
                          NULL, pressed, NULL, mods, NULL);
    bc_widgets(ctx);
    test_frame_end(ctx);
}

// Focus the field, hover it, select all, copy, paste: five frames.
static void bc_script(WLX_Context *ctx) {
    uint32_t cmd = test_command_mod();
    strcpy(_bc_field, "field");
    strcpy(_bc_doc, "editor line\nsecond");
    _bc_doc_len = strlen(_bc_doc);
    _bc_copied[0] = '\0';
    bc_frame(ctx, true, WLX_KEY_NONE, 0);
    bc_frame(ctx, false, WLX_KEY_NONE, 0);
    bc_frame(ctx, false, WLX_KEY_A, cmd);
    bc_frame(ctx, false, WLX_KEY_C, cmd);
    bc_frame(ctx, false, WLX_KEY_V, cmd);
}

// Every callback is reached, and every one receives the table's `user`.
// The script runs twice: with the slice pair, then without it, which is
// the only way the core calls the NUL-terminated pair.
TEST(backend_full_table_reaches_every_callback) {
    ASSERT_EQ_INT(bc_member_count(sizeof(WLX_Backend), offsetof(WLX_Backend, draw_rect)),
                  BC_CALLBACK_COUNT);
    int instance = 7;
    WLX_Context ctx;
    test_ctx_init(&ctx, BC_ROOT_W, BC_ROOT_H);
    ctx.backend = bc_full_table(&instance);
    _bc_expect_user = &instance;
    _bc_bad_user = 0;
    _bc_log = NULL;

    _bc_hits = 0;
    bc_script(&ctx);
    uint32_t legacy = (1u << BC_DRAW_TEXT) | (1u << BC_MEASURE_TEXT);
    uint32_t all = (1u << BC_CALLBACK_COUNT) - 1u;
    ASSERT_EQ_INT(_bc_hits & legacy, 0);              // the slice pair is preferred
    ASSERT_EQ_INT(_bc_hits, all & ~legacy);
    ASSERT_EQ_STR(_bc_copied, "field");               // select all and copy ran
    ASSERT_EQ_STR(_bc_field, "pasted");               // and so did paste
    wlx_context_destroy(&ctx);

    test_ctx_init(&ctx, BC_ROOT_W, BC_ROOT_H);
    ctx.backend = bc_full_table(&instance);
    ctx.backend.draw_text_slice = NULL;
    ctx.backend.measure_text_slice = NULL;
    _bc_hits = 0;
    bc_script(&ctx);
    ASSERT_EQ_INT(_bc_hits & legacy, legacy);
    ASSERT_EQ_INT(_bc_bad_user, 0);
    wlx_context_destroy(&ctx);
    _bc_expect_user = NULL;
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
    ASSERT_TRUE(ctx.backend.get_content_scale == NULL);  // the v2 slot stays optional
    ASSERT_TRUE(ctx.backend.set_text_input_area == NULL); // and so does the composition anchor

    _v1_rect_calls = _v1_text_calls = _v1_frame_calls = 0;
    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_EQ_F(wlx_content_scale(&ctx), 1.0f, 0.0f);   // NULL slot -> 1.0
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_button(&ctx, "shim");
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_TRUE(_v1_frame_calls >= 1);
    ASSERT_TRUE(_v1_rect_calls >= 1);
    ASSERT_TRUE(_v1_text_calls >= 1);
    wlx_context_destroy(&ctx);
}

// The recording callbacks in v1 shape: each forwards to its v2 twin with
// the expected instance pointer, so both tables log through one code path.
static void bc1_draw_rect(WLX_Rect r, WLX_Color c) { bcf_draw_rect(r, c, _bc_expect_user); }
static void bc1_draw_rect_lines(WLX_Rect r, float t, WLX_Color c) { bcf_draw_rect_lines(r, t, c, _bc_expect_user); }
static void bc1_draw_rect_rounded(WLX_Rect r, float ro, int seg, WLX_Color c) { bcf_draw_rect_rounded(r, ro, seg, c, _bc_expect_user); }
static void bc1_draw_rect_rounded_lines(WLX_Rect r, float ro, int seg, float t, WLX_Color c) { bcf_draw_rect_rounded_lines(r, ro, seg, t, c, _bc_expect_user); }
static void bc1_draw_circle(float cx, float cy, float rad, int seg, WLX_Color c) { bcf_draw_circle(cx, cy, rad, seg, c, _bc_expect_user); }
static void bc1_draw_ring(float cx, float cy, float ri, float ro, int seg, WLX_Color c) { bcf_draw_ring(cx, cy, ri, ro, seg, c, _bc_expect_user); }
static void bc1_draw_line(float x1, float y1, float x2, float y2, float t, WLX_Color c) { bcf_draw_line(x1, y1, x2, y2, t, c, _bc_expect_user); }
static void bc1_draw_text(const char *t, float x, float y, WLX_Text_Style st) { bcf_draw_text(t, x, y, st, _bc_expect_user); }
static void bc1_measure_text(const char *t, WLX_Text_Style st, float *w, float *h) { bcf_measure_text(t, st, w, h, _bc_expect_user); }
static void bc1_draw_texture(WLX_Texture tex, WLX_Rect s, WLX_Rect d, WLX_Color c) { bcf_draw_texture(tex, s, d, c, _bc_expect_user); }
static void bc1_begin_scissor(WLX_Rect r) { bcf_begin_scissor(r, _bc_expect_user); }
static void bc1_end_scissor(void) { bcf_end_scissor(_bc_expect_user); }
static float bc1_get_frame_time(void) { return bcf_get_frame_time(_bc_expect_user); }
static void bc1_draw_text_slice(const char *t, size_t n, float x, float y, WLX_Text_Style st) { bcf_draw_text_slice(t, n, x, y, st, _bc_expect_user); }
static void bc1_measure_text_slice(const char *t, size_t n, WLX_Text_Style st, float *w, float *h) { bcf_measure_text_slice(t, n, st, w, h, _bc_expect_user); }
static size_t bc1_measure_text_advances(const char *t, size_t n, WLX_Text_Style st, const size_t *ends, size_t count, float *adv) {
    return bcf_measure_text_advances(t, n, st, ends, count, adv, _bc_expect_user);
}
static void bc1_draw_shadow(WLX_Rect r, WLX_Color c, float ox, float oy, float blur, int layers, float ro, int seg) {
    bcf_draw_shadow(r, c, ox, oy, blur, layers, ro, seg, _bc_expect_user);
}
static void bc1_draw_glow(WLX_Rect r, WLX_Color c, float spread, int rings, float ro, int seg) {
    bcf_draw_glow(r, c, spread, rings, ro, seg, _bc_expect_user);
}
static void bc1_draw_gradient_v(WLX_Rect r, WLX_Color top, WLX_Color bottom, float ro, int seg) {
    bcf_draw_gradient_v(r, top, bottom, ro, seg, _bc_expect_user);
}
static const char *bc1_clipboard_get(void) { return bcf_clipboard_get(_bc_expect_user); }
static void bc1_clipboard_set(const char *t, size_t n) { bcf_clipboard_set(t, n, _bc_expect_user); }
static void bc1_set_cursor(WLX_Cursor_Shape shape) { bcf_set_cursor(shape, _bc_expect_user); }

static WLX_Backend_V1 bc_full_v1_table(void) {
    WLX_Backend_V1 t;
    memset(&t, 0, sizeof(t));
    t.draw_rect               = bc1_draw_rect;
    t.draw_rect_lines         = bc1_draw_rect_lines;
    t.draw_rect_rounded       = bc1_draw_rect_rounded;
    t.draw_rect_rounded_lines = bc1_draw_rect_rounded_lines;
    t.draw_circle             = bc1_draw_circle;
    t.draw_ring               = bc1_draw_ring;
    t.draw_line               = bc1_draw_line;
    t.draw_text               = bc1_draw_text;
    t.measure_text            = bc1_measure_text;
    t.draw_texture            = bc1_draw_texture;
    t.begin_scissor           = bc1_begin_scissor;
    t.end_scissor             = bc1_end_scissor;
    t.get_frame_time          = bc1_get_frame_time;
    t.draw_text_slice         = bc1_draw_text_slice;
    t.measure_text_slice      = bc1_measure_text_slice;
    t.measure_text_advances   = bc1_measure_text_advances;
    t.draw_shadow             = bc1_draw_shadow;
    t.draw_glow               = bc1_draw_glow;
    t.draw_gradient_v         = bc1_draw_gradient_v;
    t.clipboard_get           = bc1_clipboard_get;
    t.clipboard_set           = bc1_clipboard_set;
    t.set_cursor              = bc1_set_cursor;
    return t;
}

static bool bc_call_equal(const BC_Call *a, const BC_Call *b) {
    if (a->id != b->id || a->len != b->len || a->hash != b->hash) return false;
    for (int k = 0; k < 8; k++) if (a->f[k] != b->f[k]) return false;
    for (int k = 0; k < 5; k++) if (a->i[k] != b->i[k]) return false;
    return true;
}

// Runs the script once on a table and logs it; returns the callbacks hit.
static uint32_t bc_log_script(WLX_Backend table, BC_Log *log) {
    WLX_Context ctx;
    test_ctx_init(&ctx, BC_ROOT_W, BC_ROOT_H);
    ctx.backend = table;
    log->count = 0;
    _bc_log = log;
    _bc_hits = 0;
    bc_script(&ctx);
    _bc_log = NULL;
    wlx_context_destroy(&ctx);
    return _bc_hits;
}

// Each of the 22 trampolines forwards its arguments unchanged: a v1 table
// through the shim and a v2 table of the same recording callbacks (without
// the two v2-only slots) log the same calls, call for call, over the
// script with the slice pair and without it. Nothing here states what a
// callback should receive, so the check cannot share a mistake with the
// shim.
TEST(backend_v1_shim_echoes_every_callback) {
    ASSERT_EQ_INT(bc_member_count(sizeof(WLX_Backend_V1), 0), BC_V1_CALLBACK_COUNT);
    static BC_Log v1_log, v2_log;
    static WLX_Backend_V1 v1;
    int instance = 9;
    uint32_t reached = 0;
    _bc_expect_user = &instance;
    _bc_bad_user = 0;
    for (int pass = 0; pass < 2; pass++) {
        WLX_Backend v2 = bc_full_table(&instance);
        v2.get_content_scale = NULL;
        v2.set_text_input_area = NULL;
        v1 = bc_full_v1_table();
        if (pass == 1) {
            v2.draw_text_slice = NULL; v2.measure_text_slice = NULL;
            v1.draw_text_slice = NULL; v1.measure_text_slice = NULL;
        }
        uint32_t v2_hits = bc_log_script(v2, &v2_log);
        uint32_t v1_hits = bc_log_script(wlx_backend_from_v1(&v1), &v1_log);
        ASSERT_EQ_INT(v1_hits, v2_hits);
        ASSERT_EQ_INT(v1_log.count, v2_log.count);
        ASSERT_TRUE(v1_log.count <= BC_LOG_CAP);
        int first_diff = -1;
        for (int k = 0; k < v1_log.count && first_diff < 0; k++)
            if (!bc_call_equal(&v1_log.calls[k], &v2_log.calls[k])) first_diff = k;
        if (first_diff >= 0)
            fprintf(stderr, "    pass %d, call %d: v1 callback %d, v2 callback %d\n", pass, first_diff,
                    v1_log.calls[first_diff].id, v2_log.calls[first_diff].id);
        ASSERT_EQ_INT(first_diff, -1);
        reached |= v1_hits;
    }
    ASSERT_EQ_INT(reached, (1u << BC_V1_CALLBACK_COUNT) - 1u);   // all 22 trampolines ran
    ASSERT_EQ_INT(_bc_bad_user, 0);
    _bc_expect_user = NULL;
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
    ASSERT_EQ_INT((int)s->env.env_generation + 1, (int)ctx.text_env_generation);
    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    ASSERT_EQ_INT((int)s->env.env_generation, (int)ctx.text_env_generation);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(backend_contract) {
    RUN_TEST(backend_full_table_reaches_every_callback);
    RUN_TEST(backend_v1_shim_forwards_and_keeps_null_optionals);
    RUN_TEST(backend_v1_shim_echoes_every_callback);
    RUN_TEST(style_transform_applies_at_boundary);
    RUN_TEST(style_transform_reset_clears_geometry);
}
