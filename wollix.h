/*
 * wollix.h - Woven layouts for C.
 *
 * Version: 0.3.0  (WOLLIX_VERSION / WLX_VERSION)
 *
 * Copyright (c) 2026 Dainis Berzins
 * Licensed under the MIT License. See LICENSE file for full text.
 *
 * Repository:  https://github.com/dberzins/wollix
 *
 * ---------------------------------------------------------------------------
 * WHAT IS THIS?
 * ---------------------------------------------------------------------------
 * Wollix is a lightweight, header-only, immediate-mode UI layout library
 * written in C11. It lets you compose rows, columns, grids, and nested
 * panels that interlock widgets into place. The core has zero external
 * dependencies; rendering is delegated to thin backend adapters:
 *
 *   - wollix_raylib.h   (Raylib backend)
 *   - wollix_sdl3.h     (SDL3 backend)
 *   - wollix_wasm.h     (Bare WASM32 backend)
 *
 * Built-in widgets and compound helpers include labels, buttons, checkboxes
 * (including textured checkboxes), input boxes, sliders, progress bars,
 * toggles, radio buttons, separators, scroll panels, panels, and split
 * layouts.
 *
 * ---------------------------------------------------------------------------
 * USAGE
 * ---------------------------------------------------------------------------
 * This is a single-header library. In exactly ONE translation unit, define
 * the implementation macro before including the header:
 *
 *   #define WOLLIX_IMPLEMENTATION
 *   #include "wollix.h"
 *
 * All other files that need the API can simply #include "wollix.h".
 *
 * A matching backend header must be included AFTER wollix.h:
 *
 *   #include <raylib.h>             // must come first so RAYLIB_H is defined
 *   #define WOLLIX_IMPLEMENTATION
 *   #include "wollix.h"
 *   #include "wollix_raylib.h"
 *
 * Quick start:
 *
 *   WLX_Context ctx = {0};
 *   wlx_context_init_raylib(&ctx);  // or wlx_context_init_sdl3(...)
 *                                  // or wlx_context_init_wasm(...)
 *
 *   wlx_begin(&ctx, root_rect, wlx_process_raylib_input);
 *     wlx_layout_begin(&ctx, 2, WLX_VERT, .padding = 8);
 *       wlx_label(&ctx, "Hello!", .font_size = 24);
 *       if (wlx_button(&ctx, "OK")) { ... }
 *     wlx_layout_end(&ctx);
 *   wlx_end(&ctx);
 *
 * ---------------------------------------------------------------------------
 * IMPORTANT COMPILE-TIME MACROS (define BEFORE including wollix.h)
 * ---------------------------------------------------------------------------
 *
 * WOLLIX_IMPLEMENTATION
 *     Required in exactly one .c file to emit the function bodies.
 *
 * WLXDEF
 *     Linkage qualifier prepended to every public function. Defaults to
 *     nothing (normal external linkage). Define as `static` before including
 *     to make all functions file-local for single translation-unit builds:
 *       #define WLXDEF static
 *
 * WLX_SHORT_NAMES
 *     Provides un-prefixed aliases (e.g. layout_begin, button, slider)
 *     for the wlx_* API.  Convenient for terse code; optional.
 *
 * WLX_SLOT_MINMAX_REDISTRIBUTE
 *     Enables iterative freeze-and-redistribute for slot min/max
 *     constraints. When defined, surplus/deficit from clamped slots is
 *     re-distributed among unfrozen siblings so offsets stay consistent.
 *     Default (undefined): single-pass clamp - simpler and O(n), but may
 *     leave a small gap or overflow when min/max constraints fire.
 *
 * ---------------------------------------------------------------------------
 * LIMITS, DEFAULTS, AND HOOK MACROS
 * ---------------------------------------------------------------------------
 *
 * WLX_MAX_SLOT_COUNT  (default 100000)
 *     Upper-bound sanity check for slot/row/column counts.
 *
 * WLX_CONTENT_SLOTS_MAX  (default 32)
 *     Maximum number of WLX_SIZE_CONTENT slots tracked per layout frame.
 *
 * WLX_OFFSET_STACK_LIMIT  (default 64)
 *     Stack scratch limit for min/max redistribution; larger layouts fall
 *     back to heap-backed scratch buffers.
 *
 * WLX_INPUTBOX_CURSOR_TEMP_SIZE  (default 512)
 *     Temporary UTF-8 buffer size used while computing input-box cursor
 *     positions.
 *
 * WLX_DA_INIT_CAP  (default 256)
 *     Initial capacity for internal dynamic arrays.
 *
 * WLX_TEXT_RUN_MAX_GLYPHS  (default 512)
 *     Maximum glyphs processed in a single text-layout run.
 *
 * WLX_STYLE_ROW_HEIGHT / WLX_STYLE_BUTTON_HEIGHT / WLX_STYLE_INPUT_HEIGHT
 * WLX_STYLE_HEADING_FONT_SIZE / WLX_STYLE_BORDER_WIDTH
 * WLX_STYLE_ROUNDNESS / WLX_STYLE_CONTENT_PADDING
 *     Advisory style-guide defaults for widget sizing and decoration.
 *     These may be overridden before including wollix.h.
 *
 * wlx_alloc(size)  /  wlx_calloc(count, size)  /  wlx_realloc(ptr, size)  /  wlx_free(ptr)
 *     Internal allocation macros used by the implementation. They currently
 *     forward to malloc/calloc/realloc/free.
 *
 * ---------------------------------------------------------------------------
 * DEBUG MACROS
 * ---------------------------------------------------------------------------
 *
 * WLX_DEBUG
 *     When defined, enables debug instrumentation: call-site duplicate
 *     detection, FLEX/FILL unbounded-height warnings, CONTENT-slot
 *     oscillation detection, split widget pairing assertions, and widget
 *     clipping warnings. All debug state is isolated in a separate context
 *     (WLX_Debug_Context) - production structs are not affected.
 *     Use ctx->dbg->warn_cb to capture warnings programmatically.
 *
 * WLX_MEMORY_DEBUG
 *     When defined, every internal malloc/realloc/free prints the source
 *     location, pointer address, and byte count to stdout. Useful for
 *     tracking down leaks inside the library. Not intended for user code.
 *
 * ---------------------------------------------------------------------------
 * BACKEND AUTO-DETECTION
 * ---------------------------------------------------------------------------
 * If <raylib.h> is included before wollix.h (i.e. RAYLIB_H is defined),
 * WLX_Color maps to Raylib's Color and WLX_Texture maps to Texture2D.
 * Otherwise, standalone struct types are provided so the core compiles
 * without any graphics library.
 */

#ifndef WOLLIX_H_
#define WOLLIX_H_

#define WOLLIX_VERSION "0.3.0"
#define WLX_VERSION WOLLIX_VERSION

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

// Linkage qualifier prepended to every public function declaration and definition.
//
// By default, WLXDEF expands to nothing (normal external linkage).
// To make all functions file-local (single translation-unit builds):
//   #define WLXDEF static
#ifndef WLXDEF
#define WLXDEF
#endif

// ============================================================================
// Backend-neutral color and texture types
// ============================================================================

#ifdef RAYLIB_H
typedef Color WLX_Color;
typedef Texture2D WLX_Texture;

#define WLX_WHITE WHITE
#define WLX_BLACK BLACK
#define WLX_LIGHTGRAY LIGHTGRAY

#else
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} WLX_Color;

typedef struct {
    uintptr_t handle;
    int width;
    int height;
} WLX_Texture;

#define WLX_WHITE WLX_RGBA(255, 255, 255, 255)
#define WLX_BLACK WLX_RGBA(0, 0, 0, 255)
#define WLX_LIGHTGRAY WLX_RGBA(200, 200, 200, 255)

#endif // RAYLIB_H

#define WLX_RGBA(r, g, b, a) ((WLX_Color){ (r), (g), (b), (a) })

// Deprecated: use ctx->theme->background / scrollbar.bar / hover_brightness instead.
// Kept for backward compatibility with existing demo ClearBackground() calls.
#define WLX_BACKGROUND_COLOR WLX_RGBA(18, 18, 18, 255)
#define WLX_SCROLLBAR_COLOR WLX_RGBA(38, 38, 38, 255)
#define WLX_HOVER_BRIGHTNESS 0.75f

// ============================================================================
// Core macros and utility helpers
// ============================================================================

// Sane upper bound for slot / row / column counts.
// Catches accidental negative -> size_t wraparound (e.g. passing -1).
#define WLX_MAX_SLOT_COUNT 100000

// Maximum number of slots that can use WLX_SIZE_CONTENT in a single layout.
// Stores per-slot measured heights from the previous frame (128 bytes).
#define WLX_CONTENT_SLOTS_MAX 32

// Stack-allocated working array limit for min/max redistribution in wlx_compute_offsets.
// Counts above this fall back to heap allocation.
#define WLX_OFFSET_STACK_LIMIT 64

// Temporary buffer size for cursor position calculation in inputbox.
#define WLX_INPUTBOX_CURSOR_TEMP_SIZE 512

#define WLX_UNUSED(value) (void)(value)
#define WLX_TODO(message) do { fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, message); abort(); } while(0)
#define WLX_UNREACHABLE(message) do { fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message); abort(); } while(0)

#define wlx_array_len(array) (sizeof(array)/sizeof(array[0]))
#define wlx_zero_struct(instance) memset(&(instance), 0, sizeof(instance))
#define wlx_zero_array(count, pointer) memset((pointer), 0, (count) * sizeof((pointer)[0]))

#define wlx_alloc(size) wlx_alloc_impl(size, __FILE__, __LINE__, __FUNCTION__)
static inline void* wlx_alloc_impl(size_t size, const char *file, int line, const char *func) {
    void *ptr = malloc(size);
    #ifdef WLX_MEMORY_DEBUG
        printf("%s:%d: Allocated = %s, %p [%li bytes]\n", file, line, func, ptr, size);
    #else
        WLX_UNUSED(file); WLX_UNUSED(line); WLX_UNUSED(func);
    #endif
    return ptr;
}

#define wlx_calloc(count, size) wlx_calloc_impl(count, size, __FILE__, __LINE__, __FUNCTION__)
static inline void* wlx_calloc_impl(size_t count, size_t size, const char *file, int line, const char *func) {
    void *ptr = calloc(count, size);
    #ifdef WLX_MEMORY_DEBUG
        printf("%s:%d: Callocated = %s, %p [%zu x %li bytes]\n", file, line, func, ptr, count, size);
    #else
        WLX_UNUSED(file); WLX_UNUSED(line); WLX_UNUSED(func);
    #endif
    return ptr;
}

#define wlx_realloc(pointer, size) wlx_realloc_impl(pointer, size, __FILE__, __LINE__, __FUNCTION__)
static inline void* wlx_realloc_impl(void* ptr, size_t size, const char *file, int line, const char *func) {
    void *new_ptr = realloc(ptr, size);
    if (new_ptr) {
        #ifdef WLX_MEMORY_DEBUG
            printf("%s:%d: Deallocated = %s, %p\n", file, line, func, ptr);
            printf("%s:%d: Reallocated = %s, %p [%li bytes]\n", file, line, func, new_ptr, size);
        #else
            WLX_UNUSED(file); WLX_UNUSED(line); WLX_UNUSED(func);
        #endif
    }
    return new_ptr;
}

#define wlx_free(pointer) wlx_free_impl(pointer, __FILE__, __LINE__, __FUNCTION__)
static inline void wlx_free_impl(void* ptr, const char *file, int line, const char *func) {
    #ifdef WLX_MEMORY_DEBUG
        printf("%s:%d: Deallocated = %s, %p\n", file, line, func, ptr);
    #else
        WLX_UNUSED(file); WLX_UNUSED(line); WLX_UNUSED(func);
    #endif
    free(ptr);
}

