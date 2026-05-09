#ifndef WOLLIX_SDL3_H_
#define WOLLIX_SDL3_H_

#if defined(__INTELLISENSE__) && !defined(WOLLIX_H_)
#include "wollix.h"
#endif

#ifndef WOLLIX_H_
#error "Include wollix.h before wollix_sdl3.h"
#endif

#include <SDL3/SDL.h>

#include <math.h>
#include <stddef.h>
#include <stdint.h>

static SDL_Renderer *g_wlx_sdl3_renderer = NULL;
static int g_wlx_sdl3_wheel_delta = 0;
static char g_wlx_sdl3_text_input[32] = {0};
static size_t g_wlx_sdl3_text_len = 0;
static bool g_wlx_sdl3_event_watch_installed = false;
static Uint64 g_wlx_sdl3_last_counter = 0;

// SDL_ttf can show a lower-row smear on tiny text when draw origins land on
// fractional pixels. Snap to integer coordinates to stabilize rasterization,
// and use floor so tight scissors do not clip descenders by shifting text down.
static inline float wlx_sdl3_snap_text_coord(float value) {
    return floorf(value);
}

#ifdef WLX_PERF
typedef struct {
    uint64_t frame_index;
    bool timer_available;
    uint64_t draw_text_calls;
    uint64_t measure_text_calls;
    uint64_t draw_rect_calls;
    uint64_t draw_rect_lines_calls;
    uint64_t draw_rect_rounded_calls;
    uint64_t draw_rect_rounded_lines_calls;
    uint64_t draw_circle_calls;
    uint64_t draw_ring_calls;
    uint64_t draw_line_calls;
    uint64_t draw_texture_calls;
    uint64_t begin_scissor_calls;
    uint64_t end_scissor_calls;
    uint64_t geometry_submit_calls;
    uint64_t clip_change_calls;
    uint64_t text_draw_ns;
    uint64_t text_measure_ns;
    uint64_t geometry_ns;
    uint64_t scissor_ns;
    uint64_t texture_ns;
    uint64_t present_ns;
    uint64_t ttf_get_string_size_calls;
    uint64_t ttf_render_text_blended_calls;
    uint64_t create_texture_from_surface_calls;
    uint64_t destroy_texture_calls;
    uint64_t destroy_surface_calls;
    uint64_t render_texture_calls;
    uint64_t render_geometry_calls;
    uint64_t render_rect_calls;
    uint64_t render_line_calls;
    uint64_t set_clip_rect_calls;
    uint64_t set_draw_blend_mode_calls;
    uint64_t set_font_size_calls;
    uint64_t text_engine_create_attempts;
    uint64_t text_engine_create_successes;
    uint64_t text_engine_draw_calls;
    uint64_t ttf_text_creates;
    uint64_t ttf_text_destroys;
    uint64_t text_engine_fallback_draw_calls;
    uint64_t font_variant_lookups;
    uint64_t font_variant_hits;
    uint64_t font_variant_misses;
    uint64_t font_variant_creates;
    uint64_t font_variant_evictions;
    uint64_t font_variant_generation_invalidations;
    uint64_t font_variant_fallback_resolutions;
    uint64_t ttf_set_font_char_spacing_calls;
    uint64_t text_cache_lookups;
    uint64_t text_cache_hits;
    uint64_t text_cache_misses;
    uint64_t text_cache_creates;
    uint64_t text_cache_destroys;
    uint64_t text_cache_evictions;
    uint64_t text_cache_collision_rejections;
    uint64_t text_cache_fallback_resolutions;
    uint64_t text_cache_heap_allocs;
    uint64_t text_cache_heap_frees;
    uint64_t text_cache_color_mutations;
    uint64_t text_cache_color_skips;
    uint64_t text_cache_variant_invalidation_evictions;
} WLX_Perf_SDL3_Frame;

typedef struct {
    WLX_Perf_SDL3_Frame current;
    WLX_Perf_SDL3_Frame last;
    uint64_t present_start_ns;
    bool capturing;
} WLX_Perf_SDL3_State;

static WLX_Perf_SDL3_State g_wlx_perf_sdl3_state = {0};

static inline bool wlx_perf_sdl3_timer_available(void) {
    return SDL_GetPerformanceFrequency() != 0;
}

static inline uint64_t wlx_perf_sdl3_timestamp(void *user) {
    Uint64 frequency;
    Uint64 counter;
    WLX_UNUSED(user);

    frequency = SDL_GetPerformanceFrequency();
    if (frequency == 0) return 0;
    counter = SDL_GetPerformanceCounter();
    return (uint64_t)(((long double)counter * 1000000000.0L) / (long double)frequency);
}

static inline void wlx_perf_sdl3_install_timer(WLX_Context *ctx) {
    wlx_perf_set_timer(ctx, wlx_perf_sdl3_timestamp, NULL);
}

static inline void wlx_perf_sdl3_begin_frame(uint64_t frame_index) {
    wlx_zero_struct(g_wlx_perf_sdl3_state.current);
    g_wlx_perf_sdl3_state.current.frame_index = frame_index;
    g_wlx_perf_sdl3_state.current.timer_available = wlx_perf_sdl3_timer_available();
    g_wlx_perf_sdl3_state.present_start_ns = 0;
    g_wlx_perf_sdl3_state.capturing = true;
}

static inline void wlx_perf_sdl3_end_frame(uint64_t frame_index) {
    if (!g_wlx_perf_sdl3_state.capturing) return;
    if (frame_index != 0) g_wlx_perf_sdl3_state.current.frame_index = frame_index;
    g_wlx_perf_sdl3_state.last = g_wlx_perf_sdl3_state.current;
    g_wlx_perf_sdl3_state.capturing = false;
    g_wlx_perf_sdl3_state.present_start_ns = 0;
}

static inline void wlx_perf_sdl3_reset(void) {
    wlx_zero_struct(g_wlx_perf_sdl3_state);
}

static inline const WLX_Perf_SDL3_Frame *wlx_perf_sdl3_get_last_frame(void) {
    return &g_wlx_perf_sdl3_state.last;
}

static inline void wlx_perf_sdl3_inc(uint64_t *counter) {
    if (!g_wlx_perf_sdl3_state.capturing) return;
    (*counter)++;
}

static inline uint64_t wlx_perf_sdl3_time_begin(void) {
    if (!g_wlx_perf_sdl3_state.capturing || !g_wlx_perf_sdl3_state.current.timer_available) return 0;
    return wlx_perf_sdl3_timestamp(NULL);
}

