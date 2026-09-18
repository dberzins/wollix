/*
 * Copyright (c) 2026 Dainis Berzins
 * Licensed under the MIT License. See LICENSE file for full text.
 */

#ifndef WOLLIX_WASM_H_
#define WOLLIX_WASM_H_

#if defined(__INTELLISENSE__) && !defined(WOLLIX_H_)
#include "wollix.h"
#endif

#ifndef WOLLIX_H_
#error "Include wollix.h before wollix_wasm.h"
#endif

// ============================================================================
// wollix_wasm.h - Bare-wasm32 backend adapter for wollix
//
// Bridges the WLX_Backend interface to wasm imports implemented by the JS
// host (wollix_wasm.js). Structs (WLX_Rect, WLX_Color) are
// flattened to scalar arguments; WLX_Color is packed as a single uint32_t.
//
// Usage:
//   #define WOLLIX_IMPLEMENTATION
//   #include "wollix.h"
//   #include "wollix_wasm.h"
//
//   WLX_Context ctx = {0};
//   wlx_context_init_wasm(&ctx);
// ============================================================================

// ============================================================================
// Color packing
// ============================================================================

static inline uint32_t wlx_wasm_pack_color(WLX_Color c) {
    return ((uint32_t)c.r << 24) | ((uint32_t)c.g << 16)
         | ((uint32_t)c.b << 8)  | ((uint32_t)c.a);
}

// ============================================================================
// WASM page-pool allocator
//
// Recycles fixed-size blocks bucketed by power-of-two size class. The bare
// WASM libc shim implements free() as a no-op, so every realloc() leaks the
// old buffer. Routing the WLX_Arena_Pool general group through this pool
// returns shrunk/replaced blocks to a free-list keyed by class, eliminating
// cumulative heap waste.
//
// Sizing constants:
//   - WLX_WASM_POOL_MIN_ORDER: 8  -> 256 byte minimum block
//   - WLX_WASM_POOL_MAX_ORDER: 20 -> 1 MiB maximum block
// Sub-arenas growing past 1 MiB use the upper class and revert to malloc on
// every growth above the cap; this is acceptable because the affected
// general-group buffers (commands, scratch, etc.) cap well below 1 MiB.
// ============================================================================

// Both orders are overridable before including this header.
#ifndef WLX_WASM_POOL_MIN_ORDER
#define WLX_WASM_POOL_MIN_ORDER  8
#endif
#ifndef WLX_WASM_POOL_MAX_ORDER
#define WLX_WASM_POOL_MAX_ORDER  20
#endif
#define WLX_WASM_POOL_CLASSES    (WLX_WASM_POOL_MAX_ORDER - WLX_WASM_POOL_MIN_ORDER + 1)

#ifdef WLX_PERF
typedef struct {
    WLX_PERF_BACKEND_COMMON_FIELDS;
} WLX_Perf_Wasm_Frame;

typedef struct {
    WLX_Perf_Wasm_Frame current;
    WLX_Perf_Wasm_Frame last;
    WLX_Perf_Backend_Clock clock;
} WLX_Perf_Wasm_State;

static WLX_Perf_Wasm_State g_wlx_perf_wasm_state = {0};
#endif

typedef struct WLX_Wasm_Block {
    struct WLX_Wasm_Block *next;
} WLX_Wasm_Block;

typedef struct {
    WLX_Wasm_Block *free_list[WLX_WASM_POOL_CLASSES];
    size_t alloc_count;   // total alloc operations that hit malloc
    size_t reuse_count;   // total alloc operations that reused a free block
    size_t free_count;    // total free operations into the pool
    size_t bytes_in_use;  // bytes currently handed out to callers
    size_t high_water;    // peak bytes_in_use observed
} WLX_Wasm_Pool;

static inline int wlx_wasm_pool_class(size_t size) {
    int order = WLX_WASM_POOL_MIN_ORDER;
    size_t block;
    if (size == 0) size = 1;
    block = (size_t)1 << order;
    while (block < size && order < WLX_WASM_POOL_MAX_ORDER) {
        order++;
        block <<= 1;
    }
    return order - WLX_WASM_POOL_MIN_ORDER;
}

