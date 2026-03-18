#ifndef WOLLIX_SDL3_H_
#define WOLLIX_SDL3_H_

#ifndef WOLLIX_H_
#error "Include wollix.h before wollix_sdl3.h"
#endif

#include <SDL3/SDL.h>

#include <stddef.h>
#include <stdint.h>

static SDL_Renderer *wlx_sdl3_renderer = NULL;
static int wlx_sdl3_wheel_delta = 0;
static char wlx_sdl3_text_input[32] = {0};
static size_t wlx_sdl3_text_len = 0;
static bool wlx_sdl3_event_watch_installed = false;
static Uint64 wlx_sdl3_last_counter = 0;

static inline SDL_Color wlx_sdl3_to_color(WLX_Color c) {
    return (SDL_Color){ c.r, c.g, c.b, c.a };
}

static inline SDL_FRect wlx_sdl3_to_frect(WLX_Rect r) {
    return (SDL_FRect){ r.x, r.y, r.w, r.h };
}

static inline WLX_Texture wlx_texture_from_sdl3(SDL_Texture *texture, int width, int height) {
    WLX_Texture out = {0};
    out.handle = (uintptr_t)texture;
    out.width = width;
    out.height = height;
    return out;
}

static inline SDL_Scancode wlx_sdl3_to_scancode(WLX_Key_Code key) {
    switch (key) {
        case WLX_KEY_ESCAPE: return SDL_SCANCODE_ESCAPE;
        case WLX_KEY_ENTER: return SDL_SCANCODE_RETURN;
        case WLX_KEY_BACKSPACE: return SDL_SCANCODE_BACKSPACE;
        case WLX_KEY_TAB: return SDL_SCANCODE_TAB;
        case WLX_KEY_SPACE: return SDL_SCANCODE_SPACE;
        case WLX_KEY_LEFT: return SDL_SCANCODE_LEFT;
        case WLX_KEY_RIGHT: return SDL_SCANCODE_RIGHT;
        case WLX_KEY_UP: return SDL_SCANCODE_UP;
        case WLX_KEY_DOWN: return SDL_SCANCODE_DOWN;
        case WLX_KEY_A: return SDL_SCANCODE_A;
        case WLX_KEY_B: return SDL_SCANCODE_B;
        case WLX_KEY_C: return SDL_SCANCODE_C;
        case WLX_KEY_D: return SDL_SCANCODE_D;
        case WLX_KEY_E: return SDL_SCANCODE_E;
        case WLX_KEY_F: return SDL_SCANCODE_F;
        case WLX_KEY_G: return SDL_SCANCODE_G;
        case WLX_KEY_H: return SDL_SCANCODE_H;
        case WLX_KEY_I: return SDL_SCANCODE_I;
        case WLX_KEY_J: return SDL_SCANCODE_J;
        case WLX_KEY_K: return SDL_SCANCODE_K;
        case WLX_KEY_L: return SDL_SCANCODE_L;
        case WLX_KEY_M: return SDL_SCANCODE_M;
        case WLX_KEY_N: return SDL_SCANCODE_N;
        case WLX_KEY_O: return SDL_SCANCODE_O;
        case WLX_KEY_P: return SDL_SCANCODE_P;
        case WLX_KEY_Q: return SDL_SCANCODE_Q;
        case WLX_KEY_R: return SDL_SCANCODE_R;
        case WLX_KEY_S: return SDL_SCANCODE_S;
        case WLX_KEY_T: return SDL_SCANCODE_T;
        case WLX_KEY_U: return SDL_SCANCODE_U;
        case WLX_KEY_V: return SDL_SCANCODE_V;
        case WLX_KEY_W: return SDL_SCANCODE_W;
        case WLX_KEY_X: return SDL_SCANCODE_X;
        case WLX_KEY_Y: return SDL_SCANCODE_Y;
        case WLX_KEY_Z: return SDL_SCANCODE_Z;
        case WLX_KEY_0: return SDL_SCANCODE_0;
        case WLX_KEY_1: return SDL_SCANCODE_1;
        case WLX_KEY_2: return SDL_SCANCODE_2;
        case WLX_KEY_3: return SDL_SCANCODE_3;
        case WLX_KEY_4: return SDL_SCANCODE_4;
        case WLX_KEY_5: return SDL_SCANCODE_5;
        case WLX_KEY_6: return SDL_SCANCODE_6;
        case WLX_KEY_7: return SDL_SCANCODE_7;
        case WLX_KEY_8: return SDL_SCANCODE_8;
        case WLX_KEY_9: return SDL_SCANCODE_9;
        default: return SDL_SCANCODE_UNKNOWN;
    }
}