static inline void wlx_perf_sdl3_time_end(uint64_t start_ns, uint64_t *total_ns) {
    uint64_t end_ns;

    if (!g_wlx_perf_sdl3_state.capturing || !g_wlx_perf_sdl3_state.current.timer_available) return;
    end_ns = wlx_perf_sdl3_timestamp(NULL);
    if (end_ns >= start_ns) *total_ns += end_ns - start_ns;
}

static inline void wlx_perf_sdl3_present_begin(void) {
    g_wlx_perf_sdl3_state.present_start_ns = wlx_perf_sdl3_time_begin();
}

static inline void wlx_perf_sdl3_present_end(void) {
    wlx_perf_sdl3_time_end(g_wlx_perf_sdl3_state.present_start_ns,
        &g_wlx_perf_sdl3_state.current.present_ns);
    g_wlx_perf_sdl3_state.present_start_ns = 0;
}

#define WLX_SDL3_PERF_INC(field) \
    wlx_perf_sdl3_inc(&g_wlx_perf_sdl3_state.current.field)
#else
#define WLX_SDL3_PERF_INC(field) ((void)0)
#endif

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
            if (event->wheel.y > 0) g_wlx_sdl3_wheel_delta += 1;
            else if (event->wheel.y < 0) g_wlx_sdl3_wheel_delta -= 1;
            break;
        }
        case SDL_EVENT_TEXT_INPUT: {
            const char *src = event->text.text;
            while (*src != '\0' && g_wlx_sdl3_text_len < sizeof(g_wlx_sdl3_text_input) - 1) {
                g_wlx_sdl3_text_input[g_wlx_sdl3_text_len++] = *src++;
            }
            g_wlx_sdl3_text_input[g_wlx_sdl3_text_len] = '\0';
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

    ctx->input.wheel_delta = g_wlx_sdl3_wheel_delta;
    g_wlx_sdl3_wheel_delta = 0;

    wlx_zero_struct(ctx->input.text_input);
    if (g_wlx_sdl3_text_len > 0) {
        size_t copy_len = g_wlx_sdl3_text_len;
        if (copy_len > sizeof(ctx->input.text_input) - 1) {
            copy_len = sizeof(ctx->input.text_input) - 1;
        }
        memcpy(ctx->input.text_input, g_wlx_sdl3_text_input, copy_len);
    }
    wlx_zero_struct(g_wlx_sdl3_text_input);
    g_wlx_sdl3_text_len = 0;
}

