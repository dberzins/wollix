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

static float sa_scale;   // > 0 forces the adapter's device scale for the frame

static void sa_input(WLX_Context *ctx) {
    wlx_process_sdl3_input(ctx);
    if (sa_scale > 0.0f) g_wlx_sdl3_scale = sa_scale;
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

// ============================================================================
// Suite
// ============================================================================

SUITE(adapter_sdl3) {
    RUN_TEST(sdl3_outline_honours_thickness);
    RUN_TEST(sdl3_diagonal_line_honours_thickness);
}

int main(void) {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "test_adapter_sdl3: SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    RUN_SUITE(adapter_sdl3);
    SDL_Quit();
    return test_summary();
}