static bool wlx_sdl3_event_watch(void *userdata, SDL_Event *event) {
    WLX_UNUSED(userdata);

    if (event == NULL) return true;

    switch (event->type) {
        case SDL_EVENT_MOUSE_WHEEL: {
            if (event->wheel.y > 0) wlx_sdl3_wheel_delta += 1;
            else if (event->wheel.y < 0) wlx_sdl3_wheel_delta -= 1;
            break;
        }
        case SDL_EVENT_TEXT_INPUT: {
            const char *src = event->text.text;
            while (*src != '\0' && wlx_sdl3_text_len < sizeof(wlx_sdl3_text_input) - 1) {
                wlx_sdl3_text_input[wlx_sdl3_text_len++] = *src++;
            }
            wlx_sdl3_text_input[wlx_sdl3_text_len] = '\0';
            break;
        }
        default:
            break;
    }

    return true;
}

static inline void wlx_process_sdl3_input(WLX_Context *ctx) {
    assert(ctx != NULL);

    static bool prev_mouse_down = false;

    SDL_PumpEvents();

    float mx = 0.0f;
    float my = 0.0f;
    SDL_MouseButtonFlags mouse = SDL_GetMouseState(&mx, &my);
    ctx->input.mouse_x = (int)mx;
    ctx->input.mouse_y = (int)my;
    ctx->input.mouse_down = (mouse & SDL_BUTTON_LMASK) != 0;
    ctx->input.mouse_clicked = ctx->input.mouse_down && !prev_mouse_down;
    ctx->input.mouse_held = ctx->input.mouse_down;
    prev_mouse_down = ctx->input.mouse_down;

    const bool *state = SDL_GetKeyboardState(NULL);
    for (int k = 0; k < WLX_KEY_COUNT; k++) {
        bool was_down = ctx->input.keys_down[k];
        bool is_down = false;

        SDL_Scancode scancode = wlx_sdl3_to_scancode((WLX_Key_Code)k);
        if (scancode != SDL_SCANCODE_UNKNOWN) {
            is_down = state[scancode];
        }

        ctx->input.keys_down[k] = is_down;
        ctx->input.keys_pressed[k] = is_down && !was_down;
    }

    ctx->input.wheel_delta = wlx_sdl3_wheel_delta;
    wlx_sdl3_wheel_delta = 0;

    wlx_zero_struct(ctx->input.text_input);
    if (wlx_sdl3_text_len > 0) {
        size_t copy_len = wlx_sdl3_text_len;
        if (copy_len > sizeof(ctx->input.text_input) - 1) {
            copy_len = sizeof(ctx->input.text_input) - 1;
        }
        memcpy(ctx->input.text_input, wlx_sdl3_text_input, copy_len);
    }
    wlx_zero_struct(wlx_sdl3_text_input);
    wlx_sdl3_text_len = 0;
}

