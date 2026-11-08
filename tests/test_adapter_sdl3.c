// test_adapter_sdl3.c - the SDL3 adapter driven headless, on SDL's
// offscreen video driver and software renderer (set through SDL hints, so
// the binary needs no environment): what it draws, read back from the
// render target, how it keeps its session and its composition state, and
// its SDL_ttf text path. Its own binary (`make test-sdl3`), since SDL3 is an
// optional dependency. Built with SDL_ttf, as every SDL3 target is: the
// default font still takes the adapter's debug-font path, and the SDL_ttf
// section opens demos/assets/DejaVuSans.ttf (run from the repository root).

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include <SDL3_ttf/SDL_ttf.h>
#include "wollix_sdl3.h"
#include "tests.h"

#include <stdlib.h>
#include <string.h>

// ============================================================================
// Fixture
// ============================================================================

static float sa_scale;        // > 0 forces the adapter's device scale for the frame
static int   sa_click = -1;   // >= 0 puts the mouse at (sa_mouse_x, 30), pressed when 1
static int   sa_mouse_x = 100;

static void sa_input(WLX_Context *ctx) {
    wlx_process_sdl3_input(ctx);
    if (sa_scale > 0.0f) g_wlx_sdl3.scale = sa_scale;
    if (sa_click >= 0) {
        ctx->input.mouse_x = sa_mouse_x;
        ctx->input.mouse_y = 30;
        ctx->input.mouse_down = ctx->input.mouse_clicked = ctx->input.mouse_held = sa_click != 0;
    }
}

// The hints go before every SDL_Init: SDL_Quit resets them.
static bool sa_init(void) {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    return SDL_Init(SDL_INIT_VIDEO);
}

static SDL_Window *sa_open(int w, int h) {
    SDL_Window *win = SDL_CreateWindow("wollix adapter test", w, h, 0);
    if (win != NULL && SDL_CreateRenderer(win, NULL) == NULL) {
        SDL_DestroyWindow(win);
        return NULL;
    }
    return win;
}

static void sa_close(SDL_Window *win) {
    SDL_DestroyRenderer(SDL_GetRenderer(win));
    SDL_DestroyWindow(win);
}

static void sa_clear(SDL_Window *win) {
    SDL_Renderer *r = SDL_GetRenderer(win);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);
}

