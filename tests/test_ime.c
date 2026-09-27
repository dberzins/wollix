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
// frames.
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

// A composition string lands in the buffer at the caret as ordinary bytes:
// drawn, underlined, the caret inside it; the journal records nothing and
// its length guard follows the true length.
TEST(ime_tentative_span_shows_and_journal_untouched) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
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
    ASSERT_TRUE(c->composing);
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
    IME_EXPECT_PIECE(ime_piece_index(0), "abni", 9.0f, 45.0f);
    int ul = ime_find_underline(10.0f);
    ASSERT_TRUE(ul >= 0);
    ASSERT_EQ_F(ime_draws[ul].x, 9.0f + 10.0f, 0.01f);
    ASSERT_TRUE(ime_draws[ul].y > 45.0f && ime_draws[ul].y <= 55.0f);
    wlx_context_destroy(&ctx);
}

// Each update replaces the whole span in place: one span, at the same
// start, the buffer holding only the latest string.
TEST(ime_update_replaces_span) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
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
    // An unchanged string re-applies to the same bytes.
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ASSERT_EQ_STR(buf, "abnihon");
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));
    wlx_context_destroy(&ctx);
}

// An empty string ends the composition: the buffer and the journal are
// exactly what they were before it started, and undo still reaches the
// typed text.
TEST(ime_cancel_restores_buffer_and_journal) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
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
    ASSERT_FALSE(c->composing);
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

// The composition caret follows the input method's codepoint cursor,
// snapped forward when it lands inside a grapheme cluster, at the end for
// -1 or past the end.
TEST(ime_cursor_codepoints_snapped_and_clamped) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
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

// The selected clause draws in the selection colour from the caret for
// the given codepoints; the widget's own selection stays empty.
TEST(ime_clause_highlight_range) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ctx.backend.draw_rect = ime_capture_rect;
    ime_widget = IME_W_INPUTBOX;
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

// ============================================================================
// Commit, adopt, selection, capacity
// ============================================================================

// A commit lands where the span began as one journal step of its own; undo
// removes the whole clause and redo restores it.
TEST(ime_commit_lands_as_one_undo_step) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ime_compose(&ctx, buf, sizeof(buf), &len, "\xE6\x97\xA5\xE6\x9C\xAC", "", -1, -1);
    ASSERT_EQ_STR(buf, "ab\xE6\x97\xA5\xE6\x9C\xAC");
    ASSERT_EQ_INT(8, (int)len);
    ASSERT_FALSE(ime_caret(&ctx)->composing);
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

// A commit and the next clause's first string arrive in one frame: the
// commit lands, the new span follows it.
TEST(ime_commit_and_next_clause_in_one_frame) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
    char buf[64] = "";
    size_t len = 0;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "nihon", -1, -1);
    ime_compose(&ctx, buf, sizeof(buf), &len, "X", "h", -1, -1);
    ASSERT_EQ_STR(buf, "abXh");
    WLX_Text_Edit_State *c = ime_caret(&ctx);
    ASSERT_TRUE(c->composing);
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

// Focus leaving with a span in place adopts it: the bytes stay and the
// journal holds the step a commit would have made.
TEST(ime_adopt_on_blur_matches_a_commit) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
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
    ASSERT_FALSE(c->composing);
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

// Composing over a selection deletes it first as a step of its own; a
// cancelled composition leaves that step, and undo restores the selection.
TEST(ime_selection_deleted_at_start_stands_alone) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
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
    ASSERT_TRUE(c->composing);
    ASSERT_EQ_INT(3, (int)c->preedit_len);
    ASSERT_EQ_INT(5, (int)c->cursor_pos);
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "", -1, -1);
    ASSERT_EQ_STR(buf, "ab");
    wlx_context_destroy(&ctx);
}

