// test_ime.c - composition input on the text widgets. Opens with the
// identity pins: with no composition string staged, the inputbox, the
// textarea and the editor (linear and wrapped) type, move, select, delete,
// undo and blur exactly as they did before composition existed - buffer,
// caret pair, journal counts and the recorded draw calls of a settled
// frame. The pins are recorded from the pre-composition code and are never
// re-recorded. Then the composition itself: the tentative span shown,
// updated and cancelled with the journal untouched, the caret at the
// input method's codepoint cursor snapped to a cluster boundary, the
// selected clause, the commit as one undo step (also with the next clause
// in the same frame), adopt-on-blur, composing over a selection, capacity
// truncation, read-only and password fields, the forget rules, keys and
// the mouse off while composing, the text-input-area pushes, wrapped
// reflow and the editor's index patched (no rebuild) across composing
// frames. Last, word motion on the same four shapes: Ctrl+Left,
// Ctrl+Backspace and the double-click word never stop inside a space that
// carries a combining mark (one text unit).
//
// Geometry model (mock backend): char width = font_size/2 = 5 px at
// font_size 10, line height 10. Fixtures: a 400x100 context holding one
// 40-tall widget at a stable call site (content padding 4, border 0), so
// a press at the context's bottom lands outside it and blurs. Reuses
// nothing from the editor fixtures so it can follow any suite.

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
// Fixture: draw capture (text pieces and lines) and the three widgets
// ============================================================================

#define IME_MAX_DRAWS_ 256
typedef struct {
    int kind;            // 1 = text piece, 2 = line
    char text[64];
    float x, y;          // piece origin, or line start
    float x2, y2;        // line end
    float thick;
} Ime_Draw;
static Ime_Draw ime_draws[IME_MAX_DRAWS_];
static int ime_draw_count;

static void ime_capture_text_slice(const char *text, size_t len, float x, float y,
    WLX_Text_Style style, void *user)
{
    (void)style; (void)user;
    if (ime_draw_count >= IME_MAX_DRAWS_) return;
    Ime_Draw *d = &ime_draws[ime_draw_count++];
    memset(d, 0, sizeof(*d));
    d->kind = 1;
    size_t keep = len < sizeof(d->text) - 1 ? len : sizeof(d->text) - 1;
    if (keep > 0) memcpy(d->text, text, keep);
    d->text[keep] = '\0';
    d->x = x;
    d->y = y;
}

static void ime_capture_text(const char *text, float x, float y, WLX_Text_Style style, void *user) {
    ime_capture_text_slice(text, text ? strlen(text) : 0, x, y, style, user);
}

static void ime_capture_line(float x1, float y1, float x2, float y2, float thick,
    WLX_Color color, void *user)
{
    (void)color; (void)user;
    if (ime_draw_count >= IME_MAX_DRAWS_) return;
    Ime_Draw *d = &ime_draws[ime_draw_count++];
    memset(d, 0, sizeof(*d));
    d->kind = 2;
    d->x = x1; d->y = y1; d->x2 = x2; d->y2 = y2; d->thick = thick;
}

// Filled rects (kind 3): x, y, then x2 = width, y2 = height.
static void ime_capture_rect(WLX_Rect r, WLX_Color color, void *user) {
    (void)color; (void)user;
    if (ime_draw_count >= IME_MAX_DRAWS_) return;
    Ime_Draw *d = &ime_draws[ime_draw_count++];
    memset(d, 0, sizeof(*d));
    d->kind = 3;
    d->x = r.x; d->y = r.y; d->x2 = r.w; d->y2 = r.h;
}

// Index of the first captured line whose width is `w` (an underline), or -1.
static int ime_find_underline(float w) {
    for (int i = 0; i < ime_draw_count; i++) {
        if (ime_draws[i].kind == 2 && ime_draws[i].y == ime_draws[i].y2
            && fabsf((ime_draws[i].x2 - ime_draws[i].x) - w) < 0.01f) return i;
    }
    return -1;
}

// Index of the first captured rect of the given width and height, or -1.
static int ime_find_rect(float w, float h) {
    for (int i = 0; i < ime_draw_count; i++) {
        if (ime_draws[i].kind == 3 && fabsf(ime_draws[i].x2 - w) < 0.01f
            && fabsf(ime_draws[i].y2 - h) < 0.01f) return i;
    }
    return -1;
}

static int ime_count_pieces(void) {
    int n = 0;
    for (int i = 0; i < ime_draw_count; i++) if (ime_draws[i].kind == 1) n++;
    return n;
}

static void ime_reset_draws(void) {
    memset(ime_draws, 0, sizeof(ime_draws));
    ime_draw_count = 0;
}

static void ime_reset_options(void);

static void ime_ctx_init(WLX_Context *ctx) {
    test_ctx_init(ctx, 400, 100);
    ctx->backend.draw_text_slice = ime_capture_text_slice;
    ctx->backend.draw_text = ime_capture_text;
    ctx->backend.draw_line = ime_capture_line;
    test_reset_mock_tia();
    ime_reset_options();
}

// Draw index of the n-th text piece (0-based), or -1.
static int ime_piece_index(int n) {
    for (int i = 0; i < ime_draw_count; i++) {
        if (ime_draws[i].kind == 1 && n-- == 0) return i;
    }
    return -1;
}

// The piece at index i is exactly `text` at (x, y).
#define IME_EXPECT_PIECE(i, want_text, want_x, want_y) do {                    \
    ASSERT_TRUE((i) < ime_draw_count);                                         \
    ASSERT_EQ_INT(1, ime_draws[(i)].kind);                                     \
    ASSERT_EQ_STR(ime_draws[(i)].text, (want_text));                           \
    ASSERT_EQ_F(ime_draws[(i)].x, (want_x), 0.01f);                            \
    ASSERT_EQ_F(ime_draws[(i)].y, (want_y), 0.01f);                            \
} while (0)

// The line at index i runs from (x1, y1) to (x2, y2).
#define IME_EXPECT_LINE(i, want_x1, want_y1, want_x2, want_y2) do {             \
    ASSERT_TRUE((i) < ime_draw_count);                                         \
    ASSERT_EQ_INT(2, ime_draws[(i)].kind);                                     \
    ASSERT_EQ_F(ime_draws[(i)].x, (want_x1), 0.01f);                           \
    ASSERT_EQ_F(ime_draws[(i)].y, (want_y1), 0.01f);                           \
    ASSERT_EQ_F(ime_draws[(i)].x2, (want_x2), 0.01f);                          \
    ASSERT_EQ_F(ime_draws[(i)].y2, (want_y2), 0.01f);                          \
} while (0)

typedef enum { IME_W_INPUTBOX, IME_W_TEXTAREA, IME_W_EDITOR, IME_W_EDITOR_WRAP } Ime_Widget;

static Ime_Widget ime_widget;
static bool ime_last_changed;
static bool ime_last_focused;
static bool ime_read_only;
static bool ime_password;
static uint32_t ime_revision;
static bool ime_skip_widget;   // place nothing this frame (the widget is not drawn)

static void ime_reset_options(void) {
    ime_read_only = false;
    ime_password = false;
    ime_revision = 0;
    ime_skip_widget = false;
}

// The shapes a behaviour case runs on: every widget the composition
// reaches, and the three that hold more than one line. The password and
// capacity cases stay inputbox-only: the editor has neither.
static const Ime_Widget ime_shapes[] = {
    IME_W_INPUTBOX, IME_W_TEXTAREA, IME_W_EDITOR, IME_W_EDITOR_WRAP
};
static const Ime_Widget ime_multiline_shapes[] = {
    IME_W_TEXTAREA, IME_W_EDITOR, IME_W_EDITOR_WRAP
};
#define IME_SHAPE_COUNT(a) (sizeof(a) / sizeof((a)[0]))

static const char *ime_shape_name(Ime_Widget w) {
    switch (w) {
        case IME_W_INPUTBOX:    return "inputbox";
        case IME_W_TEXTAREA:    return "textarea";
        case IME_W_EDITOR:      return "editor";
        case IME_W_EDITOR_WRAP: return "editor (wrap)";
    }
    return "?";
}

// One case per shape. The cases share their assertion lines, so a case
// that fails names its shape after the failing line.
static void ime_for_shapes(const Ime_Widget *shapes, size_t n, void (*body)(Ime_Widget)) {
    for (size_t i = 0; i < n; i++) {
        int failed_before = _test_current_failed;
        _test_current_failed = 0;
        body(shapes[i]);
        if (_test_current_failed) fprintf(stderr, "    (shape: %s)\n", ime_shape_name(shapes[i]));
        _test_current_failed |= failed_before;
    }
}
#define IME_FOR_SHAPES(shapes, body) ime_for_shapes((shapes), IME_SHAPE_COUNT(shapes), (body))

// The y of a one-line buffer's row: the inputbox and the textarea centre
// it in the 32-tall band (the widget sits at y = 30, padding 4), the
// editor top-anchors it.
static float ime_row_y(Ime_Widget w) {
    return (w == IME_W_INPUTBOX || w == IME_W_TEXTAREA) ? 45.0f : 34.0f;
}

static void ime_place_widget(WLX_Context *ctx, char *buf, size_t cap, size_t *len) {
    ime_last_focused = false;
    if (ime_skip_widget) return;
    switch (ime_widget) {
        case IME_W_INPUTBOX:
        case IME_W_TEXTAREA:
            ime_last_changed = wlx_inputbox_impl(ctx, NULL, buf, cap,
                wlx_default_inputbox_opt(.height = 40, .content_padding = 4, .font_size = 10,
                    .border_width = 0, .multiline = ime_widget == IME_W_TEXTAREA,
                    .read_only = ime_read_only, .password = ime_password,
                    .revision = ime_revision, .out_focused = &ime_last_focused),
                __FILE__, __LINE__);
            *len = strlen(buf);
            break;
        case IME_W_EDITOR:
        case IME_W_EDITOR_WRAP:
            ime_last_changed = wlx_editor_impl(ctx, NULL, buf, cap, len,
                wlx_default_editor_opt(.height = 40, .content_padding = 4, .font_size = 10,
                    .border_width = 0, .wrap = ime_widget == IME_W_EDITOR_WRAP,
                    .read_only = ime_read_only, .revision = ime_revision,
                    .out_focused = &ime_last_focused),
                __FILE__, __LINE__);
            break;
    }
}

