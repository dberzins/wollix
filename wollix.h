/*
 * wollix.h — Woven layouts for C.
 *
 * Version: 0.1.0  (WOLLIX_VERSION / WLX_VERSION)
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
 * written in C99. It lets you compose rows, columns, grids, and nested
 * panels that interlock widgets into place. The core has zero external
 * dependencies; rendering is delegated to thin backend adapters:
 *
 *   - wollix_raylib.h   (Raylib backend)
 *   - wollix_sdl3.h     (SDL3 backend)
 *
 * Built-in widgets: button, checkbox, textured checkbox, text box, input box,
 * slider, and scroll panel.
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
 *
 *   wlx_begin(&ctx, root_rect, wlx_process_raylib_input);
 *     wlx_layout_begin(&ctx, 2, WLX_VERT, .padding = 8);
 *       wlx_textbox(&ctx, "Hello!", .font_size = 24);
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
 *     Default (undefined): single-pass clamp — simpler and O(n), but may
 *     leave a small gap or overflow when min/max constraints fire.
 *
 * ---------------------------------------------------------------------------
 * TUNING MACROS (override with your own values if needed)
 * ---------------------------------------------------------------------------
 *
 * WLX_MAX_SLOT_COUNT  (default 100000)
 *     Upper-bound sanity check for slot/row/column counts.
 *
 * WLX_DA_INIT_CAP  (default 256)
 *     Initial capacity for internal dynamic arrays.
 *
 * wlx_alloc(size)  /  wlx_calloc(count, size)  /  wlx_realloc(ptr, size)  /  wlx_free(ptr)
 *     Internal allocation macros. By default they forward to malloc/calloc/
 *     realloc/free. Redefine them BEFORE including wollix.h to plug in your
 *     own allocator (arena, pool, etc.):
 *       #define wlx_alloc(size)          my_alloc(size)
 *       #define wlx_calloc(count, size)  my_calloc(count, size)
 *       #define wlx_realloc(ptr, size)   my_realloc(ptr, size)
 *       #define wlx_free(ptr)            my_free(ptr)
 *     These are for the library's internal use — user/demo code should
 *     call its own allocation routines directly.
 *
 * ---------------------------------------------------------------------------
 * DEBUG MACROS
 * ---------------------------------------------------------------------------
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

#define WOLLIX_VERSION "0.1.0"
#define WLX_VERSION WOLLIX_VERSION

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

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
// Catches accidental negative → size_t wraparound (e.g. passing -1).
#define WLX_MAX_SLOT_COUNT 100000


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
    ptr = NULL;
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

// Bundled text rendering parameters — replaces loose
// (font_size, spacing, color) arguments in backend callbacks and text helpers.
typedef struct {
    WLX_Font  font;      // 0 → backend default font
    int      font_size;  // 0 → resolve from theme
    int      spacing;    // 0 → resolve from theme
    WLX_Color color;     // {0,0,0,0} → resolve from theme foreground
} WLX_Text_Style;

#define WLX_TEXT_STYLE_DEFAULT \
    ((WLX_Text_Style){ .font = WLX_FONT_DEFAULT, .font_size = 0, .spacing = 0, .color = {0} })

typedef struct {
    void (*draw_rect)(WLX_Rect rect, WLX_Color color);
    void (*draw_rect_lines)(WLX_Rect rect, float thick, WLX_Color color);
    void (*draw_rect_rounded)(WLX_Rect rect, float roundness, int segments, WLX_Color color);
    void (*draw_line)(float x1, float y1, float x2, float y2, float thick, WLX_Color color);
    void (*draw_text)(const char *text, float x, float y, WLX_Text_Style style);
    void (*measure_text)(const char *text, WLX_Text_Style style, float *out_w, float *out_h);
    void (*draw_texture)(WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint);
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

typedef enum {
    WLX_LAYOUT_LINEAR,
    WLX_LAYOUT_GRID,
} WLX_Layout_Kind;

typedef struct {
    WLX_Layout_Kind kind;
    WLX_Rect rect;
    size_t count;
    size_t index;
    bool overflow;
    float accumulated_content_height;

    union {
        struct {
            WLX_Orient orient;
            float *offsets;  // points into ctx->slot_size_offsets buffer
            // auto-sizing / dynamic append
            bool   dynamic;
            float  slot_size;
            size_t slot_size_offsets_base;
            float  next_slot_size;
        } linear;

        struct {
            size_t rows;
            size_t cols;
            float *row_offsets;     // [0..rows], points into scratch buffer
            float *col_offsets;     // [0..cols], points into scratch buffer
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
        } grid;
    };
} WLX_Layout;

// Growable slot sizes buffer for layout slot offsets — reused across frames,
// reset each frame in wlx_begin(). Layouts reserve slices from this buffer
// via wlx_scratch_alloc(), so there is zero per-layout heap traffic.
typedef struct {
    float *items;
    size_t count;
    size_t capacity;
} WLX_Scratch_Offsets;

typedef struct {
    WLX_Layout *items;
    size_t count;
    size_t capacity;
} WLX_Layout_Stack;

typedef struct {
    size_t cursor_pos;
    float cursor_blink_time;
} WLX_Inputbox_State;

typedef struct {
    float scroll_offset;
    float content_height;
    bool auto_height;
    WLX_Rect panel_rect;
    bool dragging_scrollbar;  // true while dragging the scrollbar thumb
    float drag_offset;        // mouse offset from scrollbar thumb top when drag started
    bool hovered;             // mouse is over this panel's rect this frame
    float wheel_scroll_speed;

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

// ID stack for loop disambiguation — use wlx_push_id()/wlx_pop_id()
typedef struct {
    size_t *items;
    size_t count;
    size_t capacity;
} WLX_Id_Stack;

// Handle returned by wlx_get_state() — gives access to both the ID and the data pointer
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
// Theme system
// ============================================================================

typedef struct {
    // ── Global colors ───────────────────────────────────────────────
    WLX_Color background;        // window / panel clear color
    WLX_Color foreground;        // default text color (front_color)
    WLX_Color surface;           // widget background (back_color for buttons, etc.)
    WLX_Color border;            // default border color
    WLX_Color accent;            // active / focused accent (fill bar, focus ring)

    // ── Text ────────────────────────────────────────────────────────
    WLX_Font font;               // default font for all widgets (0 = backend default)
    int   font_size;            // default font size for all widgets
    int   spacing;              // default text spacing

    // ── Geometry ────────────────────────────────────────────────────
    float padding;              // default inner padding for layout slots
    float roundness;            // default corner roundness (0 = sharp)
    int   rounded_segments;     // segment count for rounded drawing

    // ── Interaction feedback ────────────────────────────────────────
    float hover_brightness;     // brightness shift on hover

    // ── Opacity ─────────────────────────────────────────────────────
    float opacity;              // global opacity multiplier (0 = fully opaque sentinel, 0.0001–1.0 = explicit)

    // ── Widget-specific overrides (zero = use globals) ──────────────
    struct {
        WLX_Color border_focus;  // {0} → derive from accent
        WLX_Color cursor;        // {0} → use foreground
    } input;

    struct {
        WLX_Color track;         // {0} → derive from surface
        WLX_Color thumb;         // {0} → use foreground
        WLX_Color label;         // {0} → use foreground
        float    track_height;  // 0 → default (6)
        float    thumb_width;   // 0 → default (14)
    } slider;

    struct {
        WLX_Color check;         // {0} → use foreground
        WLX_Color border;        // {0} → use theme border
    } checkbox;

    struct {
        WLX_Color bar;           // scrollbar thumb color
        float    width;         // scrollbar width (0 → default 10)
    } scrollbar;
    } WLX_Theme;

// Built-in theme presets (defined in WOLLIX_IMPLEMENTATION section)
extern const WLX_Theme wlx_theme_dark;
extern const WLX_Theme wlx_theme_light;

// Helper: check if a color is the sentinel (all-zero = "use theme default")
static inline bool wlx_color_is_zero(WLX_Color c) {
    return c.r == 0 && c.g == 0 && c.b == 0 && c.a == 0;
}

typedef struct {
    WLX_Rect rect;
    WLX_Backend backend;
    WLX_Input_State input;

    // Widget interaction state (hot = hovered, active = pressed/focused)
    struct {
        size_t hot_id;
        size_t active_id;
        bool   active_id_seen; // true if any widget matched active_id this frame
    } interaction;

    WLX_Layout_Stack layouts;
    WLX_State_Map states;
    WLX_Id_Stack id_stack;

    // Auto scroll panel content height tracking
    struct {
        size_t panel_id;      // 0 = not tracking
        float  total_height;
    } auto_scroll;

    // Nested scroll panel stack
    WLX_Scroll_Panel_Stack scroll_panels;
    // Scratch buffer for layout slot offsets (reset each frame)
    WLX_Scratch_Offsets slot_size_offsets;

#ifdef WLX_DEBUG
    // Debug: per-call-site hit tracking to detect missing wlx_push_id() in loops.
    // Each slot tracks a unique (file/line + id_stack) combination per frame.
    struct {
        struct { size_t key; const char *file; int line; bool warned; } slots[512];
        size_t used;
    } debug_site_hits;
    // Optional callback for debug warnings (NULL = fprintf to stderr).
    // Signature: void callback(const char *file, int line, size_t hit_count, void *user_data)
    void (*debug_warn_cb)(const char *, int, size_t, void *);
    void *debug_warn_user_data;
#endif

    // Theme — NULL means use &wlx_theme_dark (set automatically in wlx_begin)
    const WLX_Theme *theme;
} WLX_Context;

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

static inline bool wlx_backend_is_ready(const WLX_Context *ctx) {
    return ctx != NULL &&
        ctx->backend.draw_rect != NULL &&
        ctx->backend.draw_rect_lines != NULL &&
        ctx->backend.draw_rect_rounded != NULL &&
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
WLXDEF WLX_Layout wlx_create_layout(WLX_Context *ctx, WLX_Rect r, size_t count, WLX_Orient orient);
WLXDEF WLX_Layout wlx_create_layout_auto(WLX_Context *ctx, WLX_Rect r, WLX_Orient orient, float slot_px);
WLXDEF WLX_Layout wlx_create_grid(WLX_Context *ctx, WLX_Rect r, size_t rows, size_t cols,
    const WLX_Slot_Size *row_sizes, const WLX_Slot_Size *col_sizes);
WLXDEF WLX_Layout wlx_create_grid_auto(WLX_Context *ctx, WLX_Rect r,
    size_t cols, float row_px, const WLX_Slot_Size *col_sizes);
WLXDEF WLX_Rect wlx_get_slot_rect(WLX_Context *ctx, WLX_Layout *l, int pos, size_t span);

WLXDEF WLX_Rect wlx_get_align_rect(WLX_Rect parent_rect, int width, int height, WLX_Align align);

WLXDEF bool wlx_is_key_down(WLX_Context *ctx, WLX_Key_Code key);
WLXDEF bool wlx_is_key_pressed(WLX_Context *ctx, WLX_Key_Code key);
WLXDEF bool wlx_point_in_rect(int px, int py, int x, int y, int w, int h);

// ID stack — push before a loop body, pop after, to give each iteration a unique stable ID.
// Works for both `wlx_get_interaction()` and `wlx_get_state()`.
WLXDEF void wlx_push_id(WLX_Context *ctx, size_t id);
WLXDEF void wlx_pop_id(WLX_Context *ctx);

// Unified interaction handler - replaces get_widget_state/get_input_state/inline state
// Use `WLX_Interact_Flags` to specify desired behavior. Only use ONE of CLICK/FOCUS/DRAG.
// Interaction IDs are hash(file, line) ^ id_stack_hash — the same formula as
// persistent state IDs. Use wlx_push_id()/wlx_pop_id() when the same source
// line is reached multiple times (loops, reusable widget functions).
WLXDEF WLX_Interaction wlx_get_interaction(WLX_Context *ctx, WLX_Rect rect, uint32_t flags, const char *file, int line);

// Generic persistent state — returns a handle with the state's ID and a pointer
// to zero-initialized persistent data. The data survives across frames.
// Persistent state IDs are hash(file, line) ^ id_stack_hash — the same formula
// as interaction IDs. Use wlx_push_id()/wlx_pop_id() when the same source
// line is reached multiple times (loops, reusable widget functions).
WLXDEF WLX_State wlx_get_state_impl(WLX_Context *ctx, size_t state_size, const char *file, int line);
#define wlx_get_state(ctx, type) wlx_get_state_impl((ctx), sizeof(type), __FILE__, __LINE__)

WLXDEF void wlx_begin(WLX_Context *ctx, WLX_Rect r, WLX_Input_Handler input_handler);
WLXDEF void wlx_end(WLX_Context *ctx);
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
    float padding

#define WLX_LAYOUT_SLOT_DEFAULTS \
    .pos = -1, .span = 1, .overflow = false, .padding = 0

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    const WLX_Slot_Size *sizes;
} WLX_Layout_Opt;

typedef struct {
    size_t row_span;
    size_t col_span;
} WLX_Grid_Cell_Opt;

#define wlx_default_grid_cell_opt(...) \
    (WLX_Grid_Cell_Opt){ .row_span = 1, .col_span = 1, __VA_ARGS__ }

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    const WLX_Slot_Size *row_sizes;
    const WLX_Slot_Size *col_sizes;
} WLX_Grid_Opt;

#define wlx_default_grid_opt(...) \
    (WLX_Grid_Opt){ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        .row_sizes = NULL, .col_sizes = NULL, \
        __VA_ARGS__ \
    }

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    const WLX_Slot_Size *col_sizes;
} WLX_Grid_Auto_Opt;

#define wlx_default_grid_auto_opt(...) \
    (WLX_Grid_Auto_Opt){ WLX_LAYOUT_SLOT_DEFAULTS, .col_sizes = NULL, __VA_ARGS__ }


#define wlx_default_layout_opt(...) \
    (WLX_Layout_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        .sizes = NULL, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_layout_begin_impl(WLX_Context *ctx, size_t count, WLX_Orient orient, WLX_Layout_Opt opt);
#define wlx_layout_begin(ctx, count, orient, ...) wlx_layout_begin_impl((ctx), (count), (orient), wlx_default_layout_opt(__VA_ARGS__))

WLXDEF void wlx_layout_begin_auto_impl(WLX_Context *ctx, WLX_Orient orient, float slot_px, WLX_Layout_Opt opt);
// Dynamic layout: slot count grows as children are added.
//   slot_px > 0  — fixed pixel size for every slot (height for `WLX_VERT`, width for `WLX_HORZ`).
//   slot_px = 0  — variable-size mode: each child must call `wlx_layout_auto_slot_px()` before it.
// Use two-pass counting for equal-division when count is unknown.
#define wlx_layout_begin_auto(ctx, orient, slot_px, ...) wlx_layout_begin_auto_impl((ctx), (orient), (slot_px), wlx_default_layout_opt(__VA_ARGS__))

// Override the pixel size for the *next* slot in the enclosing dynamic layout.
// Call this immediately before the widget (or nested `wlx_layout_begin()`) whose size differs
// from the layout's global slot_px.  The override is consumed and cleared automatically.
WLXDEF void wlx_layout_auto_slot_px(WLX_Context *ctx, float px);

// Override the pixel height for the *next* row in the enclosing dynamic grid.
// Call this immediately before the first widget of the row whose height differs
// from the grid's default row_px.  The override is consumed and cleared
// automatically when the new row is created.
WLXDEF void wlx_grid_auto_row_px(WLX_Context *ctx, float px);

WLXDEF void wlx_grid_cell_impl(WLX_Context *ctx, int row, int col, WLX_Grid_Cell_Opt opt);
#define wlx_grid_cell(ctx, row, col, ...) \
    wlx_grid_cell_impl((ctx), (row), (col), wlx_default_grid_cell_opt(__VA_ARGS__))

WLXDEF void wlx_layout_end(WLX_Context *ctx);

WLXDEF void wlx_grid_begin_impl(WLX_Context *ctx, size_t rows, size_t cols, WLX_Grid_Opt opt);
#define wlx_grid_begin(ctx, rows, cols, ...) \
    wlx_grid_begin_impl((ctx), (rows), (cols), wlx_default_grid_opt(__VA_ARGS__))

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
    int16_t width;      /* -1 use parent width  */ \
    int16_t height;     /* -1 use parent height */ \
    int16_t min_width;  /* 0 = unconstrained */ \
    int16_t min_height; /* 0 = unconstrained */ \
    int16_t max_width;  /* 0 = unconstrained */ \
    int16_t max_height; /* 0 = unconstrained */ \
    float opacity       /* 0 = fully opaque (sentinel), 0.0001–1.0 = explicit */