// Some handy macros from Alexey Kutepov - https://github.com/tsoding/nob.h
#define WLX_DA_INIT_CAP 256
#define wlx_da_reserve(da, expected_capacity)                                          \
    do {                                                                                    \
        if ((expected_capacity) > (da)->capacity) {                                         \
            if ((da)->capacity == 0) {                                                      \
                    (da)->capacity = WLX_DA_INIT_CAP;                                 \
            }                                                                               \
            while ((expected_capacity) > (da)->capacity) {                                  \
                (da)->capacity *= 2;                                                        \
            }                                                                               \
            (da)->items = wlx_realloc((da)->items, (da)->capacity * sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Unable to allocate more RAM");                   \
        }                                                                                   \
    } while (0)

#define wlx_da_append(da, item)            \
    do {                                        \
        wlx_da_reserve((da), (da)->count + 1); \
        (da)->items[(da)->count++] = (item);    \
    } while (0)

// ============================================================================
// Core geometry and backend interface
// ============================================================================

typedef struct {
    float x;
    float y;
    float w;
    float h;
} WLX_Rect;

// Opaque font identifier.  0 = default / unset.
typedef uintptr_t WLX_Font;
#define WLX_FONT_DEFAULT ((WLX_Font)0)

// Bundled text rendering parameters - replaces loose
// (font_size, spacing, color) arguments in backend callbacks and text helpers.
typedef struct {
    WLX_Font  font;      // 0 -> backend default font
    int      font_size;  // 0 -> resolve from theme
    int      spacing;    // 0 -> resolve from theme
    WLX_Color color;     // {0,0,0,0} -> resolve from theme foreground
} WLX_Text_Style;

#define WLX_TEXT_STYLE_DEFAULT \
    ((WLX_Text_Style){ .font = WLX_FONT_DEFAULT, .font_size = 0, .spacing = 0, .color = {0} })

typedef struct {
    void (*draw_rect)(WLX_Rect rect, WLX_Color color);
    void (*draw_rect_lines)(WLX_Rect rect, float thick, WLX_Color color);
    void (*draw_rect_rounded)(WLX_Rect rect, float roundness, int segments, WLX_Color color);
    void (*draw_rect_rounded_lines)(WLX_Rect rect, float roundness, int segments, float thick, WLX_Color color);
    void (*draw_circle)(float cx, float cy, float radius, int segments, WLX_Color color); /* optional: NULL falls back to draw_rect_rounded */
    void (*draw_ring)(float cx, float cy, float inner_r, float outer_r, int segments, WLX_Color color); /* optional: NULL falls back to draw_rect_rounded_lines */
    void (*draw_line)(float x1, float y1, float x2, float y2, float thick, WLX_Color color);
    void (*draw_text)(const char *text, float x, float y, WLX_Text_Style style);
    void (*measure_text)(const char *text, WLX_Text_Style style, float *out_w, float *out_h);
    void (*draw_texture)(WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint);
    // Scissor contract: begin_scissor installs rect as the current clip.
    // Core code explicitly restores parent clipping after ending nested child
    // regions, so backends must not rely on implicit push/pop clip stacks.
    void (*begin_scissor)(WLX_Rect rect);
    void (*end_scissor)(void);
    float (*get_frame_time)(void);
} WLX_Backend;

// ============================================================================
// Public enums
// ============================================================================

typedef enum {
    WLX_HORZ,
    WLX_VERT,
} WLX_Orient;

typedef enum {
    WLX_SIZE_AUTO,
    WLX_SIZE_PIXELS,
    WLX_SIZE_PERCENT,
    WLX_SIZE_FLEX,
    WLX_SIZE_FILL,
    WLX_SIZE_CONTENT,
} WLX_Size_Kind;

typedef struct {
    WLX_Size_Kind kind;
    float value;
    float min;   // 0 = unconstrained
    float max;   // 0 = unconstrained
} WLX_Slot_Size;

#define WLX_SLOT_AUTO ((WLX_Slot_Size){ WLX_SIZE_AUTO, 0, 0, 0 })
#define WLX_SLOT_PX(px) ((WLX_Slot_Size){ WLX_SIZE_PIXELS, (px), 0, 0 })
#define WLX_SLOT_PCT(pct) ((WLX_Slot_Size){ WLX_SIZE_PERCENT, (pct), 0, 0 })
#define WLX_SLOT_FLEX(w) ((WLX_Slot_Size){ WLX_SIZE_FLEX, (w), 0, 0 })

// Constrained slot size variants
#define WLX_SLOT_PX_MINMAX(px, lo, hi) ((WLX_Slot_Size){ WLX_SIZE_PIXELS, (px), (lo), (hi) })
#define WLX_SLOT_FLEX_MIN(w, lo)        ((WLX_Slot_Size){ WLX_SIZE_FLEX, (w), (lo), 0 })
#define WLX_SLOT_FLEX_MAX(w, hi)        ((WLX_Slot_Size){ WLX_SIZE_FLEX, (w), 0, (hi) })
#define WLX_SLOT_FLEX_MINMAX(w, lo, hi) ((WLX_Slot_Size){ WLX_SIZE_FLEX, (w), (lo), (hi) })
#define WLX_SLOT_AUTO_MIN(lo)           ((WLX_Slot_Size){ WLX_SIZE_AUTO, 0, (lo), 0 })
#define WLX_SLOT_AUTO_MAX(hi)           ((WLX_Slot_Size){ WLX_SIZE_AUTO, 0, 0, (hi) })
#define WLX_SLOT_AUTO_MINMAX(lo, hi)    ((WLX_Slot_Size){ WLX_SIZE_AUTO, 0, (lo), (hi) })
#define WLX_SLOT_PCT_MINMAX(pct, lo, hi)((WLX_Slot_Size){ WLX_SIZE_PERCENT, (pct), (lo), (hi) })

// Viewport-fill: resolve against the innermost scroll panel viewport
// (or root rect if no scroll panel). value = fraction (1.0 = full viewport).
#define WLX_SLOT_FILL                  ((WLX_Slot_Size){ WLX_SIZE_FILL, 1.0f, 0, 0 })
#define WLX_SLOT_FILL_PCT(p)           ((WLX_Slot_Size){ WLX_SIZE_FILL, (p) / 100.0f, 0, 0 })
#define WLX_SLOT_FILL_MIN(lo)          ((WLX_Slot_Size){ WLX_SIZE_FILL, 1.0f, (lo), 0 })
#define WLX_SLOT_FILL_MAX(hi)          ((WLX_Slot_Size){ WLX_SIZE_FILL, 1.0f, 0, (hi) })
#define WLX_SLOT_FILL_MINMAX(lo, hi)   ((WLX_Slot_Size){ WLX_SIZE_FILL, 1.0f, (lo), (hi) })

// Content-fit: slot height is determined by the child's preferred height
// (measured from the previous frame). min/max constrain the measured value.
#define WLX_SLOT_CONTENT                ((WLX_Slot_Size){ WLX_SIZE_CONTENT, 0, 0, 0 })
#define WLX_SLOT_CONTENT_MIN(lo)        ((WLX_Slot_Size){ WLX_SIZE_CONTENT, 0, (lo), 0 })
#define WLX_SLOT_CONTENT_MAX(hi)        ((WLX_Slot_Size){ WLX_SIZE_CONTENT, 0, 0, (hi) })
#define WLX_SLOT_CONTENT_MINMAX(lo, hi) ((WLX_Slot_Size){ WLX_SIZE_CONTENT, 0, (lo), (hi) })

// Helper: check if a slot size is the all-zero sentinel (unset)
static inline bool wlx_slot_size_is_zero(WLX_Slot_Size s) {
    return s.kind == 0 && s.value == 0.0f && s.min == 0.0f && s.max == 0.0f;
}

// Auto-counting sizes helper. Expands to two comma-separated arguments:
// the element count (size_t) and a WLX_Slot_Size[] compound literal pointer.
// Use with wlx_layout_begin_s():
//
//   wlx_layout_begin_s(ctx, WLX_VERT,
//       WLX_SIZES(WLX_SLOT_PX(44), WLX_SLOT_FLEX(1), WLX_SLOT_PX(24)));
//
// NOTE: arguments are evaluated twice (sizeof + literal). Only use with
// side-effect-free expressions (all WLX_SLOT_* macros are safe).
// The outer (()) around the compound literal protects internal commas from
// the preprocessor when passed through variadic macro arguments.
#define WLX_SIZES(...) \
    (sizeof((WLX_Slot_Size[]){ __VA_ARGS__ }) / sizeof(WLX_Slot_Size)), \
    ((WLX_Slot_Size[]){ __VA_ARGS__ })

typedef enum {
    WLX_ALIGN_NONE,
    WLX_TOP,
    WLX_BOTTOM,
    WLX_LEFT,
    WLX_RIGHT,
    WLX_CENTER,
    WLX_TOP_LEFT,
    WLX_TOP_RIGHT,
    WLX_TOP_CENTER,
    WLX_BOTTOM_LEFT,
    WLX_BOTTOM_RIGHT,
    WLX_BOTTOM_CENTER,
} WLX_Align;

typedef enum {
    WLX_KEY_NONE = 0,
    WLX_KEY_ESCAPE,
    WLX_KEY_ENTER,
    WLX_KEY_BACKSPACE,
    WLX_KEY_TAB,
    WLX_KEY_SPACE,
    WLX_KEY_LEFT,
    WLX_KEY_RIGHT,
    WLX_KEY_UP,
    WLX_KEY_DOWN,
    WLX_KEY_A, WLX_KEY_B, WLX_KEY_C, WLX_KEY_D, WLX_KEY_E, WLX_KEY_F, WLX_KEY_G, WLX_KEY_H,
    WLX_KEY_I, WLX_KEY_J, WLX_KEY_K, WLX_KEY_L, WLX_KEY_M, WLX_KEY_N, WLX_KEY_O, WLX_KEY_P,
    WLX_KEY_Q, WLX_KEY_R, WLX_KEY_S, WLX_KEY_T, WLX_KEY_U, WLX_KEY_V, WLX_KEY_W, WLX_KEY_X,
    WLX_KEY_Y, WLX_KEY_Z,
    WLX_KEY_0, WLX_KEY_1, WLX_KEY_2, WLX_KEY_3, WLX_KEY_4, WLX_KEY_5, WLX_KEY_6, WLX_KEY_7, WLX_KEY_8, WLX_KEY_9,
    WLX_KEY_COUNT
} WLX_Key_Code;

// ============================================================================
// Core state and context types
// ============================================================================

typedef struct {
    int mouse_x;
    int mouse_y;
    bool mouse_down;
    bool mouse_clicked; // true for one frame when clicked
    bool mouse_held;    // true while mouse button is held down
    float wheel_delta;  // mouse wheel movement this frame (positive = up, negative = down)
    bool keys_down[WLX_KEY_COUNT];     // current key states (held down)
    bool keys_pressed[WLX_KEY_COUNT];  // true for one frame when key pressed
    char text_input[32];    // text input this frame (for typing)

} WLX_Input_State;

// Persistent state for layouts that contain WLX_SIZE_CONTENT slots.
// Stores per-slot measured heights from the previous frame.
typedef struct {
    float measured[WLX_CONTENT_SLOTS_MAX];
} WLX_Content_Slot_State;

typedef enum {
    WLX_LAYOUT_LINEAR,
    WLX_LAYOUT_GRID,
} WLX_Layout_Kind;

typedef struct WLX_Layout {
    WLX_Layout_Kind kind;
    WLX_Rect rect;
    size_t count;
    size_t index;
    bool overflow;
    float accumulated_content_height;
    float padding;         // original uniform inset padding
    float padding_top;     // resolved top padding (for content-height propagation)
    float padding_bottom;  // resolved bottom padding (for content-height propagation)
    float gap;             // inter-slot spacing (stored for dynamic append + content-height)
    WLX_Color slot_back_color;
    WLX_Color slot_border_color;
    float     slot_border_width;
    float viewport;  // viewport dimension along orient axis (for WLX_SIZE_FILL)
    int cmd_range_idx;     // command range table index for this layout (-1 = none)
    bool pushed_scope_id;  // true when a scope id was pushed at begin and must be popped at end

    // Content-fit tracking (NULL when no CONTENT slots)
    const WLX_Slot_Size *content_sizes;     // original sizes array (for kind check in layout_end)
    size_t content_sizes_scratch_off;       // byte offset into ctx->scratch for content_sizes
    WLX_Content_Slot_State *content_state;  // persistent state pointer
    size_t content_slot_heights_off;        // index into slot_size_offsets for content_slot_heights
    bool has_content_slot_heights;          // true when content_slot_heights_off is valid
    size_t grid_row_content_heights_off;    // index into slot_size_offsets for grid_row_content_heights
    bool has_grid_row_content_heights;      // true when grid_row_content_heights_off is valid

    union {
        struct {
            WLX_Orient orient;
            // auto-sizing / dynamic append
            bool   dynamic;
            float  slot_size;
            size_t slot_size_offsets_base;
            float  next_slot_size;
            // per-slot style override (one-shot, consumed by wlx_get_slot_rect)
            WLX_Color next_slot_back_color;
            WLX_Color next_slot_border_color;
            float     next_slot_border_width;
        } linear;

        struct {
            size_t rows;
            size_t cols;
            // auto-advance cursor
            size_t cursor_row;
            size_t cursor_col;
            // per-cell override (consumed by next child)
            bool   cell_set;
            size_t next_row;
            size_t next_col;
            size_t next_row_span;
            size_t next_col_span;
            // dynamic (auto-sizing) grid: rows grow on demand
            bool   dynamic;
            float  row_size;            // fixed px per row
            size_t row_offsets_base;    // index in scratch buffer where row_offsets starts
            size_t col_offsets_base;    // index in scratch buffer where col_offsets starts
            float  next_row_size;       // per-row override
            size_t last_placed_row;     // row of the most recently placed cell
            // per-cell style override (one-shot, consumed by wlx_get_slot_rect)
            WLX_Color next_cell_back_color;
            WLX_Color next_cell_border_color;
            float     next_cell_border_width;
        } grid;
    };
} WLX_Layout;

// Growable slot sizes buffer for layout slot offsets - reused across frames,
// reset each frame in wlx_begin(). Layouts reserve slices from this buffer
// via wlx_scratch_alloc(), so there is zero per-layout heap traffic.
typedef struct {
    float *items;
    size_t count;
    size_t capacity;
} WLX_Scratch_Offsets;

// Generic byte-level scratch arena - same lifetime as WLX_Scratch_Offsets
// (reset each frame, grows as needed, never freed until context_destroy).
// Used for frame-local allocations that aren't float arrays (e.g. copies
// of WLX_Slot_Size arrays that must outlive the caller's stack frame).
typedef struct {
    uint8_t *items;
    size_t count;      // bytes used
    size_t capacity;   // bytes allocated
} WLX_Scratch_Bytes;

// Optional runtime allocator for frame-managed buffers. When omitted, callers
// continue using the compile-time wlx_alloc/wlx_realloc/wlx_free overrides.
typedef struct WLX_Allocator {
    void *(*alloc)(size_t size, void *user);
    void *(*realloc)(void *ptr, size_t old_size, size_t new_size, void *user);
    void  (*free)(void *ptr, size_t size, void *user);
    void *user;
} WLX_Allocator;

// Generic growable arena for future frame-pool migration. Count/capacity are
// in elements, except when item_size == 1 and alloc_bytes is used.
typedef struct {
    void *items;
    size_t count;
    size_t capacity;
    size_t item_size;
    size_t high_water;
    WLX_Allocator *allocator;
} WLX_Sub_Arena;

#define wlx_sub_arena_at(sa, type, index) \
    (&((type *)((sa)->items))[(index)])

#define wlx_sub_arena_bytes_at(sa, type, byte_off) \
    ((type *)((uint8_t *)((sa)->items) + (byte_off)))

static inline void wlx_sub_arena_init(WLX_Sub_Arena *sa, size_t item_size,
    WLX_Allocator *allocator)
{
    assert(sa != NULL);
    assert(item_size > 0 && "WLX_Sub_Arena item_size must be > 0");
    sa->items = NULL;
    sa->count = 0;
    sa->capacity = 0;
    sa->item_size = item_size;
    sa->high_water = 0;
    sa->allocator = allocator;
}

static inline void wlx_sub_arena_reserve(WLX_Sub_Arena *sa, size_t needed) {
    size_t new_capacity;
    size_t old_bytes;
    size_t new_bytes;
    void *new_items = NULL;

    assert(sa != NULL);
    assert(sa->item_size > 0 && "WLX_Sub_Arena item_size must be > 0");

    if (needed <= sa->capacity) return;

    new_capacity = sa->capacity == 0 ? WLX_DA_INIT_CAP : sa->capacity;
    while (needed > new_capacity) {
        assert(new_capacity <= SIZE_MAX / 2 && "size_t overflow in sub-arena reserve");
        new_capacity *= 2;
    }

    assert(new_capacity <= SIZE_MAX / sa->item_size && "size_t overflow in sub-arena bytes");
    old_bytes = sa->capacity * sa->item_size;
    new_bytes = new_capacity * sa->item_size;

    if (sa->allocator != NULL) {
        if (sa->items == NULL && sa->allocator->alloc != NULL) {
            new_items = sa->allocator->alloc(new_bytes, sa->allocator->user);
        } else if (sa->allocator->realloc != NULL) {
            new_items = sa->allocator->realloc(sa->items, old_bytes, new_bytes,
                sa->allocator->user);
        } else if (sa->allocator->alloc != NULL) {
            new_items = sa->allocator->alloc(new_bytes, sa->allocator->user);
            if (new_items != NULL && sa->items != NULL && old_bytes > 0) {
                memcpy(new_items, sa->items, old_bytes);
                if (sa->allocator->free != NULL) {
                    sa->allocator->free(sa->items, old_bytes, sa->allocator->user);
                }
            }
        }
    } else {
        new_items = wlx_realloc(sa->items, new_bytes);
    }

    assert(new_items != NULL && "Unable to allocate more RAM");
    sa->items = new_items;
    sa->capacity = new_capacity;
}

static inline size_t wlx_sub_arena_alloc(WLX_Sub_Arena *sa, size_t n) {
    size_t base;

    assert(sa != NULL);
    assert(n == 0 || sa->count <= SIZE_MAX - n);

    base = sa->count;
    wlx_sub_arena_reserve(sa, sa->count + n);
    sa->count += n;
    if (sa->count > sa->high_water) sa->high_water = sa->count;
    return base;
}

static inline size_t wlx_sub_arena_alloc_bytes(WLX_Sub_Arena *sa,
    size_t size, size_t align)
{
    size_t mask;
    size_t aligned;
    size_t needed;

    assert(sa != NULL);
    assert(sa->item_size == 1 && "wlx_sub_arena_alloc_bytes requires item_size == 1");

    if (align == 0) align = 1;
    assert((align & (align - 1)) == 0 && "align must be a power of two");

    mask = align - 1;
    assert(sa->count <= SIZE_MAX - mask);
    aligned = (sa->count + mask) & ~mask;
    assert(size <= SIZE_MAX - aligned);
    needed = aligned + size;

    wlx_sub_arena_reserve(sa, needed);
    sa->count = needed;
    if (sa->count > sa->high_water) sa->high_water = sa->count;
    return aligned;
}

static inline void wlx_sub_arena_reset(WLX_Sub_Arena *sa) {
    assert(sa != NULL);
    sa->count = 0;
}

static inline void wlx_sub_arena_destroy(WLX_Sub_Arena *sa) {
    assert(sa != NULL);
    if (sa->items != NULL) {
        size_t bytes = sa->capacity * sa->item_size;
        if (sa->allocator != NULL) {
            if (sa->allocator->free != NULL) {
                sa->allocator->free(sa->items, bytes, sa->allocator->user);
            }
        } else {
            wlx_free(sa->items);
        }
    }
    wlx_zero_struct(*sa);
}

// Frame-arena pool: owns every per-frame buffer on WLX_Context. The pool
// groups sub-arenas by allocator strategy (contiguous vs general). When
// either group is configured with a NULL allocator, the underlying sub-arenas
// fall back to the compile-time wlx_realloc / wlx_free macros. See
// docs/dev/state/MEMORY_MANAGEMENT_D1_A3_ANALYSIS.md for the rationale.

// Forward declarations for sub-arena element types defined later.
typedef struct WLX_Layout WLX_Layout;
typedef struct WLX_Cmd WLX_Cmd;
typedef struct WLX_Cmd_Range WLX_Cmd_Range;
typedef struct WLX_Scroll_Panel_State WLX_Scroll_Panel_State;

typedef struct {
    WLX_Allocator *contiguous;  // backs flat float offset arrays
    WLX_Allocator *general;     // backs scratch, commands, layouts, stacks
} WLX_Arena_Pool_Config;

// X-macro enumerating all nine sub-arenas in WLX_Arena_Pool.
// Columns: (name, item_size, alloc_group)
//   name        - field on WLX_Arena_Pool; accessed as pool->name / ctx->arena.name
//   item_size   - byte size per element passed to wlx_sub_arena_init
//   alloc_group - name of a local WLX_Allocator * variable in wlx_arena_pool_init
//                 (contig = contiguous group, gen = general group)
#define WLX_ARENA_POOL_FIELDS(X) \
    X(slot_size_offsets, sizeof(float),                    contig) \
    X(dyn_offsets,       sizeof(float),                    contig) \
    X(scratch,           1,                                gen)    \
    X(commands,          sizeof(WLX_Cmd),                  gen)    \
    X(cmd_ranges,        sizeof(WLX_Cmd_Range),            gen)    \
    X(layouts,           sizeof(WLX_Layout),               gen)    \
    X(scroll_panels,     sizeof(WLX_Scroll_Panel_State *), gen)    \
    X(id_stack,          sizeof(size_t),                   gen)    \
    X(opacity_stack,     sizeof(float),                    gen)

typedef struct {
#define WLX__ARENA_FIELD(name, item_size, alloc_group) WLX_Sub_Arena name;
    WLX_ARENA_POOL_FIELDS(WLX__ARENA_FIELD)
#undef WLX__ARENA_FIELD
} WLX_Arena_Pool;

// Typed views into the pool sub-arenas. Each helper re-derives the typed
// pointer from the current sub-arena `items` field, so callers must not
// cache the result across operations that may grow the underlying buffer
// (wlx_sub_arena_reserve / _alloc / _alloc_bytes).
typedef struct WLX_Context WLX_Context;

#define wlx_pool_layouts(ctx)        ((WLX_Layout *)(ctx)->arena.layouts.items)
#define wlx_pool_commands(ctx)       ((WLX_Cmd *)(ctx)->arena.commands.items)
#define wlx_pool_cmd_ranges(ctx)     ((WLX_Cmd_Range *)(ctx)->arena.cmd_ranges.items)
#define wlx_pool_scratch(ctx)        ((uint8_t *)(ctx)->arena.scratch.items)
#define wlx_pool_slot_size_offsets(ctx)   ((float *)(ctx)->arena.slot_size_offsets.items)
#define wlx_pool_dyn_offsets(ctx)        ((float *)(ctx)->arena.dyn_offsets.items)
#define wlx_pool_scroll_panels(ctx)  ((WLX_Scroll_Panel_State **)(ctx)->arena.scroll_panels.items)
#define wlx_pool_id_stack(ctx)       ((size_t *)(ctx)->arena.id_stack.items)
#define wlx_pool_opacity_stack(ctx)  ((float *)(ctx)->arena.opacity_stack.items)

#define wlx_pool_push(sa, type, item) do { \
    size_t _wlx_idx = wlx_sub_arena_alloc((sa), 1); \
    *wlx_sub_arena_at((sa), type, _wlx_idx) = (item); \
} while (0)

// ============================================================================
// Draw command replay types (strict deferral)
// ============================================================================

typedef enum {
    WLX_CMD_RECT,
    WLX_CMD_RECT_LINES,
    WLX_CMD_RECT_ROUNDED,
    WLX_CMD_RECT_ROUNDED_LINES,
    WLX_CMD_CIRCLE,
    WLX_CMD_RING,
    WLX_CMD_LINE,
    WLX_CMD_TEXT,
    WLX_CMD_TEXTURE,
    WLX_CMD_SCISSOR_BEGIN,
    WLX_CMD_SCISSOR_END,
} WLX_Cmd_Type;

typedef struct WLX_Cmd {
    WLX_Cmd_Type type;
    union {
        struct { WLX_Rect rect; WLX_Color color; } rect;
        struct { WLX_Rect rect; float thick; WLX_Color color; } rect_lines;
        struct { WLX_Rect rect; float roundness; int segments; WLX_Color color; } rect_rounded;
        struct { WLX_Rect rect; float roundness; int segments; float thick; WLX_Color color; } rect_rounded_lines;
        struct { float cx; float cy; float radius; int segments; WLX_Color color; } circle;
        struct { float cx; float cy; float inner_r; float outer_r; int segments; WLX_Color color; } ring;
        struct { float x1; float y1; float x2; float y2; float thick; WLX_Color color; } line;
        struct { size_t text_off; float x; float y; WLX_Text_Style style; } text;
        struct { WLX_Texture texture; WLX_Rect src; WLX_Rect dst; WLX_Color tint; } texture;
        struct { WLX_Rect rect; } scissor_begin;
    } data;
} WLX_Cmd;

#define WLX_NO_RANGE (-1)

typedef struct WLX_Cmd_Range {
    size_t start_idx;
    size_t end_idx;
    float  dy_offset;
    int    parent_range_idx;   // WLX_NO_RANGE for root
} WLX_Cmd_Range;

typedef struct {
    WLX_Cmd *items;
    size_t count;
    size_t capacity;
} WLX_Commands;

typedef struct {
    WLX_Cmd_Range *items;
    size_t count;
    size_t capacity;
} WLX_Cmd_Ranges;

typedef struct {
    WLX_Layout *items;
    size_t count;
    size_t capacity;
} WLX_Layout_Stack;

typedef struct {
    size_t cursor_pos;
    float cursor_blink_time;
} WLX_Inputbox_State;

typedef struct WLX_Scroll_Panel_State {
    float scroll_offset;
    float content_height;
    bool auto_height;
    WLX_Rect panel_rect;
    bool dragging_scrollbar;  // true while dragging the scrollbar thumb
    float drag_offset;        // mouse offset from scrollbar thumb top when drag started
    bool hovered;             // mouse is over this panel's rect this frame
    float wheel_scroll_speed;
    const char *pushed_id;    // non-NULL when a scope id was pushed; popped in wlx_scroll_panel_end

    // Saved outer auto-scroll context (for nesting non-auto panels inside auto panels)
    size_t saved_auto_scroll_panel_id;
    float  saved_auto_scroll_total_height;
} WLX_Scroll_Panel_State;

// Generic persistent state map (open-addressing hashmap, power-of-2 sized)
#define WLX_STATE_MAP_INIT_CAP  64
#define WLX_STATE_MAP_MAX_LOAD  0.7f

typedef struct {
    size_t id;        // 0 = empty slot
    void *data;
    size_t data_size;
} WLX_State_Map_Slot;

typedef struct {
    WLX_State_Map_Slot *slots;
    size_t count;     // number of occupied slots
    size_t capacity;  // always power of 2 (slot array length)
} WLX_State_Map;

// ID stack for loop disambiguation - use wlx_push_id()/wlx_pop_id()
typedef struct {
    size_t *items;
    size_t count;
    size_t capacity;
} WLX_Id_Stack;

// Opacity stack - push/pop region-level opacity via wlx_push_opacity()/wlx_pop_opacity()
typedef struct {
    float *items;
    size_t count;
    size_t capacity;
} WLX_Opacity_Stack;

// Handle returned by wlx_get_state() - gives access to both the ID and the data pointer
typedef struct {
    size_t id;
    void *data;
} WLX_State;

// Stack of active scroll panel state pointers (for nested scroll panels)
typedef struct {
    WLX_Scroll_Panel_State **items;
    size_t count;
    size_t capacity;
} WLX_Scroll_Panel_Stack;

// ============================================================================
// Arena pool init/reset/destroy
// (defined here so sizeof(WLX_Cmd) and friends are complete)
// ============================================================================

static inline void wlx_arena_pool_init(WLX_Arena_Pool *pool,
    const WLX_Arena_Pool_Config *cfg)
{
    WLX_Allocator *contig = cfg ? cfg->contiguous : NULL;
    WLX_Allocator *gen    = cfg ? cfg->general    : NULL;
    assert(pool != NULL);
#define WLX__ARENA_INIT(name, item_size, alloc_group) wlx_sub_arena_init(&pool->name, (item_size), (alloc_group));
    WLX_ARENA_POOL_FIELDS(WLX__ARENA_INIT)
#undef WLX__ARENA_INIT
}

static inline void wlx_arena_pool_reset(WLX_Arena_Pool *pool) {
    assert(pool != NULL);
#define WLX__ARENA_RESET(name, item_size, alloc_group) wlx_sub_arena_reset(&pool->name);
    WLX_ARENA_POOL_FIELDS(WLX__ARENA_RESET)
#undef WLX__ARENA_RESET
}

static inline void wlx_arena_pool_destroy(WLX_Arena_Pool *pool) {
    assert(pool != NULL);
#define WLX__ARENA_DESTROY(name, item_size, alloc_group) wlx_sub_arena_destroy(&pool->name);
    WLX_ARENA_POOL_FIELDS(WLX__ARENA_DESTROY)
#undef WLX__ARENA_DESTROY
}

// ============================================================================
// Theme system
// ============================================================================

// Style guide constants - advisory defaults for widget sizing.
// Override before including wollix.h to customize.
#ifndef WLX_STYLE_ROW_HEIGHT
#define WLX_STYLE_ROW_HEIGHT        40
#endif
#ifndef WLX_STYLE_BUTTON_HEIGHT
#define WLX_STYLE_BUTTON_HEIGHT     44
#endif
#ifndef WLX_STYLE_INPUT_HEIGHT
#define WLX_STYLE_INPUT_HEIGHT      44
#endif
#ifndef WLX_STYLE_HEADING_FONT_SIZE
#define WLX_STYLE_HEADING_FONT_SIZE 20
#endif
#ifndef WLX_STYLE_BORDER_WIDTH
#define WLX_STYLE_BORDER_WIDTH      1.0f
#endif
#ifndef WLX_STYLE_ROUNDNESS
#define WLX_STYLE_ROUNDNESS         0.25f
#endif
#ifndef WLX_STYLE_CONTENT_PADDING
#define WLX_STYLE_CONTENT_PADDING   8
#endif

// Sentinel for optional float fields where negative values are legitimate
// (e.g. hover_brightness).  Use -1 sentinel for fields where negative is
// never valid (border_width, roundness).  Resolve checks: <= WLX_FLOAT_UNSET.
#define WLX_FLOAT_UNSET (-1e30f)

typedef struct {
    // ── Global colors ───────────────────────────────────────────────
    WLX_Color background;        // window / panel clear color
    WLX_Color foreground;        // default text color (front_color)
    WLX_Color surface;           // widget background (back_color for buttons, etc.)
    WLX_Color border;            // default border color
    float     border_width;      // default border width (0 = no border)
    WLX_Color accent;            // active / focused accent (fill bar, focus ring)

    // ── Text ────────────────────────────────────────────────────────
    WLX_Font font;               // default font for all widgets (0 = backend default)
    int   font_size;            // default font size for all widgets
    int   spacing;              // default text spacing

    // ── Geometry ────────────────────────────────────────────────────
    float padding;              // default inner padding for layout slots
    float roundness;            // default corner roundness (0 = sharp)
    int   rounded_segments;     // segment count for rounded drawing
    int   min_rounded_segments; // minimum segment floor for fully-round widgets (0 = no minimum)

    // ── Interaction feedback ────────────────────────────────────────
    float hover_brightness;     // brightness shift on hover

    // ── Opacity ─────────────────────────────────────────────────────
    float opacity;              // global opacity multiplier (<0 = unset sentinel, 0.0-1.0 = explicit)

    // ── Widget-specific overrides (zero = use globals) ──────────────
    struct {
        WLX_Color border_focus;  // {0} -> derive from accent
        WLX_Color cursor;        // {0} -> use foreground
        float     border_width;  // 0 -> use global border_width
    } input;

    struct {
        WLX_Color track;         // {0} -> derive from surface
        WLX_Color thumb;         // {0} -> use foreground
        WLX_Color label;         // {0} -> use foreground
        float    track_height;  // 0 -> default (6)
        float    thumb_width;   // 0 -> default (14)
    } slider;

    struct {
        WLX_Color check;         // {0} -> use foreground
        WLX_Color border;        // {0} -> use theme border
        float     border_width;  // 0 -> use global border_width
    } checkbox;

    struct {
        WLX_Color track;              // {0} -> derive from surface
        WLX_Color track_active;       // {0} -> derive from accent
        WLX_Color thumb;              // {0} -> use foreground
        float     track_height;       // 0 -> default (computed from widget height)
        float     track_to_height_ratio; // 0 -> default (2.0)
        float     thumb_inset_ratio;  // 0 -> default (0.15)
    } toggle;

    struct {
        WLX_Color ring;               // {0} -> use border
        WLX_Color fill;               // {0} -> use accent
        WLX_Color label;              // {0} -> use foreground
        float     border_width;       // 0 -> use global border_width
        float     selected_inset_ratio; // 0 -> default (0.25)
    } radio;

    struct {
        WLX_Color track;         // {0} -> fall back to slider.track
        WLX_Color fill;          // {0} -> fall back to accent
        float     track_height;  // 0 -> fall back to slider.track_height
    } progress;

    struct {
        WLX_Color bar;           // scrollbar thumb color
        float    width;         // scrollbar width (0 -> default 10)
    } scrollbar;
} WLX_Theme;

// Built-in theme presets (defined in WOLLIX_IMPLEMENTATION section)
extern const WLX_Theme wlx_theme_dark;
extern const WLX_Theme wlx_theme_light;
extern const WLX_Theme wlx_theme_glass;

// Helper: check if a color is the sentinel (all-zero = "use theme default")
static inline bool wlx_color_is_zero(WLX_Color c) {
    return c.r == 0 && c.g == 0 && c.b == 0 && c.a == 0;
}

// Returns true when x uses the -1 sentinel (negative-is-never-valid fields:
// border_width, roundness, scrollbar_width, opacity, padding, etc.).
static inline bool wlx_is_negative_unset(float x) {
    return x < 0;
}

// Returns true when x uses WLX_FLOAT_UNSET (fields where negative values are
// legitimate: hover_brightness, thumb_hover_brightness, scrollbar_hover_brightness).
static inline bool wlx_is_float_unset(float x) {
    return x <= WLX_FLOAT_UNSET;
}

static inline WLX_Color wlx_color_or(WLX_Color a, WLX_Color b) {
    return wlx_color_is_zero(a) ? b : a;
}

typedef struct WLX_Context {
    WLX_Rect rect;
    WLX_Backend backend;
    WLX_Input_State input;

    // Widget interaction state (hot = hovered, active = pressed/focused)
    struct {
        size_t hot_id;
        size_t active_id;
        bool   active_id_seen; // true if any widget matched active_id this frame
    } interaction;

    // Per-frame buffer pool. Owns layouts, commands, cmd_ranges, scratch,
    // slot offsets, scroll-panel stack, id stack, and opacity stack.
    WLX_Arena_Pool arena;

    WLX_State_Map states;

    // Auto scroll panel content height tracking
    struct {
        size_t panel_id;      // 0 = not tracking
        float  total_height;
    } auto_scroll;

    int current_range_idx;     // active range during recording, -1 when none
    bool immediate_mode;       // true = dispatch directly (today's behavior)

    // Theme - NULL means use &wlx_theme_dark (set automatically in wlx_begin)
    const WLX_Theme *theme;

    // Debug context - heap-allocated under WLX_DEBUG, NULL in release builds.
    // See the debug implementation section at the end of this file.
    struct WLX_Debug_Context *dbg;
} WLX_Context;

// Accessors that derive WLX_Layout offset pointers from the slot_size_offsets
// sub-arena on demand. These replace cached pointer fields on WLX_Layout so
// the layout record does not have to be repaired after a sub-arena realloc.
static inline float *wlx_layout_offsets(const WLX_Context *ctx, const WLX_Layout *l) {
    if (l->linear.dynamic)
        return wlx_pool_dyn_offsets(ctx) + l->linear.slot_size_offsets_base;
    return wlx_pool_slot_size_offsets(ctx) + l->linear.slot_size_offsets_base;
}

static inline float *wlx_grid_row_offsets(const WLX_Context *ctx, const WLX_Layout *l) {
    if (l->grid.dynamic)
        return wlx_pool_dyn_offsets(ctx) + l->grid.row_offsets_base;
    return wlx_pool_slot_size_offsets(ctx) + l->grid.row_offsets_base;
}

static inline float *wlx_grid_col_offsets(const WLX_Context *ctx, const WLX_Layout *l) {
    return wlx_pool_slot_size_offsets(ctx) + l->grid.col_offsets_base;
}

static inline float *wlx_layout_content_heights(const WLX_Context *ctx, const WLX_Layout *l) {
    return wlx_pool_slot_size_offsets(ctx) + l->content_slot_heights_off;
}

static inline float *wlx_grid_row_content_heights(const WLX_Context *ctx, const WLX_Layout *l) {
    return wlx_pool_slot_size_offsets(ctx) + l->grid_row_content_heights_off;
}

// Debug hook macros - expand to helper calls under WLX_DEBUG, no-ops otherwise.
// Defined here so core implementation functions can use them unconditionally.
// The actual helper functions are defined in the debug implementation section
// at the end of the file.
#ifdef WLX_DEBUG
  #define WLX_DBG(fn, ...) wlx_dbg_##fn(__VA_ARGS__)
#else
  #define WLX_DBG(fn, ...) ((void)0)
#endif

// ============================================================================
// Interaction flags and interaction results
// ============================================================================

// Interaction flags for wlx_get_interaction()
// Combine with bitwise OR to specify desired interaction behavior.
typedef enum {
    WLX_INTERACT_HOVER       = 1 << 0,  // Hover detection (sets hot_id)
    WLX_INTERACT_CLICK       = 1 << 1,  // Click-to-activate (button-like: press, release while hovering = clicked)
    WLX_INTERACT_FOCUS       = 1 << 2,  // Click-to-focus (input-like: stays focused until click elsewhere or enter)
    WLX_INTERACT_DRAG        = 1 << 3,  // Click-to-drag (slider-like: active while mouse held after click)
    WLX_INTERACT_KEYBOARD    = 1 << 4,  // Keyboard activation (space/enter when hot triggers clicked)
} WLX_Interact_Flags;

typedef struct {
    size_t id;
    bool hover;          // Mouse is over widget
    bool pressed;        // Mouse is currently down on this widget
    bool clicked;        // Click completed (CLICK mode) or keyboard activated
    bool focused;        // Has focus (FOCUS mode)
    bool active;         // Is the active widget (being pressed, dragged, or focused)
    bool just_focused;   // Became focused this frame
    bool just_unfocused; // Lost focus this frame
} WLX_Interaction;

typedef void (*WLX_Input_Handler)(WLX_Context *ctx);

// ============================================================================
// Core API declarations
// ============================================================================

// draw_circle and draw_ring are intentionally excluded from this check: they are optional.
// A NULL callback falls back to draw_rect_rounded / draw_rect_rounded_lines respectively.
static inline bool wlx_backend_is_ready(const WLX_Context *ctx) {
    return ctx != NULL &&
        ctx->backend.draw_rect != NULL &&
        ctx->backend.draw_rect_lines != NULL &&
        ctx->backend.draw_rect_rounded != NULL &&
        ctx->backend.draw_rect_rounded_lines != NULL &&
        ctx->backend.draw_line != NULL &&
        ctx->backend.draw_text != NULL &&
        ctx->backend.measure_text != NULL &&
        ctx->backend.draw_texture != NULL &&
        ctx->backend.begin_scissor != NULL &&
        ctx->backend.end_scissor != NULL &&
        ctx->backend.get_frame_time != NULL;
    }

WLXDEF WLX_Rect wlx_rect(float x, float y, float w, float h);
WLXDEF WLX_Rect wlx_rect_intersect(WLX_Rect a, WLX_Rect b);
WLXDEF WLX_Layout wlx_create_layout(WLX_Context *ctx, WLX_Rect r, size_t count, WLX_Orient orient, float gap);
WLXDEF WLX_Layout wlx_create_layout_auto(WLX_Context *ctx, WLX_Rect r, WLX_Orient orient, float slot_px);
WLXDEF WLX_Layout wlx_create_grid(WLX_Context *ctx, WLX_Rect r, size_t rows, size_t cols,
    const WLX_Slot_Size *row_sizes, const WLX_Slot_Size *col_sizes, float gap);
WLXDEF WLX_Layout wlx_create_grid_auto(WLX_Context *ctx, WLX_Rect r,
    size_t cols, float row_px, const WLX_Slot_Size *col_sizes, float gap);
WLXDEF WLX_Rect wlx_get_slot_rect(WLX_Context *ctx, WLX_Layout *l, int pos, size_t span);

WLXDEF WLX_Rect wlx_get_align_rect(WLX_Rect parent_rect, float width, float height, WLX_Align align);

// Parent rect queries - non-consuming peeks at the current layout container.
WLXDEF WLX_Rect wlx_get_parent_rect(WLX_Context *ctx);
WLXDEF WLX_Rect wlx_get_scroll_panel_viewport(WLX_Context *ctx);

WLXDEF bool wlx_is_key_down(WLX_Context *ctx, WLX_Key_Code key);
WLXDEF bool wlx_is_key_pressed(WLX_Context *ctx, WLX_Key_Code key);
WLXDEF bool wlx_point_in_rect(int px, int py, int x, int y, int w, int h);

// ---------------------------------------------------------------------------
// Identity model
// ---------------------------------------------------------------------------
// Wollix uses one shared hash formula for all identity purposes:
//
//   id = hash(file, line) ^ id_stack_hash
//
// Three conceptual roles map onto this single formula:
//
//   Widget ID  - uniquely identifies a single immediate-mode call site.
//                Derived automatically from __FILE__ / __LINE__ inside every
//                widget macro.  No caller action required unless the same
//                source line is reached more than once per frame (see below).
//
//   State ID   - key for persistent state returned by wlx_get_state().
//                Same formula as Widget ID; call-site stability keeps state
//                alive across frames without explicit keys.
//
//   Scope ID   - a container-level id that pushes onto the id stack for the
//                entire container body, disambiguating all descendant Widget
//                IDs and State IDs.  Set via the `.id` field on container
//                option structs (WLX_Layout_Opt, WLX_Grid_Opt,
//                WLX_Grid_Auto_Opt, WLX_Panel_Opt, WLX_Split_Opt, and
//                WLX_Scroll_Panel_Opt).  When set, the hashed id is pushed
//                at begin and popped at end automatically.
//
//   v0.x rule  - container `.id` acts as both Scope ID and State ID for the
//                container itself.  A separate `.state_id` field is deferred
//                until state and scope need to diverge in practice.
//
// Use wlx_push_id()/wlx_pop_id() directly only when you need loop-level or
// reusable-function-level disambiguation that container `.id` does not cover.
WLXDEF void wlx_push_id(WLX_Context *ctx, size_t id);
WLXDEF void wlx_pop_id(WLX_Context *ctx);

// Opacity stack - push region-level opacity that multiplies with theme and per-widget opacity.
// The effective opacity is: widget.opacity * theme.opacity * ctx_stack_opacity.
WLXDEF void  wlx_push_opacity(WLX_Context *ctx, float opacity);
WLXDEF void  wlx_pop_opacity(WLX_Context *ctx);
WLXDEF float wlx_get_opacity(const WLX_Context *ctx);

// Unified interaction handler - replaces get_widget_state/get_input_state/inline state
// Use `WLX_Interact_Flags` to specify desired behavior. Only use ONE of CLICK/FOCUS/DRAG.
// Widget IDs are hash(file, line) ^ id_stack_hash.  The id stack is modified
// automatically by container `.id` (Scope ID) and manually by wlx_push_id()/wlx_pop_id()
// when the same source line is reached multiple times (loops, reusable widget functions).
WLXDEF WLX_Interaction wlx_get_interaction(WLX_Context *ctx, WLX_Rect rect, uint32_t flags, const char *file, int line);

// Generic persistent state - returns a handle with the state's ID and a pointer
// to zero-initialized persistent data. The data survives across frames.
// State IDs are hash(file, line) ^ id_stack_hash - the same formula as Widget IDs.
// The id stack is modified automatically by container `.id` (Scope ID) and manually
// by wlx_push_id()/wlx_pop_id() when the same source line is reached multiple times.
WLXDEF WLX_State wlx_get_state_impl(WLX_Context *ctx, size_t state_size, const char *file, int line);
#define wlx_get_state(ctx, type) wlx_get_state_impl((ctx), sizeof(type), __FILE__, __LINE__)

WLXDEF void wlx_begin(WLX_Context *ctx, WLX_Rect r, WLX_Input_Handler input_handler);
WLXDEF void wlx_begin_immediate(WLX_Context *ctx, WLX_Rect r, WLX_Input_Handler input_handler);
WLXDEF void wlx_end(WLX_Context *ctx);
WLXDEF void wlx_context_init(WLX_Context *ctx);
WLXDEF void wlx_context_init_ex(WLX_Context *ctx, const WLX_Arena_Pool_Config *cfg);
WLXDEF void wlx_context_destroy(WLX_Context *ctx);

// ============================================================================
// Layout API
// ============================================================================

// Shared placement fields used by layout and widget option structs:
// - `pos`: target child slot index within the current parent layout. Use `-1`
//   to consume the next slot sequentially.
// - `span`: number of consecutive slots to occupy starting at `pos` (or the
//   next sequential slot when `pos == -1`).
// - `overflow`: when true, allow the resolved widget rect to use explicit
//   width/height directly from the slot origin instead of aligning within the
//   slot bounds.
// - `padding`: uniform inset applied to the resolved slot before the widget or
//   nested layout uses it.

#define WLX_LAYOUT_SLOT_FIELDS \
    int pos; \
    size_t span; \
    bool overflow; \
    float padding; \
    float padding_top; \
    float padding_right; \
    float padding_bottom; \
    float padding_left

#define WLX_LAYOUT_SLOT_DEFAULTS \
    .pos = -1, .span = 1, .overflow = false, .padding = 0, \
    .padding_top = -1.0f, .padding_right = -1.0f, \
    .padding_bottom = -1.0f, .padding_left = -1.0f

#define WLX_CONTAINER_DECOR_FIELDS \
    WLX_Color back_color; \
    WLX_Color border_color; \
    float     border_width; \
    float     roundness; \
    int       rounded_segments; \
    float     gap

#define WLX_CONTAINER_DECOR_DEFAULTS \
    .back_color = {0}, .border_color = {0}, \
    .border_width = 0, .roundness = 0, .rounded_segments = 0, \
    .gap = 0

#define WLX_SLOT_DECOR_FIELDS \
    WLX_Color slot_back_color; \
    WLX_Color slot_border_color; \
    float     slot_border_width

#define WLX_SLOT_DECOR_DEFAULTS \
    .slot_back_color = {0}, .slot_border_color = {0}, .slot_border_width = 0

#define WLX_PADDING_FIELDS \
    float padding; \
    float padding_top; \
    float padding_right; \
    float padding_bottom; \
    float padding_left

#define WLX_PADDING_DEFAULTS \
    .padding = -1.0f, \
    .padding_top = -1.0f, \
    .padding_right = -1.0f, \
    .padding_bottom = -1.0f, \
    .padding_left = -1.0f

#define WLX_RESOLVE_PADDING(opt) \
    wlx_resolve_padding((opt).padding, (opt).padding_top, (opt).padding_right, (opt).padding_bottom, (opt).padding_left)

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    const WLX_Slot_Size *sizes;
    WLX_CONTAINER_DECOR_FIELDS;
    WLX_SLOT_DECOR_FIELDS;
    // Scope ID: when non-NULL, scopes all descendants for the layout body.
    const char *id;
} WLX_Layout_Opt;

typedef struct {
    size_t    row_span;
    size_t    col_span;
    WLX_Color back_color;
    WLX_Color border_color;
    float     border_width;
} WLX_Slot_Style_Opt;

#define wlx_default_slot_style_opt(...) \
    (WLX_Slot_Style_Opt){ .row_span = 1, .col_span = 1, \
        .back_color = {0}, .border_color = {0}, .border_width = 0, \
        __VA_ARGS__ }

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    const WLX_Slot_Size *row_sizes;
    const WLX_Slot_Size *col_sizes;
    WLX_CONTAINER_DECOR_FIELDS;
    WLX_SLOT_DECOR_FIELDS;
    // Scope ID: when non-NULL, scopes all descendants for the grid body.
    const char *id;
} WLX_Grid_Opt;

#define wlx_default_grid_opt(...) \
    (WLX_Grid_Opt){ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        .row_sizes = NULL, .col_sizes = NULL, \
        WLX_CONTAINER_DECOR_DEFAULTS, \
        WLX_SLOT_DECOR_DEFAULTS, \
        __VA_ARGS__ \
    }

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    const WLX_Slot_Size *col_sizes;
    WLX_CONTAINER_DECOR_FIELDS;
    WLX_SLOT_DECOR_FIELDS;
    // Scope ID: when non-NULL, scopes all descendants for the grid body.
    const char *id;
} WLX_Grid_Auto_Opt;

#define wlx_default_grid_auto_opt(...) \
    (WLX_Grid_Auto_Opt){ \
        WLX_LAYOUT_SLOT_DEFAULTS, .col_sizes = NULL, \
        WLX_CONTAINER_DECOR_DEFAULTS, \
        WLX_SLOT_DECOR_DEFAULTS, \
        __VA_ARGS__ \
    }


