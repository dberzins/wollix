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

// Capacity of the Raylib backend measurement cache (default 2048).
// Originally suggested 256 from the analysis estimate of 100-300
// unique (font, size, spacing, text) tuples per frame; Validation
// captured ~1700 unique tuples on the warmed Theme Lab Dark workload at
// 1920x1080, mirroring the working-set finding that drove SDL3's default
// cap from 256 to 1024. The Raylib default has therefore been raised to
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
// factor at full CAP stays below 0.6. For default CAP=256 this resolves to
// 512 slots (load factor max ~0.5). Hardcoded list covers caps up to 65536.
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

// FNV-1a 64-bit hash over the byte range [text, text+len). Cache key uses
// (font_handle, font_size_bits, spacing_bits, text_len, text_hash) without
// storing the original bytes; hash collisions therefore manifest as
// false-positive hits and are tracked by text_cache_collision_rejections.
static inline uint64_t wlx_raylib_text_cache_hash(const char *text, size_t len) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; ++i) {
        h ^= (uint64_t)(unsigned char)text[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

static inline uint32_t wlx_raylib_text_cache_float_bits(float v) {
    union { float f; uint32_t u; } pun;
    pun.f = v;
    return pun.u;
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
    uint64_t text_cache_lookups;
    uint64_t text_cache_hits;
    uint64_t text_cache_misses;
    uint64_t text_cache_evictions;
    uint64_t text_cache_collision_rejections;
} WLX_Perf_Raylib_Frame;

typedef struct {
    WLX_Perf_Raylib_Frame current;
    WLX_Perf_Raylib_Frame last;
    uint64_t present_start_ns;
    bool capturing;
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
    wlx_zero_struct(g_wlx_perf_raylib_state.current);
    g_wlx_perf_raylib_state.current.frame_index = frame_index;
    g_wlx_perf_raylib_state.current.timer_available = true;
    g_wlx_perf_raylib_state.present_start_ns = 0;
    g_wlx_perf_raylib_state.capturing = true;
}

static inline void wlx_perf_raylib_end_frame(uint64_t frame_index) {
    if (!g_wlx_perf_raylib_state.capturing) return;
    if (frame_index != 0) g_wlx_perf_raylib_state.current.frame_index = frame_index;
    g_wlx_perf_raylib_state.last = g_wlx_perf_raylib_state.current;
    g_wlx_perf_raylib_state.capturing = false;
    g_wlx_perf_raylib_state.present_start_ns = 0;
}

static inline void wlx_perf_raylib_reset(void) {
    wlx_zero_struct(g_wlx_perf_raylib_state);
}

static inline const WLX_Perf_Raylib_Frame *wlx_perf_raylib_get_last_frame(void) {
    return &g_wlx_perf_raylib_state.last;
}

static inline void wlx_perf_raylib_inc(uint64_t *counter) {
    if (!g_wlx_perf_raylib_state.capturing) return;
    (*counter)++;
}

static inline uint64_t wlx_perf_raylib_time_begin(void) {
    if (!g_wlx_perf_raylib_state.capturing || !g_wlx_perf_raylib_state.current.timer_available) return 0;
    return wlx_perf_raylib_timestamp(NULL);
}

static inline void wlx_perf_raylib_time_end(uint64_t start_ns, uint64_t *total_ns) {
    uint64_t end_ns;

    if (!g_wlx_perf_raylib_state.capturing || !g_wlx_perf_raylib_state.current.timer_available) return;
    end_ns = wlx_perf_raylib_timestamp(NULL);
    if (end_ns >= start_ns) *total_ns += end_ns - start_ns;
}

static inline void wlx_perf_raylib_present_begin(void) {
    g_wlx_perf_raylib_state.present_start_ns = wlx_perf_raylib_time_begin();
}

static inline void wlx_perf_raylib_present_end(void) {
    wlx_perf_raylib_time_end(g_wlx_perf_raylib_state.present_start_ns,
        &g_wlx_perf_raylib_state.current.present_ns);
    g_wlx_perf_raylib_state.present_start_ns = 0;
}

#define WLX_RAYLIB_PERF_INC(field) \
    wlx_perf_raylib_inc(&g_wlx_perf_raylib_state.current.field)
#else
#define WLX_RAYLIB_PERF_INC(field) ((void)0)
#endif

static inline WLX_Key_Code wlx_raylib_map_key(int raylib_key) {
    switch (raylib_key) {
        case KEY_ESCAPE: return WLX_KEY_ESCAPE;
        case KEY_ENTER: return WLX_KEY_ENTER;
        case KEY_BACKSPACE: return WLX_KEY_BACKSPACE;
        case KEY_TAB: return WLX_KEY_TAB;
        case KEY_SPACE: return WLX_KEY_SPACE;
        case KEY_LEFT: return WLX_KEY_LEFT;
        case KEY_RIGHT: return WLX_KEY_RIGHT;
        case KEY_UP: return WLX_KEY_UP;
        case KEY_DOWN: return WLX_KEY_DOWN;
        case KEY_A: return WLX_KEY_A;
        case KEY_B: return WLX_KEY_B;
        case KEY_C: return WLX_KEY_C;
        case KEY_D: return WLX_KEY_D;
        case KEY_E: return WLX_KEY_E;
        case KEY_F: return WLX_KEY_F;
        case KEY_G: return WLX_KEY_G;
        case KEY_H: return WLX_KEY_H;
        case KEY_I: return WLX_KEY_I;
        case KEY_J: return WLX_KEY_J;
        case KEY_K: return WLX_KEY_K;
        case KEY_L: return WLX_KEY_L;
        case KEY_M: return WLX_KEY_M;
        case KEY_N: return WLX_KEY_N;
        case KEY_O: return WLX_KEY_O;
        case KEY_P: return WLX_KEY_P;
        case KEY_Q: return WLX_KEY_Q;
        case KEY_R: return WLX_KEY_R;
        case KEY_S: return WLX_KEY_S;
        case KEY_T: return WLX_KEY_T;
        case KEY_U: return WLX_KEY_U;
        case KEY_V: return WLX_KEY_V;
        case KEY_W: return WLX_KEY_W;
        case KEY_X: return WLX_KEY_X;
        case KEY_Y: return WLX_KEY_Y;
        case KEY_Z: return WLX_KEY_Z;
        case KEY_ZERO: return WLX_KEY_0;
        case KEY_ONE: return WLX_KEY_1;
        case KEY_TWO: return WLX_KEY_2;
        case KEY_THREE: return WLX_KEY_3;
        case KEY_FOUR: return WLX_KEY_4;
        case KEY_FIVE: return WLX_KEY_5;
        case KEY_SIX: return WLX_KEY_6;
        case KEY_SEVEN: return WLX_KEY_7;
        case KEY_EIGHT: return WLX_KEY_8;
        case KEY_NINE: return WLX_KEY_9;
        default: return WLX_KEY_NONE;
    }
}

static inline void wlx_process_raylib_input(WLX_Context *ctx) {
    static bool prev_mouse_down = false;
    static float prev_wheel_dir = 0.0f;   // last non-zero wheel direction (+1 or -1)
    static bool  prev_was_zero  = true;    // was previous frame's raw wheel zero?

    Vector2 mouse_pos = GetMousePosition();
    ctx->input.mouse_x = mouse_pos.x;
    ctx->input.mouse_y = mouse_pos.y;
    ctx->input.mouse_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    ctx->input.mouse_clicked = ctx->input.mouse_down && !prev_mouse_down;
    ctx->input.mouse_held = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    // Debounce mouse wheel encoder bounce: if the direction suddenly
    // reverses for a single frame mid-scroll, suppress the spurious tick.
    float raw_wheel = GetMouseWheelMove();
    if (raw_wheel != 0.0f) {
        float dir = (raw_wheel > 0.0f) ? 1.0f : -1.0f;
        if (prev_wheel_dir != 0.0f && dir != prev_wheel_dir && !prev_was_zero) {
            // Single-frame reversal mid-scroll -> likely encoder bounce, suppress
            ctx->input.wheel_delta = 0.0f;
            // Don't update prev_wheel_dir - keep the "real" direction
        } else {
            ctx->input.wheel_delta = raw_wheel;
            prev_wheel_dir = dir;
        }
        prev_was_zero = false;
    } else {
        ctx->input.wheel_delta = 0.0f;
        prev_was_zero = true;
    }
    prev_mouse_down = ctx->input.mouse_down;

    wlx_zero_struct(ctx->input.keys_pressed);
    for (int raylib_key = 0; raylib_key < 350; raylib_key++) {
        WLX_Key_Code key = wlx_raylib_map_key(raylib_key);
        if (key != WLX_KEY_NONE) {
            bool is_down = IsKeyDown(raylib_key);
            bool was_down = ctx->input.keys_down[key];
            ctx->input.keys_down[key] = is_down;
            if (is_down && !was_down) {
                ctx->input.keys_pressed[key] = true;
            }
        }
    }

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

static inline void wlx_raylib_draw_texture(WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
    WLX_RAYLIB_PERF_INC(draw_texture_calls);
    DrawTexturePro(
        texture,
        (Rectangle){src.x, src.y, src.w, src.h},
        (Rectangle){dst.x, dst.y, dst.w, dst.h},
        (Vector2){0, 0},
        0.0f,
        tint
    );
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.texture_ns);
#endif
}

static inline void wlx_raylib_draw_rect(WLX_Rect rect, WLX_Color color) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
    WLX_RAYLIB_PERF_INC(draw_rect_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawRectangleRec((Rectangle){rect.x, rect.y, rect.w, rect.h}, color);
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.geometry_ns);
#endif
}

static inline void wlx_raylib_draw_rect_lines(WLX_Rect rect, float thick, WLX_Color color) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
    WLX_RAYLIB_PERF_INC(draw_rect_lines_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawRectangleLinesEx((Rectangle){rect.x, rect.y, rect.w, rect.h}, thick, color);
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.geometry_ns);
#endif
}

