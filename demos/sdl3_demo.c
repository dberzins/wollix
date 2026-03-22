#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_sdl3.h"

#define FONT_PATH "demos/assets/LiberationSans-Regular.ttf"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 640

int main(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Wollix SDL3 Demo", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!TTF_Init()) {
        fprintf(stderr, "TTF_Init failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    TTF_Font *font_title = TTF_OpenFont(FONT_PATH, 30);
    TTF_Font *font_body  = TTF_OpenFont(FONT_PATH, 20);
    TTF_Font *font_small = TTF_OpenFont(FONT_PATH, 18);
    if (!font_title || !font_body || !font_small) {
        fprintf(stderr, "TTF_OpenFont failed: %s\n", SDL_GetError());
    }

    WLX_Font wf_title = wlx_font_from_sdl3(font_title);
    WLX_Font wf_body  = wlx_font_from_sdl3(font_body);
    WLX_Font wf_small = wlx_font_from_sdl3(font_small);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_sdl3(ctx, window, renderer);

    float slider_value = 0.42f;
    bool check_enabled = true;
    int click_count = 0;
    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        int render_w = 0;
        int render_h = 0;
        SDL_GetRenderOutputSize(renderer, &render_w, &render_h);

        WLX_Rect root = {
            .x = 0,
            .y = 0,
            .w = (float)render_w,
            .h = (float)render_h,
        };

        wlx_begin(ctx, root, wlx_process_sdl3_input);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, WLX_BACKGROUND_COLOR.r, WLX_BACKGROUND_COLOR.g, WLX_BACKGROUND_COLOR.b, WLX_BACKGROUND_COLOR.a);
        SDL_RenderClear(renderer);

        wlx_layout_begin(ctx, 10, WLX_VERT);

        wlx_textbox(ctx, "SDL3 Backend Demo", .height = 52, .font_size = 30, .font = wf_title, .align = WLX_CENTER, .back_color = WLX_RGBA(28, 32, 45, 255));

        char click_label[64];
        snprintf(click_label, sizeof(click_label), "Clicks: %d", click_count);
        wlx_textbox(ctx, click_label, .height = 40, .font_size = 18, .font = wf_small, .align = WLX_CENTER, .back_color = WLX_RGBA(20, 24, 34, 255));

        if (wlx_button(ctx, "Click me", .height = 44, .font_size = 20, .font = wf_body, .align = WLX_CENTER, .back_color = WLX_RGBA(55, 70, 95, 255))) {
            click_count++;
        }

        wlx_checkbox(ctx, "Enable feature", &check_enabled, .height = 40, .font_size = 20, .font = wf_body, .align = WLX_LEFT);

        wlx_slider(ctx, "Value", &slider_value,
               .height = 44,
               .font_size = 18,
               .font = wf_small,
               .min_value = 0.0f,
               .max_value = 1.0f,
               .track_color = WLX_RGBA(60, 60, 70, 255),
               .thumb_color = WLX_RGBA(120, 180, 255, 255));

        char status[128];
        snprintf(status, sizeof(status), "State: %s  |  value=%.2f", check_enabled ? "ON" : "OFF", slider_value);
        wlx_textbox(ctx, status, .height = 40, .font_size = 18, .font = wf_small, .align = WLX_LEFT, .back_color = WLX_RGBA(20, 24, 34, 255));

        wlx_layout_end(ctx);

        wlx_end(ctx);
        SDL_RenderPresent(renderer);
    }

    wlx_context_destroy(ctx);
    free(ctx);
    TTF_CloseFont(font_small);
    TTF_CloseFont(font_body);
    TTF_CloseFont(font_title);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
