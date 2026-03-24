# API Reference

Complete reference for every public function, struct, enum, and macro in the
layout library.

> **Header files:**
> - `wollix.h` — core library (include with `#define WOLLIX_IMPLEMENTATION` in exactly one translation unit)
> - `wollix_raylib.h` — Raylib backend adapter
> - `wollix_sdl3.h` — SDL3 backend adapter

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
22. [Widget — `wlx_checkbox_tex`](#widget--wlx_checkbox_tex)
23. [Widget — `wlx_inputbox`](#widget--wlx_inputbox)
24. [Widget — `wlx_slider`](#widget--wlx_slider)
25. [Widget — `wlx_scroll_panel`](#widget--wlx_scroll_panel)
26. [Shared Option Field Macros](#shared-option-field-macros)
27. [Theme Presets](#theme-presets)
28. [Backend — Raylib](#backend--raylib)
29. [Backend — SDL3](#backend--sdl3)

---

## Feature Flags

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
| `draw_rect_lines` | Draw a rectangle outline with given thickness |
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

### Frame Loop Pattern

```c
while (!WindowShouldClose()) {
    wlx_begin(ctx, (WLX_Rect){0, 0, w, h}, wlx_process_raylib_input);
        BeginDrawing();
            ClearBackground(WLX_BACKGROUND_COLOR);
            // ... wlx_layout_begin / widgets / wlx_layout_end ...
        EndDrawing();
    wlx_end(ctx);
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

### `wlx_layout_begin_auto`

```c
#define wlx_layout_begin_auto(ctx, orient, slot_px, ...options)
```

Push a dynamic layout whose slot count grows as children are added.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ctx` | `WLX_Context *` | The UI context |
| `orient` | `WLX_Orient` | `WLX_HORZ` or `WLX_VERT` |
| `slot_px` | `float` | Fixed pixel size per slot. `0` = variable (use `wlx_layout_auto_slot_px()`) |

### `wlx_layout_auto_slot_px`

```c
void wlx_layout_auto_slot_px(WLX_Context *ctx, float px);
```

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
| `row_sizes` | `const WLX_Slot_Size *` | `NULL` | Array of `rows` row sizes. `NULL` = equal division |
| `col_sizes` | `const WLX_Slot_Size *` | `NULL` | Array of `cols` column sizes. `NULL` = equal division |

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
| `col_sizes` | `const WLX_Slot_Size *` | `NULL` | Array of `cols` column sizes |

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

**Options** (`WLX_Grid_Cell_Opt`):

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `row_span` | `size_t` | `1` | Number of rows to span |
| `col_span` | `size_t` | `1` | Number of columns to span |

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
| `WLX_SLOT_PCT(pct)` | Percentage of parent (0.0–1.0) |
| `WLX_SLOT_FLEX(weight)` | Flex weight — proportional share of remaining space |

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

---

## Interaction & State

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

Push/pop an extra ID value onto the ID stack. Use inside loops to give each
iteration a unique stable ID for interaction and persistent state.

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
| `show_background` | `bool` | `false` | Draw filled background behind text |

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

**Option struct:** `WLX_Button_Opt` — same fields as `WLX_Label_Opt` except\nno `show_background` (button always draws its background).

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

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| *shared fields* | | | placement, sizing, typography (default `wrap = false`), colors |
| `show_background` | `bool` | `false` | Draw filled background |
| `full_slot_hit` | `bool` | `true` | Use the full slot rect for hover/click interaction |
| `border_color` | `WLX_Color` | `{0}` | Indicator border. `{0}` = theme `checkbox.border` |
| `check_color` | `WLX_Color` | `{0}` | Check mark color. `{0}` = theme `checkbox.check` |

---

## Widget — `wlx_checkbox_tex`

```c
#define wlx_checkbox_tex(ctx, text, checked, ...options)
// Returns: bool — true when toggled
```

Same as `wlx_checkbox` but renders custom textures instead of the drawn indicator.

**Option struct:** `WLX_Checkbox_Tex_Opt`

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| *shared fields* | | | placement, sizing, typography, colors |
| `show_background` | `bool` | `false` | Draw filled background |
| `full_slot_hit` | `bool` | `true` | Use the full slot rect for hover/click interaction |
| `tex_checked` | `WLX_Texture` | — | Texture when checked |
| `tex_unchecked` | `WLX_Texture` | — | Texture when unchecked |

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
| `border_focus_color` | `WLX_Color` | `{0}` | Focused border. `{0}` = theme `input.border_focus` |
| `cursor_color` | `WLX_Color` | `{0}` | Blinking cursor. `{0}` = theme `input.cursor` |

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
| `roundness` | `float` | `0` | Corner rounding |
| `rounded_segments` | `int` | `0` | Segments for rounded drawing |
| `hover_brightness` | `float` | `0` | Track hover brightness shift |
| `thumb_hover_brightness` | `float` | `0` | Thumb hover brightness shift |
| `fill_inactive_brightness` | `float` | `-0.3` | Filled-portion brightness offset |
| `min_value` | `float` | `0.0` | Minimum slider value |
| `max_value` | `float` | `1.0` | Maximum slider value |

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
| `scrollbar_hover_brightness` | `float` | `0` | Scrollbar hover brightness shift |
| `scrollbar_width` | `float` | `0` | Scrollbar width. `0` = default (10) |
| `wheel_scroll_speed` | `float` | `20.0` | Pixels per wheel tick |
| `show_scrollbar` | `bool` | `true` | Draw the scrollbar |

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
            EndDrawing();
        wlx_end(ctx);
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
            SDL_RenderPresent(ren);
        wlx_end(ctx);
    }

    wlx_context_destroy(ctx);
    free(ctx);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}
```