static inline void wlx_raylib_draw_rect_rounded(WLX_Rect rect, float roundness, int segments, WLX_Color color) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
    WLX_RAYLIB_PERF_INC(draw_rect_rounded_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawRectangleRounded((Rectangle){rect.x, rect.y, rect.w, rect.h}, roundness, segments, color);
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.geometry_ns);
#endif
}

static inline void wlx_raylib_draw_rect_rounded_lines(WLX_Rect rect, float roundness, int segments, float thick, WLX_Color color) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
    WLX_RAYLIB_PERF_INC(draw_rect_rounded_lines_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawRectangleRoundedLinesEx((Rectangle){rect.x, rect.y, rect.w, rect.h}, roundness, segments, thick, color);
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.geometry_ns);
#endif
}

// Native vertical two-stop gradient. Sharp rects use raylib's float-precise
// DrawRectangleGradientEx (top stops on the top corners, bottom stops on the
// bottom corners). Raylib has no rounded gradient, so rounded rects fall back to
// stacked rounded bands (the same approximation the core software fallback uses).
static inline void wlx_raylib_draw_gradient_v(WLX_Rect rect, WLX_Color top, WLX_Color bottom,
                                              float roundness, int rounded_segs) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
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
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.geometry_ns);
#endif
}

static inline void wlx_raylib_draw_circle(float cx, float cy, float radius, int segments, WLX_Color color) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
    WLX_RAYLIB_PERF_INC(draw_circle_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawCircleSector((Vector2){cx, cy}, radius, 0, 360, segments, color);
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.geometry_ns);
#endif
}