static inline void wlx_sdl3_draw_texture(WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    assert(g_wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

    SDL_Texture *tex = (SDL_Texture *)(uintptr_t)texture.handle;
    assert(tex != NULL && "WLX_Texture.handle must contain SDL_Texture*");

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(draw_texture_calls);

    SDL_SetTextureColorMod(tex, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(tex, tint.a);

    SDL_FRect s = wlx_sdl3_to_frect(src);
    SDL_FRect d = wlx_sdl3_to_frect(dst);
    WLX_SDL3_PERF_INC(render_texture_calls);
    SDL_RenderTexture(g_wlx_sdl3_renderer, tex, &s, &d);
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.texture_ns);
#endif
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
    WLX_SDL3_PERF_INC(set_draw_blend_mode_calls);
    SDL_SetRenderDrawBlendMode(g_wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
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

static inline void wlx_sdl3_draw_rect_impl(WLX_Rect rect, WLX_Color color) {
    WLX_SDL3_PERF_INC(set_draw_blend_mode_calls);
    SDL_SetRenderDrawBlendMode(g_wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_wlx_sdl3_renderer, color.r, color.g, color.b, color.a);

    SDL_FRect r = wlx_sdl3_to_frect(rect);
    WLX_SDL3_PERF_INC(render_rect_calls);
    SDL_RenderFillRect(g_wlx_sdl3_renderer, &r);
}

static inline void wlx_sdl3_draw_rect(WLX_Rect rect, WLX_Color color) {
    assert(g_wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(draw_rect_calls);
    wlx_sdl3_draw_rect_impl(rect, color);
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.geometry_ns);
#endif
}

static inline void wlx_sdl3_draw_rect_lines_impl(WLX_Rect rect, float thick, WLX_Color color) {
    WLX_UNUSED(thick);

    WLX_SDL3_PERF_INC(set_draw_blend_mode_calls);
    SDL_SetRenderDrawBlendMode(g_wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_wlx_sdl3_renderer, color.r, color.g, color.b, color.a);

    SDL_FRect r = wlx_sdl3_to_frect(rect);
    WLX_SDL3_PERF_INC(render_rect_calls);
    SDL_RenderRect(g_wlx_sdl3_renderer, &r);
}

static inline void wlx_sdl3_draw_rect_lines(WLX_Rect rect, float thick, WLX_Color color) {
    assert(g_wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(draw_rect_lines_calls);
    wlx_sdl3_draw_rect_lines_impl(rect, thick, color);
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.geometry_ns);
#endif
}

static inline void wlx_sdl3_draw_rect_rounded(
    WLX_Rect rect, float roundness, int segments, WLX_Color color)
{
    assert(g_wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(draw_rect_rounded_calls);

    float r = wlx_sdl3_corner_radius(rect, roundness);
    if (r < 0.5f) {
        wlx_sdl3_draw_rect_impl(rect, color);
#ifdef WLX_PERF
        wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.geometry_ns);
#endif
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

    WLX_SDL3_PERF_INC(geometry_submit_calls);
    WLX_SDL3_PERF_INC(render_geometry_calls);
    SDL_RenderGeometry(g_wlx_sdl3_renderer, NULL, verts, nv, indices, perimeter_count * 3);
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.geometry_ns);
#endif
}

static inline void wlx_sdl3_draw_rect_rounded_lines(
    WLX_Rect rect, float roundness, int segments, float thick, WLX_Color color)
{
    assert(g_wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(draw_rect_rounded_lines_calls);

    float r = wlx_sdl3_corner_radius(rect, roundness);
    if (r < 0.5f) {
        wlx_sdl3_draw_rect_lines_impl(rect, thick, color);
#ifdef WLX_PERF
        wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.geometry_ns);
#endif
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

    WLX_SDL3_PERF_INC(geometry_submit_calls);
    WLX_SDL3_PERF_INC(render_geometry_calls);
    SDL_RenderGeometry(g_wlx_sdl3_renderer, NULL, verts, nv, indices, perimeter_count * 6);
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.geometry_ns);
#endif
}

static inline void wlx_sdl3_draw_circle(float cx, float cy, float radius, int segments, WLX_Color color) {
    assert(g_wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(draw_circle_calls);

    segments = wlx_sdl3_clamp_segments(segments);
    SDL_FColor fc = wlx_sdl3_setup_blend_color(color);

    SDL_Vertex verts[WLX_SDL3_MAX_CORNER_SEGMENTS + 1];
    wlx_sdl3_emit_circle_fan_verts(verts, cx, cy, radius, segments, fc);

    int indices[WLX_SDL3_MAX_CORNER_SEGMENTS * 3];
    wlx_sdl3_emit_fan_indices(indices, segments);

    WLX_SDL3_PERF_INC(geometry_submit_calls);
    WLX_SDL3_PERF_INC(render_geometry_calls);
    SDL_RenderGeometry(g_wlx_sdl3_renderer, NULL, verts, segments + 1, indices, segments * 3);
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.geometry_ns);
#endif
}

static inline void wlx_sdl3_draw_ring(float cx, float cy, float inner_r, float outer_r,
        int segments, WLX_Color color) {
    assert(g_wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(draw_ring_calls);

    segments = wlx_sdl3_clamp_segments(segments);
    SDL_FColor fc = wlx_sdl3_setup_blend_color(color);

    SDL_Vertex verts[WLX_SDL3_MAX_CORNER_SEGMENTS * 2];
    wlx_sdl3_emit_ring_strip_verts(verts, cx, cy, outer_r, inner_r, segments, fc);

    int indices[WLX_SDL3_MAX_CORNER_SEGMENTS * 6];
    wlx_sdl3_emit_strip_indices(indices, segments);

    WLX_SDL3_PERF_INC(geometry_submit_calls);
    WLX_SDL3_PERF_INC(render_geometry_calls);
    SDL_RenderGeometry(g_wlx_sdl3_renderer, NULL, verts, segments * 2, indices, segments * 6);
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.geometry_ns);
#endif
}

static inline void wlx_sdl3_draw_line(float x1, float y1, float x2, float y2, float thick, WLX_Color color) {
    WLX_UNUSED(thick);

    assert(g_wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(draw_line_calls);

    WLX_SDL3_PERF_INC(set_draw_blend_mode_calls);
    SDL_SetRenderDrawBlendMode(g_wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_wlx_sdl3_renderer, color.r, color.g, color.b, color.a);
    WLX_SDL3_PERF_INC(render_line_calls);
    SDL_RenderLine(g_wlx_sdl3_renderer, x1, y1, x2, y2);
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.geometry_ns);
#endif
}

#ifdef SDL_TTF_VERSION
// Forward declarations for the retained TTF_Text cache eviction
// helpers. Definitions live below alongside the cache state, but the
// renderer text-engine renderer-change branch and the font-variant resolver's
// LRU/generation eviction paths both must request retained-text destruction
// before destroying the engine or closing a variant `TTF_Font *`. Forward
// declaration keeps those callers above the cache state in this file.
static inline void wlx_sdl3_destroy_text_cache(void);
static inline void wlx_sdl3_evict_text_cache_for_variant(TTF_Font *variant);

// Backend-owned renderer text engine. Bound to g_wlx_sdl3_renderer;
// invalidated when the renderer pointer changes or via the public flush.
static TTF_TextEngine *g_wlx_sdl3_text_engine = NULL;
static SDL_Renderer *g_wlx_sdl3_text_engine_renderer = NULL;
static bool g_wlx_sdl3_text_engine_failed = false;

static inline void wlx_sdl3_destroy_text_engine(void) {
    if (g_wlx_sdl3_text_engine != NULL) {
        TTF_DestroyRendererTextEngine(g_wlx_sdl3_text_engine);
        g_wlx_sdl3_text_engine = NULL;
    }
    g_wlx_sdl3_text_engine_renderer = NULL;
    g_wlx_sdl3_text_engine_failed = false;
}

static inline TTF_TextEngine *wlx_sdl3_get_text_engine(void) {
    if (g_wlx_sdl3_renderer == NULL) return NULL;

    if (g_wlx_sdl3_text_engine_renderer != g_wlx_sdl3_renderer) {
        // Retained TTF_Text objects are renderer-bound through the old
        // engine; destroy them before the engine they depend on goes away.
        wlx_sdl3_destroy_text_cache();
        if (g_wlx_sdl3_text_engine != NULL) {
            TTF_DestroyRendererTextEngine(g_wlx_sdl3_text_engine);
            g_wlx_sdl3_text_engine = NULL;
        }
        g_wlx_sdl3_text_engine_renderer = g_wlx_sdl3_renderer;
        g_wlx_sdl3_text_engine_failed = false;
    }

    if (g_wlx_sdl3_text_engine != NULL) return g_wlx_sdl3_text_engine;
    if (g_wlx_sdl3_text_engine_failed) return NULL;

    WLX_SDL3_PERF_INC(text_engine_create_attempts);
    g_wlx_sdl3_text_engine = TTF_CreateRendererTextEngine(g_wlx_sdl3_renderer);
    if (g_wlx_sdl3_text_engine == NULL) {
        g_wlx_sdl3_text_engine_failed = true;
    } else {
        WLX_SDL3_PERF_INC(text_engine_create_successes);
    }
    return g_wlx_sdl3_text_engine;
}

// Backend-owned effective font variants.
// One entry per (base_font, font_size, spacing) tuple. The variant is a
// TTF_CopyFont-derived TTF_Font* with size and char spacing applied once at
// publish time, so the hot path can stop mutating the shared base font with
// TTF_SetFontSize. base_generation snapshots TTF_GetFontGeneration(base_font)
// at create time; a mismatch on lookup forces eviction. last_use is a
// monotonically increasing LRU stamp from g_wlx_sdl3_font_variant_cursor.
//
// Variants are not renderer-bound, but they are released alongside renderer-
// bound state by wlx_sdl3_text_cache_clear() to keep teardown a single call.
#ifndef WLX_SDL3_FONT_VARIANT_CAP
#define WLX_SDL3_FONT_VARIANT_CAP 32
#endif

#if WLX_SDL3_FONT_VARIANT_CAP > 0 && defined(SDL_TTF_VERSION_ATLEAST)
#if SDL_TTF_VERSION_ATLEAST(3, 3, 0)
#define WLX_SDL3_HAS_FONT_VARIANTS 1
#else
#define WLX_SDL3_HAS_FONT_VARIANTS 0
#endif
#else
#define WLX_SDL3_HAS_FONT_VARIANTS 0
#endif

#if WLX_SDL3_HAS_FONT_VARIANTS
typedef struct {
    TTF_Font *base_font;
    TTF_Font *variant;
    int       font_size;
    int       spacing;
    Uint32    base_generation;
    uint64_t  last_use;
} WLX_SDL3_Font_Variant;

static WLX_SDL3_Font_Variant wlx_sdl3_font_variants[WLX_SDL3_FONT_VARIANT_CAP] = {0};
static uint64_t g_wlx_sdl3_font_variant_cursor = 0;
#endif

// Backend-owned retained TTF_Text cache.
// One entry per (font_variant, text bytes) pair. Color is not part of the key; 
// TTF_SetTextColor is applied per draw with a
// last_color_rgba32 redundancy. Each entry stores the
// resolved variant pointer (never the base font), the
// size/spacing/generation snapshot used to detect variant invalidation, the
// FNV-1a 64 hash plus full byte length of the cached string for
// collision-rejecting hit verification, owned text bytes (inline up to
// WLX_SDL3_TEXT_INLINE_CAP, heap-allocated otherwise), the retained
// TTF_Text *, the dimensions returned by TTF_GetTextSize at create time, and
// an LRU stamp drawn from g_wlx_sdl3_text_cache_cursor.
//
// Retained TTF_Text * objects are renderer-bound through the text engine and
// variant-bound through the variant pointer. Eviction rules
// destroy entries before the renderer text engine, before variants, and on
// renderer change.
#ifndef WLX_SDL3_TEXT_CACHE_CAP
#define WLX_SDL3_TEXT_CACHE_CAP 1024
#endif

#ifndef WLX_SDL3_TEXT_INLINE_CAP
#define WLX_SDL3_TEXT_INLINE_CAP 64
#endif

#if WLX_SDL3_TEXT_CACHE_CAP > 0 && WLX_SDL3_HAS_FONT_VARIANTS
#define WLX_SDL3_HAS_TEXT_CACHE 1
#else
#define WLX_SDL3_HAS_TEXT_CACHE 0
#endif

#if WLX_SDL3_HAS_TEXT_CACHE
typedef struct {
    TTF_Font *font_variant;
    int       font_size;
    int       spacing;
    Uint32    font_generation;
    size_t    slice_len;
    size_t    text_len;
    uint64_t  text_hash;
    char      inline_bytes[WLX_SDL3_TEXT_INLINE_CAP];
    char     *heap_bytes;
    TTF_Text *text;
    float     cached_w;
    float     cached_h;
    Uint32    last_color_rgba32;
    bool      last_color_set;
    uint64_t  last_use;
} WLX_SDL3_Text_Cache_Entry;

static WLX_SDL3_Text_Cache_Entry 
    wlx_sdl3_text_cache[WLX_SDL3_TEXT_CACHE_CAP] = {0};
static uint64_t g_wlx_sdl3_text_cache_cursor = 0;
#endif

// Resolve a stable TTF_Font* for (base_font, font_size, spacing) without
// mutating the shared base font on the hot path. Returns NULL when the
// caller must fall back to wlx_sdl3_set_font_size on the shared base font
// (TTF_CopyFont failure, TTF_SetFontSize/TTF_SetFontCharSpacing failure,
// or invalid inputs).
//
// On hit, refreshes the LRU stamp and returns the published variant if its
// snapshotted base_generation still matches TTF_GetFontGeneration(base_font);
// otherwise the entry is evicted and treated as miss. On miss, evicts the
// LRU entry if the table is full, calls TTF_CopyFont, applies the size and
// spacing once, and publishes the entry.
static inline TTF_Font *wlx_sdl3_get_font_variant(TTF_Font *base_font, int font_size, int spacing) {
    
    if (base_font == NULL || font_size <= 0) {
        WLX_SDL3_PERF_INC(font_variant_fallback_resolutions);
        return NULL;
    }

    WLX_SDL3_PERF_INC(font_variant_lookups);

#if WLX_SDL3_HAS_FONT_VARIANTS
    Uint32 base_gen = TTF_GetFontGeneration(base_font);

    int      free_slot = -1;
    int      lru_slot = 0;
    uint64_t lru_stamp = wlx_sdl3_font_variants[0].last_use;

    for (int i = 0; i < WLX_SDL3_FONT_VARIANT_CAP; ++i) {
        WLX_SDL3_Font_Variant *e = &wlx_sdl3_font_variants[i];

        if (e->base_font == base_font
                && e->font_size == font_size
                && e->spacing == spacing) {
            if (e->base_generation != base_gen) {
                // Stale variant; evict and treat as miss. Retained TTF_Text
                // entries reference this variant pointer and must be
                // destroyed before TTF_CloseFont releases the variant.
                WLX_SDL3_PERF_INC(font_variant_generation_invalidations);
                if (e->variant != NULL) {
                    wlx_sdl3_evict_text_cache_for_variant(e->variant);
                    TTF_CloseFont(e->variant);
                }
                wlx_zero_struct(*e);
                if (free_slot < 0) free_slot = i;
                continue;
            }
            WLX_SDL3_PERF_INC(font_variant_hits);
            e->last_use = ++g_wlx_sdl3_font_variant_cursor;
            return e->variant;
        }

        if (e->base_font == NULL) {
            if (free_slot < 0) free_slot = i;
        } else if (e->last_use < lru_stamp) {
            lru_stamp = e->last_use;
            lru_slot = i;
        }
    }

    WLX_SDL3_PERF_INC(font_variant_misses);

    int slot = (free_slot >= 0) ? free_slot : lru_slot;
    WLX_SDL3_Font_Variant *e = &wlx_sdl3_font_variants[slot];
    if (e->variant != NULL) {
        // LRU eviction: evict any retained TTF_Text dependents before
        // releasing the variant they reference.
        WLX_SDL3_PERF_INC(font_variant_evictions);
        wlx_sdl3_evict_text_cache_for_variant(e->variant);
        TTF_CloseFont(e->variant);
        wlx_zero_struct(*e);
    }

    TTF_Font *variant = TTF_CopyFont(base_font);
    if (variant == NULL) {
        WLX_SDL3_PERF_INC(font_variant_fallback_resolutions);
        return NULL;
    }
    if (!TTF_SetFontSize(variant, (float)font_size)) {
        TTF_CloseFont(variant);
        WLX_SDL3_PERF_INC(font_variant_fallback_resolutions);
        return NULL;
    }
    WLX_SDL3_PERF_INC(ttf_set_font_char_spacing_calls);
    if (!TTF_SetFontCharSpacing(variant, spacing)) {
        TTF_CloseFont(variant);
        WLX_SDL3_PERF_INC(font_variant_fallback_resolutions);
        return NULL;
    }

    WLX_SDL3_PERF_INC(font_variant_creates);
    e->base_font       = base_font;
    e->variant         = variant;
    e->font_size       = font_size;
    e->spacing         = spacing;
    e->base_generation = base_gen;
    e->last_use        = ++g_wlx_sdl3_font_variant_cursor;
    return variant;
#else
    WLX_UNUSED(spacing);
    WLX_SDL3_PERF_INC(font_variant_fallback_resolutions);
    return NULL;
#endif
}

#if WLX_SDL3_HAS_TEXT_CACHE
// FNV-1a 64-bit hash over the byte range [text, text+len). Used as the text
// portion of the retained TTF_Text cache key. Hash matches alone are not
// sufficient for a hit; entries also store text_len and the original bytes
// for collision-rejecting verification.
static inline uint64_t wlx_sdl3_text_cache_hash(const char *text, size_t len) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; ++i) {
        h ^= (uint64_t)(unsigned char)text[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

// Bytes accessor: inline buffer when text fits, heap allocation otherwise.
static inline const char *wlx_sdl3_text_cache_entry_bytes(const WLX_SDL3_Text_Cache_Entry *e) {
    return (e->heap_bytes != NULL) ? e->heap_bytes : e->inline_bytes;
}

// Release any heap-owned bytes for an entry without zeroing the entry.
static inline void wlx_sdl3_text_cache_release_bytes(WLX_SDL3_Text_Cache_Entry *e) {
    if (e->heap_bytes != NULL) {
        WLX_SDL3_PERF_INC(text_cache_heap_frees);
        wlx_free(e->heap_bytes);
        e->heap_bytes = NULL;
    }
}

// Destroy the retained TTF_Text and any heap-owned bytes for an entry, then
// zero it. Bumps both text_cache_destroys counter and the 
// ttf_text_destroys counter so totals stay consistent. Callers that need
// eviction or invalidation accounting must increment the appropriate counter
// before calling.
static inline void wlx_sdl3_text_cache_destroy_entry(WLX_SDL3_Text_Cache_Entry *e) {
    if (e->text != NULL) {
        WLX_SDL3_PERF_INC(text_cache_destroys);
        WLX_SDL3_PERF_INC(ttf_text_destroys);
        TTF_DestroyText(e->text);
    }
    wlx_sdl3_text_cache_release_bytes(e);
    wlx_zero_struct(*e);
}
#endif

// Destroy every retained TTF_Text in the cache, free any heap-owned text
// bytes, and reset the LRU cursor. Safe before first use and idempotent
// across repeated calls. When WLX_SDL3_HAS_TEXT_CACHE is 0 this is a no-op
// so callers and the public flush remain link-compatible.
static inline void wlx_sdl3_destroy_text_cache(void) {
#if WLX_SDL3_HAS_TEXT_CACHE
    for (int i = 0; i < WLX_SDL3_TEXT_CACHE_CAP; ++i) {
        WLX_SDL3_Text_Cache_Entry *e = &wlx_sdl3_text_cache[i];
        if (e->text != NULL) {
            wlx_sdl3_text_cache_destroy_entry(e);
        } else if (e->heap_bytes != NULL) {
            wlx_sdl3_text_cache_release_bytes(e);
            wlx_zero_struct(*e);
        }
    }
    g_wlx_sdl3_text_cache_cursor = 0;
#endif
}

// Cascade-evict every retained TTF_Text whose stored variant pointer matches
// the supplied `variant`. Called by the font-variant resolver before
// TTF_CloseFont releases a variant, so retained entries cannot dangle past
// the lifetime of the underlying TTF_Font *. Increments the
// text_cache_variant_invalidation_evictions counter once per evicted entry.
// No-op when WLX_SDL3_HAS_TEXT_CACHE is 0.
static inline void wlx_sdl3_evict_text_cache_for_variant(TTF_Font *variant) {
#if WLX_SDL3_HAS_TEXT_CACHE
    if (variant == NULL) return;
    for (int i = 0; i < WLX_SDL3_TEXT_CACHE_CAP; ++i) {
        WLX_SDL3_Text_Cache_Entry *e = &wlx_sdl3_text_cache[i];
        if (e->text != NULL && e->font_variant == variant) {
            WLX_SDL3_PERF_INC(text_cache_variant_invalidation_evictions);
            wlx_sdl3_text_cache_destroy_entry(e);
        }
    }
#else
    WLX_UNUSED(variant);
#endif
}

// Resolve a retained TTF_Text * for (variant, text, slice_len). Returns the
// cache entry on success; the caller may read entry->text, entry->cached_w,
// entry->cached_h, and mutate entry->last_color_rgba32 / entry->last_color_set
// when applying TTF_SetTextColor immediately before draw. Returns NULL when
// the caller must fall back to the per-call path: NULL inputs, empty
// effective length, TTF_CreateText failure, owned-bytes allocation failure,
// or compile-time disable (WLX_SDL3_HAS_TEXT_CACHE = 0).
//
// slice_len == 0 means "until NUL" per SDL_ttf; the helper normalizes to the
// strlen byte range so cache hits do not depend on caller phrasing. The
// retained text object is created with the explicit normalized length.
//
// On hit, verifies stored text bytes against the request (collision-rejecting)
// and re-checks TTF_GetFontGeneration on the variant; mismatches evict and
// follow the miss path. On miss, evicts the LRU entry (or reuses a free slot
// or a slot freed by collision/generation eviction) and publishes a new
// retained entry with snapshotted variant pointer, size, spacing, generation,
// hash, length, owned bytes, TTF_GetTextSize dimensions, and a forced-color
// state so the next draw sets TTF_SetTextColor.
#if WLX_SDL3_HAS_TEXT_CACHE
static inline WLX_SDL3_Text_Cache_Entry *wlx_sdl3_get_text_cache_entry(
        TTF_TextEngine *engine, TTF_Font *variant,
        int font_size, int spacing,
        const char *text, size_t slice_len)
{
    if (engine == NULL || variant == NULL || text == NULL) {
        WLX_SDL3_PERF_INC(text_cache_fallback_resolutions);
        return NULL;
    }

    size_t effective_len = (slice_len == 0) ? strlen(text) : slice_len;
    if (effective_len == 0) {
        WLX_SDL3_PERF_INC(text_cache_fallback_resolutions);
        return NULL;
    }

    WLX_SDL3_PERF_INC(text_cache_lookups);

    uint64_t text_hash   = wlx_sdl3_text_cache_hash(text, effective_len);
    Uint32   variant_gen = TTF_GetFontGeneration(variant);

    int      free_slot = -1;
    int      lru_slot  = 0;
    uint64_t lru_stamp = wlx_sdl3_text_cache[0].last_use;

    // Code is evicting (removing) cache entries that collide (same hash, font, etc., 
    // but different text or font generation) to ensure that only valid, 
    // non-colliding entries remain in the cache. This prevents stale or mismatched 
    // entries from being reused, so every cache slot is either empty, a valid match, 
    // or available for new data. This keeps the cache consistent and avoids collisions.
   
    // NOTE: If this turns out to be a performance bottleneck due to high collision rates, 
    // consider using set-associative cache techniques or a more robust hashing strategy to 
    // reduce collisions.
    for (int i = 0; i < WLX_SDL3_TEXT_CACHE_CAP; ++i) {
        WLX_SDL3_Text_Cache_Entry *e = &wlx_sdl3_text_cache[i];

        if (e->text != NULL
            && e->font_variant == variant
            && e->slice_len == effective_len
            && e->text_hash == text_hash) 
        {
            const char *stored = wlx_sdl3_text_cache_entry_bytes(e);
            if (e->text_len != effective_len
                    || memcmp(stored, text, effective_len) != 0) {
                WLX_SDL3_PERF_INC(text_cache_collision_rejections);
                wlx_sdl3_text_cache_destroy_entry(e);
                if (free_slot < 0) free_slot = i;
                continue;
            }
            if (e->font_generation != variant_gen) {
                wlx_sdl3_text_cache_destroy_entry(e);
                if (free_slot < 0) free_slot = i;
                continue;
            }
            WLX_SDL3_PERF_INC(text_cache_hits);
            e->last_use = ++g_wlx_sdl3_text_cache_cursor;
            return e;
        }

        if (e->text == NULL) {
            if (free_slot < 0) free_slot = i;
        } else if (e->last_use < lru_stamp) {
            lru_stamp = e->last_use;
            lru_slot  = i;
        }
    }

    WLX_SDL3_PERF_INC(text_cache_misses);

    int slot = (free_slot >= 0) ? free_slot : lru_slot;
    WLX_SDL3_Text_Cache_Entry *e = &wlx_sdl3_text_cache[slot];
    if (e->text != NULL) {
        WLX_SDL3_PERF_INC(text_cache_evictions);
        wlx_sdl3_text_cache_destroy_entry(e);
    } else if (e->heap_bytes != NULL) {
        wlx_sdl3_text_cache_release_bytes(e);
    }

    WLX_SDL3_PERF_INC(text_cache_creates);
    WLX_SDL3_PERF_INC(ttf_text_creates);
    TTF_Text *ttext = TTF_CreateText(engine, variant, text, effective_len);
    if (ttext == NULL) {
        WLX_SDL3_PERF_INC(text_cache_fallback_resolutions);
        wlx_zero_struct(*e);
        return NULL;
    }

    int w = 0, h = 0;
    TTF_GetTextSize(ttext, &w, &h);

    char *bytes_dst;
    if (effective_len <= (size_t)WLX_SDL3_TEXT_INLINE_CAP) {
        bytes_dst     = e->inline_bytes;
        e->heap_bytes = NULL;
    } else {
        WLX_SDL3_PERF_INC(text_cache_heap_allocs);
        e->heap_bytes = (char *)wlx_alloc(effective_len);
        if (e->heap_bytes == NULL) {
            WLX_SDL3_PERF_INC(ttf_text_destroys);
            TTF_DestroyText(ttext);
            WLX_SDL3_PERF_INC(text_cache_fallback_resolutions);
            wlx_zero_struct(*e);
            return NULL;
        }
        bytes_dst = e->heap_bytes;
    }
    memcpy(bytes_dst, text, effective_len);

    e->font_variant      = variant;
    e->font_size         = font_size;
    e->spacing           = spacing;
    e->font_generation   = variant_gen;
    e->slice_len         = effective_len;
    e->text_len          = effective_len;
    e->text_hash         = text_hash;
    e->text              = ttext;
    e->cached_w          = (float)w;
    e->cached_h          = (float)h;
    e->last_color_rgba32 = 0;
    e->last_color_set    = false;
    e->last_use          = ++g_wlx_sdl3_text_cache_cursor;
    return e;
}
#endif

static inline void wlx_sdl3_set_font_size(TTF_Font *font, int font_size) {
    if (font == NULL || font_size <= 0) return;

    float target = (float)font_size;
    float current = TTF_GetFontSize(font);
    if (current < target - 0.01f || current > target + 0.01f) {
        WLX_SDL3_PERF_INC(set_font_size_calls);
        TTF_SetFontSize(font, target);
    }
}

// Custom-font draw shared by wlx_sdl3_draw_text and wlx_sdl3_draw_text_slice.
// slice_len == 0 means draw the null-terminated string, matching SDL_ttf.
// Resolves a stable effective-font variant first; when the variant is
// available and a retained TTF_Text is in the cache, draws from the cached
// object with TTF_SetTextColor applied per draw under a redundancy skip. On
// retained-cache miss/failure falls through to the per-call
// TTF_CreateText path, then to the surface-to-texture path.
static inline void wlx_sdl3_draw_ttf_text(
        TTF_Font *base_font, const char *text, size_t slice_len,
        float x, float y, WLX_Text_Style style)
{
    float draw_x = wlx_sdl3_snap_text_coord(x);
    float draw_y = wlx_sdl3_snap_text_coord(y);
    TTF_Font *variant = wlx_sdl3_get_font_variant(base_font, style.font_size, style.spacing);
    TTF_Font *font = (variant != NULL) ? variant : base_font;
    if (variant == NULL) {
        wlx_sdl3_set_font_size(font, style.font_size);
    }

    TTF_TextEngine *engine = wlx_sdl3_get_text_engine();
    if (engine != NULL) {
#if WLX_SDL3_HAS_TEXT_CACHE
        if (variant != NULL) {
            WLX_SDL3_Text_Cache_Entry *entry = wlx_sdl3_get_text_cache_entry(
                    engine, variant, style.font_size, style.spacing,
                    text, slice_len);

            if (entry != NULL) {

                Uint32 packed_color =
                    ((Uint32)style.color.r << 24)
                    | ((Uint32)style.color.g << 16)
                    | ((Uint32)style.color.b << 8)
                    | (Uint32)style.color.a;

                if (!entry->last_color_set || entry->last_color_rgba32 != packed_color) {
                    TTF_SetTextColor(entry->text,
                        style.color.r, style.color.g,
                        style.color.b, style.color.a);
                    entry->last_color_rgba32 = packed_color;
                    entry->last_color_set    = true;
                    WLX_SDL3_PERF_INC(text_cache_color_mutations);
                } else {
                    WLX_SDL3_PERF_INC(text_cache_color_skips);
                }
                WLX_SDL3_PERF_INC(text_engine_draw_calls);
                if (TTF_DrawRendererText(entry->text, draw_x, draw_y)) {
                    return;
                }
            }
        }
#endif
        WLX_SDL3_PERF_INC(ttf_text_creates);
        TTF_Text *ttext = TTF_CreateText(engine, font, text, slice_len);
        if (ttext != NULL) {
            TTF_SetTextColor(ttext,
                style.color.r, style.color.g, style.color.b, style.color.a);
            WLX_SDL3_PERF_INC(text_engine_draw_calls);
            bool drew = TTF_DrawRendererText(ttext, draw_x, draw_y);
            WLX_SDL3_PERF_INC(ttf_text_destroys);
            TTF_DestroyText(ttext);
            if (drew) return;
        }
    }

    WLX_SDL3_PERF_INC(text_engine_fallback_draw_calls);
    SDL_Color fg = wlx_sdl3_to_color(style.color);
    WLX_SDL3_PERF_INC(ttf_render_text_blended_calls);
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, slice_len, fg);
    if (surf) {
        WLX_SDL3_PERF_INC(create_texture_from_surface_calls);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(g_wlx_sdl3_renderer, surf);
        if (tex) {
            SDL_FRect dst = { draw_x, draw_y, (float)surf->w, (float)surf->h };
            WLX_SDL3_PERF_INC(render_texture_calls);
            SDL_RenderTexture(g_wlx_sdl3_renderer, tex, NULL, &dst);
            WLX_SDL3_PERF_INC(destroy_texture_calls);
            SDL_DestroyTexture(tex);
        }
        WLX_SDL3_PERF_INC(destroy_surface_calls);
        SDL_DestroySurface(surf);
    }
}

// Custom-font measure shared by wlx_sdl3_measure_text and
// wlx_sdl3_measure_text_slice. slice_len == 0 means measure the
// null-terminated string, matching SDL_ttf. Resolves a stable effective-font
// variant first; when the variant is available and the retained text cache
// returns an entry, returns its TTF_GetTextSize dimensions (warming the draw
// cache during build phase). On retained-cache miss/failure falls through to
// TTF_GetStringSize on the variant, or to the shared base font with
// wlx_sdl3_set_font_size when the variant itself is unavailable (spacing
// ignored on that path).
static inline void wlx_sdl3_measure_ttf_text(
        TTF_Font *base_font, const char *text, size_t slice_len,
        WLX_Text_Style style, float *out_w, float *out_h)
{
    TTF_Font *variant = wlx_sdl3_get_font_variant(base_font, style.font_size, style.spacing);
    TTF_Font *font = (variant != NULL) ? variant : base_font;
    if (variant == NULL) {
        wlx_sdl3_set_font_size(font, style.font_size);
    }

#if WLX_SDL3_HAS_TEXT_CACHE
    if (variant != NULL) {
        TTF_TextEngine *engine = wlx_sdl3_get_text_engine();
        if (engine != NULL) {
            WLX_SDL3_Text_Cache_Entry *entry = wlx_sdl3_get_text_cache_entry(
                    engine, variant, style.font_size, style.spacing,
                    text, slice_len);

            if (entry != NULL) {
                *out_w = entry->cached_w;
                *out_h = entry->cached_h;
                return;
            }
        }
    }
#endif

    int w = 0, h = 0;
    WLX_SDL3_PERF_INC(ttf_get_string_size_calls);
    TTF_GetStringSize(font, text, slice_len, &w, &h);
    *out_w = (float)w;
    *out_h = (float)h;
}
#endif

#ifdef SDL_TTF_VERSION
// Close every live variant in the table and reset bookkeeping. Safe before
// first use and idempotent across repeated calls. Variants are not
// renderer-bound, so this does not depend on g_wlx_sdl3_renderer.
static inline void wlx_sdl3_destroy_font_variants(void) {
#if WLX_SDL3_HAS_FONT_VARIANTS
    for (int i = 0; i < WLX_SDL3_FONT_VARIANT_CAP; ++i) {
        WLX_SDL3_Font_Variant *e = &wlx_sdl3_font_variants[i];
        if (e->variant != NULL) TTF_CloseFont(e->variant);
        wlx_zero_struct(*e);
    }
    g_wlx_sdl3_font_variant_cursor = 0;
#endif
}
#endif

// Public flush for backend-owned SDL3 text state. Safe to call before first
// draw and around renderer replacement. Teardown order:
//   1. Retained TTF_Text cache (depends on the renderer text engine and on
//      backend-owned variants).
//   2. Backend-owned font variants (depend on user-owned base fonts).
//   3. Renderer text engine (depends on the active SDL renderer).
// Callers must invoke this before TTF_CloseFont() on any base font handle
// previously passed to the SDL3 backend, since variants share the underlying
// font file with the base and retained TTF_Text entries reference variants.
static inline void wlx_sdl3_text_cache_clear(void) {
#ifdef SDL_TTF_VERSION
    wlx_sdl3_destroy_text_cache();
    wlx_sdl3_destroy_font_variants();
    wlx_sdl3_destroy_text_engine();
#endif
}

static inline void wlx_sdl3_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    assert(g_wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");
    if (text == NULL || text[0] == '\0') return;

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(draw_text_calls);

#ifdef SDL_TTF_VERSION
    if (style.font != WLX_FONT_DEFAULT) {
        TTF_Font *font = (TTF_Font *)(uintptr_t)style.font;
        wlx_sdl3_draw_ttf_text(font, text, 0, x, y, style);
#ifdef WLX_PERF
        wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.text_draw_ns);
#endif
        return;
    }
#endif

    // Fallback: debug font
    SDL_Color sc = wlx_sdl3_to_color(style.color);
    WLX_SDL3_PERF_INC(set_draw_blend_mode_calls);
    SDL_SetRenderDrawBlendMode(g_wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_wlx_sdl3_renderer, sc.r, sc.g, sc.b, sc.a);
    SDL_RenderDebugText(g_wlx_sdl3_renderer,
        wlx_sdl3_snap_text_coord(x), wlx_sdl3_snap_text_coord(y), text);
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.text_draw_ns);
#endif
}

static inline void wlx_sdl3_measure_text(const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
    if (out_w == NULL || out_h == NULL) return;
    if (text == NULL) text = "";

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(measure_text_calls);

#ifdef SDL_TTF_VERSION
    if (style.font != WLX_FONT_DEFAULT) {
        TTF_Font *base_font = (TTF_Font *)(uintptr_t)style.font;
        wlx_sdl3_measure_ttf_text(base_font, text, 0, style, out_w, out_h);
#ifdef WLX_PERF
        wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.text_measure_ns);
#endif
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

    *out_w = (float)len * glyph_w;
    *out_h = glyph_h;
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.text_measure_ns);
#endif
}