#define WLX_WIDGET_SIZING_DEFAULTS \
    .widget_align = WLX_LEFT, .width = -1, .height = -1, \
    .min_width = 0, .min_height = 0, .max_width = 0, .max_height = 0, \
    .opacity = 0

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

typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // Color
    WLX_Color color;

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
    bool boxed;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Textbox_Opt;


#define wlx_default_textbox_opt(...) \
    (WLX_Textbox_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        /* Typography */ \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        .wrap = true, \
        /* Styles */ \
        WLX_TEXT_COLOR_DEFAULTS, \
        .boxed = false, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_textbox_impl(WLX_Context *ctx, const char *text, WLX_Textbox_Opt opt, const char *file, int line);
#define wlx_textbox(ctx, text, ...) wlx_textbox_impl((ctx), (text), wlx_default_textbox_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // Typography
    WLX_TEXT_TYPOGRAPHY_FIELDS;

    // Styles
    WLX_TEXT_COLOR_FIELDS;
    bool boxed;

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
        .boxed = false, \
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
    bool boxed;
    WLX_Color border_color;
    WLX_Color check_color;

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
        .boxed = false, \
        .border_color = {0}, \
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

    // Typography
    WLX_TEXT_TYPOGRAPHY_FIELDS;

    // Styles
    WLX_TEXT_COLOR_FIELDS;
    bool boxed;
    WLX_Texture tex_checked;
    WLX_Texture tex_unchecked;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Checkbox_Tex_Opt;

#define wlx_default_checkbox_tex_opt(...) \
    (WLX_Checkbox_Tex_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        /* Typography */ \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        .wrap = true, \
        /* Styles */ \
        WLX_TEXT_COLOR_DEFAULTS, \
        .boxed = false, \
        __VA_ARGS__ \
    }

WLXDEF bool wlx_checkbox_tex_impl(WLX_Context *ctx, const char *text, bool *checked, WLX_Checkbox_Tex_Opt opt, const char *file, int line);
#define wlx_checkbox_tex(ctx, text, checked, ...) wlx_checkbox_tex_impl((ctx), (text), (checked), wlx_default_checkbox_tex_opt(__VA_ARGS__), __FILE__, __LINE__)

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
    float roundness;      // rounding for track/fill/thumb
    int rounded_segments; // segments for rounded drawing
    float hover_brightness;
    float thumb_hover_brightness;
    float fill_inactive_brightness;
    float min_value;
    float max_value;

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
        .roundness = 0, \
        .rounded_segments = 0, \
        .hover_brightness = 0, \
        .thumb_hover_brightness = 0, \
        .fill_inactive_brightness = -0.3f, \
        .min_value = 0.0f, \
        .max_value = 1.0f, \
        __VA_ARGS__ \
    }

WLXDEF bool wlx_slider_impl(WLX_Context *ctx, const char *label, float *value, WLX_Slider_Opt opt, const char *file, int line);
#define wlx_slider(ctx, label, value, ...) wlx_slider_impl((ctx), (label), (value), wlx_default_slider_opt(__VA_ARGS__), __FILE__, __LINE__)

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
        .scrollbar_hover_brightness = 0, \
        .scrollbar_width = 0, \
        .wheel_scroll_speed = 20.0f, \
        .show_scrollbar = true, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_scroll_panel_begin_impl(WLX_Context *ctx, float content_height, WLX_Scroll_Panel_Opt opt, const char *file, int line);
#define wlx_scroll_panel_begin(ctx, content_height, ...) wlx_scroll_panel_begin_impl((ctx), (content_height), wlx_default_scroll_panel_opt(__VA_ARGS__), __FILE__, __LINE__)
WLXDEF void wlx_scroll_panel_end(WLX_Context *ctx);

#ifdef WLX_SHORT_NAMES
#define layout_begin(ctx, count, orient, ...) wlx_layout_begin((ctx), (count), (orient), __VA_ARGS__)
#define layout_begin_auto(ctx, orient, slot_px, ...) wlx_layout_begin_auto((ctx), (orient), (slot_px), __VA_ARGS__)
#define layout_auto_slot_px(ctx, px) wlx_layout_auto_slot_px((ctx), (px))
#define layout_end(ctx) wlx_layout_end((ctx))
#define grid_cell(ctx, row, col, ...) wlx_grid_cell((ctx), (row), (col), __VA_ARGS__)
#define grid_begin(ctx, rows, cols, ...) wlx_grid_begin((ctx), (rows), (cols), __VA_ARGS__)
#define grid_begin_auto(ctx, cols, row_px, ...) wlx_grid_begin_auto((ctx), (cols), (row_px), __VA_ARGS__)
#define grid_begin_auto_tile(ctx, tile_w, tile_h, ...) wlx_grid_begin_auto_tile((ctx), (tile_w), (tile_h), __VA_ARGS__)
#define grid_end(ctx) wlx_grid_end((ctx))
#define textbox(ctx, text, ...) wlx_textbox((ctx), (text), __VA_ARGS__)
#define button(ctx, text, ...) wlx_button((ctx), (text), __VA_ARGS__)
#define checkbox(ctx, text, checked, ...) wlx_checkbox((ctx), (text), (checked), __VA_ARGS__)
#define checkbox_tex(ctx, text, checked, ...) wlx_checkbox_tex((ctx), (text), (checked), __VA_ARGS__)
#define inputbox(ctx, label, buffer, buffer_size, ...) wlx_inputbox((ctx), (label), (buffer), (buffer_size), __VA_ARGS__)
#define slider(ctx, label, value, ...) wlx_slider((ctx), (label), (value), __VA_ARGS__)
#define scroll_panel_begin(ctx, content_height, ...) wlx_scroll_panel_begin((ctx), (content_height), __VA_ARGS__)
#define scroll_panel_end(ctx) wlx_scroll_panel_end((ctx))
#define widget(ctx, ...) wlx_widget((ctx), __VA_ARGS__)
#endif // WLX_SHORT_NAMES


#endif // WOLLIX_H_


////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
#ifdef WOLLIX_IMPLEMENTATION
#include <math.h>



// ============================================================================
// Theme preset definitions
// ============================================================================

const WLX_Theme wlx_theme_dark = {
    .background       = { 18,  18,  18, 255},
    .foreground       = {255, 255, 255, 255},
    .surface          = {  0,   0,   0, 255},
    .border           = {200, 200, 200, 255},
    .accent           = {255, 255, 255, 255},
    .font              = WLX_FONT_DEFAULT,
    .font_size        = 16,
    .spacing          = 2,
    .padding          = 0,
    .roundness        = 0.0f,
    .rounded_segments = 8,
    .hover_brightness = 0.75f,
    .opacity          = 0,
    .input = {
        .border_focus = {255, 255, 255, 255},
        .cursor       = {255, 255, 255, 255},
    },
    .slider = {
        .track        = { 60,  60,  60, 255},
        .thumb        = {255, 255, 255, 255},
        .label        = {255, 255, 255, 255},
        .track_height = 6.0f,
        .thumb_width  = 14.0f,
    },
    .checkbox = {
        .check  = {255, 255, 255, 255},
        .border = {255, 255, 255, 255},
    },
    .scrollbar = {
        .bar   = { 38,  38,  38, 255},
        .width = 10.0f,
    },
};

