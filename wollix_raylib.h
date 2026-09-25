/*
 * Copyright (c) 2026 Dainis Berzins
 * Licensed under the MIT License. See LICENSE file for full text.
 */

#ifndef WOLLIX_RAYLIB_H_
#define WOLLIX_RAYLIB_H_

#if defined(__INTELLISENSE__) && !defined(WOLLIX_H_)
#include "wollix.h"
#endif

#ifndef WOLLIX_H_
#error "Include wollix.h before wollix_raylib.h"
#endif

#ifndef RAYLIB_H
#error "Include raylib.h before wollix.h when using wollix_raylib.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Capacity of the Raylib backend measurement cache (default 2048).
// Originally suggested 256 from the analysis estimate of 100-300
// unique (font, size, spacing, text) tuples per frame; Validation
// captured ~1700 unique tuples on the warmed Theme Lab Dark workload at
// 1920x1080, mirroring the working-set finding that drove SDL3's default
// cap up (now 4096). The Raylib default has therefore been raised to
// 2048 to keep steady-state hit rate comfortably above the >95% target
// without thrashing. Setting this to 0 disables the cache entirely; lookup
// short-circuits to a forced miss with no counter increments and no table
// is allocated. Mirrors the SDL3 WLX_SDL3_TEXT_CACHE_CAP=0 disable contract.
#ifndef WLX_RAYLIB_TEXT_CACHE_CAP
#define WLX_RAYLIB_TEXT_CACHE_CAP 2048
#endif

// Linear-probe window for the open-addressed measurement table. Lookups walk
// up to this many slots starting at (text_hash & (N-1)) before declaring a
// miss. 8 slots is the locked default.
#ifndef WLX_RAYLIB_TEXT_CACHE_PROBE_LIMIT
#define WLX_RAYLIB_TEXT_CACHE_PROBE_LIMIT 8
#endif

#if WLX_RAYLIB_TEXT_CACHE_CAP > 0
// Pick the smallest power of two strictly greater than CAP * 5/3 so the load
// factor at full CAP stays below 0.6. For the default CAP=2048 this resolves
// to 4096 slots (load factor max 0.5). Hardcoded list covers caps up to 65536.
#define WLX_RAYLIB_TEXT_CACHE__MIN_SLOTS \
    ((unsigned)((WLX_RAYLIB_TEXT_CACHE_CAP) * 5 / 3 + 1))
#define WLX_RAYLIB_TEXT_CACHE__POW2_GE(x) \
    ((x) <= 1u     ? 1u     : (x) <= 2u     ? 2u     : (x) <= 4u     ? 4u     : \
     (x) <= 8u     ? 8u     : (x) <= 16u    ? 16u    : (x) <= 32u    ? 32u    : \
     (x) <= 64u    ? 64u    : (x) <= 128u   ? 128u   : (x) <= 256u   ? 256u   : \
     (x) <= 512u   ? 512u   : (x) <= 1024u  ? 1024u  : (x) <= 2048u  ? 2048u  : \
     (x) <= 4096u  ? 4096u  : (x) <= 8192u  ? 8192u  : (x) <= 16384u ? 16384u : \
     (x) <= 32768u ? 32768u : 65536u)
#define WLX_RAYLIB_TEXT_CACHE_SLOTS \
    WLX_RAYLIB_TEXT_CACHE__POW2_GE(WLX_RAYLIB_TEXT_CACHE__MIN_SLOTS)

typedef struct {
    uintptr_t font_handle;
    uint32_t  font_size_bits;
    uint32_t  spacing_bits;
    uint32_t  text_len;
    uint64_t  text_hash;
    float     w;
    float     h;
    uint32_t  generation;
} WLX_Raylib_Text_Cache_Entry;

static WLX_Raylib_Text_Cache_Entry
    g_wlx_raylib_text_cache[WLX_RAYLIB_TEXT_CACHE_SLOTS] = {0};
// Generation 0 marks an entry as stale/empty. wlx_raylib_text_cache_clear()
// bumps the generation; the next store into a slot rewrites its generation.
static uint32_t g_wlx_raylib_text_cache_generation = 1;
// Slot chosen by the most recent lookup miss; consumed by the next store.
static size_t   g_wlx_raylib_text_cache_pending_slot = 0;
static bool     g_wlx_raylib_text_cache_pending_valid = false;
#endif

// Cache key: (font_handle, font_size_bits, spacing_bits, text_len, and the
// core's wlx_hash_fnv1a64 over the text) without storing the original
// bytes; hash collisions therefore manifest as false-positive hits and are
// tracked by text_cache_collision_rejections.

static inline uint32_t wlx_raylib_text_cache_float_bits(float v) {
    union { float f; uint32_t u; } pun;
    pun.f = v;
    return pun.u;
}