// A read-only field never takes a composition string and never asks for
// text input.
TEST(ime_read_only_never_composes) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
    ime_read_only = true;
    char buf[64] = "ab";
    size_t len = 2;

    ime_focus(&ctx, buf, sizeof(buf), &len);
    ASSERT_TRUE(ime_last_focused);
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "ni", -1, -1);
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_FALSE(ime_caret(&ctx)->composing);
    ASSERT_FALSE(wlx_text_composing(&ctx));
    ASSERT_EQ_INT(0, mock_tia_calls());
    ASSERT_FALSE(wlx_text_input_area(&ctx).active);
    wlx_context_destroy(&ctx);
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
    ASSERT_TRUE(ime_caret(&ctx)->composing);
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
// the editor's revision bump forgets it; the bytes stay in every case.
TEST(ime_forget_on_focus_and_on_external_change) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
    char buf[64] = "";
    size_t len = 0;

    // Not drawn for a frame while composing: focus is released without
    // the widget running (no adopt), then a click refocuses.
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ime_type(&ctx, buf, sizeof(buf), &len, "ab");
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "ni", -1, -1);
    ASSERT_TRUE(ime_caret(&ctx)->composing);
    ime_skip_widget = true;
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ime_skip_widget = false;
    ime_focus(&ctx, buf, sizeof(buf), &len);
    ASSERT_TRUE(ime_last_focused);
    ASSERT_EQ_STR(buf, "abni");
    ASSERT_FALSE(ime_caret(&ctx)->composing);
    ASSERT_EQ_INT(0, (int)ime_caret(&ctx)->preedit_len);
    ASSERT_EQ_INT(1, (int)ime_undo_count(&ctx));

    // The application rewrites the buffer while a span is in place.
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "go", -1, -1);
    ASSERT_EQ_STR(buf, "abnigo");
    strcpy(buf, "zz");
    len = 2;
    ime_compose(&ctx, buf, sizeof(buf), &len, NULL, "", -1, -1);
    ASSERT_EQ_STR(buf, "zz");
    ASSERT_FALSE(ime_caret(&ctx)->composing);
    ASSERT_EQ_INT(0, (int)ime_caret(&ctx)->preedit_len);
    wlx_context_destroy(&ctx);

    // The editor: an outside rewrite of the same length with a revision
    // bump (the one case the length guard cannot see) forgets the span;
    // the bytes the application wrote stay.
    ime_ctx_init(&ctx);
    ime_widget = IME_W_EDITOR;
    char doc[64] = "ab\ncd";
    size_t dlen = 5;
    ime_idle(&ctx, doc, sizeof(doc), &dlen);
    ime_focus(&ctx, doc, sizeof(doc), &dlen);
    ime_key(&ctx, doc, sizeof(doc), &dlen, WLX_KEY_END, test_command_mod());
    ime_compose(&ctx, doc, sizeof(doc), &dlen, NULL, "ni", -1, -1);
    ASSERT_EQ_STR(doc, "ab\ncdni");
    ASSERT_TRUE(ime_caret(&ctx)->composing);
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
    ASSERT_TRUE(ime_caret(&ctx)->composing);
    ASSERT_EQ_INT(2, (int)ime_caret(&ctx)->preedit_len);
    wlx_context_destroy(&ctx);
}

// While a span is in the buffer, keys and clicks change nothing: the input
// method owns them.
TEST(ime_keys_and_mouse_are_off_while_composing) {
    WLX_Context ctx;
    ime_ctx_init(&ctx);
    ime_widget = IME_W_INPUTBOX;
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
    // A click at the field's start would place the caret at 0 otherwise.
    ime_compose_key(&ctx, buf, sizeof(buf), &len, "ni", 10, 50, true, WLX_KEY_NONE, 0);
    ASSERT_EQ_INT(4, (int)ime_caret(&ctx)->cursor_pos);
    ASSERT_TRUE(ime_last_focused);
    ASSERT_TRUE(ime_caret(&ctx)->composing);
    wlx_context_destroy(&ctx);
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
    ASSERT_EQ_F(mock_tia_last().cursor, cursor0 + 10.0f + WLX_TEXT_CARET_PADDING, 0.01f);
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ime_idle(&ctx, buf, sizeof(buf), &len);
    ASSERT_EQ_INT(2, mock_tia_calls());                     // unchanged frames push nothing
    ime_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ASSERT_EQ_INT(3, mock_tia_calls());
    ASSERT_EQ_F(mock_tia_last().cursor, cursor0 + 5.0f + WLX_TEXT_CARET_PADDING, 0.01f);
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

// ============================================================================
// Suite
// ============================================================================

SUITE(ime) {
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
    RUN_TEST(ime_editor_wrapped_span_reflows);
    RUN_TEST(ime_editor_linear_span_and_index_patch);
}