// One frame carrying committed text and a composition string (mouse at
// rest, no keys).
static void ime_compose(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                        const char *text_input, const char *preedit,
                        int32_t cursor, int32_t sel_len)
{
    test_frame_begin_ime(ctx, text_input, preedit, cursor, sel_len);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    ime_place_widget(ctx, buf, cap, len);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// One frame with a composition string AND a key or a click, for the
// "keys and the mouse are off while composing" pins.
static void ime_compose_key(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                            const char *preedit, int mx, int my, bool clicked,
                            WLX_Key_Code key, uint32_t mods)
{
    WLX_Input_State in;
    memset(&in, 0, sizeof(in));
    in.mouse_x = mx;
    in.mouse_y = my;
    in.mouse_down = clicked;
    in.mouse_clicked = clicked;
    in.mouse_held = clicked;
    if (key != WLX_KEY_NONE) { in.keys_pressed[key] = true; in.keys_down[key] = true; }
    in.modifiers = mods;
    size_t n = strlen(preedit);
    if (n >= sizeof(in.preedit)) n = sizeof(in.preedit) - 1;
    memcpy(in.preedit, preedit, n);
    in.preedit_cursor = -1;
    in.preedit_sel_len = -1;
    test_frame_begin_input(ctx, &in);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    ime_place_widget(ctx, buf, cap, len);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// One frame carrying committed text, a composition string and one pressed
// key with modifiers: the frame a host reports when the key that confirms
// or cancels a composition reaches it too (mouse at rest).
static void ime_end_key(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                        const char *text_input, const char *preedit,
                        WLX_Key_Code key, uint32_t mods)
{
    WLX_Input_State in;
    memset(&in, 0, sizeof(in));
    in.mouse_x = 200;
    in.mouse_y = 50;
    if (key != WLX_KEY_NONE) { in.keys_pressed[key] = true; in.keys_down[key] = true; }
    in.modifiers = mods;
    if (text_input != NULL) {
        size_t n = strlen(text_input);
        if (n >= sizeof(in.text_input)) n = sizeof(in.text_input) - 1;
        memcpy(in.text_input, text_input, n);
    }
    size_t n = strlen(preedit);
    if (n >= sizeof(in.preedit)) n = sizeof(in.preedit) - 1;
    memcpy(in.preedit, preedit, n);
    in.preedit_cursor = -1;
    in.preedit_sel_len = -1;
    test_frame_begin_input(ctx, &in);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    ime_place_widget(ctx, buf, cap, len);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// One frame: mouse, at most one pressed key with modifiers, typed text.
static void ime_frame(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                      int mx, int my, bool clicked, WLX_Key_Code key, uint32_t mods,
                      const char *text)
{
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    if (key != WLX_KEY_NONE) keys_pressed[key] = true;
    test_frame_begin_full(ctx, mx, my, clicked, clicked, clicked, 0.0f, NULL,
        key != WLX_KEY_NONE ? keys_pressed : NULL, NULL, mods, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    ime_place_widget(ctx, buf, cap, len);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void ime_focus(WLX_Context *ctx, char *buf, size_t cap, size_t *len) {
    ime_frame(ctx, buf, cap, len, 200, 50, true, WLX_KEY_NONE, 0, NULL);
}
static void ime_idle(WLX_Context *ctx, char *buf, size_t cap, size_t *len) {
    ime_frame(ctx, buf, cap, len, 200, 50, false, WLX_KEY_NONE, 0, NULL);
}
static void ime_type(WLX_Context *ctx, char *buf, size_t cap, size_t *len, const char *text) {
    ime_frame(ctx, buf, cap, len, 200, 50, false, WLX_KEY_NONE, 0, text);
}
static void ime_key(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                    WLX_Key_Code key, uint32_t mods) {
    ime_frame(ctx, buf, cap, len, 200, 50, false, key, mods, NULL);
}
// A press below the 40-tall widget: it blurs.
static void ime_blur(WLX_Context *ctx, char *buf, size_t cap, size_t *len) {
    ime_frame(ctx, buf, cap, len, 2, 98, true, WLX_KEY_NONE, 0, NULL);
}
// One wheel notch set with the pointer on the widget (negative scrolls the
// view down); Shift turns it horizontal in the editor.
static void ime_wheel(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                      float delta, uint32_t mods) {
    test_frame_begin_full(ctx, 200, 50, false, false, false, delta, NULL,
        NULL, NULL, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    ime_place_widget(ctx, buf, cap, len);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// The fixture widget's caret state (the context holds one widget).
static WLX_Text_Edit_State *ime_caret(WLX_Context *ctx) {
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        if (ctx->states.slots[i].id != 0) {
            // Both widget states embed the caret state first.
            return (WLX_Text_Edit_State *)ctx->states.slots[i].data;
        }
    }
    return NULL;
}

// The fixture widget's journal, or NULL.
static WLX_Text_Undo_Journal *ime_journal(WLX_Context *ctx) {
    return ctx->text_undo.count > 0 ? &ctx->text_undo.items[0] : NULL;
}

static size_t ime_undo_count(WLX_Context *ctx) {
    WLX_Text_Undo_Journal *j = ime_journal(ctx);
    return j ? j->undo.count : 0;
}

static size_t ime_redo_count(WLX_Context *ctx) {
    WLX_Text_Undo_Journal *j = ime_journal(ctx);
    return j ? j->redo.count : 0;
}

static size_t ime_expected_len(WLX_Context *ctx) {
    WLX_Text_Undo_Journal *j = ime_journal(ctx);
    return j ? j->expected_len : 0;
}

// The newest undo entry, or a zero entry.
static WLX_Text_Undo_Entry ime_newest(WLX_Context *ctx) {
    WLX_Text_Undo_Entry zero;
    memset(&zero, 0, sizeof(zero));
    WLX_Text_Undo_Journal *j = ime_journal(ctx);
    if (j == NULL || j->undo.count == 0) return zero;
    return j->undo.entries[j->undo.count - 1];
}

static uint32_t ime_editor_rebuilds(WLX_Context *ctx) {
    return ctx->editor_indices.count > 0 ? ctx->editor_indices.items[0].rebuilds : 0;
}

static uint32_t ime_editor_patches(WLX_Context *ctx) {
    return ctx->editor_indices.count > 0 ? ctx->editor_indices.items[0].patches : 0;
}

#ifdef IME_PROBE_
static void ime_dump(const char *tag, WLX_Context *ctx, const char *buf, size_t len) {
    WLX_Text_Edit_State *c = ime_caret(ctx);
    printf("[%s] buf=\"%s\" len=%zu caret=%zu anchor=%zu undo=%zu redo=%zu changed=%d focused=%d\n",
        tag, buf, len, c ? c->cursor_pos : 0, c ? c->selection_anchor : 0,
        ime_undo_count(ctx), ime_redo_count(ctx), ime_last_changed, ime_last_focused);
    for (int i = 0; i < ime_draw_count; i++) {
        if (ime_draws[i].kind == 1)
            printf("    piece %d \"%s\" @ %.2f,%.2f\n", i, ime_draws[i].text, ime_draws[i].x, ime_draws[i].y);
        else
            printf("    line  %d (%.2f,%.2f)-(%.2f,%.2f) t=%.2f\n", i, ime_draws[i].x, ime_draws[i].y,
                ime_draws[i].x2, ime_draws[i].y2, ime_draws[i].thick);
    }
}
#define IME_DUMP(tag) ime_dump(tag, &ctx, buf, len)
#else
#define IME_DUMP(tag) ((void)0)
#endif

// ============================================================================
// Identity: no composition string, every widget behaves as before
// ============================================================================

TEST(ime_identity_inputbox) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ASSERT_TRUE(ime_last_focused);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_TRUE(ime_last_changed);
    ASSERT_EQ_INT(2, (int)ime_caret(&ctx)->cursor_pos);
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, WLX_MOD_SHIFT);
    ASSERT_EQ_INT(2, (int)ime_caret(&ctx)->cursor_pos);
    ASSERT_EQ_INT(1, (int)ime_caret(&ctx)->selection_anchor);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "a");
    ASSERT_EQ_INT(2, (int)ime_undo_count(&ctx));
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(1, (int)ime_redo_count(&ctx));
    ASSERT_EQ_INT(2, (int)ime_caret(&ctx)->cursor_pos);
    ASSERT_EQ_INT(1, (int)ime_caret(&ctx)->selection_anchor);
    ime_blur(&ctx, buf, sizeof(buf), &len);
    ASSERT_FALSE(ime_last_focused);
    ime_reset_draws();
    ime_idle(&ctx, buf, sizeof(buf), &len);
    IME_DUMP("settled");
    IME_EXPECT_PIECE(0, "ab", 9.0f, 45.0f);
    ASSERT_EQ_INT(1, ime_draw_count);
    wlx_context_destroy(&ctx);
}

TEST(ime_identity_textarea) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_TEXTAREA;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ASSERT_TRUE(ime_last_focused);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_ENTER, 0);
    ASSERT_EQ_STR(buf, "ab\n");
    ASSERT_EQ_INT(2, (int)ime_undo_count(&ctx));
    ime_type(&ctx, buf, sizeof(buf), &len, "cd");
    ASSERT_EQ_STR(buf, "ab\ncd");
    ASSERT_EQ_INT(3, (int)ime_undo_count(&ctx));
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, WLX_MOD_SHIFT);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "ab\nc");
    ASSERT_EQ_INT(4, (int)ime_undo_count(&ctx));
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_STR(buf, "ab\ncd");
    ASSERT_EQ_INT(3, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(1, (int)ime_redo_count(&ctx));
    ASSERT_EQ_INT(5, (int)ime_caret(&ctx)->cursor_pos);
    ASSERT_EQ_INT(4, (int)ime_caret(&ctx)->selection_anchor);
    ime_blur(&ctx, buf, sizeof(buf), &len);
    ASSERT_FALSE(ime_last_focused);
    ime_reset_draws();
    ime_idle(&ctx, buf, sizeof(buf), &len);
    IME_DUMP("settled");
    IME_EXPECT_PIECE(0, "ab", 9.0f, 40.0f);
    IME_EXPECT_PIECE(1, "cd", 9.0f, 50.0f);
    ASSERT_EQ_INT(2, ime_draw_count);
    wlx_context_destroy(&ctx);
}

static void ime_identity_editor_body(bool wrap) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = wrap ? IME_W_EDITOR_WRAP : IME_W_EDITOR;
    char buf[64] = "";
    size_t len = 0;

    ime_idle(&ctx, buf, sizeof(buf), &len);   // the index exists before the press
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ASSERT_TRUE(ime_last_focused);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_EQ_INT(2, (int)len);
    ASSERT_TRUE(ime_last_changed);
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_ENTER, 0);
    ime_type(&ctx, buf, sizeof(buf), &len, "cd");
    ASSERT_EQ_STR(buf, "ab\ncd");
    ASSERT_EQ_INT(3, (int)ime_undo_count(&ctx));
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, WLX_MOD_SHIFT);
    ASSERT_EQ_INT(5, (int)ime_caret(&ctx)->cursor_pos);
    ASSERT_EQ_INT(4, (int)ime_caret(&ctx)->selection_anchor);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "ab\nc");
    ASSERT_EQ_INT(4, (int)ime_undo_count(&ctx));
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_STR(buf, "ab\ncd");
    ASSERT_EQ_INT(5, (int)len);
    ASSERT_EQ_INT(3, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(1, (int)ime_redo_count(&ctx));
    ASSERT_EQ_INT(5, (int)ime_caret(&ctx)->cursor_pos);
    ASSERT_EQ_INT(4, (int)ime_caret(&ctx)->selection_anchor);
    ime_blur(&ctx, buf, sizeof(buf), &len);
    ASSERT_FALSE(ime_last_focused);
    ime_reset_draws();
    ime_idle(&ctx, buf, sizeof(buf), &len);
    IME_DUMP(wrap ? "settled wrap" : "settled linear");
    IME_EXPECT_PIECE(0, "ab", 9.0f, 34.0f);
    IME_EXPECT_PIECE(1, "cd", 9.0f, 44.0f);
    ASSERT_EQ_INT(2, ime_draw_count);
    wlx_context_destroy(&ctx);
}

TEST(ime_identity_editor_linear) { ime_identity_editor_body(false); }
TEST(ime_identity_editor_wrapped) { ime_identity_editor_body(true); }

// ============================================================================
// The tentative span
// ============================================================================

// The caret blink clock is bounded: past 1024 s of focus one frame
// reduces it to its phase (fmodf by the period), so a long-focused field
// keeps blinking. It used to grow without bound until, past 2^19 s, a
// 60 fps frame's increment rounded away and the blink stopped.
static void text_caret_blink_clock_bounded_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;
    ime_focus(&ctx, buf, sizeof(buf), &len);
    WLX_Text_Edit_State *c = ime_caret(&ctx);
    c->cursor_blink_time = 1000000.25f;
    ime_idle(&ctx, buf, sizeof(buf), &len);
    // At 1e6 the frame's 1/60 s rounds away (the float step there is
    // 1/16 s), so the phase is 0.25 both before and after the reduction.
    ASSERT_TRUE(c->cursor_blink_time < 1.0f);
    ASSERT_EQ_F(c->cursor_blink_time, 0.25f, 0.0001f);
    wlx_context_destroy(&ctx);
}

TEST(text_caret_blink_clock_bounded) {
    IME_FOR_SHAPES(ime_shapes, text_caret_blink_clock_bounded_case);
}

// A composition string lands in the buffer at the caret as ordinary bytes:
// drawn, underlined, the caret inside it; the journal records nothing and
// its length guard follows the true length.
static void ime_tentative_span_shows_and_journal_untouched_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(2, (int)ime_expected_len(&ctx));
    ASSERT_FALSE(wlx_text_composing(&ctx));

    ime_reset_draws();
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "ni", -1, -1);
    ASSERT_EQ_STR(buf, "abni");
    ASSERT_EQ_INT(4, (int)len);
    ASSERT_TRUE(ime_last_changed);
    ASSERT_TRUE(wlx_text_composing(&ctx));
    WLX_Text_Edit_State *c = ime_caret(&ctx);
    ASSERT_TRUE(wlx_text_edit_composing(c));
    ASSERT_EQ_INT(2, (int)c->preedit_start);
    ASSERT_EQ_INT(2, (int)c->preedit_len);
    ASSERT_EQ_INT(4, (int)c->preedit_doc_len);
    ASSERT_EQ_INT(4, (int)c->cursor_pos);
    ASSERT_EQ_INT(4, (int)c->selection_anchor);
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(0, (int)ime_redo_count(&ctx));
    ASSERT_EQ_INT(4, (int)ime_expected_len(&ctx));
    // The text piece carries the span; the underline spans its two units
    // (10 px) at the row's bottom, right of the "ab" prefix.
    float row_y = ime_row_y(w);
    IME_EXPECT_PIECE(ime_piece_index(0), "abni", 9.0f, row_y);
    int ul = ime_find_underline(10.0f);
    ASSERT_TRUE(ul >= 0);
    ASSERT_EQ_F(ime_draws[ul].x, 9.0f + 10.0f, 0.01f);
    ASSERT_TRUE(ime_draws[ul].y > row_y && ime_draws[ul].y <= row_y + 10.0f);
    wlx_context_destroy(&ctx);
}

