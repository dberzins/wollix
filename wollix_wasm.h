#ifndef WOLLIX_WASM_H_
#define WOLLIX_WASM_H_

#ifndef WOLLIX_H_
#error "Include wollix.h before wollix_wasm.h"
#endif

// ============================================================================
// wollix_wasm.h — Bare-wasm32 backend adapter for wollix
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

#define WLX_WASM_POOL_MIN_ORDER  8
#define WLX_WASM_POOL_MAX_ORDER  20
#define WLX_WASM_POOL_CLASSES    (WLX_WASM_POOL_MAX_ORDER - WLX_WASM_POOL_MIN_ORDER + 1)

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
} WLX_Perf_Wasm_Frame;

typedef struct {
    WLX_Perf_Wasm_Frame current;
    WLX_Perf_Wasm_Frame last;
    uint64_t present_start_ns;
    bool capturing;
} WLX_Perf_Wasm_State;

static WLX_Perf_Wasm_State wlx_perf_wasm_state = {0};
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

WLX_WASM_IMPORT("draw_texture")
extern void wlx_wasm_import_draw_texture(
    uintptr_t handle, float sx, float sy, float sw, float sh,
    float dx, float dy, float dw, float dh, uint32_t tint);

WLX_WASM_IMPORT("begin_scissor")
extern void wlx_wasm_import_begin_scissor(
    float x, float y, float w, float h);

WLX_WASM_IMPORT("end_scissor")
extern void wlx_wasm_import_end_scissor(void);

WLX_WASM_IMPORT("get_frame_time")
extern float wlx_wasm_import_get_frame_time(void);

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
    wlx_zero_struct(wlx_perf_wasm_state.current);
    wlx_perf_wasm_state.current.frame_index = frame_index;
    wlx_perf_wasm_state.current.timer_available = wlx_perf_wasm_timer_available();
    wlx_perf_wasm_state.present_start_ns = 0;
    wlx_perf_wasm_state.capturing = true;
}

static inline void wlx_perf_wasm_end_frame(uint64_t frame_index) {
    if (!wlx_perf_wasm_state.capturing) return;
    if (frame_index != 0) wlx_perf_wasm_state.current.frame_index = frame_index;
    wlx_perf_wasm_state.last = wlx_perf_wasm_state.current;
    wlx_perf_wasm_state.capturing = false;
    wlx_perf_wasm_state.present_start_ns = 0;
}

static inline void wlx_perf_wasm_reset(void) {
    wlx_zero_struct(wlx_perf_wasm_state);
}

static inline const WLX_Perf_Wasm_Frame *wlx_perf_wasm_get_last_frame(void) {
    return &wlx_perf_wasm_state.last;
}

static inline void wlx_perf_wasm_inc(uint64_t *counter) {
    if (!wlx_perf_wasm_state.capturing) return;
    (*counter)++;
}

static inline uint64_t wlx_perf_wasm_time_begin(void) {
    if (!wlx_perf_wasm_state.capturing || !wlx_perf_wasm_state.current.timer_available) return 0;
    return wlx_perf_wasm_timestamp(NULL);
}

static inline void wlx_perf_wasm_time_end(uint64_t start_ns, uint64_t *total_ns) {
    uint64_t end_ns;

    if (!wlx_perf_wasm_state.capturing || !wlx_perf_wasm_state.current.timer_available) return;
    end_ns = wlx_perf_wasm_timestamp(NULL);
    if (end_ns >= start_ns) *total_ns += end_ns - start_ns;
}

static inline void wlx_perf_wasm_present_begin(void) {
    wlx_perf_wasm_state.present_start_ns = wlx_perf_wasm_time_begin();
}

static inline void wlx_perf_wasm_present_end(void) {
    wlx_perf_wasm_time_end(wlx_perf_wasm_state.present_start_ns,
        &wlx_perf_wasm_state.current.present_ns);
    wlx_perf_wasm_state.present_start_ns = 0;
}

#define WLX_WASM_PERF_INC(field) \
    wlx_perf_wasm_inc(&wlx_perf_wasm_state.current.field)
#else
#define WLX_WASM_PERF_INC(field) ((void)0)
#endif

// ============================================================================
// Backend callback wrappers
// ============================================================================

static inline void wlx_wasm_draw_rect(WLX_Rect r, WLX_Color c) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(draw_rect_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_rect(r.x, r.y, r.w, r.h, wlx_wasm_pack_color(c));
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.geometry_ns);
#endif
}