const WLX_Theme wlx_theme_light = {
    .background       = {240, 240, 240, 255},
    .foreground       = { 20,  20,  20, 255},
    .surface          = {255, 255, 255, 255},
    .border           = {180, 180, 180, 255},
    .accent           = { 40,  80, 200, 255},
    .font              = WLX_FONT_DEFAULT,
    .font_size        = 16,
    .spacing          = 2,
    .padding          = 0,
    .roundness        = 0.0f,
    .rounded_segments = 8,
    .hover_brightness = -0.15f,
    .opacity          = 0,
    .input = {
        .border_focus = { 40,  80, 200, 255},
        .cursor       = { 20,  20,  20, 255},
    },
    .slider = {
        .track        = {200, 200, 200, 255},
        .thumb        = { 40,  80, 200, 255},
        .label        = { 20,  20,  20, 255},
        .track_height = 6.0f,
        .thumb_width  = 14.0f,
    },
    .checkbox = {
        .check  = { 40,  80, 200, 255},
        .border = {180, 180, 180, 255},
    },
    .scrollbar = {
        .bar   = {200, 200, 200, 255},
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
// Implementation: backend wrappers
// ============================================================================

static inline void wlx_measure_text(WLX_Context *ctx, const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
    assert(ctx->backend.measure_text != NULL && "WLX_Backend.measure_text must be set");
    ctx->backend.measure_text(text, style, out_w, out_h);
}

static inline void wlx_draw_text(WLX_Context *ctx, const char *text, float x, float y, WLX_Text_Style style) {
    assert(ctx->backend.draw_text != NULL && "WLX_Backend.draw_text must be set");
    ctx->backend.draw_text(text, x, y, style);
}

static inline void wlx_draw_rect(WLX_Context *ctx, WLX_Rect rect, WLX_Color color) {
    assert(ctx->backend.draw_rect != NULL && "WLX_Backend.draw_rect must be set");
    ctx->backend.draw_rect(rect, color);
}

static inline void wlx_draw_rect_lines(WLX_Context *ctx, WLX_Rect rect, float thick, WLX_Color color) {
    assert(ctx->backend.draw_rect_lines != NULL && "WLX_Backend.draw_rect_lines must be set");
    ctx->backend.draw_rect_lines(rect, thick, color);
}

static inline void wlx_draw_rect_rounded(WLX_Context *ctx, WLX_Rect rect, float roundness, int segments, WLX_Color color) {
    assert(ctx->backend.draw_rect_rounded != NULL && "WLX_Backend.draw_rect_rounded must be set");
    ctx->backend.draw_rect_rounded(rect, roundness, segments, color);
}

static inline void wlx_draw_line(WLX_Context *ctx, float x1, float y1, float x2, float y2, float thick, WLX_Color color) {
    assert(ctx->backend.draw_line != NULL && "WLX_Backend.draw_line must be set");
    ctx->backend.draw_line(x1, y1, x2, y2, thick, color);
}

static inline void wlx_draw_texture(WLX_Context *ctx, WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    assert(ctx->backend.draw_texture != NULL && "WLX_Backend.draw_texture must be set");
    ctx->backend.draw_texture(texture, src, dst, tint);
}

static inline float wlx_get_frame_time(WLX_Context *ctx) {
    assert(ctx->backend.get_frame_time != NULL && "WLX_Backend.get_frame_time must be set");
    return ctx->backend.get_frame_time();
}

static inline void wlx_begin_scissor(WLX_Context *ctx, WLX_Rect rect) {
    assert(ctx->backend.begin_scissor != NULL && "WLX_Backend.begin_scissor must be set");
    ctx->backend.begin_scissor(rect);
}

static inline void wlx_end_scissor(WLX_Context *ctx) {
    assert(ctx->backend.end_scissor != NULL && "WLX_Backend.end_scissor must be set");
    ctx->backend.end_scissor();
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

static inline WLX_Rect wlx_get_widget_cell_rect(WLX_Context *ctx, int pos, size_t span, float padding) {
    assert(ctx != NULL);
    assert(ctx->layouts.count > 0);
    WLX_Rect rect = wlx_get_slot_rect(ctx, &ctx->layouts.items[ctx->layouts.count - 1], pos, span);
    return wlx_rect_inset(rect, padding);
}

static inline WLX_Rect wlx_resolve_widget_rect(WLX_Rect cell_rect, int16_t width, int16_t height,
    int16_t min_w, int16_t min_h, int16_t max_w, int16_t max_h,
    WLX_Align align, bool overflow) {
    float w = (width > 0)  ? (float)width  : cell_rect.w;
    float h = (height > 0) ? (float)height : cell_rect.h;

    // Clamp to widget-level min/max constraints (0 = unconstrained)
    if (min_w > 0 && w < min_w) w = (float)min_w;
    if (max_w > 0 && w > max_w) w = (float)max_w;
    if (min_h > 0 && h < min_h) h = (float)min_h;
    if (max_h > 0 && h > max_h) h = (float)max_h;

    if (overflow) {
        return (WLX_Rect){
            .x = cell_rect.x,
            .y = cell_rect.y,
            .w = w,
            .h = h,
        };
    }

    WLX_Rect r = wlx_get_align_rect(cell_rect, (int)w, (int)h, align);
    // wlx_get_align_rect won't grow beyond parent — enforce min constraints
    if (min_w > 0 && r.w < w) r.w = w;
    if (min_h > 0 && r.h < h) r.h = h;
    return r;
}

// Widget prologue helper — consolidates the 3-step cell→scroll→resolve
// sequence shared by every widget implementation.
typedef struct {
    WLX_Rect cell;   // the raw layout cell (before alignment/sizing)
    WLX_Rect rect;   // the resolved widget rect (aligned/sized within cell)
} WLX_Widget_Rect;

static inline WLX_Widget_Rect wlx_widget_begin(WLX_Context *ctx, int pos,
    size_t span, float padding, int16_t width, int16_t height,
    int16_t min_w, int16_t min_h, int16_t max_w, int16_t max_h,
    WLX_Align align, bool overflow)
{
    WLX_Rect cell = wlx_get_widget_cell_rect(ctx, pos, span, padding);
    // Track content height on the parent layout. `wlx_layout_end()` contributes
    // the accumulated total to auto_scroll_total_height, so widgets never
    // need to know about scroll panels.
    if (ctx->layouts.count > 0) {
        ctx->layouts.items[ctx->layouts.count - 1].accumulated_content_height +=
            (height > 0) ? (float)height : cell.h;
    }
    WLX_Rect rect = wlx_resolve_widget_rect(cell, width, height, min_w, min_h, max_w, max_h, align, overflow);
    return (WLX_Widget_Rect){ .cell = cell, .rect = rect };
}


static size_t wlx_hash_id(const char *file, int line) {
    size_t hash = 5389;
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
// to the first element. The buffer grows as needed but is never freed —
// it is simply reset (count = 0) each frame in wlx_begin().
static inline float *wlx_scratch_alloc(WLX_Context *ctx, size_t n) {
    assert(ctx != NULL);
    assert(n > 0 && n <= WLX_MAX_SLOT_COUNT && "unreasonable scratch alloc — did you pass a negative count?");
    assert(ctx->slot_size_offsets.count + n > ctx->slot_size_offsets.count && "size_t overflow in scratch alloc");
    wlx_da_reserve(&ctx->slot_size_offsets, ctx->slot_size_offsets.count + n);
    float *ptr = &ctx->slot_size_offsets.items[ctx->slot_size_offsets.count];
    ctx->slot_size_offsets.count += n;
    return ptr;
}

// Core offset math — fills offsets[0..count] given a total extent and
// optional per-slot sizes.  Used by both 1D layouts (single axis) and
// grid layouts (row axis + column axis).
static inline void wlx_compute_offsets(float *offsets, size_t count,
                                       float total, const WLX_Slot_Size *sizes)
{
    assert(offsets != NULL);
    assert(count > 0 && count <= WLX_MAX_SLOT_COUNT && "unreasonable slot count");

    if (sizes == NULL) {
        // Equal division
        for (size_t i = 0; i <= count; i++) {
            offsets[i] = ((float)i * total) / (float)count;
        }
        return;
    }

    // Two-pass: measure fixed/percent, accumulate flex/auto weights
    float used = 0.0f;
    float total_weight = 0.0f;
    for (size_t i = 0; i < count; i++) {
        switch (sizes[i].kind) {
            case WLX_SIZE_AUTO:    total_weight += 1.0f; break;
            case WLX_SIZE_FLEX:    total_weight += sizes[i].value; break;
            case WLX_SIZE_PIXELS:  used += sizes[i].value; break;
            case WLX_SIZE_PERCENT: used += (sizes[i].value * total) / 100.0f; break;
            default: WLX_UNREACHABLE("Undefined slot size kind");
        }
    }
    float remaining = (total - used > 0.0f) ? (total - used) : 0.0f;

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
            case WLX_SIZE_PERCENT: slot_size = (sizes[i].value * total) / 100.0f; break;
            default: WLX_UNREACHABLE("Undefined slot size kind"); slot_size = 0.0f;
        }
        // Clamp to min/max constraints (0 = unconstrained)
        if (sizes[i].min > 0 && slot_size < sizes[i].min) slot_size = sizes[i].min;
        if (sizes[i].max > 0 && slot_size > sizes[i].max) slot_size = sizes[i].max;
        offset += slot_size;
    }
    offsets[count] = offset;

#ifdef WLX_SLOT_MINMAX_REDISTRIBUTE
    // --- Pass 3: iterative freeze-and-redistribute ---
    // Ensures offsets[count] == total by redistributing surplus/deficit
    // from clamped slots to unfrozen neighbors.
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
            bool frozen_buf[64];
            float raw_buf[64];
            bool *frozen = (count <= 64) ? frozen_buf : (bool *)wlx_alloc(count * sizeof(bool));
            float *raw    = (count <= 64) ? raw_buf   : (float *)wlx_alloc(count * sizeof(float));

            for (size_t i = 0; i < count; i++) {
                frozen[i] = false;
                raw[i] = offsets[i + 1] - offsets[i];
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
                            default: break;
                        }
                    }
                }

                float unfrozen_remaining = total - frozen_total;
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

            // Rebuild offsets from raw sizes
            offset = 0.0f;
            for (size_t i = 0; i < count; i++) {
                offsets[i] = offset;
                offset += raw[i];
            }
            offsets[count] = offset;

            if (count > 64) {
                wlx_free(frozen);
                wlx_free(raw);
            }
        }
    }
#endif // WLX_SLOT_MINMAX_REDISTRIBUTE
}

static inline void wlx_compute_slot_offsets(WLX_Layout *l, const WLX_Slot_Size *sizes) {
    assert(l != NULL);
    assert(l->count > 0);
    assert(l->linear.offsets != NULL);
    float total = (l->linear.orient == WLX_HORZ) ? l->rect.w : l->rect.h;
    wlx_compute_offsets(l->linear.offsets, l->count, total, sizes);
}

// Create a layout and allocate its offsets from the context slot sizes buffer.
// The offsets are filled with equal-division values by default.
WLXDEF WLX_Layout wlx_create_layout(WLX_Context *ctx, WLX_Rect r, size_t count, WLX_Orient orient) {
    assert(ctx != NULL);
    assert(count > 0 && count <= WLX_MAX_SLOT_COUNT
           && "slot count is 0 or absurdly large — did you pass a negative int?");

    WLX_Layout l = {
        .kind = WLX_LAYOUT_LINEAR,
        .rect = r,
        .index = 0,
        .count = count,
        .linear = {
            .orient = orient,
            .offsets = wlx_scratch_alloc(ctx, count + 1),
        },
    };

    wlx_compute_slot_offsets(&l, NULL);

    return l;
}

// Create a dynamic (auto-sizing) layout.  The slot sizes buffer holds only the
// initial sentinel offset[0] = 0.  Each child widget appends one boundary via
// wlx_get_slot_rect() so no upfront count is required.
//
// slot_px  — fixed pixel size per slot along the layout axis. (Must be >= 0)
WLXDEF WLX_Layout wlx_create_layout_auto(WLX_Context *ctx, WLX_Rect r, WLX_Orient orient, float slot_px) {
    assert(ctx != NULL);
    assert(slot_px >= 0.0f && "slot_px must be non-negative (0 = variable-size mode)");

    size_t base = ctx->slot_size_offsets.count;
    wlx_da_reserve(&ctx->slot_size_offsets, base + 1);
    ctx->slot_size_offsets.items[base] = 0.0f;
    ctx->slot_size_offsets.count = base + 1;

    WLX_Layout l = {
        .kind         = WLX_LAYOUT_LINEAR,
        .rect         = r,
        .index        = 0,
        .count        = 0,
        .linear = {
            .orient       = orient,
            .offsets      = &ctx->slot_size_offsets.items[base],
            .dynamic      = true,
            .slot_size    = slot_px,
            .slot_size_offsets_base = base,
        },
    };

    return l;
}