TEST(ime_tentative_span_shows_and_journal_untouched) {
    IME_FOR_SHAPES(ime_shapes, ime_tentative_span_shows_and_journal_untouched_case);
}

// Each update replaces the whole span in place: one span, at the same
// start, the buffer holding only the latest string.
static void ime_update_replaces_span_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "ni", -1, -1);
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nih", -1, -1);
    ASSERT_EQ_STR(buf, "abnih");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ASSERT_EQ_STR(buf, "abnihon");
    WLX_Text_Edit_State *c = ime_caret(&ctx);
    ASSERT_EQ_INT(2, (int)c->preedit_start);
    ASSERT_EQ_INT(5, (int)c->preedit_len);
    ASSERT_EQ_INT(7, (int)c->preedit_doc_len);
    ASSERT_EQ_INT(7, (int)c->cursor_pos);
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(7, (int)ime_expected_len(&ctx));
    // An unchanged string keeps the span on the same bytes.
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ASSERT_EQ_STR(buf, "abnihon");
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(ime_update_replaces_span) {
    IME_FOR_SHAPES(ime_shapes, ime_update_replaces_span_case);
}

// An empty string ends the composition: the buffer and the journal are
// exactly what they were before it started, and undo still reaches the
// typed text.
static void ime_cancel_restores_buffer_and_journal_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    WLX_Text_Undo_Entry before = ime_newest(&ctx);
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "", -1, -1);
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_EQ_INT(2, (int)len);
    WLX_Text_Edit_State *c = ime_caret(&ctx);
    ASSERT_FALSE(wlx_text_edit_composing(c));
    ASSERT_EQ_INT(0, (int)c->preedit_len);
    ASSERT_EQ_INT(2, (int)c->cursor_pos);
    ASSERT_EQ_INT(2, (int)c->selection_anchor);
    ASSERT_FALSE(wlx_text_composing(&ctx));
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(2, (int)ime_expected_len(&ctx));
    WLX_Text_Undo_Entry after = ime_newest(&ctx);
    ASSERT_EQ_INT((int)before.start, (int)after.start);
    ASSERT_EQ_INT((int)before.inserted_len, (int)after.inserted_len);
    ASSERT_EQ_INT((int)before.cls, (int)after.cls);
    // The cancelled frame drew no underline.
    ime_reset_draws();
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ASSERT_TRUE(ime_find_underline(10.0f) < 0 && ime_find_underline(25.0f) < 0);
    // Undo reaches the typed text as if nothing had happened in between.
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_STR(buf, "");
    ASSERT_EQ_INT(0, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(1, (int)ime_redo_count(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(ime_cancel_restores_buffer_and_journal) {
    IME_FOR_SHAPES(ime_shapes, ime_cancel_restores_buffer_and_journal_case);
}

// The composition caret follows the input method's codepoint cursor,
// snapped forward when it lands inside a grapheme cluster, at the end for
// -1 or past the end.
static void ime_cursor_codepoints_snapped_and_clamped_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;
    static const char pre[] = "e\xCC\x81x";   // e + U+0301 + x: 4 bytes, 3 codepoints, 2 units

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, pre, 0, -1);
    ASSERT_EQ_INT(6, (int)len);
    ASSERT_EQ_INT(2, (int)ime_caret(&ctx)->cursor_pos);       // at the span start
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, pre, 1, -1);
    ASSERT_EQ_INT(5, (int)ime_caret(&ctx)->cursor_pos);       // inside the cluster: snapped past it
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, pre, 2, -1);
    ASSERT_EQ_INT(5, (int)ime_caret(&ctx)->cursor_pos);       // on the boundary
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, pre, 3, -1);
    ASSERT_EQ_INT(6, (int)ime_caret(&ctx)->cursor_pos);       // the end
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, pre, 99, -1);
    ASSERT_EQ_INT(6, (int)ime_caret(&ctx)->cursor_pos);       // clamped
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, pre, -1, -1);
    ASSERT_EQ_INT(6, (int)ime_caret(&ctx)->cursor_pos);       // unknown = the end
    ASSERT_EQ_INT(6, (int)ime_caret(&ctx)->selection_anchor);
    wlx_context_destroy(&ctx);
}

TEST(ime_cursor_codepoints_snapped_and_clamped) {
    IME_FOR_SHAPES(ime_shapes, ime_cursor_codepoints_snapped_and_clamped_case);
}

// The selected clause draws in the selection colour from the caret for
// the given codepoints; the widget's own selection stays empty.
static void ime_clause_highlight_range_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ctx.backend.draw_rect = ime_capture_rect;
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_reset_draws();
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", 1, 2);
    WLX_Text_Edit_State *c = ime_caret(&ctx);
    ASSERT_EQ_INT(3, (int)c->cursor_pos);
    ASSERT_EQ_INT(3, (int)c->selection_anchor);
    ASSERT_FALSE(wlx_text_edit_has_selection(c));
    // The clause "ih": two units (10 px) one row tall, at x = 9 + 3 units.
    int band = ime_find_rect(10.0f, 10.0f);
    ASSERT_TRUE(band >= 0);
    ASSERT_EQ_F(ime_draws[band].x, 9.0f + 15.0f, 0.01f);
    // The underline covers the whole span (5 units).
    int ul = ime_find_underline(25.0f);
    ASSERT_TRUE(ul >= 0);
    ASSERT_EQ_F(ime_draws[ul].x, 9.0f + 10.0f, 0.01f);
    // No clause: no band of that shape.
    ime_reset_draws();
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", 1, -1);
    ASSERT_TRUE(ime_find_rect(10.0f, 10.0f) < 0);
    wlx_context_destroy(&ctx);
}

TEST(ime_clause_highlight_range) {
    IME_FOR_SHAPES(ime_shapes, ime_clause_highlight_range_case);
}

// ============================================================================
// Commit, adopt, selection, capacity
// ============================================================================

// A commit lands where the span began as one journal step of its own; undo
// removes the whole clause and redo restores it.
static void ime_commit_lands_as_one_undo_step_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ime_compose(&ctx, buf, sizeof(buf), &len, "\xE6\x97\xA5\xE6\x9C\xAC", "", -1, -1);
    ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5\xE6\x9C\xAC");
    ASSERT_EQ_INT(8, (int)len);
    ASSERT_FALSE(wlx_text_edit_composing(ime_caret(&ctx)));
    ASSERT_EQ_INT(8, (int)ime_caret(&ctx)->cursor_pos);
    ASSERT_EQ_INT(2, (int)ime_undo_count(&ctx));
    WLX_Text_Undo_Entry e = ime_newest(&ctx);
    ASSERT_EQ_INT(WLX_TEXT_UNDO_CLS_COMPOSE, (int)e.cls);
    ASSERT_EQ_INT(2, (int)e.start);
    ASSERT_EQ_INT(6, (int)e.inserted_len);
    ASSERT_EQ_INT(0, (int)e.removed_len);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_EQ_INT(2, (int)ime_caret(&ctx)->cursor_pos);
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_Z, test_command_mod() | WLX_MOD_SHIFT);
    ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5\xE6\x9C\xAC");
    ASSERT_EQ_INT(2, (int)ime_undo_count(&ctx));
    // A second clause committed right after is its own step, not merged.
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "go", -1, -1);
    ime_compose(&ctx, buf, sizeof(buf), &len, "\xE8\xAA\x9E", "", -1, -1);
    ASSERT_EQ_INT(3, (int)ime_undo_count(&ctx));
    // And typing after a commit opens a new typed step.
    ime_type(&ctx, buf, sizeof(buf), &len, "x");
    ASSERT_EQ_INT(4, (int)ime_undo_count(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(ime_commit_lands_as_one_undo_step) {
    IME_FOR_SHAPES(ime_shapes, ime_commit_lands_as_one_undo_step_case);
}

// A commit and the next clause's first string arrive in one frame: the
// commit lands, the new span follows it.
static void ime_commit_and_next_clause_in_one_frame_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ime_compose(&ctx, buf, sizeof(buf), &len, "X", "h", -1, -1);
    ASSERT_EQ_STR(buf, "abXh");
    WLX_Text_Edit_State *c = ime_caret(&ctx);
    ASSERT_TRUE(wlx_text_edit_composing(c));
    ASSERT_EQ_INT(3, (int)c->preedit_start);
    ASSERT_EQ_INT(1, (int)c->preedit_len);
    ASSERT_EQ_INT(4, (int)c->cursor_pos);
    ASSERT_EQ_INT(2, (int)ime_undo_count(&ctx));
    WLX_Text_Undo_Entry e = ime_newest(&ctx);
    ASSERT_EQ_INT(WLX_TEXT_UNDO_CLS_COMPOSE, (int)e.cls);
    ASSERT_EQ_INT(2, (int)e.start);
    ASSERT_EQ_INT(1, (int)e.inserted_len);
    ASSERT_EQ_INT(4, (int)ime_expected_len(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(ime_commit_and_next_clause_in_one_frame) {
    IME_FOR_SHAPES(ime_shapes, ime_commit_and_next_clause_in_one_frame_case);
}

// Focus leaving with a span in place adopts it: the bytes stay and the
// journal holds the step a commit would have made.
static void ime_adopt_on_blur_matches_a_commit_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ime_blur(&ctx, buf, sizeof(buf), &len);
    ASSERT_FALSE(ime_last_focused);
    ASSERT_TRUE(ime_last_changed);
    ASSERT_EQ_STR(buf, "abnihon");
    WLX_Text_Edit_State *c = ime_caret(&ctx);
    ASSERT_FALSE(wlx_text_edit_composing(c));
    ASSERT_EQ_INT(0, (int)c->preedit_len);
    ASSERT_EQ_INT(7, (int)c->cursor_pos);
    ASSERT_FALSE(wlx_text_composing(&ctx));
    ASSERT_EQ_INT(2, (int)ime_undo_count(&ctx));
    WLX_Text_Undo_Entry e = ime_newest(&ctx);
    ASSERT_EQ_INT(WLX_TEXT_UNDO_CLS_COMPOSE, (int)e.cls);
    ASSERT_EQ_INT(2, (int)e.start);
    ASSERT_EQ_INT(5, (int)e.inserted_len);
    ASSERT_EQ_INT(7, (int)ime_expected_len(&ctx));
    // Refocus and undo: the adopted clause goes as one step.
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_STR(buf, "ab");
    wlx_context_destroy(&ctx);
}

TEST(ime_adopt_on_blur_matches_a_commit) {
    IME_FOR_SHAPES(ime_shapes, ime_adopt_on_blur_matches_a_commit_case);
}

// Composing over a selection deletes it first as a step of its own; a
// cancelled composition leaves that step, and undo restores the selection.
static void ime_selection_deleted_at_start_stands_alone_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, WLX_MOD_SHIFT);
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "n", -1, -1);
    ASSERT_EQ_STR(buf, "an");
    WLX_Text_Edit_State *c = ime_caret(&ctx);
    ASSERT_EQ_INT(1, (int)c->preedit_start);
    ASSERT_EQ_INT(2, (int)c->cursor_pos);
    ASSERT_EQ_INT(2, (int)ime_undo_count(&ctx));
    WLX_Text_Undo_Entry e = ime_newest(&ctx);
    ASSERT_EQ_INT(WLX_TEXT_UNDO_CLS_SELECTION, (int)e.cls);
    ASSERT_EQ_INT(1, (int)e.start);
    ASSERT_EQ_INT(1, (int)e.removed_len);
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "", -1, -1);
    ASSERT_EQ_STR(buf, "a");
    ASSERT_EQ_INT(2, (int)ime_undo_count(&ctx));
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_STR(buf, "");
    wlx_context_destroy(&ctx);
}

TEST(ime_selection_deleted_at_start_stands_alone) {
    IME_FOR_SHAPES(ime_shapes, ime_selection_deleted_at_start_stands_alone_case);
}

// A string that does not fit lands whole clusters only, on the buffer's
// capacity; the span is what landed.
TEST(ime_capacity_truncates_on_a_cluster_boundary) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
    char buf[8] = "";    // 7 bytes of text
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "\xE6\x97\xA5\xE6\x9C\xAC", -1, -1);
    ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5");
    ASSERT_EQ_INT(5, (int)len);
    WLX_Text_Edit_State *c = ime_caret(&ctx);
    ASSERT_TRUE(wlx_text_edit_composing(c));
    ASSERT_EQ_INT(3, (int)c->preedit_len);
    ASSERT_EQ_INT(5, (int)c->cursor_pos);
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "", -1, -1);
    ASSERT_EQ_STR(buf, "ab");
    wlx_context_destroy(&ctx);
}