#define wlx_default_layout_opt(...) \
    (WLX_Layout_Opt) { \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        .sizes = NULL, \
        WLX_CONTAINER_DECOR_DEFAULTS, \
        WLX_SLOT_DECOR_DEFAULTS, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_layout_begin_impl(WLX_Context *ctx, size_t count, WLX_Orient orient, WLX_Layout_Opt opt,
                                   const char *file, int line);
#define wlx_layout_begin(ctx, count, orient, ...) \
    wlx_layout_begin_impl((ctx), (count), (orient), wlx_default_layout_opt(__VA_ARGS__), __FILE__, __LINE__)

// Layout with auto-counted sizes - no manual count parameter needed.
// Pass WLX_SIZES(...) which expands to (count, sizes_ptr):
//
//   wlx_layout_begin_s(ctx, WLX_VERT,
//       WLX_SIZES(WLX_SLOT_PX(44), WLX_SLOT_FLEX(1), WLX_SLOT_PX(24)),
//       .padding = 4);
//
// Uses double-expansion so WLX_SIZES is expanded before argument counting.
#define wlx_layout_begin_s(ctx, orient, ...) \
    WLX__LAYOUT_BEGIN_S_EXPAND(ctx, orient, __VA_ARGS__)
#define WLX__LAYOUT_BEGIN_S_EXPAND(ctx, orient, count_val, sizes_ptr, ...) \
    wlx_layout_begin_impl((ctx), (count_val), (orient), \
        wlx_default_layout_opt(.sizes = (sizes_ptr), __VA_ARGS__), __FILE__, __LINE__)

WLXDEF void wlx_layout_begin_auto_impl(WLX_Context *ctx, WLX_Orient orient, float slot_px, WLX_Layout_Opt opt);
// Dynamic layout: slot count grows as children are added.
//   slot_px > 0  - fixed pixel size for every slot (height for `WLX_VERT`, width for `WLX_HORZ`).
//   slot_px = 0  - variable-size mode: each child must call `wlx_layout_auto_slot_px()` before it.
// Use two-pass counting for equal-division when count is unknown.
#define wlx_layout_begin_auto(ctx, orient, slot_px, ...) wlx_layout_begin_auto_impl((ctx), (orient), (slot_px), wlx_default_layout_opt(__VA_ARGS__))

// Set the size for the *next* slot in the enclosing dynamic layout.
// Accepts any WLX_Slot_Size - PX, PCT, FLEX, FILL, CONTENT (with min/max).
// Non-PX types are resolved to pixels immediately:
//   PX      -> exact value
//   PCT     -> percentage of layout total
//   FILL    -> fraction of viewport
//   FLEX    -> all remaining space (greedy; last FLEX wins if multiple used)
//   AUTO    -> all remaining space (greedy, same as FLEX(1))
//   CONTENT -> uses size.value as pre-resolved px (0 if not set)
// Call immediately before the widget whose slot size you want to control.
WLXDEF void wlx_layout_auto_slot(WLX_Context *ctx, WLX_Slot_Size size);

// Convenience: set the next slot to a fixed pixel size.
// Equivalent to `wlx_layout_auto_slot(ctx, WLX_SLOT_PX(px))`.
WLXDEF void wlx_layout_auto_slot_px(WLX_Context *ctx, float px);

// Override the pixel height for the *next* row in the enclosing dynamic grid.
// Call this immediately before the first widget of the row whose height differs
// from the grid's default row_px.  The override is consumed and cleared
// automatically when the new row is created.
WLXDEF void wlx_grid_auto_row_px(WLX_Context *ctx, float px);

WLXDEF void wlx_grid_cell_impl(WLX_Context *ctx, int row, int col, WLX_Slot_Style_Opt opt);
#define wlx_grid_cell(ctx, row, col, ...) \
    wlx_grid_cell_impl((ctx), (row), (col), wlx_default_slot_style_opt(__VA_ARGS__))

WLXDEF void wlx_slot_style_impl(WLX_Context *ctx, WLX_Slot_Style_Opt opt);
#define wlx_slot_style(ctx, ...) \
    wlx_slot_style_impl((ctx), wlx_default_slot_style_opt(__VA_ARGS__))

WLXDEF void wlx_grid_cell_style_impl(WLX_Context *ctx, WLX_Slot_Style_Opt opt);
#define wlx_grid_cell_style(ctx, ...) \
    wlx_grid_cell_style_impl((ctx), wlx_default_slot_style_opt(__VA_ARGS__))

WLXDEF void wlx_layout_end(WLX_Context *ctx);

WLXDEF void wlx_grid_begin_impl(WLX_Context *ctx, size_t rows, size_t cols, WLX_Grid_Opt opt,
                                const char *file, int line);
#define wlx_grid_begin(ctx, rows, cols, ...) \
    wlx_grid_begin_impl((ctx), (rows), (cols), wlx_default_grid_opt(__VA_ARGS__), __FILE__, __LINE__)

WLXDEF void wlx_grid_begin_auto_impl(WLX_Context *ctx, size_t cols, float row_px, WLX_Grid_Auto_Opt opt);
#define wlx_grid_begin_auto(ctx, cols, row_px, ...) \
    wlx_grid_begin_auto_impl((ctx), (cols), (row_px), wlx_default_grid_auto_opt(__VA_ARGS__))

// Convenience: tile grid where column count is derived from tile width.
// Equivalent to computing cols = floor(available_width / tile_w), then
// calling `wlx_grid_begin_auto(ctx, cols, tile_h, ...)`.
WLXDEF void wlx_grid_begin_auto_tile_impl(WLX_Context *ctx, float tile_w, float tile_h, WLX_Grid_Auto_Opt opt);
#define wlx_grid_begin_auto_tile(ctx, tile_w, tile_h, ...) \
    wlx_grid_begin_auto_tile_impl((ctx), (tile_w), (tile_h), wlx_default_grid_auto_opt(__VA_ARGS__))

#define wlx_grid_end(ctx) wlx_layout_end(ctx)

// ============================================================================
// Widget option structs and widget API
// ============================================================================

// See https://x.com/vkrajacic/status/1749816169736073295 for more info on how to use such macros.
#define WLX_WIDGET_SIZING_FIELDS \
    WLX_Align widget_align; \
    float width;      /* -1 use parent width  */ \
    float height;     /* -1 use parent height */ \
    float min_width;  /* 0 = unconstrained */ \
    float min_height; /* 0 = unconstrained */ \
    float max_width;  /* 0 = unconstrained */ \
    float max_height; /* 0 = unconstrained */ \
    float opacity       /* <0 = unset (sentinel), 0.0-1.0 = explicit */

#define WLX_WIDGET_SIZING_DEFAULTS \
    .widget_align = WLX_LEFT, .width = -1, .height = -1, \
    .min_width = 0, .min_height = 0, .max_width = 0, .max_height = 0, \
    .opacity = -1

#define WLX_TEXT_TYPOGRAPHY_FIELDS \
    WLX_Font font; \
    int font_size; \
    int spacing; \
    WLX_Align align; \
    bool wrap

#define WLX_TEXT_TYPOGRAPHY_DEFAULTS \
    .font = WLX_FONT_DEFAULT, .font_size = 0, .spacing = 0, .align = WLX_LEFT

#define WLX_TEXT_COLOR_FIELDS \
    WLX_Color front_color; \
    WLX_Color back_color

#define WLX_TEXT_COLOR_DEFAULTS \
    .front_color = {0}, .back_color = {0}

#define WLX_BORDER_FIELDS \
    WLX_Color border_color; \
    float     border_width; \
    float     roundness; \
    int       rounded_segments

#define WLX_BORDER_DEFAULTS \
    .border_color = {0}, .border_width = -1, \
    .roundness = -1, .rounded_segments = -1

typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // Color
    WLX_Color color;

    // Border
    WLX_BORDER_FIELDS;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Widget_Opt;

#define wlx_default_widget_opt(...) \
    (WLX_Widget_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        /* Color */ \
        .color = {0}, \
        /* Border */ \
        WLX_BORDER_DEFAULTS, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_widget_impl(WLX_Context *ctx, WLX_Widget_Opt opt, const char *file, int line);
#define wlx_widget(ctx, ...) wlx_widget_impl((ctx), wlx_default_widget_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // Typography
    WLX_TEXT_TYPOGRAPHY_FIELDS;

    // Styles
    WLX_TEXT_COLOR_FIELDS;
    bool show_background;

    // Border
    WLX_BORDER_FIELDS;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Label_Opt;


#define wlx_default_label_opt(...) \
    (WLX_Label_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        /* Typography */ \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        .wrap = true, \
        /* Styles */ \
        WLX_TEXT_COLOR_DEFAULTS, \
        .show_background = false, \
        /* Border */ \
        WLX_BORDER_DEFAULTS, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_label_impl(WLX_Context *ctx, const char *text, WLX_Label_Opt opt, const char *file, int line);
#define wlx_label(ctx, text, ...) wlx_label_impl((ctx), (text), wlx_default_label_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // Typography
    WLX_TEXT_TYPOGRAPHY_FIELDS;

    // Styles
    WLX_TEXT_COLOR_FIELDS;

    // Border
    WLX_BORDER_FIELDS;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Button_Opt;

#define wlx_default_button_opt(...) \
    (WLX_Button_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        /* Typography */ \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        .wrap = true, \
        /* Styles */ \
        WLX_TEXT_COLOR_DEFAULTS, \
        /* Border */ \
        WLX_BORDER_DEFAULTS, \
        __VA_ARGS__ \
    }

WLXDEF bool wlx_button_impl(WLX_Context *ctx, const char *text, WLX_Button_Opt opt, const char *file, int line);
#define wlx_button(ctx, text, ...) wlx_button_impl((ctx), (text), wlx_default_button_opt(__VA_ARGS__), __FILE__, __LINE__)


typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // Typography
    WLX_TEXT_TYPOGRAPHY_FIELDS;

    // Styles
    WLX_TEXT_COLOR_FIELDS;
    bool full_slot_hit;
    WLX_Color border_color;
    float     border_width;
    float     roundness;
    int       rounded_segments;
    WLX_Color check_color;

    // Texture mode (when tex_checked.width > 0, uses texture rendering)
    WLX_Texture tex_checked;
    WLX_Texture tex_unchecked;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Checkbox_Opt;

#define wlx_default_checkbox_opt(...) \
    (WLX_Checkbox_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        /* Typography */ \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        .wrap = false, \
        /* Styles */ \
        WLX_TEXT_COLOR_DEFAULTS, \
        .full_slot_hit = true, \
        .border_color = {0}, \
        .border_width = -1, \
        .roundness = -1, \
        .rounded_segments = -1, \
        .check_color = {0}, \
        __VA_ARGS__ \
    }

WLXDEF bool wlx_checkbox_impl(WLX_Context *ctx, const char *text, bool *checked, WLX_Checkbox_Opt opt, const char *file, int line);
#define wlx_checkbox(ctx, text, checked, ...) wlx_checkbox_impl((ctx), (text), (checked), wlx_default_checkbox_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;
    float content_padding;

    // Typography
    WLX_TEXT_TYPOGRAPHY_FIELDS;

    // Styles
    WLX_TEXT_COLOR_FIELDS;
    WLX_Color border_color;
    float     border_width;
    float     roundness;
    int       rounded_segments;
    WLX_Color border_focus_color;
    WLX_Color cursor_color;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Inputbox_Opt;

#define wlx_default_inputbox_opt(...) \
    (WLX_Inputbox_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        .content_padding = 10, \
        /* Typography */ \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        .wrap = true, \
        /* Styles */ \
        WLX_TEXT_COLOR_DEFAULTS, \
        .border_color = {0}, \
        .border_width = -1, \
        .roundness = -1, \
        .rounded_segments = -1, \
        .border_focus_color = {0}, \
        .cursor_color = {0}, \
        __VA_ARGS__ \
    }

WLXDEF bool wlx_inputbox_impl(WLX_Context *ctx, const char *label, char *buffer, size_t buffer_size, WLX_Inputbox_Opt opt, const char *file, int line);
#define wlx_inputbox(ctx, label, buffer, buffer_size, ...) wlx_inputbox_impl((ctx), (label), (buffer), (buffer_size), wlx_default_inputbox_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // Typography
    WLX_Font font;
    int font_size;
    int spacing;
    WLX_Align align;
    bool show_label;      // show value label next to slider

    // Styles
    WLX_Color track_color;
    WLX_Color thumb_color;
    WLX_Color label_color;
    float track_height;   // height of the track bar
    float thumb_width;    // width of the thumb handle
    float hover_brightness;
    float thumb_hover_brightness;
    float fill_inactive_brightness;
    float min_value;
    float max_value;

    // Border
    WLX_BORDER_FIELDS;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Slider_Opt;


#define wlx_default_slider_opt(...) \
    (WLX_Slider_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        /* Typography */ \
        .font = WLX_FONT_DEFAULT, \
        .font_size = 0, \
        .spacing = 0, \
        .align = WLX_LEFT, \
        .show_label = true, \
        /* Styles */ \
        .track_color = {0}, \
        .thumb_color = {0}, \
        .label_color = {0}, \
        .track_height = 0, \
        .thumb_width = 0, \
        .hover_brightness = WLX_FLOAT_UNSET, \
        .thumb_hover_brightness = WLX_FLOAT_UNSET, \
        .fill_inactive_brightness = -0.3f, \
        .min_value = 0.0f, \
        .max_value = 1.0f, \
        /* Border */ \
        WLX_BORDER_DEFAULTS, \
        __VA_ARGS__ \
    }

WLXDEF bool wlx_slider_impl(WLX_Context *ctx, const char *label, float *value, WLX_Slider_Opt opt, const char *file, int line);
#define wlx_slider(ctx, label, value, ...) wlx_slider_impl((ctx), (label), (value), wlx_default_slider_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    WLX_WIDGET_SIZING_FIELDS;
    WLX_Color color;
    float     thickness;
    const char *id;
} WLX_Separator_Opt;

#define wlx_default_separator_opt(...) \
    (WLX_Separator_Opt) { \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        WLX_WIDGET_SIZING_DEFAULTS, \
        .color = {0}, \
        .thickness = 1.0f, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_separator_impl(WLX_Context *ctx, WLX_Separator_Opt opt, const char *file, int line);
#define wlx_separator(ctx, ...) wlx_separator_impl((ctx), wlx_default_separator_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    WLX_WIDGET_SIZING_FIELDS;

    WLX_Color track_color;
    WLX_Color fill_color;
    float     track_height;       // 0 -> full widget height
    WLX_BORDER_FIELDS;

    const char *id;
} WLX_Progress_Opt;

#define wlx_default_progress_opt(...) \
    (WLX_Progress_Opt) { \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        WLX_WIDGET_SIZING_DEFAULTS, \
        .track_color = {0}, \
        .fill_color = {0}, \
        .track_height = 0, \
        WLX_BORDER_DEFAULTS, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_progress_impl(WLX_Context *ctx, float value, WLX_Progress_Opt opt, const char *file, int line);
#define wlx_progress(ctx, value, ...) wlx_progress_impl((ctx), (value), wlx_default_progress_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    WLX_WIDGET_SIZING_FIELDS;
    WLX_TEXT_TYPOGRAPHY_FIELDS;
    WLX_TEXT_COLOR_FIELDS;

    WLX_Color track_color;
    WLX_Color track_active_color;
    WLX_Color thumb_color;
    float     hover_brightness;
    WLX_BORDER_FIELDS;

    const char *id;
} WLX_Toggle_Opt;

#define wlx_default_toggle_opt(...) \
    (WLX_Toggle_Opt) { \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        WLX_WIDGET_SIZING_DEFAULTS, \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        .wrap = false, \
        WLX_TEXT_COLOR_DEFAULTS, \
        .track_color = {0}, \
        .track_active_color = {0}, \
        .thumb_color = {0}, \
        .hover_brightness = WLX_FLOAT_UNSET, \
        WLX_BORDER_DEFAULTS, \
        __VA_ARGS__ \
    }

WLXDEF bool wlx_toggle_impl(WLX_Context *ctx, const char *label, bool *value, WLX_Toggle_Opt opt, const char *file, int line);
#define wlx_toggle(ctx, label, value, ...) wlx_toggle_impl((ctx), (label), (value), wlx_default_toggle_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    WLX_WIDGET_SIZING_FIELDS;
    WLX_TEXT_TYPOGRAPHY_FIELDS;
    WLX_TEXT_COLOR_FIELDS;

    WLX_Color ring_color;
    WLX_Color fill_color;
    float     ring_border_width;
    float     hover_brightness;

    const char *id;
} WLX_Radio_Opt;

#define wlx_default_radio_opt(...) \
    (WLX_Radio_Opt) { \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        WLX_WIDGET_SIZING_DEFAULTS, \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        .wrap = false, \
        WLX_TEXT_COLOR_DEFAULTS, \
        .ring_color = {0}, \
        .fill_color = {0}, \
        .ring_border_width = -1, \
        .hover_brightness = WLX_FLOAT_UNSET, \
        __VA_ARGS__ \
    }

WLXDEF bool wlx_radio_impl(WLX_Context *ctx, const char *label, int *active, int index, WLX_Radio_Opt opt, const char *file, int line);
#define wlx_radio(ctx, label, active, index, ...) wlx_radio_impl((ctx), (label), (active), (index), wlx_default_radio_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // Styles
    WLX_Color back_color;
    WLX_Color scrollbar_color;
    float scrollbar_hover_brightness;
    float scrollbar_width;
    float wheel_scroll_speed;
    bool show_scrollbar;

    // Border
    WLX_BORDER_FIELDS;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Scroll_Panel_Opt;

#define wlx_default_scroll_panel_opt(...) \
    (WLX_Scroll_Panel_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        /* Styles */ \
        .back_color = {0}, \
        .scrollbar_color = {0}, \
        .scrollbar_hover_brightness = WLX_FLOAT_UNSET, \
        .scrollbar_width = -1, \
        .wheel_scroll_speed = 20.0f, \
        .show_scrollbar = true, \
        /* Border */ \
        WLX_BORDER_DEFAULTS, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_scroll_panel_begin_impl(WLX_Context *ctx, float content_height, WLX_Scroll_Panel_Opt opt, const char *file, int line);
#define wlx_scroll_panel_begin(ctx, content_height, ...) wlx_scroll_panel_begin_impl((ctx), (content_height), wlx_default_scroll_panel_opt(__VA_ARGS__), __FILE__, __LINE__)
WLXDEF void wlx_scroll_panel_end(WLX_Context *ctx);

// ============================================================================
// Compound widget: Split panel (two-pane split with independent scroll)
// ============================================================================

typedef struct {
    WLX_Slot_Size first_size;       // default: WLX_SLOT_PX(280)
    WLX_Slot_Size second_size;      // default: WLX_SLOT_FLEX(1)
    WLX_Slot_Size fill_size;        // default: WLX_SLOT_FILL
    WLX_PADDING_FIELDS;
    float gap;                      // inter-pane spacing (default: -1 sentinel -> 0)
    WLX_Color first_back_color;     // first pane scroll panel bg (default: theme)
    WLX_Color second_back_color;    // second pane scroll panel bg (default: theme)
    // Scope ID: when non-NULL, scopes all descendants for the split body.
    const char *id;
} WLX_Split_Opt;

#define wlx_default_split_opt(...) \
    (WLX_Split_Opt){ \
        .first_size       = {0}, \
        .second_size      = {0}, \
        .fill_size        = {0}, \
        WLX_PADDING_DEFAULTS, \
        .gap              = -1.0f, \
        .first_back_color = {0}, \
        .second_back_color = {0}, \
        __VA_ARGS__ \
    }

typedef struct {
    WLX_Color back_color;  // override second pane bg color
} WLX_Split_Next_Opt;

WLXDEF void wlx_split_begin_impl(WLX_Context *ctx, WLX_Split_Opt opt, const char *file, int line);
WLXDEF void wlx_split_next_impl(WLX_Context *ctx, WLX_Split_Next_Opt opt, const char *file, int line);
WLXDEF void wlx_split_end_impl(WLX_Context *ctx);

#define wlx_split_begin(ctx, ...) \
    wlx_split_begin_impl((ctx), wlx_default_split_opt(__VA_ARGS__), __FILE__, __LINE__)
#define wlx_split_next(ctx, ...) \
    wlx_split_next_impl((ctx), (WLX_Split_Next_Opt){ __VA_ARGS__ }, __FILE__, __LINE__)
#define wlx_split_end(ctx) \
    wlx_split_end_impl((ctx))

// ============================================================================
// Compound widget: Panel (titled layout with auto-counted CONTENT slots)
// ============================================================================

typedef struct {
    // Title text. NULL = emit no heading, skip title slot.
    const char *title;

    // Title styling (resolved to defaults if 0/{0})
    int       title_font_size;       // default: 18
    float     title_height;          // default: 32
    WLX_Align title_align;           // default: WLX_CENTER
    WLX_Color title_back_color;      // default: {0} (theme surface)

    // Layout options
    WLX_PADDING_FIELDS;
    float gap;                       // inter-slot spacing (default: 0)
    int   capacity;                  // max child count (default: 32)

    // Scope ID: when non-NULL, scopes all descendants for the panel body.
    const char *id;
} WLX_Panel_Opt;

#define wlx_default_panel_opt(...) \
    (WLX_Panel_Opt){ \
        .title            = NULL, \
        .title_font_size  = 0, \
        .title_height     = 0, \
        .title_align      = 0, \
        .title_back_color = {0}, \
        WLX_PADDING_DEFAULTS, \
        .gap              = 0.0f, \
        .capacity         = 0, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_panel_begin_impl(WLX_Context *ctx, WLX_Panel_Opt opt, const char *file, int line);
WLXDEF void wlx_panel_end(WLX_Context *ctx);

#define wlx_panel_begin(ctx, ...) \
    wlx_panel_begin_impl((ctx), wlx_default_panel_opt(__VA_ARGS__), __FILE__, __LINE__)

#ifdef WLX_SHORT_NAMES
#define layout_begin(ctx, count, orient, ...) wlx_layout_begin((ctx), (count), (orient), __VA_ARGS__)
#define layout_begin_s(ctx, orient, ...) wlx_layout_begin_s((ctx), (orient), __VA_ARGS__)
#define layout_begin_auto(ctx, orient, slot_px, ...) wlx_layout_begin_auto((ctx), (orient), (slot_px), __VA_ARGS__)
#define layout_auto_slot(ctx, size) wlx_layout_auto_slot((ctx), (size))
#define layout_auto_slot_px(ctx, px) wlx_layout_auto_slot_px((ctx), (px))
#define layout_end(ctx) wlx_layout_end((ctx))
#define grid_cell(ctx, row, col, ...) wlx_grid_cell((ctx), (row), (col), __VA_ARGS__)
#define grid_begin(ctx, rows, cols, ...) wlx_grid_begin((ctx), (rows), (cols), __VA_ARGS__)
#define grid_begin_auto(ctx, cols, row_px, ...) wlx_grid_begin_auto((ctx), (cols), (row_px), __VA_ARGS__)
#define grid_begin_auto_tile(ctx, tile_w, tile_h, ...) wlx_grid_begin_auto_tile((ctx), (tile_w), (tile_h), __VA_ARGS__)
#define grid_end(ctx) wlx_grid_end((ctx))
#define label(ctx, text, ...) wlx_label((ctx), (text), __VA_ARGS__)
#define button(ctx, text, ...) wlx_button((ctx), (text), __VA_ARGS__)
#define checkbox(ctx, text, checked, ...) wlx_checkbox((ctx), (text), (checked), __VA_ARGS__)
#define inputbox(ctx, label, buffer, buffer_size, ...) wlx_inputbox((ctx), (label), (buffer), (buffer_size), __VA_ARGS__)
#define slider(ctx, label, value, ...) wlx_slider((ctx), (label), (value), __VA_ARGS__)
#define separator(ctx, ...) wlx_separator((ctx), __VA_ARGS__)
#define progress(ctx, value, ...) wlx_progress((ctx), (value), __VA_ARGS__)
#define toggle(ctx, label, value, ...) wlx_toggle((ctx), (label), (value), __VA_ARGS__)
#define radio(ctx, label, active, index, ...) wlx_radio((ctx), (label), (active), (index), __VA_ARGS__)
#define scroll_panel_begin(ctx, content_height, ...) wlx_scroll_panel_begin((ctx), (content_height), __VA_ARGS__)
#define scroll_panel_end(ctx) wlx_scroll_panel_end((ctx))
#define split_begin(ctx, ...) wlx_split_begin((ctx), __VA_ARGS__)
#define split_next(ctx, ...) wlx_split_next((ctx), __VA_ARGS__)
#define split_end(ctx) wlx_split_end((ctx))
#define panel_begin(ctx, ...) wlx_panel_begin((ctx), __VA_ARGS__)
#define panel_end(ctx) wlx_panel_end((ctx))
#define widget(ctx, ...) wlx_widget((ctx), __VA_ARGS__)
#endif // WLX_SHORT_NAMES


#endif // WOLLIX_H_


////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
#ifdef WOLLIX_IMPLEMENTATION
#include <math.h>

// Forward declarations for debug hook functions (defined at end of file).
#ifdef WLX_DEBUG
static inline void wlx_dbg_init(WLX_Context *ctx);
static inline void wlx_dbg_destroy(WLX_Context *ctx);
static inline void wlx_dbg_warn(WLX_Context *ctx, const char *file, int line, const char *fmt, ...);
static inline bool wlx_dbg_warn_once(WLX_Context *ctx, const char *file, int line, const char *fmt, ...);
static inline void wlx_dbg_frame_begin(WLX_Context *ctx);
static inline void wlx_dbg_interaction_id(WLX_Context *ctx, size_t base, const char *file, int line);
static inline void wlx_dbg_layout_begin(WLX_Context *ctx, int vb_force,
    const WLX_Slot_Size *sizes, size_t count, int orient,
    int pos, int span, const char *file, int line);
static inline void wlx_dbg_layout_end(WLX_Context *ctx);
static inline void wlx_dbg_auto_slot(WLX_Context *ctx, float px, int kind, float total, float used);
static inline void wlx_dbg_widget_begin(WLX_Context *ctx, WLX_Rect cell, float height, int span, bool overflow, const char *file, int line);
static inline void wlx_dbg_split_begin(WLX_Context *ctx);
static inline void wlx_dbg_split_suppress_warn(WLX_Context *ctx, bool suppress);
static inline void wlx_dbg_split_next(WLX_Context *ctx);
static inline void wlx_dbg_split_end(WLX_Context *ctx);
#endif // WLX_DEBUG



// ============================================================================
// Theme preset definitions
// ============================================================================

const WLX_Theme wlx_theme_dark = {
    .background       = { 27,  27,  27, 255},
    .foreground       = {200, 200, 200, 255},
    .surface          = { 40,  40,  40, 255},
    .border           = { 60,  60,  60, 255},
    .border_width     = 0,
    .accent           = { 90, 140, 210, 255},
    .font              = WLX_FONT_DEFAULT,
    .font_size        = 16,
    .spacing          = 2,
    .padding          = 0,
    .roundness        = 0,
    .rounded_segments = 0,
    .min_rounded_segments = 16,
    .hover_brightness = 0.08f,
    .opacity          = -1,
    .input = {
        .border_focus = { 90, 140, 210, 255},
        .cursor       = {200, 200, 200, 255},
        .border_width = 0.5f,
    },
    .slider = {
        .track        = { 50,  50,  50, 255},
        .thumb        = {170, 170, 170, 255},
        .label        = {200, 200, 200, 255},
        .track_height = 6.0f,
        .thumb_width  = 14.0f,
    },
    .checkbox = {
        .check  = {170, 170, 170, 255},
        .border = { 60,  60,  60, 255},
        .border_width = 0.5f,
    },
    .toggle = {
        .track        = {0},
        .track_active = { 90, 140, 210, 255},
        .thumb        = {0},
        .track_height = 0,
    },
    .radio = {
        .ring         = {0},
        .fill         = {0},
        .label        = {0},
        .border_width = 1.5f,
    },
    .progress = {
        .track        = { 50,  50,  50, 255},
        .fill         = { 90, 140, 210, 255},
        .track_height = 6.0f,
    },
    .scrollbar = {
        .bar   = { 50,  50,  50, 255},
        .width = 10.0f,
    },
};

const WLX_Theme wlx_theme_light = {
    .background       = {245, 246, 250, 255},
    .foreground       = { 30,  35,  50, 255},
    .surface          = {255, 255, 255, 255},
    .border           = {185, 190, 210, 255},
    .border_width     = 0,
    .accent           = { 55,  80, 190, 255},
    .font              = WLX_FONT_DEFAULT,
    .font_size        = 16,
    .spacing          = 2,
    .padding          = 0,
    .roundness        = 0,
    .rounded_segments = 0,
    .min_rounded_segments = 16,
    .hover_brightness = -0.08f,
    .opacity          = -1,
    .input = {
        .border_focus = { 55,  80, 190, 255},
        .cursor       = { 30,  35,  50, 255},
        .border_width = 0.5f,
    },
    .slider = {
        .track        = {210, 215, 225, 255},
        .thumb        = { 55,  80, 190, 255},
        .label        = { 30,  35,  50, 255},
        .track_height = 6.0f,
        .thumb_width  = 14.0f,
    },
    .checkbox = {
        .check  = { 55,  80, 190, 255},
        .border = {185, 190, 210, 255},
        .border_width = 0.5f,
    },
    .toggle = {
        .track        = {0},
        .track_active = { 55,  80, 190, 255},
        .thumb        = {0},
        .track_height = 0,
    },
    .radio = {
        .ring         = {0},
        .fill         = {0},
        .label        = {0},
        .border_width = 1.5f,
    },
    .progress = {
        .track        = {210, 215, 225, 255},
        .fill         = { 55,  80, 190, 255},
        .track_height = 6.0f,
    },
    .scrollbar = {
        .bar   = {200, 205, 220, 255},
        .width = 10.0f,
    },
};

const WLX_Theme wlx_theme_glass = {
    .background       = {  6,   8,  20, 255},
    .foreground       = {210, 215, 235, 255},
    .surface          = { 14,  18,  35, 220},
    .border           = { 50,  60, 100, 180},
    .border_width     = 0,
    .accent           = { 90, 110, 200, 255},
    .font             = WLX_FONT_DEFAULT,
    .font_size        = 16,
    .spacing          = 2,
    .padding          = 0,
    .roundness        = 0,
    .rounded_segments = 0,
    // .roundness        = 0.35f,
    // .rounded_segments = 6,
    .min_rounded_segments = 16,
    .hover_brightness = 0.05f,
    .opacity          = -1,
    .input = {
        .border_focus = { 90, 110, 200, 255},
        .cursor       = {210, 215, 235, 255},
        .border_width = 1.2f,
    },
    .slider = {
        .track        = { 20,  28,  60, 200},
        .thumb        = {210, 215, 235, 255},
        .label        = {190, 195, 215, 255},
        .track_height = 6.0f,
        .thumb_width  = 14.0f,
    },
    .checkbox = {
        .check  = {210, 215, 235, 255},
        .border = { 50,  60, 100, 180},
        .border_width = 1.2f,
    },
    .toggle = {
        .track        = {0},
        .track_active = { 90, 110, 200, 255},
        .thumb        = {0},
        .track_height = 0,
    },
    .radio = {
        .ring         = {0},
        .fill         = {0},
        .label        = {0},
        .border_width = 1.5f,
    },
    .progress = {
        .track        = { 20,  28,  60, 200},
        .fill         = { 90, 110, 200, 255},
        .track_height = 6.0f,
    },
    .scrollbar = {
        .bar   = { 30,  38,  65, 200},
        .width = 10.0f,
    },
};

// ============================================================================
// Implementation: utility helpers
// ============================================================================

static inline uint8_t wlx_clamp_u8(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (uint8_t)value;
}

// Adjust color brightness by factor in [-1.0, 1.0].
// Positive: lerp toward white  (0.0 = unchanged, 1.0 = white).
// Negative: scale toward black (0.0 = unchanged, -1.0 = black).
static inline WLX_Color wlx_color_brightness(WLX_Color color, float factor) {
    if (factor < -1.0f) factor = -1.0f;
    if (factor > 1.0f) factor = 1.0f;

    WLX_Color result = color;

    if (factor < 0.0f) {
        float scale = 1.0f + factor;
        result.r = wlx_clamp_u8((int)roundf((float)color.r * scale));
        result.g = wlx_clamp_u8((int)roundf((float)color.g * scale));
        result.b = wlx_clamp_u8((int)roundf((float)color.b * scale));
    } else {
        result.r = wlx_clamp_u8((int)roundf((float)color.r + (255.0f - (float)color.r) * factor));
        result.g = wlx_clamp_u8((int)roundf((float)color.g + (255.0f - (float)color.g) * factor));
        result.b = wlx_clamp_u8((int)roundf((float)color.b + (255.0f - (float)color.b) * factor));
    }

    return result;
}

static inline WLX_Color wlx_color_apply_opacity(WLX_Color c, float opacity) {
    if (opacity >= 1.0f) return c;
    if (opacity <= 0.0f) return (WLX_Color){ c.r, c.g, c.b, 0 };
    c.a = wlx_clamp_u8((int)roundf((float)c.a * opacity));
    return c;
}

// ============================================================================
// Implementation: command recording (strict deferral)
// ============================================================================

static inline void *wlx_scratch_alloc_bytes(WLX_Context *ctx, size_t size, size_t align);

// Internal: build and push a typed draw command in one step.
// payload_field is the union member name; variadic args are the payload initializers.
#define WLX_CMD_RECORD(ctx, cmd_type, payload_field, ...) do {                     \
    WLX_Cmd _cmd = { .type = (cmd_type), .data.payload_field = { __VA_ARGS__ } }; \
    wlx_pool_push(&(ctx)->arena.commands, WLX_Cmd, _cmd);                         \
} while (0)

// Variant for commands that carry no payload data (e.g. WLX_CMD_SCISSOR_END).
#define WLX_CMD_RECORD_BARE(ctx, cmd_type) do {               \
    WLX_Cmd _cmd = { .type = (cmd_type) };                    \
    wlx_pool_push(&(ctx)->arena.commands, WLX_Cmd, _cmd);     \
} while (0)

static inline void wlx_cmd_record_rect(WLX_Context *ctx, WLX_Rect rect, WLX_Color color) {
    WLX_CMD_RECORD(ctx, WLX_CMD_RECT, rect, rect, color);
}

static inline void wlx_cmd_record_rect_lines(WLX_Context *ctx, WLX_Rect rect, float thick, WLX_Color color) {
    WLX_CMD_RECORD(ctx, WLX_CMD_RECT_LINES, rect_lines, rect, thick, color);
}

static inline void wlx_cmd_record_rect_rounded(WLX_Context *ctx, WLX_Rect rect, float roundness, int segments, WLX_Color color) {
    WLX_CMD_RECORD(ctx, WLX_CMD_RECT_ROUNDED, rect_rounded, rect, roundness, segments, color);
}

static inline void wlx_cmd_record_rect_rounded_lines(WLX_Context *ctx, WLX_Rect rect, float roundness, int segments, float thick, WLX_Color color) {
    WLX_CMD_RECORD(ctx, WLX_CMD_RECT_ROUNDED_LINES, rect_rounded_lines, rect, roundness, segments, thick, color);
}

static inline void wlx_cmd_record_circle(WLX_Context *ctx, float cx, float cy, float radius, int segments, WLX_Color color) {
    WLX_CMD_RECORD(ctx, WLX_CMD_CIRCLE, circle, cx, cy, radius, segments, color);
}

static inline void wlx_cmd_record_ring(WLX_Context *ctx, float cx, float cy, float inner_r, float outer_r, int segments, WLX_Color color) {
    WLX_CMD_RECORD(ctx, WLX_CMD_RING, ring, cx, cy, inner_r, outer_r, segments, color);
}

static inline void wlx_cmd_record_line(WLX_Context *ctx, float x1, float y1, float x2, float y2, float thick, WLX_Color color) {
    WLX_CMD_RECORD(ctx, WLX_CMD_LINE, line, x1, y1, x2, y2, thick, color);
}

static inline void wlx_cmd_record_text(WLX_Context *ctx, const char *text, float x, float y, WLX_Text_Style style) {
    size_t len = strlen(text) + 1;
    char *copy = (char *)wlx_scratch_alloc_bytes(ctx, len, 1);
    memcpy(copy, text, len);
    size_t off = (size_t)(copy - (char *)wlx_pool_scratch(ctx));
    WLX_CMD_RECORD(ctx, WLX_CMD_TEXT, text, off, x, y, style);
}

static inline void wlx_cmd_record_texture(WLX_Context *ctx, WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    WLX_CMD_RECORD(ctx, WLX_CMD_TEXTURE, texture, texture, src, dst, tint);
}

static inline void wlx_cmd_record_scissor_begin(WLX_Context *ctx, WLX_Rect rect) {
    WLX_CMD_RECORD(ctx, WLX_CMD_SCISSOR_BEGIN, scissor_begin, rect);
}

static inline void wlx_cmd_record_scissor_end(WLX_Context *ctx) {
    WLX_CMD_RECORD_BARE(ctx, WLX_CMD_SCISSOR_END);
}

// Close the most recently opened sibling range in the current layout.
// A "sibling" is a range whose parent is the current layout's range.
// Does nothing if the current range IS the layout range or if no layout is active.
static inline void wlx_cmd_close_sibling_range(WLX_Context *ctx) {
    if (ctx->arena.layouts.count > 0 && ctx->current_range_idx >= 0) {
        WLX_Layout *parent = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
        if (parent->cmd_range_idx >= 0 && ctx->current_range_idx != parent->cmd_range_idx) {
            wlx_pool_cmd_ranges(ctx)[ctx->current_range_idx].end_idx = ctx->arena.commands.count;
            ctx->current_range_idx = wlx_pool_cmd_ranges(ctx)[ctx->current_range_idx].parent_range_idx;
        }
    }
}

// Open a new range entry and push it as the current range.
// Returns the index of the newly created range.
static inline int wlx_cmd_open_range(WLX_Context *ctx) {
    WLX_Cmd_Range range = {
        .start_idx = ctx->arena.commands.count,
        .end_idx = 0,
        .dy_offset = 0.0f,
        .parent_range_idx = ctx->current_range_idx,
    };
    wlx_pool_push(&ctx->arena.cmd_ranges, WLX_Cmd_Range, range);
    assert(ctx->arena.cmd_ranges.count <= (size_t)INT_MAX && "cmd_range count exceeds int range");
    int idx = (int)(ctx->arena.cmd_ranges.count - 1);
    ctx->current_range_idx = idx;
    return idx;
}

// ============================================================================
// Implementation: backend wrappers
// ============================================================================

static inline void wlx_measure_text(WLX_Context *ctx, const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
    assert(ctx->backend.measure_text != NULL && "WLX_Backend.measure_text must be set");
    ctx->backend.measure_text(text, style, out_w, out_h);
}

static inline void wlx_draw_text(WLX_Context *ctx, const char *text, float x, float y, WLX_Text_Style style) {
    if (ctx->immediate_mode) {
        assert(ctx->backend.draw_text != NULL && "WLX_Backend.draw_text must be set");
        ctx->backend.draw_text(text, x, y, style);
    } else {
        wlx_cmd_record_text(ctx, text, x, y, style);
    }
}

static inline void wlx_draw_rect(WLX_Context *ctx, WLX_Rect rect, WLX_Color color) {
    if (ctx->immediate_mode) {
        assert(ctx->backend.draw_rect != NULL && "WLX_Backend.draw_rect must be set");
        ctx->backend.draw_rect(rect, color);
    } else {
        wlx_cmd_record_rect(ctx, rect, color);
    }
}

static inline void wlx_draw_rect_lines(WLX_Context *ctx, WLX_Rect rect, float thick, WLX_Color color) {
    if (thick < 1.0f) {
        color = wlx_color_apply_opacity(color, thick);
        thick = 1.0f;
    }
    if (ctx->immediate_mode) {
        assert(ctx->backend.draw_rect_lines != NULL && "WLX_Backend.draw_rect_lines must be set");
        ctx->backend.draw_rect_lines(rect, thick, color);
    } else {
        wlx_cmd_record_rect_lines(ctx, rect, thick, color);
    }
}

static inline void wlx_draw_rect_rounded(WLX_Context *ctx, WLX_Rect rect, float roundness, int segments, WLX_Color color) {
    if (roundness >= 1.0f && fabsf(rect.w - rect.h) < 0.5f
            && ctx->backend.draw_circle) {
        float r = rect.w * 0.5f;
        if (ctx->immediate_mode) {
            ctx->backend.draw_circle(rect.x + r, rect.y + r, r, segments, color);
        } else {
            wlx_cmd_record_circle(ctx, rect.x + r, rect.y + r, r, segments, color);
        }
        return;
    }
    if (ctx->immediate_mode) {
        assert(ctx->backend.draw_rect_rounded != NULL && "WLX_Backend.draw_rect_rounded must be set");
        ctx->backend.draw_rect_rounded(rect, roundness, segments, color);
    } else {
        wlx_cmd_record_rect_rounded(ctx, rect, roundness, segments, color);
    }
}

static inline void wlx_draw_rect_rounded_lines(WLX_Context *ctx, WLX_Rect rect, float roundness, int segments, float thick, WLX_Color color) {
    if (roundness >= 1.0f && fabsf(rect.w - rect.h) < 0.5f
            && ctx->backend.draw_ring) {
        float r = rect.w * 0.5f;
        if (ctx->immediate_mode) {
            ctx->backend.draw_ring(rect.x + r, rect.y + r, r - thick, r,
                                   segments, color);
        } else {
            wlx_cmd_record_ring(ctx, rect.x + r, rect.y + r, r - thick, r,
                                segments, color);
        }
        return;
    }
    if (thick < 1.0f) {
        color = wlx_color_apply_opacity(color, thick);
        thick = 1.0f;
    }
    if (ctx->immediate_mode) {
        assert(ctx->backend.draw_rect_rounded_lines != NULL && "WLX_Backend.draw_rect_rounded_lines must be set");
        ctx->backend.draw_rect_rounded_lines(rect, roundness, segments, thick, color);
    } else {
        wlx_cmd_record_rect_rounded_lines(ctx, rect, roundness, segments, thick, color);
    }
}

static inline void wlx_draw_line(WLX_Context *ctx, float x1, float y1, float x2, float y2, float thick, WLX_Color color) {
    if (ctx->immediate_mode) {
        assert(ctx->backend.draw_line != NULL && "WLX_Backend.draw_line must be set");
        ctx->backend.draw_line(x1, y1, x2, y2, thick, color);
    } else {
        wlx_cmd_record_line(ctx, x1, y1, x2, y2, thick, color);
    }
}

static inline void wlx_draw_texture(WLX_Context *ctx, WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    if (ctx->immediate_mode) {
        assert(ctx->backend.draw_texture != NULL && "WLX_Backend.draw_texture must be set");
        ctx->backend.draw_texture(texture, src, dst, tint);
    } else {
        wlx_cmd_record_texture(ctx, texture, src, dst, tint);
    }
}

static inline float wlx_get_frame_time(WLX_Context *ctx) {
    assert(ctx->backend.get_frame_time != NULL && "WLX_Backend.get_frame_time must be set");
    return ctx->backend.get_frame_time();
}

static inline void wlx_begin_scissor(WLX_Context *ctx, WLX_Rect rect) {
    if (ctx->immediate_mode) {
        assert(ctx->backend.begin_scissor != NULL && "WLX_Backend.begin_scissor must be set");
        ctx->backend.begin_scissor(rect);
    } else {
        wlx_cmd_record_scissor_begin(ctx, rect);
    }
}

static inline void wlx_end_scissor(WLX_Context *ctx) {
    if (ctx->immediate_mode) {
        assert(ctx->backend.end_scissor != NULL && "WLX_Backend.end_scissor must be set");
        ctx->backend.end_scissor();
    } else {
        wlx_cmd_record_scissor_end(ctx);
    }
}

static inline void wlx_assert_backend_ready(WLX_Context *ctx) {
    assert(ctx != NULL && "WLX_Context must not be NULL");
    assert(
        wlx_backend_is_ready(ctx) &&
        "WLX_Backend is not initialized. Set ctx->backend before wlx_begin (e.g. wlx_context_init_raylib(ctx))."
    );
}

// ============================================================================
// Implementation: geometry, layout, and alignment
// ============================================================================

static inline WLX_Rect wlx_rect_inset(WLX_Rect rect, float inset) {
    if (inset <= 0.0f) {
        return rect;
    }

    rect.x += inset;
    rect.y += inset;
    rect.w -= inset * 2.0f;
    rect.h -= inset * 2.0f;

    if (rect.w < 0.0f) {
        rect.w = 0.0f;
    }

    if (rect.h < 0.0f) {
        rect.h = 0.0f;
    }

    return rect;
}

static inline WLX_Rect wlx_rect_inset_sides(WLX_Rect r,
    float top, float right, float bottom, float left) {
    r.x += left;
    r.y += top;
    r.w -= left + right;
    r.h -= top + bottom;
    if (r.w < 0.0f) r.w = 0.0f;
    if (r.h < 0.0f) r.h = 0.0f;
    return r;
}

typedef struct {
    float top, right, bottom, left;
} WLX_Resolved_Padding;

static inline WLX_Resolved_Padding wlx_resolve_padding(
    float uniform, float pt, float pr, float pb, float pl) {
    return (WLX_Resolved_Padding){
        .top    = (pt >= 0.0f) ? pt : uniform,
        .right  = (pr >= 0.0f) ? pr : uniform,
        .bottom = (pb >= 0.0f) ? pb : uniform,
        .left   = (pl >= 0.0f) ? pl : uniform,
    };
}

// Convenience helpers - non-consuming width/height of the innermost layout rect.
static inline float wlx_get_available_width(WLX_Context *ctx) {
    return wlx_get_parent_rect(ctx).w;
}
static inline float wlx_get_available_height(WLX_Context *ctx) {
    return wlx_get_parent_rect(ctx).h;
}

static inline WLX_Rect wlx_get_widget_cell_rect(WLX_Context *ctx, int pos, size_t span,
    float padding, float padding_top, float padding_right, float padding_bottom, float padding_left) {
    assert(ctx != NULL);
    assert(ctx->arena.layouts.count > 0);
    WLX_Rect rect = wlx_get_slot_rect(ctx, &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1], pos, span);
    WLX_Resolved_Padding p = wlx_resolve_padding(padding, padding_top, padding_right, padding_bottom, padding_left);
    return wlx_rect_inset_sides(rect, p.top, p.right, p.bottom, p.left);
}