WLXDEF WLX_Layout wlx_create_grid(WLX_Context *ctx, WLX_Rect r,
    size_t rows, size_t cols, const WLX_Slot_Size *row_sizes, const WLX_Slot_Size *col_sizes)
{
    assert(ctx != NULL);
    assert(rows > 0 && rows <= WLX_MAX_SLOT_COUNT && "Grid must have at least 1 row (and not absurdly many)");
    assert(cols > 0 && cols <= WLX_MAX_SLOT_COUNT && "Grid must have at least 1 column (and not absurdly many)");

    float *row_off = wlx_scratch_alloc(ctx, rows + 1);
    float *col_off = wlx_scratch_alloc(ctx, cols + 1);

    wlx_compute_offsets(row_off, rows, r.h, row_sizes);
    wlx_compute_offsets(col_off, cols, r.w, col_sizes);

    return (WLX_Layout){
        .kind        = WLX_LAYOUT_GRID,
        .rect        = r,
        .count       = rows * cols,
        .index       = 0,
        .grid = {
            .rows        = rows,
            .cols        = cols,
            .row_offsets  = row_off,
            .col_offsets  = col_off,
        },
    };
}

WLXDEF WLX_Layout wlx_create_grid_auto(WLX_Context *ctx, WLX_Rect r,
    size_t cols, float row_px, const WLX_Slot_Size *col_sizes)
{
    assert(ctx != NULL);
    assert(cols > 0 && cols <= WLX_MAX_SLOT_COUNT && "Grid must have at least 1 column (and not absurdly many)");
    assert(row_px > 0.0f && "Dynamic grid requires row_px > 0");

    // Column offsets: fixed, computed immediately
    size_t col_base = ctx->slot_size_offsets.count;
    float *col_off = wlx_scratch_alloc(ctx, cols + 1);
    wlx_compute_offsets(col_off, cols, r.w, col_sizes);

    // Row offsets: dynamic, starts with sentinel only
    size_t row_base = ctx->slot_size_offsets.count;
    wlx_da_reserve(&ctx->slot_size_offsets, row_base + 1);
    ctx->slot_size_offsets.items[row_base] = 0.0f;
    ctx->slot_size_offsets.count = row_base + 1;

    return (WLX_Layout){
        .kind  = WLX_LAYOUT_GRID,
        .rect  = r,
        .count = 0,
        .index = 0,
        .grid  = {
            .rows             = 0,
            .cols             = cols,
            .row_offsets      = &ctx->slot_size_offsets.items[row_base],
            .col_offsets      = col_off,
            .dynamic          = true,
            .row_size         = row_px,
            .row_offsets_base = row_base,
            .col_offsets_base = col_base,
        },
    };
}

static inline WLX_Rect wlx_calc_grid_slot_rect(const WLX_Layout *l,
    size_t row, size_t col, size_t row_span, size_t col_span)
{
    assert(l != NULL);
    assert(l->kind == WLX_LAYOUT_GRID);
    assert(row + row_span <= l->grid.rows);
    assert(col + col_span <= l->grid.cols);

    return (WLX_Rect){
        .x = l->rect.x + l->grid.col_offsets[col],
        .y = l->rect.y + l->grid.row_offsets[row],
        .w = l->grid.col_offsets[col + col_span] - l->grid.col_offsets[col],
        .h = l->grid.row_offsets[row + row_span] - l->grid.row_offsets[row],
    };
}

static inline WLX_Rect wlx_calc_layout_slot_rect(const WLX_Layout *l, size_t cell_index, size_t span) {
    assert(l != NULL);
    assert(l->count > 0);
    assert(cell_index < l->count);
    assert((cell_index + span) <= l->count);

    WLX_Rect slot_rect = {0};

    switch (l->linear.orient) {
        case WLX_HORZ: {
            slot_rect.x = l->rect.x + l->linear.offsets[cell_index];
            slot_rect.y = l->rect.y;
            slot_rect.w = l->linear.offsets[cell_index + span] - l->linear.offsets[cell_index];
            slot_rect.h = l->rect.h;
            break;
        }
        case WLX_VERT: {
            slot_rect.x = l->rect.x;
            slot_rect.y = l->rect.y + l->linear.offsets[cell_index];
            slot_rect.w = l->rect.w;
            slot_rect.h = l->linear.offsets[cell_index + span] - l->linear.offsets[cell_index];
            break;
        }
        default:
            WLX_UNREACHABLE("Undefined orientation");
    }

    return slot_rect;
}

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
            cspan = span;  // 1D span → col_span for grid auto-advance
        }

        // Dynamic grid: grow rows on demand when cursor/explicit placement
        // reaches beyond the current row count.
        if (l->grid.dynamic && (row + rspan) > l->grid.rows) {
            size_t rows_needed = (row + rspan) - l->grid.rows;
            size_t new_total = l->grid.rows + rows_needed;

            // Ensure scratch buffer has room for the new row boundaries
            size_t needed = l->grid.row_offsets_base + new_total + 1;
            wlx_da_reserve(&ctx->slot_size_offsets, needed);
            // Re-derive both pointers (buffer may have moved)
            l->grid.row_offsets = &ctx->slot_size_offsets.items[l->grid.row_offsets_base];
            l->grid.col_offsets = &ctx->slot_size_offsets.items[l->grid.col_offsets_base];

            for (size_t r = l->grid.rows; r < new_total; r++) {
                // Use per-row override if set, otherwise fall back to row_size.
                float effective = (l->grid.next_row_size > 0.0f)
                    ? l->grid.next_row_size : l->grid.row_size;
                l->grid.row_offsets[r + 1] = l->grid.row_offsets[r] + effective;
                // Consume per-row override after the first new row uses it.
                l->grid.next_row_size = 0.0f;
            }
            ctx->slot_size_offsets.count = needed;
            l->grid.rows = new_total;
            l->count = l->grid.rows * l->grid.cols;
        }

        assert(row + rspan <= l->grid.rows && "Grid row + row_span out of bounds");
        assert(col + cspan <= l->grid.cols && "Grid col + col_span out of bounds");

        WLX_Rect slot_rect = wlx_calc_grid_slot_rect(l, row, col, rspan, cspan);

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
        // Dynamic (auto-sizing) layout: append `span` new slot boundaries to the
        // slot sizes buffer before computing the slot rect.
        //
        // Limitation: the dynamic layout's offsets occupy a contiguous
        // region starting at slot_size_offsets_base. Nested `wlx_layout_begin()` /
        // `wlx_layout_end()` blocks
        // interleave additional scratch allocations after the first child slot,
        // which would corrupt this region.  Assert that scratch.count still sits
        // exactly at the end of this layout's region so we catch violations early.
        assert(pos < 0 && "Positional access (pos >= 0) is not supported on dynamic layouts");
        assert(ctx->slot_size_offsets.count == l->linear.slot_size_offsets_base + l->count + 1 &&
               "Dynamic layout does not support nested wlx_layout_begin blocks inside its body");

        // Use per-item override if set, otherwise fall back to slot_size.
        float effective = (l->linear.next_slot_size > 0.0f) ? l->linear.next_slot_size : l->linear.slot_size;
        assert(effective > 0.0f &&
               "Dynamic layout: slot size is 0 — call wlx_layout_auto_slot_px() before each child "
               "when using variable-size mode (slot_px = 0)");
        // Consume the per-item override so the next child reverts to slot_size.
        l->linear.next_slot_size = 0.0f;

        size_t needed = l->linear.slot_size_offsets_base + l->count + span + 1;
        wlx_da_reserve(&ctx->slot_size_offsets, needed);
        // Re-derive offsets pointer in case wlx_da_reserve moved the buffer.
        l->linear.offsets = &ctx->slot_size_offsets.items[l->linear.slot_size_offsets_base];
        // Append span boundaries using the effective slot size.
        for (size_t s = 0; s < span; s++) {
            l->linear.offsets[l->count + 1 + s] = l->linear.offsets[l->count + s] + effective;
        }
        ctx->slot_size_offsets.count = needed;
        l->count += span;
        // Fall through to the standard rect calculation below.
    } else {
        assert(pos < (int)l->count);
    }

    size_t cell_index = (pos < 0) ? l->index : (size_t)pos;
    WLX_Rect slot_rect = wlx_calc_layout_slot_rect(l, cell_index, span);

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

WLXDEF void wlx_begin(WLX_Context *ctx, WLX_Rect r, WLX_Input_Handler input_handler) {
    wlx_assert_backend_ready(ctx);
    assert(input_handler != NULL && "WLX input handler must not be NULL");
    input_handler(ctx);
    ctx->interaction.hot_id = 0;
    ctx->interaction.active_id_seen = false;
    ctx->id_stack.count = 0;  // reset ID stack each frame
#ifdef WLX_DEBUG
    ctx->debug_site_hits.used = 0;  // reset per-frame site tracking
    for (size_t _dsi = 0; _dsi < 512; _dsi++) ctx->debug_site_hits.slots[_dsi].key = 0;
#endif
    ctx->slot_size_offsets.count = 0;  // reset slot sizes buffer each frame
    ctx->rect = r;
    if (ctx->theme == NULL) ctx->theme = &wlx_theme_dark;
}

WLXDEF void wlx_end(WLX_Context *ctx) {
    // Clear stale active_id: if no widget matched active_id this frame, the
    // active widget has disappeared (e.g. its ID shifted due to variable
    // widget counts).  Without this cleanup, active_id stays set forever,
    // blocking all hover and click interactions.
    if (ctx->interaction.active_id != 0 && !ctx->interaction.active_id_seen) {
        ctx->interaction.active_id = 0;
    }
}

WLXDEF void wlx_context_destroy(WLX_Context *ctx) {
    // Free state map data entries
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        if (ctx->states.slots[i].id != 0) {
            wlx_free(ctx->states.slots[i].data);
        }
    }
    // Free dynamic array backing buffers
    wlx_free(ctx->layouts.items);
    wlx_free(ctx->states.slots);
    wlx_free(ctx->id_stack.items);
    wlx_free(ctx->scroll_panels.items);
    wlx_free(ctx->slot_size_offsets.items);
    // Zero out the context so it's safe to reuse or free
    wlx_zero_struct(*ctx);
}

WLXDEF void wlx_layout_begin_impl(WLX_Context *ctx, size_t count, WLX_Orient orient, WLX_Layout_Opt opt) {
    WLX_Rect r = {0};
    if (ctx->layouts.count <= 0) {
        r = ctx->rect;
    }
    else {
        r = wlx_get_slot_rect(ctx, &ctx->layouts.items[ctx->layouts.count - 1], opt.pos, opt.span);
    }

    r = wlx_rect_inset(r, opt.padding);

    WLX_Layout l = wlx_create_layout(ctx, r, count, orient);
    if (opt.sizes != NULL) {
        wlx_compute_slot_offsets(&l, opt.sizes);
    }
    wlx_da_append(&ctx->layouts, l);
}

// wlx_layout_begin_auto_impl — dynamic variant of wlx_layout_begin_impl.
// The layout's slot count starts at 0 and grows by one for each direct child
// widget or nested layout_begin call.  slot_px controls the fixed pixel size
// of every slot along the layout axis.
WLXDEF void wlx_layout_begin_auto_impl(WLX_Context *ctx, WLX_Orient orient, float slot_px, WLX_Layout_Opt opt) {
    WLX_Rect r = {0};
    if (ctx->layouts.count <= 0) {
        r = ctx->rect;
    } else {
        r = wlx_get_slot_rect(ctx, &ctx->layouts.items[ctx->layouts.count - 1], opt.pos, opt.span);
    }

    r = wlx_rect_inset(r, opt.padding);

    WLX_Layout l = wlx_create_layout_auto(ctx, r, orient, slot_px);
    wlx_da_append(&ctx->layouts, l);
}

WLXDEF void wlx_grid_begin_impl(WLX_Context *ctx, size_t rows, size_t cols, WLX_Grid_Opt opt) {
    WLX_Rect r = {0};
    if (ctx->layouts.count <= 0) {
        r = ctx->rect;
    } else {
        r = wlx_get_slot_rect(
            ctx, &ctx->layouts.items[ctx->layouts.count - 1],
            opt.pos, opt.span);
    }

    r = wlx_rect_inset(r, opt.padding);

    WLX_Layout l = wlx_create_grid(ctx, r, rows, cols,
                                         opt.row_sizes, opt.col_sizes);
    wlx_da_append(&ctx->layouts, l);
}