// Device pixels of a pure red run along one row, from x0, of the render
// target.
static int sa_red_run(SDL_Window *win, int x0, int y, int max) {
    SDL_Rect row = { x0, y, max, 1 };
    SDL_Surface *s = SDL_RenderReadPixels(SDL_GetRenderer(win), &row);
    if (s == NULL) return -1;
    SDL_Surface *c = SDL_ConvertSurface(s, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(s);
    if (c == NULL) return -1;
    const Uint8 *p = (const Uint8 *)c->pixels;
    int n = 0;
    while (n < max && p[n * 4] > 200 && p[n * 4 + 1] < 60 && p[n * 4 + 2] < 60) n++;
    SDL_DestroySurface(c);
    return n;
}

// Red pixels anywhere on one row of the render target.
static int sa_red_count(SDL_Window *win, int y, int width) {
    SDL_Rect row = { 0, y, width, 1 };
    SDL_Surface *s = SDL_RenderReadPixels(SDL_GetRenderer(win), &row);
    if (s == NULL) return -1;
    SDL_Surface *c = SDL_ConvertSurface(s, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(s);
    if (c == NULL) return -1;
    const Uint8 *p = (const Uint8 *)c->pixels;
    int n = 0;
    for (int i = 0; i < width; i++)
        if (p[i * 4] > 200 && p[i * 4 + 1] < 60 && p[i * 4 + 2] < 60) n++;
    SDL_DestroySurface(c);
    return n;
}

// A region of the render target as RGBA32 bytes, into out (w * h * 4).
static bool sa_read(SDL_Window *win, SDL_Rect r, Uint8 *out) {
    SDL_Surface *s = SDL_RenderReadPixels(SDL_GetRenderer(win), &r);
    if (s == NULL) return false;
    SDL_Surface *c = SDL_ConvertSurface(s, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(s);
    if (c == NULL) return false;
    for (int y = 0; y < r.h; y++)
        memcpy(out + (size_t)y * (size_t)r.w * 4, (const Uint8 *)c->pixels + (size_t)y * (size_t)c->pitch,
               (size_t)r.w * 4);
    SDL_DestroySurface(c);
    return true;
}

// The left border, in device pixels, of a label filling the window's top
// 60 units with a 4-unit red border at `roundness`, drawn at `scale`.
static int sa_border_px(SDL_Window *win, WLX_Context *ctx, float roundness, float scale) {
    sa_scale = scale;
    sa_clear(win);
    wlx_begin(ctx, wlx_sdl3_root_rect(), sa_input);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    wlx_label(ctx, "", .height = 60, .border_width = 4, .border_color = { 255, 0, 0, 255 },
              .back_color = { 0, 0, 1, 255 }, .roundness = roundness);
    wlx_layout_end(ctx);
    wlx_end(ctx);
    int px = sa_red_run(win, 0, (int)(30.0f * scale), 40);
    sa_scale = 0.0f;
    return px;
}

// ============================================================================
// Drawing
// ============================================================================

// A sharp border keeps the width a rounded one has: 4 units are 4 device
// pixels at scale 1 and 8 at scale 2, whatever the roundness.
TEST(sdl3_outline_honours_thickness) {
    SDL_Window *win = sa_open(200, 100);
    ASSERT_TRUE(win != NULL);
    WLX_Context *ctx = calloc(1, sizeof *ctx);
    wlx_context_init_sdl3(ctx, win, SDL_GetRenderer(win));
    ASSERT_EQ_INT(4, sa_border_px(win, ctx, 0.0f, 1.0f));
    ASSERT_EQ_INT(8, sa_border_px(win, ctx, 0.0f, 2.0f));
    ASSERT_EQ_INT(4, sa_border_px(win, ctx, 0.3f, 1.0f));
    ASSERT_EQ_INT(8, sa_border_px(win, ctx, 0.3f, 2.0f));
    wlx_context_destroy(ctx);
    free(ctx);
    sa_close(win);
}

// A thick diagonal line (a check mark's stroke) is as thick as asked: a
// 6-unit 45-degree line crosses a row over about 6 * sqrt(2) pixels.
TEST(sdl3_diagonal_line_honours_thickness) {
    SDL_Window *win = sa_open(200, 100);
    ASSERT_TRUE(win != NULL);
    WLX_Context *ctx = calloc(1, sizeof *ctx);
    wlx_context_init_sdl3(ctx, win, SDL_GetRenderer(win));
    sa_clear(win);
    ctx->backend.draw_line(20.0f, 10.0f, 80.0f, 70.0f, 6.0f, (WLX_Color){ 255, 0, 0, 255 },
        ctx->backend.user);
    int across = sa_red_count(win, 40, 200);
    ASSERT_TRUE(across >= 7 && across <= 10);
    // A thin diagonal stays one pixel.
    sa_clear(win);
    ctx->backend.draw_line(20.0f, 10.0f, 80.0f, 70.0f, 1.0f, (WLX_Color){ 255, 0, 0, 255 },
        ctx->backend.user);
    ASSERT_TRUE(sa_red_count(win, 40, 200) <= 2);
    wlx_context_destroy(ctx);
    free(ctx);
    sa_close(win);
}

// The legacy C-string text callbacks agree with the slice pair: the same
// measure, and the same pixels, also against a slice cut out of a longer
// string. (No SDL_ttf here: the debug-font path.)
TEST(sdl3_legacy_text_callbacks_match_the_slice_pair) {
    SDL_Window *win = sa_open(200, 100);
    ASSERT_TRUE(win != NULL);
    WLX_Context *ctx = calloc(1, sizeof *ctx);
    wlx_context_init_sdl3(ctx, win, SDL_GetRenderer(win));
    WLX_Backend *b = &ctx->backend;
    WLX_Text_Style style = { .font = WLX_FONT_DEFAULT, .font_size = 16,
                             .color = { 255, 255, 255, 255 } };
    float lw = 0, lh = 0, sw = 0, sh = 0;
    b->measure_text("Hello", style, &lw, &lh, b->user);
    b->measure_text_slice("Hello, world", 5, style, &sw, &sh, b->user);
    ASSERT_EQ_F(lw, sw, 0.0f);
    ASSERT_EQ_F(lh, sh, 0.0f);
    ASSERT_TRUE(lw > 0.0f);
    b->measure_text("", style, &lw, &lh, b->user);
    b->measure_text_slice("x", 0, style, &sw, &sh, b->user);
    ASSERT_EQ_F(lw, sw, 0.0f);
    ASSERT_EQ_F(lh, sh, 0.0f);

    enum { W = 120, H = 24 };
    static Uint8 legacy[W * H * 4], slice[W * H * 4];
    SDL_Rect r = { 0, 0, W, H };
    sa_clear(win);
    b->draw_text("Hello", 4.0f, 4.0f, style, b->user);
    ASSERT_TRUE(sa_read(win, r, legacy));
    sa_clear(win);
    b->draw_text_slice("Hello, world", 5, 4.0f, 4.0f, style, b->user);
    ASSERT_TRUE(sa_read(win, r, slice));
    ASSERT_TRUE(memcmp(legacy, slice, sizeof legacy) == 0);
    int lit = 0;
    for (int i = 0; i < W * H; i++) lit += legacy[i * 4] > 128;
    ASSERT_TRUE(lit > 0);
    wlx_context_destroy(ctx);
    free(ctx);
    sa_close(win);
}

// ============================================================================
// Session
// ============================================================================

// One frame of a window-high inputbox under the mouse.
static void sa_field_frame(WLX_Context *ctx, char *buf, size_t cap) {
    wlx_begin(ctx, wlx_sdl3_root_rect(), sa_input);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    wlx_inputbox(ctx, NULL, buf, cap, .height = 60);
    wlx_layout_end(ctx);
    wlx_end(ctx);
}

// A context re-bound to a replacement window keeps its focused field's
// text input: the adapter forgets the old window, and the core hands the
// unchanged anchor to the new one.
TEST(sdl3_rebound_context_restarts_text_input) {
    SDL_Window *a = sa_open(200, 100);
    ASSERT_TRUE(a != NULL);
    WLX_Context *ctx = calloc(1, sizeof *ctx);
    wlx_context_init_sdl3(ctx, a, SDL_GetRenderer(a));
    char buf[64] = "";
    sa_click = 0; sa_field_frame(ctx, buf, sizeof buf);
    sa_click = 1; sa_field_frame(ctx, buf, sizeof buf);
    sa_click = 0; sa_field_frame(ctx, buf, sizeof buf);
    sa_field_frame(ctx, buf, sizeof buf);
    ASSERT_TRUE(SDL_TextInputActive(a));
    sa_close(a);

    SDL_Window *b = sa_open(200, 100);
    ASSERT_TRUE(b != NULL);
    wlx_context_init_sdl3(ctx, b, SDL_GetRenderer(b));
    for (int i = 0; i < 3; i++) sa_field_frame(ctx, buf, sizeof buf);
    ASSERT_TRUE(SDL_TextInputActive(b));
    sa_click = -1;
    wlx_context_destroy(ctx);
    free(ctx);
    sa_close(b);
}

// The event watch is SDL's, and SDL_Quit drops it: a context initialised
// after a re-init still hears the events the watch collects (the wheel).
TEST(sdl3_event_watch_survives_sdl_reinit) {
    SDL_Quit();
    ASSERT_TRUE(sa_init());
    SDL_Window *win = sa_open(200, 100);
    ASSERT_TRUE(win != NULL);
    WLX_Context *ctx = calloc(1, sizeof *ctx);
    wlx_context_init_sdl3(ctx, win, SDL_GetRenderer(win));
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_EVENT_MOUSE_WHEEL;
    e.wheel.windowID = SDL_GetWindowID(win);
    e.wheel.y = 1.0f;
    ASSERT_TRUE(SDL_PushEvent(&e));
    wlx_process_sdl3_input(ctx);
    ASSERT_EQ_F(ctx->input.wheel_delta, 1.0f, 1e-6f);
    wlx_context_destroy(ctx);
    free(ctx);
    sa_close(win);
}

// ============================================================================
// Committed text
// ============================================================================

static void sa_push_text(SDL_Window *win, const char *text) {
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_EVENT_TEXT_INPUT;
    e.text.windowID = SDL_GetWindowID(win);
    e.text.text = text;
    SDL_PushEvent(&e);
}

// The commit-stream rule: the first codepoint that does not fit ends the
// frame's burst. The overflowing event keeps its prefix that fits, and a
// later short event of the same frame, which would fit, is dropped too;
// the next frame starts empty.
TEST(sdl3_text_burst_ends_at_the_first_overflow) {
    SDL_Window *win = sa_open(200, 100);
    ASSERT_TRUE(win != NULL);
    WLX_Context *ctx = calloc(1, sizeof *ctx);
    wlx_context_init_sdl3(ctx, win, SDL_GetRenderer(win));
    // WLX_INPUT_TEXT_BYTES - 3 ASCII bytes leave room for 2 more.
    static char fill[WLX_INPUT_TEXT_BYTES];
    memset(fill, 'a', WLX_INPUT_TEXT_BYTES - 3);
    fill[WLX_INPUT_TEXT_BYTES - 3] = '\0';
    sa_push_text(win, fill);
    sa_push_text(win, "b\xE2\x82\xAC");  // "b" fits, the euro sign (3 bytes) does not
    sa_push_text(win, "c");                // fits, but the burst is over
    wlx_process_sdl3_input(ctx);
    size_t n = strlen(ctx->input.text_input);
    ASSERT_EQ_INT(WLX_INPUT_TEXT_BYTES - 2, (int)n);
    ASSERT_TRUE(ctx->input.text_input[n - 1] == 'b');
    sa_push_text(win, "d");
    wlx_process_sdl3_input(ctx);
    ASSERT_EQ_STR("d", ctx->input.text_input);
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    wlx_context_destroy(ctx);
    free(ctx);
    sa_close(win);
}

// ============================================================================
// Composition state
// ============================================================================

static void sa_push_editing(SDL_Window *win, const char *text, int start, int length) {
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_EVENT_TEXT_EDITING;
    e.edit.windowID = SDL_GetWindowID(win);
    e.edit.text = text;
    e.edit.start = start;
    e.edit.length = length;
    SDL_PushEvent(&e);
}

static void sa_push_window(SDL_Window *win, SDL_EventType type) {
    SDL_Event e;
    SDL_zero(e);
    e.type = type;
    e.window.windowID = SDL_GetWindowID(win);
    SDL_PushEvent(&e);
}

// Focus a window-high inputbox and run frames until text input is on.
static void sa_focus_field(WLX_Context *ctx, char *buf, size_t cap) {
    sa_click = 0; sa_field_frame(ctx, buf, cap);
    sa_click = 1; sa_field_frame(ctx, buf, cap);
    sa_click = 0; sa_field_frame(ctx, buf, cap);
    sa_field_frame(ctx, buf, cap);
}

// The composition string is state: an editing event sets the string, its
// cursor and its selection, and they persist across frames until the input
// method replaces them; an empty editing event (the input method's commit)
// and a window focus loss clear them. While text input is off an editing
// event is ignored.
TEST(sdl3_preedit_state_machine) {
    SDL_Window *win = sa_open(200, 100);
    ASSERT_TRUE(win != NULL);
    WLX_Context *ctx = calloc(1, sizeof *ctx);
    wlx_context_init_sdl3(ctx, win, SDL_GetRenderer(win));
    char buf[64] = "";

    // Text input off: ignored.
    sa_push_editing(win, "\xE3\x81\x8B", 1, 0);
    wlx_process_sdl3_input(ctx);
    ASSERT_EQ_STR("", ctx->input.preedit);

    sa_focus_field(ctx, buf, sizeof buf);
    ASSERT_TRUE(SDL_TextInputActive(win));
    sa_push_editing(win, "\xE3\x81\x8B\xE3\x81\xAA", 1, 1);
    wlx_process_sdl3_input(ctx);
    ASSERT_EQ_STR("\xE3\x81\x8B\xE3\x81\xAA", ctx->input.preedit);
    ASSERT_EQ_INT(1, ctx->input.preedit_cursor);
    ASSERT_EQ_INT(1, ctx->input.preedit_sel_len);
    wlx_process_sdl3_input(ctx);                       // held: no event, same state
    ASSERT_EQ_STR("\xE3\x81\x8B\xE3\x81\xAA", ctx->input.preedit);
    ASSERT_EQ_INT(1, ctx->input.preedit_cursor);

    // The commit: an empty editing event, then the committed text.
    sa_push_editing(win, "", 0, 0);
    sa_push_text(win, "\xE4\xBB\xAE");
    wlx_process_sdl3_input(ctx);
    ASSERT_EQ_STR("", ctx->input.preedit);
    ASSERT_EQ_STR("\xE4\xBB\xAE", ctx->input.text_input);

    // A focus loss clears a held string.
    sa_push_editing(win, "k", 1, 0);
    wlx_process_sdl3_input(ctx);
    ASSERT_EQ_STR("k", ctx->input.preedit);
    sa_push_window(win, SDL_EVENT_WINDOW_FOCUS_LOST);
    wlx_process_sdl3_input(ctx);
    ASSERT_EQ_STR("", ctx->input.preedit);
    ASSERT_EQ_INT(-1, ctx->input.preedit_cursor);
    ASSERT_EQ_INT(-1, ctx->input.preedit_sel_len);

    sa_click = -1;
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    wlx_context_destroy(ctx);
    free(ctx);
    sa_close(win);
}

// Two fields side by side: the left half and the right half of the window.
static void sa_two_fields_frame(WLX_Context *ctx, char *a, char *b, size_t cap) {
    wlx_begin(ctx, wlx_sdl3_root_rect(), sa_input);
    wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 0, .gap = 0);
    wlx_inputbox(ctx, NULL, a, cap, .height = 60);
    wlx_inputbox(ctx, NULL, b, cap, .height = 60);
    wlx_layout_end(ctx);
    wlx_end(ctx);
}

// A field-to-field move ends the composition: the core pushes the move as
// one inactive frame, the adapter stops text input and clears the string
// with the session, and the next frame starts text input for the new field.
TEST(sdl3_field_move_clears_preedit) {
    SDL_Window *win = sa_open(200, 100);
    ASSERT_TRUE(win != NULL);
    WLX_Context *ctx = calloc(1, sizeof *ctx);
    wlx_context_init_sdl3(ctx, win, SDL_GetRenderer(win));
    char a[64] = "", b[64] = "";
    sa_mouse_x = 50;                                   // the left field
    sa_click = 0; sa_two_fields_frame(ctx, a, b, sizeof a);
    sa_click = 1; sa_two_fields_frame(ctx, a, b, sizeof a);
    sa_click = 0; sa_two_fields_frame(ctx, a, b, sizeof a);
    sa_two_fields_frame(ctx, a, b, sizeof a);
    ASSERT_TRUE(SDL_TextInputActive(win));
    sa_push_editing(win, "\xE3\x81\x8B", 1, 0);
    sa_two_fields_frame(ctx, a, b, sizeof a);
    ASSERT_EQ_STR("\xE3\x81\x8B", ctx->input.preedit);
    ASSERT_TRUE(wlx_text_composing(ctx));

    sa_mouse_x = 150;                                  // press the right field
    sa_click = 1; sa_two_fields_frame(ctx, a, b, sizeof a);
    ASSERT_FALSE(SDL_TextInputActive(win));            // the inactive push
    ASSERT_EQ_STR("", g_wlx_sdl3.preedit);
    sa_click = 0; sa_two_fields_frame(ctx, a, b, sizeof a);
    ASSERT_EQ_STR("", ctx->input.preedit);
    ASSERT_FALSE(wlx_text_composing(ctx));
    ASSERT_TRUE(SDL_TextInputActive(win));             // restarted for the right field

    sa_click = -1;
    sa_mouse_x = 100;
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    wlx_context_destroy(ctx);
    free(ctx);
    sa_close(win);
}

// The composition string keeps whole codepoints within its 127 bytes: an
// over-long editing event is cut at the last codepoint that fits.
TEST(sdl3_preedit_keeps_whole_codepoints) {
    SDL_Window *win = sa_open(200, 100);
    ASSERT_TRUE(win != NULL);
    WLX_Context *ctx = calloc(1, sizeof *ctx);
    wlx_context_init_sdl3(ctx, win, SDL_GetRenderer(win));
    char buf[64] = "";
    sa_focus_field(ctx, buf, sizeof buf);
    ASSERT_TRUE(SDL_TextInputActive(win));
    // 50 three-byte codepoints (150 bytes): 42 fit (126 bytes), the 43rd
    // would end at 129.
    static char longer[151];
    for (int i = 0; i < 50; i++) memcpy(longer + i * 3, "\xE3\x81\x8B", 3);
    longer[150] = '\0';
    sa_push_editing(win, longer, 0, 0);
    wlx_process_sdl3_input(ctx);
    ASSERT_EQ_INT(WLX_INPUT_PREEDIT_BYTES - 1, 127);
    ASSERT_EQ_INT(126, (int)strlen(ctx->input.preedit));
    ASSERT_TRUE(memcmp(ctx->input.preedit, longer, 126) == 0);
    // A string of four-byte codepoints: 31 fit (124 bytes).
    static char emoji[161];
    for (int i = 0; i < 40; i++) memcpy(emoji + i * 4, "\xF0\x9F\x98\x80", 4);
    emoji[160] = '\0';
    sa_push_editing(win, emoji, 0, 0);
    wlx_process_sdl3_input(ctx);
    ASSERT_EQ_INT(124, (int)strlen(ctx->input.preedit));

    sa_click = -1;
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    wlx_context_destroy(ctx);
    free(ctx);
    sa_close(win);
}

// ============================================================================
// Scissor edges at a fractional scale
// ============================================================================

// The red span of row y, as [first, end) device pixels (first = -1: none).
static void sa_red_span(SDL_Window *win, int y, int width, int *first, int *end) {
    SDL_Rect row = { 0, y, width, 1 };
    *first = -1;
    *end = -1;
    SDL_Surface *s = SDL_RenderReadPixels(SDL_GetRenderer(win), &row);
    if (s == NULL) return;
    SDL_Surface *c = SDL_ConvertSurface(s, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(s);
    if (c == NULL) return;
    const Uint8 *p = (const Uint8 *)c->pixels;
    for (int i = 0; i < width; i++) {
        bool red = p[i * 4] > 200 && p[i * 4 + 1] < 60 && p[i * 4 + 2] < 60;
        if (red && *first < 0) *first = i;
        if (red) *end = i + 1;
    }
    SDL_DestroySurface(c);
}

// At scale 1.5 a scissor's unit edges land between device pixels; each
// edge rounds to the nearest pixel boundary (x.5 rounds up), read back
// from the render target. Two clips that share a unit edge share the
// pixel edge: no gap, no overlap.
TEST(sdl3_scissor_edges_at_scale_one_and_a_half) {
    SDL_Window *win = sa_open(200, 100);
    ASSERT_TRUE(win != NULL);
    WLX_Context *ctx = calloc(1, sizeof *ctx);
    wlx_context_init_sdl3(ctx, win, SDL_GetRenderer(win));
    WLX_Backend *b = &ctx->backend;
    WLX_Color red = { 255, 0, 0, 255 };
    WLX_Rect all = { 0, 0, 200, 100 };
    g_wlx_sdl3.scale = 1.5f;
    int first, end;

    // Units [3, 14) are device [4.5, 21): pixels [5, 21).
    sa_clear(win);
    b->begin_scissor((WLX_Rect){ 3.0f, 3.0f, 11.0f, 11.0f }, b->user);
    b->draw_rect(all, red, b->user);
    b->end_scissor(b->user);
    sa_red_span(win, 10, 200, &first, &end);
    ASSERT_EQ_INT(5, first);
    ASSERT_EQ_INT(21, end);
    ASSERT_EQ_INT(0, sa_red_count(win, 4, 200));      // the top edge rounds the same way
    ASSERT_TRUE(sa_red_count(win, 5, 200) > 0);
    ASSERT_TRUE(sa_red_count(win, 20, 200) > 0);
    ASSERT_EQ_INT(0, sa_red_count(win, 21, 200));

    // Units [1, 7) are device [1.5, 10.5): pixels [2, 11).
    sa_clear(win);
    b->begin_scissor((WLX_Rect){ 1.0f, 1.0f, 6.0f, 6.0f }, b->user);
    b->draw_rect(all, red, b->user);
    b->end_scissor(b->user);
    sa_red_span(win, 5, 200, &first, &end);
    ASSERT_EQ_INT(2, first);
    ASSERT_EQ_INT(11, end);

    // Abutting clips [0, 5) and [5, 10) units: [0, 8) and [8, 15) pixels,
    // each filled once, so a translucent fill shows no doubled column.
    sa_clear(win);
    WLX_Color half = { 255, 0, 0, 128 };
    b->begin_scissor((WLX_Rect){ 0.0f, 0.0f, 5.0f, 10.0f }, b->user);
    b->draw_rect(all, half, b->user);
    b->begin_scissor((WLX_Rect){ 5.0f, 0.0f, 5.0f, 10.0f }, b->user);
    b->draw_rect(all, half, b->user);
    b->end_scissor(b->user);
    static Uint8 px[16 * 4];
    ASSERT_TRUE(sa_read(win, (SDL_Rect){ 0, 3, 16, 1 }, px));
    for (int i = 0; i < 15; i++) ASSERT_EQ_INT(px[0], px[i * 4]);   // one level across both
    ASSERT_TRUE(px[0] > 100 && px[0] < 160);
    ASSERT_EQ_INT(0, px[15 * 4]);

    g_wlx_sdl3.scale = 1.0f;
    wlx_context_destroy(ctx);
    free(ctx);
    sa_close(win);
}

// ============================================================================
// SDL_ttf text
// ============================================================================

#define SA_FONT_PATH "demos/assets/DejaVuSans.ttf"

// The unit ends of a string, as the core derives them.
static size_t sa_unit_ends(const char *s, size_t *ends, size_t cap) {
    size_t len = strlen(s), n = 0, pos = 0;
    while (pos < len && n < cap) {
        pos = wlx_text_unit_next(s, len, pos);
        ends[n++] = pos;
    }
    return n;
}

// The advances callback is registered exactly where it can fill: with
// SDL_ttf it needs the font-variant machinery (SDL_ttf 3.3.0 and later).
TEST(sdl3_ttf_advances_registered_with_font_variants) {
    SDL_Window *win = sa_open(200, 100);
    ASSERT_TRUE(win != NULL);
    WLX_Context *ctx = calloc(1, sizeof *ctx);
    wlx_context_init_sdl3(ctx, win, SDL_GetRenderer(win));
    ASSERT_EQ_INT(WLX_SDL3_HAS_FONT_VARIANTS ? 1 : 0, ctx->backend.measure_text_advances != NULL);
    printf("    (SDL_ttf %d.%d.%d, font variants %s)\n", SDL_TTF_MAJOR_VERSION, SDL_TTF_MINOR_VERSION,
           SDL_TTF_MICRO_VERSION, WLX_SDL3_HAS_FONT_VARIANTS ? "on" : "off");
    wlx_context_destroy(ctx);
    free(ctx);
    sa_close(win);
}

// Where it is registered, the fill over a TrueType font is non-decreasing
// and ends where the slice measure ends, over ASCII and over a
// combining-mark cluster.
TEST(sdl3_ttf_advances_end_at_the_measure) {
    if (!WLX_SDL3_HAS_FONT_VARIANTS) return;
    ASSERT_TRUE(TTF_Init());
    TTF_Font *font = TTF_OpenFont(SA_FONT_PATH, 16.0f);
    ASSERT_TRUE(font != NULL);
    SDL_Window *win = sa_open(200, 100);
    ASSERT_TRUE(win != NULL);
    WLX_Context *ctx = calloc(1, sizeof *ctx);
    wlx_context_init_sdl3(ctx, win, SDL_GetRenderer(win));
    WLX_Backend *b = &ctx->backend;
    ASSERT_TRUE(b->measure_text_advances != NULL);
    WLX_Text_Style style = { .font = wlx_font_from_sdl3(font), .font_size = 16,
                             .color = { 255, 255, 255, 255 } };
    static const char *const samples[] = { "Hello, world", "cafe\xCC\x81 ole\xCC\x81" };
    for (size_t k = 0; k < 2; k++) {
        const char *s = samples[k];
        size_t ends[32];
        float adv[32];
        size_t n = sa_unit_ends(s, ends, 32);
        size_t filled = b->measure_text_advances(s, strlen(s), style, ends, n, adv, b->user);
        ASSERT_EQ_INT((int)n, (int)filled);
        for (size_t i = 1; i < n; i++) ASSERT_TRUE(adv[i] >= adv[i - 1]);
        ASSERT_TRUE(adv[0] > 0.0f);
        float w = 0, h = 0;
        b->measure_text_slice(s, strlen(s), style, &w, &h, b->user);
        ASSERT_EQ_F(adv[n - 1], w, 0.001f);
    }
    wlx_sdl3_text_cache_clear();
    wlx_context_destroy(ctx);
    free(ctx);
    sa_close(win);
    TTF_CloseFont(font);
    TTF_Quit();
}

// The content scale keys the font size: at scale 2 the text is set at
// twice the pixel size, and its measure in units matches scale 1 up to the
// hinting error, half a unit per cluster (each size rounds every advance to
// its own pixel grid: ten "i" are 40 units at 16 px and 45 at 32 px). A
// variant not sized by the scale, or a measure left in device pixels,
// halves or doubles the width.
TEST(sdl3_ttf_measure_holds_across_scale) {
    ASSERT_TRUE(TTF_Init());
    TTF_Font *font = TTF_OpenFont(SA_FONT_PATH, 16.0f);
    ASSERT_TRUE(font != NULL);
    SDL_Window *win = sa_open(200, 100);
    ASSERT_TRUE(win != NULL);
    WLX_Context *ctx = calloc(1, sizeof *ctx);
    wlx_context_init_sdl3(ctx, win, SDL_GetRenderer(win));
    WLX_Backend *b = &ctx->backend;
    WLX_Text_Style style = { .font = wlx_font_from_sdl3(font), .font_size = 16,
                             .color = { 255, 255, 255, 255 } };
    static const char *const samples[] = { "Hello", "Wollix 0.9", "cafe\xCC\x81", "iiiiiiiiii" };
    for (size_t k = 0; k < 4; k++) {
        const char *s = samples[k];
        float w1 = 0, h1 = 0, w2 = 0, h2 = 0;
        g_wlx_sdl3.scale = 1.0f;
        b->measure_text_slice(s, strlen(s), style, &w1, &h1, b->user);
        g_wlx_sdl3.scale = 2.0f;
        b->measure_text_slice(s, strlen(s), style, &w2, &h2, b->user);
        g_wlx_sdl3.scale = 1.0f;
        size_t ends[16];
        size_t clusters = sa_unit_ends(s, ends, 16);
        ASSERT_TRUE(w1 > 0.0f && h1 > 0.0f);
        ASSERT_EQ_F(w1, w2, 0.5f * (float)clusters + 1.0f);
        ASSERT_EQ_F(h1, h2, 1.0f);
    }
    wlx_sdl3_text_cache_clear();
    wlx_context_destroy(ctx);
    free(ctx);
    sa_close(win);
    TTF_CloseFont(font);
    TTF_Quit();
}

// ============================================================================
// Suite
// ============================================================================

SUITE(adapter_sdl3) {
    RUN_TEST(sdl3_outline_honours_thickness);
    RUN_TEST(sdl3_diagonal_line_honours_thickness);
    RUN_TEST(sdl3_legacy_text_callbacks_match_the_slice_pair);
    RUN_TEST(sdl3_rebound_context_restarts_text_input);
    RUN_TEST(sdl3_event_watch_survives_sdl_reinit);
    RUN_TEST(sdl3_text_burst_ends_at_the_first_overflow);
    RUN_TEST(sdl3_preedit_state_machine);
    RUN_TEST(sdl3_field_move_clears_preedit);
    RUN_TEST(sdl3_preedit_keeps_whole_codepoints);
    RUN_TEST(sdl3_scissor_edges_at_scale_one_and_a_half);
    RUN_TEST(sdl3_ttf_advances_registered_with_font_variants);
    RUN_TEST(sdl3_ttf_advances_end_at_the_measure);
    RUN_TEST(sdl3_ttf_measure_holds_across_scale);
}

int main(void) {
    if (!sa_init()) {
        fprintf(stderr, "test_adapter_sdl3: SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    RUN_SUITE(adapter_sdl3);
    SDL_Quit();
    return test_summary();
}