static inline void wlx_raylib_draw_ring(float cx, float cy, float inner_r, float outer_r, int segments, WLX_Color color) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
    WLX_RAYLIB_PERF_INC(draw_ring_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawRing((Vector2){cx, cy}, inner_r, outer_r, 0, 360, segments, color);
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.geometry_ns);
#endif
}

static inline void wlx_raylib_draw_line(float x1, float y1, float x2, float y2, float thick, WLX_Color color) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
    WLX_RAYLIB_PERF_INC(draw_line_calls);
    WLX_RAYLIB_PERF_INC(geometry_submit_calls);
    DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, thick, color);
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.geometry_ns);
#endif
}

static inline void wlx_raylib_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
    WLX_RAYLIB_PERF_INC(draw_text_calls);
    Font font = (style.font != WLX_FONT_DEFAULT)
              ? *(Font *)(uintptr_t)style.font
              : GetFontDefault();
    DrawTextEx(font, text, (Vector2){x, y}, style.font_size, (float)style.spacing,
               (Color){style.color.r, style.color.g, style.color.b, style.color.a});
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.text_draw_ns);
#endif
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

    uint32_t font_size_bits = wlx_raylib_text_cache_float_bits(style.font_size);
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
    e->font_size_bits = wlx_raylib_text_cache_float_bits(style.font_size);
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

static inline void wlx_raylib_measure_text(const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
    WLX_RAYLIB_PERF_INC(measure_text_calls);

    size_t   len  = (text != NULL) ? strlen(text) : 0;
    uint64_t hash = wlx_raylib_text_cache_hash(text != NULL ? text : "", len);

    if (wlx_raylib_text_cache_lookup((uintptr_t)style.font, style, len, hash,
                                     out_w, out_h)) {
#ifdef WLX_PERF
        wlx_perf_raylib_time_end(perf_start_ns,
            &g_wlx_perf_raylib_state.current.text_measure_ns);
#endif
        return;
    }

    Font font = (style.font != WLX_FONT_DEFAULT)
              ? *(Font *)(uintptr_t)style.font
              : GetFontDefault();
    Vector2 size = MeasureTextEx(font, text != NULL ? text : "",
                                 style.font_size, (float)style.spacing);
    *out_w = size.x;
    *out_h = size.y;
    wlx_raylib_text_cache_store((uintptr_t)style.font, style, len, hash,
                                *out_w, *out_h);
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.text_measure_ns);
#endif
}