static inline WLX_Rect wlx_resolve_widget_rect(WLX_Rect cell_rect, float width, float height,
    float min_w, float min_h, float max_w, float max_h,
    WLX_Align align, bool overflow) {
    float w = (width > 0)  ? width  : cell_rect.w;
    float h = (height > 0) ? height : cell_rect.h;

    // Clamp to widget-level min/max constraints (0 = unconstrained)
    if (min_w > 0 && w < min_w) w = min_w;
    if (max_w > 0 && w > max_w) w = max_w;
    if (min_h > 0 && h < min_h) h = min_h;
    if (max_h > 0 && h > max_h) h = max_h;

    if (overflow) {
        return (WLX_Rect){
            .x = cell_rect.x,
            .y = cell_rect.y,
            .w = w,
            .h = h,
        };
    }

    WLX_Rect r = wlx_get_align_rect(cell_rect, w, h, align);
    // wlx_get_align_rect won't grow beyond parent - enforce min constraints
    if (min_w > 0 && r.w < w) r.w = w;
    if (min_h > 0 && r.h < h) r.h = h;
    return r;
}

// Widget prologue helper - consolidates the cell->scroll->resolve
// sequence shared by every widget implementation.
typedef struct {
    WLX_Rect slot_rect;  // the raw layout cell (before alignment/sizing)
    WLX_Rect rect;       // the resolved widget rect (aligned/sized within cell)
} WLX_Widget_Rect;

// Collects the layout + sizing fields passed to wlx_widget_begin.
typedef struct {
    int       pos;
    size_t    span;
    float     padding;
    float     padding_top;
    float     padding_right;
    float     padding_bottom;
    float     padding_left;
    float     width;
    float     height;
    float     min_w;
    float     min_h;
    float     max_w;
    float     max_h;
    WLX_Align align;
    bool      overflow;
} WLX_Widget_Layout;

// Extract a WLX_Widget_Layout from any widget opt struct that uses the
// standard WLX_LAYOUT_SLOT_FIELDS + WLX_WIDGET_SIZING_FIELDS macros.
#define WLX_WIDGET_LAYOUT(opt) \
    (WLX_Widget_Layout){ \
        .pos = (opt).pos, .span = (opt).span, .padding = (opt).padding, \
        .padding_top = (opt).padding_top, .padding_right = (opt).padding_right, \
        .padding_bottom = (opt).padding_bottom, .padding_left = (opt).padding_left, \
        .width = (opt).width, .height = (opt).height, \
        .min_w = (opt).min_width, .min_h = (opt).min_height, \
        .max_w = (opt).max_width, .max_h = (opt).max_height, \
        .align = (opt).widget_align, .overflow = (opt).overflow, \
    }

static inline WLX_Widget_Rect wlx_widget_begin(WLX_Context *ctx, WLX_Widget_Layout ly)
{
    WLX_Rect cell = wlx_get_widget_cell_rect(ctx, ly.pos, ly.span, ly.padding,
        ly.padding_top, ly.padding_right, ly.padding_bottom, ly.padding_left);

    wlx_cmd_close_sibling_range(ctx);
    wlx_cmd_open_range(ctx);

    // Track content height on the parent layout. `wlx_layout_end()` contributes
    // the accumulated total to auto_scroll_total_height, so widgets never
    // need to know about scroll panels.
    if (ctx->arena.layouts.count > 0) {
        WLX_Layout *parent_l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
        float h_contrib = (ly.height > 0) ? ly.height
                       : (ly.min_h > cell.h) ? ly.min_h : cell.h;
        // HORZ layouts: content height = max of children (side by side)
        // VERT layouts: content height = sum of children (stacked)
        if (parent_l->kind == WLX_LAYOUT_LINEAR && parent_l->linear.orient == WLX_HORZ) {
            if (h_contrib > parent_l->accumulated_content_height)
                parent_l->accumulated_content_height = h_contrib;
        } else {
            parent_l->accumulated_content_height += h_contrib;
        }

        // Per-slot content height tracking for CONTENT slots.
        // l->index was just advanced by wlx_get_slot_rect, so the slot
        // that this widget occupies is at (index - span).
        WLX_Layout *l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
        if (l->has_content_slot_heights) {
            size_t slot_idx = l->index - ly.span;
            if (slot_idx < WLX_CONTENT_SLOTS_MAX) {
                float contrib = (ly.height > 0) ? ly.height
                              : (ly.min_h > cell.h) ? ly.min_h : cell.h;
                wlx_layout_content_heights(ctx, l)[slot_idx] += contrib;
            }
        }

        // Per-row content height tracking for grid CONTENT rows.
        // Uses max() across cells in the same row.
        if (l->has_grid_row_content_heights) {
            size_t row = l->grid.last_placed_row;
            if (row < l->grid.rows) {
                float contrib = (ly.height > 0) ? ly.height
                              : (ly.min_h > cell.h) ? ly.min_h : cell.h;
                float *rch = wlx_grid_row_content_heights(ctx, l);
                if (contrib > rch[row])
                    rch[row] = contrib;
            }
        }
    }

    // --- Widget rect resolution ---
    WLX_Rect rect = wlx_resolve_widget_rect(cell, ly.width, ly.height, ly.min_w, ly.min_h, ly.max_w, ly.max_h, ly.align, ly.overflow);
    return (WLX_Widget_Rect){ .slot_rect = cell, .rect = rect };
}

static size_t wlx_hash_id(const char *file, int line) {
    size_t hash = 5381;
    while (*file) {
        hash = ((hash << 5) + hash) + (unsigned char)(*file);
        file++;
    }
    hash = ((hash << 5) + hash) + (size_t)line;
    return hash;
}

// djb2 hash for user-supplied string IDs.
static inline size_t wlx_hash_string(const char *s) {
    if (!s) return 0;
    size_t h = 5381;
    while (*s) {
        h = ((h << 5) + h) + (unsigned char)(*s++);
    }
    return h;
}

// Wraps the per-widget prologue/epilogue: push_id (if set), widget_begin, DBG.
// Use wlx_widget_frame_begin / wlx_widget_frame_end in every widget _impl.
// Store the returned frame and pass it to end: wlx_widget_frame_end(ctx, frame).
typedef struct {
    WLX_Rect slot_rect;
    WLX_Rect rect;
    const char *pushed_id;
} WLX_Widget_Frame;

static inline WLX_Widget_Frame wlx_widget_frame_begin(
    WLX_Context *ctx, const char *id, WLX_Widget_Layout ly,
    const char *file, int line)
{
    WLX_UNUSED(file); 
    WLX_UNUSED(line);

    if (id) wlx_push_id(ctx, wlx_hash_string(id));
    WLX_Widget_Rect wg = wlx_widget_begin(ctx, ly);
    WLX_DBG(widget_begin, ctx, wg.slot_rect, ly.height, (int)ly.span, ly.overflow, file, line);
    return (WLX_Widget_Frame){ .slot_rect = wg.slot_rect, .rect = wg.rect, .pushed_id = id };
}

static inline void wlx_widget_frame_end(WLX_Context *ctx, WLX_Widget_Frame frame)
{
    if (frame.pushed_id) wlx_pop_id(ctx);
}

WLXDEF WLX_Rect wlx_rect(float x, float y, float w, float h)
{
    WLX_Rect r = {0};
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    return r;
}

WLXDEF WLX_Rect wlx_rect_intersect(WLX_Rect a, WLX_Rect b) {
    float x1 = a.x > b.x ? a.x : b.x;
    float y1 = a.y > b.y ? a.y : b.y;
    float x2 = (a.x + a.w) < (b.x + b.w) ? (a.x + a.w) : (b.x + b.w);
    float y2 = (a.y + a.h) < (b.y + b.h) ? (a.y + a.h) : (b.y + b.h);
    return (WLX_Rect){
        .x = x1,
        .y = y1,
        .w = (x2 > x1) ? (x2 - x1) : 0,
        .h = (y2 > y1) ? (y2 - y1) : 0,
    };
}

// Reserve `n` floats from the context slot sizes buffer, returning a pointer
// to the first element. The buffer grows as needed but is never freed -
// it is simply reset (count = 0) each frame in wlx_begin().
static inline float *wlx_scratch_alloc(WLX_Context *ctx, size_t n) {
    assert(ctx != NULL);
    assert(n > 0 && n <= WLX_MAX_SLOT_COUNT && "unreasonable scratch alloc - did you pass a negative count?");
    assert(ctx->arena.slot_size_offsets.count + n > ctx->arena.slot_size_offsets.count && "size_t overflow in scratch alloc");
    wlx_sub_arena_reserve(&ctx->arena.slot_size_offsets, ctx->arena.slot_size_offsets.count + n);
    float *ptr = &wlx_pool_slot_size_offsets(ctx)[ctx->arena.slot_size_offsets.count];
    ctx->arena.slot_size_offsets.count += n;
    return ptr;
}

// Reserve `n` floats from the dynamic-layout offset buffer, returning a pointer
// to the first element. Mirrors wlx_scratch_alloc but uses the separate
// dyn_offsets arena so dynamic-layout growth cannot corrupt static offsets.
static inline float *wlx_dyn_scratch_alloc(WLX_Context *ctx, size_t n) {
    assert(ctx != NULL);
    assert(n > 0 && n <= WLX_MAX_SLOT_COUNT && "unreasonable dyn scratch alloc");
    assert(ctx->arena.dyn_offsets.count + n > ctx->arena.dyn_offsets.count && "size_t overflow in dyn scratch alloc");
    wlx_sub_arena_reserve(&ctx->arena.dyn_offsets, ctx->arena.dyn_offsets.count + n);
    float *ptr = &wlx_pool_dyn_offsets(ctx)[ctx->arena.dyn_offsets.count];
    ctx->arena.dyn_offsets.count += n;
    return ptr;
}

// Reserve `size` bytes (with `align` alignment) from the generic byte scratch
// arena. Same lifetime as wlx_scratch_alloc - reset each frame in wlx_begin().
static inline void *wlx_scratch_alloc_bytes(WLX_Context *ctx, size_t size, size_t align) {
    assert(ctx != NULL);
    assert(size > 0);
    size_t aligned = (ctx->arena.scratch.count + align - 1) & ~(align - 1);
    size_t needed = aligned + size;
    wlx_sub_arena_reserve(&ctx->arena.scratch, needed);
    void *ptr = &wlx_pool_scratch(ctx)[aligned];
    ctx->arena.scratch.count = needed;
    return ptr;
}

// Core offset math - fills offsets[0..count] given a total extent and
// optional per-slot sizes.  Used by both 1D layouts (single axis) and
// grid layouts (row axis + column axis).
static inline void wlx_compute_offsets(float *offsets, size_t count,
                                       float total, float viewport,
                                       const WLX_Slot_Size *sizes, float gap)
{
    assert(offsets != NULL);
    assert(count > 0 && count <= WLX_MAX_SLOT_COUNT && "unreasonable slot count");

    float total_gap = gap * (float)(count > 1 ? count - 1 : 0);

    if (sizes == NULL) {
        // Equal division - subtract gap from distributable total, then snap
        // each boundary to integer pixels to prevent sub-pixel gaps.
        float slot_total = total - total_gap;
        if (slot_total < 0.0f) slot_total = 0.0f;
        float slot_w = slot_total / (float)count;
        float offset = 0.0f;
        for (size_t i = 0; i <= count; i++) {
            offsets[i] = floorf(offset + 0.5f);
            if (i < count) offset += slot_w + (i < count - 1 ? gap : 0.0f);
        }
        return;
    }

    // Adjusted total: distributable space after subtracting inter-slot gaps
    float adjusted_total = total - total_gap;
    if (adjusted_total < 0.0f) adjusted_total = 0.0f;

    // Two-pass: measure fixed/percent, accumulate flex/auto weights
    float used = 0.0f;
    float total_weight = 0.0f;
    for (size_t i = 0; i < count; i++) {
        switch (sizes[i].kind) {
            case WLX_SIZE_AUTO:    total_weight += 1.0f; break;
            case WLX_SIZE_FLEX:    total_weight += sizes[i].value; break;
            case WLX_SIZE_PIXELS:  used += sizes[i].value; break;
            case WLX_SIZE_PERCENT: used += (sizes[i].value * adjusted_total) / 100.0f; break;
            case WLX_SIZE_FILL:    used += sizes[i].value * viewport; break;
            case WLX_SIZE_CONTENT: used += sizes[i].value; break; // pre-resolved or fallback 0
            default: WLX_UNREACHABLE("Undefined slot size kind");
        }
    }
    float remaining = (adjusted_total - used > 0.0f) ? (adjusted_total - used) : 0.0f;

    // --- Pass 2: compute raw slot sizes and write offsets ---
    float offset = 0.0f;
    for (size_t i = 0; i < count; i++) {
        offsets[i] = offset;
        float slot_size;
        switch (sizes[i].kind) {
            case WLX_SIZE_AUTO:
                slot_size = (total_weight > 0.0f)
                    ? remaining * (1.0f / total_weight) : 0.0f;
                break;
            case WLX_SIZE_FLEX:
                slot_size = (total_weight > 0.0f)
                    ? remaining * (sizes[i].value / total_weight) : 0.0f;
                break;
            case WLX_SIZE_PIXELS:  slot_size = sizes[i].value; break;
            case WLX_SIZE_PERCENT: slot_size = (sizes[i].value * adjusted_total) / 100.0f; break;
            case WLX_SIZE_FILL:    slot_size = sizes[i].value * viewport; break;
            case WLX_SIZE_CONTENT: slot_size = sizes[i].value; break; // pre-resolved or fallback 0
            default: WLX_UNREACHABLE("Undefined slot size kind"); slot_size = 0.0f;
        }
        // Clamp to min/max constraints (0 = unconstrained)
        if (sizes[i].min > 0 && slot_size < sizes[i].min) slot_size = sizes[i].min;
        if (sizes[i].max > 0 && slot_size > sizes[i].max) slot_size = sizes[i].max;
        offset += slot_size;
        if (i < count - 1) offset += gap;
        // Snap each boundary to integer pixels to prevent sub-pixel gaps.
        offsets[i] = floorf(offsets[i] + 0.5f);
    }
    offsets[count] = floorf(offset + 0.5f);

#ifdef WLX_SLOT_MINMAX_REDISTRIBUTE
    // --- Pass 3: iterative freeze-and-redistribute ---
    // Ensures offsets[count] == adjusted_total + total_gap by redistributing
    // surplus/deficit from clamped slots to unfrozen neighbors.
    {
        // Check if any slot has min/max constraints at all
        bool has_constraints = false;
        for (size_t i = 0; i < count; i++) {
            if (sizes[i].min > 0 || sizes[i].max > 0) {
                has_constraints = true;
                break;
            }
        }
        if (has_constraints) {
            // Stack-allocate working arrays (slot counts are small, typically < 20)
            bool frozen_buf[WLX_OFFSET_STACK_LIMIT];
            float raw_buf[WLX_OFFSET_STACK_LIMIT];
            bool *frozen = (count <= WLX_OFFSET_STACK_LIMIT) ? frozen_buf : (bool *)wlx_alloc(count * sizeof(bool));
            float *raw    = (count <= WLX_OFFSET_STACK_LIMIT) ? raw_buf   : (float *)wlx_alloc(count * sizeof(float));

            // Extract raw slot sizes (without gap) from current offsets
            for (size_t i = 0; i < count; i++) {
                frozen[i] = false;
                float raw_size = offsets[i + 1] - offsets[i];
                if (i < count - 1) raw_size -= gap;
                raw[i] = raw_size;
            }

            // Iterate until no new slots freeze (converges in <= count iterations)
            for (size_t iter = 0; iter < count; iter++) {
                bool changed = false;

                // Compute total unfrozen weight and frozen usage
                float unfrozen_weight = 0.0f;
                float frozen_total = 0.0f;
                for (size_t i = 0; i < count; i++) {
                    if (frozen[i]) {
                        frozen_total += raw[i];
                    } else {
                        switch (sizes[i].kind) {
                            case WLX_SIZE_AUTO:    unfrozen_weight += 1.0f; break;
                            case WLX_SIZE_FLEX:    unfrozen_weight += sizes[i].value; break;
                            case WLX_SIZE_PIXELS:  frozen_total += raw[i]; break; // fixed sizes act like frozen
                            case WLX_SIZE_PERCENT: frozen_total += raw[i]; break;
                            case WLX_SIZE_FILL:    frozen_total += raw[i]; break; // viewport-fill acts like frozen
                            case WLX_SIZE_CONTENT: frozen_total += raw[i]; break; // pre-resolved, acts like frozen
                            default: break;
                        }
                    }
                }

                float unfrozen_remaining = adjusted_total - frozen_total;
                if (unfrozen_remaining < 0.0f) unfrozen_remaining = 0.0f;

                // Redistribute among unfrozen flex/auto slots
                for (size_t i = 0; i < count; i++) {
                    if (frozen[i]) continue;
                    if (sizes[i].kind != WLX_SIZE_AUTO && sizes[i].kind != WLX_SIZE_FLEX) continue;

                    float w = (sizes[i].kind == WLX_SIZE_FLEX) ? sizes[i].value : 1.0f;
                    float new_size = (unfrozen_weight > 0.0f)
                        ? unfrozen_remaining * (w / unfrozen_weight) : 0.0f;

                    // Clamp and freeze if needed
                    float clamped = new_size;
                    if (sizes[i].min > 0 && clamped < sizes[i].min) clamped = sizes[i].min;
                    if (sizes[i].max > 0 && clamped > sizes[i].max) clamped = sizes[i].max;

                    if (clamped != new_size) {
                        frozen[i] = true;
                        raw[i] = clamped;
                        changed = true;
                    } else {
                        raw[i] = new_size;
                    }
                }

                if (!changed) break;
            }

            // Rebuild offsets from raw sizes with gap - snap to integer pixels.
            offset = 0.0f;
            for (size_t i = 0; i < count; i++) {
                offsets[i] = floorf(offset + 0.5f);
                offset += raw[i];
                if (i < count - 1) offset += gap;
            }
            offsets[count] = floorf(offset + 0.5f);

            if (count > WLX_OFFSET_STACK_LIMIT) {
                wlx_free(frozen);
                wlx_free(raw);
            }
        }
    }
#endif // WLX_SLOT_MINMAX_REDISTRIBUTE
}

static inline void wlx_compute_slot_offsets(WLX_Context *ctx, WLX_Layout *l, const WLX_Slot_Size *sizes) {
    assert(l != NULL);
    assert(l->count > 0);
    float total = (l->linear.orient == WLX_HORZ) ? l->rect.w : l->rect.h;
    wlx_compute_offsets(wlx_layout_offsets(ctx, l), l->count, total, l->viewport, sizes, l->gap);
}

// Create a layout and allocate its offsets from the context slot sizes buffer.
// The offsets are filled with equal-division values by default.
WLXDEF WLX_Layout wlx_create_layout(WLX_Context *ctx, WLX_Rect r, size_t count, WLX_Orient orient, float gap) {
    assert(ctx != NULL);
    assert(count > 0 && count <= WLX_MAX_SLOT_COUNT
           && "slot count is 0 or absurdly large - did you pass a negative int?");

    size_t offsets_base = ctx->arena.slot_size_offsets.count;
    (void)wlx_scratch_alloc(ctx, count + 1);

    WLX_Layout l = {
        .kind = WLX_LAYOUT_LINEAR,
        .rect = r,
        .index = 0,
        .count = count,
        .gap = gap,
        .cmd_range_idx = -1,
        .linear = {
            .orient = orient,
            .slot_size_offsets_base = offsets_base,
        },
    };

    wlx_compute_slot_offsets(ctx, &l, NULL);

    return l;
}

// Create a dynamic (auto-sizing) layout.  The slot sizes buffer holds only the
// initial sentinel offset[0] = 0.  Each child widget appends one boundary via
// wlx_get_slot_rect() so no upfront count is required.
//
// slot_px  - fixed pixel size per slot along the layout axis. (Must be >= 0)
WLXDEF WLX_Layout wlx_create_layout_auto(WLX_Context *ctx, WLX_Rect r, WLX_Orient orient, float slot_px) {
    assert(ctx != NULL);
    assert(slot_px >= 0.0f && "slot_px must be non-negative (0 = variable-size mode)");

    size_t base = ctx->arena.dyn_offsets.count;
    wlx_sub_arena_reserve(&ctx->arena.dyn_offsets, base + 1);
    wlx_pool_dyn_offsets(ctx)[base] = 0.0f;
    ctx->arena.dyn_offsets.count = base + 1;

    WLX_Layout l = {
        .kind         = WLX_LAYOUT_LINEAR,
        .rect         = r,
        .index        = 0,
        .count        = 0,
        .cmd_range_idx = -1,
        .linear = {
            .orient       = orient,
            .dynamic      = true,
            .slot_size    = slot_px,
            .slot_size_offsets_base = base,
        },
    };

    return l;
}

WLXDEF WLX_Layout wlx_create_grid(WLX_Context *ctx, WLX_Rect r,
    size_t rows, size_t cols, const WLX_Slot_Size *row_sizes, const WLX_Slot_Size *col_sizes, float gap)
{
    assert(ctx != NULL);
    assert(rows > 0 && rows <= WLX_MAX_SLOT_COUNT && "Grid must have at least 1 row (and not absurdly many)");
    assert(cols > 0 && cols <= WLX_MAX_SLOT_COUNT && "Grid must have at least 1 column (and not absurdly many)");

    size_t row_base = ctx->arena.slot_size_offsets.count;
    float *row_off = wlx_scratch_alloc(ctx, rows + 1);
    size_t col_base = ctx->arena.slot_size_offsets.count;
    float *col_off = wlx_scratch_alloc(ctx, cols + 1);

    wlx_compute_offsets(row_off, rows, r.h, r.h, row_sizes, gap);
    wlx_compute_offsets(col_off, cols, r.w, r.w, col_sizes, gap);

    return (WLX_Layout){
        .kind        = WLX_LAYOUT_GRID,
        .rect        = r,
        .count       = rows * cols,
        .index       = 0,
        .gap         = gap,
        .cmd_range_idx = -1,
        .grid = {
            .rows             = rows,
            .cols             = cols,
            .row_offsets_base = row_base,
            .col_offsets_base = col_base,
        },
    };
}

WLXDEF WLX_Layout wlx_create_grid_auto(WLX_Context *ctx, WLX_Rect r,
    size_t cols, float row_px, const WLX_Slot_Size *col_sizes, float gap)
{
    assert(ctx != NULL);
    assert(cols > 0 && cols <= WLX_MAX_SLOT_COUNT && "Grid must have at least 1 column (and not absurdly many)");
    assert(row_px > 0.0f && "Dynamic grid requires row_px > 0");

    // Column offsets: fixed, computed immediately
    size_t col_base = ctx->arena.slot_size_offsets.count;
    float *col_off = wlx_scratch_alloc(ctx, cols + 1);
    wlx_compute_offsets(col_off, cols, r.w, r.w, col_sizes, gap);

    // Row offsets: dynamic, starts with sentinel only (on dyn_offsets arena)
    size_t row_base = ctx->arena.dyn_offsets.count;
    wlx_sub_arena_reserve(&ctx->arena.dyn_offsets, row_base + 1);
    wlx_pool_dyn_offsets(ctx)[row_base] = 0.0f;
    ctx->arena.dyn_offsets.count = row_base + 1;

    return (WLX_Layout){
        .kind  = WLX_LAYOUT_GRID,
        .rect  = r,
        .count = 0,
        .index = 0,
        .gap   = gap,
        .cmd_range_idx = -1,
        .grid  = {
            .rows             = 0,
            .cols             = cols,
            .dynamic          = true,
            .row_size         = row_px,
            .row_offsets_base = row_base,
            .col_offsets_base = col_base,
        },
    };
}

static inline WLX_Rect wlx_calc_grid_slot_rect(const WLX_Context *ctx, const WLX_Layout *l,
    size_t row, size_t col, size_t row_span, size_t col_span)
{
    assert(l != NULL);
    assert(l->kind == WLX_LAYOUT_GRID);
    assert(row + row_span <= l->grid.rows);
    assert(col + col_span <= l->grid.cols);

    const float *row_off = wlx_grid_row_offsets(ctx, l);
    const float *col_off = wlx_grid_col_offsets(ctx, l);
    float raw_w = col_off[col + col_span] - col_off[col];
    float raw_h = row_off[row + row_span] - row_off[row];
    float col_gap = (col + col_span < l->grid.cols) ? l->gap : 0.0f;
    float row_gap = (row + row_span < l->grid.rows) ? l->gap : 0.0f;

    return (WLX_Rect){
        .x = l->rect.x + col_off[col],
        .y = l->rect.y + row_off[row],
        .w = raw_w - col_gap,
        .h = raw_h - row_gap,
    };
}

static inline WLX_Rect wlx_calc_layout_slot_rect(const WLX_Context *ctx, const WLX_Layout *l, size_t cell_index, size_t span) {
    assert(l != NULL);
    assert(l->count > 0);
    assert(cell_index < l->count);
    assert((cell_index + span) <= l->count);

    WLX_Rect slot_rect = {0};
    const float *offsets = wlx_layout_offsets(ctx, l);
    float raw = offsets[cell_index + span] - offsets[cell_index];
    float trailing_gap = (cell_index + span < l->count) ? l->gap : 0.0f;

    switch (l->linear.orient) {
        case WLX_HORZ: {
            slot_rect.x = l->rect.x + offsets[cell_index];
            slot_rect.y = l->rect.y;
            slot_rect.w = raw - trailing_gap;
            slot_rect.h = l->rect.h;
            break;
        }
        case WLX_VERT: {
            slot_rect.x = l->rect.x;
            slot_rect.y = l->rect.y + offsets[cell_index];
            slot_rect.w = l->rect.w;
            slot_rect.h = raw - trailing_gap;
            break;
        }
        default:
            WLX_UNREACHABLE("Undefined orientation");
    }

    return slot_rect;
}

static void wlx_draw_layout_background(WLX_Context *ctx, WLX_Rect rect,
                                        WLX_Color back_color, WLX_Color border_color,
                                        float border_width, float roundness,
                                        int rounded_segments);

WLXDEF WLX_Rect wlx_get_slot_rect(WLX_Context *ctx, WLX_Layout *l, int pos, size_t span) {
    assert(l != NULL);

    if (l->kind == WLX_LAYOUT_GRID) {
        size_t row, col, rspan, cspan;

        if (l->grid.cell_set) {
            // Explicit placement via grid_cell()
            row   = l->grid.next_row;
            col   = l->grid.next_col;
            rspan = l->grid.next_row_span;
            cspan = l->grid.next_col_span;
            l->grid.cell_set = false;
        } else {
            // Auto-advance: row-major, left-to-right, top-to-bottom
            row   = l->grid.cursor_row;
            col   = l->grid.cursor_col;
            rspan = 1;
            cspan = span;  // 1D span -> col_span for grid auto-advance
        }

        // Dynamic grid: grow rows on demand when cursor/explicit placement
        // reaches beyond the current row count.
        if (l->grid.dynamic && (row + rspan) > l->grid.rows) {
            size_t rows_needed = (row + rspan) - l->grid.rows;
            size_t new_total = l->grid.rows + rows_needed;

            size_t needed = l->grid.row_offsets_base + new_total + 1;
            wlx_sub_arena_reserve(&ctx->arena.dyn_offsets, needed);

            float *row_off = wlx_grid_row_offsets(ctx, l);
            for (size_t r = l->grid.rows; r < new_total; r++) {
                float effective = (l->grid.next_row_size > 0.0f)
                    ? l->grid.next_row_size : l->grid.row_size;
                float gap_add = (r > 0) ? l->gap : 0.0f;
                row_off[r + 1] = row_off[r] + effective + gap_add;
                l->grid.next_row_size = 0.0f;
            }
            ctx->arena.dyn_offsets.count = needed;
            l->grid.rows = new_total;
            l->count = l->grid.rows * l->grid.cols;
        }

        assert(row + rspan <= l->grid.rows && "Grid row + row_span out of bounds");
        assert(col + cspan <= l->grid.cols && "Grid col + col_span out of bounds");

        WLX_Rect slot_rect = wlx_calc_grid_slot_rect(ctx, l, row, col, rspan, cspan);

        {
            bool has_override = !wlx_color_is_zero(l->grid.next_cell_back_color)
                             || l->grid.next_cell_border_width > 0;
            WLX_Color bg  = has_override ? l->grid.next_cell_back_color   : l->slot_back_color;
            WLX_Color bc  = has_override ? l->grid.next_cell_border_color : l->slot_border_color;
            float     bw  = has_override ? l->grid.next_cell_border_width : l->slot_border_width;
            l->grid.next_cell_back_color   = (WLX_Color){0};
            l->grid.next_cell_border_color = (WLX_Color){0};
            l->grid.next_cell_border_width = 0.0f;
            wlx_draw_layout_background(ctx, slot_rect, bg, bc, bw, 0, 0);
        }

        l->grid.last_placed_row = row;

        // Advance cursor past this cell (row-major order)
        l->grid.cursor_col = col + cspan;
        if (l->grid.cursor_col >= l->grid.cols) {
            l->grid.cursor_col = 0;
            l->grid.cursor_row += 1;
        }

        // Keep l->index in sync for compatibility (e.g. scroll height)
        l->index = l->grid.cursor_row * l->grid.cols + l->grid.cursor_col;

        return slot_rect;
    }

    if (l->linear.dynamic) {
        assert(pos < 0 && "Positional access (pos >= 0) is not supported on dynamic layouts");
        assert(ctx->arena.dyn_offsets.count == l->linear.slot_size_offsets_base + l->count + 1 &&
               "Dynamic layout offset region is not contiguous — nested layout_begin inside a dynamic body?");

        float effective = (l->linear.next_slot_size > 0.0f) ? l->linear.next_slot_size : l->linear.slot_size;
        assert(effective > 0.0f &&
               "Dynamic layout: slot size is 0 - call wlx_layout_auto_slot_px() before each child "
               "when using variable-size mode (slot_px = 0)");
        l->linear.next_slot_size = 0.0f;

        size_t needed = l->linear.slot_size_offsets_base + l->count + span + 1;
        wlx_sub_arena_reserve(&ctx->arena.dyn_offsets, needed);
        float *offsets = wlx_layout_offsets(ctx, l);
        for (size_t s = 0; s < span; s++) {
            float gap_add = (l->count + s > 0) ? l->gap : 0.0f;
            offsets[l->count + 1 + s] = floorf(offsets[l->count + s] + effective + gap_add + 0.5f);
        }
        ctx->arena.dyn_offsets.count = needed;
        l->count += span;
    } else {
        assert(pos < (int)l->count);
    }

    size_t cell_index = (pos < 0) ? l->index : (size_t)pos;
    WLX_Rect slot_rect = wlx_calc_layout_slot_rect(ctx, l, cell_index, span);

    {
        bool has_override = !wlx_color_is_zero(l->linear.next_slot_back_color)
                         || l->linear.next_slot_border_width > 0;
        WLX_Color bg  = has_override ? l->linear.next_slot_back_color   : l->slot_back_color;
        WLX_Color bc  = has_override ? l->linear.next_slot_border_color : l->slot_border_color;
        float     bw  = has_override ? l->linear.next_slot_border_width : l->slot_border_width;
        l->linear.next_slot_back_color   = (WLX_Color){0};
        l->linear.next_slot_border_color = (WLX_Color){0};
        l->linear.next_slot_border_width = 0.0f;
        wlx_draw_layout_background(ctx, slot_rect, bg, bc, bw, 0, 0);
    }

    l->index = cell_index + span;

    return slot_rect;
}

WLXDEF bool wlx_is_key_down(WLX_Context *ctx, WLX_Key_Code key) {
    return key >= 0 && key < WLX_KEY_COUNT && ctx->input.keys_down[key];
}

WLXDEF bool wlx_is_key_pressed(WLX_Context *ctx, WLX_Key_Code key) {
    return key >= 0 && key < WLX_KEY_COUNT && ctx->input.keys_pressed[key];
}

