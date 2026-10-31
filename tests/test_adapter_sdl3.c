// test_adapter_sdl3.c - the SDL3 adapter driven headless, on SDL's
// offscreen video driver and software renderer (set through SDL hints, so
// the binary needs no environment): what it draws, read back from the
// render target, and how it keeps its session. Its own binary (`make
// test-sdl3`), since SDL3 is an optional dependency; no SDL_ttf, so text
// takes the adapter's debug-font path.

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_sdl3.h"
#include "tests.h"

#include <stdlib.h>
#include <string.h>

// ============================================================================
// Fixture
// ============================================================================

static float sa_scale;      // > 0 forces the adapter's device scale for the frame
static int   sa_click = -1; // >= 0 puts the mouse at (100, 30), pressed when 1

static void sa_input(WLX_Context *ctx) {
    wlx_process_sdl3_input(ctx);
    if (sa_scale > 0.0f) g_wlx_sdl3.scale = sa_scale;
    if (sa_click >= 0) {
        ctx->input.mouse_x = 100;
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
// Suite
// ============================================================================

SUITE(adapter_sdl3) {
    RUN_TEST(sdl3_outline_honours_thickness);
    RUN_TEST(sdl3_diagonal_line_honours_thickness);
    RUN_TEST(sdl3_legacy_text_callbacks_match_the_slice_pair);
    RUN_TEST(sdl3_rebound_context_restarts_text_input);
    RUN_TEST(sdl3_event_watch_survives_sdl_reinit);
    RUN_TEST(sdl3_text_burst_ends_at_the_first_overflow);
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