// A read-only field never takes a composition string and never asks for
// text input.
static void ime_read_only_never_composes_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    ime_read_only = true;
    char buf[64] = "ab";
    size_t len = 2;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ASSERT_TRUE(ime_last_focused);
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "ni", -1, -1);
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_FALSE(wlx_text_edit_composing(ime_caret(&ctx)));
    ASSERT_FALSE(wlx_text_composing(&ctx));
    ASSERT_EQ_INT(0, mock_tia_calls());
    ASSERT_FALSE(wlx_text_input_area(&ctx).active);
    wlx_context_destroy(&ctx);
}

TEST(ime_read_only_never_composes) {
    IME_FOR_SHAPES(ime_shapes, ime_read_only_never_composes_case);
}

// A masked field masks the span like the rest and underlines its mask
// glyphs; no plaintext reaches a draw call.
TEST(ime_password_masks_the_span) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
    ime_password = true;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_reset_draws();
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "ni", -1, -1);
    ASSERT_EQ_STR(buf, "abni");
    ASSERT_TRUE(wlx_text_edit_composing(ime_caret(&ctx)));
    bool plaintext_drawn = false;
    int pieces = 0;
    for (int i = 0; i < ime_draw_count; i++) {
        if (ime_draws[i].kind != 1) continue;
        pieces++;
        if (strstr(ime_draws[i].text, "ni") != NULL) plaintext_drawn = true;
    }
    ASSERT_TRUE(pieces > 0);
    ASSERT_FALSE(plaintext_drawn);
    // The underline covers two mask glyphs.
    bool underlined = false;
    for (int i = 0; i < ime_draw_count; i++) {
        if (ime_draws[i].kind == 2 && ime_draws[i].y == ime_draws[i].y2
            && ime_draws[i].x2 > ime_draws[i].x) underlined = true;
    }
    ASSERT_TRUE(underlined);
    ASSERT_TRUE(wlx_text_input_area(&ctx).password);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Forget rules, the input method owning the keys, the anchor
// ============================================================================

// A span is never deleted on stale evidence: a refocus after the widget
// was not drawn forgets it, an outside change of the buffer forgets it,
// the editor's revision bump forgets it; the bytes stay in every case. A
// widget drawn without focus while it holds a span adopts it instead.
static void ime_forget_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;

    // Not drawn for a frame while composing: focus is released without
    // the widget running. Drawn unfocused next, it adopts the span it
    // still holds, as a blur would: plain text, not underlined, with the
    // step a commit makes. Then a click refocuses.
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "ni", -1, -1);
    ASSERT_TRUE(wlx_text_edit_composing(ime_caret(&ctx)));
    ime_skip_widget = true;
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ime_skip_widget = false;
    ime_reset_draws();
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ASSERT_FALSE(ime_last_focused);
    ASSERT_EQ_STR(buf, "abni");
    ASSERT_EQ_INT(0, (int)ime_caret(&ctx)->preedit_len);   // adopted: held without focus
    ASSERT_EQ_INT(2, (int)ime_undo_count(&ctx));
    IME_EXPECT_PIECE(ime_piece_index(0), "abni", 9.0f, ime_row_y(w));
    ASSERT_TRUE(ime_find_underline(10.0f) < 0);
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ASSERT_TRUE(ime_last_focused);
    ASSERT_EQ_STR(buf, "abni");
    ASSERT_FALSE(wlx_text_edit_composing(ime_caret(&ctx)));
    ASSERT_EQ_INT(0, (int)ime_caret(&ctx)->preedit_len);
    ASSERT_EQ_INT(2, (int)ime_undo_count(&ctx));

    // The application rewrites the buffer while a span is in place.
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "go", -1, -1);
    ASSERT_EQ_STR(buf, "abnigo");
    strcpy(buf, "zz");
    len = 2;
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "", -1, -1);
    ASSERT_EQ_STR(buf, "zz");
    ASSERT_FALSE(wlx_text_edit_composing(ime_caret(&ctx)));
    ASSERT_EQ_INT(0, (int)ime_caret(&ctx)->preedit_len);

    // Not drawn while composing, then a click on the frame it is drawn
    // again: the focus frame forgets the span recorded under the earlier
    // focus, and its bytes stay. Kept, the span would be dropped as a
    // cancelled composition.
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, test_command_mod());
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "ni", -1, -1);
    ASSERT_EQ_STR(buf, "zzni");
    ime_skip_widget = true;
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ime_skip_widget = false;
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ASSERT_TRUE(ime_last_focused);
    ASSERT_EQ_STR(buf, "zzni");
    ASSERT_FALSE(wlx_text_edit_composing(ime_caret(&ctx)));
    wlx_context_destroy(&ctx);
}

TEST(ime_forget_on_focus_and_on_external_change) {
    IME_FOR_SHAPES(ime_shapes, ime_forget_case);

    // The editor: an outside rewrite of the same length with a revision
    // bump (the one case the length guard cannot see) forgets the span;
    // the bytes the application wrote stay.
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_EDITOR;
    char doc[64] = "ab\ncd";
    size_t dlen = 5;
    ime_idle(&ctx, doc, sizeof(doc), &dlen);
    ime_focus(&ctx, doc, sizeof(doc), &dlen);
    ime_key(&ctx, doc, sizeof(doc), &dlen, WLX_KEY_END, test_command_mod());
    ime_compose(&ctx, doc, sizeof(doc), &dlen, NULL, "ni", -1, -1);
    ASSERT_EQ_STR(doc, "ab\ncdni");
    ASSERT_TRUE(wlx_text_edit_composing(ime_caret(&ctx)));
    memcpy(doc, "xy\nzw\nq", 7);
    dlen = 7;
    doc[7] = '\0';
    ime_revision = 1;
    ime_compose(&ctx, doc, sizeof(doc), &dlen, NULL, "ni", -1, -1);
    // The same string re-applies at the (clamped) caret as a fresh span;
    // every byte the application wrote is still there.
    ASSERT_EQ_INT(9, (int)dlen);
    ASSERT_TRUE(strstr(doc, "xy\nzw") != NULL || strstr(doc, "zw\nq") != NULL);
    ASSERT_TRUE(strstr(doc, "ni") != NULL);
    ASSERT_TRUE(wlx_text_edit_composing(ime_caret(&ctx)));
    ASSERT_EQ_INT(2, (int)ime_caret(&ctx)->preedit_len);
    wlx_context_destroy(&ctx);
}

// While a span is in the buffer, keys and clicks change nothing: the input
// method owns them.
static void ime_keys_and_mouse_are_off_while_composing_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "ni", -1, -1);
    ASSERT_EQ_INT(4, (int)ime_caret(&ctx)->cursor_pos);
    ime_compose_key(&ctx, buf, sizeof(buf), &len, "ni", 200, 50, false, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "abni");
    ime_compose_key(&ctx, buf, sizeof(buf), &len, "ni", 200, 50, false, WLX_KEY_LEFT, 0);
    ASSERT_EQ_INT(4, (int)ime_caret(&ctx)->cursor_pos);
    ime_compose_key(&ctx, buf, sizeof(buf), &len, "ni", 200, 50, false, WLX_KEY_HOME, 0);
    ASSERT_EQ_INT(4, (int)ime_caret(&ctx)->cursor_pos);
    ime_compose_key(&ctx, buf, sizeof(buf), &len, "ni", 200, 50, false, WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_STR(buf, "abni");
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    ime_compose_key(&ctx, buf, sizeof(buf), &len, "ni", 200, 50, false, WLX_KEY_A, test_command_mod());
    ASSERT_FALSE(wlx_text_edit_has_selection(ime_caret(&ctx)));
    // Enter and Escape blur a single-line field otherwise (and the blur
    // would adopt the string).
    ime_compose_key(&ctx, buf, sizeof(buf), &len, "ni", 200, 50, false, WLX_KEY_ENTER, 0);
    ASSERT_TRUE(ime_last_focused);
    ASSERT_TRUE(wlx_text_edit_composing(ime_caret(&ctx)));
    ASSERT_EQ_STR(buf, "abni");
    ime_compose_key(&ctx, buf, sizeof(buf), &len, "ni", 200, 50, false, WLX_KEY_ESCAPE, 0);
    ASSERT_TRUE(ime_last_focused);
    ASSERT_TRUE(wlx_text_edit_composing(ime_caret(&ctx)));
    ASSERT_EQ_STR(buf, "abni");
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    // A click at the field's start would place the caret at 0 otherwise.
    ime_compose_key(&ctx, buf, sizeof(buf), &len, "ni", 10, 50, true, WLX_KEY_NONE, 0);
    ASSERT_EQ_INT(4, (int)ime_caret(&ctx)->cursor_pos);
    ASSERT_TRUE(ime_last_focused);
    ASSERT_TRUE(wlx_text_edit_composing(ime_caret(&ctx)));
    wlx_context_destroy(&ctx);
}

// The motion keys the multi-line shapes add: UP / DOWN, PageUp / PageDown
// and HOME / END. The span sits mid-line on the middle of three lines, so
// each key would move the caret (END included: the line goes on after the
// span); a key frame must leave the caret, the anchor and the bytes as the
// input method placed them. The textarea has no paging keys, so those two
// frames are inert there for a different reason.
static void ime_keys_off_vertical_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "aa\nbb\ncc";
    size_t len = 8;

    ime_idle(&ctx, buf, sizeof(buf), &len);   // the editor's index exists before the press
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, test_command_mod());
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_UP, 0);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ASSERT_EQ_INT(4, (int)ime_caret(&ctx)->cursor_pos);   // "b|b" on the middle line
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "ni", -1, -1);
    ASSERT_EQ_STR(buf, "aa\nbnib\ncc");
    ASSERT_EQ_INT(6, (int)ime_caret(&ctx)->cursor_pos);
    static const WLX_Key_Code keys[] = {
        WLX_KEY_UP, WLX_KEY_DOWN, WLX_KEY_PAGE_UP, WLX_KEY_PAGE_DOWN, WLX_KEY_HOME, WLX_KEY_END,
    };
    for (size_t k = 0; k < sizeof keys / sizeof keys[0]; k++) {
        ime_compose_key(&ctx, buf, sizeof(buf), &len, "ni", 200, 50, false, keys[k], 0);
        ASSERT_EQ_STR(buf, "aa\nbnib\ncc");
        ASSERT_EQ_INT(6, (int)ime_caret(&ctx)->cursor_pos);
        ASSERT_EQ_INT(6, (int)ime_caret(&ctx)->selection_anchor);
        ASSERT_TRUE(wlx_text_edit_composing(ime_caret(&ctx)));
    }
    // The commit lands where the span began: the keys moved nothing.
    ime_compose(&ctx, buf, sizeof(buf), &len, "NI", "", -1, -1);
    ASSERT_EQ_STR(buf, "aa\nbNIb\ncc");
    ASSERT_EQ_INT(6, (int)ime_caret(&ctx)->cursor_pos);
    wlx_context_destroy(&ctx);
}

TEST(ime_keys_and_mouse_are_off_while_composing) {
    IME_FOR_SHAPES(ime_shapes, ime_keys_and_mouse_are_off_while_composing_case);
    IME_FOR_SHAPES(ime_multiline_shapes, ime_keys_off_vertical_case);
}

// The composition anchor is pushed on the edges and on change only: the
// caret line and offset of the focused editable field, inactive after blur.
TEST(ime_area_pushed_on_edges_and_change) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
    char buf[64] = "";
    size_t len = 0;

    ime_idle(&ctx, buf, sizeof(buf), &len);
    ASSERT_EQ_INT(0, mock_tia_calls());                     // nothing changed from inactive
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ASSERT_EQ_INT(1, mock_tia_calls());
    WLX_Text_Input_Area a = mock_tia_last();
    ASSERT_TRUE(a.active);
    ASSERT_FALSE(a.multiline);
    ASSERT_FALSE(a.password);
    ASSERT_EQ_F(a.line.h, 10.0f, 0.01f);
    ASSERT_TRUE(a.line.w > 100.0f);
    ASSERT_TRUE(a.cursor >= 0.0f);
    float cursor0 = a.cursor;
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ASSERT_EQ_INT(2, mock_tia_calls());
    ASSERT_EQ_F(mock_tia_last().cursor, cursor0 + 10.0f, 0.01f);
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ASSERT_EQ_INT(2, mock_tia_calls());                     // unchanged frames push nothing
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ASSERT_EQ_INT(3, mock_tia_calls());
    ASSERT_EQ_F(mock_tia_last().cursor, cursor0 + 5.0f, 0.01f);
    ime_blur(&ctx, buf, sizeof(buf), &len);
    ASSERT_EQ_INT(4, mock_tia_calls());
    ASSERT_FALSE(mock_tia_last().active);
    ASSERT_FALSE(wlx_text_input_area(&ctx).active);
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ASSERT_EQ_INT(4, mock_tia_calls());
    wlx_context_destroy(&ctx);

    // The editor reports multiline and its caret row.
    ime_ctx_init(&ctx);
    ime_widget = IME_W_EDITOR;
    char doc[64] = "ab\ncd";
    size_t dlen = 5;
    ime_idle(&ctx, doc, sizeof(doc), &dlen);
    ASSERT_EQ_INT(0, mock_tia_calls());
    ime_focus(&ctx, doc, sizeof(doc), &dlen);
    ASSERT_TRUE(mock_tia_calls() >= 1);
    a = mock_tia_last();
    ASSERT_TRUE(a.active);
    ASSERT_TRUE(a.multiline);
    ASSERT_EQ_F(a.line.h, 10.0f, 0.01f);
    // The focusing click landed on the second row; UP moves the caret to
    // the first, and the anchor's line follows by one row.
    int calls = mock_tia_calls();
    ime_key(&ctx, doc, sizeof(doc), &dlen, WLX_KEY_UP, 0);
    ASSERT_TRUE(mock_tia_calls() > calls);
    ASSERT_EQ_F(mock_tia_last().line.y, a.line.y - 10.0f, 0.01f);
    ime_blur(&ctx, doc, sizeof(doc), &dlen);
    ASSERT_FALSE(mock_tia_last().active);
    wlx_context_destroy(&ctx);
}