static inline size_t wlx_wasm_pool_class_size(int cls) {
    return (size_t)1 << ((size_t)cls + WLX_WASM_POOL_MIN_ORDER);
}

static inline void *wlx_wasm_pool_alloc(size_t size, void *user) {
    WLX_Wasm_Pool *pool = (WLX_Wasm_Pool *)user;
    int cls;
    size_t block_size;
    void *p;

    if (size == 0) size = 1;
    cls = wlx_wasm_pool_class(size);
    block_size = wlx_wasm_pool_class_size(cls);

    if (cls < WLX_WASM_POOL_CLASSES && pool->free_list[cls] != NULL) {
        WLX_Wasm_Block *blk = pool->free_list[cls];
        pool->free_list[cls] = blk->next;
        pool->reuse_count++;
        pool->bytes_in_use += block_size;
        if (pool->bytes_in_use > pool->high_water) pool->high_water = pool->bytes_in_use;
        return (void *)blk;
    }

    p = malloc(block_size);
    if (p != NULL) {
        pool->alloc_count++;
        pool->bytes_in_use += block_size;
        if (pool->bytes_in_use > pool->high_water) pool->high_water = pool->bytes_in_use;
    }
    return p;
}

static inline void wlx_wasm_pool_free(void *ptr, size_t size, void *user) {
    WLX_Wasm_Pool *pool = (WLX_Wasm_Pool *)user;
    int cls;
    size_t block_size;
    WLX_Wasm_Block *blk;

    if (ptr == NULL) return;
    cls = wlx_wasm_pool_class(size);
    block_size = wlx_wasm_pool_class_size(cls);
    pool->free_count++;
    if (pool->bytes_in_use >= block_size) pool->bytes_in_use -= block_size;

    if (cls >= WLX_WASM_POOL_CLASSES) return;
    blk = (WLX_Wasm_Block *)ptr;
    blk->next = pool->free_list[cls];
    pool->free_list[cls] = blk;
}

static inline void *wlx_wasm_pool_realloc(void *ptr, size_t old_size,
    size_t new_size, void *user)
{
    int old_cls;
    int new_cls;
    void *new_ptr;
    size_t copy;

    if (ptr == NULL) return wlx_wasm_pool_alloc(new_size, user);
    if (new_size == 0) {
        wlx_wasm_pool_free(ptr, old_size, user);
        return NULL;
    }

    old_cls = wlx_wasm_pool_class(old_size == 0 ? 1 : old_size);
    new_cls = wlx_wasm_pool_class(new_size);
    if (new_cls == old_cls) {
        return ptr;
    }

    new_ptr = wlx_wasm_pool_alloc(new_size, user);
    if (new_ptr != NULL) {
        copy = old_size < new_size ? old_size : new_size;
        if (copy > 0) memcpy(new_ptr, ptr, copy);
    }
    wlx_wasm_pool_free(ptr, old_size, user);
    return new_ptr;
}

static inline WLX_Allocator wlx_wasm_allocator(WLX_Wasm_Pool *pool) {
    return (WLX_Allocator){
        .alloc   = wlx_wasm_pool_alloc,
        .realloc = wlx_wasm_pool_realloc,
        .free    = wlx_wasm_pool_free,
        .user    = pool,
    };
}

// ============================================================================
// Wasm import declarations (provided by JS "wlx" module)
// ============================================================================

#ifdef __wasm__
#define WLX_WASM_IMPORT(name) \
    __attribute__((import_module("wlx"), import_name(name)))
#else
#define WLX_WASM_IMPORT(name)
#endif

WLX_WASM_IMPORT("draw_rect")
extern void wlx_wasm_import_draw_rect(
    float x, float y, float w, float h, uint32_t rgba);

WLX_WASM_IMPORT("draw_rect_lines")
extern void wlx_wasm_import_draw_rect_lines(
    float x, float y, float w, float h, float thick, uint32_t rgba);

WLX_WASM_IMPORT("draw_rect_rounded")
extern void wlx_wasm_import_draw_rect_rounded(
    float x, float y, float w, float h, float roundness, int segments,
    uint32_t rgba);