WLXDEF void wlx_grid_begin_auto_impl(WLX_Context *ctx, size_t cols, float row_px, WLX_Grid_Auto_Opt opt) {
    WLX_Rect r = {0};
    if (ctx->layouts.count <= 0) {
        r = ctx->rect;
    } else {
        r = wlx_get_slot_rect(
            ctx, &ctx->layouts.items[ctx->layouts.count - 1],
            opt.pos, opt.span);
    }

    r = wlx_rect_inset(r, opt.padding);

    WLX_Layout l = wlx_create_grid_auto(ctx, r, cols, row_px, opt.col_sizes);
    wlx_da_append(&ctx->layouts, l);
}

WLXDEF void wlx_grid_begin_auto_tile_impl(WLX_Context *ctx, float tile_w, float tile_h, WLX_Grid_Auto_Opt opt) {
    assert(tile_w > 0.0f && "tile width must be positive");
    assert(tile_h > 0.0f && "tile height must be positive");

    WLX_Rect r = {0};
    if (ctx->layouts.count <= 0) {
        r = ctx->rect;
    } else {
        r = wlx_get_slot_rect(
            ctx, &ctx->layouts.items[ctx->layouts.count - 1],
            opt.pos, opt.span);
    }

    r = wlx_rect_inset(r, opt.padding);

    size_t cols = (size_t)(r.w / tile_w);
    if (cols < 1) cols = 1;

    WLX_Layout l = wlx_create_grid_auto(ctx, r, cols, tile_h, opt.col_sizes);
    wlx_da_append(&ctx->layouts, l);
}

WLXDEF void wlx_layout_end(WLX_Context *ctx) {
    assert(ctx != NULL);
    assert(ctx->layouts.count > 0);
    WLX_Layout *l = &ctx->layouts.items[ctx->layouts.count - 1];
    // Contribute this layout's accumulated content height to auto-scroll tracking.
    // Each child widget tracked its preferred height on the layout during the frame.
    // This replaces the previous per-widget and per-dynamic-layout special cases.
    if (ctx->auto_scroll.panel_id) {
        // For dynamic grids, the authoritative content height is the total
        // row offset — override the per-widget accumulation.
        if (l->kind == WLX_LAYOUT_GRID && l->grid.dynamic && l->grid.rows > 0) {
            l->accumulated_content_height = l->grid.row_offsets[l->grid.rows];
        }
        ctx->auto_scroll.total_height += l->accumulated_content_height;
    }
    ctx->layouts.count -= 1;
}

// Declare the pixel size for the next slot in the innermost dynamic layout.
// Must be called immediately before the widget or nested `wlx_layout_begin()` that will
// consume the slot.  Only valid when the enclosing layout was created with
// `wlx_layout_begin_auto()`.  The override is automatically consumed after one slot.
WLXDEF void wlx_layout_auto_slot_px(WLX_Context *ctx, float px) {
    assert(ctx != NULL);
    assert(ctx->layouts.count > 0 && "wlx_layout_auto_slot_px called outside a layout");
    assert(px > 0.0f && "wlx_layout_auto_slot_px requires a positive pixel size");

    WLX_Layout *l = &ctx->layouts.items[ctx->layouts.count - 1];
    assert(l->linear.dynamic && "wlx_layout_auto_slot_px is only valid inside a wlx_layout_begin_auto block");

    l->linear.next_slot_size = px;
}

WLXDEF void wlx_grid_auto_row_px(WLX_Context *ctx, float px) {
    assert(ctx != NULL);
    assert(ctx->layouts.count > 0 && "wlx_grid_auto_row_px called outside a layout");
    assert(px > 0.0f && "wlx_grid_auto_row_px requires a positive pixel size");

    WLX_Layout *l = &ctx->layouts.items[ctx->layouts.count - 1];
    assert(l->kind == WLX_LAYOUT_GRID && l->grid.dynamic &&
           "wlx_grid_auto_row_px is only valid inside a wlx_grid_begin_auto block");

    l->grid.next_row_size = px;
}

WLXDEF void wlx_grid_cell_impl(WLX_Context *ctx, int row, int col, WLX_Grid_Cell_Opt opt) {
    assert(ctx != NULL);
    assert(ctx->layouts.count > 0 && "grid_cell() called outside a layout");

    WLX_Layout *l = &ctx->layouts.items[ctx->layouts.count - 1];
    assert(l->kind == WLX_LAYOUT_GRID && "grid_cell() is only valid inside a grid_begin block");

    // -1 means "use cursor position for that axis"
    size_t r = (row < 0) ? l->grid.cursor_row : (size_t)row;
    size_t c = (col < 0) ? l->grid.cursor_col : (size_t)col;

    assert(r + opt.row_span <= l->grid.rows && "grid_cell row + row_span out of bounds");
    assert(c + opt.col_span <= l->grid.cols && "grid_cell col + col_span out of bounds");

    l->grid.cell_set      = true;
    l->grid.next_row      = r;
    l->grid.next_col      = c;
    l->grid.next_row_span = opt.row_span;
    l->grid.next_col_span = opt.col_span;
}

WLXDEF WLX_Rect wlx_get_align_rect(WLX_Rect parent_rect, int width, int height, WLX_Align align) {
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
        // align left (or no alignment) — r.x already equals parent_rect.x
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
        // align top (or no alignment) — r.y already equals parent_rect.y
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
    for (size_t i = 0; i < ctx->id_stack.count; i++) {
        h = h * 2654435761u ^ ctx->id_stack.items[i];
    }
    return h;
}

WLXDEF void wlx_push_id(WLX_Context *ctx, size_t id) {
    wlx_da_append(&ctx->id_stack, id);
}

WLXDEF void wlx_pop_id(WLX_Context *ctx) {
    assert(ctx->id_stack.count > 0);
    ctx->id_stack.count--;
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
    size_t base = wlx_hash_id(file, line) ^ wlx_id_stack_hash(ctx);

#ifdef WLX_DEBUG
    // Track per-call-site hits to detect duplicate sites without wlx_push_id.
    // 512 slots with linear probing — sufficient for any realistic UI frame.
    // If the table exceeds 75% load, stop tracking (degrades gracefully).
    if (ctx->debug_site_hits.used < 384) {  // 75% of 512
        size_t dslot = base & 511u;
        // Linear probe
        for (size_t _dp = 0; _dp < 512; _dp++) {
            size_t idx = (dslot + _dp) & 511u;
            if (ctx->debug_site_hits.slots[idx].key == 0) {
                // Empty slot — first hit for this site
                ctx->debug_site_hits.slots[idx].key = base;
                ctx->debug_site_hits.slots[idx].file = file;
                ctx->debug_site_hits.slots[idx].line = line;
                ctx->debug_site_hits.slots[idx].warned = false;
                ctx->debug_site_hits.used++;
                break;
            } else if (ctx->debug_site_hits.slots[idx].key == base
                       && ctx->debug_site_hits.slots[idx].line == line
                       && ctx->debug_site_hits.slots[idx].file == file) {
                // Duplicate hit — same (file, line, id_stack) without push_id
                if (!ctx->debug_site_hits.slots[idx].warned) {
                    ctx->debug_site_hits.slots[idx].warned = true;
                    if (ctx->debug_warn_cb) {
                        ctx->debug_warn_cb(file, line, 2, ctx->debug_warn_user_data);
                    } else {
                        fprintf(stderr,
                            "wollix [DEBUG]: widget at %s:%d hit multiple times "
                            "without wlx_push_id()\n", file, line);
                    }
                }
                break;
            }
        }
    }
#endif

    return (base == 0) ? 1 : base;  // reserve 0 for "no widget"
}

static inline bool wlx_interaction_mouse_over(WLX_Context *ctx, WLX_Rect rect) {
    return wlx_point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y, rect.x, rect.y, rect.w, rect.h);
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

// Find slot for `id` — returns pointer to the slot (empty or matching)
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
// Uses wlx_hash_id(file, line) combined with the ID stack — no sequential counter.
// This makes state IDs fully stable across frames. The ID stack (wlx_push_id/
// wlx_pop_id) provides disambiguation when the same source line is hit multiple
// times (e.g. widgets generated in a loop). This differs from interaction IDs,
// which add a frame-local sequence so repeated interaction queries in one frame
// do not collide.
WLXDEF WLX_State wlx_get_state_impl(WLX_Context *ctx, size_t state_size, const char *file, int line) {
    size_t id = wlx_hash_id(file, line) ^ wlx_id_stack_hash(ctx);
    if (id == 0) id = 1;

    WLX_State_Map_Slot *slot = wlx_state_map_get(&ctx->states, id, state_size);
    assert(slot->data_size == state_size);
    return (WLX_State){ .id = id, .data = slot->data };
}

WLXDEF void wlx_widget_impl(WLX_Context *ctx, WLX_Widget_Opt opt, const char *file, int line)
{
    if (opt.id) wlx_push_id(ctx, wlx_hash_string(opt.id));
    const WLX_Theme *theme = ctx->theme;
    WLX_Widget_Rect wg = wlx_widget_begin(ctx, opt.pos, opt.span, opt.padding,
        opt.width, opt.height, opt.min_width, opt.min_height,
        opt.max_width, opt.max_height, opt.widget_align, opt.overflow);
    WLX_Rect wr = wg.rect;

    WLX_Interaction state = wlx_get_interaction(
        ctx,
        wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        file, line
    );

    WLX_Color c = opt.color;
    if (state.hover) {
        c = wlx_color_brightness(c, theme->hover_brightness);
    }

    WLX_Rect rect = {
        .x = ceilf(wr.x),
        .y = ceilf(wr.y),
        .w = ceilf(wr.w),
        .h = ceilf(wr.h),
    };

    wlx_draw_rect(ctx, rect, c);
}

// ============================================================================
// Implementation: text layout and rendering helpers
// ============================================================================
// Text helpers in this section are implementation-owned helpers and use the
// canonical `wlx_` prefix.

// This function returns how many bytes the next UTF-8 character uses, based on the first byte in s.
// UTF-8 leading-byte patterns:
//       0xxxxxxx → 1 byte (ASCII)
//       Checked by (c & 0x80) == 0x00
//       110xxxxx → 2 bytes
//       Checked by (c & 0xE0) == 0xC0
//       1110xxxx → 3 bytes
//       Checked by (c & 0xF0) == 0xE0
//       11110xxx → 4 bytes
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
// Returns number of bytes consumed (1–4).
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
// Returns number of bytes written (1–4).  Invalid codepoints write U+FFFD.
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
    // Invalid codepoint → encode U+FFFD
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