WLXDEF bool wlx_point_in_rect(int px, int py, int x, int y, int w, int h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

WLXDEF void wlx_context_init(WLX_Context *ctx) {
    wlx_context_init_ex(ctx, NULL);
}

WLXDEF void wlx_context_init_ex(WLX_Context *ctx, const WLX_Arena_Pool_Config *cfg) {
    assert(ctx != NULL);
    wlx_zero_struct(*ctx);
    wlx_arena_pool_init(&ctx->arena, cfg);
    ctx->current_range_idx = -1;
}

WLXDEF void wlx_begin(WLX_Context *ctx, WLX_Rect r, WLX_Input_Handler input_handler) {
    wlx_assert_backend_ready(ctx);
    assert(input_handler != NULL && "WLX input handler must not be NULL");
    input_handler(ctx);
    ctx->interaction.hot_id = 0;
    ctx->interaction.active_id_seen = false;
    // Lazy pool init: callers that zero-init WLX_Context and skip
    // wlx_context_init still get the default macro-backed allocators.
    if (ctx->arena.layouts.item_size == 0) {
        wlx_arena_pool_init(&ctx->arena, NULL);
    }
    wlx_arena_pool_reset(&ctx->arena);
    ctx->current_range_idx = -1;
    ctx->immediate_mode = false;
    ctx->rect = r;
    if (ctx->theme == NULL) ctx->theme = &wlx_theme_dark;
    WLX_DBG(frame_begin, ctx);
}

WLXDEF void wlx_begin_immediate(WLX_Context *ctx, WLX_Rect r, WLX_Input_Handler input_handler) {
    wlx_begin(ctx, r, input_handler);
    ctx->immediate_mode = true;
}

WLXDEF void wlx_end(WLX_Context *ctx) {
    if (ctx->interaction.active_id != 0 && !ctx->interaction.active_id_seen) {
        ctx->interaction.active_id = 0;
    }

    if (ctx->immediate_mode || ctx->arena.commands.count == 0) return;

    // Accumulate nested offsets: parents appear before children by construction.
    for (size_t i = 0; i < ctx->arena.cmd_ranges.count; i++) {
        int p = wlx_pool_cmd_ranges(ctx)[i].parent_range_idx;
        if (p != WLX_NO_RANGE) {
            wlx_pool_cmd_ranges(ctx)[i].dy_offset += wlx_pool_cmd_ranges(ctx)[(size_t)p].dy_offset;
        }
    }

    // Build per-command offset lookup: children overwrite parent entries,
    // so each command gets the deepest (most specific) accumulated offset.
    float *cmd_dy = (float *)wlx_scratch_alloc_bytes(ctx,
        ctx->arena.commands.count * sizeof(float), _Alignof(float));
    wlx_zero_array(ctx->arena.commands.count, cmd_dy);
    for (size_t i = 0; i < ctx->arena.cmd_ranges.count; i++) {
        WLX_Cmd_Range *r = &wlx_pool_cmd_ranges(ctx)[i];
        for (size_t ci = r->start_idx; ci < r->end_idx; ci++) {
            cmd_dy[ci] = r->dy_offset;
        }
    }

    // Dispatch loop: translate y-coordinates and call backend.
    for (size_t i = 0; i < ctx->arena.commands.count; i++) {
        WLX_Cmd *c = &wlx_pool_commands(ctx)[i];
        float dy = cmd_dy[i];

        switch (c->type) {
        case WLX_CMD_RECT:
            ctx->backend.draw_rect(
                (WLX_Rect){c->data.rect.rect.x, c->data.rect.rect.y + dy,
                           c->data.rect.rect.w, c->data.rect.rect.h},
                c->data.rect.color);
            break;

        case WLX_CMD_RECT_LINES:
            ctx->backend.draw_rect_lines(
                (WLX_Rect){c->data.rect_lines.rect.x, c->data.rect_lines.rect.y + dy,
                           c->data.rect_lines.rect.w, c->data.rect_lines.rect.h},
                c->data.rect_lines.thick, c->data.rect_lines.color);
            break;

        case WLX_CMD_RECT_ROUNDED:
            ctx->backend.draw_rect_rounded(
                (WLX_Rect){c->data.rect_rounded.rect.x, c->data.rect_rounded.rect.y + dy,
                           c->data.rect_rounded.rect.w, c->data.rect_rounded.rect.h},
                c->data.rect_rounded.roundness, c->data.rect_rounded.segments,
                c->data.rect_rounded.color);
            break;

        case WLX_CMD_RECT_ROUNDED_LINES:
            ctx->backend.draw_rect_rounded_lines(
                (WLX_Rect){c->data.rect_rounded_lines.rect.x, c->data.rect_rounded_lines.rect.y + dy,
                           c->data.rect_rounded_lines.rect.w, c->data.rect_rounded_lines.rect.h},
                c->data.rect_rounded_lines.roundness, c->data.rect_rounded_lines.segments,
                c->data.rect_rounded_lines.thick, c->data.rect_rounded_lines.color);
            break;

        case WLX_CMD_CIRCLE:
            ctx->backend.draw_circle(
                c->data.circle.cx, c->data.circle.cy + dy,
                c->data.circle.radius, c->data.circle.segments,
                c->data.circle.color);
            break;

        case WLX_CMD_RING:
            ctx->backend.draw_ring(
                c->data.ring.cx, c->data.ring.cy + dy,
                c->data.ring.inner_r, c->data.ring.outer_r,
                c->data.ring.segments, c->data.ring.color);
            break;

        case WLX_CMD_LINE:
            ctx->backend.draw_line(
                c->data.line.x1, c->data.line.y1 + dy,
                c->data.line.x2, c->data.line.y2 + dy,
                c->data.line.thick, c->data.line.color);
            break;

        case WLX_CMD_TEXT:
            ctx->backend.draw_text(
                (const char *)&wlx_pool_scratch(ctx)[c->data.text.text_off],
                c->data.text.x, c->data.text.y + dy,
                c->data.text.style);
            break;

        case WLX_CMD_TEXTURE:
            ctx->backend.draw_texture(
                c->data.texture.texture, c->data.texture.src,
                (WLX_Rect){c->data.texture.dst.x, c->data.texture.dst.y + dy,
                           c->data.texture.dst.w, c->data.texture.dst.h},
                c->data.texture.tint);
            break;

        case WLX_CMD_SCISSOR_BEGIN:
            ctx->backend.begin_scissor(
                (WLX_Rect){c->data.scissor_begin.rect.x, c->data.scissor_begin.rect.y + dy,
                           c->data.scissor_begin.rect.w, c->data.scissor_begin.rect.h});
            break;

        case WLX_CMD_SCISSOR_END:
            ctx->backend.end_scissor();
            break;
        }
    }
}

WLXDEF void wlx_context_destroy(WLX_Context *ctx) {
    // Free state map data entries
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        if (ctx->states.slots[i].id != 0) {
            wlx_free(ctx->states.slots[i].data);
        }
    }
    // Release pool-owned per-frame buffers and the persistent state map.
    wlx_arena_pool_destroy(&ctx->arena);
    wlx_free(ctx->states.slots);
    WLX_DBG(destroy, ctx);
    // Zero out the context so it's safe to reuse or free
    wlx_zero_struct(*ctx);
}

// Internal draw helper for a filled rectangle with an optional border.
// fill    - fill color; zero means no fill drawn.
// border  - border color; zero or border_width <= 0 means no border drawn.
// rounded_segments is resolved against theme->rounded_segments when <= 0.
typedef struct {
    WLX_Color fill;
    WLX_Color border;
    float border_width;
    float roundness;
    int   rounded_segments;
} WLX_Box_Style;

static inline void wlx_draw_box(WLX_Context *ctx, WLX_Rect rect, WLX_Box_Style style)
{
    bool has_fill   = !wlx_color_is_zero(style.fill);
    bool has_border = style.border_width > 0 && !wlx_color_is_zero(style.border);
    if (!has_fill && !has_border) return;
    int segs = style.rounded_segments > 0 ? style.rounded_segments : ctx->theme->rounded_segments;
    if (style.roundness > 0) {
        if (has_fill)   wlx_draw_rect_rounded(ctx, rect, style.roundness, segs, style.fill);
        if (has_border) wlx_draw_rect_rounded_lines(ctx, rect, style.roundness, segs, style.border_width, style.border);
    } else {
        if (has_fill)   wlx_draw_rect(ctx, rect, style.fill);
        if (has_border) wlx_draw_rect_lines(ctx, rect, style.border_width, style.border);
    }
}

static void wlx_draw_layout_background(WLX_Context *ctx, WLX_Rect rect,
                                        WLX_Color back_color, WLX_Color border_color,
                                        float border_width, float roundness,
                                        int rounded_segments)
{
    bool has_bg = !wlx_color_is_zero(back_color);
    if (!has_bg && !(border_width > 0)) return;
    // When bg is set and border_width is positive, fall back to theme->border
    // if no explicit border_color was provided.
    if (has_bg && border_width > 0 && wlx_color_is_zero(border_color)) {
        border_color = ctx->theme->border;
    }
    wlx_draw_box(ctx, rect, (WLX_Box_Style){
        .fill            = back_color,
        .border          = border_color,
        .border_width    = border_width,
        .roundness       = roundness,
        .rounded_segments = rounded_segments,
    });
}

// Internal common options extracted from any layout opt type.
// Use WLX_LAYOUT_COMMON_OPT(opt) to build from WLX_Layout_Opt,
// WLX_Grid_Opt, or WLX_Grid_Auto_Opt.
typedef struct {
    int pos;
    size_t span;
    WLX_Color back_color;
    WLX_Color border_color;
    float border_width;
    float roundness;
    int rounded_segments;
    float padding;
    float padding_top;
    float padding_right;
    float padding_bottom;
    float padding_left;
    float gap;
    WLX_Color slot_back_color;
    WLX_Color slot_border_color;
    float slot_border_width;
    const char *id;
} WLX_Layout_Common_Opt;

#define WLX_LAYOUT_COMMON_OPT(opt) \
    (WLX_Layout_Common_Opt){ \
        .pos = (opt).pos, .span = (opt).span, \
        .back_color = (opt).back_color, .border_color = (opt).border_color, \
        .border_width = (opt).border_width, .roundness = (opt).roundness, \
        .rounded_segments = (opt).rounded_segments, \
        .padding = (opt).padding, .padding_top = (opt).padding_top, \
        .padding_right = (opt).padding_right, .padding_bottom = (opt).padding_bottom, \
        .padding_left = (opt).padding_left, .gap = (opt).gap, \
        .slot_back_color = (opt).slot_back_color, \
        .slot_border_color = (opt).slot_border_color, \
        .slot_border_width = (opt).slot_border_width, \
        .id = (opt).id, \
    }

// Internal per-layout-begin frame: captures common prologue results shared by
// all five layout-begin entry points.
typedef struct {
    WLX_Rect rect;                 // inset content rect (post-padding)
    WLX_Resolved_Padding padding;  // resolved padding values
    int cmd_range_idx;             // command range index for this layout
    float viewport_horz;           // horizontal viewport dimension for FILL resolution
    float viewport_vert;           // vertical viewport dimension for FILL resolution
    bool pushed_scope_id;          // true when co.id was non-NULL and a scope id was pushed
} WLX_Layout_Frame;

// Common prologue for all layout-begin entry points: selects the slot rect or
// root rect, closes any open sibling range, opens a new command range, draws
// the container decoration, resolves padding, insets the rect, and captures
// the active viewport dimensions for WLX_SIZE_FILL resolution.
static inline WLX_Layout_Frame wlx_layout_frame_begin(
    WLX_Context *ctx, WLX_Layout_Common_Opt co,
    const char *file, int line)
{
    (void)file; (void)line;
    if (co.id) wlx_push_id(ctx, wlx_hash_string(co.id));
    WLX_Rect r;
    if (ctx->arena.layouts.count <= 0) {
        r = ctx->rect;
    } else {
        r = wlx_get_slot_rect(ctx,
            &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1], co.pos, co.span);
    }
    wlx_cmd_close_sibling_range(ctx);
    int cmd_range_idx = wlx_cmd_open_range(ctx);
    wlx_draw_layout_background(ctx, r, co.back_color, co.border_color,
                               co.border_width, co.roundness, co.rounded_segments);
    WLX_Resolved_Padding p = wlx_resolve_padding(
        co.padding, co.padding_top, co.padding_right, co.padding_bottom, co.padding_left);
    r = wlx_rect_inset_sides(r, p.top, p.right, p.bottom, p.left);
    float viewport_horz, viewport_vert;
    if (ctx->arena.scroll_panels.count > 0) {
        WLX_Rect vp = wlx_pool_scroll_panels(ctx)[ctx->arena.scroll_panels.count - 1]->panel_rect;
        viewport_horz = vp.w;
        viewport_vert = vp.h;
    } else {
        viewport_horz = ctx->rect.w;
        viewport_vert = ctx->rect.h;
    }
    return (WLX_Layout_Frame){
        .rect            = r,
        .padding         = p,
        .cmd_range_idx   = cmd_range_idx,
        .viewport_horz   = viewport_horz,
        .viewport_vert   = viewport_vert,
        .pushed_scope_id = (co.id != NULL),
    };
}

// Applies common layout fields (padding, slot decoration, gap, command-range
// index) from co and frame to the already-created layout l.
// Viewport assignment is left to each entry point since it depends on orient.
static inline void wlx_layout_apply_common(
    WLX_Layout *l, WLX_Layout_Common_Opt co, WLX_Layout_Frame frame)
{
    l->padding           = co.padding;
    l->padding_top       = frame.padding.top;
    l->padding_bottom    = frame.padding.bottom;
    l->gap               = co.gap;
    l->slot_back_color   = co.slot_back_color;
    l->slot_border_color = co.slot_border_color;
    l->slot_border_width = co.slot_border_width;
    l->cmd_range_idx     = frame.cmd_range_idx;
}

// ============================================================================
// CONTENT sizing lifecycle helpers
// ============================================================================

// Returns true if any slot in sizes[0..count-1] has WLX_SIZE_CONTENT.
// Null-safe: returns false when sizes is NULL.
static inline bool wlx_has_content_sizes(const WLX_Slot_Size *sizes, size_t count) {
    if (sizes == NULL) return false;
    for (size_t i = 0; i < count; i++) {
        if (sizes[i].kind == WLX_SIZE_CONTENT) return true;
    }
    return false;
}

// Prepares CONTENT sizing for a layout or grid begin:
//  - asserts count <= WLX_CONTENT_SLOTS_MAX
//  - fetches persistent WLX_Content_Slot_State via wlx_get_state_impl
//  - fills resolved[0..count-1] replacing CONTENT -> PX(measured)
//  - copies original sizes to the byte scratch arena
// resolved must point to a caller-allocated buffer of at least WLX_CONTENT_SLOTS_MAX elements.
// Returns false (and zeroes out-params) when no CONTENT slot is found.
static inline bool wlx_prepare_content_sizes(
    WLX_Context *ctx,
    const WLX_Slot_Size *sizes, size_t count,
    const char *file, int line,
    WLX_Content_Slot_State **out_state,
    WLX_Slot_Size **out_sizes_copy,
    WLX_Slot_Size *resolved)
{
    *out_state     = NULL;
    *out_sizes_copy = NULL;

    if (!wlx_has_content_sizes(sizes, count)) return false;

    assert(count <= WLX_CONTENT_SLOTS_MAX && "CONTENT slots exceed WLX_CONTENT_SLOTS_MAX");
    
    WLX_State persistant = wlx_get_state_impl(ctx, sizeof(WLX_Content_Slot_State), file, line);
    WLX_Content_Slot_State *state = (WLX_Content_Slot_State *)persistant.data;
    *out_state = state;
    for (size_t i = 0; i < count; i++) {
        resolved[i] = sizes[i];
        if (sizes[i].kind == WLX_SIZE_CONTENT) {
            float h = state->measured[i];
            if (h <= 0) h = (sizes[i].min > 0.0f) ? 0.0f : 1.0f;
            resolved[i].kind  = WLX_SIZE_PIXELS;
            resolved[i].value = h;
        }
    }

    WLX_Slot_Size *sizes_copy = (WLX_Slot_Size *)wlx_scratch_alloc_bytes(
        ctx, count * sizeof(WLX_Slot_Size), _Alignof(WLX_Slot_Size));

    memcpy(sizes_copy, sizes, count * sizeof(WLX_Slot_Size));
    *out_sizes_copy = sizes_copy;
    
    return true;
}

// Allocates a zeroed float buffer of `count` floats from the slot-size-offsets
// scratch arena. Writes the buffer's starting index to *out_offset.
// Returns a pointer to the first element.
static inline float *wlx_alloc_content_measure_buffer(
    WLX_Context *ctx, size_t count, size_t *out_offset)
{
    *out_offset = ctx->arena.slot_size_offsets.count;
    float *buf = wlx_scratch_alloc(ctx, count);
    wlx_zero_array(count, buf);
    return buf;
}

// Writes accumulated per-slot or per-row content measurements back to the
// persistent WLX_Content_Slot_State for the layout l.
// No-op when l->content_state is NULL or no measurement buffer is present.
static inline void wlx_write_content_measurements(WLX_Context *ctx, WLX_Layout *l) {
    if (l->content_state == NULL || l->content_sizes == NULL) return;
    if (l->kind == WLX_LAYOUT_GRID && l->has_grid_row_content_heights) {
        const float *rch = wlx_grid_row_content_heights(ctx, l);
        for (size_t r = 0; r < l->grid.rows; r++) {
            if (l->content_sizes[r].kind == WLX_SIZE_CONTENT) {
                l->content_state->measured[r] = rch[r];
            }
        }
    } else if (l->has_content_slot_heights) {
        const float *csh = wlx_layout_content_heights(ctx, l);
        for (size_t i = 0; i < l->count; i++) {
            if (l->content_sizes[i].kind == WLX_SIZE_CONTENT) {
                l->content_state->measured[i] = csh[i];
            }
        }
    }
}

WLXDEF void wlx_layout_begin_impl(WLX_Context *ctx, size_t count, WLX_Orient orient, WLX_Layout_Opt opt,
                                   const char *file, int line) {
    WLX_Layout_Frame frame = wlx_layout_frame_begin(ctx, WLX_LAYOUT_COMMON_OPT(opt), file, line);

    WLX_Layout l = wlx_create_layout(ctx, frame.rect, count, orient, opt.gap);
    wlx_layout_apply_common(&l, WLX_LAYOUT_COMMON_OPT(opt), frame);
    l.viewport = (orient == WLX_HORZ) ? frame.viewport_horz : frame.viewport_vert;

    // --- CONTENT slot pre-resolution ---
    WLX_Content_Slot_State *cstate = NULL;
    WLX_Slot_Size *sizes_copy = NULL;
    WLX_Slot_Size resolved[WLX_CONTENT_SLOTS_MAX];
    if (wlx_prepare_content_sizes(ctx, opt.sizes, count, file, line,
            &cstate, &sizes_copy, resolved)) {
        wlx_compute_slot_offsets(ctx, &l, resolved);
        size_t slot_heights_off;
        wlx_alloc_content_measure_buffer(ctx, count, &slot_heights_off);
        l.content_sizes = sizes_copy;
        l.content_sizes_scratch_off = (size_t)((uint8_t *)sizes_copy - wlx_pool_scratch(ctx));
        l.content_state = cstate;
        l.content_slot_heights_off = slot_heights_off;
        l.has_content_slot_heights = true;
    } else if (opt.sizes != NULL) {
        wlx_compute_slot_offsets(ctx, &l, opt.sizes);
    }

    l.pushed_scope_id = frame.pushed_scope_id;
    wlx_pool_push(&ctx->arena.layouts, WLX_Layout, l);
    // layout_begin(ctx, vb_force, sizes, count, orient, pos, span, file, line)
    //   vb_force=-1: auto-detect vert_bounded from parent chain
    //   sizes: original slot sizes array (for FLEX/FILL warning + child vert_bounded checks)
    //   count: number of slots
    //   orient: WLX_VERT/WLX_HORZ (only VERT triggers FLEX/FILL warnings)
    //   pos/span: slot position in parent (for parent-slot-kind bounded check)
    //   file/line: caller location for warning deduplication
    WLX_DBG(layout_begin, ctx, -1, opt.sizes, count, (int)orient, opt.pos, opt.span, file, line);
}

// wlx_layout_begin_auto_impl - dynamic variant of wlx_layout_begin_impl.
// The layout's slot count starts at 0 and grows by one for each direct child
// widget or nested layout_begin call.  slot_px controls the fixed pixel size
// of every slot along the layout axis.
WLXDEF void wlx_layout_begin_auto_impl(WLX_Context *ctx, WLX_Orient orient, float slot_px, WLX_Layout_Opt opt) {
    WLX_Layout_Frame frame = wlx_layout_frame_begin(ctx, WLX_LAYOUT_COMMON_OPT(opt), NULL, 0);

    WLX_Layout l = wlx_create_layout_auto(ctx, frame.rect, orient, slot_px);
    wlx_layout_apply_common(&l, WLX_LAYOUT_COMMON_OPT(opt), frame);
    l.viewport = (orient == WLX_HORZ) ? frame.viewport_horz : frame.viewport_vert;

    l.pushed_scope_id = frame.pushed_scope_id;
    wlx_pool_push(&ctx->arena.layouts, WLX_Layout, l);
    // layout_begin: auto layouts have no sizes array - inherit parent vert_bounded only
    WLX_DBG(layout_begin, ctx, -1, NULL, 0, 0, -1, 1, NULL, 0);
}

WLXDEF void wlx_grid_begin_impl(WLX_Context *ctx, size_t rows, size_t cols, WLX_Grid_Opt opt,
                                const char *file, int line) {
    WLX_Layout_Frame frame = wlx_layout_frame_begin(ctx, WLX_LAYOUT_COMMON_OPT(opt), file, line);

    // --- CONTENT row pre-resolution ---
    WLX_Content_Slot_State *cstate = NULL;
    WLX_Slot_Size *sizes_copy = NULL;
    WLX_Slot_Size resolved[WLX_CONTENT_SLOTS_MAX];
    const WLX_Slot_Size *effective_row_sizes = opt.row_sizes;
    if (wlx_prepare_content_sizes(ctx, opt.row_sizes, rows, file, line,
            &cstate, &sizes_copy, resolved)) {
        effective_row_sizes = resolved;
    }

    WLX_Layout l = wlx_create_grid(ctx, frame.rect, rows, cols,
                                         effective_row_sizes, opt.col_sizes, opt.gap);
    wlx_layout_apply_common(&l, WLX_LAYOUT_COMMON_OPT(opt), frame);

    if (cstate != NULL) {
        l.content_sizes = sizes_copy;
        l.content_sizes_scratch_off = (size_t)((uint8_t *)sizes_copy - wlx_pool_scratch(ctx));
        l.content_state = cstate;
    }

    // Always allocate per-row content tracking. Used both for CONTENT-row
    // measurement write-back and for intrinsic-height contribution when
    // the grid's own rect is starved (frame-1 bootstrap inside CONTENT slot).
    if (rows > 0) {
        size_t rch_off;
        wlx_alloc_content_measure_buffer(ctx, rows, &rch_off);
        l.grid_row_content_heights_off = rch_off;
        l.has_grid_row_content_heights = true;
    }

    l.pushed_scope_id = frame.pushed_scope_id;
    wlx_pool_push(&ctx->arena.layouts, WLX_Layout, l);
    // layout_begin: grid layouts - inherit parent vert_bounded, no per-slot FLEX/FILL check
    WLX_DBG(layout_begin, ctx, -1, NULL, 0, 0, -1, 1, NULL, 0);
}

WLXDEF void wlx_grid_begin_auto_impl(WLX_Context *ctx, size_t cols, float row_px, WLX_Grid_Auto_Opt opt) {
    WLX_Layout_Frame frame = wlx_layout_frame_begin(ctx, WLX_LAYOUT_COMMON_OPT(opt), NULL, 0);

    WLX_Layout l = wlx_create_grid_auto(ctx, frame.rect, cols, row_px, opt.col_sizes, opt.gap);
    wlx_layout_apply_common(&l, WLX_LAYOUT_COMMON_OPT(opt), frame);
    l.pushed_scope_id = frame.pushed_scope_id;
    wlx_pool_push(&ctx->arena.layouts, WLX_Layout, l);
    // layout_begin: grid auto - inherit parent vert_bounded
    WLX_DBG(layout_begin, ctx, -1, NULL, 0, 0, -1, 1, NULL, 0);
}

WLXDEF void wlx_grid_begin_auto_tile_impl(WLX_Context *ctx, float tile_w, float tile_h, WLX_Grid_Auto_Opt opt) {
    assert(tile_w > 0.0f && "tile width must be positive");
    assert(tile_h > 0.0f && "tile height must be positive");

    WLX_Layout_Frame frame = wlx_layout_frame_begin(ctx, WLX_LAYOUT_COMMON_OPT(opt), NULL, 0);

    size_t cols = (opt.gap > 0.0f)
        ? (size_t)((frame.rect.w + opt.gap) / (tile_w + opt.gap))
        : (size_t)(frame.rect.w / tile_w);
    if (cols < 1) cols = 1;

    WLX_Layout l = wlx_create_grid_auto(ctx, frame.rect, cols, tile_h, opt.col_sizes, opt.gap);
    wlx_layout_apply_common(&l, WLX_LAYOUT_COMMON_OPT(opt), frame);
    l.pushed_scope_id = frame.pushed_scope_id;
    wlx_pool_push(&ctx->arena.layouts, WLX_Layout, l);
    // layout_begin: grid auto tile - inherit parent vert_bounded
    WLX_DBG(layout_begin, ctx, -1, NULL, 0, 0, -1, 1, NULL, 0);
}

// Scope-pop half of the layout frame lifecycle: pops the layout from the stack
// and pops the scope id if one was pushed at begin.
static inline void wlx_layout_frame_end(WLX_Context *ctx, WLX_Layout *l) {
    bool pop_scope = l->pushed_scope_id;
    ctx->arena.layouts.count -= 1;
    if (pop_scope) wlx_pop_id(ctx);
}

WLXDEF void wlx_layout_end(WLX_Context *ctx) {
    assert(ctx != NULL);
    assert(ctx->arena.layouts.count > 0);
    WLX_Layout *l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];

    // Reconstruct content_sizes pointer: the byte scratch arena may have
    // reallocated since layout_begin, invalidating the original pointer.
    if (l->content_sizes != NULL) {
        l->content_sizes = (const WLX_Slot_Size *)&wlx_pool_scratch(ctx)[l->content_sizes_scratch_off];
    }

    // Close any open child widget/layout ranges (may span multiple levels
    // when scroll panels sit between this layout and its children)
    while (l->cmd_range_idx >= 0
        && ctx->current_range_idx >= 0
        && ctx->current_range_idx != l->cmd_range_idx) {
        wlx_pool_cmd_ranges(ctx)[ctx->current_range_idx].end_idx = ctx->arena.commands.count;
        ctx->current_range_idx = wlx_pool_cmd_ranges(ctx)[ctx->current_range_idx].parent_range_idx;
    }

    // --- Write CONTENT slot measurements to persistent state ---
    WLX_DBG(layout_end, ctx);
    wlx_write_content_measurements(ctx, l);

    // For grids, the authoritative content height is the total row offset.
    // Override the per-widget accumulation (which naively sums every cell
    // in a multi-column grid).  When the grid was starved of space (parent
    // gave it 0px on frame 1 inside a CONTENT slot), fall back to the
    // intrinsic per-row content sum so the parent can measure us correctly.
    // Must run before parent contribution so parent CONTENT slots see the
    // corrected value.
    if (l->kind == WLX_LAYOUT_GRID && l->grid.rows > 0) {
        float row_total = wlx_grid_row_offsets(ctx, l)[l->grid.rows];
        float intrinsic = 0.0f;
        if (l->has_grid_row_content_heights) {
            const float *rch = wlx_grid_row_content_heights(ctx, l);
            for (size_t r = 0; r < l->grid.rows; r++) {
                intrinsic += rch[r];
            }
            if (l->gap > 0.0f && l->grid.rows > 1) {
                intrinsic += l->gap * (float)(l->grid.rows - 1);
            }
        }
        l->accumulated_content_height = (intrinsic > row_total) ? intrinsic : row_total;
    }

    // --- Contribute this layout's content height to parent CONTENT tracking ---
    if (ctx->arena.layouts.count > 1) {
        WLX_Layout *parent = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 2];
        if (parent->has_content_slot_heights && parent->index > 0) {
            size_t slot_idx = parent->index - 1;
            if (slot_idx < WLX_CONTENT_SLOTS_MAX) {
                wlx_layout_content_heights(ctx, parent)[slot_idx] +=
                    l->accumulated_content_height + l->padding_top + l->padding_bottom;
            }
        }
        // Grid parent: contribute to per-row max content height
        if (parent->has_grid_row_content_heights) {
            size_t row = parent->grid.last_placed_row;
            if (row < parent->grid.rows) {
                float child_h = l->accumulated_content_height + l->padding_top + l->padding_bottom;
                float *rch = wlx_grid_row_content_heights(ctx, parent);
                if (child_h > rch[row])
                    rch[row] = child_h;
            }
        }
    }

    // For VERT linear layouts, add gap contribution to content height.
    // Gap is between children, so total gap = gap * (children - 1).
    if (l->kind == WLX_LAYOUT_LINEAR && l->linear.orient == WLX_VERT
        && l->gap > 0.0f && l->index > 1) {
        l->accumulated_content_height += l->gap * (float)(l->index - 1);
    }

    // --- Propagate content height to parent layout ---
    // Instead of contributing to auto_scroll.total_height per-layout (which
    // caused double-counting for nested layouts), fold this layout's content
    // height into the parent's accumulated_content_height.  The wrapper
    // layout in wlx_scroll_panel_end reads the fully-aggregated tree total.
    if (ctx->arena.layouts.count > 1) {
        WLX_Layout *parent = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 2];
        float child_h = l->accumulated_content_height + l->padding_top + l->padding_bottom;
        if (parent->kind == WLX_LAYOUT_LINEAR
            && parent->linear.orient == WLX_HORZ) {
            if (child_h > parent->accumulated_content_height)
                parent->accumulated_content_height = child_h;
        } else {
            parent->accumulated_content_height += child_h;
        }
    }

    // --- Compute CONTENT slot deltas for deferred replay ---
    // Gated on !immediate_mode: scratch pointers (content_sizes, content_slot_heights)
    // may be invalidated by scratch buffer reallocation in nested layout_begin calls.
    // Safe once deferred mode manages scratch lifetime.
    if (!ctx->immediate_mode && l->cmd_range_idx >= 0 && l->content_state != NULL
        && l->content_sizes != NULL && l->kind == WLX_LAYOUT_LINEAR
        && l->linear.orient == WLX_VERT && l->has_content_slot_heights) {
        const float *csh = wlx_layout_content_heights(ctx, l);
        WLX_Slot_Size resolved[WLX_CONTENT_SLOTS_MAX];
        for (size_t i = 0; i < l->count; i++) {
            resolved[i] = l->content_sizes[i];
            if (l->content_sizes[i].kind == WLX_SIZE_CONTENT) {
                resolved[i].kind = WLX_SIZE_PIXELS;
                resolved[i].value = csh[i];
            }
        }
        float new_offsets[WLX_CONTENT_SLOTS_MAX + 1];
        float total = l->rect.h;
        wlx_compute_offsets(new_offsets, l->count, total, l->viewport, resolved, l->gap);

        // Walk child ranges of this layout and assign per-slot deltas.
        // Child ranges appear in slot order by construction.
        const float *offsets = wlx_layout_offsets(ctx, l);
        size_t slot = 0;
        for (size_t ri = 0; ri < ctx->arena.cmd_ranges.count && slot < l->count; ri++) {
            if (wlx_pool_cmd_ranges(ctx)[ri].parent_range_idx == l->cmd_range_idx) {
                wlx_pool_cmd_ranges(ctx)[ri].dy_offset = new_offsets[slot] - offsets[slot];
                slot++;
            }
        }
    }

    // Close this layout's range
    if (l->cmd_range_idx >= 0) {
        assert(ctx->current_range_idx == l->cmd_range_idx);
        wlx_pool_cmd_ranges(ctx)[l->cmd_range_idx].end_idx = ctx->arena.commands.count;
        ctx->current_range_idx = wlx_pool_cmd_ranges(ctx)[l->cmd_range_idx].parent_range_idx;
    }

    wlx_layout_frame_end(ctx, l);
}

WLXDEF WLX_Rect wlx_get_parent_rect(WLX_Context *ctx) {
    assert(ctx != NULL);
    if (ctx->arena.layouts.count > 0) {
        return wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1].rect;
    }
    return ctx->rect;
}

WLXDEF WLX_Rect wlx_get_scroll_panel_viewport(WLX_Context *ctx) {
    assert(ctx != NULL);
    if (ctx->arena.scroll_panels.count > 0) {
        return wlx_pool_scroll_panels(ctx)[ctx->arena.scroll_panels.count - 1]->panel_rect;
    }
    return (WLX_Rect){0};
}

// Resolve a WLX_Slot_Size to pixels and set it as the next slot size in the
// innermost dynamic layout.  FLEX/AUTO are greedy (take all remaining space).
WLXDEF void wlx_layout_auto_slot(WLX_Context *ctx, WLX_Slot_Size size) {
    assert(ctx != NULL);
    assert(ctx->arena.layouts.count > 0 && "wlx_layout_auto_slot called outside a layout");

    WLX_Layout *l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
    assert(l->linear.dynamic && "wlx_layout_auto_slot is only valid inside a wlx_layout_begin_auto block");

    float total = (l->linear.orient == WLX_HORZ) ? l->rect.w : l->rect.h;
    float used  = (l->count > 0) ? wlx_layout_offsets(ctx, l)[l->count] : 0.0f;
    float px;

    switch (size.kind) {
        case WLX_SIZE_PIXELS:  px = size.value; break;
        case WLX_SIZE_PERCENT: px = (size.value * total) / 100.0f; break;
        case WLX_SIZE_FILL:    px = size.value * l->viewport; break;
        case WLX_SIZE_FLEX:    px = (total - used > 0.0f) ? (total - used) : 0.0f; break;
        case WLX_SIZE_AUTO:    px = (total - used > 0.0f) ? (total - used) : 0.0f; break;
        case WLX_SIZE_CONTENT: px = size.value; break;  // pre-resolved or 0
        default: px = 0.0f; break;
    }

    // Apply min/max constraints
    if (size.min > 0.0f && px < size.min) px = size.min;
    if (size.max > 0.0f && px > size.max) px = size.max;
    if (px < 1.0f) {
        WLX_DBG(auto_slot, ctx, px, size.kind, total, used);
        px = 1.0f;
    }

    l->linear.next_slot_size = px;
}

// Convenience wrapper: set the next slot to a fixed pixel size.
WLXDEF void wlx_layout_auto_slot_px(WLX_Context *ctx, float px) {
    wlx_layout_auto_slot(ctx, WLX_SLOT_PX(px));
}