static inline void wlx_wasm_draw_rect_lines(WLX_Rect r, float thick, WLX_Color c) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(draw_rect_lines_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_rect_lines(
        r.x, r.y, r.w, r.h, thick, wlx_wasm_pack_color(c));
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.geometry_ns);
#endif
}

static inline void wlx_wasm_draw_rect_rounded(
        WLX_Rect r, float roundness, int segments, WLX_Color c) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(draw_rect_rounded_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_rect_rounded(
        r.x, r.y, r.w, r.h, roundness, segments, wlx_wasm_pack_color(c));
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.geometry_ns);
#endif
}

static inline void wlx_wasm_draw_rect_rounded_lines(
        WLX_Rect r, float roundness, int segments, float thick, WLX_Color c) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(draw_rect_rounded_lines_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_rect_rounded_lines(
        r.x, r.y, r.w, r.h, roundness, segments, thick, wlx_wasm_pack_color(c));
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.geometry_ns);
#endif
}

static inline void wlx_wasm_draw_circle(
        float cx, float cy, float radius, int segments, WLX_Color c) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(draw_circle_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_circle(
        cx, cy, radius, segments, wlx_wasm_pack_color(c));
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.geometry_ns);
#endif
}

static inline void wlx_wasm_draw_ring(
        float cx, float cy, float inner_r, float outer_r, int segments,
        WLX_Color c) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(draw_ring_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_ring(
        cx, cy, inner_r, outer_r, segments, wlx_wasm_pack_color(c));
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.geometry_ns);
#endif
}

static inline void wlx_wasm_draw_line(
        float x1, float y1, float x2, float y2, float thick, WLX_Color c) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(draw_line_calls);
    WLX_WASM_PERF_INC(geometry_submit_calls);
    wlx_wasm_import_draw_line(x1, y1, x2, y2, thick, wlx_wasm_pack_color(c));
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.geometry_ns);
#endif
}

static inline void wlx_wasm_draw_text(
        const char *text, float x, float y, WLX_Text_Style style) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(draw_text_calls);
    wlx_wasm_import_draw_text(
        text, x, y, style.font, style.font_size,
        wlx_wasm_pack_color(style.color));
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.text_draw_ns);
#endif
}

static inline void wlx_wasm_measure_text(
        const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(measure_text_calls);
    wlx_wasm_import_measure_text(
        text, style.font, style.font_size, out_w, out_h);
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.text_measure_ns);
#endif
}

static inline void wlx_wasm_draw_text_slice(
        const char *text, size_t len, float x, float y, WLX_Text_Style style) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(draw_text_calls);
    wlx_wasm_import_draw_text_slice(
        text, (uint32_t)len, x, y, style.font, style.font_size,
        wlx_wasm_pack_color(style.color));
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.text_draw_ns);
#endif
}

static inline void wlx_wasm_measure_text_slice(
        const char *text, size_t len, WLX_Text_Style style,
        float *out_w, float *out_h) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(measure_text_calls);
    wlx_wasm_import_measure_text_slice(
        text, (uint32_t)len, style.font, style.font_size,
        out_w, out_h);
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.text_measure_ns);
#endif
}

static inline void wlx_wasm_draw_texture(
        WLX_Texture tex, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(draw_texture_calls);
    wlx_wasm_import_draw_texture(
        tex.handle,
        src.x, src.y, src.w, src.h,
        dst.x, dst.y, dst.w, dst.h,
        wlx_wasm_pack_color(tint));
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.texture_ns);
#endif
}

static inline void wlx_wasm_begin_scissor(WLX_Rect r) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(begin_scissor_calls);
    WLX_WASM_PERF_INC(clip_change_calls);
    wlx_wasm_import_begin_scissor(r.x, r.y, r.w, r.h);
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.scissor_ns);
#endif
}

static inline void wlx_wasm_end_scissor(void) {
#ifdef WLX_PERF
    uint64_t perf_start_ns = wlx_perf_wasm_time_begin();
#endif
    WLX_WASM_PERF_INC(end_scissor_calls);
    WLX_WASM_PERF_INC(clip_change_calls);
    wlx_wasm_import_end_scissor();
#ifdef WLX_PERF
    wlx_perf_wasm_time_end(perf_start_ns, &wlx_perf_wasm_state.current.scissor_ns);
#endif
}

static inline float wlx_wasm_get_frame_time(void) {
    return wlx_wasm_import_get_frame_time();
}

// ============================================================================
// Backend factory
// ============================================================================

static inline WLX_Backend wlx_backend_wasm(void) {
    return (WLX_Backend){
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