static bool wlx_layout_text_run(WLX_Context *ctx, WLX_Rect rect, const char *text, WLX_Text_Style style, WLX_Align align, bool wrap,
    bool draw, float *out_end_x, float *out_end_y) {

    if (style.font_size <= 0) return false;

    float text_w = 0;
    float text_h = 0;
    const char *measure_text = (text != NULL && strlen(text) > 0) ? text : " ";
    wlx_measure_text(ctx, measure_text, style, &text_w, &text_h);
    if (text_h > rect.h) return false;

    WLX_Rect tr = wlx_get_align_rect(rect, text_w, text_h, align);

    float glyph_x = tr.x;
    float glyph_y = tr.y;

    if (text == NULL) text = "";
    size_t length = strlen(text);

    for (size_t i = 0; i < length;) {
        size_t char_len = wlx_utf8_char_len(&text[i]);
        if (i + char_len > length) char_len = 1;

        char glyph[8] = {0};
        memcpy(glyph, &text[i], char_len);
        glyph[char_len] = '\0';

        float glyph_w = 0;
        float glyph_h = 0;
        wlx_measure_text(ctx, glyph, style, &glyph_w, &glyph_h);
        if (glyph_h <= 0) glyph_h = (float)style.font_size;

        if (glyph_x + glyph_w > rect.x + rect.w) {
            if (wrap) {
                glyph_y += glyph_h;
                if (glyph_y > rect.y + rect.h - glyph_h) break;
                glyph_x = tr.x;
            } else {
                break;
            }
        }

        if (draw) {
            WLX_Text_Style glyph_style = style;
            glyph_style.spacing = 0;
            wlx_draw_text(ctx, glyph, glyph_x, glyph_y, glyph_style);
        }

        if (i + char_len < length) glyph_w += style.spacing;
        glyph_x += glyph_w;
        i += char_len;
    }

    if (out_end_x != NULL) *out_end_x = glyph_x;
    if (out_end_y != NULL) *out_end_y = glyph_y;
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
static const float WLX_CHECKBOX_BORDER_THICKNESS = 1.0f;
static const float WLX_CHECKBOX_CHECK_PADDING_RATIO = 0.2f;
static const float WLX_CHECKBOX_CHECK_THICKNESS = 3.0f;

static const float WLX_INPUTBOX_BORDER_THICKNESS = 1.0f;
static const float WLX_INPUTBOX_CURSOR_WIDTH = 2.0f;
static const float WLX_INPUTBOX_CURSOR_PADDING = 2.0f;
static const float WLX_INPUTBOX_CURSOR_BLINK_PERIOD = 1.0f;
static const float WLX_INPUTBOX_CURSOR_VISIBLE_FRACTION = 0.5f;

static void wlx_resolve_opt_textbox(const WLX_Theme *theme, WLX_Textbox_Opt *opt) {
    if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->surface;
    if (opt->font == WLX_FONT_DEFAULT)      opt->font        = theme->font;
    if (opt->font_size <= 0)                opt->font_size   = theme->font_size;
    if (opt->spacing   <= 0)                opt->spacing     = theme->spacing;
    if (opt->opacity   <= 0.0f)             opt->opacity     = 1.0f;
    float theme_opacity = (theme->opacity <= 0.0f) ? 1.0f : theme->opacity;
    opt->opacity *= theme_opacity;
    opt->front_color = wlx_color_apply_opacity(opt->front_color, opt->opacity);
    opt->back_color  = wlx_color_apply_opacity(opt->back_color,  opt->opacity);
}

WLXDEF void wlx_textbox_impl(WLX_Context *ctx, const char *text, WLX_Textbox_Opt opt, const char *file, int line)
{
    if (opt.id) wlx_push_id(ctx, wlx_hash_string(opt.id));
    wlx_resolve_opt_textbox(ctx->theme, &opt);

    WLX_Widget_Rect wg = wlx_widget_begin(ctx, opt.pos, opt.span, opt.padding, opt.width, opt.height, opt.min_width, opt.min_height, opt.max_width, opt.max_height, opt.widget_align, opt.overflow);
    WLX_Rect wr = wg.rect;

    WLX_Interaction state = wlx_get_interaction(
        ctx,
        wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        file, line
    );

    if (opt.boxed) {
        WLX_Rect rect = { wr.x, wr.y, wr.w, wr.h };

        WLX_Color bg = (state.hover) ? wlx_color_brightness(opt.back_color, ctx->theme->hover_brightness) : opt.back_color;
        wlx_draw_rect(ctx, rect, bg);
    }

    wlx_draw_text_fitted(ctx, wr, text, (WLX_Text_Style){ .font = opt.font, .font_size = opt.font_size, .spacing = opt.spacing, .color = opt.front_color }, opt.align, opt.wrap);
    if (opt.id) wlx_pop_id(ctx);
}

static void wlx_resolve_opt_button(const WLX_Theme *theme, WLX_Button_Opt *opt) {
    if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->surface;
    if (opt->font == WLX_FONT_DEFAULT)      opt->font        = theme->font;
    if (opt->font_size <= 0)                opt->font_size   = theme->font_size;
    if (opt->spacing   <= 0)                opt->spacing     = theme->spacing;
    if (opt->opacity   <= 0.0f)             opt->opacity     = 1.0f;
    float theme_opacity = (theme->opacity <= 0.0f) ? 1.0f : theme->opacity;
    opt->opacity *= theme_opacity;
    opt->front_color = wlx_color_apply_opacity(opt->front_color, opt->opacity);
    opt->back_color  = wlx_color_apply_opacity(opt->back_color,  opt->opacity);
}

WLXDEF bool wlx_button_impl(WLX_Context *ctx, const char *text, WLX_Button_Opt opt, const char *file, int line)
{
    if (opt.id) wlx_push_id(ctx, wlx_hash_string(opt.id));
    wlx_resolve_opt_button(ctx->theme, &opt);

    WLX_Widget_Rect wg = wlx_widget_begin(ctx, opt.pos, opt.span, opt.padding, opt.width, opt.height, opt.min_width, opt.min_height, opt.max_width, opt.max_height, opt.widget_align, opt.overflow);
    WLX_Rect wr = wg.rect;

    WLX_Interaction state = wlx_get_interaction(
        ctx,
        wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        file, line
    );


    WLX_Color bg = (state.hover) ? wlx_color_brightness(opt.back_color, ctx->theme->hover_brightness) : opt.back_color;

    WLX_Rect rect = { wr.x, wr.y, wr.w, wr.h };
    wlx_draw_rect(ctx, rect, bg);

    wlx_draw_text_fitted(ctx, wr, text, (WLX_Text_Style){ .font = opt.font, .font_size = opt.font_size, .spacing = opt.spacing, .color = opt.front_color }, opt.align, opt.wrap);

    if (opt.id) wlx_pop_id(ctx);
    return state.clicked;
}

static void wlx_resolve_opt_checkbox(const WLX_Theme *theme, WLX_Checkbox_Opt *opt) {
    if (wlx_color_is_zero(opt->front_color))  opt->front_color  = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))   opt->back_color   = theme->surface;
    if (wlx_color_is_zero(opt->border_color)) opt->border_color = theme->checkbox.border;
    if (wlx_color_is_zero(opt->check_color))  opt->check_color  = theme->checkbox.check;
    if (opt->font == WLX_FONT_DEFAULT)       opt->font         = theme->font;
    if (opt->font_size <= 0)                 opt->font_size    = theme->font_size;
    if (opt->spacing   <= 0)                 opt->spacing      = theme->spacing;
    if (opt->opacity   <= 0.0f)              opt->opacity      = 1.0f;
    float theme_opacity = (theme->opacity <= 0.0f) ? 1.0f : theme->opacity;
    opt->opacity *= theme_opacity;
    opt->front_color  = wlx_color_apply_opacity(opt->front_color,  opt->opacity);
    opt->back_color   = wlx_color_apply_opacity(opt->back_color,   opt->opacity);
    opt->border_color = wlx_color_apply_opacity(opt->border_color, opt->opacity);
    opt->check_color  = wlx_color_apply_opacity(opt->check_color,  opt->opacity);
}

WLXDEF bool wlx_checkbox_impl(WLX_Context *ctx, const char *text, bool *checked, WLX_Checkbox_Opt opt, const char *file, int line)
{
    if (opt.id) wlx_push_id(ctx, wlx_hash_string(opt.id));
    wlx_resolve_opt_checkbox(ctx->theme, &opt);

    WLX_Widget_Rect wg = wlx_widget_begin(ctx, opt.pos, opt.span, opt.padding, opt.width, opt.height, opt.min_width, opt.min_height, opt.max_width, opt.max_height, opt.widget_align, opt.overflow);
    WLX_Rect wr = wg.rect;

    // Draw checkbox
    float checkbox_size = (wr.h > opt.font_size) ? opt.font_size : wr.h * WLX_CHECKBOX_SIZE_RATIO;
    float text_w = 0;
    float text_h = 0;
    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size, .spacing = opt.spacing, .color = opt.front_color };
    wlx_measure_text(ctx, text, ts, &text_w, &text_h);
    float padding = opt.font_size * WLX_CHECKBOX_LABEL_PADDING_FACTOR;

    float height = text_h > checkbox_size ? text_h : checkbox_size;
    WLX_Rect acr = wlx_get_align_rect(wr, checkbox_size + padding + text_w, height, opt.align);

    WLX_Interaction state = wlx_get_interaction(
        ctx,
        (opt.boxed) ? wr : acr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        file, line
    );

    WLX_Rect checkbox_rect = { acr.x, acr.y, checkbox_size, checkbox_size };

    if (state.clicked && checked != NULL) {
        *checked = !(*checked);
    }

    WLX_Color checkbox_bg = state.hover ? wlx_color_brightness(opt.back_color, ctx->theme->hover_brightness) : opt.back_color;
    wlx_draw_rect(ctx, checkbox_rect, checkbox_bg);

    // Draw checkbox border
    wlx_draw_rect_lines(ctx, checkbox_rect, WLX_CHECKBOX_BORDER_THICKNESS, opt.border_color);

    // Draw check
    if (checked != NULL && *checked) {
        float check_padding = checkbox_size * WLX_CHECKBOX_CHECK_PADDING_RATIO;
        wlx_draw_line(
            ctx,
            checkbox_rect.x + check_padding, checkbox_rect.y + checkbox_size / 2,
            checkbox_rect.x + checkbox_size / 2, checkbox_rect.y + checkbox_size - check_padding,
            WLX_CHECKBOX_CHECK_THICKNESS, opt.check_color
        );
        wlx_draw_line(
            ctx,
            checkbox_rect.x + checkbox_size / 2, checkbox_rect.y + checkbox_size - check_padding,
            checkbox_rect.x + checkbox_size - check_padding, checkbox_rect.y + check_padding,
            WLX_CHECKBOX_CHECK_THICKNESS, opt.check_color
        );
    }

    WLX_Rect text_rect = {
      .x = acr.x + checkbox_size + padding,
      .y = acr.y,
      .w = acr.w - checkbox_size + padding,
      .h = wr.y + wr.h - acr.y
    };
    wlx_draw_text_fitted(ctx, text_rect, text, ts, WLX_ALIGN_NONE, opt.wrap);

    if (opt.id) wlx_pop_id(ctx);
    return state.clicked;
}

static void wlx_resolve_opt_checkbox_tex(const WLX_Theme *theme, WLX_Checkbox_Tex_Opt *opt) {
    if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->surface;
    if (opt->font == WLX_FONT_DEFAULT)      opt->font        = theme->font;
    if (opt->font_size <= 0) opt->font_size = theme->font_size;
    if (opt->spacing   <= 0) opt->spacing   = theme->spacing;
    if (opt->opacity   <= 0.0f) opt->opacity = 1.0f;
    float theme_opacity = (theme->opacity <= 0.0f) ? 1.0f : theme->opacity;
    opt->opacity *= theme_opacity;
    opt->front_color = wlx_color_apply_opacity(opt->front_color, opt->opacity);
    opt->back_color  = wlx_color_apply_opacity(opt->back_color,  opt->opacity);
}

WLXDEF bool wlx_checkbox_tex_impl(WLX_Context *ctx, const char *text, bool *checked, WLX_Checkbox_Tex_Opt opt, const char *file, int line)
{
    if (opt.id) wlx_push_id(ctx, wlx_hash_string(opt.id));
    wlx_resolve_opt_checkbox_tex(ctx->theme, &opt);

    WLX_Widget_Rect wg = wlx_widget_begin(ctx, opt.pos, opt.span, opt.padding, opt.width, opt.height, opt.min_width, opt.min_height, opt.max_width, opt.max_height, opt.widget_align, opt.overflow);
    WLX_Rect wr = wg.rect;

    // Draw checkbox
    float checkbox_size = (wr.h > opt.font_size) ? opt.font_size : wr.h * WLX_CHECKBOX_SIZE_RATIO;
    float text_w = 0;
    float text_h = 0;
    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size, .spacing = opt.spacing, .color = opt.front_color };
    wlx_measure_text(ctx, text, ts, &text_w, &text_h);

    float padding = opt.font_size * WLX_CHECKBOX_LABEL_PADDING_FACTOR;

    float height = text_h > checkbox_size ? text_h : checkbox_size;
    WLX_Rect acr = wlx_get_align_rect(wr, checkbox_size + padding + text_w, height, opt.align);

    WLX_Interaction state = wlx_get_interaction(
        ctx,
        (opt.boxed) ? wr : acr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        file, line
    );

    WLX_Rect checkbox_rect = { acr.x, acr.y, checkbox_size, checkbox_size };

    if (state.clicked && checked != NULL) {
        *checked = !(*checked);
    }

    // Draw the appropriate texture based on checked state
    WLX_Texture current_tex = (checked != NULL && *checked) ? opt.tex_checked : opt.tex_unchecked;

    // Apply hover brightness if hovered
    WLX_Color tint = state.hover ? wlx_color_brightness(opt.back_color, ctx->theme->hover_brightness) : opt.back_color;

    wlx_draw_texture(
        ctx,
        current_tex,
        (WLX_Rect){0, 0, (float)current_tex.width, (float)current_tex.height},
        (WLX_Rect){checkbox_rect.x, checkbox_rect.y, checkbox_rect.w, checkbox_rect.h},
        tint
    );

    WLX_Rect text_rect = {
      .x = acr.x + checkbox_size + padding,
      .y = acr.y,
      .w = acr.w - checkbox_size + padding,
      .h = wr.y + wr.h - acr.y
    };
    wlx_draw_text_fitted(ctx, text_rect, text, ts, WLX_ALIGN_NONE, opt.wrap);

    if (opt.id) wlx_pop_id(ctx);
    return state.clicked;
}

