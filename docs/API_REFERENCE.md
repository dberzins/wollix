# API Reference

Complete reference for every public function, struct, enum, and macro in the
layout library.

> **Header files:**
> - `wollix.h` — core library (include with `#define WOLLIX_IMPLEMENTATION` in exactly one translation unit)
> - `wollix_raylib.h` — Raylib backend adapter
> - `wollix_sdl3.h` — SDL3 backend adapter
> - `wollix_wasm.h` — bare WASM32 backend adapter and page-pool allocator helpers

> **Related docs:**
> - [LAYOUT_MODEL.md](LAYOUT_MODEL.md)
> - [SENTINEL.md](SENTINEL.md)
> - [CORE_PATTERNS_GUIDE.md](CORE_PATTERNS_GUIDE.md)

---

## Table of Contents

1. [Feature Flags](#feature-flags)
2. [Types — Geometry & Color](#types--geometry--color)
3. [Types — Backend](#types--backend)
4. [Types — Enums](#types--enums)
5. [Types — Input](#types--input)
6. [Types — Interaction](#types--interaction)
7. [Types — Layout](#types--layout)
8. [Types — Persistent State](#types--persistent-state)
9. [Types — Theme](#types--theme)
10. [Types — Context](#types--context)
11. [Frame Lifecycle](#frame-lifecycle)
12. [Layout API](#layout-api)
13. [Grid API](#grid-api)
14. [Slot Size Macros](#slot-size-macros)
15. [Interaction & State](#interaction--state)
16. [Input Queries](#input-queries)
17. [Utility Functions](#utility-functions)
18. [Widget — `wlx_widget`](#widget--wlx_widget)
19. [Widget — `wlx_label`](#widget--wlx_label)
20. [Widget — `wlx_button`](#widget--wlx_button)
21. [Widget — `wlx_checkbox`](#widget--wlx_checkbox)
22. [Widget — `wlx_inputbox`](#widget--wlx_inputbox)
23. [Widget — `wlx_slider`](#widget--wlx_slider)
24. [Widget — `wlx_separator`](#widget--wlx_separator)
25. [Widget — `wlx_progress`](#widget--wlx_progress)
26. [Widget — `wlx_toggle`](#widget--wlx_toggle)
27. [Widget — `wlx_radio`](#widget--wlx_radio)
28. [Widget — `wlx_scroll_panel`](#widget--wlx_scroll_panel)
29. [Compound Widget — `wlx_split`](#compound-widget--wlx_split)
30. [Compound Widget — `wlx_panel`](#compound-widget--wlx_panel)
31. [Shared Option Field Macros](#shared-option-field-macros)
32. [Theme Presets](#theme-presets)
33. [Backend — Raylib](#backend--raylib)
34. [Backend — SDL3](#backend--sdl3)

---

## Feature Flags

### `WLX_SHORT_NAMES`

```c
// #define WLX_SHORT_NAMES   // define before including wollix.h
```

Exposes un-prefixed convenience aliases for the `wlx_*` API. Optional and
off by default. All public `wlx_*` functions remain available regardless.

| Short alias | Full name |
|---|---|
| `layout_begin(...)` | `wlx_layout_begin(...)` |
| `layout_begin_s(...)` | `wlx_layout_begin_s(...)` |
| `layout_begin_auto(...)` | `wlx_layout_begin_auto(...)` |
| `layout_auto_slot(...)` | `wlx_layout_auto_slot(...)` |
| `layout_auto_slot_px(...)` | `wlx_layout_auto_slot_px(...)` |
| `layout_end(ctx)` | `wlx_layout_end(ctx)` |
| `grid_cell(...)` | `wlx_grid_cell(...)` |
| `grid_begin(...)` | `wlx_grid_begin(...)` |
| `grid_begin_auto(...)` | `wlx_grid_begin_auto(...)` |
| `grid_begin_auto_tile(...)` | `wlx_grid_begin_auto_tile(...)` |
| `grid_end(ctx)` | `wlx_grid_end(ctx)` |
| `label(...)` | `wlx_label(...)` |
| `button(...)` | `wlx_button(...)` |
| `checkbox(...)` | `wlx_checkbox(...)` |
| `inputbox(...)` | `wlx_inputbox(...)` |
| `slider(...)` | `wlx_slider(...)` |
| `separator(...)` | `wlx_separator(...)` |
| `progress(ctx, value, ...)` | `wlx_progress(ctx, value, ...)` |
| `toggle(ctx, label, value, ...)` | `wlx_toggle(ctx, label, value, ...)` |
| `radio(ctx, label, active, index, ...)` | `wlx_radio(ctx, label, active, index, ...)` |
| `scroll_panel_begin(...)` | `wlx_scroll_panel_begin(...)` |
| `scroll_panel_end(ctx)` | `wlx_scroll_panel_end(ctx)` |
| `split_begin(...)` | `wlx_split_begin(...)` |
| `split_next(...)` | `wlx_split_next(...)` |
| `split_end(ctx)` | `wlx_split_end(ctx)` |
| `panel_begin(...)` | `wlx_panel_begin(...)` |
| `panel_end(ctx)` | `wlx_panel_end(ctx)` |
| `widget(...)` | `wlx_widget(...)` |

---

### `WLX_SLOT_MINMAX_REDISTRIBUTE`

```c
// #define WLX_SLOT_MINMAX_REDISTRIBUTE   // define before including wollix.h
```

Enable iterative freeze-and-redistribute for slot min/max constraints.
When enabled, surplus/deficit from clamped slots is redistributed among
unfrozen siblings so that `offsets[count] == total` (no gaps or overflow).

**Default (undefined):** single-pass clamp — simpler, O(n), but may leave a
gap or overflow when min/max constraints fire.

---

## Types — Geometry & Color

### `WLX_Rect`

```c
typedef struct {
    float x, y, w, h;
} WLX_Rect;
```

Axis-aligned rectangle used for all layout rects, widget rects, and scissor
regions.

### `WLX_Color`

```c
// With Raylib:
typedef Color WLX_Color;

// Without Raylib:
typedef struct {
    uint8_t r, g, b, a;
} WLX_Color;
```

RGBA color. Fields are `r`, `g`, `b`, `a` — each 0–255. When all four are 0
(`{0,0,0,0}`), widget code treats it as a sentinel meaning "use theme default."

### `WLX_RGBA(r, g, b, a)`

```c
#define WLX_RGBA(r, g, b, a) ((WLX_Color){ (r), (g), (b), (a) })
```

Construct a `WLX_Color` literal.

### `WLX_Texture`

```c
// With Raylib:
typedef Texture2D WLX_Texture;

// Without Raylib:
typedef struct {
    uintptr_t handle;
    int width, height;
} WLX_Texture;
```

Opaque texture handle passed to `draw_texture`. Backend-specific.

### Color Constants

| Macro | Value |
|-------|-------|
| `WLX_WHITE` | `{255, 255, 255, 255}` |
| `WLX_BLACK` | `{0, 0, 0, 255}` |
| `WLX_LIGHTGRAY` | `{200, 200, 200, 255}` |

### Deprecated Color Constants

| Macro | Value | Replacement |
|-------|-------|-------------|
| `WLX_BACKGROUND_COLOR` | `{18, 18, 18, 255}` | `ctx->theme->background` |
| `WLX_SCROLLBAR_COLOR` | `{38, 38, 38, 255}` | `ctx->theme->scrollbar.bar` |
| `WLX_HOVER_BRIGHTNESS` | `0.75f` | `ctx->theme->hover_brightness` |

---

## Types — Backend

### `WLX_Font`

```c
typedef uintptr_t WLX_Font;
#define WLX_FONT_DEFAULT ((WLX_Font)0)
```

Opaque font identifier. `0` (`WLX_FONT_DEFAULT`) means use the backend's
default font, or the theme's font if set.

### `WLX_Text_Style`

```c
typedef struct {
    WLX_Font  font;       // 0 → backend default font
    int      font_size;  // 0 → resolve from theme
    int      spacing;    // 0 → resolve from theme
    WLX_Color color;      // {0} → resolve from theme foreground
} WLX_Text_Style;
```

Bundled text rendering parameters passed to backend `draw_text` and
`measure_text` callbacks.

### `WLX_TEXT_STYLE_DEFAULT`

```c
#define WLX_TEXT_STYLE_DEFAULT \
    ((WLX_Text_Style){ .font = WLX_FONT_DEFAULT, .font_size = 0, .spacing = 0, .color = {0} })
```

### `WLX_Backend`

```c
typedef struct {
    void  (*draw_rect)(WLX_Rect rect, WLX_Color color);
    void  (*draw_rect_lines)(WLX_Rect rect, float thick, WLX_Color color);
    void  (*draw_rect_rounded)(WLX_Rect rect, float roundness, int segments, WLX_Color color);
    void  (*draw_line)(float x1, float y1, float x2, float y2, float thick, WLX_Color color);
    void  (*draw_text)(const char *text, float x, float y, WLX_Text_Style style);
    void  (*measure_text)(const char *text, WLX_Text_Style style, float *out_w, float *out_h);
    void  (*draw_texture)(WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint);
    void  (*begin_scissor)(WLX_Rect rect);
    void  (*end_scissor)(void);
    float (*get_frame_time)(void);
} WLX_Backend;
```

Function-pointer table that the library calls to perform all rendering and
time queries. Every pointer must be non-NULL — use `wlx_backend_is_ready()` to
verify.

| Callback | Purpose |
|----------|---------|
| `draw_rect` | Fill a rectangle with a solid color |
| `draw_rect_lines` | Draw a rectangle outline with given thickness. Sub-pixel values (`thick < 1`) are rendered as 1px with alpha scaled proportionally (e.g. 0.3 → 1px at 30% opacity) |
| `draw_rect_rounded` | Fill a rounded rectangle |
| `draw_line` | Draw a line segment |
| `draw_text` | Render text at a position with the given style |
| `measure_text` | Measure text dimensions (writes to `out_w`, `out_h`) |
| `draw_texture` | Draw a texture from `src` rect to `dst` rect with a tint |
| `begin_scissor` | Begin a rectangular clip region |
| `end_scissor` | End the current clip region |
| `get_frame_time` | Return elapsed time since last frame in seconds |

### `wlx_backend_is_ready`

```c
static inline bool wlx_backend_is_ready(const WLX_Context *ctx);
```

Returns `true` if `ctx` is non-NULL and all backend function pointers are set.

---

## Types — Enums

### `WLX_Orient`

```c
typedef enum {
    WLX_HORZ,   // Horizontal: children flow left-to-right
    WLX_VERT,   // Vertical: children flow top-to-bottom
} WLX_Orient;
```

### `WLX_Align`

```c
typedef enum {
    WLX_ALIGN_NONE,       // No alignment (use raw slot rect)
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
```

Used for both widget placement inside a slot (`widget_align`) and text
alignment inside a widget (`align`).

### `WLX_Size_Kind`

```c
typedef enum {
    WLX_SIZE_AUTO,      // Auto-size (equal division of remaining space)
    WLX_SIZE_PIXELS,    // Fixed pixel size
    WLX_SIZE_PERCENT,   // Percentage of parent dimension (0.0–1.0)
    WLX_SIZE_FLEX,      // Flex/weight — proportional share of remaining space
} WLX_Size_Kind;
```

### `WLX_Slot_Size`

```c
typedef struct {
    WLX_Size_Kind kind;
    float value;       // Pixels, percentage (0–1), or flex weight
    float min;         // 0 = unconstrained
    float max;         // 0 = unconstrained
} WLX_Slot_Size;
```

Describes how a single slot in a layout or grid is sized.

### `WLX_Key_Code`

```c
typedef enum {
    WLX_KEY_NONE = 0,
    WLX_KEY_ESCAPE, WLX_KEY_ENTER, WLX_KEY_BACKSPACE, WLX_KEY_TAB, WLX_KEY_SPACE,
    WLX_KEY_LEFT, WLX_KEY_RIGHT, WLX_KEY_UP, WLX_KEY_DOWN,
    WLX_KEY_A .. WLX_KEY_Z,
    WLX_KEY_0 .. WLX_KEY_9,
    WLX_KEY_COUNT
} WLX_Key_Code;
```

Backend-neutral key codes. Backend adapters map platform keys to these values.

### `WLX_Layout_Kind`

```c
typedef enum {
    WLX_LAYOUT_LINEAR,  // wlx_layout_begin / wlx_layout_begin_auto
    WLX_LAYOUT_GRID,    // wlx_grid_begin / wlx_grid_begin_auto
} WLX_Layout_Kind;
```

---

## Types — Input

### `WLX_Input_State`

```c
typedef struct {
    int   mouse_x, mouse_y;
    bool  mouse_down;        // Mouse button is down this frame
    bool  mouse_clicked;     // True for one frame on press
    bool  mouse_held;        // True while mouse button is held
    float wheel_delta;       // Positive = up, negative = down
    bool  keys_down[WLX_KEY_COUNT];     // Current held-down states
    bool  keys_pressed[WLX_KEY_COUNT];  // True for one frame on press
    char  text_input[32];              // Text typed this frame (for inputbox)
} WLX_Input_State;
```

Populated by the backend's input handler each frame. Access via `ctx->input`.

### `WLX_Input_Handler`

```c
typedef void (*WLX_Input_Handler)(WLX_Context *ctx);
```

Callback passed to `wlx_begin()`. Called once per frame to fill `ctx->input`.

---

## Types — Interaction

### `WLX_Interact_Flags`

```c
typedef enum {
    WLX_INTERACT_HOVER    = 1 << 0,  // Hover detection (sets hot_id)
    WLX_INTERACT_CLICK    = 1 << 1,  // Button-like: press + release while hovering = clicked
    WLX_INTERACT_FOCUS    = 1 << 2,  // Input-like: stays focused until click elsewhere or Enter
    WLX_INTERACT_DRAG     = 1 << 3,  // Slider-like: active while mouse held after click
    WLX_INTERACT_KEYBOARD = 1 << 4,  // Space/Enter when hovered triggers clicked
} WLX_Interact_Flags;
```

Combine with bitwise OR. Use only **one** of `CLICK` / `FOCUS` / `DRAG` per
call.

### `WLX_Interaction`

```c
typedef struct {
    size_t id;
    bool hover;           // Mouse is over widget
    bool pressed;         // Mouse is currently down on this widget
    bool clicked;         // Click completed (CLICK mode) or keyboard activated
    bool focused;         // Has focus (FOCUS mode)
    bool active;          // Being pressed, dragged, or focused
    bool just_focused;    // Became focused this frame
    bool just_unfocused;  // Lost focus this frame
} WLX_Interaction;
```

Returned by `wlx_get_interaction()`.

---

## Types — Layout

### `WLX_Layout`

```c
typedef struct {
    WLX_Layout_Kind kind;
    WLX_Rect rect;
    size_t count;
    size_t index;
    bool overflow;
    float accumulated_content_height;

    union {
        struct { /* linear fields */ } linear;
        struct { /* grid fields   */ } grid;
    };
} WLX_Layout;
```

Internal layout node pushed onto the layout stack. Typically not accessed
directly — use the layout API functions instead.

### `WLX_Layout_Stack`

```c
typedef struct {
    WLX_Layout *items;
    size_t count, capacity;
} WLX_Layout_Stack;
```

Dynamic array of `WLX_Layout` nodes. Managed by the library; stored in
`ctx->layouts`.

### `WLX_Widget_Opt`

```c
typedef struct {
    WLX_LAYOUT_SLOT_FIELDS;
    WLX_WIDGET_SIZING_FIELDS;
    WLX_Color color;
} WLX_Widget_Opt;
```

Option struct for the `wlx_widget()` colored-rectangle macro. Follows the same
designated-initializer pattern as all other widget option structs.

---

## Types — Persistent State

### `WLX_State`

```c
typedef struct {
    size_t id;
    void  *data;
} WLX_State;
```

Handle returned by `wlx_get_state()`. The `data` pointer is zero-initialized on
first access and survives across frames.

### `WLX_Inputbox_State`

```c
typedef struct {
    size_t cursor_pos;
    float  cursor_blink_time;
} WLX_Inputbox_State;
```

Persistent state for `inputbox` widgets (managed internally).

### `WLX_Scroll_Panel_State`

```c
typedef struct {
    float   scroll_offset;
    float   content_height;
    bool    auto_height;
    WLX_Rect panel_rect;
    bool    dragging_scrollbar;
    float   drag_offset;
    bool    hovered;
    float   wheel_scroll_speed;
    size_t  saved_auto_scroll_panel_id;
    float   saved_auto_scroll_total_height;
} WLX_Scroll_Panel_State;
```

Persistent state for `scroll_panel` widgets (managed internally).

### `WLX_State_Pool`

```c
typedef struct {
    WLX_State_Pool_Entry *items;
    size_t count, capacity;
} WLX_State_Pool;
```

Pool of all persistent state entries. Stored in `ctx->states`.

### `WLX_Id_Stack`

```c
typedef struct {
    size_t *items;
    size_t count, capacity;
} WLX_Id_Stack;
```

Stack of extra ID values pushed by `wlx_push_id()`. Stored in `ctx->id_stack`.

### `WLX_Allocator`

```c
typedef struct WLX_Allocator {
    void *(*alloc)(size_t size, void *user);
    void *(*realloc)(void *ptr, size_t old_size, size_t new_size, void *user);
    void  (*free)(void *ptr, size_t size, void *user);
    void *user;
} WLX_Allocator;
```

Optional runtime allocator interface for arena-backed frame buffers. When an
arena does not have an attached allocator, Wollix continues to use the compile-
time `wlx_alloc` / `wlx_realloc` / `wlx_free` override path.

### `WLX_Sub_Arena`

```c
typedef struct {
    void *items;
    size_t count;
    size_t capacity;
    size_t item_size;
    size_t high_water;
    WLX_Allocator *allocator;
} WLX_Sub_Arena;
```

Reusable growable buffer abstraction for the in-progress memory-management
migration. Adds the type and helper operations without yet replacing
the existing `WLX_Context` frame buffers.

Helper operations:
- `wlx_sub_arena_init`
- `wlx_sub_arena_reserve`
- `wlx_sub_arena_alloc`
- `wlx_sub_arena_alloc_bytes`
- `wlx_sub_arena_reset`
- `wlx_sub_arena_destroy`
- `wlx_sub_arena_at`
- `wlx_sub_arena_bytes_at`

### `WLX_Arena_Pool` and `WLX_Arena_Pool_Config`

```c
typedef struct {
    WLX_Allocator *contiguous;  // backs flat float offset arrays
    WLX_Allocator *general;     // backs scratch, commands, layouts, stacks
} WLX_Arena_Pool_Config;
```

`WLX_Arena_Pool` owns every per-frame buffer on `WLX_Context` (slot offsets,
scratch, commands, cmd_ranges, layouts, scroll panel stack, id stack, opacity
stack). Buffers are grouped into a `contiguous` group (must remain a flat
array, currently slot offsets) and a `general` group (everything else). When
either group's allocator is `NULL`, the underlying sub-arenas use the
compile-time `wlx_realloc` / `wlx_free` macros, preserving zero behavior
change on desktop.

Pool operations are internal; callers configure the pool via
`wlx_context_init_ex`.

### `wlx_context_init` and `wlx_context_init_ex`

```c
void wlx_context_init(WLX_Context *ctx);
void wlx_context_init_ex(WLX_Context *ctx, const WLX_Arena_Pool_Config *cfg);
```

`wlx_context_init` is unchanged and equivalent to
`wlx_context_init_ex(ctx, NULL)`. Pass a non-`NULL` config to attach a
runtime allocator to either arena group. A zero-initialized `WLX_Context`
that skips both still works: `wlx_begin` lazily initializes the pool with
NULL allocators on first use.

### WASM page-pool: `WLX_Wasm_Pool` and `wlx_wasm_allocator`

Defined in `wollix_wasm.h`.

```c
typedef struct {
    WLX_Wasm_Block *free_list[WLX_WASM_POOL_CLASSES];
    size_t alloc_count;
    size_t reuse_count;
    size_t free_count;
    size_t bytes_in_use;
    size_t high_water;
} WLX_Wasm_Pool;

WLX_Allocator wlx_wasm_allocator(WLX_Wasm_Pool *pool);
```

Power-of-two free-list allocator targeted at the bare-WASM backend, where
the libc shim's `free` is a no-op and every `realloc` would otherwise leak
the old block. Buckets cover sizes from 256 bytes
(`WLX_WASM_POOL_MIN_ORDER = 8`) up to 1 MiB
(`WLX_WASM_POOL_MAX_ORDER = 20`). Typical usage attaches the allocator to
the `general` group of `WLX_Arena_Pool_Config`:

```c
static WLX_Wasm_Pool g_pool;
static WLX_Allocator g_alloc;

g_alloc = wlx_wasm_allocator(&g_pool);
WLX_Arena_Pool_Config cfg = { .contiguous = NULL, .general = &g_alloc };
wlx_context_init_ex(ctx, &cfg);
wlx_context_init_wasm(ctx);
```

The counters on `WLX_Wasm_Pool` provide the validation hook: in steady
state `alloc_count` stops growing while `reuse_count` continues to climb.

---

## Types — Theme

### `WLX_Theme`

```c
typedef struct {
    // Global colors
    WLX_Color background;        // Window / panel clear color
    WLX_Color foreground;        // Default text color
    WLX_Color surface;           // Widget background
    WLX_Color border;            // Default border color
    WLX_Color accent;            // Active / focused accent

    // Text
    WLX_Font font;               // Default font (0 = backend default)
    int     font_size;          // Default font size
    int     spacing;            // Default text spacing

    // Geometry
    float   padding;            // Default inner padding
    float   roundness;          // Default corner roundness (0 = sharp)
    int     rounded_segments;   // Segment count for rounded drawing

    // Interaction
    float   hover_brightness;   // Brightness shift on hover

    // Opacity
    float   opacity;            // Global opacity (0 = opaque sentinel, 0.0001–1.0 = explicit)

    // Widget-specific overrides (zero = use globals)
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
        WLX_Color bar;           // Scrollbar thumb color
        float    width;         // Scrollbar width (0 → default 10)
    } scrollbar;
    } WLX_Theme;
```

All `{0}` color values are sentinels meaning "derive from the corresponding
global." Set `ctx->theme` before `wlx_begin()`.

---

## Types — Context

### `WLX_Context`

```c
typedef struct {
    WLX_Rect       rect;
    WLX_Backend    backend;
    WLX_Input_State input;

    struct {
        size_t hot_id;      // Currently hovered widget
        size_t active_id;   // Currently pressed/focused/dragged widget
        size_t next_id;     // Per-frame sequential counter
    } interaction;

    WLX_Layout_Stack       layouts;
    WLX_State_Pool         states;
    WLX_Id_Stack           id_stack;

    struct {
        size_t panel_id;
        float  total_height;
    } auto_scroll;

    WLX_Scroll_Panel_Stack scroll_panels;
    WLX_Scratch_Offsets    slot_size_offsets;

    const WLX_Theme       *theme;   // NULL → &wlx_theme_dark
} WLX_Context;
```

Central state for the entire UI. Allocate with `malloc`, zero-initialize with
`memset`, then set the backend. Call `wlx_context_destroy()` before freeing:

```c
WLX_Context *ctx = malloc(sizeof(*ctx));
memset(ctx, 0, sizeof(*ctx));
wlx_context_init_raylib(ctx);          // or wlx_context_init_sdl3(ctx, win, ren)
ctx->theme = &wlx_theme_light;         // optional
// ... use ctx ...
wlx_context_destroy(ctx);              // frees internal buffers
free(ctx);
```

---

## Frame Lifecycle

### `wlx_begin`

```c
void wlx_begin(WLX_Context *ctx, WLX_Rect r, WLX_Input_Handler input_handler);
```

Start a new frame. Resets per-frame state, calls `input_handler(ctx)` to
populate `ctx->input`, and sets the root layout rect. If `ctx->theme` is NULL,
defaults to `&wlx_theme_dark`.

| Parameter | Description |
|-----------|-------------|
| `ctx` | The UI context |
| `r` | Root rect (typically the full window: `{0, 0, width, height}`) |
| `input_handler` | Callback that fills `ctx->input` (e.g. `wlx_process_raylib_input`) |

### `wlx_end`

```c
void wlx_end(WLX_Context *ctx);
```

End the frame. Finalizes interaction state for the next frame. Call after all
drawing and layout calls.

Because deferred draw commands replay inside `wlx_end()`, keep it inside the
backend's active frame scope. In Raylib that means calling `wlx_end(ctx)`
before `EndDrawing()`. See
[ADR_004_DRAW_COMMAND_REPLAY.md](dev/ADR_004_DRAW_COMMAND_REPLAY.md).

### Frame Loop Pattern

```c
while (!WindowShouldClose()) {
    wlx_begin(ctx, (WLX_Rect){0, 0, w, h}, wlx_process_raylib_input);
    BeginDrawing();
        ClearBackground(WLX_BACKGROUND_COLOR);
        // ... wlx_layout_begin / widgets / wlx_layout_end ...
    wlx_end(ctx);  // replays deferred draw commands; must precede EndDrawing
    EndDrawing();
}
```

---

## Layout API

### `wlx_layout_begin`

```c
#define wlx_layout_begin(ctx, count, orient, ...options)
```

Push a fixed-slot linear layout onto the layout stack. Consumes one slot in the
parent layout.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ctx` | `WLX_Context *` | The UI context |
| `count` | `size_t` | Number of child slots |
| `orient` | `WLX_Orient` | `WLX_HORZ` (left-to-right) or `WLX_VERT` (top-to-bottom) |

**Options** (`WLX_Layout_Opt`):

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `pos` | `int` | `-1` | Target slot in parent (`-1` = next) |
| `span` | `size_t` | `1` | Slots to occupy in parent |
| `overflow` | `bool` | `false` | Allow exceeding slot bounds |
| `padding` | `float` | `0` | Uniform inset on the slot |
| `sizes` | `const WLX_Slot_Size *` | `NULL` | Array of `count` slot sizes. `NULL` = equal division |
| `slot_back_color` | `WLX_Color` | `{0}` | Background fill drawn behind every slot |
| `slot_border_color` | `WLX_Color` | `{0}` | Border color drawn around every slot |
| `slot_border_width` | `float` | `0` | Border thickness in pixels (`0` = no border) |
| `id` | `const char *` | `NULL` | Scope ID: scopes all descendant widget and state IDs for the full layout body. Also keys any container-owned state. `NULL` = no scoping |

### `wlx_layout_begin_auto`

```c
#define wlx_layout_begin_auto(ctx, orient, slot_px, ...options)
```

Push a dynamic layout whose slot count grows as children are added.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ctx` | `WLX_Context *` | The UI context |
| `orient` | `WLX_Orient` | `WLX_HORZ` or `WLX_VERT` |
| `slot_px` | `float` | Fixed pixel size per slot. `0` = variable (use `wlx_layout_auto_slot()` or `wlx_layout_auto_slot_px()`) |

### `wlx_layout_auto_slot`

```c
void wlx_layout_auto_slot(WLX_Context *ctx, WLX_Slot_Size size);
```

Resolve a `WLX_Slot_Size` to pixels and set it as the **next** slot size in the
enclosing dynamic layout. Call immediately before the widget that should receive
this size. The resolved pixel value is consumed automatically when the next
child calls `wlx_get_slot_rect()`.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ctx` | `WLX_Context *` | The UI context |
| `size` | `WLX_Slot_Size` | Any slot size — `WLX_SLOT_PX`, `WLX_SLOT_PCT`, `WLX_SLOT_FLEX`, `WLX_SLOT_FILL`, etc. |

**Resolution rules:**

| Kind | Resolution |
|------|------------|
| `WLX_SIZE_PIXELS` | Exact pixel value |
| `WLX_SIZE_PERCENT` | `value × total / 100` where total is the layout rect width (HORZ) or height (VERT) |
| `WLX_SIZE_FILL` | `value × viewport` (scroll panel viewport, or layout rect if none) |
| `WLX_SIZE_FLEX` | **Greedy:** consumes all remaining space (`total − used`). Only the *last* FLEX slot in the layout gets the correct split — multiple FLEX slots do **not** share proportionally. |
| `WLX_SIZE_AUTO` | Same as FLEX (greedy remaining space) |
| `WLX_SIZE_CONTENT` | Uses `size.value` directly (pre-resolved externally, or 0) |

Min/max constraints from the `WLX_Slot_Size` are applied after resolution.

**Example — PX header, FLEX body, PX footer:**

```c
wlx_layout_begin_auto(ctx, WLX_VERT, 0);
    wlx_layout_auto_slot(ctx, WLX_SLOT_PX(44));
    render_header(ctx);
    wlx_layout_auto_slot(ctx, WLX_SLOT_FLEX(1));  // fill remaining
    render_body(ctx);
    wlx_layout_auto_slot(ctx, WLX_SLOT_PX(24));
    render_footer(ctx);
wlx_layout_end(ctx);
```

> **Note:** FLEX is greedy in dynamic layouts. For proportional multi-FLEX
> splitting, use a static layout (`wlx_layout_begin_s`) instead.

Short alias: `layout_auto_slot(ctx, size)`

### `wlx_layout_auto_slot_px`

```c
void wlx_layout_auto_slot_px(WLX_Context *ctx, float px);
```

Convenience wrapper — equivalent to
`wlx_layout_auto_slot(ctx, WLX_SLOT_PX(px))`.

Override the pixel size for the **next** slot in the enclosing dynamic layout.
Call immediately before the widget whose size differs from the layout's global
`slot_px`. Consumed automatically.

### `wlx_layout_end`

```c
void wlx_layout_end(WLX_Context *ctx);
```

Pop the current layout from the stack. Every `wlx_layout_begin` / `wlx_layout_begin_auto`
/ `wlx_grid_begin` / `wlx_grid_begin_auto` must have a matching `wlx_layout_end` (or
`wlx_grid_end`).

---

## Grid API

### `wlx_grid_begin`

```c
#define wlx_grid_begin(ctx, rows, cols, ...options)
```

Push a fixed-size grid layout.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ctx` | `WLX_Context *` | The UI context |
| `rows` | `size_t` | Number of rows |
| `cols` | `size_t` | Number of columns |

**Options** (`WLX_Grid_Opt`):

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `pos` | `int` | `-1` | Target slot in parent |
| `span` | `size_t` | `1` | Slots to occupy in parent |
| `overflow` | `bool` | `false` | Allow exceeding slot bounds |
| `padding` | `float` | `0` | Uniform inset on the slot |
| `back_color` | `WLX_Color` | `{0}` | Background fill drawn behind the whole grid |
| `border_color` | `WLX_Color` | `{0}` | Border color for the whole grid container |
| `border_width` | `float` | `0` | Border thickness in pixels for the whole grid container |
| `roundness` | `float` | `0` | Rounded corner radius factor for the grid container |
| `rounded_segments` | `int` | `0` | Segment count for rounded corners (`0` = theme default) |
| `gap` | `float` | `0` | Spacing between adjacent cells |
| `row_sizes` | `const WLX_Slot_Size *` | `NULL` | Array of `rows` row sizes. `NULL` = equal division. Supports `WLX_SLOT_CONTENT` (with min/max) for rows that adapt to the tallest widget per row (one-frame delay). Max 32 CONTENT rows. |
| `col_sizes` | `const WLX_Slot_Size *` | `NULL` | Array of `cols` column sizes. `NULL` = equal division |
| `slot_back_color` | `WLX_Color` | `{0}` | Background fill drawn behind every cell |
| `slot_border_color` | `WLX_Color` | `{0}` | Border color drawn around every cell |
| `slot_border_width` | `float` | `0` | Border thickness in pixels (`0` = no border) |
| `id` | `const char *` | `NULL` | Scope ID: scopes all descendant widget and state IDs for the full grid body. `NULL` = no scoping |

Use `border_*` to decorate the outer grid rect and `slot_border_*` to decorate each cell independently.

### `wlx_grid_begin_auto`

```c
#define wlx_grid_begin_auto(ctx, cols, row_px, ...options)
```

Push a dynamic grid whose row count grows as children are added.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ctx` | `WLX_Context *` | The UI context |
| `cols` | `size_t` | Fixed number of columns |
| `row_px` | `float` | Fixed pixel height per row |

**Options** (`WLX_Grid_Auto_Opt`):

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `pos` | `int` | `-1` | Target slot in parent |
| `span` | `size_t` | `1` | Slots to occupy in parent |
| `overflow` | `bool` | `false` | Allow exceeding slot bounds |
| `padding` | `float` | `0` | Uniform inset |
| `back_color` | `WLX_Color` | `{0}` | Background fill drawn behind the whole grid |
| `border_color` | `WLX_Color` | `{0}` | Border color for the whole grid container |
| `border_width` | `float` | `0` | Border thickness in pixels for the whole grid container |
| `roundness` | `float` | `0` | Rounded corner radius factor for the grid container |
| `rounded_segments` | `int` | `0` | Segment count for rounded corners (`0` = theme default) |
| `gap` | `float` | `0` | Spacing between adjacent cells |
| `col_sizes` | `const WLX_Slot_Size *` | `NULL` | Array of `cols` column sizes |
| `slot_back_color` | `WLX_Color` | `{0}` | Background fill drawn behind every cell |
| `slot_border_color` | `WLX_Color` | `{0}` | Border color drawn around every cell |
| `slot_border_width` | `float` | `0` | Border thickness in pixels (`0` = no border) |
| `id` | `const char *` | `NULL` | Scope ID: scopes all descendant widget and state IDs for the full grid body. `NULL` = no scoping |

Use `border_*` to decorate the outer grid rect and `slot_border_*` to decorate each cell independently.

### `wlx_grid_begin_auto_tile`

```c
#define wlx_grid_begin_auto_tile(ctx, tile_w, tile_h, ...options)
```

Convenience: tile grid where the column count is derived from `tile_w`.
Equivalent to `cols = floor(available_width / tile_w)` then
`wlx_grid_begin_auto(ctx, cols, tile_h, ...)`.

| Parameter | Type | Description |
|-----------|------|-------------|
| `tile_w` | `float` | Desired tile width in pixels |
| `tile_h` | `float` | Desired tile height in pixels |

### `wlx_grid_cell`

```c
#define wlx_grid_cell(ctx, row, col, ...options)
```

Override the auto-advance cursor so the **next** child occupies a specific cell.

| Parameter | Type | Description |
|-----------|------|-------------|
| `row` | `int` | Target row index |
| `col` | `int` | Target column index |

**Options** (`WLX_Slot_Style_Opt`):

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `row_span` | `size_t` | `1` | Number of rows to span |
| `col_span` | `size_t` | `1` | Number of columns to span |
| `back_color` | `WLX_Color` | `{0}` | One-shot background fill for this cell only |
| `border_color` | `WLX_Color` | `{0}` | One-shot border color for this cell only |
| `border_width` | `float` | `0` | One-shot border thickness for this cell only |

### `wlx_grid_cell_style`

```c
#define wlx_grid_cell_style(ctx, ...options)
```

Set a one-shot style override for the **next** grid cell. Call immediately before
the widget that should receive the override. The override is consumed and reset
after that cell's slot rect is computed; subsequent cells revert to the uniform
decoration (or no decoration if none is set).

**Options** (`WLX_Slot_Style_Opt`): decoration fields only (`row_span`/`col_span` are ignored).

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `back_color` | `WLX_Color` | `{0}` | Background fill for this cell |
| `border_color` | `WLX_Color` | `{0}` | Border color for this cell |
| `border_width` | `float` | `0` | Border thickness in pixels |

### `wlx_slot_style`

```c
#define wlx_slot_style(ctx, ...options)
```

Set a one-shot style override for the **next** slot in the enclosing linear
layout. Call immediately before the widget that should receive the override.
The override is consumed and reset after that slot's rect is computed.

**Options** (`WLX_Slot_Style_Opt`): same fields as `wlx_grid_cell_style` (`row_span`/`col_span` are ignored).

### `wlx_grid_auto_row_px`

```c
void wlx_grid_auto_row_px(WLX_Context *ctx, float px);
```

Override the pixel height for the **next** row in the enclosing dynamic grid.
Call immediately before the first widget of the row whose height differs from
the grid's default `row_px`.

### `wlx_grid_end`

```c
#define wlx_grid_end(ctx) wlx_layout_end(ctx)
```

Alias for `wlx_layout_end(ctx)`.

---

## Slot Size Macros

Construct `WLX_Slot_Size` values for use in `sizes` / `row_sizes` / `col_sizes`
arrays.

### Basic variants

| Macro | Description |
|-------|-------------|
| `WLX_SLOT_AUTO` | Equal division of remaining space |
| `WLX_SLOT_PX(px)` | Fixed pixel size |
| `WLX_SLOT_PCT(pct)` | Percentage of parent (0–100) |
| `WLX_SLOT_FLEX(weight)` | Flex weight — proportional share of remaining space |
| `WLX_SLOT_FILL` | Fill entire viewport (scroll panel or layout rect) |
| `WLX_SLOT_CONTENT` | Measured from child content (frame-delayed) |

### Fill variants

| Macro | Description |
|-------|-------------|
| `WLX_SLOT_FILL` | Fill entire viewport (scroll panel or layout rect) |
| `WLX_SLOT_FILL_PCT(pct)` | Fill `pct`% of viewport |
| `WLX_SLOT_FILL_MIN(lo)` | Fill viewport with minimum |
| `WLX_SLOT_FILL_MAX(hi)` | Fill viewport with maximum |
| `WLX_SLOT_FILL_MINMAX(lo, hi)` | Fill viewport with both min and max |

### Content variants

| Macro | Description |
|-------|-------------|
| `WLX_SLOT_CONTENT` | Size measured from child content (frame-delayed) |
| `WLX_SLOT_CONTENT_MIN(lo)` | Content-sized with minimum |
| `WLX_SLOT_CONTENT_MAX(hi)` | Content-sized with maximum |
| `WLX_SLOT_CONTENT_MINMAX(lo, hi)` | Content-sized with both min and max |

### Constrained variants

| Macro | Description |
|-------|-------------|
| `WLX_SLOT_PX_MINMAX(px, lo, hi)` | Fixed pixels with min/max clamp |
| `WLX_SLOT_PCT_MINMAX(pct, lo, hi)` | Percentage with min/max clamp |
| `WLX_SLOT_FLEX_MIN(weight, lo)` | Flex with minimum |
| `WLX_SLOT_FLEX_MAX(weight, hi)` | Flex with maximum |
| `WLX_SLOT_FLEX_MINMAX(weight, lo, hi)` | Flex with both min and max |
| `WLX_SLOT_AUTO_MIN(lo)` | Auto with minimum |
| `WLX_SLOT_AUTO_MAX(hi)` | Auto with maximum |
| `WLX_SLOT_AUTO_MINMAX(lo, hi)` | Auto with both min and max |

**Example:**

```c
WLX_Slot_Size sizes[] = {
    WLX_SLOT_PX(200),        // sidebar: fixed 200px
    WLX_SLOT_FLEX(1),        // content: fill remaining space
    WLX_SLOT_PX(100),        // panel: fixed 100px
};
wlx_layout_begin(ctx, 3, WLX_HORZ, .sizes = sizes);
```

### `WLX_SIZES`

```c
#define WLX_SIZES(...)
```

Auto-counting slot sizes helper. Expands to **two** comma-separated values:
the element count and a `WLX_Slot_Size[]` compound literal. Designed to be
passed to `wlx_layout_begin_s()` so you never need to manually count slots:

```c
wlx_layout_begin_s(ctx, WLX_VERT,
    WLX_SIZES(WLX_SLOT_PX(44), WLX_SLOT_FLEX(1), WLX_SLOT_PX(24)),
    .padding = 4);
```

Adding or removing entries automatically updates the slot count — no manual
counting needed.

> **Note:** arguments are evaluated twice (once in `sizeof`, once in the
> literal). Only use with side-effect-free expressions — all `WLX_SLOT_*`
> macros are safe.

### `wlx_layout_begin_s`

```c
#define wlx_layout_begin_s(ctx, orient, count_val, sizes_ptr, ...)
```

Layout with auto-counted sizes. Equivalent to `wlx_layout_begin` but the
count is derived from `WLX_SIZES()` instead of being passed manually.
The `WLX_SIZES(...)` argument expands into `count_val` and `sizes_ptr`.
Trailing options (`.padding`, `.pos`, `.span`, etc.) are passed through.

```c
// These two are equivalent:
wlx_layout_begin(ctx, 2, WLX_HORZ,
    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(200), WLX_SLOT_FLEX(1) });

wlx_layout_begin_s(ctx, WLX_HORZ,
    WLX_SIZES(WLX_SLOT_PX(200), WLX_SLOT_FLEX(1)));
```

---

## Interaction & State

### Identity Model

Wollix uses one shared hash formula for all identity purposes:

```
id = hash(file, line) ^ id_stack_hash
```

Three conceptual roles map onto this formula:

| Role | What it identifies | How it is set |
|------|--------------------|---------------|
| **Widget ID** | A single immediate-mode call site | Derived automatically from `__FILE__`/`__LINE__` inside every widget macro |
| **State ID** | Key for `wlx_get_state()` persistent data | Same formula; call-site stability keeps data alive across frames |
| **Scope ID** | Container-level id that disambiguates all descendants | Set via `.id` on container option structs; pushed at begin, popped at end automatically |

**v0.x rule:** container `.id` acts as both Scope ID and State ID for the
container itself. A separate `.state_id` field is deferred until state and
scope need to diverge in practice.

**Repeated containers:** Without a `.id`, two instances of the same container
at the same call site share an id stack context, so descendant widgets from
identical source lines inside both instances will collide (same ID). Set `.id`
to a distinct string on each instance to isolate descendants.

```c
// Two inspector panels with isolated descendant state:
wlx_panel_begin(&ctx, .id = "inspector_A");
    wlx_button(&ctx, "Save");   // ID scoped under "inspector_A"
wlx_panel_end(&ctx);

wlx_panel_begin(&ctx, .id = "inspector_B");
    wlx_button(&ctx, "Save");   // different ID even from the same source line
wlx_panel_end(&ctx);
```

### `wlx_get_interaction`

```c
WLX_Interaction wlx_get_interaction(
    WLX_Context *ctx, WLX_Rect rect, uint32_t flags,
    const char *file, int line
);
```

Unified interaction handler. Returns a `WLX_Interaction` struct describing the
current hover/press/click/focus/drag state for the given rect.

Interaction IDs are derived from call-site `file`/`line` plus the ID stack
plus a frame-local sequential counter — unique even if the same source line is
reached multiple times.

| Parameter | Description |
|-----------|-------------|
| `ctx` | The UI context |
| `rect` | The widget's screen rect |
| `flags` | Bitwise OR of `WLX_Interact_Flags` |
| `file`, `line` | Call-site (passed automatically by widget macros) |

Typically not called directly — widgets call it internally.

### `wlx_get_state`

```c
#define wlx_get_state(ctx, type)
// Expands to: wlx_get_state_impl(ctx, sizeof(type), __FILE__, __LINE__)
```

Returns a `WLX_State` handle with a pointer to zero-initialized persistent data
of the given `type`. The data survives across frames.

State IDs use call-site `file`/`line` plus the ID stack (no frame-local
counter), so they are stable across frames. Use exactly **one** call per
widget instance per frame.

```c
typedef struct { float scroll_y; int page; } MyState;

WLX_State s = wlx_get_state(ctx, MyState);
MyState *state = (MyState *)s.data;
state->page = 1;  // persists until the program exits
```

### `wlx_push_id` / `wlx_pop_id`

```c
void wlx_push_id(WLX_Context *ctx, size_t id);
void wlx_pop_id(WLX_Context *ctx);
```

Low-level escape hatch that pushes an arbitrary integer onto the ID stack.
Prefer setting `.id` on container option structs (Scope ID) for
container-body disambiguation. Use `wlx_push_id`/`wlx_pop_id` directly only
for loop-level or reusable-function-level disambiguation that container `.id`
does not cover.

```c
for (int i = 0; i < count; i++) {
    wlx_push_id(ctx, i);
        if (wlx_button(ctx, items[i].name)) { /* ... */ }
    wlx_pop_id(ctx);
}
```

---

## Input Queries

### `wlx_is_key_down`

```c
bool wlx_is_key_down(WLX_Context *ctx, WLX_Key_Code key);
```

Returns `true` if the key is currently held down.

### `wlx_is_key_pressed`

```c
bool wlx_is_key_pressed(WLX_Context *ctx, WLX_Key_Code key);
```

Returns `true` for one frame when the key is first pressed.

### `wlx_point_in_rect`

```c
bool wlx_point_in_rect(int px, int py, int x, int y, int w, int h);
```

Returns `true` if point `(px, py)` is inside the rectangle `(x, y, w, h)`.

---

## Utility Functions

### `wlx_rect`

```c
WLX_Rect wlx_rect(float x, float y, float w, float h);
```

Construct a `WLX_Rect`.

### `wlx_rect_intersect`

```c
WLX_Rect wlx_rect_intersect(WLX_Rect a, WLX_Rect b);
```

Return the intersection of two rects. Result may have zero or negative
dimensions if they don't overlap.

### `wlx_get_align_rect`

```c
WLX_Rect wlx_get_align_rect(WLX_Rect parent_rect, int width, int height, WLX_Align align);
```

Compute a sub-rect of `parent_rect` positioned according to `align`. Used
internally for `widget_align`.

### `wlx_color_is_zero`

```c
static inline bool wlx_color_is_zero(WLX_Color c);
```

Returns `true` if all four color components are zero (the sentinel meaning
"use theme default").

### Low-Level Layout Constructors

These create `WLX_Layout` values without pushing them onto the stack. Rarely
needed — prefer the `wlx_layout_begin` / `wlx_grid_begin` macros.

```c
WLX_Layout wlx_create_layout(WLX_Context *ctx, WLX_Rect r, size_t count, WLX_Orient orient);
WLX_Layout wlx_create_layout_auto(WLX_Context *ctx, WLX_Rect r, WLX_Orient orient, float slot_px);
WLX_Layout wlx_create_grid(WLX_Context *ctx, WLX_Rect r, size_t rows, size_t cols,
                           const WLX_Slot_Size *row_sizes, const WLX_Slot_Size *col_sizes);
WLX_Layout wlx_create_grid_auto(WLX_Context *ctx, WLX_Rect r,
                                size_t cols, float row_px, const WLX_Slot_Size *col_sizes);
```

### `wlx_get_slot_rect`

```c
WLX_Rect wlx_get_slot_rect(WLX_Context *ctx, WLX_Layout *l, int pos, size_t span);
```

Compute the rect for slot `pos` (spanning `span` slots) in layout `l`.

### `wlx_get_parent_rect`

```c
WLX_Rect wlx_get_parent_rect(WLX_Context *ctx);
```

Return the full rect of the innermost layout on the stack. If no layout is
active, returns the root rect (`ctx->rect`). Does **not** advance the layout
cursor — this is a non-consuming peek.

Use this to size a child relative to its container without hardcoding pixel
values:

```c
float h = wlx_get_parent_rect(ctx).h;
wlx_layout_begin(ctx, 1, WLX_VERT,
    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(h) });
```

### `wlx_get_scroll_panel_viewport`

```c
WLX_Rect wlx_get_scroll_panel_viewport(WLX_Context *ctx);
```

Return the viewport rect of the innermost scroll panel. Returns `{0}` if no
scroll panel is active. Useful for sizing children to the visible area rather
than the (potentially larger) scrollable content area.

### `wlx_get_available_width` / `wlx_get_available_height`

```c
static inline float wlx_get_available_width(WLX_Context *ctx);
static inline float wlx_get_available_height(WLX_Context *ctx);
```

Convenience wrappers around `wlx_get_parent_rect()`. Return the width or
height of the innermost layout rect.

---

## Widget — `wlx_widget`

```c
#define wlx_widget(ctx, ...options)
// void — no return value
```

Low-level colored rectangle. No text, no interaction beyond hover. Use for
dividers, spacers, or color swatches.

**Option struct:** `WLX_Widget_Opt`

```c
wlx_widget(ctx,
    .widget_align = WLX_CENTER, .width = 100, .height = 4,
    .color = (WLX_Color){ 80, 80, 80, 255 }
);
```

---

## Widget — `wlx_label`

```c
#define wlx_label(ctx, text, ...options)
// void — no return value
```

Static text label. Renders text fitted within its slot rect. Optional filled
background when `show_background = true`.

**Option struct:** `WLX_Label_Opt`

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| *placement* | | | See [Shared Option Field Macros](#shared-option-field-macros) |
| *sizing* | | | See [Shared Option Field Macros](#shared-option-field-macros) |
| *typography* | | | `font`, `font_size`, `spacing`, `align`, `wrap` (default `true`) |
| *colors* | | | `front_color`, `back_color` |
| *border* | | | `border_color`, `border_width`, `roundness`, `rounded_segments` |
| `show_background` | `bool` | `false` | Draw filled background behind text |
| `id` | `const char *` | `NULL` | Explicit widget ID. `NULL` = auto from call-site |

---

## Widget — `wlx_button`

```c
#define wlx_button(ctx, text, ...options)
// Returns: bool — true on click
```

Clickable button. Always draws a filled `back_color` rectangle; hover
brightens it automatically. Returns `true` on the frame the click completes
(press then release while hovering) or on keyboard activation (Space/Enter
while hovered).

**Option struct:** `WLX_Button_Opt` — same fields as `WLX_Label_Opt` except
no `show_background` (button always draws its background).

---

## Widget — `wlx_checkbox`

```c
#define wlx_checkbox(ctx, text, checked, ...options)
// Returns: bool — true when toggled
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `text` | `const char *` | Label text |
| `checked` | `bool *` | Pointer to state — toggled on click |

**Option struct:** `WLX_Checkbox_Opt`

Supports native and texture-backed rendering. When `tex_checked.width > 0`,
the widget draws textures instead of the default box + check mark. Use
`wlx_checkbox(..., .tex_checked = ..., .tex_unchecked = ...)` instead of the
removed `wlx_checkbox_tex` compatibility macro.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| *shared fields* | | | placement, sizing, typography (default `wrap = false`), colors |
| `full_slot_hit` | `bool` | `true` | Use the full slot rect for hover/click interaction |
| `border_color` | `WLX_Color` | `{0}` | Indicator border. `{0}` = theme `checkbox.border` |
| `border_width` | `float` | `-1` | Indicator border width. `-1` = theme `checkbox.border_width` |
| `roundness` | `float` | `-1` | Indicator corner roundness. `-1` = theme default |
| `rounded_segments` | `int` | `-1` | Rounded-corner segment count. `-1` = theme default |
| `check_color` | `WLX_Color` | `{0}` | Check mark color. `{0}` = theme `checkbox.check` |
| `tex_checked` | `WLX_Texture` | zero handle | Checked texture. When `width > 0`, the widget uses texture mode |
| `tex_unchecked` | `WLX_Texture` | zero handle | Unchecked texture paired with `tex_checked` |
| `id` | `const char *` | `NULL` | Explicit widget ID. `NULL` = auto from call-site |

---

## Widget — `wlx_inputbox`

```c
#define wlx_inputbox(ctx, label, buffer, buffer_size, ...options)
// Returns: bool — true while focused
```

Single-line text input. Click to focus, type to edit, Enter/Escape/click-away
to unfocus.

| Parameter | Type | Description |
|-----------|------|-------------|
| `label` | `const char *` | Label drawn to the left |
| `buffer` | `char *` | Writable text buffer |
| `buffer_size` | `size_t` | Total buffer size (must be ≥ 2) |

**Option struct:** `WLX_Inputbox_Opt`

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| *shared fields* | | | placement, sizing, typography (default `wrap = true`), colors |
| `content_padding` | `float` | `10` | Horizontal padding inside the editing area |
| `border_color` | `WLX_Color` | `{0}` | Unfocused border. `{0}` = theme `border` |
| `border_width` | `float` | `-1` | Border width. `-1` = theme `input.border_width`, then theme `border_width` |
| `roundness` | `float` | `-1` | Corner roundness. `-1` = theme default |
| `rounded_segments` | `int` | `-1` | Rounded-corner segment count. `-1` = theme default |
| `border_focus_color` | `WLX_Color` | `{0}` | Focused border. `{0}` = theme `input.border_focus` |
| `cursor_color` | `WLX_Color` | `{0}` | Blinking cursor. `{0}` = theme `input.cursor` |
| `id` | `const char *` | `NULL` | Explicit widget ID. `NULL` = auto from call-site |

---

## Widget — `wlx_slider`

```c
#define wlx_slider(ctx, label, value, ...options)
// Returns: bool — true when value changes
```

Horizontal slider. Click/drag the thumb or click the track to jump.

| Parameter | Type | Description |
|-----------|------|-------------|
| `label` | `const char *` | Label to the left (can be `NULL`) |
| `value` | `float *` | Pointer to value — clamped to `[min_value, max_value]` |

**Option struct:** `WLX_Slider_Opt`

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| *placement* | | | `pos`, `span`, `overflow`, `padding` |
| *sizing* | | | `widget_align`, `width`, `height`, min/max, `opacity` |
| `font` | `WLX_Font` | `WLX_FONT_DEFAULT` | Font for label/value text |
| `font_size` | `int` | `0` | Font size |
| `spacing` | `int` | `0` | Character spacing |
| `align` | `WLX_Align` | `WLX_LEFT` | Label text alignment |
| `show_label` | `bool` | `true` | Show numeric value beside the track |
| `track_color` | `WLX_Color` | `{0}` | Track background. `{0}` = theme `slider.track` |
| `thumb_color` | `WLX_Color` | `{0}` | Thumb handle. `{0}` = theme `slider.thumb` |
| `label_color` | `WLX_Color` | `{0}` | Label text. `{0}` = theme `slider.label` |
| `track_height` | `float` | `0` | Track bar height. `0` = default (6) |
| `thumb_width` | `float` | `0` | Thumb handle width. `0` = default (14) |
| `border_color` | `WLX_Color` | `{0}` | Border color. `{0}` = theme `border` |
| `border_width` | `float` | `-1` | Border width. `-1` = theme `border_width` |
| `roundness` | `float` | `-1` | Corner rounding. `-1` = theme default |
| `rounded_segments` | `int` | `-1` | Segments for rounded drawing. `-1` = theme default |
| `hover_brightness` | `float` | `WLX_FLOAT_UNSET` | Track hover brightness shift. Uses `theme->hover_brightness` when unset |
| `thumb_hover_brightness` | `float` | `WLX_FLOAT_UNSET` | Thumb hover brightness shift. Uses `theme->hover_brightness` when unset |
| `fill_inactive_brightness` | `float` | `-0.3` | Filled-portion brightness offset |
| `min_value` | `float` | `0.0` | Minimum slider value |
| `max_value` | `float` | `1.0` | Maximum slider value |
| `id` | `const char *` | `NULL` | Explicit widget ID. `NULL` = auto from call-site |

---

## Widget — `wlx_separator`

```c
#define wlx_separator(ctx, ...options)
// void — no return value
```

Non-interactive divider. Draws a horizontal line when the resolved widget rect
is wider than tall, and a vertical line when it is taller than wide.

**Option struct:** `WLX_Separator_Opt`

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| *placement* | | | See [Shared Option Field Macros](#shared-option-field-macros) |
| *sizing* | | | See [Shared Option Field Macros](#shared-option-field-macros) |
| `color` | `WLX_Color` | `{0}` | Divider color. `{0}` = theme `border` |
| `thickness` | `float` | `1.0` | Divider thickness in pixels |
| `id` | `const char *` | `NULL` | Explicit widget ID. `NULL` = auto from call-site |

---

## Widget — `wlx_progress`

```c
#define wlx_progress(ctx, value, ...options)
// void — no return value
```

Non-interactive progress bar. `value` is clamped to the range `[0.0, 1.0]`
before drawing.

| Parameter | Type | Description |
|-----------|------|-------------|
| `value` | `float` | Fill ratio. Values below `0` clamp to empty; values above `1` clamp to full |

**Option struct:** `WLX_Progress_Opt`

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| *placement* | | | See [Shared Option Field Macros](#shared-option-field-macros) |
| *sizing* | | | See [Shared Option Field Macros](#shared-option-field-macros) |
| *border* | | | `border_color`, `border_width`, `roundness`, `rounded_segments` |
| `track_color` | `WLX_Color` | `{0}` | Track color. `{0}` = theme `progress.track`, falling back to `slider.track` |
| `fill_color` | `WLX_Color` | `{0}` | Fill color. `{0}` = theme `progress.fill`, falling back to `accent` |
| `track_height` | `float` | `0` | Inner bar height. `0` = theme default |
| `id` | `const char *` | `NULL` | Explicit widget ID. `NULL` = auto from call-site |

---

## Widget — `wlx_toggle`

```c
#define wlx_toggle(ctx, label, value, ...options)
// Returns: bool — true when toggled
```

Switch-style boolean control. Click or keyboard-activate it to flip `*value`.
The `label` may be `NULL`.

| Parameter | Type | Description |
|-----------|------|-------------|
| `label` | `const char *` | Label text shown beside the switch. May be `NULL` |
| `value` | `bool *` | Pointer to state — flipped when the toggle is activated |

**Option struct:** `WLX_Toggle_Opt`

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| *placement* | | | See [Shared Option Field Macros](#shared-option-field-macros) |
| *sizing* | | | See [Shared Option Field Macros](#shared-option-field-macros) |
| *typography* | | | `font`, `font_size`, `spacing`, `align`, `wrap` (default `false`) |
| *colors* | | | `front_color`, `back_color` (`back_color` is currently unused by this widget) |
| *border* | | | `border_color`, `border_width`, `roundness`, `rounded_segments` |
| `track_color` | `WLX_Color` | `{0}` | Inactive track color. `{0}` = theme `toggle.track`, falling back to `slider.track` |
| `track_active_color` | `WLX_Color` | `{0}` | Active track color. `{0}` = theme `toggle.track_active`, falling back to `accent` |
| `thumb_color` | `WLX_Color` | `{0}` | Thumb color. `{0}` = theme `toggle.thumb`, falling back to `foreground` |
| `hover_brightness` | `float` | `WLX_FLOAT_UNSET` | Hover brightness shift. Uses `theme->hover_brightness` when unset |
| `id` | `const char *` | `NULL` | Explicit widget ID. `NULL` = auto from call-site |

---

## Widget — `wlx_radio`

```c
#define wlx_radio(ctx, label, active, index, ...options)
// Returns: bool — true when selected on this frame
```

Select-one radio control. Activating the widget sets `*active = index`. The
`label` may be `NULL`.

| Parameter | Type | Description |
|-----------|------|-------------|
| `label` | `const char *` | Label text shown beside the ring. May be `NULL` |
| `active` | `int *` | Pointer to the currently selected index |
| `index` | `int` | Index represented by this radio button |

**Option struct:** `WLX_Radio_Opt`

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| *placement* | | | See [Shared Option Field Macros](#shared-option-field-macros) |
| *sizing* | | | See [Shared Option Field Macros](#shared-option-field-macros) |
| *typography* | | | `font`, `font_size`, `spacing`, `align`, `wrap` (default `false`) |
| *colors* | | | `front_color`, `back_color` (`back_color` is currently unused by this widget) |
| `ring_color` | `WLX_Color` | `{0}` | Ring color. `{0}` = theme `radio.ring`, falling back to `border` |
| `fill_color` | `WLX_Color` | `{0}` | Selected fill color. `{0}` = theme `radio.fill`, falling back to `accent` |
| `ring_border_width` | `float` | `-1` | Ring outline thickness. Uses theme radio/border defaults when unset |
| `hover_brightness` | `float` | `WLX_FLOAT_UNSET` | Hover brightness shift. Uses `theme->hover_brightness` when unset |
| `id` | `const char *` | `NULL` | Explicit widget ID. `NULL` = auto from call-site |

---

## Widget — `wlx_scroll_panel`

```c
#define wlx_scroll_panel_begin(ctx, content_height, ...options)
void wlx_scroll_panel_end(WLX_Context *ctx);
```

Scrollable container. Used as a begin/end pair with layout content between.

| Parameter | Type | Description |
|-----------|------|-------------|
| `content_height` | `float` | Total scrollable height in pixels. `-1` = auto (measured from children) |

**Option struct:** `WLX_Scroll_Panel_Opt`

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| *placement* | | | `pos`, `span`, `overflow`, `padding` |
| *sizing* | | | `widget_align`, `width`, `height`, min/max, `opacity` |
| `back_color` | `WLX_Color` | `{0}` | Panel background |
| `scrollbar_color` | `WLX_Color` | `{0}` | Scrollbar thumb color |
| *border* | | | `border_color`, `border_width`, `roundness`, `rounded_segments` |
| `scrollbar_hover_brightness` | `float` | `WLX_FLOAT_UNSET` | Scrollbar hover brightness shift. Uses `theme->hover_brightness` when unset |
| `scrollbar_width` | `float` | `-1` | Scrollbar width. `-1` = theme `scrollbar.width` |
| `wheel_scroll_speed` | `float` | `20.0` | Pixels per wheel tick |
| `show_scrollbar` | `bool` | `true` | Draw the scrollbar |
| `id` | `const char *` | `NULL` | Combined State + Scope ID. Keys the panel's own scroll state and scopes all descendant widget and state IDs for the full panel body. `NULL` = auto from call-site (no descendant scoping) |

---

## Compound Widget — `wlx_split`

```c
#define wlx_split_begin(ctx, ...options)
#define wlx_split_next(ctx, ...options)
void wlx_split_end(WLX_Context *ctx);
```

Two-pane split layout with independent scroll panels. Replaces the common
pattern of outer VERT wrapper + HORZ layout + two scroll panels (6 begin/end
pairs) with 3 calls. Each pane gets its own auto-height scroll panel;
the user creates their own inner layout inside each pane.

**Option struct:** `WLX_Split_Opt` (for `wlx_split_begin`)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `first_size` | `WLX_Slot_Size` | `WLX_SLOT_PX(280)` | Width/size of the first (left) pane |
| `second_size` | `WLX_Slot_Size` | `WLX_SLOT_FLEX(1)` | Width/size of the second (right) pane |
| `fill_size` | `WLX_Slot_Size` | `WLX_SLOT_FILL` | Outer wrapper slot size (e.g. `WLX_SLOT_FILL_MIN(400)`) |
| `padding` | `float` | `4` | Uniform padding for the inner HORZ split layout |
| `padding_top` | `float` | `-1` | Top padding override. Negative = inherit `padding` |
| `padding_right` | `float` | `-1` | Right padding override. Negative = inherit `padding` |
| `padding_bottom` | `float` | `-1` | Bottom padding override. Negative = inherit `padding` |
| `padding_left` | `float` | `-1` | Left padding override. Negative = inherit `padding` |
| `gap` | `float` | `0` | Space between the two pane slots |
| `first_back_color` | `WLX_Color` | `{0}` | First pane scroll panel background. `{0}` = theme default |
| `second_back_color` | `WLX_Color` | `{0}` | Second pane scroll panel background. `{0}` = theme default |
| `id` | `const char *` | `NULL` | Scope ID: scopes all descendant widget and state IDs for the full split body (both panes). `NULL` = no scoping |

**Option struct:** `WLX_Split_Next_Opt` (for `wlx_split_next`)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `back_color` | `WLX_Color` | `{0}` | Override second pane background color |

**Internal structure created by split:**

1. Outer VERT layout (1 slot, `fill_size`)
2. Inner HORZ layout (2 slots: `first_size`, `second_size`, with padding and `gap`)
3. First pane: auto-height scroll panel (`content_height = -1`)
4. *(user code for first pane)*
5. `wlx_split_next` closes first scroll panel, opens second
6. *(user code for second pane)*
7. `wlx_split_end` closes second scroll panel + both layouts

**Debug assertions:** `split_depth` counter in `WLX_Context` (under `WLX_DEBUG`)
catches mismatched begin/next/end calls.

---

## Compound Widget — `wlx_panel`

```c
#define wlx_panel_begin(ctx, ...options)
void wlx_panel_end(WLX_Context *ctx);
```

Capacity-based CONTENT layout with optional heading label. Replaces the
common pattern of `wlx_layout_begin_s` with N manually-counted
`WLX_SLOT_CONTENT` entries plus a heading label. The panel pre-allocates a
fixed number of CONTENT slots — unused slots measure 0px and have no visual
impact. Adding or removing child widgets requires no slot-count updates.

**Option struct:** `WLX_Panel_Opt`

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `title` | `const char *` | `NULL` | Heading text. `NULL` = no heading (full capacity available for children) |
| `title_font_size` | `int` | `18` | Heading font size |
| `title_height` | `float` | `32` | Heading slot height in pixels |
| `title_align` | `WLX_Align` | `WLX_CENTER` | Heading text alignment |
| `title_back_color` | `WLX_Color` | `{0}` | Heading background color. `{0}` = theme surface |
| `padding` | `float` | `2` | Inner layout padding |
| `padding_top` | `float` | `-1` | Top padding override. Negative = inherit `padding` |
| `padding_right` | `float` | `-1` | Right padding override. Negative = inherit `padding` |
| `padding_bottom` | `float` | `-1` | Bottom padding override. Negative = inherit `padding` |
| `padding_left` | `float` | `-1` | Left padding override. Negative = inherit `padding` |
| `gap` | `float` | `0` | Gap between generated CONTENT slots |
| `capacity` | `int` | `32` | Maximum number of child widgets (excluding title). Clamped to `WLX_CONTENT_SLOTS_MAX` (64) |
| `id` | `const char *` | `NULL` | Scope ID: scopes all descendant widget and state IDs for the full panel body. `NULL` = no scoping |

**Internal structure:**

1. VERT layout with `capacity` (+ 1 if title) CONTENT slots
2. Optional heading label in the first slot
3. `wlx_panel_end` closes the layout

The panel does **not** create a scroll panel. Inside a split pane, the scroll
panel is already provided by `wlx_split_begin`. For standalone scrollable
panels, wrap in `wlx_scroll_panel_begin` / `wlx_scroll_panel_end`.

---

## Shared Option Field Macros

These X-macros inject fields into widget option structs. You never use them
directly — they define the fields available in widget `...options`.

### `WLX_LAYOUT_SLOT_FIELDS` / `WLX_LAYOUT_SLOT_DEFAULTS`

Injected fields: `pos`, `span`, `overflow`, `padding`.

### `WLX_WIDGET_SIZING_FIELDS` / `WLX_WIDGET_SIZING_DEFAULTS`

Injected fields: `widget_align`, `width`, `height`, `min_width`, `min_height`,
`max_width`, `max_height`, `opacity`.

### `WLX_TEXT_TYPOGRAPHY_FIELDS` / `WLX_TEXT_TYPOGRAPHY_DEFAULTS`

Injected fields: `font`, `font_size`, `spacing`, `align`, `wrap`.

### `WLX_TEXT_COLOR_FIELDS` / `WLX_TEXT_COLOR_DEFAULTS`

Injected fields: `front_color`, `back_color`.

### `WLX_BORDER_FIELDS` / `WLX_BORDER_DEFAULTS`

Injected fields: `border_color`, `border_width`, `roundness`,
`rounded_segments`.

---

## Theme Presets

```c
extern const WLX_Theme wlx_theme_dark;
extern const WLX_Theme wlx_theme_light;
```

Two built-in theme presets. Set before `wlx_begin()`:

```c
ctx->theme = &wlx_theme_dark;   // default if theme is NULL
ctx->theme = &wlx_theme_light;
```

---

## Backend — Raylib

**Header:** `wollix_raylib.h`  
**Prerequisite:** `#include <raylib.h>` before `#include "wollix.h"`

### `wlx_context_init_raylib`

```c
static inline void wlx_context_init_raylib(WLX_Context *ctx);
```

Set all `ctx->backend` function pointers to Raylib implementations.

### `wlx_process_raylib_input`

```c
static inline void wlx_process_raylib_input(WLX_Context *ctx);
```

`WLX_Input_Handler` callback. Reads mouse, keyboard, and mouse wheel from
Raylib and populates `ctx->input`. Includes encoder-bounce debouncing for the
mouse wheel.

### `wlx_backend_raylib`

```c
static inline WLX_Backend wlx_backend_raylib(void);
```

Returns a `WLX_Backend` struct with all Raylib function pointers. Called
internally by `wlx_context_init_raylib`.

### `wlx_font_from_raylib`

```c
static inline WLX_Font wlx_font_from_raylib(Font *font);
```

Convert a pointer to a Raylib `Font` into a `WLX_Font` handle. Pass to widget
options or theme fields:

```c
Font my_font = LoadFontEx("myfont.ttf", 24, NULL, 0);
ctx->theme_copy.font = wlx_font_from_raylib(&my_font);
```

### Raylib Setup Pattern

```c
#include <raylib.h>

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_raylib.h"

int main(void) {
    InitWindow(800, 600, "My App");
    SetTargetFPS(60);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    while (!WindowShouldClose()) {
        float w = GetRenderWidth(), h = GetRenderHeight();
        wlx_begin(ctx, (WLX_Rect){0, 0, w, h}, wlx_process_raylib_input);
            BeginDrawing();
                ClearBackground((Color){18, 18, 18, 255});
                // ... widgets ...
            wlx_end(ctx);
            EndDrawing();
    }

    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
}
```

---

## Backend — SDL3

**Header:** `wollix_sdl3.h`  
**Prerequisite:** `#include <SDL3/SDL.h>` before `#include "wollix.h"`

### `wlx_context_init_sdl3`

```c
static inline void wlx_context_init_sdl3(WLX_Context *ctx, SDL_Window *window, SDL_Renderer *renderer);
```

Set all `ctx->backend` function pointers to SDL3 implementations. Installs an
SDL event watch for mouse wheel and text input events. Calls
`SDL_StartTextInput(window)`.

### `wlx_process_sdl3_input`

```c
static inline void wlx_process_sdl3_input(WLX_Context *ctx);
```

`WLX_Input_Handler` callback for SDL3. Calls `SDL_PumpEvents()` and reads
mouse/keyboard state.

### `wlx_backend_sdl3`

```c
static inline WLX_Backend wlx_backend_sdl3(SDL_Renderer *renderer);
```

Returns a `WLX_Backend` struct with all SDL3 function pointers.

### `wlx_texture_from_sdl3`

```c
static inline WLX_Texture wlx_texture_from_sdl3(SDL_Texture *texture, int width, int height);
```

Convert an SDL3 texture to a `WLX_Texture` handle.

### SDL3 Setup Pattern

```c
#include <SDL3/SDL.h>

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_sdl3.h"

int main(void) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("My App", 800, 600, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren = SDL_CreateRenderer(win, NULL);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_sdl3(ctx, win, ren);

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
        }
        int w, h;
        SDL_GetWindowSize(win, &w, &h);
        wlx_begin(ctx, (WLX_Rect){0, 0, w, h}, wlx_process_sdl3_input);
            SDL_SetRenderDrawColor(ren, 18, 18, 18, 255);
            SDL_RenderClear(ren);
            // ... widgets ...
        wlx_end(ctx);
            SDL_RenderPresent(ren);
    }

    wlx_context_destroy(ctx);
    free(ctx);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}
```