WLXDEF void wlx_grid_auto_row_px(WLX_Context *ctx, float px) {
    assert(ctx != NULL);
    assert(ctx->arena.layouts.count > 0 && "wlx_grid_auto_row_px called outside a layout");
    assert(px > 0.0f && "wlx_grid_auto_row_px requires a positive pixel size");

    WLX_Layout *l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
    assert(l->kind == WLX_LAYOUT_GRID && l->grid.dynamic &&
           "wlx_grid_auto_row_px is only valid inside a wlx_grid_begin_auto block");

    l->grid.next_row_size = px;
}

WLXDEF void wlx_grid_cell_impl(WLX_Context *ctx, int row, int col, WLX_Slot_Style_Opt opt) {
    assert(ctx != NULL);
    assert(ctx->arena.layouts.count > 0 && "grid_cell() called outside a layout");

    WLX_Layout *l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
    assert(l->kind == WLX_LAYOUT_GRID && "grid_cell() is only valid inside a grid_begin block");

    // -1 means "use cursor position for that axis"
    size_t r = (row < 0) ? l->grid.cursor_row : (size_t)row;
    size_t c = (col < 0) ? l->grid.cursor_col : (size_t)col;

    size_t rs = (opt.row_span == 0) ? 1 : opt.row_span;
    size_t cs = (opt.col_span == 0) ? 1 : opt.col_span;
    assert(r + rs <= l->grid.rows && "grid_cell row + row_span out of bounds");
    assert(c + cs <= l->grid.cols && "grid_cell col + col_span out of bounds");

    l->grid.cell_set      = true;
    l->grid.next_row      = r;
    l->grid.next_col      = c;
    l->grid.next_row_span = rs;
    l->grid.next_col_span = cs;

    if (!wlx_color_is_zero(opt.back_color) || opt.border_width > 0) {
        l->grid.next_cell_back_color   = opt.back_color;
        l->grid.next_cell_border_color = opt.border_color;
        l->grid.next_cell_border_width = opt.border_width;
    }
}

WLXDEF void wlx_slot_style_impl(WLX_Context *ctx, WLX_Slot_Style_Opt opt) {
    assert(ctx != NULL);
    assert(ctx->arena.layouts.count > 0 && "wlx_slot_style() called outside a layout");
    WLX_Layout *l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
    assert(l->kind == WLX_LAYOUT_LINEAR && "wlx_slot_style() is only valid inside a linear layout");
    l->linear.next_slot_back_color   = opt.back_color;
    l->linear.next_slot_border_color = opt.border_color;
    l->linear.next_slot_border_width = opt.border_width;
}

WLXDEF void wlx_grid_cell_style_impl(WLX_Context *ctx, WLX_Slot_Style_Opt opt) {
    assert(ctx != NULL);
    assert(ctx->arena.layouts.count > 0 && "wlx_grid_cell_style() called outside a layout");
    WLX_Layout *l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
    assert(l->kind == WLX_LAYOUT_GRID && "wlx_grid_cell_style() is only valid inside a grid layout");
    l->grid.next_cell_back_color   = opt.back_color;
    l->grid.next_cell_border_color = opt.border_color;
    l->grid.next_cell_border_width = opt.border_width;
}

WLXDEF WLX_Rect wlx_get_align_rect(WLX_Rect parent_rect, float width, float height, WLX_Align align) {
    if (width < 0 && height < 0)
        return parent_rect;

    WLX_Rect r = parent_rect;

    if (width >= 0 && parent_rect.w > width)
        r.w = width;
    if (height >= 0 && parent_rect.h > height)
        r.h = height;

    // Compute horizontal position
    switch (align) {
        // center horizontally
        case WLX_CENTER: case WLX_TOP: case WLX_BOTTOM:
        case WLX_TOP_CENTER: case WLX_BOTTOM_CENTER:
            if (width > 0 && parent_rect.w > width)
                r.x = parent_rect.x + (parent_rect.w - width) / 2.0f;
            break;
        // align right
        case WLX_RIGHT: case WLX_TOP_RIGHT: case WLX_BOTTOM_RIGHT:
            if (width > 0 && parent_rect.w > width)
                r.x = parent_rect.x + (parent_rect.w - width);
            break;
        // align left (or no alignment) - r.x already equals parent_rect.x
        default:
            break;
    }

    // Compute vertical position
    switch (align) {
        // center vertically
        case WLX_CENTER: case WLX_LEFT: case WLX_RIGHT:
            if (height > 0 && parent_rect.h > height)
                r.y = parent_rect.y + (parent_rect.h - height) / 2.0f;
            break;
        // align bottom
        case WLX_BOTTOM: case WLX_BOTTOM_LEFT: case WLX_BOTTOM_RIGHT:
        case WLX_BOTTOM_CENTER:
            if (height > 0 && parent_rect.h > height)
                r.y = parent_rect.y + (parent_rect.h - height);
            break;
        // align top (or no alignment) - r.y already equals parent_rect.y
        default:
            break;
    }

    return r;
}

// ---------------------------------------------------------------------------
// ID stack for loop disambiguation
// ---------------------------------------------------------------------------
static size_t wlx_id_stack_hash(WLX_Context *ctx) {
    size_t h = 0;
    for (size_t i = 0; i < ctx->arena.id_stack.count; i++) {
        h = h * 2654435761u ^ wlx_pool_id_stack(ctx)[i];
    }
    return h;
}

// Asymmetric hash combine (Boost hash_combine) - avoids the collision-prone
// plain-XOR that let different (file:line, id_stack) pairs produce the same ID.
static inline size_t wlx_combine_id_hash(size_t base, size_t stack) {
    if (stack == 0) return base; // fast path - no push_id active
    return base ^ (stack + 0x9e3779b9u + (base << 6) + (base >> 2));
}

WLXDEF void wlx_push_id(WLX_Context *ctx, size_t id) {
    wlx_pool_push(&ctx->arena.id_stack, size_t, id);
}

WLXDEF void wlx_pop_id(WLX_Context *ctx) {
    assert(ctx->arena.id_stack.count > 0);
    ctx->arena.id_stack.count--;
}

WLXDEF void wlx_push_opacity(WLX_Context *ctx, float opacity) {
    float parent = wlx_get_opacity(ctx);
    wlx_pool_push(&ctx->arena.opacity_stack, float, parent * opacity);
}

WLXDEF void wlx_pop_opacity(WLX_Context *ctx) {
    assert(ctx->arena.opacity_stack.count > 0);
    ctx->arena.opacity_stack.count--;
}

WLXDEF float wlx_get_opacity(const WLX_Context *ctx) {
    if (ctx->arena.opacity_stack.count == 0) return 1.0f;
    return wlx_pool_opacity_stack(ctx)[ctx->arena.opacity_stack.count - 1];
}

// ---------------------------------------------------------------------------
// Unified interaction handler
// ---------------------------------------------------------------------------
// Handles all common widget interaction patterns through composable flags.
// Use exactly ONE of WLX_INTERACT_CLICK, WLX_INTERACT_FOCUS, or WLX_INTERACT_DRAG
// to select the primary interaction mode. Combine with WLX_INTERACT_HOVER and
// WLX_INTERACT_KEYBOARD as needed.
//
// ID model:
//   Both interaction IDs and persistent state IDs = hash(file, line) ^ id_stack_hash.
//   Use wlx_push_id()/wlx_pop_id() when the same source line is reached
//   multiple times (loops, reusable widget functions).
//
// Modes:
//   CLICK - Button-like: activate on mouse click, fire "clicked" on release while hovering.
//   FOCUS - Input-like: click to focus, click elsewhere or press Enter to unfocus.
//   DRAG  - Slider-like: activate on mouse down, stay active while held, deactivate on release.
//
static inline size_t wlx_interaction_make_id(WLX_Context *ctx, const char *file, int line) {
    size_t base = wlx_combine_id_hash(wlx_hash_id(file, line), wlx_id_stack_hash(ctx));
    WLX_DBG(interaction_id, ctx, base, file, line);
    return (base == 0) ? 1 : base;  // reserve 0 for "no widget"
}

static inline bool wlx_interaction_mouse_over(WLX_Context *ctx, WLX_Rect rect) {
    if (!wlx_point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, rect.x, rect.y, rect.w, rect.h))
        return false;
    // Clip against all active scroll panel viewports so that widgets
    // scrolled out of view cannot claim mouse interaction.
    for (size_t i = 0; i < ctx->arena.scroll_panels.count; i++) {
        WLX_Rect vp = wlx_pool_scroll_panels(ctx)[i]->panel_rect;
        if (!wlx_point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, vp.x, vp.y, vp.w, vp.h))
            return false;
    }
    return true;
}

static inline void wlx_interaction_compute_hover(WLX_Context *ctx, size_t id, bool mouse_over, WLX_Interaction *result) {
    if (mouse_over && (ctx->interaction.active_id == 0 || ctx->interaction.active_id == id)) {
        ctx->interaction.hot_id = id;
    }

    result->hover = (ctx->interaction.hot_id == id);
}

static inline void wlx_interaction_handle_click(WLX_Context *ctx, size_t id, bool mouse_over, WLX_Interaction *result) {
    if (mouse_over && ctx->interaction.active_id == 0 && ctx->input.mouse_clicked) {
        ctx->interaction.active_id = id;
    }

    result->active = (ctx->interaction.active_id == id);
    result->pressed = result->active && ctx->input.mouse_down;

    if (ctx->interaction.active_id == id && !ctx->input.mouse_down) {
        if (ctx->interaction.hot_id == id) {
            result->clicked = true;
        }
        ctx->interaction.active_id = 0;
        result->active = false;
    }
}

static inline void wlx_interaction_handle_focus(WLX_Context *ctx, size_t id, bool mouse_over, WLX_Interaction *result) {
    bool was_focused = (ctx->interaction.active_id == id);
    result->focused = was_focused;

    if (ctx->input.mouse_clicked) {
        if (mouse_over) {
            ctx->interaction.active_id = id;
            result->focused = true;
            if (!was_focused) {
                result->just_focused = true;
            }
        } else if (was_focused) {
            ctx->interaction.active_id = 0;
            result->focused = false;
            result->just_unfocused = true;
        }
    }

    if (result->focused && wlx_is_key_pressed(ctx, WLX_KEY_ENTER)) {
        ctx->interaction.active_id = 0;
        result->focused = false;
        result->just_unfocused = true;
    }

    result->active = result->focused;
}

static inline void wlx_interaction_handle_drag(WLX_Context *ctx, size_t id, bool mouse_over, WLX_Interaction *result) {
    if (mouse_over && ctx->interaction.active_id == 0 && ctx->input.mouse_down) {
        ctx->interaction.active_id = id;
    }

    if (ctx->interaction.active_id == id) {
        if (ctx->input.mouse_held) {
            result->active = true;
            result->pressed = true;
        } else {
            ctx->interaction.active_id = 0;
            result->active = false;
        }
    }
}

static inline void wlx_interaction_handle_keyboard(WLX_Context *ctx, size_t id, WLX_Interaction *result) {
    if (ctx->interaction.hot_id == id) {
        if (wlx_is_key_pressed(ctx, WLX_KEY_SPACE) || wlx_is_key_pressed(ctx, WLX_KEY_ENTER)) {
            result->clicked = true;
        }
    }
}

WLXDEF WLX_Interaction wlx_get_interaction(WLX_Context *ctx, WLX_Rect rect, uint32_t flags, const char *file, int line) {
    // ID = hash(file, line) ^ id_stack_hash.
    // Use wlx_push_id()/wlx_pop_id() for loop disambiguation.
    size_t id = wlx_interaction_make_id(ctx, file, line);
    WLX_Interaction result = { .id = id };

    bool mouse_over = wlx_interaction_mouse_over(ctx, rect);

    if (flags & WLX_INTERACT_HOVER) {
        wlx_interaction_compute_hover(ctx, id, mouse_over, &result);
    }

    if (flags & WLX_INTERACT_CLICK) {
        wlx_interaction_handle_click(ctx, id, mouse_over, &result);
    }

    if (flags & WLX_INTERACT_FOCUS) {
        wlx_interaction_handle_focus(ctx, id, mouse_over, &result);
    }

    if (flags & WLX_INTERACT_DRAG) {
        wlx_interaction_handle_drag(ctx, id, mouse_over, &result);
    }

    if (flags & WLX_INTERACT_KEYBOARD) {
        wlx_interaction_handle_keyboard(ctx, id, &result);
    }

    // Track whether the currently active widget was seen this frame.
    // Done AFTER handlers so newly-activated widgets are also tracked.
    if (ctx->interaction.active_id != 0 && ctx->interaction.active_id == id) {
        ctx->interaction.active_id_seen = true;
    }

    return result;
}

// ---------------------------------------------------------------------------
// State map helpers (open-addressing hashmap)
// ---------------------------------------------------------------------------

// Find slot for `id` - returns pointer to the slot (empty or matching)
static WLX_State_Map_Slot *wlx_state_map_find(WLX_State_Map *map, size_t id) {
    size_t mask = map->capacity - 1;
    size_t idx = id & mask;
    for (;;) {
        WLX_State_Map_Slot *slot = &map->slots[idx];
        if (slot->id == 0 || slot->id == id) return slot;
        idx = (idx + 1) & mask;
    }
}

// Grow and rehash when load factor exceeded
static void wlx_state_map_grow(WLX_State_Map *map) {
    size_t old_cap = map->capacity;
    WLX_State_Map_Slot *old_slots = map->slots;

    map->capacity = (old_cap == 0) ? WLX_STATE_MAP_INIT_CAP : old_cap * 2;
    map->slots = (WLX_State_Map_Slot *)wlx_calloc(map->capacity, sizeof(WLX_State_Map_Slot));
    assert(map->slots != NULL && "Unable to allocate more RAM");
    // count stays the same

    for (size_t i = 0; i < old_cap; i++) {
        if (old_slots[i].id != 0) {
            WLX_State_Map_Slot *dst = wlx_state_map_find(map, old_slots[i].id);
            *dst = old_slots[i];
        }
    }
    wlx_free(old_slots);
}

// Insert-or-find: returns pointer to the slot
static WLX_State_Map_Slot *wlx_state_map_get(WLX_State_Map *map, size_t id, size_t data_size) {
    if (map->capacity == 0 || (float)(map->count + 1) > (float)map->capacity * WLX_STATE_MAP_MAX_LOAD) {
        wlx_state_map_grow(map);
    }
    WLX_State_Map_Slot *slot = wlx_state_map_find(map, id);
    if (slot->id == 0) {
        // New entry
        slot->id = id;
        slot->data = wlx_calloc(1, data_size);
        assert(slot->data != NULL && "Unable to allocate more RAM");
        slot->data_size = data_size;
        map->count++;
    }
    return slot;
}

// ---------------------------------------------------------------------------
// Generic persistent state
// ---------------------------------------------------------------------------
// Uses wlx_hash_id(file, line) combined with the ID stack - no sequential counter.
// This makes state IDs fully stable across frames. The ID stack (wlx_push_id/
// wlx_pop_id) provides disambiguation when the same source line is hit multiple
// times (e.g. widgets generated in a loop). This differs from interaction IDs,
// which add a frame-local sequence so repeated interaction queries in one frame
// do not collide.
WLXDEF WLX_State wlx_get_state_impl(WLX_Context *ctx, size_t state_size, const char *file, int line) {
    size_t id = wlx_combine_id_hash(wlx_hash_id(file, line), wlx_id_stack_hash(ctx));
    if (id == 0) id = 1;

    WLX_State_Map_Slot *slot = wlx_state_map_get(&ctx->states, id, state_size);
    assert(slot->data_size == state_size);
    return (WLX_State){ .id = id, .data = slot->data };
}

static inline void wlx_resolve_roundness(const WLX_Theme *theme, float *roundness, int *segments);
static void wlx_resolve_opt_widget(const WLX_Context *ctx, WLX_Widget_Opt *opt);

WLXDEF void wlx_widget_impl(WLX_Context *ctx, WLX_Widget_Opt opt, const char *file, int line)
{
    wlx_resolve_opt_widget(ctx, &opt);

    // Prologue: compute widget frame and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;

    WLX_Interaction inter = wlx_get_interaction(
        ctx,
        wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        file, line
    );

    if (inter.hover) {
        opt.color        = wlx_color_brightness(opt.color,        ctx->theme->hover_brightness);
        opt.border_color = wlx_color_brightness(opt.border_color, ctx->theme->hover_brightness);
    }

    WLX_Rect rect = {
        .x = ceilf(wr.x),
        .y = ceilf(wr.y),
        .w = ceilf(wr.w),
        .h = ceilf(wr.h),
    };

    wlx_draw_box(ctx, rect, (WLX_Box_Style){
        .fill             = opt.color,
        .border           = opt.border_color,
        .border_width     = opt.border_width,
        .roundness        = opt.roundness,
        .rounded_segments = opt.rounded_segments,
    });

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);
}

// ============================================================================
// Implementation: text layout and rendering helpers
// ============================================================================
// Text helpers in this section are implementation-owned helpers and use the
// canonical `wlx_` prefix.

// This function returns how many bytes the next UTF-8 character uses, based on the first byte in s.
// UTF-8 leading-byte patterns:
//       0xxxxxxx -> 1 byte (ASCII)
//       Checked by (c & 0x80) == 0x00
//       110xxxxx -> 2 bytes
//       Checked by (c & 0xE0) == 0xC0
//       1110xxxx -> 3 bytes
//       Checked by (c & 0xF0) == 0xE0
//       11110xxx -> 4 bytes
//       Checked by (c & 0xF8) == 0xF0
//       return 1; at the end is a fallback for invalid/unsupported lead bytes.
static inline size_t wlx_utf8_char_len(const char *s) {
    unsigned char c = (unsigned char)s[0];
    if ((c & 0x80u) == 0x00u) return 1;
    if ((c & 0xE0u) == 0xC0u) return 2;
    if ((c & 0xF0u) == 0xE0u) return 3;
    if ((c & 0xF8u) == 0xF0u) return 4;
    return 1;
}

// Decode one UTF-8 codepoint from s.  Store codepoint in *cp.
// Returns number of bytes consumed (1-4).
// On invalid sequence: returns 1 and sets *cp = 0xFFFD (replacement char).
static inline size_t wlx_utf8_decode(const char *s, uint32_t *cp) {
    const unsigned char *b = (const unsigned char *)s;
    if (b[0] < 0x80) {
        *cp = b[0];
        return 1;
    }
    if ((b[0] & 0xE0) == 0xC0) {
        if ((b[1] & 0xC0) != 0x80) { *cp = 0xFFFD; return 1; }
        *cp = ((uint32_t)(b[0] & 0x1F) << 6) | (b[1] & 0x3F);
        return 2;
    }
    if ((b[0] & 0xF0) == 0xE0) {
        if ((b[1] & 0xC0) != 0x80 || (b[2] & 0xC0) != 0x80) { *cp = 0xFFFD; return 1; }
        *cp = ((uint32_t)(b[0] & 0x0F) << 12) | ((uint32_t)(b[1] & 0x3F) << 6) | (b[2] & 0x3F);
        return 3;
    }
    if ((b[0] & 0xF8) == 0xF0) {
        if ((b[1] & 0xC0) != 0x80 || (b[2] & 0xC0) != 0x80 || (b[3] & 0xC0) != 0x80) { *cp = 0xFFFD; return 1; }
        *cp = ((uint32_t)(b[0] & 0x07) << 18) | ((uint32_t)(b[1] & 0x3F) << 12)
            | ((uint32_t)(b[2] & 0x3F) << 6) | (b[3] & 0x3F);
        return 4;
    }
    *cp = 0xFFFD;
    return 1;
}

// Encode codepoint cp as UTF-8 into out[].  out must have space for 4 bytes.
// Returns number of bytes written (1-4).  Invalid codepoints write U+FFFD.
static inline size_t wlx_utf8_encode(uint32_t cp, char *out) {
    if (cp < 0x80) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    if (cp <= 0x10FFFF) {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
    // Invalid codepoint -> encode U+FFFD
    out[0] = (char)0xEF; out[1] = (char)0xBF; out[2] = (char)0xBD;
    return 3;
}

// Count codepoints in a NUL-terminated UTF-8 string.
static inline size_t wlx_utf8_strlen(const char *s) {
    size_t count = 0;
    while (*s) {
        s += wlx_utf8_char_len(s);
        count++;
    }
    return count;
}

// Move byte position backward to the previous codepoint boundary.
// Returns new byte position (0 if already at start).
static inline size_t wlx_utf8_prev(const char *s, size_t byte_pos) {
    if (byte_pos == 0) return 0;
    byte_pos--;
    // Walk back over continuation bytes (10xxxxxx)
    while (byte_pos > 0 && ((unsigned char)s[byte_pos] & 0xC0) == 0x80) {
        byte_pos--;
    }
    return byte_pos;
}

// Move byte position forward to the next codepoint boundary.
// Returns new byte position (len if already at end).
static inline size_t wlx_utf8_next(const char *s, size_t byte_pos, size_t len) {
    if (byte_pos >= len) return len;
    byte_pos += wlx_utf8_char_len(s + byte_pos);
    if (byte_pos > len) byte_pos = len;
    return byte_pos;
}

#ifndef WLX_TEXT_RUN_MAX_GLYPHS
#define WLX_TEXT_RUN_MAX_GLYPHS 512
#endif
#define WLX_TEXT_RUN_MAX_LINES_ 128

static bool wlx_layout_text_run(WLX_Context *ctx, WLX_Rect rect, const char *text, WLX_Text_Style style, WLX_Align align, bool wrap,
    bool draw, float *out_end_x, float *out_end_y) {

    if (style.font_size <= 0) return false;
    if (text == NULL) text = "";
    size_t length = strlen(text);

    float ref_w = 0, glyph_h = 0;
    wlx_measure_text(ctx, " ", style, &ref_w, &glyph_h);
    if (glyph_h <= 0) glyph_h = (float)style.font_size;
    if (glyph_h > rect.h) return false;

    if (length == 0) {
        WLX_Rect tr = wlx_get_align_rect(rect, 0, glyph_h, align);
        if (out_end_x) *out_end_x = tr.x;
        if (out_end_y) *out_end_y = tr.y;
        return true;
    }

    float spacing = style.spacing;

    // --- Pass 1: measure glyphs, determine line breaks ---

    struct { size_t off; size_t len; float adv; } gs[WLX_TEXT_RUN_MAX_GLYPHS];
    struct { size_t start; size_t count; float w; } ls[WLX_TEXT_RUN_MAX_LINES_];
    size_t ng = 0, nl = 0;
    size_t line_start = 0;
    float line_w = 0;

    for (size_t i = 0; i < length;) {
        if (ng >= WLX_TEXT_RUN_MAX_GLYPHS) break;

        size_t char_len = wlx_utf8_char_len(&text[i]);
        if (i + char_len > length) char_len = 1;

        char glyph[8] = {0};
        memcpy(glyph, &text[i], char_len);
        glyph[char_len] = '\0';

        float gw = 0, gh = 0;
        wlx_measure_text(ctx, glyph, style, &gw, &gh);

        if (line_w + gw > rect.w && ng > line_start) {
            if (!wrap) break;
            if (nl >= WLX_TEXT_RUN_MAX_LINES_ - 1) break;
            ls[nl].start = line_start;
            ls[nl].count = ng - line_start;
            ls[nl].w = line_w - spacing;
            nl++;
            line_start = ng;
            line_w = 0;
        }

        gs[ng].off = i;
        gs[ng].len = char_len;
        gs[ng].adv = gw;
        ng++;
        line_w += gw + spacing;
        i += char_len;
    }

    if (ng > line_start && nl < WLX_TEXT_RUN_MAX_LINES_) {
        ls[nl].start = line_start;
        ls[nl].count = ng - line_start;
        ls[nl].w = line_w - spacing;
        nl++;
    }

    if (nl == 0) {
        if (out_end_x) *out_end_x = rect.x;
        if (out_end_y) *out_end_y = rect.y;
        return true;
    }

    // --- Pass 2: align and draw/measure per line ---

    float total_h = (float)nl * glyph_h;
    WLX_Rect vert = wlx_get_align_rect(rect, -1, total_h, align);
    float cy = vert.y;
    float end_x = rect.x, end_y = cy;

    for (size_t li = 0; li < nl; li++) {
        if (cy + glyph_h > rect.y + rect.h) break;

        WLX_Rect lr = { rect.x, cy, rect.w, glyph_h };
        WLX_Rect aligned = wlx_get_align_rect(lr, ls[li].w, glyph_h, align);
        float gx = aligned.x;

        size_t gend = ls[li].start + ls[li].count;
        for (size_t gi = ls[li].start; gi < gend; gi++) {
            if (draw) {
                char glyph[8] = {0};
                memcpy(glyph, &text[gs[gi].off], gs[gi].len);
                glyph[gs[gi].len] = '\0';
                WLX_Text_Style glyph_style = style;
                glyph_style.spacing = 0;
                wlx_draw_text(ctx, glyph, gx, cy, glyph_style);
            }
            gx += gs[gi].adv;
            if (gi + 1 < gend) gx += spacing;
        }

        end_x = gx;
        end_y = cy;
        cy += glyph_h;
    }

    if (out_end_x) *out_end_x = end_x;
    if (out_end_y) *out_end_y = end_y;
    return true;
}

WLXDEF bool wlx_draw_text_fitted(WLX_Context *ctx, WLX_Rect rect, const char *text, WLX_Text_Style style, WLX_Align align, bool wrap) {
    return wlx_layout_text_run(ctx, rect, text, style, align, wrap, true, NULL, NULL);
}

WLXDEF bool wlx_calc_cursor_position(WLX_Context *ctx, WLX_Rect rect, const char *text, WLX_Text_Style style, WLX_Align align, bool wrap,
    float *cursor_x, float *cursor_y) {
    return wlx_layout_text_run(ctx, rect, text, style, align, wrap, false, cursor_x, cursor_y);
}



// ============================================================================
// Implementation: widget rendering and widget interaction
// ============================================================================

static const float WLX_CHECKBOX_SIZE_RATIO = 0.8f;
static const float WLX_CHECKBOX_LABEL_PADDING_FACTOR = 0.5f;
static const float WLX_CHECKBOX_CHECK_PADDING_RATIO = 0.2f;
static const float WLX_CHECKBOX_CHECK_THICKNESS_RATIO = 0.12f;
static const float WLX_CHECKBOX_CHECK_GLOW_RATIO = 0.2f;
static const float WLX_CHECKBOX_CHECK_GLOW_ALPHA = 0.25f;

static const float WLX_INPUTBOX_CURSOR_WIDTH = 2.0f;
static const float WLX_INPUTBOX_CURSOR_PADDING = 2.0f;
static const float WLX_INPUTBOX_CURSOR_BLINK_PERIOD = 1.0f;
static const float WLX_INPUTBOX_CURSOR_VISIBLE_FRACTION = 0.5f;

// Resolve widget opacity: if the per-widget value is unset (< 0), default to
// fully opaque; then multiply by theme-level and context-stack opacity.
static inline float wlx_resolve_opacity(float opt_opacity, float theme_opacity, float ctx_opacity) {
    if (opt_opacity < 0.0f) opt_opacity = 1.0f;
    float t = (theme_opacity < 0.0f) ? 1.0f : theme_opacity;
    return opt_opacity * t * ctx_opacity;
}

// Convenience wrapper: reads theme->opacity and context opacity stack automatically.
static inline float wlx_resolve_opacity_for(const WLX_Context *ctx, float opt_opacity) {
    return wlx_resolve_opacity(opt_opacity, ctx->theme->opacity, wlx_get_opacity(ctx));
}

// Apply a resolved opacity value to a variadic list of WLX_Color pointers.
#define WLX_APPLY_OPACITY(opacity, /*WLX_Color* args*/ ...) do { \
    WLX_Color *_cs[] = { __VA_ARGS__ };                          \
    for (size_t _i = 0; _i < sizeof(_cs)/sizeof(_cs[0]); ++_i)  \
        *_cs[_i] = wlx_color_apply_opacity(*_cs[_i], (opacity)); \
} while (0)

static inline void wlx_resolve_roundness(const WLX_Theme *theme,
                                          float *roundness,
                                          int *segments)
{
    if (wlx_is_negative_unset(*roundness)) *roundness = theme->roundness;
    if (*segments < 0)                     *segments  = theme->rounded_segments;
}

// Resolve common text/font fields against theme defaults.
// min_height is set to font_size if still unset after font_size resolution.
static inline void wlx_resolve_typography(const WLX_Theme *theme,
    WLX_Font *font, int *font_size, int *spacing, float *min_height)
{
    if (*font      == WLX_FONT_DEFAULT) *font      = theme->font;
    if (*font_size <= 0)                *font_size = theme->font_size;
    if (*spacing   <= 0)                *spacing   = theme->spacing;
    if (*min_height <= 0)               *min_height = (float)*font_size;
}

// Resolve common border/roundness fields against theme defaults.
// Wraps wlx_resolve_roundness so callers need only one call for all four.
static inline void wlx_resolve_border(const WLX_Theme *theme,
    WLX_Color *border_color, float *border_width,
    float *roundness, int *rounded_segments)
{
    if (wlx_color_is_zero(*border_color))       *border_color = theme->border;
    if (wlx_is_negative_unset(*border_width))  *border_width = theme->border_width;
    wlx_resolve_roundness(theme, roundness, rounded_segments);
}

static void wlx_resolve_opt_widget(const WLX_Context *ctx, WLX_Widget_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);
    opt->opacity = wlx_resolve_opacity_for(ctx, opt->opacity);
    WLX_APPLY_OPACITY(opt->opacity, &opt->color, &opt->border_color);
}

static void wlx_resolve_opt_label(const WLX_Context *ctx, WLX_Label_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->surface;
    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->spacing, &opt->min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);
    opt->opacity = wlx_resolve_opacity_for(ctx, opt->opacity);
    WLX_APPLY_OPACITY(opt->opacity, &opt->front_color, &opt->back_color, &opt->border_color);
}

WLXDEF void wlx_label_impl(WLX_Context *ctx, const char *text, WLX_Label_Opt opt, const char *file, int line)
{
    wlx_resolve_opt_label(ctx, &opt);

    // Prologue: compute widget frame and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;

    WLX_Interaction inter = wlx_get_interaction(
        ctx,
        wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        file, line
    );

    if (opt.show_background || opt.border_width > 0) {
        WLX_Color bg = opt.show_background
            ? ((inter.hover) ? wlx_color_brightness(opt.back_color, ctx->theme->hover_brightness) : opt.back_color)
            : (WLX_Color){0};
        wlx_draw_box(ctx, (WLX_Rect){wr.x, wr.y, wr.w, wr.h}, (WLX_Box_Style){
            .fill            = bg,
            .border          = opt.border_color,
            .border_width    = opt.border_width,
            .roundness       = opt.roundness,
            .rounded_segments = opt.rounded_segments,
        });
    }

    wlx_draw_text_fitted(ctx, wr, text, (WLX_Text_Style){ .font = opt.font, .font_size = opt.font_size, .spacing = opt.spacing, .color = opt.front_color }, opt.align, opt.wrap);

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);
}

static void wlx_resolve_opt_button(const WLX_Context *ctx, WLX_Button_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->surface;
    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->spacing, &opt->min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);
    opt->opacity = wlx_resolve_opacity_for(ctx, opt->opacity);
    WLX_APPLY_OPACITY(opt->opacity, &opt->front_color, &opt->back_color, &opt->border_color);
}

WLXDEF bool wlx_button_impl(WLX_Context *ctx, const char *text, WLX_Button_Opt opt, const char *file, int line)
{
    wlx_resolve_opt_button(ctx, &opt);

    // Prologue: compute widget frame and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;

    WLX_Interaction inter = wlx_get_interaction(
        ctx,
        wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        file, line
    );


    WLX_Color bg = (inter.hover) ? wlx_color_brightness(opt.back_color, ctx->theme->hover_brightness) : opt.back_color;

    wlx_draw_box(ctx, (WLX_Rect){wr.x, wr.y, wr.w, wr.h}, (WLX_Box_Style){
        .fill            = bg,
        .border          = opt.border_color,
        .border_width    = opt.border_width,
        .roundness       = opt.roundness,
        .rounded_segments = opt.rounded_segments,
    });

    wlx_draw_text_fitted(ctx, wr, text, (WLX_Text_Style){ .font = opt.font, .font_size = opt.font_size, .spacing = opt.spacing, .color = opt.front_color }, opt.align, opt.wrap);

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);

    return inter.clicked;
}

static void wlx_resolve_opt_checkbox(const WLX_Context *ctx, WLX_Checkbox_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->surface;
    if (wlx_color_is_zero(opt->check_color)) opt->check_color = theme->checkbox.check;
    // Widget-specific border fallbacks before common helper
    if (wlx_color_is_zero(opt->border_color))         opt->border_color = theme->checkbox.border;
    if (wlx_is_negative_unset(opt->border_width)) opt->border_width = theme->checkbox.border_width;
    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->spacing, &opt->min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);
    opt->opacity = wlx_resolve_opacity_for(ctx, opt->opacity);
    WLX_APPLY_OPACITY(opt->opacity, &opt->front_color, &opt->back_color, &opt->border_color, &opt->check_color);
}