// Wrapped editor: a span that overflows the row reflows into a new row and
// the caret follows into it, as any typed bytes would.
TEST(ime_editor_wrapped_span_reflows) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_EDITOR_WRAP;
    char doc[128];
    size_t dlen = 70;
    memset(doc, 'a', dlen);
    doc[dlen] = '\0';

    ime_idle(&ctx, doc, sizeof(doc), &dlen);
    ime_focus(&ctx, doc, sizeof(doc), &dlen);
    ime_key(&ctx, doc, sizeof(doc), &dlen, WLX_KEY_END, 0);
    ime_reset_draws();
    ime_idle(&ctx, doc, sizeof(doc), &dlen);
    int before = ime_count_pieces();
    ASSERT_EQ_INT(1, before);
    ime_reset_draws();
    ime_compose(&ctx, doc, sizeof(doc), &dlen, NULL, "0123456789", -1, -1);
    ASSERT_EQ_INT(80, (int)dlen);
    ASSERT_EQ_INT(2, ime_count_pieces());
    ASSERT_EQ_INT(80, (int)ime_caret(&ctx)->cursor_pos);
    ime_reset_draws();
    ime_compose(&ctx, doc, sizeof(doc), &dlen, NULL, "", -1, -1);
    ASSERT_EQ_INT(70, (int)dlen);
    ASSERT_EQ_INT(1, ime_count_pieces());
    wlx_context_destroy(&ctx);
}

// Linear editor: the span draws and underlines on its row, and the line
// index is patched across the composing and commit frames (no rebuild).
TEST(ime_editor_linear_span_and_index_patch) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_EDITOR;
    char doc[64] = "ab\ncd";
    size_t dlen = 5;

    ime_idle(&ctx, doc, sizeof(doc), &dlen);
    ime_focus(&ctx, doc, sizeof(doc), &dlen);
    ime_key(&ctx, doc, sizeof(doc), &dlen, WLX_KEY_END, test_command_mod());
    ASSERT_EQ_INT(5, (int)ime_caret(&ctx)->cursor_pos);
    uint32_t rebuilds = ime_editor_rebuilds(&ctx);
    ime_reset_draws();
    ime_compose(&ctx, doc, sizeof(doc), &dlen, NULL, "ni", -1, -1);
    ASSERT_EQ_STR(doc, "ab\ncdni");
    ASSERT_EQ_INT(7, (int)dlen);
    IME_EXPECT_PIECE(ime_piece_index(1), "cdni", 9.0f, 44.0f);
    int ul = ime_find_underline(10.0f);
    ASSERT_TRUE(ul >= 0);
    ASSERT_EQ_F(ime_draws[ul].x, 9.0f + 10.0f, 0.01f);
    ASSERT_TRUE(ime_draws[ul].y > 44.0f && ime_draws[ul].y <= 54.0f);
    ime_compose(&ctx, doc, sizeof(doc), &dlen, NULL, "nihon", -1, -1);
    ime_compose(&ctx, doc, sizeof(doc), &dlen, "\xE6\x97\xA5", "", -1, -1);
    ASSERT_EQ_STR(doc, "ab\ncd\xE6\x97\xA5");
    ASSERT_EQ_INT(rebuilds, ime_editor_rebuilds(&ctx));
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(WLX_TEXT_UNDO_CLS_COMPOSE, (int)ime_newest(&ctx).cls);
    // The index agrees with a fresh scan: two lines, the second at 3.
    ASSERT_EQ_INT(2, (int)ctx.editor_indices.items[0].count);
    ASSERT_EQ_INT(3, (int)ctx.editor_indices.items[0].offsets[1]);
    ime_key(&ctx, doc, sizeof(doc), &dlen, WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_STR(doc, "ab\ncd");
    ASSERT_EQ_INT(rebuilds, ime_editor_rebuilds(&ctx));
    wlx_context_destroy(&ctx);
}

// One frame over two inputboxes stacked in the 400x100 context (the first
// in the upper half, the second in the lower), from a staged pointer and
// composition string (NULL = none).
static void ime_two_fields_key(WLX_Context *ctx, char *first, char *second, size_t cap,
                               int my, bool clicked, const char *text_input,
                               const char *preedit, WLX_Key_Code key)
{
    WLX_Input_State in;
    memset(&in, 0, sizeof(in));
    in.mouse_x = 200;
    in.mouse_y = my;
    in.mouse_down = clicked;
    in.mouse_clicked = clicked;
    in.mouse_held = clicked;
    if (key != WLX_KEY_NONE) { in.keys_pressed[key] = true; in.keys_down[key] = true; }
    if (text_input != NULL) {
        size_t n = strlen(text_input);
        if (n >= sizeof(in.text_input)) n = sizeof(in.text_input) - 1;
        memcpy(in.text_input, text_input, n);
    }
    if (preedit != NULL) {
        size_t n = strlen(preedit);
        if (n >= sizeof(in.preedit)) n = sizeof(in.preedit) - 1;
        memcpy(in.preedit, preedit, n);
    }
    in.preedit_cursor = -1;
    in.preedit_sel_len = -1;
    test_frame_begin_input(ctx, &in);
    wlx_layout_begin(ctx, 2, WLX_VERT, .padding = 0, .gap = 0);
    wlx_inputbox_impl(ctx, NULL, first, cap,
        wlx_default_inputbox_opt(.height = 40, .content_padding = 4, .font_size = 10,
            .border_width = 0), __FILE__, __LINE__);
    wlx_inputbox_impl(ctx, NULL, second, cap,
        wlx_default_inputbox_opt(.height = 40, .content_padding = 4, .font_size = 10,
            .border_width = 0), __FILE__, __LINE__);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void ime_two_fields(WLX_Context *ctx, char *first, char *second, size_t cap,
                           int my, bool clicked, const char *preedit)
{
    ime_two_fields_key(ctx, first, second, cap, my, clicked, NULL, preedit, WLX_KEY_NONE);
}

// Compose in the field at from_y, then press the field at to_y while the
// backend still carries the string: the leaving field keeps it as a commit
// would, the entering field never shows it, and the backend gets one
// inactive frame between the two owners before the new anchor.
static void ime_focus_move_body(int from_y, int to_y) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    char upper[64] = "ab";
    char lower[64] = "cd";
    char *from = from_y < 50 ? upper : lower;
    char *to = from_y < 50 ? lower : upper;
    const char *from_composed = from_y < 50 ? "abho" : "cdho";
    const char *to_plain = from_y < 50 ? "cd" : "ab";
    const char *to_composed = from_y < 50 ? "cdx" : "abx";

    ime_two_fields(&ctx, upper, lower, sizeof(upper), from_y, false, NULL);
    ime_two_fields(&ctx, upper, lower, sizeof(upper), from_y, true, NULL);
    ime_two_fields(&ctx, upper, lower, sizeof(upper), from_y, false, NULL);
    ASSERT_EQ_INT(1, mock_tia_calls());
    ASSERT_TRUE(mock_tia_last().active);
    float from_line_y = mock_tia_last().line.y;
    ime_two_fields(&ctx, upper, lower, sizeof(upper), from_y, false, "ho");
    ASSERT_EQ_STR(from, from_composed);
    ASSERT_TRUE(wlx_text_composing(&ctx));
    int calls = mock_tia_calls();

    // The press on the other field; the string is still in the input.
    ime_two_fields(&ctx, upper, lower, sizeof(upper), to_y, true, "ho");
    ASSERT_EQ_STR(from, from_composed);                     // adopted
    ASSERT_EQ_STR(to, to_plain);                            // never shown
    ASSERT_FALSE(wlx_text_composing(&ctx));
    ASSERT_EQ_INT(calls + 1, mock_tia_calls());
    ASSERT_FALSE(mock_tia_last().active);                   // the loss, alone
    ASSERT_FALSE(wlx_text_input_area(&ctx).active);

    // The backend dropped its string on the inactive edge; the new owner's
    // anchor follows.
    ime_two_fields(&ctx, upper, lower, sizeof(upper), to_y, false, NULL);
    ASSERT_EQ_INT(calls + 2, mock_tia_calls());
    ASSERT_TRUE(mock_tia_last().active);
    ASSERT_TRUE(mock_tia_last().line.y != from_line_y);
    ASSERT_EQ_STR(from, from_composed);
    ASSERT_EQ_STR(to, to_plain);

    // A composition started now is the entering field's own.
    ime_two_fields(&ctx, upper, lower, sizeof(upper), to_y, false, "x");
    ASSERT_EQ_STR(to, to_composed);
    ASSERT_EQ_STR(from, from_composed);
    ASSERT_TRUE(wlx_text_composing(&ctx));
    ime_two_fields(&ctx, upper, lower, sizeof(upper), to_y, false, NULL);
    ASSERT_EQ_STR(to, to_plain);
    ASSERT_FALSE(wlx_text_composing(&ctx));
    wlx_context_destroy(&ctx);
}

// Both layout orders: the entering field runs after the leaving one, and
// before it.
TEST(ime_focus_move_to_a_later_field_ends_the_composition) { ime_focus_move_body(25, 75); }
TEST(ime_focus_move_to_an_earlier_field_ends_the_composition) { ime_focus_move_body(75, 25); }

// ============================================================================
// A held string: the input method stages the same bytes frame after frame
// ============================================================================