static void wlx_resolve_opt_inputbox(const WLX_Theme *theme, WLX_Inputbox_Opt *opt) {
    if (wlx_color_is_zero(opt->front_color))        opt->front_color        = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))         opt->back_color         = theme->surface;
    if (wlx_color_is_zero(opt->border_color))       opt->border_color       = theme->border;
    if (wlx_color_is_zero(opt->border_focus_color)) opt->border_focus_color = theme->input.border_focus;
    if (wlx_color_is_zero(opt->cursor_color))       opt->cursor_color       = theme->input.cursor;
    if (opt->font == WLX_FONT_DEFAULT)             opt->font               = theme->font;
    if (opt->font_size <= 0)                       opt->font_size          = theme->font_size;
    if (opt->spacing   <= 0)                       opt->spacing            = theme->spacing;
    if (opt->opacity   <= 0.0f)                    opt->opacity            = 1.0f;
    float theme_opacity = (theme->opacity <= 0.0f) ? 1.0f : theme->opacity;
    opt->opacity *= theme_opacity;
    opt->front_color        = wlx_color_apply_opacity(opt->front_color,        opt->opacity);
    opt->back_color         = wlx_color_apply_opacity(opt->back_color,         opt->opacity);
    opt->border_color       = wlx_color_apply_opacity(opt->border_color,       opt->opacity);
    opt->border_focus_color = wlx_color_apply_opacity(opt->border_focus_color, opt->opacity);
    opt->cursor_color       = wlx_color_apply_opacity(opt->cursor_color,       opt->opacity);
}

WLXDEF bool wlx_inputbox_impl(WLX_Context *ctx, const char *label, char *buffer, size_t buffer_size, WLX_Inputbox_Opt opt, const char *file, int line) {
    if (opt.id) wlx_push_id(ctx, wlx_hash_string(opt.id));
    assert(ctx != NULL);
    assert(buffer != NULL && "inputbox buffer must not be NULL");
    assert(buffer_size >= 2 && "buffer_size must hold at least 1 char + null terminator");
    wlx_resolve_opt_inputbox(ctx->theme, &opt);

    WLX_Widget_Rect wg = wlx_widget_begin(ctx, opt.pos, opt.span, opt.padding, opt.width, opt.height, opt.min_width, opt.min_height, opt.max_width, opt.max_height, opt.widget_align, opt.overflow);
    WLX_Rect wr = wg.rect;

    WLX_Interaction state = wlx_get_interaction(
        ctx,
        wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS,
        file, line
    );

    // Per-widget persistent cursor state
    WLX_Inputbox_State *istate = (WLX_Inputbox_State *)wlx_get_state_impl(ctx, sizeof(WLX_Inputbox_State), file, line).data;

    if (state.focused) {
        size_t current_len = strlen(buffer);

        // Initialize cursor position when first focused
        if (state.just_focused) {
            istate->cursor_pos = current_len;  // start at end
            istate->cursor_blink_time = 0.0f;
        }

        // Clamp cursor position to valid range
        if (istate->cursor_pos > current_len) {
            istate->cursor_pos = current_len;
        }

        // Handle left arrow - move cursor left (by one codepoint)
        if (wlx_is_key_pressed(ctx, WLX_KEY_LEFT) && istate->cursor_pos > 0) {
            istate->cursor_pos = wlx_utf8_prev(buffer, istate->cursor_pos);
        }

        // Handle right arrow - move cursor right (by one codepoint)
        if (wlx_is_key_pressed(ctx, WLX_KEY_RIGHT) && istate->cursor_pos < current_len) {
            istate->cursor_pos = wlx_utf8_next(buffer, istate->cursor_pos, current_len);
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
                memmove(&buffer[istate->cursor_pos + char_len],
                        &buffer[istate->cursor_pos],
                        current_len - istate->cursor_pos + 1); // +1 for NUL

                // Copy the codepoint bytes
                memcpy(&buffer[istate->cursor_pos], &ctx->input.text_input[i], char_len);
                istate->cursor_pos += char_len;
                current_len += char_len;

                i += char_len;
            }
        }

        // Handle backspace - delete the codepoint before cursor
        if (wlx_is_key_pressed(ctx, WLX_KEY_BACKSPACE) && istate->cursor_pos > 0) {
            size_t prev_pos = wlx_utf8_prev(buffer, istate->cursor_pos);
            // Shift remaining bytes (including NUL) left
            memmove(&buffer[prev_pos],
                    &buffer[istate->cursor_pos],
                    current_len - istate->cursor_pos + 1);
            istate->cursor_pos = prev_pos;
        }
    }

    float label_width = 0;
    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size, .spacing = opt.spacing, .color = opt.front_color };
    // Draw label if provided
    if (label != NULL && opt.font_size > 0) {
        float label_h = 0;
        wlx_measure_text(ctx, label, ts, &label_width, &label_h);
        label_width += opt.content_padding;

        float label_x = wr.x + opt.content_padding;
        float label_y = wr.y + (wr.h - label_h) / 2;

        WLX_Rect label_rect = {
            .x = label_x,
            .y = label_y,
            .w = label_width,
            .h = label_h,
        };
        wlx_draw_text_fitted(ctx, label_rect, label, ts, opt.align, opt.wrap);

    }

    // Input box rectangle
    float input_x = wr.x + label_width + opt.content_padding;
    float input_w = wr.w - label_width - opt.content_padding * 2;
    input_w = input_w < 0 ? 0 : input_w;

    float input_h = wr.h - opt.content_padding * 2;

    WLX_Rect input_rect = { input_x, wr.y + opt.content_padding, input_w, input_h };

    // Draw input box background
    WLX_Color bg_color = opt.back_color;
    if (state.hover && !state.focused) {
        bg_color = wlx_color_brightness(opt.back_color, ctx->theme->hover_brightness * 0.5f);
    }
    wlx_draw_rect(ctx, input_rect, bg_color);

    // Draw border
    WLX_Color bdr_color = state.focused ? opt.border_focus_color : opt.border_color;
        wlx_draw_rect_lines(ctx, input_rect, WLX_INPUTBOX_BORDER_THICKNESS, bdr_color);

    // Draw text content
    if (opt.font_size > 0 && buffer != NULL) {
        WLX_Rect text_rect = {
            .x = input_rect.x + opt.content_padding / 2,
            .y = input_rect.y,
            .w = input_rect.w - opt.content_padding / 2 - WLX_INPUTBOX_CURSOR_WIDTH - WLX_INPUTBOX_CURSOR_PADDING,
            .h = input_rect.h
        };

        float cursor_x = text_rect.x;
        float cursor_y = text_rect.y;

        // Render text up to cursor position to get cursor coordinates
        if (state.focused) {
            char temp[512];
            size_t copy_len = istate->cursor_pos < sizeof(temp) - 1 ? istate->cursor_pos : sizeof(temp) - 1;
            strncpy(temp, buffer, copy_len);
            temp[copy_len] = '\0';
            wlx_calc_cursor_position(ctx, text_rect, temp, ts, WLX_LEFT, opt.wrap, &cursor_x, &cursor_y);
        }

        // Render full text
        wlx_draw_text_fitted(ctx, text_rect, buffer, ts, WLX_LEFT, opt.wrap);

        if (cursor_x > text_rect.x)
            cursor_x += WLX_INPUTBOX_CURSOR_PADDING;

        float cursor_height = opt.font_size;
        if (state.focused) {
            istate->cursor_blink_time += wlx_get_frame_time(ctx);
        }

        // Draw cursor if focused and fited in text rect
        if (state.focused && cursor_x < (text_rect.x + text_rect.w) && (cursor_y + cursor_height) < (text_rect.y + text_rect.h)) {
            if (fmodf(istate->cursor_blink_time, WLX_INPUTBOX_CURSOR_BLINK_PERIOD) < (WLX_INPUTBOX_CURSOR_BLINK_PERIOD * WLX_INPUTBOX_CURSOR_VISIBLE_FRACTION)) {

                wlx_draw_line(ctx, cursor_x, cursor_y, cursor_x, cursor_y + cursor_height, WLX_INPUTBOX_CURSOR_WIDTH, opt.cursor_color);
            }
        }
    }

    if (opt.id) wlx_pop_id(ctx);
    return state.focused;
}

static void wlx_resolve_opt_slider(const WLX_Theme *theme, WLX_Slider_Opt *opt) {
    if (wlx_color_is_zero(opt->track_color)) opt->track_color            = theme->slider.track;
    if (wlx_color_is_zero(opt->thumb_color)) opt->thumb_color            = theme->slider.thumb;
    if (wlx_color_is_zero(opt->label_color)) opt->label_color            = theme->slider.label;
    if (opt->font == WLX_FONT_DEFAULT)      opt->font                   = theme->font;
    if (opt->font_size              <= 0)   opt->font_size              = theme->font_size;
    if (opt->spacing                <= 0)   opt->spacing                = theme->spacing;
    if (opt->track_height           <= 0)   opt->track_height           = theme->slider.track_height;
    if (opt->thumb_width            <= 0)   opt->thumb_width            = theme->slider.thumb_width;
    if (opt->roundness              <= 0)   opt->roundness              = theme->roundness;
    if (opt->rounded_segments       <= 0)   opt->rounded_segments       = theme->rounded_segments;
    if (opt->hover_brightness       == 0)   opt->hover_brightness       = theme->hover_brightness;
    if (opt->thumb_hover_brightness == 0)   opt->thumb_hover_brightness = opt->hover_brightness * 0.5f;
    if (opt->opacity                <= 0.0f) opt->opacity               = 1.0f;
    float theme_opacity = (theme->opacity <= 0.0f) ? 1.0f : theme->opacity;
    opt->opacity *= theme_opacity;
    opt->track_color = wlx_color_apply_opacity(opt->track_color, opt->opacity);
    opt->thumb_color = wlx_color_apply_opacity(opt->thumb_color, opt->opacity);
    opt->label_color = wlx_color_apply_opacity(opt->label_color, opt->opacity);
}

WLXDEF bool wlx_slider_impl(WLX_Context *ctx, const char *label, float *value, WLX_Slider_Opt opt, const char *file, int line) {
    if (opt.id) wlx_push_id(ctx, wlx_hash_string(opt.id));
    assert(ctx != NULL);
    assert(value != NULL && "slider value pointer must not be NULL");
    wlx_resolve_opt_slider(ctx->theme, &opt);

    WLX_Widget_Rect wg = wlx_widget_begin(ctx, opt.pos, opt.span, opt.padding, opt.width, opt.height, opt.min_width, opt.min_height, opt.max_width, opt.max_height, opt.widget_align, opt.overflow);
    WLX_Rect wr = wg.rect;

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
        snprintf(value_str, sizeof(value_str), "%.2f", *value);
        float value_text_h = 0;
        wlx_measure_text(ctx, value_str, ts, &value_text_width, &value_text_h);
        value_text_width += padding;
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

    float thumb_cx = usable_x + t * usable_w;
    float thumb_h = wr.h * 0.7f;
    float thumb_y = wr.y + (wr.h - thumb_h) / 2.0f;

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
        thumb_cx = usable_x + t * usable_w;
    }

    bool is_active = inter.active;
    bool is_hover = inter.hover;

    // Draw label
    if (label != NULL && opt.font_size > 0) {
        WLX_Rect label_rect = { wr.x, wr.y, label_width, wr.h };
        wlx_draw_text_fitted(ctx, label_rect, label, ts, WLX_LEFT, false);
    }

    // Draw track
    WLX_Color tc = is_hover || is_active ? wlx_color_brightness(opt.track_color, opt.hover_brightness) : opt.track_color;
    WLX_Rect track_rect = { track_x, track_y, track_w, opt.track_height };
    wlx_draw_rect_rounded(ctx, track_rect, opt.roundness, opt.rounded_segments, tc);

    // Draw filled portion of track
    float fill_w = thumb_cx - track_x;
    if (fill_w > 0) {
        WLX_Color fill_color = is_active ? opt.thumb_color : wlx_color_brightness(opt.thumb_color, opt.fill_inactive_brightness);
        WLX_Rect fill_rect = { track_x, track_y, fill_w, opt.track_height };
        wlx_draw_rect_rounded(ctx, fill_rect, opt.roundness, opt.rounded_segments, fill_color);
    }

    // Draw thumb
    WLX_Color thumb_c = opt.thumb_color;
    if (is_active) {
        thumb_c = wlx_color_brightness(thumb_c, opt.hover_brightness);
    } else if (is_hover) {
        thumb_c = wlx_color_brightness(thumb_c, opt.thumb_hover_brightness);
    }
    WLX_Rect thumb_rect = { thumb_cx - half_thumb, thumb_y, opt.thumb_width, thumb_h };
    wlx_draw_rect_rounded(ctx, thumb_rect, opt.roundness, opt.rounded_segments, thumb_c);

    // Draw value text
    if (opt.show_label && opt.font_size > 0) {
        // Recalculate value string in case it changed
        snprintf(value_str, sizeof(value_str), "%.2f", *value);
        float vt_x = track_x + track_w + padding;
        WLX_Rect value_rect = { vt_x, wr.y, value_text_width, wr.h };
        wlx_draw_text_fitted(ctx, value_rect, value_str, ts, WLX_LEFT, false);
    }

    if (opt.id) wlx_pop_id(ctx);
    return changed;
}