static inline void wlx_sdl3_draw_texture(WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    assert(wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

    SDL_Texture *tex = (SDL_Texture *)(uintptr_t)texture.handle;
    assert(tex != NULL && "WLX_Texture.handle must contain SDL_Texture*");

    SDL_SetTextureColorMod(tex, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(tex, tint.a);

    SDL_FRect s = wlx_sdl3_to_frect(src);
    SDL_FRect d = wlx_sdl3_to_frect(dst);
    SDL_RenderTexture(wlx_sdl3_renderer, tex, &s, &d);
}

static inline void wlx_sdl3_draw_rect(WLX_Rect rect, WLX_Color color) {
    assert(wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

    SDL_SetRenderDrawBlendMode(wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(wlx_sdl3_renderer, color.r, color.g, color.b, color.a);

    SDL_FRect r = wlx_sdl3_to_frect(rect);
    SDL_RenderFillRect(wlx_sdl3_renderer, &r);
}

static inline void wlx_sdl3_draw_rect_lines(WLX_Rect rect, float thick, WLX_Color color) {
    WLX_UNUSED(thick);

    assert(wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

    SDL_SetRenderDrawBlendMode(wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(wlx_sdl3_renderer, color.r, color.g, color.b, color.a);

    SDL_FRect r = wlx_sdl3_to_frect(rect);
    SDL_RenderRect(wlx_sdl3_renderer, &r);
}

static inline void wlx_sdl3_draw_rect_rounded(WLX_Rect rect, float roundness, int segments, WLX_Color color) {
    WLX_UNUSED(roundness);
    WLX_UNUSED(segments);
    wlx_sdl3_draw_rect(rect, color);
}

static inline void wlx_sdl3_draw_line(float x1, float y1, float x2, float y2, float thick, WLX_Color color) {
    WLX_UNUSED(thick);

    assert(wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

    SDL_SetRenderDrawBlendMode(wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(wlx_sdl3_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderLine(wlx_sdl3_renderer, x1, y1, x2, y2);
}

static inline void wlx_sdl3_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)style.font;

    assert(wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

    if (text == NULL) return;

    SDL_Color sc = wlx_sdl3_to_color(style.color);
    SDL_SetRenderDrawBlendMode(wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(wlx_sdl3_renderer, sc.r, sc.g, sc.b, sc.a);
    SDL_RenderDebugText(wlx_sdl3_renderer, x, y, text);
}

static inline void wlx_sdl3_measure_text(const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
    if (out_w == NULL || out_h == NULL) return;

    if (text == NULL) text = "";

    size_t len = wlx_utf8_strlen(text);
    float glyph_w = (style.font_size > 0) ? (style.font_size * 0.5f) : 8.0f;
    float glyph_h = (style.font_size > 0) ? (float)style.font_size : 8.0f;

    if (len == 0) {
        *out_w = glyph_w;
        *out_h = glyph_h;
        return;
    }

    *out_w = (float)len * glyph_w + (float)((len > 1) ? (len - 1) : 0) * (float)style.spacing;
    *out_h = glyph_h;
}

static inline void wlx_sdl3_begin_scissor(WLX_Rect rect) {
    assert(wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

    SDL_Rect r = {
        .x = (int)rect.x,
        .y = (int)rect.y,
        .w = (int)rect.w,
        .h = (int)rect.h,
    };
    SDL_SetRenderClipRect(wlx_sdl3_renderer, &r);
}

static inline void wlx_sdl3_end_scissor(void) {
    assert(wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");
    SDL_SetRenderClipRect(wlx_sdl3_renderer, NULL);
}

static inline float wlx_sdl3_get_frame_time(void) {
    Uint64 counter = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();

    if (wlx_sdl3_last_counter == 0 || frequency == 0) {
        wlx_sdl3_last_counter = counter;
        return 0.0f;
    }

    float frame_time = (float)(counter - wlx_sdl3_last_counter) / (float)frequency;
    wlx_sdl3_last_counter = counter;
    return frame_time;
}

static inline WLX_Backend wlx_backend_sdl3(SDL_Renderer *renderer) {
    wlx_sdl3_renderer = renderer;
    wlx_sdl3_last_counter = 0;

    return (WLX_Backend){
        .draw_rect = wlx_sdl3_draw_rect,
        .draw_rect_lines = wlx_sdl3_draw_rect_lines,
        .draw_rect_rounded = wlx_sdl3_draw_rect_rounded,
        .draw_line = wlx_sdl3_draw_line,
        .draw_text = wlx_sdl3_draw_text,
        .measure_text = wlx_sdl3_measure_text,
        .draw_texture = wlx_sdl3_draw_texture,
        .begin_scissor = wlx_sdl3_begin_scissor,
        .end_scissor = wlx_sdl3_end_scissor,
        .get_frame_time = wlx_sdl3_get_frame_time,
    };
}

static inline void wlx_context_init_sdl3(WLX_Context *ctx, SDL_Window *window, SDL_Renderer *renderer) {
    assert(ctx != NULL);
    assert(window != NULL);
    assert(renderer != NULL);

    if (!wlx_sdl3_event_watch_installed) {
        SDL_AddEventWatch(wlx_sdl3_event_watch, NULL);
        wlx_sdl3_event_watch_installed = true;
    }

    SDL_StartTextInput(window);
    ctx->backend = wlx_backend_sdl3(renderer);
}

#endif // WOLLIX_SDL3_H_