// The frames between two keystrokes of a clause keep the span in place:
// no byte moves, the widget reports no change, the editor's index is not
// patched and the caret blinks as on any idle frame. A cursor move inside
// the unchanged string moves the caret (a caret change, not a text
// change); a different clause is decoration only. The commit after it
// lands as one step, as without the hold.
static void ime_held_string_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ASSERT_TRUE(ime_last_changed);
    ASSERT_EQ_STR(buf, "abnihon");
    uint32_t patches = ime_editor_patches(&ctx);
    uint32_t rebuilds = ime_editor_rebuilds(&ctx);
    for (int i = 0; i < 40; i++) {
        ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
        ASSERT_FALSE(ime_last_changed);
        ASSERT_TRUE(wlx_text_composing(&ctx));
    }
    ASSERT_EQ_STR(buf, "abnihon");
    ASSERT_EQ_INT(7, (int)len);
    ASSERT_EQ_INT(patches, ime_editor_patches(&ctx));
    ASSERT_EQ_INT(rebuilds, ime_editor_rebuilds(&ctx));
    WLX_Text_Edit_State *c = ime_caret(&ctx);
    ASSERT_TRUE(wlx_text_edit_composing(c));
    ASSERT_EQ_INT(2, (int)c->preedit_start);
    ASSERT_EQ_INT(5, (int)c->preedit_len);
    ASSERT_EQ_INT(7, (int)c->cursor_pos);
    ASSERT_TRUE(c->cursor_blink_time > 0.5f);               // 40 frames at 60 fps, never reset
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(7, (int)ime_expected_len(&ctx));

    // The input method moves its cursor inside the same string.
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", 2, -1);
    ASSERT_FALSE(ime_last_changed);
    ASSERT_EQ_INT(4, (int)c->cursor_pos);
    ASSERT_EQ_INT(4, (int)c->selection_anchor);
    ASSERT_TRUE(c->cursor_blink_time < 0.02f);              // reset, then one frame's time
    // A clause selected over the same string.
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", 2, 3);
    ASSERT_FALSE(ime_last_changed);
    ASSERT_EQ_INT(4, (int)c->cursor_pos);
    ASSERT_EQ_STR(buf, "abnihon");
    ASSERT_EQ_INT(patches, ime_editor_patches(&ctx));

    ime_compose(&ctx, buf, sizeof(buf), &len, "\xE6\x97\xA5", "", -1, -1);
    ASSERT_TRUE(ime_last_changed);
    ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5");
    ASSERT_FALSE(wlx_text_composing(&ctx));
    ASSERT_EQ_INT(2, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(WLX_TEXT_UNDO_CLS_COMPOSE, (int)ime_newest(&ctx).cls);
    ASSERT_EQ_INT(rebuilds, ime_editor_rebuilds(&ctx));

    // A commit beside the same string is not a hold: the commit lands
    // where the span began and the string follows it.
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "go", -1, -1);
    ime_compose(&ctx, buf, sizeof(buf), &len, "G", "go", -1, -1);
    ASSERT_TRUE(ime_last_changed);
    ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5Ggo");
    ASSERT_EQ_INT(6, (int)c->preedit_start);
    ASSERT_EQ_INT(3, (int)ime_undo_count(&ctx));

    // A field that turns read-only under a held string drops the span:
    // only an editable field keeps one.
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "go", -1, -1);
    ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5Ggo");
    ime_read_only = true;
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "go", -1, -1);
    ASSERT_TRUE(ime_last_changed);
    ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5G");
    ASSERT_FALSE(wlx_text_composing(&ctx));
    ASSERT_EQ_INT(3, (int)ime_undo_count(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(ime_held_string_keeps_its_span) {
    IME_FOR_SHAPES(ime_shapes, ime_held_string_case);
}

// ============================================================================
// Composition anchor: active follows focus alone. The caret blinking off,
// a caret line scrolled out of the text band and an empty buffer keep it;
// the line stays inside the band and the cursor inside the line.
// ============================================================================

// How far the fixture widget's view is scrolled down from the top: the
// inputbox's pixel offset, the editor's first visible line and row.
static float ime_view_scrolled(WLX_Context *ctx) {
    void *data = (void *)ime_caret(ctx);
    if (data == NULL) return 0.0f;
    if (ime_widget == IME_W_INPUTBOX || ime_widget == IME_W_TEXTAREA) {
        return ((WLX_Inputbox_State *)data)->scroll_y;
    }
    const WLX_Editor_State *e = (const WLX_Editor_State *)data;
    return (float)(e->first_line + e->first_row) + e->y_frac;
}

// 150 idle frames at the mock's 60 fps cross five blink half-periods: the
// anchor stays active and nothing is pushed.
static void ime_anchor_idle_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "ab";
    size_t len = 2;
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ASSERT_TRUE(wlx_text_input_area(&ctx).active);
    int calls = mock_tia_calls();
    for (int i = 0; i < 150; i++) {
        ime_idle(&ctx, buf, sizeof(buf), &len);
        ASSERT_TRUE(wlx_text_input_area(&ctx).active);
        ASSERT_EQ_INT(calls, mock_tia_calls());
    }
    wlx_context_destroy(&ctx);
}

TEST(ime_anchor_holds_across_idle_frames) {
    IME_FOR_SHAPES(ime_shapes, ime_anchor_idle_case);
}

// The caret parked on the first line, the view wheeled down past it: the
// anchor stays active, its line pinned to the top of the band where the
// first line sat, the cursor inside the line.
static void ime_anchor_scrolled_out_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[256];
    size_t len = 0;
    for (int i = 0; i < 20; i++) len += (size_t)snprintf(buf + len, sizeof(buf) - len, "l%02d\n", i);
    ime_focus(&ctx, buf, sizeof(buf), &len);
    for (int i = 0; i < 25; i++) ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_UP, 0);
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ASSERT_EQ_F(0.0f, ime_view_scrolled(&ctx), 0.001f);
    WLX_Text_Input_Area top = wlx_text_input_area(&ctx);
    ASSERT_TRUE(top.active);
    for (int i = 0; i < 3; i++) ime_wheel(&ctx, buf, sizeof(buf), &len, -3.0f, 0);
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ASSERT_TRUE(ime_view_scrolled(&ctx) > 0.0f);           // the first line left the view
    WLX_Text_Input_Area a = wlx_text_input_area(&ctx);
    ASSERT_TRUE(a.active);
    ASSERT_EQ_F(top.line.x, a.line.x, 0.001f);
    ASSERT_EQ_F(top.line.w, a.line.w, 0.001f);
    ASSERT_EQ_F(top.line.h, a.line.h, 0.001f);
    ASSERT_EQ_F(top.line.y, a.line.y, 0.001f);
    ASSERT_TRUE(a.cursor >= 0.0f && a.cursor <= a.line.w);
    wlx_context_destroy(&ctx);
}

// The linear editor's caret at column 0 of a long line, the view wheeled
// right past it: the cursor pins to the line start, never negative.
static void ime_anchor_scrolled_out_horizontal_case(void) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_EDITOR;
    char buf[256];
    memset(buf, 'x', 200);
    buf[200] = '\0';
    size_t len = 200;
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_HOME, 0);
    for (int i = 0; i < 3; i++) ime_wheel(&ctx, buf, sizeof(buf), &len, -3.0f, WLX_MOD_SHIFT);
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ASSERT_EQ_INT(0, (int)ime_caret(&ctx)->cursor_pos);
    ASSERT_TRUE(((WLX_Editor_State *)(void *)ime_caret(&ctx))->scroll_x > 0.0f);
    WLX_Text_Input_Area a = wlx_text_input_area(&ctx);
    ASSERT_TRUE(a.active);
    ASSERT_EQ_F(0.0f, a.cursor, 0.001f);
    wlx_context_destroy(&ctx);
}

TEST(ime_anchor_holds_with_caret_scrolled_out) {
    IME_FOR_SHAPES(ime_multiline_shapes, ime_anchor_scrolled_out_case);
    ime_anchor_scrolled_out_horizontal_case();
}

// Index of the first captured vertical line (a caret) whose x lies in
// [x_lo, x_hi], or -1.
static int ime_find_caret_line(float x_lo, float x_hi) {
    for (int i = 0; i < ime_draw_count; i++) {
        if (ime_draws[i].kind == 2 && ime_draws[i].x == ime_draws[i].x2
            && ime_draws[i].y2 > ime_draws[i].y
            && ime_draws[i].x >= x_lo && ime_draws[i].x <= x_hi) return i;
    }
    return -1;
}

// An empty buffer: the focus frame already pushes an active anchor with
// the caret at the line start, and draws the caret there.
static void ime_anchor_empty_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ime_reset_draws();
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ASSERT_TRUE(mock_tia_calls() >= 1);
    ASSERT_TRUE(mock_tia_last().active);
    WLX_Text_Input_Area a = wlx_text_input_area(&ctx);
    ASSERT_TRUE(a.active);
    ASSERT_EQ_F(0.0f, a.cursor, 0.001f);
    int caret = ime_find_caret_line(a.line.x, a.line.x + 2.0f);
    ASSERT_TRUE(caret >= 0);
    ASSERT_TRUE(ime_draws[caret].y >= a.line.y - 0.01f);
    ASSERT_TRUE(ime_draws[caret].y2 <= a.line.y + a.line.h + 0.01f);
    wlx_context_destroy(&ctx);
}

TEST(ime_anchor_active_on_empty_focus_frame) {
    IME_FOR_SHAPES(ime_shapes, ime_anchor_empty_case);
}

// ============================================================================
// A text widget writes to the caller's buffer only on a frame whose text
// changes: never on an unfocused frame that adopts no span, never in a
// read-only field. The buffer here is read-only storage, so a stray write
// (a trailing NUL rewritten on a frame that changed nothing) faults the
// runner inside these tests.
// ============================================================================

static const char ime_ro_text[] = "ab\ncd";

static void ime_unfocused_never_writes_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char *buf = (char *)ime_ro_text;
    size_t len = sizeof(ime_ro_text) - 1;
    ime_idle(&ctx, buf, sizeof(ime_ro_text), &len);              // hovered
    ime_blur(&ctx, buf, sizeof(ime_ro_text), &len);              // a press elsewhere
    ime_wheel(&ctx, buf, sizeof(ime_ro_text), &len, -3.0f, 0);   // wheeled
    ime_compose(&ctx, buf, sizeof(ime_ro_text), &len, "x", "ni", -1, -1);   // input for someone else
    ime_idle(&ctx, buf, sizeof(ime_ro_text), &len);
    ASSERT_FALSE(ime_last_focused);
    ASSERT_FALSE(ime_last_changed);
    ASSERT_EQ_INT(5, (int)len);
    ASSERT_EQ_INT(0, mock_tia_calls());
    wlx_context_destroy(&ctx);
}

TEST(ime_unfocused_frames_never_write_the_buffer) {
    IME_FOR_SHAPES(ime_shapes, ime_unfocused_never_writes_case);
}

// A read-only field focused, navigated, selected, copied, typed and
// composed at, then blurred, over read-only storage: every key path runs,
// none writes.
static void ime_read_only_never_writes_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    ime_read_only = true;
    char *buf = (char *)ime_ro_text;
    size_t len = sizeof(ime_ro_text) - 1;
    ime_idle(&ctx, buf, sizeof(ime_ro_text), &len);
    ime_focus(&ctx, buf, sizeof(ime_ro_text), &len);
    ASSERT_TRUE(ime_last_focused);
    ime_key(&ctx, buf, sizeof(ime_ro_text), &len, WLX_KEY_LEFT, 0);
    ime_key(&ctx, buf, sizeof(ime_ro_text), &len, WLX_KEY_HOME, WLX_MOD_SHIFT);
    ime_key(&ctx, buf, sizeof(ime_ro_text), &len, WLX_KEY_A, test_command_mod());
    ime_key(&ctx, buf, sizeof(ime_ro_text), &len, WLX_KEY_C, test_command_mod());
    ime_key(&ctx, buf, sizeof(ime_ro_text), &len, WLX_KEY_BACKSPACE, 0);
    ime_type(&ctx, buf, sizeof(ime_ro_text), &len, "x");
    ime_compose(&ctx, buf, sizeof(ime_ro_text), &len, NULL, "ni", -1, -1);
    ime_blur(&ctx, buf, sizeof(ime_ro_text), &len);
    ASSERT_FALSE(ime_last_focused);
    ASSERT_FALSE(ime_last_changed);
    ASSERT_EQ_INT(5, (int)len);
    wlx_context_destroy(&ctx);
}

TEST(ime_read_only_never_writes) {
    IME_FOR_SHAPES(ime_shapes, ime_read_only_never_writes_case);
}

// ============================================================================
// Suite
// ============================================================================

// ============================================================================
// A string whose edge joins the text beside it: the caret sits where the
// input method put it, on a unit boundary of the string, which need not be
// one of the buffer; after the commit it is a buffer boundary again.
// ============================================================================

#define IME_MAN   "\xF0\x9F\x91\xA8"
#define IME_WOMAN "\xF0\x9F\x91\xA9"
#define IME_ZWJ   "\xE2\x80\x8D"

typedef struct {
    const char *before;   // typed before the composition
    int lefts;            // Left presses after typing
    const char *pre;      // the composition string
    int32_t cursor;       // its codepoint cursor
    size_t caret;         // the caret while composing (not a buffer boundary)
    const char *after;    // the buffer after the commit
} Ime_Edge_Case;

static const Ime_Edge_Case ime_edge_cases[] = {
    // The string's first unit (a mark) joins the 'e' before it; cursor 0.
    { "e", 0, "\xCC\x81" "x", 0, 1, "e\xCC\x81x" },
    // The mark after the span joins the string's last unit; cursor at the end.
    { "\xCC\x81", 1, "e", -1, 1, "e\xCC\x81" },
    // A ZWJ between two emoji joins both edges: no in-span offset is a
    // buffer boundary, at either end of the string.
    { IME_MAN IME_WOMAN, 1, IME_ZWJ, 0, 4, IME_MAN IME_ZWJ IME_WOMAN },
    { IME_MAN IME_WOMAN, 1, IME_ZWJ, -1, 7, IME_MAN IME_ZWJ IME_WOMAN },
};

static void ime_span_edge_case(Ime_Widget w, const Ime_Edge_Case *c) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, c->before);
    for (int i = 0; i < c->lefts; i++) ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    // Applied, then held one more frame (drawn, anchored, placed again).
    for (int frame = 0; frame < 2; frame++) {
        ime_compose(&ctx, buf, sizeof(buf), &len, NULL, c->pre, c->cursor, -1);
        ASSERT_EQ_INT((long)c->caret, (long)ime_caret(&ctx)->cursor_pos);
        ASSERT_FALSE(wlx_text_unit_boundary(buf, len, c->caret));
    }
    ime_compose(&ctx, buf, sizeof(buf), &len, c->pre, "", -1, -1);
    ASSERT_EQ_INT((long)strlen(c->after), (long)len);
    ASSERT_TRUE(memcmp(buf, c->after, len) == 0);
    ASSERT_TRUE(wlx_text_unit_boundary(buf, len, ime_caret(&ctx)->cursor_pos));
    wlx_context_destroy(&ctx);
}