// Slice-aware draw: accepts explicit byte length.
// Uses SDL3_ttf with exact length for custom fonts; falls back to a
// null-terminated copy for the debug font path.
static inline void wlx_sdl3_draw_text_slice(
        const char *text, size_t slice_len, float x, float y, WLX_Text_Style style) {
    assert(g_wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");
    if (text == NULL || slice_len == 0) return;

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(draw_text_calls);

#ifdef SDL_TTF_VERSION
    if (style.font != WLX_FONT_DEFAULT) {
        TTF_Font *font = (TTF_Font *)(uintptr_t)style.font;
        wlx_sdl3_draw_ttf_text(font, text, slice_len, x, y, style);
#ifdef WLX_PERF
        wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.text_draw_ns);
#endif
        return;
    }
#endif

    // Fallback: debug font (SDL_RenderDebugText requires null-terminated).
    if (text[slice_len] == '\0') {
        SDL_Color sc = wlx_sdl3_to_color(style.color);
        WLX_SDL3_PERF_INC(set_draw_blend_mode_calls);
        SDL_SetRenderDrawBlendMode(g_wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(g_wlx_sdl3_renderer, sc.r, sc.g, sc.b, sc.a);
        SDL_RenderDebugText(g_wlx_sdl3_renderer,
            wlx_sdl3_snap_text_coord(x), wlx_sdl3_snap_text_coord(y), text);
    } else {
        char stack_buf[WLX_TEXT_RANGE_STACK_CAP];
        char *buf = stack_buf;
        bool heap_buf = false;
        if (slice_len + 1 > sizeof(stack_buf)) {
            buf = (char *)wlx_alloc(slice_len + 1);
            if (buf == NULL) {
#ifdef WLX_PERF
                wlx_perf_sdl3_time_end(perf_start_ns,
                    &g_wlx_perf_sdl3_state.current.text_draw_ns);
#endif
                return;
            }
            heap_buf = true;
        }
        memcpy(buf, text, slice_len);
        buf[slice_len] = '\0';
        SDL_Color sc = wlx_sdl3_to_color(style.color);
        WLX_SDL3_PERF_INC(set_draw_blend_mode_calls);
        SDL_SetRenderDrawBlendMode(g_wlx_sdl3_renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(g_wlx_sdl3_renderer, sc.r, sc.g, sc.b, sc.a);
        SDL_RenderDebugText(g_wlx_sdl3_renderer,
            wlx_sdl3_snap_text_coord(x), wlx_sdl3_snap_text_coord(y), buf);
        if (heap_buf) wlx_free(buf);
    }
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.text_draw_ns);
#endif
}

// Slice-aware measure: accepts explicit byte length.
// Uses SDL3_ttf with exact length for custom fonts; falls back to codepoint
// estimation for the debug font path.
static inline void wlx_sdl3_measure_text_slice(
        const char *text, size_t slice_len, WLX_Text_Style style,
        float *out_w, float *out_h) {
    if (out_w == NULL || out_h == NULL) return;
    if (text == NULL) text = "";

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(measure_text_calls);

#ifdef SDL_TTF_VERSION
    if (style.font != WLX_FONT_DEFAULT) {
        TTF_Font *base_font = (TTF_Font *)(uintptr_t)style.font;
        wlx_sdl3_measure_ttf_text(base_font, text, slice_len, style, out_w, out_h);
#ifdef WLX_PERF
        wlx_perf_sdl3_time_end(perf_start_ns,
            &g_wlx_perf_sdl3_state.current.text_measure_ns);
#endif
        return;
    }
#endif

    // Fallback: debug font estimation using codepoint count in the span.
    float glyph_w = (style.font_size > 0) ? (style.font_size * 0.5f) : 8.0f;
    float glyph_h = (style.font_size > 0) ? (float)style.font_size : 8.0f;

    size_t codepoints = wlx_utf8_slicelen(text, slice_len);

    if (codepoints == 0) {
        *out_w = glyph_w;
        *out_h = glyph_h;
    } else {
        *out_w = (float)codepoints * glyph_w;
        *out_h = glyph_h;
    }
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.text_measure_ns);
#endif
}