#ifdef WLX_PERF
typedef struct {
    WLX_PERF_BACKEND_COMMON_FIELDS;
    uint64_t text_cache_lookups;
    uint64_t text_cache_hits;
    uint64_t text_cache_misses;
    uint64_t text_cache_evictions;
    uint64_t text_cache_collision_rejections;
} WLX_Perf_Raylib_Frame;

typedef struct {
    WLX_Perf_Raylib_Frame current;
    WLX_Perf_Raylib_Frame last;
    WLX_Perf_Backend_Clock clock;
} WLX_Perf_Raylib_State;

static WLX_Perf_Raylib_State g_wlx_perf_raylib_state = {0};

static inline uint64_t wlx_perf_raylib_timestamp(void *user) {
    double seconds;
    WLX_UNUSED(user);

    seconds = GetTime();
    if (seconds <= 0.0) return 0;
    return (uint64_t)(seconds * 1000000000.0);
}

static inline void wlx_perf_raylib_install_timer(WLX_Context *ctx) {
    wlx_perf_set_timer(ctx, wlx_perf_raylib_timestamp, NULL);
}

static inline void wlx_perf_raylib_begin_frame(uint64_t frame_index) {
    WLX_Perf_Raylib_State *st = &g_wlx_perf_raylib_state;
    wlx_zero_struct(st->current);
    st->current.frame_index = frame_index;
    st->current.timer_available = true;
    st->clock.timer_available = true;
    st->clock.timestamp = wlx_perf_raylib_timestamp;
    st->clock.timestamp_user = NULL;
    wlx_perf_backend_frame_begin(&st->clock);
}

static inline void wlx_perf_raylib_end_frame(uint64_t frame_index) {
    WLX_Perf_Raylib_State *st = &g_wlx_perf_raylib_state;
    if (!st->clock.capturing) return;
    if (frame_index != 0) st->current.frame_index = frame_index;
    st->last = st->current;
    wlx_perf_backend_frame_end(&st->clock);
}

static inline void wlx_perf_raylib_reset(void) {
    wlx_zero_struct(g_wlx_perf_raylib_state);
}

static inline const WLX_Perf_Raylib_Frame *wlx_perf_raylib_get_last_frame(void) {
    return &g_wlx_perf_raylib_state.last;
}

static inline void wlx_perf_raylib_present_begin(void) {
    wlx_perf_backend_present_begin(&g_wlx_perf_raylib_state.clock);
}

static inline void wlx_perf_raylib_present_end(void) {
    wlx_perf_backend_present_end(&g_wlx_perf_raylib_state.clock,
        &g_wlx_perf_raylib_state.current.present_ns);
}

#define WLX_RAYLIB_PERF_INC(field) \
    wlx_perf_backend_inc(&g_wlx_perf_raylib_state.clock, \
                         &g_wlx_perf_raylib_state.current.field)
#else
#define WLX_RAYLIB_PERF_INC(field) ((void)0)
#endif

// Timed scope of one backend callback body: END (one per exit path)
// accumulates into the named duration field; both are no-ops without
// WLX_PERF.
#define WLX_RAYLIB_SCOPE_BEGIN() \
    WLX_PERF_SCOPE_BEGIN(&g_wlx_perf_raylib_state.clock)
#define WLX_RAYLIB_SCOPE_END(field) \
    WLX_PERF_SCOPE_END(&g_wlx_perf_raylib_state.clock, \
                       &g_wlx_perf_raylib_state.current.field)