WLX_WASM_IMPORT("draw_rect_rounded_lines")
extern void wlx_wasm_import_draw_rect_rounded_lines(
    float x, float y, float w, float h, float roundness, int segments,
    float thick, uint32_t rgba);

WLX_WASM_IMPORT("draw_circle")
extern void wlx_wasm_import_draw_circle(
    float cx, float cy, float radius, int segments, uint32_t rgba);

WLX_WASM_IMPORT("draw_ring")
extern void wlx_wasm_import_draw_ring(
    float cx, float cy, float inner_r, float outer_r, int segments,
    uint32_t rgba);

WLX_WASM_IMPORT("draw_line")
extern void wlx_wasm_import_draw_line(
    float x1, float y1, float x2, float y2, float thick, uint32_t rgba);

WLX_WASM_IMPORT("draw_text")
extern void wlx_wasm_import_draw_text(
    const char *text, float x, float y, uintptr_t font, int font_size,
    uint32_t rgba);

WLX_WASM_IMPORT("measure_text")
extern void wlx_wasm_import_measure_text(
    const char *text, uintptr_t font, int font_size,
    float *out_w, float *out_h);

WLX_WASM_IMPORT("draw_text_slice")
extern void wlx_wasm_import_draw_text_slice(
    const char *text, uint32_t len, float x, float y, uintptr_t font,
    int font_size, uint32_t rgba);

WLX_WASM_IMPORT("measure_text_slice")
extern void wlx_wasm_import_measure_text_slice(
    const char *text, uint32_t len, uintptr_t font, int font_size,
    float *out_w, float *out_h);

// Batched cumulative advances: the host fills out[i] with the canvas width
// of the run prefix [0, unit_ends[i]) for each of unit_count strictly
// increasing byte ends (size_t == uint32 on wasm32) and returns the number
// filled - one boundary crossing per chunk instead of one per unit.
WLX_WASM_IMPORT("measure_text_advances")
extern uint32_t wlx_wasm_import_measure_text_advances(
    const char *text, uint32_t len, uintptr_t font, int font_size,
    const size_t *unit_ends, uint32_t unit_count, float *out_advances);

WLX_WASM_IMPORT("draw_texture")
extern void wlx_wasm_import_draw_texture(
    uintptr_t handle, float sx, float sy, float sw, float sh,
    float dx, float dy, float dw, float dh, uint32_t tint);

// Texture authoring imports. The host owns texture storage; C passes RGBA8
// pixel data and receives an opaque nonzero handle (or 0 on failure). Pixels
// are copied during the create call, so the buffer can be freed on return.
WLX_WASM_IMPORT("create_texture")
extern uintptr_t wlx_wasm_import_create_texture(
    const uint8_t *rgba, uint32_t width, uint32_t height);

WLX_WASM_IMPORT("destroy_texture")
extern void wlx_wasm_import_destroy_texture(uintptr_t handle);

WLX_WASM_IMPORT("begin_scissor")
extern void wlx_wasm_import_begin_scissor(
    float x, float y, float w, float h);

WLX_WASM_IMPORT("end_scissor")
extern void wlx_wasm_import_end_scissor(void);

WLX_WASM_IMPORT("get_frame_time")
extern float wlx_wasm_import_get_frame_time(void);

// Clipboard transport. clipboard_get_into copies the host's cached clipboard
// string into buf (up to cap bytes, UTF-8-boundary safe) and returns the byte
// count. clipboard_set copies a (text, len) span to the host clipboard cache.
WLX_WASM_IMPORT("clipboard_get_into")
extern uint32_t wlx_wasm_import_clipboard_get_into(char *buf, uint32_t cap);

WLX_WASM_IMPORT("clipboard_set")
extern void wlx_wasm_import_clipboard_set(const char *text, uint32_t len);

// Cursor shape: the host maps a WLX_Cursor_Shape value onto the canvas CSS
// cursor. Called by the core only when the shape changes.
WLX_WASM_IMPORT("set_cursor")
extern void wlx_wasm_import_set_cursor(uint32_t shape);

#if defined(WLX_PERF) && defined(WLX_WASM_PERF_TIMESTAMP)
WLX_WASM_IMPORT("perf_now_ns")
extern uint64_t wlx_wasm_import_perf_now_ns(void);
#endif

#undef WLX_WASM_IMPORT