// Slice-aware measure: accepts explicit byte length so wlx_span_measure_text
// can avoid the temporary null-terminated copy on internal measurement.
// MeasureTextEx requires a C-string, so we reuse the input directly when the
// byte at text[len] is already 0; otherwise we copy into a small stack buffer
// and fall back to wlx_alloc only for slices longer than the stack buffer.
// Cache lookup happens before any copy so cache hits skip the copy work too.
static inline void wlx_raylib_measure_text_slice(const char *text, size_t slice_len,
        WLX_Text_Style style, float *out_w, float *out_h) {
    if (out_w == NULL || out_h == NULL) return;
    if (text == NULL) { text = ""; slice_len = 0; }

#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
    WLX_RAYLIB_PERF_INC(measure_text_calls);

    uint64_t hash = wlx_raylib_text_cache_hash(text, slice_len);
    if (wlx_raylib_text_cache_lookup((uintptr_t)style.font, style, slice_len,
                                     hash, out_w, out_h)) {
#ifdef WLX_PERF
        wlx_perf_raylib_time_end(perf_start_ns,
            &g_wlx_perf_raylib_state.current.text_measure_ns);
#endif
        return;
    }

    Font font = (style.font != WLX_FONT_DEFAULT)
              ? *(Font *)(uintptr_t)style.font
              : GetFontDefault();

    const char *measure_text = text;
    char stack_buf[256];
    char *heap_buf = NULL;
    if (slice_len > 0 && text[slice_len] != '\0') {
        if (slice_len + 1 <= sizeof(stack_buf)) {
            memcpy(stack_buf, text, slice_len);
            stack_buf[slice_len] = '\0';
            measure_text = stack_buf;
        } else {
            heap_buf = (char *)wlx_alloc(slice_len + 1);
            if (heap_buf == NULL) {
                *out_w = 0.0f;
                *out_h = 0.0f;
#ifdef WLX_PERF
                wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.text_measure_ns);
#endif
                return;
            }
            memcpy(heap_buf, text, slice_len);
            heap_buf[slice_len] = '\0';
            measure_text = heap_buf;
        }
    } else if (slice_len == 0) {
        measure_text = "";
    }

    Vector2 size = MeasureTextEx(font, measure_text, style.font_size, (float)style.spacing);
    *out_w = size.x;
    *out_h = size.y;
    wlx_raylib_text_cache_store((uintptr_t)style.font, style, slice_len, hash,
                                *out_w, *out_h);

    if (heap_buf != NULL) wlx_free(heap_buf);
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.text_measure_ns);
#endif
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

static inline WLX_Font wlx_font_from_raylib(Font *font) {
    return (WLX_Font)(uintptr_t)font;
}

static inline void wlx_raylib_begin_scissor(WLX_Rect rect) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
    WLX_RAYLIB_PERF_INC(begin_scissor_calls);
    WLX_RAYLIB_PERF_INC(clip_change_calls);
    BeginScissorMode(rect.x, rect.y, rect.w, rect.h);
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.scissor_ns);
#endif
}

static inline void wlx_raylib_end_scissor(void) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_raylib_time_begin();
#endif
    WLX_RAYLIB_PERF_INC(end_scissor_calls);
    WLX_RAYLIB_PERF_INC(clip_change_calls);
    EndScissorMode();
#ifdef WLX_PERF
    wlx_perf_raylib_time_end(perf_start_ns, &g_wlx_perf_raylib_state.current.scissor_ns);
#endif
}

static inline float wlx_raylib_get_frame_time(void) {
    return GetFrameTime();
}

static inline WLX_Backend wlx_backend_raylib(void) {
    return (WLX_Backend){
        .draw_rect = wlx_raylib_draw_rect,
        .draw_rect_lines = wlx_raylib_draw_rect_lines,
        .draw_rect_rounded = wlx_raylib_draw_rect_rounded,
        .draw_rect_rounded_lines = wlx_raylib_draw_rect_rounded_lines,
        .draw_circle = wlx_raylib_draw_circle,
        .draw_ring = wlx_raylib_draw_ring,
        .draw_gradient_v = wlx_raylib_draw_gradient_v,
        .draw_line = wlx_raylib_draw_line,
        .draw_text = wlx_raylib_draw_text,
        .measure_text = wlx_raylib_measure_text,
        .measure_text_slice = wlx_raylib_measure_text_slice,
        .draw_texture = wlx_raylib_draw_texture,
        .begin_scissor = wlx_raylib_begin_scissor,
        .end_scissor = wlx_raylib_end_scissor,
        .get_frame_time = wlx_raylib_get_frame_time,
    };
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

#endif // WOLLIX_RAYLIB_H_