// Raylib key for one WLX key code (0 = unmapped). WLX -> platform like the
// SDL3 table, so the two maps read side by side; the switch has no default,
// so -Wswitch reports any WLX_Key_Code added without a mapping here.
static inline int wlx_raylib_key_for(WLX_Key_Code key) {
    switch (key) {
        case WLX_KEY_NONE: return 0;
        case WLX_KEY_ESCAPE: return KEY_ESCAPE;
        case WLX_KEY_ENTER: return KEY_ENTER;
        case WLX_KEY_BACKSPACE: return KEY_BACKSPACE;
        case WLX_KEY_TAB: return KEY_TAB;
        case WLX_KEY_SPACE: return KEY_SPACE;
        case WLX_KEY_LEFT: return KEY_LEFT;
        case WLX_KEY_RIGHT: return KEY_RIGHT;
        case WLX_KEY_UP: return KEY_UP;
        case WLX_KEY_DOWN: return KEY_DOWN;
        case WLX_KEY_A: return KEY_A;
        case WLX_KEY_B: return KEY_B;
        case WLX_KEY_C: return KEY_C;
        case WLX_KEY_D: return KEY_D;
        case WLX_KEY_E: return KEY_E;
        case WLX_KEY_F: return KEY_F;
        case WLX_KEY_G: return KEY_G;
        case WLX_KEY_H: return KEY_H;
        case WLX_KEY_I: return KEY_I;
        case WLX_KEY_J: return KEY_J;
        case WLX_KEY_K: return KEY_K;
        case WLX_KEY_L: return KEY_L;
        case WLX_KEY_M: return KEY_M;
        case WLX_KEY_N: return KEY_N;
        case WLX_KEY_O: return KEY_O;
        case WLX_KEY_P: return KEY_P;
        case WLX_KEY_Q: return KEY_Q;
        case WLX_KEY_R: return KEY_R;
        case WLX_KEY_S: return KEY_S;
        case WLX_KEY_T: return KEY_T;
        case WLX_KEY_U: return KEY_U;
        case WLX_KEY_V: return KEY_V;
        case WLX_KEY_W: return KEY_W;
        case WLX_KEY_X: return KEY_X;
        case WLX_KEY_Y: return KEY_Y;
        case WLX_KEY_Z: return KEY_Z;
        case WLX_KEY_0: return KEY_ZERO;
        case WLX_KEY_1: return KEY_ONE;
        case WLX_KEY_2: return KEY_TWO;
        case WLX_KEY_3: return KEY_THREE;
        case WLX_KEY_4: return KEY_FOUR;
        case WLX_KEY_5: return KEY_FIVE;
        case WLX_KEY_6: return KEY_SIX;
        case WLX_KEY_7: return KEY_SEVEN;
        case WLX_KEY_8: return KEY_EIGHT;
        case WLX_KEY_9: return KEY_NINE;
        case WLX_KEY_DELETE: return KEY_DELETE;
        case WLX_KEY_HOME: return KEY_HOME;
        case WLX_KEY_END: return KEY_END;
        case WLX_KEY_PAGE_UP: return KEY_PAGE_UP;
        case WLX_KEY_PAGE_DOWN: return KEY_PAGE_DOWN;
        case WLX_KEY_F1: return KEY_F1;
        case WLX_KEY_F2: return KEY_F2;
        case WLX_KEY_F3: return KEY_F3;
        case WLX_KEY_F4: return KEY_F4;
        case WLX_KEY_F5: return KEY_F5;
        case WLX_KEY_F6: return KEY_F6;
        case WLX_KEY_F7: return KEY_F7;
        case WLX_KEY_F8: return KEY_F8;
        case WLX_KEY_F9: return KEY_F9;
        case WLX_KEY_F10: return KEY_F10;
        case WLX_KEY_F11: return KEY_F11;
        case WLX_KEY_F12: return KEY_F12;
        case WLX_KEY_INSERT: return KEY_INSERT;
        case WLX_KEY_COUNT: return 0;
    }
    return 0;
}

static inline void wlx_process_raylib_input(WLX_Context *ctx) {
    static bool prev_mouse_down = false;
    static bool prev_right_down = false;
    static bool prev_middle_down = false;

    Vector2 mouse_pos = GetMousePosition();
    ctx->input.mouse_x = (int)mouse_pos.x;
    ctx->input.mouse_y = (int)mouse_pos.y;
    ctx->input.mouse_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    ctx->input.mouse_clicked = ctx->input.mouse_down && !prev_mouse_down;
    ctx->input.mouse_held = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    ctx->input.mouse_right_down = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    ctx->input.mouse_right_clicked = ctx->input.mouse_right_down && !prev_right_down;
    ctx->input.mouse_middle_down = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
    ctx->input.mouse_middle_clicked = ctx->input.mouse_middle_down && !prev_middle_down;

    // Raw float detents on both axes: the input contract forbids adapter-side
    // quantizing or debouncing, so precision trackpads keep their fractions.
    Vector2 wheel = GetMouseWheelMoveV();
    ctx->input.wheel_delta = wheel.y;
    ctx->input.wheel_delta_x = wheel.x;

    prev_mouse_down = ctx->input.mouse_down;
    prev_right_down = ctx->input.mouse_right_down;
    prev_middle_down = ctx->input.mouse_middle_down;

    wlx_zero_struct(ctx->input.keys_pressed);
    wlx_zero_struct(ctx->input.keys_repeated);
    for (int key = 1; key < WLX_KEY_COUNT; key++) {
        int raylib_key = wlx_raylib_key_for((WLX_Key_Code)key);
        if (raylib_key == 0) continue;
        bool is_down = IsKeyDown(raylib_key);
        bool was_down = ctx->input.keys_down[key];
        ctx->input.keys_down[key] = is_down;
        if (is_down && !was_down) {
            ctx->input.keys_pressed[key] = true;
        }
        if (IsKeyPressedRepeat(raylib_key)) {
            ctx->input.keys_repeated[key] = true;
        }
    }

    ctx->input.modifiers = 0;
    if (IsKeyDown(KEY_LEFT_SHIFT)   || IsKeyDown(KEY_RIGHT_SHIFT))   ctx->input.modifiers |= WLX_MOD_SHIFT;
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) ctx->input.modifiers |= WLX_MOD_CTRL;
    if (IsKeyDown(KEY_LEFT_ALT)     || IsKeyDown(KEY_RIGHT_ALT))     ctx->input.modifiers |= WLX_MOD_ALT;
    if (IsKeyDown(KEY_LEFT_SUPER)   || IsKeyDown(KEY_RIGHT_SUPER))   ctx->input.modifiers |= WLX_MOD_SUPER;

    wlx_zero_struct(ctx->input.text_input);
    int key = GetCharPressed();
    int text_len = 0;
    while (key > 0) {
        if (key >= 32) {
            char encoded[4];
            size_t enc_len = wlx_utf8_encode((uint32_t)key, encoded);
            if (text_len + (int)enc_len < (int)sizeof(ctx->input.text_input)) {
                memcpy(&ctx->input.text_input[text_len], encoded, enc_len);
                text_len += (int)enc_len;
            } else {
                break;  // buffer full
            }
        }
        key = GetCharPressed();
    }
}