#ifdef WLX_PERF
static inline bool wlx_perf_wasm_timer_available(void) {
#ifdef WLX_WASM_PERF_TIMESTAMP
    return true;
#else
    return false;
#endif
}

static inline uint64_t wlx_perf_wasm_timestamp(void *user) {
    WLX_UNUSED(user);
#ifdef WLX_WASM_PERF_TIMESTAMP
    return wlx_wasm_import_perf_now_ns();
#else
    return 0;
#endif
}

static inline void wlx_perf_wasm_install_timer(WLX_Context *ctx) {
#ifdef WLX_WASM_PERF_TIMESTAMP
    wlx_perf_set_timer(ctx, wlx_perf_wasm_timestamp, NULL);
#else
    WLX_UNUSED(ctx);
#endif
}

static inline void wlx_perf_wasm_begin_frame(uint64_t frame_index) {
    WLX_Perf_Wasm_State *st = &g_wlx_perf_wasm_state;
    wlx_zero_struct(st->current);
    st->current.frame_index = frame_index;
    st->current.timer_available = wlx_perf_wasm_timer_available();
    st->clock.timer_available = st->current.timer_available;
    st->clock.timestamp = wlx_perf_wasm_timestamp;
    st->clock.timestamp_user = NULL;
    wlx_perf_backend_frame_begin(&st->clock);
}

static inline void wlx_perf_wasm_end_frame(uint64_t frame_index) {
    WLX_Perf_Wasm_State *st = &g_wlx_perf_wasm_state;
    if (!st->clock.capturing) return;
    if (frame_index != 0) st->current.frame_index = frame_index;
    st->last = st->current;
    wlx_perf_backend_frame_end(&st->clock);
}

static inline void wlx_perf_wasm_reset(void) {
    wlx_zero_struct(g_wlx_perf_wasm_state);
}

static inline const WLX_Perf_Wasm_Frame *wlx_perf_wasm_get_last_frame(void) {
    return &g_wlx_perf_wasm_state.last;
}

static inline void wlx_perf_wasm_present_begin(void) {
    wlx_perf_backend_present_begin(&g_wlx_perf_wasm_state.clock);
}

static inline void wlx_perf_wasm_present_end(void) {
    wlx_perf_backend_present_end(&g_wlx_perf_wasm_state.clock,
        &g_wlx_perf_wasm_state.current.present_ns);
}

#define WLX_WASM_PERF_INC(field) \
    wlx_perf_backend_inc(&g_wlx_perf_wasm_state.clock, \
                         &g_wlx_perf_wasm_state.current.field)
#else
#define WLX_WASM_PERF_INC(field) ((void)0)
#endif

// Timed scope of one backend callback body: END (one per exit path)
// accumulates into the named duration field; both are no-ops without
// WLX_PERF.
#define WLX_WASM_SCOPE_BEGIN() \
    WLX_PERF_SCOPE_BEGIN(&g_wlx_perf_wasm_state.clock)
#define WLX_WASM_SCOPE_END(field) \
    WLX_PERF_SCOPE_END(&g_wlx_perf_wasm_state.clock, \
                       &g_wlx_perf_wasm_state.current.field)

// ============================================================================
// Backend callback wrappers
// ============================================================================