TEST(ime_span_edge_joins_neighbour) {
    for (size_t i = 0; i < IME_SHAPE_COUNT(ime_shapes); i++) {
        for (size_t k = 0; k < sizeof ime_edge_cases / sizeof ime_edge_cases[0]; k++) {
            ime_span_edge_case(ime_shapes[i], &ime_edge_cases[k]);
        }
    }
}

// ============================================================================
// The frame a composition ends on is the input method's: a frame that
// begins with a span runs no key vocabulary, no Enter or Escape blur and
// no Tab walk. The next frame takes keys as usual.
// ============================================================================

// A confirming Enter the host also reports: the commit lands, the field
// keeps focus and no newline follows.
static void ime_end_frame_enter_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ime_end_key(&ctx, buf, sizeof(buf), &len, "\xE6\x97\xA5\xE6\x9C\xAC", "", WLX_KEY_ENTER, 0);
    ASSERT_TRUE(ime_last_focused);
    ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5\xE6\x9C\xAC");
    ASSERT_FALSE(wlx_text_composing(&ctx));
    ASSERT_EQ_INT(8, (int)ime_caret(&ctx)->cursor_pos);
    ASSERT_EQ_INT(2, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(WLX_TEXT_UNDO_CLS_COMPOSE, (int)ime_newest(&ctx).cls);
    ASSERT_EQ_INT(6, (int)ime_newest(&ctx).inserted_len);
    wlx_context_destroy(&ctx);
}

TEST(ime_end_frame_commit_plus_enter_keeps_the_commit) {
    IME_FOR_SHAPES(ime_shapes, ime_end_frame_enter_case);
}

// A cancelling Escape the host also reports, with the string cleared: the
// span goes, nothing is committed and the field keeps focus.
static void ime_end_frame_escape_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ime_end_key(&ctx, buf, sizeof(buf), &len, NULL, "", WLX_KEY_ESCAPE, 0);
    ASSERT_TRUE(ime_last_focused);
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_EQ_INT(2, (int)len);
    ASSERT_FALSE(wlx_text_composing(&ctx));
    ASSERT_EQ_INT(2, (int)ime_caret(&ctx)->cursor_pos);
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(0, (int)ime_redo_count(&ctx));
    ASSERT_EQ_INT(2, (int)ime_expected_len(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(ime_end_frame_escape_cancels_without_a_commit) {
    IME_FOR_SHAPES(ime_shapes, ime_end_frame_escape_case);
}

// The undo chord in the commit frame does not take the commit back.
static void ime_end_frame_undo_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ime_end_key(&ctx, buf, sizeof(buf), &len, "\xE6\x97\xA5", "", WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5");
    ASSERT_EQ_INT(2, (int)ime_undo_count(&ctx));
    ASSERT_EQ_INT(0, (int)ime_redo_count(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(ime_end_frame_commit_plus_undo_keeps_the_commit) {
    IME_FOR_SHAPES(ime_shapes, ime_end_frame_undo_case);
}

// After an end frame whose key was dropped, the next frame's keys act as
// they always do: Enter blurs the single-line field and breaks the line
// in the others, the undo chord takes the commit back as one step, and
// Escape blurs.
static void ime_next_frame_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "";
    size_t len = 0;
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ime_end_key(&ctx, buf, sizeof(buf), &len, "\xE6\x97\xA5", "", WLX_KEY_ENTER, 0);
    ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5");
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_ENTER, 0);
    if (w == IME_W_INPUTBOX) {
        ASSERT_FALSE(ime_last_focused);
        ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5");
        ime_focus(&ctx, buf, sizeof(buf), &len);
    } else {
        ASSERT_TRUE(ime_last_focused);
        ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5\n");
        ASSERT_EQ_INT(3, (int)ime_undo_count(&ctx));
        ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_Z, test_command_mod());
        ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5");
    }
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_TRUE(ime_last_focused);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_ESCAPE, 0);
    ASSERT_FALSE(ime_last_focused);
    wlx_context_destroy(&ctx);
}

TEST(ime_next_frame_after_the_end_frame_takes_keys) {
    IME_FOR_SHAPES(ime_shapes, ime_next_frame_case);
}

// Tab while a composition is held, and in the frame it commits on, stays
// with the composing field (no traversal); the next frame's Tab moves on.
TEST(ime_tab_walk_waits_for_the_composition) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    char upper[64] = "ab";
    char lower[64] = "cd";
    ime_two_fields(&ctx, upper, lower, sizeof(upper), 25, false, NULL);
    ime_two_fields(&ctx, upper, lower, sizeof(upper), 25, true, NULL);
    ime_two_fields(&ctx, upper, lower, sizeof(upper), 25, false, "ho");
    ASSERT_EQ_STR(upper, "abho");
    ime_two_fields_key(&ctx, upper, lower, sizeof(upper), 25, false, NULL, "ho", WLX_KEY_TAB);
    ASSERT_EQ_STR(upper, "abho");
    ASSERT_TRUE(wlx_text_composing(&ctx));
    ime_two_fields_key(&ctx, upper, lower, sizeof(upper), 25, false, "H", "", WLX_KEY_TAB);
    ASSERT_EQ_STR(upper, "abH");
    ASSERT_EQ_STR(lower, "cd");
    ASSERT_FALSE(wlx_text_composing(&ctx));
    ime_two_fields_key(&ctx, upper, lower, sizeof(upper), 25, false, "z", NULL, WLX_KEY_NONE);
    ASSERT_EQ_STR(upper, "abHz");                           // focus stayed
    ime_two_fields_key(&ctx, upper, lower, sizeof(upper), 25, false, NULL, NULL, WLX_KEY_TAB);
    ime_two_fields_key(&ctx, upper, lower, sizeof(upper), 25, false, "z", NULL, WLX_KEY_NONE);
    ASSERT_EQ_STR(upper, "abHz");
    ASSERT_EQ_STR(lower, "cdz");                            // the next frame's Tab moved it
    wlx_context_destroy(&ctx);
}

// ============================================================================
// A press and a Tab in one frame while composing (two fields, either kind)
// ============================================================================

typedef struct {
    bool editor;
    bool multiline;   // inputbox only: a textarea
    char buf[64];
    size_t len;
    bool focused;
    bool skip;        // not placed this frame (the field is not drawn)
} Ime_Pair_Field;

// One frame over two fields stacked in the 400x100 context (the first in
// the upper half, the second in the lower), from a staged pointer, one key,
// committed text and a composition string (NULL = none).
static void ime_pair_frame(WLX_Context *ctx, Ime_Pair_Field f[2], int my, bool clicked,
                           WLX_Key_Code key, const char *text, const char *preedit)
{
    WLX_Input_State in;
    memset(&in, 0, sizeof(in));
    in.mouse_x = 200;
    in.mouse_y = my;
    in.mouse_down = clicked;
    in.mouse_clicked = clicked;
    in.mouse_held = clicked;
    if (key != WLX_KEY_NONE) { in.keys_pressed[key] = true; in.keys_down[key] = true; }
    if (text != NULL) memcpy(in.text_input, text, strlen(text));
    if (preedit != NULL) memcpy(in.preedit, preedit, strlen(preedit));
    in.preedit_cursor = -1;
    in.preedit_sel_len = -1;
    test_frame_begin_input(ctx, &in);
    wlx_layout_begin(ctx, 2, WLX_VERT, .padding = 0, .gap = 0);
    for (int i = 0; i < 2; i++) {
        if (f[i].skip) continue;
        wlx_push_id(ctx, (size_t)i + 1);
        if (f[i].editor) {
            wlx_editor_impl(ctx, NULL, f[i].buf, sizeof(f[i].buf), &f[i].len,
                wlx_default_editor_opt(.height = 40, .content_padding = 4, .font_size = 10,
                    .border_width = 0, .out_focused = &f[i].focused), __FILE__, __LINE__);
        } else {
            wlx_inputbox_impl(ctx, NULL, f[i].buf, sizeof(f[i].buf),
                wlx_default_inputbox_opt(.height = 40, .content_padding = 4, .font_size = 10,
                    .border_width = 0, .multiline = f[i].multiline,
                    .out_focused = &f[i].focused), __FILE__, __LINE__);
            f[i].len = strlen(f[i].buf);
        }
        wlx_pop_id(ctx);
    }
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// Text widget states holding a tentative span (both widget states embed
// the caret state first, and the pair fixture holds no other state).
static int ime_spans_held(WLX_Context *ctx) {
    int n = 0;
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        if (ctx->states.slots[i].id == 0) continue;
        if (((const WLX_Text_Edit_State *)ctx->states.slots[i].data)->preedit_len > 0) n++;
    }
    return n;
}

// Fields "ab" (upper) and "cd" (lower); the field at index ci (an editor
// when ci_editor, else an inputbox, a textarea when ci_multiline) is
// focused and composing "ho". The other field is an editor when
// other_editor.
static void ime_pair_compose(WLX_Context *ctx, Ime_Pair_Field f[2], int ci,
                             bool ci_editor, bool ci_multiline, bool other_editor)
{
    memset(f, 0, 2 * sizeof(*f));
    f[ci].editor = ci_editor;
    f[ci].multiline = ci_multiline;
    f[1 - ci].editor = other_editor;
    strcpy(f[0].buf, "ab");
    f[0].len = 2;
    strcpy(f[1].buf, "cd");
    f[1].len = 2;
    int cy = ci == 0 ? 25 : 75;
    ime_pair_frame(ctx, f, cy, false, WLX_KEY_NONE, NULL, NULL);
    ime_pair_frame(ctx, f, cy, true, WLX_KEY_NONE, NULL, NULL);
    ime_pair_frame(ctx, f, cy, false, WLX_KEY_NONE, NULL, "ho");
}

// A press on the composing field and a Tab in one frame. Were the Tab
// walk to run, it would release the field and the press re-acquire it in
// the same query: the field would see its focus gained, forget its live
// span and re-apply the string, so the text would land twice. A
// composition held last frame stops the walk, so the field keeps its
// focus and the commit lands one copy.
static void ime_press_tab_one_copy_case(int ci, bool ci_multiline, bool other_editor) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    Ime_Pair_Field f[2];
    ime_pair_compose(&ctx, f, ci, false, ci_multiline, other_editor);
    const char *one_copy = ci == 0 ? "abho" : "cdho";
    const char *other = ci == 0 ? "cd" : "ab";
    int cy = ci == 0 ? 25 : 75;
    ASSERT_EQ_STR(f[ci].buf, one_copy);

    ime_pair_frame(&ctx, f, cy, true, WLX_KEY_TAB, NULL, "ho");
    ASSERT_EQ_STR(f[ci].buf, one_copy);
    ASSERT_EQ_STR(f[1 - ci].buf, other);
    if (ci == 1) {
        ASSERT_TRUE(f[ci].focused);
        ASSERT_TRUE(wlx_text_composing(&ctx));
        ime_pair_frame(&ctx, f, cy, false, WLX_KEY_NONE, "ho", NULL);
        ASSERT_EQ_STR(f[ci].buf, one_copy);
        ASSERT_FALSE(wlx_text_composing(&ctx));
    }
    wlx_context_destroy(&ctx);
}

TEST(ime_press_and_tab_on_the_composing_field_land_one_copy) {
    for (int ci = 0; ci < 2; ci++) {
        for (int m = 0; m < 2; m++) {
            for (int other_editor = 0; other_editor < 2; other_editor++) {
                ime_press_tab_one_copy_case(ci, m != 0, other_editor != 0);
            }
        }
    }
}

