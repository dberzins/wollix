// test_ime.c - composition input on the text widgets. Opens with the
// identity pins: with no composition string staged, the inputbox, the
// textarea and the editor (linear and wrapped) type, move, select, delete,
// undo and blur exactly as they did before composition existed - buffer,
// caret pair, journal counts and the recorded draw calls of a settled
// frame. The pins are recorded from the pre-composition code and are never
// re-recorded.
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

static void ime_reset_draws(void) {
    memset(ime_draws, 0, sizeof(ime_draws));
    ime_draw_count = 0;
}

static void ime_ctx_init(WLX_Context *ctx) {
    test_ctx_init(ctx, 400, 100);
    ctx->backend.draw_text_slice = ime_capture_text_slice;
    ctx->backend.draw_text = ime_capture_text;
    ctx->backend.draw_line = ime_capture_line;
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

static void ime_place_widget(WLX_Context *ctx, char *buf, size_t cap, size_t *len) {
    ime_last_focused = false;
    switch (ime_widget) {
        case IME_W_INPUTBOX:
        case IME_W_TEXTAREA:
            ime_last_changed = wlx_inputbox_impl(ctx, NULL, buf, cap,
                wlx_default_inputbox_opt(.height = 40, .content_padding = 4, .font_size = 10,
                    .border_width = 0, .multiline = ime_widget == IME_W_TEXTAREA,
                    .out_focused = &ime_last_focused),
                __FILE__, __LINE__);
            *len = strlen(buf);
            break;
        case IME_W_EDITOR:
        case IME_W_EDITOR_WRAP:
            ime_last_changed = wlx_editor_impl(ctx, NULL, buf, cap, len,
                wlx_default_editor_opt(.height = 40, .content_padding = 4, .font_size = 10,
                    .border_width = 0, .wrap = ime_widget == IME_W_EDITOR_WRAP,
                    .out_focused = &ime_last_focused),
                __FILE__, __LINE__);
            break;
    }
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
// Suite
// ============================================================================

SUITE(ime) {
    RUN_TEST(ime_identity_inputbox);
    RUN_TEST(ime_identity_textarea);
    RUN_TEST(ime_identity_editor_linear);
    RUN_TEST(ime_identity_editor_wrapped);
}
