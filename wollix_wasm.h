#ifndef WOLLIX_WASM_H_
#define WOLLIX_WASM_H_

#ifndef WOLLIX_H_
#error "Include wollix.h before wollix_wasm.h"
#endif

// ============================================================================
// wollix_wasm.h — Bare-wasm32 backend adapter for wollix
//
// Bridges the 10-callback WLX_Backend interface to wasm imports implemented
// by the JS host (wollix_wasm.js). Structs (WLX_Rect, WLX_Color) are
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
// Wasm import declarations (provided by JS "wlx" module)
// ============================================================================

#define WLX_WASM_IMPORT(name) \
    __attribute__((import_module("wlx"), import_name(name)))

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

WLX_WASM_IMPORT("draw_line")
extern void wlx_wasm_import_draw_line(
    float x1, float y1, float x2, float y2, float thick, uint32_t rgba);

WLX_WASM_IMPORT("draw_text")
extern void wlx_wasm_import_draw_text(
    const char *text, float x, float y, uintptr_t font, int font_size,
    int spacing, uint32_t rgba);

WLX_WASM_IMPORT("measure_text")
extern void wlx_wasm_import_measure_text(
    const char *text, uintptr_t font, int font_size, int spacing,
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

#undef WLX_WASM_IMPORT

// ============================================================================
// Backend callback wrappers
// ============================================================================

static inline void wlx_wasm_draw_rect(WLX_Rect r, WLX_Color c) {
    wlx_wasm_import_draw_rect(r.x, r.y, r.w, r.h, wlx_wasm_pack_color(c));
}

static inline void wlx_wasm_draw_rect_lines(WLX_Rect r, float thick, WLX_Color c) {
    wlx_wasm_import_draw_rect_lines(
        r.x, r.y, r.w, r.h, thick, wlx_wasm_pack_color(c));
}

static inline void wlx_wasm_draw_rect_rounded(
        WLX_Rect r, float roundness, int segments, WLX_Color c) {
    wlx_wasm_import_draw_rect_rounded(
        r.x, r.y, r.w, r.h, roundness, segments, wlx_wasm_pack_color(c));
}

static inline void wlx_wasm_draw_line(
        float x1, float y1, float x2, float y2, float thick, WLX_Color c) {
    wlx_wasm_import_draw_line(x1, y1, x2, y2, thick, wlx_wasm_pack_color(c));
}

static inline void wlx_wasm_draw_text(
        const char *text, float x, float y, WLX_Text_Style style) {
    wlx_wasm_import_draw_text(
        text, x, y, style.font, style.font_size, style.spacing,
        wlx_wasm_pack_color(style.color));
}

static inline void wlx_wasm_measure_text(
        const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
    wlx_wasm_import_measure_text(
        text, style.font, style.font_size, style.spacing, out_w, out_h);
}

static inline void wlx_wasm_draw_texture(
        WLX_Texture tex, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    wlx_wasm_import_draw_texture(
        tex.handle,
        src.x, src.y, src.w, src.h,
        dst.x, dst.y, dst.w, dst.h,
        wlx_wasm_pack_color(tint));
}

static inline void wlx_wasm_begin_scissor(WLX_Rect r) {
    wlx_wasm_import_begin_scissor(r.x, r.y, r.w, r.h);
}

static inline void wlx_wasm_end_scissor(void) {
    wlx_wasm_import_end_scissor();
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
        .draw_rect_rounded = wlx_wasm_draw_rect_rounded,
        .draw_line         = wlx_wasm_draw_line,
        .draw_text         = wlx_wasm_draw_text,
        .measure_text      = wlx_wasm_measure_text,
        .draw_texture      = wlx_wasm_draw_texture,
        .begin_scissor     = wlx_wasm_begin_scissor,
        .end_scissor       = wlx_wasm_end_scissor,
        .get_frame_time    = wlx_wasm_get_frame_time,
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
}

#endif // WOLLIX_WASM_H_