WLXDEF bool wlx_checkbox_impl(WLX_Context *ctx, const char *text, bool *checked, WLX_Checkbox_Opt opt, const char *file, int line)
{
    wlx_resolve_opt_checkbox(ctx, &opt);

    // Prologue: compute widget frame and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;

    // Draw checkbox
    float checkbox_size = (wr.h > opt.font_size) ? opt.font_size : wr.h * WLX_CHECKBOX_SIZE_RATIO;
    float text_w = 0;
    float text_h = 0;
    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size, .spacing = opt.spacing, .color = opt.front_color };
    wlx_measure_text(ctx, text, ts, &text_w, &text_h);
    float padding = opt.font_size * WLX_CHECKBOX_LABEL_PADDING_FACTOR;

    float height = text_h > checkbox_size ? text_h : checkbox_size;
    WLX_Rect acr = wlx_get_align_rect(wr, checkbox_size + padding + text_w, height, opt.align);
    WLX_Rect hit_rect = (opt.full_slot_hit) ? wr : acr;

    WLX_Interaction inter = wlx_get_interaction(
        ctx,
        hit_rect,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        file, line
    );

    WLX_Rect checkbox_rect = { acr.x, acr.y, checkbox_size, checkbox_size };

    if (inter.clicked && checked != NULL) {
        *checked = !(*checked);
    }

    WLX_Color checkbox_bg = inter.hover ? wlx_color_brightness(opt.back_color, ctx->theme->hover_brightness) : opt.back_color;

    if (opt.tex_checked.width > 0) {
        // Texture mode - draw checked/unchecked texture
        WLX_Texture current_tex = (checked != NULL && *checked) ? opt.tex_checked : opt.tex_unchecked;
        wlx_draw_texture(
            ctx,
            current_tex,
            (WLX_Rect){0, 0, (float)current_tex.width, (float)current_tex.height},
            (WLX_Rect){checkbox_rect.x, checkbox_rect.y, checkbox_rect.w, checkbox_rect.h},
            checkbox_bg
        );
    } else {
        // Native mode - draw box + checkmark lines
        wlx_draw_box(ctx, checkbox_rect, (WLX_Box_Style){
            .fill            = checkbox_bg,
            .border          = opt.border_color,
            .border_width    = opt.border_width,
            .roundness       = opt.roundness,
            .rounded_segments = opt.rounded_segments,
        });

        if (checked != NULL && *checked) {
            float check_padding = checkbox_size * WLX_CHECKBOX_CHECK_PADDING_RATIO;
            float thick = checkbox_size * WLX_CHECKBOX_CHECK_THICKNESS_RATIO;
            if (thick < 1.0f) thick = 1.0f;
            float glow_thick = checkbox_size * WLX_CHECKBOX_CHECK_GLOW_RATIO;
            if (glow_thick < thick + 1.0f) glow_thick = thick + 1.0f;
            float x0 = checkbox_rect.x + check_padding;
            float y0 = checkbox_rect.y + checkbox_size / 2;
            float x1 = checkbox_rect.x + checkbox_size / 2;
            float y1 = checkbox_rect.y + checkbox_size - check_padding;
            float x2 = checkbox_rect.x + checkbox_size - check_padding;
            float y2 = checkbox_rect.y + check_padding;
            WLX_Color glow = opt.check_color;
            glow.a = (unsigned char)(glow.a * WLX_CHECKBOX_CHECK_GLOW_ALPHA);
            wlx_draw_line(ctx, x0, y0, x1, y1, glow_thick, glow);
            wlx_draw_line(ctx, x1, y1, x2, y2, glow_thick, glow);
            wlx_draw_line(ctx, x0, y0, x1, y1, thick, opt.check_color);
            wlx_draw_line(ctx, x1, y1, x2, y2, thick, opt.check_color);
        }
    }

    WLX_Rect text_rect = {
      .x = acr.x + checkbox_size + padding,
      .y = acr.y,
      .w = acr.w - checkbox_size + padding,
      .h = wr.y + wr.h - acr.y
    };
    wlx_draw_text_fitted(ctx, text_rect, text, ts, WLX_ALIGN_NONE, opt.wrap);

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);

    return inter.clicked;
}

static void wlx_resolve_opt_inputbox(const WLX_Context *ctx, WLX_Inputbox_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    if (wlx_color_is_zero(opt->front_color))        opt->front_color        = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))         opt->back_color         = theme->surface;
    // Widget-specific border_width fallback before common helper
    if (wlx_is_negative_unset(opt->border_width))   opt->border_width       = theme->input.border_width;
    if (wlx_color_is_zero(opt->border_focus_color)) opt->border_focus_color = theme->input.border_focus;
    if (wlx_color_is_zero(opt->cursor_color))       opt->cursor_color       = theme->input.cursor;
    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->spacing, &opt->min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);
    opt->opacity = wlx_resolve_opacity_for(ctx, opt->opacity);
    WLX_APPLY_OPACITY(opt->opacity, &opt->front_color, &opt->back_color, &opt->border_color,
                      &opt->border_focus_color, &opt->cursor_color);
}

// Handle keyboard input for an active inputbox: cursor movement, text insertion,
// and backspace deletion. Called each frame while the inputbox is focused.
static void wlx_inputbox_handle_keys(WLX_Context *ctx, WLX_Inputbox_State *state,
                                      char *buffer, size_t buffer_size, bool just_focused)
{
    size_t current_len = strlen(buffer);

    // Initialize cursor position when first focused
    if (just_focused) {
        state->cursor_pos = current_len;
        state->cursor_blink_time = 0.0f;
    }

    // Clamp cursor position to valid range
    if (state->cursor_pos > current_len) {
        state->cursor_pos = current_len;
    }

    // Handle left arrow - move cursor left (by one codepoint)
    if (wlx_is_key_pressed(ctx, WLX_KEY_LEFT) && state->cursor_pos > 0) {
        state->cursor_pos = wlx_utf8_prev(buffer, state->cursor_pos);
    }

    // Handle right arrow - move cursor right (by one codepoint)
    if (wlx_is_key_pressed(ctx, WLX_KEY_RIGHT) && state->cursor_pos < current_len) {
        state->cursor_pos = wlx_utf8_next(buffer, state->cursor_pos, current_len);
    }

    // Add typed characters (whole codepoints) at cursor position
    {
        size_t i = 0;
        while (i < sizeof(ctx->input.text_input) && ctx->input.text_input[i] != '\0') {
            size_t char_len = wlx_utf8_char_len(&ctx->input.text_input[i]);
            // Don't read past the text_input buffer
            if (i + char_len > sizeof(ctx->input.text_input)) break;
            // Check buffer has room for the whole codepoint
            if (current_len + char_len >= buffer_size) break;

            // Shift buffer right by char_len bytes to make space
            memmove(&buffer[state->cursor_pos + char_len],
                    &buffer[state->cursor_pos],
                    current_len - state->cursor_pos + 1); // +1 for NUL

            // Copy the codepoint bytes
            memcpy(&buffer[state->cursor_pos], &ctx->input.text_input[i], char_len);
            state->cursor_pos += char_len;
            current_len += char_len;

            i += char_len;
        }
    }

    // Handle backspace - delete the codepoint before cursor
    if (wlx_is_key_pressed(ctx, WLX_KEY_BACKSPACE) && state->cursor_pos > 0) {
        size_t prev_pos = wlx_utf8_prev(buffer, state->cursor_pos);
        // Shift remaining bytes (including NUL) left
        memmove(&buffer[prev_pos],
                &buffer[state->cursor_pos],
                current_len - state->cursor_pos + 1);
        state->cursor_pos = prev_pos;
    }
}

WLXDEF bool wlx_inputbox_impl(WLX_Context *ctx, const char *label, char *buffer, size_t buffer_size, WLX_Inputbox_Opt opt, const char *file, int line) {
    assert(ctx != NULL);
    assert(buffer != NULL && "inputbox buffer must not be NULL");
    assert(buffer_size >= 2 && "buffer_size must hold at least 1 char + null terminator");
    wlx_resolve_opt_inputbox(ctx, &opt);

    // Ensure height can fit the font plus content_padding on both sides.
    float min_h = (float)(opt.font_size + opt.content_padding * 2 + 4);
    if (opt.height > 0 && opt.height < min_h) opt.height = min_h;

    // Prologue: compute widget frame and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;

    // If the layout slot constrained wr.h below the requested height,
    // shrink content_padding so the text area can still fit the font.
    {
        float available_text_h = wr.h - opt.content_padding * 2;
        if (available_text_h < (float)opt.font_size) {
            float needed = (wr.h - (float)opt.font_size) / 2.0f;
            if (needed < 0) needed = 0;
            opt.content_padding = needed;
        }
    }

    WLX_Interaction inter = wlx_get_interaction(
        ctx,
        wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS,
        file, line
    );

    WLX_State persistant = wlx_get_state_impl(ctx, sizeof(WLX_Inputbox_State), file, line);
    // Per-widget persistent cursor state
    WLX_Inputbox_State *state = (WLX_Inputbox_State *)persistant.data;

    if (inter.focused) {
        wlx_inputbox_handle_keys(ctx, state, buffer, buffer_size, inter.just_focused);
    }

    float label_width = 0;
    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size, .spacing = opt.spacing, .color = opt.front_color };
    // Draw label if provided
    if (label != NULL && opt.font_size > 0) {
        float label_h = 0;
        wlx_measure_text(ctx, label, ts, &label_width, &label_h);
        label_width += opt.content_padding;

        float label_x = wr.x + opt.content_padding;
        float label_y;
        switch (opt.align) {
            case WLX_TOP: case WLX_TOP_LEFT: case WLX_TOP_CENTER: case WLX_TOP_RIGHT:
                label_y = wr.y + opt.content_padding;
                break;
            case WLX_BOTTOM: case WLX_BOTTOM_LEFT: case WLX_BOTTOM_CENTER: case WLX_BOTTOM_RIGHT:
                label_y = wr.y + wr.h - label_h - opt.content_padding;
                break;
            default:
                label_y = wr.y + (wr.h - label_h) / 2;
                break;
        }

        WLX_Rect label_rect = {
            .x = label_x,
            .y = label_y,
            .w = label_width,
            .h = label_h,
        };
        wlx_draw_text_fitted(ctx, label_rect, label, ts, opt.align, opt.wrap);

    }

    // Input box rectangle - guarantee a minimum width so the text fieldinput
    // doesn't vanish when the label consumes most of the widget width.
    float min_input_w = (float)(opt.font_size * 3);
    if (label_width > 0 && (wr.w - label_width - opt.content_padding * 2) < min_input_w) {
        label_width = wr.w - min_input_w - opt.content_padding * 2;
        if (label_width < 0) label_width = 0;
    }

    float input_x = wr.x + label_width + opt.content_padding;
    float input_w = wr.w - label_width - opt.content_padding * 2;
    input_w = input_w < 0 ? 0 : input_w;

    float input_h = wr.h - opt.content_padding * 2;

    WLX_Rect input_rect = { input_x, wr.y + opt.content_padding, input_w, input_h };

    // Draw input box background and border
    WLX_Color bg_color = opt.back_color;
    if (inter.hover && !inter.focused) {
        bg_color = wlx_color_brightness(opt.back_color, ctx->theme->hover_brightness * 0.5f);
    }
    WLX_Color bdr_color = inter.focused ? opt.border_focus_color : opt.border_color;
    wlx_draw_box(ctx, input_rect, (WLX_Box_Style){
        .fill            = bg_color,
        .border          = bdr_color,
        .border_width    = opt.border_width,
        .roundness       = opt.roundness,
        .rounded_segments = opt.rounded_segments,
    });

    // Draw text content
    if (opt.font_size > 0 && buffer != NULL) {
        float border_inset = opt.border_width > 0 ? opt.border_width + 1.0f : 0.0f;
        WLX_Rect text_rect = {
            .x = input_rect.x + opt.content_padding / 2,
            .y = input_rect.y + border_inset,
            .w = input_rect.w - opt.content_padding / 2 - WLX_INPUTBOX_CURSOR_WIDTH - WLX_INPUTBOX_CURSOR_PADDING,
            .h = input_rect.h - border_inset * 2
        };

        float cursor_x = text_rect.x;
        float cursor_y = text_rect.y;

        // Render text up to cursor position to get cursor coordinates
        if (inter.focused) {
            char temp[WLX_INPUTBOX_CURSOR_TEMP_SIZE];
            size_t copy_len = state->cursor_pos < sizeof(temp) - 1 ? state->cursor_pos : sizeof(temp) - 1;
            strncpy(temp, buffer, copy_len);
            temp[copy_len] = '\0';
            wlx_calc_cursor_position(ctx, text_rect, temp, ts, WLX_TOP_LEFT, opt.wrap, &cursor_x, &cursor_y);
        }

        // Render full text
        wlx_draw_text_fitted(ctx, text_rect, buffer, ts, WLX_TOP_LEFT, opt.wrap);

        if (cursor_x > text_rect.x)
            cursor_x += WLX_INPUTBOX_CURSOR_PADDING;

        float cursor_height = opt.font_size;
        if (inter.focused) {
            state->cursor_blink_time += wlx_get_frame_time(ctx);
        }

        // Draw cursor if focused and fits in text rect
        if (inter.focused && cursor_x < (text_rect.x + text_rect.w) && (cursor_y + cursor_height) <= (text_rect.y + text_rect.h + 1.0f)) {
            if (fmodf(state->cursor_blink_time, WLX_INPUTBOX_CURSOR_BLINK_PERIOD) < (WLX_INPUTBOX_CURSOR_BLINK_PERIOD * WLX_INPUTBOX_CURSOR_VISIBLE_FRACTION)) {

                wlx_draw_line(ctx, cursor_x, cursor_y, cursor_x, cursor_y + cursor_height, WLX_INPUTBOX_CURSOR_WIDTH, opt.cursor_color);
            }
        }
    }

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);
    return inter.focused;
}

static void wlx_resolve_opt_slider(const WLX_Context *ctx, WLX_Slider_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    if (wlx_color_is_zero(opt->track_color))  opt->track_color            = theme->slider.track;
    if (wlx_color_is_zero(opt->thumb_color))  opt->thumb_color            = theme->slider.thumb;
    if (wlx_color_is_zero(opt->label_color))  opt->label_color            = theme->slider.label;
    if (wlx_color_is_zero(opt->border_color))           opt->border_color           = theme->border;
    if (wlx_is_negative_unset(opt->border_width))    opt->border_width           = theme->border_width;
    if (opt->font == WLX_FONT_DEFAULT)               opt->font                   = theme->font;
    if (opt->font_size              <= 0)             opt->font_size              = theme->font_size;
    if (opt->spacing                <= 0)             opt->spacing                = theme->spacing;
    if (opt->track_height           <= 0)             opt->track_height           = theme->slider.track_height;
    if (opt->thumb_width            <= 0)             opt->thumb_width            = theme->slider.thumb_width;
    if (opt->min_height             <= 0)             opt->min_height             = (float)opt->font_size;
    if (wlx_is_negative_unset(opt->roundness))       opt->roundness              = theme->roundness;
    if (opt->rounded_segments       < 0)      opt->rounded_segments       = theme->rounded_segments;
    if (wlx_is_float_unset(opt->hover_brightness))       opt->hover_brightness       = theme->hover_brightness;
    if (wlx_is_float_unset(opt->thumb_hover_brightness)) opt->thumb_hover_brightness = opt->hover_brightness * 0.5f;
    opt->opacity = wlx_resolve_opacity_for(ctx, opt->opacity);
    WLX_APPLY_OPACITY(opt->opacity, &opt->track_color, &opt->thumb_color, &opt->label_color, &opt->border_color);
}

WLXDEF bool wlx_slider_impl(WLX_Context *ctx, const char *label, float *value, WLX_Slider_Opt opt, const char *file, int line) {
    assert(ctx != NULL);
    assert(value != NULL && "slider value pointer must not be NULL");
    wlx_resolve_opt_slider(ctx, &opt);

    // Prologue: compute widget frame and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;

    bool changed = false;

    // Layout: [label] [track with thumb] [value text]
    float label_width = 0;
    float value_text_width = 0;
    float padding = opt.font_size / 2.0f;
    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size, .spacing = opt.spacing, .color = opt.label_color };

    if (label != NULL && opt.font_size > 0) {
        float label_h = 0;
        wlx_measure_text(ctx, label, ts, &label_width, &label_h);
        label_width += padding;
    }

    char value_str[32] = {0};
    if (opt.show_label) {
        // Fixed-width value area: measure worst-case string so geometry
        // never depends on the current value (prevents CONTENT oscillation)
        float abs_bound = fabsf(opt.min_value);
        float abs_bound2 = fabsf(opt.max_value);
        if (abs_bound2 > abs_bound) abs_bound = abs_bound2;
        char worst_case[32];
        snprintf(worst_case, sizeof(worst_case), "%.2f", -abs_bound);
        float tmp_h = 0;
        wlx_measure_text(ctx, worst_case, ts, &value_text_width, &tmp_h);
        value_text_width += padding;

        // Format actual value for display
        snprintf(value_str, sizeof(value_str), "%.2f", *value);
    }

    // Track area
    float track_x = wr.x + label_width;
    float track_w = wr.w - label_width - value_text_width;
    if (track_w < opt.thumb_width) track_w = opt.thumb_width;
    float track_y = wr.y + (wr.h - opt.track_height) / 2.0f;

    // Usable range for the thumb center (half thumb on each side)
    float half_thumb = opt.thumb_width / 2.0f;
    float usable_x = track_x + half_thumb;
    float usable_w = track_w - opt.thumb_width;
    if (usable_w < 1) usable_w = 1;

    // Clamp value
    if (*value < opt.min_value) *value = opt.min_value;
    if (*value > opt.max_value) *value = opt.max_value;

    float range = opt.max_value - opt.min_value;
    if (range <= 0) range = 1;
    float t = (*value - opt.min_value) / range;

    float thumb_cx = roundf(usable_x + t * usable_w);
    float thumb_h = wr.h * 0.7f;
    float thumb_y = roundf(wr.y + (wr.h - thumb_h) / 2.0f);

    // Interaction via unified handler (drag mode for continuous value updates)
    WLX_Rect hit_rect = { track_x, wr.y, track_w, wr.h };

    WLX_Interaction inter = wlx_get_interaction(
        ctx,
        hit_rect,
        WLX_INTERACT_HOVER | WLX_INTERACT_DRAG,
        file, line
    );

    // While dragging, update value from mouse position
    if (inter.active) {
        float mx = (float)ctx->input.mouse_x;
        float new_t = (mx - usable_x) / usable_w;
        if (new_t < 0) new_t = 0;
        if (new_t > 1) new_t = 1;
        float new_value = opt.min_value + new_t * range;
        if (new_value != *value) {
            *value = new_value;
            changed = true;
        }
        // Recalculate thumb position
        t = (*value - opt.min_value) / range;
        thumb_cx = roundf(usable_x + t * usable_w);
    }

    bool is_active = inter.active;
    bool is_hover = inter.hover;

    // Draw label
    if (label != NULL && opt.font_size > 0) {
        WLX_Rect label_rect = { wr.x, wr.y, label_width, wr.h };
        wlx_draw_text_fitted(ctx, label_rect, label, ts, WLX_LEFT, false);
    }

    // Draw track
    WLX_Color color_track = is_hover || is_active ? wlx_color_brightness(opt.track_color, opt.hover_brightness) : opt.track_color;
    WLX_Rect track_rect = { track_x, track_y, track_w, opt.track_height };
    wlx_draw_rect_rounded(ctx, track_rect, opt.roundness, opt.rounded_segments, color_track);

    // Draw track border
    if (opt.border_width > 0) {
        wlx_draw_rect_lines(ctx, track_rect, opt.border_width, opt.border_color);
    }

    // Draw filled portion of track (extends under the thumb so the
    // rounded right corner is hidden; the thumb is taller than the track)
    float fill_w = thumb_cx - track_x;
    if (fill_w > 0) {
        WLX_Color fill_color = is_active ? opt.thumb_color : wlx_color_brightness(opt.thumb_color, opt.fill_inactive_brightness);
        WLX_Rect fill_rect = { track_x, track_y, fill_w, opt.track_height };
        wlx_draw_rect_rounded(ctx, fill_rect, opt.roundness, opt.rounded_segments, fill_color);
    }

    // Draw thumb
    WLX_Color color_thumb = opt.thumb_color;
    if (is_active) {
        color_thumb = wlx_color_brightness(color_thumb, opt.hover_brightness);
    } else if (is_hover) {
        color_thumb = wlx_color_brightness(color_thumb, opt.thumb_hover_brightness);
    }
    WLX_Rect thumb_rect = { thumb_cx - half_thumb, thumb_y, opt.thumb_width, thumb_h };
    wlx_draw_rect_rounded(ctx, thumb_rect, opt.roundness, opt.rounded_segments, color_thumb);

    // Draw value text (fixed-width area, clipped per-glyph)
    if (opt.show_label && opt.font_size > 0) {
        // Recalculate value string in case it changed during drag
        snprintf(value_str, sizeof(value_str), "%.2f", *value);
        float vt_x = track_x + track_w;
        WLX_Rect value_rect = { vt_x, wr.y, value_text_width + padding, wr.h };
        wlx_draw_text_fitted(ctx, value_rect, value_str, ts, WLX_LEFT, false);
    }

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);
    return changed;
}

WLXDEF void wlx_separator_impl(WLX_Context *ctx, WLX_Separator_Opt opt, const char *file, int line)
{
    // Prologue: compute widget frame (no interaction for separator)
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;

    WLX_Color color = wlx_color_is_zero(opt.color) ? ctx->theme->border : opt.color;
    float t = opt.thickness;

    float cx = wr.x + wr.w * 0.5f;
    float cy = wr.y + wr.h * 0.5f;

    if (wr.w >= wr.h) {
        wlx_draw_line(ctx, wr.x, cy, wr.x + wr.w, cy, t, color);
    } else {
        wlx_draw_line(ctx, cx, wr.y, cx, wr.y + wr.h, t, color);
    }

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);
}

static void wlx_resolve_opt_progress(const WLX_Context *ctx, WLX_Progress_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    if (wlx_color_is_zero(opt->track_color))  opt->track_color  = wlx_color_or(theme->progress.track, theme->slider.track);
    if (wlx_color_is_zero(opt->fill_color))   opt->fill_color   = wlx_color_or(theme->progress.fill,  theme->accent);
    if (wlx_color_is_zero(opt->border_color)) opt->border_color = theme->border;
    if (wlx_is_negative_unset(opt->border_width))      opt->border_width = theme->border_width;
    if (opt->track_height <= 0)               opt->track_height = theme->progress.track_height > 0 ? theme->progress.track_height : theme->slider.track_height;
    if (opt->min_height <= 0)                 opt->min_height   = opt->track_height;
    wlx_resolve_roundness(theme, &opt->roundness, &opt->rounded_segments);
    opt->opacity = wlx_resolve_opacity_for(ctx, opt->opacity);
    WLX_APPLY_OPACITY(opt->opacity, &opt->track_color, &opt->fill_color, &opt->border_color);
}

WLXDEF void wlx_progress_impl(WLX_Context *ctx, float value, WLX_Progress_Opt opt, const char *file, int line) {
    assert(ctx != NULL);
    
    wlx_resolve_opt_progress(ctx, &opt);

    // Prologue: compute widget frame (no interaction for progress bar)
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;

    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;

    float th = opt.track_height > 0 ? opt.track_height : wr.h;
    WLX_Rect track = { wr.x, wr.y + (wr.h - th) / 2.0f, wr.w, th };

    wlx_draw_box(ctx, track, (WLX_Box_Style){
        .fill            = opt.track_color,
        .border          = opt.border_color,
        .border_width    = opt.border_width,
        .roundness       = opt.roundness,
        .rounded_segments = opt.rounded_segments,
    });

    float fill_w = roundf(track.w * value);
    if (fill_w > 0) {
        WLX_Rect fill = { track.x, track.y, fill_w, track.h };
        wlx_draw_box(ctx, fill, (WLX_Box_Style){
            .fill            = opt.fill_color,
            .roundness       = opt.roundness,
            .rounded_segments = opt.rounded_segments,
        });
    }

    wlx_widget_frame_end(ctx, frame);
}

static void wlx_resolve_opt_toggle(const WLX_Context *ctx, WLX_Toggle_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    opt->track_color        = wlx_color_or(opt->track_color,        wlx_color_or(theme->toggle.track, theme->slider.track));
    opt->track_active_color = wlx_color_or(opt->track_active_color, wlx_color_or(theme->toggle.track_active, theme->accent));
    opt->thumb_color        = wlx_color_or(opt->thumb_color,        wlx_color_or(theme->toggle.thumb, theme->foreground));
    if (wlx_color_is_zero(opt->front_color))       opt->front_color      = theme->foreground;
    if (wlx_is_float_unset(opt->hover_brightness))  opt->hover_brightness = theme->hover_brightness;
    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->spacing, &opt->min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);
    opt->opacity = wlx_resolve_opacity_for(ctx, opt->opacity);
    WLX_APPLY_OPACITY(opt->opacity, &opt->track_color, &opt->track_active_color, &opt->thumb_color,
                      &opt->front_color, &opt->border_color);
}

WLXDEF bool wlx_toggle_impl(WLX_Context *ctx, const char *label, bool *value, WLX_Toggle_Opt opt, const char *file, int line) {
    assert(ctx != NULL);
    assert(value != NULL && "toggle value pointer must not be NULL");
    
    wlx_resolve_opt_toggle(ctx, &opt);

    // Prologue: determine geometry of toggle components (track, thumb, label) and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;

    float track_h = opt.font_size;
    float thr = ctx->theme->toggle.track_to_height_ratio > 0.0f
                    ? ctx->theme->toggle.track_to_height_ratio : 2.0f;
    float track_w = track_h * thr;
    float padding = opt.font_size * 0.5f;

    float label_w = 0, label_h = 0;
    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size,
                           .spacing = opt.spacing, .color = opt.front_color };
    if (label != NULL && opt.font_size > 0) {
        wlx_measure_text(ctx, label, ts, &label_w, &label_h);
    }

    float content_w = track_w + (label_w > 0.0f ? padding + label_w : 0.0f);
    float content_h = track_h > label_h ? track_h : label_h;
    WLX_Rect acr = wlx_get_align_rect(wr, content_w, content_h, opt.align);

    float track_x = acr.x;
    float track_y = acr.y + (acr.h - track_h) / 2.0f;
    WLX_Rect track_rect = { track_x, track_y, track_w, track_h };

    int segs = opt.rounded_segments;
    if (segs < ctx->theme->min_rounded_segments) segs = ctx->theme->min_rounded_segments;

    WLX_Interaction inter = wlx_get_interaction(
        ctx, wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        file, line
    );

    if (inter.clicked) {
        *value = !(*value);
    }

    bool on = *value;

    WLX_Color color_track = on ? opt.track_active_color : opt.track_color;
    if (inter.hover) color_track = wlx_color_brightness(color_track, opt.hover_brightness);
    wlx_draw_rect_rounded(ctx, track_rect, 1.0f, segs, color_track);

    if (opt.border_width > 0) {
        wlx_draw_rect_rounded_lines(ctx, track_rect, 1.0f,
            segs, opt.border_width, opt.border_color);
    }

    float tir = ctx->theme->toggle.thumb_inset_ratio > 0.0f
                    ? ctx->theme->toggle.thumb_inset_ratio : 0.15f;
    float thumb_inset = track_h * tir;
    float thumb_size = track_h - thumb_inset * 2.0f;
    float thumb_x = on
        ? track_x + track_w - thumb_inset - thumb_size
        : track_x + thumb_inset;
    float thumb_y = track_y + thumb_inset;
    WLX_Rect thumb_rect = { thumb_x, thumb_y, thumb_size, thumb_size };

    WLX_Color color_thumb = opt.thumb_color;
    if (inter.hover) color_thumb = wlx_color_brightness(color_thumb, opt.hover_brightness * 0.5f);
    wlx_draw_rect_rounded(ctx, thumb_rect, 1.0f, segs, color_thumb);

    if (label != NULL && label_w > 0) {
        WLX_Rect text_rect = {
            acr.x + track_w + padding, acr.y,
            acr.w - track_w - padding, acr.h
        };
        wlx_draw_text_fitted(ctx, text_rect, label, ts, WLX_ALIGN_NONE, false);
    }

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);

    return inter.clicked;
}

static void wlx_resolve_opt_radio(const WLX_Context *ctx, WLX_Radio_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    opt->ring_color  = wlx_color_or(opt->ring_color,  wlx_color_or(theme->radio.ring, theme->border));
    opt->fill_color  = wlx_color_or(opt->fill_color,  wlx_color_or(theme->radio.fill, theme->accent));
    if (wlx_color_is_zero(opt->front_color))  opt->front_color       = theme->foreground;
    // Widget-specific ring border fallbacks (ring uses hardcoded roundness; no wlx_resolve_border)
    if (wlx_is_negative_unset(opt->ring_border_width)) opt->ring_border_width = theme->radio.border_width;
    if (wlx_is_negative_unset(opt->ring_border_width)) opt->ring_border_width = theme->border_width;
    if (wlx_is_float_unset(opt->hover_brightness))
                                              opt->hover_brightness  = theme->hover_brightness;
    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->spacing, &opt->min_height);
    opt->opacity = wlx_resolve_opacity_for(ctx, opt->opacity);
    WLX_APPLY_OPACITY(opt->opacity, &opt->ring_color, &opt->fill_color, &opt->front_color);
}

WLXDEF bool wlx_radio_impl(WLX_Context *ctx, const char *label, int *active, int index, WLX_Radio_Opt opt, const char *file, int line) {
    assert(ctx != NULL);
    assert(active != NULL && "radio active pointer must not be NULL");
    
    wlx_resolve_opt_radio(ctx, &opt);
    
    // Prologue: determine geometry of radio components (ring, fill, label) and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;

    bool selected = (*active == index);

    float circle_size = (float)opt.font_size;
    float padding = opt.font_size * 0.5f;

    float label_w = 0, label_h = 0;
    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size,
                           .spacing = opt.spacing, .color = opt.front_color };
    if (label != NULL && opt.font_size > 0) {
        wlx_measure_text(ctx, label, ts, &label_w, &label_h);
    }

    WLX_Interaction inter = wlx_get_interaction(
        ctx, wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        file, line
    );

    if (inter.clicked) {
        *active = index;
    }

    float content_w = circle_size + (label_w > 0.0f ? padding + label_w : 0.0f);
    float content_h = circle_size > label_h ? circle_size : label_h;
    WLX_Rect acr = wlx_get_align_rect(wr, content_w, content_h, opt.align);

    float circle_x = acr.x;
    float circle_y = acr.y + (acr.h - circle_size) / 2.0f;
    WLX_Rect ring_rect = { circle_x, circle_y, circle_size, circle_size };

    int segs = ctx->theme->rounded_segments;
    if (segs < ctx->theme->min_rounded_segments) segs = ctx->theme->min_rounded_segments;

    WLX_Color color_ring = opt.ring_color;
    if (inter.hover) color_ring = wlx_color_brightness(color_ring, opt.hover_brightness);

    if (opt.ring_border_width > 0) {
        wlx_draw_rect_rounded_lines(ctx, ring_rect, 1.0f,
            segs, opt.ring_border_width, color_ring);
    }

    if (selected) {
        float sir = ctx->theme->radio.selected_inset_ratio > 0.0f
                        ? ctx->theme->radio.selected_inset_ratio : 0.25f;
        float inset = circle_size * sir;
        WLX_Rect fill_rect = {
            circle_x + inset, circle_y + inset,
            circle_size - inset * 2.0f, circle_size - inset * 2.0f
        };
        wlx_draw_rect_rounded(ctx, fill_rect, 1.0f,
            segs, opt.fill_color);
    }

    if (label != NULL && label_w > 0) {
        WLX_Rect text_rect = {
            acr.x + circle_size + padding, acr.y,
            acr.w - circle_size - padding, acr.h
        };
        wlx_draw_text_fitted(ctx, text_rect, label, ts, WLX_ALIGN_NONE, false);
    }

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);

    return inter.clicked;
}

// ============================================================================
// Implementation: scroll panels
// ============================================================================

static const float WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED = 20.0f;

static void wlx_resolve_opt_scroll_panel(const WLX_Context *ctx, WLX_Scroll_Panel_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    if (wlx_color_is_zero(opt->back_color))       opt->back_color                 = theme->background;
    if (wlx_color_is_zero(opt->scrollbar_color))  opt->scrollbar_color            = theme->scrollbar.bar;
    if (wlx_color_is_zero(opt->border_color))     opt->border_color               = theme->border;
    if (wlx_is_negative_unset(opt->border_width))            opt->border_width               = theme->border_width;
    if (wlx_is_float_unset(opt->scrollbar_hover_brightness)) opt->scrollbar_hover_brightness = theme->hover_brightness;
    if (wlx_is_negative_unset(opt->scrollbar_width))       opt->scrollbar_width            = theme->scrollbar.width;
    opt->opacity = wlx_resolve_opacity_for(ctx, opt->opacity);
    WLX_APPLY_OPACITY(opt->opacity, &opt->back_color, &opt->scrollbar_color, &opt->border_color);
    wlx_resolve_roundness(theme, &opt->roundness, &opt->rounded_segments);
}

// Helper: resolve cell rect and widget rect for a scroll panel.
static inline void wlx_scroll_panel_resolve_rect(
    WLX_Context *ctx, const WLX_Scroll_Panel_Opt *opt,
    WLX_Rect *out_cell, WLX_Rect *out_widget)
{
    *out_cell = wlx_get_widget_cell_rect(ctx, opt->pos, opt->span, opt->padding,
        opt->padding_top, opt->padding_right, opt->padding_bottom, opt->padding_left);
    *out_widget = wlx_resolve_widget_rect(*out_cell, opt->width, opt->height,
        opt->min_width, opt->min_height, opt->max_width, opt->max_height,
        opt->widget_align, opt->overflow);
}

// Helper: contribute scroll panel's viewport height to the parent layout's content tracking.
static inline void wlx_scroll_panel_contribute_to_parent(WLX_Context *ctx, float vp_contrib) {
    if (ctx->arena.layouts.count > 0) {
        WLX_Layout *parent_l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
        if (parent_l->kind == WLX_LAYOUT_LINEAR && parent_l->linear.orient == WLX_HORZ) {
            if (vp_contrib > parent_l->accumulated_content_height)
                parent_l->accumulated_content_height = vp_contrib;
        } else {
            parent_l->accumulated_content_height += vp_contrib;
        }
        // Per-slot content height for WLX_SIZE_CONTENT parents (e.g. panels).
        // index was already advanced by wlx_get_slot_rect above.
        if (parent_l->has_content_slot_heights && parent_l->index > 0) {
            size_t slot_idx = parent_l->index - 1;
            if (slot_idx < WLX_CONTENT_SLOTS_MAX) {
                float *csh = wlx_layout_content_heights(ctx, parent_l);
                csh[slot_idx] += vp_contrib;
            }
        }
        if (parent_l->has_grid_row_content_heights) {
            size_t row = parent_l->grid.last_placed_row;
            if (row < parent_l->grid.rows) {
                float *rch = wlx_grid_row_content_heights(ctx, parent_l);
                if (vp_contrib > rch[row])
                    rch[row] = vp_contrib;
            }
        }
    }
}

// Helper: compute the scrollbar handle rect from panel geometry and scroll state.
static inline WLX_Rect wlx_scrollbar_rect(
    WLX_Rect panel_rect, float content_height, float scroll_offset, float scrollbar_width)
{
    float bar_h = (panel_rect.h / content_height) * panel_rect.h;
    float bar_y = (scroll_offset / content_height) * panel_rect.h;
    return (WLX_Rect){
        panel_rect.x + panel_rect.w - scrollbar_width,
        panel_rect.y + bar_y,
        scrollbar_width,
        bar_h
    };
}

// Helper: process scrollbar drag interaction and update scroll_offset in state.
static inline void wlx_scrollbar_handle_drag(
    WLX_Context *ctx, WLX_Scroll_Panel_State *state, WLX_Rect panel_rect,
    WLX_Rect sb_rect, float max_scroll, const char *file, int line)
{
    WLX_Interaction sb_inter = wlx_get_interaction(
        ctx, sb_rect, WLX_INTERACT_HOVER | WLX_INTERACT_DRAG, file, line);

    if (sb_inter.active && !state->dragging_scrollbar) {
        state->dragging_scrollbar = true;
        state->drag_offset = (float)ctx->input.mouse_y - sb_rect.y;
    }
    if (sb_inter.active && state->dragging_scrollbar) {
        float scrollbar_track = panel_rect.h - sb_rect.h;
        if (scrollbar_track > 0) {
            float new_pos = (float)ctx->input.mouse_y - panel_rect.y - state->drag_offset;
            if (new_pos < 0) new_pos = 0;
            if (new_pos > scrollbar_track) new_pos = scrollbar_track;
            state->scroll_offset = (new_pos / scrollbar_track) * max_scroll;
        }
    }
    if (!sb_inter.active) {
        state->dragging_scrollbar = false;
    }
}