static inline void wlx_raylib_draw_texture(WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint, void *user) {
    WLX_UNUSED(user);
    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(draw_texture_calls);
    DrawTexturePro(
        texture,
        (Rectangle){src.x, src.y, src.w, src.h},
        (Rectangle){dst.x, dst.y, dst.w, dst.h},
        (Vector2){0, 0},
        0.0f,
        tint
    );
    WLX_RAYLIB_SCOPE_END(texture_ns);
}

static inline void wlx_raylib_draw_rect(WLX_Rect rect, WLX_Color color, void *user) {
    WLX_UNUSED(user);
    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(draw_rect_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawRectangleRec((Rectangle){rect.x, rect.y, rect.w, rect.h}, color);
    WLX_RAYLIB_SCOPE_END(geometry_ns);
}

static inline void wlx_raylib_draw_rect_lines(WLX_Rect rect, float thick, WLX_Color color, void *user) {
    WLX_UNUSED(user);
    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(draw_rect_lines_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawRectangleLinesEx((Rectangle){rect.x, rect.y, rect.w, rect.h}, thick, color);
    WLX_RAYLIB_SCOPE_END(geometry_ns);
}

static inline void wlx_raylib_draw_rect_rounded(WLX_Rect rect, float roundness, int segments, WLX_Color color, void *user) {
    WLX_UNUSED(user);
    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(draw_rect_rounded_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawRectangleRounded((Rectangle){rect.x, rect.y, rect.w, rect.h}, roundness, segments, color);
    WLX_RAYLIB_SCOPE_END(geometry_ns);
}

static inline void wlx_raylib_draw_rect_rounded_lines(WLX_Rect rect, float roundness, int segments, float thick, WLX_Color color, void *user) {
    WLX_UNUSED(user);
    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(draw_rect_rounded_lines_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawRectangleRoundedLinesEx((Rectangle){rect.x, rect.y, rect.w, rect.h}, roundness, segments, thick, color);
    WLX_RAYLIB_SCOPE_END(geometry_ns);
}

// Native vertical two-stop gradient. Sharp rects use raylib's float-precise
// DrawRectangleGradientEx (top stops on the top corners, bottom stops on the
// bottom corners). Raylib has no rounded gradient, so rounded rects fall back to
// stacked rounded bands (the same approximation the core software fallback uses).
static inline void wlx_raylib_draw_gradient_v(WLX_Rect rect, WLX_Color top, WLX_Color bottom,
                                              float roundness, int rounded_segs, void *user) {
    WLX_UNUSED(user);
    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    if (roundness <= 0.0f) {
        DrawRectangleGradientEx((Rectangle){rect.x, rect.y, rect.w, rect.h},
                                top, bottom, bottom, top);
    } else {
        int bands = (int)(rect.h / WLX_GRADIENT_FALLBACK_BAND_HEIGHT_PX);
        if (bands < 1) bands = 1;
        float band_h = rect.h / (float)bands;
        for (int i = 0; i < bands; i++) {
            float t = (bands == 1) ? 0.0f : (float)i / (float)(bands - 1);
            WLX_Color c = wlx_color_lerp(top, bottom, t);
            DrawRectangleRounded((Rectangle){rect.x, rect.y + (float)i * band_h,
                                             rect.w, band_h + 1.0f}, roundness, rounded_segs, c);
        }
    }
    WLX_RAYLIB_SCOPE_END(geometry_ns);
}

static inline void wlx_raylib_draw_circle(float cx, float cy, float radius, int segments, WLX_Color color, void *user) {
    WLX_UNUSED(user);
    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(draw_circle_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawCircleSector((Vector2){cx, cy}, radius, 0, 360, segments, color);
    WLX_RAYLIB_SCOPE_END(geometry_ns);
}

static inline void wlx_raylib_draw_ring(float cx, float cy, float inner_r, float outer_r, int segments, WLX_Color color, void *user) {
    WLX_UNUSED(user);
    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(draw_ring_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawRing((Vector2){cx, cy}, inner_r, outer_r, 0, 360, segments, color);
    WLX_RAYLIB_SCOPE_END(geometry_ns);
}

static inline void wlx_raylib_draw_line(float x1, float y1, float x2, float y2, float thick, WLX_Color color, void *user) {
    WLX_UNUSED(user);
    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(draw_line_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, thick, color);
    WLX_RAYLIB_SCOPE_END(geometry_ns);
}

// Effective spacing for a text style. Raylib's default bitmap font stores no
// per-glyph advance (advanceX == 0), so DrawTextEx packs its glyph rects edge
// to edge when spacing is 0. The font is designed for a 1px inter-glyph gap
// at its base size, so the natural spacing is that gap scaled with the glyphs:
// font_size / baseSize as a float, min 1 (raylib's own DrawText approximates
// this with integer fontSize/10, which under-spaces sizes between multiples
// of 10; the float form matches it exactly at multiples of 10 and keeps the
// gap proportional in between). This keeps style.spacing on its core
// contract -- 0 means natural backend spacing, nonzero is extra tracking on
// top. Loaded fonts carry real advances, so their natural spacing stays 0.
// The result is a pure function of (font, font_size, spacing), all of which
// are already in the measurement-cache key, so cached sizes stay keyed
// correctly without storing the derived value.
static inline float wlx_raylib_effective_spacing(WLX_Text_Style style) {
    float spacing = (float)style.spacing;
    if (style.font == WLX_FONT_DEFAULT) {
        int base = GetFontDefault().baseSize;
        float natural = (base > 0) ? (float)style.font_size / (float)base : 1.0f;
        if (natural < 1.0f) natural = 1.0f;
        spacing += natural;
    }
    return spacing;
}

static inline void wlx_raylib_draw_text(const char *text, float x, float y, WLX_Text_Style style, void *user) {
    WLX_UNUSED(user);
    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(draw_text_calls);
    Font font = (style.font != WLX_FONT_DEFAULT)
              ? *(Font *)(uintptr_t)style.font
              : GetFontDefault();
    DrawTextEx(font, text, (Vector2){x, y}, (float)style.font_size,
               wlx_raylib_effective_spacing(style),
               (Color){style.color.r, style.color.g, style.color.b, style.color.a});
    WLX_RAYLIB_SCOPE_END(text_draw_ns);
}

// Probe the open-addressed measurement cache for the given key. On hit,
// writes (w, h) and returns true. On miss, picks the displacement target
// for the next store, increments text_cache_misses (and text_cache_evictions
// if displacing a live entry), and returns false. On every call increments
// text_cache_lookups. When CAP=0 the helper is a forced miss with no counter
// activity. The pending slot index is stashed in a backend-local global,
// consumed by the next wlx_raylib_text_cache_store call.
static inline bool wlx_raylib_text_cache_lookup(uintptr_t font_handle,
        WLX_Text_Style style, size_t len, uint64_t text_hash,
        float *out_w, float *out_h) {
#if WLX_RAYLIB_TEXT_CACHE_CAP > 0
    WLX_RAYLIB_PERF_INC(text_cache_lookups);

    uint32_t font_size_bits = wlx_raylib_text_cache_float_bits((float)style.font_size);
    uint32_t spacing_bits   = wlx_raylib_text_cache_float_bits((float)style.spacing);
    uint32_t generation     = g_wlx_raylib_text_cache_generation;
    size_t   mask  = (size_t)WLX_RAYLIB_TEXT_CACHE_SLOTS - 1u;
    size_t   start = (size_t)(text_hash & (uint64_t)mask);

    size_t target = start;
    bool   target_is_stale = false;

    for (size_t step = 0; step < WLX_RAYLIB_TEXT_CACHE_PROBE_LIMIT; ++step) {
        size_t slot = (start + step) & mask;
        WLX_Raylib_Text_Cache_Entry *e = &g_wlx_raylib_text_cache[slot];
        bool occupied = (e->generation == generation);

        if (occupied) {
            if (e->font_handle == font_handle
                    && e->font_size_bits == font_size_bits
                    && e->spacing_bits == spacing_bits
                    && e->text_len == (uint32_t)len
                    && e->text_hash == text_hash) {
                WLX_RAYLIB_PERF_INC(text_cache_hits);
                *out_w = e->w;
                *out_h = e->h;
                g_wlx_raylib_text_cache_pending_valid = false;
                return true;
            }
            // (font, size, spacing, len) match but hash differs -> probe-window
            // collision among same-shape entries. Counts the case where the
            // probe walk finds a same-length entry for the same effective
            // text style; any nonzero count signals hash distribution issues
            // worth investigating.
            if (e->font_handle == font_handle
                    && e->font_size_bits == font_size_bits
                    && e->spacing_bits == spacing_bits
                    && e->text_len == (uint32_t)len) {
                WLX_RAYLIB_PERF_INC(text_cache_collision_rejections);
            }
        } else if (!target_is_stale) {
            // First stale slot in probe window: prefer it as displacement
            // target so live entries near the start slot survive.
            target = slot;
            target_is_stale = true;
        }
    }

    WLX_RAYLIB_PERF_INC(text_cache_misses);
    if (!target_is_stale) {
        // Probe window is full of live entries with different keys: displace
        // the slot at the natural hash position.
        target = start;
        WLX_RAYLIB_PERF_INC(text_cache_evictions);
    }
    g_wlx_raylib_text_cache_pending_slot  = target;
    g_wlx_raylib_text_cache_pending_valid = true;
    return false;
#else
    (void)font_handle; (void)style; (void)len; (void)text_hash;
    (void)out_w; (void)out_h;
    return false;
#endif
}

// Write a freshly measured (w, h) into the slot chosen by the most recent
// lookup miss. No-op when CAP=0 or when no pending miss exists.
static inline void wlx_raylib_text_cache_store(uintptr_t font_handle,
        WLX_Text_Style style, size_t len, uint64_t text_hash,
        float w, float h) {
#if WLX_RAYLIB_TEXT_CACHE_CAP > 0
    if (!g_wlx_raylib_text_cache_pending_valid) return;
    WLX_Raylib_Text_Cache_Entry *e =
        &g_wlx_raylib_text_cache[g_wlx_raylib_text_cache_pending_slot];
    e->font_handle    = font_handle;
    e->font_size_bits = wlx_raylib_text_cache_float_bits((float)style.font_size);
    e->spacing_bits   = wlx_raylib_text_cache_float_bits((float)style.spacing);
    e->text_len       = (uint32_t)len;
    e->text_hash      = text_hash;
    e->w              = w;
    e->h              = h;
    e->generation     = g_wlx_raylib_text_cache_generation;
    g_wlx_raylib_text_cache_pending_valid = false;
#else
    (void)font_handle; (void)style; (void)len; (void)text_hash; (void)w; (void)h;
#endif
}

// Slice draw: DrawTextEx needs a C string either way, so the copy the core
// fallback used to make now lives here; measurement-cache behavior is
// untouched (draws are uncached).
static inline void wlx_raylib_draw_text_slice(const char *text, size_t len,
        float x, float y, WLX_Text_Style style, void *user) {
    WLX_UNUSED(user);
    WLX_CStr_Tmp tmp;
    const char *cstr = wlx_cstr_tmp_begin(&tmp, text, len);
    if (cstr == NULL) return;
    wlx_raylib_draw_text(cstr, x, y, style, user);
    wlx_cstr_tmp_end(&tmp);
}

static inline void wlx_raylib_measure_text(const char *text, WLX_Text_Style style, float *out_w, float *out_h, void *user) {
    WLX_UNUSED(user);
    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(measure_text_calls);

    size_t   len  = (text != NULL) ? strlen(text) : 0;
    uint64_t hash = wlx_hash_fnv1a64(text != NULL ? text : "", len);

    if (wlx_raylib_text_cache_lookup((uintptr_t)style.font, style, len, hash,
                                     out_w, out_h)) {
        WLX_RAYLIB_SCOPE_END(text_measure_ns);
        return;
    }

    Font font = (style.font != WLX_FONT_DEFAULT)
              ? *(Font *)(uintptr_t)style.font
              : GetFontDefault();
    Vector2 size = MeasureTextEx(font, text != NULL ? text : "",
                                 (float)style.font_size, wlx_raylib_effective_spacing(style));
    *out_w = size.x;
    *out_h = size.y;
    wlx_raylib_text_cache_store((uintptr_t)style.font, style, len, hash,
                                *out_w, *out_h);
    WLX_RAYLIB_SCOPE_END(text_measure_ns);
}

// Slice-aware measure: accepts explicit byte length so wlx_span_measure_text
// can avoid the temporary null-terminated copy on internal measurement.
// MeasureTextEx requires a C-string, so the slice is always copied through
// the core's WLX_CStr_Tmp (stack buffer, heap for long slices): probing
// text[len] for an existing terminator would read one byte past the slice,
// and spans may end exactly at the end of an allocation.
// Cache lookup happens before the copy so cache hits skip the copy work.
static inline void wlx_raylib_measure_text_slice(const char *text, size_t slice_len,
        WLX_Text_Style style, float *out_w, float *out_h, void *user) {
    WLX_UNUSED(user);
    if (out_w == NULL || out_h == NULL) return;
    if (text == NULL) { text = ""; slice_len = 0; }

    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(measure_text_calls);

    uint64_t hash = wlx_hash_fnv1a64(text, slice_len);
    if (wlx_raylib_text_cache_lookup((uintptr_t)style.font, style, slice_len,
                                     hash, out_w, out_h)) {
        WLX_RAYLIB_SCOPE_END(text_measure_ns);
        return;
    }

    Font font = (style.font != WLX_FONT_DEFAULT)
              ? *(Font *)(uintptr_t)style.font
              : GetFontDefault();

    WLX_CStr_Tmp tmp;
    const char *measure_text = wlx_cstr_tmp_begin(&tmp, text, slice_len);
    if (measure_text == NULL) {
        *out_w = 0.0f;
        *out_h = 0.0f;
        WLX_RAYLIB_SCOPE_END(text_measure_ns);
        return;
    }

    Vector2 size = MeasureTextEx(font, measure_text, (float)style.font_size,
                                 wlx_raylib_effective_spacing(style));
    *out_w = size.x;
    *out_h = size.y;
    wlx_raylib_text_cache_store((uintptr_t)style.font, style, slice_len, hash,
                                *out_w, *out_h);

    wlx_cstr_tmp_end(&tmp);
    WLX_RAYLIB_SCOPE_END(text_measure_ns);
}

// Cumulative glyph advances of one run at each requested unit end,
// accumulated exactly as MeasureTextEx does for a single line (per-glyph
// advanceX at base size, one scale-factor multiply, plus
// (codepoints - 1) * spacing), so callback-built geometry matches the
// whole-prefix slice measures bit-for-bit on Raylib's additive model.
// Codepoint decoding needs NUL-terminated input, so every slice copies
// through the core's WLX_CStr_Tmp like the slice measure (probing text[len]
// for an existing terminator would read one byte past the slice).
static inline size_t wlx_raylib_measure_text_advances(const char *text, size_t len,
        WLX_Text_Style style, const size_t *unit_ends, size_t unit_count,
        float *out_advances, void *user) {
    WLX_UNUSED(user);
    if (text == NULL || unit_ends == NULL || out_advances == NULL || unit_count == 0)
        return 0;

    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(measure_text_calls);

    Font font = (style.font != WLX_FONT_DEFAULT)
              ? *(Font *)(uintptr_t)style.font
              : GetFontDefault();
    size_t filled = 0;
    WLX_CStr_Tmp tmp;
    const char *walk_text = NULL;

    if (font.glyphs != NULL && font.baseSize > 0) {
        walk_text = wlx_cstr_tmp_begin(&tmp, text, len);  // NULL on alloc failure
    }

    if (walk_text != NULL) {
        float scale = style.font_size / (float)font.baseSize;
        float eff_spacing = wlx_raylib_effective_spacing(style);
        float raw_w = 0.0f;
        int cp_count = 0;
        size_t i = 0;
        for (size_t u = 0; u < unit_count; u++) {
            while (i < len && i < unit_ends[u]) {
                int cp_bytes = 0;
                int letter = GetCodepointNext(&walk_text[i], &cp_bytes);
                if (cp_bytes <= 0) cp_bytes = 1;
                int index = GetGlyphIndex(font, letter);
                if (font.glyphs[index].advanceX > 0) raw_w += (float)font.glyphs[index].advanceX;
                else raw_w += font.recs[index].width + (float)font.glyphs[index].offsetX;
                cp_count++;
                i += (size_t)cp_bytes;
            }
            out_advances[u] = cp_count > 0
                ? raw_w * scale + (float)(cp_count - 1) * eff_spacing
                : 0.0f;
        }
        filled = unit_count;
    }

    if (walk_text != NULL) wlx_cstr_tmp_end(&tmp);
    WLX_RAYLIB_SCOPE_END(text_measure_ns);
    return filled;
}

// Flush backend-owned Raylib measurement-cache state. O(1) invalidation via
// generation bump: every entry stores its generation at write time and the
// lookup helper compares against the current generation. No table-wide scan
// needed; the next store into a stale slot displaces it transparently.
// Auto-called from wlx_context_init_raylib. Callers must invoke this before
// UnloadFont() on any Raylib Font passed to the backend (ADR 013).
static inline void wlx_raylib_text_cache_clear(void) {
#if WLX_RAYLIB_TEXT_CACHE_CAP > 0
    g_wlx_raylib_text_cache_generation++;
    if (g_wlx_raylib_text_cache_generation == 0) {
        // 32-bit generation wrap: 0 is reserved for "stale/empty", so on
        // wraparound we must scan the table once to clear stored generations.
        // In practice this happens only after 4 billion clears.
        for (size_t i = 0; i < WLX_RAYLIB_TEXT_CACHE_SLOTS; ++i) {
            g_wlx_raylib_text_cache[i].generation = 0;
        }
        g_wlx_raylib_text_cache_generation = 1;
    }
    g_wlx_raylib_text_cache_pending_valid = false;
#endif
#ifdef WLX_PERF
    g_wlx_perf_raylib_state.current.text_cache_lookups = 0;
    g_wlx_perf_raylib_state.current.text_cache_hits = 0;
    g_wlx_perf_raylib_state.current.text_cache_misses = 0;
    g_wlx_perf_raylib_state.current.text_cache_evictions = 0;
    g_wlx_perf_raylib_state.current.text_cache_collision_rejections = 0;
    g_wlx_perf_raylib_state.last.text_cache_lookups = 0;
    g_wlx_perf_raylib_state.last.text_cache_hits = 0;
    g_wlx_perf_raylib_state.last.text_cache_misses = 0;
    g_wlx_perf_raylib_state.last.text_cache_evictions = 0;
    g_wlx_perf_raylib_state.last.text_cache_collision_rejections = 0;
#endif
}

static inline void wlx_raylib_begin_scissor(WLX_Rect rect, void *user) {
    WLX_UNUSED(user);
    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(begin_scissor_calls);
    WLX_RAYLIB_PERF_INC(clip_change_calls);
    BeginScissorMode((int)rect.x, (int)rect.y, (int)rect.w, (int)rect.h);
    WLX_RAYLIB_SCOPE_END(scissor_ns);
}

static inline void wlx_raylib_end_scissor(void *user) {
    WLX_UNUSED(user);
    WLX_RAYLIB_SCOPE_BEGIN();
    WLX_RAYLIB_PERF_INC(end_scissor_calls);
    WLX_RAYLIB_PERF_INC(clip_change_calls);
    EndScissorMode();
    WLX_RAYLIB_SCOPE_END(scissor_ns);
}

static inline float wlx_raylib_get_frame_time(void *user) {
    WLX_UNUSED(user);
    return GetFrameTime();
}

static inline const char *wlx_raylib_clipboard_get(void *user) {
    WLX_UNUSED(user);
    return GetClipboardText();
}

// Upper bound for the clipboard transport buffer; spans beyond it are
// truncated at a UTF-8 boundary. Overridable before include.
#ifndef WLX_RAYLIB_CLIPBOARD_MAX
#define WLX_RAYLIB_CLIPBOARD_MAX (16u * 1024u * 1024u)
#endif

static inline void wlx_raylib_clipboard_set(const char *text, size_t len, void *user) {
    WLX_UNUSED(user);
    // SetClipboardText needs a NUL-terminated string; copy the span into a
    // core wlx_buf_reserve grow-and-reuse buffer (process lifetime) so
    // arbitrarily long spans survive, bounded only by the soft cap.
    static char *buf = NULL;
    static size_t cap = 0;
    if (text == NULL) return;
    if (len > WLX_RAYLIB_CLIPBOARD_MAX - 1) {
        len = WLX_RAYLIB_CLIPBOARD_MAX - 1;
        // Never split a UTF-8 sequence at the cap (text[len] stays inside
        // the original span here, since the original length exceeds it).
        len = wlx_utf8_floor(text, len);
    }
    if (!wlx_buf_reserve(&buf, &cap, len + 1)) return;
    memcpy(buf, text, len);
    buf[len] = '\0';
    SetClipboardText(buf);
}

// The core calls this only when the resolved shape changes. The enum is
// append-only; this build check flags a new shape this mapping ignores.
_Static_assert(WLX_CURSOR_COUNT == 2, "update wlx_raylib_set_cursor for the new WLX_Cursor_Shape");
static inline void wlx_raylib_set_cursor(WLX_Cursor_Shape shape, void *user) {
    WLX_UNUSED(user);
    SetMouseCursor(shape == WLX_CURSOR_IBEAM ? MOUSE_CURSOR_IBEAM : MOUSE_CURSOR_DEFAULT);
}

static inline WLX_Backend wlx_backend_raylib(void) {
    return (WLX_Backend){
        .contract_version = WLX_BACKEND_CONTRACT_VERSION,
        .user = NULL,
        .draw_rect = wlx_raylib_draw_rect,
        .draw_rect_lines = wlx_raylib_draw_rect_lines,
        .draw_rect_rounded = wlx_raylib_draw_rect_rounded,
        .draw_rect_rounded_lines = wlx_raylib_draw_rect_rounded_lines,
        .draw_circle = wlx_raylib_draw_circle,
        .draw_ring = wlx_raylib_draw_ring,
        .draw_gradient_v = wlx_raylib_draw_gradient_v,
        .draw_line = wlx_raylib_draw_line,
        .draw_text = wlx_raylib_draw_text,
        .draw_text_slice = wlx_raylib_draw_text_slice,
        .measure_text = wlx_raylib_measure_text,
        .measure_text_slice = wlx_raylib_measure_text_slice,
        .measure_text_advances = wlx_raylib_measure_text_advances,
        .draw_texture = wlx_raylib_draw_texture,
        .begin_scissor = wlx_raylib_begin_scissor,
        .end_scissor = wlx_raylib_end_scissor,
        .get_frame_time = wlx_raylib_get_frame_time,
        .clipboard_get = wlx_raylib_clipboard_get,
        .clipboard_set = wlx_raylib_clipboard_set,
        .set_cursor = wlx_raylib_set_cursor,
    };
}

static inline WLX_Font wlx_font_from_raylib(Font *font) {
    return (WLX_Font)(uintptr_t)font;
}

static inline void wlx_context_init_raylib(WLX_Context *ctx) {
    assert(ctx != NULL);
    ctx->backend = wlx_backend_raylib();
    wlx_raylib_text_cache_clear();
#ifdef WLX_PERF
    wlx_perf_raylib_install_timer(ctx);
#endif
}

#undef WLX_RAYLIB_PERF_INC

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WOLLIX_RAYLIB_H_