static inline void wlx_sdl3_begin_scissor(WLX_Rect rect) {
    assert(g_wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(begin_scissor_calls);
    WLX_SDL3_PERF_INC(clip_change_calls);

    SDL_Rect r = {
        .x = (int)rect.x,
        .y = (int)rect.y,
        .w = (int)rect.w,
        .h = (int)rect.h,
    };
    WLX_SDL3_PERF_INC(set_clip_rect_calls);
    SDL_SetRenderClipRect(g_wlx_sdl3_renderer, &r);
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.scissor_ns);
#endif
}

static inline void wlx_sdl3_end_scissor(void) {
    assert(g_wlx_sdl3_renderer != NULL && "SDL3 renderer is not set");

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_sdl3_time_begin();
#endif
    WLX_SDL3_PERF_INC(end_scissor_calls);
    WLX_SDL3_PERF_INC(clip_change_calls);
    WLX_SDL3_PERF_INC(set_clip_rect_calls);
    SDL_SetRenderClipRect(g_wlx_sdl3_renderer, NULL);
#ifdef WLX_PERF
    wlx_perf_sdl3_time_end(perf_start_ns, &g_wlx_perf_sdl3_state.current.scissor_ns);
#endif
}

static inline float wlx_sdl3_get_frame_time(void) {
    Uint64 counter = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();

    if (g_wlx_sdl3_last_counter == 0 || frequency == 0) {
        g_wlx_sdl3_last_counter = counter;
        return 0.0f;
    }

    float frame_time = (float)(counter - g_wlx_sdl3_last_counter) / (float)frequency;
    g_wlx_sdl3_last_counter = counter;
    return frame_time;
}

static inline WLX_Backend wlx_backend_sdl3(SDL_Renderer *renderer) {
    g_wlx_sdl3_renderer = renderer;
    g_wlx_sdl3_last_counter = 0;

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
        .draw_text_slice    = wlx_sdl3_draw_text_slice,
        .measure_text_slice = wlx_sdl3_measure_text_slice,
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

    if (!g_wlx_sdl3_event_watch_installed) {
        SDL_AddEventWatch(wlx_sdl3_event_watch, NULL);
        g_wlx_sdl3_event_watch_installed = true;
    }

    SDL_StartTextInput(window);
    ctx->backend = wlx_backend_sdl3(renderer);
#ifdef WLX_PERF
    wlx_perf_sdl3_install_timer(ctx);
#endif
}

#undef WLX_SDL3_PERF_INC

#endif // WOLLIX_SDL3_H_