// Helper: end scissor mode and restore the parent scroll panel's scissor region if any.
static inline void wlx_scroll_panel_restore_scissor(WLX_Context *ctx) {
    wlx_end_scissor(ctx);
    if (ctx->arena.scroll_panels.count > 0) {
        WLX_Scroll_Panel_State *parent = wlx_pool_scroll_panels(ctx)[ctx->arena.scroll_panels.count - 1];
        wlx_begin_scissor(ctx, parent->panel_rect);
    }
}

// Helper: set up scissor clipping, draw the scrollbar bar, and push the content layout.
static inline void wlx_scroll_panel_begin_content_layout(
    WLX_Context *ctx, WLX_Scroll_Panel_State *state, const WLX_Scroll_Panel_Opt *opt,
    WLX_Rect wr, float content_height, bool sb_visible, WLX_Rect sb_rect)
{
    // Intersect scissor rect with all parent scroll panel rects to contain nested content.
    WLX_Rect scissor = wr;
    for (size_t i = 0; i < ctx->arena.scroll_panels.count; i++) {
        scissor = wlx_rect_intersect(scissor, wlx_pool_scroll_panels(ctx)[i]->panel_rect);
    }
    wlx_begin_scissor(ctx, scissor);

    // Draw scrollbar bar (interaction was already handled before this call).
    if (sb_visible) {
        bool sb_hover = wlx_point_in_rect(
            ctx->input.mouse_x, ctx->input.mouse_y,
            sb_rect.x, sb_rect.y, sb_rect.w, sb_rect.h);
        WLX_Color sb_draw_color = (state->dragging_scrollbar || sb_hover) ?
            wlx_color_brightness(opt->scrollbar_color, opt->scrollbar_hover_brightness) :
            opt->scrollbar_color;
        wlx_draw_rect(ctx, sb_rect, sb_draw_color);
    }

    // Push the single-slot VERT layout that the scroll panel content fills.
    WLX_Rect content_rect = {
        .x = wr.x,
        .y = wr.y - state->scroll_offset,
        .w = wr.w - (opt->show_scrollbar && content_height > wr.h ? opt->scrollbar_width : 0),
        .h = content_height
    };
    WLX_Layout l = wlx_create_layout(ctx, content_rect, 1, WLX_VERT, 0.0f);
    wlx_pool_push(&ctx->arena.layouts, WLX_Layout, l);
    // layout_begin: scroll panel - vb_force=!auto_height (bounded when not auto-sizing)
    WLX_DBG(layout_begin, ctx, !state->auto_height, NULL, 0, 0, -1, 1, NULL, 0);
}

// Prologue result for wlx_scroll_panel_begin_impl, analogous to WLX_Layout_Frame.
// Returned by wlx_scroll_panel_frame_begin; passed by value through begin_impl.
typedef struct {
    WLX_Rect                rect;           // resolved panel widget rect
    WLX_Scroll_Panel_State *state;          // persistent state for this panel
    float                   content_height; // resolved content height for this frame
    float                   max_scroll;     // content_height - rect.h, clamped >= 0
} WLX_Scroll_Panel_Frame;

// Common prologue for wlx_scroll_panel_begin_impl: pushes id, resolves opt, resolves
// rects, fetches and initialises persistent state, contributes to parent, saves and
// updates auto-scroll context, detects hover, and pushes onto the scroll-panel stack.
// opt is modified in-place by wlx_resolve_opt_scroll_panel before returning.
static inline WLX_Scroll_Panel_Frame wlx_scroll_panel_frame_begin(
    WLX_Context *ctx, float content_height, WLX_Scroll_Panel_Opt *opt,
    const char *file, int line)
{
    if (opt->id) wlx_push_id(ctx, wlx_hash_string(opt->id));

    wlx_resolve_opt_scroll_panel(ctx, opt);

    // NOTE: scroll_panel does NOT use wlx_widget_begin() because its scroll-height
    // tracking is conditional (only for non-auto-height panels) and deferred.
    WLX_Rect r, wr;
    wlx_scroll_panel_resolve_rect(ctx, opt, &r, &wr);

    WLX_State persistant = wlx_get_state_impl(ctx, sizeof(WLX_Scroll_Panel_State), file, line);
    WLX_Scroll_Panel_State *state = (WLX_Scroll_Panel_State *)persistant.data;

    // Initialize content_height on first frame (when calloc'd to 0).
    if (state->content_height == 0.0f) {
        state->content_height = (content_height < 0) ? wr.h : content_height;
    }

    state->auto_height = (content_height < 0);
    state->panel_rect = wr;
    state->wheel_scroll_speed = opt->wheel_scroll_speed;

    // Contribute this panel's viewport height to the parent layout's content tracking.
    float vp_contrib = (opt->height > 0) ? (float)opt->height : r.h;
    wlx_scroll_panel_contribute_to_parent(ctx, vp_contrib);

    // Save outer auto-scroll tracking context before potentially changing it.
    state->saved_auto_scroll_panel_id = ctx->auto_scroll.panel_id;
    state->saved_auto_scroll_total_height = ctx->auto_scroll.total_height;

    // For auto height: keep previous frame's measured content_height (stored in state).
    // For explicit height: update directly.
    if (!state->auto_height) {
        state->content_height = content_height;
    }

    if (state->auto_height) {
        ctx->auto_scroll.panel_id = persistant.id;
        ctx->auto_scroll.total_height = 0;
    } else {
        // Clear so inner widgets don't pollute outer panel's auto-height measurement.
        ctx->auto_scroll.panel_id = 0;
        ctx->auto_scroll.total_height = 0;
    }

    float ch = state->content_height;
    float max_scroll = ch - wr.h;
    if (max_scroll < 0) max_scroll = 0;

    // Panel-level hover detection (wheel scrolling is deferred to wlx_scroll_panel_end).
    state->hovered = wlx_point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, wr.x, wr.y, wr.w, wr.h);

    // Record whether an id was pushed so wlx_scroll_panel_end can pop it.
    state->pushed_id = opt->id;

    // Push onto scroll panel stack so wlx_scroll_panel_end can handle wheel events.
    wlx_pool_push(&ctx->arena.scroll_panels, WLX_Scroll_Panel_State *, state);

    return (WLX_Scroll_Panel_Frame){
        .rect           = wr,
        .state          = state,
        .content_height = ch,
        .max_scroll     = max_scroll,
    };
}

WLXDEF void wlx_scroll_panel_begin_impl(WLX_Context *ctx, float content_height, WLX_Scroll_Panel_Opt opt, const char *file, int line) {
    // Prologue: resolve opt, rects, state, contribute to parent, push scroll panel stack.
    WLX_Scroll_Panel_Frame frame = wlx_scroll_panel_frame_begin(ctx, content_height, &opt, file, line);

    // Scrollbar geometry.
    bool sb_visible = opt.show_scrollbar && frame.content_height > frame.rect.h;
    WLX_Rect sb_rect = {0};
    if (sb_visible) {
        sb_rect = wlx_scrollbar_rect(frame.rect, frame.content_height, frame.state->scroll_offset, opt.scrollbar_width);
    }

    // Scrollbar drag interaction.
    if (sb_visible) {
        wlx_scrollbar_handle_drag(ctx, frame.state, frame.rect, sb_rect, frame.max_scroll, file, line);
    } else {
        frame.state->dragging_scrollbar = false;
    }

    // Clamp scroll offset.
    if (frame.state->scroll_offset < 0) frame.state->scroll_offset = 0;
    if (frame.state->scroll_offset > frame.max_scroll) frame.state->scroll_offset = frame.max_scroll;

    // Recompute scrollbar position after drag may have changed scroll_offset.
    if (sb_visible) {
        sb_rect = wlx_scrollbar_rect(frame.rect, frame.content_height, frame.state->scroll_offset, opt.scrollbar_width);
    }

    // Draw panel background and border.
    wlx_draw_box(ctx, frame.rect, (WLX_Box_Style){
        .fill             = opt.back_color,
        .border           = opt.border_color,
        .border_width     = opt.border_width,
        .roundness        = opt.roundness,
        .rounded_segments = opt.rounded_segments,
    });

    // Scissor, scrollbar bar draw, content layout.
    // The scope id (if any) remains pushed until wlx_scroll_panel_end so descendants
    // can see the panel's id scope.
    wlx_scroll_panel_begin_content_layout(ctx, frame.state, &opt, frame.rect, frame.content_height, sb_visible, sb_rect);
}

// Scope-pop half of the scroll panel frame lifecycle: pops the scope id if one
// was pushed at begin.
static inline void wlx_scroll_panel_frame_end(WLX_Context *ctx, WLX_Scroll_Panel_State *state) {
    if (state->pushed_id) wlx_pop_id(ctx);
}

WLXDEF void wlx_scroll_panel_end(WLX_Context *ctx) {
    assert(ctx->arena.layouts.count > 0);
    // The wrapper layout's accumulated_content_height now contains the
    // fully-aggregated content tree height, propagated up from all nested
    // layouts via wlx_layout_end.  This is the single authoritative source
    // for auto-scroll content measurement.
    WLX_Layout *wrapper = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
    if (ctx->auto_scroll.panel_id) {
        ctx->auto_scroll.total_height = wrapper->accumulated_content_height;
    }
    ctx->arena.layouts.count -= 1;

    // Pop scroll panel from the stack.
    assert(ctx->arena.scroll_panels.count > 0 &&
        "wlx_scroll_panel_end without matching wlx_scroll_panel_begin - "
        "possible orphaned end call after refactoring");
    WLX_Scroll_Panel_State *state_sp = wlx_pool_scroll_panels(ctx)[--ctx->arena.scroll_panels.count];

    // Update auto-height scroll panel with measured content height FIRST,
    // before wheel handling, so max_scroll uses the fresh measurement.
    // Without this ordering, fast wheel scrolling oscillates because wheel
    // deltas are clamped to the stale (previous-frame) content_height, then
    // the content_height changes, and next frame's clamp snaps the offset
    // to a different value - producing visible flicker.
    if (state_sp->auto_height && ctx->auto_scroll.panel_id != 0 && ctx->states.capacity > 0) {
        WLX_State_Map_Slot *slot = wlx_state_map_find(&ctx->states, ctx->auto_scroll.panel_id);
        if (slot->id != 0) {
            WLX_Scroll_Panel_State *state = (WLX_Scroll_Panel_State *)slot->data;
            float measured = ctx->auto_scroll.total_height;
            if (measured > 0) {
                state->content_height = measured;
            }
        }
    }

    // Handle wheel scrolling with the up-to-date content_height.
    // Only consume the wheel event when this panel actually has scrollable
    // content (max_scroll > 0).  When content fits the viewport, let the
    // event bubble up to the parent scroll panel.
    if (state_sp->hovered && ctx->input.wheel_delta != 0.0f) {
        float max_scroll = state_sp->content_height - state_sp->panel_rect.h;
        if (max_scroll < 0) max_scroll = 0;

        if (max_scroll > 0) {
            float scroll_speed = state_sp->wheel_scroll_speed;
            if (scroll_speed <= 0.0f) scroll_speed = WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED;
            state_sp->scroll_offset -= ctx->input.wheel_delta * scroll_speed;

            if (state_sp->scroll_offset < 0) state_sp->scroll_offset = 0;
            if (state_sp->scroll_offset > max_scroll) state_sp->scroll_offset = max_scroll;
            ctx->input.wheel_delta = 0.0f;  // consume - prevent outer panels from scrolling
        }
    }

    // Re-clamp scroll offset to (possibly updated) content height
    {
        float max_scroll = state_sp->content_height - state_sp->panel_rect.h;
        if (max_scroll < 0) max_scroll = 0;
        if (state_sp->scroll_offset > max_scroll) state_sp->scroll_offset = max_scroll;
        if (state_sp->scroll_offset < 0) state_sp->scroll_offset = 0;
    }

    // Restore outer auto-scroll tracking context.
    ctx->auto_scroll.panel_id = state_sp->saved_auto_scroll_panel_id;
    ctx->auto_scroll.total_height = state_sp->saved_auto_scroll_total_height;

    wlx_scroll_panel_restore_scissor(ctx);

    wlx_scroll_panel_frame_end(ctx, state_sp);
}

// ============================================================================
// Compound widget: Split panel implementation
// ============================================================================

WLXDEF void wlx_split_begin_impl(WLX_Context *ctx, WLX_Split_Opt opt,
                                  const char *file, int line) {
    // Resolve sentinel defaults
    if (wlx_slot_size_is_zero(opt.fill_size))
        opt.fill_size = WLX_SLOT_FILL;
    if (wlx_slot_size_is_zero(opt.first_size))
        opt.first_size = WLX_SLOT_PX(280);
    if (wlx_slot_size_is_zero(opt.second_size))
        opt.second_size = WLX_SLOT_FLEX(1);
    if (opt.padding < 0.0f)
        opt.padding = 4.0f;
    if (opt.gap < 0.0f)
        opt.gap = 0.0f;

    WLX_DBG(split_begin, ctx);

    // [1] Outer VERT wrapper (viewport-fill)
    WLX_DBG(split_suppress_warn, ctx, true);
    WLX_Slot_Size fill_sizes[] = { opt.fill_size };
    wlx_layout_begin_impl(ctx, 1, WLX_VERT,
        wlx_default_layout_opt(.sizes = fill_sizes, .id = opt.id), file, line);

    // [2] HORZ split with two pane slots
    WLX_Slot_Size split_sizes[] = { opt.first_size, opt.second_size };
    wlx_layout_begin_impl(ctx, 2, WLX_HORZ,
        wlx_default_layout_opt(.sizes = split_sizes, .padding = opt.padding,
            .padding_top = opt.padding_top, .padding_right = opt.padding_right,
            .padding_bottom = opt.padding_bottom, .padding_left = opt.padding_left,
            .gap = opt.gap),
        file, line);
    WLX_DBG(split_suppress_warn, ctx, false);

    // [3] First pane scroll panel (auto-height)
    wlx_scroll_panel_begin_impl(ctx, -1,
        wlx_default_scroll_panel_opt(.back_color = opt.first_back_color),
        file, line);
}

WLXDEF void wlx_split_next_impl(WLX_Context *ctx, WLX_Split_Next_Opt opt,
                                 const char *file, int line) {
    WLX_DBG(split_next, ctx);

    // Close first pane scroll panel
    wlx_scroll_panel_end(ctx);

    // [5] Open second pane scroll panel (auto-height)
    wlx_scroll_panel_begin_impl(ctx, -1,
        wlx_default_scroll_panel_opt(.back_color = opt.back_color),
        file, line);
}

WLXDEF void wlx_split_end_impl(WLX_Context *ctx) {
    WLX_DBG(split_end, ctx);

    // Close second pane scroll panel
    wlx_scroll_panel_end(ctx);

    // Close HORZ split layout
    wlx_layout_end(ctx);

    // Close outer VERT wrapper
    wlx_layout_end(ctx);
}

// ============================================================================
// Panel compound widget
// ============================================================================

WLXDEF void wlx_panel_begin_impl(WLX_Context *ctx, WLX_Panel_Opt opt,
                                  const char *file, int line) {
    // Resolve sentinel defaults
    if (opt.title_font_size <= 0) opt.title_font_size = 18;
    if (opt.title_height <= 0)    opt.title_height = 32;
    if (opt.title_align == WLX_ALIGN_NONE) opt.title_align = WLX_CENTER;
    if (opt.padding < 0.0f)       opt.padding = 2.0f;
    if (opt.capacity <= 0)        opt.capacity = 32;

    // Capacity includes the title slot (if present)
    int total = opt.capacity;
    if (opt.title != NULL) total++;  // reserve one extra slot for heading

    // Clamp to content-slots maximum
    if (total > WLX_CONTENT_SLOTS_MAX)
        total = WLX_CONTENT_SLOTS_MAX;

    // Build all-CONTENT slot sizes array on the stack. Safe because
    // wlx_layout_begin_impl copies content_sizes into the byte scratch
    // arena, so this array does not need to outlive this function.
    WLX_Slot_Size sizes[WLX_CONTENT_SLOTS_MAX];
    for (int i = 0; i < total; i++) {
        sizes[i] = (WLX_Slot_Size){ .kind = WLX_SIZE_CONTENT };
    }

    // Create VERT layout with all-CONTENT slots
    wlx_layout_begin_impl(ctx, (size_t)total, WLX_VERT,
        wlx_default_layout_opt(.sizes = sizes, .padding = opt.padding,
            .padding_top = opt.padding_top, .padding_right = opt.padding_right,
            .padding_bottom = opt.padding_bottom, .padding_left = opt.padding_left,
            .gap = opt.gap, .id = opt.id),
        file, line);

    // Emit heading label if title is provided
    if (opt.title != NULL) {
        wlx_label_impl(ctx, opt.title,
            wlx_default_label_opt(
                .font_size = opt.title_font_size,
                .height = opt.title_height,
                .align = opt.title_align,
                .show_background = true,
                .back_color = opt.title_back_color),
            file, line);
    }
}

WLXDEF void wlx_panel_end(WLX_Context *ctx) {
    wlx_layout_end(ctx);
}

// ============================================================================
// Debug implementation (WLX_DEBUG only)
// ============================================================================
#ifdef WLX_DEBUG

#include <stdarg.h>

// --- Debug content-slot companion (oscillation detection) ---
typedef struct {
    float prev_measured[WLX_CONTENT_SLOTS_MAX];
    int   oscillation_count[WLX_CONTENT_SLOTS_MAX];
    bool  oscillation_warned[WLX_CONTENT_SLOTS_MAX];
} WLX_Debug_Content_Slot_State;

typedef struct {
    WLX_Content_Slot_State *key;
    WLX_Debug_Content_Slot_State *data;
} WLX_Debug_Companion;

typedef struct {
    bool vert_bounded;
    bool content_bootstrapping;
    const WLX_Slot_Size *sizes;  // original sizes for child vert_bounded checks
    size_t count;                // number of slots
} WLX_Debug_Layout_Shadow;

typedef struct WLX_Debug_Context {
    // Call-site hit tracking (detect missing wlx_push_id in loops)
    struct { size_t key; const char *file; int line; bool warned; } site_hits[512];
    size_t site_hits_used;

    // Layout debug shadow stack (indexed by layouts.count at push time)
    WLX_Debug_Layout_Shadow layout_shadow[256];

    // Split widget tracking
    int split_depth;
    bool split_next_called[8];
    bool suppress_flex_fill_warn;

    // Warning dispatcher
    void (*warn_cb)(const char *file, int line, const char *msg, void *user_data);
    void *warn_user_data;
    int  warn_count;  // total warnings this frame (for test assertions)

    // Once-per-site deduplication table (persistent across frames)
    size_t warned_site_keys[64];
    int warned_sites_count;

    // Content-slot oscillation companions (keyed by content_state pointer)
    WLX_Debug_Companion content_companions[32];
    int content_companions_count;
} WLX_Debug_Context;

// --- Lifecycle ---

static inline void wlx_dbg_init(WLX_Context *ctx) {
    if (ctx->dbg) return;  // already allocated
    ctx->dbg = (WLX_Debug_Context *)wlx_calloc(1, sizeof(WLX_Debug_Context));
    if (!ctx->dbg) {
        fprintf(stderr, "wollix [DEBUG]: failed to allocate debug context\n");
    }
}

static inline void wlx_dbg_destroy(WLX_Context *ctx) {
    if (ctx->dbg) {
        for (int i = 0; i < ctx->dbg->content_companions_count; i++) {
            wlx_free(ctx->dbg->content_companions[i].data);
        }
        wlx_free(ctx->dbg);
        ctx->dbg = NULL;
    }
}

// --- Warning dispatcher ---

static inline void wlx_dbg_warn(WLX_Context *ctx, const char *file, int line,
                                const char *fmt, ...) {
    if (!ctx->dbg) return;
    ctx->dbg->warn_count++;
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (ctx->dbg->warn_cb) {
        ctx->dbg->warn_cb(file, line, buf, ctx->dbg->warn_user_data);
    } else {
        fprintf(stderr, "wollix [DEBUG] %s:%d: %s\n", file, line, buf);
    }
}

static inline bool wlx_dbg_warn_once(WLX_Context *ctx, const char *file, int line,
                                     const char *fmt, ...) {
    size_t site_key;

    if (!ctx->dbg) return false;
    site_key = wlx_hash_id(file != NULL ? file : "", line);

    // Check if we already warned for this file content + line.
    for (int i = 0; i < ctx->dbg->warned_sites_count; i++) {
        if (ctx->dbg->warned_site_keys[i] == site_key) {
            return false;  // already warned
        }
    }
    // Record it
    if (ctx->dbg->warned_sites_count < (int)wlx_array_len(ctx->dbg->warned_site_keys)) {
        ctx->dbg->warned_site_keys[ctx->dbg->warned_sites_count] = site_key;
        ctx->dbg->warned_sites_count++;
    }
    ctx->dbg->warn_count++;
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (ctx->dbg->warn_cb) {
        ctx->dbg->warn_cb(file, line, buf, ctx->dbg->warn_user_data);
    } else {
        fprintf(stderr, "wollix [DEBUG] %s:%d: %s\n", file, line, buf);
    }
    return true;
}

static inline void wlx_dbg_frame_begin(WLX_Context *ctx) {
    wlx_dbg_init(ctx);  // lazy init on first frame
    if (!ctx->dbg) return;
    ctx->dbg->warn_count = 0;
    // Reset per-frame site-hit tracking
    ctx->dbg->site_hits_used = 0;
    wlx_zero_array(wlx_array_len(ctx->dbg->site_hits), ctx->dbg->site_hits);
}

static inline void wlx_dbg_interaction_id(WLX_Context *ctx, size_t base,
                                          const char *file, int line) {
    if (!ctx->dbg) return;
    // Track per-call-site hits to detect duplicate sites without wlx_push_id.
    // Linear probing hash table - sufficient for any realistic UI frame.
    // If the table exceeds 75% load, stop tracking (degrades gracefully).
    size_t sites_cap = wlx_array_len(ctx->dbg->site_hits);
    if (ctx->dbg->site_hits_used < (sites_cap * 3 / 4)) {
        size_t dslot = base & (sites_cap - 1);
        for (size_t p = 0; p < sites_cap; p++) {
            size_t idx = (dslot + p) & (sites_cap - 1);
            if (ctx->dbg->site_hits[idx].key == 0) {
                // Empty slot - first hit for this site
                ctx->dbg->site_hits[idx].key = base;
                ctx->dbg->site_hits[idx].file = file;
                ctx->dbg->site_hits[idx].line = line;
                ctx->dbg->site_hits[idx].warned = false;
                ctx->dbg->site_hits_used++;
                break;
            } else if (ctx->dbg->site_hits[idx].key == base
                       && ctx->dbg->site_hits[idx].line == line
                       && ctx->dbg->site_hits[idx].file == file) {
                // Duplicate hit - same (file, line, id_stack) without push_id
                if (!ctx->dbg->site_hits[idx].warned) {
                    ctx->dbg->site_hits[idx].warned = true;
                    wlx_dbg_warn(ctx, file, line,
                        "widget hit multiple times without wlx_push_id()\n"
                        "  hint: wrap each loop iteration in "
                        "wlx_push_id(ctx, i) / wlx_pop_id(ctx)");
                }
                break;
            }
        }
    }
}

static inline void wlx_dbg_layout_begin(WLX_Context *ctx, int vb_force,
    const WLX_Slot_Size *sizes, size_t count, int orient,
    int pos, int span, const char *file, int line) {
    if (!ctx->dbg) return;
    size_t idx = ctx->arena.layouts.count - 1;
    if (idx >= wlx_array_len(ctx->dbg->layout_shadow)) return;
    
    
    
    // --- Compute vert_bounded ---
    bool vb;
    if (vb_force >= 0) {
        vb = (bool)vb_force;
    } else if (ctx->auto_scroll.panel_id == 0) {
        vb = true;  // no auto-scroll active - always bounded
    } else if (idx > 0) {
        WLX_Debug_Layout_Shadow *parent_shadow = &ctx->dbg->layout_shadow[idx-1];
        vb = parent_shadow->vert_bounded;  // inherit from parent
        WLX_Layout *parent = &wlx_pool_layouts(ctx)[idx - 1];
        if (parent->kind == WLX_LAYOUT_LINEAR) {
            // A HORZ parent fully bounds its child's height (child.h == parent.h),
            // so the child is vertically bounded iff the HORZ itself is.
            if (parent->linear.orient == WLX_HORZ) {
                vb = parent_shadow->vert_bounded;
            } else if (parent_shadow->sizes != NULL) {
                // VERT parent: child bound depends on the parent slot kind.
                const WLX_Slot_Size *parent_sizes = parent_shadow->sizes;
                size_t parent_count = parent_shadow->count;
                size_t slot_idx = (pos >= 0)
                    ? (size_t)pos
                    : (parent->index > (size_t)span ? parent->index - (size_t)span : 0);
                if (slot_idx < parent_count) {
                    WLX_Size_Kind kind = parent_sizes[slot_idx].kind;
                    // PX/FILL bound directly. CONTENT bounds via measurement
                    // once converged; on the first frame the bootstrap path
                    // suppresses warnings via shadow->content_bootstrapping.
                    if (kind == WLX_SIZE_PIXELS || kind == WLX_SIZE_FILL
                        || kind == WLX_SIZE_CONTENT) vb = true;
                }
            }
        }
    } else {
        vb = false;  // root layout inside auto-scroll with no parent
    }

    WLX_Debug_Layout_Shadow *shadow = &ctx->dbg->layout_shadow[idx];
    shadow->vert_bounded = vb;
    // Prefer the layout's persistent (scratch-arena) copy of the sizes
    // array when available.  Compound widgets like wlx_panel build their
    // sizes array on the stack; the user-supplied `sizes` pointer becomes
    // dangling once the compound widget's frame returns, while the
    // scratch copy stays live until wlx_layout_end.
    {
        WLX_Layout *self = &wlx_pool_layouts(ctx)[idx];
        shadow->sizes = (self->content_sizes != NULL) ? self->content_sizes : sizes;
    }
    shadow->count = count;

    bool cb = false;
    if (idx > 0) {
        WLX_Debug_Layout_Shadow *parent_shadow = &ctx->dbg->layout_shadow[idx - 1];
        if (parent_shadow->content_bootstrapping) {
            cb = true;
        } else if (parent_shadow->sizes != NULL) {
            WLX_Layout *parent = &wlx_pool_layouts(ctx)[idx - 1];
            if (parent->kind == WLX_LAYOUT_LINEAR) {
                size_t slot_idx = (pos >= 0)
                    ? (size_t)pos
                    : (parent->index > (size_t)span
                        ? parent->index - (size_t)span : 0);
                WLX_Layout *current = &wlx_pool_layouts(ctx)[idx];
                float dim = (parent->linear.orient == WLX_HORZ)
                    ? current->rect.w : current->rect.h;
                if (slot_idx < parent_shadow->count
                    && parent_shadow->sizes[slot_idx].kind == WLX_SIZE_CONTENT
                    && dim <= 1.0f) {
                    cb = true;
                }
            }
        }
    }
    shadow->content_bootstrapping = cb;

    // --- Warn about FLEX/FILL slots inside auto-height scroll panels ---
    if (sizes != NULL && !vb && !cb && !ctx->dbg->suppress_flex_fill_warn
        && orient == (int)WLX_VERT) {
        for (size_t i = 0; i < count; i++) {
            if (sizes[i].kind == WLX_SIZE_FLEX) {
                wlx_dbg_warn_once(ctx, file, line,
                    "FLEX slot %zu inside auto-height scroll panel "
                    "- FLEX distributes remaining space which is unbounded "
                    "in auto-height mode. Use PX or CONTENT instead.",
                    i);
            } else if (sizes[i].kind == WLX_SIZE_FILL) {
                wlx_dbg_warn_once(ctx, file, line,
                    "FILL slot %zu inside auto-height scroll panel "
                    "- FILL resolves to viewport height which is unbounded "
                    "in auto-height mode. Use PX or CONTENT instead.",
                    i);
            }
        }
    }
}

static inline WLX_Debug_Content_Slot_State *wlx_dbg_get_companion(
    WLX_Context *ctx, WLX_Content_Slot_State *key) {
    
    WLX_Debug_Companion *comps = ctx->dbg->content_companions;
    
    // Linear scan - content-state instances are rare (<=10 typical)
    for (int i = 0; i < ctx->dbg->content_companions_count; i++) {
        if (comps[i].key == key)
            return comps[i].data;
    }
    // Allocate new companion
    if (ctx->dbg->content_companions_count >= (int)wlx_array_len(ctx->dbg->content_companions))
        return NULL;
    WLX_Debug_Content_Slot_State *comp =
        (WLX_Debug_Content_Slot_State *)wlx_calloc(1, sizeof(WLX_Debug_Content_Slot_State));
    if (!comp) return NULL;
    int ci = ctx->dbg->content_companions_count++;
    comps[ci].key = key;
    comps[ci].data = comp;
    return comp;
}

static inline void wlx_dbg_layout_end(WLX_Context *ctx) {
    if (!ctx->dbg) return;
    WLX_Layout *l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
    if (l->content_state == NULL || l->content_sizes == NULL) return;

    WLX_Debug_Content_Slot_State *comp = wlx_dbg_get_companion(ctx, l->content_state);
    if (!comp) return;

    // Determine iteration count and height source based on layout kind
    size_t slot_count;
    float *heights;
    if (l->kind == WLX_LAYOUT_GRID && l->has_grid_row_content_heights) {
        slot_count = l->grid.rows;
        heights = wlx_grid_row_content_heights(ctx, l);
    } else if (l->has_content_slot_heights) {
        slot_count = l->count;
        heights = wlx_layout_content_heights(ctx, l);
    } else {
        return;
    }

    for (size_t i = 0; i < slot_count; i++) {
        if (l->content_sizes[i].kind == WLX_SIZE_CONTENT) {
            float new_val = heights[i];
            float old_val = l->content_state->measured[i];
            float prev_val = comp->prev_measured[i];
            float epsilon = 0.5f;
            bool changed = fabsf(new_val - old_val) > epsilon;
            bool bouncing = changed && (fabsf(new_val - prev_val) < epsilon);
            if (bouncing) {
                comp->oscillation_count[i]++;
                if (comp->oscillation_count[i] >= 3 && !comp->oscillation_warned[i]) {
                    comp->oscillation_warned[i] = true;
                    wlx_dbg_warn(ctx, NULL, 0,
                        "CONTENT slot %zu oscillation detected "
                        "(%.1f <-> %.1f, %d consecutive frames)",
                        i, old_val, new_val, comp->oscillation_count[i]);
                }
            } else {
                comp->oscillation_count[i] = 0;
                comp->oscillation_warned[i] = false;
            }
            comp->prev_measured[i] = old_val;
        }
    }
}

static inline void wlx_dbg_auto_slot(WLX_Context *ctx, float px, int kind,
                                     float total, float used) {
    if (!ctx->dbg) return;
    if (px <= 0.0f) {
        wlx_dbg_warn(ctx, NULL, 0,
            "wlx_layout_auto_slot resolved to %.1fpx "
            "(kind=%d, total=%.1f, used=%.1f) - clamped to 1px",
            px, kind, total, used);
    }
}

static inline bool wlx_dbg_widget_in_content_slot(WLX_Context *ctx, int span) {
    if (!ctx->dbg) return false;
    if (ctx->arena.layouts.count == 0) return false;

    size_t idx = ctx->arena.layouts.count - 1;
    WLX_Layout *pl = &wlx_pool_layouts(ctx)[idx];

    if (pl->content_sizes != NULL) {
        const WLX_Slot_Size *cs = (const WLX_Slot_Size *)
            &wlx_pool_scratch(ctx)[pl->content_sizes_scratch_off];

        if (pl->kind == WLX_LAYOUT_GRID && pl->has_grid_row_content_heights) {
            size_t row = pl->grid.last_placed_row;
            return row < pl->grid.rows && cs[row].kind == WLX_SIZE_CONTENT;
        }

        if (pl->has_content_slot_heights && pl->index >= (size_t)span) {
            size_t si = pl->index - (size_t)span;
            if (si < WLX_CONTENT_SLOTS_MAX && cs[si].kind == WLX_SIZE_CONTENT)
                return true;
        }
    }

    if (idx < wlx_array_len(ctx->dbg->layout_shadow)
        && ctx->dbg->layout_shadow[idx].content_bootstrapping)
        return true;

    return false;
}

static inline void wlx_dbg_widget_begin(WLX_Context *ctx, WLX_Rect cell,
                                        float height, int span, bool overflow,
                                        const char *file, int line) {
    if (!ctx->dbg) return;
    // Warn when a widget's requested height significantly exceeds the slot
    // height, which causes silent clipping.  Skip CONTENT slots (they will
    // auto-resize on the next frame) and only warn once per call site.
    if (height > 0 && cell.h > 0 && height > cell.h * 1.5f && !overflow) {
        if (!wlx_dbg_widget_in_content_slot(ctx, span)) {
            wlx_dbg_warn_once(ctx, file, line,
                "widget requested height %.0f but slot is only %.0fpx "
                "\xe2\x80\x94 content will be clipped",
                height, cell.h);
        }
    }
}

static inline void wlx_dbg_split_begin(WLX_Context *ctx) {
    if (!ctx->dbg) return;
    assert(ctx->dbg->split_depth < 8 && "split nesting too deep (max 8)");
    ctx->dbg->split_next_called[ctx->dbg->split_depth] = false;
    ctx->dbg->split_depth++;
    // One-time explanation of split's internal structure
    static bool split_explained = false;
    if (!split_explained) {
        fprintf(stderr,
            "wollix [DEBUG]: wlx_split_begin creates:\n"
            "  [1] outer VERT layout (fill_size, default=FILL)\n"
            "  [2] inner HORZ layout (first_size + second_size)\n"
            "  [3] auto-height scroll panel per pane\n"
            "  Do not add your own scroll panels unless nesting is intended.\n");
        split_explained = true;
    }
}

static inline void wlx_dbg_split_suppress_warn(WLX_Context *ctx, bool suppress) {
    if (!ctx->dbg) return;
    ctx->dbg->suppress_flex_fill_warn = suppress;
}

static inline void wlx_dbg_split_next(WLX_Context *ctx) {
    if (!ctx->dbg) return;
    assert(ctx->dbg->split_depth > 0 && "wlx_split_next without matching wlx_split_begin");
    ctx->dbg->split_next_called[ctx->dbg->split_depth - 1] = true;
}

static inline void wlx_dbg_split_end(WLX_Context *ctx) {
    if (!ctx->dbg) return;
    assert(ctx->dbg->split_depth > 0 && "wlx_split_end without matching wlx_split_begin");
    assert(ctx->dbg->split_next_called[ctx->dbg->split_depth - 1] &&
        "wlx_split_end called without wlx_split_next \xe2\x80\x94 "
        "did you forget to separate the two panes?");
    ctx->dbg->split_depth--;
}

#endif // WLX_DEBUG

#endif // WOLLIX_IMPLEMENTATION