// The same frame with the Tab target queried after the composing field.
// Were the walk to run, the target would take the focus and drop it on
// the press, so the composing field would lose its focus without seeing
// it leave and keep a span no one owns. With the walk stopped, no field
// holds a span without focus the frame after, and the commit leaves none.
static void ime_press_tab_no_orphan_case(bool ci_multiline, bool other_editor) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    Ime_Pair_Field f[2];
    ime_pair_compose(&ctx, f, 0, false, ci_multiline, other_editor);

    ime_pair_frame(&ctx, f, 25, true, WLX_KEY_TAB, NULL, "ho");
    ime_pair_frame(&ctx, f, 25, false, WLX_KEY_NONE, NULL, "ho");
    ASSERT_EQ_INT(f[0].focused ? 1 : 0, ime_spans_held(&ctx));
    ASSERT_TRUE(strncmp(f[0].buf, "abho", 4) == 0);
    ASSERT_EQ_STR(f[1].buf, "cd");
    ime_pair_frame(&ctx, f, 25, false, WLX_KEY_NONE, "ho", NULL);
    ASSERT_EQ_INT(0, ime_spans_held(&ctx));
    ASSERT_FALSE(wlx_text_composing(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(ime_press_and_tab_on_the_composing_field_leave_no_orphan_span) {
    for (int m = 0; m < 2; m++) {
        for (int other_editor = 0; other_editor < 2; other_editor++) {
            ime_press_tab_no_orphan_case(m != 0, other_editor != 0);
        }
    }
}

// A press on the other field plus a Tab while composing: the composing
// field adopts its span on the frame focus leaves, and the other field
// never shows the string or the Tab, in both layout orders and with
// either kind on either side. The input method's Tab is no widget's: an
// editor that takes focus by the press does not insert it.
TEST(ime_press_elsewhere_and_tab_adopt_the_span) {
    for (int ci = 0; ci < 2; ci++) {
        for (int kinds = 0; kinds < 4; kinds++) {
            WLX_Context ctx;
            ime_ctx_init(&ctx);
            Ime_Pair_Field f[2];
            ime_pair_compose(&ctx, f, ci, (kinds & 1) != 0, false, (kinds & 2) != 0);
            const char *composed = ci == 0 ? "abho" : "cdho";
            const char *other = ci == 0 ? "cd" : "ab";
            ASSERT_EQ_STR(f[ci].buf, composed);
            int oy = ci == 0 ? 75 : 25;
            ime_pair_frame(&ctx, f, oy, true, WLX_KEY_TAB, NULL, "ho");
            ASSERT_FALSE(f[ci].focused);
            ASSERT_EQ_STR(f[ci].buf, composed);
            ASSERT_EQ_STR(f[1 - ci].buf, other);
            ASSERT_EQ_INT(0, ime_spans_held(&ctx));
            ASSERT_FALSE(wlx_text_composing(&ctx));
            wlx_context_destroy(&ctx);
        }
    }
}

// A commit and the next clause in one frame where the commit fills the
// buffer: the clause has no room and no span is applied, so the frame
// leaves the composition and its caret is the buffer's - normalized onto a
// unit boundary like any other keystroke's, here behind a combining mark
// the commit landed in front of.
static void ime_commit_fills_the_buffer_case(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[8] = "\xCC\x81";   // U+0301 alone
    size_t len = 2;
    // Three bytes of text: the inputbox keeps one for the NUL.
    size_t cap = (w == IME_W_INPUTBOX || w == IME_W_TEXTAREA) ? 4 : 3;
    ime_idle(&ctx, buf, cap, &len);
    ime_focus(&ctx, buf, cap, &len);
    ime_key(&ctx, buf, cap, &len, WLX_KEY_HOME, test_command_mod());
    ASSERT_EQ_INT(0, (int)ime_caret(&ctx)->cursor_pos);
    ime_compose(&ctx, buf, cap, &len, "e", "x", -1, -1);
    ASSERT_EQ_INT(3, (int)len);
    ASSERT_TRUE(memcmp(buf, "e\xCC\x81", 3) == 0);
    ASSERT_EQ_INT(0, (int)ime_caret(&ctx)->preedit_len);
    ASSERT_FALSE(wlx_text_composing(&ctx));
    ASSERT_TRUE(wlx_text_unit_boundary(buf, len, ime_caret(&ctx)->cursor_pos));
    ASSERT_TRUE(wlx_text_unit_boundary(buf, len, ime_caret(&ctx)->selection_anchor));
    wlx_context_destroy(&ctx);
}

TEST(ime_commit_that_fills_the_buffer_leaves_a_boundary_caret) {
    IME_FOR_SHAPES(ime_shapes, ime_commit_fills_the_buffer_case);
}

// The press and the Tab on the frame a composition starts: no composition
// was held last frame, so the Tab walk still runs, releases the field and
// leaves it to the press; a Tab target queried later can take the focus
// away without an edge. The span the field applied that frame is the live
// composition - a press on the field the next frame carries it on, one
// copy - and a field left without focus adopts it the next frame it runs.
static void ime_press_tab_at_start_case(bool ci_multiline, bool other_editor, bool repress) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    Ime_Pair_Field f[2];
    memset(f, 0, sizeof(f));
    f[0].multiline = ci_multiline;
    f[1].editor = other_editor;
    strcpy(f[0].buf, "ab");
    f[0].len = 2;
    strcpy(f[1].buf, "cd");
    f[1].len = 2;
    ime_pair_frame(&ctx, f, 25, false, WLX_KEY_NONE, NULL, NULL);
    ime_pair_frame(&ctx, f, 25, true, WLX_KEY_NONE, NULL, NULL);
    ime_pair_frame(&ctx, f, 25, false, WLX_KEY_NONE, NULL, NULL);

    ime_pair_frame(&ctx, f, 25, true, WLX_KEY_TAB, NULL, "ho");
    ASSERT_EQ_STR(f[0].buf, "abho");
    ime_pair_frame(&ctx, f, 25, repress, WLX_KEY_NONE, NULL, "ho");
    ASSERT_EQ_STR(f[0].buf, "abho");
    ASSERT_EQ_STR(f[1].buf, "cd");
    ASSERT_EQ_INT(f[0].focused ? 1 : 0, ime_spans_held(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(ime_press_and_tab_as_a_composition_starts) {
    for (int m = 0; m < 2; m++) {
        for (int other_editor = 0; other_editor < 2; other_editor++) {
            for (int repress = 0; repress < 2; repress++) {
                ime_press_tab_at_start_case(m != 0, other_editor != 0, repress != 0);
            }
        }
    }
}

// A composing field that is not drawn for a frame loses its focus at
// wlx_end without running; drawn again without focus, it adopts the span
// it still holds instead of keeping it until a refocus forgets it.
TEST(ime_span_held_by_an_undrawn_field_is_adopted_when_drawn) {
    for (int ci = 0; ci < 2; ci++) {
        for (int kinds = 0; kinds < 4; kinds++) {
            WLX_Context ctx;
            ime_ctx_init(&ctx);
            Ime_Pair_Field f[2];
            ime_pair_compose(&ctx, f, ci, (kinds & 1) != 0, false, (kinds & 2) != 0);
            const char *composed = ci == 0 ? "abho" : "cdho";
            ASSERT_EQ_STR(f[ci].buf, composed);
            // The composing field's journal is the only one: nothing typed yet.
            ASSERT_EQ_INT(1, (int)ctx.text_undo.count);
            ASSERT_EQ_INT(0, (int)ctx.text_undo.items[0].undo.count);
            f[ci].skip = true;
            ime_pair_frame(&ctx, f, 50, false, WLX_KEY_NONE, NULL, "ho");
            f[ci].skip = false;
            ime_pair_frame(&ctx, f, 50, false, WLX_KEY_NONE, NULL, NULL);
            ASSERT_FALSE(f[ci].focused);
            ASSERT_EQ_STR(f[ci].buf, composed);
            ASSERT_EQ_INT(0, ime_spans_held(&ctx));
            // Adopted, not forgotten: the step a commit would have made.
            ASSERT_EQ_INT(1, (int)ctx.text_undo.items[0].undo.count);
            wlx_context_destroy(&ctx);
        }
    }
}

// ============================================================================
// Word motion over text units (every shape: a rule of the shared edit phase)
// ============================================================================

// "xy ab " + U+0301: the space and the mark are one unit [5, 8). The word
// steps treat it as the separator it starts with, landing where a plain
// space would.
static void word_steps_body(Ime_Widget w) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "xy ab \xCC\x81";
    size_t len = strlen(buf);

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ASSERT_EQ_INT(8, (int)ime_caret(&ctx)->cursor_pos);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, WLX_MOD_CTRL);
    ASSERT_EQ_INT(3, (int)ime_caret(&ctx)->cursor_pos);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, 0);
    ASSERT_EQ_INT(8, (int)ime_caret(&ctx)->cursor_pos);
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, WLX_MOD_CTRL);
    ASSERT_EQ_STR(buf, "xy ");
    ASSERT_EQ_INT(3, (int)ime_caret(&ctx)->cursor_pos);
    wlx_context_destroy(&ctx);
}

TEST(word_steps_never_split_a_unit) {
    IME_FOR_SHAPES(ime_shapes, word_steps_body);
}

// A double-click at screen x on "x " + U+0301 + "yz" (text origin x 9,
// 5 px a byte): returns the selection as anchor / caret.
static void word_double_click(Ime_Widget w, int x, size_t *anchor, size_t *caret) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = w;
    char buf[64] = "x \xCC\x81yz";
    size_t len = strlen(buf);

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_idle(&ctx, buf, sizeof(buf), &len);
    for (int i = 0; i < 2; i++) {
        ime_frame(&ctx, buf, sizeof(buf), &len, x, 50, true, WLX_KEY_NONE, 0, NULL);
        ime_frame(&ctx, buf, sizeof(buf), &len, x, 50, false, WLX_KEY_NONE, 0, NULL);
    }
    *anchor = ime_caret(&ctx)->selection_anchor;
    *caret = ime_caret(&ctx)->cursor_pos;
    wlx_context_destroy(&ctx);
}

static void word_double_click_body(Ime_Widget w) {
    size_t anchor = 0, caret = 0;
    // On "yz" (x 30 hits offset 4): the word, without the mark.
    word_double_click(w, 30, &anchor, &caret);
    ASSERT_EQ_INT(4, (int)anchor);
    ASSERT_EQ_INT(6, (int)caret);
    // On the space (x 16 hits offset 1): the whole space + mark unit.
    word_double_click(w, 16, &anchor, &caret);
    ASSERT_EQ_INT(1, (int)anchor);
    ASSERT_EQ_INT(4, (int)caret);
}

TEST(word_double_click_never_splits_a_unit) {
    IME_FOR_SHAPES(ime_shapes, word_double_click_body);
}

SUITE(ime) {
    RUN_TEST(text_caret_blink_clock_bounded);
    RUN_TEST(ime_identity_inputbox);
    RUN_TEST(ime_identity_textarea);
    RUN_TEST(ime_identity_editor_linear);
    RUN_TEST(ime_identity_editor_wrapped);
    RUN_TEST(ime_tentative_span_shows_and_journal_untouched);
    RUN_TEST(ime_update_replaces_span);
    RUN_TEST(ime_cancel_restores_buffer_and_journal);
    RUN_TEST(ime_cursor_codepoints_snapped_and_clamped);
    RUN_TEST(ime_clause_highlight_range);
    RUN_TEST(ime_commit_lands_as_one_undo_step);
    RUN_TEST(ime_commit_and_next_clause_in_one_frame);
    RUN_TEST(ime_adopt_on_blur_matches_a_commit);
    RUN_TEST(ime_selection_deleted_at_start_stands_alone);
    RUN_TEST(ime_capacity_truncates_on_a_cluster_boundary);
    RUN_TEST(ime_read_only_never_composes);
    RUN_TEST(ime_password_masks_the_span);
    RUN_TEST(ime_forget_on_focus_and_on_external_change);
    RUN_TEST(ime_keys_and_mouse_are_off_while_composing);
    RUN_TEST(ime_area_pushed_on_edges_and_change);
    RUN_TEST(ime_anchor_holds_across_idle_frames);
    RUN_TEST(ime_anchor_holds_with_caret_scrolled_out);
    RUN_TEST(ime_anchor_active_on_empty_focus_frame);
    RUN_TEST(ime_focus_move_to_a_later_field_ends_the_composition);
    RUN_TEST(ime_focus_move_to_an_earlier_field_ends_the_composition);
    RUN_TEST(ime_editor_wrapped_span_reflows);
    RUN_TEST(ime_editor_linear_span_and_index_patch);
    RUN_TEST(ime_held_string_keeps_its_span);
    RUN_TEST(ime_span_edge_joins_neighbour);
    RUN_TEST(ime_unfocused_frames_never_write_the_buffer);
    RUN_TEST(ime_read_only_never_writes);
    RUN_TEST(ime_end_frame_commit_plus_enter_keeps_the_commit);
    RUN_TEST(ime_end_frame_escape_cancels_without_a_commit);
    RUN_TEST(ime_end_frame_commit_plus_undo_keeps_the_commit);
    RUN_TEST(ime_next_frame_after_the_end_frame_takes_keys);
    RUN_TEST(ime_tab_walk_waits_for_the_composition);
    RUN_TEST(ime_press_elsewhere_and_tab_adopt_the_span);
    RUN_TEST(ime_press_and_tab_on_the_composing_field_land_one_copy);
    RUN_TEST(ime_press_and_tab_on_the_composing_field_leave_no_orphan_span);
    RUN_TEST(ime_commit_that_fills_the_buffer_leaves_a_boundary_caret);
    RUN_TEST(ime_press_and_tab_as_a_composition_starts);
    RUN_TEST(ime_span_held_by_an_undrawn_field_is_adopted_when_drawn);
    RUN_TEST(word_steps_never_split_a_unit);
    RUN_TEST(word_double_click_never_splits_a_unit);
}
