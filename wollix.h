/*
 * wollix.h - Woven layouts for C.
 *
 * Version: 0.8.0  (WOLLIX_VERSION / WLX_VERSION)
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
 * (including textured checkboxes), input boxes and textareas, sliders,
 * progress bars, toggles, radio buttons, images, separators, scroll panels,
 * panels, split layouts, the list clipper, and the popup family on overlay
 * layers (dropdown, tooltip, menu / submenu / menu button, wlx_overlay).
 * Input covers three mouse buttons, float wheel axes, F-keys, keyboard
 * focus traversal with a focus ring, and an optional cursor-shape callback.
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
 * COMPILER FLAGS
 * ---------------------------------------------------------------------------
 * The options-struct call style (library defaults first, then your
 * designated-initializer overrides) intentionally repeats initializers,
 * which compilers can warn about. Build with the matching suppression:
 *
 *   clang:  -Wno-initializer-overrides   (clang warns even without -Wextra)
 *   gcc:    -Wno-override-init           (needed together with -Wextra)
 *
 * The header also relies on two widely supported C11 extensions: empty
 * __VA_ARGS__ in the option macros (standard C23) and __FUNCTION__ in the
 * allocation wrappers. GCC and Clang are the supported compilers.
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
 * WLX_SLOT_SINGLE_PASS_CLAMP
 *     Opt out of iterative freeze-and-redistribute and use the simpler O(n)
 *     single-pass clamp for slot min/max constraints. When defined, surplus
 *     or deficit from a clamped slot is NOT handed back to its siblings, so
 *     a clamped slot may leave a small gap or overflow when min/max fires.
 *     Default (undefined): redistribute, so offsets stay consistent.
 *
 * WLX_SLOT_MINMAX_REDISTRIBUTE
 *     Deprecated / no-op. Redistribution is now the default; this symbol is
 *     accepted but ignored, retained for source compatibility. Use
 *     WLX_SLOT_SINGLE_PASS_CLAMP to opt out of redistribution.
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
 * WLX_DA_INIT_CAP  (default 256)
 *     Initial capacity for internal dynamic arrays.
 *
 * WLX_OVERLAY_MAX_LAYERS  (default 8)
 *     Maximum overlay nesting depth (command replay passes). Content past
 *     the cap draws on the top layer.
 *
 * WLX_MENU_STACK_MAX  (default 4)
 *     Maximum depth of open wlx_menu_begin / wlx_submenu_begin pairs.
 *
 * WLX_DROPDOWN_MAX_LIST_HEIGHT  (default 240.0f)
 *     Default cap for a dropdown's open list height; taller lists scroll.
 *
 * WLX_FOCUS_RING_THICKNESS  (default 0.4f)  /  WLX_FOCUS_RING_GAP  (default 1.0f)
 *     Keyboard focus ring outline thickness and gap outside the widget rect.
 *
 * WLX_SCISSOR_STACK_MAX  (default 64)
 *     Maximum depth of explicit scissor scopes.
 *
 * WLX_INPUTBOX_MASK_MAX  (default 256)
 *     Maximum rendered mask glyphs for password inputboxes; longer text
 *     keeps editing correctly but the visible mask stops growing.
 *
 * WLX_TEXT_RANGE_STACK_CAP  (default 1024)
 *     Stack buffer for temporary NUL-terminated span copies on fallback
 *     paths; longer spans are heap-allocated.
 *
 * WLX_TEXT_RUN_MAX_UNITS  (default 512)
 *     Maximum text units (codepoints or fallback bytes) processed in a single text-layout run.
 *
 * WLX_TEXT_RUN_MAX_LINES  (default 128)
 *     Maximum visual lines produced by a single text-layout run.
 *
 * WLX_INPUTBOX_MULTILINE_MAX_UNITS  (default 4096)
 * WLX_INPUTBOX_MULTILINE_MAX_LINES  (default 512)
 *     Text-run budget for multiline inputbox geometry (caret, hit-test,
 *     selection, scroll, draw). Content beyond the budget stays in the
 *     buffer but drops out of geometry.
 *
 * WLX_EDITOR_MAX_LINE_UNITS  (default 1024)
 *     Per-record text-unit budget for truncate-and-continue line builds.
 *     A safety cap on the units any single window record measures, not a
 *     horizontal reach limit: the editor's no-wrap geometry re-enters a
 *     line longer than the budget at a measure origin near the view, so
 *     caret, view, and edits reach every byte of every line. Wrapped
 *     lines share one budget across their rows and freeze past it.
 *
 * WLX_EDITOR_ORIGIN_BACKSCAN  (default 64)
 *     How far (bytes) a no-wrap re-entry origin scans backward to prefer
 *     the boundary just after a space over an arbitrary unit boundary.
 *
 * WLX_TEXT_UNDO_ENTRIES  (default 512)  /  WLX_TEXT_UNDO_BYTES  (default 262144)
 *     Undo journal bounds per text widget (inputbox, textarea, editor) and
 *     per direction: retained undo entries and bytes of removed text. Whole
 *     oldest undo steps are evicted first; a single step larger than either
 *     cap is admitted and evicts everything older. Password fields keep no
 *     journal. WLX_TEXT_UNDO_ENTRIES 0 compiles the journal out.
 *
 * WLX_TEXT_ADVANCES_CHUNK  (default 256)
 *     Maximum text units filled per WLX_Backend.measure_text_advances
 *     call. Consecutive chunks splice by adding the running base advance.
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
 * ASSERTION POLICY
 * ---------------------------------------------------------------------------
 * Plain assert() diagnoses API-contract violations (bad arguments, unbalanced
 * begin/end pairs, unreasonable counts) and compiles out under NDEBUG.
 * WLX_HARD_ASSERT stays active in release builds and is reserved for guards
 * whose failure would corrupt memory: out-of-bounds writes, failed allocation
 * results, and persistent-state size collisions. Any new guard whose failure
 * mode is writing memory must use WLX_HARD_ASSERT, never plain assert().
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

#define WOLLIX_VERSION "0.8.0"
#define WLX_VERSION WOLLIX_VERSION

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

// The public half is a C surface with C linkage. A C++ translation unit
// includes it as-is and links against the implementation compiled as C11
// in one C translation unit; see the "Calling from C++" notes below and
// wlx_<x>_opt_defaults().
#ifdef __cplusplus
extern "C" {
#endif

// Linkage qualifier prepended to every public function declaration and definition.
//
// By default, WLXDEF expands to nothing (normal external linkage).
// To make all functions file-local (single translation-unit builds):
//   #define WLXDEF static
#ifndef WLXDEF
#define WLXDEF
#endif

// Core helpers whose only callers live in an extension header (e.g.
// wollix_editor.h) compile unused in translation units that include the
// core alone; the attribute keeps those TUs warning-clean without
// weakening -Wunused-function for genuinely dead code.
#if defined(__GNUC__) || defined(__clang__)
#define WLX_EXTENSION_USED __attribute__((unused))
#else
#define WLX_EXTENSION_USED
#endif

#ifdef WLX_PERF
typedef uint64_t (*WLX_Perf_Timestamp_Fn)(void *user);

typedef struct {
    uint64_t alloc_calls;
    uint64_t calloc_calls;
    uint64_t realloc_calls;
    uint64_t free_calls;
    uint64_t alloc_bytes;
    uint64_t calloc_bytes;
    uint64_t realloc_bytes;
} WLX_Perf_Allocator_Stats;

static WLX_Perf_Allocator_Stats *wlx_perf_allocator_sink = NULL;

static inline void wlx_perf_note_alloc(size_t size) {
    if (!wlx_perf_allocator_sink) return;
    wlx_perf_allocator_sink->alloc_calls++;
    wlx_perf_allocator_sink->alloc_bytes += (uint64_t)size;
}

static inline void wlx_perf_note_calloc(size_t count, size_t size) {
    if (!wlx_perf_allocator_sink) return;
    wlx_perf_allocator_sink->calloc_calls++;
    wlx_perf_allocator_sink->calloc_bytes += (uint64_t)(count * size);
}

static inline void wlx_perf_note_realloc(size_t size) {
    if (!wlx_perf_allocator_sink) return;
    wlx_perf_allocator_sink->realloc_calls++;
    wlx_perf_allocator_sink->realloc_bytes += (uint64_t)size;
}

static inline void wlx_perf_note_free(void *ptr) {
    if (!wlx_perf_allocator_sink || ptr == NULL) return;
    wlx_perf_allocator_sink->free_calls++;
}
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

// Colour literal. C spells it as a compound literal; C++ (which has none)
// as a brace-initialised WLX_Color prvalue, with the casts C++11
// list-initialisation needs to accept int arguments without narrowing.
#ifdef __cplusplus
#define WLX_RGBA(r, g, b, a) (WLX_Color{ (uint8_t)(r), (uint8_t)(g), (uint8_t)(b), (uint8_t)(a) })
#else
#define WLX_RGBA(r, g, b, a) ((WLX_Color){ (r), (g), (b), (a) })
#endif

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
// Overridable before include.
#ifndef WLX_MAX_SLOT_COUNT
#define WLX_MAX_SLOT_COUNT 100000
#endif

// Maximum number of slots that can use WLX_SIZE_CONTENT in a single layout.
// Stores per-slot measured main-axis extents from the previous frame (128
// bytes at the default). Overridable before include.
#ifndef WLX_CONTENT_SLOTS_MAX
#define WLX_CONTENT_SLOTS_MAX 32
#endif

// Stack-allocated working array limit for min/max redistribution in wlx_compute_offsets.
// Counts above this fall back to heap allocation. Overridable before include.
#ifndef WLX_OFFSET_STACK_LIMIT
#define WLX_OFFSET_STACK_LIMIT 64
#endif

// Keyboard focus ring: an accent outline drawn at frame end around the
// Tab-focused widget, this thick and this far outside its rect (pixels).
// Overridable before include.
#ifndef WLX_FOCUS_RING_THICKNESS
#define WLX_FOCUS_RING_THICKNESS 0.4f
#endif
#ifndef WLX_FOCUS_RING_GAP
#define WLX_FOCUS_RING_GAP 1.0f
#endif

// Maximum overlay nesting depth (command replay passes). Content past the
// cap draws on the top layer. Overridable before include.
#ifndef WLX_OVERLAY_MAX_LAYERS
#define WLX_OVERLAY_MAX_LAYERS 8
#endif

// Stack buffer capacity for temporary null-terminated span copies in fallback
// paths. Spans larger than this are heap-allocated. Overridable before include.
#ifndef WLX_TEXT_RANGE_STACK_CAP
#define WLX_TEXT_RANGE_STACK_CAP 1024
#endif

#define WLX_UNUSED(value) (void)(value)
#define WLX_UNREACHABLE(message) do { fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message); abort(); } while(0)

// Memory-safety guard that stays active in release (NDEBUG) builds. Reserved
// for conditions whose failure would write out of bounds, dereference a failed
// allocation, or hand out a wrongly-sized persistent-state buffer. See the
// ASSERTION POLICY section in the header preamble.
#define WLX_HARD_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "%s:%d: wollix fatal: %s\n", \
        __FILE__, __LINE__, (msg)); abort(); } } while (0)

#define wlx_array_len(array) (sizeof(array)/sizeof(array[0]))
#define wlx_zero_struct(instance) memset(&(instance), 0, sizeof(instance))
#define wlx_zero_array(count, pointer) memset((pointer), 0, (count) * sizeof((pointer)[0]))

#define wlx_alloc(size) wlx_alloc_impl(size, __FILE__, __LINE__, __FUNCTION__)
static inline void* wlx_alloc_impl(size_t size, const char *file, int line, const char *func) {
    void *ptr = malloc(size);
    #ifdef WLX_PERF
        wlx_perf_note_alloc(size);
    #endif
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
    #ifdef WLX_PERF
        wlx_perf_note_calloc(count, size);
    #endif
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
        #ifdef WLX_PERF
            wlx_perf_note_realloc(size);
        #endif
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
    #ifdef WLX_PERF
        wlx_perf_note_free(ptr);
    #endif
    #ifdef WLX_MEMORY_DEBUG
        printf("%s:%d: Deallocated = %s, %p\n", file, line, func, ptr);
    #else
        WLX_UNUSED(file); WLX_UNUSED(line); WLX_UNUSED(func);
    #endif
    free(ptr);
}

// Grow-and-reuse byte buffer (process- or context-lifetime): ensure room
// for `needed` bytes, geometric growth from 1024, old buffer kept intact on
// allocation failure (the only case that returns false). Shared by the
// backend clipboard transports and any other lazily grown flat buffer.
static inline bool wlx_buf_reserve(char **buf, size_t *cap, size_t needed) {
    if (needed <= *cap) return true;
    size_t new_cap = *cap == 0 ? 1024 : *cap;
    while (new_cap < needed) {
        WLX_HARD_ASSERT(new_cap <= SIZE_MAX / 2,
            "size_t overflow in wlx_buf_reserve");
        new_cap *= 2;
    }
    char *new_buf = (char *)wlx_realloc(*buf, new_cap);
    if (new_buf == NULL) return false;
    *buf = new_buf;
    *cap = new_cap;
    return true;
}

// Largest offset <= pos that does not point at a UTF-8 continuation byte
// (a boundary floor; offset 0 is always a boundary). The caller guarantees
// pos indexes readable bytes of s. Bounded back-offs (a floor above 0)
// keep their own loops.
static inline size_t wlx_utf8_floor(const char *s, size_t pos) {
    while (pos > 0 && ((unsigned char)s[pos] & 0xC0) == 0x80) pos--;
    return pos;
}

// Initial capacity for growable internal buffers (see wlx_sub_arena_reserve).
// Overridable before include.
#ifndef WLX_DA_INIT_CAP
#define WLX_DA_INIT_CAP 256
#endif

// Seed capacity of the doubling growers outside the arenas (interaction
// candidate list, text-geometry unit arrays). Internal, not a knob.
#define WLX_GROW_INIT_CAP 64

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
// (font_size, color) arguments in backend callbacks and text helpers.
typedef struct {
    WLX_Font  font;      // 0 -> backend default font
    int      font_size;  // 0 -> resolve from theme
    WLX_Color color;     // {0,0,0,0} -> resolve from theme foreground
    int      spacing;    // 0 -> natural backend spacing; nonzero = opt-in extra tracking
} WLX_Text_Style;

#define WLX_TEXT_STYLE_DEFAULT \
    ((WLX_Text_Style){ .font = WLX_FONT_DEFAULT, .font_size = 0, .color = {0}, .spacing = 0 })

// Mouse cursor shape the core asks the backend to show. ARROW is 0 so a
// zero-initialized context matches every platform's default cursor. The
// enum is append-only; new shapes are added when a widget needs them.
typedef enum {
    WLX_CURSOR_ARROW = 0,
    WLX_CURSOR_IBEAM,
    WLX_CURSOR_COUNT
} WLX_Cursor_Shape;

// Backend contract version. A table must carry WLX_BACKEND_CONTRACT_VERSION
// in `contract_version`; wlx_begin refuses any other value in every build,
// since calling a table of the wrong shape through these signatures is
// memory-unsafe. Version 2 (v0.9): every callback takes a trailing
// `void *user`, the table's own `user` member, so an adapter can reach
// per-instance state instead of file-scope globals. A v0.8 table is
// wrapped in one line with wlx_backend_from_v1 (deprecated shim, removed in
// the first minor release after 0.9).
#define WLX_BACKEND_CONTRACT_VERSION 2u

typedef struct {
    uint32_t contract_version;  // WLX_BACKEND_CONTRACT_VERSION
    void    *user;              // passed as the last argument of every callback; the core never reads it
    void (*draw_rect)(WLX_Rect rect, WLX_Color color, void *user);
    void (*draw_rect_lines)(WLX_Rect rect, float thick, WLX_Color color, void *user);
    void (*draw_rect_rounded)(WLX_Rect rect, float roundness, int segments, WLX_Color color, void *user);
    void (*draw_rect_rounded_lines)(WLX_Rect rect, float roundness, int segments, float thick, WLX_Color color, void *user);
    void (*draw_circle)(float cx, float cy, float radius, int segments, WLX_Color color, void *user); /* optional: NULL falls back to draw_rect_rounded */
    void (*draw_ring)(float cx, float cy, float inner_r, float outer_r, int segments, WLX_Color color, void *user); /* optional: NULL falls back to draw_rect_rounded_lines */
    void (*draw_line)(float x1, float y1, float x2, float y2, float thick, WLX_Color color, void *user);
    // Text callbacks.
    //
    // draw_text / measure_text accept NUL-terminated text. They are required
    // for backend readiness and remain available for downstream backends that
    // have not implemented the slice callbacks below.
    //
    // draw_text_slice / measure_text_slice accept (text, len) byte spans and
    // are the preferred path: all in-tree backends (Raylib, SDL3, WASM)
    // implement them and the core text pipeline routes through them by
    // default. New backends SHOULD implement the slice callbacks; the
    // NUL-terminated callbacks then receive only synthetic NUL-terminated
    // input via the core fallback (rare, off the hot path).
    //
    // Every text callback receives the style after the context's style
    // transform (wlx_set_style_transform), the one place an application
    // reshapes text styles: draw, measure and advances therefore always
    // agree. An application that still overrides individual callbacks of an
    // adapter-installed table must forward `user` unchanged and must not
    // repoint `backend.user`, which every other callback of the table reads.
    void (*draw_text)(const char *text, float x, float y, WLX_Text_Style style, void *user);
    void (*measure_text)(const char *text, WLX_Text_Style style, float *out_w, float *out_h, void *user);
    void (*draw_texture)(WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint, void *user);
    // Scissor contract: begin_scissor installs rect as the current clip.
    // Core code explicitly restores parent clipping after ending nested child
    // regions, so backends must not rely on implicit push/pop clip stacks.
    void (*begin_scissor)(WLX_Rect rect, void *user);
    void (*end_scissor)(void *user);
    // Frame delta seconds. The core calls this exactly once per frame (in
    // wlx_begin) and caches the value for every wlx_get_frame_time read, so
    // an adapter may measure elapsed time since its own previous call.
    float (*get_frame_time)(void *user);
    // Slice text pair: the core calls these in preference to draw_text /
    // measure_text whenever they are set.
    void (*draw_text_slice)(const char *text, size_t len, float x, float y, WLX_Text_Style style, void *user);
    void (*measure_text_slice)(const char *text, size_t len, WLX_Text_Style style, float *out_w, float *out_h, void *user);
    // Optional batched advance measurement: fill out_advances[i] with the
    // cumulative advance width in pixels of the prefix [0, unit_ends[i]) of
    // one text run, for every i < unit_count, and return the number of
    // leading entries filled (a partial fill is valid; the core falls back
    // to per-unit prefix measures for the rest). NULL -> the core measures
    // per-unit prefixes through measure_text_slice.
    //
    // The run (text, len) is a single-style, single-line span with no tabs
    // when tab expansion is active (the core splits at tabs and applies
    // next-tab-stop rounding between segments itself). unit_ends is
    // strictly increasing with unit_ends[unit_count - 1] == len; the core
    // derives the unit policy (UTF-8 codepoints, malformed bytes as
    // one-byte units), so backends never re-implement it - they walk their
    // own glyph/cluster geometry and report the advance at (or snapped to
    // the nearest cluster edge after) each requested byte end. Reported
    // advances should be non-decreasing; the core clamps regardless.
    //
    // Runs are capped at WLX_TEXT_ADVANCES_CHUNK units. Consecutive chunks
    // of one line are spliced by adding the previous chunk's final advance,
    // so shaping context does not carry across a chunk boundary - the same
    // documented approximation class as a tab stop inside a line. Its
    // results are retained as caret, hit-test and fit geometry against text
    // drawn through draw_text_slice, which is why both see the transformed
    // style.
    size_t (*measure_text_advances)(const char *text, size_t len, WLX_Text_Style style,
                                    const size_t *unit_ends, size_t unit_count,
                                    float *out_advances, void *user); /* optional */
    // Optional soft-effect callbacks: NULL -> software fallback (layered rects /
    // concentric rings). rect is the element rect (not grown); color already has
    // effective opacity applied; roundness and rounded_segs come from the element.
    void (*draw_shadow)(WLX_Rect rect, WLX_Color color, float offset_x, float offset_y,
                        float blur, int layers, float roundness, int rounded_segs, void *user); /* optional */
    void (*draw_glow)(WLX_Rect rect, WLX_Color color, float spread, int rings,
                      float roundness, int rounded_segs, void *user); /* optional */
    // Optional vertical two-stop gradient: NULL -> software fallback (stacked
    // solid bands). top/bottom already have effective opacity applied;
    // roundness = 0 means sharp rect, > 0 with rounded_segs for rounding.
    void (*draw_gradient_v)(WLX_Rect rect, WLX_Color top, WLX_Color bottom,
                            float roundness, int rounded_segs, void *user); /* optional */
    // Optional clipboard transport. clipboard_get returns the current system
    // clipboard text as a borrowed, NUL-terminated UTF-8 string owned by the
    // backend and valid only until the next clipboard call or end of frame; the
    // core copies out immediately. clipboard_set copies the (text, len) span to
    // the system clipboard and must not retain the pointer. Either NULL ->
    // clipboard operations are safe no-ops.
    const char *(*clipboard_get)(void *user); /* optional */
    void (*clipboard_set)(const char *text, size_t len, void *user); /* optional */
    // Optional cursor shape. The core resolves the shape from the widget
    // under the pointer once per frame in wlx_begin and calls this only when
    // the shape changes, so implementations stay stateless. NULL -> the
    // platform cursor is never touched.
    void (*set_cursor)(WLX_Cursor_Shape shape, void *user); /* optional */
} WLX_Backend;

// Deprecated: the v0.8 backend table (contract v1), kept so an existing
// backend migrates in one line: `ctx->backend = wlx_backend_from_v1(&t);`.
// The v2 table it returns forwards every callback through 22 trampolines
// that reach the v1 table through `user`, so the caller keeps `t` alive
// for the context's lifetime (static storage in practice). A NULL v1
// member stays NULL on the v2 table, so the core's optional-callback
// fallbacks apply exactly as for a native table. Removed, with this type,
// in the first minor release after 0.9.
typedef struct {
    void (*draw_rect)(WLX_Rect rect, WLX_Color color);
    void (*draw_rect_lines)(WLX_Rect rect, float thick, WLX_Color color);
    void (*draw_rect_rounded)(WLX_Rect rect, float roundness, int segments, WLX_Color color);
    void (*draw_rect_rounded_lines)(WLX_Rect rect, float roundness, int segments, float thick, WLX_Color color);
    void (*draw_circle)(float cx, float cy, float radius, int segments, WLX_Color color);
    void (*draw_ring)(float cx, float cy, float inner_r, float outer_r, int segments, WLX_Color color);
    void (*draw_line)(float x1, float y1, float x2, float y2, float thick, WLX_Color color);
    void (*draw_text)(const char *text, float x, float y, WLX_Text_Style style);
    void (*measure_text)(const char *text, WLX_Text_Style style, float *out_w, float *out_h);
    void (*draw_texture)(WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint);
    void (*begin_scissor)(WLX_Rect rect);
    void (*end_scissor)(void);
    float (*get_frame_time)(void);
    void (*draw_text_slice)(const char *text, size_t len, float x, float y, WLX_Text_Style style);
    void (*measure_text_slice)(const char *text, size_t len, WLX_Text_Style style, float *out_w, float *out_h);
    size_t (*measure_text_advances)(const char *text, size_t len, WLX_Text_Style style,
                                    const size_t *unit_ends, size_t unit_count,
                                    float *out_advances);
    void (*draw_shadow)(WLX_Rect rect, WLX_Color color, float offset_x, float offset_y,
                        float blur, int layers, float roundness, int rounded_segs);
    void (*draw_glow)(WLX_Rect rect, WLX_Color color, float spread, int rings,
                      float roundness, int rounded_segs);
    void (*draw_gradient_v)(WLX_Rect rect, WLX_Color top, WLX_Color bottom,
                            float roundness, int rounded_segs);
    const char *(*clipboard_get)(void);
    void (*clipboard_set)(const char *text, size_t len);
    void (*set_cursor)(WLX_Cursor_Shape shape);
} WLX_Backend_V1;

WLXDEF WLX_Backend wlx_backend_from_v1(const WLX_Backend_V1 *v1);

// Context-level style transform: the one place an application reshapes
// text styles (a font-size scale for one backend, a face substitution).
// The core applies it to the WLX_Text_Style immediately before every text
// callback - draw, measure and advances alike - and nowhere else, so
// widget options, the command buffer and retained geometry hold nominal
// styles while the backend sees only transformed ones. The function must
// be pure in (style, user) and return a style the backend can render.
typedef WLX_Text_Style (*WLX_Style_Transform_Fn)(WLX_Text_Style style, void *user);

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

// Slot size literal, spelled per language: C as a compound literal, C++
// (which has none) as a brace-initialised WLX_Slot_Size prvalue with the
// casts C++11 list-initialisation needs to accept int arguments without
// narrowing. Every WLX_SLOT_* macro below expands through it, so the two
// languages share one vocabulary.
#ifdef __cplusplus
#define WLX_SLOT_LIT(kind, v, lo, hi) (WLX_Slot_Size{ (kind), (float)(v), (float)(lo), (float)(hi) })
#else
#define WLX_SLOT_LIT(kind, v, lo, hi) ((WLX_Slot_Size){ (kind), (v), (lo), (hi) })
#endif

#define WLX_SLOT_AUTO WLX_SLOT_LIT(WLX_SIZE_AUTO, 0, 0, 0)
#define WLX_SLOT_PX(px) WLX_SLOT_LIT(WLX_SIZE_PIXELS, (px), 0, 0)
#define WLX_SLOT_PCT(pct) WLX_SLOT_LIT(WLX_SIZE_PERCENT, (pct), 0, 0)
#define WLX_SLOT_FLEX(w) WLX_SLOT_LIT(WLX_SIZE_FLEX, (w), 0, 0)

// Constrained slot size variants
#define WLX_SLOT_PX_MINMAX(px, lo, hi) WLX_SLOT_LIT(WLX_SIZE_PIXELS, (px), (lo), (hi))
#define WLX_SLOT_FLEX_MIN(w, lo)        WLX_SLOT_LIT(WLX_SIZE_FLEX, (w), (lo), 0)
#define WLX_SLOT_FLEX_MAX(w, hi)        WLX_SLOT_LIT(WLX_SIZE_FLEX, (w), 0, (hi))
#define WLX_SLOT_FLEX_MINMAX(w, lo, hi) WLX_SLOT_LIT(WLX_SIZE_FLEX, (w), (lo), (hi))
#define WLX_SLOT_AUTO_MIN(lo)           WLX_SLOT_LIT(WLX_SIZE_AUTO, 0, (lo), 0)
#define WLX_SLOT_AUTO_MAX(hi)           WLX_SLOT_LIT(WLX_SIZE_AUTO, 0, 0, (hi))
#define WLX_SLOT_AUTO_MINMAX(lo, hi)    WLX_SLOT_LIT(WLX_SIZE_AUTO, 0, (lo), (hi))
#define WLX_SLOT_PCT_MINMAX(pct, lo, hi) WLX_SLOT_LIT(WLX_SIZE_PERCENT, (pct), (lo), (hi))

// Viewport-fill: resolve against the innermost scroll panel viewport
// (or root rect if no scroll panel). value = fraction (1.0 = full viewport).
#define WLX_SLOT_FILL                  WLX_SLOT_LIT(WLX_SIZE_FILL, 1.0f, 0, 0)
#define WLX_SLOT_FILL_PCT(p)           WLX_SLOT_LIT(WLX_SIZE_FILL, (p) / 100.0f, 0, 0)
#define WLX_SLOT_FILL_MIN(lo)          WLX_SLOT_LIT(WLX_SIZE_FILL, 1.0f, (lo), 0)
#define WLX_SLOT_FILL_MAX(hi)          WLX_SLOT_LIT(WLX_SIZE_FILL, 1.0f, 0, (hi))
#define WLX_SLOT_FILL_MINMAX(lo, hi)   WLX_SLOT_LIT(WLX_SIZE_FILL, 1.0f, (lo), (hi))

// Content-fit: slot height is determined by the child's preferred height
// (measured from the previous frame). min/max constrain the measured value.
#define WLX_SLOT_CONTENT                WLX_SLOT_LIT(WLX_SIZE_CONTENT, 0, 0, 0)
#define WLX_SLOT_CONTENT_MIN(lo)        WLX_SLOT_LIT(WLX_SIZE_CONTENT, 0, (lo), 0)
#define WLX_SLOT_CONTENT_MAX(hi)        WLX_SLOT_LIT(WLX_SIZE_CONTENT, 0, 0, (hi))
#define WLX_SLOT_CONTENT_MINMAX(lo, hi) WLX_SLOT_LIT(WLX_SIZE_CONTENT, 0, (lo), (hi))

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
// C only: an array compound literal has no C++ expression form. A C++
// caller names the array and passes its count and pointer:
//
//   static const WLX_Slot_Size sizes[] = { WLX_SLOT_PX(44), WLX_SLOT_FLEX(1) };
//   WLX_Layout_Opt lo = wlx_layout_opt_defaults();
//   lo.sizes = sizes;
//   wlx_layout_begin_impl(ctx, 2, WLX_VERT, lo, __FILE__, __LINE__);
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

// Basis used by widgets when vertically centering text within a rect.
// LINE_HEIGHT (default) uses the backend-measured line height; FONT_SIZE
// uses the typographic em size for cross-backend cap-height parity.
typedef enum {
    WLX_VMETRIC_LINE_HEIGHT = 0,
    WLX_VMETRIC_FONT_SIZE,
} WLX_Vertical_Metric;

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
    WLX_KEY_DELETE,
    WLX_KEY_HOME,
    WLX_KEY_END,
    WLX_KEY_PAGE_UP,
    WLX_KEY_PAGE_DOWN,
    WLX_KEY_F1, WLX_KEY_F2, WLX_KEY_F3, WLX_KEY_F4, WLX_KEY_F5, WLX_KEY_F6,
    WLX_KEY_F7, WLX_KEY_F8, WLX_KEY_F9, WLX_KEY_F10, WLX_KEY_F11, WLX_KEY_F12,
    WLX_KEY_INSERT,
    WLX_KEY_COUNT
} WLX_Key_Code;

// Modifier-key state as an orthogonal bitfield (not WLX_Key_Code entries):
// modifiers qualify other keys rather than acting as glyph/navigation
// actuations. Backends set WLX_Input_State.modifiers fresh each frame; query
// with wlx_mod_down(). Editing shortcuts use wlx_mod_command_down(), which
// resolves to SUPER (Cmd) on Apple platforms and CTRL elsewhere.
typedef enum {
    WLX_MOD_SHIFT = 1 << 0,
    WLX_MOD_CTRL  = 1 << 1,
    WLX_MOD_ALT   = 1 << 2,
    WLX_MOD_SUPER = 1 << 3,
} WLX_Key_Mod;

// ============================================================================
// Core state and context types
// ============================================================================

typedef struct {
    int mouse_x;
    int mouse_y;
    bool mouse_down;
    bool mouse_clicked; // true for one frame when clicked
    bool mouse_held;    // legacy: equal to mouse_down (ADR_040); the core reads mouse_down
    float wheel_delta;  // vertical wheel detents this frame (positive = up; 1.0 = one
                        // notch, fractional allowed from precision devices - backends
                        // must not quantize or debounce)
    bool keys_down[WLX_KEY_COUNT];     // current key states (held down)
    bool keys_pressed[WLX_KEY_COUNT];  // true for one frame when key pressed
    char text_input[32];    // text input this frame (for typing)

    // New fields are appended at the end of the struct so the byte offsets of
    // the fields above stay put for the WASM host. WLX_KEY_COUNT growth still
    // shifts everything after keys_down; the WASM-side offset asserts and the
    // JS INPUT_OFFSETS table police the layout in lockstep.
    bool keys_repeated[WLX_KEY_COUNT]; // true on each OS auto-repeat tick (in addition to keys_pressed on first press)
    uint32_t modifiers;                // active WLX_Key_Mod bits this frame
    float wheel_delta_x;    // horizontal wheel detents this frame; polarity mirrors
                            // wheel_delta (positive scrolls the offset back toward 0)
                            // so consumers reuse the same value - delta * speed form
    bool mouse_right_down;
    bool mouse_right_clicked;   // true for one frame on right press
    bool mouse_middle_down;
    bool mouse_middle_clicked;  // true for one frame on middle press
} WLX_Input_State;

// Persistent state for layouts that contain WLX_SIZE_CONTENT slots.
// Stores each slot's measured MAIN-AXIS extent from the previous frame:
// child heights for VERT layouts, child widths for HORZ layouts (a layout
// has one orientation, so one array serves both).
typedef struct {
    float measured[WLX_CONTENT_SLOTS_MAX];
} WLX_Content_Slot_State;

typedef enum {
    WLX_LAYOUT_LINEAR,
    WLX_LAYOUT_GRID,
} WLX_Layout_Kind;

// Start indices into the frame's open scroll panels, layouts and scissor
// scopes from which the current layer's clip walkers iterate (all zero on
// the base layer). Snapshotted on the overlay root at wlx_overlay_begin.
typedef struct {
    size_t panels_from;    // index into arena.scroll_panels
    size_t layouts_from;   // index into arena.layouts
    size_t scissor_from;   // index into scissor_stack
} WLX_Clip_Base;

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
    bool pushed_scope;  // true when a scope id was pushed at begin and must be popped at end
    bool clip_active;      // true when this layout began a clip scissor that wlx_layout_end must release
    bool is_overlay_root;  // overlay subtree root: contributes nothing to parent content tracking
    bool overlay_clip;     // overlay root recorded a body scissor that wlx_overlay_end must release
    WLX_Rect clip_rect;    // rect this layout clips to (clip_active layouts and overlay_clip roots)
    WLX_Clip_Base overlay_saved_base;  // overlay root only: enclosing clip context, saved at begin / restored at end

    // Content-fit tracking (content_state NULL when no CONTENT slots)
    bool   has_content_sizes;               // a slot-size array was retained in the byte scratch
    size_t content_sizes_scratch_off;       // byte offset into ctx->scratch of that array;
                                            // read only via wlx_layout_content_sizes
    WLX_Content_Slot_State *content_state;  // persistent state pointer
    size_t content_slot_measures_off;        // index into slot_size_offsets for the content measure buffer
    bool has_content_slot_measures;          // true when content_slot_measures_off is valid
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
        WLX_HARD_ASSERT(new_capacity <= SIZE_MAX / 2,
            "size_t overflow in sub-arena reserve");
        new_capacity *= 2;
    }

    WLX_HARD_ASSERT(new_capacity <= SIZE_MAX / sa->item_size,
        "size_t overflow in sub-arena bytes");
    old_bytes = sa->capacity * sa->item_size;
    new_bytes = new_capacity * sa->item_size;

    if (sa->allocator != NULL) {
        if (sa->items == NULL && sa->allocator->alloc != NULL) {
            new_items = sa->allocator->alloc(new_bytes, sa->allocator->user);
            #ifdef WLX_PERF
                if (new_items != NULL) wlx_perf_note_alloc(new_bytes);
            #endif
        } else if (sa->allocator->realloc != NULL) {
            new_items = sa->allocator->realloc(sa->items, old_bytes, new_bytes,
                sa->allocator->user);
            #ifdef WLX_PERF
                if (new_items != NULL) wlx_perf_note_realloc(new_bytes);
            #endif
        } else if (sa->allocator->alloc != NULL) {
            new_items = sa->allocator->alloc(new_bytes, sa->allocator->user);
            #ifdef WLX_PERF
                if (new_items != NULL) wlx_perf_note_alloc(new_bytes);
            #endif
            if (new_items != NULL && sa->items != NULL && old_bytes > 0) {
                memcpy(new_items, sa->items, old_bytes);
                if (sa->allocator->free != NULL) {
                    #ifdef WLX_PERF
                        wlx_perf_note_free(sa->items);
                    #endif
                    sa->allocator->free(sa->items, old_bytes, sa->allocator->user);
                }
            }
        }
    } else {
        new_items = wlx_realloc(sa->items, new_bytes);
    }

    WLX_HARD_ASSERT(new_items != NULL, "Unable to allocate more RAM");
    sa->items = new_items;
    sa->capacity = new_capacity;
}

static inline size_t wlx_sub_arena_alloc(WLX_Sub_Arena *sa, size_t n) {
    size_t base;

    assert(sa != NULL);
    WLX_HARD_ASSERT(n == 0 || sa->count <= SIZE_MAX - n,
        "size_t overflow in sub-arena alloc count");

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
    WLX_HARD_ASSERT(sa->count <= SIZE_MAX - mask,
        "size_t overflow aligning sub-arena byte offset");
    aligned = (sa->count + mask) & ~mask;
    WLX_HARD_ASSERT(size <= SIZE_MAX - aligned,
        "size_t overflow in sub-arena byte size");
    needed = aligned + size;

    wlx_sub_arena_reserve(sa, needed);
    sa->count = needed;
    if (sa->count > sa->high_water) sa->high_water = sa->count;
    return aligned;
}

// Grow the arena so elements [0, needed) are live: reserves capacity,
// advances count to `needed` (never shrinks), and updates the high-water
// mark. For dynamic layouts that append at absolute offsets instead of
// bump-allocating from the current end.
static inline void wlx_sub_arena_extend_to(WLX_Sub_Arena *sa, size_t needed) {
    assert(sa != NULL);
    wlx_sub_arena_reserve(sa, needed);
    if (needed > sa->count) sa->count = needed;
    if (sa->count > sa->high_water) sa->high_water = sa->count;
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

#ifdef WLX_PERF
typedef enum {
    WLX_ARENA_SLOT_SIZE_OFFSETS,
    WLX_ARENA_DYN_OFFSETS,
    WLX_ARENA_SCRATCH,
    WLX_ARENA_COMMANDS,
    WLX_ARENA_CMD_RANGES,
    WLX_ARENA_LAYOUTS,
    WLX_ARENA_SCROLL_PANELS,
    WLX_ARENA_ID_STACK,
    WLX_ARENA_OPACITY_STACK,
    WLX_ARENA_GROUP_COUNT,
} WLX_Arena_Group;
#endif

typedef struct {
#define WLX_ARENA_FIELD(name, item_size, alloc_group) WLX_Sub_Arena name;
    WLX_ARENA_POOL_FIELDS(WLX_ARENA_FIELD)
#undef WLX_ARENA_FIELD
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
    WLX_CMD_SHADOW,   // deferred soft drop-shadow: draw_shadow callback or fallback
    WLX_CMD_GLOW,     // deferred outer glow: draw_glow callback or fallback
    WLX_CMD_GRADIENT_V, // deferred vertical two-stop gradient: draw_gradient_v callback or fallback
    WLX_CMD_TYPE_COUNT,
} WLX_Cmd_Type;

#ifdef WLX_PERF
typedef struct {
    uint64_t total_ns;
    uint64_t begin_ns;
    uint64_t input_ns;
    uint64_t user_build_ns;
    uint64_t end_ns;
    uint64_t range_accum_ns;
    uint64_t offset_lookup_ns;
    uint64_t dispatch_ns;
    uint64_t backend_callback_ns;
} WLX_Perf_Phase_Timings;

typedef struct {
    uint64_t total_commands;
    uint64_t command_ranges;
    uint64_t by_type[WLX_CMD_TYPE_COUNT];
} WLX_Perf_Command_Stats;

typedef struct {
    uint64_t measure_calls;
    uint64_t measured_bytes;
    uint64_t emitted_text_commands;
    uint64_t emitted_text_bytes;
    uint64_t fitted_text_runs;
} WLX_Perf_Text_Stats;

typedef struct {
    size_t count;
    size_t capacity;
    size_t high_water;
    size_t grow_count;
    size_t bytes_used;
    size_t bytes_capacity;
} WLX_Perf_Arena_Stats;

typedef struct {
    uint64_t frame_index;
    bool timer_available;
    WLX_Perf_Phase_Timings timings;
    WLX_Perf_Command_Stats commands;
    WLX_Perf_Text_Stats text;
    WLX_Perf_Arena_Stats arena[WLX_ARENA_GROUP_COUNT];
    WLX_Perf_Allocator_Stats allocator;
} WLX_Perf_Frame;
#endif

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
        struct { size_t text_off; size_t text_len; float x; float y; WLX_Text_Style style; } text;
        struct { WLX_Texture texture; WLX_Rect src; WLX_Rect dst; WLX_Color tint; } texture;
        struct { WLX_Rect rect; } scissor_begin;
        struct { WLX_Rect rect; WLX_Color color; float offset_x; float offset_y;
                 float blur; int layers; float roundness; int rounded_segs; } shadow;
        struct { WLX_Rect rect; WLX_Color color; float spread; int rings;
                 float roundness; int rounded_segs; } glow;
        struct { WLX_Rect rect; WLX_Color top; WLX_Color bottom;
                 float roundness; int rounded_segs; } gradient_v;
    } data;
} WLX_Cmd;

#define WLX_NO_RANGE (-1)

typedef struct WLX_Cmd_Range {
    size_t start_idx;
    size_t end_idx;
    float  dy_offset;
    float  dx_offset;
    int    layer;              // replay pass; higher layers draw over lower
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

// Shared caret/selection/gesture state for text-editing widgets. POD; the
// zero value is the default state. Embedded FIRST in each widget state so
// the zero-fill contract of wlx_get_state_impl covers it unchanged.
typedef struct {
    // Selection covers [min(anchor, cursor_pos), max(anchor, cursor_pos));
    // empty when both are equal. The anchor is the fixed end, the cursor the
    // moving end. Both are byte offsets into the widget's plain text.
    size_t cursor_pos;
    size_t selection_anchor;
    float cursor_blink_time;
    // Mouse selection: true while the press that started inside the field is
    // still held, so dragging keeps extending the selection.
    bool mouse_selecting;
    // Multi-click detection: seconds accumulated since the previous click,
    // the text offset it landed on, and the running click count (1 = caret,
    // 2 = word, 3 = select all).
    float last_click_time;
    size_t last_click_pos;
    int click_count;
    // Sticky column for UP/DOWN caret motion: the x position the caret aims
    // for on vertical moves, so traversing a shorter line does not lose the
    // column. Valid until the next horizontal caret change; the
    // vertical-motion path is the only setter.
    float preferred_x;
    bool preferred_x_valid;
    // Previous frame's caret offset: caret-follow compares against it to
    // detect motion and keep the caret inside the view.
    size_t prev_cursor_pos;
    // Scrollbar thumb drag gesture (primary axis).
    bool dragging_scrollbar;
    float sb_drag_offset;  // pointer offset from thumb start at drag start
} WLX_Text_Edit_State;

typedef struct {
    WLX_Text_Edit_State caret;
    // Vertical scroll (multiline): pixels of content hidden above the text
    // band, clamped to [0, content_h - band_h]. Wheel scrolling moves the
    // view alone; caret-follow drags it back to the caret line.
    float scroll_y;
    // Scrollbar visibility of the previous frame: seeds the wrap-mode
    // probe-width prediction for the line build, so a steadily overflowing
    // field pays one build per frame instead of two.
    bool sb_was_visible;
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
    bool pushed_scope;        // true when a scope id was pushed at begin; popped in wlx_scroll_panel_end

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

// Retained line-geometry entry: the per-unit prefix advances one hard
// line's build measured, kept across frames so unchanged lines rebuild
// their window records without backend measure calls. Keyed by the
// line's absolute byte span; offsets inside the entry are relative to
// line_start, so a widget edit shifts the key without touching the
// arrays. A key that no longer matches the live line index is a miss:
// the entry is dropped and rebuilt, never trusted.
typedef struct {
    size_t line_start;      // absolute hard line start (key)
    size_t line_next;       // absolute start of the next hard line, or the
                            // document length for the last line (key)
    uint32_t lru;           // last-touch stamp for eviction
    bool used;
    // No-wrap measure origin: the byte the stored advances are measured
    // from (0 = the line start), and the frozen content-space x of that
    // byte. On a line wider than the unit budget can reach, the origin
    // re-anchors near the view instead of the line start; tab stops
    // restart at the origin, the rule wrapped rows already apply at row
    // starts. origin_x is exact at 0, carried exactly across stitched
    // origin moves, and an average-advance estimate only after a far
    // jump onto unmeasured content - the documented x-space
    // approximation (byte offsets stay exact everywhere).
    uint32_t origin_rel;
    float origin_x;
    // Measured units in scan order. unit_ends[i] is the byte end of unit i
    // relative to line_start; advances[i] the measured prefix width from
    // the owning record's start (the origin unwrapped, the row start
    // wrapped); heights[i] the measured prefix height.
    size_t units;
    size_t unit_cap;
    uint32_t *unit_ends;
    float *advances;
    float *heights;
    uint32_t scan_rel;      // next unmeasured byte, relative to line_start
    uint32_t first_tab_rel; // first '\t' in [origin_rel, scan_rel);
                            // UINT32_MAX = none
    bool complete;          // the scan reached the line's separator or EOF
    // Wrapped-row table: row r spans units [row_units[r], row_units[r+1])
    // (the last row ends at units). Unused in no-wrap mode.
    size_t rows;
    size_t row_cap;
    uint32_t *row_units;
} WLX_Text_Geom_Entry;

// Measurement environment the retained geometry is valid for; any change
// clears the store (entries are cheap to rebuild, wrong geometry is not).
// Horizontal reach is deliberately not part of the key: no-wrap entries
// extend in place as the view scrolls deeper into a line.
typedef struct {
    WLX_Font font;
    int font_size;
    int spacing;
    float tab_advance;
    float band_w;
    float line_h;
    bool wrap;
    uint32_t transform_generation;  // ctx->style_transform_generation the entries were measured under
} WLX_Text_Geom_Env;

// Per-editor retained geometry store: a bounded, LRU-evicted set of line
// entries sized from the viewport. Allocations grow and are reused, never
// freed per frame, so steady-state frames allocate nothing.
typedef struct {
    WLX_Text_Geom_Entry *entries;
    size_t count;        // allocated slots in entries
    size_t want_cap;     // sizing target from the current viewport
    uint32_t lru_clock;
    WLX_Text_Geom_Env env;
    bool env_seen;
    // Last measured average unit advance across all entries (sticky,
    // survives clears): the far-jump origin estimate needs a unit width
    // before any unit of the target span is measured.
    float avg_advance;
} WLX_Text_Geom_Store;

// Editor line index: byte offsets of every hard line start of one editor's
// document, keyed by widget id. offsets[0] is always 0; a document ending in
// a newline separator owns a trailing empty line starting at the document
// length; count is the document line count (an empty document is one empty
// line). Context-owned, rebuilt by a newline scan when the per-frame guard
// detects a document change, freed with the context. Also home of the
// retained line-geometry store, which follows the index's document
// identity.
typedef struct {
    size_t id;        // widget id
    size_t *offsets;  // hard line start offsets, count entries
    size_t count;
    size_t cap;
    uint32_t rebuilds; // total rebuild count (guard/idle instrumentation)
    WLX_Text_Geom_Store geom;
} WLX_Editor_Line_Index;

typedef struct {
    WLX_Editor_Line_Index *items;
    size_t count;
    size_t capacity;
} WLX_Editor_Line_Index_Cache;

// Text undo journal: per-widget history of the buffer mutations made
// through the shared text-edit primitives, keyed by widget id and owned by
// the context. Each entry is one exact contiguous replace: the removed_len
// original bytes at start (kept in the stack's arena) were replaced by the
// inserted_len bytes now at start. Entries push in edit order and revert
// last-in-first-out, so an entry's coordinates are valid at the moment it
// is reverted. Entries sharing a group id form one undo step. Inserted
// bytes are never stored: reverting an insert is a delete, and that
// delete's own recording keeps the bytes for redo.
typedef enum {
    WLX_TEXT_UNDO_CLS_NONE = 0,
    WLX_TEXT_UNDO_CLS_TYPING,       // text-input inserts (and the selection they replace)
    WLX_TEXT_UNDO_CLS_BACKSPACE,    // codepoint backspace
    WLX_TEXT_UNDO_CLS_DELETE,       // codepoint forward delete
    WLX_TEXT_UNDO_CLS_SELECTION,    // selection delete under Backspace/Delete
    WLX_TEXT_UNDO_CLS_WORD_DELETE,  // word-granularity Backspace/Delete
    WLX_TEXT_UNDO_CLS_NEWLINE,
    WLX_TEXT_UNDO_CLS_TAB,
    WLX_TEXT_UNDO_CLS_PASTE,
    WLX_TEXT_UNDO_CLS_CUT,
    WLX_TEXT_UNDO_CLS_REPLAY        // recorded while an undo or redo replays
} WLX_Text_Undo_Class;

typedef struct {
    uint32_t group;        // undo step id; equal ids revert together
    uint8_t cls;           // WLX_Text_Undo_Class of the recording path
    size_t start;          // byte offset of the replaced range
    size_t removed_len;    // original bytes removed, stored in the arena
    size_t inserted_len;   // bytes inserted at start, live in the document
    size_t arena_off;      // offset of the removed bytes in the arena
    size_t caret_before;   // caret pair before the edit
    size_t anchor_before;
    size_t caret_after;    // caret pair after the edit
    size_t anchor_after;
} WLX_Text_Undo_Entry;

// One direction's history: entries and their removed bytes in push order
// (the newest entry's bytes end the arena, so a pop is O(1) and the newest
// entry can grow at either end). Bounded by WLX_TEXT_UNDO_ENTRIES and
// WLX_TEXT_UNDO_BYTES with whole oldest steps evicted first; a single step
// larger than either cap is admitted and the arrays grow to hold it.
// Neither array shrinks before destroy.
typedef struct {
    WLX_Text_Undo_Entry *entries;
    size_t count;
    size_t cap;
    char *arena;
    size_t arena_used;
    size_t arena_cap;
} WLX_Text_Undo_Stack;

typedef struct {
    size_t id;              // widget id
    uint32_t touch;         // cache clock at the last lookup (eviction aid)
    WLX_Text_Undo_Stack undo;
    WLX_Text_Undo_Stack redo;
    uint32_t next_group;    // last step id handed out
    // Staleness guard: the document length after the last recorded or
    // replayed mutation and the caller revision seen with it. A mismatch
    // at lookup means the buffer changed outside the widget: the history
    // is dropped rather than applied to bytes it never saw.
    size_t expected_len;
    uint32_t revision_seen;
    bool guard_seen;
    // Transaction: one key-handler invocation. Every primitive call inside
    // records under txn_group; the first record decides whether the
    // transaction continues the newest entry's step (coalescing) or opens
    // a new one. replaying routes recordings to the opposite stack.
    bool in_txn;
    bool replaying;
    bool txn_first;
    bool last_was_replay;
    uint8_t cls;            // class the current handler path declared
    uint32_t txn_group;
    size_t txn_caret0;      // caret pair at transaction start
    size_t txn_anchor0;
    bool alloc_failed;      // a record could not be kept; the step is void
    // Replay: the direction being applied and the caret pairs its mirror
    // entries carry (the step's own, not the replay's working carets).
    bool replay_redo;
    size_t rep_caret_before;
    size_t rep_anchor_before;
    size_t rep_caret_after;
    size_t rep_anchor_after;
} WLX_Text_Undo_Journal;

typedef struct {
    WLX_Text_Undo_Journal *items;
    size_t count;
    size_t capacity;
    uint32_t clock;
} WLX_Text_Undo_Cache;

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
#define WLX_ARENA_INIT(name, item_size, alloc_group) wlx_sub_arena_init(&pool->name, (item_size), (alloc_group));
    WLX_ARENA_POOL_FIELDS(WLX_ARENA_INIT)
#undef WLX_ARENA_INIT
}

static inline void wlx_arena_pool_reset(WLX_Arena_Pool *pool) {
    assert(pool != NULL);
#define WLX_ARENA_RESET(name, item_size, alloc_group) wlx_sub_arena_reset(&pool->name);
    WLX_ARENA_POOL_FIELDS(WLX_ARENA_RESET)
#undef WLX_ARENA_RESET
}

static inline void wlx_arena_pool_destroy(WLX_Arena_Pool *pool) {
    assert(pool != NULL);
#define WLX_ARENA_DESTROY(name, item_size, alloc_group) wlx_sub_arena_destroy(&pool->name);
    WLX_ARENA_POOL_FIELDS(WLX_ARENA_DESTROY)
#undef WLX_ARENA_DESTROY
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

#ifndef WLX_SCISSOR_STACK_MAX
#define WLX_SCISSOR_STACK_MAX 64
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

// Option-field unset rules (the full taxonomy is docs/SENTINEL.md):
//   - Rule Z: zero is unset. Only for fields where zero can never be a
//     meaningful explicit value: colors ({0}), fonts, pointers and ids,
//     flags that default to false, and numbers whose domain excludes zero
//     (font_size, track_height, shadow_blur, min/max sizes, ...).
//   - Rule U: WLX_UNSET is unset. For every numeric field where zero is a
//     meaningful explicit value (border widths, roundness, opacity, width
//     and height, content padding, brightness shifts, ...). The option
//     macro installs WLX_UNSET; the resolver replaces it with the field's
//     documented fallback and passes every value >= 0 through unchanged.
//     Brightness shifts are the one signed-domain Rule U family: their
//     domain is -1 < b <= 1 and -1 is the sentinel.
//   - Everything else is a literal default installed by the macro and used
//     as written (span, wrap, the align fields, container chrome, knobs).
// A designator follows the same rule in every option struct. New fields
// must follow these rules; the request tokens below are explicit values,
// not unset markers.
#define WLX_UNSET (-1)

// Deprecated alias of WLX_UNSET for the brightness fields (their sentinel
// was a large negative float before v0.9); still resolved as unset. Removed
// in the first minor release after 0.9.
#define WLX_FLOAT_UNSET (-1e30f)

// Request token for WLX_CONTENT_PADDING_FIELDS uniform `content_padding`.
// Pass `.content_padding = WLX_PADDING_USE_THEME` to opt a widget into the
// project-wide inner padding knob (WLX_Theme.padding, defaulting to
// WLX_STYLE_CONTENT_PADDING). Numerically distinct from WLX_UNSET so the
// resolver can tell "unset" apart from "explicit theme request".
#define WLX_PADDING_USE_THEME (-2.0f)

// Sentinel for WLX_Parent_Contribution slot_index / grid_row: skips the
// per-slot CONTENT or per-row grid bucket update in the parent layout
// while still contributing to accumulated_content_height.
#define WLX_SLOT_SKIP SIZE_MAX

typedef struct {
    // --- Global colors ---
    WLX_Color background;        // window / panel clear color
    WLX_Color foreground;        // default text color (front_color)
    WLX_Color surface;           // widget background (back_color for buttons, etc.)
    WLX_Color border;            // default border color
    float     border_width;      // default border width (0 = no border)
    WLX_Color accent;            // active / focused accent (fill bar, focus ring)

    // --- Text ---
    WLX_Font font;               // default font for all widgets (0 = backend default)
    int   font_size;            // default font size for all widgets

    // --- Geometry ---
    float padding;              // default inner padding for layout slots
    float roundness;            // default corner roundness (0 = sharp)
    int   rounded_segments;     // segment count for rounded drawing
    int   min_rounded_segments; // minimum segment floor for fully-round widgets (0 = no minimum)

    // --- Interaction feedback ---
    float hover_brightness;     // brightness shift on hover
    float disabled_brightness;  // brightness shift applied when a widget is disabled (WLX_UNSET = no shift)

    // --- Opacity ---
    float opacity;              // global opacity multiplier (<0 = unset sentinel, 0.0-1.0 = explicit)
    float disabled_opacity;     // alpha multiplier applied when a widget is disabled (<0 = unset sentinel, 0.0-1.0 = explicit)

    // --- Widget-specific overrides (zero = use globals) ---
    struct {
        WLX_Color border_focus;  // {0} -> derive from accent
        WLX_Color cursor;        // {0} -> use foreground
        WLX_Color selection;     // {0} -> derive from accent (translucent)
        float     border_width;  // 0 -> use global border_width
    } input;

    struct {
        WLX_Color track;         // {0} -> derive from surface
        WLX_Color thumb;         // {0} -> use foreground
        WLX_Color label;         // {0} -> use foreground
        float    track_height;  // 0 -> default (6)
        float    thumb_width;   // 0 -> default (14); also default visual height
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
        float     segment_gap;   // default inter-cell gap; 0 -> 2px fallback (segmented mode)
    } progress;

    struct {
        WLX_Color bar;           // scrollbar thumb color
        float    width;         // scrollbar width (0 -> default 10)
    } scrollbar;

    struct {
        float blur;   // 0 -> 8px fallback
        int   layers; // 0 -> 4 fallback
    } shadow;

    struct {
        float spread; // 0 -> 4px fallback
        int   rings;  // 0 -> 3 fallback
    } glow;
} WLX_Theme;

// Built-in theme presets (defined in WOLLIX_IMPLEMENTATION section)
extern const WLX_Theme wlx_theme_dark;
extern const WLX_Theme wlx_theme_light;
extern const WLX_Theme wlx_theme_glass;

// Helper: check if a color is the sentinel (all-zero = "use theme default")
static inline bool wlx_color_is_zero(WLX_Color c) {
    return c.r == 0 && c.g == 0 && c.b == 0 && c.a == 0;
}

// Helper: exact component-wise color equality.
static inline bool wlx_color_eq(WLX_Color a, WLX_Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// Returns true when x carries WLX_UNSET on a Rule U field whose domain is
// non-negative (border_width, roundness, scrollbar_width, opacity, padding,
// etc.): any negative value is unset.
static inline bool wlx_is_negative_unset(float x) {
    return x < 0;
}

// Returns true when x carries WLX_UNSET on a signed-domain Rule U field
// (hover_brightness, thumb_hover_brightness, scrollbar_hover_brightness,
// disabled_brightness): -1 and anything below it, which keeps the deprecated
// WLX_FLOAT_UNSET value resolving as unset.
static inline bool wlx_is_float_unset(float x) {
    return x <= -1.0f;
}

static inline WLX_Color wlx_color_or(WLX_Color a, WLX_Color b) {
    return wlx_color_is_zero(a) ? b : a;
}

// One interactive query's hit candidate. The previous frame's candidates
// arbitrate this frame's press/hover ownership: the highest layer wins,
// then the latest query (array order). rect is container-clipped at record
// time (scroll panel viewports, .clip layouts, a popup's own rect) so
// scrolled-away or clipped-away widgets cannot own the pointer.
typedef struct {
    size_t   id;
    WLX_Rect rect;
    int      layer;
    uint8_t  cursor;    // WLX_Cursor_Shape the widget wants while it owns the pointer
    bool     focusable; // a Tab stop: FOCUS-class, or CLICK+KEYBOARD, and not TAB_SKIP
} WLX_Interaction_Candidate;

typedef struct {
    WLX_Interaction_Candidate *items;
    size_t count;
    size_t capacity;
} WLX_Candidate_List;

// Persistent per-id menu state. item_count is last frame's item tally; it
// sizes the menu chrome, which therefore adapts one frame after the item
// list changes.
typedef struct {
    int  item_count;
    bool was_open;   // menu body was built last frame (guards the open-press frame)
} WLX_Menu_State;

// Frame-transient bookkeeping for one open wlx_menu_begin/end pair. Defined
// with the menu implementation (it embeds WLX_Menu_Opt, which is declared
// with the menu API); the context holds a heap-backed stack of them.
typedef struct WLX_Menu_Frame WLX_Menu_Frame;

#ifndef WLX_MENU_STACK_MAX
#define WLX_MENU_STACK_MAX 4
#endif

typedef struct WLX_Context {
    WLX_Rect rect;
    WLX_Backend backend;
    // Style transform (wlx_set_style_transform); NULL = identity. The
    // generation counts every set so retained text geometry measured
    // under an earlier transform is dropped.
    WLX_Style_Transform_Fn style_transform;
    void    *style_transform_user;
    uint32_t style_transform_generation;
    WLX_Input_State input;

    // Frame delta seconds, sampled from backend.get_frame_time exactly once
    // per frame in wlx_begin; every wlx_get_frame_time read serves this.
    float frame_dt;

    // Widget interaction state (hot = hovered, active = pressed/focused)
    struct {
        size_t hot_id;
        size_t active_id;
        bool   active_id_seen; // true if any widget matched active_id this frame
        bool   enter_consumed; // Enter already used this frame (focus blur or newline insert); blocks keyboard activation
        // Frame-begin ownership arbitration (computed from the previous
        // frame's candidate list): press_owner is the topmost candidate
        // under a fresh press, latched until mouse release; hot_id holds
        // the hover owner. arbitrate is false only when the previous frame
        // recorded no candidates (first frame of a context) - acquisition
        // then falls back to query-time capture.
        bool     arbitrate;
        size_t   press_owner;
        size_t   right_press_owner; // topmost candidate under a fresh right press; read on the press frame (right_clicked), latched until release so a right-drag could own it; never touches focus or hot
        int      pointer_layer;     // layer of the pointer's topmost candidate (0 when none): the wheel belongs to this layer
        bool     press_claimed;     // set when this frame's press owner's own query runs; popups snapshot it around their subtree to detect outside presses
        bool     active_is_focus;   // active_id holder is focus-class (inputbox/editor)
        size_t   focus_released_id; // holder released at frame begin; it still reports just_unfocused
        // Keyboard (Tab) focus - a third identity beside hot and active.
        // Traversal runs at frame begin over the previous frame's focusable
        // candidates; FOCUS-class targets bridge into active_id through
        // focus_gained_id so Tab into a field focuses it for typing.
        size_t   focus_id;            // keyboard-focused widget; 0 = none
        size_t   focus_gained_id;     // one-shot: traversal landed here this frame
        bool     focus_id_seen;       // focus_id holder was queried this frame (GC mirror of active_id_seen)
        bool     active_consumes_tab; // active_id holder was queried with FOCUS_HOLD_TAB (editor): Tab inserts, no traversal
        bool     tab_consumed;        // one-shot: traversal ate this frame's Tab; text edit must not also insert
        WLX_Rect focus_rect;          // clipped rect recorded at the focused widget's query (focus ring geometry)
        WLX_Rect focus_clip;          // container clip at that query; the ring is clamped to it
        bool     focus_clip_active;   // focus_clip holds a clip (false when no container clips the widget)
    } interaction;

    // Double-buffered interaction candidates: cands[cand_frame & 1] collects
    // this frame's queries; the other buffer holds the previous frame's list
    // for ownership arbitration. Grow-and-reuse; freed in
    // wlx_context_destroy.
    WLX_Candidate_List cands[2];
    int cand_frame;

    // Open wlx_menu_begin/end pairs (innermost last): WLX_MENU_STACK_MAX
    // entries, allocated on the first menu push (never reallocated, so
    // frame pointers stay valid for the frame) and freed in
    // wlx_context_destroy. NULL until a menu opens.
    WLX_Menu_Frame *menu_stack;
    int menu_stack_count;

    // Rect of the most recent widget placed this frame (see wlx_last_rect).
    WLX_Rect last_widget_rect;

    // Cursor shape last pushed to backend.set_cursor (zero = ARROW = the
    // platform default, so no first-frame push is needed).
    uint8_t cursor_applied;

    // Per-frame buffer pool. Owns layouts, commands, cmd_ranges, scratch,
    // slot offsets, scroll-panel stack, id stack, and opacity stack.
    WLX_Arena_Pool arena;

    WLX_State_Map states;

    // Text line-record scratch: one context-owned, lazily grown buffer that
    // the inputbox geometry build borrows each frame instead of stacking its
    // own array. Single borrower at a time (never lend it across a nested
    // text-layout call). Freed in wlx_context_destroy.
    struct WLX_Text_Line_Record *text_line_scratch;
    size_t text_line_scratch_cap;  // capacity in records

    // Per-editor line indices (one entry per editor widget id). Freed in
    // wlx_context_destroy.
    WLX_Editor_Line_Index_Cache editor_indices;

    // Per-widget text undo journals (inputbox, textarea, editor), keyed by
    // widget id. Freed in wlx_context_destroy.
    WLX_Text_Undo_Cache text_undo;

    // Auto scroll panel content height tracking
    struct {
        size_t panel_id;      // 0 = not tracking
        float  total_height;
    } auto_scroll;

    int current_range_idx;     // active range during recording, -1 when none
    int current_layer;         // layer stamped onto newly opened ranges (0 = base)
    bool immediate_mode;       // true = dispatch directly (today's behavior)
    bool cull_offscreen;       // true = skip recording rect-bounded draw commands
                               // fully outside the active clip (deferred path only)

    WLX_Rect scissor_stack[WLX_SCISSOR_STACK_MAX];
    size_t scissor_stack_count;

    // The clip context of the current layer. Base-layer content sees every
    // scroll panel, clip layout and scissor scope opened so far; an overlay
    // starts a fresh context at its own rect (escaping the base clips is the
    // point of an overlay) and wlx_overlay_end restores the enclosing one.
    // Every walker that intersects "all active clips" - for drawing or for
    // hit-testing - iterates from this base (wlx_enclosing_clip).
    WLX_Clip_Base clip_base;

    // Theme - NULL means use &wlx_theme_dark (set automatically in wlx_begin)
    const WLX_Theme *theme;

    // Debug context - heap-allocated under WLX_DEBUG, NULL in release builds.
    // See the debug implementation section at the end of this file.
    struct WLX_Debug_Context *dbg;

    // Perf context - heap-allocated under WLX_PERF, NULL otherwise.
    // See the perf implementation section at the end of this file.
    struct WLX_Perf_Context *perf;
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

static inline float *wlx_layout_content_measures(const WLX_Context *ctx, const WLX_Layout *l) {
    return wlx_pool_slot_size_offsets(ctx) + l->content_slot_measures_off;
}

static inline float *wlx_grid_row_content_heights(const WLX_Context *ctx, const WLX_Layout *l) {
    return wlx_pool_slot_size_offsets(ctx) + l->grid_row_content_heights_off;
}

// Slot-size array a layout retained at begin (CONTENT pre-resolution or the
// fixed-kind retention path), re-derived from its byte-scratch offset on
// every read: the scratch sub-arena is realloc-grown, so any pointer taken
// at layout_begin may be stale by the time a child reads it. NULL when the
// layout retained no array. This is the only read path.
static inline const WLX_Slot_Size *wlx_layout_content_sizes(const WLX_Context *ctx, const WLX_Layout *l) {
    if (!l->has_content_sizes) return NULL;
    return (const WLX_Slot_Size *)&wlx_pool_scratch(ctx)[l->content_sizes_scratch_off];
}

// Axis predicates for the "linear and HORZ / VERT" tests the layout core
// repeats; a grid answers false to both.
static inline bool wlx_layout_is_horz(const WLX_Layout *l) {
    return l->kind == WLX_LAYOUT_LINEAR && l->linear.orient == WLX_HORZ;
}
static inline bool wlx_layout_is_vert(const WLX_Layout *l) {
    return l->kind == WLX_LAYOUT_LINEAR && l->linear.orient == WLX_VERT;
}

// A linear layout's extent along its main axis.
static inline float wlx_layout_main_extent(const WLX_Layout *l) {
    assert(l->kind == WLX_LAYOUT_LINEAR && "main extent is a linear-layout notion");
    return (l->linear.orient == WLX_HORZ) ? l->rect.w : l->rect.h;
}

// True when slot `slot` of linear layout l is a tracked CONTENT slot:
// content sizes recorded, the slot inside both the layout and the measure
// table, and declared CONTENT.
static inline bool wlx_layout_slot_is_content(const WLX_Context *ctx, const WLX_Layout *l, size_t slot) {
    if (!l->has_content_slot_measures || slot >= l->count || slot >= WLX_CONTENT_SLOTS_MAX) return false;
    const WLX_Slot_Size *sizes = wlx_layout_content_sizes(ctx, l);
    return sizes != NULL && sizes[slot].kind == WLX_SIZE_CONTENT;
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

#ifdef WLX_PERF
    #define WLX_PERF_HOOK(fn, ...) wlx_perf_##fn(__VA_ARGS__)
#else
    #define WLX_PERF_HOOK(fn, ...) ((void)0)
#endif

// Wrap one backend callback body in a timed scope against an adapter's
// WLX_Perf_Backend_Clock: BEGIN declares the start sample, END (one per
// exit path) accumulates into the given duration field. Outside WLX_PERF
// both expand to no-ops and their arguments are never evaluated, so call
// sites may name perf-only symbols unconditionally. A perf build that
// opens a scope and never closes one leaves the start sample unused and
// -Wall reports it.
#ifdef WLX_PERF
    #define WLX_PERF_SCOPE_BEGIN(clock) \
        uint64_t wlx_perf_scope_start_ns_ = wlx_perf_backend_time_begin(clock)
    #define WLX_PERF_SCOPE_END(clock, total_ptr) \
        wlx_perf_backend_time_end((clock), wlx_perf_scope_start_ns_, (total_ptr))
#else
    #define WLX_PERF_SCOPE_BEGIN(clock) ((void)0)
    #define WLX_PERF_SCOPE_END(clock, total_ptr) ((void)0)
#endif

// ============================================================================
// Interaction flags and interaction results
// ============================================================================

// Interaction flags for wlx_get_interaction()
// Combine with bitwise OR to specify desired interaction behavior.
typedef enum {
    WLX_INTERACT_HOVER       = 1 << 0,  // Hover detection (sets hot_id)
    WLX_INTERACT_CLICK       = 1 << 1,  // Click-to-activate (button-like: press, release while hovering = clicked)
    WLX_INTERACT_FOCUS       = 1 << 2,  // Click-to-focus (input-like: stays focused until click elsewhere, Escape, or Enter (unless FOCUS_HOLD_ENTER))
    WLX_INTERACT_DRAG        = 1 << 3,  // Click-to-drag (slider-like: active while mouse held after click)
    WLX_INTERACT_KEYBOARD    = 1 << 4,  // Keyboard activation (Space/Enter while hot or keyboard-focused triggers clicked)
    WLX_INTERACT_FOCUS_HOLD_ENTER = 1 << 5,  // Modifies FOCUS: Enter does not blur (multiline input); inert without FOCUS
    WLX_INTERACT_TEXT_CURSOR = 1 << 6,  // Show the I-beam while this rect owns the pointer (text-editing surfaces)
    WLX_INTERACT_FOCUS_HOLD_TAB = 1 << 7,  // Modifies FOCUS: while focused, Tab stays with the widget (editor indent) instead of traversing
    WLX_INTERACT_TAB_SKIP = 1 << 8,  // Never a Tab stop (decoration widgets that query interaction but are not operable)
} WLX_Interact_Flags;

typedef struct {
    size_t id;
    bool hover;          // Mouse is over widget
    bool pressed;        // Mouse is currently down on this widget
    bool clicked;        // Click completed (CLICK mode) or keyboard activated
    bool right_clicked;  // Right press landed here this frame (press-frame edge, topmost-wins)
    bool focused;        // Has focus (FOCUS mode)
    bool active;         // Is the active widget (being pressed, dragged, or focused)
    bool just_focused;   // Became focused this frame
    bool just_unfocused; // Lost focus this frame
    bool disabled;       // Widget was queried with the disabled gate; clicked/pressed/focused/active are forced false
} WLX_Interaction;

typedef void (*WLX_Input_Handler)(WLX_Context *ctx);

// ============================================================================
// Core API declarations
// ============================================================================

// draw_circle and draw_ring are intentionally excluded from this check: they are optional.
// A NULL callback falls back to draw_rect_rounded / draw_rect_rounded_lines respectively.
// Text callbacks are accepted in either form per direction: the slice entry
// points (draw_text_slice / measure_text_slice) are the preferred contract;
// the NUL-terminated legacy pair remains for compatibility. A backend may
// implement only one form of each.
static inline bool wlx_backend_is_ready(const WLX_Context *ctx) {
    return ctx != NULL &&
        ctx->backend.draw_rect != NULL &&
        ctx->backend.draw_rect_lines != NULL &&
        ctx->backend.draw_rect_rounded != NULL &&
        ctx->backend.draw_rect_rounded_lines != NULL &&
        ctx->backend.draw_line != NULL &&
        (ctx->backend.draw_text != NULL || ctx->backend.draw_text_slice != NULL) &&
        (ctx->backend.measure_text != NULL || ctx->backend.measure_text_slice != NULL) &&
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
// Current vertical scroll offset of the innermost active scroll panel, in
// pixels (0 when no panel is active). Pairs with wlx_get_scroll_panel_viewport
// so callers can compute which rows of a long list are on screen.
WLXDEF float wlx_get_scroll_panel_offset(WLX_Context *ctx);
// Rect of the most recent widget placed this frame - the natural
// wlx_tooltip_for anchor right after a widget call. {0} before the first
// widget of a frame; a dropdown and a menu_button block report their face,
// inside an open menu body it reports the latest item (see the definition).
WLXDEF WLX_Rect wlx_last_rect(WLX_Context *ctx);

WLXDEF bool wlx_is_key_down(WLX_Context *ctx, WLX_Key_Code key);
WLXDEF bool wlx_is_key_pressed(WLX_Context *ctx, WLX_Key_Code key);
// True when the key edged this frame OR fired an OS auto-repeat tick. Edit and
// navigation widgets use this so held keys (backspace, delete, arrows) repeat.
WLXDEF bool wlx_is_key_actuated(WLX_Context *ctx, WLX_Key_Code key);
// True when all WLX_Key_Mod bits in mask are active this frame.
WLXDEF bool wlx_mod_down(WLX_Context *ctx, uint32_t mask);
// True when the platform "command" modifier for editing shortcuts is down:
// SUPER (Cmd) on Apple platforms, CTRL elsewhere.
WLXDEF bool wlx_mod_command_down(WLX_Context *ctx);
// Id of the keyboard-focused widget (Tab traversal), 0 when none. Compare
// against WLX_Interaction.id. Independent of hot (hover) and active (press /
// drag / typing focus): a Tab-focused inputbox is both focused and active; a
// Tab-focused button is focused only and activates on Enter/Space.
WLXDEF size_t wlx_focused_id(WLX_Context *ctx);
// Right/middle mouse button state. The *_clicked variants are one-frame
// press edges, set by the backend on the frame the button goes down.
WLXDEF bool wlx_is_mouse_right_down(WLX_Context *ctx);
WLXDEF bool wlx_is_mouse_right_clicked(WLX_Context *ctx);
WLXDEF bool wlx_is_mouse_middle_down(WLX_Context *ctx);
WLXDEF bool wlx_is_mouse_middle_clicked(WLX_Context *ctx);
// Set the system clipboard to a UTF-8 byte span. No-op when the backend
// installs no clipboard_set hook. The hook copies the bytes; it never retains
// the caller's pointer.
WLXDEF void wlx_clipboard_set_text(WLX_Context *ctx, const char *text, size_t len);
// Copy the system clipboard text into out (NUL-terminated), truncated to
// out_size on a UTF-8 codepoint boundary. Returns the number of bytes written
// (excluding the NUL). Zero when no clipboard_get hook is installed or the
// clipboard is empty. The hook returns a borrowed pointer; this copies out of
// it immediately.
WLXDEF size_t wlx_clipboard_get_copy(WLX_Context *ctx, char *out, size_t out_size);
// Float-precision point-in-rect test on a WLX_Rect; the canonical geometry
// query. wlx_point_in_rect is the legacy int-based shim around it.
WLXDEF bool wlx_rect_contains(WLX_Rect r, float px, float py);
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

// Opt-in (default off): when enabled, the deferred recorder skips rect-bounded
// draw commands that lie fully outside the active clip, trimming command-buffer
// size and backend replay for over-drawn lists/panels. Output is unchanged
// because only provably-invisible commands are dropped. Immediate mode is
// unaffected (it relies on the backend scissor).
WLXDEF void wlx_set_cull_offscreen(WLX_Context *ctx, bool enabled);

// Install (or clear, with NULL) the context's style transform. Every call
// counts as a measurement-environment change: retained text geometry
// measured under the previous transform is rebuilt. Change the transform's
// behaviour only through this call; mutating `user` behind it leaves
// retained geometry stale.
WLXDEF void wlx_set_style_transform(WLX_Context *ctx, WLX_Style_Transform_Fn fn, void *user);

#ifdef WLX_PERF
WLXDEF void wlx_perf_set_timer(WLX_Context *ctx, WLX_Perf_Timestamp_Fn timestamp_fn, void *user);
WLXDEF const WLX_Perf_Frame *wlx_perf_get_last_frame(const WLX_Context *ctx);
WLXDEF void wlx_perf_reset(WLX_Context *ctx);
#endif

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
    .pos = WLX_UNSET, .span = 1, .overflow = false, .padding = 0, \
    .padding_top = WLX_UNSET, .padding_right = WLX_UNSET, \
    .padding_bottom = WLX_UNSET, .padding_left = WLX_UNSET

// Field-group copy initializers: `WLX_*_COPY(src)` expands to the designated
// initializers that copy one field group from `src` (any option struct that
// declares the group) into another option struct. Kept beside each group's
// FIELDS / DEFAULTS so a new field is added in all three places at once.
// Used where a compound widget hands part of its options to an inner widget
// (the dropdown and menu-button faces build the WLX_Button_Opt their shared
// face draws).
#define WLX_LAYOUT_SLOT_COPY(src) \
    .pos = (src).pos, .span = (src).span, .overflow = (src).overflow, \
    .padding = (src).padding, \
    .padding_top = (src).padding_top, .padding_right = (src).padding_right, \
    .padding_bottom = (src).padding_bottom, .padding_left = (src).padding_left

// Soft drop-shadow decoration fields, shared by widgets and containers. A
// zero shadow_color disables the effect; the numeric knobs fall back to theme
// defaults (and then hard-coded fallbacks) when left at 0 / <= 0, so a
// zero-initialized option emits no shadow.
#define WLX_SHADOW_FIELDS \
    WLX_Color shadow_color;    /* {0} -> no shadow (disabled) */ \
    float     shadow_offset_x; /* px right; literal 0 = no horizontal shift */ \
    float     shadow_offset_y; /* px down;  literal 0 = no vertical shift */ \
    float     shadow_blur;     /* approx blur radius px; <= 0 -> theme default */ \
    int       shadow_layers    /* fallback layer count; 0 -> theme default */
#define WLX_SHADOW_DEFAULTS \
    .shadow_color = {0}, .shadow_offset_x = 0, .shadow_offset_y = 0, \
    .shadow_blur = 0, .shadow_layers = 0
#define WLX_SHADOW_COPY(src) \
    .shadow_color = (src).shadow_color, .shadow_offset_x = (src).shadow_offset_x, \
    .shadow_offset_y = (src).shadow_offset_y, .shadow_blur = (src).shadow_blur, \
    .shadow_layers = (src).shadow_layers

// Soft outer-glow (halo) decoration fields, shared by widgets and containers.
// A zero glow_color disables the effect; spread / rings fall back to theme
// defaults when 0 / <= 0.
#define WLX_GLOW_FIELDS \
    WLX_Color glow_color;  /* {0} -> no glow (disabled) */ \
    float     glow_spread; /* outward growth per ring px; <= 0 -> theme default */ \
    int       glow_rings   /* fallback ring count; 0 -> theme default */
#define WLX_GLOW_DEFAULTS \
    .glow_color = {0}, .glow_spread = 0, .glow_rings = 0
#define WLX_GLOW_COPY(src) \
    .glow_color = (src).glow_color, .glow_spread = (src).glow_spread, \
    .glow_rings = (src).glow_rings

// Vertical two-stop gradient fill fields, shared by widgets and containers.
// A zero gradient_top disables the gradient (the element renders its solid
// fill); a zero gradient_bottom is treated as equal to gradient_top (uniform
// fill). The gradient replaces the solid fill, never the border.
#define WLX_GRADIENT_FIELDS \
    WLX_Color gradient_top;    /* {0} -> no gradient; fall back to solid fill */ \
    WLX_Color gradient_bottom  /* {0} -> treated as gradient_top (solid)       */
#define WLX_GRADIENT_DEFAULTS \
    .gradient_top = {0}, .gradient_bottom = {0}
#define WLX_GRADIENT_COPY(src) \
    .gradient_top = (src).gradient_top, .gradient_bottom = (src).gradient_bottom

// Software-fallback band height for vertical gradients: the rect is sliced into
// max(1, rect.h / this) solid bands interpolating the two stops. Compile-time
// constant; not a theme knob in v1.
#define WLX_GRADIENT_FALLBACK_BAND_HEIGHT_PX 4.0f

// Corner selection mask for per-corner rounding. The radius comes from
// corner_radius / roundness; this mask selects which corners use it. A zero
// (unset) mask is treated as WLX_CORNERS_ALL, so existing call sites round all
// four corners exactly as before. Corners absent from the mask are squared off.
typedef enum {
    WLX_CORNER_TOP_LEFT     = 1 << 0,
    WLX_CORNER_TOP_RIGHT    = 1 << 1,
    WLX_CORNER_BOTTOM_RIGHT = 1 << 2,
    WLX_CORNER_BOTTOM_LEFT  = 1 << 3,
} WLX_Corner;

#define WLX_CORNERS_ALL    (WLX_CORNER_TOP_LEFT | WLX_CORNER_TOP_RIGHT | \
                            WLX_CORNER_BOTTOM_RIGHT | WLX_CORNER_BOTTOM_LEFT)
#define WLX_CORNERS_TOP    (WLX_CORNER_TOP_LEFT | WLX_CORNER_TOP_RIGHT)
#define WLX_CORNERS_BOTTOM (WLX_CORNER_BOTTOM_LEFT | WLX_CORNER_BOTTOM_RIGHT)
#define WLX_CORNERS_LEFT   (WLX_CORNER_TOP_LEFT | WLX_CORNER_BOTTOM_LEFT)
#define WLX_CORNERS_RIGHT  (WLX_CORNER_TOP_RIGHT | WLX_CORNER_BOTTOM_RIGHT)

#define WLX_CONTAINER_DECOR_FIELDS \
    WLX_Color back_color; \
    WLX_Color border_color; \
    float     border_width; \
    float     roundness; \
    float     corner_radius;   /* > 0 -> absolute px radius, overrides roundness; 0 -> unset */ \
    int       rounded_segments; \
    int       rounded_corners; /* WLX_CORNERS_* mask; 0 -> all corners rounded */ \
    float     gap; \
    WLX_Color border_color_top;    WLX_Color border_color_right; \
    WLX_Color border_color_bottom; WLX_Color border_color_left; \
    float     border_width_top;    float     border_width_right; \
    float     border_width_bottom; float     border_width_left; \
    WLX_SHADOW_FIELDS; \
    WLX_GLOW_FIELDS; \
    WLX_GRADIENT_FIELDS; \
    /* Interaction opt-in + hover-variant chrome (color-only, replace */ \
    /* semantics). interact == 0 keeps the container non-interactive with no */ \
    /* overhead. When interact != 0, begin computes the interaction on the */ \
    /* container rect and, if interact_out is non-NULL, writes the result. */ \
    /* Each non-zero hover_* color replaces its base color while hovered; a */ \
    /* {0} twin leaves the base color unchanged. Widths/gradient/shadow/glow */ \
    /* have no hover variant. */ \
    uint32_t         interact;       /* WLX_Interact_Flags; 0 -> non-interactive */ \
    WLX_Interaction *interact_out;   /* non-NULL -> begin writes the result here */ \
    WLX_Color hover_back_color; \
    WLX_Color hover_border_color; \
    WLX_Color hover_border_color_top;    WLX_Color hover_border_color_right; \
    WLX_Color hover_border_color_bottom; WLX_Color hover_border_color_left

// Container chrome defaults. Layouts, grids and slot styles have no theme
// chrome, so their border width, roundness and segment count are literal
// zeros; a panel is a widget with theme chrome and installs WLX_UNSET for
// the same three fields through WLX_CONTAINER_DECOR_DEFAULTS_CHROME.
#define WLX_CONTAINER_DECOR_DEFAULTS \
    WLX_CONTAINER_DECOR_DEFAULTS_CHROME(0, 0, 0)
#define WLX_CONTAINER_DECOR_DEFAULTS_CHROME(bw, rn, rs) \
    .back_color = {0}, .border_color = {0}, \
    .border_width = (bw), .roundness = (rn), .corner_radius = 0, .rounded_segments = (rs), \
    .rounded_corners = 0, \
    .gap = 0, \
    .border_color_top = {0}, .border_color_right = {0}, \
    .border_color_bottom = {0}, .border_color_left = {0}, \
    .border_width_top = WLX_UNSET, .border_width_right = WLX_UNSET, \
    .border_width_bottom = WLX_UNSET, .border_width_left = WLX_UNSET, \
    WLX_SHADOW_DEFAULTS, \
    WLX_GLOW_DEFAULTS, \
    WLX_GRADIENT_DEFAULTS, \
    .interact = 0, .interact_out = NULL, \
    .hover_back_color = {0}, .hover_border_color = {0}, \
    .hover_border_color_top = {0}, .hover_border_color_right = {0}, \
    .hover_border_color_bottom = {0}, .hover_border_color_left = {0}

#define WLX_SLOT_DECOR_FIELDS \
    WLX_Color slot_back_color; \
    WLX_Color slot_border_color; \
    float     slot_border_width

#define WLX_SLOT_DECOR_DEFAULTS \
    .slot_back_color = {0}, .slot_border_color = {0}, .slot_border_width = 0

// Content padding: inset applied around a widget's content (text + image)
// while the chrome (background, border) and hit rect remain at the full
// widget rect. Field names are `content_padding*` so widgets that also
// embed WLX_LAYOUT_SLOT_FIELDS (where the slot's own `padding*` is the
// outer-margin knob) can carry both without name collision. Embedded by
// every widget with chrome: wlx_label, wlx_button, wlx_inputbox,
// wlx_tooltip_for, wlx_split (compound), wlx_panel (compound). Per-side
// fields >= 0 win over the uniform; an unset uniform (WLX_UNSET) resolves
// to the widget's own default inset - 0 for leaf widgets, the constants
// below for the compound ones - so writing WLX_UNSET means the same as
// omitting the field; a uniform of WLX_PADDING_USE_THEME opts in to the
// theme's `padding` knob (default WLX_STYLE_CONTENT_PADDING). The resolver
// clamps the resolved inset proportionally so the content rect never has
// negative dimensions.
#define WLX_INPUTBOX_CONTENT_PADDING 10.0f
#define WLX_SPLIT_CONTENT_PADDING     4.0f
#define WLX_PANEL_CONTENT_PADDING     2.0f
#define WLX_TOOLTIP_CONTENT_PADDING   6.0f
#define WLX_CONTENT_PADDING_FIELDS \
    float content_padding; \
    float content_padding_top; \
    float content_padding_right; \
    float content_padding_bottom; \
    float content_padding_left

#define WLX_CONTENT_PADDING_DEFAULTS \
    .content_padding = WLX_UNSET, \
    .content_padding_top = WLX_UNSET, \
    .content_padding_right = WLX_UNSET, \
    .content_padding_bottom = WLX_UNSET, \
    .content_padding_left = WLX_UNSET
#define WLX_CONTENT_PADDING_COPY(src) \
    .content_padding = (src).content_padding, \
    .content_padding_top = (src).content_padding_top, \
    .content_padding_right = (src).content_padding_right, \
    .content_padding_bottom = (src).content_padding_bottom, \
    .content_padding_left = (src).content_padding_left

#define WLX_RESOLVE_CONTENT_PADDING(ctx, opt) \
    wlx_resolve_content_padding((ctx)->theme, \
        (opt).content_padding, (opt).content_padding_top, (opt).content_padding_right, \
        (opt).content_padding_bottom, (opt).content_padding_left)

// Same, for a widget whose unset uniform means its own default inset.
#define WLX_RESOLVE_CONTENT_PADDING_EX(ctx, opt, widget_default) \
    wlx_resolve_content_padding_ex((ctx)->theme, (widget_default), \
        (opt).content_padding, (opt).content_padding_top, (opt).content_padding_right, \
        (opt).content_padding_bottom, (opt).content_padding_left)

// Resolved left+right content padding of a widget opt, for adding around an
// intrinsic content width (the same field spread as WLX_RESOLVE_CONTENT_PADDING).
#define WLX_INTRINSIC_PAD_LR(ctx, opt) \
    wlx_intrinsic_pad_lr((ctx), (opt).content_padding, \
        (opt).content_padding_top, (opt).content_padding_right, \
        (opt).content_padding_bottom, (opt).content_padding_left)

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    const WLX_Slot_Size *sizes;
    WLX_CONTAINER_DECOR_FIELDS;
    WLX_SLOT_DECOR_FIELDS;
    // Scope ID: when non-NULL, scopes all descendants for the layout body.
    const char *id;
    // Clip: when true, children draw clipped to this layout's content rect.
    // A scissor is begun at layout begin (after the chrome is recorded) and
    // released at layout end. Container chrome is outside the clip; a child's
    // own glow/shadow is inside it. The clip rect also bounds the pointer: a
    // child's hit zone is intersected with it, as with a scroll panel
    // viewport. Defaults false.
    bool clip;
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
        .clip = false, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_layout_begin_impl(WLX_Context *ctx, size_t count, WLX_Orient orient, WLX_Layout_Opt opt,
                                   const char *file, int line);
#define wlx_layout_begin(ctx, count, orient, ...) \
    wlx_layout_begin_impl((ctx), (count), (orient), wlx_default_layout_opt(__VA_ARGS__), __FILE__, __LINE__)

// Options for wlx_overlay_begin. The overlay body is a linear layout of
// `count` slots rooted at an absolute window-space rect on the next layer.
typedef struct {
    const char *id;              // scope id for the overlay body
    const WLX_Slot_Size *sizes;  // per-slot sizes (NULL = equal split); CONTENT is unsupported here
    WLX_Orient orient;           // body orientation (default WLX_VERT)
    float gap;
    bool clip;                   // scissor body content to the rect (default true)
    WLX_Color back_color;        // panel fill ({0} = none)
    WLX_Color border_color;
    float border_width;
    float roundness;
    int rounded_segments;
    WLX_CONTENT_PADDING_FIELDS;  // body inset
} WLX_Overlay_Opt;

#define wlx_default_overlay_opt(...) \
    (WLX_Overlay_Opt) { \
        .orient = WLX_VERT, \
        .clip = true, \
        __VA_ARGS__ \
    }

// Begin an overlay: an absolutely positioned subtree at a window-space rect,
// drawn on the next layer (over everything on lower layers this frame) and
// owning press/hover on top per the arbitration model. It consumes no parent
// slot and contributes nothing to parent content tracking. Deferred mode
// only; in immediate mode the body draws in place at the call position
// (WLX_DEBUG warns once per site). Close with wlx_overlay_end.
WLXDEF void wlx_overlay_begin_impl(WLX_Context *ctx, size_t count, WLX_Rect rect,
    WLX_Overlay_Opt opt, const char *file, int line);
#define wlx_overlay_begin(ctx, count, rect, ...) \
    wlx_overlay_begin_impl((ctx), (count), (rect), \
        wlx_default_overlay_opt(__VA_ARGS__), __FILE__, __LINE__)

WLXDEF void wlx_overlay_end(WLX_Context *ctx);

// Layout with auto-counted sizes - no manual count parameter needed.
// Pass WLX_SIZES(...) which expands to (count, sizes_ptr):
//
//   wlx_layout_begin_s(ctx, WLX_VERT,
//       WLX_SIZES(WLX_SLOT_PX(44), WLX_SLOT_FLEX(1), WLX_SLOT_PX(24)),
//       .padding = 4);
//
// Uses double-expansion so WLX_SIZES is expanded before argument counting.
#define wlx_layout_begin_s(ctx, orient, ...) \
    WLX_LAYOUT_BEGIN_S_EXPAND(ctx, orient, __VA_ARGS__)
#define WLX_LAYOUT_BEGIN_S_EXPAND(ctx, orient, count_val, sizes_ptr, ...) \
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
//
// Two alignment fields sit on most widgets and name what they align:
//   - `slot_align` places the widget rect inside its layout slot (used when
//     `width` / `height` make the rect smaller than the slot);
//   - `content_align` (WLX_TEXT_TYPOGRAPHY_FIELDS) places the text or image
//     content inside the widget rect.
// They pair with `padding` (the slot inset) and `content_padding` (the
// content inset). `widget_align` and `align` are the pre-0.9 names: same
// storage, removed in the first minor release after 0.9.
#define WLX_WIDGET_SIZING_FIELDS \
    union { WLX_Align slot_align; WLX_Align widget_align; /* deprecated: use slot_align */ }; \
    float width;      /* WLX_UNSET: use parent width  */ \
    float height;     /* WLX_UNSET: use parent height */ \
    float min_width;  /* 0 = unconstrained */ \
    float min_height; /* 0 = unconstrained */ \
    float max_width;  /* 0 = unconstrained */ \
    float max_height; /* 0 = unconstrained */ \
    float opacity       /* <0 = unset (sentinel), 0.0-1.0 = explicit */

#define WLX_WIDGET_SIZING_DEFAULTS \
    .slot_align = WLX_LEFT, .width = WLX_UNSET, .height = WLX_UNSET, \
    .min_width = 0, .min_height = 0, .max_width = 0, .max_height = 0, \
    .opacity = WLX_UNSET
#define WLX_WIDGET_SIZING_COPY(src) \
    .slot_align = (src).slot_align, .width = (src).width, .height = (src).height, \
    .min_width = (src).min_width, .min_height = (src).min_height, \
    .max_width = (src).max_width, .max_height = (src).max_height, \
    .opacity = (src).opacity

// Per-widget interaction-state fields. Currently a single `disabled` flag;
// when true, the widget skips active-state interaction (click/press/focus/drag)
// and the visual treatment is shifted by `theme->disabled_brightness` and
// `theme->disabled_opacity`. Hover arbitration is preserved (tooltip anchor).
#define WLX_WIDGET_STATE_FIELDS \
    bool disabled

#define WLX_WIDGET_STATE_DEFAULTS \
    .disabled = false
#define WLX_WIDGET_STATE_COPY(src) \
    .disabled = (src).disabled

#define WLX_TEXT_TYPOGRAPHY_FIELDS \
    WLX_Font font; \
    int font_size; \
    union { WLX_Align content_align; WLX_Align align; /* deprecated: use content_align */ }; \
    int spacing

#define WLX_TEXT_TYPOGRAPHY_DEFAULTS \
    .font = WLX_FONT_DEFAULT, .font_size = 0, .content_align = WLX_LEFT, .spacing = 0
#define WLX_TEXT_TYPOGRAPHY_COPY(src) \
    .font = (src).font, .font_size = (src).font_size, \
    .content_align = (src).content_align, .spacing = (src).spacing

// Paragraph-wrap toggle: separated from WLX_TEXT_TYPOGRAPHY_FIELDS so widgets
// can opt in independently. Embed alongside the typography macro when the
// widget renders multi-line text; omit (e.g. on slider) when wrap is meaningless.
#define WLX_TEXT_WRAP_FIELDS \
    bool wrap

#define WLX_TEXT_WRAP_DEFAULTS \
    .wrap = false

#define WLX_TEXT_COLOR_FIELDS \
    WLX_Color front_color; \
    WLX_Color back_color

#define WLX_TEXT_COLOR_DEFAULTS \
    .front_color = {0}, .back_color = {0}
#define WLX_TEXT_COLOR_COPY(src) \
    .front_color = (src).front_color, .back_color = (src).back_color

#define WLX_BORDER_FIELDS \
    WLX_Color border_color; \
    float     border_width; \
    float     roundness; \
    float     corner_radius;   /* > 0 -> absolute px radius, overrides roundness; 0 -> unset */ \
    int       rounded_segments; \
    int       rounded_corners; /* WLX_CORNERS_* mask; 0 -> all corners rounded */ \
    WLX_Color border_color_top;    WLX_Color border_color_right; \
    WLX_Color border_color_bottom; WLX_Color border_color_left; \
    float     border_width_top;    float     border_width_right; \
    float     border_width_bottom; float     border_width_left; \
    WLX_SHADOW_FIELDS; \
    WLX_GLOW_FIELDS; \
    WLX_GRADIENT_FIELDS

#define WLX_BORDER_DEFAULTS \
    .border_color = {0}, .border_width = WLX_UNSET, \
    .roundness = WLX_UNSET, .corner_radius = 0, .rounded_segments = WLX_UNSET, \
    .rounded_corners = 0, \
    .border_color_top = {0}, .border_color_right = {0}, \
    .border_color_bottom = {0}, .border_color_left = {0}, \
    .border_width_top = WLX_UNSET, .border_width_right = WLX_UNSET, \
    .border_width_bottom = WLX_UNSET, .border_width_left = WLX_UNSET, \
    WLX_SHADOW_DEFAULTS, \
    WLX_GLOW_DEFAULTS, \
    WLX_GRADIENT_DEFAULTS
#define WLX_BORDER_COPY(src) \
    .border_color = (src).border_color, .border_width = (src).border_width, \
    .roundness = (src).roundness, .corner_radius = (src).corner_radius, \
    .rounded_segments = (src).rounded_segments, \
    .rounded_corners = (src).rounded_corners, \
    .border_color_top = (src).border_color_top, .border_color_right = (src).border_color_right, \
    .border_color_bottom = (src).border_color_bottom, .border_color_left = (src).border_color_left, \
    .border_width_top = (src).border_width_top, .border_width_right = (src).border_width_right, \
    .border_width_bottom = (src).border_width_bottom, .border_width_left = (src).border_width_left, \
    WLX_SHADOW_COPY(src), \
    WLX_GLOW_COPY(src), \
    WLX_GRADIENT_COPY(src)

// wlx_widget is a decoration primitive: it draws a rect and exposes hover
// for tooltip anchoring but does not return a click/focus/active result.
// It therefore does not carry .disabled.
typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // Fill (renamed from `.color` in the v0.6 group for cross-widget
    // consistency; the deprecated alias was removed in v0.7).
    WLX_Color back_color;

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
        /* Fill */ \
        .back_color = {0}, \
        /* Border */ \
        WLX_BORDER_DEFAULTS, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_widget_impl(WLX_Context *ctx, WLX_Widget_Opt opt, const char *file, int line);
#define wlx_widget(ctx, ...) wlx_widget_impl((ctx), wlx_default_widget_opt(__VA_ARGS__), __FILE__, __LINE__)

// Shared image public types. Used by image-capable widgets such as wlx_image,
// wlx_button, and wlx_label. STRETCH matches the raw draw_texture behavior;
// FIT preserves aspect with letterbox/pillarbox; FILL preserves aspect by
// cropping the source rect; NONE renders 1:1 pixels anchored by `align`.
typedef enum {
    WLX_IMAGE_SCALE_STRETCH = 0,
    WLX_IMAGE_SCALE_FIT,
    WLX_IMAGE_SCALE_FILL,
    WLX_IMAGE_SCALE_NONE,
} WLX_Image_Scale;

// Where an image sits relative to text inside an image-capable widget.
typedef enum {
    WLX_IMAGE_PLACEMENT_LEFT = 0,
    WLX_IMAGE_PLACEMENT_RIGHT,
    WLX_IMAGE_PLACEMENT_TOP,
    WLX_IMAGE_PLACEMENT_BOTTOM,
} WLX_Image_Placement;

// wlx_label is a non-interactive text widget: it exposes hover for tooltip
// anchoring but does not return a click/focus/active result. It therefore
// does not carry .disabled.
typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // Typography
    WLX_TEXT_TYPOGRAPHY_FIELDS;
    WLX_TEXT_WRAP_FIELDS;

    // Optional aggregate text style. When .style.font_size > 0 it overrides
    // the individual font / font_size / front_color / spacing fields.
    WLX_Text_Style       style;

    // Vertical centering basis for the text block. Default LINE_HEIGHT
    // reproduces existing behavior; FONT_SIZE pins centering to the
    // typographic em size for cross-backend cap-height parity.
    WLX_Vertical_Metric  vertical_metric;

    // Styles
    WLX_TEXT_COLOR_FIELDS;
    bool show_background;

    // Border
    WLX_BORDER_FIELDS;

    // Content padding: inset around text + image only. Chrome (background,
    // border) and the hit rect stay at the full widget rect. Defaults
    // resolve to 0 for back-compat; pass
    // `.content_padding = WLX_PADDING_USE_THEME` to opt into the theme's
    // `padding` value.
    WLX_CONTENT_PADDING_FIELDS;

    // Optional image content. .texture = {0} means text-only.
    WLX_Texture         texture;
    WLX_Rect            texture_src;     // src.w <= 0 -> full texture
    WLX_Image_Scale     texture_scale;   // default WLX_IMAGE_SCALE_FIT
    WLX_Color           texture_tint;    // {0} -> WLX_WHITE
    WLX_Image_Placement image_placement; // default WLX_IMAGE_PLACEMENT_LEFT
    float               image_size;      // <= 0 -> automatic
    float               image_text_gap;  // < 0 -> font-derived

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
        .style = WLX_TEXT_STYLE_DEFAULT, \
        .vertical_metric = WLX_VMETRIC_LINE_HEIGHT, \
        /* Styles */ \
        WLX_TEXT_COLOR_DEFAULTS, \
        .show_background = false, \
        /* Border */ \
        WLX_BORDER_DEFAULTS, \
        /* Content padding */ \
        WLX_CONTENT_PADDING_DEFAULTS, \
        /* Image content (inert by default) */ \
        .texture = {0}, \
        .texture_src = {0}, \
        .texture_scale = WLX_IMAGE_SCALE_FIT, \
        .texture_tint = {0}, \
        .image_placement = WLX_IMAGE_PLACEMENT_LEFT, \
        .image_size = 0, \
        .image_text_gap = WLX_UNSET, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_label_impl(WLX_Context *ctx, const char *text, WLX_Label_Opt opt, const char *file, int line);
#define wlx_label(ctx, text, ...) wlx_label_impl((ctx), (text), wlx_default_label_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // State
    WLX_WIDGET_STATE_FIELDS;

    // Typography
    WLX_TEXT_TYPOGRAPHY_FIELDS;
    WLX_TEXT_WRAP_FIELDS;

    // Styles
    WLX_TEXT_COLOR_FIELDS;

    // Border
    WLX_BORDER_FIELDS;

    // Content padding: inset around text + image only. Chrome (background,
    // border) and the hit rect stay at the full widget rect. Defaults
    // resolve to 0 for back-compat; pass
    // `.content_padding = WLX_PADDING_USE_THEME` to opt into the theme's
    // `padding` value.
    WLX_CONTENT_PADDING_FIELDS;

    // Optional image content. .texture = {0} means text-only.
    WLX_Texture         texture;
    WLX_Rect            texture_src;     // src.w <= 0 -> full texture
    WLX_Image_Scale     texture_scale;   // default WLX_IMAGE_SCALE_FIT
    WLX_Color           texture_tint;    // {0} -> WLX_WHITE
    WLX_Image_Placement image_placement; // default WLX_IMAGE_PLACEMENT_LEFT
    float               image_size;      // <= 0 -> automatic
    float               image_text_gap;  // < 0 -> font-derived

    // Per-call hover override for the fill, mirroring wlx_toggle/wlx_radio/
    // wlx_slider. hover_brightness defaults to WLX_UNSET (use
    // theme->hover_brightness); a negative value darkens. hover_back_color
    // defaults to {0} (use the brightness path); when set it replaces the
    // fill while hovered.
    float     hover_brightness;
    WLX_Color hover_back_color;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Button_Opt;

#define wlx_default_button_opt(...) \
    (WLX_Button_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        /* State */ \
        WLX_WIDGET_STATE_DEFAULTS, \
        /* Typography */ \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        .wrap = true, \
        /* Styles */ \
        WLX_TEXT_COLOR_DEFAULTS, \
        /* Border */ \
        WLX_BORDER_DEFAULTS, \
        /* Hover override (inert by default) */ \
        .hover_brightness = WLX_UNSET, \
        .hover_back_color = {0}, \
        /* Content padding */ \
        WLX_CONTENT_PADDING_DEFAULTS, \
        /* Image content (inert by default) */ \
        .texture = {0}, \
        .texture_src = {0}, \
        .texture_scale = WLX_IMAGE_SCALE_FIT, \
        .texture_tint = {0}, \
        .image_placement = WLX_IMAGE_PLACEMENT_LEFT, \
        .image_size = 0, \
        .image_text_gap = WLX_UNSET, \
        __VA_ARGS__ \
    }

WLXDEF bool wlx_button_impl(WLX_Context *ctx, const char *text, WLX_Button_Opt opt, const char *file, int line);
#define wlx_button(ctx, text, ...) wlx_button_impl((ctx), (text), wlx_default_button_opt(__VA_ARGS__), __FILE__, __LINE__)

// Persistent per-id dropdown state.
typedef struct {
    bool open;   // the option list overlay is showing
} WLX_Dropdown_State;

// Default cap for the open list's height; taller lists scroll.
#ifndef WLX_DROPDOWN_MAX_LIST_HEIGHT
#define WLX_DROPDOWN_MAX_LIST_HEIGHT 240.0f
#endif

// Options for wlx_dropdown. The closed face styles like a button; the
// list_* fields style the overlay row list anchored below the face.
typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // State
    WLX_WIDGET_STATE_FIELDS;

    // Typography (face and rows; rows never wrap)
    WLX_TEXT_TYPOGRAPHY_FIELDS;

    // Styles
    WLX_TEXT_COLOR_FIELDS;

    // Border
    WLX_BORDER_FIELDS;

    // Hover override (inert by default), face and rows
    float     hover_brightness;
    WLX_Color hover_back_color;

    // Content padding (face text inset; rows inherit)
    WLX_CONTENT_PADDING_FIELDS;

    // List
    float row_height;             // <= 0 -> font_size + 12
    float max_list_height;        // <= 0 -> WLX_DROPDOWN_MAX_LIST_HEIGHT
    WLX_Color list_back_color;    // {0} -> theme background
    WLX_Color list_border_color;  // {0} -> face border color
    float list_border_width;      // < 0 -> face border width

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Dropdown_Opt;

#define wlx_default_dropdown_opt(...) \
    (WLX_Dropdown_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        /* State */ \
        WLX_WIDGET_STATE_DEFAULTS, \
        /* Typography */ \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        /* Styles */ \
        WLX_TEXT_COLOR_DEFAULTS, \
        /* Border */ \
        WLX_BORDER_DEFAULTS, \
        /* Hover override (inert by default) */ \
        .hover_brightness = WLX_UNSET, \
        .hover_back_color = {0}, \
        /* Content padding */ \
        WLX_CONTENT_PADDING_DEFAULTS, \
        /* List */ \
        .row_height = 0, \
        .max_list_height = 0, \
        .list_back_color = {0}, \
        .list_border_color = {0}, \
        .list_border_width = WLX_UNSET, \
        __VA_ARGS__ \
    }

// Closed-face dropdown. The face shows options[*selected] (or `label` when
// *selected is out of range); clicking it toggles an overlay list anchored
// below at face width, capped at max_list_height and scrollable beyond it.
// Choosing an option writes *selected, closes, and returns true. The list
// closes on Escape or on a press whose owner lies outside the dropdown.
WLXDEF bool wlx_dropdown_impl(WLX_Context *ctx, const char *label,
    int *selected, const char **options, size_t count,
    WLX_Dropdown_Opt opt, const char *file, int line);
#define wlx_dropdown(ctx, label, selected, options, count, ...) \
    wlx_dropdown_impl((ctx), (label), (selected), (options), (count), \
        wlx_default_dropdown_opt(__VA_ARGS__), __FILE__, __LINE__)

// Persistent per-id tooltip state.
typedef struct {
    float hover_time;   // seconds the pointer has been over the anchor
} WLX_Tooltip_State;

// Options for wlx_tooltip_for. The tip is a single-line label on the next
// layer; it draws only and never takes part in input.
typedef struct {
    float delay;      // seconds of hover before showing; WLX_UNSET -> 0.5
    float offset_x;   // tip origin relative to the pointer
    float offset_y;

    // Content padding around the tip text (WLX_CONTENT_PADDING_FIELDS spelled
    // out so the uniform member can carry its deprecated alias). Unset
    // resolves to WLX_TOOLTIP_CONTENT_PADDING. `.padding` is the pre-v0.9
    // name of the uniform field (renamed: everywhere else `padding` is the
    // slot inset); same storage, removed in the first minor after 0.9.
    union {
        float content_padding;
        float padding;   // deprecated: use content_padding
    };
    float content_padding_top;
    float content_padding_right;
    float content_padding_bottom;
    float content_padding_left;

    // Typography
    WLX_TEXT_TYPOGRAPHY_FIELDS;

    // Styles
    WLX_Color front_color;   // {0} -> theme foreground
    WLX_Color back_color;    // {0} -> theme background
    WLX_Color border_color;  // {0} -> theme border
    float border_width;      // < 0 -> theme border width
    float roundness;         // < 0 -> theme roundness
    int rounded_segments;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Tooltip_Opt;

#define wlx_default_tooltip_opt(...) \
    (WLX_Tooltip_Opt) { \
        .delay = WLX_UNSET, \
        .offset_x = 12, \
        .offset_y = 18, \
        WLX_CONTENT_PADDING_DEFAULTS, \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        .front_color = {0}, \
        .back_color = {0}, \
        .border_color = {0}, \
        .border_width = WLX_UNSET, \
        .roundness = WLX_UNSET, \
        .rounded_segments = WLX_UNSET, \
        __VA_ARGS__ \
    }

// Pointer-anchored tooltip for an anchor rect. While the pointer rests over
// the anchor (on the anchor's layer, button up), a per-id timer accumulates
// frame time; past the delay the tip draws near the pointer on the next
// layer, clamped to the window. Draw-only: it appends no interaction
// candidates, so it can never steal hover or the press. Returns whether the
// tip is showing this frame.
WLXDEF bool wlx_tooltip_for_impl(WLX_Context *ctx, WLX_Rect anchor,
    const char *text, WLX_Tooltip_Opt opt, const char *file, int line);
#define wlx_tooltip_for(ctx, anchor, text, ...) \
    wlx_tooltip_for_impl((ctx), (anchor), (text), \
        wlx_default_tooltip_opt(__VA_ARGS__), __FILE__, __LINE__)

// Options for wlx_menu_begin. Rows style like flat buttons on the menu's
// back_color; the menu panel chrome takes the border fields.
typedef struct {
    float width;         // WLX_UNSET -> 180
    float row_height;    // <= 0 -> font_size + 12
    float item_padding;  // left/right text inset on rows; < 0 -> 8

    // Typography (rows never wrap)
    WLX_TEXT_TYPOGRAPHY_FIELDS;

    // Styles
    WLX_Color front_color;   // {0} -> theme foreground
    WLX_Color back_color;    // {0} -> theme background
    WLX_Color border_color;  // {0} -> theme border
    float border_width;      // < 0 -> theme border width
    float roundness;         // < 0 -> theme roundness
    int rounded_segments;

    // Hover override for rows (inert by default)
    float     hover_brightness;
    WLX_Color hover_back_color;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Menu_Opt;

#define wlx_default_menu_opt(...) \
    (WLX_Menu_Opt) { \
        .width = WLX_UNSET, \
        .row_height = 0, \
        .item_padding = WLX_UNSET, \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        .front_color = {0}, \
        .back_color = {0}, \
        .border_color = {0}, \
        .border_width = WLX_UNSET, \
        .roundness = WLX_UNSET, \
        .rounded_segments = WLX_UNSET, \
        .hover_brightness = WLX_UNSET, \
        .hover_back_color = {0}, \
        __VA_ARGS__ \
    }

// Options for wlx_menu_item.
typedef struct {
    WLX_WIDGET_STATE_FIELDS;
    WLX_Color front_color;   // {0} -> menu front_color
    bool keep_open;          // clicking does not close the menu (submenu triggers, checkable items)
} WLX_Menu_Item_Opt;

#define wlx_default_menu_item_opt(...) \
    (WLX_Menu_Item_Opt) { \
        WLX_WIDGET_STATE_DEFAULTS, \
        .front_color = {0}, \
        .keep_open = false, \
        __VA_ARGS__ \
    }

// Point-anchored overlay menu. Opening is caller-triggered: any caller
// event sets *open, and wlx_menu_begin builds the menu while it stays true.
// Returns whether the menu is open - add items and call wlx_menu_end ONLY
// when it returned true:
//
//   if (wlx_menu_begin(&ctx, &open, x, y)) {
//       if (wlx_menu_item(&ctx, "Copy"))  { ... }
//       if (wlx_menu_item(&ctx, "Paste")) { ... }
//       wlx_menu_end(&ctx);
//   }
//
// An item click, Escape, or a press whose owner lies outside the menu
// clears *open. One nested wlx_menu_begin inside the body opens a submenu
// on the next layer. The chrome height follows the previous frame's item
// count (one-frame adaptation on first open or item changes).
WLXDEF bool wlx_menu_begin_impl(WLX_Context *ctx, bool *open, float x, float y,
    WLX_Menu_Opt opt, const char *file, int line);
#define wlx_menu_begin(ctx, open, x, y, ...) \
    wlx_menu_begin_impl((ctx), (open), (x), (y), \
        wlx_default_menu_opt(__VA_ARGS__), __FILE__, __LINE__)

// One menu row; returns true when clicked, which also dismisses the whole
// open menu chain at the wlx_menu_end calls (a .keep_open item does not
// close anything). Loop-generated items need wlx_push_id like any widget.
WLXDEF bool wlx_menu_item_impl(WLX_Context *ctx, const char *text,
    WLX_Menu_Item_Opt opt, const char *file, int line);
#define wlx_menu_item(ctx, text, ...) \
    wlx_menu_item_impl((ctx), (text), \
        wlx_default_menu_item_opt(__VA_ARGS__), __FILE__, __LINE__)

WLXDEF void wlx_menu_end(WLX_Context *ctx);

// Options for wlx_menu_button_begin. The face styles like a button (the
// shared `width` sizing field is the face's); the list_* fields style the
// menu list anchored below it, `menu_width` its width (<= 0 -> the
// face's resolved width).
typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // State
    WLX_WIDGET_STATE_FIELDS;

    // Typography (face and items; items never wrap)
    WLX_TEXT_TYPOGRAPHY_FIELDS;

    // Styles
    WLX_TEXT_COLOR_FIELDS;

    // Border
    WLX_BORDER_FIELDS;

    // Hover override (inert by default), face and items
    float     hover_brightness;
    WLX_Color hover_back_color;

    // Content padding (face text inset)
    WLX_CONTENT_PADDING_FIELDS;

    // List
    float menu_width;             // list width; <= 0 -> face width
    float row_height;             // <= 0 -> font_size + 12
    float item_padding;           // item text left/right inset; < 0 -> 8
    WLX_Color list_back_color;    // {0} -> theme background
    WLX_Color list_border_color;  // {0} -> face border color
    float list_border_width;      // < 0 -> face border width

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Menu_Button_Opt;

#define wlx_default_menu_button_opt(...) \
    (WLX_Menu_Button_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        /* State */ \
        WLX_WIDGET_STATE_DEFAULTS, \
        /* Typography */ \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        /* Styles */ \
        WLX_TEXT_COLOR_DEFAULTS, \
        /* Border */ \
        WLX_BORDER_DEFAULTS, \
        /* Hover override (inert by default) */ \
        .hover_brightness = WLX_UNSET, \
        .hover_back_color = {0}, \
        /* Content padding */ \
        WLX_CONTENT_PADDING_DEFAULTS, \
        /* List */ \
        .menu_width = 0, \
        .row_height = 0, \
        .item_padding = WLX_UNSET, \
        .list_back_color = {0}, \
        .list_border_color = {0}, \
        .list_border_width = WLX_UNSET, \
        __VA_ARGS__ \
    }

// Button-anchored menu: a face button (drawn every frame, consuming a
// layout slot) that toggles *open like a dropdown face - the first click
// opens, the second closes, with no outside-press fight because the face
// belongs to the menu's press scope. The list anchors below the face.
// Body contract matches wlx_menu_begin: add items and call wlx_menu_end
// ONLY when it returned true. Use this for menu-bar / toolbar menus;
// wlx_menu_begin stays the point-anchored entry for context menus and
// submenus, where reopening at a new position on the summoning press is
// the intended behavior.
WLXDEF bool wlx_menu_button_begin_impl(WLX_Context *ctx, const char *label,
    bool *open, WLX_Menu_Button_Opt opt, const char *file, int line);
#define wlx_menu_button_begin(ctx, label, open, ...) \
    wlx_menu_button_begin_impl((ctx), (label), (open), \
        wlx_default_menu_button_opt(__VA_ARGS__), __FILE__, __LINE__)

// Submenu, valid only inside a menu body (between wlx_menu_begin /
// wlx_menu_button_begin and wlx_menu_end). It takes no position: the list
// anchors flush to the parent panel's right edge at the row of the last
// emitted item - the trigger, which should be a `.keep_open` item that
// toggles *open. Unset options inherit the parent's resolved styling
// (width, row height, colors, border), so a submenu matches its parent by
// default. It also shares the parent's press scope: pressing anything in
// the parent (the trigger included) is an inside press, so the trigger
// toggles cleanly. Body contract matches wlx_menu_begin - items and
// wlx_menu_end ONLY when it returned true:
//
//   if (wlx_menu_item(&ctx, "More...", .keep_open = true))
//       sub_open = !sub_open;
//   if (wlx_submenu_begin(&ctx, &sub_open)) {
//       if (wlx_menu_item(&ctx, "Rename")) { ... }
//       wlx_menu_end(&ctx);
//   }
WLXDEF bool wlx_submenu_begin_impl(WLX_Context *ctx, bool *open,
    WLX_Menu_Opt opt, const char *file, int line);
#define wlx_submenu_begin(ctx, open, ...) \
    wlx_submenu_begin_impl((ctx), (open), \
        wlx_default_menu_opt(__VA_ARGS__), __FILE__, __LINE__)


typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // State
    WLX_WIDGET_STATE_FIELDS;

    // Typography
    WLX_TEXT_TYPOGRAPHY_FIELDS;
    WLX_TEXT_WRAP_FIELDS;

    // Styles
    WLX_TEXT_COLOR_FIELDS;
    bool full_slot_hit;
    WLX_BORDER_FIELDS;
    WLX_Color check_color;

    // Content padding: inset around the compound content (checkbox glyph +
    // label) only. Chrome and hit rect stay at the full widget rect.
    // Defaults resolve to 0; pass
    // `.content_padding = WLX_PADDING_USE_THEME` to opt into the theme's
    // `padding` value.
    WLX_CONTENT_PADDING_FIELDS;

    // Texture mode: active only when both tex_checked and tex_unchecked
    // are drawable. Each state has its own source rect and tint, matching
    // the image-capable label/button/image contract.
    //   tex_*_src  {0} -> full selected texture
    //   tex_*_tint {0} -> WLX_WHITE
    WLX_Texture tex_checked;
    WLX_Texture tex_unchecked;
    WLX_Rect    tex_checked_src;
    WLX_Rect    tex_unchecked_src;
    WLX_Color   tex_checked_tint;
    WLX_Color   tex_unchecked_tint;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Checkbox_Opt;

#define wlx_default_checkbox_opt(...) \
    (WLX_Checkbox_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        /* State */ \
        WLX_WIDGET_STATE_DEFAULTS, \
        /* Typography */ \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        WLX_TEXT_WRAP_DEFAULTS, \
        /* Styles */ \
        WLX_TEXT_COLOR_DEFAULTS, \
        .full_slot_hit = true, \
        WLX_BORDER_DEFAULTS, \
        .check_color = {0}, \
        /* Content padding */ \
        WLX_CONTENT_PADDING_DEFAULTS, \
        .tex_checked_src = {0}, \
        .tex_unchecked_src = {0}, \
        .tex_checked_tint = {0}, \
        .tex_unchecked_tint = {0}, \
        __VA_ARGS__ \
    }

WLXDEF bool wlx_checkbox_impl(WLX_Context *ctx, const char *text, bool *checked, WLX_Checkbox_Opt opt, const char *file, int line);
#define wlx_checkbox(ctx, text, checked, ...) wlx_checkbox_impl((ctx), (text), (checked), wlx_default_checkbox_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;
    WLX_CONTENT_PADDING_FIELDS;       // outer gutter around label/input rect (default: 10)

    // State
    WLX_WIDGET_STATE_FIELDS;

    // Typography
    WLX_TEXT_TYPOGRAPHY_FIELDS;
    WLX_TEXT_WRAP_FIELDS;

    // Styles
    WLX_TEXT_COLOR_FIELDS;
    WLX_BORDER_FIELDS;
    WLX_Color border_focus_color;
    WLX_Color cursor_color;
    WLX_Color selection_color;   // {0} -> theme->input.selection

    // Optional icon rendered INSIDE the field frame, on the leading or
    // trailing interior edge. .texture = {0} means text-only (inert; existing
    // callers are unaffected). The reserved icon band insets the text and
    // caret so they never overlap the glyph.
    //   texture_src   w/h <= 0 -> full texture
    //   texture_tint  {0}      -> WLX_WHITE
    //   image_placement        -> WLX_IMAGE_PLACEMENT_LEFT (default) or RIGHT;
    //                             TOP/BOTTOM are treated as LEFT (a single-line
    //                             field has no vertical band).
    //   image_size    <= 0     -> auto from font_size, clamped to field interior
    //   image_text_gap < 0     -> font-derived (font_size * 0.5)
    WLX_Texture         texture;
    WLX_Rect            texture_src;
    WLX_Color           texture_tint;
    WLX_Image_Placement image_placement;
    float               image_size;
    float               image_text_gap;

    // Optional out-param: receives this frame's focus state (the pre-v0.6
    // return value). NULL = not reported.
    bool *out_focused;

    // Password mode: renders one mask character per codepoint while the
    // buffer keeps the plaintext. Forces single-line (wrap off) and
    // suppresses copy/cut so the plaintext cannot leave the field.
    bool password;

    // Read-only mode: the field stays focusable, selectable, and copyable,
    // but every mutation (typing, delete, cut, paste) is rejected. Distinct
    // from .disabled: no interaction lockout and no dimmed rendering.
    bool read_only;

    // Multiline mode: Enter inserts a newline at the caret and keeps focus;
    // Escape or a click elsewhere blurs. Excluded by .password (masked
    // fields are always single-line). Composes with .read_only: the field
    // keeps focus on Enter but the insert is rejected.
    bool multiline;

    // Multiline scrollbar: draw a draggable vertical scrollbar while the
    // content overflows the field (multiline only; inert otherwise).
    // false keeps wheel and caret-follow scrolling without the affordance.
    bool show_scrollbar;

    // External-mutation guard: bump after mutating the buffer outside the
    // widget (same length included); the widget drops its undo history when
    // the revision or the buffer length changes.
    uint32_t revision;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Inputbox_Opt;

#define wlx_default_inputbox_opt(...) \
    (WLX_Inputbox_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        WLX_CONTENT_PADDING_DEFAULTS, \
        /* State */ \
        WLX_WIDGET_STATE_DEFAULTS, \
        /* Typography */ \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        .wrap = true, \
        /* Styles */ \
        WLX_TEXT_COLOR_DEFAULTS, \
        WLX_BORDER_DEFAULTS, \
        .border_focus_color = {0}, \
        .cursor_color = {0}, \
        .selection_color = {0}, \
        /* Icon content (inert by default) */ \
        .texture = {0}, \
        .texture_src = {0}, \
        .texture_tint = {0}, \
        .image_placement = WLX_IMAGE_PLACEMENT_LEFT, \
        .image_size = 0, \
        .image_text_gap = WLX_UNSET, \
        .out_focused = NULL, \
        .password = false, \
        .read_only = false, \
        .multiline = false, \
        .show_scrollbar = true, \
        .revision = 0, \
        __VA_ARGS__ \
    }

WLXDEF bool wlx_inputbox_impl(WLX_Context *ctx, const char *label, char *buffer, size_t buffer_size, WLX_Inputbox_Opt opt, const char *file, int line);
#define wlx_inputbox(ctx, label, buffer, buffer_size, ...) wlx_inputbox_impl((ctx), (label), (buffer), (buffer_size), wlx_default_inputbox_opt(__VA_ARGS__), __FILE__, __LINE__)

// Multiline sugar over wlx_inputbox: presets .multiline plus a top-left text
// anchor (the natural reading origin for a tall note field); both presets sit
// before the caller's options so any of them can still be overridden.
#define wlx_textarea(ctx, label, buffer, buffer_size, ...) \
    wlx_inputbox_impl((ctx), (label), (buffer), (buffer_size), \
        wlx_default_inputbox_opt(.multiline = true, .content_align = WLX_TOP_LEFT, __VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;

    // State
    WLX_WIDGET_STATE_FIELDS;

    // Typography (no wrap: slider renders single-line label + value text)
    WLX_TEXT_TYPOGRAPHY_FIELDS;
    // Show the numeric value readout next to the track (renamed from
    // `.show_label` in the v0.6 group: it never controlled the label text,
    // only the value readout; the deprecated alias was removed in v0.7).
    bool show_value;

    // Styles
    WLX_Color track_color;
    WLX_Color thumb_color;
    WLX_Color label_color;
    float track_height;   // height of the track bar
    float thumb_width;    // width of the thumb handle; also its default visual height
    float hover_brightness;
    float thumb_hover_brightness;
    float fill_inactive_brightness;
    float min_value;
    float max_value;

    // Border
    WLX_BORDER_FIELDS;

    // Content padding: inset around the label / track / value content. Slot
    // contribution and the track-region hit rect are derived from the inset
    // rect, so a padded slider stays interactive only inside the inset.
    // Defaults resolve to 0; pass
    // `.content_padding = WLX_PADDING_USE_THEME` for the theme value.
    WLX_CONTENT_PADDING_FIELDS;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Slider_Opt;


#define wlx_default_slider_opt(...) \
    (WLX_Slider_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        /* State */ \
        WLX_WIDGET_STATE_DEFAULTS, \
        /* Typography */ \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        .show_value = true, \
        /* Styles */ \
        .track_color = {0}, \
        .thumb_color = {0}, \
        .label_color = {0}, \
        .track_height = 0, \
        .thumb_width = 0, \
        .hover_brightness = WLX_UNSET, \
        .thumb_hover_brightness = WLX_UNSET, \
        .fill_inactive_brightness = -0.3f, \
        .min_value = 0.0f, \
        .max_value = 1.0f, \
        /* Border */ \
        WLX_BORDER_DEFAULTS, \
        /* Content padding */ \
        WLX_CONTENT_PADDING_DEFAULTS, \
        __VA_ARGS__ \
    }

WLXDEF bool wlx_slider_impl(WLX_Context *ctx, const char *label, float *value, WLX_Slider_Opt opt, const char *file, int line);
#define wlx_slider(ctx, label, value, ...) wlx_slider_impl((ctx), (label), (value), wlx_default_slider_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    WLX_WIDGET_SIZING_FIELDS;
    // Line color (renamed from `.color` in the v0.6 group; the deprecated
    // alias was removed in v0.7).
    WLX_Color back_color;
    float     thickness;
    const char *id;
} WLX_Separator_Opt;

#define wlx_default_separator_opt(...) \
    (WLX_Separator_Opt) { \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        WLX_WIDGET_SIZING_DEFAULTS, \
        .back_color = {0}, \
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
    int       segments;           // 0 -> continuous (current behavior); > 0 -> discrete cells
    float     segment_gap;        // px gap between cells; <= 0 -> theme default
    WLX_BORDER_FIELDS;

    // Content padding: inset around the track. Slot contribution stays
    // unchanged. Defaults resolve to 0; pass
    // `.content_padding = WLX_PADDING_USE_THEME` for the theme value.
    WLX_CONTENT_PADDING_FIELDS;

    const char *id;
} WLX_Progress_Opt;

#define wlx_default_progress_opt(...) \
    (WLX_Progress_Opt) { \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        WLX_WIDGET_SIZING_DEFAULTS, \
        .track_color = {0}, \
        .fill_color = {0}, \
        .track_height = 0, \
        .segments = 0, \
        .segment_gap = 0, \
        WLX_BORDER_DEFAULTS, \
        /* Content padding */ \
        WLX_CONTENT_PADDING_DEFAULTS, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_progress_impl(WLX_Context *ctx, float value, WLX_Progress_Opt opt, const char *file, int line);
#define wlx_progress(ctx, value, ...) wlx_progress_impl((ctx), (value), wlx_default_progress_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    WLX_WIDGET_SIZING_FIELDS;

    WLX_Image_Scale scale;   // default WLX_IMAGE_SCALE_STRETCH
    // Positions/crops the image inside the widget rect; default WLX_CENTER.
    // `align` is the pre-0.9 name (same storage, removed in the first minor
    // after 0.9).
    union { WLX_Align content_align; WLX_Align align; /* deprecated: use content_align */ };
    WLX_Color       tint;    // {0} -> WLX_WHITE
    WLX_Rect        src;     // src.w <= 0 -> full texture

    const char *id;
} WLX_Image_Opt;

#define wlx_default_image_opt(...) \
    (WLX_Image_Opt) { \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        WLX_WIDGET_SIZING_DEFAULTS, \
        .scale = WLX_IMAGE_SCALE_STRETCH, \
        .content_align = WLX_CENTER, \
        .tint  = {0}, \
        .src   = {0}, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_image_impl(WLX_Context *ctx, WLX_Texture texture, WLX_Image_Opt opt, const char *file, int line);
#define wlx_image(ctx, texture, ...) \
    wlx_image_impl((ctx), (texture), wlx_default_image_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    WLX_WIDGET_SIZING_FIELDS;
    WLX_WIDGET_STATE_FIELDS;
    WLX_TEXT_TYPOGRAPHY_FIELDS;
    WLX_TEXT_WRAP_FIELDS;
    WLX_TEXT_COLOR_FIELDS;

    WLX_Color track_color;
    WLX_Color track_active_color;
    WLX_Color thumb_color;
    float     hover_brightness;
    WLX_BORDER_FIELDS;

    // Content padding: inset around the compound content (track + label).
    // Chrome and the click/hover hit rect stay at the full widget rect.
    // Defaults resolve to 0; pass
    // `.content_padding = WLX_PADDING_USE_THEME` for the theme value.
    WLX_CONTENT_PADDING_FIELDS;

    const char *id;
} WLX_Toggle_Opt;

#define wlx_default_toggle_opt(...) \
    (WLX_Toggle_Opt) { \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        WLX_WIDGET_SIZING_DEFAULTS, \
        WLX_WIDGET_STATE_DEFAULTS, \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        WLX_TEXT_WRAP_DEFAULTS, \
        WLX_TEXT_COLOR_DEFAULTS, \
        .track_color = {0}, \
        .track_active_color = {0}, \
        .thumb_color = {0}, \
        .hover_brightness = WLX_UNSET, \
        WLX_BORDER_DEFAULTS, \
        /* Content padding */ \
        WLX_CONTENT_PADDING_DEFAULTS, \
        __VA_ARGS__ \
    }

WLXDEF bool wlx_toggle_impl(WLX_Context *ctx, const char *label, bool *value, WLX_Toggle_Opt opt, const char *file, int line);
#define wlx_toggle(ctx, label, value, ...) wlx_toggle_impl((ctx), (label), (value), wlx_default_toggle_opt(__VA_ARGS__), __FILE__, __LINE__)

typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    WLX_WIDGET_SIZING_FIELDS;
    WLX_WIDGET_STATE_FIELDS;
    WLX_TEXT_TYPOGRAPHY_FIELDS;
    WLX_TEXT_WRAP_FIELDS;
    WLX_TEXT_COLOR_FIELDS;

    WLX_Color ring_color;
    WLX_Color fill_color;
    float     ring_border_width;
    float     hover_brightness;

    // Content padding: inset around the compound content (ring + label).
    // Chrome and the click/hover hit rect stay at the full widget rect.
    // Defaults resolve to 0; pass
    // `.content_padding = WLX_PADDING_USE_THEME` for the theme value.
    WLX_CONTENT_PADDING_FIELDS;

    const char *id;
} WLX_Radio_Opt;

#define wlx_default_radio_opt(...) \
    (WLX_Radio_Opt) { \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        WLX_WIDGET_SIZING_DEFAULTS, \
        WLX_WIDGET_STATE_DEFAULTS, \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        WLX_TEXT_WRAP_DEFAULTS, \
        WLX_TEXT_COLOR_DEFAULTS, \
        .ring_color = {0}, \
        .fill_color = {0}, \
        .ring_border_width = WLX_UNSET, \
        .hover_brightness = WLX_UNSET, \
        /* Content padding */ \
        WLX_CONTENT_PADDING_DEFAULTS, \
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
    bool transparent_background; // true -> draw no panel fill; let the container show through
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
        .transparent_background = false, \
        .scrollbar_color = {0}, \
        .scrollbar_hover_brightness = WLX_UNSET, \
        .scrollbar_width = WLX_UNSET, \
        .wheel_scroll_speed = 20.0f, \
        .show_scrollbar = true, \
        /* Border */ \
        WLX_BORDER_DEFAULTS, \
        __VA_ARGS__ \
    }

// content_height sentinel: measure the content during the frame instead of
// passing a fixed pixel height.
#define WLX_SCROLL_AUTO_HEIGHT (-1.0f)

WLXDEF void wlx_scroll_panel_begin_impl(WLX_Context *ctx, float content_height, WLX_Scroll_Panel_Opt opt, const char *file, int line);
#define wlx_scroll_panel_begin(ctx, content_height, ...) wlx_scroll_panel_begin_impl((ctx), (content_height), wlx_default_scroll_panel_opt(__VA_ARGS__), __FILE__, __LINE__)
WLXDEF void wlx_scroll_panel_end(WLX_Context *ctx);

// ============================================================================
// List clipper: virtualized rows inside a scroll panel
// ============================================================================
// Builds only the rows of a long list that fall within the enclosing scroll
// panel's viewport, reserving the off-screen extent with spacers so the
// scrollbar stays correct. Drive the panel with an explicit content_height
// (wlx_list_clipper_height), not WLX_SCROLL_AUTO_HEIGHT. Rows outside the
// returned [first, last) range are not produced, so they receive no ids,
// interactions, or persistent state that frame - keep stateful or interactive
// widgets out of virtualized rows, or widen the range with opt.overscan.
//
//   float h = wlx_list_clipper_height(count, row_h, NULL);
//   wlx_scroll_panel_begin(ctx, h, ...);
//     WLX_List_Clipper c = wlx_list_clipper_begin(ctx, count, row_h, .id = "rows");
//     for (int i = c.first; i < c.last; i++) { build row i }
//     wlx_list_clipper_end(ctx, &c);
//   wlx_scroll_panel_end(ctx);

typedef struct {
    const char  *id;            // stable id for the content layout (NULL = call-site)
    const float *item_offsets;  // variable height: prefix sums, length item_count+1,
                                // monotonic with [0]==0. NULL selects fixed pitch.
    float        overscan;      // extra pixels built above/below the viewport
} WLX_List_Clipper_Opt;

typedef struct {
    int          first;         // first visible item index (inclusive)
    int          last;          // one-past-last visible item index (exclusive)
    // Internal bookkeeping - do not rely on these fields.
    int          count;
    float        row_height;
    const float *offsets;
    float        before;
    float        after;
} WLX_List_Clipper;

#define wlx_default_list_clipper_opt(...) \
    (WLX_List_Clipper_Opt){ .id = NULL, .item_offsets = NULL, .overscan = 0.0f, __VA_ARGS__ }

// Total content height for a list. Pass the result to wlx_scroll_panel_begin so
// the panel height and the clipper spacers agree exactly. item_offsets may be
// NULL for fixed pitch (returns item_count * row_height).
WLXDEF float wlx_list_clipper_height(int item_count, float row_height, const float *item_offsets);

WLXDEF WLX_List_Clipper wlx_list_clipper_begin_impl(WLX_Context *ctx, int item_count,
    float row_height, WLX_List_Clipper_Opt opt);
#define wlx_list_clipper_begin(ctx, item_count, row_height, ...) \
    wlx_list_clipper_begin_impl((ctx), (item_count), (row_height), wlx_default_list_clipper_opt(__VA_ARGS__))
WLXDEF void wlx_list_clipper_end(WLX_Context *ctx, WLX_List_Clipper *clip);

// Height of item i: in variable mode offsets[i+1]-offsets[i], else row_height.
// In variable mode call wlx_layout_auto_slot_px(ctx, wlx_list_clipper_item_height(&c, i))
// before building row i so its slot matches its offset span.
WLXDEF float wlx_list_clipper_item_height(const WLX_List_Clipper *clip, int i);

// ============================================================================
// Compound widget: Split panel (two-pane split with independent scroll)
// ============================================================================

typedef struct {
    WLX_Slot_Size first_size;       // default: WLX_SLOT_PX(280)
    WLX_Slot_Size second_size;      // default: WLX_SLOT_FLEX(1)
    WLX_Slot_Size fill_size;        // default: WLX_SLOT_FLEX(1) (fills parent slot)
    WLX_CONTENT_PADDING_FIELDS;     // inner padding around panes (default: 4)
    float gap;                      // inter-pane spacing (literal, default 0)
    WLX_Color first_back_color;     // first pane scroll panel bg (default: theme)
    WLX_Color second_back_color;    // second pane scroll panel bg (default: theme)
    // Scope ID: when non-NULL, scopes all descendants for the split body.
    const char *id;
} WLX_Split_Opt;

#define wlx_default_split_opt(...) \
    (WLX_Split_Opt){ \
        .first_size       = WLX_SLOT_PX(280), \
        .second_size      = WLX_SLOT_FLEX(1), \
        .fill_size        = WLX_SLOT_FLEX(1), \
        WLX_CONTENT_PADDING_DEFAULTS, \
        .gap              = 0, \
        .first_back_color = {0}, \
        .second_back_color = {0}, \
        __VA_ARGS__ \
    }

typedef struct {
    WLX_Color back_color;  // override second pane bg color
} WLX_Split_Next_Opt;

#define wlx_default_split_next_opt(...) \
    (WLX_Split_Next_Opt){ \
        .back_color = {0}, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_split_begin_impl(WLX_Context *ctx, WLX_Split_Opt opt, const char *file, int line);
WLXDEF void wlx_split_next_impl(WLX_Context *ctx, WLX_Split_Next_Opt opt, const char *file, int line);
WLXDEF void wlx_split_end_impl(WLX_Context *ctx);

#define wlx_split_begin(ctx, ...) \
    wlx_split_begin_impl((ctx), wlx_default_split_opt(__VA_ARGS__), __FILE__, __LINE__)
#define wlx_split_next(ctx, ...) \
    wlx_split_next_impl((ctx), wlx_default_split_next_opt(__VA_ARGS__), __FILE__, __LINE__)
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

    // Body decor: the shared container decor set (background, uniform and
    // per-side border, roundness/segments, gap, shadow/glow/gradient) plus
    // per-slot decor, forwarded to the panel's body layout. Defaults keep
    // the previous rendering: everything off. border_width uses the -1
    // unset sentinel (v0.6): unset inherits theme->border_width and a {0}
    // border_color inherits theme->border, matching every widget; pass an
    // explicit `.border_width = 0` for a borderless panel under a theme
    // with a non-zero border width.
    WLX_CONTAINER_DECOR_FIELDS;
    WLX_SLOT_DECOR_FIELDS;
    bool clip;                       // clip body content and pointer hit zones to panel bounds. false = no clip

    // Layout options
    WLX_CONTENT_PADDING_FIELDS;      // inner padding around panel body (default: 2)
    int   capacity;                  // max child count (default: 32)

    // Scope ID: when non-NULL, scopes all descendants for the panel body.
    const char *id;
} WLX_Panel_Opt;

#define wlx_default_panel_opt(...) \
    (WLX_Panel_Opt){ \
        .title            = NULL, \
        .title_font_size  = 0, \
        .title_height     = 0, \
        .title_align      = WLX_CENTER, \
        .title_back_color = {0}, \
        WLX_CONTAINER_DECOR_DEFAULTS_CHROME(WLX_UNSET, WLX_UNSET, WLX_UNSET), \
        WLX_SLOT_DECOR_DEFAULTS, \
        .clip             = false, \
        WLX_CONTENT_PADDING_DEFAULTS, \
        .capacity         = 0, \
        __VA_ARGS__ \
    }

WLXDEF void wlx_panel_begin_impl(WLX_Context *ctx, WLX_Panel_Opt opt, const char *file, int line);
WLXDEF void wlx_panel_end(WLX_Context *ctx);

#define wlx_panel_begin(ctx, ...) \
    wlx_panel_begin_impl((ctx), wlx_default_panel_opt(__VA_ARGS__), __FILE__, __LINE__)

// ============================================================================
// Option defaults as values
// ============================================================================
// Every option struct's defaults, exactly as its wlx_default_*_opt macro
// installs them, returned by value. The macros are the C11 call surface
// (defaults first, the caller's designators override); these functions are
// the surface for callers that cannot repeat designators - a C++ caller
// takes the defaults, assigns, and calls the _impl function. A macro and its
// function can never disagree: each function returns the macro's expansion.

WLXDEF WLX_Slot_Style_Opt   wlx_slot_style_opt_defaults(void);
WLXDEF WLX_Grid_Opt         wlx_grid_opt_defaults(void);
WLXDEF WLX_Grid_Auto_Opt    wlx_grid_auto_opt_defaults(void);
WLXDEF WLX_Layout_Opt       wlx_layout_opt_defaults(void);
WLXDEF WLX_Overlay_Opt      wlx_overlay_opt_defaults(void);
WLXDEF WLX_Widget_Opt       wlx_widget_opt_defaults(void);
WLXDEF WLX_Label_Opt        wlx_label_opt_defaults(void);
WLXDEF WLX_Button_Opt       wlx_button_opt_defaults(void);
WLXDEF WLX_Dropdown_Opt     wlx_dropdown_opt_defaults(void);
WLXDEF WLX_Tooltip_Opt      wlx_tooltip_opt_defaults(void);
WLXDEF WLX_Menu_Opt         wlx_menu_opt_defaults(void);
WLXDEF WLX_Menu_Item_Opt    wlx_menu_item_opt_defaults(void);
WLXDEF WLX_Menu_Button_Opt  wlx_menu_button_opt_defaults(void);
WLXDEF WLX_Checkbox_Opt     wlx_checkbox_opt_defaults(void);
WLXDEF WLX_Inputbox_Opt     wlx_inputbox_opt_defaults(void);
WLXDEF WLX_Slider_Opt       wlx_slider_opt_defaults(void);
WLXDEF WLX_Separator_Opt    wlx_separator_opt_defaults(void);
WLXDEF WLX_Progress_Opt     wlx_progress_opt_defaults(void);
WLXDEF WLX_Image_Opt        wlx_image_opt_defaults(void);
WLXDEF WLX_Toggle_Opt       wlx_toggle_opt_defaults(void);
WLXDEF WLX_Radio_Opt        wlx_radio_opt_defaults(void);
WLXDEF WLX_Scroll_Panel_Opt wlx_scroll_panel_opt_defaults(void);
WLXDEF WLX_List_Clipper_Opt wlx_list_clipper_opt_defaults(void);
WLXDEF WLX_Split_Opt        wlx_split_opt_defaults(void);
WLXDEF WLX_Split_Next_Opt   wlx_split_next_opt_defaults(void);
WLXDEF WLX_Panel_Opt        wlx_panel_opt_defaults(void);

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
#define textarea(ctx, label, buffer, buffer_size, ...) wlx_textarea((ctx), (label), (buffer), (buffer_size), __VA_ARGS__)
#define slider(ctx, label, value, ...) wlx_slider((ctx), (label), (value), __VA_ARGS__)
#define separator(ctx, ...) wlx_separator((ctx), __VA_ARGS__)
#define progress(ctx, value, ...) wlx_progress((ctx), (value), __VA_ARGS__)
#define toggle(ctx, label, value, ...) wlx_toggle((ctx), (label), (value), __VA_ARGS__)
#define radio(ctx, label, active, index, ...) wlx_radio((ctx), (label), (active), (index), __VA_ARGS__)
#define scroll_panel_begin(ctx, content_height, ...) wlx_scroll_panel_begin((ctx), (content_height), __VA_ARGS__)
#define scroll_panel_end(ctx) wlx_scroll_panel_end((ctx))
#define overlay_begin(ctx, count, rect, ...) wlx_overlay_begin((ctx), (count), (rect), __VA_ARGS__)
#define overlay_end(ctx) wlx_overlay_end((ctx))
#define dropdown(ctx, label, selected, options, count, ...) wlx_dropdown((ctx), (label), (selected), (options), (count), __VA_ARGS__)
#define tooltip_for(ctx, anchor, text, ...) wlx_tooltip_for((ctx), (anchor), (text), __VA_ARGS__)
#define menu_begin(ctx, open, x, y, ...) wlx_menu_begin((ctx), (open), (x), (y), __VA_ARGS__)
#define menu_button_begin(ctx, label, open, ...) wlx_menu_button_begin((ctx), (label), (open), __VA_ARGS__)
#define submenu_begin(ctx, open, ...) wlx_submenu_begin((ctx), (open), __VA_ARGS__)
#define menu_item(ctx, text, ...) wlx_menu_item((ctx), (text), __VA_ARGS__)
#define menu_end(ctx) wlx_menu_end((ctx))
#define split_begin(ctx, ...) wlx_split_begin((ctx), __VA_ARGS__)
#define split_next(ctx, ...) wlx_split_next((ctx), __VA_ARGS__)
#define split_end(ctx) wlx_split_end((ctx))
#define panel_begin(ctx, ...) wlx_panel_begin((ctx), __VA_ARGS__)
#define panel_end(ctx) wlx_panel_end((ctx))
#define widget(ctx, ...) wlx_widget((ctx), __VA_ARGS__)
#define image(ctx, texture, ...) wlx_image((ctx), (texture), __VA_ARGS__)
#define slot_style(ctx, ...) wlx_slot_style((ctx), __VA_ARGS__)
#define grid_cell_style(ctx, ...) wlx_grid_cell_style((ctx), __VA_ARGS__)
#define last_rect(ctx) wlx_last_rect((ctx))
#define push_id(ctx, id) wlx_push_id((ctx), (id))
#define pop_id(ctx) wlx_pop_id((ctx))
#define push_opacity(ctx, opacity) wlx_push_opacity((ctx), (opacity))
#define pop_opacity(ctx) wlx_pop_opacity((ctx))
#endif // WLX_SHORT_NAMES

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WOLLIX_H_


////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
#ifdef WOLLIX_IMPLEMENTATION
#include <math.h>

// Internal interaction resolver with an explicit disabled gate. Forward-declared
// here so interaction-aware container begins can resolve before its definition.
static inline WLX_Interaction wlx_get_interaction_for(WLX_Context *ctx, WLX_Rect rect, uint32_t flags, bool disabled, const char *file, int line);

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
static inline void wlx_dbg_slot_overflow(WLX_Context *ctx, const WLX_Layout *l, const char *file, int line);
static inline void wlx_dbg_widget_begin(WLX_Context *ctx, WLX_Rect cell, float height, int span, bool overflow, const char *file, int line);
static inline void wlx_dbg_split_begin(WLX_Context *ctx);
static inline void wlx_dbg_split_suppress_warn(WLX_Context *ctx, bool suppress);
static inline void wlx_dbg_split_next(WLX_Context *ctx);
static inline void wlx_dbg_split_end(WLX_Context *ctx);
#endif // WLX_DEBUG

#ifdef WLX_PERF
static inline void wlx_perf_frame_begin(WLX_Context *ctx);
static inline void wlx_perf_input_begin(WLX_Context *ctx);
static inline void wlx_perf_input_end(WLX_Context *ctx);
static inline void wlx_perf_begin_end(WLX_Context *ctx);
static inline void wlx_perf_end_begin(WLX_Context *ctx);
static inline void wlx_perf_range_begin(WLX_Context *ctx);
static inline void wlx_perf_range_end(WLX_Context *ctx);
static inline void wlx_perf_offset_begin(WLX_Context *ctx);
static inline void wlx_perf_offset_end(WLX_Context *ctx);
static inline void wlx_perf_dispatch_begin(WLX_Context *ctx);
static inline void wlx_perf_dispatch_end(WLX_Context *ctx);
static inline void wlx_perf_backend_callback_begin(WLX_Context *ctx, WLX_Cmd_Type type);
static inline void wlx_perf_backend_callback_end(WLX_Context *ctx, WLX_Cmd_Type type);
static inline void wlx_perf_command_record(WLX_Context *ctx, WLX_Cmd_Type type);
static inline void wlx_perf_text_measure(WLX_Context *ctx, size_t bytes);
static inline void wlx_perf_text_command(WLX_Context *ctx, size_t bytes);
static inline void wlx_perf_text_run(WLX_Context *ctx, size_t bytes);
static inline void wlx_perf_frame_publish(WLX_Context *ctx);
static inline void wlx_perf_destroy(WLX_Context *ctx);
#endif



// ============================================================================
// Theme preset definitions
// ============================================================================

// Shared default for disabled-state alpha multiplier across built-in presets.
// Custom themes that omit `disabled_opacity` get the back-compat fall-back
// (no multiply); the built-in presets opt into this dim treatment.
#define WLX_DEFAULT_DISABLED_OPACITY 0.55f

const WLX_Theme wlx_theme_dark = {
    .background       = { 27,  27,  27, 255},
    .foreground       = {200, 200, 200, 255},
    .surface          = { 40,  40,  40, 255},
    .border           = { 60,  60,  60, 255},
    .border_width     = 0,
    .accent           = { 90, 140, 210, 255},
    .font              = WLX_FONT_DEFAULT,
    .font_size        = 16,
    .padding          = 0,
    .roundness        = 0,
    .rounded_segments = 0,
    .min_rounded_segments = 16,
    .hover_brightness = 0.08f,
    .disabled_brightness = -0.35f,
    .opacity          = -1,
    .disabled_opacity = WLX_DEFAULT_DISABLED_OPACITY,
    .input = {
        .border_focus = { 90, 140, 210, 255},
        .cursor       = {200, 200, 200, 255},
        .selection    = { 90, 140, 210,  90},
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
    .padding          = 0,
    .roundness        = 0,
    .rounded_segments = 0,
    .min_rounded_segments = 16,
    .hover_brightness = -0.08f,
    .disabled_brightness = 0.30f,
    .opacity          = -1,
    .disabled_opacity = WLX_DEFAULT_DISABLED_OPACITY,
    .input = {
        .border_focus = { 55,  80, 190, 255},
        .cursor       = { 30,  35,  50, 255},
        .selection    = { 55,  80, 190,  60},
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
    .padding          = 0,
    .roundness        = 0,
    .rounded_segments = 0,
    // .roundness        = 0.35f,
    // .rounded_segments = 6,
    .min_rounded_segments = 16,
    .hover_brightness = 0.05f,
    .disabled_brightness = -0.25f,
    .opacity          = -1,
    .disabled_opacity = WLX_DEFAULT_DISABLED_OPACITY,
    .input = {
        .border_focus = { 90, 110, 200, 255},
        .cursor       = {210, 215, 235, 255},
        .selection    = { 90, 110, 200, 100},
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

// Linear interpolate every channel of two colors. t clamps to [0, 1].
static inline WLX_Color wlx_color_lerp(WLX_Color a, WLX_Color b, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    WLX_Color r;
    r.r = wlx_clamp_u8((int)((float)a.r + ((float)b.r - (float)a.r) * t + 0.5f));
    r.g = wlx_clamp_u8((int)((float)a.g + ((float)b.g - (float)a.g) * t + 0.5f));
    r.b = wlx_clamp_u8((int)((float)a.b + ((float)b.b - (float)a.b) * t + 0.5f));
    r.a = wlx_clamp_u8((int)((float)a.a + ((float)b.a - (float)a.a) * t + 0.5f));
    return r;
}

// Hover-state brightness tint. Returns the input color unchanged when hover
// is false or disabled is true; otherwise returns wlx_color_brightness(c,
// brightness). Centralizes the (hover && !disabled) gate so it stays in
// sync across widgets when future visual states (pressed, focus) join.
static inline WLX_Color wlx_color_hover_tint(
    WLX_Color c, bool hover, bool disabled, float brightness)
{
    return (hover && !disabled) ? wlx_color_brightness(c, brightness) : c;
}

// ============================================================================
// Implementation: command recording (strict deferral)
// ============================================================================

static inline void *wlx_scratch_alloc_bytes(WLX_Context *ctx, size_t size, size_t align);

// Defined with the clip helpers further down; forward-declared so the cull test
// below can run at record time. Returns the active clip rect and whether one is
// set; the returned rect is always a superset of the true clip, so a rect fully
// outside it is certainly invisible.
static inline bool wlx_active_scissor_rect(const WLX_Context *ctx, WLX_Rect *out_rect);

// Opt-in cull predicate: true when ctx->cull_offscreen is set, a clip is active,
// and `rect` does not overlap it. Only rect-bounded primitives use this; text,
// shadow, glow, line, circle, and ring draw outside or independent of a single
// rect and are never culled.
static inline bool wlx_cmd_rect_culled(WLX_Context *ctx, WLX_Rect rect) {
    if (!ctx->cull_offscreen) return false;
    // wlx_active_scissor_rect answers for the current layer's clip context:
    // on a popup layer that is the overlay's own rect (or nothing), never a
    // base-layer clip the overlay escapes, so culling is safe on every layer.
    WLX_Rect clip;
    if (!wlx_active_scissor_rect(ctx, &clip)) return false;
    WLX_Rect it = wlx_rect_intersect(rect, clip);
    return it.w <= 0.0f || it.h <= 0.0f;
}

// Internal: build and push a typed draw command in one step.
// payload_field is the union member name; variadic args are the payload initializers.
#define WLX_CMD_RECORD(ctx, cmd_type, payload_field, ...) do {                     \
    WLX_Cmd _cmd = { .type = (cmd_type), .data.payload_field = { __VA_ARGS__ } }; \
    wlx_pool_push(&(ctx)->arena.commands, WLX_Cmd, _cmd);                         \
    WLX_PERF_HOOK(command_record, (ctx), (cmd_type));                              \
} while (0)

// Variant for commands that carry no payload data (e.g. WLX_CMD_SCISSOR_END).
#define WLX_CMD_RECORD_BARE(ctx, cmd_type) do {               \
    WLX_Cmd _cmd = { .type = (cmd_type) };                    \
    wlx_pool_push(&(ctx)->arena.commands, WLX_Cmd, _cmd);     \
    WLX_PERF_HOOK(command_record, (ctx), (cmd_type));          \
} while (0)

static inline void wlx_cmd_record_rect(WLX_Context *ctx, WLX_Rect rect, WLX_Color color) {
    if (wlx_cmd_rect_culled(ctx, rect)) return;
    WLX_CMD_RECORD(ctx, WLX_CMD_RECT, rect, rect, color);
}

static inline void wlx_cmd_record_rect_lines(WLX_Context *ctx, WLX_Rect rect, float thick, WLX_Color color) {
    if (wlx_cmd_rect_culled(ctx, rect)) return;
    WLX_CMD_RECORD(ctx, WLX_CMD_RECT_LINES, rect_lines, rect, thick, color);
}

static inline void wlx_cmd_record_rect_rounded(WLX_Context *ctx, WLX_Rect rect, float roundness, int segments, WLX_Color color) {
    if (wlx_cmd_rect_culled(ctx, rect)) return;
    WLX_CMD_RECORD(ctx, WLX_CMD_RECT_ROUNDED, rect_rounded, rect, roundness, segments, color);
}

static inline void wlx_cmd_record_rect_rounded_lines(WLX_Context *ctx, WLX_Rect rect, float roundness, int segments, float thick, WLX_Color color) {
    if (wlx_cmd_rect_culled(ctx, rect)) return;
    WLX_CMD_RECORD(ctx, WLX_CMD_RECT_ROUNDED_LINES, rect_rounded_lines, rect, roundness, segments, thick, color);
}

static inline void wlx_cmd_record_circle(WLX_Context *ctx, float cx, float cy, float radius, int segments, WLX_Color color) {
    WLX_CMD_RECORD(ctx, WLX_CMD_CIRCLE, circle, cx, cy, radius, segments, color);
}

static inline void wlx_cmd_record_ring(WLX_Context *ctx, float cx, float cy, float inner_r, float outer_r, int segments, WLX_Color color) {
    WLX_CMD_RECORD(ctx, WLX_CMD_RING, ring, cx, cy, inner_r, outer_r, segments, color);
}

// Record a deferred soft drop-shadow. color carries effective opacity already.
// At replay the dispatcher calls backend.draw_shadow when set, else the
// software fallback (layered offset rounded rects).
static inline void wlx_cmd_record_shadow(WLX_Context *ctx, WLX_Rect rect,
        WLX_Color color, float offset_x, float offset_y, float blur,
        int layers, float roundness, int rounded_segs) {
    WLX_CMD_RECORD(ctx, WLX_CMD_SHADOW, shadow,
                   rect, color, offset_x, offset_y, blur, layers,
                   roundness, rounded_segs);
}

// Record a deferred outer glow. color carries effective opacity already. At
// replay the dispatcher calls backend.draw_glow when set, else the software
// fallback (concentric expanding rounded outline rings).
static inline void wlx_cmd_record_glow(WLX_Context *ctx, WLX_Rect rect,
        WLX_Color color, float spread, int rings,
        float roundness, int rounded_segs) {
    WLX_CMD_RECORD(ctx, WLX_CMD_GLOW, glow,
                   rect, color, spread, rings, roundness, rounded_segs);
}

// Record a deferred vertical two-stop gradient. top/bottom carry effective
// opacity already. At replay the dispatcher calls backend.draw_gradient_v when
// set, else the software fallback (stacked solid bands).
static inline void wlx_cmd_record_gradient_v(WLX_Context *ctx, WLX_Rect rect,
        WLX_Color top, WLX_Color bottom, float roundness, int rounded_segs) {
    if (wlx_cmd_rect_culled(ctx, rect)) return;
    WLX_CMD_RECORD(ctx, WLX_CMD_GRADIENT_V, gradient_v,
                   rect, top, bottom, roundness, rounded_segs);
}

static inline void wlx_cmd_record_line(WLX_Context *ctx, float x1, float y1, float x2, float y2, float thick, WLX_Color color) {
    WLX_CMD_RECORD(ctx, WLX_CMD_LINE, line, x1, y1, x2, y2, thick, color);
}

static inline void wlx_cmd_record_text_span(WLX_Context *ctx, const char *text, size_t len, float x, float y, WLX_Text_Style style) {
    size_t off = 0;
    if (len > 0) {
        char *copy = (char *)wlx_scratch_alloc_bytes(ctx, len, 1);
        memcpy(copy, text, len);
        off = (size_t)(copy - (char *)wlx_pool_scratch(ctx));
    }
    WLX_PERF_HOOK(text_command, ctx, len);
    WLX_CMD_RECORD(ctx, WLX_CMD_TEXT, text, off, len, x, y, style);
}

static inline void wlx_cmd_record_text(WLX_Context *ctx, const char *text, float x, float y, WLX_Text_Style style) {
    if (text == NULL) text = "";
    wlx_cmd_record_text_span(ctx, text, strlen(text), x, y, style);
}

static inline void wlx_cmd_record_texture(WLX_Context *ctx, WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    if (wlx_cmd_rect_culled(ctx, dst)) return;
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
    int layer = ctx->current_layer;
    if (layer >= WLX_OVERLAY_MAX_LAYERS) layer = WLX_OVERLAY_MAX_LAYERS - 1;
    WLX_Cmd_Range range = {
        .start_idx = ctx->arena.commands.count,
        .end_idx = 0,
        .dy_offset = 0.0f,
        .dx_offset = 0.0f,
        .layer = layer,
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

// Temporary NUL-terminated copy of a byte span, for the legacy backend
// callbacks that take C strings. The span is never peeked past `len` (a slice
// at the end of an allocation must not read text[len]); the copy is always
// made. Stack buffer for short spans, heap fallback for long ones.
// wlx_cstr_tmp_begin returns NULL when the heap fallback fails;
// wlx_cstr_tmp_end releases the heap copy.
typedef struct {
    char  stack_buf[WLX_TEXT_RANGE_STACK_CAP];
    char *ptr;
    bool  heap;
} WLX_CStr_Tmp;

static inline const char *wlx_cstr_tmp_begin(WLX_CStr_Tmp *t,
        const char *text, size_t len) {
    t->ptr = t->stack_buf;
    t->heap = false;
    if (len + 1 > sizeof(t->stack_buf)) {
        t->ptr = (char *)wlx_alloc(len + 1);
        if (t->ptr == NULL) return NULL;
        t->heap = true;
    }
    if (len > 0) memcpy(t->ptr, text, len);
    t->ptr[len] = '\0';
    return t->ptr;
}

static inline void wlx_cstr_tmp_end(WLX_CStr_Tmp *t) {
    if (t->heap) wlx_free(t->ptr);
}

// Private: measure a byte span (text, len).
// Prefers measure_text_slice when available; falls back to the legacy
// measure_text callback via a temporary null-terminated copy.
// The style a text callback receives: the context's style transform
// applied to the nominal style, at the backend boundary only.
static inline WLX_Text_Style wlx_style_at_boundary(const WLX_Context *ctx, WLX_Text_Style style) {
    return ctx->style_transform != NULL
        ? ctx->style_transform(style, ctx->style_transform_user) : style;
}

// NULL text is normalized to an empty string.
static inline void wlx_span_measure_text(WLX_Context *ctx,
        const char *text, size_t len,
        WLX_Text_Style style, float *out_w, float *out_h) {
    if (text == NULL) { text = ""; len = 0; }
    style = wlx_style_at_boundary(ctx, style);

    if (ctx->backend.measure_text_slice != NULL) {
        ctx->backend.measure_text_slice(text, len, style, out_w, out_h, ctx->backend.user);
        return;
    }

    WLX_CStr_Tmp tmp;
    const char *cstr = wlx_cstr_tmp_begin(&tmp, text, len);
    if (cstr == NULL) {
        if (out_w) *out_w = 0.0f;
        if (out_h) *out_h = 0.0f;
        return;
    }
    ctx->backend.measure_text(cstr, style, out_w, out_h, ctx->backend.user);
    wlx_cstr_tmp_end(&tmp);
}

// Private: draw a byte span in immediate mode.
// Prefers draw_text_slice when available; falls back to the legacy draw_text
// callback via a temporary null-terminated copy.
static inline void wlx_draw_text_span_immediate(WLX_Context *ctx,
        const char *text, size_t len, float x, float y, WLX_Text_Style style) {
    if (text == NULL) { text = ""; len = 0; }
    style = wlx_style_at_boundary(ctx, style);

    if (ctx->backend.draw_text_slice != NULL) {
        ctx->backend.draw_text_slice(text, len, x, y, style, ctx->backend.user);
        return;
    }

    WLX_CStr_Tmp tmp;
    const char *cstr = wlx_cstr_tmp_begin(&tmp, text, len);
    if (cstr == NULL) return;
    ctx->backend.draw_text(cstr, x, y, style, ctx->backend.user);
    wlx_cstr_tmp_end(&tmp);
}

// Private: draw-or-record a byte span (immediate or deferred).
static inline void wlx_draw_text_span(WLX_Context *ctx,
        const char *text, size_t len, float x, float y, WLX_Text_Style style) {
    if (text == NULL) { text = ""; len = 0; }
    if (ctx->immediate_mode) {
        wlx_draw_text_span_immediate(ctx, text, len, x, y, style);
    } else {
        wlx_cmd_record_text_span(ctx, text, len, x, y, style);
    }
}

// Public: measure a byte span. This is the canonical slice-based entry; the
// C-string wlx_measure_text is a one-strlen shim around this helper. NULL
// text is normalized to an empty span.
static inline void wlx_measure_text_slice(WLX_Context *ctx,
        const char *text, size_t len,
        WLX_Text_Style style, float *out_w, float *out_h) {
    assert((ctx->backend.measure_text != NULL || ctx->backend.measure_text_slice != NULL) && "WLX_Backend.measure_text or measure_text_slice must be set");
    if (text == NULL) { text = ""; len = 0; }
    WLX_PERF_HOOK(text_measure, ctx, len);
    wlx_span_measure_text(ctx, text, len, style, out_w, out_h);
}

// Public: draw a byte span (immediate or deferred). C-string wlx_draw_text is
// a one-strlen shim around this helper.
static inline void wlx_draw_text_slice(WLX_Context *ctx,
        const char *text, size_t len, float x, float y, WLX_Text_Style style) {
    if (ctx->immediate_mode) {
        assert((ctx->backend.draw_text != NULL || ctx->backend.draw_text_slice != NULL) && "WLX_Backend.draw_text or draw_text_slice must be set");
    }
    if (text == NULL) { text = ""; len = 0; }
    wlx_draw_text_span(ctx, text, len, x, y, style);
}

static inline void wlx_measure_text(WLX_Context *ctx, const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
    if (text == NULL) text = "";
    wlx_measure_text_slice(ctx, text, strlen(text), style, out_w, out_h);
}

static inline void wlx_draw_text(WLX_Context *ctx, const char *text, float x, float y, WLX_Text_Style style) {
    if (text == NULL) text = "";
    wlx_draw_text_slice(ctx, text, strlen(text), x, y, style);
}

static inline void wlx_draw_rect(WLX_Context *ctx, WLX_Rect rect, WLX_Color color) {
    if (ctx->immediate_mode) {
        assert(ctx->backend.draw_rect != NULL && "WLX_Backend.draw_rect must be set");
        ctx->backend.draw_rect(rect, color, ctx->backend.user);
    } else {
        wlx_cmd_record_rect(ctx, rect, color);
    }
}

// Sub-pixel outline rule: backends get a 1 px line with the alpha scaled
// by the requested thickness, so a faint outline looks the same everywhere.
static inline void wlx_outline_subpixel(float *thick, WLX_Color *color) {
    if (*thick < 1.0f) {
        *color = wlx_color_apply_opacity(*color, *thick);
        *thick = 1.0f;
    }
}

static inline void wlx_draw_rect_lines(WLX_Context *ctx, WLX_Rect rect, float thick, WLX_Color color) {
    wlx_outline_subpixel(&thick, &color);
    if (ctx->immediate_mode) {
        assert(ctx->backend.draw_rect_lines != NULL && "WLX_Backend.draw_rect_lines must be set");
        ctx->backend.draw_rect_lines(rect, thick, color, ctx->backend.user);
    } else {
        wlx_cmd_record_rect_lines(ctx, rect, thick, color);
    }
}

static inline void wlx_draw_rect_rounded(WLX_Context *ctx, WLX_Rect rect, float roundness, int segments, WLX_Color color) {
    if (roundness >= 1.0f && fabsf(rect.w - rect.h) < 0.5f
            && ctx->backend.draw_circle) {
        float r = rect.w * 0.5f;
        if (ctx->immediate_mode) {
            ctx->backend.draw_circle(rect.x + r, rect.y + r, r, segments, color, ctx->backend.user);
        } else {
            wlx_cmd_record_circle(ctx, rect.x + r, rect.y + r, r, segments, color);
        }
        return;
    }
    if (ctx->immediate_mode) {
        assert(ctx->backend.draw_rect_rounded != NULL && "WLX_Backend.draw_rect_rounded must be set");
        ctx->backend.draw_rect_rounded(rect, roundness, segments, color, ctx->backend.user);
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
                                   segments, color, ctx->backend.user);
        } else {
            wlx_cmd_record_ring(ctx, rect.x + r, rect.y + r, r - thick, r,
                                segments, color);
        }
        return;
    }
    wlx_outline_subpixel(&thick, &color);
    if (ctx->immediate_mode) {
        assert(ctx->backend.draw_rect_rounded_lines != NULL && "WLX_Backend.draw_rect_rounded_lines must be set");
        ctx->backend.draw_rect_rounded_lines(rect, roundness, segments, thick, color, ctx->backend.user);
    } else {
        wlx_cmd_record_rect_rounded_lines(ctx, rect, roundness, segments, thick, color);
    }
}

static inline void wlx_draw_line(WLX_Context *ctx, float x1, float y1, float x2, float y2, float thick, WLX_Color color) {
    if (ctx->immediate_mode) {
        assert(ctx->backend.draw_line != NULL && "WLX_Backend.draw_line must be set");
        ctx->backend.draw_line(x1, y1, x2, y2, thick, color, ctx->backend.user);
    } else {
        wlx_cmd_record_line(ctx, x1, y1, x2, y2, thick, color);
    }
}

static inline void wlx_draw_texture(WLX_Context *ctx, WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    if (ctx->immediate_mode) {
        assert(ctx->backend.draw_texture != NULL && "WLX_Backend.draw_texture must be set");
        ctx->backend.draw_texture(texture, src, dst, tint, ctx->backend.user);
    } else {
        wlx_cmd_record_texture(ctx, texture, src, dst, tint);
    }
}

// Frame delta seconds: the per-frame sample taken in wlx_begin. All reads
// within one frame agree; the backend is never called here.
static inline float wlx_get_frame_time(WLX_Context *ctx) {
    return ctx->frame_dt;
}

static inline void wlx_begin_scissor(WLX_Context *ctx, WLX_Rect rect) {
    if (ctx->immediate_mode) {
        assert(ctx->backend.begin_scissor != NULL && "WLX_Backend.begin_scissor must be set");
        ctx->backend.begin_scissor(rect, ctx->backend.user);
    } else {
        wlx_cmd_record_scissor_begin(ctx, rect);
    }
}

static inline void wlx_end_scissor(WLX_Context *ctx) {
    if (ctx->immediate_mode) {
        assert(ctx->backend.end_scissor != NULL && "WLX_Backend.end_scissor must be set");
        ctx->backend.end_scissor(ctx->backend.user);
    } else {
        wlx_cmd_record_scissor_end(ctx);
    }
}

static inline void wlx_assert_backend_ready(WLX_Context *ctx) {
    assert(ctx != NULL && "WLX_Context must not be NULL");
    // Contract-version check, live in every build: a table of another
    // version compiles on compilers that only warn about the callback
    // signatures, and calling it through these signatures is memory-unsafe.
    WLX_HARD_ASSERT(ctx->backend.contract_version == WLX_BACKEND_CONTRACT_VERSION,
        "WLX_Backend contract v2 required: set .contract_version = "
        "WLX_BACKEND_CONTRACT_VERSION and add a trailing void *user parameter "
        "to every callback, or wrap a v1 table with wlx_backend_from_v1");
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

// Internal slot-padding resolver. Resolves uniform + per-side WLX_LAYOUT_SLOT
// padding fields; per-side `>= 0` wins over the uniform, otherwise the
// uniform is returned as-is (including negative sentinels). Public widget
// padding goes through wlx_resolve_content_padding instead.
static inline WLX_Resolved_Padding wlx_resolve_padding(
    float uniform, float pt, float pr, float pb, float pl) {
    return (WLX_Resolved_Padding){
        .top    = (pt >= 0.0f) ? pt : uniform,
        .right  = (pr >= 0.0f) ? pr : uniform,
        .bottom = (pb >= 0.0f) ? pb : uniform,
        .left   = (pl >= 0.0f) ? pl : uniform,
    };
}

// Resolve content padding. Per-side values >= 0 win; otherwise the side
// falls back to the uniform. An unset uniform resolves to `widget_default`
// (0 for leaf widgets, the widget's documented inset for compound ones)
// unless it is WLX_PADDING_USE_THEME, in which case the side resolves to
// the theme's `padding` knob (WLX_STYLE_CONTENT_PADDING default).
static inline WLX_Resolved_Padding wlx_resolve_content_padding_ex(
    const WLX_Theme *theme, float widget_default,
    float uniform, float pt, float pr, float pb, float pl) {
    bool theme_opt_in = (uniform == WLX_PADDING_USE_THEME);
    float theme_value = theme ? theme->padding : (float)WLX_STYLE_CONTENT_PADDING;
    float fallback = theme_opt_in ? theme_value : widget_default;
    float u = (uniform >= 0.0f) ? uniform : fallback;
    return (WLX_Resolved_Padding){
        .top    = (pt >= 0.0f) ? pt : u,
        .right  = (pr >= 0.0f) ? pr : u,
        .bottom = (pb >= 0.0f) ? pb : u,
        .left   = (pl >= 0.0f) ? pl : u,
    };
}

// Leaf-widget form (label, button, ...): an unset uniform is 0.
static inline WLX_Resolved_Padding wlx_resolve_content_padding(
    const WLX_Theme *theme,
    float uniform, float pt, float pr, float pb, float pl) {
    return wlx_resolve_content_padding_ex(theme, 0.0f, uniform, pt, pr, pb, pl);
}

// Clamp resolved padding so that left+right <= max_w and top+bottom <= max_h.
// Scales the opposing pair proportionally rather than clipping one side, so
// the inset stays balanced. Never produces negative values. Mirrors the
// inputbox content_padding clamp.
static inline void wlx_clamp_resolved_padding(WLX_Resolved_Padding *rp, float max_w, float max_h) {
    if (rp->left < 0.0f) rp->left = 0.0f;
    if (rp->right < 0.0f) rp->right = 0.0f;
    if (rp->top < 0.0f) rp->top = 0.0f;
    if (rp->bottom < 0.0f) rp->bottom = 0.0f;

    if (max_w < 0.0f) max_w = 0.0f;
    if (max_h < 0.0f) max_h = 0.0f;

    float hsum = rp->left + rp->right;
    if (hsum > max_w && hsum > 0.0f) {
        float s = max_w / hsum;
        rp->left  *= s;
        rp->right *= s;
    }
    float vsum = rp->top + rp->bottom;
    if (vsum > max_h && vsum > 0.0f) {
        float s = max_h / vsum;
        rp->top    *= s;
        rp->bottom *= s;
    }
}

// Resolve content padding, clamp it to widget_rect, and return the inset
// content rect. Collapses the three-line "resolve, clamp, inset" pattern
// repeated by every content-padded leaf widget. Inputbox does not use this
// helper because its min-input-width calculation needs the resolved padding
// before the inset step.
static inline WLX_Rect wlx_resolve_content_rect_full(
    const WLX_Theme *theme, WLX_Rect widget_rect,
    float content_padding,
    float content_padding_top, float content_padding_right,
    float content_padding_bottom, float content_padding_left)
{
    WLX_Resolved_Padding rp = wlx_resolve_content_padding(theme,
        content_padding, content_padding_top, content_padding_right,
        content_padding_bottom, content_padding_left);
    wlx_clamp_resolved_padding(&rp, widget_rect.w, widget_rect.h);
    return wlx_rect_inset_sides(widget_rect, rp.top, rp.right, rp.bottom, rp.left);
}

// Convenience macro for widgets that embed WLX_CONTENT_PADDING_FIELDS.
#define WLX_RESOLVE_CONTENT_RECT(ctx, opt, widget_rect) \
    wlx_resolve_content_rect_full((ctx)->theme, (widget_rect), \
        (opt).content_padding, \
        (opt).content_padding_top, (opt).content_padding_right, \
        (opt).content_padding_bottom, (opt).content_padding_left)

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
    // Intrinsic (natural) width offered to a HORZ CONTENT parent slot when
    // no explicit width is set; 0 = none. Always the single-line unwrapped
    // measure: a pure function of content, so the contribution never depends
    // on the width being computed.
    float intrinsic_w;
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
        .align = (opt).slot_align, .overflow = (opt).overflow, \
    }

// Aggregated description of a child's contribution to its parent layout's
// content tracking: measured height and (when the child has one) intrinsic
// or explicit width. Pass WLX_SLOT_SKIP for slot_index / grid_row to skip
// the per-slot CONTENT or per-row grid bucket update.
typedef struct {
    float  h_contrib;
    float  w_contrib;
    size_t slot_index;
    size_t grid_row;
} WLX_Parent_Contribution;

// Single entry point for "child contributes its measured extents to parent
// content tracking". Updates the parent's accumulated_content_height
// (HORZ max, VERT/grid sum; height-only by design - it feeds auto-height
// scroll panels), the per-slot CONTENT bucket (which stores the parent's
// MAIN-AXIS extent: child heights for VERT parents, child widths for HORZ
// parents), and the per-row grid bucket (max height across cells in the
// same row).
static inline void wlx_contribute_to_parent_layout(
    WLX_Context *ctx, WLX_Layout *parent, WLX_Parent_Contribution c)
{
    if (wlx_layout_is_horz(parent)) {
        if (c.h_contrib > parent->accumulated_content_height)
            parent->accumulated_content_height = c.h_contrib;
    } else {
        parent->accumulated_content_height += c.h_contrib;
    }

    if (parent->has_content_slot_measures && c.slot_index < WLX_CONTENT_SLOTS_MAX) {
        float main_extent = wlx_layout_is_horz(parent) ? c.w_contrib : c.h_contrib;
        wlx_layout_content_measures(ctx, parent)[c.slot_index] += main_extent;
    }

    if (parent->has_grid_row_content_heights && c.grid_row < parent->grid.rows) {
        float *rch = wlx_grid_row_content_heights(ctx, parent);
        if (c.h_contrib > rch[c.grid_row])
            rch[c.grid_row] = c.h_contrib;
    }
}

// True when the slot this widget is about to occupy in the innermost layout
// is a CONTENT slot of a HORZ linear layout - i.e. the parent will consume
// an intrinsic width. Widgets gate their intrinsic measure on this, so
// steady-state measure traffic is unchanged wherever the feature is unused.
// pos: the widget's slot override (< 0 = next sequential slot).
static inline bool wlx_parent_wants_intrinsic_width(WLX_Context *ctx, int pos) {
    if (ctx->arena.layouts.count == 0) return false;
    const WLX_Layout *parent = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
    if (!wlx_layout_is_horz(parent)) return false;
    return wlx_layout_slot_is_content(ctx, parent, (pos >= 0) ? (size_t)pos : parent->index);
}

// Resolved left+right content padding, for adding around an intrinsic
// content width.
static inline float wlx_intrinsic_pad_lr(const WLX_Context *ctx,
    float pad, float pt, float pr, float pb, float pl)
{
    WLX_Resolved_Padding rp = wlx_resolve_content_padding(ctx->theme, pad, pt, pr, pb, pl);
    return rp.left + rp.right;
}

// Single-line (unwrapped) measured width of a widget's text span. Routes
// through wlx_measure_text_slice so intrinsic traffic hits the perf hook the
// draw path uses; callers gate on wlx_parent_wants_intrinsic_width and pass
// the length they already computed (one strlen per widget).
static inline float wlx_intrinsic_text_width_slice(WLX_Context *ctx,
    const char *text, size_t len, WLX_Text_Style ts)
{
    if (text == NULL || len == 0 || ts.font_size <= 0) return 0.0f;
    float w = 0.0f, h = 0.0f;
    wlx_measure_text_slice(ctx, text, len, ts, &w, &h);
    return w;
}

// C-string shim: one strlen, then the slice measure. For callers that do not
// already hold the length (a dropdown's per-option measures, the tooltip).
static inline float wlx_intrinsic_text_width(WLX_Context *ctx, const char *text,
    WLX_Text_Style ts)
{
    return text != NULL ? wlx_intrinsic_text_width_slice(ctx, text, strlen(text), ts) : 0.0f;
}

// Reference line height for a text style: the backend's height for a
// space, falling back to the font size when the backend reports none.
// out_space_w (optional) receives the space advance (tab stops).
static inline float wlx_text_line_height(WLX_Context *ctx, WLX_Text_Style style, float *out_space_w) {
    float w = 0.0f, h = 0.0f;
    wlx_measure_text_slice(ctx, " ", 1, style, &w, &h);
    if (out_space_w) *out_space_w = w;
    return (h > 0.0f) ? h : (float)style.font_size;
}

// Defined with the widget-content layout helpers below; declared here so the
// intrinsic width can reserve the same band the face draws.
static inline float wlx_widget_auto_image_size(float image_size, WLX_Image_Placement placement,
    WLX_Rect widget_rect, float font_size, bool has_text);

// Intrinsic content width of an image-capable text widget: measured text
// plus the image band for side placements, or the wider of the two for
// TOP/BOTTOM (the image stacks over the text column there). Callers add
// resolved padding.
static inline float wlx_intrinsic_text_image_width(WLX_Context *ctx,
    const char *text, size_t len, WLX_Text_Style ts,
    WLX_Texture texture, WLX_Rect texture_src, float image_size,
    WLX_Image_Placement image_placement, float image_text_gap)
{
    float text_w = wlx_intrinsic_text_width_slice(ctx, text, len, ts);
    float img_w = 0.0f;
    if (texture.width > 0) {
        // With text, the face reserves the auto band (image_size, else a
        // font-derived band) - a zero rect disables the draw-time clamp, so
        // the result stays a pure function of content. Without text the
        // face fills the cell; the texture is the only content-derived
        // width an intrinsic can report.
        img_w = (text_w > 0.0f)
            ? wlx_widget_auto_image_size(image_size, image_placement,
                                         (WLX_Rect){0}, ts.font_size, true)
            : ((image_size > 0.0f) ? image_size
               : (texture_src.w > 0.0f) ? texture_src.w : (float)texture.width);
    }
    if (img_w <= 0.0f) return text_w;
    if (image_placement == WLX_IMAGE_PLACEMENT_TOP
        || image_placement == WLX_IMAGE_PLACEMENT_BOTTOM) {
        return text_w > img_w ? text_w : img_w;
    }
    return text_w + ((text_w > 0.0f) ? image_text_gap : 0.0f) + img_w;
}

static inline WLX_Widget_Rect wlx_widget_begin(WLX_Context *ctx, WLX_Widget_Layout ly)
{
    WLX_Rect cell = wlx_get_widget_cell_rect(ctx, ly.pos, ly.span, ly.padding,
        ly.padding_top, ly.padding_right, ly.padding_bottom, ly.padding_left);

    wlx_cmd_close_sibling_range(ctx);
    wlx_cmd_open_range(ctx);

    // Contribute to the parent layout: height always (`wlx_layout_end()`
    // folds the accumulated total into auto_scroll_total_height, so widgets
    // never need to know about scroll panels), width when explicit or
    // intrinsic.
    if (ctx->arena.layouts.count > 0) {
        WLX_Layout *parent_l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
        float h_contrib = (ly.height > 0) ? ly.height
                       : (ly.min_h > cell.h) ? ly.min_h : cell.h;
        // l->index was just advanced by wlx_get_slot_rect, so the slot this
        // widget occupies is at (index - span).
        // Width: explicit size wins, else the widget's intrinsic width; no
        // cell fallback - for a CONTENT slot the provisional cell width is
        // last frame's measurement, and feeding it back would be circular.
        wlx_contribute_to_parent_layout(ctx, parent_l, (WLX_Parent_Contribution){
            .h_contrib  = h_contrib,
            .w_contrib  = (ly.width > 0) ? ly.width : ly.intrinsic_w,
            .slot_index = parent_l->index - ly.span,
            .grid_row   = parent_l->grid.last_placed_row,
        });
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

// FNV-1a 64-bit over [bytes, bytes + len): the byte-content hash the backend
// measurement caches key their text with.
static inline uint64_t wlx_hash_fnv1a64(const char *bytes, size_t len) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; ++i) {
        h ^= (uint64_t)(unsigned char)bytes[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

// Scope-id push/pop helpers: convert an optional string `id` into an id-stack
// push, and remember whether a push happened so the matching pop can be a
// single-condition branch. Used by every frame helper that supports a `.id`
// scope (widget, layout, scroll-panel). The returned bool is the only state
// the caller must retain.
static inline bool wlx_scope_push(WLX_Context *ctx, const char *id) {
    if (!id) return false;
    wlx_push_id(ctx, wlx_hash_string(id));
    return true;
}

static inline void wlx_scope_pop(WLX_Context *ctx, bool was_pushed) {
    if (was_pushed) wlx_pop_id(ctx);
}

// Wraps the per-widget prologue/epilogue: push_id (if set), widget_begin, DBG.
// Two shapes use it:
//   - Leaf widgets (button, label, inputbox, ...): begin with opt.id, end
//     with the returned frame - the frame owns the widget's scope, and any
//     persistent state is fetched inside it.
//   - Compound widgets whose scope must span more than one inner frame or a
//     subtree (dropdown, menu button, menus, overlay, scroll panel): push
//     the scope themselves (wlx_scope_push(opt.id)), fetch state under it,
//     build inner frames / faces with id = NULL, and pop it last. The rule
//     is: whoever pushes the scope pops it; an inner frame under an
//     externally owned scope never pushes one.
// Store the returned frame and pass it to end: wlx_widget_frame_end(ctx, frame).
typedef struct {
    WLX_Rect slot_rect;
    WLX_Rect rect;
    bool pushed_scope;
} WLX_Widget_Frame;

static inline WLX_Widget_Frame wlx_widget_frame_begin(
    WLX_Context *ctx, const char *id, WLX_Widget_Layout ly,
    const char *file, int line)
{
    WLX_UNUSED(file);
    WLX_UNUSED(line);

    bool pushed = wlx_scope_push(ctx, id);
    WLX_Widget_Rect wg = wlx_widget_begin(ctx, ly);
    WLX_DBG(widget_begin, ctx, wg.slot_rect, ly.height, (int)ly.span, ly.overflow, file, line);
    ctx->last_widget_rect = wg.rect;
    return (WLX_Widget_Frame){ .slot_rect = wg.slot_rect, .rect = wg.rect, .pushed_scope = pushed };
}

static inline void wlx_widget_frame_end(WLX_Context *ctx, WLX_Widget_Frame frame)
{
    wlx_scope_pop(ctx, frame.pushed_scope);
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

typedef struct {
    bool parent_active;
    WLX_Rect parent_rect;
} WLX_Scissor_Scope;

// The one walker over the current layer's clip context (wollix.h keeps two
// flavors of "all active clips", both bounded by ctx->clip_base):
//   WLX_CLIP_DRAW       - what the backend scissor is: an explicit scissor
//                         scope when one is open (already intersected with
//                         its parent at push), else every scroll panel
//                         viewport, clip layout and overlay-root clip rect
//                         of this layer intersected. Every drawing walker
//                         uses it: layout and overlay clips, scroll-panel
//                         scissors and their re-arm.
//   WLX_CLIP_CONTAINERS - the same containers without the scissor-scope
//                         short-circuit: what hit-testing uses, so hit zones
//                         stop where the drawing stops. Explicit scissor
//                         scopes wrap primitive draws and never gate the
//                         pointer.
// An overlay root with opt.clip contributes its own rect on both flavors, so
// popup content is clipped, culled and hit-tested against the popup itself
// and never against the base panels it floats over.
typedef enum { WLX_CLIP_DRAW, WLX_CLIP_CONTAINERS } WLX_Clip_Query;

static inline bool wlx_enclosing_clip(const WLX_Context *ctx, WLX_Clip_Query query, WLX_Rect *out_rect) {
    bool active = false;
    WLX_Rect clip = {0};

    if (query == WLX_CLIP_DRAW && ctx->scissor_stack_count > ctx->clip_base.scissor_from) {
        if (out_rect != NULL) *out_rect = ctx->scissor_stack[ctx->scissor_stack_count - 1];
        return true;
    }

    for (size_t i = ctx->clip_base.panels_from; i < ctx->arena.scroll_panels.count; i++) {
        WLX_Scroll_Panel_State *state = wlx_pool_scroll_panels(ctx)[i];
        if (state == NULL) continue;
        clip = active ? wlx_rect_intersect(clip, state->panel_rect) : state->panel_rect;
        active = true;
    }

    for (size_t i = ctx->clip_base.layouts_from; i < ctx->arena.layouts.count; i++) {
        const WLX_Layout *layout = &wlx_pool_layouts(ctx)[i];
        bool clips = (layout->is_overlay_root && layout->overlay_clip)
                  || layout->clip_active;
        if (!clips) continue;
        clip = active ? wlx_rect_intersect(clip, layout->clip_rect) : layout->clip_rect;
        active = true;
    }

    if (active && out_rect != NULL) *out_rect = clip;
    return active;
}

static inline bool wlx_active_scissor_rect(const WLX_Context *ctx, WLX_Rect *out_rect) {
    return wlx_enclosing_clip(ctx, WLX_CLIP_DRAW, out_rect);
}

// Opt-in clip for the layout just pushed by a begin call: flag it and begin
// a scissor on its content rect, intersected with the active clip so nested
// clips never widen the visible region (e.g. inside a scroll panel). The
// chrome (background or border) was already recorded by
// wlx_layout_frame_begin, so it is never cropped. wlx_layout_end owns the
// matching release via clip_active. Shared by the counted and the
// auto-counted begin.
static inline void wlx_layout_clip_begin(WLX_Context *ctx) {
    WLX_Layout *top = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
    WLX_Rect clip_rect = top->rect;
    WLX_Rect active;
    if (wlx_active_scissor_rect(ctx, &active)) {
        clip_rect = wlx_rect_intersect(clip_rect, active);
    }
    top->clip_active = true;
    top->clip_rect = top->rect;   // the walkers intersect with it themselves
    wlx_begin_scissor(ctx, clip_rect);
}

static inline WLX_Scissor_Scope wlx_scissor_scope_begin(WLX_Context *ctx, WLX_Rect rect) {
    WLX_Scissor_Scope scope = {0};
    WLX_Rect clip = rect;

    WLX_HARD_ASSERT(ctx->scissor_stack_count < WLX_SCISSOR_STACK_MAX,
        "scissor scope stack overflow - nesting deeper than WLX_SCISSOR_STACK_MAX");

    scope.parent_active = wlx_active_scissor_rect(ctx, &scope.parent_rect);
    if (scope.parent_active) clip = wlx_rect_intersect(clip, scope.parent_rect);

    ctx->scissor_stack[ctx->scissor_stack_count++] = clip;
    wlx_begin_scissor(ctx, clip);
    return scope;
}

static inline void wlx_scissor_scope_end(WLX_Context *ctx, WLX_Scissor_Scope scope) {
    assert(ctx->scissor_stack_count > 0 && "wlx_scissor_scope_end without matching begin");
    if (ctx->scissor_stack_count > 0) ctx->scissor_stack_count--;
    wlx_end_scissor(ctx);
    if (scope.parent_active) wlx_begin_scissor(ctx, scope.parent_rect);
}

// Reserve `n` floats from the context slot sizes buffer, returning a pointer
// to the first element. The buffer grows as needed but is never freed -
// it is simply reset (count = 0) each frame in wlx_begin().
static inline float *wlx_scratch_alloc(WLX_Context *ctx, size_t n) {
    assert(ctx != NULL);
    assert(n > 0 && n <= WLX_MAX_SLOT_COUNT && "unreasonable scratch alloc - did you pass a negative count?");
    size_t base = wlx_sub_arena_alloc(&ctx->arena.slot_size_offsets, n);
    return &wlx_pool_slot_size_offsets(ctx)[base];
}

// Reserve `n` floats from the dynamic-layout offset buffer, returning a pointer
// to the first element. Mirrors wlx_scratch_alloc but uses the separate
// dyn_offsets arena so dynamic-layout growth cannot corrupt static offsets.
static inline float *wlx_dyn_scratch_alloc(WLX_Context *ctx, size_t n) {
    assert(ctx != NULL);
    assert(n > 0 && n <= WLX_MAX_SLOT_COUNT && "unreasonable dyn scratch alloc");
    size_t base = wlx_sub_arena_alloc(&ctx->arena.dyn_offsets, n);
    return &wlx_pool_dyn_offsets(ctx)[base];
}

// Reserve `size` bytes (with `align` alignment) from the generic byte scratch
// arena. Same lifetime as wlx_scratch_alloc - reset each frame in wlx_begin().
static inline void *wlx_scratch_alloc_bytes(WLX_Context *ctx, size_t size, size_t align) {
    assert(ctx != NULL);
    assert(size > 0);
    size_t off = wlx_sub_arena_alloc_bytes(&ctx->arena.scratch, size, align);
    return &wlx_pool_scratch(ctx)[off];
}

// The size a slot resolves to without the flex pool: PIXELS and CONTENT
// carry their value, PERCENT a share of the distributable total, FILL a share
// of the viewport. AUTO and FLEX resolve from the remaining space instead and
// report 0 here.
static inline float wlx_slot_fixed_size(const WLX_Slot_Size *s,
                                        float adjusted_total, float viewport)
{
    switch (s->kind) {
        case WLX_SIZE_AUTO:
        case WLX_SIZE_FLEX:    return 0.0f;
        case WLX_SIZE_PIXELS:  return s->value;
        case WLX_SIZE_PERCENT: return (s->value * adjusted_total) / 100.0f;
        case WLX_SIZE_FILL:    return s->value * viewport;
        case WLX_SIZE_CONTENT: return s->value; // pre-resolved or fallback 0
        default: WLX_UNREACHABLE("Undefined slot size kind"); return 0.0f;
    }
}

// Clamp a resolved size to the slot's min / max (0 = unconstrained).
static inline float wlx_slot_clamp(const WLX_Slot_Size *s, float size) {
    if (s->min > 0 && size < s->min) size = s->min;
    if (s->max > 0 && size > s->max) size = s->max;
    return size;
}

// Core offset math - fills offsets[0..count] given a total extent and
// optional per-slot sizes.  Used by both 1D layouts (single axis) and
// grid layouts (row axis + column axis).
// ctx provides the frame byte-scratch arena for the redistribution working
// buffers when count exceeds WLX_OFFSET_STACK_LIMIT; it may be NULL for
// standalone callers (the math needs no context), which then take a heap
// fallback. Callers with a context must not hold byte-scratch pointers
// across this call (the working-buffer allocation may grow the arena).
static inline void wlx_compute_offsets_ctx(WLX_Context *ctx,
                                           float *offsets, size_t count,
                                           float total, float viewport,
                                           const WLX_Slot_Size *sizes, float gap)
{
    assert(offsets != NULL);
    WLX_HARD_ASSERT(count > 0 && count <= WLX_MAX_SLOT_COUNT,
        "unreasonable slot count (negative count wrapped to size_t?)");
    WLX_UNUSED(ctx);  // unused under WLX_SLOT_SINGLE_PASS_CLAMP

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
            case WLX_SIZE_AUTO: total_weight += 1.0f; break;
            case WLX_SIZE_FLEX: total_weight += sizes[i].value; break;
            default:            used += wlx_slot_fixed_size(&sizes[i], adjusted_total, viewport); break;
        }
    }
    float remaining = (adjusted_total - used > 0.0f) ? (adjusted_total - used) : 0.0f;

#ifndef WLX_SLOT_SINGLE_PASS_CLAMP
    // Redistribution scratch: the exact clamped size of every slot (pass 2
    // records it, so the rebuild in pass 3 is the only snapping stage and
    // each slot lands within one pixel of that size) plus the freeze flags.
    bool has_constraints = false;
    for (size_t i = 0; i < count; i++) {
        if (sizes[i].min > 0 || sizes[i].max > 0) {
            has_constraints = true;
            break;
        }
    }
    // Stack-allocate working arrays (slot counts are small, typically < 20)
    bool frozen_buf[WLX_OFFSET_STACK_LIMIT];
    float raw_buf[WLX_OFFSET_STACK_LIMIT];
    bool *frozen = frozen_buf;
    float *raw = raw_buf;
    bool heap_scratch = false;
    if (has_constraints && count > WLX_OFFSET_STACK_LIMIT) {
        // One combined working block: floats first (stricter alignment),
        // bools after. Frame arena when a context is available (frame reset
        // reclaims it); heap fallback only for standalone callers without a
        // context.
        size_t bytes = count * (sizeof(float) + sizeof(bool));
        void *block;
        if (ctx != NULL) {
            block = wlx_scratch_alloc_bytes(ctx, bytes, _Alignof(float));
        } else {
            block = wlx_alloc(bytes);
            WLX_HARD_ASSERT(block != NULL, "Unable to allocate more RAM");
            heap_scratch = true;
        }
        raw = (float *)block;
        frozen = (bool *)((uint8_t *)block + count * sizeof(float));
    }
#endif

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
            default:
                slot_size = wlx_slot_fixed_size(&sizes[i], adjusted_total, viewport);
                break;
        }
        slot_size = wlx_slot_clamp(&sizes[i], slot_size);
#ifndef WLX_SLOT_SINGLE_PASS_CLAMP
        if (has_constraints) {
            raw[i] = slot_size;
            frozen[i] = false;
        }
#endif
        offset += slot_size;
        if (i < count - 1) offset += gap;
        // Snap each boundary to integer pixels to prevent sub-pixel gaps.
        offsets[i] = floorf(offsets[i] + 0.5f);
    }
    offsets[count] = floorf(offset + 0.5f);

#ifndef WLX_SLOT_SINGLE_PASS_CLAMP
    // --- Pass 3: iterative freeze-and-redistribute (default) ---
    // Ensures offsets[count] == adjusted_total + total_gap by redistributing
    // surplus/deficit from clamped slots to unfrozen neighbors. Define
    // WLX_SLOT_SINGLE_PASS_CLAMP to skip this and keep the single-pass clamp.
    if (has_constraints) {
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
                float clamped = wlx_slot_clamp(&sizes[i], new_size);
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

        if (heap_scratch) wlx_free(raw);
    }
#endif // !WLX_SLOT_SINGLE_PASS_CLAMP
}

// Standalone entry: identical math without a context. Redistribution working
// buffers fall back to the heap above WLX_OFFSET_STACK_LIMIT slots; in-core
// callers use wlx_compute_offsets_ctx so frame work rides the frame arena.
static inline void wlx_compute_offsets(float *offsets, size_t count,
                                       float total, float viewport,
                                       const WLX_Slot_Size *sizes, float gap)
{
    wlx_compute_offsets_ctx(NULL, offsets, count, total, viewport, sizes, gap);
}

static inline void wlx_compute_slot_offsets(WLX_Context *ctx, WLX_Layout *l, const WLX_Slot_Size *sizes) {
    assert(l != NULL);
    assert(l->count > 0);
    float total = wlx_layout_main_extent(l);
    wlx_compute_offsets_ctx(ctx, wlx_layout_offsets(ctx, l), l->count, total, l->viewport, sizes, l->gap);
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
    wlx_sub_arena_extend_to(&ctx->arena.dyn_offsets, base + 1);
    wlx_pool_dyn_offsets(ctx)[base] = 0.0f;

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

    wlx_compute_offsets_ctx(ctx, row_off, rows, r.h, r.h, row_sizes, gap);
    wlx_compute_offsets_ctx(ctx, col_off, cols, r.w, r.w, col_sizes, gap);

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
    wlx_compute_offsets_ctx(ctx, col_off, cols, r.w, r.w, col_sizes, gap);

    // Row offsets: dynamic, starts with sentinel only (on dyn_offsets arena)
    size_t row_base = ctx->arena.dyn_offsets.count;
    wlx_sub_arena_extend_to(&ctx->arena.dyn_offsets, row_base + 1);
    wlx_pool_dyn_offsets(ctx)[row_base] = 0.0f;

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

            wlx_sub_arena_extend_to(&ctx->arena.dyn_offsets,
                l->grid.row_offsets_base + new_total + 1);

            float *row_off = wlx_grid_row_offsets(ctx, l);
            for (size_t r = l->grid.rows; r < new_total; r++) {
                float effective = (l->grid.next_row_size > 0.0f)
                    ? l->grid.next_row_size : l->grid.row_size;
                float gap_add = (r > 0) ? l->gap : 0.0f;
                row_off[r + 1] = row_off[r] + effective + gap_add;
                l->grid.next_row_size = 0.0f;
            }
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
               "Dynamic layout offset region is not contiguous - nested layout_begin inside a dynamic body?");

        float effective = (l->linear.next_slot_size > 0.0f) ? l->linear.next_slot_size : l->linear.slot_size;
        assert(effective > 0.0f &&
               "Dynamic layout: slot size is 0 - call wlx_layout_auto_slot_px() before each child "
               "when using variable-size mode (slot_px = 0)");
        l->linear.next_slot_size = 0.0f;

        wlx_sub_arena_extend_to(&ctx->arena.dyn_offsets,
            l->linear.slot_size_offsets_base + l->count + span + 1);
        float *offsets = wlx_layout_offsets(ctx, l);
        for (size_t s = 0; s < span; s++) {
            float gap_add = (l->count + s > 0) ? l->gap : 0.0f;
            offsets[l->count + 1 + s] = floorf(offsets[l->count + s] + effective + gap_add + 0.5f);
        }
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

WLXDEF bool wlx_is_key_actuated(WLX_Context *ctx, WLX_Key_Code key) {
    return key >= 0 && key < WLX_KEY_COUNT &&
           (ctx->input.keys_pressed[key] || ctx->input.keys_repeated[key]);
}

WLXDEF bool wlx_mod_down(WLX_Context *ctx, uint32_t mask) {
    return (ctx->input.modifiers & mask) == mask;
}

WLXDEF bool wlx_mod_command_down(WLX_Context *ctx) {
#if defined(__APPLE__)
    return wlx_mod_down(ctx, WLX_MOD_SUPER);
#else
    return wlx_mod_down(ctx, WLX_MOD_CTRL);
#endif
}

WLXDEF bool wlx_is_mouse_right_down(WLX_Context *ctx) {
    return ctx->input.mouse_right_down;
}

WLXDEF bool wlx_is_mouse_right_clicked(WLX_Context *ctx) {
    return ctx->input.mouse_right_clicked;
}

WLXDEF bool wlx_is_mouse_middle_down(WLX_Context *ctx) {
    return ctx->input.mouse_middle_down;
}

WLXDEF bool wlx_is_mouse_middle_clicked(WLX_Context *ctx) {
    return ctx->input.mouse_middle_clicked;
}

WLXDEF void wlx_clipboard_set_text(WLX_Context *ctx, const char *text, size_t len) {
    if (ctx->backend.clipboard_set == NULL || text == NULL) return;
    ctx->backend.clipboard_set(text, len, ctx->backend.user);
}

WLXDEF size_t wlx_clipboard_get_copy(WLX_Context *ctx, char *out, size_t out_size) {
    if (out == NULL || out_size == 0) return 0;
    out[0] = '\0';
    if (ctx->backend.clipboard_get == NULL) return 0;
    const char *src = ctx->backend.clipboard_get(ctx->backend.user);
    if (src == NULL) return 0;

    size_t src_len = strlen(src);
    size_t copy = src_len < out_size - 1 ? src_len : out_size - 1;
    // Never split a UTF-8 codepoint at the truncation boundary: back up over
    // any trailing continuation bytes (0b10xxxxxx) of an incomplete sequence.
    copy = wlx_utf8_floor(src, copy);
    memcpy(out, src, copy);
    out[copy] = '\0';
    return copy;
}

WLXDEF bool wlx_rect_contains(WLX_Rect r, float px, float py) {
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

WLXDEF bool wlx_point_in_rect(int px, int py, int x, int y, int w, int h) {
    return wlx_rect_contains(
        (WLX_Rect){ (float)x, (float)y, (float)w, (float)h },
        (float)px, (float)py);
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

// The Tab ring's next stop. Focusable candidates on the highest layer that
// has any form the ring, in declaration (array) order; start_id is the
// current keyboard focus (or the mouse-focused field), 0 for none. Forward
// returns the first ring member after start (wrapping to the first member),
// backward the latest member before it (wrapping to the last); a start that
// is not on the ring lands on the first / last member; a ring of one returns
// that member. 0 when no candidate is focusable.
static size_t wlx_focus_next_stop(const WLX_Candidate_List *prev,
                                  size_t start_id, bool backward)
{
    int top = -1;
    for (size_t i = 0; i < prev->count; i++) {
        if (prev->items[i].focusable && prev->items[i].layer > top) top = prev->items[i].layer;
    }
    if (top < 0) return 0;

    bool   have_start = false, have_first = false;
    bool   have_after = false, have_before = false;
    size_t first = 0, last = 0, after = 0, before = 0;
    for (size_t i = 0; i < prev->count; i++) {
        const WLX_Interaction_Candidate *c = &prev->items[i];
        if (!c->focusable || c->layer != top) continue;
        if (!have_first) { first = i; have_first = true; }
        last = i;
        if (start_id != 0 && c->id == start_id) { have_start = true; continue; }
        if (!have_start) { before = i; have_before = true; }    // latest member before start
        else if (!have_after) { after = i; have_after = true; } // first member after start
    }
    size_t target;
    if (!have_start)   target = backward ? last : first;
    else if (backward) target = have_before ? before : last;
    else               target = have_after ? after : first;
    return prev->items[target].id;
}

// Release the typing-focus holder (inputbox / editor) at frame begin: it
// reports just_unfocused when queried this frame (focus_released_id), and
// its successor may take active_id the same frame. Clears the holder's Tab
// claim too - the next holder recomputes it when queried.
static inline void wlx_interaction_release_focus_holder(WLX_Context *ctx) {
    ctx->interaction.focus_released_id = ctx->interaction.active_id;
    ctx->interaction.active_id = 0;
    ctx->interaction.active_is_focus = false;
    ctx->interaction.active_consumes_tab = false;
}

// Blur the queried focus widget now: drop active_id and report the edge.
static inline void wlx_interaction_blur(WLX_Context *ctx, WLX_Interaction *result) {
    ctx->interaction.active_id = 0;
    result->focused = false;
    result->just_unfocused = true;
}

// Frame-begin ownership arbitration from the previous frame's candidate
// list: the topmost candidate under the pointer (highest layer, then latest
// query) owns hover and the cursor shape, a fresh left or right press
// latches its owner at the press point until release, a press or a
// bare Escape drops the keyboard focus ring, and Tab walks the ring. A
// frame with no previous candidates (first frame of a context) has nothing
// to arbitrate with and falls back to query-time capture.
static void wlx_frame_arbitrate(WLX_Context *ctx)
{
    ctx->cand_frame ^= 1;
    ctx->cands[ctx->cand_frame & 1].count = 0;
    WLX_Candidate_List *prev = &ctx->cands[(ctx->cand_frame ^ 1) & 1];
    ctx->interaction.arbitrate = prev->count > 0;

    size_t owner = 0;
    int best_layer = -1;
    uint8_t owner_cursor = WLX_CURSOR_ARROW;
    uint8_t active_cursor = WLX_CURSOR_ARROW;
    for (size_t i = 0; i < prev->count; i++) {
        if (ctx->interaction.active_id != 0
                && prev->items[i].id == ctx->interaction.active_id) {
            active_cursor = prev->items[i].cursor;
        }
        if (!wlx_rect_contains(prev->items[i].rect,
                (float)ctx->input.mouse_x, (float)ctx->input.mouse_y)) continue;
        if (prev->items[i].layer >= best_layer) {
            best_layer = prev->items[i].layer;
            owner = prev->items[i].id;
            owner_cursor = prev->items[i].cursor;
        }
    }

    ctx->interaction.pointer_layer = (best_layer >= 0) ? best_layer : 0;

    // Cursor shape follows the pointer's topmost candidate; while a
    // press is latched the active widget's shape wins, so a text
    // selection drag keeps the I-beam after leaving the rect. Pushed to
    // the backend only on change.
    {
        uint8_t shape = owner_cursor;
        if (ctx->interaction.active_id != 0 && ctx->input.mouse_down) {
            shape = active_cursor;
        }
        if (shape != ctx->cursor_applied) {
            ctx->cursor_applied = shape;
            if (ctx->backend.set_cursor != NULL) {
                ctx->backend.set_cursor((WLX_Cursor_Shape)shape, ctx->backend.user);
            }
        }
    }

    // While a widget is active (pressed or focused), only it may be
    // hot - the pre-arbitration rule, preserved.
    size_t hover_owner = owner;
    if (ctx->interaction.active_id != 0
            && hover_owner != ctx->interaction.active_id) {
        hover_owner = 0;
    }
    if (ctx->interaction.arbitrate) {
        ctx->interaction.hot_id = hover_owner;
    }

    ctx->interaction.focus_released_id = 0;
    ctx->interaction.press_claimed = false;
    if (ctx->input.mouse_clicked) {
        ctx->interaction.press_owner = owner;
        // A fresh press owned by anyone but the focused widget releases
        // focus before any widget is queried, so the same press can
        // activate its real target regardless of declaration order.
        // Drag/click holders are never released here.
        if (ctx->interaction.arbitrate
                && ctx->interaction.active_id != 0
                && ctx->interaction.active_is_focus
                && owner != ctx->interaction.active_id) {
            wlx_interaction_release_focus_holder(ctx);
        }
    } else if (!ctx->input.mouse_down) {
        ctx->interaction.press_owner = 0;
    }

    // Right-press ownership mirrors the left latch through the same
    // topmost candidate walk. A right press never releases focus,
    // never touches the left press owner, and never moves hot_id.
    if (ctx->input.mouse_right_clicked) {
        ctx->interaction.right_press_owner = owner;
    } else if (!ctx->input.mouse_right_down) {
        ctx->interaction.right_press_owner = 0;
    }

    // Keyboard focus traversal. The ring is keyboard-modal: a pointer
    // press drops it, and Escape drops it when no widget is active (a
    // focused field's own Escape blur runs in its handler). Tab walks
    // the previous frame's focusable candidates on the highest layer
    // that has any, in declaration order (Shift reverses, both ends
    // wrap), unless the active widget holds Tab for itself (editor).
    if (ctx->input.mouse_clicked) {
        ctx->interaction.focus_id = 0;
    }
    if (wlx_is_key_pressed(ctx, WLX_KEY_ESCAPE) && ctx->interaction.active_id == 0) {
        ctx->interaction.focus_id = 0;
    }
    if (ctx->interaction.arbitrate
            && wlx_is_key_actuated(ctx, WLX_KEY_TAB)
            && !(ctx->interaction.active_id != 0 && ctx->interaction.active_consumes_tab)) {
        // Start from the keyboard-focused widget, else from a mouse-focused
        // field, so Tab continues from where the user is.
        size_t start_id = ctx->interaction.focus_id != 0
            ? ctx->interaction.focus_id : ctx->interaction.active_id;
        size_t next_id = wlx_focus_next_stop(prev, start_id, wlx_mod_down(ctx, WLX_MOD_SHIFT));
        if (next_id != 0) {
            ctx->interaction.focus_id = next_id;
            ctx->interaction.focus_gained_id = next_id;
            ctx->interaction.tab_consumed = true;
            // A typing-focus holder that lost the ring releases now, so it
            // reports just_unfocused and the target can take active_id the
            // same frame.
            if (ctx->interaction.active_id != 0
                    && ctx->interaction.active_is_focus
                    && ctx->interaction.active_id != next_id) {
                wlx_interaction_release_focus_holder(ctx);
            }
        }
    }
}

WLXDEF void wlx_begin(WLX_Context *ctx, WLX_Rect r, WLX_Input_Handler input_handler) {
    wlx_assert_backend_ready(ctx);
    assert(input_handler != NULL && "WLX input handler must not be NULL");
    WLX_PERF_HOOK(frame_begin, ctx);
    WLX_PERF_HOOK(input_begin, ctx);
    input_handler(ctx);
    WLX_PERF_HOOK(input_end, ctx);
    ctx->interaction.hot_id = 0;
    ctx->interaction.active_id_seen = false;
    ctx->interaction.enter_consumed = false;
    ctx->interaction.focus_id_seen = false;
    ctx->interaction.focus_gained_id = 0;
    ctx->interaction.tab_consumed = false;
    // The frame's single backend time sample; adapters may measure time
    // since their own previous call because the core calls exactly once.
    ctx->frame_dt = ctx->backend.get_frame_time != NULL
        ? ctx->backend.get_frame_time(ctx->backend.user) : 0.0f;
    wlx_frame_arbitrate(ctx);
    // Lazy pool init: callers that zero-init WLX_Context and skip
    // wlx_context_init still get the default macro-backed allocators.
    if (ctx->arena.layouts.item_size == 0) {
        wlx_arena_pool_init(&ctx->arena, NULL);
    }
    wlx_arena_pool_reset(&ctx->arena);
    ctx->current_range_idx = -1;
    ctx->current_layer = 0;
    ctx->scissor_stack_count = 0;
    ctx->clip_base = (WLX_Clip_Base){0};
    ctx->menu_stack_count = 0;
    ctx->last_widget_rect = (WLX_Rect){0};
    ctx->immediate_mode = false;
    ctx->rect = r;
    if (ctx->theme == NULL) ctx->theme = &wlx_theme_dark;
    WLX_DBG(frame_begin, ctx);
    WLX_PERF_HOOK(begin_end, ctx);
}

WLXDEF void wlx_begin_immediate(WLX_Context *ctx, WLX_Rect r, WLX_Input_Handler input_handler) {
    wlx_begin(ctx, r, input_handler);
    ctx->immediate_mode = true;
}

// ----------------------------------------------------------------------------
// Soft-effect software fallback geometry / alpha. Pure functions (no backend,
// no context). The replay dispatcher runs these when a backend
// leaves draw_shadow / draw_glow NULL.
// ----------------------------------------------------------------------------

// Shadow layer `i` (1..layers, 1 = innermost/closest, layers = outermost):
// the element rect shifted by a fraction t = i/layers of the offset.
static inline WLX_Rect wlx_shadow_layer_rect(WLX_Rect rect, float ox, float oy, int layers, int i) {
    if (layers < 1) layers = 1;
    if (i < 1) i = 1;
    if (i > layers) i = layers;
    float t = (float)i / (float)layers;
    return (WLX_Rect){ rect.x + ox * t, rect.y + oy * t, rect.w, rect.h };
}

// Shadow layer `i` premultiplied alpha = base_a * i / (layers*layers). Alpha
// grows with `i`: the layer closest to the element (i=1, smallest offset) is
// faintest, the fully-offset outermost layer (i=layers) is the most opaque.
static inline uint8_t wlx_shadow_layer_alpha(uint8_t base_a, int layers, int i) {
    if (layers < 1) layers = 1;
    if (i < 1) i = 1;
    if (i > layers) i = layers;
    float a = (float)base_a * (float)i / ((float)layers * (float)layers);
    return wlx_clamp_u8((int)(a + 0.5f));
}

// Glow ring `i` (0..rings-1, 0 = innermost/smallest, rings-1 = outermost):
// the element rect grown outward by grow = spread * (i+1)/rings on every side.
static inline WLX_Rect wlx_glow_ring_rect(WLX_Rect rect, float spread, int rings, int i) {
    if (rings < 1) rings = 1;
    if (i < 0) i = 0;
    if (i > rings - 1) i = rings - 1;
    float grow = spread * (float)(i + 1) / (float)rings;
    return (WLX_Rect){ rect.x - grow, rect.y - grow, rect.w + 2.0f * grow, rect.h + 2.0f * grow };
}

// Glow ring `i` premultiplied alpha = base_a * (rings - i) / rings; ring 0
// (innermost) is strongest, outermost is faintest.
static inline uint8_t wlx_glow_ring_alpha(uint8_t base_a, int rings, int i) {
    if (rings < 1) rings = 1;
    if (i < 0) i = 0;
    if (i > rings - 1) i = rings - 1;
    float a = (float)base_a * (float)(rings - i) / (float)rings;
    return wlx_clamp_u8((int)(a + 0.5f));
}

// Render one shadow: the native callback when present, else the layered
// software fallback. Calls the backend directly (immediate), so the sub-rects
// are not re-enqueued or counted. Shared by the replay dispatcher (deferred)
// and the immediate-mode emission path in wlx_draw_box.
static inline void wlx_render_shadow(WLX_Context *ctx, WLX_Rect rect, WLX_Color color,
        float ox, float oy, float blur, int layers, float roundness, int segs) {
    if (ctx->backend.draw_shadow) {
        ctx->backend.draw_shadow(rect, color, ox, oy, blur, layers, roundness, segs, ctx->backend.user);
        return;
    }
    if (layers < 1) layers = 1;
    for (int i = layers; i >= 1; i--) {
        WLX_Color lc = color;
        lc.a = wlx_shadow_layer_alpha(color.a, layers, i);
        ctx->backend.draw_rect_rounded(wlx_shadow_layer_rect(rect, ox, oy, layers, i),
                                       roundness, segs, lc, ctx->backend.user);
    }
}

// Render one glow: the native callback when present, else the concentric-ring
// software fallback. See wlx_render_shadow for the dispatch contract.
static inline void wlx_render_glow(WLX_Context *ctx, WLX_Rect rect, WLX_Color color,
        float spread, int rings, float roundness, int segs) {
    if (ctx->backend.draw_glow) {
        ctx->backend.draw_glow(rect, color, spread, rings, roundness, segs, ctx->backend.user);
        return;
    }
    if (rings < 1) rings = 1;
    for (int i = rings - 1; i >= 0; i--) {
        WLX_Color rc = color;
        rc.a = wlx_glow_ring_alpha(color.a, rings, i);
        ctx->backend.draw_rect_rounded_lines(wlx_glow_ring_rect(rect, spread, rings, i),
                                             roundness, segs, 2.0f, rc, ctx->backend.user);
    }
}

// Render one vertical two-stop gradient: the native callback when present, else
// the software stacked-band fallback (solid bands interpolating top -> bottom).
// See wlx_render_shadow for the dispatch contract. top/bottom already carry
// effective opacity; band colours derive from the dimmed stops (no re-dim).
static inline void wlx_render_gradient_v(WLX_Context *ctx, WLX_Rect rect,
        WLX_Color top, WLX_Color bottom, float roundness, int segs) {
    if (ctx->backend.draw_gradient_v) {
        ctx->backend.draw_gradient_v(rect, top, bottom, roundness, segs, ctx->backend.user);
        return;
    }
    int bands = (int)(rect.h / WLX_GRADIENT_FALLBACK_BAND_HEIGHT_PX);
    if (bands < 1) bands = 1;
    float band_h = rect.h / (float)bands;
    for (int i = 0; i < bands; i++) {
        float t = (bands == 1) ? 0.0f : (float)i / (float)(bands - 1);
        WLX_Color c = wlx_color_lerp(top, bottom, t);
        // +1.0f guards against sub-pixel seams between adjacent bands.
        WLX_Rect b = { rect.x, rect.y + (float)i * band_h, rect.w, band_h + 1.0f };
        if (roundness > 0.0f) ctx->backend.draw_rect_rounded(b, roundness, segs, c, ctx->backend.user);
        else                  ctx->backend.draw_rect(b, c, ctx->backend.user);
    }
}

// Replay one recorded command through the backend, translated by the
// accumulated (dx, dy) offsets of its innermost range.
static inline void wlx_replay_dispatch_cmd(WLX_Context *ctx, WLX_Cmd *c,
                                           float dx, float dy) {
    switch (c->type) {
        case WLX_CMD_RECT:
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            ctx->backend.draw_rect(
                (WLX_Rect){c->data.rect.rect.x + dx, c->data.rect.rect.y + dy,
                           c->data.rect.rect.w, c->data.rect.rect.h},
                c->data.rect.color, ctx->backend.user);
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;

        case WLX_CMD_RECT_LINES:
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            ctx->backend.draw_rect_lines(
                (WLX_Rect){c->data.rect_lines.rect.x + dx, c->data.rect_lines.rect.y + dy,
                           c->data.rect_lines.rect.w, c->data.rect_lines.rect.h},
                c->data.rect_lines.thick, c->data.rect_lines.color, ctx->backend.user);
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;

        case WLX_CMD_RECT_ROUNDED:
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            ctx->backend.draw_rect_rounded(
                (WLX_Rect){c->data.rect_rounded.rect.x + dx, c->data.rect_rounded.rect.y + dy,
                           c->data.rect_rounded.rect.w, c->data.rect_rounded.rect.h},
                c->data.rect_rounded.roundness, c->data.rect_rounded.segments,
                c->data.rect_rounded.color, ctx->backend.user);
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;

        case WLX_CMD_RECT_ROUNDED_LINES:
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            ctx->backend.draw_rect_rounded_lines(
                (WLX_Rect){c->data.rect_rounded_lines.rect.x + dx, c->data.rect_rounded_lines.rect.y + dy,
                           c->data.rect_rounded_lines.rect.w, c->data.rect_rounded_lines.rect.h},
                c->data.rect_rounded_lines.roundness, c->data.rect_rounded_lines.segments,
                c->data.rect_rounded_lines.thick, c->data.rect_rounded_lines.color, ctx->backend.user);
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;

        case WLX_CMD_CIRCLE:
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            ctx->backend.draw_circle(
                c->data.circle.cx + dx, c->data.circle.cy + dy,
                c->data.circle.radius, c->data.circle.segments,
                c->data.circle.color, ctx->backend.user);
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;

        case WLX_CMD_RING:
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            ctx->backend.draw_ring(
                c->data.ring.cx + dx, c->data.ring.cy + dy,
                c->data.ring.inner_r, c->data.ring.outer_r,
                c->data.ring.segments, c->data.ring.color, ctx->backend.user);
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;

        case WLX_CMD_LINE:
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            ctx->backend.draw_line(
                c->data.line.x1 + dx, c->data.line.y1 + dy,
                c->data.line.x2 + dx, c->data.line.y2 + dy,
                c->data.line.thick, c->data.line.color, ctx->backend.user);
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;

        case WLX_CMD_TEXT: {
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            // The recorded style is nominal; the transform applies here, at
            // the boundary, exactly as for the measure that placed it.
            WLX_Text_Style style = wlx_style_at_boundary(ctx, c->data.text.style);
            if (ctx->backend.draw_text_slice != NULL) {
                ctx->backend.draw_text_slice(
                    (const char *)&wlx_pool_scratch(ctx)[c->data.text.text_off],
                    c->data.text.text_len,
                    c->data.text.x + dx, c->data.text.y + dy,
                    style, ctx->backend.user);
            } else {
                // Legacy draw_text expects NUL-terminated input; the recorded
                // scratch span carries only `text_len` bytes so synthesise a
                // NUL-terminated copy on demand.
                WLX_CStr_Tmp tmp;
                const char *cstr = wlx_cstr_tmp_begin(&tmp,
                    (const char *)&wlx_pool_scratch(ctx)[c->data.text.text_off],
                    c->data.text.text_len);
                if (cstr == NULL) {
                    WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
                    break;
                }
                ctx->backend.draw_text(cstr,
                    c->data.text.x + dx, c->data.text.y + dy,
                    style, ctx->backend.user);
                wlx_cstr_tmp_end(&tmp);
            }
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;
        }

        case WLX_CMD_TEXTURE:
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            ctx->backend.draw_texture(
                c->data.texture.texture, c->data.texture.src,
                (WLX_Rect){c->data.texture.dst.x + dx, c->data.texture.dst.y + dy,
                           c->data.texture.dst.w, c->data.texture.dst.h},
                c->data.texture.tint, ctx->backend.user);
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;

        case WLX_CMD_SCISSOR_BEGIN:
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            ctx->backend.begin_scissor(
                (WLX_Rect){c->data.scissor_begin.rect.x + dx, c->data.scissor_begin.rect.y + dy,
                           c->data.scissor_begin.rect.w, c->data.scissor_begin.rect.h}, ctx->backend.user);
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;

        case WLX_CMD_SCISSOR_END:
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            ctx->backend.end_scissor(ctx->backend.user);
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;

        case WLX_CMD_SHADOW: {
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            WLX_Rect sr = { c->data.shadow.rect.x + dx, c->data.shadow.rect.y + dy,
                            c->data.shadow.rect.w, c->data.shadow.rect.h };
            wlx_render_shadow(ctx, sr, c->data.shadow.color,
                c->data.shadow.offset_x, c->data.shadow.offset_y,
                c->data.shadow.blur, c->data.shadow.layers,
                c->data.shadow.roundness, c->data.shadow.rounded_segs);
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;
        }

        case WLX_CMD_GLOW: {
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            WLX_Rect gr = { c->data.glow.rect.x + dx, c->data.glow.rect.y + dy,
                            c->data.glow.rect.w, c->data.glow.rect.h };
            wlx_render_glow(ctx, gr, c->data.glow.color,
                c->data.glow.spread, c->data.glow.rings,
                c->data.glow.roundness, c->data.glow.rounded_segs);
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;
        }

        case WLX_CMD_GRADIENT_V: {
            WLX_PERF_HOOK(backend_callback_begin, ctx, c->type);
            WLX_Rect gvr = { c->data.gradient_v.rect.x + dx, c->data.gradient_v.rect.y + dy,
                             c->data.gradient_v.rect.w, c->data.gradient_v.rect.h };
            wlx_render_gradient_v(ctx, gvr, c->data.gradient_v.top,
                c->data.gradient_v.bottom, c->data.gradient_v.roundness,
                c->data.gradient_v.rounded_segs);
            WLX_PERF_HOOK(backend_callback_end, ctx, c->type);
            break;
        }

        case WLX_CMD_TYPE_COUNT:
            break;
    }
}

// Keyboard focus ring: one accent outline around the Tab-focused widget's
// recorded rect, pushed straight to the backend on top of everything else.
// Drawn only while a widget holds the ring (a pointer press drops it), so
// mouse-driven sessions never see it.
static void wlx_focus_ring_draw(WLX_Context *ctx) {
    if (ctx->interaction.focus_id == 0 || ctx->backend.draw_rect_lines == NULL) return;
    WLX_Rect r = ctx->interaction.focus_rect;
    if (r.w <= 0.0f || r.h <= 0.0f) return;
    float thick = WLX_FOCUS_RING_THICKNESS;
    float grow = WLX_FOCUS_RING_GAP + thick;
    WLX_Rect ring = { r.x - grow, r.y - grow, r.w + 2.0f * grow, r.h + 2.0f * grow };
    // Stay inside the container clip recorded with the rect: a widget cut by
    // a scroll panel viewport or a .clip layout gets its ring around the
    // visible part, never stroked over the neighbouring content.
    if (ctx->interaction.focus_clip_active) {
        ring = wlx_rect_intersect(ring, ctx->interaction.focus_clip);
        if (ring.w <= 0.0f || ring.h <= 0.0f) return;
    }
    const WLX_Theme *theme = ctx->theme ? ctx->theme : &wlx_theme_dark;
    WLX_Color color = theme->accent;
    wlx_outline_subpixel(&thick, &color);
    ctx->backend.draw_rect_lines(ring, thick, color, ctx->backend.user);
}

WLXDEF void wlx_end(WLX_Context *ctx) {
    WLX_PERF_HOOK(end_begin, ctx);
    if (ctx->interaction.active_id != 0 && !ctx->interaction.active_id_seen) {
        ctx->interaction.active_id = 0;
    }
    if (ctx->interaction.focus_id != 0 && !ctx->interaction.focus_id_seen) {
        ctx->interaction.focus_id = 0;
    }

#ifdef WLX_DEBUG
    // Scope push/pop balance: every wlx_scope_push (via frame helpers) and
    // every direct wlx_push_id must be matched before frame end. The arena
    // pool reset at wlx_begin clears id_stack to 0; if we exit non-zero,
    // some scope or push_id is unpaired.
    assert(ctx->arena.id_stack.count == 0
        && "wlx_end: unbalanced wlx_push_id / wlx_scope_push (missing pop)");
#endif

    if (ctx->immediate_mode || ctx->arena.commands.count == 0) {
        wlx_focus_ring_draw(ctx);
        WLX_PERF_HOOK(frame_publish, ctx);
        return;
    }

    // Accumulate nested offsets: parents appear before children by construction.
    // The same walk finds the highest layer in use (0 = no overlay this frame).
    WLX_PERF_HOOK(range_begin, ctx);
    int max_layer = 0;
    for (size_t i = 0; i < ctx->arena.cmd_ranges.count; i++) {
        int p = wlx_pool_cmd_ranges(ctx)[i].parent_range_idx;
        if (p != WLX_NO_RANGE) {
            wlx_pool_cmd_ranges(ctx)[i].dy_offset += wlx_pool_cmd_ranges(ctx)[(size_t)p].dy_offset;
            wlx_pool_cmd_ranges(ctx)[i].dx_offset += wlx_pool_cmd_ranges(ctx)[(size_t)p].dx_offset;
        }
        if (wlx_pool_cmd_ranges(ctx)[i].layer > max_layer) {
            max_layer = wlx_pool_cmd_ranges(ctx)[i].layer;
        }
    }
    WLX_PERF_HOOK(range_end, ctx);

    // Build per-command offset lookup: children overwrite parent entries,
    // so each command gets the deepest (most specific) accumulated offset.
    WLX_PERF_HOOK(offset_begin, ctx);
    // One scratch block for the per-command dx, dy and layer tables. The
    // scratch sub-arena is realloc-grown, so a second allocation here would
    // strand the first pointer; nothing below may allocate scratch until the
    // dispatch loops finish (wlx_replay_dispatch_cmd does not).
    size_t cmd_count = ctx->arena.commands.count;
    uint8_t *cmd_tables = (uint8_t *)wlx_scratch_alloc_bytes(ctx,
        cmd_count * (2 * sizeof(float) + 1), _Alignof(float));
    float   *cmd_dx    = (float *)cmd_tables;
    float   *cmd_dy    = cmd_dx + cmd_count;
    // The per-command layer tag is a byte; the overridable layer cap must fit.
    _Static_assert(WLX_OVERLAY_MAX_LAYERS <= 255, "cmd_layer stores the layer in a byte");
    uint8_t *cmd_layer = (uint8_t *)(cmd_dy + cmd_count);
    wlx_zero_array(cmd_count, cmd_dy);
    wlx_zero_array(cmd_count, cmd_dx);
    for (size_t i = 0; i < ctx->arena.cmd_ranges.count; i++) {
        WLX_Cmd_Range *r = &wlx_pool_cmd_ranges(ctx)[i];
        for (size_t ci = r->start_idx; ci < r->end_idx; ci++) {
            cmd_dy[ci] = r->dy_offset;
            cmd_dx[ci] = r->dx_offset;
        }
    }
    WLX_PERF_HOOK(offset_end, ctx);

    // Dispatch: translate x/y-coordinates and call the backend. All ranges
    // on layer 0 (no overlay) is the common case and keeps the single flat
    // walk. Layered frames replay ascending, one pass per layer, so higher
    // layers draw over everything below; each pass must leave no scissor
    // open (a dangling clip would crop the next layer), so a depth counter
    // ends any open clip at the pass boundary and skips ends that belong to
    // another layer's scope.
    WLX_PERF_HOOK(dispatch_begin, ctx);
    if (max_layer == 0) {
        for (size_t i = 0; i < ctx->arena.commands.count; i++) {
            wlx_replay_dispatch_cmd(ctx, &wlx_pool_commands(ctx)[i],
                                    cmd_dx[i], cmd_dy[i]);
        }
    } else {
        memset(cmd_layer, 0, cmd_count);
        for (size_t i = 0; i < ctx->arena.cmd_ranges.count; i++) {
            WLX_Cmd_Range *r = &wlx_pool_cmd_ranges(ctx)[i];
            for (size_t ci = r->start_idx; ci < r->end_idx; ci++) {
                cmd_layer[ci] = (uint8_t)r->layer;
            }
        }
        for (int layer = 0; layer <= max_layer; layer++) {
            int scissor_depth = 0;
            for (size_t i = 0; i < ctx->arena.commands.count; i++) {
                if (cmd_layer[i] != (uint8_t)layer) continue;
                WLX_Cmd *c = &wlx_pool_commands(ctx)[i];
                if (c->type == WLX_CMD_SCISSOR_BEGIN) {
                    scissor_depth++;
                } else if (c->type == WLX_CMD_SCISSOR_END) {
                    if (scissor_depth == 0) continue;
                    scissor_depth--;
                }
                wlx_replay_dispatch_cmd(ctx, c, cmd_dx[i], cmd_dy[i]);
            }
            while (scissor_depth-- > 0) {
                if (ctx->backend.end_scissor) ctx->backend.end_scissor(ctx->backend.user);
            }
        }
    }
    WLX_PERF_HOOK(dispatch_end, ctx);
    // The focus ring goes on after every layer: Tab traverses the top layer
    // and any pointer press drops the ring, so topmost is always right.
    wlx_focus_ring_draw(ctx);
    WLX_PERF_HOOK(frame_publish, ctx);
}

// Defined with the retained-geometry store tier; destruction is its
// terminal lifecycle exit.
static void wlx_text_geom_store_free(WLX_Text_Geom_Store *s);
static void wlx_text_undo_stack_free(WLX_Text_Undo_Stack *s);

WLXDEF void wlx_context_destroy(WLX_Context *ctx) {
    WLX_PERF_HOOK(destroy, ctx);
    // Free state map data entries
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        if (ctx->states.slots[i].id != 0) {
            wlx_free(ctx->states.slots[i].data);
        }
    }
    // Release pool-owned per-frame buffers and the persistent state map.
    wlx_arena_pool_destroy(&ctx->arena);
    wlx_free(ctx->cands[0].items);
    wlx_free(ctx->cands[1].items);
    wlx_free(ctx->states.slots);
    wlx_free(ctx->text_line_scratch);
    wlx_free(ctx->menu_stack);
    for (size_t i = 0; i < ctx->editor_indices.count; i++) {
        WLX_Editor_Line_Index *idx = &ctx->editor_indices.items[i];
        wlx_free(idx->offsets);
        wlx_text_geom_store_free(&idx->geom);
    }
    wlx_free(ctx->editor_indices.items);
    for (size_t i = 0; i < ctx->text_undo.count; i++) {
        wlx_text_undo_stack_free(&ctx->text_undo.items[i].undo);
        wlx_text_undo_stack_free(&ctx->text_undo.items[i].redo);
    }
    wlx_free(ctx->text_undo.items);
    WLX_DBG(destroy, ctx);
    // Zero out the context so it's safe to reuse or free
    wlx_zero_struct(*ctx);
}

// Resolved per-side border. Each edge carries a color and a width; `all_equal`
// is set when every edge shares one color and one width, letting callers route
// through the uniform rounded/sharp border path for backward parity.
typedef struct { WLX_Color color; float width; } WLX_Edge;
typedef struct { WLX_Edge top, right, bottom, left; bool all_equal; } WLX_Border_Sides;

// Resolve uniform + per-side border fields into per-edge (color, width). A
// per-side color of {0} inherits the uniform color; a negative per-side width
// inherits the uniform width (an explicit 0 switches that edge off). `all_equal`
// is set when every edge ends up with the same color and the same width.
static inline WLX_Border_Sides wlx_resolve_border_sides(
    WLX_Color uniform_color, float uniform_width,
    WLX_Color ct, WLX_Color cr, WLX_Color cb, WLX_Color cl,
    float wt, float wr, float wb, float wl)
{
    WLX_Border_Sides s;
    s.top.color    = wlx_color_is_zero(ct) ? uniform_color : ct;
    s.right.color  = wlx_color_is_zero(cr) ? uniform_color : cr;
    s.bottom.color = wlx_color_is_zero(cb) ? uniform_color : cb;
    s.left.color   = wlx_color_is_zero(cl) ? uniform_color : cl;
    s.top.width    = wlx_is_negative_unset(wt) ? uniform_width : wt;
    s.right.width  = wlx_is_negative_unset(wr) ? uniform_width : wr;
    s.bottom.width = wlx_is_negative_unset(wb) ? uniform_width : wb;
    s.left.width   = wlx_is_negative_unset(wl) ? uniform_width : wl;
    s.all_equal =
        wlx_color_eq(s.top.color, s.right.color) &&
        wlx_color_eq(s.top.color, s.bottom.color) &&
        wlx_color_eq(s.top.color, s.left.color) &&
        s.top.width == s.right.width &&
        s.top.width == s.bottom.width &&
        s.top.width == s.left.width;
    return s;
}

// Apply the disabled-brightness and opacity transforms that the uniform border
// color already received from the resolver (WLX_RESOLVE_VISUAL_STATE) to a
// single color, so an explicit per-side color stays in visual sync with it.
static inline WLX_Color wlx_color_apply_state(const WLX_Theme *theme,
    bool disabled, float opacity, WLX_Color c)
{
    if (disabled && !wlx_is_float_unset(theme->disabled_brightness))
        c = wlx_color_brightness(c, theme->disabled_brightness);
    return wlx_color_apply_opacity(c, opacity);
}

// Forward the eight per-side border fields of a widget opt into
// wlx_border_sides_for_widget, mirroring WLX_BOX_STYLE_EFFECTS. Adding a
// per-side field means extending this macro once, not every call site.
#define WLX_BORDER_SIDES_ARGS(opt) \
    (opt).border_color_top, (opt).border_color_right, \
    (opt).border_color_bottom, (opt).border_color_left, \
    (opt).border_width_top, (opt).border_width_right, \
    (opt).border_width_bottom, (opt).border_width_left

// Widget-path per-side resolution: an unset edge inherits the already-resolved
// uniform border color, while an explicit edge color receives the same
// disabled/opacity treatment and hover tint the uniform color does, so beveled
// or single-edge widgets keep hover/disabled feedback.
static inline WLX_Border_Sides wlx_border_sides_for_widget(
    const WLX_Theme *theme, bool hover, bool disabled, float opacity,
    WLX_Color uniform_color, float uniform_width,
    WLX_Color ct, WLX_Color cr, WLX_Color cb, WLX_Color cl,
    float wt, float wr, float wb, float wl)
{
    float hb = theme->hover_brightness;
    if (!wlx_color_is_zero(ct)) ct = wlx_color_hover_tint(wlx_color_apply_state(theme, disabled, opacity, ct), hover, disabled, hb);
    if (!wlx_color_is_zero(cr)) cr = wlx_color_hover_tint(wlx_color_apply_state(theme, disabled, opacity, cr), hover, disabled, hb);
    if (!wlx_color_is_zero(cb)) cb = wlx_color_hover_tint(wlx_color_apply_state(theme, disabled, opacity, cb), hover, disabled, hb);
    if (!wlx_color_is_zero(cl)) cl = wlx_color_hover_tint(wlx_color_apply_state(theme, disabled, opacity, cl), hover, disabled, hb);
    return wlx_resolve_border_sides(uniform_color, uniform_width, ct, cr, cb, cl, wt, wr, wb, wl);
}

// Draw four independent sharp edge spans, one per side. Edges overlap at the
// corners; the draw order top, left, bottom, right keeps light edges before
// dark for a two-tone bevel look.
static inline void wlx_draw_box_edges(WLX_Context *ctx, WLX_Rect r, WLX_Border_Sides s)
{
    if (s.top.width    > 0 && !wlx_color_is_zero(s.top.color))
        wlx_draw_rect(ctx, (WLX_Rect){ r.x, r.y, r.w, s.top.width }, s.top.color);
    if (s.left.width   > 0 && !wlx_color_is_zero(s.left.color))
        wlx_draw_rect(ctx, (WLX_Rect){ r.x, r.y, s.left.width, r.h }, s.left.color);
    if (s.bottom.width > 0 && !wlx_color_is_zero(s.bottom.color))
        wlx_draw_rect(ctx, (WLX_Rect){ r.x, r.y + r.h - s.bottom.width, r.w, s.bottom.width }, s.bottom.color);
    if (s.right.width  > 0 && !wlx_color_is_zero(s.right.color))
        wlx_draw_rect(ctx, (WLX_Rect){ r.x + r.w - s.right.width, r.y, s.right.width, r.h }, s.right.color);
}

// Internal draw helper for a filled rectangle with an optional border.
// fill    - fill color; zero means no fill drawn.
// border  - border color; zero or border_width <= 0 means no border drawn.
// rounded_segments is resolved against theme->rounded_segments when <= 0.
// When per_side is true the border is taken from `sides`: equal sides collapse
// back to the uniform rounded/sharp path, differing sides draw four sharp edge
// spans (always sharp, even over a rounded fill).
typedef struct {
    WLX_Color fill;
    WLX_Color border;
    float border_width;
    float roundness;
    float corner_radius;   /* > 0 -> absolute px radius, overrides roundness; 0 -> unset */
    int   rounded_segments;
    int   rounded_corners; /* WLX_CORNERS_* mask; 0 -> all corners rounded */
    WLX_Border_Sides sides;
    bool  per_side;
    WLX_SHADOW_FIELDS;
    WLX_GLOW_FIELDS;
    WLX_GRADIENT_FIELDS;
} WLX_Box_Style;

// Standalone shadow/glow bundle used to carry effect fields through the
// container decor path (where no widget opt is available).
typedef struct {
    WLX_SHADOW_FIELDS;
    WLX_GLOW_FIELDS;
} WLX_Effect_Style;

// Forward the shadow/glow fields from a source (a widget opt or a
// WLX_Effect_Style) into a WLX_Box_Style designated initializer.
#define WLX_BOX_STYLE_EFFECTS(src) \
    .shadow_color = (src).shadow_color, .shadow_offset_x = (src).shadow_offset_x, \
    .shadow_offset_y = (src).shadow_offset_y, .shadow_blur = (src).shadow_blur, \
    .shadow_layers = (src).shadow_layers, .glow_color = (src).glow_color, \
    .glow_spread = (src).glow_spread, .glow_rings = (src).glow_rings

// Forward the gradient stop fields from a source (a widget opt or container
// common opt) into a WLX_Box_Style designated initializer.
#define WLX_BOX_STYLE_GRADIENT(src) \
    .gradient_top = (src).gradient_top, .gradient_bottom = (src).gradient_bottom

// Forward the absolute pixel corner radius and per-corner mask from a source (a
// widget opt or container common opt) into a WLX_Box_Style designated initializer.
#define WLX_BOX_STYLE_CORNER(src) \
    .corner_radius = (src).corner_radius, \
    .rounded_corners = (src).rounded_corners

// Emit one vertical gradient fill: immediate render now, or a deferred command.
static inline void wlx_draw_box_gradient(WLX_Context *ctx, WLX_Rect rect,
        WLX_Color top, WLX_Color bottom, float roundness, int segs) {
    if (ctx->immediate_mode)
        wlx_render_gradient_v(ctx, rect, top, bottom, roundness, segs);
    else
        wlx_cmd_record_gradient_v(ctx, rect, top, bottom, roundness, segs);
}

// Convert an absolute pixel corner radius to the normalized roundness fraction
// the backends consume (they map roundness back to roundness * min(w,h)/2). The
// result is clamped to [0, 1] so it never overshoots min/2. Returns a negative
// value to signal "no override" when px <= 0 or the rect is degenerate (min <= 0).
static inline float wlx_corner_radius_to_roundness(WLX_Rect rect, float px) {
    if (px <= 0.0f) return -1.0f;
    float m = (rect.w < rect.h) ? rect.w : rect.h;
    if (m <= 0.0f) return -1.0f;
    float f = 2.0f * px / m;
    return (f > 1.0f) ? 1.0f : f;
}

// Solid rounded fill with an optional per-corner mask. Callers pass roundness > 0;
// `corners` is a WLX_CORNERS_* mask (0 means all four corners). When the mask omits
// a corner, that corner is squared off by overdrawing its r x r corner box with the
// fill color, so no backend rounded-corner-mask op is needed. When the mask is all
// corners this emits exactly one rounded fill, identical to the prior path.
static inline void wlx_draw_rect_rounded_corners(WLX_Context *ctx, WLX_Rect rect,
        float roundness, int segments, WLX_Color color, int corners) {
    wlx_draw_rect_rounded(ctx, rect, roundness, segments, color);
    if (corners == 0 || corners == WLX_CORNERS_ALL) return;
    float m = (rect.w < rect.h) ? rect.w : rect.h;
    float r = ceilf(roundness * m * 0.5f);
    if (r <= 0.0f) return;
    if (r > rect.w) r = rect.w;
    if (r > rect.h) r = rect.h;
    if (!(corners & WLX_CORNER_TOP_LEFT))
        wlx_draw_rect(ctx, (WLX_Rect){ rect.x, rect.y, r, r }, color);
    if (!(corners & WLX_CORNER_TOP_RIGHT))
        wlx_draw_rect(ctx, (WLX_Rect){ rect.x + rect.w - r, rect.y, r, r }, color);
    if (!(corners & WLX_CORNER_BOTTOM_RIGHT))
        wlx_draw_rect(ctx, (WLX_Rect){ rect.x + rect.w - r, rect.y + rect.h - r, r, r }, color);
    if (!(corners & WLX_CORNER_BOTTOM_LEFT))
        wlx_draw_rect(ctx, (WLX_Rect){ rect.x, rect.y + rect.h - r, r, r }, color);
}

static inline void wlx_draw_box(WLX_Context *ctx, WLX_Rect rect, WLX_Box_Style style)
{
    bool has_fill = !wlx_color_is_zero(style.fill);
    // A gradient fill replaces the solid fill (never the border). gradient_top
    // is the enable gate; a zero gradient_bottom is treated as gradient_top so a
    // single-stop caller gets a uniform fill through the gradient path.
    bool has_gradient = !wlx_color_is_zero(style.gradient_top);
    WLX_Color grad_bottom = wlx_color_is_zero(style.gradient_bottom)
                          ? style.gradient_top : style.gradient_bottom;
    int segs = style.rounded_segments > 0 ? style.rounded_segments : ctx->theme->rounded_segments;

    // An absolute pixel corner radius overrides the fractional roundness once,
    // here, where the rect is finally known. The resolved fraction then feeds
    // every downstream consumer of style.roundness unchanged (shadow, glow,
    // gradient, fill, and the per-side/equal-sides border branches), and the
    // recorded command still carries a plain fraction - no backend change.
    if (style.corner_radius > 0.0f) {
        float f = wlx_corner_radius_to_roundness(rect, style.corner_radius);
        if (f >= 0.0f) style.roundness = f;
    }

    // Soft effects ride behind the element: emit them before any fill/border
    // (and before the early returns below) so an effect-only box still emits,
    // and so the effect commands precede the fill in the buffer. Numeric knobs
    // resolve to the theme value, then to a hard fallback. Colors already carry
    // effective opacity from the resolver / caller. Immediate mode renders now
    // (no replay pass); deferred mode records a command for replay.
    if (!wlx_color_is_zero(style.shadow_color)) {
        float blur   = style.shadow_blur   > 0 ? style.shadow_blur
                     : (ctx->theme->shadow.blur   > 0 ? ctx->theme->shadow.blur   : 8.0f);
        int   layers = style.shadow_layers > 0 ? style.shadow_layers
                     : (ctx->theme->shadow.layers > 0 ? ctx->theme->shadow.layers : 4);
        if (ctx->immediate_mode)
            wlx_render_shadow(ctx, rect, style.shadow_color, style.shadow_offset_x,
                              style.shadow_offset_y, blur, layers, style.roundness, segs);
        else
            wlx_cmd_record_shadow(ctx, rect, style.shadow_color, style.shadow_offset_x,
                                  style.shadow_offset_y, blur, layers, style.roundness, segs);
    }
    if (!wlx_color_is_zero(style.glow_color)) {
        float spread = style.glow_spread > 0 ? style.glow_spread
                     : (ctx->theme->glow.spread > 0 ? ctx->theme->glow.spread : 4.0f);
        int   rings  = style.glow_rings  > 0 ? style.glow_rings
                     : (ctx->theme->glow.rings  > 0 ? ctx->theme->glow.rings  : 3);
        if (ctx->immediate_mode)
            wlx_render_glow(ctx, rect, style.glow_color, spread, rings, style.roundness, segs);
        else
            wlx_cmd_record_glow(ctx, rect, style.glow_color, spread, rings, style.roundness, segs);
    }

    if (style.per_side && !style.sides.all_equal) {
        if (has_gradient) {
            wlx_draw_box_gradient(ctx, rect, style.gradient_top, grad_bottom, style.roundness, segs);
        } else if (has_fill) {
            if (style.roundness > 0) wlx_draw_rect_rounded_corners(ctx, rect, style.roundness, segs, style.fill, style.rounded_corners);
            else                     wlx_draw_rect(ctx, rect, style.fill);
        }
        wlx_draw_box_edges(ctx, rect, style.sides);
        return;
    }

    if (style.per_side) {
        // Equal sides: collapse the resolved per-side values back into the
        // uniform border so explicit-but-equal sides still drive this path.
        style.border       = style.sides.top.color;
        style.border_width = style.sides.top.width;
    }

    bool has_border = style.border_width > 0 && !wlx_color_is_zero(style.border);
    if (!has_fill && !has_border && !has_gradient) return;
    if (style.roundness > 0) {
        if (has_gradient)   wlx_draw_box_gradient(ctx, rect, style.gradient_top, grad_bottom, style.roundness, segs);
        else if (has_fill)  wlx_draw_rect_rounded_corners(ctx, rect, style.roundness, segs, style.fill, style.rounded_corners);
        if (has_border) wlx_draw_rect_rounded_lines(ctx, rect, style.roundness, segs, style.border_width, style.border);
    } else {
        if (has_gradient)   wlx_draw_box_gradient(ctx, rect, style.gradient_top, grad_bottom, style.roundness, segs);
        else if (has_fill)  wlx_draw_rect(ctx, rect, style.fill);
        if (has_border) wlx_draw_rect_lines(ctx, rect, style.border_width, style.border);
    }
}

// Container decor draw path with per-side border support. Containers do not
// tint borders, so the per-side edges flow straight through the shared resolver.
static void wlx_draw_layout_decor(WLX_Context *ctx, WLX_Rect rect,
                                   WLX_Color back_color, WLX_Color border_color,
                                   float border_width, float roundness,
                                   int rounded_segments, float corner_radius,
                                   int rounded_corners,
                                   WLX_Color ct, WLX_Color cr, WLX_Color cb, WLX_Color cl,
                                   float wt, float wr, float wb, float wl,
                                   WLX_Effect_Style fx,
                                   WLX_Color gradient_top, WLX_Color gradient_bottom)
{
    bool has_bg = !wlx_color_is_zero(back_color);
    WLX_Border_Sides sides = wlx_resolve_border_sides(
        border_color, border_width, ct, cr, cb, cl, wt, wr, wb, wl);
    bool has_border = sides.top.width > 0 || sides.right.width > 0
                   || sides.bottom.width > 0 || sides.left.width > 0;
    // An effect-only or gradient-only container (no bg, no border) must still
    // reach wlx_draw_box so its shadow/glow/gradient command is recorded.
    bool has_fx = !wlx_color_is_zero(fx.shadow_color) || !wlx_color_is_zero(fx.glow_color);
    bool has_gradient = !wlx_color_is_zero(gradient_top);
    if (!has_bg && !has_border && !has_fx && !has_gradient) return;
    // When bg is set and a border is requested, fall back to theme->border if no
    // explicit uniform border_color was provided, then re-resolve so unset sides
    // inherit it.
    if (has_bg && has_border && wlx_color_is_zero(border_color)) {
        border_color = ctx->theme->border;
        sides = wlx_resolve_border_sides(
            border_color, border_width, ct, cr, cb, cl, wt, wr, wb, wl);
    }
    wlx_draw_box(ctx, rect, (WLX_Box_Style){
        .fill            = back_color,
        .border          = border_color,
        .border_width    = border_width,
        .roundness       = roundness,
        .corner_radius   = corner_radius,
        .rounded_segments = rounded_segments,
        .rounded_corners = rounded_corners,
        .sides           = sides,
        .per_side        = true,
        WLX_BOX_STYLE_EFFECTS(fx),
        .gradient_top    = gradient_top,
        .gradient_bottom = gradient_bottom,
    });
}

// Uniform-only convenience wrapper (slot decoration). Per-side edges are left
// unset so behavior is identical to the original uniform border path.
static void wlx_draw_layout_background(WLX_Context *ctx, WLX_Rect rect,
                                        WLX_Color back_color, WLX_Color border_color,
                                        float border_width, float roundness,
                                        int rounded_segments)
{
    wlx_draw_layout_decor(ctx, rect, back_color, border_color, border_width,
        roundness, rounded_segments, 0.0f, 0,
        (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0},
        -1.0f, -1.0f, -1.0f, -1.0f, (WLX_Effect_Style){0},
        (WLX_Color){0}, (WLX_Color){0});
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
    float corner_radius;
    int rounded_segments;
    int rounded_corners;
    float padding;
    float padding_top;
    float padding_right;
    float padding_bottom;
    float padding_left;
    float gap;
    WLX_Color slot_back_color;
    WLX_Color slot_border_color;
    float slot_border_width;
    WLX_Color border_color_top;
    WLX_Color border_color_right;
    WLX_Color border_color_bottom;
    WLX_Color border_color_left;
    float border_width_top;
    float border_width_right;
    float border_width_bottom;
    float border_width_left;
    WLX_SHADOW_FIELDS;
    WLX_GLOW_FIELDS;
    WLX_GRADIENT_FIELDS;
    uint32_t         interact;
    WLX_Interaction *interact_out;
    WLX_Color hover_back_color;
    WLX_Color hover_border_color;
    WLX_Color hover_border_color_top;
    WLX_Color hover_border_color_right;
    WLX_Color hover_border_color_bottom;
    WLX_Color hover_border_color_left;
    const char *id;
} WLX_Layout_Common_Opt;

#define WLX_LAYOUT_COMMON_OPT(opt) \
    (WLX_Layout_Common_Opt){ \
        .pos = (opt).pos, .span = (opt).span, \
        .back_color = (opt).back_color, .border_color = (opt).border_color, \
        .border_width = (opt).border_width, .roundness = (opt).roundness, \
        .corner_radius = (opt).corner_radius, \
        .rounded_segments = (opt).rounded_segments, \
        .rounded_corners = (opt).rounded_corners, \
        .padding = (opt).padding, .padding_top = (opt).padding_top, \
        .padding_right = (opt).padding_right, .padding_bottom = (opt).padding_bottom, \
        .padding_left = (opt).padding_left, .gap = (opt).gap, \
        .slot_back_color = (opt).slot_back_color, \
        .slot_border_color = (opt).slot_border_color, \
        .slot_border_width = (opt).slot_border_width, \
        .border_color_top = (opt).border_color_top, \
        .border_color_right = (opt).border_color_right, \
        .border_color_bottom = (opt).border_color_bottom, \
        .border_color_left = (opt).border_color_left, \
        .border_width_top = (opt).border_width_top, \
        .border_width_right = (opt).border_width_right, \
        .border_width_bottom = (opt).border_width_bottom, \
        .border_width_left = (opt).border_width_left, \
        WLX_BOX_STYLE_EFFECTS((opt)), \
        WLX_BOX_STYLE_GRADIENT((opt)), \
        .interact = (opt).interact, .interact_out = (opt).interact_out, \
        .hover_back_color = (opt).hover_back_color, \
        .hover_border_color = (opt).hover_border_color, \
        .hover_border_color_top = (opt).hover_border_color_top, \
        .hover_border_color_right = (opt).hover_border_color_right, \
        .hover_border_color_bottom = (opt).hover_border_color_bottom, \
        .hover_border_color_left = (opt).hover_border_color_left, \
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
    bool pushed_scope;          // true when co.id was non-NULL and a scope id was pushed
} WLX_Layout_Frame;

// Common prologue for all layout-begin entry points: selects the slot rect or
// root rect, closes any open sibling range, opens a new command range, draws
// the container decoration, resolves padding, insets the rect, and captures
// the active viewport dimensions for WLX_SIZE_FILL resolution.
static inline WLX_Layout_Frame wlx_layout_frame_begin(
    WLX_Context *ctx, WLX_Layout_Common_Opt co,
    const char *file, int line)
{
    bool pushed = wlx_scope_push(ctx, co.id);
    WLX_Rect r;
    if (ctx->arena.layouts.count <= 0) {
        r = ctx->rect;
    } else {
        r = wlx_get_slot_rect(ctx,
            &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1], co.pos, co.span);
    }
    wlx_cmd_close_sibling_range(ctx);
    int cmd_range_idx = wlx_cmd_open_range(ctx);

    // Interaction-aware container: when co.interact is set, resolve the
    // interaction on the container rect (the scope id is already pushed, so the
    // id folds in the container scope, matching wlx_get_state's identity rule)
    // before the chrome is recorded, then select hover-variant chrome colors
    // with replace semantics - a non-zero hover twin replaces its base color
    // while hovered, a {0} twin keeps the base. The raw result is handed back
    // through co.interact_out. interact == 0 is the non-interactive default and
    // skips the query entirely, keeping the chrome path byte-identical.
    WLX_Color back = co.back_color;
    WLX_Color bcol = co.border_color;
    WLX_Color bc_t = co.border_color_top,    bc_r = co.border_color_right;
    WLX_Color bc_b = co.border_color_bottom, bc_l = co.border_color_left;
    if (co.interact != 0) {
#ifdef WLX_DEBUG
        if (file == NULL) {
            wlx_dbg_warn_once(ctx, file, line,
                "wollix: .interact is not supported on auto-counted container begins "
                "(no stable call-site id); the interaction id collides across instances");
        }
        if (co.interact & (WLX_INTERACT_FOCUS | WLX_INTERACT_DRAG)) {
            wlx_dbg_warn_once(ctx, file, line,
                "wollix: WLX_INTERACT_FOCUS / WLX_INTERACT_DRAG are not supported on "
                "containers; use HOVER / CLICK / KEYBOARD");
        }
#endif
        WLX_Interaction it = wlx_get_interaction_for(ctx, r, co.interact, false, file, line);
        if (it.hover) {
            if (!wlx_color_is_zero(co.hover_back_color))          back = co.hover_back_color;
            if (!wlx_color_is_zero(co.hover_border_color))        bcol = co.hover_border_color;
            if (!wlx_color_is_zero(co.hover_border_color_top))    bc_t = co.hover_border_color_top;
            if (!wlx_color_is_zero(co.hover_border_color_right))  bc_r = co.hover_border_color_right;
            if (!wlx_color_is_zero(co.hover_border_color_bottom)) bc_b = co.hover_border_color_bottom;
            if (!wlx_color_is_zero(co.hover_border_color_left))   bc_l = co.hover_border_color_left;
        }
        if (co.interact_out != NULL) *co.interact_out = it;
    }

    wlx_draw_layout_decor(ctx, r, back, bcol,
                          co.border_width, co.roundness, co.rounded_segments,
                          co.corner_radius, co.rounded_corners,
                          bc_t, bc_r, bc_b, bc_l,
                          co.border_width_top, co.border_width_right,
                          co.border_width_bottom, co.border_width_left,
                          (WLX_Effect_Style){ WLX_BOX_STYLE_EFFECTS(co) },
                          co.gradient_top, co.gradient_bottom);
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
        .pushed_scope = pushed,
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
    if (count > WLX_CONTENT_SLOTS_MAX) {
        // Release fallback: tracking this layout would overflow the caller's
        // resolved[] buffer and the persistent measured[] array. Warn and
        // disable CONTENT tracking instead - CONTENT slots then size to their
        // value (0) plus min clamp in wlx_compute_offsets.
        fprintf(stderr, "%s:%d: wollix: layout has %zu slots; CONTENT sizing "
            "supports at most %d - CONTENT slots fall back to min size\n",
            file, line, count, WLX_CONTENT_SLOTS_MAX);
        return false;
    }

    WLX_State persistent = wlx_get_state_impl(ctx, sizeof(WLX_Content_Slot_State), file, line);
    WLX_Content_Slot_State *state = (WLX_Content_Slot_State *)persistent.data;
    *out_state = state;
    for (size_t i = 0; i < count; i++) {
        resolved[i] = sizes[i];
        if (sizes[i].kind == WLX_SIZE_CONTENT) {
            // Last frame's main-axis measure (a height in VERT, a width in HORZ).
            float m = state->measured[i];
            if (m <= 0) m = (sizes[i].min > 0.0f) ? 0.0f : 1.0f;
            resolved[i].kind  = WLX_SIZE_PIXELS;
            resolved[i].value = m;
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
    const WLX_Slot_Size *sizes = wlx_layout_content_sizes(ctx, l);
    if (l->content_state == NULL || sizes == NULL) return;
    if (l->kind == WLX_LAYOUT_GRID && l->has_grid_row_content_heights) {
        const float *rch = wlx_grid_row_content_heights(ctx, l);
        for (size_t r = 0; r < l->grid.rows; r++) {
            if (sizes[r].kind == WLX_SIZE_CONTENT) {
                l->content_state->measured[r] = rch[r];
            }
        }
    } else if (l->has_content_slot_measures) {
        const float *csm = wlx_layout_content_measures(ctx, l);
        for (size_t i = 0; i < l->count; i++) {
            if (sizes[i].kind == WLX_SIZE_CONTENT) {
                l->content_state->measured[i] = csm[i];
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
        size_t slot_measures_off;
        wlx_alloc_content_measure_buffer(ctx, count, &slot_measures_off);
        l.has_content_sizes = true;
        l.content_sizes_scratch_off = (size_t)((uint8_t *)sizes_copy - wlx_pool_scratch(ctx));
        l.content_state = cstate;
        l.content_slot_measures_off = slot_measures_off;
        l.has_content_slot_measures = true;
    } else if (opt.sizes != NULL) {
        wlx_compute_slot_offsets(ctx, &l, opt.sizes);
        // Retain a frame-local copy of the slot sizes so wlx_layout_end can read
        // each slot's kind even when no slot is CONTENT. This lets the fixed-slot
        // contribution override (which keeps PX/CONTENT slots from undercounting
        // an auto-height scroll panel's content height) apply to pure PX layouts.
        // The CONTENT-measurement machinery stays gated on content_state /
        // has_content_slot_measures, which remain unset on this path.
        WLX_Slot_Size *sizes_keep = (WLX_Slot_Size *)wlx_scratch_alloc_bytes(
            ctx, count * sizeof(WLX_Slot_Size), _Alignof(WLX_Slot_Size));
        memcpy(sizes_keep, opt.sizes, count * sizeof(WLX_Slot_Size));
        l.has_content_sizes = true;
        l.content_sizes_scratch_off = (size_t)((uint8_t *)sizes_keep - wlx_pool_scratch(ctx));
    }

    l.pushed_scope = frame.pushed_scope;
    wlx_pool_push(&ctx->arena.layouts, WLX_Layout, l);
    // layout_begin(ctx, vb_force, sizes, count, orient, pos, span, file, line)
    //   vb_force=-1: auto-detect vert_bounded from parent chain
    //   sizes: original slot sizes array (for FLEX/FILL warning + child vert_bounded checks)
    //   count: number of slots
    //   orient: WLX_VERT/WLX_HORZ (only VERT triggers FLEX/FILL warnings)
    //   pos/span: slot position in parent (for parent-slot-kind bounded check)
    //   file/line: caller location for warning deduplication
    WLX_DBG(layout_begin, ctx, -1, opt.sizes, count, (int)orient, opt.pos, opt.span, file, line);

    // Opt-in clip on the pushed layout (see wlx_layout_clip_begin).
    if (opt.clip) wlx_layout_clip_begin(ctx);

    // Diagnose slot over-allocation (debug builds only): when fixed/min slot
    // sizes resolve to a boundary past the layout extent, children overflow
    // onto their siblings. Checked after redistribution, and suppressed when an
    // active clip (this layout's own or an ancestor's) already contains it.
    WLX_DBG(slot_overflow, ctx,
        &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1], file, line);
}

// Begin an absolutely positioned overlay subtree on the next layer. The
// chrome (fill/border) records before the body scissor so it is never
// cropped; the body is a linear layout rooted at the padded rect. The
// overlay consumes no parent slot and contributes nothing to any parent
// layout's content tracking.
WLXDEF void wlx_overlay_begin_impl(WLX_Context *ctx, size_t count, WLX_Rect rect,
    WLX_Overlay_Opt opt, const char *file, int line)
{
    bool pushed = wlx_scope_push(ctx, opt.id);

#ifdef WLX_DEBUG
    if (ctx->immediate_mode) {
        wlx_dbg_warn_once(ctx, file, line,
            "wollix: wlx_overlay in immediate mode draws in place - layering "
            "and topmost input arbitration need the deferred recorder");
    }
    if (ctx->current_layer + 1 >= WLX_OVERLAY_MAX_LAYERS) {
        wlx_dbg_warn_once(ctx, file, line,
            "wollix: overlay nesting exceeds WLX_OVERLAY_MAX_LAYERS; content "
            "draws on the top layer");
    }
#else
    WLX_UNUSED(file);
    WLX_UNUSED(line);
#endif
    ctx->current_layer++;   // wlx_cmd_open_range clamps at the cap

    wlx_cmd_close_sibling_range(ctx);
    int range_idx = wlx_cmd_open_range(ctx);

    // Chrome before the scissor, like every container.
    if (!wlx_color_is_zero(opt.back_color) || opt.border_width > 0.0f) {
        wlx_draw_box(ctx, rect, (WLX_Box_Style){
            .fill             = opt.back_color,
            .border           = opt.border_color,
            .border_width     = opt.border_width,
            .roundness        = opt.roundness,
            .rounded_segments = opt.rounded_segments,
        });
    }

    if (opt.clip) {
        // Deliberately NOT intersected with the active clip: escaping the
        // base layer's clipping is the point of an overlay. The matching end
        // in wlx_overlay_end records into this same range, so the layer's
        // scissors stay balanced within its replay pass.
        wlx_begin_scissor(ctx, rect);
    }

    WLX_Rect body = wlx_resolve_content_rect_full(ctx->theme, rect,
        opt.content_padding,
        opt.content_padding_top, opt.content_padding_right,
        opt.content_padding_bottom, opt.content_padding_left);

    WLX_Layout l = wlx_create_layout(ctx, body, count, opt.orient, opt.gap);
    l.cmd_range_idx   = range_idx;
    l.pushed_scope    = pushed;
    l.is_overlay_root = true;
    l.overlay_clip    = opt.clip;
    l.clip_rect       = rect;     // the full overlay rect, not the padded body
    l.viewport = (opt.orient == WLX_HORZ) ? body.w : body.h;
    if (opt.sizes != NULL) {
        wlx_compute_slot_offsets(ctx, &l, opt.sizes);
    }
    // Fresh clip context for this layer: the walkers start at the overlay
    // root (pushed next, so it is the first layout they see) and ignore
    // every base-layer panel, clip layout and scissor scope. The enclosing
    // context is saved on the root and restored by wlx_overlay_end.
    l.overlay_saved_base = ctx->clip_base;
    ctx->clip_base = (WLX_Clip_Base){
        .panels_from  = ctx->arena.scroll_panels.count,
        .layouts_from = ctx->arena.layouts.count,
        .scissor_from = ctx->scissor_stack_count,
    };
    wlx_pool_push(&ctx->arena.layouts, WLX_Layout, l);
}

// Close the overlay: end the body scissor inside the overlay's own range,
// close the subtree like a normal layout (minus parent content
// contribution), restore the enclosing clip context, drop back to the
// previous layer, and - in immediate mode only - re-arm the enclosing clip
// (an ancestor clip layout or scroll panel) so subsequent base-layer content
// stays clipped. The deferred recorder needs no re-arm: the overlay's
// scissors replay in their own layer pass, so the base pass never lost its
// clip (an unconditional re-arm there emitted a stray BEGIN that only the
// pass-boundary drain closed).
WLXDEF void wlx_overlay_end(WLX_Context *ctx)
{
    // Hard guard: with an empty layout stack, count - 1 wraps to SIZE_MAX
    // (out-of-bounds read here, corrupting pool writes downstream once
    // wlx_layout_end underflows the count).
    WLX_HARD_ASSERT(ctx->arena.layouts.count > 0,
        "wlx_overlay_end without a matching begin");
    WLX_Layout *top = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
    assert(top->is_overlay_root && "wlx_overlay_end: innermost container is not an overlay");
    bool had_clip = top->overlay_clip;

    if (had_clip) wlx_end_scissor(ctx);
    // Restore the enclosing clip context before the pop (wlx_layout_end
    // pops `top`, and an overlay root never takes its clip-release branch,
    // the only clip query in there, so nothing observes the root's rect
    // under the restored base).
    ctx->clip_base = top->overlay_saved_base;
    wlx_layout_end(ctx);
    if (ctx->current_layer > 0) ctx->current_layer--;

    if (had_clip && ctx->immediate_mode) {
        WLX_Rect enclosing;
        if (wlx_active_scissor_rect(ctx, &enclosing)) {
            wlx_begin_scissor(ctx, enclosing);
        }
    }
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

    l.pushed_scope = frame.pushed_scope;
    wlx_pool_push(&ctx->arena.layouts, WLX_Layout, l);
    // layout_begin: auto layouts have no sizes array - inherit parent vert_bounded only
    WLX_DBG(layout_begin, ctx, -1, NULL, 0, 0, -1, 1, NULL, 0);

    // Opt-in clip on the pushed layout, same path as the counted begin.
    if (opt.clip) wlx_layout_clip_begin(ctx);
}

WLXDEF void wlx_grid_begin_impl(WLX_Context *ctx, size_t rows, size_t cols, WLX_Grid_Opt opt,
                                const char *file, int line) {
    WLX_Layout_Frame frame = wlx_layout_frame_begin(ctx, WLX_LAYOUT_COMMON_OPT(opt), file, line);

    // --- CONTENT row pre-resolution ---
    WLX_Content_Slot_State *cstate = NULL;
    WLX_Slot_Size *sizes_copy = NULL;
    WLX_Slot_Size resolved[WLX_CONTENT_SLOTS_MAX];
    const WLX_Slot_Size *effective_row_sizes = opt.row_sizes;
    size_t sizes_copy_off = 0;
    if (wlx_prepare_content_sizes(ctx, opt.row_sizes, rows, file, line,
            &cstate, &sizes_copy, resolved)) {
        effective_row_sizes = resolved;
        // Capture the scratch offset now: wlx_create_grid may grow the byte
        // scratch arena (wide constrained column sets allocate redistribution
        // working buffers there), which would invalidate sizes_copy.
        sizes_copy_off = (size_t)((uint8_t *)sizes_copy - wlx_pool_scratch(ctx));
    }

    WLX_Layout l = wlx_create_grid(ctx, frame.rect, rows, cols,
                                         effective_row_sizes, opt.col_sizes, opt.gap);
    wlx_layout_apply_common(&l, WLX_LAYOUT_COMMON_OPT(opt), frame);

    if (cstate != NULL) {
        l.has_content_sizes = true;
        l.content_sizes_scratch_off = sizes_copy_off;
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

    l.pushed_scope = frame.pushed_scope;
    wlx_pool_push(&ctx->arena.layouts, WLX_Layout, l);
    // layout_begin: grid layouts - inherit parent vert_bounded, no per-slot FLEX/FILL check
    WLX_DBG(layout_begin, ctx, -1, NULL, 0, 0, -1, 1, NULL, 0);
}

WLXDEF void wlx_grid_begin_auto_impl(WLX_Context *ctx, size_t cols, float row_px, WLX_Grid_Auto_Opt opt) {
    WLX_Layout_Frame frame = wlx_layout_frame_begin(ctx, WLX_LAYOUT_COMMON_OPT(opt), NULL, 0);

    WLX_Layout l = wlx_create_grid_auto(ctx, frame.rect, cols, row_px, opt.col_sizes, opt.gap);
    wlx_layout_apply_common(&l, WLX_LAYOUT_COMMON_OPT(opt), frame);
    l.pushed_scope = frame.pushed_scope;
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
    l.pushed_scope = frame.pushed_scope;
    wlx_pool_push(&ctx->arena.layouts, WLX_Layout, l);
    // layout_begin: grid auto tile - inherit parent vert_bounded
    WLX_DBG(layout_begin, ctx, -1, NULL, 0, 0, -1, 1, NULL, 0);
}

// Scope-pop half of the layout frame lifecycle: pops the layout from the stack
// and pops the scope id if one was pushed at begin.
static inline void wlx_layout_frame_end(WLX_Context *ctx, WLX_Layout *l) {
    bool pop_scope = l->pushed_scope;
    ctx->arena.layouts.count -= 1;
    wlx_scope_pop(ctx, pop_scope);
}

WLXDEF void wlx_layout_end(WLX_Context *ctx) {
    assert(ctx != NULL);
    assert(ctx->arena.layouts.count > 0);
    WLX_Layout *l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];

    // Single owner of clip scissor release: any layout (including a panel body)
    // that began a clip scissor at begin releases it here once its child range
    // has been closed.
    bool clip = l->clip_active;

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
    // Overlay roots float outside the slot tree and contribute nothing.
    if (!l->is_overlay_root && ctx->arena.layouts.count > 1) {
        WLX_Layout *parent = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 2];
        // VERT parents only: the bucket stores the parent's main-axis extent,
        // and a nested layout has no intrinsic width to offer a HORZ parent
        // (elastic layouts cannot report a natural width).
        bool parent_horz = wlx_layout_is_horz(parent);
        if (parent->has_content_slot_measures && !parent_horz && parent->index > 0) {
            size_t slot_idx = parent->index - 1;
            if (slot_idx < WLX_CONTENT_SLOTS_MAX) {
                wlx_layout_content_measures(ctx, parent)[slot_idx] +=
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
    if (wlx_layout_is_vert(l) && l->gap > 0.0f && l->index > 1) {
        l->accumulated_content_height += l->gap * (float)(l->index - 1);
    }

    // --- Propagate content height to parent layout ---
    // Instead of contributing to auto_scroll.total_height per-layout (which
    // caused double-counting for nested layouts), fold this layout's content
    // height into the parent's accumulated_content_height.  The wrapper
    // layout in wlx_scroll_panel_end reads the fully-aggregated tree total.
    // Overlay roots float outside the slot tree and contribute nothing.
    if (!l->is_overlay_root && ctx->arena.layouts.count > 1) {
        WLX_Layout *parent = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 2];
        float child_h = l->accumulated_content_height + l->padding_top + l->padding_bottom;
        // For VERT linear parents with explicitly-sized slots (PX or
        // CONTENT), the child occupies exactly the slot's allocated
        // height in the parent rect, regardless of how tall the child
        // actually rendered. Use the slot height as the contribution
        // so siblings placed in fixed-size slots that render shorter
        // than their slot do not undercount the parent's total content
        // height (which the auto-height scroll panel uses to compute
        // max_scroll), and so children that overflow their slot do
        // not overcount it (overflow is overdraw, not extra content).
        // FLEX/FILL slots keep child_h (size depends on parent rect, would
        // create runaway feedback inside auto-height scroll panels).
        float contrib = child_h;
        const WLX_Slot_Size *parent_sizes = wlx_layout_content_sizes(ctx, parent);
        if (wlx_layout_is_vert(parent)
            && parent_sizes != NULL
            && parent->index > 0 && parent->index <= parent->count) {
            WLX_Size_Kind k = parent_sizes[parent->index - 1].kind;
            if (k == WLX_SIZE_PIXELS || k == WLX_SIZE_CONTENT) {
                const float *poff = wlx_layout_offsets(ctx, parent);
                float slot_h = poff[parent->index] - poff[parent->index - 1];
                // wlx_compute_offsets stores boundary positions, so the
                // delta between adjacent boundaries includes the trailing
                // inter-slot gap for non-last slots. Strip it here - the
                // parent adds total_gap once via the gap accumulation
                // step above (l->gap * (l->index - 1)).
                if (parent->index < parent->count) slot_h -= parent->gap;
                contrib = slot_h;
            }
        }
        // Accumulated-only contribution: per-slot and per-row buckets were
        // already updated earlier with the pre-gap, non-overridden child_h.
        wlx_contribute_to_parent_layout(ctx, parent, (WLX_Parent_Contribution){
            .h_contrib  = contrib,
            .slot_index = WLX_SLOT_SKIP,
            .grid_row   = WLX_SLOT_SKIP,
        });
    }

    // --- Compute CONTENT slot deltas for deferred replay ---
    // Gated on !immediate_mode: the deferred recorder owns the scratch
    // lifetime the dx/dy correction relies on (the sizes array itself is
    // re-derived from its offset by wlx_layout_content_sizes).
    const WLX_Slot_Size *own_sizes = wlx_layout_content_sizes(ctx, l);
    if (!ctx->immediate_mode && l->cmd_range_idx >= 0 && l->content_state != NULL
        && own_sizes != NULL && l->kind == WLX_LAYOUT_LINEAR
        && l->has_content_slot_measures) {
        bool horz = wlx_layout_is_horz(l);
        const float *csm = wlx_layout_content_measures(ctx, l);
        WLX_Slot_Size resolved[WLX_CONTENT_SLOTS_MAX];
        WLX_HARD_ASSERT(l->count <= WLX_CONTENT_SLOTS_MAX,
            "CONTENT-tracked layout slot count exceeds WLX_CONTENT_SLOTS_MAX");
        for (size_t i = 0; i < l->count; i++) {
            resolved[i] = own_sizes[i];
            if (own_sizes[i].kind == WLX_SIZE_CONTENT) {
                resolved[i].kind = WLX_SIZE_PIXELS;
                resolved[i].value = csm[i];
            }
        }
        float new_offsets[WLX_CONTENT_SLOTS_MAX + 1];
        float total = wlx_layout_main_extent(l);
        wlx_compute_offsets_ctx(ctx, new_offsets, l->count, total, l->viewport, resolved, l->gap);

        // Walk child ranges of this layout and assign per-slot deltas on the
        // layout's main axis. Child ranges appear in slot order by
        // construction.
        const float *offsets = wlx_layout_offsets(ctx, l);
        size_t slot = 0;
        for (size_t ri = 0; ri < ctx->arena.cmd_ranges.count && slot < l->count; ri++) {
            if (wlx_pool_cmd_ranges(ctx)[ri].parent_range_idx == l->cmd_range_idx) {
                float delta = new_offsets[slot] - offsets[slot];
                if (horz) wlx_pool_cmd_ranges(ctx)[ri].dx_offset = delta;
                else      wlx_pool_cmd_ranges(ctx)[ri].dy_offset = delta;
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

    // Release the clip scissor after the range is closed, so the SCISSOR_END
    // is recorded into the parent range (matching the original panel order).
    // Backends end clipping by disabling the scissor test outright (raylib's
    // EndScissorMode) rather than popping to the previously active region, so
    // after releasing this layout's clip we must re-arm the enclosing scissor
    // (an ancestor clip layout or a scroll panel). Without this, content drawn
    // after a clipped layout inside a scroll panel escapes the panel clip and
    // bleeds over neighbouring chrome (e.g. a top bar above the panel).
    if (clip) {
        wlx_end_scissor(ctx);
        l->clip_active = false;  // exclude self from the enclosing-scissor query
        WLX_Rect enclosing;
        if (wlx_active_scissor_rect(ctx, &enclosing)) {
            wlx_begin_scissor(ctx, enclosing);
        }
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

WLXDEF float wlx_get_scroll_panel_offset(WLX_Context *ctx) {
    assert(ctx != NULL);
    if (ctx->arena.scroll_panels.count > 0) {
        return wlx_pool_scroll_panels(ctx)[ctx->arena.scroll_panels.count - 1]->scroll_offset;
    }
    return 0.0f;
}

// Rect of the most recent widget placed this frame (any widget on the
// standard prologue: button, label, checkbox, dropdown face, ...). The
// natural anchor for wlx_tooltip_for right after the widget call:
//
//   wlx_button(&ctx, "Save");
//   wlx_tooltip_for(&ctx, wlx_last_rect(&ctx), "Write the file to disk");
//
// {0,0,0,0} before the first widget of a frame. A dropdown reports its
// face even while its list is open (the rows are internal). Inside an
// open menu body it reports the latest item; after the wlx_menu_end of a
// wlx_menu_button_begin block it reports the face again, while a
// point-anchored wlx_menu_begin leaves the last item in place.
WLXDEF WLX_Rect wlx_last_rect(WLX_Context *ctx)
{
    return ctx->last_widget_rect;
}

WLXDEF void wlx_set_style_transform(WLX_Context *ctx, WLX_Style_Transform_Fn fn, void *user) {
    assert(ctx != NULL);
    ctx->style_transform = fn;
    ctx->style_transform_user = user;
    ctx->style_transform_generation++;
}

WLXDEF void wlx_set_cull_offscreen(WLX_Context *ctx, bool enabled) {
    assert(ctx != NULL);
    ctx->cull_offscreen = enabled;
}

// Resolve a WLX_Slot_Size to pixels and set it as the next slot size in the
// innermost dynamic layout.  FLEX/AUTO are greedy (take all remaining space).
WLXDEF void wlx_layout_auto_slot(WLX_Context *ctx, WLX_Slot_Size size) {
    assert(ctx != NULL);
    assert(ctx->arena.layouts.count > 0 && "wlx_layout_auto_slot called outside a layout");

    WLX_Layout *l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
    assert(l->linear.dynamic && "wlx_layout_auto_slot is only valid inside a wlx_layout_begin_auto block");

    float total = wlx_layout_main_extent(l);
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

// A widget rect clipped to the current layer's container clips - scroll
// panel viewports, .clip layouts, and the overlay root's own rect on a popup
// layer: the zone the pointer must be inside for the widget to claim it, and
// the candidate rect recorded for arbitration. Widgets scrolled or clipped
// out of view thus cannot claim the pointer, and popup content is never
// gated by the base panels it floats over.
static inline WLX_Rect wlx_interaction_clip_rect(WLX_Context *ctx, WLX_Rect rect) {
    WLX_Rect clip;
    if (wlx_enclosing_clip(ctx, WLX_CLIP_CONTAINERS, &clip)) rect = wlx_rect_intersect(rect, clip);
    return rect;
}

static inline bool wlx_interaction_mouse_over(WLX_Context *ctx, WLX_Rect rect) {
    float mx = (float)ctx->input.mouse_x, my = (float)ctx->input.mouse_y;
    if (!wlx_rect_contains(rect, mx, my)) return false;
    return wlx_rect_contains(wlx_interaction_clip_rect(ctx, rect), mx, my);
}

// Record the focus ring geometry at the focused widget's query: the rect
// clipped to the layer's containers, and the container clip itself so the
// ring drawn at frame end stays inside it.
static inline void wlx_focus_rect_record(WLX_Context *ctx, WLX_Rect rect) {
    WLX_Rect clip;
    bool clipped = wlx_enclosing_clip(ctx, WLX_CLIP_CONTAINERS, &clip);
    ctx->interaction.focus_rect = clipped ? wlx_rect_intersect(rect, clip) : rect;
    ctx->interaction.focus_clip = clip;
    ctx->interaction.focus_clip_active = clipped;
}

static inline void wlx_interaction_compute_hover(WLX_Context *ctx, size_t id, bool mouse_over, WLX_Interaction *result) {
    // Arbitrated frames resolve hover once at frame begin (hot_id holds the
    // owner); the query-time capture below is the bootstrap fallback.
    if (!ctx->interaction.arbitrate) {
        if (mouse_over && (ctx->interaction.active_id == 0 || ctx->interaction.active_id == id)) {
            ctx->interaction.hot_id = id;
        }
    }

    result->hover = (ctx->interaction.hot_id == id);
}

static inline void wlx_interaction_handle_click(WLX_Context *ctx, size_t id, bool mouse_over, WLX_Interaction *result) {
    bool acquire = ctx->interaction.arbitrate
        ? (ctx->interaction.press_owner == id && ctx->input.mouse_clicked
           && ctx->interaction.active_id == 0)
        : (mouse_over && ctx->interaction.active_id == 0 && ctx->input.mouse_clicked);
    if (acquire) {
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

static inline void wlx_interaction_handle_focus(WLX_Context *ctx, size_t id, bool mouse_over, WLX_Interaction *result, bool hold_enter) {
    bool was_focused = (ctx->interaction.active_id == id);
    result->focused = was_focused;

    // Focus was force-released at frame begin (press outside the recorded
    // rect): deliver the blur edge the widget would have seen from its own
    // click-elsewhere branch.
    if (ctx->interaction.focus_released_id == id) {
        result->just_unfocused = true;
        ctx->interaction.focus_released_id = 0;
    }

    // Keyboard traversal landed here this frame: acquire exactly as a
    // click would, so Tab into a field focuses it for typing. One-shot, so
    // an Escape blur later is not re-acquired next frame.
    if (ctx->interaction.focus_gained_id == id) {
        ctx->interaction.focus_gained_id = 0;
        ctx->interaction.active_id = id;
        result->focused = true;
        if (!was_focused) {
            result->just_focused = true;
        }
        was_focused = true;
    }

    if (ctx->input.mouse_clicked) {
        bool acquire = ctx->interaction.arbitrate
            ? (ctx->interaction.press_owner == id)
            : mouse_over;
        if (acquire) {
            ctx->interaction.active_id = id;
            result->focused = true;
            if (!was_focused) {
                result->just_focused = true;
            }
        } else if (was_focused) {
            // Arbitrated outside-presses release at frame begin; this branch
            // is the bootstrap fallback.
            wlx_interaction_blur(ctx, result);
        }
    }

    if (result->focused && !hold_enter && wlx_is_key_pressed(ctx, WLX_KEY_ENTER)) {
        wlx_interaction_blur(ctx, result);
        // The same Enter press must not also keyboard-activate a widget
        // processed later this frame.
        ctx->interaction.enter_consumed = true;
    }

    if (result->focused && wlx_is_key_pressed(ctx, WLX_KEY_ESCAPE)) {
        wlx_interaction_blur(ctx, result);
    }

    result->active = result->focused;
}

static inline void wlx_interaction_handle_drag(WLX_Context *ctx, size_t id, bool mouse_over, WLX_Interaction *result) {
    // press_owner stays latched while the button is held, so only the widget
    // the press landed on may (re)acquire the drag.
    bool acquire = ctx->interaction.arbitrate
        ? (ctx->interaction.press_owner == id && ctx->interaction.active_id == 0
           && ctx->input.mouse_down)
        : (mouse_over && ctx->interaction.active_id == 0 && ctx->input.mouse_down);
    if (acquire) {
        ctx->interaction.active_id = id;
    }

    if (ctx->interaction.active_id == id) {
        // Hold on mouse_down, the field every host fills; mouse_held is its
        // legacy twin (ADR_040) and a host that fills only mouse_down must
        // not lose the drag on the next frame.
        if (ctx->input.mouse_down) {
            result->active = true;
            result->pressed = true;
        } else {
            ctx->interaction.active_id = 0;
            result->active = false;
        }
    }
}

static inline void wlx_interaction_handle_keyboard(WLX_Context *ctx, size_t id, WLX_Interaction *result) {
    // Hovered or keyboard-focused widgets activate on Space/Enter.
    if (ctx->interaction.hot_id != id && ctx->interaction.focus_id != id) return;
    // Keyboard activation is only valid when no other widget owns active_id
    // (a focused input field owns the keyboard).
    if (ctx->interaction.active_id != 0 && ctx->interaction.active_id != id) return;
    bool enter_hit = wlx_is_key_pressed(ctx, WLX_KEY_ENTER) && !ctx->interaction.enter_consumed;
    if (wlx_is_key_pressed(ctx, WLX_KEY_SPACE) || enter_hit) {
        result->clicked = true;
    }
}

// Append one interactive query to this frame's candidate list (consumed by
// next frame's ownership arbitration). Grow-and-reuse storage; a failed
// grow drops the candidate and arbitration degrades gracefully.
static inline void wlx_candidates_push(WLX_Context *ctx, size_t id,
                                       WLX_Rect rect, int layer,
                                       WLX_Cursor_Shape cursor, bool focusable) {
    WLX_Candidate_List *cur = &ctx->cands[ctx->cand_frame & 1];
    if (cur->count == cur->capacity) {
        size_t new_cap = cur->capacity == 0 ? WLX_GROW_INIT_CAP : cur->capacity * 2;
        WLX_HARD_ASSERT(new_cap <= SIZE_MAX / sizeof(WLX_Interaction_Candidate),
            "candidate list size overflow");
        WLX_Interaction_Candidate *ni = (WLX_Interaction_Candidate *)
            wlx_realloc(cur->items, new_cap * sizeof(*ni));
        if (ni == NULL) return;
        cur->items = ni;
        cur->capacity = new_cap;
    }
    cur->items[cur->count++] = (WLX_Interaction_Candidate){
        .id = id, .rect = rect, .layer = layer, .cursor = (uint8_t)cursor,
        .focusable = focusable };
}

// Internal variant of wlx_get_interaction() that adds an explicit `disabled`
// gate. When `disabled` is true, the global interaction state (hot_id and
// active_id) is left untouched, all action booleans (clicked, pressed,
// focused, active, just_focused, just_unfocused) are forced false, and
// result.disabled is set. Hover is still reported when the WLX_INTERACT_HOVER
// flag is set, but only as a local rect-containment check so a disabled
// widget cannot claim hot ownership from a sibling.
static inline WLX_Interaction wlx_get_interaction_for(WLX_Context *ctx, WLX_Rect rect, uint32_t flags, bool disabled, const char *file, int line) {
    size_t id = wlx_interaction_make_id(ctx, file, line);
    WLX_Interaction result = { .id = id };

    // The hit zone: the rect clipped to the layer's container clips (scroll
    // panel viewports, .clip layouts, a popup's own rect). Containment
    // against it is the mouse-over test, and it is the candidate rect
    // recorded below.
    WLX_Rect crect = wlx_interaction_clip_rect(ctx, rect);
    bool mouse_over = wlx_rect_contains(crect, (float)ctx->input.mouse_x, (float)ctx->input.mouse_y);

    if (disabled) {
        if (flags & WLX_INTERACT_HOVER) {
            result.hover = mouse_over;
        }
        result.disabled = true;
        return result;
    }

    // Record the hit candidate for next frame's ownership arbitration: the
    // same clipped zone the mouse-over test used, so scrolled-away or
    // clipped-away widgets cannot own the pointer.
    {
        // A Tab stop must also be visible somewhere: a widget scrolled out of
        // its panel or cropped away by a .clip layout has an empty hit zone
        // and is skipped, or keystrokes would reach an invisible widget.
        bool focusable = !(flags & WLX_INTERACT_TAB_SKIP)
            && ((flags & WLX_INTERACT_FOCUS)
                || ((flags & WLX_INTERACT_CLICK) && (flags & WLX_INTERACT_KEYBOARD)))
            && crect.w > 0.0f && crect.h > 0.0f;
        wlx_candidates_push(ctx, id, crect, ctx->current_layer,
            (flags & WLX_INTERACT_TEXT_CURSOR) ? WLX_CURSOR_IBEAM : WLX_CURSOR_ARROW,
            focusable);
        // The keyboard-focused widget records its rect for the focus ring
        // and proves it is still declared (GC otherwise drops the focus).
        if (ctx->interaction.focus_id != 0 && ctx->interaction.focus_id == id) {
            ctx->interaction.focus_id_seen = true;
            wlx_focus_rect_record(ctx, rect);
        }
    }

    // The press owner announced itself: an enclosing popup comparing the
    // flag before and after its subtree learns whether this frame's press
    // landed inside it, with no rect math.
    if (ctx->interaction.press_owner != 0
            && id == ctx->interaction.press_owner) {
        ctx->interaction.press_claimed = true;
    }

    // Right press is a press-frame edge resolved for every enabled query,
    // independent of the flag set: the previous frame's topmost candidate
    // under the pointer owns it; bootstrap frames fall back to direct
    // containment. Fires on the press, not the release.
    if (ctx->input.mouse_right_clicked) {
        result.right_clicked = ctx->interaction.arbitrate
            ? (ctx->interaction.right_press_owner == id)
            : mouse_over;
    }

    if (flags & WLX_INTERACT_HOVER) {
        wlx_interaction_compute_hover(ctx, id, mouse_over, &result);
    }

    if (flags & WLX_INTERACT_CLICK) {
        wlx_interaction_handle_click(ctx, id, mouse_over, &result);
    }

    if (flags & WLX_INTERACT_FOCUS) {
        wlx_interaction_handle_focus(ctx, id, mouse_over, &result,
                                     (flags & WLX_INTERACT_FOCUS_HOLD_ENTER) != 0);
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
        // Record the holder's class for the frame-begin focus release and
        // for the Tab traversal gate.
        ctx->interaction.active_is_focus = (flags & WLX_INTERACT_FOCUS) != 0;
        ctx->interaction.active_consumes_tab =
            (flags & (WLX_INTERACT_FOCUS | WLX_INTERACT_FOCUS_HOLD_TAB))
            == (WLX_INTERACT_FOCUS | WLX_INTERACT_FOCUS_HOLD_TAB);
    }

    return result;
}

WLXDEF size_t wlx_focused_id(WLX_Context *ctx) {
    return ctx->interaction.focus_id;
}

// Replace the focus-ring rect recorded by the query that just ran, for
// widgets whose interactive zone is a sub-rect of their visual bounds (the
// editor's gutter-excluded band): the ring should wrap what the user sees,
// not the hit zone. Call only when that query proved it holds the ring
// (focus_id_seen flipped false -> true across it); applies the same
// container clip as the candidate record.
static inline void wlx_focus_ring_rect(WLX_Context *ctx, WLX_Rect rect) {
    wlx_focus_rect_record(ctx, rect);
}

WLXDEF WLX_Interaction wlx_get_interaction(WLX_Context *ctx, WLX_Rect rect, uint32_t flags, const char *file, int line) {
    // ID = hash(file, line) ^ id_stack_hash.
    // Use wlx_push_id()/wlx_pop_id() for loop disambiguation.
    return wlx_get_interaction_for(ctx, rect, flags, false, file, line);
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
    WLX_HARD_ASSERT(map->slots != NULL, "Unable to allocate more RAM");
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
        WLX_HARD_ASSERT(slot->data != NULL, "Unable to allocate more RAM");
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
    WLX_HARD_ASSERT(slot->data_size == state_size,
        "state id collision: two call sites resolved to the same id with different state sizes");
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

    // wlx_widget intentionally does not carry .disabled (see ADR_018
    // coverage matrix); the _for form with disabled=false keeps the
    // call shape consistent with the disabled-aware widgets and makes
    // a future opt-in a one-line change.
    WLX_Interaction inter = wlx_get_interaction_for(
        ctx,
        wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD
            | WLX_INTERACT_TAB_SKIP,
        false,
        file, line
    );

    opt.back_color = wlx_color_hover_tint(
        opt.back_color, inter.hover, inter.disabled, ctx->theme->hover_brightness);
    opt.border_color = wlx_color_hover_tint(
        opt.border_color, inter.hover, inter.disabled, ctx->theme->hover_brightness);

    WLX_Border_Sides sides = wlx_border_sides_for_widget(
        ctx->theme, inter.hover, inter.disabled, opt.opacity,
        opt.border_color, opt.border_width,
        WLX_BORDER_SIDES_ARGS(opt));

    wlx_draw_box(ctx, wr, (WLX_Box_Style){
        .fill             = opt.back_color,
        .border           = opt.border_color,
        .border_width     = opt.border_width,
        .roundness        = opt.roundness,
        .rounded_segments = opt.rounded_segments,
        .sides            = sides,
        .per_side         = true,
        WLX_BOX_STYLE_EFFECTS(opt),
        WLX_BOX_STYLE_GRADIENT(opt),
        WLX_BOX_STYLE_CORNER(opt),
    });

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);
}

// ============================================================================
// Implementation: text layout and rendering helpers
// ============================================================================
// Text helpers in this section are implementation-owned helpers and use the
// canonical `wlx_` prefix.
//
// The machinery reads in six regions, in file order (see
// docs/TEXT_PIPELINE_MAP.md for the function-by-function map and the
// invariant registry that binds them):
//   1. UTF-8 text units and newline policy - unit stepping, word and
//      separator helpers.
//   2. Budgets, policy constants, and the build types - including
//      WLX_Text_Line_Record, the "layout line" every geometry consumer
//      reads.
//   3. Measurement primitives - tab-aware prefix measurement, the shared
//      fit decision, and the batched cumulative-advance fill.
//   4. Retained editor line geometry - the editor's shaped-line cache.
//   5. The build kernel - a resumable greedy fitter over cumulative unit
//      advances ("advance provider" sources).
//   6. Alignment, emission, and the from-lines geometry consumers.

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

// Count codepoints in a UTF-8 slice.
static inline size_t wlx_utf8_slicelen(const char *s, size_t len) {
    size_t count = 0;
    size_t i = 0;
    while (i < len) {
        i += wlx_utf8_char_len(s + i);
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

static inline bool wlx_text_utf8_is_continuation(unsigned char c) {
    return (c & 0xC0u) == 0x80u;
}

// Returns true only when off starts a valid 2-4 byte UTF-8 sequence fully
// contained in the range. ASCII and malformed bytes fall back to one-byte
// handling in the caller.
static inline bool wlx_text_utf8_sequence_at(const char *text, size_t length, size_t off, size_t *out_len) {
    if (text == NULL || off >= length) return false;
    size_t char_len = wlx_utf8_char_len(text + off);
    if (char_len <= 1 || off + char_len > length) return false;
    for (size_t i = 1; i < char_len; i++) {
        if (!wlx_text_utf8_is_continuation((unsigned char)text[off + i])) return false;
    }
    if (out_len) *out_len = char_len;
    return true;
}

// Next unit boundary after pos: one whole codepoint when a valid
// multibyte sequence lies fully in range, else one byte so malformed
// input still makes progress; clamps at the text end. The single
// stepping entry - layout walks and caret/edit motion all step through
// it, so they agree over invalid bytes.
static inline size_t wlx_text_unit_next(const char *text, size_t length, size_t pos) {
    if (pos >= length) return length;
    size_t char_len = 1;
    size_t seq_len = 0;
    if (wlx_text_utf8_sequence_at(text, length, pos, &seq_len)) char_len = seq_len;
    return pos + char_len > length ? length : pos + char_len;
}

// Move byte position forward to the next codepoint boundary.
// Returns new byte position (len if already at end). Keeps the
// historical (s, pos, len) parameter order; steps exactly like
// wlx_text_unit_next, including the one-byte fallback on malformed
// bytes.
static inline size_t wlx_utf8_next(const char *s, size_t byte_pos, size_t len) {
    return wlx_text_unit_next(s, len, byte_pos);
}

// Byte class used by word-wise cursor motion: whitespace separates words;
// every other byte is a word byte. Scanning single bytes is UTF-8 safe here
// because the separators are ASCII and multibyte sequences never contain
// ASCII bytes, so the returned positions always land on codepoint boundaries.
static inline bool wlx_utf8_is_word_separator(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// Move left to the start of the previous word: skip separators, then the word.
static inline size_t wlx_utf8_word_prev(const char *s, size_t byte_pos) {
    while (byte_pos > 0 && wlx_utf8_is_word_separator(s[byte_pos - 1])) byte_pos--;
    while (byte_pos > 0 && !wlx_utf8_is_word_separator(s[byte_pos - 1])) byte_pos--;
    return byte_pos;
}

// Move right to the end of the next word: skip separators, then the word.
static inline size_t wlx_utf8_word_next(const char *s, size_t byte_pos, size_t len) {
    while (byte_pos < len && wlx_utf8_is_word_separator(s[byte_pos])) byte_pos++;
    while (byte_pos < len && !wlx_utf8_is_word_separator(s[byte_pos])) byte_pos++;
    return byte_pos;
}

#ifndef WLX_TEXT_RUN_MAX_UNITS
#define WLX_TEXT_RUN_MAX_UNITS 512
#endif
#ifndef WLX_TEXT_RUN_MAX_LINES
#define WLX_TEXT_RUN_MAX_LINES 128
#endif

// Multiline inputbox text-run budget. The multiline geometry build (caret,
// hit-test, selection, scroll, draw) uses these caps instead of the global
// text-run caps, so a tall note field can hold a few kilobytes of prose
// without raising the per-run budget of every other text widget. Content
// beyond the caps stays in the buffer but drops out of geometry: the caret
// pins to the end of the last built line and the view cannot scroll past it.
#ifndef WLX_INPUTBOX_MULTILINE_MAX_UNITS
#define WLX_INPUTBOX_MULTILINE_MAX_UNITS 4096
#endif
#ifndef WLX_INPUTBOX_MULTILINE_MAX_LINES
#define WLX_INPUTBOX_MULTILINE_MAX_LINES 512
#endif

// Per-record unit budget for truncate-and-continue line builds. Each
// non-wrap record measures at most this many text units (codepoints or
// fallback bytes); in wrap mode the budget is shared per hard line
// across its wrapped rows. Geometry past the cap freezes per line and
// the invisible tail is skipped to the next hard line start without
// measuring.
#ifndef WLX_EDITOR_MAX_LINE_UNITS
#define WLX_EDITOR_MAX_LINE_UNITS 1024
#endif

// A no-wrap re-entry origin snaps to a text-unit boundary, preferring the
// edge just after a space within this many bytes behind the candidate:
// word-shaped content then re-enters at a shaping-neutral edge.
#ifndef WLX_EDITOR_ORIGIN_BACKSCAN
#define WLX_EDITOR_ORIGIN_BACKSCAN 64
#endif

// Text undo journal bounds, per widget and per direction (undo, redo):
// retained entries and bytes of removed text. Whole oldest undo steps are
// evicted first; a single step larger than either cap is admitted and
// evicts everything older. WLX_TEXT_UNDO_ENTRIES 0 compiles the journal
// out (no history, the undo chords are no-ops).
#ifndef WLX_TEXT_UNDO_ENTRIES
#define WLX_TEXT_UNDO_ENTRIES 512
#endif
#ifndef WLX_TEXT_UNDO_BYTES
#define WLX_TEXT_UNDO_BYTES 262144
#endif
#if WLX_TEXT_UNDO_ENTRIES != 0 && WLX_TEXT_UNDO_ENTRIES < 8
#error "WLX_TEXT_UNDO_ENTRIES must be 0 (journal compiled out) or at least 8"
#endif
// A coalesced typing or delete run closes once it holds this many bytes,
// so one undo step stays bounded and the run's in-place growth stays cheap.
#define WLX_TEXT_UNDO_GROUP_BYTES 4096

// Unit cap per measure_text_advances request. One backend call fills at
// most this many cumulative advances; longer stretches issue consecutive
// chunks spliced by adding the running base advance (the splice is a
// shaping-context seam, same class as a tab stop). Also sizes the
// stack-local batching buffers, so keep it a few hundred at most.
#ifndef WLX_TEXT_ADVANCES_CHUNK
#define WLX_TEXT_ADVANCES_CHUNK 256
#endif

// Retained-geometry (geom store) policy constants. Not tuning knobs: the
// values pair with the origin policy's estimates and the suites that pin
// them.
#define WLX_TEXT_GEOM_ORIGIN_VIEW_SLACK 0.5f   // origin back margin, in view widths
#define WLX_TEXT_GEOM_ANCHOR_HEADROOM_DIV 2    // anchor headroom: unit budget / this
#define WLX_TEXT_GEOM_FAR_GAP_BUDGETS 4        // gap probes stop after this many budgets
#define WLX_TEXT_GEOM_AVG_MIN_UNITS 64         // units before an entry seeds the avg advance
#define WLX_TEXT_GEOM_FALLBACK_ADVANCE_EM 0.5f // avg-advance fallback, in line heights
#define WLX_TEXT_GEOM_STORE_SLACK 2            // store sizing: viewports of entries retained
#define WLX_TEXT_GEOM_STORE_MIN 16             // store sizing floor, in entries

// One visual line of a build: four byte ranges over the source slice
// (source / visible / separator / cursor - consecutive records tile
// the covered text) plus measured geometry and the draw origin the
// alignment pass fills. The record array is the pipeline's one
// geometry source; the byte-range diagram is docs/LINE_RUN_MODEL.md
// section 4.
typedef struct WLX_Text_Line_Record {
    size_t source_start;
    size_t source_end;
    size_t visible_start;
    size_t visible_end;
    size_t cursor_start;
    size_t cursor_end;
    size_t separator_start;
    size_t separator_end;
    float advance_w;
    float measured_w;
    float measured_h;
    float origin_x;
    float origin_y;
    float line_h;
    bool ended_by_newline;
    bool empty_visual;
} WLX_Text_Line_Record;

typedef struct WLX_Text_Line_Record_Opt {
    size_t source_start;
    size_t visible_start;
    size_t visible_end;
    size_t separator_start;
    size_t separator_end;
    float measured_w;
    float measured_h;
    bool has_measurement;
    float line_h;
    WLX_Text_Style style;
    bool ended_by_newline;
} WLX_Text_Line_Record_Opt;

// Frame-local wrap row-count memo, direct-mapped by line index. rows == 0
// marks an empty slot (a counted line always yields >= 1 row). Valid for
// one frame at one band width/style: the editor widget zeroes it each
// frame after the scrollbar strips fix the band, and edits run before
// any geometry, so nothing invalidates mid-frame.
#define WLX_WRAP_ROW_MEMO_SLOTS 8   // power of two; anchor + caret lines dominate
typedef struct {
    size_t line[WLX_WRAP_ROW_MEMO_SLOTS];
    size_t rows[WLX_WRAP_ROW_MEMO_SLOTS];
} WLX_Wrap_Row_Memo;

// Everything one line build reads: the measurement environment, the
// fit rect, the mode flags, and the editor-only extensions (retained
// store, view geometry, tail-skip hint). Mode axes are
// sentinel-encoded - 0 / NULL / negative mean disabled - and the field
// comments below carry the legal combinations.
typedef struct WLX_Text_Build_Inputs {
    WLX_Context *ctx;
    const char *text;
    size_t length;
    WLX_Text_Style style;
    WLX_Rect rect;
    bool wrap;
    float line_h;
    size_t text_unit_cap;
    // Truncated-line continuation. Non-wrap: a width- or budget-truncated
    // record does not end the build; its invisible tail is skipped to the
    // next hard line start without measuring, and the unit budget resets
    // per record (capped at WLX_EDITOR_MAX_LINE_UNITS instead of
    // text_unit_cap). Wrap: the unit budget is per hard line - one
    // WLX_EDITOR_MAX_LINE_UNITS budget shared by the line's wrapped rows,
    // refreshed at hard line starts; a line exhausting it freezes and its
    // final record skips the unmeasured tail to the next hard line start.
    bool truncate_continue;
    // Tab expansion: > 0 makes every '\t' advance the measured prefix to
    // the next multiple of this many pixels (next-tab-stop policy);
    // segments between tabs keep backend run metrics. 0 measures tabs as
    // whatever glyph the backend gives them (passthrough).
    float tab_advance;
    // Known start of the hard line after the one being built (or the text
    // length for the final line). When nonzero, the truncated-tail skip
    // jumps straight to it instead of scanning the tail for the newline -
    // the editor's line index already knows every line end, and the scan
    // is O(line length) per frame on a pathological single-line document.
    // 0 keeps the scan (a line's next start is never offset 0).
    size_t known_line_next;
    // Frame-local wrap row-count memo (editor wrap only); NULL disables
    // memoization.
    WLX_Wrap_Row_Memo *row_memo;
    // Retained line-geometry store (editor only); NULL disables retention.
    // Callers that set it must build at the store's environment (style,
    // tab advance, wrap band width) and, per record, supply the true
    // known_line_next - the store keys entries on that span.
    WLX_Text_Geom_Store *geom;
    // No-wrap windowed origin (editor only): the view in content space,
    // [view_x, view_x + view_w]. A positive view_w lets a retained line
    // re-enter at a measure origin near the view when the unit budget
    // cannot reach it from the line start; 0 keeps every origin at the
    // line start and reproduces the unwindowed records exactly.
    float view_x;
    float view_w;
} WLX_Text_Build_Inputs;

typedef struct WLX_Text_Build_Cursor {
    size_t text_unit_count;
    // Units consumed by the current hard line across its wrapped rows;
    // maintained (and reset at hard line starts) only when the per-line
    // budget mode is active (wrap + truncate_continue).
    size_t line_unit_count;
    // First '\t' at/after the current hard line's start; SIZE_MAX = none
    // seen yet. Discovered as the scan walks the bytes and reset at hard
    // line starts, so the growing-prefix measures answer their
    // contains-a-tab precheck without rescanning [start, end) per call.
    // A stale value from a mid-line entry only costs the fast path, never
    // correctness (the segment walk measures a tab-free range exactly).
    size_t line_first_tab;
} WLX_Text_Build_Cursor;

typedef struct {
    WLX_Text_Line_Record line;
    size_t next_offset;
    bool   produced;
    bool   stop_after_line;
    bool   append_trailing_empty_line;
} WLX_Text_Build_Step;

typedef struct WLX_Text_Line_Array_Result {
    size_t text_length;
    size_t line_count;
    float line_h;
} WLX_Text_Line_Array_Result;

// Next tab stop after x.
static inline float wlx_tab_stop_next(float x, float tab_advance) {
    return (floorf(x / tab_advance) + 1.0f) * tab_advance;
}

// A valid split point must not fall inside a valid UTF-8 multibyte sequence.
// Invalid bytes remain one-byte fallback units for layout progress.
static inline bool wlx_text_utf8_boundary(const char *text, size_t length, size_t off) {
    if (text == NULL) return off == 0;
    if (off > length) return false;
    if (off == 0 || off == length) return true;
    if (!wlx_text_utf8_is_continuation((unsigned char)text[off])) return true;

    for (size_t back = 1; back <= 3 && back <= off; back++) {
        size_t lead = off - back;
        size_t seq_len = 0;
        if (wlx_text_utf8_sequence_at(text, length, lead, &seq_len) && lead + seq_len > off) return false;
    }

    return true;
}

static inline bool wlx_text_range_is_utf8_safe(const char *text, size_t length, size_t start, size_t end) {
    if (start > end || end > length) return false;
    return wlx_text_utf8_boundary(text, length, start) && wlx_text_utf8_boundary(text, length, end);
}

static inline bool wlx_measure_text_range(WLX_Context *ctx, const char *text, size_t length, size_t start, size_t end,
    WLX_Text_Style style, float *out_w, float *out_h) {
    if (out_w) *out_w = 0.0f;
    if (out_h) *out_h = 0.0f;
    if (ctx == NULL) return false;
    if (text == NULL) {
        text = "";
        length = 0;
        start = 0;
        end = 0;
    }
    if (!wlx_text_range_is_utf8_safe(text, length, start, end)) return false;

    wlx_span_measure_text(ctx, text + start, end - start, style, out_w, out_h);
    return true;
}

static inline bool wlx_draw_text_range(WLX_Context *ctx, const char *text, size_t length, size_t start, size_t end,
    float x, float y, WLX_Text_Style style) {
    if (ctx == NULL) return false;
    if (text == NULL) {
        text = "";
        length = 0;
        start = 0;
        end = 0;
    }
    if (!wlx_text_range_is_utf8_safe(text, length, start, end)) return false;

    size_t range_len = end - start;
    if (range_len == 0) return true;

    wlx_draw_text_span(ctx, text + start, range_len, x, y, style);
    return true;
}

static inline size_t wlx_text_normalize_cursor_offset(const char *text, size_t length, size_t cursor_offset) {
    if (text == NULL) return 0;
    if (cursor_offset > length) cursor_offset = length;
    while (cursor_offset > 0 && !wlx_text_utf8_boundary(text, length, cursor_offset)) {
        size_t prev = wlx_utf8_prev(text, cursor_offset);
        if (prev >= cursor_offset) break;
        cursor_offset = prev;
    }
    if (cursor_offset > length) cursor_offset = length;
    return cursor_offset;
}

static inline bool wlx_text_cursor_is_on_line(const WLX_Text_Line_Record *line, size_t cursor_offset) {
    if (line == NULL) return false;
    if (cursor_offset < line->source_start) return false;
    if (line->ended_by_newline) return cursor_offset < line->separator_end;
    return cursor_offset <= line->visible_end;
}

static inline bool wlx_text_newline_at(const char *text, size_t length, size_t off, size_t *out_separator_end) {
    if (text == NULL || off >= length) return false;
    if (text[off] == '\n') {
        if (out_separator_end) *out_separator_end = off + 1;
        return true;
    }
    if (text[off] == '\r') {
        size_t separator_end = (off + 1 < length && text[off + 1] == '\n') ? off + 2 : off + 1;
        if (out_separator_end) *out_separator_end = separator_end;
        return true;
    }
    return false;
}

// True when a line-ending separator starts at off; the separator-end
// out-param is discarded. The predicate form of wlx_text_newline_at for
// the many walks that only need the boundary test.
static inline bool wlx_text_at_line_break(const char *text, size_t length, size_t off) {
    size_t sep_probe = 0;
    return wlx_text_newline_at(text, length, off, &sep_probe);
}

// The newline separator ending immediately at next_start, read backward:
// fills out_sep_start with the separator's first byte and returns true
// when one ends exactly there (CRLF two bytes, LF or lone CR one; a CR
// directly before an LF is the pair's start, so nothing ends between
// them). False at offset 0 or when no separator ends at next_start.
// This is the one backward reading of the grammar wlx_text_newline_at
// owns forward; keep the two in lockstep.
static inline bool wlx_text_separator_before(const char *text, size_t length,
    size_t next_start, size_t *out_sep_start)
{
    if (text == NULL || next_start == 0 || next_start > length) return false;
    if (next_start >= 2 && text[next_start - 2] == '\r' && text[next_start - 1] == '\n') {
        if (out_sep_start) *out_sep_start = next_start - 2;
        return true;
    }
    char prev = text[next_start - 1];
    if (prev == '\n') {
        if (out_sep_start) *out_sep_start = next_start - 1;
        return true;
    }
    if (prev == '\r' && (next_start >= length || text[next_start] != '\n')) {
        if (out_sep_start) *out_sep_start = next_start - 1;
        return true;
    }
    return false;
}

// True when off is a hard line start: offset 0, or the end of a newline
// separator (the \n inside a \r\n pair is not a line start). Hard line
// starts are always UTF-8 boundaries because separators are ASCII.
static inline bool wlx_text_hard_line_start_at(const char *text, size_t length, size_t off) {
    if (off == 0) return true;
    if (text == NULL || off > length) return false;
    return wlx_text_separator_before(text, length, off, NULL);
}

static inline void wlx_text_line_record_from_range(WLX_Context *ctx, WLX_Text_Line_Record *line, const char *text,
    size_t length, WLX_Text_Line_Record_Opt opt) {
    assert(line != NULL);
    wlx_zero_struct(*line);
    line->source_start = opt.source_start;
    line->separator_start = opt.separator_start;
    line->separator_end = opt.separator_end;
    line->line_h = opt.line_h;
    line->ended_by_newline = opt.ended_by_newline;

    line->visible_start = opt.visible_start;
    line->visible_end = opt.visible_end;

    line->source_end = opt.ended_by_newline ? opt.separator_end : line->visible_end;
    line->cursor_start = line->source_start;
    line->cursor_end = line->visible_end;
    line->empty_visual = (line->visible_start == line->visible_end);

    if (!line->empty_visual) {
        if (opt.has_measurement) {
            line->measured_w = opt.measured_w;
            line->measured_h = opt.measured_h;
        } else {
            bool measured = wlx_measure_text_range(ctx, text, length, line->visible_start, line->visible_end, opt.style,
                &line->measured_w, &line->measured_h);
            if (!measured) {
                line->measured_w = 0.0f;
                line->measured_h = opt.line_h;
            }
        }
    } else {
        line->measured_w = 0.0f;
        line->measured_h = opt.line_h;
    }

    if (line->measured_h <= 0.0f) line->measured_h = opt.line_h;
    line->advance_w = line->measured_w;
}

// The measurement environment threaded through the prefix, advances, and
// retained-geometry walks: one build's context, slice, style, tab advance,
// and uniform line height, passed by const pointer instead of six loose
// scalars. line_h is the record-height substitute on walks that do not
// measure heights (the advances paths); callers with no such walk pass 0.
typedef struct WLX_Text_Measure_Args {
    WLX_Context *ctx;
    const char *text;
    size_t length;
    WLX_Text_Style style;
    float tab_advance;
    float line_h;
} WLX_Text_Measure_Args;

// Canonical contains-a-tab precheck over a first-tab fact: first_tab is
// the absolute offset of the first known '\t' at or after the measured
// range's start, SIZE_MAX meaning none seen. SIZE_MAX compares false
// against any real end, so the sentinel needs no separate check; an
// overestimating first_tab is safe (it only costs the tab-free fast
// path of the segment walk below).
static inline bool wlx_text_pen_has_tab(size_t first_tab, size_t next) {
    return first_tab < next;
}

// Iterate the tab-delimited segments of [*io_pos, end): each call yields
// one [start, end) segment (possibly empty between adjacent tabs) plus
// whether a '\t' delimiter follows it, returning false once the range is
// exhausted. The shared shape of tab-aware measurement and segmented
// drawing: act on the non-empty segment, then apply the per-tab action
// when tab_after is set.
typedef struct WLX_Text_Tab_Seg {
    size_t start;
    size_t end;
    bool tab_after;
} WLX_Text_Tab_Seg;

static inline bool wlx_text_tab_seg_next(const char *text, size_t end,
    size_t *io_pos, WLX_Text_Tab_Seg *out)
{
    size_t p = *io_pos;
    if (p > end) return false;
    out->start = p;
    while (p < end && text[p] != '\t') p++;
    out->end = p;
    out->tab_after = p < end;
    *io_pos = p + 1;
    return true;
}

// Measure the prefix [start, end) with optional tab expansion, given
// the caller's contains-a-tab precheck (wlx_text_pen_has_tab over a
// first-tab fact). Fills the prefix's cumulative width and the max
// segment height.
//
// With tabs expanded, the pen walks tab-delimited segments:
//
//   [ segment ]<tab>[ segment ]<tab>[ segment...
//   x += measure --^ x = next   --^
//        (one backend  tab stop
//        run each)     multiple
//
// A non-positive tab_advance or a tab-free precheck measures the whole
// prefix in one backend call, so run metrics (kerning, shaping) are
// exact on the existing paths. A has_tab overestimate is safe: the
// segment walk measures a tab-free range in one call too, it just
// walks the bytes to find that out. See docs/EDITOR_MODEL.md
// section 8 for the tab-stop policy.
static bool wlx_text_measure_prefix_tabs_known(const WLX_Text_Measure_Args *args,
    size_t start, size_t end, bool has_tab, float *out_w, float *out_h)
{
    WLX_Context *ctx = args->ctx;
    const char *text = args->text;
    size_t length = args->length;
    WLX_Text_Style style = args->style;
    float tab_advance = args->tab_advance;
    if (!has_tab || tab_advance <= 0.0f || text == NULL)
        return wlx_measure_text_range(ctx, text, length, start, end, style, out_w, out_h);

    float x = 0.0f;
    float h = 0.0f;
    size_t pos = start;
    WLX_Text_Tab_Seg seg;
    while (wlx_text_tab_seg_next(text, end, &pos, &seg)) {
        if (seg.end > seg.start) {
            float seg_w = 0.0f, seg_h = 0.0f;
            if (!wlx_measure_text_range(ctx, text, length, seg.start, seg.end, style, &seg_w, &seg_h)) return false;
            x += seg_w;
            if (seg_h > h) h = seg_h;
        }
        if (seg.tab_after) x = wlx_tab_stop_next(x, tab_advance);
    }
    if (h <= 0.0f) {
        float empty_w = 0.0f;
        wlx_measure_text_range(ctx, text, length, start, start, style, &empty_w, &h);
    }
    if (out_w) *out_w = x;
    if (out_h) *out_h = h;
    return true;
}

// Scanning wrapper for callers without a hoisted tab fact (selection draw,
// caret x, hit tests): one [start, end) scan answers the precheck. The
// build loop calls the _known entry instead - its growing prefixes share
// their line start, and rescanning per call is O(n^2) bytes per line.
static bool wlx_text_measure_prefix_tabs(WLX_Context *ctx, const char *text, size_t length,
    size_t start, size_t end, WLX_Text_Style style, float tab_advance, float *out_w, float *out_h)
{
    bool has_tab = false;
    if (tab_advance > 0.0f && text != NULL) {
        for (size_t i = start; i < end; i++) {
            if (text[i] == '\t') { has_tab = true; break; }
        }
    }
    WLX_Text_Measure_Args args = { ctx, text, length, style, tab_advance, 0.0f };
    return wlx_text_measure_prefix_tabs_known(&args, start, end, has_tab, out_w, out_h);
}

// One fit decision of the line scan, shared by every measuring walk and
// every replay: a record's first unit is always accepted (records never
// go empty), an accepted non-fitting unit ends the record, and a later
// non-fitting unit is rejected to start the next row/record. Replay
// correctness depends on every site sharing this exact triple.
typedef enum {
    WLX_TEXT_FIT_ACCEPT,      // unit joins the record; scan continues
    WLX_TEXT_FIT_ACCEPT_END,  // unit joins the record; record ends
    WLX_TEXT_FIT_REJECT       // unit belongs to the next record/row
} WLX_Text_Fit;

static inline WLX_Text_Fit wlx_text_fit_step(float advance_w, float fit_w,
    size_t record_units)
{
    if (advance_w <= fit_w) return WLX_TEXT_FIT_ACCEPT;
    return record_units == 0 ? WLX_TEXT_FIT_ACCEPT_END : WLX_TEXT_FIT_REJECT;
}

// Fill cumulative unit advances for the next stretch of one record
// through the optional measure_text_advances callback: out_ends gets
// absolute unit-end offsets, out_adv the cumulative advance at each
// end in the record's frame (base_x = the advance already accumulated
// at pos). Returns the units produced; 0 means the callback is absent,
// the walk found no unit, or the backend could not fill - the caller
// picks its own fail-over.
//
// Three passes over the batch:
//   1. boundary walk - collect unit ends from pos, stopping at a
//      separator, the text end, max_units, or WLX_TEXT_ADVANCES_CHUNK;
//      updates the first-tab fact io_first_tab (absolute, SIZE_MAX =
//      none) on discovery.
//   2. backend dispatch - each tab-free stretch is one callback
//      request, and with tab expansion on each '\t' advances the
//      running x to the next tab stop exactly like the per-unit pen
//      walk (expansion off keeps tabs inside the run as backend
//      glyphs); a partial fill truncates the batch.
//   3. monotonic clamp - advances are made non-decreasing.
static size_t wlx_text_measure_advances_batch(const WLX_Text_Measure_Args *args,
    size_t pos, float base_x, size_t max_units, size_t *out_ends, float *out_adv,
    size_t *io_first_tab)
{
    WLX_Context *ctx = args->ctx;
    const char *text = args->text;
    size_t length = args->length;
    WLX_Text_Style style = wlx_style_at_boundary(ctx, args->style);
    float tab_advance = args->tab_advance;
    if (ctx->backend.measure_text_advances == NULL || text == NULL) return 0;

    size_t cap = max_units < (size_t)WLX_TEXT_ADVANCES_CHUNK
        ? max_units : (size_t)WLX_TEXT_ADVANCES_CHUNK;
    size_t n = 0;
    size_t p = pos;
    while (n < cap) {
        if (p >= length || wlx_text_at_line_break(text, length, p)) break;
        if (*io_first_tab == SIZE_MAX && text[p] == '\t') *io_first_tab = p;
        size_t next = wlx_text_unit_next(text, length, p);
        out_ends[n++] = next;
        p = next;
    }
    if (n == 0) return 0;

    float x = base_x;
    size_t i = 0;
    while (i < n) {
        size_t unit_start = i == 0 ? pos : out_ends[i - 1];
        if (tab_advance > 0.0f && text[unit_start] == '\t') {
            x = wlx_tab_stop_next(x, tab_advance);
            out_adv[i] = x;
            i++;
            continue;
        }
        size_t j = i + 1;
        while (j < n && !(tab_advance > 0.0f && text[out_ends[j - 1]] == '\t')) j++;
        size_t rel_ends[WLX_TEXT_ADVANCES_CHUNK];
        size_t run_units = j - i;
        for (size_t k = 0; k < run_units; k++) rel_ends[k] = out_ends[i + k] - unit_start;
        size_t filled = ctx->backend.measure_text_advances(text + unit_start,
            rel_ends[run_units - 1], style, rel_ends, run_units, out_adv + i, ctx->backend.user);
        if (filled > run_units) filled = run_units;
        for (size_t k = 0; k < filled; k++) out_adv[i + k] += x;
        if (filled < run_units) {
            n = i + filled;
            break;
        }
        x = out_adv[j - 1];
        i = j;
    }

    float prev = base_x;
    for (size_t k = 0; k < n; k++) {
        if (out_adv[k] < prev) out_adv[k] = prev;
        prev = out_adv[k];
    }
    return n;
}

// The advance provider's one fetch entry: the next measured units from
// pos, as absolute unit ends, cumulative advances in the caller's frame
// (base_x already accumulated at pos), and per-unit heights. Two arms:
//   - batched, when the caller allows it and the backend implements the
//     advances callback: one chunked fill, heights substitute the
//     uniform line_h (advances report x geometry only - the documented
//     heights split);
//   - per-unit, otherwise: exactly one unit, measured as a whole prefix
//     from measure_base (record start, entry origin, or row start - the
//     caller's frame), height as the backend measured it.
// io_first_tab (absolute, SIZE_MAX = none) is updated on discovery and
// feeds the per-unit tab precheck. Returns the units produced; 0 means
// the batch cannot progress, and the CALLER picks the fail-over policy -
// the three consumers deliberately differ (the build scan falls to
// per-unit for the rest of its record, entry extension retries per call,
// the wrap store build aborts to the measuring build) - so no fail-over
// lives in here.
static size_t wlx_text_unit_fetch(const WLX_Text_Measure_Args *args,
    size_t pos, size_t measure_base, float base_x, size_t budget_left,
    bool batch_allowed, size_t *ends, float *advs, float *heights,
    size_t *io_first_tab)
{
    const char *text = args->text;
    size_t length = args->length;
    if (batch_allowed && args->ctx != NULL
        && args->ctx->backend.measure_text_advances != NULL) {
        size_t n = wlx_text_measure_advances_batch(args, pos, base_x,
            budget_left, ends, advs, io_first_tab);
        for (size_t i = 0; i < n; i++) heights[i] = args->line_h;
        return n;
    }
    size_t next = wlx_text_unit_next(text, length, pos);
    if (*io_first_tab == SIZE_MAX && text[pos] == '\t') *io_first_tab = pos;
    float w = 0.0f, h = args->line_h;
    bool measured = wlx_text_measure_prefix_tabs_known(args, measure_base, next,
        wlx_text_pen_has_tab(*io_first_tab, next), &w, &h);
    if (!measured) { w = 0.0f; h = args->line_h; }
    ends[0] = next;
    advs[0] = w;
    heights[0] = h;
    return 1;
}

// ============================================================================
// Retained editor line geometry
//
// The store keeps, per hard line, the growing-prefix measurements the line
// build performs, so frames whose lines did not change replay records from
// floats instead of re-asking the backend. Misses and reach extensions
// measure through the same walk the from-scratch build uses and land in
// the store; every consumer keeps the from-scratch path as its miss
// fallback, so a dropped or evicted entry costs traffic, never geometry.
//
// In the pipeline's vocabulary this is the shaped-line cache: per hard
// line, width-independent cumulative unit advances (plus a row table
// under wrap), LRU-bounded, invalidated by measurement environment or
// edit span.
// ============================================================================

// ----------------------------------------------------------------------------
// The pure store tier: entry lifecycle, lookups, and field reads.
// Nothing in this tier touches the context or the backend - measurement
// lives in the driver tier below it, view and caret anchoring in the
// origin policy after that, and the entry-level consumer queries close
// the section.
// ----------------------------------------------------------------------------

// The entry stores its first-tab fact line-relative and uint32-packed;
// these convert to and from the absolute-offset form (SIZE_MAX = none)
// the measure walks and wlx_text_pen_has_tab consume, keeping the two
// representations from being re-derived at every boundary.
static inline size_t wlx_text_geom_first_tab_abs(const WLX_Text_Geom_Entry *e) {
    return e->first_tab_rel == UINT32_MAX
        ? SIZE_MAX : e->line_start + (size_t)e->first_tab_rel;
}

static inline void wlx_text_geom_first_tab_note(WLX_Text_Geom_Entry *e,
    size_t first_tab_abs)
{
    if (e->first_tab_rel == UINT32_MAX && first_tab_abs != SIZE_MAX)
        e->first_tab_rel = (uint32_t)(first_tab_abs - e->line_start);
}

// Absolute byte offset of the entry's measure origin (the line start
// unless the origin re-entered deeper).
static inline size_t wlx_text_geom_origin_abs(const WLX_Text_Geom_Entry *e) {
    return e->line_start + (size_t)e->origin_rel;
}

// Release one entry back to the free pool (its allocations stay for
// reuse): the consumer-side lifecycle exit for a replay that could not
// answer.
static inline void wlx_text_geom_drop(WLX_Text_Geom_Entry *e) {
    e->used = false;
}

// Invalidate everything but keep every allocation for reuse: clearing is
// the safe default on any doubt and must stay cheap enough to run freely.
static void wlx_text_geom_clear(WLX_Text_Geom_Store *s) {
    for (size_t i = 0; i < s->count; i++) s->entries[i].used = false;
}

// Free every allocation the store owns: the terminal lifecycle exit,
// called from context destruction.
static void wlx_text_geom_store_free(WLX_Text_Geom_Store *s) {
    for (size_t i = 0; i < s->count; i++) {
        WLX_Text_Geom_Entry *e = &s->entries[i];
        wlx_free(e->unit_ends);
        wlx_free(e->advances);
        wlx_free(e->heights);
        wlx_free(e->row_units);
    }
    wlx_free(s->entries);
}

static bool wlx_text_geom_env_equal(const WLX_Text_Geom_Env *a,
    const WLX_Text_Geom_Env *b)
{
    return a->font == b->font && a->font_size == b->font_size
        && a->spacing == b->spacing && a->tab_advance == b->tab_advance
        && a->band_w == b->band_w && a->line_h == b->line_h
        && a->wrap == b->wrap
        && a->transform_generation == b->transform_generation;
}

// Compare the measurement environment and clear the store when any part of
// it changed; also raise the sizing target to the current viewport need.
static WLX_EXTENSION_USED void wlx_text_geom_env_check(WLX_Text_Geom_Store *s,
    const WLX_Text_Geom_Env *env, size_t want_cap)
{
    if (!s->env_seen || !wlx_text_geom_env_equal(&s->env, env)) {
        wlx_text_geom_clear(s);
        s->env = *env;
        s->env_seen = true;
        s->avg_advance = 0.0f;
    }
    if (want_cap > s->want_cap) s->want_cap = want_cap;
}

// Invalidate the store precisely for one widget-applied edit: the
// pre-edit byte range [start, old_end) became [start, new_end), so
// entries touching the edit drop while entries past it keep their
// measurements and only shift their byte keys by the delta. This is
// what lets a typing frame re-measure just the edited line instead of
// clearing the store.
//
//              start             old_end
//   -------------|-----------------|--------------->  document bytes
//    [A]                                   keep: ends before start
//            [ B ]                         drop: ends exactly at start
//           [    C    ]                    drop: touches the range
//                                  [ D ]   drop: starts exactly at old_end
//                                      [ E ] shift keys by new_end - old_end
//
// The two boundary drops are deliberate: an append at B's end can
// extend its line and a deletion can eat its separator; the deletion
// may likewise have eaten the separator that made D a line start.
static WLX_EXTENSION_USED void wlx_text_geom_edit_shift(WLX_Text_Geom_Store *s,
    size_t start, size_t old_end, size_t new_end)
{
    for (size_t i = 0; i < s->count; i++) {
        WLX_Text_Geom_Entry *e = &s->entries[i];
        if (!e->used) continue;
        if (e->line_start > old_end) {
            e->line_start = e->line_start - old_end + new_end;
            e->line_next = e->line_next - old_end + new_end;
        } else if (e->line_next >= start) {
            e->used = false;
        }
    }
}

// Find the live entry keyed exactly (line_start, line_next) and
// refresh its LRU stamp; NULL on a miss (the caller measures).
static WLX_Text_Geom_Entry *wlx_text_geom_find(WLX_Text_Geom_Store *s,
    size_t line_start, size_t line_next)
{
    for (size_t i = 0; i < s->count; i++) {
        WLX_Text_Geom_Entry *e = &s->entries[i];
        if (e->used && e->line_start == line_start && e->line_next == line_next) {
            e->lru = ++s->lru_clock;
            return e;
        }
    }
    return NULL;
}

// Index of the unit whose end is exactly rel (unit ends are strictly
// increasing), or SIZE_MAX when rel is not a measured unit boundary.
static size_t wlx_text_geom_unit_index(const WLX_Text_Geom_Entry *e, size_t rel) {
    size_t lo = 0, hi = e->units;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        size_t v = e->unit_ends[mid];
        if (v == rel) return mid;
        if (v < rel) lo = mid + 1; else hi = mid;
    }
    return SIZE_MAX;
}

// Find the live entry whose line span contains offset, validating against
// the caller-known next line start (wrapped rows re-enter mid-line).
// Find the live entry of the hard line containing offset, keyed by the
// line's end (line_next), and refresh its LRU stamp; NULL on a miss.
// Wrapped rows and mid-line records locate their line's entry this way
// - their own start is not the entry's key.
static WLX_Text_Geom_Entry *wlx_text_geom_find_containing(
    WLX_Text_Geom_Store *s, size_t offset, size_t line_next)
{
    for (size_t i = 0; i < s->count; i++) {
        WLX_Text_Geom_Entry *e = &s->entries[i];
        if (e->used && e->line_next == line_next
            && e->line_start <= offset && offset < e->line_next) {
            e->lru = ++s->lru_clock;
            return e;
        }
    }
    return NULL;
}

// Stored cumulative advance at the unit ending exactly at line-relative
// rel; false when rel is not a measured unit boundary.
static inline bool wlx_text_geom_advance_at(const WLX_Text_Geom_Entry *e,
    size_t rel, float *out_x)
{
    size_t u = wlx_text_geom_unit_index(e, rel);
    if (u == SIZE_MAX) return false;
    *out_x = e->advances[u];
    return true;
}

// The entry authoritative for a record spanning [start, end) of the
// line ending at line_next: a containing entry whose measured span
// [origin_rel, scan_rel) covers the record's bytes. NULL when no entry
// answers (the caller measures).
static WLX_EXTENSION_USED WLX_Text_Geom_Entry *wlx_text_geom_covering(
    WLX_Text_Geom_Store *s, size_t start, size_t end, size_t line_next)
{
    if (line_next <= start) return NULL;
    WLX_Text_Geom_Entry *e = wlx_text_geom_find_containing(s, start, line_next);
    if (e == NULL) return NULL;
    if (e->units == 0
        || start - e->line_start < (size_t)e->origin_rel
        || end - e->line_start > (size_t)e->scan_rel) return NULL;
    return e;
}

// Row count of a complete wrapped entry, 0 when the store cannot answer
// (no entry, an incomplete one, or an empty line's rows == 0 record -
// the streaming count owns those).
static WLX_EXTENSION_USED size_t wlx_text_geom_rows_of(WLX_Text_Geom_Store *s,
    size_t line_start, size_t line_next)
{
    WLX_Text_Geom_Entry *e = wlx_text_geom_find(s, line_start, line_next);
    return (e != NULL && e->complete && e->rows > 0) ? e->rows : 0;
}

// Nearest caret offset at a content-space x within [visible_start,
// visible_end) of a record, walked over the entry's stored advances by
// the midpoint rule. A record starting at the entry's measure origin
// walks from the first stored unit; a wrapped row starting mid-line
// locates its start on the previous row's last unit end. Returns false
// on any boundary mismatch (the caller's measuring walk takes over).
static WLX_EXTENSION_USED bool wlx_text_geom_offset_at_x(const WLX_Text_Geom_Entry *e,
    size_t visible_start, size_t visible_end, float content_x, size_t *out_off)
{
    if (e->units == 0) return false;
    size_t start_rel = visible_start - e->line_start;
    size_t end_rel = visible_end - e->line_start;
    size_t lo = 0;
    if (start_rel > 0 && start_rel != (size_t)e->origin_rel) {
        size_t su = wlx_text_geom_unit_index(e, start_rel);
        if (su == SIZE_MAX) return false;
        lo = su + 1;
    }
    size_t hu = wlx_text_geom_unit_index(e, end_rel);
    if (hu == SIZE_MAX || hu < lo) return false;
    size_t off = visible_start;
    float prev_w = 0.0f;
    for (size_t u = lo; u <= hu; u++) {
        size_t next = e->line_start + e->unit_ends[u];
        float w = e->advances[u];
        if (content_x < (prev_w + w) * 0.5f) { *out_off = off; return true; }
        prev_w = w;
        off = next;
    }
    *out_off = off;
    return true;
}

// Find-or-create the entry for one hard line, reusing a free slot's
// arrays, growing toward the sizing target, or evicting the least
// recently used entry. A found entry returns with its measurements
// intact; a created or evicted slot returns reset to an empty entry at
// (line_start, line_next). Returns NULL only when the store is unsized
// or cannot grow.
static WLX_Text_Geom_Entry *wlx_text_geom_acquire(WLX_Text_Geom_Store *s,
    size_t line_start, size_t line_next)
{
    WLX_Text_Geom_Entry *e = wlx_text_geom_find(s, line_start, line_next);
    if (e != NULL) return e;

    WLX_Text_Geom_Entry *slot = NULL;
    for (size_t i = 0; i < s->count; i++) {
        if (!s->entries[i].used) { slot = &s->entries[i]; break; }
    }
    if (slot == NULL && s->count < s->want_cap) {
        WLX_HARD_ASSERT(s->want_cap <= SIZE_MAX / sizeof(WLX_Text_Geom_Entry),
            "geom store size overflow");
        WLX_Text_Geom_Entry *grown = (WLX_Text_Geom_Entry *)wlx_realloc(
            s->entries, s->want_cap * sizeof(WLX_Text_Geom_Entry));
        if (grown != NULL) {
            memset(grown + s->count, 0,
                (s->want_cap - s->count) * sizeof(WLX_Text_Geom_Entry));
            s->entries = grown;
            slot = &grown[s->count];
            s->count = s->want_cap;
        }
    }
    if (slot == NULL && s->count > 0) {
        slot = &s->entries[0];
        for (size_t i = 1; i < s->count; i++) {
            if (s->entries[i].lru < slot->lru) slot = &s->entries[i];
        }
    }
    if (slot == NULL) return NULL;

    slot->used = true;
    slot->line_start = line_start;
    slot->line_next = line_next;
    slot->units = 0;
    slot->rows = 0;
    slot->origin_rel = 0;
    slot->origin_x = 0.0f;
    slot->scan_rel = 0;
    slot->first_tab_rel = UINT32_MAX;
    slot->complete = false;
    slot->lru = ++s->lru_clock;
    return slot;
}

// Append one measured unit to an entry, growing the parallel arrays
// geometrically. Returns false on allocation failure.
static bool wlx_text_geom_push_unit(WLX_Text_Geom_Entry *e,
    uint32_t end_rel, float advance, float height)
{
    if (e->units == e->unit_cap) {
        size_t new_cap = e->unit_cap == 0 ? WLX_GROW_INIT_CAP : e->unit_cap * 2;
        WLX_HARD_ASSERT(new_cap <= SIZE_MAX / sizeof(uint32_t),
            "geom entry size overflow");
        uint32_t *ends = (uint32_t *)wlx_realloc(e->unit_ends, new_cap * sizeof(uint32_t));
        if (ends == NULL) return false;
        e->unit_ends = ends;
        float *adv = (float *)wlx_realloc(e->advances, new_cap * sizeof(float));
        if (adv == NULL) return false;
        e->advances = adv;
        float *hs = (float *)wlx_realloc(e->heights, new_cap * sizeof(float));
        if (hs == NULL) return false;
        e->heights = hs;
        e->unit_cap = new_cap;
    }
    e->unit_ends[e->units] = end_rel;
    e->advances[e->units] = advance;
    e->heights[e->units] = height;
    e->units++;
    return true;
}

// ----------------------------------------------------------------------------
// The measurement driver tier: the only geom functions that ask the
// backend, always through the unit fetch over WLX_Text_Measure_Args -
// never through view geometry (where to measure from is the origin
// policy's decision, below).
// ----------------------------------------------------------------------------

// Extend a no-wrap entry by its next unmeasured units through the unit
// fetch: one chunked advances batch when the backend implements the
// callback, else one whole-prefix measure from the entry's origin (the
// line start unless the origin re-entered deeper). A batch that cannot
// progress retries as a single per-unit step now and the batch again on
// the next call. Heights land as fetched: measured on the per-unit
// path, the uniform line height on the advances path. Returns false
// when nothing more can be measured (line end, unit budget, or
// allocation failure).
static bool wlx_text_geom_extend(const WLX_Text_Measure_Args *args,
    WLX_Text_Geom_Entry *e)
{
    const char *text = args->text;
    size_t length = args->length;
    if (e->complete || e->units >= (size_t)WLX_EDITOR_MAX_LINE_UNITS) return false;
    size_t line_start = e->line_start;
    size_t scan_pos = line_start + e->scan_rel;
    if (scan_pos >= length || wlx_text_at_line_break(text, length, scan_pos)) {
        e->complete = true;
        return false;
    }
    size_t ends[WLX_TEXT_ADVANCES_CHUNK];
    float advs[WLX_TEXT_ADVANCES_CHUNK];
    float heights[WLX_TEXT_ADVANCES_CHUNK];
    size_t first_tab = wlx_text_geom_first_tab_abs(e);
    float base_x = e->units > 0 ? e->advances[e->units - 1] : 0.0f;
    size_t n = wlx_text_unit_fetch(args, scan_pos, line_start + e->origin_rel,
        base_x, (size_t)WLX_EDITOR_MAX_LINE_UNITS - e->units, true,
        ends, advs, heights, &first_tab);
    if (n == 0) {
        n = wlx_text_unit_fetch(args, scan_pos, line_start + e->origin_rel,
            base_x, 1, false, ends, advs, heights, &first_tab);
        if (n == 0) return false;
    }
    wlx_text_geom_first_tab_note(e, first_tab);
    for (size_t i = 0; i < n; i++) {
        if (!wlx_text_geom_push_unit(e, (uint32_t)(ends[i] - line_start),
                advs[i], heights[i]))
            return false;
        // Checkpoint per pushed unit: a mid-batch allocation failure must
        // leave scan_rel at the stored prefix, or the next extension would
        // append duplicate unit ends.
        e->scan_rel = (uint32_t)(ends[i] - line_start);
    }
    return true;
}

// Extend a no-wrap entry until it can answer a fit decision at width w:
// either a stored advance exceeds w, or the line's measurable text ends.
static void wlx_text_geom_ensure_width(const WLX_Text_Measure_Args *args,
    WLX_Text_Geom_Entry *e, float w)
{
    while (e->units == 0 || e->advances[e->units - 1] <= w) {
        if (!wlx_text_geom_extend(args, e))
            break;
    }
}

// Extend a no-wrap entry until its units cover the byte offset (relative
// to the line start) or nothing more can be measured.
static void wlx_text_geom_ensure_offset(const WLX_Text_Measure_Args *args,
    WLX_Text_Geom_Entry *e, size_t offset_rel)
{
    while (e->units == 0 || (size_t)e->unit_ends[e->units - 1] < offset_rel) {
        if (!wlx_text_geom_extend(args, e))
            break;
    }
}

// ----------------------------------------------------------------------------
// No-wrap windowed origin. A line the unit budget cannot cover from its
// start re-enters at a measure origin near the view: the entry's advances
// are measured from the origin (tab stops restart there, the rule wrapped
// rows already apply at row starts) and consumers add the frozen origin_x
// to place them in content space. origin_x is exact at the line start,
// carried exactly across stitched moves, and an average-advance estimate
// only after a far jump onto unmeasured content; byte offsets are exact
// everywhere. Origins move only when the view leaves what the current
// origin can measure, so a held view never re-anchors.
// ----------------------------------------------------------------------------

// Average unit advance for origin estimates: the entry's own measurements
// first, then the store's sticky last-seen average (surviving entry drops
// and clears), then a glyph-shaped guess off the line height.
static float wlx_text_geom_avg_advance(const WLX_Text_Geom_Store *s,
    const WLX_Text_Geom_Entry *e, float line_h)
{
    if (e != NULL && e->units > 0 && e->advances[e->units - 1] > 0.0f)
        return e->advances[e->units - 1] / (float)e->units;
    if (s != NULL && s->avg_advance > 0.0f) return s->avg_advance;
    return line_h > 0.0f ? line_h * WLX_TEXT_GEOM_FALLBACK_ADVANCE_EM : 1.0f;
}

// Refresh the store's sticky average from an entry with enough units to
// be representative. No-wrap entries only: wrapped advances are
// row-relative and would understate the average.
static void wlx_text_geom_note_avg(WLX_Text_Geom_Store *s,
    const WLX_Text_Geom_Entry *e)
{
    if (s != NULL && e->rows == 0
        && e->units >= (size_t)WLX_TEXT_GEOM_AVG_MIN_UNITS
        && e->advances[e->units - 1] > 0.0f)
        s->avg_advance = e->advances[e->units - 1] / (float)e->units;
}

// Count text units in [from, to), capped: far-jump checks stay bounded and
// estimate walks stay explicit. Returns cap when the walk did not reach to.
static size_t wlx_text_geom_units_between(const char *text, size_t length,
    size_t from, size_t to, size_t cap)
{
    size_t n = 0;
    while (from < to && n < cap) {
        from = wlx_text_unit_next(text, length, from);
        n++;
    }
    return n;
}

// Walk up to n text units forward from off, stopping at the line's
// separator or the text end. *out_steps receives the units consumed.
static size_t wlx_text_geom_units_fwd(const char *text, size_t length,
    size_t off, size_t n, size_t *out_steps)
{
    size_t steps = 0;
    while (steps < n && off < length
        && !wlx_text_at_line_break(text, length, off)) {
        off = wlx_text_unit_next(text, length, off);
        steps++;
    }
    if (out_steps) *out_steps = steps;
    return off;
}

// Walk up to n text units backward from off, flooring at the line start.
// Steps land on UTF-8 lead bytes; malformed sequences may group
// differently than the forward walk, which only shifts an origin
// candidate, never a stored offset. *out_steps receives the units taken.
static size_t wlx_text_geom_units_back(const char *text, size_t off,
    size_t floor_off, size_t n, size_t *out_steps)
{
    size_t steps = 0;
    while (steps < n && off > floor_off) {
        off--;
        while (off > floor_off && ((unsigned char)text[off] & 0xC0) == 0x80) off--;
        steps++;
    }
    if (out_steps) *out_steps = steps;
    return off;
}

// Snap an origin candidate to the boundary just after a space within the
// backscan window, else to the candidate's own UTF-8 lead byte.
static size_t wlx_text_geom_snap_origin(const char *text, size_t line_start,
    size_t candidate)
{
    size_t lo = candidate > (size_t)WLX_EDITOR_ORIGIN_BACKSCAN
        ? candidate - (size_t)WLX_EDITOR_ORIGIN_BACKSCAN : 0;
    if (lo < line_start) lo = line_start;
    for (size_t p = candidate; p > lo; p--) {
        if (text[p - 1] == ' ') return p;
    }
    while (candidate > line_start
        && ((unsigned char)text[candidate] & 0xC0) == 0x80) candidate--;
    return candidate;
}

// Move a no-wrap entry's measure origin to the line-relative offset
// new_rel and set the new origin's absolute x, dropping the stored
// units so the scan restarts at the new origin.
//
// est_x is the caller's fallback x for the new origin (an estimate);
// stitch may be true only when the old origin lies within one unit
// budget of new_rel - the stitch-back arm re-measures that span, and a
// farther origin could not reach it. Everything except the origin
// fields resets: units, rows, and the first-tab fact clear.
//
// The new origin_x is chosen by the most exact rule that applies:
//
//        0        new_rel'  old origin  [measured units]
//        |            |         |        u0   u1   u2
//   x -> +------------+---------+-------|----|----|------------
//   (1)  new_rel == 0:               origin_x = 0            exact
//   (2)  new_rel at a measured unit end (u0/u1/u2):
//            old origin_x + stored advance                   exact
//   (3)  new_rel' behind the old origin, stitch set:
//            re-measure [new_rel', old origin), subtract     exact on
//            from old origin_x                        additive metrics
//   (4)  anything else: est_x, the caller's
//            average-advance estimate                x-space estimate
//
// Byte offsets stay exact on every arm; only x carries the estimate.
// See docs/EDITOR_MODEL.md section 6 and the stitch-precondition
// invariant in docs/TEXT_PIPELINE_MAP.md.
static void wlx_text_geom_set_origin(const WLX_Text_Measure_Args *args,
    WLX_Text_Geom_Entry *e, size_t new_rel, float est_x, bool stitch)
{
    size_t old_rel = e->origin_rel;
    float old_x = e->origin_x;
    float new_x = est_x;
    bool stitch_back = false;
    if (new_rel == 0) {
        new_x = 0.0f;
    } else if (new_rel > old_rel) {
        size_t u = wlx_text_geom_unit_index(e, new_rel);
        if (u != SIZE_MAX) new_x = old_x + e->advances[u];
    } else if (new_rel < old_rel) {
        stitch_back = stitch;
    }

    e->units = 0;
    e->rows = 0;
    e->origin_rel = (uint32_t)new_rel;
    e->origin_x = new_x;
    e->scan_rel = (uint32_t)new_rel;
    e->first_tab_rel = UINT32_MAX;
    e->complete = false;

    if (stitch_back) {
        // Measure forward to the old origin and derive the new origin_x
        // from it; the units measured here are the window's own content,
        // not throwaway work. The budget can end the walk first (a very
        // deep retreat) - the estimate then stands.
        wlx_text_geom_ensure_offset(args, e, old_rel);
        size_t u = wlx_text_geom_unit_index(e, old_rel);
        if (u != SIZE_MAX) e->origin_x = old_x - e->advances[u];
    }
    if (e->origin_x < 0.0f) e->origin_x = 0.0f;
}

// Make a no-wrap entry's measured coverage serve the current view,
// moving its measure origin only when the view has left what the
// current origin can reach. Runs before every replay; it is the single
// origin authority every consumer resolves the entry through, so
// geometry within a viewport window agrees by construction.
//
//      line start     origin           stored units (coverage)
//   x -> |..............|================================|
//                       |<-- back -->|<==== view ====>|
//                           margin    view_x..view_x+w
//
//   retreat: view_x crossed left of origin_x -> re-anchor half a view
//            before view_x (stitch when the old origin is in reach)
//   advance: the unit budget ran out short of the view's right edge ->
//            re-anchor half a view before view_x, but never right of
//            view_x itself (the non-oscillation rule: an origin the
//            view starts behind would be pulled straight back)
//
// A held view or a small wheel notch stays inside the back margin and
// never re-anchors - steady frames replay without measuring. See
// docs/EDITOR_MODEL.md section 6 and the origin non-oscillation
// invariant in docs/TEXT_PIPELINE_MAP.md.
static void wlx_text_geom_window_linear(const WLX_Text_Build_Inputs *inputs,
    WLX_Text_Geom_Entry *e)
{
    WLX_Text_Measure_Args margs = { inputs->ctx, inputs->text, inputs->length,
        inputs->style, inputs->tab_advance, inputs->line_h };
    const char *text = inputs->text;
    size_t length = inputs->length;
    size_t line_start = e->line_start;
    float view_x = inputs->view_x;
    float view_w = inputs->view_w;

    if (view_w > 0.0f && e->origin_rel > 0 && view_x < e->origin_x) {
        // Retreat: unmeasured content left of the origin entered the view.
        float target_x = view_x - view_w * WLX_TEXT_GEOM_ORIGIN_VIEW_SLACK;
        if (target_x <= 0.0f) {
            wlx_text_geom_set_origin(&margs, e, 0, 0.0f, false);
        } else {
            float ua = wlx_text_geom_avg_advance(inputs->geom, e, inputs->line_h);
            size_t back_n = (size_t)((e->origin_x - target_x) / ua) + 1;
            size_t steps = 0;
            size_t cand = wlx_text_geom_units_back(text,
                line_start + e->origin_rel, line_start, back_n, &steps);
            size_t snapped = wlx_text_geom_snap_origin(text, line_start, cand);
            // Snap distance in bytes ~ units: deliberate inside the x estimate.
            float est = e->origin_x - ua * ((float)steps + (float)(cand - snapped));
            if (snapped <= line_start) {
                wlx_text_geom_set_origin(&margs, e, 0, 0.0f, false);
            } else if (snapped < line_start + e->origin_rel) {
                wlx_text_geom_set_origin(&margs, e, snapped - line_start, est,
                    steps <= (size_t)WLX_EDITOR_MAX_LINE_UNITS);
            }
        }
    }

    wlx_text_geom_ensure_width(&margs, e, inputs->rect.w - e->origin_x);

    if (view_w > 0.0f && !e->complete
        && e->units >= (size_t)WLX_EDITOR_MAX_LINE_UNITS
        && e->origin_x + e->advances[e->units - 1] < view_x + view_w) {
        // Advance: the budget ran out short of the view. Anchor half a
        // view behind the view's left edge - except when that anchor is
        // at or behind the current origin, which means a budget's units
        // do not span the view (a very wide band, a tiny font, or a
        // narrowed WLX_EDITOR_MAX_LINE_UNITS): the back margin is then
        // unaffordable, and the origin aims at the view's left edge so
        // the whole budget lands inside the view.
        float cov_right = e->origin_x + e->advances[e->units - 1];
        float target_x = view_x - view_w * WLX_TEXT_GEOM_ORIGIN_VIEW_SLACK;
        if (target_x <= e->origin_x) target_x = view_x;
        size_t new_rel = e->origin_rel;
        float est_x = target_x;
        if (target_x <= cov_right) {
            // Target inside measured coverage: pick the covering unit
            // boundary (space-preferring backscan), an exact stitch.
            float target_rel = target_x - e->origin_x;
            size_t lo = 0, hi = e->units;
            while (lo < hi) {
                size_t mid = lo + (hi - lo) / 2;
                if (e->advances[mid] < target_rel) lo = mid + 1; else hi = mid;
            }
            size_t pick = lo < e->units ? lo : e->units - 1;
            size_t stop = pick > (size_t)WLX_EDITOR_ORIGIN_BACKSCAN
                ? pick - (size_t)WLX_EDITOR_ORIGIN_BACKSCAN : 0;
            for (size_t k = pick + 1; k > stop; k--) {
                size_t end_abs = line_start + e->unit_ends[k - 1];
                if (end_abs > 0 && text[end_abs - 1] == ' ') { pick = k - 1; break; }
            }
            // Never anchor right of the view's left edge: the retreat rule
            // would pull such an origin back on the next frame, and the two
            // rules would then trade the origin (and a full re-measure) back
            // and forth every frame. The covering boundary can overshoot by
            // one unit; step back to the last one the view starts at or
            // after, and hold the origin when even that overshoots.
            while (pick > 0 && e->origin_x + e->advances[pick] > view_x) pick--;
            if (e->origin_x + e->advances[pick] <= view_x)
                new_rel = e->unit_ends[pick];
        } else {
            // Target beyond coverage: byte-walk the gap and estimate its
            // width from the average advance.
            float ua = wlx_text_geom_avg_advance(inputs->geom, e, inputs->line_h);
            size_t gap_n = (size_t)((target_x - cov_right) / ua) + 1;
            size_t steps = 0;
            size_t cand = wlx_text_geom_units_fwd(text, length,
                line_start + e->scan_rel, gap_n, &steps);
            est_x = cov_right + ua * (float)steps;
            if (steps < gap_n) {
                // The line ended inside the gap: anchor a budget's worth
                // of units before its end so the tail stays measurable.
                size_t back_n = (size_t)WLX_EDITOR_MAX_LINE_UNITS
                    / WLX_TEXT_GEOM_ANCHOR_HEADROOM_DIV;
                size_t back_steps = 0;
                cand = wlx_text_geom_units_back(text, cand,
                    line_start + e->scan_rel, back_n, &back_steps);
                est_x -= ua * (float)back_steps;
            }
            if (est_x > view_x) {
                // Estimated landing right of the view's left edge (a target
                // clamped to that edge, or an average that overshot): the
                // retreat rule would pull it straight back next frame. Step
                // back the estimated overshoot plus a unit of slack for the
                // estimate itself, floored at the measured coverage.
                size_t over = (size_t)((est_x - view_x) / ua) + 2;
                size_t back_steps = 0;
                cand = wlx_text_geom_units_back(text, cand,
                    line_start + e->scan_rel, over, &back_steps);
                est_x -= ua * (float)back_steps;
            }
            size_t snapped = wlx_text_geom_snap_origin(text, line_start, cand);
            // Snap distance in bytes ~ units: deliberate inside the x estimate.
            est_x -= ua * (float)(cand - snapped);
            if (snapped > line_start + e->origin_rel) new_rel = snapped - line_start;
        }
        if (new_rel > e->origin_rel) {
            wlx_text_geom_set_origin(&margs, e, new_rel, est_x, false);
            wlx_text_geom_ensure_width(&margs, e, inputs->rect.w - e->origin_x);
        }
    }
    wlx_text_geom_note_avg(inputs->geom, e);
}

// Extend or re-anchor a no-wrap entry so the caret's line-relative
// offset_rel lands inside measured coverage - the anchored-consumer
// entry point (it may move a settled measure origin; passive consumers
// must not call it). The x of a far-jump re-anchor is an estimate;
// byte offsets stay exact.
//
//   bytes -> |--------------|=========*=========|--------------|
//            line start     ^    caret (rel)    ^       line end
//                           |                   |
//                        new origin:       coverage end:
//                        caret - budget/2  origin + unit budget
//
// The caret lands mid-coverage, so motion and typing in either
// direction stay inside measured coverage before the next re-anchor.
// See docs/EDITOR_MODEL.md section 6 for the origin model.
static WLX_EXTENSION_USED void wlx_text_geom_ensure_caret(const WLX_Text_Measure_Args *args,
    WLX_Text_Geom_Store *s, WLX_Text_Geom_Entry *e, size_t offset_rel)
{
    const char *text = args->text;
    size_t length = args->length;
    float line_h = args->line_h;
    size_t line_start = e->line_start;
    size_t budget = (size_t)WLX_EDITOR_MAX_LINE_UNITS;

    if (offset_rel < e->origin_rel) {
        // Retreat: the offset is behind the origin.
        size_t steps = 0;
        size_t cand = wlx_text_geom_units_back(text, line_start + offset_rel,
            line_start, budget / WLX_TEXT_GEOM_ANCHOR_HEADROOM_DIV, &steps);
        cand = wlx_text_geom_snap_origin(text, line_start, cand);
        float ua = wlx_text_geom_avg_advance(s, e, line_h);
        size_t gap = wlx_text_geom_units_between(text, length, cand,
            line_start + e->origin_rel, WLX_TEXT_GEOM_FAR_GAP_BUDGETS * budget);
        float est = gap < WLX_TEXT_GEOM_FAR_GAP_BUDGETS * budget
            ? e->origin_x - ua * (float)gap
            : e->origin_x * ((float)(cand - line_start) / (float)e->origin_rel);
        wlx_text_geom_set_origin(args, e,
            cand <= line_start ? 0 : cand - line_start, est,
            gap <= budget);
    } else if (offset_rel > e->origin_rel) {
        // Bounded distance probe first: measuring a budget of units only
        // to find the offset beyond them would be pure throwaway work.
        size_t dist = e->units + wlx_text_geom_units_between(text, length,
            line_start + (e->units > 0 ? e->scan_rel : e->origin_rel),
            line_start + offset_rel, budget + 1 - (e->units < budget ? e->units : budget));
        if (dist <= budget) {
            wlx_text_geom_ensure_offset(args, e, offset_rel);
        }
        if (e->units == 0 || (size_t)e->unit_ends[e->units - 1] < offset_rel) {
            if (!e->complete && (e->units >= budget || dist > budget)) {
                // Far advance: anchor half a budget before the offset.
                float ua = wlx_text_geom_avg_advance(s, e, line_h);
                size_t cov_end = e->units > 0 ? e->scan_rel : e->origin_rel;
                float cov_right = e->units > 0
                    ? e->origin_x + e->advances[e->units - 1] : e->origin_x;
                size_t steps = 0;
                size_t cand = wlx_text_geom_units_back(text,
                    line_start + offset_rel, line_start + cov_end,
                    budget / WLX_TEXT_GEOM_ANCHOR_HEADROOM_DIV, &steps);
                cand = wlx_text_geom_snap_origin(text, line_start, cand);
                if (cand > line_start + e->origin_rel) {
                    size_t gap = wlx_text_geom_units_between(text, length,
                        line_start + cov_end, cand, WLX_TEXT_GEOM_FAR_GAP_BUDGETS * budget);
                    float est = gap < WLX_TEXT_GEOM_FAR_GAP_BUDGETS * budget
                        ? cov_right + ua * (float)gap
                        // Past the probe cap the gap estimate goes
                        // byte-proportional - bytes ~ units is acceptable
                        // inside the documented x estimate.
                        : cov_right + ua * (float)(cand - (line_start + cov_end));
                    wlx_text_geom_set_origin(args, e, cand - line_start, est, false);
                    wlx_text_geom_ensure_offset(args, e, offset_rel);
                }
            }
        }
    }
    wlx_text_geom_note_avg(s, e);
}

// ----------------------------------------------------------------------------
// Consumer queries: the entry-level answers the editor's geometry
// consumers read instead of pattern-matching entry fields. Each keeps
// the measuring path as its miss fallback at the caller (false / NULL
// means the entry cannot answer).
// ----------------------------------------------------------------------------

// Caret x in line-content space from a no-wrap entry, carrying the
// anchored/passive consumer split of the windowed origin. Returns
// false when the entry cannot answer and the caller must fall back to
// its storeless measure.
//
//   caret (rel) is...              anchored consumer   passive consumer
//   behind the measure origin      re-anchor, resolve  0 (off-view left)
//   at the origin                  origin_x            origin_x
//   in coverage, on a unit end     origin_x + advance  same (no measure)
//   in coverage, off a boundary    prefix measure      same
//   beyond coverage                extend/re-anchor,   pin to the
//                                  then resolve        coverage edge
//
// Passive extension is allowed only within the far-gap bound
// (WLX_TEXT_GEOM_FAR_GAP_BUDGETS budgets past the origin); farther
// carets pin without measuring, which is what keeps a caret parked
// off-view at zero steady-frame traffic. Anchored consumers may move
// a settled origin; the caret draw and other passive readers never do.
static WLX_EXTENSION_USED bool wlx_text_geom_x_at(const WLX_Text_Measure_Args *args,
    WLX_Text_Geom_Store *s, WLX_Text_Geom_Entry *e, size_t rel, bool anchor,
    float *out_x)
{
    if (anchor) {
        wlx_text_geom_ensure_caret(args, s, e, rel);
    } else if (rel < (size_t)e->origin_rel) {
        *out_x = 0.0f;
        return true;
    } else if (e->units == 0
        || rel - (size_t)e->origin_rel
            <= WLX_TEXT_GEOM_FAR_GAP_BUDGETS * (size_t)WLX_EDITOR_MAX_LINE_UNITS) {
        // Extend within the current origin's budget only; a caret it
        // cannot reach (a byte gap no budget of units covers) pins to
        // the coverage edge below without measuring. An empty entry
        // still fills once - bounded, then cached - so a caret parked
        // off the vertical window keeps its zero-measure steady frames.
        wlx_text_geom_ensure_offset(args, e, rel);
    }
    if (rel == (size_t)e->origin_rel) { *out_x = e->origin_x; return true; }
    if (rel > (size_t)e->origin_rel && e->units > 0) {
        bool covered = (size_t)e->unit_ends[e->units - 1] >= rel;
        if (covered) {
            float adv = 0.0f;
            if (wlx_text_geom_advance_at(e, rel, &adv)) {
                *out_x = e->origin_x + adv;
                return true;
            }
            // A covered caret off any measured unit boundary falls to
            // the exact prefix measure below, never to the pin.
        } else if (!anchor || e->units >= (size_t)WLX_EDITOR_MAX_LINE_UNITS) {
            // Beyond coverage: at the per-record budget the origin
            // policy already re-anchored, so this is one window's
            // worth past the origin; a passive consumer pins here
            // unconditionally rather than measure per frame.
            *out_x = e->origin_x + e->advances[e->units - 1];
            return true;
        }
        if (covered || e->complete) {
            // Off any measured unit boundary, or inside a complete
            // line's separator: measure the exact origin-relative
            // prefix (tab stops restart at the origin, matching the
            // drawn window).
            float w = 0.0f, h = 0.0f;
            if (wlx_text_measure_prefix_tabs(args->ctx, args->text, args->length,
                    wlx_text_geom_origin_abs(e), e->line_start + rel,
                    args->style, args->tab_advance, &w, &h)) {
                *out_x = e->origin_x + w;
                return true;
            }
        }
    }
    return false;
}

// Produce one no-wrap record's fit outcome (visible range, measured
// width and height, unit count) from a retained entry's stored
// advances, with zero backend calls. Returns false when the entry
// cannot answer and the caller's measuring scan must take over.
//
// The origin policy runs first, so a line deeper than the unit budget
// re-enters at a measure origin near the view; the record then starts
// at that origin and its measured width is origin-relative. The fit
// walk repeats the measuring scan's decisions exactly - same
// wlx_text_fit_step, first unit always accepted, budget-capped - which
// is the replay-equals-scan invariant of docs/TEXT_PIPELINE_MAP.md.
static bool wlx_text_geom_replay_linear(const WLX_Text_Build_Inputs *inputs,
    WLX_Text_Geom_Entry *e, size_t *visible_start, size_t *visible_end,
    float *measured_w, float *measured_h, size_t *scan_unit_count)
{
    wlx_text_geom_window_linear(inputs, e);
    if (e->units == 0) return false;

    float fit_w = inputs->rect.w - e->origin_x;
    size_t fit = 0;
    while (fit < e->units) {
        if (fit >= (size_t)WLX_EDITOR_MAX_LINE_UNITS) break;
        WLX_Text_Fit verdict = wlx_text_fit_step(e->advances[fit], fit_w, fit);
        if (verdict == WLX_TEXT_FIT_REJECT) break;
        fit++;
        if (verdict == WLX_TEXT_FIT_ACCEPT_END) break;
    }
    // Reject the replay when the walk ran out of stored units while
    // the line still fit: a complete line or a spent budget ends that
    // way legitimately, anything else is an under-measured entry (a
    // mid-extension allocation failure) the measuring scan must redo.
    if (fit == e->units && !e->complete
        && e->units < (size_t)WLX_EDITOR_MAX_LINE_UNITS
        && e->advances[e->units - 1] <= fit_w) {
        return false;
    }

    *visible_start = e->line_start + e->origin_rel;
    *visible_end = e->line_start + e->unit_ends[fit - 1];
    *measured_w = e->advances[fit - 1];
    *measured_h = e->heights[fit - 1];
    *scan_unit_count = fit;
    return true;
}

// Build a wrapped line's full retained geometry in one pass: the same
// per-row growing-prefix scan the wrapped build runs, for every row of
// the hard line up to the per-line unit budget. Units arrive through
// wlx_text_unit_fetch (chunked advances batch or per-unit prefix
// measure, per backend capability, row-start measure base); a candidate
// that fits (or starts its row) is stored with its row-relative advance,
// and a rejected candidate is discarded together with the rest of its
// fetched batch and re-measured from the next row's start, exactly like
// the measuring scan. Returns false on allocation failure or a backend
// fill that cannot progress (the caller then falls back to the
// measuring build).
//
//   per hard line, until its separator or the per-line budget:
//     row loop      [ row 0 ][ row 1 ][ row 2 ] ...
//       fetch loop    |batch of units|  (one chunked fill, or one
//                                       per-unit-measured unit)
//         unit loop     fit each unit: ACCEPT stores it with its
//                       row-relative advance; REJECT discards the
//                       batch remainder and opens the next row at
//                       that unit
static bool wlx_text_geom_ensure_wrap(const WLX_Text_Build_Inputs *inputs,
    WLX_Text_Geom_Entry *e)
{
    if (e->complete) return true;
    const char *text = inputs->text;
    size_t length = inputs->length;
    size_t line_start = e->line_start;
    WLX_Text_Measure_Args margs = { inputs->ctx, inputs->text, inputs->length,
        inputs->style, inputs->tab_advance, inputs->line_h };

    e->units = 0;
    e->rows = 0;
    e->scan_rel = 0;
    e->first_tab_rel = UINT32_MAX;

    size_t ends[WLX_TEXT_ADVANCES_CHUNK];
    float advs[WLX_TEXT_ADVANCES_CHUNK];
    float heights[WLX_TEXT_ADVANCES_CHUNK];
    size_t first_tab = SIZE_MAX;

    size_t line_units = 0;
    size_t row_start = line_start;
    for (;;) {
        if (row_start >= length || line_units >= (size_t)WLX_EDITOR_MAX_LINE_UNITS
            || wlx_text_at_line_break(text, length, row_start)) {
            break;
        }
        if (e->rows == e->row_cap) {
            size_t new_cap = e->row_cap == 0 ? 8 : e->row_cap * 2;
            uint32_t *grown = (uint32_t *)wlx_realloc(e->row_units, new_cap * sizeof(uint32_t));
            if (grown == NULL) return false;
            e->row_units = grown;
            e->row_cap = new_cap;
        }
        e->row_units[e->rows++] = (uint32_t)e->units;

        size_t pos = row_start;
        size_t row_units_count = 0;
        float row_x = 0.0f;
        bool row_done = false;
        while (!row_done) {
            if (pos >= length || wlx_text_at_line_break(text, length, pos)) {
                row_start = length; // line text exhausted; outer loop ends
                break;
            }
            if (line_units >= (size_t)WLX_EDITOR_MAX_LINE_UNITS) {
                row_start = length; // per-line budget spent; final row freezes
                break;
            }
            size_t n = wlx_text_unit_fetch(&margs, pos, row_start, row_x,
                (size_t)WLX_EDITOR_MAX_LINE_UNITS - line_units, true, ends,
                advs, heights, &first_tab);
            if (n == 0) return false; // abort to the measuring build
            for (size_t i = 0; i < n; i++) {
                WLX_Text_Fit fit = wlx_text_fit_step(advs[i], inputs->rect.w,
                    row_units_count);
                if (fit == WLX_TEXT_FIT_REJECT) {
                    // Rejected: the unit belongs to the next row and will be
                    // re-measured from that row's start.
                    row_start = pos;
                    row_done = true;
                    break;
                }
                if (!wlx_text_geom_push_unit(e, (uint32_t)(ends[i] - line_start),
                        advs[i], heights[i]))
                    return false;
                pos = ends[i];
                row_units_count++;
                line_units++;
                if (fit == WLX_TEXT_FIT_ACCEPT_END) {
                    row_start = pos;
                    row_done = true;
                    break;
                }
            }
            if (!row_done) row_x = advs[n - 1];
        }
    }
    wlx_text_geom_first_tab_note(e, first_tab);
    e->scan_rel = e->units > 0 ? e->unit_ends[e->units - 1] : 0;
    e->complete = true;
    return true;
}

// Replay one wrapped row's scan outcome from a retained entry. The row is
// located by its byte start; a mid-line entry miss (evicted mid-stream)
// returns false and the caller's measuring scan takes over.
static bool wlx_text_geom_wrap_step(const WLX_Text_Build_Inputs *inputs,
    WLX_Text_Build_Cursor *cursor, size_t source_offset, size_t *visible_end,
    float *measured_w, float *measured_h, size_t *scan_unit_count)
{
    size_t line_next = inputs->known_line_next;
    if (line_next <= source_offset || line_next > inputs->length) return false;

    WLX_Text_Geom_Entry *e = wlx_text_geom_find_containing(inputs->geom,
        source_offset, line_next);
    if (e == NULL) {
        if (!wlx_text_hard_line_start_at(inputs->text, inputs->length, source_offset))
            return false;
        e = wlx_text_geom_acquire(inputs->geom, source_offset, line_next);
        if (e == NULL) return false;
    }
    if (!e->complete && !wlx_text_geom_ensure_wrap(inputs, e)) {
        wlx_text_geom_drop(e);
        return false;
    }
    if (e->rows == 0 || e->units == 0) return false;

    size_t rel = source_offset - e->line_start;
    size_t row = SIZE_MAX;
    for (size_t r = 0; r < e->rows; r++) {
        size_t start_rel = r == 0 ? 0 : (size_t)e->unit_ends[e->row_units[r] - 1];
        if (start_rel == rel) { row = r; break; }
        if (start_rel > rel) break;
    }
    if (row == SIZE_MAX) return false;

    size_t lo = e->row_units[row];
    size_t hi = row + 1 < e->rows ? (size_t)e->row_units[row + 1] : e->units;
    if (hi <= lo) return false;

    *visible_end = e->line_start + e->unit_ends[hi - 1];
    *measured_w = e->advances[hi - 1];
    *measured_h = e->heights[hi - 1];
    *scan_unit_count = hi - lo;
    // Backfill the build cursor's first-tab fact, but only as far as
    // the replayed range would itself have discovered it: later prefix
    // prechecks see the same overestimate-safe fact the measuring scan
    // would have produced, so the replay changes no downstream result.
    size_t replayed_first_tab = wlx_text_geom_first_tab_abs(e);
    if (cursor->line_first_tab == SIZE_MAX
        && wlx_text_pen_has_tab(replayed_first_tab,
            e->line_start + (size_t)e->unit_ends[hi - 1])) {
        cursor->line_first_tab = replayed_first_tab;
    }
    return true;
}

// ----------------------------------------------------------------------------
// The build kernel. One step produces one line record by greedy fitting
// over cumulative unit advances drawn from one of three sources - the
// retained-store replay, the backend's batched advances callback, or the
// per-unit prefix measure - all sharing wlx_text_fit_step so replayed
// and measured builds make identical fit decisions.
// ----------------------------------------------------------------------------

// Produce the next line record at source_offset (a hard line start or
// a wrap/truncation continuation) and report where the following
// record begins. Advance sources are tried in order: retained-store
// replay when the inputs carry the editor's store, else the measuring
// fetch - batched or per-unit arm, gated so non-editor builds never
// batch; the separator / truncated-tail epilogue is shared by every
// outcome. A budget descriptor resolved at entry maps the three cap
// regimes (whole-text, per-record, per-hard-line) onto one
// counter-plus-cap pair. trailing_empty_line appends the
// caret-addressable empty record after a final separator. See the
// build-step diagram in docs/LINE_RUN_MODEL.md section 5.
static WLX_Text_Build_Step wlx_text_build_step(const WLX_Text_Build_Inputs *inputs,
    WLX_Text_Build_Cursor *cursor, size_t source_offset, bool trailing_empty_line)
{
    WLX_Text_Build_Step step = {0};

    if (inputs == NULL || cursor == NULL) return step;

    const char *text = inputs->text ? inputs->text : "";
    size_t length = inputs->length;

    // Per-hard-line budget mode (wrap + truncate_continue): the wrapped
    // rows of one hard line share one unit budget, refreshed whenever a
    // step enters at a hard line start. The first-tab fact resets there in
    // every mode - the growing-prefix measures below test against it.
    bool line_budget = inputs->truncate_continue && inputs->wrap;
    if (wlx_text_hard_line_start_at(text, length, source_offset)) {
        if (line_budget) cursor->line_unit_count = 0;
        cursor->line_first_tab = SIZE_MAX;
    }

    if (trailing_empty_line) {
        if (source_offset > length) return step;
        wlx_text_line_record_from_range(inputs->ctx, &step.line, text, length, (WLX_Text_Line_Record_Opt){
            .source_start    = source_offset,
            .visible_start   = source_offset,
            .visible_end     = source_offset,
            .separator_start = source_offset,
            .separator_end   = source_offset,
            .measured_w      = 0.0f,
            .measured_h      = inputs->line_h,
            .has_measurement = false,
            .line_h          = inputs->line_h,
            .style           = inputs->style,
            .ended_by_newline = false,
        });
        step.next_offset = source_offset;
        step.append_trailing_empty_line = false;
        step.produced = true;
        return step;
    }

    if (source_offset >= length) return step;

    size_t separator_end = 0;
    if (wlx_text_newline_at(text, length, source_offset, &separator_end)) {
        if (source_offset > length || separator_end > length) return step;
        wlx_text_line_record_from_range(inputs->ctx, &step.line, text, length, (WLX_Text_Line_Record_Opt){
            .source_start    = source_offset,
            .visible_start   = source_offset,
            .visible_end     = source_offset,
            .separator_start = source_offset,
            .separator_end   = separator_end,
            .measured_w      = 0.0f,
            .measured_h      = inputs->line_h,
            .has_measurement = false,
            .line_h          = inputs->line_h,
            .style           = inputs->style,
            .ended_by_newline = true,
        });
        step.next_offset = separator_end;
        step.append_trailing_empty_line = (separator_end == length);
        step.produced = true;
        return step;
    }

    size_t visible_start = source_offset;
    size_t visible_end = source_offset;
    size_t sep_end = source_offset;
    float measured_w = 0.0f;
    float measured_h = inputs->line_h;
    bool ended_by_newline = false;
    bool have_line = false;
    size_t scan_unit_count = 0;
    bool truncate_continue = inputs->truncate_continue && !inputs->wrap;

    // Retained geometry replay: the editor's per-line store answers the
    // scan without backend measures when it holds this line at the needed
    // reach; misses and extensions measure through the same walk below and
    // land in the store for the frames that follow. The separator and
    // tail-skip logic after the scan is shared by both paths.
    bool geom_replayed = false;
    if (inputs->geom != NULL && inputs->truncate_continue && length > 0) {
        if (!inputs->wrap) {
            if (inputs->known_line_next > source_offset
                && inputs->known_line_next <= length) {
#ifdef WLX_DEBUG
                assert(wlx_text_hard_line_start_at(text, length, source_offset)
                    && "geom store: no-wrap records must start at hard line starts");
#endif
                WLX_Text_Geom_Entry *ge = wlx_text_geom_acquire(inputs->geom,
                    source_offset, inputs->known_line_next);
                if (ge != NULL) {
                    geom_replayed = wlx_text_geom_replay_linear(inputs, ge,
                        &visible_start, &visible_end, &measured_w, &measured_h,
                        &scan_unit_count);
                    if (!geom_replayed) wlx_text_geom_drop(ge);
                }
            }
        } else {
            geom_replayed = wlx_text_geom_wrap_step(inputs, cursor, source_offset,
                &visible_end, &measured_w, &measured_h, &scan_unit_count);
        }
    }
    if (geom_replayed) have_line = true;

    size_t scan_pos = source_offset;
    WLX_Text_Measure_Args margs = { inputs->ctx, text, length,
        inputs->style, inputs->tab_advance, inputs->line_h };
    // Budget descriptor, resolved once: which cumulative counter (NULL =
    // record-local only) joins this record's used-unit count, and the
    // cap. The three regimes keep their locked semantics - whole-text
    // freeze-at-cap, per-record, per-hard-line - only the accounting
    // mechanism is shared.
    size_t *budget_counter = line_budget ? &cursor->line_unit_count
        : truncate_continue ? NULL : &cursor->text_unit_count;
    size_t budget_cap = (truncate_continue || line_budget)
        ? (size_t)WLX_EDITOR_MAX_LINE_UNITS : inputs->text_unit_cap;
    // The measuring scan: one fetch-and-fit loop over the unit fetch.
    // Batching is gated on editor-mode scans (truncate_continue), never
    // on backend capability alone, so non-editor builds stay per-unit
    // and byte-identical whatever the backend provides. A batch that
    // cannot progress falls to per-unit for the rest of this record
    // (sticky), still measuring whole prefixes from source_offset; a fit
    // overflow ends the record outright, never switches modes.
    bool batch_rec = inputs->truncate_continue && inputs->ctx != NULL
        && inputs->ctx->backend.measure_text_advances != NULL;
    while (!geom_replayed && scan_pos < length) {
        if (wlx_text_newline_at(text, length, scan_pos, &sep_end)) {
            ended_by_newline = true;
            break;
        }
        size_t units_used = (budget_counter != NULL ? *budget_counter : 0)
            + scan_unit_count;
        if (units_used >= budget_cap) break;

        size_t ends[WLX_TEXT_ADVANCES_CHUNK];
        float advs[WLX_TEXT_ADVANCES_CHUNK];
        float heights[WLX_TEXT_ADVANCES_CHUNK];
        size_t n = wlx_text_unit_fetch(&margs, scan_pos, source_offset,
            measured_w, budget_cap - units_used, batch_rec, ends, advs,
            heights, &cursor->line_first_tab);
        if (n == 0) { batch_rec = false; continue; }
        bool overflow = false;
        for (size_t i = 0; i < n; i++) {
            WLX_Text_Fit fit = wlx_text_fit_step(advs[i], inputs->rect.w,
                scan_unit_count);
            if (fit == WLX_TEXT_FIT_REJECT) { overflow = true; break; }
            visible_end = ends[i];
            measured_w = advs[i];
            measured_h = heights[i];
            have_line = true;
            scan_pos = ends[i];
            scan_unit_count++;
            if (fit == WLX_TEXT_FIT_ACCEPT_END) { overflow = true; break; }
        }
        if (overflow) break;
    }

    if (!have_line) return step;
    if (scan_unit_count == 0 && visible_end > source_offset) return step;

    size_t separator_start = visible_end;
    bool skip_tail_to_eof = false;
    // A hard line that exhausted its per-line budget freezes: its final
    // wrapped row skips the unmeasured tail like a truncated record.
    bool line_budget_spent = line_budget
        && cursor->line_unit_count + scan_unit_count >= (size_t)WLX_EDITOR_MAX_LINE_UNITS;
    if (!ended_by_newline) {
        size_t separator_after = 0;
        if (wlx_text_newline_at(text, length, visible_end, &separator_after)) {
            ended_by_newline = true;
            sep_end = separator_after;
        } else if (truncate_continue || line_budget_spent) {
            // Skip the invisible tail to the next hard line start without
            // measuring; the record keeps the full source range so offset
            // math over the tail stays well-defined. A caller-supplied
            // next-line-start hint replaces the newline scan; the
            // separator sits immediately before that start (CRLF two
            // bytes, LF/CR one, none on the final line).
            size_t tail = visible_end;
            bool have_tail = false;
            size_t hint = inputs->known_line_next;
            if (hint > visible_end && hint <= length) {
                size_t sep_start = hint;
                if (!wlx_text_separator_before(text, length, hint, &sep_start))
                    sep_start = hint;
                if (sep_start >= visible_end) {
                    tail = sep_start;
                    have_tail = true;
                }
            }
            if (!have_tail) {
                while (tail < length && !wlx_text_newline_at(text, length, tail, &separator_after)) tail++;
            }
            if (tail < length && wlx_text_newline_at(text, length, tail, &separator_after)) {
                ended_by_newline = true;
                separator_start = tail;
                sep_end = separator_after;
            } else {
                separator_start = length;
                sep_end = length;
                skip_tail_to_eof = true;
            }
        } else {
            sep_end = visible_end;
        }
    }

    cursor->text_unit_count += scan_unit_count;
    if (line_budget) cursor->line_unit_count += scan_unit_count;

    wlx_text_line_record_from_range(inputs->ctx, &step.line, text, length, (WLX_Text_Line_Record_Opt){
        .source_start    = source_offset,
        .visible_start   = visible_start,
        .visible_end     = visible_end,
        .separator_start = separator_start,
        .separator_end   = sep_end,
        .measured_w      = measured_w,
        .measured_h      = measured_h,
        .has_measurement = true,
        .line_h          = inputs->line_h,
        .style           = inputs->style,
        .ended_by_newline = ended_by_newline,
    });

    step.next_offset = ended_by_newline ? sep_end : visible_end;
    if (skip_tail_to_eof) {
        step.line.source_end = length;
        step.next_offset = length;
    }
    step.append_trailing_empty_line = ended_by_newline && step.next_offset == length;
    step.stop_after_line = !inputs->wrap && !ended_by_newline && step.next_offset < length;
    step.produced = true;
    return step;
}

// Build line records starting at start_offset, which must be a hard line
// start (offset 0 or the end of a newline separator). The produced records
// match the corresponding tail of a full build over the same inputs.
static size_t wlx_text_build_lines_from(const WLX_Text_Build_Inputs *inputs, WLX_Text_Build_Cursor *cursor,
    size_t start_offset, WLX_Text_Line_Record *lines, size_t line_cap) {
    if (inputs == NULL || cursor == NULL || lines == NULL || line_cap == 0) return 0;

#ifdef WLX_DEBUG
    assert(wlx_text_hard_line_start_at(inputs->text, inputs->length, start_offset));
    assert(start_offset >= inputs->length || inputs->text == NULL
        || !wlx_text_utf8_is_continuation((unsigned char)inputs->text[start_offset]));
#endif

    size_t line_count = 0;
    size_t line_offset = start_offset;
    // A start at end-of-text after a final separator is the trailing empty
    // line; a full build reaches it with the append flag already set.
    bool append_trailing_empty_line = start_offset == inputs->length && inputs->length > 0
        && wlx_text_hard_line_start_at(inputs->text, inputs->length, start_offset);

    while (line_count < line_cap) {
        WLX_Text_Build_Step step = wlx_text_build_step(inputs, cursor, line_offset, append_trailing_empty_line);
        if (!step.produced) break;
        lines[line_count++] = step.line;
        line_offset = step.next_offset;
        append_trailing_empty_line = step.append_trailing_empty_line;
        if (step.stop_after_line) break;
    }

    return line_count;
}

static size_t wlx_text_build_lines(const WLX_Text_Build_Inputs *inputs, WLX_Text_Build_Cursor *cursor,
    WLX_Text_Line_Record *lines, size_t line_cap) {
    return wlx_text_build_lines_from(inputs, cursor, 0, lines, line_cap);
}

// ----------------------------------------------------------------------------
// Alignment, emission, and the from-lines geometry consumers. Everything
// from here answers from the record arrays the build produced: caret
// position, hit testing, selection spans, word bounds, and the one draw
// call per visible line. Geometry always derives from line records,
// never from a second line-breaking pass.
// ----------------------------------------------------------------------------
static void wlx_text_align_lines(WLX_Rect rect, WLX_Align align, float line_h,
    WLX_Vertical_Metric vmetric, int font_size,
    WLX_Text_Line_Record *lines, size_t line_count) {

    if (lines == NULL || line_count == 0) return;

    float vmetric_h = (vmetric == WLX_VMETRIC_FONT_SIZE && font_size > 0)
        ? (float)font_size
        : line_h;

    float total_h = (float)line_count * vmetric_h;
    WLX_Rect vert = wlx_get_align_rect(rect, -1, total_h, align);
    float line_y = vert.y;

    for (size_t line_index = 0; line_index < line_count; line_index++) {
        WLX_Rect line_rect = { rect.x, line_y, rect.w, vmetric_h };
        WLX_Rect aligned = wlx_get_align_rect(line_rect, lines[line_index].advance_w, vmetric_h, align);
        lines[line_index].origin_x = aligned.x;
        lines[line_index].origin_y = line_y;
        line_y += vmetric_h;
    }
}

static bool wlx_text_emit_lines(WLX_Context *ctx, WLX_Rect rect, const char *text, size_t length,
    WLX_Text_Style style, const WLX_Text_Line_Record *lines, size_t line_count) {

    if (lines == NULL && line_count > 0) return false;

    for (size_t line_index = 0; line_index < line_count; line_index++) {
        const WLX_Text_Line_Record *line = &lines[line_index];
        float line_top = line->origin_y;
        float line_bottom = line_top + line->line_h;

        if (line_bottom > rect.y && line_top < rect.y + rect.h && !line->empty_visual) {
            bool drew_line = wlx_draw_text_range(ctx, text, length, line->visible_start, line->visible_end,
                line->origin_x, line->origin_y, style);
            if (!drew_line) return false;
        }
    }

    return true;
}

static bool wlx_text_lines_need_scissor(WLX_Rect rect, const WLX_Text_Line_Record *lines, size_t line_count) {
    if (lines == NULL && line_count > 0) return false;

    for (size_t line_index = 0; line_index < line_count; line_index++) {
        const WLX_Text_Line_Record *line = &lines[line_index];
        float line_top = line->origin_y;
        float line_bottom = line_top + line->line_h;

        if (line->empty_visual) continue;
        if (!(line_bottom > rect.y && line_top < rect.y + rect.h)) continue;

        if (line->origin_x < rect.x) return true;
        if (line->origin_x + line->measured_w > rect.x + rect.w) return true;
        if (line_top < rect.y) return true;
        if (line_bottom > rect.y + rect.h) return true;
    }

    return false;
}

// Caret x,y for cursor_offset from prepared records: the record whose
// cursor range owns the offset gives y, and a prefix measure
// [visible_start, offset) gives x; offsets at or past visible_end (a
// truncated tail) pin to the line's measured right edge. An offset no
// record owns falls back to the last line's end; empty line_count
// answers the rect corner. Returns false only on a NULL record array.
static bool wlx_text_resolve_cursor_from_lines(WLX_Context *ctx, WLX_Rect rect, const char *text, size_t length,
    WLX_Text_Style style, const WLX_Text_Line_Record *lines, size_t line_count, size_t cursor_offset,
    float *out_x, float *out_y) {

    if (lines == NULL && line_count > 0) return false;

    if (line_count == 0) {
        if (out_x) *out_x = rect.x;
        if (out_y) *out_y = rect.y;
        return true;
    }

    for (size_t i = 0; i < line_count; i++) {
        const WLX_Text_Line_Record *line = &lines[i];
        if (wlx_text_cursor_is_on_line(line, cursor_offset)) {
            float prefix_w = 0.0f;
            if (!line->empty_visual) {
                if (cursor_offset >= line->visible_end) {
                    prefix_w = line->measured_w;
                } else if (cursor_offset > line->visible_start) {
                    float prefix_h = 0.0f;
                    bool measured = wlx_measure_text_range(ctx, text, length, line->visible_start, cursor_offset, style,
                        &prefix_w, &prefix_h);
                    if (!measured) prefix_w = 0.0f;
                }
            }

            if (out_x) *out_x = line->origin_x + prefix_w;
            if (out_y) *out_y = line->origin_y;
            return true;
        }
    }

    const WLX_Text_Line_Record *last_line = &lines[line_count - 1];
    if (out_x) *out_x = last_line->origin_x + last_line->measured_w;
    if (out_y) *out_y = last_line->origin_y;
    return true;
}

// Borrow the context's line-record scratch, growing it to hold at least
// min_records. Returns NULL only on allocation failure. The buffer is a
// loan, not a transfer: exactly one caller may hold it at a time, and its
// contents are invalid after the next borrower writes to it.
static WLX_Text_Line_Record *wlx_text_line_scratch(WLX_Context *ctx, size_t min_records) {
    WLX_HARD_ASSERT(min_records <= SIZE_MAX / sizeof(WLX_Text_Line_Record),
        "line scratch size overflow");
    if (ctx->text_line_scratch_cap < min_records) {
        WLX_Text_Line_Record *grown = (WLX_Text_Line_Record *)wlx_realloc(
            ctx->text_line_scratch, min_records * sizeof(WLX_Text_Line_Record));
        if (grown == NULL) return NULL;
        ctx->text_line_scratch = grown;
        ctx->text_line_scratch_cap = min_records;
    }
    return ctx->text_line_scratch;
}

// Options for preparing fitted line records. Designated-initializer
// defaults: zero-init gives no alignment, no wrap, the line-height
// vertical metric, and the standard per-run unit budget (a zero
// text_unit_cap means WLX_TEXT_RUN_MAX_UNITS - the default is nonzero,
// so 0 is the unambiguous "use the default" spelling).
typedef struct WLX_Text_Prepare_Opt {
    WLX_Align align;
    bool wrap;
    WLX_Vertical_Metric vmetric;
    size_t text_unit_cap;   // 0 = WLX_TEXT_RUN_MAX_UNITS
} WLX_Text_Prepare_Opt;

static bool wlx_text_prepare_lines_slice(WLX_Context *ctx, WLX_Rect rect, const char *text, size_t length, WLX_Text_Style style,
    WLX_Text_Prepare_Opt opt, WLX_Text_Line_Record *lines, size_t line_cap, WLX_Text_Line_Array_Result *out_result) {

    if (lines == NULL || out_result == NULL) return false;
    wlx_zero_struct(*out_result);

    if (style.font_size <= 0) return false;
    WLX_PERF_HOOK(text_run, ctx, length);

    float line_h = wlx_text_line_height(ctx, style, NULL);

    out_result->text_length = length;
    out_result->line_h = line_h;

    if (length == 0) return true;

    WLX_Text_Build_Inputs inputs = {
        .ctx = ctx,
        .text = text,
        .length = length,
        .style = style,
        .rect = rect,
        .wrap = opt.wrap,
        .line_h = line_h,
        .text_unit_cap = opt.text_unit_cap > 0
            ? opt.text_unit_cap : (size_t)WLX_TEXT_RUN_MAX_UNITS,
    };
    WLX_Text_Build_Cursor cursor = { .text_unit_count = 0 };

    out_result->line_count = wlx_text_build_lines(&inputs, &cursor, lines, line_cap);
    wlx_text_align_lines(rect, opt.align, line_h, opt.vmetric, style.font_size, lines, out_result->line_count);
    return true;
}

// Caller-declared prepare-and-consume aggregate: the standard stack
// record array plus its result, filled in one call by wlx_text_prepare.
// Failure mapping stays with the caller.
typedef struct WLX_Text_Prepared {
    WLX_Text_Line_Record lines[WLX_TEXT_RUN_MAX_LINES];
    WLX_Text_Line_Array_Result result;
} WLX_Text_Prepared;

static bool wlx_text_prepare(WLX_Context *ctx, WLX_Rect rect, const char *text, size_t length,
    WLX_Text_Style style, WLX_Text_Prepare_Opt opt, WLX_Text_Prepared *p)
{
    return wlx_text_prepare_lines_slice(ctx, rect, text, length, style, opt,
        p->lines, WLX_TEXT_RUN_MAX_LINES, &p->result);
}

static bool wlx_calc_cursor_position_for_text(WLX_Context *ctx, WLX_Rect rect, const char *text, WLX_Text_Style style,
    WLX_Align align, bool wrap, size_t cursor_offset, float *cursor_x, float *cursor_y) {

    if (!text) text = "";
    WLX_Text_Prepared p;
    if (!wlx_text_prepare(ctx, rect, text, strlen(text), style,
            (WLX_Text_Prepare_Opt){ .align = align, .wrap = wrap }, &p)) {
        return false;
    }

    cursor_offset = wlx_text_normalize_cursor_offset(text, p.result.text_length, cursor_offset);

    if (p.result.text_length == 0) {
        WLX_Rect aligned = wlx_get_align_rect(rect, 0.0f, p.result.line_h, align);
        if (cursor_x) *cursor_x = aligned.x;
        if (cursor_y) *cursor_y = aligned.y;
        return true;
    }

    return wlx_text_resolve_cursor_from_lines(ctx, rect, text, p.result.text_length, style,
        p.lines, p.result.line_count, cursor_offset, cursor_x, cursor_y);
}

// Map a point to the nearest cursor offset in already-prepared lines: pick
// the visual line whose vertical band contains py (clamped to the first/last
// line), then walk codepoint boundaries measuring prefix advances and choose
// the boundary nearest px (midpoint rule). Inverse of
// wlx_text_resolve_cursor_from_lines, built on the same line records.
static size_t wlx_text_offset_at_point_from_lines(WLX_Context *ctx, const char *text, size_t length,
    WLX_Text_Style style, const WLX_Text_Line_Record *lines, size_t line_count, float px, float py)
{
    if (text == NULL || length == 0 || lines == NULL || line_count == 0) return 0;

    const WLX_Text_Line_Record *line = &lines[line_count - 1];
    for (size_t i = 0; i < line_count; i++) {
        if (py < lines[i].origin_y + lines[i].line_h) {
            line = &lines[i];
            break;
        }
    }

    if (line->empty_visual) return line->cursor_start;

    size_t off = line->visible_start;
    float prev_w = 0.0f;
    while (off < line->visible_end) {
        size_t next = wlx_text_unit_next(text, length, off);
        if (next > line->visible_end) next = line->visible_end;
        float w = 0.0f, h = 0.0f;
        if (!wlx_measure_text_range(ctx, text, length, line->visible_start, next, style, &w, &h)) break;
        if (px < line->origin_x + (prev_w + w) * 0.5f) return off;
        prev_w = w;
        off = next;
    }
    return off;
}

// Convenience wrapper over wlx_text_offset_at_point_from_lines that prepares
// the line records itself. Test seam: the widgets hit-test through their
// frame's shared records; the suites call this to probe the same geometry
// without a frame.
static inline size_t wlx_text_offset_at_point(WLX_Context *ctx, WLX_Rect rect, const char *text, size_t length,
    WLX_Text_Style style, WLX_Align align, bool wrap, float px, float py)
{
    if (text == NULL || length == 0) return 0;

    WLX_Text_Prepared p;
    if (!wlx_text_prepare(ctx, rect, text, length, style,
            (WLX_Text_Prepare_Opt){ .align = align, .wrap = wrap }, &p)) {
        return length;
    }
    return wlx_text_offset_at_point_from_lines(ctx, text, length, style,
        p.lines, p.result.line_count, px, py);
}

// Word range around a byte offset: the run of word bytes containing it, or
// the run of separators when the offset sits between words (mirrors common
// double-click behaviour). Byte-level scanning is UTF-8 safe because the
// separators are ASCII.
static void wlx_text_word_bounds(const char *text, size_t length, size_t offset,
    size_t *out_start, size_t *out_end)
{
    if (length == 0) {
        if (out_start) *out_start = 0;
        if (out_end) *out_end = 0;
        return;
    }
    if (offset >= length) offset = length - 1;
    // Back off continuation bytes so the class test reads a lead/ASCII byte.
    offset = wlx_utf8_floor(text, offset);

    bool sep_class = wlx_utf8_is_word_separator(text[offset]);
    size_t start = offset;
    size_t end = offset;
    while (start > 0 && wlx_utf8_is_word_separator(text[start - 1]) == sep_class) start--;
    while (end < length && wlx_utf8_is_word_separator(text[end]) == sep_class) end++;
    if (out_start) *out_start = start;
    if (out_end) *out_end = end;
}

// Emit already-prepared lines with overflow clipping against rect.
static bool wlx_draw_text_lines_fitted(WLX_Context *ctx, WLX_Rect rect, const char *text, size_t length,
    WLX_Text_Style style, const WLX_Text_Line_Record *lines, size_t line_count) {

    if (length == 0 || lines == NULL || line_count == 0) return true;
    if (wlx_text_lines_need_scissor(rect, lines, line_count)) {
        // Clip text overflow to the rect. A single line is never clipped
        // vertically by its own rect: the rendered line can exceed the rect
        // height (leading, descenders, or a backend-side font scale), and
        // cropping it would cut ascenders/descenders. Expand the clip to the
        // line's vertical extent while keeping the horizontal bounds, so wide
        // text still clips horizontally. Multi-line text keeps clipping to the
        // rect (overflow lines are cropped), and the enclosing scissor (panel,
        // scroll viewport) still bounds the result either way.
        WLX_Rect clip = rect;
        if (line_count == 1) {
            float line_top = lines[0].origin_y;
            float line_bottom = line_top + lines[0].line_h;
            if (line_top < clip.y) { clip.h += clip.y - line_top; clip.y = line_top; }
            if (line_bottom > clip.y + clip.h) clip.h = line_bottom - clip.y;
        }
        WLX_Scissor_Scope sc = wlx_scissor_scope_begin(ctx, clip);
        bool ok = wlx_text_emit_lines(ctx, rect, text, length, style, lines, line_count);
        wlx_scissor_scope_end(ctx, sc);
        return ok;
    }

    return wlx_text_emit_lines(ctx, rect, text, length, style, lines, line_count);
}

static bool wlx_draw_text_fitted_slice(WLX_Context *ctx, WLX_Rect rect, const char *text, size_t length, WLX_Text_Style style,
    WLX_Text_Prepare_Opt opt) {

    if (!text) { text = ""; length = 0; }
    WLX_Text_Prepared p;
    if (!wlx_text_prepare(ctx, rect, text, length, style, opt, &p)) return false;

    return wlx_draw_text_lines_fitted(ctx, rect, text, p.result.text_length, style,
        p.lines, p.result.line_count);
}

// NUL-terminated adapter: one strlen, then the slice path.
static bool wlx_draw_text_fitted(WLX_Context *ctx, WLX_Rect rect, const char *text, WLX_Text_Style style,
    WLX_Align align, bool wrap) {
    if (!text) text = "";
    return wlx_draw_text_fitted_slice(ctx, rect, text, strlen(text), style,
        (WLX_Text_Prepare_Opt){ .align = align, .wrap = wrap });
}

WLXDEF bool wlx_calc_cursor_position(WLX_Context *ctx, WLX_Rect rect, const char *text, WLX_Text_Style style, WLX_Align align, bool wrap,
    float *cursor_x, float *cursor_y) {
    size_t cursor_offset = text ? strlen(text) : 0;
    return wlx_calc_cursor_position_for_text(ctx, rect, text, style, align, wrap, cursor_offset, cursor_x, cursor_y);
}



// ============================================================================
// Implementation: widget rendering and widget interaction
// ============================================================================

static const float WLX_CHECKBOX_SIZE_RATIO = 0.8f;
// Track width as a multiple of height when the theme leaves the ratio unset.
static const float WLX_TOGGLE_TRACK_RATIO_FALLBACK = 2.0f;
// Gap between the glyph and its trailing label, as a fraction of the font
// size. Shared by all glyph + label compounds (checkbox, toggle, radio) via
// wlx_layout_glyph_row.
static const float WLX_GLYPH_ROW_LABEL_PADDING_FACTOR = 0.5f;

// Width of a glyph + label block: the glyph, then gap + label when the row
// keeps label space (reserve_empty_label, or a label that measured wider
// than zero).
static inline float wlx_glyph_row_block_w(float glyph_w, float label_w, float padding, bool reserve_empty_label) {
    bool has_label_space = reserve_empty_label || label_w > 0.0f;
    return glyph_w + (has_label_space ? padding + label_w : 0.0f);
}

// Intrinsic width of a glyph+label row (checkbox/toggle/radio): glyph box,
// label gap, single-line label measure - the same shape wlx_layout_glyph_row
// produces at draw time. reserve_empty_label mirrors that helper's contract.
// Callers add resolved padding.
static inline float wlx_intrinsic_glyph_row_width(WLX_Context *ctx,
    float glyph_w, const char *label, size_t label_len, WLX_Text_Style ts,
    bool reserve_empty_label)
{
    float label_w = wlx_intrinsic_text_width_slice(ctx, label, label_len, ts);
    float padding = (float)ts.font_size * WLX_GLYPH_ROW_LABEL_PADDING_FACTOR;
    return wlx_glyph_row_block_w(glyph_w, label_w, padding, reserve_empty_label);
}
static const float WLX_CHECKBOX_CHECK_PADDING_RATIO = 0.2f;
static const float WLX_CHECKBOX_CHECK_THICKNESS_RATIO = 0.12f;
static const float WLX_CHECKBOX_CHECK_GLOW_RATIO = 0.2f;
static const float WLX_CHECKBOX_CHECK_GLOW_ALPHA = 0.25f;

// Wheel-scroll pixels per delta unit, shared by scroll panels and the
// multiline inputbox so both feel identical under the same wheel.
static const float WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED = 20.0f;

// Scrollbar strip width when the theme leaves its knob unset.
static const float WLX_SCROLLBAR_FALLBACK_WIDTH = 10.0f;

// Shortest thumb a scrollbar draws. A proportional thumb over a long
// document (30k lines in a 20-row band) is a fraction of a pixel: the
// length floors here (or at the track, when the track is shorter) and
// the position maps the scroll range onto the track left over, so the
// thumb stays visible and draggable and the track end still means the
// content end.
static const float WLX_SCROLLBAR_MIN_THUMB = 20.0f;

// Thumb span along one scrollbar axis: length the viewport's share of
// the content (floored), position the scroll's share of the track left
// over. Both thumb gestures (wlx_thumb_drag_update and
// wlx_scrollbar_handle_drag) map the pointer over that same leftover
// length onto content minus viewport, so this is their exact inverse at
// every thumb length - a held thumb never creeps.
static inline void wlx_scrollbar_thumb_span(float track_l, float view_l,
    float content_l, float scroll, float *out_pos, float *out_len)
{
    float len = content_l > 0.0f ? (view_l / content_l) * track_l : track_l;
    float min_len = WLX_SCROLLBAR_MIN_THUMB < track_l ? WLX_SCROLLBAR_MIN_THUMB : track_l;
    if (len < min_len) len = min_len;
    if (len > track_l) len = track_l;
    float max_scroll = content_l - view_l;
    *out_pos = max_scroll > 0.0f ? (scroll / max_scroll) * (track_l - len) : 0.0f;
    *out_len = len;
}

// Vertical thumb rect at the track's right edge; the track is the
// viewport (both panel_rect.h tall). Shared by scroll panels and the
// text widgets.
static inline WLX_Rect wlx_scrollbar_rect(
    WLX_Rect panel_rect, float content_height, float scroll_offset, float scrollbar_width)
{
    float bar_y, bar_h;
    wlx_scrollbar_thumb_span(panel_rect.h, panel_rect.h, content_height, scroll_offset,
        &bar_y, &bar_h);
    return (WLX_Rect){
        panel_rect.x + panel_rect.w - scrollbar_width,
        panel_rect.y + bar_y,
        scrollbar_width,
        bar_h
    };
}

// Horizontal thumb rect along a strip-tall track under a view_w-wide
// band (the editor's horizontal bar).
static inline WLX_Rect wlx_scrollbar_rect_h(
    WLX_Rect track, float view_w, float content_width, float scroll_x)
{
    float bar_x, bar_w;
    wlx_scrollbar_thumb_span(track.w, view_w, content_width, scroll_x, &bar_x, &bar_w);
    return (WLX_Rect){ track.x + bar_x, track.y, bar_w, track.h };
}

static inline float wlx_clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// One raw-mouse scrollbar thumb gesture (press-hit -> drag_offset -> track
// mapping -> release), either axis. Raw mouse is deliberate: a focused text
// field owns active_id, which would starve a drag interaction of
// acquisition for as long as the field has focus (and no id also means no
// call-site collision). The thumb rect doubles as the press target and the
// mapping length; a caller with a frozen drag range passes the frozen
// thumb size and max_scroll while dragging (the press only ever sees the
// live rect - the range is frozen at the press, when both agree). When
// out_track_pos is non-NULL it receives the mapped thumb position on a
// live drag frame and is left untouched otherwise. Returns the updated
// scroll offset.
static float wlx_thumb_drag_update(WLX_Context *ctx, WLX_Rect track, WLX_Rect thumb,
    bool vertical, float max_scroll, float scroll,
    bool *dragging, float *drag_offset, float *out_track_pos)
{
    float mouse_p = vertical ? (float)ctx->input.mouse_y : (float)ctx->input.mouse_x;
    float track_p = vertical ? track.y : track.x;
    float track_l = vertical ? track.h - thumb.h : track.w - thumb.w;
    if (ctx->input.mouse_clicked && wlx_rect_contains(thumb,
            (float)ctx->input.mouse_x, (float)ctx->input.mouse_y)) {
        *dragging = true;
        *drag_offset = mouse_p - (vertical ? thumb.y : thumb.x);
    }
    if (*dragging && ctx->input.mouse_down) {
        if (track_l > 0.0f) {
            float pos = wlx_clampf(mouse_p - track_p - *drag_offset, 0.0f, track_l);
            scroll = (pos / track_l) * max_scroll;
            if (out_track_pos != NULL) *out_track_pos = pos;
        }
    } else if (!ctx->input.mouse_down) {
        *dragging = false;
    }
    return scroll;
}

// Drag-select auto-scroll: speed grows with the pointer's distance past the
// band edge (px/s per overshoot px), with the overshoot capped so a far
// fling cannot teleport the view in one frame.
static const float WLX_TEXT_DRAG_SCROLL_GAIN = 15.0f;
static const float WLX_TEXT_DRAG_SCROLL_MAX_OVERSHOOT = 60.0f;

static const float WLX_TEXT_CARET_WIDTH = 2.0f;
static const float WLX_TEXT_CARET_PADDING = 2.0f;
static const float WLX_TEXT_CARET_BLINK_PERIOD = 1.0f;
static const float WLX_TEXT_CARET_VISIBLE_FRACTION = 0.5f;

// True when the pointer belongs to the caller's layer: its topmost
// candidate (arbitrated at frame begin) sits on the layer currently being
// built, so an overlay under the pointer keeps base-layer consumers (wheel,
// tooltip) from reacting. Bootstrap frames have no arbitration data and
// gate nothing.
static inline bool wlx_pointer_on_current_layer(WLX_Context *ctx) {
    return !ctx->interaction.arbitrate
        || ctx->current_layer == ctx->interaction.pointer_layer;
}

// Consume the wheel on one axis when it can move: scroll toward the delta,
// clamp to [0, max], zero the context delta so enclosing panels do not
// double-scroll (innermost scrollable wins). The caller gates on
// hover/disabled; an axis without overflow leaves the delta untouched for
// the enclosing panel.
static bool wlx_wheel_consume_axis(WLX_Context *ctx, float *delta,
    float *value, float max, float speed)
{
    if (max <= 0.0f || *delta == 0.0f) return false;
    if (!wlx_pointer_on_current_layer(ctx)) return false;
    *value = wlx_clampf(*value - *delta * speed, 0.0f, max);
    *delta = 0.0f;
    return true;
}

static inline bool wlx_wheel_consume(WLX_Context *ctx, float *value, float max, float speed) {
    return wlx_wheel_consume_axis(ctx, &ctx->input.wheel_delta, value, max, speed);
}

// Horizontal-axis twin of wlx_wheel_consume, driven by wheel_delta_x.
static inline bool wlx_wheel_consume_x(WLX_Context *ctx, float *value, float max, float speed) {
    return wlx_wheel_consume_axis(ctx, &ctx->input.wheel_delta_x, value, max, speed);
}

static inline bool wlx_text_caret_blink_on(float blink_time) {
    return fmodf(blink_time, WLX_TEXT_CARET_BLINK_PERIOD)
        < WLX_TEXT_CARET_BLINK_PERIOD * WLX_TEXT_CARET_VISIBLE_FRACTION;
}

// Caret line inside a scissor clip, shared by the inputbox and both editor
// caret paths.
static void wlx_text_caret_draw(WLX_Context *ctx, WLX_Rect clip,
    float x, float top, float h, WLX_Color color)
{
    // Keep the whole caret body inside the clip: a column-0 caret lands
    // exactly on the clip's left edge, where a centered line loses half
    // its width and a one-pixel line sits at the mercy of the driver's
    // edge rounding (observed invisible on some SDL3 setups).
    if (clip.w >= WLX_TEXT_CARET_WIDTH) {
        x = wlx_clampf(x, clip.x + WLX_TEXT_CARET_WIDTH * 0.5f,
                       clip.x + clip.w - WLX_TEXT_CARET_WIDTH * 0.5f);
    }
    WLX_Scissor_Scope sc = wlx_scissor_scope_begin(ctx, clip);
    wlx_draw_line(ctx, x, top, x, top + h, WLX_TEXT_CARET_WIDTH, color);
    wlx_scissor_scope_end(ctx, sc);
}

// Thumb fill with hover/drag brightness, shared by every text-widget
// scrollbar.
static void wlx_scrollbar_thumb_draw(WLX_Context *ctx, WLX_Rect thumb, bool dragging) {
    bool hover = wlx_interaction_mouse_over(ctx, thumb);
    WLX_Color c = (dragging || hover)
        ? wlx_color_brightness(ctx->theme->scrollbar.bar, ctx->theme->hover_brightness)
        : ctx->theme->scrollbar.bar;
    wlx_draw_rect(ctx, thumb, c);
}

// Extra height guaranteed above the font plus vertical content padding when
// clamping a text field's requested height.
static const float WLX_TEXT_FIELD_MIN_HEIGHT_SLACK = 4.0f;
// Minimum field interior width in font_size units: a long label gives width
// back rather than starving the field to nothing.
static const int WLX_TEXT_FIELD_MIN_WIDTH_EM = 3;

// Box chrome inputs for a text field, lifted verbatim from the widget opt
// (WLX_TEXT_FIELD_CHROME below builds one).
typedef struct {
    WLX_Color back_color;
    WLX_Color border_color;
    WLX_Color border_focus_color;
    float border_width;
    float roundness;
    int   rounded_segments;
    float opacity;
    // Per-side border overrides, forwarded via WLX_BORDER_SIDES_ARGS.
    WLX_Color border_color_top, border_color_right;
    WLX_Color border_color_bottom, border_color_left;
    float border_width_top, border_width_right;
    float border_width_bottom, border_width_left;
    // Shadow/glow and absolute-corner fields, forwarded into WLX_Box_Style.
    WLX_SHADOW_FIELDS;
    WLX_GLOW_FIELDS;
    float corner_radius;
    int   rounded_corners;
} WLX_Text_Field_Chrome;

#define WLX_TEXT_FIELD_CHROME(opt) (WLX_Text_Field_Chrome){ \
    .back_color = (opt).back_color, .border_color = (opt).border_color, \
    .border_focus_color = (opt).border_focus_color, \
    .border_width = (opt).border_width, .roundness = (opt).roundness, \
    .rounded_segments = (opt).rounded_segments, .opacity = (opt).opacity, \
    .border_color_top = (opt).border_color_top, \
    .border_color_right = (opt).border_color_right, \
    .border_color_bottom = (opt).border_color_bottom, \
    .border_color_left = (opt).border_color_left, \
    .border_width_top = (opt).border_width_top, \
    .border_width_right = (opt).border_width_right, \
    .border_width_bottom = (opt).border_width_bottom, \
    .border_width_left = (opt).border_width_left, \
    WLX_BOX_STYLE_EFFECTS(opt), WLX_BOX_STYLE_CORNER(opt) }

typedef struct {
    WLX_Rect input_rect;   // field interior after label + min-width giveback
    float    label_width;  // final label width (post-clamp)
} WLX_Text_Field_Frame;

// Text-field frame prologue shared by the inputbox and the editor: measure
// and place the optional leading label (its y follows the vertical
// component of label_align), guarantee the minimum field width by giving
// label width back when the label would starve the field, derive the field
// rect, and draw the box chrome (hover-tint suppressed under focus, focus
// border-color swap). Interaction stays at the call site - the editor
// insets its zone by the gutter.
static WLX_Text_Field_Frame wlx_text_field_frame(WLX_Context *ctx, WLX_Rect wr,
    WLX_Resolved_Padding rp, const char *label, WLX_Text_Style ts,
    WLX_Align label_align, bool label_wrap, bool hover, bool focused, bool disabled,
    const WLX_Text_Field_Chrome *chrome)
{
    float label_width = 0;
    if (label != NULL && ts.font_size > 0) {
        size_t label_len = strlen(label);
        float label_h = 0;
        wlx_measure_text_slice(ctx, label, label_len, ts, &label_width, &label_h);
        label_width += rp.left;

        float label_x = wr.x + rp.left;
        float label_y;
        switch (label_align) {
            case WLX_TOP: case WLX_TOP_LEFT: case WLX_TOP_CENTER: case WLX_TOP_RIGHT:
                label_y = wr.y + rp.top;
                break;
            case WLX_BOTTOM: case WLX_BOTTOM_LEFT: case WLX_BOTTOM_CENTER: case WLX_BOTTOM_RIGHT:
                label_y = wr.y + wr.h - label_h - rp.bottom;
                break;
            default:
                label_y = wr.y + (wr.h - label_h) / 2;
                break;
        }
        WLX_Rect label_rect = { label_x, label_y, label_width, label_h };
        wlx_draw_text_fitted_slice(ctx, label_rect, label, label_len, ts,
            (WLX_Text_Prepare_Opt){ .align = label_align, .wrap = label_wrap });
    }

    float min_input_w = (float)(ts.font_size * WLX_TEXT_FIELD_MIN_WIDTH_EM);
    float horz_pad = rp.left + rp.right;
    if (label_width > 0 && (wr.w - label_width - horz_pad) < min_input_w) {
        label_width = wr.w - min_input_w - horz_pad;
        if (label_width < 0) label_width = 0;
    }
    float input_x = wr.x + label_width + rp.left;
    float input_w = wr.w - label_width - horz_pad;
    input_w = input_w < 0 ? 0 : input_w;
    float input_h = wr.h - rp.top - rp.bottom;
    if (input_h < 0) input_h = 0;
    WLX_Rect input_rect = { input_x, wr.y + rp.top, input_w, input_h };

    WLX_Color bg_color = wlx_color_hover_tint(
        chrome->back_color, hover && !focused, disabled,
        ctx->theme->hover_brightness * 0.5f);
    WLX_Color bdr_color = focused ? chrome->border_focus_color : chrome->border_color;
    WLX_Border_Sides sides = wlx_border_sides_for_widget(
        ctx->theme, false, disabled, chrome->opacity,
        bdr_color, chrome->border_width,
        WLX_BORDER_SIDES_ARGS(*chrome));
    wlx_draw_box(ctx, input_rect, (WLX_Box_Style){
        .fill            = bg_color,
        .border          = bdr_color,
        .border_width    = chrome->border_width,
        .roundness       = chrome->roundness,
        .rounded_segments = chrome->rounded_segments,
        .sides           = sides,
        .per_side        = true,
        WLX_BOX_STYLE_EFFECTS(*chrome),
        WLX_BOX_STYLE_CORNER(*chrome),
    });

    return (WLX_Text_Field_Frame){ .input_rect = input_rect, .label_width = label_width };
}

// Breathing margin kept between the caret ends and the field interior edges so
// the caret never sits on the border, even when the backend's reported line
// height exceeds the (border-inset) interior of a short field. Kept at 1px so a
// cramped field still yields a caret that matches the text extent (rather than
// shrinking well below it) while staying clear of the border.
static const float WLX_INPUTBOX_CARET_MARGIN = 1.0f;
// Fixed inner x-axis inset between the input box rect and the editable text.
// Preserves the historical `content_padding / 2` visual at the default
// content_padding of 10.
static const float WLX_TEXT_FIELD_INSET = 5.0f;

// Maximum gap between clicks (at the same text offset) that still counts as
// part of a double/triple-click sequence.
static const float WLX_TEXT_MULTI_CLICK_SECONDS = 0.4f;
// Cap on the multi-click clock accumulator so it cannot lose float
// precision over long sessions; anything past the multi-click gap reads
// the same.
static const float WLX_TEXT_MULTI_CLICK_CLOCK_CAP = 10.0f;

// Advance the multi-click detection clock by this frame, capped so the
// accumulator cannot lose float precision over long sessions.
static inline void wlx_text_edit_tick_click_clock(WLX_Context *ctx, WLX_Text_Edit_State *st) {
    st->last_click_time += wlx_get_frame_time(ctx);
    if (st->last_click_time > WLX_TEXT_MULTI_CLICK_CLOCK_CAP)
        st->last_click_time = WLX_TEXT_MULTI_CLICK_CLOCK_CAP;
}

// Password mask capacity: at most this many codepoints (one mask byte each)
// are rendered; longer plaintext keeps editing correctly but the visible
// mask stops growing.
#ifndef WLX_INPUTBOX_MASK_MAX
#define WLX_INPUTBOX_MASK_MAX 256
#endif

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

// Apply disabled-state visual transforms in place when `disabled` is true.
// Each color is shifted by `theme->disabled_brightness` (skipped when the
// theme value is WLX_UNSET), and `*opacity_ptr` is multiplied by
// `theme->disabled_opacity` (skipped when the theme value is negative).
// Intended to run after opacity resolution and before WLX_APPLY_OPACITY so
// the adjusted opacity is premultiplied into color alphas downstream.
#define WLX_APPLY_DISABLED(theme, disabled, opacity_ptr, /*WLX_Color* args*/ ...) do { \
    if (disabled) {                                                                    \
        const WLX_Theme *_wd_t = (theme);                                              \
        float _wd_b = _wd_t->disabled_brightness;                                      \
        if (!wlx_is_float_unset(_wd_b)) {                                              \
            WLX_Color *_wd_cs[] = { __VA_ARGS__ };                                     \
            for (size_t _wd_i = 0; _wd_i < sizeof(_wd_cs)/sizeof(_wd_cs[0]); ++_wd_i)  \
                *_wd_cs[_wd_i] = wlx_color_brightness(*_wd_cs[_wd_i], _wd_b);          \
        }                                                                              \
        float _wd_o = _wd_t->disabled_opacity;                                         \
        if (_wd_o >= 0.0f && (opacity_ptr) != NULL) {                                  \
            *(opacity_ptr) = *(opacity_ptr) * _wd_o;                                   \
        }                                                                              \
    }                                                                                  \
} while (0)

// Unified resolver tail: resolve opacity, apply disabled-state transforms,
// then premultiply opacity into the color list. The color list is named
// exactly once, eliminating the "added a color to one list but forgot the
// other" drift target. Pass `disabled = false` for purely decorative
// resolvers (label, progress, image, widget, scroll-panel) so the call
// shape is symmetric across all widgets.
#define WLX_RESOLVE_VISUAL_STATE(ctx, opt_ptr, disabled, /*WLX_Color* args*/ ...) do { \
    (opt_ptr)->opacity = wlx_resolve_opacity_for((ctx), (opt_ptr)->opacity);           \
    WLX_APPLY_DISABLED((ctx)->theme, (disabled), &(opt_ptr)->opacity, __VA_ARGS__);    \
    WLX_APPLY_OPACITY((opt_ptr)->opacity, __VA_ARGS__);                                \
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
    WLX_Font *font, int *font_size, float *min_height)
{
    if (*font      == WLX_FONT_DEFAULT) *font      = theme->font;
    if (*font_size <= 0)                *font_size = theme->font_size;
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
    WLX_RESOLVE_VISUAL_STATE(ctx, opt, false, &opt->back_color, &opt->border_color,
        &opt->shadow_color, &opt->glow_color,
        &opt->gradient_top, &opt->gradient_bottom);
}

// Forward declarations for shared image helpers used by wlx_label_impl below.
// Definitions live further down alongside wlx_button_impl.
static void wlx_resolve_image_fit(WLX_Rect target, WLX_Rect src, WLX_Image_Scale scale,
    WLX_Align align, WLX_Rect *out_src, WLX_Rect *out_dst);
static inline bool wlx_widget_has_image(WLX_Texture texture);
static inline WLX_Rect wlx_widget_image_src(WLX_Texture texture, WLX_Rect texture_src);
static inline float wlx_widget_auto_image_size(float image_size, WLX_Image_Placement placement,
    WLX_Rect widget_rect, float font_size, bool has_text);
static inline void wlx_widget_layout_image_text(WLX_Rect widget_rect, WLX_Image_Placement placement,
    float image_size, float gap, float text_w, float text_h, WLX_Align align,
    WLX_Rect *out_image_rect, WLX_Rect *out_text_rect);

// Content payload for image-capable text widgets (label, button): everything
// wlx_draw_widget_content needs to emit the image and/or text block into a
// content rect.
typedef struct {
    WLX_Texture         texture;
    WLX_Rect            texture_src;
    WLX_Image_Scale     texture_scale;
    WLX_Color           texture_tint;
    WLX_Image_Placement image_placement;
    float               image_size;
    float               image_text_gap;
    int                 font_size;  // auto-image-size basis: the resolved
                                    // widget font size, which may differ from
                                    // ts.font_size when a style aggregate wins
    WLX_Align           align;
    bool                wrap;
    WLX_Vertical_Metric vmetric;
} WLX_Widget_Content;

static void wlx_draw_widget_content(WLX_Context *ctx, WLX_Rect content_rect,
    const char *text, size_t text_len, WLX_Text_Style ts, WLX_Widget_Content c);

// Shared resolver tail for image-capable text widgets: default tint and
// font-relative image/text gap. image_size <= 0 stays unresolved; it is
// resolved against the content rect at draw time.
static inline void wlx_resolve_widget_content(WLX_Color *texture_tint,
    float *image_text_gap, int font_size)
{
    if (wlx_color_is_zero(*texture_tint)) *texture_tint = WLX_WHITE;
    if (*image_text_gap < 0)              *image_text_gap = (float)font_size * 0.5f;
}

static void wlx_resolve_opt_label(const WLX_Context *ctx, WLX_Label_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->surface;
    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);

    wlx_resolve_widget_content(&opt->texture_tint, &opt->image_text_gap, opt->font_size);

    WLX_RESOLVE_VISUAL_STATE(ctx, opt, false,
        &opt->front_color, &opt->back_color, &opt->border_color, &opt->texture_tint,
        &opt->shadow_color, &opt->glow_color);
}

WLXDEF void wlx_label_impl(WLX_Context *ctx, const char *text, WLX_Label_Opt opt, const char *file, int line)
{
#ifdef WLX_DEBUG
    int _wlx_label_user_font_size = opt.font_size;
#endif
    wlx_resolve_opt_label(ctx, &opt);

    size_t text_len = (text != NULL) ? strlen(text) : 0;
    WLX_Widget_Layout wly = WLX_WIDGET_LAYOUT(opt);
    if (wly.width <= 0 && wlx_parent_wants_intrinsic_width(ctx, wly.pos)) {
        // Single-line unwrapped measure even under .wrap: the contribution
        // is a pure function of content, so a fit slot sized to it renders
        // the text unwrapped (a MAX clamp wraps inside the clamp, stably).
        WLX_Text_Style mts = (opt.style.font_size > 0)
            ? opt.style
            : (WLX_Text_Style){ .font = opt.font, .font_size = opt.font_size,
                                .spacing = opt.spacing };
        wly.intrinsic_w = wlx_intrinsic_text_image_width(ctx, text, text_len, mts,
                opt.texture, opt.texture_src, opt.image_size,
                opt.image_placement, opt.image_text_gap)
            + WLX_INTRINSIC_PAD_LR(ctx, opt);
    }

    // Prologue: compute widget frame and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, wly, file, line);
    WLX_Rect wr = frame.rect;

    WLX_Rect content_rect = WLX_RESOLVE_CONTENT_RECT(ctx, opt, wr);

    // wlx_label intentionally does not carry .disabled (see ADR_018
    // coverage matrix); the _for form with disabled=false keeps the
    // call shape consistent with the disabled-aware widgets and makes
    // a future opt-in a one-line change.
    WLX_Interaction inter = wlx_get_interaction_for(
        ctx,
        wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD
            | WLX_INTERACT_TAB_SKIP,
        false,
        file, line
    );

    WLX_Border_Sides label_sides = wlx_border_sides_for_widget(
        ctx->theme, false, false, opt.opacity,
        opt.border_color, opt.border_width,
        WLX_BORDER_SIDES_ARGS(opt));
    bool label_has_border = label_sides.top.width > 0 || label_sides.right.width > 0
                         || label_sides.bottom.width > 0 || label_sides.left.width > 0;
    if (opt.show_background || label_has_border) {
        WLX_Color bg = opt.show_background
            ? wlx_color_hover_tint(opt.back_color, inter.hover, inter.disabled, ctx->theme->hover_brightness)
            : (WLX_Color){0};
        wlx_draw_box(ctx, (WLX_Rect){wr.x, wr.y, wr.w, wr.h}, (WLX_Box_Style){
            .fill            = bg,
            .border          = opt.border_color,
            .border_width    = opt.border_width,
            .roundness       = opt.roundness,
            .rounded_segments = opt.rounded_segments,
            .sides           = label_sides,
            .per_side        = true,
            WLX_BOX_STYLE_EFFECTS(opt),
            WLX_BOX_STYLE_CORNER(opt),
        });
    }

    WLX_Text_Style ts;
    if (opt.style.font_size > 0) {
#ifdef WLX_DEBUG
        if (_wlx_label_user_font_size > 0) {
            WLX_DBG(warn_once, ctx, file, line,
                "wlx_label: .style and .font_size both set; .style wins");
        }
#endif
        ts = opt.style;
        if (ts.color.a == 0 && ts.color.r == 0 && ts.color.g == 0 && ts.color.b == 0) {
            ts.color = opt.front_color;
        }
    } else {
        ts = (WLX_Text_Style){
            .font      = opt.font,
            .font_size = opt.font_size,
            .color     = opt.front_color,
            .spacing   = opt.spacing,
        };
    }

    // No text and no image: chrome-only (if configured); label remains non-interactive.
    wlx_draw_widget_content(ctx, content_rect, text, text_len, ts, (WLX_Widget_Content){
        .texture         = opt.texture,
        .texture_src     = opt.texture_src,
        .texture_scale   = opt.texture_scale,
        .texture_tint    = opt.texture_tint,
        .image_placement = opt.image_placement,
        .image_size      = opt.image_size,
        .image_text_gap  = opt.image_text_gap,
        .font_size       = opt.font_size,
        .align           = opt.content_align,
        .wrap            = opt.wrap,
        .vmetric         = opt.vertical_metric,
    });

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);
}

// Resolve drawing rects for an image fitted into a target rect using the
// given scale mode and alignment. Inputs: target rect to fit into and a
// normalized source rect (w, h > 0). Outputs: source rect to sample (FILL
// may crop) and destination rect to draw into. Shared between wlx_image
// and the image branch of wlx_button.
static void wlx_resolve_image_fit(
    WLX_Rect target,
    WLX_Rect src,
    WLX_Image_Scale scale,
    WLX_Align align,
    WLX_Rect *out_src,
    WLX_Rect *out_dst)
{

    WLX_Rect s = src;
    WLX_Rect dst = target;
    switch (scale) {
        case WLX_IMAGE_SCALE_STRETCH:
            dst = target;
            break;

        case WLX_IMAGE_SCALE_NONE:
            dst = wlx_get_align_rect(target, s.w, s.h, align);
            break;

        case WLX_IMAGE_SCALE_FIT: {
            float sx = target.w / s.w;
            float sy = target.h / s.h;
            float k  = sx < sy ? sx : sy;
            dst = wlx_get_align_rect(target, s.w * k, s.h * k, align);
            break;
        }

        case WLX_IMAGE_SCALE_FILL: {
            // Narrow s to match target's aspect; align selects which edge survives.
            if (target.h > 0 && s.h > 0) {
                float t_aspect = target.w / target.h;
                float s_aspect = s.w  / s.h;
                if (s_aspect > t_aspect) {
                    float new_w = s.h * t_aspect;
                    switch (align) {
                        case WLX_LEFT: case WLX_TOP_LEFT: case WLX_BOTTOM_LEFT:
                            break;
                        case WLX_RIGHT: case WLX_TOP_RIGHT: case WLX_BOTTOM_RIGHT:
                            s.x += (s.w - new_w);
                            break;
                        default:
                            s.x += (s.w - new_w) * 0.5f;
                            break;
                    }
                    s.w = new_w;
                } else if (s_aspect < t_aspect) {
                    float new_h = s.w / t_aspect;
                    switch (align) {
                        case WLX_TOP: case WLX_TOP_LEFT: case WLX_TOP_RIGHT: case WLX_TOP_CENTER:
                            break;
                        case WLX_BOTTOM: case WLX_BOTTOM_LEFT: case WLX_BOTTOM_RIGHT: case WLX_BOTTOM_CENTER:
                            s.y += (s.h - new_h);
                            break;
                        default:
                            s.y += (s.h - new_h) * 0.5f;
                            break;
                    }
                    s.h = new_h;
                }
            }
            dst = target;
            break;
        }
    }
    *out_src = s;
    *out_dst = dst;
}

// True when a widget option carries a drawable texture.
static inline bool wlx_widget_has_image(WLX_Texture texture) {
    return texture.width > 0 && texture.height > 0;
}

// True when both checkbox state textures are drawable. Texture mode is
// gated on this so a half-configured checkbox cannot draw a partial
// texture path; if either state is missing, the native path runs instead.
static inline bool wlx_checkbox_texture_mode_active(WLX_Texture checked, WLX_Texture unchecked) {
    return wlx_widget_has_image(checked) && wlx_widget_has_image(unchecked);
}

// Resolve a widget's image source rect. An unset texture_src (w/h <= 0)
// means use the full texture.
static inline WLX_Rect wlx_widget_image_src(WLX_Texture texture, WLX_Rect texture_src) {
    WLX_Rect s = texture_src;
    if (s.w <= 0 || s.h <= 0) {
        s = (WLX_Rect){ 0, 0, (float)texture.width, (float)texture.height };
    }
    return s;
}

// Compute a reserved square size for a widget's image content.
//   image_size > 0      -> caller-specified, used as-is.
//   has_text + horiz    -> derived from font_size, clamped to widget height.
//   has_text + vert     -> derived from font_size, clamped to widget width.
//   image-only          -> smaller of widget width/height.
static inline float wlx_widget_auto_image_size(
    float image_size,
    WLX_Image_Placement placement,
    WLX_Rect widget_rect,
    float font_size,
    bool has_text)
{
    if (image_size > 0) return image_size;

    if (!has_text) {
        float m = widget_rect.w < widget_rect.h ? widget_rect.w : widget_rect.h;
        return m > 0 ? m : 0;
    }
    float s = font_size * 1.5f;

    bool horizontal = (placement == WLX_IMAGE_PLACEMENT_LEFT || placement == WLX_IMAGE_PLACEMENT_RIGHT);

    float clamp = horizontal ? widget_rect.h : widget_rect.w;
    if (clamp > 0 && s > clamp) s = clamp;

    return s > 0 ? s : 0;
}

// Compute image and text rects for an image+text widget. The combined block
// (image + gap + text) is aligned inside widget_rect using `align`; the
// reserved image rect is square (image_size x image_size) and centered on
// the cross axis within the block.
static inline void wlx_widget_layout_image_text(
    WLX_Rect widget_rect,
    WLX_Image_Placement placement,
    float image_size,
    float gap,
    float text_w,
    float text_h,
    WLX_Align align,
    WLX_Rect *out_image_rect,
    WLX_Rect *out_text_rect)
{
    bool horizontal = (placement == WLX_IMAGE_PLACEMENT_LEFT || placement == WLX_IMAGE_PLACEMENT_RIGHT);

    float block_w, block_h;
    if (horizontal) {
        block_w = image_size + gap + text_w;
        block_h = image_size > text_h ? image_size : text_h;
    } else {
        block_w = image_size > text_w ? image_size : text_w;
        block_h = image_size + gap + text_h;
    }
    if (block_w > widget_rect.w) block_w = widget_rect.w;
    if (block_h > widget_rect.h) block_h = widget_rect.h;
    if (block_w < 0) block_w = 0;
    if (block_h < 0) block_h = 0;

    WLX_Rect block = wlx_get_align_rect(widget_rect, block_w, block_h, align);

    WLX_Rect img, txt;
    if (horizontal) {
        float img_y = block.y + (block.h - image_size) * 0.5f;
        if (img_y < block.y) img_y = block.y;
        float available_text_w = block.w - image_size - gap;
        if (available_text_w < 0) available_text_w = 0;
        if (placement == WLX_IMAGE_PLACEMENT_LEFT) {
            img = (WLX_Rect){ block.x, img_y, image_size, image_size };
            txt = (WLX_Rect){ block.x + image_size + gap, block.y, available_text_w, block.h };
        } else {
            txt = (WLX_Rect){ block.x, block.y, available_text_w, block.h };
            img = (WLX_Rect){ block.x + available_text_w + gap, img_y, image_size, image_size };
        }
    } else {
        float img_x = block.x + (block.w - image_size) * 0.5f;
        if (img_x < block.x) img_x = block.x;
        float available_text_h = block.h - image_size - gap;
        if (available_text_h < 0) available_text_h = 0;
        if (placement == WLX_IMAGE_PLACEMENT_TOP) {
            img = (WLX_Rect){ img_x, block.y, image_size, image_size };
            txt = (WLX_Rect){ block.x, block.y + image_size + gap, block.w, available_text_h };
        } else {
            txt = (WLX_Rect){ block.x, block.y, block.w, available_text_h };
            img = (WLX_Rect){ img_x, block.y + available_text_h + gap, image_size, image_size };
        }
    }

    *out_image_rect = img;
    *out_text_rect = txt;
}

// Crop a textured blit (src -> dst) to `bounds` geometrically, without a
// scissor: intersect dst with bounds, then narrow src proportionally so the
// surviving destination still samples the matching sub-rectangle of the
// source. Returns false when dst falls fully outside bounds (nothing to draw).
// This lets a fixed-size image keep its requested size and crop at the widget
// edge when the widget is squeezed below it, instead of scaling the image down.
// A blit already inside bounds is left untouched (intersection is a no-op).
static bool wlx_clip_textured_blit(WLX_Rect bounds, WLX_Rect *src, WLX_Rect *dst) {
    float x0 = dst->x > bounds.x ? dst->x : bounds.x;
    float y0 = dst->y > bounds.y ? dst->y : bounds.y;
    float dx1 = dst->x + dst->w, dy1 = dst->y + dst->h;
    float bx1 = bounds.x + bounds.w, by1 = bounds.y + bounds.h;
    float x1 = dx1 < bx1 ? dx1 : bx1;
    float y1 = dy1 < by1 ? dy1 : by1;
    // Strict: a zero-extent overlap (edge-touching, or a degenerate zero-size
    // destination already inside bounds) still draws as a harmless no-op, the
    // same as before clipping; only a strictly negative overlap (dst entirely
    // past an edge) is dropped, which would otherwise flip the rect.
    if (x1 < x0 || y1 < y0) return false;

    if (dst->w > 0) {
        float sx = src->w / dst->w;
        src->x += (x0 - dst->x) * sx;
        src->w  = (x1 - x0) * sx;
    }
    if (dst->h > 0) {
        float sy = src->h / dst->h;
        src->y += (y0 - dst->y) * sy;
        src->h  = (y1 - y0) * sy;
    }
    dst->x = x0; dst->y = y0;
    dst->w = x1 - x0; dst->h = y1 - y0;
    return true;
}

// Emit the content of an image-capable text widget into its content rect.
// Three branches: image + text (measure text, auto-size the image, lay out
// the combined block, fit, draw both), image only, text only. No text and no
// image draws nothing; the chrome-only contract stays with the caller. In the
// image + text branch the image keeps its requested size and is cropped to
// content_rect on overflow (widget drawing is not scissored), so a squeezed
// control clips its glyph at the edge rather than shrinking it.
static void wlx_draw_widget_content(WLX_Context *ctx, WLX_Rect content_rect,
    const char *text, size_t text_len, WLX_Text_Style ts, WLX_Widget_Content c)
{
    bool has_text  = text_len > 0;
    bool has_image = wlx_widget_has_image(c.texture);

    if (has_image && has_text) {
        float text_w = 0.0f, text_h = 0.0f;
        wlx_measure_text_slice(ctx, text, text_len, ts, &text_w, &text_h);

        float image_size = wlx_widget_auto_image_size(c.image_size, c.image_placement,
            content_rect, (float)c.font_size, true);

        WLX_Rect image_rect, text_rect;
        wlx_widget_layout_image_text(content_rect, c.image_placement, image_size,
            c.image_text_gap, text_w, text_h, c.align, &image_rect, &text_rect);

        WLX_Rect src = wlx_widget_image_src(c.texture, c.texture_src);
        WLX_Rect tex_src, tex_dst;
        wlx_resolve_image_fit(image_rect, src, c.texture_scale, WLX_CENTER, &tex_src, &tex_dst);
        if (wlx_clip_textured_blit(content_rect, &tex_src, &tex_dst))
            wlx_draw_texture(ctx, c.texture, tex_src, tex_dst, c.texture_tint);

        wlx_draw_text_fitted_slice(ctx, text_rect, text, text_len, ts,
            (WLX_Text_Prepare_Opt){ .align = c.align, .wrap = c.wrap, .vmetric = c.vmetric });
    } else if (has_image) {
        // image-only sizes its target through wlx_get_align_rect, which already
        // clamps to content_rect, so the draw cannot overflow the widget and
        // needs no extra crop.
        WLX_Rect target = (c.image_size > 0)
            ? wlx_get_align_rect(content_rect, c.image_size, c.image_size, c.align)
            : content_rect;
        WLX_Rect src = wlx_widget_image_src(c.texture, c.texture_src);
        WLX_Rect tex_src, tex_dst;
        wlx_resolve_image_fit(target, src, c.texture_scale, c.align, &tex_src, &tex_dst);
        wlx_draw_texture(ctx, c.texture, tex_src, tex_dst, c.texture_tint);
    } else if (has_text) {
        wlx_draw_text_fitted_slice(ctx, content_rect, text, text_len, ts,
            (WLX_Text_Prepare_Opt){ .align = c.align, .wrap = c.wrap, .vmetric = c.vmetric });
    }
}

static void wlx_resolve_opt_button(const WLX_Context *ctx, WLX_Button_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;

    if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->surface;

    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);

    if (wlx_is_float_unset(opt->hover_brightness))
        opt->hover_brightness = theme->hover_brightness;

    wlx_resolve_widget_content(&opt->texture_tint, &opt->image_text_gap, opt->font_size);

    WLX_RESOLVE_VISUAL_STATE(ctx, opt, opt->disabled,
        &opt->front_color, &opt->back_color, &opt->border_color, &opt->texture_tint,
        &opt->shadow_color, &opt->glow_color);
}

// The button face for an already-resolved WLX_Button_Opt: widget frame,
// HOVER|CLICK|KEYBOARD query, hover tint, per-side border, box, text/image
// content, frame end. Shared by wlx_button, the dropdown face and the
// menu-button face, so a face feature lands once. It never resolves -
// every caller resolves exactly once beforehand (a second
// WLX_RESOLVE_VISUAL_STATE would dim a disabled face twice). `id` is the
// frame's scope id: the button's own, NULL for the popups (their scope is
// already pushed). Returns the interaction; `out_rect` (optional) receives
// the widget rect.
static WLX_Interaction wlx_button_face(WLX_Context *ctx,
    const char *text, size_t text_len, const WLX_Button_Opt *opt,
    WLX_Widget_Layout wly, const char *id, WLX_Rect *out_rect,
    const char *file, int line)
{
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, id, wly, file, line);
    WLX_Rect wr = frame.rect;
    WLX_Rect content_rect = WLX_RESOLVE_CONTENT_RECT(ctx, *opt, wr);

    WLX_Interaction inter = wlx_get_interaction_for(
        ctx,
        wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        opt->disabled,
        file, line
    );

    // Per-call hover override: an explicit hover_back_color replaces the fill
    // while hovered; otherwise the resolved hover_brightness drives the tint
    // (a negative value darkens, e.g. for an already-bright accent fill).
    WLX_Color bg;
    if (!wlx_color_is_zero(opt->hover_back_color) && inter.hover && !inter.disabled) {
        bg = opt->hover_back_color;
    } else {
        bg = wlx_color_hover_tint(
            opt->back_color, inter.hover, inter.disabled, opt->hover_brightness);
    }

    WLX_Border_Sides sides = wlx_border_sides_for_widget(
        ctx->theme, false, inter.disabled, opt->opacity,
        opt->border_color, opt->border_width,
        WLX_BORDER_SIDES_ARGS(*opt));

    wlx_draw_box(ctx, wr, (WLX_Box_Style){
        .fill             = bg,
        .border           = opt->border_color,
        .border_width     = opt->border_width,
        .roundness        = opt->roundness,
        .rounded_segments = opt->rounded_segments,
        .sides            = sides,
        .per_side         = true,
        WLX_BOX_STYLE_EFFECTS(*opt),
        WLX_BOX_STYLE_CORNER(*opt),
    });

    WLX_Text_Style ts = {
        .font      = opt->font,
        .font_size = opt->font_size,
        .color     = opt->front_color,
        .spacing   = opt->spacing,
    };

    // No text and no image: chrome-only; click contract preserved.
    // Face text centers on the line-height metric; the cap-height knob
    // (.vertical_metric) is a label-only presentation option.
    wlx_draw_widget_content(ctx, content_rect, text, text_len, ts, (WLX_Widget_Content){
        .texture         = opt->texture,
        .texture_src     = opt->texture_src,
        .texture_scale   = opt->texture_scale,
        .texture_tint    = opt->texture_tint,
        .image_placement = opt->image_placement,
        .image_size      = opt->image_size,
        .image_text_gap  = opt->image_text_gap,
        .font_size       = opt->font_size,
        .align           = opt->content_align,
        .wrap            = opt->wrap,
        .vmetric         = WLX_VMETRIC_LINE_HEIGHT,
    });

    wlx_widget_frame_end(ctx, frame);
    if (out_rect != NULL) *out_rect = wr;
    return inter;
}

WLXDEF bool wlx_button_impl(WLX_Context *ctx, const char *text, WLX_Button_Opt opt, const char *file, int line)
{
    wlx_resolve_opt_button(ctx, &opt);

    size_t text_len = (text != NULL) ? strlen(text) : 0;
    WLX_Widget_Layout wly = WLX_WIDGET_LAYOUT(opt);
    if (wly.width <= 0 && wlx_parent_wants_intrinsic_width(ctx, wly.pos)) {
        WLX_Text_Style mts = { .font = opt.font, .font_size = opt.font_size,
                               .spacing = opt.spacing };
        wly.intrinsic_w = wlx_intrinsic_text_image_width(ctx, text, text_len, mts,
                opt.texture, opt.texture_src, opt.image_size,
                opt.image_placement, opt.image_text_gap)
            + WLX_INTRINSIC_PAD_LR(ctx, opt);
    }

    return wlx_button_face(ctx, text, text_len, &opt, wly, opt.id, NULL, file, line).clicked;
}

// Row geometry produced by wlx_layout_glyph_row for glyph + label compounds.
typedef struct {
    WLX_Rect glyph;   // glyph cell at the left edge of the aligned block
    WLX_Rect text;    // trailing label rect (right of the glyph + padding)
    WLX_Rect block;   // combined block (glyph + padding + label), aligned
    float label_w;    // measured label extents (0 when not measured)
    float label_h;
} WLX_Glyph_Row;

// Shared row layout for glyph + label compound widgets (checkbox, toggle,
// radio): measures the label, sizes the combined block (glyph + padding +
// label) against the tallest part, aligns it inside content_rect, and splits
// it into the glyph and trailing text rects.
//   reserve_empty_label - measure even a missing label and keep the label
//       padding in the block (checkbox's historical row shape); when false
//       the label space collapses for empty labels (toggle, radio).
//   center_glyph - center the glyph vertically in the block (toggle, radio);
//       when false the glyph keeps the block's top edge (checkbox).
static WLX_Glyph_Row wlx_layout_glyph_row(WLX_Context *ctx, WLX_Rect content_rect,
    float glyph_w, float glyph_h, const char *label, size_t label_len,
    WLX_Text_Style ts, WLX_Align align, bool reserve_empty_label, bool center_glyph)
{
    WLX_Glyph_Row row = {0};
    float padding = (float)ts.font_size * WLX_GLYPH_ROW_LABEL_PADDING_FACTOR;

    if (reserve_empty_label || (label != NULL && ts.font_size > 0)) {
        wlx_measure_text_slice(ctx, label, label_len, ts, &row.label_w, &row.label_h);
    }

    float block_w = wlx_glyph_row_block_w(glyph_w, row.label_w, padding, reserve_empty_label);
    float block_h = glyph_h > row.label_h ? glyph_h : row.label_h;
    row.block = wlx_get_align_rect(content_rect, block_w, block_h, align);

    row.glyph = (WLX_Rect){
        row.block.x,
        center_glyph ? row.block.y + (row.block.h - glyph_h) / 2.0f : row.block.y,
        glyph_w,
        glyph_h,
    };
    row.text = (WLX_Rect){
        row.block.x + glyph_w + padding,
        row.block.y,
        row.block.w - glyph_w - padding,
        row.block.h,
    };
    return row;
}

static void wlx_resolve_opt_checkbox(const WLX_Context *ctx, WLX_Checkbox_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;

    if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->surface;
    if (wlx_color_is_zero(opt->check_color)) opt->check_color = theme->checkbox.check;
    if (wlx_color_is_zero(opt->tex_checked_tint))   opt->tex_checked_tint   = WLX_WHITE;
    if (wlx_color_is_zero(opt->tex_unchecked_tint)) opt->tex_unchecked_tint = WLX_WHITE;
    // Widget-specific border fallbacks before common helper
    if (wlx_color_is_zero(opt->border_color))         opt->border_color = theme->checkbox.border;
    if (wlx_is_negative_unset(opt->border_width)) opt->border_width = theme->checkbox.border_width;

    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);

    WLX_RESOLVE_VISUAL_STATE(ctx, opt, opt->disabled,
        &opt->front_color, &opt->back_color, &opt->border_color, &opt->check_color,
        &opt->tex_checked_tint, &opt->tex_unchecked_tint,
        &opt->shadow_color, &opt->glow_color);
}

WLXDEF bool wlx_checkbox_impl(WLX_Context *ctx, const char *text, bool *checked, WLX_Checkbox_Opt opt, const char *file, int line)
{
    wlx_resolve_opt_checkbox(ctx, &opt);

    size_t text_len = (text != NULL) ? strlen(text) : 0;
    WLX_Widget_Layout wly = WLX_WIDGET_LAYOUT(opt);
    if (wly.width <= 0 && wlx_parent_wants_intrinsic_width(ctx, wly.pos)) {
        // font_size is the glyph's upper bound (the drawn box clamps to
        // the content height), so the slot is never undersized.
        WLX_Text_Style mts = { .font = opt.font, .font_size = opt.font_size,
                               .spacing = opt.spacing };
        wly.intrinsic_w = wlx_intrinsic_glyph_row_width(ctx,
                (float)opt.font_size, text, text_len, mts, true)
            + WLX_INTRINSIC_PAD_LR(ctx, opt);
    }

    // Prologue: compute widget frame and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, wly, file, line);
    WLX_Rect wr = frame.rect;

    WLX_Rect content_rect = WLX_RESOLVE_CONTENT_RECT(ctx, opt, wr);

    // Draw checkbox
    float checkbox_size = (content_rect.h > opt.font_size) ? opt.font_size : content_rect.h * WLX_CHECKBOX_SIZE_RATIO;
    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size, .color = opt.front_color, .spacing = opt.spacing };
    WLX_Glyph_Row row = wlx_layout_glyph_row(ctx, content_rect, checkbox_size, checkbox_size,
        text, text_len, ts, opt.content_align, true, false);
    WLX_Rect hit_rect = (opt.full_slot_hit) ? wr : row.block;

    WLX_Interaction inter = wlx_get_interaction_for(
        ctx,
        hit_rect,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        opt.disabled,
        file, line
    );

    WLX_Rect checkbox_rect = row.glyph;

    if (inter.clicked && checked != NULL) {
        *checked = !(*checked);
    }

    WLX_Color checkbox_bg = wlx_color_hover_tint(
        opt.back_color, inter.hover, inter.disabled, ctx->theme->hover_brightness);

    if (wlx_checkbox_texture_mode_active(opt.tex_checked, opt.tex_unchecked)) {
        bool        is_checked   = (checked != NULL && *checked);
        WLX_Texture current_tex  = is_checked ? opt.tex_checked      : opt.tex_unchecked;
        WLX_Rect    current_src  = is_checked ? opt.tex_checked_src  : opt.tex_unchecked_src;
        WLX_Color   current_tint = is_checked ? opt.tex_checked_tint : opt.tex_unchecked_tint;
        WLX_Rect    src          = wlx_widget_image_src(current_tex, current_src);
        wlx_draw_texture(
            ctx,
            current_tex,
            src,
            (WLX_Rect){checkbox_rect.x, checkbox_rect.y, checkbox_rect.w, checkbox_rect.h},
            current_tint
        );
    } else {
        // Native mode - draw box + checkmark lines
        WLX_Border_Sides checkbox_sides = wlx_border_sides_for_widget(
            ctx->theme, false, inter.disabled, opt.opacity,
            opt.border_color, opt.border_width,
            WLX_BORDER_SIDES_ARGS(opt));
        wlx_draw_box(ctx, checkbox_rect, (WLX_Box_Style){
            .fill            = checkbox_bg,
            .border          = opt.border_color,
            .border_width    = opt.border_width,
            .roundness       = opt.roundness,
            .rounded_segments = opt.rounded_segments,
            .sides           = checkbox_sides,
            .per_side        = true,
            WLX_BOX_STYLE_EFFECTS(opt),
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

    WLX_Rect text_rect = row.text;
    // The checkbox label may use the full remaining content height below the
    // block's top edge, not just the block height.
    text_rect.h = content_rect.y + content_rect.h - row.block.y;
    wlx_draw_text_fitted_slice(ctx, text_rect, text, text_len, ts,
        (WLX_Text_Prepare_Opt){ .align = WLX_ALIGN_NONE, .wrap = opt.wrap });

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
    if (wlx_color_is_zero(opt->selection_color))    opt->selection_color    = theme->input.selection;
    // Custom themes that predate the selection field fall back to a
    // translucent accent so the highlight is never invisible.
    if (wlx_color_is_zero(opt->selection_color)) {
        opt->selection_color = theme->accent;
        opt->selection_color.a = 90;
    }

    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);

    // A masked field is always single-line: the mask has no meaningful line
    // structure and wrapping it would leak nothing but look broken. The same
    // reasoning excludes multiline editing.
    if (opt->password) {
        opt->wrap = false;
        opt->multiline = false;
    }

    // Icon tint defaults to white and the icon/text gap to a font-relative
    // value. The tint is an image tint, not a chrome color, so it stays out of
    // the visual-state pass below (matching the label/button image path).
    wlx_resolve_widget_content(&opt->texture_tint, &opt->image_text_gap, opt->font_size);

    WLX_RESOLVE_VISUAL_STATE(ctx, opt, opt->disabled,
        &opt->front_color, &opt->back_color, &opt->border_color,
        &opt->border_focus_color, &opt->cursor_color, &opt->selection_color,
        &opt->shadow_color, &opt->glow_color);
}

// Resolve the visual line (as produced by the wrapped text layout) that holds
// cursor_offset, reporting its cursor range: line start, and line end before
// any newline separator. Runs the same line pipeline as the caret and the
// text draw, so the jump targets always match what is on screen.
static void wlx_inputbox_visual_line_bounds_from_lines(const char *text, size_t length,
    const WLX_Text_Line_Record *lines, size_t line_count, size_t cursor_offset,
    size_t *out_start, size_t *out_end)
{
    if (out_start) *out_start = 0;
    if (out_end) *out_end = length;

    if (lines == NULL || line_count == 0) {
        if (out_end) *out_end = 0;
        return;
    }

    cursor_offset = wlx_text_normalize_cursor_offset(text, length, cursor_offset);
    const WLX_Text_Line_Record *hit = &lines[line_count - 1];
    for (size_t i = 0; i < line_count; i++) {
        if (wlx_text_cursor_is_on_line(&lines[i], cursor_offset)) {
            hit = &lines[i];
            break;
        }
    }
    if (out_start) *out_start = hit->cursor_start;
    if (out_end) *out_end = hit->cursor_end;
}

// Test seam: the widget resolves bounds through its frame's shared
// records; the suites call this to probe the same geometry without a frame.
static inline bool wlx_inputbox_visual_line_bounds(WLX_Context *ctx, WLX_Rect rect, const char *text, size_t length,
    WLX_Text_Style style, WLX_Align align, bool wrap, size_t cursor_offset,
    size_t *out_start, size_t *out_end)
{
    if (out_start) *out_start = 0;
    if (out_end) *out_end = length;

    WLX_Text_Prepared p;
    if (!wlx_text_prepare(ctx, rect, text, length, style,
            (WLX_Text_Prepare_Opt){ .align = align, .wrap = wrap }, &p)) {
        return false;
    }
    wlx_inputbox_visual_line_bounds_from_lines(text, p.result.text_length,
        p.lines, p.result.line_count, cursor_offset, out_start, out_end);
    return true;
}

// Selection highlight over prepared line records: per line, intersect the
// selection byte range with the line's visible range; the covered span is
// the difference of two tab-aware prefix measures, so highlights line up
// with the tab-expanded glyph positions (a zero tab_advance routes every
// measure through the single-range path for tab-free widgets). Runs before
// the text draw so the glyphs render on top; clipped to the band so wide
// lines cannot spill over the field chrome.
// Document line holding a byte offset: binary search over the hard line
// starts. An offset inside a separator belongs to the line the separator
// ends.
static size_t wlx_editor_index_line_of(const WLX_Editor_Line_Index *idx, size_t offset) {
    if (idx->count == 0) return 0;
    size_t lo = 0, hi = idx->count - 1;
    while (lo < hi) {
        size_t mid = lo + (hi - lo + 1) / 2;
        if (idx->offsets[mid] <= offset) lo = mid; else hi = mid - 1;
    }
    return lo;
}

// One past a line's text and separator: the next hard line start, or the
// document end for the last line. The line-next key every retained-entry
// lookup and record build derives from the index.
static inline size_t wlx_editor_line_next(const WLX_Editor_Line_Index *idx,
    size_t line, size_t len)
{
    return line + 1 < idx->count ? idx->offsets[line + 1] : len;
}

// Draw the selection band over the given records. idx and geom are the
// editor's retained-geometry hooks: with both present, span edges that
// land on measured unit boundaries resolve from stored advances; the
// inputbox passes NULL for both and every edge measures.
static void wlx_text_draw_selection(WLX_Context *ctx, WLX_Rect band, const char *text, size_t len,
    WLX_Text_Style ts, float tab_advance, const WLX_Editor_Line_Index *idx,
    WLX_Text_Geom_Store *geom, const WLX_Text_Line_Record *lines, size_t count,
    size_t sel_min, size_t sel_max, WLX_Color color)
{
    if (sel_min >= sel_max || len == 0 || color.a == 0) return;
    if (lines == NULL || count == 0) return;

    WLX_Scissor_Scope sc = wlx_scissor_scope_begin(ctx, band);
    for (size_t i = 0; i < count; i++) {
        const WLX_Text_Line_Record *line = &lines[i];
        size_t lo = sel_min > line->visible_start ? sel_min : line->visible_start;
        size_t hi = sel_max < line->visible_end ? sel_max : line->visible_end;
        if (lo >= hi) continue;

        float lead_w = 0.0f, hi_w = 0.0f, unused_h = 0.0f;
        bool replayed = false;
        if (geom != NULL && idx != NULL && idx->count > 0 && !line->empty_visual) {
            size_t li = wlx_editor_index_line_of(idx, line->visible_start);
            size_t lnext = wlx_editor_line_next(idx, li, len);
            WLX_Text_Geom_Entry *e = lnext > line->visible_start
                ? wlx_text_geom_find_containing(geom, line->visible_start, lnext)
                : NULL;
            if (e != NULL && e->units > 0) {
                bool ok = true;
                if (lo > line->visible_start)
                    ok = wlx_text_geom_advance_at(e, lo - e->line_start, &lead_w);
                if (ok && wlx_text_geom_advance_at(e, hi - e->line_start, &hi_w))
                    replayed = true;
            }
        }
        if (!replayed) {
            lead_w = 0.0f;
            if (lo > line->visible_start) {
                wlx_text_measure_prefix_tabs(ctx, text, len, line->visible_start, lo, ts, tab_advance,
                    &lead_w, &unused_h);
            }
            if (!wlx_text_measure_prefix_tabs(ctx, text, len, line->visible_start, hi, ts, tab_advance,
                    &hi_w, &unused_h)) continue;
        }
        float span_w = hi_w - lead_w;
        if (span_w <= 0.0f) continue;

        wlx_draw_rect(ctx, (WLX_Rect){ line->origin_x + lead_w, line->origin_y, span_w, line->line_h }, color);
    }
    wlx_scissor_scope_end(ctx, sc);
}

// Password-mask offset mapping: the display text carries one byte per
// plaintext codepoint, so display offsets are codepoint indices. Both
// directions are the identity when the field is not masked.
static size_t wlx_inputbox_display_offset(const char *buffer, size_t buf_len, size_t plain_off, bool password) {
    if (!password) return plain_off;
    if (plain_off > buf_len) plain_off = buf_len;
    size_t count = 0;
    size_t off = 0;
    while (off < plain_off) {
        off = wlx_utf8_next(buffer, off, buf_len);
        count++;
    }
    return count;
}

static size_t wlx_inputbox_plain_offset(const char *buffer, size_t buf_len, size_t disp_off, bool password) {
    if (!password) return disp_off;
    size_t off = 0;
    while (disp_off > 0 && off < buf_len) {
        off = wlx_utf8_next(buffer, off, buf_len);
        disp_off--;
    }
    return off;
}

static inline size_t wlx_text_edit_selection_min(const WLX_Text_Edit_State *st) {
    return st->selection_anchor < st->cursor_pos ? st->selection_anchor : st->cursor_pos;
}

static inline size_t wlx_text_edit_selection_max(const WLX_Text_Edit_State *st) {
    return st->selection_anchor > st->cursor_pos ? st->selection_anchor : st->cursor_pos;
}

static inline bool wlx_text_edit_has_selection(const WLX_Text_Edit_State *st) {
    return st->selection_anchor != st->cursor_pos;
}

// ---------------------------------------------------------------------------
// Text undo journal
// ---------------------------------------------------------------------------
//
// History of the mutations a text widget made through the two edit
// primitives below, kept per widget id in a context-owned cache. The
// primitives record into it (removed bytes are copied before they move),
// so every path of the shared key vocabulary is journaled by construction.
// A NULL journal means no history: compiled out, a password field, or a
// caller that keeps none.

static void wlx_text_undo_stack_free(WLX_Text_Undo_Stack *s) {
    wlx_free(s->entries);
    wlx_free(s->arena);
    wlx_zero_struct(*s);
}

#if WLX_TEXT_UNDO_ENTRIES != 0

static inline void wlx_text_undo_stack_reset(WLX_Text_Undo_Stack *s) {
    s->count = 0;
    s->arena_used = 0;
}

// Drop both directions of history and re-arm the staleness guard at the
// given document length. Allocations are kept for reuse.
static void wlx_text_undo_clear(WLX_Text_Undo_Journal *j, size_t length) {
    if (j == NULL) return;
    wlx_text_undo_stack_reset(&j->undo);
    wlx_text_undo_stack_reset(&j->redo);
    j->expected_len = length;
    j->last_was_replay = false;
    j->alloc_failed = false;
}

static WLX_Text_Undo_Journal *wlx_text_undo_find(WLX_Context *ctx, size_t id) {
    WLX_Text_Undo_Cache *cache = &ctx->text_undo;
    for (size_t i = 0; i < cache->count; i++) {
        if (cache->items[i].id == id) return &cache->items[i];
    }
    return NULL;
}

// Release a widget's journal outright: a field shown as password keeps no
// history, including anything recorded before the mode switched.
static void wlx_text_undo_drop(WLX_Context *ctx, size_t id) {
    WLX_Text_Undo_Cache *cache = &ctx->text_undo;
    for (size_t i = 0; i < cache->count; i++) {
        if (cache->items[i].id != id) continue;
        wlx_text_undo_stack_free(&cache->items[i].undo);
        wlx_text_undo_stack_free(&cache->items[i].redo);
        cache->items[i] = cache->items[cache->count - 1];
        cache->count--;
        return;
    }
}

// Drop a widget's history when it has one, at the given document length:
// the editor's line-index probe reports in-place rewrites this way.
static inline void wlx_text_undo_clear_if_present(WLX_Context *ctx, size_t id,
    size_t length)
{
    wlx_text_undo_clear(wlx_text_undo_find(ctx, id), length);
}

// Find or create the journal of a focused text widget and run the
// staleness guard: a document length or caller revision that differs from
// what the journal last saw means the buffer changed outside the widget,
// so the history is dropped before anything could apply it to bytes it
// never recorded. Returns NULL for password fields (any journal the id had
// is released) and on allocation failure.
static WLX_Text_Undo_Journal *wlx_text_undo_get(WLX_Context *ctx, size_t id,
    size_t length, uint32_t revision, bool password)
{
    if (password) {
        wlx_text_undo_drop(ctx, id);
        return NULL;
    }
    WLX_Text_Undo_Cache *cache = &ctx->text_undo;
    WLX_Text_Undo_Journal *j = wlx_text_undo_find(ctx, id);
    if (j == NULL) {
        if (cache->count == cache->capacity) {
            size_t new_cap = cache->capacity == 0 ? 4 : cache->capacity * 2;
            WLX_Text_Undo_Journal *grown = (WLX_Text_Undo_Journal *)wlx_realloc(
                cache->items, new_cap * sizeof(WLX_Text_Undo_Journal));
            if (grown == NULL) return NULL;
            cache->items = grown;
            cache->capacity = new_cap;
        }
        j = &cache->items[cache->count++];
        wlx_zero_struct(*j);
        j->id = id;
    }
    if (!j->guard_seen) {
        j->guard_seen = true;
        j->expected_len = length;
        j->revision_seen = revision;
    } else if (j->expected_len != length || j->revision_seen != revision) {
        wlx_text_undo_clear(j, length);
        j->revision_seen = revision;
    }
    j->touch = ++cache->clock;
    return j;
}

// Make room in a stack for one more entry carrying need_bytes of removed
// text. Whole oldest undo steps are evicted while either cap is exceeded
// and a step older than the one being recorded exists; once only the
// current step remains, the arrays grow past the caps for it (a single
// oversize step is admitted rather than losing the history of the edit a
// user most wants back). Returns false only on allocation failure.
static bool wlx_text_undo_make_room(WLX_Text_Undo_Stack *s, size_t need_bytes,
    uint32_t current_group)
{
    while (s->count > 0 && s->entries[0].group != current_group
        && (s->count >= (size_t)WLX_TEXT_UNDO_ENTRIES
            || s->arena_used + need_bytes > (size_t)WLX_TEXT_UNDO_BYTES)) {
        uint32_t oldest = s->entries[0].group;
        size_t n = 0;
        size_t bytes = 0;
        while (n < s->count && s->entries[n].group == oldest) {
            bytes += s->entries[n].removed_len;
            n++;
        }
        memmove(s->entries, s->entries + n, (s->count - n) * sizeof(*s->entries));
        s->count -= n;
        if (bytes > 0) {
            memmove(s->arena, s->arena + bytes, s->arena_used - bytes);
            s->arena_used -= bytes;
            for (size_t i = 0; i < s->count; i++) s->entries[i].arena_off -= bytes;
        }
    }
    if (s->count == s->cap) {
        size_t new_cap = s->cap == 0 ? 16 : s->cap * 2;
        if (new_cap > (size_t)WLX_TEXT_UNDO_ENTRIES
            && s->count < (size_t)WLX_TEXT_UNDO_ENTRIES) {
            new_cap = (size_t)WLX_TEXT_UNDO_ENTRIES;
        }
        WLX_Text_Undo_Entry *grown = (WLX_Text_Undo_Entry *)wlx_realloc(
            s->entries, new_cap * sizeof(*grown));
        if (grown == NULL) return false;
        s->entries = grown;
        s->cap = new_cap;
    }
    size_t need = s->arena_used + need_bytes;
    if (need > s->arena_cap) {
        size_t new_cap = s->arena_cap == 0 ? 256 : s->arena_cap;
        while (new_cap < need) {
            WLX_HARD_ASSERT(new_cap <= SIZE_MAX / 2,
                "size_t overflow in wlx_text_undo_make_room");
            new_cap *= 2;
        }
        if (new_cap > (size_t)WLX_TEXT_UNDO_BYTES) {
            new_cap = need > (size_t)WLX_TEXT_UNDO_BYTES ? need : (size_t)WLX_TEXT_UNDO_BYTES;
        }
        char *grown = (char *)wlx_realloc(s->arena, new_cap);
        if (grown == NULL) return false;
        s->arena = grown;
        s->arena_cap = new_cap;
    }
    return true;
}

// Stack a recording lands on: the replay of an undo records onto the redo
// stack and the replay of a redo onto the undo stack.
static inline WLX_Text_Undo_Stack *wlx_text_undo_target(WLX_Text_Undo_Journal *j) {
    if (!j->replaying) return &j->undo;
    return j->replay_redo ? &j->undo : &j->redo;
}

// Open the transaction one key-handler invocation forms. The caret pair it
// starts from is the before-state its first record keeps and the
// continuity test for coalescing into the newest step.
static inline void wlx_text_undo_txn_begin(WLX_Text_Undo_Journal *j, size_t caret,
    size_t anchor)
{
    if (j == NULL) return;
    j->in_txn = true;
    j->txn_first = true;
    j->cls = WLX_TEXT_UNDO_CLS_NONE;
    j->txn_caret0 = caret;
    j->txn_anchor0 = anchor;
}

// Close the transaction at the document length any later history must
// find unchanged. A record that could not be kept has already dropped the
// history; the flag is spent here.
static inline void wlx_text_undo_txn_end(WLX_Text_Undo_Journal *j, size_t length) {
    if (j == NULL) return;
    j->in_txn = false;
    j->expected_len = length;
    j->alloc_failed = false;
}

// Name the handler path about to mutate: the class decides which steps
// may coalesce and which always stand alone.
static inline void wlx_text_undo_set_class(WLX_Text_Undo_Journal *j, uint8_t cls) {
    if (j != NULL) j->cls = cls;
}

// Coalescing: the first record of a transaction may extend the newest
// undo entry instead of opening a new step, so a typed run, a Backspace
// run or a Delete run undoes as one. Conditions: nothing was replayed
// since that entry; the same class, and one of the three run classes; the
// caret pair this transaction started from is the pair the entry left
// behind (a click or arrow key in between breaks it without any hook); the
// new range touches the entry's range on its growing side (a typing insert
// at its inserted end, a Backspace delete ending at its start or a Delete
// at its start, the latter two on an entry that inserted nothing); and the
// entry is still below the run bound. Returns the entry to extend or NULL.
static WLX_Text_Undo_Entry *wlx_text_undo_coalesce_target(WLX_Text_Undo_Journal *j,
    size_t start, size_t removed_len, size_t inserted_len)
{
    WLX_Text_Undo_Stack *s = &j->undo;
    if (j->last_was_replay || s->count == 0) return NULL;
    WLX_Text_Undo_Entry *e = &s->entries[s->count - 1];
    if (e->cls != j->cls) return NULL;
    if (j->txn_caret0 != e->caret_after || j->txn_anchor0 != e->anchor_after) return NULL;
    if (e->removed_len + e->inserted_len >= (size_t)WLX_TEXT_UNDO_GROUP_BYTES) return NULL;
    if (j->cls == WLX_TEXT_UNDO_CLS_TYPING) {
        return (removed_len == 0 && inserted_len > 0
            && start == e->start + e->inserted_len) ? e : NULL;
    }
    if (j->cls == WLX_TEXT_UNDO_CLS_BACKSPACE) {
        return (inserted_len == 0 && removed_len > 0 && e->inserted_len == 0
            && start + removed_len == e->start) ? e : NULL;
    }
    if (j->cls == WLX_TEXT_UNDO_CLS_DELETE) {
        return (inserted_len == 0 && removed_len > 0 && e->inserted_len == 0
            && start == e->start) ? e : NULL;
    }
    return NULL;
}

// Record one primitive mutation before it moves bytes: the removed_len
// original bytes at start (copied into the arena) are about to be replaced
// by inserted_len bytes. The first record of a transaction either extends
// the newest step (coalescing) or opens a new one and drops the redo
// history; a replay records the mirror entry on the opposite stack under
// the replayed step's id. Outside a transaction the record forms a step of
// its own. On allocation failure the history is dropped rather than left
// with a gap.
static void wlx_text_undo_record(WLX_Text_Undo_Journal *j, size_t start,
    const char *removed, size_t removed_len, size_t inserted_len,
    size_t caret, size_t anchor)
{
    if (j == NULL || (removed_len == 0 && inserted_len == 0)) return;
    bool implicit = !j->in_txn;
    if (implicit) wlx_text_undo_txn_begin(j, caret, anchor);
    WLX_Text_Undo_Stack *s = wlx_text_undo_target(j);
    bool first = j->txn_first;
    j->txn_first = false;
    if (first && !j->replaying) {
        wlx_text_undo_stack_reset(&j->redo);
        WLX_Text_Undo_Entry *e = wlx_text_undo_coalesce_target(j, start, removed_len, inserted_len);
        j->last_was_replay = false;
        if (e != NULL) {
            j->txn_group = e->group;
            if (!wlx_text_undo_make_room(s, removed_len, e->group)) {
                wlx_text_undo_stack_reset(&j->undo);
                j->alloc_failed = true;
            } else {
                e = &s->entries[s->count - 1];
                if (inserted_len > 0) {
                    e->inserted_len += inserted_len;
                } else if (start + removed_len == e->start) {
                    // Backspace run: the new bytes precede the entry's run,
                    // which ends the arena.
                    memmove(s->arena + e->arena_off + removed_len,
                        s->arena + e->arena_off, e->removed_len);
                    memcpy(s->arena + e->arena_off, removed, removed_len);
                    s->arena_used += removed_len;
                    e->removed_len += removed_len;
                    e->start = start;
                } else {
                    // Delete run: the new bytes follow the entry's run.
                    memcpy(s->arena + s->arena_used, removed, removed_len);
                    s->arena_used += removed_len;
                    e->removed_len += removed_len;
                }
            }
            if (implicit) j->in_txn = false;
            return;
        }
        j->txn_group = ++j->next_group;
    }
    if (!wlx_text_undo_make_room(s, removed_len, j->txn_group)) {
        wlx_text_undo_stack_reset(&j->undo);
        wlx_text_undo_stack_reset(&j->redo);
        j->alloc_failed = true;
    } else {
        WLX_Text_Undo_Entry *e = &s->entries[s->count++];
        e->group = j->txn_group;
        e->cls = j->cls;
        e->start = start;
        e->removed_len = removed_len;
        e->inserted_len = inserted_len;
        e->arena_off = s->arena_used;
        if (j->replaying) {
            e->caret_before = j->rep_caret_before;
            e->anchor_before = j->rep_anchor_before;
            e->caret_after = j->rep_caret_after;
            e->anchor_after = j->rep_anchor_after;
        } else {
            e->caret_before = first ? j->txn_caret0 : caret;
            e->anchor_before = first ? j->txn_anchor0 : anchor;
            e->caret_after = caret;
            e->anchor_after = anchor;
        }
        if (removed_len > 0) {
            memcpy(s->arena + s->arena_used, removed, removed_len);
            s->arena_used += removed_len;
        }
    }
    if (implicit) j->in_txn = false;
}

// Close a primitive's record: the caret pair it left behind (a replay's
// mirror entries keep the step's own pairs instead) and the document
// length any later history must find unchanged.
static void wlx_text_undo_note_caret_after(WLX_Text_Undo_Journal *j,
    size_t caret, size_t anchor, size_t length)
{
    if (j == NULL) return;
    if (!j->replaying && j->undo.count > 0) {
        WLX_Text_Undo_Entry *e = &j->undo.entries[j->undo.count - 1];
        e->caret_after = caret;
        e->anchor_after = anchor;
    }
    j->expected_len = length;
}

#else  // WLX_TEXT_UNDO_ENTRIES == 0: the journal is compiled out.

static inline WLX_Text_Undo_Journal *wlx_text_undo_find(WLX_Context *ctx, size_t id) {
    WLX_UNUSED(ctx); WLX_UNUSED(id);
    return NULL;
}
static inline void wlx_text_undo_clear_if_present(WLX_Context *ctx, size_t id,
    size_t length)
{
    WLX_UNUSED(ctx); WLX_UNUSED(id); WLX_UNUSED(length);
}
static inline WLX_Text_Undo_Journal *wlx_text_undo_get(WLX_Context *ctx, size_t id,
    size_t length, uint32_t revision, bool password)
{
    WLX_UNUSED(ctx); WLX_UNUSED(id); WLX_UNUSED(length);
    WLX_UNUSED(revision); WLX_UNUSED(password);
    return NULL;
}
static inline void wlx_text_undo_record(WLX_Text_Undo_Journal *j, size_t start,
    const char *removed, size_t removed_len, size_t inserted_len,
    size_t caret, size_t anchor)
{
    WLX_UNUSED(j); WLX_UNUSED(start); WLX_UNUSED(removed); WLX_UNUSED(removed_len);
    WLX_UNUSED(inserted_len); WLX_UNUSED(caret); WLX_UNUSED(anchor);
}
static inline void wlx_text_undo_note_caret_after(WLX_Text_Undo_Journal *j,
    size_t caret, size_t anchor, size_t length)
{
    WLX_UNUSED(j); WLX_UNUSED(caret); WLX_UNUSED(anchor); WLX_UNUSED(length);
}
static inline void wlx_text_undo_txn_begin(WLX_Text_Undo_Journal *j, size_t caret,
    size_t anchor)
{
    WLX_UNUSED(j); WLX_UNUSED(caret); WLX_UNUSED(anchor);
}
static inline void wlx_text_undo_txn_end(WLX_Text_Undo_Journal *j, size_t length) {
    WLX_UNUSED(j); WLX_UNUSED(length);
}
static inline void wlx_text_undo_set_class(WLX_Text_Undo_Journal *j, uint8_t cls) {
    WLX_UNUSED(j); WLX_UNUSED(cls);
}

#endif  // WLX_TEXT_UNDO_ENTRIES

// Byte-span report of the buffer mutations one frame's shared edit
// vocabulary applied: the pre-frame byte range [start, old_end) was
// replaced by [start, new_end) in the current buffer. Sequential edits in
// the same frame merge into one conservative range (a caller invalidating
// derived per-line state drops everything inside it and shifts everything
// past it by the byte delta).
typedef struct {
    bool edited;
    size_t start;    // first byte touched
    size_t old_end;  // end of the touched range in pre-frame coordinates
    size_t new_end;  // end of the replacement in current coordinates
} WLX_Text_Edit_Span;

// Merge one edit (current-coordinate range [start, old_end) replaced by
// [start, new_end)) into the running frame span.
static void wlx_text_edit_span_add(WLX_Text_Edit_Span *span, size_t start,
    size_t old_end, size_t new_end)
{
    if (span == NULL) return;
    if (!span->edited) {
        span->edited = true;
        span->start = start;
        span->old_end = old_end;
        span->new_end = new_end;
        return;
    }
    // The running span maps pre-frame [S, OE) to current [S, NE); the new
    // edit is expressed in current coordinates. Bytes it touches beyond NE
    // map back through the running delta; the merged current end tracks
    // the new edit's shift of everything at or past it.
    long run_delta = (long)span->new_end - (long)span->old_end;
    long edit_delta = (long)new_end - (long)old_end;
    if (start < span->start) span->start = start;
    if (old_end > span->new_end) {
        size_t mapped = (size_t)((long)old_end - run_delta);
        if (mapped > span->old_end) span->old_end = mapped;
    }
    size_t shifted = (size_t)((long)span->new_end + edit_delta);
    span->new_end = new_end > shifted ? new_end : shifted;
}

// Remove the byte range [min(*cursor, *anchor), max(*cursor, *anchor)) from
// a length-explicit buffer, collapsing caret and anchor to the range start.
// The buffer is treated as a byte slice: no NUL is read or written. The
// removed bytes are journaled before they move. Returns true when bytes
// were removed.
static bool wlx_text_edit_delete_selection(char *buffer, size_t *length, size_t *cursor,
    size_t *anchor, WLX_Text_Edit_Span *span, WLX_Text_Undo_Journal *undo) {
    size_t sel_min = *cursor < *anchor ? *cursor : *anchor;
    size_t sel_max = *cursor > *anchor ? *cursor : *anchor;
    if (sel_min == sel_max) return false;

    wlx_text_undo_record(undo, sel_min, &buffer[sel_min], sel_max - sel_min, 0,
        *cursor, *anchor);
    memmove(&buffer[sel_min], &buffer[sel_max], *length - sel_max);
    *length -= sel_max - sel_min;
    *cursor = sel_min;
    *anchor = sel_min;
    wlx_text_edit_span_add(span, sel_min, sel_max, sel_min);
    wlx_text_undo_note_caret_after(undo, *cursor, *anchor, *length);
    return true;
}

// Insert a byte slice at the caret into a length-explicit buffer bounded by
// buffer_cap, truncating on a UTF-8 boundary so only whole codepoints land.
// No NUL is read or written. The insert is journaled by position and length
// only (the bytes live in the document). Returns the number of bytes
// inserted.
static size_t wlx_text_edit_insert(char *buffer, size_t buffer_cap, size_t *length,
    size_t *cursor, size_t *anchor, const char *text, size_t len,
    WLX_Text_Edit_Span *span, WLX_Text_Undo_Journal *undo)
{
    size_t room = buffer_cap > *length ? buffer_cap - *length : 0;
    size_t ins = len < room ? len : room;
    while (ins > 0 && !wlx_text_utf8_boundary(text, len, ins)) ins--;
    if (ins == 0) return 0;

    wlx_text_undo_record(undo, *cursor, NULL, 0, ins, *cursor, *anchor);
    memmove(&buffer[*cursor + ins], &buffer[*cursor], *length - *cursor);
    memcpy(&buffer[*cursor], text, ins);
    wlx_text_edit_span_add(span, *cursor, *cursor, *cursor + ins);
    *cursor += ins;
    *anchor = *cursor;
    *length += ins;
    wlx_text_undo_note_caret_after(undo, *cursor, *anchor, *length);
    return ins;
}

#if WLX_TEXT_UNDO_ENTRIES != 0
// Undo (redo = false) or redo (redo = true) the newest step of that
// direction by replaying its entries through the edit primitives, newest
// first: delete what the entry inserted, insert what it removed. The
// replay records the mirror entries onto the opposite stack under the same
// step id, so the reverse chord is the mirror image and the frame span
// reports the bytes exactly as a keystroke would. A step applies
// all-or-nothing: every entry is validated against the live length and the
// capacity before any byte moves, and a step that cannot apply (the buffer
// changed under the journal) drops the history instead. The caret pair is
// restored to the step's before-state (undo) or after-state (redo). Returns
// true when the buffer changed.
static bool wlx_text_undo_apply(WLX_Text_Undo_Journal *j, bool redo,
    WLX_Text_Edit_State *st, char *buffer, size_t buffer_cap, size_t *length,
    WLX_Text_Edit_Span *span)
{
    WLX_Text_Undo_Stack *src = redo ? &j->redo : &j->undo;
    if (src->count == 0) return false;
    uint32_t group = src->entries[src->count - 1].group;
    size_t first = src->count;
    while (first > 0 && src->entries[first - 1].group == group) first--;

    size_t len = *length;
    for (size_t i = src->count; i-- > first;) {
        const WLX_Text_Undo_Entry *e = &src->entries[i];
        if (e->start > len || e->inserted_len > len - e->start
            || len - e->inserted_len + e->removed_len > buffer_cap) {
            wlx_text_undo_clear(j, *length);
            return false;
        }
        len = len - e->inserted_len + e->removed_len;
    }

    j->rep_caret_before = src->entries[first].caret_before;
    j->rep_anchor_before = src->entries[first].anchor_before;
    j->rep_caret_after = src->entries[src->count - 1].caret_after;
    j->rep_anchor_after = src->entries[src->count - 1].anchor_after;
    j->replaying = true;
    j->replay_redo = redo;
    j->txn_first = false;
    j->txn_group = group;
    j->cls = WLX_TEXT_UNDO_CLS_REPLAY;
    bool changed = false;
    bool intact = true;
    while (src->count > first && intact) {
        // Pop first: the entry's bytes end the source arena and stay in
        // place until this replay has read them (nothing writes the source
        // stack while its mirror records on the other one).
        WLX_Text_Undo_Entry e = src->entries[src->count - 1];
        src->count--;
        src->arena_used -= e.removed_len;
        size_t cur = e.start;
        size_t anc = e.start + e.inserted_len;
        if (e.inserted_len > 0
            && wlx_text_edit_delete_selection(buffer, length, &cur, &anc, span, j)) {
            changed = true;
        }
        if (e.removed_len > 0) {
            cur = e.start;
            anc = e.start;
            size_t ins = wlx_text_edit_insert(buffer, buffer_cap, length, &cur, &anc,
                src->arena + e.arena_off, e.removed_len, span, j);
            if (ins > 0) changed = true;
            if (ins != e.removed_len) intact = false;
        }
        if (j->alloc_failed) intact = false;
    }
    j->replaying = false;
    // A later record in this same transaction opens a step of its own.
    j->txn_first = true;
    if (!intact) {
        wlx_text_undo_clear(j, *length);
        return changed;
    }
    st->cursor_pos = redo ? j->rep_caret_after : j->rep_caret_before;
    st->selection_anchor = redo ? j->rep_anchor_after : j->rep_anchor_before;
    j->last_was_replay = true;
    return changed;
}
#else
static inline bool wlx_text_undo_apply(WLX_Text_Undo_Journal *j, bool redo,
    WLX_Text_Edit_State *st, char *buffer, size_t buffer_cap, size_t *length,
    WLX_Text_Edit_Span *span)
{
    WLX_UNUSED(j); WLX_UNUSED(redo); WLX_UNUSED(st); WLX_UNUSED(buffer);
    WLX_UNUSED(buffer_cap); WLX_UNUSED(length); WLX_UNUSED(span);
    return false;
}
#endif

// Capability gates for the shared text-edit key vocabulary. The zero value
// is the most permissive single-line editable field: mutations allowed,
// plain-codepoint deletes, no newline or tab inserts, clipboard open.
typedef struct {
    bool read_only;       // reject every mutation; navigation/selection/copy live
    bool allow_newline;   // multiline inputbox, editor
    bool allow_tab;       // editor only (inputbox Tab reserved for focus traversal)
    bool word_delete;     // Ctrl/Alt-Backspace/Delete stretch to word granularity
    bool mask_clipboard;  // password: suppress copy AND cut (hard gate)
} WLX_Text_Edit_Caps;

// Shared editing vocabulary on an explicit-length byte slice: clipboard
// shortcuts and select-all on the platform command modifier, typing
// (replacing a live selection), Enter/Tab inserts, Backspace/Delete with
// word variants, and LEFT/RIGHT with collapse-else-move. The caller
// maintains any trailing NUL. Caret and anchor are clamped on entry and
// normalized on exit; any caret or text change resets the blink and drops
// the sticky UP/DOWN column - every caret change here is horizontal, and a
// key that changes nothing (LEFT at the start, RIGHT at the end) leaves the
// column latched. Every mutation records into the undo journal when one
// is given (NULL keeps no history). Returns true when the text mutated.
static bool wlx_text_edit_handle_keys(WLX_Context *ctx, WLX_Text_Edit_State *st,
    char *buffer, size_t buffer_cap, size_t *length, WLX_Text_Edit_Caps caps,
    WLX_Text_Edit_Span *span, WLX_Text_Undo_Journal *undo)
{
    bool text_changed = false;
    bool moved = false;

    if (st->cursor_pos > *length) st->cursor_pos = *length;
    if (st->selection_anchor > *length) st->selection_anchor = *length;

    // One handler invocation is one undo transaction: every mutation below
    // records under one step, so typing over a selection undoes as a unit.
    wlx_text_undo_txn_begin(undo, st->cursor_pos, st->selection_anchor);

    // SHIFT keeps the anchor in place so caret motion extends the selection;
    // Ctrl or Alt stretches motion and deletes to word granularity (Alt
    // covers Apple platforms, Ctrl the rest; accepting both keeps one path).
    bool shift = wlx_mod_down(ctx, WLX_MOD_SHIFT);
    bool word_motion = wlx_mod_down(ctx, WLX_MOD_CTRL) || wlx_mod_down(ctx, WLX_MOD_ALT);

    // Editing shortcuts on the platform command modifier: copy, cut, paste,
    // select-all. Handled before the motion keys so a shortcut frame cannot
    // also move the caret.
    if (wlx_mod_command_down(ctx)) {
        // Undo and redo come first, so a chord frame never also copies or
        // pastes in an ambiguous order: command+Z steps back, command+Shift+Z
        // and command+Y step forward, and a held chord keeps stepping. Both
        // are mutations, so a read-only field rejects them like any other.
        if (undo != NULL && !caps.read_only) {
            bool stepped = false;
            if (wlx_is_key_actuated(ctx, WLX_KEY_Z)) {
                stepped = wlx_text_undo_apply(undo, shift, st, buffer, buffer_cap,
                    length, span);
            } else if (wlx_is_key_actuated(ctx, WLX_KEY_Y)) {
                stepped = wlx_text_undo_apply(undo, true, st, buffer, buffer_cap,
                    length, span);
            }
            if (stepped) text_changed = true;
        }

        size_t sel_min = wlx_text_edit_selection_min(st);
        size_t sel_max = wlx_text_edit_selection_max(st);

        // Copy: selected bytes only; an empty selection copies nothing.
        // mask_clipboard suppresses copy AND cut entirely: a silent
        // cut-delete would suggest the plaintext reached the clipboard.
        if (wlx_is_key_pressed(ctx, WLX_KEY_C) && sel_max > sel_min && !caps.mask_clipboard) {
            wlx_clipboard_set_text(ctx, buffer + sel_min, sel_max - sel_min);
        }

        // Cut: copy, then remove the selection.
        if (wlx_is_key_pressed(ctx, WLX_KEY_X) && sel_max > sel_min
            && !caps.mask_clipboard && !caps.read_only) {
            wlx_clipboard_set_text(ctx, buffer + sel_min, sel_max - sel_min);
            wlx_text_undo_set_class(undo, WLX_TEXT_UNDO_CLS_CUT);
            if (wlx_text_edit_delete_selection(buffer, length,
                    &st->cursor_pos, &st->selection_anchor, span, undo)) {
                text_changed = true;
            }
        }

        // Paste: replace the selection with the clipboard bytes. The insert
        // primitive truncates to the buffer capacity on a UTF-8 boundary and
        // takes the borrowed backend string directly, so paste length is not
        // limited by the per-frame text_input ring.
        if (wlx_is_key_pressed(ctx, WLX_KEY_V) && !caps.read_only
            && ctx->backend.clipboard_get != NULL) {
            const char *clip = ctx->backend.clipboard_get(ctx->backend.user);
            size_t clip_len = clip ? strlen(clip) : 0;
            if (clip_len > 0) {
                wlx_text_undo_set_class(undo, WLX_TEXT_UNDO_CLS_PASTE);
                if (wlx_text_edit_delete_selection(buffer, length,
                        &st->cursor_pos, &st->selection_anchor, span, undo)) {
                    text_changed = true;
                }
                if (wlx_text_edit_insert(buffer, buffer_cap, length,
                        &st->cursor_pos, &st->selection_anchor, clip, clip_len, span, undo) > 0) {
                    text_changed = true;
                }
            }
        }

        // Select all: anchor at the start, caret at the end.
        if (wlx_is_key_pressed(ctx, WLX_KEY_A)) {
            st->selection_anchor = 0;
            st->cursor_pos = *length;
            moved = true;
        }
    }

    // Typed characters land at the caret, replacing a live selection.
    {
        size_t type_len = 0;
        while (type_len < sizeof(ctx->input.text_input) && ctx->input.text_input[type_len] != '\0') {
            type_len++;
        }
        if (type_len > 0 && !caps.read_only) {
            wlx_text_undo_set_class(undo, WLX_TEXT_UNDO_CLS_TYPING);
            if (wlx_text_edit_delete_selection(buffer, length,
                    &st->cursor_pos, &st->selection_anchor, span, undo)) {
                text_changed = true;
            }
            if (wlx_text_edit_insert(buffer, buffer_cap, length,
                    &st->cursor_pos, &st->selection_anchor,
                    ctx->input.text_input, type_len, span, undo) > 0) {
                text_changed = true;
            }
        }
    }

    // Enter inserts a hard newline (replacing a live selection). Actuated on
    // press and OS auto-repeat. The widget owns the press: it must not
    // double as a keyboard activation elsewhere this frame.
    if (caps.allow_newline && !caps.read_only && wlx_is_key_actuated(ctx, WLX_KEY_ENTER)) {
        wlx_text_undo_set_class(undo, WLX_TEXT_UNDO_CLS_NEWLINE);
        if (wlx_text_edit_delete_selection(buffer, length,
                &st->cursor_pos, &st->selection_anchor, span, undo)) {
            text_changed = true;
        }
        if (wlx_text_edit_insert(buffer, buffer_cap, length,
                &st->cursor_pos, &st->selection_anchor, "\n", 1, span, undo) > 0) {
            text_changed = true;
        }
        ctx->interaction.enter_consumed = true;
    }

    // Tab inserts a literal tab character - unless keyboard traversal
    // already spent this frame's Tab landing focus here.
    if (caps.allow_tab && !caps.read_only && !ctx->interaction.tab_consumed
            && wlx_is_key_actuated(ctx, WLX_KEY_TAB)) {
        wlx_text_undo_set_class(undo, WLX_TEXT_UNDO_CLS_TAB);
        if (wlx_text_edit_delete_selection(buffer, length,
                &st->cursor_pos, &st->selection_anchor, span, undo)) {
            text_changed = true;
        }
        if (wlx_text_edit_insert(buffer, buffer_cap, length,
                &st->cursor_pos, &st->selection_anchor, "\t", 1, span, undo) > 0) {
            text_changed = true;
        }
    }

    // Backspace: the selection, else the word or codepoint before the caret
    // (word deletes reuse the range delete by parking the anchor). Actuated
    // on press and OS auto-repeat so holding the key keeps deleting.
    if (!caps.read_only && wlx_is_key_actuated(ctx, WLX_KEY_BACKSPACE)) {
        wlx_text_undo_set_class(undo, WLX_TEXT_UNDO_CLS_SELECTION);
        if (wlx_text_edit_delete_selection(buffer, length,
                &st->cursor_pos, &st->selection_anchor, span, undo)) {
            text_changed = true;
        } else if (st->cursor_pos > 0) {
            bool word = caps.word_delete && word_motion;
            size_t prev_pos = word
                ? wlx_utf8_word_prev(buffer, st->cursor_pos)
                : wlx_utf8_prev(buffer, st->cursor_pos);
            if (prev_pos < st->cursor_pos) {
                wlx_text_undo_set_class(undo, word
                    ? WLX_TEXT_UNDO_CLS_WORD_DELETE : WLX_TEXT_UNDO_CLS_BACKSPACE);
                st->selection_anchor = prev_pos;
                if (wlx_text_edit_delete_selection(buffer, length,
                        &st->cursor_pos, &st->selection_anchor, span, undo)) {
                    text_changed = true;
                }
            }
        }
    }

    // Delete: the selection, else the word or codepoint at the caret
    // (forward delete: the caret stays put, following bytes shift left).
    if (!caps.read_only && wlx_is_key_actuated(ctx, WLX_KEY_DELETE)) {
        wlx_text_undo_set_class(undo, WLX_TEXT_UNDO_CLS_SELECTION);
        if (wlx_text_edit_delete_selection(buffer, length,
                &st->cursor_pos, &st->selection_anchor, span, undo)) {
            text_changed = true;
        } else if (st->cursor_pos < *length) {
            bool word = caps.word_delete && word_motion;
            size_t next_pos = word
                ? wlx_utf8_word_next(buffer, st->cursor_pos, *length)
                : wlx_utf8_next(buffer, st->cursor_pos, *length);
            if (next_pos > st->cursor_pos) {
                wlx_text_undo_set_class(undo, word
                    ? WLX_TEXT_UNDO_CLS_WORD_DELETE : WLX_TEXT_UNDO_CLS_DELETE);
                st->selection_anchor = next_pos;
                if (wlx_text_edit_delete_selection(buffer, length,
                        &st->cursor_pos, &st->selection_anchor, span, undo)) {
                    text_changed = true;
                }
            }
        }
    }

    // LEFT: collapse a live selection to its start, else move the caret
    // left by one codepoint or word.
    if (wlx_is_key_actuated(ctx, WLX_KEY_LEFT)) {
        if (!shift && wlx_text_edit_has_selection(st)) {
            st->cursor_pos = wlx_text_edit_selection_min(st);
            st->selection_anchor = st->cursor_pos;
            moved = true;
        } else if (st->cursor_pos > 0) {
            size_t next_pos = word_motion
                ? wlx_utf8_word_prev(buffer, st->cursor_pos)
                : wlx_utf8_prev(buffer, st->cursor_pos);
            if (next_pos != st->cursor_pos) {
                st->cursor_pos = next_pos;
                if (!shift) st->selection_anchor = next_pos;
                moved = true;
            }
        }
    }

    // RIGHT: collapse a live selection to its end, else move the caret
    // right by one codepoint or word.
    if (wlx_is_key_actuated(ctx, WLX_KEY_RIGHT)) {
        if (!shift && wlx_text_edit_has_selection(st)) {
            st->cursor_pos = wlx_text_edit_selection_max(st);
            st->selection_anchor = st->cursor_pos;
            moved = true;
        } else if (st->cursor_pos < *length) {
            size_t next_pos = word_motion
                ? wlx_utf8_word_next(buffer, st->cursor_pos, *length)
                : wlx_utf8_next(buffer, st->cursor_pos, *length);
            if (next_pos != st->cursor_pos) {
                st->cursor_pos = next_pos;
                if (!shift) st->selection_anchor = next_pos;
                moved = true;
            }
        }
    }

    size_t normalized = wlx_text_normalize_cursor_offset(buffer, *length, st->cursor_pos);
    if (normalized != st->cursor_pos) moved = true;
    st->cursor_pos = normalized;
    st->selection_anchor = wlx_text_normalize_cursor_offset(buffer, *length, st->selection_anchor);
    if (moved || text_changed) {
        st->cursor_blink_time = 0.0f;
        // Every caret change here is horizontal (motion, edit, select-all),
        // so it invalidates the sticky UP/DOWN column.
        st->preferred_x_valid = false;
    }
    wlx_text_undo_txn_end(undo, *length);
    return text_changed;
}

// Widget-specific hooks for the shared pointer driver. The driver owns the
// click bookkeeping and widening (1 = caret, 2 = word, 3 = select all), the
// mouse_selecting lifecycle, and the overshoot computation; hit resolution
// and drag auto-scroll differ per widget and come through here.
typedef struct {
    void *user;
    // Pointer position -> byte offset in the widget's plain text space. The
    // y is pre-clamped to the band; x is pre-clamped only during a drag
    // with clamp_drag_x set. The inputbox resolves via its line records
    // plus the password display->plain mapping; the editor via index
    // arithmetic (wrap or linear).
    size_t (*hit)(void *user, float x, float y);
    // Word bounds around a hit offset, in the same plain space (the
    // inputbox computes display-space bounds and maps back; the editor is
    // direct).
    void (*word_bounds)(void *user, size_t hit, size_t *start, size_t *end);
    // Drag auto-scroll for a pointer past the band edge; each overshoot is
    // pre-clamped to +/- WLX_TEXT_DRAG_SCROLL_MAX_OVERSHOOT per axis. The
    // inputbox shifts scroll_y plus the line origins (y only); the editor
    // steps the wrap anchor or the pixel offsets (both axes).
    void (*auto_scroll)(void *user, float over_x, float over_y, float dt);
    // Clamp the drag hit x to the band: the editor auto-scrolls toward the
    // pointer so the hit stays at the band edge while the view moves; the
    // inputbox has no horizontal scroll and lets the hit run past the edge
    // to reach clipped text.
    bool clamp_drag_x;
} WLX_Text_Mouse_Ops;

// One press/multi-click/drag caret gesture over a text band. press is this
// frame's caret-placing click, with the widget-specific exclusions
// (scrollbar strips, gutter, live thumb drags, the widget rect test)
// already applied by the caller; while the press is held, dragging keeps
// extending the selection with edge auto-scroll. Returns true when the
// caret or the selection changed.
static bool wlx_text_edit_handle_mouse(WLX_Context *ctx, WLX_Text_Edit_State *st,
    WLX_Rect band, size_t text_len, bool shift, bool press,
    const WLX_Text_Mouse_Ops *ops)
{
    float mx = (float)ctx->input.mouse_x;
    float my = (float)ctx->input.mouse_y;
    bool changed = false;

    if (press) {
        // The band's bottom edge maps to the first line past the window: a
        // press below the text must land on the last visible line, not
        // teleport the caret - and the view with it - offscreen.
        float hit_y = my;
        if (hit_y > band.y + band.h - 1.0f) hit_y = band.y + band.h - 1.0f;
        if (hit_y < band.y) hit_y = band.y;
        size_t hit = ops->hit(ops->user, mx, hit_y);

        bool multi = st->last_click_time <= WLX_TEXT_MULTI_CLICK_SECONDS
            && hit == st->last_click_pos;
        st->click_count = multi ? st->click_count + 1 : 1;
        st->last_click_time = 0.0f;
        st->last_click_pos = hit;

        if (st->click_count >= 3) {
            st->selection_anchor = 0;
            st->cursor_pos = text_len;
            st->mouse_selecting = false;
            st->click_count = 0;
        } else if (st->click_count == 2) {
            size_t word_start = 0, word_end = 0;
            ops->word_bounds(ops->user, hit, &word_start, &word_end);
            st->selection_anchor = word_start;
            st->cursor_pos = word_end;
            st->mouse_selecting = false;
        } else {
            st->cursor_pos = hit;
            if (!shift) st->selection_anchor = hit;
            st->mouse_selecting = true;
        }
        st->cursor_blink_time = 0.0f;
        st->preferred_x_valid = false;
        changed = true;
    } else if (st->mouse_selecting) {
        if (ctx->input.mouse_down) {
            // Dragging past a band edge auto-scrolls toward the pointer on
            // that axis so a selection can grow beyond one viewport; speed
            // scales with the overshoot (capped). The hit point is then
            // clamped to the band so the selection only ever extends to the
            // edge and grows as the view scrolls.
            float over_x = 0.0f, over_y = 0.0f;
            if (mx < band.x) over_x = mx - band.x;
            else if (mx > band.x + band.w) over_x = mx - (band.x + band.w);
            if (my < band.y) over_y = my - band.y;
            else if (my > band.y + band.h) over_y = my - (band.y + band.h);
            over_x = wlx_clampf(over_x, -WLX_TEXT_DRAG_SCROLL_MAX_OVERSHOOT, WLX_TEXT_DRAG_SCROLL_MAX_OVERSHOOT);
            over_y = wlx_clampf(over_y, -WLX_TEXT_DRAG_SCROLL_MAX_OVERSHOOT, WLX_TEXT_DRAG_SCROLL_MAX_OVERSHOOT);
            ops->auto_scroll(ops->user, over_x, over_y, wlx_get_frame_time(ctx));

            float hit_x = mx;
            if (ops->clamp_drag_x) {
                hit_x = wlx_clampf(hit_x, band.x, band.x + band.w);
            }
            float hit_y = my;
            hit_y = wlx_clampf(hit_y, band.y, band.y + band.h);
            size_t hit = ops->hit(ops->user, hit_x, hit_y);
            if (hit != st->cursor_pos) {
                st->cursor_pos = hit;
                st->cursor_blink_time = 0.0f;
                st->preferred_x_valid = false;
                changed = true;
            }
        } else {
            st->mouse_selecting = false;
        }
    }
    return changed;
}

// Keyboard input for a focused inputbox: caret placement on first focus,
// then the shared text-edit vocabulary over the buffer's slice view (the
// trailing NUL is restored after the handling). read_only rejects every
// mutation while navigation, selection, and copy keep working; password
// suppresses copy/cut and keeps no undo journal, so plaintext never leaves
// the field; multiline turns Enter into a newline insert. The widget's undo
// journal is found by id under the caller's revision (the staleness guard
// drops history the buffer outgrew). Returns true when the buffer text was
// mutated (insert or delete); cursor-only movement resets the blink but
// reports false.
static bool wlx_inputbox_handle_keys(WLX_Context *ctx, WLX_Inputbox_State *state,
    char *buffer, size_t buffer_size, bool just_focused, bool read_only, bool password,
    bool multiline, size_t id, uint32_t revision)
{
    size_t len = strlen(buffer);

    // Initialize cursor position when first focused
    if (just_focused) {
        state->caret.cursor_pos = len;
        state->caret.selection_anchor = len;
        state->caret.cursor_blink_time = 0.0f;
        state->caret.preferred_x_valid = false;
    }

    WLX_Text_Undo_Journal *undo = wlx_text_undo_get(ctx, id, len, revision, password);
    bool text_changed = wlx_text_edit_handle_keys(ctx, &state->caret, buffer,
        buffer_size - 1, &len,
        (WLX_Text_Edit_Caps){ .read_only = read_only,
                              .allow_newline = multiline,
                              .word_delete = true,
                              .mask_clipboard = password }, NULL, undo);
    buffer[len] = '\0';
    return text_changed;
}

// Pointer-driver hooks for the inputbox: hits resolve through the frame's
// line records on the display text (mask under password), then map to the
// plain space; auto-scroll shifts scroll_y together with the
// already-positioned records so the post-scroll hit lands on the shifted
// geometry.
typedef struct {
    WLX_Context *ctx;
    WLX_Inputbox_State *state;
    const char *buffer;      // plain text
    size_t buf_len;
    const char *disp_text;   // display text (mask under password)
    size_t disp_len;
    WLX_Text_Style ts;
    WLX_Text_Line_Record *lines;
    size_t line_count;
    float max_scroll;
    bool password;
} WLX_Inputbox_Mouse_Ctx;

static size_t wlx_inputbox_mouse_hit(void *user, float x, float y) {
    WLX_Inputbox_Mouse_Ctx *mc = (WLX_Inputbox_Mouse_Ctx *)user;
    size_t hit_disp = wlx_text_offset_at_point_from_lines(mc->ctx, mc->disp_text, mc->disp_len,
        mc->ts, mc->lines, mc->line_count, x, y);
    return wlx_inputbox_plain_offset(mc->buffer, mc->buf_len, hit_disp, mc->password);
}

static void wlx_inputbox_mouse_word_bounds(void *user, size_t hit, size_t *start, size_t *end) {
    WLX_Inputbox_Mouse_Ctx *mc = (WLX_Inputbox_Mouse_Ctx *)user;
    size_t hit_disp = wlx_inputbox_display_offset(mc->buffer, mc->buf_len, hit, mc->password);
    size_t word_start = 0, word_end = 0;
    wlx_text_word_bounds(mc->disp_text, mc->disp_len, hit_disp, &word_start, &word_end);
    *start = wlx_inputbox_plain_offset(mc->buffer, mc->buf_len, word_start, mc->password);
    *end = wlx_inputbox_plain_offset(mc->buffer, mc->buf_len, word_end, mc->password);
}

static void wlx_inputbox_mouse_auto_scroll(void *user, float over_x, float over_y, float dt) {
    WLX_Inputbox_Mouse_Ctx *mc = (WLX_Inputbox_Mouse_Ctx *)user;
    (void)over_x;   // no horizontal scroll: the drag hit reaches past the band instead
    if (over_y == 0.0f || mc->max_scroll <= 0.0f) return;
    float new_scroll = mc->state->scroll_y + over_y * WLX_TEXT_DRAG_SCROLL_GAIN * dt;
    new_scroll = wlx_clampf(new_scroll, 0.0f, mc->max_scroll);
    float delta = new_scroll - mc->state->scroll_y;
    if (delta == 0.0f) return;
    mc->state->scroll_y = new_scroll;
    for (size_t i = 0; i < mc->line_count; i++) mc->lines[i].origin_y -= delta;
}

// Per-frame band and display-text resolve: the text band placed by the
// vertical component of opt.content_align (inset by the caller's icon band), and
// the password mask.
typedef struct {
    WLX_Rect text_rect;     // text band inside the field interior
    float line_h;           // backend-reported line height (fallback font_size)
    size_t buf_len;         // plaintext length
    const char *disp_text;  // display text (mask under password)
    size_t disp_len;
} WLX_Inputbox_Band;

static WLX_Inputbox_Band wlx_inputbox_resolve_band(WLX_Context *ctx,
    const WLX_Inputbox_Opt *opt, const char *buffer, WLX_Rect input_rect,
    WLX_Text_Style ts, float border_inset, float icon_band, bool icon_leading,
    char *mask_buf, size_t mask_cap)
{
    // Size the text band by the backend's reported line height, not by
    // opt.font_size. Leading, descenders, and any backend-side font scaling
    // make the rendered line taller than the nominal size; a band shorter
    // than the line gets clipped by the fitted-text scissor (cropped
    // descenders). The band is at least one line tall so a single line is
    // never clipped, and it is placed within the box interior by the
    // vertical component of opt.content_align: WLX_LEFT (default) centers, the
    // WLX_TOP_* family top-anchors (for tall multi-line note fields), and
    // the WLX_BOTTOM_* family bottom-anchors.
    float line_h = wlx_text_line_height(ctx, ts, NULL);

    float interior_h = input_rect.h - border_inset * 2.0f;
    if (interior_h < 0.0f) interior_h = 0.0f;
    float text_h = line_h > interior_h ? line_h : interior_h;

    float band_slack = interior_h - text_h;
    float band_off;
    switch (opt->content_align) {
        case WLX_TOP: case WLX_TOP_LEFT: case WLX_TOP_CENTER: case WLX_TOP_RIGHT:
            band_off = 0.0f; break;
        case WLX_BOTTOM: case WLX_BOTTOM_LEFT: case WLX_BOTTOM_CENTER: case WLX_BOTTOM_RIGHT:
            band_off = band_slack; break;
        default:
            band_off = band_slack * 0.5f; break;
    }

    // A leading icon pushes the text start right; a trailing icon only
    // narrows the band. Either way the band width drops by icon_band, and
    // the width is clamped so a narrow field can't produce a negative
    // width or push the caret clamp out of bounds.
    float text_lead_inset = WLX_TEXT_FIELD_INSET + (icon_leading ? icon_band : 0.0f);
    float text_w = input_rect.w - WLX_TEXT_FIELD_INSET - WLX_TEXT_CARET_WIDTH
                   - WLX_TEXT_CARET_PADDING - icon_band;
    if (text_w < 0.0f) text_w = 0.0f;

    WLX_Rect text_rect = {
        .x = input_rect.x + text_lead_inset,
        .y = input_rect.y + border_inset + band_off,
        .w = text_w,
        .h = text_h
    };

    // Password mode renders a mask (one byte per plaintext codepoint)
    // while the buffer keeps the plaintext. Every geometry query below
    // (hit test, line bounds, caret, highlight, draw) runs on the display
    // text; offsets map between the domains via the codepoint index.
    size_t buf_len = strlen(buffer);
    const char *disp_text = buffer;
    size_t disp_len = buf_len;
    if (opt->password) {
        disp_len = 0;
        size_t mask_off = 0;
        while (mask_off < buf_len && disp_len < mask_cap - 1) {
            mask_buf[disp_len++] = '*';
            mask_off = wlx_utf8_next(buffer, mask_off, buf_len);
        }
        mask_buf[disp_len] = '\0';
        disp_text = mask_buf;
    }

    return (WLX_Inputbox_Band){
        .text_rect = text_rect, .line_h = line_h,
        .buf_len = buf_len, .disp_text = disp_text, .disp_len = disp_len,
    };
}

// The frame's line records plus the scroll state resolved around them:
// scratch build with the scrollbar-width prediction, the strip decision
// and one rebuild at the final width, the wheel, the thumb gesture, and
// the virtual re-anchor for a scrolled run.
typedef struct {
    WLX_Text_Line_Record *lines;   // NULL on scratch allocation failure
    size_t line_count;
    WLX_Text_Line_Array_Result line_array;
    WLX_Rect text_rect;            // narrowed when the scrollbar shows
    WLX_Rect sb_track;
    float sb_w;
    bool sb_visible;
    float content_h;
    float max_scroll;
} WLX_Inputbox_Lines;

static WLX_Inputbox_Lines wlx_inputbox_build_lines(WLX_Context *ctx,
    const WLX_Inputbox_Opt *opt, WLX_Inputbox_State *state, WLX_Interaction inter,
    const char *disp_text, size_t disp_len, WLX_Text_Style ts, WLX_Rect text_rect,
    WLX_Rect input_rect, float border_inset)
{
    // One line-record build serves every geometry consumer below (mouse
    // hit-test, HOME/END and UP/DOWN line lookups, caret, selection
    // highlight, and the text draw). It runs after the key handling, so
    // every text mutation of this frame is already in the buffer and the
    // records cannot go stale mid-frame. On allocation failure the
    // consumers degrade to their empty-line behavior. Multiline fields
    // run on their own (larger) text-run budget; single-line fields
    // keep the global caps.
    size_t build_line_cap = opt->multiline
        ? WLX_INPUTBOX_MULTILINE_MAX_LINES : WLX_TEXT_RUN_MAX_LINES;
    size_t build_unit_cap = opt->multiline
        ? WLX_INPUTBOX_MULTILINE_MAX_UNITS : WLX_TEXT_RUN_MAX_UNITS;
    WLX_Text_Line_Record *lines = wlx_text_line_scratch(ctx, build_line_cap);
    WLX_Text_Line_Array_Result line_array = {0};

    // The scrollbar reserves a strip of the band when the run overflows,
    // but the run must be built at some width before the overflow is
    // known. In wrap mode content height is monotonic in width (a
    // narrower band only adds wrap lines), so last frame's visibility
    // picks this frame's probe width: a correct guess costs one build
    // instead of two, and a wrong one is caught below and rebuilt once
    // at the final width. Content that overflows the narrowed band while
    // fitting the full one keeps its scrollbar (sticky hysteresis; the
    // bar stays scrollable). Non-wrap fields never predict: their build
    // stops at the first width-truncated line, which makes content
    // height anti-monotonic in width and would let a narrow probe flip
    // the decision every frame.
    float sb_w = ctx->theme->scrollbar.width > 0.0f
        ? ctx->theme->scrollbar.width : WLX_SCROLLBAR_FALLBACK_WIDTH;
    WLX_Rect sb_track = { input_rect.x + border_inset, text_rect.y,
                          input_rect.w - border_inset * 2.0f, text_rect.h };
    bool predict_sb = opt->multiline && opt->wrap && opt->show_scrollbar
        && state->sb_was_visible;
    WLX_Rect probe_rect = text_rect;
    if (predict_sb) {
        probe_rect.w -= sb_w;
        if (probe_rect.w < 0.0f) probe_rect.w = 0.0f;
    }
    if (lines != NULL) {
        wlx_text_prepare_lines_slice(ctx, probe_rect, disp_text, disp_len, ts,
            (WLX_Text_Prepare_Opt){ .align = opt->content_align, .wrap = opt->wrap,
                                    .text_unit_cap = build_unit_cap },
            lines, build_line_cap, &line_array);
    }
    size_t line_count = lines != NULL ? line_array.line_count : 0;

    // Multiline content taller than the band scrolls: the line records
    // are re-anchored to a virtual rect starting scroll_y pixels above
    // the band, so every consumer below (hit-test, caret, selection,
    // draw) sees the shifted geometry and the band-clipping draws crop
    // it to the visible window. The virtual rect is exactly content
    // tall, which top-anchors the run regardless of the vertical align
    // component; when content fits, scroll_y clamps to zero and the
    // band alignment applies untouched.
    float content_h = (float)line_count * line_array.line_h;

    bool sb_visible = opt->multiline && opt->show_scrollbar && content_h > text_rect.h;
    if (sb_visible) {
        text_rect.w -= sb_w;
        if (text_rect.w < 0.0f) text_rect.w = 0.0f;
    }
    if (sb_visible != predict_sb && lines != NULL) {
        wlx_text_prepare_lines_slice(ctx, text_rect, disp_text, disp_len, ts,
            (WLX_Text_Prepare_Opt){ .align = opt->content_align, .wrap = opt->wrap,
                                    .text_unit_cap = build_unit_cap },
            lines, build_line_cap, &line_array);
        line_count = line_array.line_count;
        content_h = (float)line_count * line_array.line_h;
    }
    state->sb_was_visible = sb_visible;

    float max_scroll = 0.0f;
    if (opt->multiline && content_h > text_rect.h) max_scroll = content_h - text_rect.h;
    state->scroll_y = wlx_clampf(state->scroll_y, 0.0f, max_scroll);

    // Wheel scrolling: a hovered field with scrollable overflow owns the
    // wheel; the consume helper leaves the delta to the enclosing panel
    // when nothing can move.
    if (inter.hover && !inter.disabled) {
        wlx_wheel_consume(ctx, &state->scroll_y, max_scroll,
            WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED);
    }

    // Scrollbar thumb drag on raw mouse primitives. Runs before the
    // caret mouse block below, which skips presses on the bar strip so
    // a thumb press never places the caret or starts a selection.
    if (sb_visible && !inter.disabled) {
        WLX_Rect sb_rect = wlx_scrollbar_rect(sb_track, content_h, state->scroll_y, sb_w);
        state->scroll_y = wlx_thumb_drag_update(ctx, sb_track, sb_rect, true,
            max_scroll, state->scroll_y,
            &state->caret.dragging_scrollbar, &state->caret.sb_drag_offset, NULL);
    } else {
        state->caret.dragging_scrollbar = false;
    }

    if (max_scroll > 0.0f) {
        WLX_Rect virt_rect = { text_rect.x, text_rect.y - state->scroll_y, text_rect.w, content_h };
        wlx_text_align_lines(virt_rect, opt->content_align, line_array.line_h,
            WLX_VMETRIC_LINE_HEIGHT, ts.font_size, lines, line_count);
    }

    return (WLX_Inputbox_Lines){
        .lines = lines, .line_count = line_count, .line_array = line_array,
        .text_rect = text_rect, .sb_track = sb_track, .sb_w = sb_w,
        .sb_visible = sb_visible, .content_h = content_h, .max_scroll = max_scroll,
    };
}

// Pointer and line-based caret input for a focused inputbox, resolved
// against the frame's line records (the same records the caret and draw
// use, so the jump targets always match what is on screen): the pointer
// driver behind the scrollbar-strip exclusion, HOME/END on the visual
// line (command modifier stretches to the whole buffer), and UP/DOWN
// visual-line motion with the sticky column.
static void wlx_inputbox_caret_input(WLX_Context *ctx, const WLX_Inputbox_Opt *opt,
    WLX_Inputbox_State *state, WLX_Interaction inter, const char *buffer,
    const WLX_Inputbox_Band *ib, const WLX_Inputbox_Lines *il, WLX_Text_Style ts,
    WLX_Rect input_rect)
{
    if (!inter.focused || inter.disabled) return;

    const char *disp_text = ib->disp_text;
    size_t disp_len = ib->disp_len;
    size_t buf_len = ib->buf_len;
    WLX_Text_Line_Record *lines = il->lines;
    size_t line_count = il->line_count;
    WLX_Rect text_rect = il->text_rect;
    bool shift = wlx_mod_down(ctx, WLX_MOD_SHIFT);

    // Mouse: a click inside the field places the caret at the nearest
    // text boundary; repeated clicks on the same spot widen the
    // selection to word then all; dragging while held extends it.
    // Presses on the scrollbar strip belong to the thumb gesture and
    // must not touch the caret or the selection.
    bool sb_strip_hit = il->sb_visible && wlx_rect_contains(
        (WLX_Rect){ il->sb_track.x + il->sb_track.w - il->sb_w, il->sb_track.y,
                    il->sb_w, il->sb_track.h },
        (float)ctx->input.mouse_x, (float)ctx->input.mouse_y);
    bool press = ctx->input.mouse_clicked && !sb_strip_hit
        && !state->caret.dragging_scrollbar
        && wlx_rect_contains(input_rect,
            (float)ctx->input.mouse_x, (float)ctx->input.mouse_y);
    WLX_Inputbox_Mouse_Ctx mouse_ctx = {
        .ctx = ctx, .state = state,
        .buffer = buffer, .buf_len = buf_len,
        .disp_text = disp_text, .disp_len = disp_len,
        .ts = ts, .lines = lines, .line_count = line_count,
        .max_scroll = il->max_scroll, .password = opt->password,
    };
    WLX_Text_Mouse_Ops mouse_ops = {
        .user = &mouse_ctx,
        .hit = wlx_inputbox_mouse_hit,
        .word_bounds = wlx_inputbox_mouse_word_bounds,
        .auto_scroll = wlx_inputbox_mouse_auto_scroll,
    };
    wlx_text_edit_handle_mouse(ctx, &state->caret, text_rect, buf_len,
        shift, press, &mouse_ops);

    // HOME/END jump within the visual line under the caret; the
    // command modifier stretches the jump to the whole buffer, and
    // SHIFT extends the selection instead of collapsing it.
    bool home_hit = wlx_is_key_actuated(ctx, WLX_KEY_HOME);
    bool end_hit  = wlx_is_key_actuated(ctx, WLX_KEY_END);
    if (home_hit || end_hit) {
        if (wlx_mod_command_down(ctx)) {
            state->caret.cursor_pos = end_hit ? buf_len : 0;
        } else {
            size_t disp_cursor = wlx_inputbox_display_offset(buffer, buf_len,
                state->caret.cursor_pos, opt->password);
            size_t line_start = 0, line_end = disp_len;
            wlx_inputbox_visual_line_bounds_from_lines(disp_text, disp_len,
                lines, line_count, disp_cursor, &line_start, &line_end);
            state->caret.cursor_pos = wlx_inputbox_plain_offset(buffer, buf_len,
                end_hit ? line_end : line_start, opt->password);
        }
        if (!shift) state->caret.selection_anchor = state->caret.cursor_pos;
        state->caret.cursor_blink_time = 0.0f;
        state->caret.preferred_x_valid = false;
    }

    // UP/DOWN move the caret to the adjacent visual line, aiming at a
    // sticky column: the first vertical move latches the caret x and
    // later moves keep aiming at it across shorter lines, until a
    // horizontal caret change invalidates it. UP on the first line
    // clamps to the line start, DOWN on the last line to the line end
    // (and the clamp drops the latched column). SHIFT extends the
    // selection; without it the anchor follows the caret.
    bool up_hit   = opt->multiline && wlx_is_key_actuated(ctx, WLX_KEY_UP);
    bool down_hit = opt->multiline && wlx_is_key_actuated(ctx, WLX_KEY_DOWN);
    if (up_hit != down_hit && line_count > 0) {
        size_t disp_cursor = wlx_text_normalize_cursor_offset(disp_text, il->line_array.text_length,
            wlx_inputbox_display_offset(buffer, buf_len, state->caret.cursor_pos, opt->password));

        size_t line_idx = line_count - 1;
        for (size_t i = 0; i < line_count; i++) {
            if (wlx_text_cursor_is_on_line(&lines[i], disp_cursor)) { line_idx = i; break; }
        }

        if (!state->caret.preferred_x_valid) {
            float caret_x = text_rect.x;
            float caret_y = text_rect.y;
            wlx_text_resolve_cursor_from_lines(ctx, text_rect, disp_text, disp_len, ts,
                lines, line_count, disp_cursor, &caret_x, &caret_y);
            state->caret.preferred_x = caret_x;
            state->caret.preferred_x_valid = true;
        }

        size_t target_disp;
        if (up_hit && line_idx == 0) {
            target_disp = lines[0].cursor_start;
            state->caret.preferred_x_valid = false;
        } else if (down_hit && line_idx == line_count - 1) {
            target_disp = lines[line_idx].cursor_end;
            state->caret.preferred_x_valid = false;
        } else {
            size_t target_idx = up_hit ? line_idx - 1 : line_idx + 1;
            target_disp = wlx_text_offset_at_point_from_lines(ctx, disp_text, disp_len, ts,
                lines, line_count, state->caret.preferred_x,
                lines[target_idx].origin_y + lines[target_idx].line_h * 0.5f);
        }

        state->caret.cursor_pos = wlx_inputbox_plain_offset(buffer, buf_len, target_disp, opt->password);
        if (!shift) state->caret.selection_anchor = state->caret.cursor_pos;
        state->caret.cursor_blink_time = 0.0f;
    }
}

// Resolve the caret from the shared line records so it cannot drift from
// the wrapped lines, explicit newlines, and long buffers the draw
// renders, then caret-follow: a caret move or edit this frame drags the
// view the minimal distance that puts the caret line fully inside the
// band, shifting the already-positioned records in place. Wheel scrolling
// never moves the caret, so it may park the caret outside the window; the
// next caret change snaps the view back to it.
static void wlx_inputbox_caret_resolve(WLX_Context *ctx, const WLX_Inputbox_Opt *opt,
    WLX_Inputbox_State *state, WLX_Interaction inter, bool changed, const char *buffer,
    const WLX_Inputbox_Band *ib, const WLX_Inputbox_Lines *il, WLX_Text_Style ts,
    float *out_cursor_x, float *out_cursor_y)
{
    const char *disp_text = ib->disp_text;
    size_t disp_len = ib->disp_len;
    WLX_Rect text_rect = il->text_rect;
    float cursor_x = text_rect.x;
    float cursor_y = text_rect.y;

    // An empty buffer has no line records; anchor the caret where an
    // empty run would start under this alignment.
    if (inter.focused) {
        if (disp_len == 0) {
            WLX_Rect aligned = wlx_get_align_rect(text_rect, 0.0f, il->line_array.line_h, opt->content_align);
            cursor_x = aligned.x;
            cursor_y = aligned.y;
        } else {
            wlx_text_resolve_cursor_from_lines(ctx, text_rect, disp_text, disp_len, ts,
                il->lines, il->line_count,
                wlx_text_normalize_cursor_offset(disp_text, disp_len,
                    wlx_inputbox_display_offset(buffer, ib->buf_len, state->caret.cursor_pos, opt->password)),
                &cursor_x, &cursor_y);
        }
    }

    if (inter.focused && il->max_scroll > 0.0f
        && (changed || state->caret.cursor_pos != state->caret.prev_cursor_pos)) {
        float follow = 0.0f;
        if (cursor_y < text_rect.y) {
            follow = cursor_y - text_rect.y;
        } else if (cursor_y + il->line_array.line_h > text_rect.y + text_rect.h) {
            follow = (cursor_y + il->line_array.line_h) - (text_rect.y + text_rect.h);
        }
        if (follow != 0.0f) {
            float new_scroll = state->scroll_y + follow;
            new_scroll = wlx_clampf(new_scroll, 0.0f, il->max_scroll);
            float delta = new_scroll - state->scroll_y;
            if (delta != 0.0f) {
                state->scroll_y = new_scroll;
                for (size_t i = 0; i < il->line_count; i++) il->lines[i].origin_y -= delta;
                cursor_y -= delta;
            }
        }
    }
    if (inter.focused) state->caret.prev_cursor_pos = state->caret.cursor_pos;

    *out_cursor_x = cursor_x;
    *out_cursor_y = cursor_y;
}

// Draw pass over the frame's records: selection highlight between the
// field background and the text, the text run, the caret (centered on
// its line, clamped to the field interior with a breathing margin), and
// the scrollbar thumb last so it overlays the band edge.
static void wlx_inputbox_draw_content(WLX_Context *ctx, const WLX_Inputbox_Opt *opt,
    WLX_Inputbox_State *state, WLX_Interaction inter, const char *buffer,
    const WLX_Inputbox_Band *ib, const WLX_Inputbox_Lines *il, WLX_Text_Style ts,
    WLX_Rect input_rect, float border_inset, float cursor_x, float cursor_y)
{
    const char *disp_text = ib->disp_text;
    size_t disp_len = ib->disp_len;
    WLX_Rect text_rect = il->text_rect;
    float line_h = ib->line_h;

    if (inter.focused && wlx_text_edit_has_selection(&state->caret)) {
        wlx_text_draw_selection(ctx, text_rect, disp_text, disp_len, ts, 0.0f,
            NULL, NULL, il->lines, il->line_count,
            wlx_inputbox_display_offset(buffer, ib->buf_len, wlx_text_edit_selection_min(&state->caret), opt->password),
            wlx_inputbox_display_offset(buffer, ib->buf_len, wlx_text_edit_selection_max(&state->caret), opt->password),
            opt->selection_color);
    }

    wlx_draw_text_lines_fitted(ctx, text_rect, disp_text, disp_len, ts, il->lines, il->line_count);

    if (cursor_x > text_rect.x)
        cursor_x += WLX_TEXT_CARET_PADDING;

    // cursor_y is the top of the line band (line_h tall). Center a
    // font_size-tall caret on the line so it tracks the vertically
    // centered text, then clamp both ends to the field interior (with a
    // small breathing margin) so a line height taller than a cramped
    // interior -- e.g. a small font in a short field, where the backend's
    // reported line_h exceeds the box interior -- can't produce a caret
    // that spans the whole box and sits on the bottom border. When the
    // interior is too tight to keep the full font_size height, the caret
    // shrinks to fit rather than overflow.
    float caret_top = cursor_y + (line_h - (float)opt->font_size) * 0.5f;
    float caret_bottom = caret_top + (float)opt->font_size;
    float caret_limit_top = input_rect.y + border_inset + WLX_INPUTBOX_CARET_MARGIN;
    float caret_limit_bottom = input_rect.y + input_rect.h - border_inset - WLX_INPUTBOX_CARET_MARGIN;
    if (caret_limit_bottom > caret_limit_top) {
        if (caret_top < caret_limit_top) caret_top = caret_limit_top;
        if (caret_bottom > caret_limit_bottom) caret_bottom = caret_limit_bottom;
    }
    float cursor_height = caret_bottom - caret_top;
    if (cursor_height < 1.0f) cursor_height = 1.0f;
    float caret_y = caret_top;
    if (inter.focused) {
        state->caret.cursor_blink_time += wlx_get_frame_time(ctx);
    }

    // Draw cursor if focused and its line intersects the text rect. The
    // top-side check runs on the unclamped line top: a caret line
    // scrolled above the band must not clamp into view as a sliver
    // pinned to the top border.
    if (inter.focused && cursor_x < (text_rect.x + text_rect.w)
        && (cursor_y + line_h) > text_rect.y
        && (caret_y + cursor_height) <= (text_rect.y + text_rect.h + 1.0f)
        && wlx_text_caret_blink_on(state->caret.cursor_blink_time)) {
        wlx_text_caret_draw(ctx, text_rect, cursor_x, caret_y, cursor_height, opt->cursor_color);
    }

    // The thumb rect is recomputed here because wheel, drag, and
    // caret-follow may all have moved the offset since the interaction
    // pass.
    if (il->sb_visible) {
        wlx_scrollbar_thumb_draw(ctx,
            wlx_scrollbar_rect(il->sb_track, il->content_h, state->scroll_y, il->sb_w),
            state->caret.dragging_scrollbar);
    }
}

WLXDEF bool wlx_inputbox_impl(WLX_Context *ctx, const char *label, char *buffer, size_t buffer_size,
    WLX_Inputbox_Opt opt, const char *file, int line)
{
    assert(ctx != NULL);
    assert(buffer != NULL && "inputbox buffer must not be NULL");
    WLX_HARD_ASSERT(buffer_size >= 2, "buffer_size must hold at least 1 char + null terminator");
    wlx_resolve_opt_inputbox(ctx, &opt);

    WLX_Resolved_Padding rp = WLX_RESOLVE_CONTENT_PADDING_EX(ctx, opt, WLX_INPUTBOX_CONTENT_PADDING);

    // Ensure height can fit the font plus content padding on both sides.
    float min_h = (float)opt.font_size + rp.top + rp.bottom + WLX_TEXT_FIELD_MIN_HEIGHT_SLACK;
    if (opt.height > 0 && opt.height < min_h) opt.height = min_h;

    // Prologue: compute widget frame and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;

    // Scale the resolved padding so the content rect never has negative
    // dimensions even when the layout slot constrains wr below opt.height.
    wlx_clamp_resolved_padding(&rp, wr.w, wr.h);

    WLX_Interaction inter = wlx_get_interaction_for(
        ctx,
        wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS | WLX_INTERACT_TEXT_CURSOR
            | (opt.multiline ? WLX_INTERACT_FOCUS_HOLD_ENTER : 0),
        opt.disabled,
        file, line
    );

    WLX_State persistent = wlx_get_state_impl(ctx, sizeof(WLX_Inputbox_State), file, line);
    // Per-widget persistent cursor state
    WLX_Inputbox_State *state = (WLX_Inputbox_State *)persistent.data;

    wlx_text_edit_tick_click_clock(ctx, &state->caret);

    bool changed = false;
    if (inter.focused) {
        changed = wlx_inputbox_handle_keys(ctx, state, buffer, buffer_size, inter.just_focused,
            opt.read_only, opt.password, opt.multiline, persistent.id, opt.revision);
    }
    if (opt.out_focused != NULL) *opt.out_focused = inter.focused;

    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size, .color = opt.front_color, .spacing = opt.spacing };
    WLX_Text_Field_Chrome chrome = WLX_TEXT_FIELD_CHROME(opt);
    WLX_Text_Field_Frame field = wlx_text_field_frame(ctx, wr, rp, label, ts,
        opt.content_align, opt.wrap, inter.hover, inter.focused, inter.disabled, &chrome);
    WLX_Rect input_rect = field.input_rect;

    // Reserve an interior band for an optional leading/trailing icon and draw
    // the glyph centered vertically within the field interior. The band insets
    // the text and caret below so they never overlap the icon. Centering is on
    // the field interior, independent of opt.content_align - a glyph is not text and
    // must not follow a multi-line text band. icon_band stays 0 when no texture
    // is set, so a text-only field keeps its original geometry exactly.
    float icon_band = 0.0f;
    bool icon_leading = (opt.image_placement != WLX_IMAGE_PLACEMENT_RIGHT);
    if (wlx_widget_has_image(opt.texture)) {
        float icon_border_inset = opt.border_width > 0 ? opt.border_width + 1.0f : 0.0f;
        float icon_interior_h = input_rect.h - icon_border_inset * 2.0f;
        if (icon_interior_h < 0.0f) icon_interior_h = 0.0f;

        float icon_sz = wlx_widget_auto_image_size(
            opt.image_size, WLX_IMAGE_PLACEMENT_LEFT, input_rect,
            (float)opt.font_size, true);
        if (icon_sz > icon_interior_h) icon_sz = icon_interior_h;

        if (icon_sz > 0.0f) {
            float icon_x = icon_leading
                ? input_rect.x + WLX_TEXT_FIELD_INSET
                : input_rect.x + input_rect.w - WLX_TEXT_FIELD_INSET - icon_sz;
            float icon_y = input_rect.y + (input_rect.h - icon_sz) * 0.5f;
            WLX_Rect icon_cell = { icon_x, icon_y, icon_sz, icon_sz };

            WLX_Rect icon_src = wlx_widget_image_src(opt.texture, opt.texture_src);
            WLX_Rect fit_src, fit_dst;
            wlx_resolve_image_fit(icon_cell, icon_src, WLX_IMAGE_SCALE_FIT, WLX_CENTER,
                                  &fit_src, &fit_dst);
            wlx_draw_texture(ctx, opt.texture, fit_src, fit_dst, opt.texture_tint);

            icon_band = icon_sz + opt.image_text_gap;
        }
    }

    // Draw text content
    if (opt.font_size > 0 && buffer != NULL) {
        float border_inset = opt.border_width > 0 ? opt.border_width + 1.0f : 0.0f;

        char mask_buf[WLX_INPUTBOX_MASK_MAX];
        WLX_Inputbox_Band ib = wlx_inputbox_resolve_band(ctx, &opt, buffer, input_rect,
            ts, border_inset, icon_band, icon_leading, mask_buf, sizeof(mask_buf));

        WLX_Inputbox_Lines il = wlx_inputbox_build_lines(ctx, &opt, state, inter,
            ib.disp_text, ib.disp_len, ts, ib.text_rect, input_rect, border_inset);

        // Pointer and line-based interactions are resolved here rather than
        // with the other key handling because they need the resolved text
        // geometry (the same line records the caret and draw use below).
        wlx_inputbox_caret_input(ctx, &opt, state, inter, buffer, &ib, &il, ts, input_rect);

        float cursor_x = 0.0f, cursor_y = 0.0f;
        wlx_inputbox_caret_resolve(ctx, &opt, state, inter, changed, buffer,
            &ib, &il, ts, &cursor_x, &cursor_y);

        wlx_inputbox_draw_content(ctx, &opt, state, inter, buffer, &ib, &il, ts,
            input_rect, border_inset, cursor_x, cursor_y);
    }

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);
    return changed;
}

static void wlx_resolve_opt_slider(const WLX_Context *ctx, WLX_Slider_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;

    // Widget-specific fallbacks
    if (wlx_color_is_zero(opt->track_color)) opt->track_color = theme->slider.track;
    if (wlx_color_is_zero(opt->thumb_color)) opt->thumb_color = theme->slider.thumb;
    if (wlx_color_is_zero(opt->label_color)) opt->label_color = theme->slider.label;
    if (opt->track_height <= 0)              opt->track_height = theme->slider.track_height;
    if (opt->thumb_width  <= 0)              opt->thumb_width  = theme->slider.thumb_width;

    // Common helpers (typography, border, roundness)
    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width,
                       &opt->roundness, &opt->rounded_segments);
    if (opt->rounded_segments < theme->min_rounded_segments)
        opt->rounded_segments = theme->min_rounded_segments;

    // Brightness derivations: thumb defaults to half of base hover.
    if (wlx_is_float_unset(opt->hover_brightness))
        opt->hover_brightness = theme->hover_brightness;
    if (wlx_is_float_unset(opt->thumb_hover_brightness))
        opt->thumb_hover_brightness = opt->hover_brightness * 0.5f;

    WLX_RESOLVE_VISUAL_STATE(ctx, opt, opt->disabled,
        &opt->track_color, &opt->thumb_color, &opt->label_color, &opt->border_color,
        &opt->shadow_color, &opt->glow_color);
}

WLXDEF bool wlx_slider_impl(WLX_Context *ctx, const char *label, float *value, WLX_Slider_Opt opt, const char *file, int line) {
    assert(ctx != NULL);
    assert(value != NULL && "slider value pointer must not be NULL");
    wlx_resolve_opt_slider(ctx, &opt);

    // Prologue: compute widget frame and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;

    WLX_Rect content_rect = WLX_RESOLVE_CONTENT_RECT(ctx, opt, wr);

    bool changed = false;

    // Layout: [label] [track with thumb] [value text]
    float label_width = 0;
    float value_text_width = 0;
    float padding = opt.font_size / 2.0f;
    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size, .color = opt.label_color, .spacing = opt.spacing };

    size_t label_len = (label != NULL) ? strlen(label) : 0;
    if (label != NULL && opt.font_size > 0) {
        float label_h = 0;
        wlx_measure_text_slice(ctx, label, label_len, ts, &label_width, &label_h);
        label_width += padding;
    }

    char value_str[32] = {0};
    if (opt.show_value) {
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
    float track_x = content_rect.x + label_width;
    float track_w = content_rect.w - label_width - value_text_width;
    if (track_w < opt.thumb_width) track_w = opt.thumb_width;
    float track_y = content_rect.y + (content_rect.h - opt.track_height) / 2.0f;

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
    float thumb_min_h = opt.track_height + 4.0f;
    float thumb_max_h = content_rect.h - 4.0f;
    if (thumb_max_h < opt.track_height) thumb_max_h = opt.track_height;
    float thumb_h = opt.thumb_width;
    if (thumb_h < thumb_min_h) thumb_h = thumb_min_h;
    if (thumb_h > thumb_max_h) thumb_h = thumb_max_h;
    if (thumb_h < 1.0f) thumb_h = 1.0f;
    float thumb_y = roundf(content_rect.y + (content_rect.h - thumb_h) / 2.0f);

    // Interaction via unified handler (drag mode for continuous value updates)
    WLX_Rect hit_rect = { track_x, content_rect.y, track_w, content_rect.h };

    WLX_Interaction inter = wlx_get_interaction_for(
        ctx,
        hit_rect,
        WLX_INTERACT_HOVER | WLX_INTERACT_DRAG,
        opt.disabled,
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
    bool is_hover = inter.hover && !inter.disabled;

    // Draw label
    if (label != NULL && opt.font_size > 0) {
        WLX_Rect label_rect = { content_rect.x, content_rect.y, label_width, content_rect.h };
        wlx_draw_text_fitted_slice(ctx, label_rect, label, label_len, ts,
            (WLX_Text_Prepare_Opt){ .align = WLX_LEFT });
    }

    // Draw track
    WLX_Color color_track = wlx_color_hover_tint(
        opt.track_color, is_hover || is_active, inter.disabled, opt.hover_brightness);
    WLX_Rect track_rect = { track_x, track_y, track_w, opt.track_height };
    wlx_draw_rect_rounded(ctx, track_rect, opt.roundness, opt.rounded_segments, color_track);

    // Draw track border
    if (opt.border_width > 0) {
        wlx_draw_rect_rounded_lines(ctx, track_rect, opt.roundness, opt.rounded_segments, opt.border_width, opt.border_color);
    }

    // Draw filled portion of track (extends under the thumb so the
    // rounded right corner is hidden; the thumb is taller than the track)
    float fill_w = thumb_cx - track_x;
    if (fill_w > 0) {
        WLX_Color fill_color = wlx_color_brightness(opt.thumb_color, opt.fill_inactive_brightness);
        WLX_Rect fill_rect = { track_x, track_y, fill_w, opt.track_height };
        wlx_draw_rect_rounded(ctx, fill_rect, opt.roundness, opt.rounded_segments, fill_color);
    }

    // Draw thumb: active takes priority over hover; both gate disabled
    // through the helper for parity with other widgets.
    WLX_Color color_thumb = opt.thumb_color;
    if (is_active) {
        color_thumb = wlx_color_hover_tint(
            color_thumb, true, inter.disabled, opt.hover_brightness);
    } else {
        color_thumb = wlx_color_hover_tint(
            color_thumb, is_hover, inter.disabled, opt.thumb_hover_brightness);
    }
    WLX_Rect thumb_rect = { thumb_cx - half_thumb, thumb_y, opt.thumb_width, thumb_h };
    // Soft effect rides behind the thumb: emit before the thumb fill so the
    // glow/shadow sits under it. Gated on a non-zero effect color.
    if (!wlx_color_is_zero(opt.glow_color) || !wlx_color_is_zero(opt.shadow_color)) {
        wlx_draw_box(ctx, thumb_rect, (WLX_Box_Style){
            .roundness        = opt.roundness,
            .rounded_segments = opt.rounded_segments,
            WLX_BOX_STYLE_EFFECTS(opt),
        });
    }
    wlx_draw_rect_rounded(ctx, thumb_rect, opt.roundness, opt.rounded_segments, color_thumb);

    // Draw value text (fixed-width area, clipped per-glyph)
    if (opt.show_value && opt.font_size > 0) {
        // Recalculate value string in case it changed during drag
        snprintf(value_str, sizeof(value_str), "%.2f", *value);
        float vt_x = track_x + track_w;
        WLX_Rect value_rect = { vt_x, content_rect.y, value_text_width + padding, content_rect.h };
        wlx_draw_text_fitted(ctx, value_rect, value_str, ts, WLX_LEFT, false);
    }

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);
    return changed;
}

static void wlx_resolve_opt_separator(const WLX_Context *ctx, WLX_Separator_Opt *opt) {
    if (wlx_color_is_zero(opt->back_color)) opt->back_color = ctx->theme->border;
    WLX_RESOLVE_VISUAL_STATE(ctx, opt, false, &opt->back_color);
}

WLXDEF void wlx_separator_impl(WLX_Context *ctx, WLX_Separator_Opt opt, const char *file, int line)
{
    wlx_resolve_opt_separator(ctx, &opt);

    WLX_Widget_Layout wly = WLX_WIDGET_LAYOUT(opt);
    if (wly.width <= 0 && wlx_parent_wants_intrinsic_width(ctx, wly.pos)) {
        // In a width-consuming slot the separator renders as a vertical
        // divider; its natural width is the line thickness.
        wly.intrinsic_w = opt.thickness;
    }

    // Prologue: compute widget frame (no interaction for separator)
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, wly, file, line);
    WLX_Rect wr = frame.rect;

    float t = opt.thickness;
    float cx = wr.x + wr.w * 0.5f;
    float cy = wr.y + wr.h * 0.5f;

    if (wr.w >= wr.h) {
        wlx_draw_line(ctx, wr.x, cy, wr.x + wr.w, cy, t, opt.back_color);
    } else {
        wlx_draw_line(ctx, cx, wr.y, cx, wr.y + wr.h, t, opt.back_color);
    }

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);
}

// round(value * segments), clamped to [0, segments]. Maps a continuous 0..1
// progress value to a discrete count of filled cells for segmented mode.
static int wlx_progress_filled_count(float value, int segments) {
    if (segments <= 0) return 0;
    if (value <= 0.0f) return 0;
    if (value >= 1.0f) return segments;
    int n = (int)(value * (float)segments + 0.5f);
    if (n < 0) n = 0;
    if (n > segments) n = segments;
    return n;
}

// i-th cell rect within `track`: equal floor-rounded widths, last cell absorbs
// the sub-pixel remainder, 1px minimum. `gap` is the resolved (>0) inter-cell gap.
static WLX_Rect wlx_progress_cell_rect(WLX_Rect track, int segments,
                                       float gap, int i) {
    if (segments < 1) segments = 1;

    // Gap compression: when the inter-cell gaps alone would fill or overflow
    // the track, shrink the gap so every cell still reserves at least 1px.
    // Guarded on segments > 1 to avoid a divide-by-zero.
    if (segments > 1 && gap * (float)(segments - 1) >= track.w) {
        gap = floorf((track.w - (float)segments) / (float)(segments - 1));
        if (gap < 0.0f) gap = 0.0f;
    }

    float cell_w = floorf((track.w - gap * (float)(segments - 1)) / (float)segments);
    if (cell_w < 1.0f) cell_w = 1.0f;
    float last_w = track.w - cell_w * (float)(segments - 1)
                           - gap * (float)(segments - 1);
    if (last_w < 1.0f) last_w = 1.0f;
    float x = track.x + (float)i * (cell_w + gap);
    float w = (i == segments - 1) ? last_w : cell_w;

    // Trailing-edge clamp: never let a cell draw past the track. A cell whose
    // origin already sits at/after the track's right edge collapses to a
    // zero-width sliver pinned to that edge; otherwise the width is capped to
    // the remaining span inside the track.
    float track_right = track.x + track.w;
    if (x >= track_right) {
        x = track_right;
        w = 0.0f;
    } else if (x + w > track_right) {
        w = track_right - x;
    }
    if (w < 0.0f) w = 0.0f;

    return (WLX_Rect){ x, track.y, w, track.h };
}

static void wlx_resolve_opt_progress(const WLX_Context *ctx, WLX_Progress_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    if (wlx_color_is_zero(opt->track_color))  opt->track_color  = wlx_color_or(theme->progress.track, theme->slider.track);
    if (wlx_color_is_zero(opt->fill_color))   opt->fill_color   = wlx_color_or(theme->progress.fill,  theme->accent);
    if (wlx_color_is_zero(opt->border_color)) opt->border_color = theme->border;
    if (wlx_is_negative_unset(opt->border_width))      opt->border_width = theme->border_width;
    if (opt->track_height <= 0)               opt->track_height = theme->progress.track_height > 0 ? theme->progress.track_height : theme->slider.track_height;
    if (opt->min_height <= 0)                 opt->min_height   = opt->track_height;
    if (opt->segment_gap <= 0)                opt->segment_gap  = theme->progress.segment_gap > 0 ? theme->progress.segment_gap : 2.0f;
    wlx_resolve_roundness(theme, &opt->roundness, &opt->rounded_segments);
    WLX_RESOLVE_VISUAL_STATE(ctx, opt, false,
        &opt->track_color, &opt->fill_color, &opt->border_color,
        &opt->shadow_color, &opt->glow_color);
}

WLXDEF void wlx_progress_impl(WLX_Context *ctx, float value, WLX_Progress_Opt opt, const char *file, int line) {
    assert(ctx != NULL);

    wlx_resolve_opt_progress(ctx, &opt);

    // Prologue: compute widget frame (no interaction for progress bar)
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;

    WLX_Rect content_rect = WLX_RESOLVE_CONTENT_RECT(ctx, opt, wr);

    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;

    float th = opt.track_height > 0 ? opt.track_height : content_rect.h;
    if (th > content_rect.h) th = content_rect.h;
    WLX_Rect track = { content_rect.x, content_rect.y + (content_rect.h - th) / 2.0f, content_rect.w, th };

    // Segmented (discrete) mode: draw only N cells. The inactive cells are the
    // track and the gaps are transparent, so no continuous track box or border
    // is drawn. The continuous path below is reached only when segments == 0.
    if (opt.segments > 0) {
        int filled = wlx_progress_filled_count(value, opt.segments);
        // Soft effect rides behind the active run: one effect-only box spanning
        // the first..last filled cell, emitted before the per-cell loop so the
        // glow/shadow sits under the cells. Gated on a non-zero effect color.
        if (filled > 0 &&
            (!wlx_color_is_zero(opt.glow_color) || !wlx_color_is_zero(opt.shadow_color))) {
            WLX_Rect first = wlx_progress_cell_rect(track, opt.segments, opt.segment_gap, 0);
            WLX_Rect last  = wlx_progress_cell_rect(track, opt.segments, opt.segment_gap, filled - 1);
            WLX_Rect run   = { first.x, track.y, (last.x + last.w) - first.x, track.h };
            wlx_draw_box(ctx, run, (WLX_Box_Style){
                .roundness        = opt.roundness,
                .rounded_segments = opt.rounded_segments,
                WLX_BOX_STYLE_EFFECTS(opt),
            });
        }
        for (int i = 0; i < opt.segments; i++) {
            WLX_Rect cell = wlx_progress_cell_rect(track, opt.segments, opt.segment_gap, i);
            WLX_Color c = (i < filled) ? opt.fill_color : opt.track_color;
            wlx_draw_box(ctx, cell, (WLX_Box_Style){
                .fill             = c,
                .roundness        = opt.roundness,
                .rounded_segments = opt.rounded_segments,
            });
        }
        wlx_widget_frame_end(ctx, frame);
        return;
    }

    WLX_Border_Sides progress_sides = wlx_border_sides_for_widget(
        ctx->theme, false, false, opt.opacity,
        opt.border_color, opt.border_width,
        WLX_BORDER_SIDES_ARGS(opt));
    wlx_draw_box(ctx, track, (WLX_Box_Style){
        .fill            = opt.track_color,
        .border          = opt.border_color,
        .border_width    = opt.border_width,
        .roundness       = opt.roundness,
        .rounded_segments = opt.rounded_segments,
        .sides           = progress_sides,
        .per_side        = true,
        WLX_BOX_STYLE_EFFECTS(opt),
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

static void wlx_resolve_opt_image(const WLX_Context *ctx, WLX_Image_Opt *opt) {
    if (wlx_color_is_zero(opt->tint)) opt->tint = WLX_WHITE;
    WLX_RESOLVE_VISUAL_STATE(ctx, opt, false, &opt->tint);
}

WLXDEF void wlx_image_impl(WLX_Context *ctx, WLX_Texture texture, WLX_Image_Opt opt, const char *file, int line) {
    assert(ctx != NULL);

    wlx_resolve_opt_image(ctx, &opt);

    WLX_Widget_Layout wly = WLX_WIDGET_LAYOUT(opt);
    if (wly.width <= 0 && wlx_parent_wants_intrinsic_width(ctx, wly.pos)) {
        wly.intrinsic_w = (opt.src.w > 0.0f) ? opt.src.w : (float)texture.width;
    }

    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, wly, file, line);
    WLX_Rect wr = frame.rect;

    // Empty / unloaded texture: assert under WLX_DEBUG, no-op the draw in
    // release. The frame still closes so layout indices and IDs stay
    // consistent for subsequent widgets. Tests that intentionally exercise
    // the no-op path may pre-define WLX_IMAGE_ASSERT_TEXTURE_VALID as a noop.
    #ifndef WLX_IMAGE_ASSERT_TEXTURE_VALID
    #define WLX_IMAGE_ASSERT_TEXTURE_VALID(tex) \
        assert((tex).width > 0 && (tex).height > 0 \
               && "wlx_image: texture appears unloaded (width/height <= 0)")
    #endif
    WLX_IMAGE_ASSERT_TEXTURE_VALID(texture);
    if (texture.width <= 0 || texture.height <= 0) {
        wlx_widget_frame_end(ctx, frame);
        return;
    }

    WLX_Rect s = opt.src;
    if (s.w <= 0 || s.h <= 0) {
        s = (WLX_Rect){ 0, 0, (float)texture.width, (float)texture.height };
    }

    WLX_Rect dst;
    wlx_resolve_image_fit(wr, s, opt.scale, opt.content_align, &s, &dst);

    assert(s.x >= 0.0f && s.y >= 0.0f
           && s.x + s.w <= (float)texture.width  + 0.5f
           && s.y + s.h <= (float)texture.height + 0.5f
           && "wlx_image: FILL crop produced out-of-bounds src rect");

    wlx_draw_texture(ctx, texture, s, dst, opt.tint);

    wlx_widget_frame_end(ctx, frame);
}

static void wlx_resolve_opt_toggle(const WLX_Context *ctx, WLX_Toggle_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    opt->track_color        = wlx_color_or(opt->track_color,        wlx_color_or(theme->toggle.track, theme->slider.track));
    opt->track_active_color = wlx_color_or(opt->track_active_color, wlx_color_or(theme->toggle.track_active, theme->accent));
    opt->thumb_color        = wlx_color_or(opt->thumb_color,        wlx_color_or(theme->toggle.thumb, theme->foreground));
    if (wlx_color_is_zero(opt->front_color))       opt->front_color      = theme->foreground;
    if (wlx_is_float_unset(opt->hover_brightness))  opt->hover_brightness = theme->hover_brightness;
    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);
    WLX_RESOLVE_VISUAL_STATE(ctx, opt, opt->disabled,
        &opt->track_color, &opt->track_active_color, &opt->thumb_color,
        &opt->front_color, &opt->border_color,
        &opt->shadow_color, &opt->glow_color);
}

WLXDEF bool wlx_toggle_impl(WLX_Context *ctx, const char *label, bool *value, WLX_Toggle_Opt opt, const char *file, int line) {
    assert(ctx != NULL);
    assert(value != NULL && "toggle value pointer must not be NULL");

    wlx_resolve_opt_toggle(ctx, &opt);

    size_t label_len = (label != NULL) ? strlen(label) : 0;
    WLX_Widget_Layout wly = WLX_WIDGET_LAYOUT(opt);
    if (wly.width <= 0 && wlx_parent_wants_intrinsic_width(ctx, wly.pos)) {
        float thr_i = ctx->theme->toggle.track_to_height_ratio > 0.0f
                    ? ctx->theme->toggle.track_to_height_ratio
                    : WLX_TOGGLE_TRACK_RATIO_FALLBACK;
        WLX_Text_Style mts = { .font = opt.font, .font_size = opt.font_size,
                               .spacing = opt.spacing };
        wly.intrinsic_w = wlx_intrinsic_glyph_row_width(ctx,
                (float)opt.font_size * thr_i, label, label_len, mts, false)
            + WLX_INTRINSIC_PAD_LR(ctx, opt);
    }

    // Prologue: determine geometry of toggle components (track, thumb, label) and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, wly, file, line);
    WLX_Rect wr = frame.rect;

    WLX_Rect content_rect = WLX_RESOLVE_CONTENT_RECT(ctx, opt, wr);

    float track_h = opt.font_size;
    float thr = ctx->theme->toggle.track_to_height_ratio > 0.0f
                    ? ctx->theme->toggle.track_to_height_ratio
                    : WLX_TOGGLE_TRACK_RATIO_FALLBACK;
    float track_w = track_h * thr;

    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size,
                           .color = opt.front_color, .spacing = opt.spacing };
    WLX_Glyph_Row row = wlx_layout_glyph_row(ctx, content_rect, track_w, track_h,
        label, label_len, ts, opt.content_align, false, true);
    WLX_Rect track_rect = row.glyph;

    int segs = opt.rounded_segments;
    if (segs < ctx->theme->min_rounded_segments) segs = ctx->theme->min_rounded_segments;

    WLX_Interaction inter = wlx_get_interaction_for(
        ctx, wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        opt.disabled,
        file, line
    );

    if (inter.clicked) {
        *value = !(*value);
    }

    bool on = *value;

    WLX_Color color_track = on ? opt.track_active_color : opt.track_color;
    color_track = wlx_color_hover_tint(
        color_track, inter.hover, inter.disabled, opt.hover_brightness);
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
        ? track_rect.x + track_w - thumb_inset - thumb_size
        : track_rect.x + thumb_inset;
    float thumb_y = track_rect.y + thumb_inset;
    WLX_Rect thumb_rect = { thumb_x, thumb_y, thumb_size, thumb_size };

    WLX_Color color_thumb = opt.thumb_color;
    color_thumb = wlx_color_hover_tint(
        color_thumb, inter.hover, inter.disabled, opt.hover_brightness * 0.5f);
    // Soft effect rides behind the thumb: emit before the thumb fill so the
    // glow/shadow sits under it, matching the thumb curvature. Gated on a
    // non-zero effect color.
    if (!wlx_color_is_zero(opt.glow_color) || !wlx_color_is_zero(opt.shadow_color)) {
        wlx_draw_box(ctx, thumb_rect, (WLX_Box_Style){
            .roundness        = 1.0f,
            .rounded_segments = segs,
            WLX_BOX_STYLE_EFFECTS(opt),
        });
    }
    wlx_draw_rect_rounded(ctx, thumb_rect, 1.0f, segs, color_thumb);

    if (label != NULL && row.label_w > 0) {
        wlx_draw_text_fitted_slice(ctx, row.text, label, label_len, ts,
            (WLX_Text_Prepare_Opt){ .align = WLX_ALIGN_NONE, .wrap = opt.wrap });
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
    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->min_height);
    WLX_RESOLVE_VISUAL_STATE(ctx, opt, opt->disabled,
        &opt->ring_color, &opt->fill_color, &opt->front_color);
}

WLXDEF bool wlx_radio_impl(WLX_Context *ctx, const char *label, int *active, int index, WLX_Radio_Opt opt, const char *file, int line) {
    assert(ctx != NULL);
    assert(active != NULL && "radio active pointer must not be NULL");

    wlx_resolve_opt_radio(ctx, &opt);

    size_t label_len = (label != NULL) ? strlen(label) : 0;
    WLX_Widget_Layout wly = WLX_WIDGET_LAYOUT(opt);
    if (wly.width <= 0 && wlx_parent_wants_intrinsic_width(ctx, wly.pos)) {
        WLX_Text_Style mts = { .font = opt.font, .font_size = opt.font_size,
                               .spacing = opt.spacing };
        wly.intrinsic_w = wlx_intrinsic_glyph_row_width(ctx,
                (float)opt.font_size, label, label_len, mts, false)
            + WLX_INTRINSIC_PAD_LR(ctx, opt);
    }

    // Prologue: determine geometry of radio components (ring, fill, label) and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, wly, file, line);
    WLX_Rect wr = frame.rect;

    WLX_Rect content_rect = WLX_RESOLVE_CONTENT_RECT(ctx, opt, wr);

    bool selected = (*active == index);

    float circle_size = (float)opt.font_size;

    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size,
                           .color = opt.front_color, .spacing = opt.spacing };
    WLX_Glyph_Row row = wlx_layout_glyph_row(ctx, content_rect, circle_size, circle_size,
        label, label_len, ts, opt.content_align, false, true);

    WLX_Interaction inter = wlx_get_interaction_for(
        ctx, wr,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        opt.disabled,
        file, line
    );

    if (inter.clicked) {
        *active = index;
    }

    WLX_Rect ring_rect = row.glyph;

    int segs = ctx->theme->rounded_segments;
    if (segs < ctx->theme->min_rounded_segments) segs = ctx->theme->min_rounded_segments;

    WLX_Color color_ring = opt.ring_color;
    color_ring = wlx_color_hover_tint(
        color_ring, inter.hover, inter.disabled, opt.hover_brightness);

    if (opt.ring_border_width > 0) {
        wlx_draw_rect_rounded_lines(ctx, ring_rect, 1.0f,
            segs, opt.ring_border_width, color_ring);
    }

    if (selected) {
        float sir = ctx->theme->radio.selected_inset_ratio > 0.0f
                        ? ctx->theme->radio.selected_inset_ratio : 0.25f;
        float inset = circle_size * sir;
        WLX_Rect fill_rect = {
            ring_rect.x + inset, ring_rect.y + inset,
            circle_size - inset * 2.0f, circle_size - inset * 2.0f
        };
        wlx_draw_rect_rounded(ctx, fill_rect, 1.0f,
            segs, opt.fill_color);
    }

    if (label != NULL && row.label_w > 0) {
        wlx_draw_text_fitted_slice(ctx, row.text, label, label_len, ts,
            (WLX_Text_Prepare_Opt){ .align = WLX_ALIGN_NONE, .wrap = opt.wrap });
    }

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);

    return inter.clicked;
}

// ============================================================================
// Implementation: scroll panels
// ============================================================================

static void wlx_resolve_opt_scroll_panel(const WLX_Context *ctx, WLX_Scroll_Panel_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    // A transparent panel keeps a zero back_color so wlx_draw_box draws no fill and
    // the container surface shows through; otherwise a zero back_color defaults to
    // the theme background.
    if (!opt->transparent_background && wlx_color_is_zero(opt->back_color))
                                                  opt->back_color                 = theme->background;
    if (wlx_color_is_zero(opt->scrollbar_color))  opt->scrollbar_color            = theme->scrollbar.bar;
    if (wlx_color_is_zero(opt->border_color))     opt->border_color               = theme->border;
    if (wlx_is_negative_unset(opt->border_width))            opt->border_width               = theme->border_width;
    if (wlx_is_float_unset(opt->scrollbar_hover_brightness)) opt->scrollbar_hover_brightness = theme->hover_brightness;
    if (wlx_is_negative_unset(opt->scrollbar_width))       opt->scrollbar_width            = theme->scrollbar.width;
    WLX_RESOLVE_VISUAL_STATE(ctx, opt, false,
        &opt->back_color, &opt->scrollbar_color, &opt->border_color,
        &opt->shadow_color, &opt->glow_color);
    opt->roundness        = 0.0f;
    opt->rounded_segments = 0;
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
        opt->slot_align, opt->overflow);
}

// Helper: contribute the scroll panel's viewport height - and its explicit
// width, when one was given - to the parent layout's content tracking. A
// panel without .width contributes no width (the viewport rect would be
// circular for a CONTENT-sized slot).
static inline void wlx_scroll_panel_contribute_to_parent(WLX_Context *ctx, float vp_contrib,
                                                         float w_contrib) {
    if (ctx->arena.layouts.count == 0) return;
    WLX_Layout *parent_l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
    // Per-slot bucket needs parent_l->index > 0 (index was already advanced
    // by wlx_get_slot_rect); guard by clamping the index when not eligible.
    size_t slot_index = (parent_l->index > 0) ? (parent_l->index - 1) : WLX_SLOT_SKIP;
    wlx_contribute_to_parent_layout(ctx, parent_l, (WLX_Parent_Contribution){
        .h_contrib  = vp_contrib,
        .w_contrib  = w_contrib,
        .slot_index = slot_index,
        .grid_row   = parent_l->grid.last_placed_row,
    });
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
    // Re-arm the enclosing draw clip of this layer (an explicit scissor
    // scope, else a parent panel, a .clip layout, or a popup's own rect);
    // never a base-layer clip from inside an overlay.
    WLX_Rect enclosing;
    if (wlx_active_scissor_rect(ctx, &enclosing)) {
        wlx_begin_scissor(ctx, enclosing);
    }
}

// Helper: set up scissor clipping, draw the scrollbar bar, and push the content layout.
static inline void wlx_scroll_panel_begin_content_layout(
    WLX_Context *ctx, WLX_Scroll_Panel_State *state, const WLX_Scroll_Panel_Opt *opt,
    WLX_Rect wr, float content_height, bool sb_visible, WLX_Rect sb_rect)
{
    // Intersect the scissor rect with the enclosing draw clip of this layer
    // (an explicit scissor scope, else parent panels, .clip layouts, or a
    // popup's own rect) to contain nested content.
    WLX_Rect scissor = wr;
    WLX_Rect enclosing;
    if (wlx_active_scissor_rect(ctx, &enclosing)) {
        scissor = wlx_rect_intersect(scissor, enclosing);
    }
    wlx_begin_scissor(ctx, scissor);

    // Draw scrollbar bar (interaction was already handled before this call).
    if (sb_visible) {
        bool sb_hover = wlx_interaction_mouse_over(ctx, sb_rect);
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
    bool pushed = wlx_scope_push(ctx, opt->id);

    wlx_resolve_opt_scroll_panel(ctx, opt);

    // NOTE: scroll_panel does NOT use wlx_widget_begin() because its scroll-height
    // tracking is conditional (only for non-auto-height panels) and deferred.
    WLX_Rect r, wr;
    wlx_scroll_panel_resolve_rect(ctx, opt, &r, &wr);

    WLX_State persistent = wlx_get_state_impl(ctx, sizeof(WLX_Scroll_Panel_State), file, line);
    WLX_Scroll_Panel_State *state = (WLX_Scroll_Panel_State *)persistent.data;

    // Initialize content_height on first frame (when calloc'd to 0).
    if (state->content_height == 0.0f) {
        state->content_height = (content_height < 0) ? wr.h : content_height;
    }

    state->auto_height = (content_height < 0);
    state->panel_rect = wr;
    state->wheel_scroll_speed = opt->wheel_scroll_speed;

    // Contribute this panel's viewport height - and its explicit width,
    // if any - to the parent layout's content tracking.
    float vp_contrib = (opt->height > 0) ? (float)opt->height : r.h;
    wlx_scroll_panel_contribute_to_parent(ctx, vp_contrib,
        (opt->width > 0) ? (float)opt->width : 0.0f);

    // Save outer auto-scroll tracking context before potentially changing it.
    state->saved_auto_scroll_panel_id = ctx->auto_scroll.panel_id;
    state->saved_auto_scroll_total_height = ctx->auto_scroll.total_height;

    // For auto height: keep previous frame's measured content_height (stored in state).
    // For explicit height: update directly.
    if (!state->auto_height) {
        state->content_height = content_height;
    }

    if (state->auto_height) {
        ctx->auto_scroll.panel_id = persistent.id;
        ctx->auto_scroll.total_height = 0;
    } else {
        // Clear so inner widgets don't pollute outer panel's auto-height measurement.
        ctx->auto_scroll.panel_id = 0;
        ctx->auto_scroll.total_height = 0;
    }

    float ch = state->content_height;
    float max_scroll = ch - wr.h;
    if (max_scroll < 0) max_scroll = 0;

    // Panel-level hover for the wheel (consumed in wlx_scroll_panel_end): the
    // viewport clipped to the enclosing containers, so a panel cropped by a
    // .clip layout or scrolled out of an outer panel does not take the wheel
    // through its invisible part. Computed before this panel is pushed, so
    // its own viewport is not in the walk.
    state->hovered = wlx_rect_contains(wlx_interaction_clip_rect(ctx, wr),
        (float)ctx->input.mouse_x, (float)ctx->input.mouse_y);

    // Record whether an id was pushed so wlx_scroll_panel_end can pop it.
    state->pushed_scope = pushed;

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
    WLX_Border_Sides panel_sides = wlx_border_sides_for_widget(
        ctx->theme, false, false, opt.opacity,
        opt.border_color, opt.border_width,
        WLX_BORDER_SIDES_ARGS(opt));
    wlx_draw_box(ctx, frame.rect, (WLX_Box_Style){
        .fill             = opt.back_color,
        .border           = opt.border_color,
        .border_width     = opt.border_width,
        .roundness        = opt.roundness,
        .rounded_segments = opt.rounded_segments,
        .sides            = panel_sides,
        .per_side         = true,
        WLX_BOX_STYLE_EFFECTS(opt),
        WLX_BOX_STYLE_CORNER(opt),
    });

    // Scissor, scrollbar bar draw, content layout.
    // The scope id (if any) remains pushed until wlx_scroll_panel_end so descendants
    // can see the panel's id scope.
    wlx_scroll_panel_begin_content_layout(ctx, frame.state, &opt, frame.rect, frame.content_height, sb_visible, sb_rect);
}

// Scope-pop half of the scroll panel frame lifecycle: pops the scope id if one
// was pushed at begin.
static inline void wlx_scroll_panel_frame_end(WLX_Context *ctx, WLX_Scroll_Panel_State *state) {
    wlx_scope_pop(ctx, state->pushed_scope);
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
    if (state_sp->hovered) {
        float max_scroll = state_sp->content_height - state_sp->panel_rect.h;
        float scroll_speed = state_sp->wheel_scroll_speed;
        if (scroll_speed <= 0.0f) scroll_speed = WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED;
        wlx_wheel_consume(ctx, &state_sp->scroll_offset, max_scroll, scroll_speed);
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
// Popup shared helpers (dropdown, tooltip, menu)
// ============================================================================

// Default popup row height: the font size plus this much vertical room.
static const float WLX_POPUP_ROW_HEIGHT_PAD = 12.0f;
// Point-anchored menu width when none is given.
static const float WLX_MENU_DEFAULT_WIDTH = 180.0f;
// Left/right text inset of a menu item row.
static const float WLX_MENU_ITEM_PADDING = 8.0f;

// Overlay chrome for a popup panel (list background and border); `clip`
// per the popup's own rule (lists clip, menus leave it off because their
// chrome lags the item list by one frame).
static inline WLX_Overlay_Opt wlx_overlay_chrome_opt(WLX_Color back, WLX_Color border,
        float border_width, float roundness, int rounded_segments, bool clip) {
    WLX_Overlay_Opt lopt = wlx_default_overlay_opt();
    lopt.back_color       = back;
    lopt.border_color     = border;
    lopt.border_width     = border_width;
    lopt.roundness        = roundness;
    lopt.rounded_segments = rounded_segments;
    lopt.clip             = clip;
    return lopt;
}

// The list styling of a button-anchored popup (dropdown, menu button) as a
// resolved WLX_Menu_Opt, so its rows and chrome go through the same helpers
// a menu uses. `opt` carries the popup's resolved face and list_* fields.
#define WLX_POPUP_LIST_OPT(opt, width_, item_padding_) \
    ((WLX_Menu_Opt){ \
        .width = (width_), .row_height = (opt).row_height, \
        .item_padding = (item_padding_), \
        WLX_TEXT_TYPOGRAPHY_COPY(opt), \
        .front_color = (opt).front_color, .back_color = (opt).list_back_color, \
        .border_color = (opt).list_border_color, .border_width = (opt).list_border_width, \
        .roundness = (opt).roundness, .rounded_segments = (opt).rounded_segments, \
        .hover_brightness = (opt).hover_brightness, \
        .hover_back_color = (opt).hover_back_color, \
    })

// A popup row: a flat button on the list background, styled from the list's
// resolved WLX_Menu_Opt. Callers add their own text inset (the dropdown its
// face content padding, the menu item its item_padding); front_override
// ({0} = none) recolors one row.
static inline WLX_Button_Opt wlx_popup_row_opt(const WLX_Menu_Opt *style,
        bool disabled, WLX_Color front_override) {
    return wlx_default_button_opt(
        .height           = style->row_height,
        WLX_TEXT_TYPOGRAPHY_COPY(*style),
        .wrap             = false,
        .disabled         = disabled,
        .front_color      = wlx_color_is_zero(front_override) ? style->front_color : front_override,
        .back_color       = style->back_color,
        .border_width     = 0,
        .roundness        = 0,
        .hover_brightness = style->hover_brightness,
        .hover_back_color = style->hover_back_color,
    );
}

// True when this frame's press landed outside a popup: arbitration is live,
// a press happened, and no query inside the popup's subtree claimed it
// (claim_before is the press_claimed snapshot taken before that subtree).
// Bootstrap frames cannot arbitrate ownership, so they never report one.
static inline bool wlx_popup_outside_press(const WLX_Context *ctx, bool claim_before) {
    bool press_inside = ctx->interaction.press_claimed && !claim_before;
    return ctx->interaction.arbitrate && ctx->input.mouse_clicked && !press_inside;
}

// ============================================================================
// Dropdown implementation
// ============================================================================

// Id-stack salt separating the list subtree (overlay, scroll panel, rows)
// from the face and its persistent state at the same call site.
enum { WLX_DROPDOWN_LIST_SALT = 0x64726F70 };

static void wlx_resolve_opt_dropdown(const WLX_Context *ctx, WLX_Dropdown_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;

    if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->surface;

    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);

    if (wlx_is_float_unset(opt->hover_brightness))
        opt->hover_brightness = theme->hover_brightness;

    if (opt->row_height <= 0.0f) opt->row_height = (float)opt->font_size + WLX_POPUP_ROW_HEIGHT_PAD;
    if (opt->max_list_height <= 0.0f) opt->max_list_height = WLX_DROPDOWN_MAX_LIST_HEIGHT;
    if (wlx_color_is_zero(opt->list_back_color))   opt->list_back_color = theme->background;
    if (wlx_color_is_zero(opt->list_border_color)) opt->list_border_color = opt->border_color;
    if (opt->list_border_width < 0.0f) opt->list_border_width = opt->border_width;

    WLX_RESOLVE_VISUAL_STATE(ctx, opt, opt->disabled,
        &opt->front_color, &opt->back_color, &opt->border_color,
        &opt->shadow_color, &opt->glow_color);
}

WLXDEF bool wlx_dropdown_impl(WLX_Context *ctx, const char *label,
    int *selected, const char **options, size_t count,
    WLX_Dropdown_Opt opt, const char *file, int line)
{
    assert(selected != NULL && "wlx_dropdown: selected must not be NULL");
    assert((count == 0 || options != NULL) && "wlx_dropdown: options must not be NULL");
    wlx_resolve_opt_dropdown(ctx, &opt);

    bool scope_pushed = wlx_scope_push(ctx, opt.id);
    WLX_State persistent = wlx_get_state_impl(ctx, sizeof(WLX_Dropdown_State), file, line);
    WLX_Dropdown_State *state = (WLX_Dropdown_State *)persistent.data;
    if (opt.disabled) state->open = false;

    // Presses claimed between here and the end of the call belong to this
    // dropdown when deciding whether a press landed outside it.
    bool claim_before = ctx->interaction.press_claimed;

    const char *face_text = (*selected >= 0 && (size_t)*selected < count)
        ? options[*selected] : label;

    WLX_Widget_Layout wly = WLX_WIDGET_LAYOUT(opt);
    if (wly.width <= 0 && wlx_parent_wants_intrinsic_width(ctx, wly.pos)) {
        // The widest option (or the label) decides the intrinsic face
        // width, so the face does not resize when the selection changes.
        WLX_Text_Style mts = { .font = opt.font, .font_size = opt.font_size,
                               .spacing = opt.spacing };
        float w = wlx_intrinsic_text_width(ctx, label, mts);
        for (size_t i = 0; i < count; i++) {
            float ow = wlx_intrinsic_text_width(ctx, options[i], mts);
            if (ow > w) w = ow;
        }
        wly.intrinsic_w = w + WLX_INTRINSIC_PAD_LR(ctx, opt);
    }

    // The face is a button drawn from the resolved dropdown options
    // (text-only: the image fields stay zero).
    WLX_Button_Opt face = {
        WLX_LAYOUT_SLOT_COPY(opt), WLX_WIDGET_SIZING_COPY(opt),
        WLX_WIDGET_STATE_COPY(opt), WLX_TEXT_TYPOGRAPHY_COPY(opt),
        WLX_TEXT_COLOR_COPY(opt), WLX_BORDER_COPY(opt),
        WLX_CONTENT_PADDING_COPY(opt),
        .hover_brightness = opt.hover_brightness,
        .hover_back_color = opt.hover_back_color,
        .wrap             = false,
    };
    size_t face_len = (face_text != NULL) ? strlen(face_text) : 0;
    WLX_Rect wr;
    WLX_Interaction inter = wlx_button_face(ctx, face_text, face_len, &face, wly,
                                            NULL, &wr, file, line);

    if (inter.clicked) state->open = !state->open;
    if (state->open && wlx_is_key_pressed(ctx, WLX_KEY_ESCAPE)) state->open = false;

    bool changed = false;
    if (state->open && count > 0) {
        float list_h = opt.row_height * (float)count;
        if (list_h > opt.max_list_height) list_h = opt.max_list_height;
        WLX_Rect list_rect = { wr.x, wr.y + wr.h, wr.w, list_h };

        WLX_Menu_Opt list = WLX_POPUP_LIST_OPT(opt, wr.w, 0.0f);
        WLX_Overlay_Opt lopt = wlx_overlay_chrome_opt(list.back_color, list.border_color,
            list.border_width, list.roundness, list.rounded_segments, true);

        // Rows inherit the face's content padding (all five fields).
        WLX_Button_Opt ropt = wlx_popup_row_opt(&list, false, (WLX_Color){0});
        ropt.content_padding        = opt.content_padding;
        ropt.content_padding_top    = opt.content_padding_top;
        ropt.content_padding_right  = opt.content_padding_right;
        ropt.content_padding_bottom = opt.content_padding_bottom;
        ropt.content_padding_left   = opt.content_padding_left;

        WLX_Scroll_Panel_Opt popt = wlx_default_scroll_panel_opt();
        popt.transparent_background = true;
        popt.border_width      = 0;
        popt.wheel_scroll_speed = opt.row_height;   // one notch, one row

        wlx_push_id(ctx, WLX_DROPDOWN_LIST_SALT);
        wlx_overlay_begin_impl(ctx, 1, list_rect, lopt, file, line);
        wlx_scroll_panel_begin_impl(ctx, opt.row_height * (float)count, popt, file, line);
        // The scroll panel exposes a single content slot; the rows stack in
        // their own layout filling it (count slots over count * row_height).
        wlx_layout_begin_impl(ctx, count, WLX_VERT, wlx_default_layout_opt(), file, line);
        for (size_t i = 0; i < count; i++) {
            wlx_push_id(ctx, i + 1);
            bool row_clicked = wlx_button_impl(ctx, options[i], ropt, file, line);
            wlx_pop_id(ctx);
            if (row_clicked) {
                *selected = (int)i;
                changed = true;
                state->open = false;
            }
        }
        wlx_layout_end(ctx);
        wlx_scroll_panel_end(ctx);
        wlx_overlay_end(ctx);
        wlx_pop_id(ctx);

        // A press this frame that no query inside this dropdown claimed
        // happened outside it: close.
        if (wlx_popup_outside_press(ctx, claim_before)) state->open = false;
    }

    wlx_scope_pop(ctx, scope_pushed);
    // The dropdown is one widget: its rows are internal, so the last-rect
    // query reports the face, not the final row of an open list.
    ctx->last_widget_rect = wr;
    return changed;
}

// ============================================================================
// Tooltip implementation
// ============================================================================

static const float WLX_TOOLTIP_DEFAULT_DELAY   = 0.5f;  // seconds hovered before the tip shows
static const float WLX_TOOLTIP_FLIP_GAP_X      = 4.0f;  // pointer-to-tip gap when flipped to the left
static const float WLX_TOOLTIP_FLIP_GAP_Y      = 6.0f;  // ... and when flipped above

static void wlx_resolve_opt_tooltip(const WLX_Context *ctx, WLX_Tooltip_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    float min_height = 0.0f;

    if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->background;

    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);

    if (opt->delay < 0.0f)   opt->delay = WLX_TOOLTIP_DEFAULT_DELAY;
}

WLXDEF bool wlx_tooltip_for_impl(WLX_Context *ctx, WLX_Rect anchor,
    const char *text, WLX_Tooltip_Opt opt, const char *file, int line)
{
    wlx_resolve_opt_tooltip(ctx, &opt);

    bool scope_pushed = wlx_scope_push(ctx, opt.id);
    WLX_State persistent = wlx_get_state_impl(ctx, sizeof(WLX_Tooltip_State), file, line);
    WLX_Tooltip_State *state = (WLX_Tooltip_State *)persistent.data;

    // The timer runs while the pointer rests on the anchor with the button
    // up, and only when the pointer actually belongs to the anchor's layer
    // (an overlay covering the anchor suppresses its tooltip). The anchor
    // test is the widget query's own container-clipped containment, so an
    // anchor scrolled or clipped out of view cannot light a tip from under the
    // chrome that covers it.
    bool over = wlx_interaction_mouse_over(ctx, anchor)
        && !ctx->input.mouse_down
        && wlx_pointer_on_current_layer(ctx);
    if (!over) {
        state->hover_time = 0.0f;
        wlx_scope_pop(ctx, scope_pushed);
        return false;
    }

    state->hover_time += wlx_get_frame_time(ctx);
    if (state->hover_time < opt.delay) {
        wlx_scope_pop(ctx, scope_pushed);
        return false;
    }

    WLX_Text_Style ts = {
        .font      = opt.font,
        .font_size = opt.font_size,
        .color     = opt.front_color,
        .spacing   = opt.spacing,
    };
    size_t text_len = (text != NULL) ? strlen(text) : 0;
    float text_w = 0.0f, text_h = 0.0f;
    wlx_measure_text_slice(ctx, text, text_len, ts, &text_w, &text_h);
    float line_h = wlx_text_line_height(ctx, ts, NULL);

    WLX_Resolved_Padding rp = WLX_RESOLVE_CONTENT_PADDING_EX(ctx, opt, WLX_TOOLTIP_CONTENT_PADDING);
    WLX_Rect tip = {
        (float)ctx->input.mouse_x + opt.offset_x,
        (float)ctx->input.mouse_y + opt.offset_y,
        text_w + rp.left + rp.right,
        line_h + rp.top + rp.bottom,
    };
    // Keep the tip on screen: flip to the other side of the pointer when it
    // would run past the right or bottom edge.
    if (tip.x + tip.w > ctx->rect.x + ctx->rect.w)
        tip.x = (float)ctx->input.mouse_x - tip.w - WLX_TOOLTIP_FLIP_GAP_X;
    if (tip.y + tip.h > ctx->rect.y + ctx->rect.h)
        tip.y = (float)ctx->input.mouse_y - tip.h - WLX_TOOLTIP_FLIP_GAP_Y;
    if (tip.x < ctx->rect.x) tip.x = ctx->rect.x;
    if (tip.y < ctx->rect.y) tip.y = ctx->rect.y;

    WLX_Overlay_Opt lopt = wlx_overlay_chrome_opt(opt.back_color, opt.border_color,
        opt.border_width, opt.roundness, opt.rounded_segments, true);

    // Draw-only by construction: nothing between begin and end queries
    // interaction, so the tip never appends candidates and can never own
    // hover or a press.
    wlx_overlay_begin_impl(ctx, 1, tip, lopt, file, line);
    WLX_Rect trect = { tip.x + rp.left, tip.y + rp.top,
                       tip.w - rp.left - rp.right, tip.h - rp.top - rp.bottom };
    wlx_draw_widget_content(ctx, trect, text, text_len, ts, (WLX_Widget_Content){
        .font_size = opt.font_size,
        .align     = WLX_LEFT,
        .wrap      = false,
        .vmetric   = WLX_VMETRIC_LINE_HEIGHT,
    });
    wlx_overlay_end(ctx);

    wlx_scope_pop(ctx, scope_pushed);
    return true;
}

// ============================================================================
// Menu implementation
// ============================================================================

// One open wlx_menu_begin/end pair. Item calls read row geometry and styling
// from the innermost entry; the submenu inherits from its parent entry.
struct WLX_Menu_Frame {
    bool           *open;
    WLX_Menu_State *state;
    WLX_Rect        rect;          // panel rect: x, y from the caller; w, h from style + item count
    WLX_Menu_Opt    style;         // resolved list styling (rows, chrome, submenu inheritance)
    int             row_cursor;    // items emitted so far this frame
    bool            claim_before;  // press-claim snapshot taken before the menu subtree
    bool            item_clicked;  // a leaf in this chain was activated this frame
    bool            first_frame;   // body built this frame after being closed
    bool            scope_pushed;
    bool            restore_last_rect;  // wlx_menu_end re-publishes anchor_rect as the last widget rect
    WLX_Rect        anchor_rect;        // the opener's face (menu button); a menu is one widget
};

// Shared closed-menu exit for the three begins: record the closed state,
// release the scope, report "no body this frame".
static inline bool wlx_menu_closed(WLX_Context *ctx, WLX_Menu_State *state, bool scope_pushed) {
    state->was_open = false;
    wlx_scope_pop(ctx, scope_pushed);
    return false;
}

static void wlx_resolve_opt_menu(const WLX_Context *ctx, WLX_Menu_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    float min_height = 0.0f;

    if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->background;

    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);

    if (wlx_is_float_unset(opt->hover_brightness))
        opt->hover_brightness = theme->hover_brightness;

    if (opt->width < 0.0f)         opt->width = WLX_MENU_DEFAULT_WIDTH;
    if (opt->row_height <= 0.0f)   opt->row_height = (float)opt->font_size + WLX_POPUP_ROW_HEIGHT_PAD;
    if (opt->item_padding < 0.0f)  opt->item_padding = WLX_MENU_ITEM_PADDING;
}

// Shared open-body tail for wlx_menu_begin, wlx_menu_button_begin and
// wlx_submenu_begin: panel overlay + auto item layout + menu-stack entry.
// (x, y) anchor the panel; its size comes from `style` (resolved list
// styling) and last frame's item count, so the chrome adapts one frame
// after the item list changes. claim_before is the caller's press-claim
// snapshot (taken before the opener face for the button variant, so a
// press on the face counts as inside the menu). Returns false - with the
// scope released and the state marked closed - only when the stack cannot
// be allocated; the caller then reports a closed menu for this frame.
static bool wlx_menu_frame_push(WLX_Context *ctx, bool *open,
    WLX_Menu_State *state, float x, float y, const WLX_Menu_Opt *style,
    bool claim_before, bool scope_pushed, const char *file, int line)
{
    // Overflow would write past the stack (memory corruption in release
    // builds), so this guard survives NDEBUG.
    WLX_HARD_ASSERT(ctx->menu_stack_count < WLX_MENU_STACK_MAX,
        "wlx_menu_begin: menu nesting exceeds WLX_MENU_STACK_MAX");
    if (ctx->menu_stack == NULL) {
        ctx->menu_stack = (WLX_Menu_Frame *)wlx_calloc(WLX_MENU_STACK_MAX, sizeof(WLX_Menu_Frame));
        if (ctx->menu_stack == NULL) return wlx_menu_closed(ctx, state, scope_pushed);
    }

    int rows = state->item_count > 0 ? state->item_count : 1;
    WLX_Rect rect = { x, y, style->width, style->row_height * (float)rows };

    WLX_Overlay_Opt lopt = wlx_overlay_chrome_opt(style->back_color, style->border_color,
        style->border_width, style->roundness, style->rounded_segments, false);
    wlx_overlay_begin_impl(ctx, 1, rect, lopt, file, line);
    wlx_layout_begin_auto_impl(ctx, WLX_VERT, style->row_height, wlx_default_layout_opt());

    WLX_Menu_Frame *mf = &ctx->menu_stack[ctx->menu_stack_count++];
    mf->open              = open;
    mf->state             = state;
    mf->rect              = rect;
    mf->style             = *style;
    mf->row_cursor        = 0;
    mf->claim_before      = claim_before;
    mf->item_clicked      = false;
    mf->first_frame       = !state->was_open;
    mf->scope_pushed      = scope_pushed;
    mf->restore_last_rect = false;
    mf->anchor_rect       = (WLX_Rect){0};
    state->was_open = true;
    return true;
}

WLXDEF bool wlx_menu_begin_impl(WLX_Context *ctx, bool *open, float x, float y,
    WLX_Menu_Opt opt, const char *file, int line)
{
    assert(open != NULL && "wlx_menu_begin: open must not be NULL");
    wlx_resolve_opt_menu(ctx, &opt);

    bool scope_pushed = wlx_scope_push(ctx, opt.id);
    WLX_State persistent = wlx_get_state_impl(ctx, sizeof(WLX_Menu_State), file, line);
    WLX_Menu_State *state = (WLX_Menu_State *)persistent.data;

    if (!*open) return wlx_menu_closed(ctx, state, scope_pushed);

    // The chrome takes last frame's item count; items themselves lay out in
    // an auto-growing layout, so a changed item list overflows or underfills
    // the panel for one frame only. clip stays off for the same reason.
    return wlx_menu_frame_push(ctx, open, state, x, y, &opt,
        ctx->interaction.press_claimed, scope_pushed, file, line);
}

static void wlx_resolve_opt_menu_button(const WLX_Context *ctx, WLX_Menu_Button_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;
    float min_height = 0.0f;

    if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->surface;

    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);

    if (wlx_is_float_unset(opt->hover_brightness))
        opt->hover_brightness = theme->hover_brightness;

    if (opt->row_height <= 0.0f)  opt->row_height = (float)opt->font_size + WLX_POPUP_ROW_HEIGHT_PAD;
    if (opt->item_padding < 0.0f) opt->item_padding = WLX_MENU_ITEM_PADDING;
    if (wlx_color_is_zero(opt->list_back_color))   opt->list_back_color = theme->background;
    if (wlx_color_is_zero(opt->list_border_color)) opt->list_border_color = opt->border_color;
    if (opt->list_border_width < 0.0f) opt->list_border_width = opt->border_width;

    WLX_RESOLVE_VISUAL_STATE(ctx, opt, opt->disabled,
        &opt->front_color, &opt->back_color, &opt->border_color,
        &opt->shadow_color, &opt->glow_color);
}

WLXDEF bool wlx_menu_button_begin_impl(WLX_Context *ctx, const char *label,
    bool *open, WLX_Menu_Button_Opt opt, const char *file, int line)
{
    assert(open != NULL && "wlx_menu_button_begin: open must not be NULL");
    wlx_resolve_opt_menu_button(ctx, &opt);

    bool scope_pushed = wlx_scope_push(ctx, opt.id);
    WLX_State persistent = wlx_get_state_impl(ctx, sizeof(WLX_Menu_State), file, line);
    WLX_Menu_State *state = (WLX_Menu_State *)persistent.data;
    if (opt.disabled) *open = false;

    // The face belongs to the menu: snapshotting the press claim before
    // its query makes a press on the face an inside press, so the release
    // toggle closes cleanly instead of fighting an outside-close.
    bool claim_before = ctx->interaction.press_claimed;

    size_t label_len = (label != NULL) ? strlen(label) : 0;
    WLX_Widget_Layout wly = WLX_WIDGET_LAYOUT(opt);
    if (wly.width <= 0 && wlx_parent_wants_intrinsic_width(ctx, wly.pos)) {
        WLX_Text_Style mts = { .font = opt.font, .font_size = opt.font_size,
                               .spacing = opt.spacing };
        wly.intrinsic_w = wlx_intrinsic_text_width_slice(ctx, label, label_len, mts)
            + WLX_INTRINSIC_PAD_LR(ctx, opt);
    }

    // The face is a button drawn from the resolved menu-button options
    // (text-only: the image fields stay zero).
    WLX_Button_Opt face = {
        WLX_LAYOUT_SLOT_COPY(opt), WLX_WIDGET_SIZING_COPY(opt),
        WLX_WIDGET_STATE_COPY(opt), WLX_TEXT_TYPOGRAPHY_COPY(opt),
        WLX_TEXT_COLOR_COPY(opt), WLX_BORDER_COPY(opt),
        WLX_CONTENT_PADDING_COPY(opt),
        .hover_brightness = opt.hover_brightness,
        .hover_back_color = opt.hover_back_color,
        .wrap             = false,
    };
    WLX_Rect wr;
    WLX_Interaction inter = wlx_button_face(ctx, label, label_len, &face, wly,
                                            NULL, &wr, file, line);

    if (inter.clicked) *open = !*open;

    if (!*open) return wlx_menu_closed(ctx, state, scope_pushed);

    // List styling flows through the shared frame push as a resolved
    // point-anchored opt; the anchor is the face's bottom edge.
    WLX_Menu_Opt mo = WLX_POPUP_LIST_OPT(opt,
        opt.menu_width > 0.0f ? opt.menu_width : wr.w, opt.item_padding);

    if (!wlx_menu_frame_push(ctx, open, state, wr.x, wr.y + wr.h, &mo,
            claim_before, scope_pushed, file, line)) {
        return false;
    }
    // A menu button is one widget: after wlx_menu_end the last-rect query
    // reports the face, not the final item row (the dropdown rule).
    WLX_Menu_Frame *mf = &ctx->menu_stack[ctx->menu_stack_count - 1];
    mf->restore_last_rect = true;
    mf->anchor_rect = wr;
    return true;
}

// Every unset field falls back to the parent frame's resolved value, so a
// submenu matches its parent's styling without repeating options.
static void wlx_resolve_opt_submenu(const WLX_Menu_Frame *parent, WLX_Menu_Opt *opt) {
    const WLX_Menu_Opt *ps = &parent->style;
    if (opt->width <= 0.0f)       opt->width = ps->width;
    if (opt->row_height <= 0.0f)  opt->row_height = ps->row_height;
    if (opt->item_padding < 0.0f) opt->item_padding = ps->item_padding;

    // align and spacing have no "unset" sentinel (WLX_LEFT / 0 are real
    // values), so they inherit whenever left at their defaults.
    if (opt->font == WLX_FONT_DEFAULT) opt->font = ps->font;
    if (opt->font_size <= 0)           opt->font_size = ps->font_size;
    if (opt->spacing == 0)             opt->spacing = ps->spacing;
    if (opt->content_align == WLX_LEFT) opt->content_align = ps->content_align;

    if (wlx_color_is_zero(opt->front_color))  opt->front_color = ps->front_color;
    if (wlx_color_is_zero(opt->back_color))   opt->back_color = ps->back_color;
    if (wlx_color_is_zero(opt->border_color)) opt->border_color = ps->border_color;
    if (opt->border_width < 0.0f)      opt->border_width = ps->border_width;
    if (opt->roundness < 0.0f)         opt->roundness = ps->roundness;
    if (opt->rounded_segments < 0)     opt->rounded_segments = ps->rounded_segments;

    if (wlx_is_float_unset(opt->hover_brightness))
        opt->hover_brightness = ps->hover_brightness;
    if (wlx_color_is_zero(opt->hover_back_color))
        opt->hover_back_color = ps->hover_back_color;
}

WLXDEF bool wlx_submenu_begin_impl(WLX_Context *ctx, bool *open,
    WLX_Menu_Opt opt, const char *file, int line)
{
    assert(open != NULL && "wlx_submenu_begin: open must not be NULL");
    // An empty stack would index before its first entry (out-of-bounds
    // read of the parent frame in release builds), so this guard survives
    // NDEBUG.
    WLX_HARD_ASSERT(ctx->menu_stack_count > 0,
        "wlx_submenu_begin outside a menu body");
    WLX_Menu_Frame *parent = &ctx->menu_stack[ctx->menu_stack_count - 1];
    wlx_resolve_opt_submenu(parent, &opt);

    // A submenu never survives its parent's close: an ancestor leaf
    // activated earlier this frame (the parent already carries item_clicked)
    // or a parent that is re-opening (first_frame) both clear a stale
    // caller-owned flag, whichever side of the submenu the leaf is declared.
    if (parent->item_clicked || parent->first_frame) *open = false;

    bool scope_pushed = wlx_scope_push(ctx, opt.id);
    WLX_State persistent = wlx_get_state_impl(ctx, sizeof(WLX_Menu_State), file, line);
    WLX_Menu_State *state = (WLX_Menu_State *)persistent.data;

    if (!*open) return wlx_menu_closed(ctx, state, scope_pushed);

    // Anchor beside the last emitted item - the trigger - flush to the
    // parent panel's right edge at that row.
    int trigger_row = parent->row_cursor > 0 ? parent->row_cursor - 1 : 0;
    float x = parent->rect.x + parent->rect.w;
    float y = parent->rect.y + (float)trigger_row * parent->style.row_height;

    // The submenu shares the parent's press scope: a press anywhere in the
    // parent subtree (its trigger item included) is an inside press, so
    // the trigger's release toggle closes cleanly instead of fighting an
    // outside-close on the press.
    return wlx_menu_frame_push(ctx, open, state, x, y, &opt,
        parent->claim_before, scope_pushed, file, line);
}

WLXDEF bool wlx_menu_item_impl(WLX_Context *ctx, const char *text,
    WLX_Menu_Item_Opt opt, const char *file, int line)
{
    // An empty stack would index before its first entry (the row-cursor
    // write below would corrupt memory in release builds), so this guard
    // survives NDEBUG.
    WLX_HARD_ASSERT(ctx->menu_stack_count > 0,
        "wlx_menu_item outside wlx_menu_begin/wlx_menu_end");
    WLX_Menu_Frame *mf = &ctx->menu_stack[ctx->menu_stack_count - 1];
    mf->row_cursor++;

    WLX_Button_Opt ropt = wlx_popup_row_opt(&mf->style, opt.disabled, opt.front_color);
    ropt.content_padding_left  = mf->style.item_padding;
    ropt.content_padding_right = mf->style.item_padding;

    bool clicked = wlx_button_impl(ctx, text, ropt, file, line);
    if (clicked && !opt.keep_open) {
        // A leaf activation dismisses the whole open menu chain: every
        // frame on the stack is an ancestor of this item's menu, and each
        // wlx_menu_end on the way out clears its own open flag.
        for (int i = 0; i < ctx->menu_stack_count; i++) {
            ctx->menu_stack[i].item_clicked = true;
        }
    }
    return clicked;
}

WLXDEF void wlx_menu_end(WLX_Context *ctx)
{
    // An unmatched end would drive menu_stack_count to -1 and write through
    // a garbage frame (memory corruption in release builds), so this guard
    // survives NDEBUG.
    WLX_HARD_ASSERT(ctx->menu_stack_count > 0,
        "wlx_menu_end without a matching wlx_menu_begin");
    WLX_Menu_Frame *mf = &ctx->menu_stack[--ctx->menu_stack_count];
    mf->state->item_count = mf->row_cursor;

    wlx_layout_end(ctx);
    wlx_overlay_end(ctx);

    if (mf->item_clicked) *mf->open = false;
    if (wlx_is_key_pressed(ctx, WLX_KEY_ESCAPE)) *mf->open = false;
    if (mf->restore_last_rect) ctx->last_widget_rect = mf->anchor_rect;

    // A press this frame that no query inside this menu claimed happened
    // outside it. The frame the caller opened the menu on is exempt: a
    // press-triggered open would otherwise close itself immediately.
    if (!mf->first_frame && wlx_popup_outside_press(ctx, mf->claim_before)) {
        *mf->open = false;
    }

    wlx_scope_pop(ctx, mf->scope_pushed);
}

// ============================================================================
// List clipper implementation
// ============================================================================

WLXDEF float wlx_list_clipper_height(int item_count, float row_height, const float *item_offsets) {
    if (item_count <= 0) return 0.0f;
    if (item_offsets != NULL) return item_offsets[item_count];
    if (row_height < 1.0f) row_height = 1.0f;
    return (float)item_count * row_height;
}

// Reserve an off-screen band of `px` height as a single empty auto slot. The
// child layout paints nothing, so it adds no draw commands; it only advances
// the content layout so the visible rows land at the right scroll position.
static inline void wlx_list_clipper_spacer(WLX_Context *ctx, float px) {
    if (px <= 0.0f) return;
    wlx_layout_auto_slot_px(ctx, px);
    wlx_layout_begin(ctx, 1, WLX_VERT);
    wlx_layout_end(ctx);
}

WLXDEF WLX_List_Clipper wlx_list_clipper_begin_impl(WLX_Context *ctx, int item_count,
        float row_height, WLX_List_Clipper_Opt opt) {
    assert(ctx != NULL);
    assert(ctx->arena.scroll_panels.count > 0 &&
        "wlx_list_clipper_begin must be called inside a scroll panel");
    if (item_count < 0) item_count = 0;
    if (row_height < 1.0f) row_height = 1.0f;

    WLX_Rect vp = wlx_get_scroll_panel_viewport(ctx);
    float off = wlx_get_scroll_panel_offset(ctx);
    float top = off - opt.overscan;
    float bottom = off + vp.h + opt.overscan;
    if (top < 0.0f) top = 0.0f;

    int first = 0, last = item_count;
    float before = 0.0f, after = 0.0f;
    float total = wlx_list_clipper_height(item_count, row_height, opt.item_offsets);

    if (item_count > 0) {
        if (opt.item_offsets != NULL) {
            const float *o = opt.item_offsets;
            first = 0;
            while (first < item_count && o[first + 1] <= top) first++;
            last = first;
            while (last < item_count && o[last] < bottom) last++;
            before = o[first];
            after  = total - o[last];
        } else {
            first = (int)floorf(top / row_height);
            last  = (int)ceilf(bottom / row_height);
            if (first < 0) first = 0;
            if (first > item_count) first = item_count;
            if (last < first) last = first;
            if (last > item_count) last = item_count;
            before = (float)first * row_height;
            after  = total - (float)last * row_height;
        }
    }
    if (before < 0.0f) before = 0.0f;
    if (after < 0.0f) after = 0.0f;

    // Content layout the visible rows fill; default slot height is row_height so
    // fixed-pitch callers need no per-row sizing call.
    wlx_layout_begin_auto(ctx, WLX_VERT, row_height, .id = opt.id);
    wlx_list_clipper_spacer(ctx, before);

    WLX_List_Clipper clip = {
        .first = first, .last = last,
        .count = item_count, .row_height = row_height,
        .offsets = opt.item_offsets, .before = before, .after = after,
    };
    return clip;
}

WLXDEF void wlx_list_clipper_end(WLX_Context *ctx, WLX_List_Clipper *clip) {
    assert(ctx != NULL && clip != NULL);
    wlx_list_clipper_spacer(ctx, clip->after);
    wlx_layout_end(ctx);  // close the content auto layout
}

WLXDEF float wlx_list_clipper_item_height(const WLX_List_Clipper *clip, int i) {
    assert(clip != NULL);
    if (clip->offsets != NULL && i >= 0 && i < clip->count) {
        return clip->offsets[i + 1] - clip->offsets[i];
    }
    return clip->row_height;
}

// ============================================================================
// Compound widget: Split panel implementation
// ============================================================================

WLXDEF void wlx_split_begin_impl(WLX_Context *ctx, WLX_Split_Opt opt,
                                  const char *file, int line) {
    WLX_Resolved_Padding rp = WLX_RESOLVE_CONTENT_PADDING_EX(ctx, opt, WLX_SPLIT_CONTENT_PADDING);

    WLX_DBG(split_begin, ctx);

    // [1] Outer VERT wrapper (viewport-fill)
    WLX_DBG(split_suppress_warn, ctx, true);
    WLX_Slot_Size fill_sizes[] = { opt.fill_size };
    wlx_layout_begin_impl(ctx, 1, WLX_VERT,
        wlx_default_layout_opt(.sizes = fill_sizes, .id = opt.id), file, line);

    // [2] HORZ split with two pane slots
    WLX_Slot_Size split_sizes[] = { opt.first_size, opt.second_size };
    wlx_layout_begin_impl(ctx, 2, WLX_HORZ,
        wlx_default_layout_opt(.sizes = split_sizes,
            .padding_top = rp.top, .padding_right = rp.right,
            .padding_bottom = rp.bottom, .padding_left = rp.left,
            .gap = opt.gap),
        file, line);
    WLX_DBG(split_suppress_warn, ctx, false);

    // [3] First pane scroll panel (auto-height)
    wlx_scroll_panel_begin_impl(ctx, WLX_SCROLL_AUTO_HEIGHT,
        wlx_default_scroll_panel_opt(.back_color = opt.first_back_color),
        file, line);
}

WLXDEF void wlx_split_next_impl(WLX_Context *ctx, WLX_Split_Next_Opt opt,
                                 const char *file, int line) {
    WLX_DBG(split_next, ctx);

    // Close first pane scroll panel
    wlx_scroll_panel_end(ctx);

    // [5] Open second pane scroll panel (auto-height)
    wlx_scroll_panel_begin_impl(ctx, WLX_SCROLL_AUTO_HEIGHT,
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
    if (opt.capacity <= 0)        opt.capacity = 32;
    // Theme-inherited chrome, matching widget resolution: unset width,
    // roundness and segments take the theme values, zero color takes
    // theme->border. An explicit 0 width stays borderless, an explicit 0
    // roundness stays sharp.
    wlx_resolve_border(ctx->theme, &opt.border_color, &opt.border_width,
                       &opt.roundness, &opt.rounded_segments);

    WLX_Resolved_Padding rp = WLX_RESOLVE_CONTENT_PADDING_EX(ctx, opt, WLX_PANEL_CONTENT_PADDING);

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

    // Create VERT layout with all-CONTENT slots, forwarding body frame options.
    // The panel body clips through the shared layout clip path: when opt.clip
    // is set, wlx_layout_begin_impl begins the scissor on the post-padding
    // content rect (the correct boundary: content stays within the padded area
    // and does not overlap the border) and wlx_layout_end releases it.
    wlx_layout_begin_impl(ctx, (size_t)total, WLX_VERT,
        wlx_default_layout_opt(.sizes = sizes,
            .padding_top = rp.top, .padding_right = rp.right,
            .padding_bottom = rp.bottom, .padding_left = rp.left,
            .gap = opt.gap, .id = opt.id,
            .clip = opt.clip,
            .back_color = opt.back_color,
            .border_color = opt.border_color,
            .border_width = opt.border_width,
            .roundness = opt.roundness,
            .corner_radius = opt.corner_radius,
            .rounded_segments = opt.rounded_segments,
            .border_color_top = opt.border_color_top,
            .border_color_right = opt.border_color_right,
            .border_color_bottom = opt.border_color_bottom,
            .border_color_left = opt.border_color_left,
            .border_width_top = opt.border_width_top,
            .border_width_right = opt.border_width_right,
            .border_width_bottom = opt.border_width_bottom,
            .border_width_left = opt.border_width_left,
            .slot_back_color = opt.slot_back_color,
            .slot_border_color = opt.slot_border_color,
            .slot_border_width = opt.slot_border_width,
            .interact = opt.interact,
            .interact_out = opt.interact_out,
            .hover_back_color = opt.hover_back_color,
            .hover_border_color = opt.hover_border_color,
            .hover_border_color_top = opt.hover_border_color_top,
            .hover_border_color_right = opt.hover_border_color_right,
            .hover_border_color_bottom = opt.hover_border_color_bottom,
            .hover_border_color_left = opt.hover_border_color_left,
            WLX_BOX_STYLE_EFFECTS(opt),
            WLX_BOX_STYLE_GRADIENT(opt)),
        file, line);

    // Emit heading label if title is provided
    if (opt.title != NULL) {
        wlx_label_impl(ctx, opt.title,
            wlx_default_label_opt(
                .font_size = opt.title_font_size,
                .height = opt.title_height,
                .content_align = opt.title_align,
                .show_background = true,
                .back_color = opt.title_back_color),
            file, line);
    }
}

WLXDEF void wlx_panel_end(WLX_Context *ctx) {
    // The panel body is a clip-capable layout: wlx_panel_begin sets clip_active
    // when opt.clip is requested, and wlx_layout_end is the single owner of the
    // scissor release. The panel no longer ends the scissor itself.
    wlx_layout_end(ctx);
}

// ============================================================================
// Perf implementation (WLX_PERF only)
// ============================================================================
#ifdef WLX_PERF

// ----------------------------------------------------------------------------
// Shared backend-adapter perf scaffolding. The core defines it; the adapter
// headers (included after wollix.h in the same TU) consume it. Each adapter
// keeps only its own frame extras, its timestamp source, and thin typed
// wrappers over the clock.
// ----------------------------------------------------------------------------

// One backend perf frame prefix, shared by the three adapters. Field names
// stay flat because demos read the frame structs by name; adapter extras
// (cache counters and the like) follow the macro in each frame struct.
#define WLX_PERF_BACKEND_COMMON_FIELDS \
    uint64_t frame_index; \
    bool timer_available; \
    uint64_t draw_text_calls; \
    uint64_t measure_text_calls; \
    uint64_t draw_rect_calls; \
    uint64_t draw_rect_lines_calls; \
    uint64_t draw_rect_rounded_calls; \
    uint64_t draw_rect_rounded_lines_calls; \
    uint64_t draw_circle_calls; \
    uint64_t draw_ring_calls; \
    uint64_t draw_line_calls; \
    uint64_t draw_texture_calls; \
    uint64_t begin_scissor_calls; \
    uint64_t end_scissor_calls; \
    uint64_t geometry_submit_calls; \
    uint64_t clip_change_calls; \
    uint64_t text_draw_ns; \
    uint64_t text_measure_ns; \
    uint64_t geometry_ns; \
    uint64_t scissor_ns; \
    uint64_t texture_ns; \
    uint64_t present_ns

// Capture gate and timer of one adapter's perf collector. `timestamp` NULL
// (or timer_available false) disables every duration accumulator while the
// call counters keep counting.
typedef struct {
    bool capturing;
    bool timer_available;
    uint64_t present_start_ns;
    WLX_Perf_Timestamp_Fn timestamp;
    void *timestamp_user;
} WLX_Perf_Backend_Clock;

static inline uint64_t wlx_perf_backend_now(const WLX_Perf_Backend_Clock *c) {
    return c->timestamp != NULL ? c->timestamp(c->timestamp_user) : 0;
}

static inline void wlx_perf_backend_inc(const WLX_Perf_Backend_Clock *c, uint64_t *counter) {
    if (!c->capturing) return;
    (*counter)++;
}

static inline uint64_t wlx_perf_backend_time_begin(const WLX_Perf_Backend_Clock *c) {
    if (!c->capturing || !c->timer_available) return 0;
    return wlx_perf_backend_now(c);
}

static inline void wlx_perf_backend_time_end(const WLX_Perf_Backend_Clock *c,
        uint64_t start_ns, uint64_t *total_ns) {
    if (!c->capturing || !c->timer_available) return;
    uint64_t end_ns = wlx_perf_backend_now(c);
    if (end_ns >= start_ns) *total_ns += end_ns - start_ns;
}

static inline void wlx_perf_backend_present_begin(WLX_Perf_Backend_Clock *c) {
    c->present_start_ns = wlx_perf_backend_time_begin(c);
}

static inline void wlx_perf_backend_present_end(WLX_Perf_Backend_Clock *c, uint64_t *total_ns) {
    wlx_perf_backend_time_end(c, c->present_start_ns, total_ns);
    c->present_start_ns = 0;
}

static inline void wlx_perf_backend_frame_begin(WLX_Perf_Backend_Clock *c) {
    c->capturing = true;
    c->present_start_ns = 0;
}

static inline void wlx_perf_backend_frame_end(WLX_Perf_Backend_Clock *c) {
    c->capturing = false;
    c->present_start_ns = 0;
}

typedef struct WLX_Perf_Context {
    WLX_Perf_Timestamp_Fn timestamp_fn;
    void *timer_user;
    WLX_Perf_Frame current;
    WLX_Perf_Frame last;
    uint64_t frame_start_ns;
    uint64_t begin_start_ns;
    uint64_t input_start_ns;
    uint64_t build_start_ns;
    uint64_t end_start_ns;
    uint64_t range_start_ns;
    uint64_t offset_start_ns;
    uint64_t dispatch_start_ns;
    uint64_t callback_start_ns;
    size_t frame_start_capacity[WLX_ARENA_GROUP_COUNT];
    bool capturing;
} WLX_Perf_Context;

static inline uint64_t wlx_perf_now(WLX_Context *ctx) {
    if (!ctx || !ctx->perf || !ctx->perf->timestamp_fn) return 0;
    return ctx->perf->timestamp_fn(ctx->perf->timer_user);
}

static inline uint64_t wlx_perf_elapsed(uint64_t start, uint64_t end) {
    return (end >= start) ? (end - start) : 0;
}

static inline void wlx_perf_init(WLX_Context *ctx) {
    WLX_Perf_Allocator_Stats *saved_sink;

    if (!ctx || ctx->perf) return;
    saved_sink = wlx_perf_allocator_sink;
    wlx_perf_allocator_sink = NULL;
    ctx->perf = (WLX_Perf_Context *)wlx_calloc(1, sizeof(WLX_Perf_Context));
    wlx_perf_allocator_sink = saved_sink;
}

static inline void wlx_perf_set_start_capacity(WLX_Perf_Context *perf,
    WLX_Arena_Group group, const WLX_Sub_Arena *arena)
{
    perf->frame_start_capacity[group] = arena->capacity;
}

static inline void wlx_perf_capture_start_capacities(WLX_Context *ctx) {
    WLX_Perf_Context *perf = (WLX_Perf_Context *)ctx->perf;
    wlx_perf_set_start_capacity(perf, WLX_ARENA_SLOT_SIZE_OFFSETS, &ctx->arena.slot_size_offsets);
    wlx_perf_set_start_capacity(perf, WLX_ARENA_DYN_OFFSETS, &ctx->arena.dyn_offsets);
    wlx_perf_set_start_capacity(perf, WLX_ARENA_SCRATCH, &ctx->arena.scratch);
    wlx_perf_set_start_capacity(perf, WLX_ARENA_COMMANDS, &ctx->arena.commands);
    wlx_perf_set_start_capacity(perf, WLX_ARENA_CMD_RANGES, &ctx->arena.cmd_ranges);
    wlx_perf_set_start_capacity(perf, WLX_ARENA_LAYOUTS, &ctx->arena.layouts);
    wlx_perf_set_start_capacity(perf, WLX_ARENA_SCROLL_PANELS, &ctx->arena.scroll_panels);
    wlx_perf_set_start_capacity(perf, WLX_ARENA_ID_STACK, &ctx->arena.id_stack);
    wlx_perf_set_start_capacity(perf, WLX_ARENA_OPACITY_STACK, &ctx->arena.opacity_stack);
}

static inline size_t wlx_perf_capacity_grow_count(size_t start_capacity, size_t end_capacity) {
    size_t count = 0;
    size_t cap = start_capacity;

    if (end_capacity <= start_capacity) return 0;
    if (cap == 0) {
        count++;
        cap = WLX_DA_INIT_CAP;
    }
    while (cap < end_capacity) {
        assert(cap <= SIZE_MAX / 2 && "size_t overflow in perf arena growth count");
        cap *= 2;
        count++;
    }
    return count;
}

static inline void wlx_perf_snapshot_arena(WLX_Perf_Arena_Stats *out,
    const WLX_Sub_Arena *arena, size_t start_capacity)
{
    out->count = arena->count;
    out->capacity = arena->capacity;
    out->high_water = arena->high_water;
    out->grow_count = wlx_perf_capacity_grow_count(start_capacity, arena->capacity);
    out->bytes_used = arena->count * arena->item_size;
    out->bytes_capacity = arena->capacity * arena->item_size;
}

static inline void wlx_perf_capture_arenas(WLX_Context *ctx) {
    WLX_Perf_Context *perf = (WLX_Perf_Context *)ctx->perf;
    wlx_perf_snapshot_arena(&perf->current.arena[WLX_ARENA_SLOT_SIZE_OFFSETS],
        &ctx->arena.slot_size_offsets, perf->frame_start_capacity[WLX_ARENA_SLOT_SIZE_OFFSETS]);
    wlx_perf_snapshot_arena(&perf->current.arena[WLX_ARENA_DYN_OFFSETS],
        &ctx->arena.dyn_offsets, perf->frame_start_capacity[WLX_ARENA_DYN_OFFSETS]);
    wlx_perf_snapshot_arena(&perf->current.arena[WLX_ARENA_SCRATCH],
        &ctx->arena.scratch, perf->frame_start_capacity[WLX_ARENA_SCRATCH]);
    wlx_perf_snapshot_arena(&perf->current.arena[WLX_ARENA_COMMANDS],
        &ctx->arena.commands, perf->frame_start_capacity[WLX_ARENA_COMMANDS]);
    wlx_perf_snapshot_arena(&perf->current.arena[WLX_ARENA_CMD_RANGES],
        &ctx->arena.cmd_ranges, perf->frame_start_capacity[WLX_ARENA_CMD_RANGES]);
    wlx_perf_snapshot_arena(&perf->current.arena[WLX_ARENA_LAYOUTS],
        &ctx->arena.layouts, perf->frame_start_capacity[WLX_ARENA_LAYOUTS]);
    wlx_perf_snapshot_arena(&perf->current.arena[WLX_ARENA_SCROLL_PANELS],
        &ctx->arena.scroll_panels, perf->frame_start_capacity[WLX_ARENA_SCROLL_PANELS]);
    wlx_perf_snapshot_arena(&perf->current.arena[WLX_ARENA_ID_STACK],
        &ctx->arena.id_stack, perf->frame_start_capacity[WLX_ARENA_ID_STACK]);
    wlx_perf_snapshot_arena(&perf->current.arena[WLX_ARENA_OPACITY_STACK],
        &ctx->arena.opacity_stack, perf->frame_start_capacity[WLX_ARENA_OPACITY_STACK]);
}

WLXDEF void wlx_perf_set_timer(WLX_Context *ctx, WLX_Perf_Timestamp_Fn timestamp_fn, void *user) {
    assert(ctx != NULL);
    wlx_perf_init(ctx);
    if (!ctx->perf) return;
    ctx->perf->timestamp_fn = timestamp_fn;
    ctx->perf->timer_user = user;
}

WLXDEF const WLX_Perf_Frame *wlx_perf_get_last_frame(const WLX_Context *ctx) {
    if (!ctx || !ctx->perf) return NULL;
    return &ctx->perf->last;
}

WLXDEF void wlx_perf_reset(WLX_Context *ctx) {
    WLX_Perf_Context *perf;
    WLX_Perf_Timestamp_Fn timestamp_fn;
    void *timer_user;

    assert(ctx != NULL);
    if (!ctx->perf) return;

    perf = (WLX_Perf_Context *)ctx->perf;
    timestamp_fn = perf->timestamp_fn;
    timer_user = perf->timer_user;
    if (wlx_perf_allocator_sink == &perf->current.allocator) {
        wlx_perf_allocator_sink = NULL;
    }
    wlx_zero_struct(*perf);
    perf->timestamp_fn = timestamp_fn;
    perf->timer_user = timer_user;
}

static inline void wlx_perf_frame_begin(WLX_Context *ctx) {
    WLX_Perf_Context *perf;
    uint64_t frame_index;

    wlx_perf_init(ctx);
    if (!ctx->perf) return;

    perf = (WLX_Perf_Context *)ctx->perf;
    frame_index = perf->last.frame_index + 1;
    wlx_zero_struct(perf->current);
    perf->current.frame_index = frame_index;
    perf->current.timer_available = (perf->timestamp_fn != NULL);
    perf->capturing = true;
    wlx_perf_allocator_sink = &perf->current.allocator;
    wlx_perf_capture_start_capacities(ctx);
    perf->frame_start_ns = wlx_perf_now(ctx);
    perf->begin_start_ns = perf->frame_start_ns;
}

static inline void wlx_perf_input_begin(WLX_Context *ctx) {
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    ctx->perf->input_start_ns = wlx_perf_now(ctx);
}

static inline void wlx_perf_input_end(WLX_Context *ctx) {
    uint64_t now;
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    now = wlx_perf_now(ctx);
    ctx->perf->current.timings.input_ns += wlx_perf_elapsed(ctx->perf->input_start_ns, now);
}

static inline void wlx_perf_begin_end(WLX_Context *ctx) {
    uint64_t now;
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    now = wlx_perf_now(ctx);
    ctx->perf->current.timings.begin_ns = wlx_perf_elapsed(ctx->perf->begin_start_ns, now);
    ctx->perf->build_start_ns = now;
}

static inline void wlx_perf_end_begin(WLX_Context *ctx) {
    uint64_t now;
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    now = wlx_perf_now(ctx);
    ctx->perf->end_start_ns = now;
    ctx->perf->current.timings.user_build_ns = wlx_perf_elapsed(ctx->perf->build_start_ns, now);
}

static inline void wlx_perf_range_begin(WLX_Context *ctx) {
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    ctx->perf->range_start_ns = wlx_perf_now(ctx);
}

static inline void wlx_perf_range_end(WLX_Context *ctx) {
    uint64_t now;
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    now = wlx_perf_now(ctx);
    ctx->perf->current.timings.range_accum_ns += wlx_perf_elapsed(ctx->perf->range_start_ns, now);
}

static inline void wlx_perf_offset_begin(WLX_Context *ctx) {
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    ctx->perf->offset_start_ns = wlx_perf_now(ctx);
}

static inline void wlx_perf_offset_end(WLX_Context *ctx) {
    uint64_t now;
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    now = wlx_perf_now(ctx);
    ctx->perf->current.timings.offset_lookup_ns += wlx_perf_elapsed(ctx->perf->offset_start_ns, now);
}

static inline void wlx_perf_dispatch_begin(WLX_Context *ctx) {
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    ctx->perf->dispatch_start_ns = wlx_perf_now(ctx);
}

static inline void wlx_perf_dispatch_end(WLX_Context *ctx) {
    uint64_t now;
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    now = wlx_perf_now(ctx);
    ctx->perf->current.timings.dispatch_ns += wlx_perf_elapsed(ctx->perf->dispatch_start_ns, now);
}

static inline void wlx_perf_backend_callback_begin(WLX_Context *ctx, WLX_Cmd_Type type) {
    WLX_UNUSED(type);
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    ctx->perf->callback_start_ns = wlx_perf_now(ctx);
}

static inline void wlx_perf_backend_callback_end(WLX_Context *ctx, WLX_Cmd_Type type) {
    uint64_t now;
    WLX_UNUSED(type);
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    now = wlx_perf_now(ctx);
    ctx->perf->current.timings.backend_callback_ns += wlx_perf_elapsed(ctx->perf->callback_start_ns, now);
}

static inline void wlx_perf_command_record(WLX_Context *ctx, WLX_Cmd_Type type) {
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    if (type >= 0 && type < WLX_CMD_TYPE_COUNT) {
        ctx->perf->current.commands.by_type[type]++;
    }
    ctx->perf->current.commands.total_commands++;
}

static inline void wlx_perf_text_measure(WLX_Context *ctx, size_t bytes) {
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    ctx->perf->current.text.measure_calls++;
    ctx->perf->current.text.measured_bytes += (uint64_t)bytes;
}

static inline void wlx_perf_text_command(WLX_Context *ctx, size_t bytes) {
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    ctx->perf->current.text.emitted_text_commands++;
    ctx->perf->current.text.emitted_text_bytes += (uint64_t)bytes;
}

static inline void wlx_perf_text_run(WLX_Context *ctx, size_t bytes) {
    WLX_UNUSED(bytes);
    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    ctx->perf->current.text.fitted_text_runs++;
}

static inline void wlx_perf_frame_publish(WLX_Context *ctx) {
    WLX_Perf_Context *perf;
    uint64_t now;

    if (!ctx || !ctx->perf || !ctx->perf->capturing) return;
    perf = (WLX_Perf_Context *)ctx->perf;
    now = wlx_perf_now(ctx);
    perf->current.timings.end_ns = wlx_perf_elapsed(perf->end_start_ns, now);
    perf->current.timings.total_ns = wlx_perf_elapsed(perf->frame_start_ns, now);
    perf->current.commands.command_ranges = ctx->arena.cmd_ranges.count;
    wlx_perf_capture_arenas(ctx);
    perf->last = perf->current;
    perf->capturing = false;
    if (wlx_perf_allocator_sink == &perf->current.allocator) {
        wlx_perf_allocator_sink = NULL;
    }
}

static inline void wlx_perf_destroy(WLX_Context *ctx) {
    WLX_Perf_Allocator_Stats *saved_sink;

    if (!ctx || !ctx->perf) return;
    saved_sink = wlx_perf_allocator_sink;
    if (saved_sink == &ctx->perf->current.allocator) {
        saved_sink = NULL;
    }
    wlx_perf_allocator_sink = NULL;
    wlx_free(ctx->perf);
    ctx->perf = NULL;
    wlx_perf_allocator_sink = saved_sink;
}

#endif // WLX_PERF

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

// Slot sizes a debug shadow sees for layout `idx`: the layout's scratch
// copy when it retained one (re-derived per read - the scratch may have
// grown since begin), else the caller-supplied array recorded at begin.
static inline const WLX_Slot_Size *wlx_dbg_shadow_sizes(const WLX_Context *ctx, size_t idx) {
    const WLX_Slot_Size *cs = wlx_layout_content_sizes(ctx, &wlx_pool_layouts(ctx)[idx]);
    return cs != NULL ? cs : ctx->dbg->layout_shadow[idx].sizes;
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
            } else if (wlx_dbg_shadow_sizes(ctx, idx - 1) != NULL) {
                // VERT parent: child bound depends on the parent slot kind.
                const WLX_Slot_Size *parent_sizes = wlx_dbg_shadow_sizes(ctx, idx - 1);
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
    // Readers go through wlx_dbg_shadow_sizes, which prefers the layout's
    // scratch-arena copy when it retained one (re-derived per read).
    // Compound widgets like wlx_panel build their sizes array on the stack;
    // the user-supplied `sizes` pointer becomes dangling once the compound
    // widget's frame returns, so it is recorded only as the fallback for
    // layouts that retained no copy.
    {
        WLX_Layout *self = &wlx_pool_layouts(ctx)[idx];
        shadow->sizes = self->has_content_sizes ? NULL : sizes;
    }
    shadow->count = count;

    bool cb = false;
    if (idx > 0) {
        WLX_Debug_Layout_Shadow *parent_shadow = &ctx->dbg->layout_shadow[idx - 1];
        const WLX_Slot_Size *psizes = wlx_dbg_shadow_sizes(ctx, idx - 1);
        if (parent_shadow->content_bootstrapping) {
            cb = true;
        } else if (psizes != NULL) {
            WLX_Layout *parent = &wlx_pool_layouts(ctx)[idx - 1];
            if (parent->kind == WLX_LAYOUT_LINEAR) {
                size_t slot_idx = (pos >= 0)
                    ? (size_t)pos
                    : (parent->index > (size_t)span
                        ? parent->index - (size_t)span : 0);
                WLX_Layout *current = &wlx_pool_layouts(ctx)[idx];
                float dim = wlx_layout_is_horz(parent) ? current->rect.w : current->rect.h;
                if (slot_idx < parent_shadow->count
                    && psizes[slot_idx].kind == WLX_SIZE_CONTENT
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
    const WLX_Slot_Size *sizes = wlx_layout_content_sizes(ctx, l);
    if (l->content_state == NULL || sizes == NULL) return;

    WLX_Debug_Content_Slot_State *comp = wlx_dbg_get_companion(ctx, l->content_state);
    if (!comp) return;

    // Determine iteration count and measure source based on layout kind
    // (grid rows carry heights; linear slots carry main-axis measures).
    size_t slot_count;
    float *measures;
    if (l->kind == WLX_LAYOUT_GRID && l->has_grid_row_content_heights) {
        slot_count = l->grid.rows;
        measures = wlx_grid_row_content_heights(ctx, l);
    } else if (l->has_content_slot_measures) {
        slot_count = l->count;
        measures = wlx_layout_content_measures(ctx, l);
    } else {
        return;
    }

    for (size_t i = 0; i < slot_count; i++) {
        if (sizes[i].kind == WLX_SIZE_CONTENT) {
            float new_val = measures[i];
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

// Warn when a linear layout's slots resolve to a boundary past its own extent
// (the trailing slot overflows onto whatever follows). Quiet when an active
// clip contains the overflow; deduplicated per call-site so a resize that
// holds the layout under its floor does not spam the log.
static inline void wlx_dbg_slot_overflow(WLX_Context *ctx, const WLX_Layout *l,
                                         const char *file, int line) {
    if (!ctx->dbg) return;
    if (l->kind != WLX_LAYOUT_LINEAR || l->count == 0) return;
    WLX_Rect active;
    if (wlx_active_scissor_rect(ctx, &active)) return;  // clipped: contained
    float total = wlx_layout_main_extent(l);
    const float *offsets = wlx_layout_offsets(ctx, l);
    float resolved = offsets[l->count];
    if (resolved > total + 1.0f) {
        wlx_dbg_warn_once(ctx, file, line,
            "layout slots over-allocate: resolved %.1fpx exceeds %s %.1fpx by "
            "%.1fpx (%zu slots) - fixed/min sizes do not fit and redistribution "
            "cannot shrink them; clip the layout or reduce the sizes",
            resolved, wlx_layout_is_horz(l) ? "width" : "height",
            total, resolved - total, l->count);
    }
}

static inline bool wlx_dbg_widget_in_content_slot(WLX_Context *ctx, int span) {
    if (!ctx->dbg) return false;
    if (ctx->arena.layouts.count == 0) return false;

    size_t idx = ctx->arena.layouts.count - 1;
    WLX_Layout *pl = &wlx_pool_layouts(ctx)[idx];

    const WLX_Slot_Size *cs = wlx_layout_content_sizes(ctx, pl);
    if (cs != NULL) {
        if (pl->kind == WLX_LAYOUT_GRID && pl->has_grid_row_content_heights) {
            size_t row = pl->grid.last_placed_row;
            return row < pl->grid.rows && cs[row].kind == WLX_SIZE_CONTENT;
        }

        if (pl->kind == WLX_LAYOUT_LINEAR && pl->index >= (size_t)span
                && wlx_layout_slot_is_content(ctx, pl, pl->index - (size_t)span))
            return true;
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
            "  [1] outer VERT layout (fill_size, default=FLEX(1))\n"
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

// ============================================================================
// Implementation: v1 backend shim
// ============================================================================
// One trampoline per callback: `user` is the v1 table.

#define WLX_V1(user) ((const WLX_Backend_V1 *)(user))
static void wlx_v1_draw_rect(WLX_Rect r, WLX_Color c, void *u) { WLX_V1(u)->draw_rect(r, c); }
static void wlx_v1_draw_rect_lines(WLX_Rect r, float t, WLX_Color c, void *u) { WLX_V1(u)->draw_rect_lines(r, t, c); }
static void wlx_v1_draw_rect_rounded(WLX_Rect r, float ro, int seg, WLX_Color c, void *u) { WLX_V1(u)->draw_rect_rounded(r, ro, seg, c); }
static void wlx_v1_draw_rect_rounded_lines(WLX_Rect r, float ro, int seg, float t, WLX_Color c, void *u) { WLX_V1(u)->draw_rect_rounded_lines(r, ro, seg, t, c); }
static void wlx_v1_draw_circle(float cx, float cy, float rad, int seg, WLX_Color c, void *u) { WLX_V1(u)->draw_circle(cx, cy, rad, seg, c); }
static void wlx_v1_draw_ring(float cx, float cy, float ri, float ro, int seg, WLX_Color c, void *u) { WLX_V1(u)->draw_ring(cx, cy, ri, ro, seg, c); }
static void wlx_v1_draw_line(float x1, float y1, float x2, float y2, float t, WLX_Color c, void *u) { WLX_V1(u)->draw_line(x1, y1, x2, y2, t, c); }
static void wlx_v1_draw_text(const char *text, float x, float y, WLX_Text_Style st, void *u) { WLX_V1(u)->draw_text(text, x, y, st); }
static void wlx_v1_measure_text(const char *text, WLX_Text_Style st, float *w, float *h, void *u) { WLX_V1(u)->measure_text(text, st, w, h); }
static void wlx_v1_draw_texture(WLX_Texture t, WLX_Rect s, WLX_Rect d, WLX_Color c, void *u) { WLX_V1(u)->draw_texture(t, s, d, c); }
static void wlx_v1_begin_scissor(WLX_Rect r, void *u) { WLX_V1(u)->begin_scissor(r); }
static void wlx_v1_end_scissor(void *u) { WLX_V1(u)->end_scissor(); }
static float wlx_v1_get_frame_time(void *u) { return WLX_V1(u)->get_frame_time(); }
static void wlx_v1_draw_text_slice(const char *text, size_t len, float x, float y, WLX_Text_Style st, void *u) { WLX_V1(u)->draw_text_slice(text, len, x, y, st); }
static void wlx_v1_measure_text_slice(const char *text, size_t len, WLX_Text_Style st, float *w, float *h, void *u) { WLX_V1(u)->measure_text_slice(text, len, st, w, h); }
static size_t wlx_v1_measure_text_advances(const char *text, size_t len, WLX_Text_Style st, const size_t *ends, size_t n, float *adv, void *u) { return WLX_V1(u)->measure_text_advances(text, len, st, ends, n, adv); }
static void wlx_v1_draw_shadow(WLX_Rect r, WLX_Color c, float ox, float oy, float blur, int layers, float ro, int seg, void *u) { WLX_V1(u)->draw_shadow(r, c, ox, oy, blur, layers, ro, seg); }
static void wlx_v1_draw_glow(WLX_Rect r, WLX_Color c, float spread, int rings, float ro, int seg, void *u) { WLX_V1(u)->draw_glow(r, c, spread, rings, ro, seg); }
static void wlx_v1_draw_gradient_v(WLX_Rect r, WLX_Color top, WLX_Color bottom, float ro, int seg, void *u) { WLX_V1(u)->draw_gradient_v(r, top, bottom, ro, seg); }
static const char *wlx_v1_clipboard_get(void *u) { return WLX_V1(u)->clipboard_get(); }
static void wlx_v1_clipboard_set(const char *text, size_t len, void *u) { WLX_V1(u)->clipboard_set(text, len); }
static void wlx_v1_set_cursor(WLX_Cursor_Shape shape, void *u) { WLX_V1(u)->set_cursor(shape); }
#undef WLX_V1

WLXDEF WLX_Backend wlx_backend_from_v1(const WLX_Backend_V1 *v1) {
    WLX_Backend b;
    memset(&b, 0, sizeof(b));
    b.contract_version = WLX_BACKEND_CONTRACT_VERSION;
    b.user = (void *)v1;
#define WLX_V1_FORWARD(member) if (v1->member != NULL) b.member = wlx_v1_##member
    WLX_V1_FORWARD(draw_rect);
    WLX_V1_FORWARD(draw_rect_lines);
    WLX_V1_FORWARD(draw_rect_rounded);
    WLX_V1_FORWARD(draw_rect_rounded_lines);
    WLX_V1_FORWARD(draw_circle);
    WLX_V1_FORWARD(draw_ring);
    WLX_V1_FORWARD(draw_line);
    WLX_V1_FORWARD(draw_text);
    WLX_V1_FORWARD(measure_text);
    WLX_V1_FORWARD(draw_texture);
    WLX_V1_FORWARD(begin_scissor);
    WLX_V1_FORWARD(end_scissor);
    WLX_V1_FORWARD(get_frame_time);
    WLX_V1_FORWARD(draw_text_slice);
    WLX_V1_FORWARD(measure_text_slice);
    WLX_V1_FORWARD(measure_text_advances);
    WLX_V1_FORWARD(draw_shadow);
    WLX_V1_FORWARD(draw_glow);
    WLX_V1_FORWARD(draw_gradient_v);
    WLX_V1_FORWARD(clipboard_get);
    WLX_V1_FORWARD(clipboard_set);
    WLX_V1_FORWARD(set_cursor);
#undef WLX_V1_FORWARD
    return b;
}

// ============================================================================
// Implementation: option defaults as values
// ============================================================================

WLXDEF WLX_Slot_Style_Opt wlx_slot_style_opt_defaults(void) { return wlx_default_slot_style_opt(); }
WLXDEF WLX_Grid_Opt wlx_grid_opt_defaults(void) { return wlx_default_grid_opt(); }
WLXDEF WLX_Grid_Auto_Opt wlx_grid_auto_opt_defaults(void) { return wlx_default_grid_auto_opt(); }
WLXDEF WLX_Layout_Opt wlx_layout_opt_defaults(void) { return wlx_default_layout_opt(); }
WLXDEF WLX_Overlay_Opt wlx_overlay_opt_defaults(void) { return wlx_default_overlay_opt(); }
WLXDEF WLX_Widget_Opt wlx_widget_opt_defaults(void) { return wlx_default_widget_opt(); }
WLXDEF WLX_Label_Opt wlx_label_opt_defaults(void) { return wlx_default_label_opt(); }
WLXDEF WLX_Button_Opt wlx_button_opt_defaults(void) { return wlx_default_button_opt(); }
WLXDEF WLX_Dropdown_Opt wlx_dropdown_opt_defaults(void) { return wlx_default_dropdown_opt(); }
WLXDEF WLX_Tooltip_Opt wlx_tooltip_opt_defaults(void) { return wlx_default_tooltip_opt(); }
WLXDEF WLX_Menu_Opt wlx_menu_opt_defaults(void) { return wlx_default_menu_opt(); }
WLXDEF WLX_Menu_Item_Opt wlx_menu_item_opt_defaults(void) { return wlx_default_menu_item_opt(); }
WLXDEF WLX_Menu_Button_Opt wlx_menu_button_opt_defaults(void) { return wlx_default_menu_button_opt(); }
WLXDEF WLX_Checkbox_Opt wlx_checkbox_opt_defaults(void) { return wlx_default_checkbox_opt(); }
WLXDEF WLX_Inputbox_Opt wlx_inputbox_opt_defaults(void) { return wlx_default_inputbox_opt(); }
WLXDEF WLX_Slider_Opt wlx_slider_opt_defaults(void) { return wlx_default_slider_opt(); }
WLXDEF WLX_Separator_Opt wlx_separator_opt_defaults(void) { return wlx_default_separator_opt(); }
WLXDEF WLX_Progress_Opt wlx_progress_opt_defaults(void) { return wlx_default_progress_opt(); }
WLXDEF WLX_Image_Opt wlx_image_opt_defaults(void) { return wlx_default_image_opt(); }
WLXDEF WLX_Toggle_Opt wlx_toggle_opt_defaults(void) { return wlx_default_toggle_opt(); }
WLXDEF WLX_Radio_Opt wlx_radio_opt_defaults(void) { return wlx_default_radio_opt(); }
WLXDEF WLX_Scroll_Panel_Opt wlx_scroll_panel_opt_defaults(void) { return wlx_default_scroll_panel_opt(); }
WLXDEF WLX_List_Clipper_Opt wlx_list_clipper_opt_defaults(void) { return wlx_default_list_clipper_opt(); }
WLXDEF WLX_Split_Opt wlx_split_opt_defaults(void) { return wlx_default_split_opt(); }
WLXDEF WLX_Split_Next_Opt wlx_split_next_opt_defaults(void) { return wlx_default_split_next_opt(); }
WLXDEF WLX_Panel_Opt wlx_panel_opt_defaults(void) { return wlx_default_panel_opt(); }

#endif // WOLLIX_IMPLEMENTATION