// ============================================================================
// Implementation: scroll panels
// ============================================================================

static const float WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED = 20.0f;

static void wlx_resolve_opt_scroll_panel(const WLX_Theme *theme, WLX_Scroll_Panel_Opt *opt) {
    if (wlx_color_is_zero(opt->back_color))      opt->back_color                 = theme->background;
    if (wlx_color_is_zero(opt->scrollbar_color)) opt->scrollbar_color            = theme->scrollbar.bar;
    if (opt->scrollbar_hover_brightness == 0)   opt->scrollbar_hover_brightness = theme->hover_brightness;
    if (opt->scrollbar_width            <= 0)   opt->scrollbar_width            = theme->scrollbar.width;
    if (opt->opacity                    <= 0.0f) opt->opacity                   = 1.0f;
    float theme_opacity = (theme->opacity <= 0.0f) ? 1.0f : theme->opacity;
    opt->opacity *= theme_opacity;
    opt->back_color      = wlx_color_apply_opacity(opt->back_color,      opt->opacity);
    opt->scrollbar_color = wlx_color_apply_opacity(opt->scrollbar_color, opt->opacity);
}

WLXDEF void wlx_scroll_panel_begin_impl(WLX_Context *ctx, float content_height, WLX_Scroll_Panel_Opt opt, const char *file, int line) {
    if (opt.id) wlx_push_id(ctx, wlx_hash_string(opt.id));
    wlx_resolve_opt_scroll_panel(ctx->theme, &opt);

    // NOTE: scroll_panel does NOT use wlx_widget_begin() because its scroll-height
    // tracking is conditional (only for non-auto-height panels) and deferred.
    WLX_Rect r = wlx_get_widget_cell_rect(ctx, opt.pos, opt.span, opt.padding);
    WLX_Rect wr = wlx_resolve_widget_rect(r, opt.width, opt.height, opt.min_width, opt.min_height, opt.max_width, opt.max_height, opt.widget_align, opt.overflow);

    // Get persistent state for this scroll panel
    WLX_State pers_state = wlx_get_state_impl(ctx, sizeof(WLX_Scroll_Panel_State), file, line);
    size_t panel_id = pers_state.id;
    WLX_Scroll_Panel_State *state = (WLX_Scroll_Panel_State *)pers_state.data;

    // Initialize content_height on first frame (when calloc'd to 0)
    if (state->content_height == 0.0f) {
        state->content_height = (content_height < 0) ? wr.h : content_height;
    }

    state->auto_height = (content_height < 0);
    state->panel_rect = wr;
    state->wheel_scroll_speed = opt.wheel_scroll_speed;

    // Contribute this panel's viewport height to parent layout's content tracking
    // (for outer auto-height measurement). Auto-height panels skip this because
    // they set up their own tracking below.
    if (!state->auto_height && ctx->layouts.count > 0) {
        ctx->layouts.items[ctx->layouts.count - 1].accumulated_content_height +=
            (opt.height > 0) ? (float)opt.height : r.h;
    }

    // Save outer auto-scroll tracking context before potentially changing it
    state->saved_auto_scroll_panel_id = ctx->auto_scroll.panel_id;
    state->saved_auto_scroll_total_height = ctx->auto_scroll.total_height;

    // For auto height: use previous frame's measured content_height (already stored in state)
    // For explicit height: update directly
    if (!state->auto_height) {
        state->content_height = content_height;
    }
    // else: keep state->content_height from previous frame's measurement

    if (state->auto_height) {
        ctx->auto_scroll.panel_id = panel_id;
        ctx->auto_scroll.total_height = 0;
    } else {
        // Clear so inner widgets don't pollute outer panel's auto-height measurement
        ctx->auto_scroll.panel_id = 0;
        ctx->auto_scroll.total_height = 0;
    }

    content_height = state->content_height;

    float max_scroll = content_height - wr.h;
    if (max_scroll < 0) max_scroll = 0;

    // --- Panel-level hover detection (wheel scrolling deferred to `wlx_scroll_panel_end()`) ---
    state->hovered = wlx_point_in_rect(ctx->input.mouse_x, ctx->input.mouse_y,
                                   wr.x, wr.y, wr.w, wr.h);

    // Push onto scroll panel stack so `wlx_scroll_panel_end()` can handle wheel events
    wlx_da_append(&ctx->scroll_panels, state);

    // --- Scrollbar drag (uses wlx_get_interaction() with DRAG mode) ---
    // We compute the scrollbar rect before interaction so we can use it as the hit area.
    // During an active drag we update scroll_offset from mouse position each frame.
    if (opt.show_scrollbar && content_height > wr.h) {
        float scrollbar_height = (wr.h / content_height) * wr.h;
        float scrollbar_pos = (state->scroll_offset / content_height) * wr.h;

        WLX_Rect sb_rect = {
            wr.x + wr.w - opt.scrollbar_width,
            wr.y + scrollbar_pos,
            opt.scrollbar_width,
            scrollbar_height
        };

        WLX_Interaction sb_inter = wlx_get_interaction(
            ctx, sb_rect,
            WLX_INTERACT_HOVER | WLX_INTERACT_DRAG,
            file, line
        );

        // Record drag offset on the frame we start dragging
        if (sb_inter.active && !state->dragging_scrollbar) {
            state->dragging_scrollbar = true;
            state->drag_offset = (float)ctx->input.mouse_y - (wr.y + scrollbar_pos);
        }

        // While dragging, compute new scroll offset from mouse position
        if (sb_inter.active && state->dragging_scrollbar) {
            float scrollbar_track = wr.h - scrollbar_height;
            if (scrollbar_track > 0) {
                float new_scrollbar_pos = (float)ctx->input.mouse_y - wr.y - state->drag_offset;
                if (new_scrollbar_pos < 0) new_scrollbar_pos = 0;
                if (new_scrollbar_pos > scrollbar_track) new_scrollbar_pos = scrollbar_track;
                state->scroll_offset = (new_scrollbar_pos / scrollbar_track) * max_scroll;
            }
        }

        if (!sb_inter.active) {
            state->dragging_scrollbar = false;
        }
    } else {
        state->dragging_scrollbar = false;
    }

    // Clamp scroll offset
    if (state->scroll_offset < 0) state->scroll_offset = 0;
    if (state->scroll_offset > max_scroll) state->scroll_offset = max_scroll;

    // Draw panel background
    wlx_draw_rect(ctx, wr, opt.back_color);

    // Enable scissor mode for clipping.
    // For nested scroll panels, intersect with all parent panels' rects
    // so content can't escape the outer panel's viewport.
    WLX_Rect scissor = wr;
    for (size_t i = 0; i < ctx->scroll_panels.count; i++) {
        scissor = wlx_rect_intersect(scissor, ctx->scroll_panels.items[i]->panel_rect);
    }
    wlx_begin_scissor(ctx, scissor);

    // Draw scrollbar if needed (interaction already handled above)
    if (opt.show_scrollbar && content_height > wr.h) {
        float scrollbar_height = (wr.h / content_height) * wr.h;
        float scrollbar_pos = (state->scroll_offset / content_height) * wr.h;

        WLX_Rect scrollbar_rect = {
            wr.x + wr.w - opt.scrollbar_width,
            wr.y + scrollbar_pos,
            opt.scrollbar_width,
            scrollbar_height
        };

        bool scrollbar_hover = wlx_point_in_rect(
            ctx->input.mouse_x, ctx->input.mouse_y,
            scrollbar_rect.x, scrollbar_rect.y,
            scrollbar_rect.w, scrollbar_rect.h
        );

        WLX_Color sb_draw_color = (state->dragging_scrollbar || scrollbar_hover) ?
            wlx_color_brightness(opt.scrollbar_color, opt.scrollbar_hover_brightness) :
            opt.scrollbar_color;

        wlx_draw_rect(ctx, scrollbar_rect, sb_draw_color);
    }

    // Create layout for the scrollable content
    WLX_Rect content_rect = {
        .x = wr.x,
        .y = wr.y - state->scroll_offset,
        .w = wr.w - (opt.show_scrollbar && content_height > wr.h ? opt.scrollbar_width : 0),
        .h = content_height
    };

    WLX_Layout l = wlx_create_layout(ctx, content_rect, 1, WLX_VERT);
    wlx_da_append(&ctx->layouts, l);
    if (opt.id) wlx_pop_id(ctx);
}

WLXDEF void wlx_scroll_panel_end(WLX_Context *ctx) {
    assert(ctx->layouts.count > 0);
    // Contribute the scroll panel wrapper layout's tracked content height.
    // Handles the edge case where widgets are placed directly inside a scroll
    // panel without a user-created layout (the wrapper is the only layout).
    WLX_Layout *wrapper = &ctx->layouts.items[ctx->layouts.count - 1];
    if (ctx->auto_scroll.panel_id) {
        ctx->auto_scroll.total_height += wrapper->accumulated_content_height;
    }
    ctx->layouts.count -= 1;

    // Pop scroll panel from the stack.
    assert(ctx->scroll_panels.count > 0);
    WLX_Scroll_Panel_State *state_sp = ctx->scroll_panels.items[--ctx->scroll_panels.count];

    // Update auto-height scroll panel with measured content height FIRST,
    // before wheel handling, so max_scroll uses the fresh measurement.
    // Without this ordering, fast wheel scrolling oscillates because wheel
    // deltas are clamped to the stale (previous-frame) content_height, then
    // the content_height changes, and next frame's clamp snaps the offset
    // to a different value — producing visible flicker.
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
    if (state_sp->hovered && ctx->input.wheel_delta != 0.0f) {
        float max_scroll = state_sp->content_height - state_sp->panel_rect.h;
        if (max_scroll < 0) max_scroll = 0;

        float scroll_speed = state_sp->wheel_scroll_speed;
        if (scroll_speed <= 0.0f) scroll_speed = WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED;
        state_sp->scroll_offset -= ctx->input.wheel_delta * scroll_speed;

        if (state_sp->scroll_offset < 0) state_sp->scroll_offset = 0;
        if (state_sp->scroll_offset > max_scroll) state_sp->scroll_offset = max_scroll;
        ctx->input.wheel_delta = 0.0f;  // consume — prevent outer panels from scrolling
    }

    // Re-clamp scroll offset to (possibly updated) content height
    {
        float max_scroll = state_sp->content_height - state_sp->panel_rect.h;
        if (max_scroll < 0) max_scroll = 0;
        if (state_sp->scroll_offset > max_scroll) state_sp->scroll_offset = max_scroll;
        if (state_sp->scroll_offset < 0) state_sp->scroll_offset = 0;
    }

    // Restore outer auto-scroll tracking context
    ctx->auto_scroll.panel_id = state_sp->saved_auto_scroll_panel_id;
    ctx->auto_scroll.total_height = state_sp->saved_auto_scroll_total_height;

    wlx_end_scissor(ctx);

    // If there's a parent scroll panel, restore its scissor clipping
    if (ctx->scroll_panels.count > 0) {
        WLX_Scroll_Panel_State *parent = ctx->scroll_panels.items[ctx->scroll_panels.count - 1];
        WLX_Rect pr = parent->panel_rect;
        wlx_begin_scissor(ctx, pr);
    }
}

#endif // WOLLIX_IMPLEMENTATION
