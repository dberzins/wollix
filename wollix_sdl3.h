#ifndef WOLLIX_SDL3_H_
#define WOLLIX_SDL3_H_

#ifndef WOLLIX_H_
#error "Include wollix.h before wollix_sdl3.h"
#endif

#include <SDL3/SDL.h>

#include <math.h>
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

#define WLX_SDL3_MAX_CORNER_SEGMENTS 32
#define WLX_SDL3_PI 3.14159265358979323846f

static inline SDL_FColor wlx_sdl3_to_fcolor(WLX_Color c) {
    return (SDL_FColor){
        c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f
    };
}

static inline int wlx_sdl3_clamp_segments(int segments) {
    if (segments < 16) return 16;
    if (segments > WLX_SDL3_MAX_CORNER_SEGMENTS) return WLX_SDL3_MAX_CORNER_SEGMENTS;
    return segments;
}

// Tessellation helpers - geometry building blocks shared by shape-drawing functions.

static inline float wlx_sdl3_corner_radius(WLX_Rect rect, float roundness) {
    float min_dim = (rect.w < rect.h) ? rect.w : rect.h;
    float r = roundness * min_dim * 0.5f;
    if (r > min_dim * 0.5f) r = min_dim * 0.5f;
    return r;
}

static inline SDL_FColor wlx_sdl3_setup_blend_color(WLX_Color color) {
    SDL_SetRenderDrawBlendMode(wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
    return wlx_sdl3_to_fcolor(color);
}

typedef struct {
    float cx, cy, start, end;
} WLX_SDL3_Corner;

static inline void wlx_sdl3_build_rect_corners(WLX_Rect rect, float r, WLX_SDL3_Corner corners[4]) {
    float w = rect.w, h = rect.h;
    corners[0] = (WLX_SDL3_Corner){ rect.x + w - r, rect.y + r,     -WLX_SDL3_PI * 0.5f, 0.0f };
    corners[1] = (WLX_SDL3_Corner){ rect.x + w - r, rect.y + h - r,  0.0f,                WLX_SDL3_PI * 0.5f };
    corners[2] = (WLX_SDL3_Corner){ rect.x + r,     rect.y + h - r,  WLX_SDL3_PI * 0.5f,  WLX_SDL3_PI };
    corners[3] = (WLX_SDL3_Corner){ rect.x + r,     rect.y + r,      WLX_SDL3_PI,          WLX_SDL3_PI * 1.5f };
}

static inline void wlx_sdl3_emit_perimeter_verts(SDL_Vertex *verts, int *vi,
    const WLX_SDL3_Corner corners[4], int segments, float r, SDL_FColor fc)
{
    for (int c = 0; c < 4; c++) {
        for (int i = 0; i <= segments; i++) {
            float t = (float)i / (float)segments;
            float angle = corners[c].start + t * (corners[c].end - corners[c].start);
            verts[(*vi)++] = (SDL_Vertex){
                {corners[c].cx + cosf(angle) * r, corners[c].cy + sinf(angle) * r}, fc, {0, 0}
            };
        }
    }
}

static inline void wlx_sdl3_emit_dual_perimeter_verts(SDL_Vertex *verts, int *vi,
    const WLX_SDL3_Corner corners[4], int segments, float r_outer, float r_inner, SDL_FColor fc)
{
    for (int c = 0; c < 4; c++) {
        for (int i = 0; i <= segments; i++) {
            float t = (float)i / (float)segments;
            float angle = corners[c].start + t * (corners[c].end - corners[c].start);
            float cs = cosf(angle), sn = sinf(angle);
            verts[(*vi)++] = (SDL_Vertex){
                {corners[c].cx + cs * r_outer, corners[c].cy + sn * r_outer}, fc, {0, 0}
            };
            verts[(*vi)++] = (SDL_Vertex){
                {corners[c].cx + cs * r_inner, corners[c].cy + sn * r_inner}, fc, {0, 0}
            };
        }
    }
}

static inline void wlx_sdl3_emit_fan_indices(int *indices, int perimeter_count) {
    for (int i = 0; i < perimeter_count; i++) {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = 1 + i;
        indices[i * 3 + 2] = 1 + (i + 1) % perimeter_count;
    }
}

static inline void wlx_sdl3_emit_strip_indices(int *indices, int segment_count) {
    for (int i = 0; i < segment_count; i++) {
        int next = (i + 1) % segment_count;
        int o0 = i * 2, i0 = i * 2 + 1;
        int o1 = next * 2, i1 = next * 2 + 1;
        indices[i * 6 + 0] = o0;
        indices[i * 6 + 1] = o1;
        indices[i * 6 + 2] = i0;
        indices[i * 6 + 3] = i0;
        indices[i * 6 + 4] = o1;
        indices[i * 6 + 5] = i1;
    }
}

static inline void wlx_sdl3_emit_circle_fan_verts(
    SDL_Vertex *verts, float cx, float cy, float radius, int segments, SDL_FColor fc)
{
    verts[0] = (SDL_Vertex){ {cx, cy}, fc, {0, 0} };
    for (int i = 0; i < segments; i++) {
        float angle = (float)i / (float)segments * 2.0f * WLX_SDL3_PI;
        verts[i + 1] = (SDL_Vertex){
            {cx + cosf(angle) * radius, cy + sinf(angle) * radius}, fc, {0, 0}
        };
    }
}

static inline void wlx_sdl3_emit_ring_strip_verts(
    SDL_Vertex *verts, float cx, float cy, float outer_r, float inner_r, int segments, SDL_FColor fc)
{
    for (int i = 0; i < segments; i++) {
        float angle = (float)i / (float)segments * 2.0f * WLX_SDL3_PI;
        float cs = cosf(angle), sn = sinf(angle);
        verts[i * 2]     = (SDL_Vertex){ {cx + cs * outer_r, cy + sn * outer_r}, fc, {0, 0} };
        verts[i * 2 + 1] = (SDL_Vertex){ {cx + cs * inner_r, cy + sn * inner_r}, fc, {0, 0} };
    }
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

static inline void wlx_sdl3_draw_rect_rounded(
    WLX_Rect rect, float roundness, int segments, WLX_Color color) 
{
    assert(wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

    float r = wlx_sdl3_corner_radius(rect, roundness);
    if (r < 0.5f) {
        wlx_sdl3_draw_rect(rect, color);
        return;
    }

    segments = wlx_sdl3_clamp_segments(segments);
    SDL_FColor fc = wlx_sdl3_setup_blend_color(color);

    WLX_SDL3_Corner corners[4];
    wlx_sdl3_build_rect_corners(rect, r, corners);

    int perimeter_count = 4 * (segments + 1);
    int nv = perimeter_count + 1;
    SDL_Vertex verts[4 * (WLX_SDL3_MAX_CORNER_SEGMENTS + 1) + 1];
    float cx = rect.x + rect.w * 0.5f, cy = rect.y + rect.h * 0.5f;
    verts[0] = (SDL_Vertex){ {cx, cy}, fc, {0, 0} };
    int vi = 1;
    wlx_sdl3_emit_perimeter_verts(verts, &vi, corners, segments, r, fc);

    int indices[4 * (WLX_SDL3_MAX_CORNER_SEGMENTS + 1) * 3];
    wlx_sdl3_emit_fan_indices(indices, perimeter_count);

    SDL_RenderGeometry(wlx_sdl3_renderer, NULL, verts, nv, indices, perimeter_count * 3);
}

static inline void wlx_sdl3_draw_rect_rounded_lines(
    WLX_Rect rect, float roundness, int segments, float thick, WLX_Color color) 
{
    assert(wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

    float r = wlx_sdl3_corner_radius(rect, roundness);
    if (r < 0.5f) {
        wlx_sdl3_draw_rect_lines(rect, thick, color);
        return;
    }

    segments = wlx_sdl3_clamp_segments(segments);
    SDL_FColor fc = wlx_sdl3_setup_blend_color(color);

    float r_inner = r - thick;
    if (r_inner < 0.0f) r_inner = 0.0f;

    WLX_SDL3_Corner corners[4];
    wlx_sdl3_build_rect_corners(rect, r, corners);

    int perimeter_count = 4 * (segments + 1);
    int nv = perimeter_count * 2;
    SDL_Vertex verts[2 * 4 * (WLX_SDL3_MAX_CORNER_SEGMENTS + 1)];
    int vi = 0;
    wlx_sdl3_emit_dual_perimeter_verts(verts, &vi, corners, segments, r, r_inner, fc);

    int indices[4 * (WLX_SDL3_MAX_CORNER_SEGMENTS + 1) * 6];
    wlx_sdl3_emit_strip_indices(indices, perimeter_count);

    SDL_RenderGeometry(wlx_sdl3_renderer, NULL, verts, nv, indices, perimeter_count * 6);
}

static inline void wlx_sdl3_draw_circle(float cx, float cy, float radius, int segments, WLX_Color color) {
    assert(wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");
    
    segments = wlx_sdl3_clamp_segments(segments);
    SDL_FColor fc = wlx_sdl3_setup_blend_color(color);

    SDL_Vertex verts[WLX_SDL3_MAX_CORNER_SEGMENTS + 1];
    wlx_sdl3_emit_circle_fan_verts(verts, cx, cy, radius, segments, fc);

    int indices[WLX_SDL3_MAX_CORNER_SEGMENTS * 3];
    wlx_sdl3_emit_fan_indices(indices, segments);

    SDL_RenderGeometry(wlx_sdl3_renderer, NULL, verts, segments + 1, indices, segments * 3);
}

static inline void wlx_sdl3_draw_ring(float cx, float cy, float inner_r, float outer_r,
        int segments, WLX_Color color) {

    assert(wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");
    
    segments = wlx_sdl3_clamp_segments(segments);
    SDL_FColor fc = wlx_sdl3_setup_blend_color(color);

    SDL_Vertex verts[WLX_SDL3_MAX_CORNER_SEGMENTS * 2];
    wlx_sdl3_emit_ring_strip_verts(verts, cx, cy, outer_r, inner_r, segments, fc);

    int indices[WLX_SDL3_MAX_CORNER_SEGMENTS * 6];
    wlx_sdl3_emit_strip_indices(indices, segments);

    SDL_RenderGeometry(wlx_sdl3_renderer, NULL, verts, segments * 2, indices, segments * 6);
}

static inline void wlx_sdl3_draw_line(float x1, float y1, float x2, float y2, float thick, WLX_Color color) {
    WLX_UNUSED(thick);

    assert(wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

    SDL_SetRenderDrawBlendMode(wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(wlx_sdl3_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderLine(wlx_sdl3_renderer, x1, y1, x2, y2);
}

static inline void wlx_sdl3_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    assert(wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");
    if (text == NULL || text[0] == '\0') return;

#ifdef SDL_TTF_VERSION
    if (style.font != WLX_FONT_DEFAULT) {
        TTF_Font *font = (TTF_Font *)(uintptr_t)style.font;
        SDL_Color fg = wlx_sdl3_to_color(style.color);
        SDL_Surface *surf = TTF_RenderText_Blended(font, text, 0, fg);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(wlx_sdl3_renderer, surf);
            if (tex) {
                SDL_FRect dst = { x, y, (float)surf->w, (float)surf->h };
                SDL_RenderTexture(wlx_sdl3_renderer, tex, NULL, &dst);
                SDL_DestroyTexture(tex);
            }
            SDL_DestroySurface(surf);
        }
        return;
    }
#endif

    // Fallback: debug font
    SDL_Color sc = wlx_sdl3_to_color(style.color);
    SDL_SetRenderDrawBlendMode(wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(wlx_sdl3_renderer, sc.r, sc.g, sc.b, sc.a);
    SDL_RenderDebugText(wlx_sdl3_renderer, x, y, text);
}

static inline void wlx_sdl3_measure_text(const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
    if (out_w == NULL || out_h == NULL) return;
    if (text == NULL) text = "";

#ifdef SDL_TTF_VERSION
    if (style.font != WLX_FONT_DEFAULT) {
        TTF_Font *font = (TTF_Font *)(uintptr_t)style.font;
        int w = 0, h = 0;
        TTF_GetStringSize(font, text, 0, &w, &h);
        *out_w = (float)w;
        *out_h = (float)h;
        return;
    }
#endif

    // Fallback: debug font estimation
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
        .draw_rect_rounded_lines = wlx_sdl3_draw_rect_rounded_lines,
        .draw_circle = wlx_sdl3_draw_circle,
        .draw_ring = wlx_sdl3_draw_ring,
        .draw_line = wlx_sdl3_draw_line,
        .draw_text = wlx_sdl3_draw_text,
        .measure_text = wlx_sdl3_measure_text,
        .draw_texture = wlx_sdl3_draw_texture,
        .begin_scissor = wlx_sdl3_begin_scissor,
        .end_scissor = wlx_sdl3_end_scissor,
        .get_frame_time = wlx_sdl3_get_frame_time,
    };
}

#ifdef SDL_TTF_VERSION
static inline WLX_Font wlx_font_from_sdl3(TTF_Font *font) {
    return (WLX_Font)(uintptr_t)font;
}
#endif

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