static inline void wlx_wasm_draw_rect(WLX_Rect r, WLX_Color c, void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(draw_rect_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_rect(r.x, r.y, r.w, r.h, wlx_wasm_pack_color(c));
    WLX_WASM_SCOPE_END(geometry_ns);
}

static inline void wlx_wasm_draw_rect_lines(WLX_Rect r, float thick, WLX_Color c, void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(draw_rect_lines_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_rect_lines(
        r.x, r.y, r.w, r.h, thick, wlx_wasm_pack_color(c));
    WLX_WASM_SCOPE_END(geometry_ns);
}

static inline void wlx_wasm_draw_rect_rounded(
        WLX_Rect r, float roundness, int segments, WLX_Color c, void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(draw_rect_rounded_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_rect_rounded(
        r.x, r.y, r.w, r.h, roundness, segments, wlx_wasm_pack_color(c));
    WLX_WASM_SCOPE_END(geometry_ns);
}

static inline void wlx_wasm_draw_rect_rounded_lines(
        WLX_Rect r, float roundness, int segments, float thick, WLX_Color c, void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(draw_rect_rounded_lines_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_rect_rounded_lines(
        r.x, r.y, r.w, r.h, roundness, segments, thick, wlx_wasm_pack_color(c));
    WLX_WASM_SCOPE_END(geometry_ns);
}

static inline void wlx_wasm_draw_circle(
        float cx, float cy, float radius, int segments, WLX_Color c, void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(draw_circle_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_circle(
        cx, cy, radius, segments, wlx_wasm_pack_color(c));
    WLX_WASM_SCOPE_END(geometry_ns);
}

static inline void wlx_wasm_draw_ring(
        float cx, float cy, float inner_r, float outer_r, int segments,
        WLX_Color c, void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(draw_ring_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_ring(
        cx, cy, inner_r, outer_r, segments, wlx_wasm_pack_color(c));
    WLX_WASM_SCOPE_END(geometry_ns);
}

static inline void wlx_wasm_draw_line(
        float x1, float y1, float x2, float y2, float thick, WLX_Color c, void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(draw_line_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_line(x1, y1, x2, y2, thick, wlx_wasm_pack_color(c));
    WLX_WASM_SCOPE_END(geometry_ns);
}

static inline void wlx_wasm_draw_text(
        const char *text, float x, float y, WLX_Text_Style style, void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(draw_text_calls);
    wlx_wasm_import_draw_text(
        text, x, y, style.font, style.font_size,
        wlx_wasm_pack_color(style.color));
    WLX_WASM_SCOPE_END(text_draw_ns);
}

static inline void wlx_wasm_measure_text(
        const char *text, WLX_Text_Style style, float *out_w, float *out_h, void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(measure_text_calls);
    wlx_wasm_import_measure_text(
        text, style.font, style.font_size, out_w, out_h);
    WLX_WASM_SCOPE_END(text_measure_ns);
}

static inline void wlx_wasm_draw_text_slice(
        const char *text, size_t len, float x, float y, WLX_Text_Style style, void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(draw_text_calls);
    wlx_wasm_import_draw_text_slice(
        text, (uint32_t)len, x, y, style.font, style.font_size,
        wlx_wasm_pack_color(style.color));
    WLX_WASM_SCOPE_END(text_draw_ns);
}

static inline void wlx_wasm_measure_text_slice(
        const char *text, size_t len, WLX_Text_Style style,
        float *out_w, float *out_h, void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(measure_text_calls);
    wlx_wasm_import_measure_text_slice(
        text, (uint32_t)len, style.font, style.font_size,
        out_w, out_h);
    WLX_WASM_SCOPE_END(text_measure_ns);
}

static inline size_t wlx_wasm_measure_text_advances(
        const char *text, size_t len, WLX_Text_Style style,
        const size_t *unit_ends, size_t unit_count, float *out_advances, void *user) {
    WLX_UNUSED(user);
    if (text == NULL || unit_ends == NULL || out_advances == NULL || unit_count == 0)
        return 0;
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(measure_text_calls);
    uint32_t filled = wlx_wasm_import_measure_text_advances(
        text, (uint32_t)len, style.font, style.font_size,
        unit_ends, (uint32_t)unit_count, out_advances);
    WLX_WASM_SCOPE_END(text_measure_ns);
    return (size_t)filled;
}

static inline void wlx_wasm_draw_texture(
        WLX_Texture tex, WLX_Rect src, WLX_Rect dst, WLX_Color tint, void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(draw_texture_calls);
    wlx_wasm_import_draw_texture(
        tex.handle,
        src.x, src.y, src.w, src.h,
        dst.x, dst.y, dst.w, dst.h,
        wlx_wasm_pack_color(tint));
    WLX_WASM_SCOPE_END(texture_ns);
}

static inline WLX_Texture wlx_wasm_texture_create(
        const uint8_t *rgba, int width, int height) {
    WLX_Texture tex = {0};
    if (rgba == NULL || width <= 0 || height <= 0) return tex;
    tex.handle = wlx_wasm_import_create_texture(
        rgba, (uint32_t)width, (uint32_t)height);
    if (tex.handle == 0) return (WLX_Texture){0};
    tex.width  = width;
    tex.height = height;
    return tex;
}

static inline void wlx_wasm_texture_destroy(WLX_Texture tex) {
    if (tex.handle == 0) return;
    wlx_wasm_import_destroy_texture(tex.handle);
}

static inline void wlx_wasm_begin_scissor(WLX_Rect r, void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(begin_scissor_calls);
    WLX_WASM_PERF_INC(clip_change_calls);
    wlx_wasm_import_begin_scissor(r.x, r.y, r.w, r.h);
    WLX_WASM_SCOPE_END(scissor_ns);
}

static inline void wlx_wasm_end_scissor(void *user) {
    WLX_UNUSED(user);
    WLX_WASM_SCOPE_BEGIN();
    WLX_WASM_PERF_INC(end_scissor_calls);
    WLX_WASM_PERF_INC(clip_change_calls);
    wlx_wasm_import_end_scissor();
    WLX_WASM_SCOPE_END(scissor_ns);
}

static inline float wlx_wasm_get_frame_time(void *user) {
    WLX_UNUSED(user);
    return wlx_wasm_import_get_frame_time();
}

// Upper bound for the clipboard receive buffer. Overridable before include.
#ifndef WLX_WASM_CLIPBOARD_MAX
#define WLX_WASM_CLIPBOARD_MAX (16u * 1024u * 1024u)
#endif

// Grow-and-reuse receive buffer. The host writes at most `cap` bytes (never
// splitting a UTF-8 sequence) and returns the count, so a count within the
// 3-byte backoff window of the cap may mean truncation: grow and re-fetch
// until the text provably fit or the soft cap is reached. Growth is
// geometric, so the no-op-free libc shim leaks at most ~1x the final size.
static char *g_wlx_wasm_clipboard_buf = NULL;
static uint32_t g_wlx_wasm_clipboard_cap = 0;

static inline const char *wlx_wasm_clipboard_get(void *user) {
    WLX_UNUSED(user);
    if (g_wlx_wasm_clipboard_buf == NULL) {
        // First fetch: start small, but never above the (overridable) soft
        // cap plus its terminator byte.
        g_wlx_wasm_clipboard_cap = 4096 < WLX_WASM_CLIPBOARD_MAX + 1
            ? 4096 : WLX_WASM_CLIPBOARD_MAX + 1;
        g_wlx_wasm_clipboard_buf = (char *)wlx_alloc(g_wlx_wasm_clipboard_cap);
        if (g_wlx_wasm_clipboard_buf == NULL) {
            g_wlx_wasm_clipboard_cap = 0;
            return "";
        }
    }
    for (;;) {
        uint32_t room = g_wlx_wasm_clipboard_cap - 1;
        uint32_t n = wlx_wasm_import_clipboard_get_into(g_wlx_wasm_clipboard_buf, room);
        if (n > room) n = room;
        if (n + 4 <= room || room >= WLX_WASM_CLIPBOARD_MAX) {
            g_wlx_wasm_clipboard_buf[n] = '\0';
            return g_wlx_wasm_clipboard_buf;
        }
        uint32_t new_cap = g_wlx_wasm_clipboard_cap * 2;
        if (new_cap > WLX_WASM_CLIPBOARD_MAX + 1) new_cap = WLX_WASM_CLIPBOARD_MAX + 1;
        char *new_buf = (char *)wlx_alloc(new_cap);
        if (new_buf == NULL) {
            g_wlx_wasm_clipboard_buf[n] = '\0';
            return g_wlx_wasm_clipboard_buf;
        }
        wlx_free(g_wlx_wasm_clipboard_buf);
        g_wlx_wasm_clipboard_buf = new_buf;
        g_wlx_wasm_clipboard_cap = new_cap;
    }
}

static inline void wlx_wasm_clipboard_set(const char *text, size_t len, void *user) {
    WLX_UNUSED(user);
    wlx_wasm_import_clipboard_set(text, (uint32_t)len);
}

// The host's set_cursor (web/wollix_wasm.js) maps the shape onto the canvas
// CSS cursor. The enum is append-only; this build check flags a new shape
// the JS map must learn about.
_Static_assert(WLX_CURSOR_COUNT == 2, "update the JS set_cursor map for the new WLX_Cursor_Shape");
static inline void wlx_wasm_set_cursor(WLX_Cursor_Shape shape, void *user) {
    WLX_UNUSED(user);
    wlx_wasm_import_set_cursor((uint32_t)shape);
}

// ============================================================================
// Backend factory
// ============================================================================

static inline WLX_Backend wlx_backend_wasm(void) {
    return (WLX_Backend){
        .contract_version  = WLX_BACKEND_CONTRACT_VERSION,
        .user              = NULL,
        .draw_rect         = wlx_wasm_draw_rect,
        .draw_rect_lines   = wlx_wasm_draw_rect_lines,
        .draw_rect_rounded       = wlx_wasm_draw_rect_rounded,
        .draw_rect_rounded_lines = wlx_wasm_draw_rect_rounded_lines,
        .draw_circle             = wlx_wasm_draw_circle,
        .draw_ring               = wlx_wasm_draw_ring,
        .draw_line               = wlx_wasm_draw_line,
        .draw_text         = wlx_wasm_draw_text,
        .measure_text      = wlx_wasm_measure_text,
        .draw_texture      = wlx_wasm_draw_texture,
        .begin_scissor     = wlx_wasm_begin_scissor,
        .end_scissor       = wlx_wasm_end_scissor,
        .get_frame_time    = wlx_wasm_get_frame_time,
        .draw_text_slice    = wlx_wasm_draw_text_slice,
        .measure_text_slice = wlx_wasm_measure_text_slice,
        .measure_text_advances = wlx_wasm_measure_text_advances,
        .clipboard_get     = wlx_wasm_clipboard_get,
        .clipboard_set     = wlx_wasm_clipboard_set,
        .set_cursor        = wlx_wasm_set_cursor,
    };
}

// ============================================================================
// Input shared memory
// ============================================================================
//
// The JS host writes input state directly into wasm memory each frame before
// calling wlx_wasm_frame(). The build exports the address of
// wlx_wasm_input_state so JS can cache it on init.

extern WLX_Input_State wlx_wasm_input_state;

// The JS host (web/wollix_wasm.js INPUT_OFFSETS) writes input fields at these
// fixed byte offsets. Lock them here so any change to WLX_Input_State or
// WLX_KEY_COUNT fails the build until the JS table is updated to match.
_Static_assert(offsetof(WLX_Input_State, keys_down)     == 16,  "WASM INPUT_OFFSETS.keys_down out of sync");
_Static_assert(offsetof(WLX_Input_State, keys_pressed)  == 80,  "WASM INPUT_OFFSETS.keys_pressed out of sync");
_Static_assert(offsetof(WLX_Input_State, text_input)    == 144, "WASM INPUT_OFFSETS.text_input out of sync");
_Static_assert(offsetof(WLX_Input_State, keys_repeated) == 176, "WASM INPUT_OFFSETS.keys_repeated out of sync");
_Static_assert(offsetof(WLX_Input_State, modifiers)     == 240, "WASM INPUT_OFFSETS.modifiers out of sync");
_Static_assert(offsetof(WLX_Input_State, wheel_delta_x) == 244, "WASM INPUT_OFFSETS.wheel_delta_x out of sync");
_Static_assert(offsetof(WLX_Input_State, mouse_right_down) == 248, "WASM INPUT_OFFSETS.mouse_right_down out of sync");
_Static_assert(sizeof(WLX_Input_State)                  == 252, "WASM INPUT_SIZE out of sync");

static inline WLX_Input_State *wlx_wasm_get_input_ptr(void) {
    return &wlx_wasm_input_state;
}

static inline void wlx_process_wasm_input(WLX_Context *ctx) {
    ctx->input = wlx_wasm_input_state;
}

// ============================================================================
// Context init
// ============================================================================

static inline void wlx_context_init_wasm(WLX_Context *ctx) {
    assert(ctx != NULL);
    ctx->backend = wlx_backend_wasm();
#ifdef WLX_PERF
    wlx_perf_wasm_install_timer(ctx);
#endif
}

#undef WLX_WASM_PERF_INC

#endif // WOLLIX_WASM_H_
