# Layout Model

Conceptual guide to `wollix.h` — everything a user needs to know before
writing code.

---

## Table of Contents

1. [Include Pattern](#1-include-pattern)
2. [Frame Loop](#2-frame-loop)
3. [Layout Stack](#3-layout-stack)
4. [Slot Sizing](#4-slot-sizing)
5. [Grid Layouts](#5-grid-layouts)
6. [Alignment](#6-alignment)
7. [Widgets & Option Structs](#7-widgets--option-structs)
8. [Interaction](#8-interaction)
9. [IDs & Persistent State](#9-ids--persistent-state)
10. [Theming](#10-theming)
11. [Backend Contract](#11-backend-contract)

---

## 1. Include Pattern

`wollix.h` is a single-header library. In **exactly one** `.c` file, define
`WOLLIX_IMPLEMENTATION` before including it:

```c
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
```

In all other files, include without the define to get declarations only:

```c
#include "wollix.h"
```

Then include the backend adapter after `wollix.h`:

```c
#include "wollix_raylib.h"   // or wollix_sdl3.h
```

---

## 2. Frame Loop

Every frame follows the same structure:

```c
wlx_begin(&ctx, root_rect, input_handler);
    // ... layouts, widgets, drawing ...
wlx_end(&ctx);
```

**`wlx_begin`** resets per-frame state (hot widget, ID counter, scratch buffers),
calls the input handler to populate `ctx.input`, and sets the root rect.

**`wlx_end`** closes the frame (currently a no-op, reserved for future cleanup).

The **input handler** is a callback that reads platform input and writes it to
`ctx->input`. Backend adapters provide one (e.g. `wlx_process_raylib_input`).

### Context Lifetime

```c
WLX_Context ctx = {0};              // zero-init once
wlx_context_init_raylib(&ctx);      // attach backend
ctx.theme = &wlx_theme_dark;        // optional (dark is the default)
```

`WLX_Context` owns dynamically-growing buffers (layout stack, state pool, scratch
offsets). Zero-init it once, then reuse across frames. The context is not
thread-safe — use one per thread if needed.

---

## 3. Layout Stack

Layouts divide a rectangle into **slots**. Widgets and nested layouts consume
slots sequentially.

```
┌──────────────────────────┐
│  wlx_layout_begin(ctx, 3, WLX_VERT)
│  ┌──────────────────────┐│
│  │      slot 0          ││  ← first widget or nested layout
│  ├──────────────────────┤│
│  │      slot 1          ││  ← second widget
│  ├──────────────────────┤│
│  │      slot 2          ││  ← third widget
│  └──────────────────────┘│
│  wlx_layout_end(ctx)
└──────────────────────────┘
```

### Linear Layouts

```c
wlx_layout_begin(ctx, count, orient);
    // ... count children ...
wlx_layout_end(ctx);
```

- `count` — number of slots.
- `orient` — `WLX_VERT` (stack top to bottom) or `WLX_HORZ` (left to right).
- By default, all slots get equal size.

### Nesting

Layouts nest by pushing onto an internal stack. A nested layout consumes one
slot from its parent:

```c
wlx_layout_begin(ctx, 2, WLX_HORZ);          // parent: 2 columns

    wlx_layout_begin(ctx, 3, WLX_VERT);      // left column: 3 rows
        wlx_button(ctx, "A");
        wlx_button(ctx, "B");
        wlx_button(ctx, "C");
    wlx_layout_end(ctx);

    wlx_layout_begin(ctx, 2, WLX_VERT);      // right column: 2 rows
        wlx_button(ctx, "D");
        wlx_button(ctx, "E");
    wlx_layout_end(ctx);

wlx_layout_end(ctx);
```

### Dynamic (Auto) Layouts

When the number of children is unknown at layout creation time, use a dynamic
layout. Slots are created on-demand as children are added:

```c
wlx_layout_begin_auto(ctx, WLX_VERT, 40);    // each slot is 40px tall
    for (int i = 0; i < n; i++)
    wlx_button(ctx, labels[i]);
wlx_layout_end(ctx);
```

- `slot_px > 0` — fixed pixel size per slot.
- `slot_px = 0` — variable mode: call `wlx_layout_auto_slot_px(ctx, px)` before
  each child to set its size.

### Slot Placement Options

Every layout and widget macro accepts optional named arguments:

| Field     | Default | Description |
|-----------|---------|-------------|
| `pos`     | `-1`    | Target slot index (`-1` = next sequential) |
| `span`    | `1`     | Number of consecutive slots to occupy |
| `overflow`| `false` | Allow widget to exceed slot bounds |
| `padding` | `0`     | Uniform inset applied to the slot rect |

```c
wlx_layout_begin(ctx, 4, WLX_VERT, .padding = 10);
    wlx_button(ctx, "Normal");
    wlx_button(ctx, "Wide", .span = 2);       // occupies slots 1–2
    wlx_button(ctx, "At end", .pos = 3);      // jump to slot 3
wlx_layout_end(ctx);
```

---

## 4. Slot Sizing

By default, slots share space equally. Pass a `sizes` array to control
individual slot dimensions:

```c
WLX_Slot_Size sizes[] = {
    WLX_SLOT_PX(200),       // 200 pixels
    WLX_SLOT_PCT(30),       // 30% of parent
    WLX_SLOT_FLEX(1),       // remaining space, weight 1
    WLX_SLOT_FLEX(2),       // remaining space, weight 2
};
wlx_layout_begin(ctx, 4, WLX_HORZ, .sizes = sizes);
```

### Size Modes

| Macro | Kind | Description |
|-------|------|-------------|
| `WLX_SLOT_AUTO` | `WLX_SIZE_AUTO` | Equal share of remaining space (weight 1) |
| `WLX_SLOT_PX(px)` | `WLX_SIZE_PIXELS` | Fixed pixel size |
| `WLX_SLOT_PCT(pct)` | `WLX_SIZE_PERCENT` | Percentage of parent size |
| `WLX_SLOT_FLEX(w)` | `WLX_SIZE_FLEX` | Weighted share of remaining space |

**Resolution order:** pixels and percents are subtracted from the total first.
The remaining space is distributed among flex and auto slots proportionally to
their weights (auto = weight 1).

### Min/Max Constraints

Any size mode can have min and/or max constraints:

```c
WLX_SLOT_FLEX_MIN(1, 100)         // flex weight 1, at least 100px
WLX_SLOT_FLEX_MAX(1, 300)         // flex weight 1, at most 300px
WLX_SLOT_FLEX_MINMAX(1, 100, 300) // flex weight 1, clamped to [100, 300]
WLX_SLOT_PX_MINMAX(200, 150, 250) // 200px, clamped to [150, 250]
WLX_SLOT_PCT_MINMAX(50, 100, 400) // 50%, clamped to [100, 400]
WLX_SLOT_AUTO_MIN(80)             // auto, at least 80px
WLX_SLOT_AUTO_MAX(200)            // auto, at most 200px
```

A `0` for min or max means unconstrained.

### Advanced: Redistribute Mode

By default, clamping is single-pass — surplus or deficit from clamped slots is
not redistributed. Define `WLX_SLOT_MINMAX_REDISTRIBUTE` before including
`wollix.h` to enable iterative freeze-and-redistribute, which ensures
`offsets[count] == total` with no gap.

---

## 5. Grid Layouts

Grids arrange children in a 2D rows × columns matrix.

### Fixed Grid

```c
wlx_grid_begin(ctx, 3, 4);              // 3 rows, 4 columns
    for (int i = 0; i < 12; i++)
    wlx_button(ctx, labels[i]);      // auto-advances left→right, top→bottom
wlx_grid_end(ctx);
```

Children fill cells left-to-right, wrapping to the next row automatically.

### Grid with Explicit Sizes

```c
WLX_Slot_Size row_sizes[] = { WLX_SLOT_PX(50), WLX_SLOT_FLEX(1), WLX_SLOT_PX(30) };
WLX_Slot_Size col_sizes[] = { WLX_SLOT_PX(200), WLX_SLOT_FLEX(1) };
wlx_grid_begin(ctx, 3, 2, .row_sizes = row_sizes, .col_sizes = col_sizes);
    // ...
wlx_grid_end(ctx);
```

### Explicit Cell Placement

Use `wlx_grid_cell()` to place a child at a specific row/column with optional spans:

```c
wlx_grid_begin(ctx, 3, 3);
    wlx_grid_cell(ctx, 0, 0, .col_span = 2);    // row 0, cols 0–1
    wlx_textbox(ctx, "Wide header");

    wlx_grid_cell(ctx, 1, 2, .row_span = 2);    // rows 1–2, col 2
    wlx_textbox(ctx, "Tall sidebar");
wlx_grid_end(ctx);
```

### Auto-Growing Grid

When the number of rows is unknown:

```c
wlx_grid_begin_auto(ctx, 3, 40);        // 3 columns, 40px per row
    for (int i = 0; i < n; i++)
    wlx_button(ctx, items[i]);       // rows grow as needed
wlx_grid_end(ctx);
```

Use `wlx_grid_auto_row_px(ctx, px)` to override the height of the next row.

### Tile Grid

Automatically compute column count from tile dimensions:

```c
wlx_grid_begin_auto_tile(ctx, 120, 120);    // 120×120 tiles
    for (int i = 0; i < n; i++)
    wlx_widget(ctx, ...);
wlx_grid_end(ctx);
```

---

## 6. Alignment

`WLX_Align` controls where a widget is positioned within its slot when the
widget is smaller than the slot:

```
WLX_TOP_LEFT      WLX_TOP_CENTER      WLX_TOP_RIGHT
WLX_LEFT          WLX_CENTER          WLX_RIGHT
WLX_BOTTOM_LEFT   WLX_BOTTOM_CENTER   WLX_BOTTOM_RIGHT
WLX_TOP           (= WLX_TOP_CENTER)
WLX_BOTTOM        (= WLX_BOTTOM_CENTER)
WLX_LEFT          (= vertically centered, left)
WLX_RIGHT         (= vertically centered, right)
WLX_ALIGN_NONE    (= top-left, no centering)
```

Used in two places:

1. **Widget alignment** — `widget_align` in option structs positions the widget
   rect within its slot.
2. **Text alignment** — `align` in text-based widgets aligns text within the
   widget rect.

```c
wlx_button(ctx, "Centered", .widget_align = WLX_CENTER, .align = WLX_CENTER);
```

If the widget is larger than its slot, it is clamped to the slot bounds
(unless `overflow = true`).

---

## 7. Widgets & Option Structs

Every widget follows the same pattern:

```c
return_value = widget_name(ctx, required_args, .option = value, ...);
```

The trailing named arguments are **optional overrides** via C99 designated
initializers. Omitted fields use sensible defaults (from the theme or
hard-coded fallbacks).

### How It Works Internally

Each widget has three parts:

1. **Option struct** — e.g. `WLX_Button_Opt`, with fields for placement, sizing,
   typography, and styling.
2. **Default macro** — e.g. `wlx_default_button_opt(...)`, fills defaults and
   applies your overrides.
3. **User macro** — e.g. `wlx_button(ctx, text, ...)`, calls the `_impl` function
   with the default-initialized option struct.

### Shared Fields

All widgets inherit these field groups:

**Placement** (from `WLX_LAYOUT_SLOT_FIELDS`):

| Field | Default | Description |
|-------|---------|-------------|
| `pos` | `-1` | Slot index |
| `span` | `1` | Slot span |
| `overflow` | `false` | Allow exceeding slot bounds |
| `padding` | `0` | Slot inset |

**Sizing** (from `WLX_WIDGET_SIZING_FIELDS`):

| Field | Default | Description |
|-------|---------|-------------|
| `widget_align` | `WLX_LEFT` | Position within slot |
| `width` | `-1` | Widget width (`-1` = fill slot) |
| `height` | `-1` | Widget height (`-1` = fill slot) |
| `min_width` | `0` | Minimum width (`0` = none) |
| `min_height` | `0` | Minimum height (`0` = none) |
| `max_width` | `0` | Maximum width (`0` = none) |
| `max_height` | `0` | Maximum height (`0` = none) |
| `opacity` | `0` | Opacity (`0` = fully opaque sentinel) |

**Typography** (text widgets only, from `WLX_TEXT_TYPOGRAPHY_FIELDS`):

| Field | Default | Description |
|-------|---------|-------------|
| `font` | `WLX_FONT_DEFAULT` | Font handle |
| `font_size` | `0` | Font size (`0` = use theme) |
| `spacing` | `0` | Text spacing (`0` = use theme) |
| `align` | `WLX_LEFT` | Text alignment within widget |
| `wrap` | varies | Word-wrap long text |

**Colors** (from `WLX_TEXT_COLOR_FIELDS`):

| Field | Default | Description |
|-------|---------|-------------|
| `front_color` | `{0}` | Text color (`{0}` = use theme foreground) |
| `back_color` | `{0}` | Background color (`{0}` = use theme surface) |

### Widget Quick Reference

**`wlx_textbox`** — static text display:
```c
wlx_textbox(ctx, "Hello", .font_size = 20, .align = WLX_CENTER, .wrap = true);
```

**`wlx_button`** — clickable, returns `true` on click:
```c
if (wlx_button(ctx, "OK", .height = 40, .back_color = WLX_RGBA(0, 120, 200, 255)))
    do_something();
```

**`wlx_checkbox`** — toggle, returns `true` when state changes:
```c
bool enabled = false;
if (wlx_checkbox(ctx, "Enable", &enabled))
    printf("toggled: %d\n", enabled);
```

**`wlx_checkbox_tex`** — checkbox with custom textures:
```c
wlx_checkbox_tex(ctx, "Mute", &muted,
    .tex_checked = checked_tex, .tex_unchecked = unchecked_tex);
```

**`wlx_inputbox`** — text input field, returns `true` while focused:
```c
char name[128] = {0};
if (wlx_inputbox(ctx, "Name:", name, sizeof(name), .height = 40))
    printf("editing: %s\n", name);
```

**`wlx_slider`** — drag to set a float value, returns `true` while interacting:
```c
float vol = 0.5f;
wlx_slider(ctx, "Volume", &vol, .min_value = 0, .max_value = 1);
```

**`wlx_scroll_panel`** — scrollable container (begin/end pair):
```c
wlx_scroll_panel_begin(ctx, content_height);
    // ... child widgets ...
wlx_scroll_panel_end(ctx);
```

Pass `content_height = 0` for auto-height mode, where the library measures
content height from the first frame and applies it on subsequent frames.

---

## 8. Interaction

Widgets use `wlx_get_interaction()` internally to handle mouse and keyboard
input. You typically don't call it directly — widgets do it for you. But
understanding the model helps when building custom widgets.

### Interaction Modes

Modes are flags combined with bitwise OR. Use **one** primary mode (CLICK,
FOCUS, or DRAG) optionally combined with HOVER and KEYBOARD:

| Flag | Behavior |
|------|----------|
| `WLX_INTERACT_HOVER` | Track whether the mouse is over the widget rect |
| `WLX_INTERACT_CLICK` | Button-like: activate on press, fire `clicked` on release while hovering |
| `WLX_INTERACT_FOCUS` | Input-like: click to focus, click away or press Enter to unfocus |
| `WLX_INTERACT_DRAG` | Slider-like: activate on mouse-down, stay active while held |
| `WLX_INTERACT_KEYBOARD` | Space/Enter while hot triggers `clicked` |

### Result Fields

`WLX_Interaction` has these boolean fields:

| Field | Meaning |
|-------|---------|
| `hover` | Mouse is currently over the widget |
| `pressed` | Mouse is down on this widget right now |
| `clicked` | Click completed (released while hovering) or keyboard-activated |
| `focused` | Widget has focus (FOCUS mode) |
| `active` | Widget is the active widget (being pressed, dragged, or focused) |
| `just_focused` | Became focused this frame |
| `just_unfocused` | Lost focus this frame |

### Hot / Active Model

The library tracks two global widget slots per frame:

- **`hot_id`** — the widget the mouse is hovering over (reset each frame).
- **`active_id`** — the widget currently being pressed, dragged, or focused
  (persists across frames until released).

Only one widget can be hot and one can be active at a time. This prevents
overlapping interactions.

---

## 9. IDs & Persistent State

### How IDs Work

Every widget gets a unique ID derived from:

1. **Call-site** — `__FILE__` and `__LINE__` of the macro expansion.
2. **ID stack** — values pushed via `wlx_push_id()`.
3. **Frame sequence** — a per-frame counter (interaction IDs only).

This means two calls to `wlx_button()` on different source lines automatically
get different IDs with no user effort.

### Loop Disambiguation with `wlx_push_id`

When widgets are created in a loop, every iteration hits the same source line.
Use `wlx_push_id` / `wlx_pop_id` to differentiate them:

```c
for (int i = 0; i < count; i++) {
    wlx_push_id(ctx, i);
        if (wlx_button(ctx, names[i]))
            printf("clicked %d\n", i);
    wlx_pop_id(ctx);
}
```

Without the push/pop, all buttons would share the same interaction ID and
behave incorrectly.

### Persistent State

Some widgets need state that survives across frames (e.g. scroll offset, cursor
position). Use `wlx_get_state()`:

```c
typedef struct { float scroll_y; } MyState;
WLX_State handle = wlx_get_state(ctx, MyState);
MyState *state = (MyState *)handle.data;
// state->scroll_y persists across frames
```

Persistent state IDs use `file/line + ID stack` **without** the frame sequence
counter, so they are stable across frames. Use `wlx_push_id` when the same state
call is reached multiple times.

Built-in widgets that use persistent state: `wlx_inputbox` (cursor position),
`wlx_scroll_panel` (scroll offset, drag state), `wlx_slider` (drag state).

---

## 10. Theming

### Theme Struct

A `WLX_Theme` provides default colors, fonts, spacing, and widget-specific
overrides for the entire UI. Set it on the context:

```c
ctx.theme = &wlx_theme_dark;     // built-in dark theme (default)
ctx.theme = &wlx_theme_light;    // built-in light theme
```

Or create a custom theme:

```c
WLX_Theme my_theme = wlx_theme_dark;              // start from a preset
my_theme.background = WLX_RGBA(30, 30, 50, 255);
my_theme.accent     = WLX_RGBA(0, 200, 100, 255);
my_theme.font_size  = 18;
ctx.theme = &my_theme;
```

### Theme Fields

| Field | Description |
|-------|-------------|
| `background` | Window/panel clear color |
| `foreground` | Default text color |
| `surface` | Widget background color |
| `border` | Default border color |
| `accent` | Active/focused accent color |
| `font` | Default font handle |
| `font_size` | Default font size |
| `spacing` | Default text spacing |
| `padding` | Default slot padding |
| `roundness` | Corner radius |
| `hover_brightness` | Brightness shift on hover |
| `opacity` | Global opacity multiplier |

Widget-specific overrides exist for `input` (border focus, cursor color),
`wlx_slider` (track, thumb, label), `wlx_checkbox` (check, border), and `scrollbar`
(bar color, width).

### Theme vs. Per-Widget Overrides

Theme values are used as defaults. Any field set in a widget's option struct
takes priority. A zero/sentinel value (e.g. `{0,0,0,0}` for colors, `0` for
font size) means "use theme default."

```c
// Uses theme colors everywhere
wlx_button(ctx, "Default");

// Override just the background — text color still from theme
wlx_button(ctx, "Custom BG", .back_color = WLX_RGBA(200, 0, 0, 255));
```

---

## 11. Backend Contract

`wollix.h` does not render anything itself. It calls function pointers stored
in `WLX_Backend` to do all drawing and input. Backend adapters
(`wollix_raylib.h`, `wollix_sdl3.h`) populate these pointers for specific
graphics libraries.

### WLX_Backend Function Pointers

| Function | Signature | Purpose |
|----------|-----------|---------|
| `draw_rect` | `(WLX_Rect, WLX_Color)` | Fill a rectangle |
| `draw_rect_lines` | `(WLX_Rect, float thick, WLX_Color)` | Stroke a rectangle outline |
| `draw_rect_rounded` | `(WLX_Rect, float roundness, int segments, WLX_Color)` | Fill a rounded rectangle |
| `draw_line` | `(float x1, y1, x2, y2, float thick, WLX_Color)` | Draw a line segment |
| `draw_text` | `(const char *text, float x, y, WLX_Text_Style)` | Render text at a position |
| `measure_text` | `(const char *text, WLX_Text_Style, float *w, *h)` | Measure text bounding box |
| `draw_texture` | `(WLX_Texture, WLX_Rect src, dst, WLX_Color tint)` | Draw a texture region |
| `begin_scissor` | `(WLX_Rect)` | Begin a clip rectangle |
| `end_scissor` | `(void)` | End clipping |
| `get_frame_time` | `(void) → float` | Return frame delta time in seconds |

All 10 function pointers **must** be set before calling `wlx_begin`. The library
asserts on this.

### Writing a New Backend

1. Implement all 10 functions wrapping your graphics API.
2. Write an init function that populates `ctx->backend`.
3. Write an input handler that reads platform input into `ctx->input`.

See `wollix_raylib.h` or `wollix_sdl3.h` for reference implementations.

### Input State

The input handler callback writes to `ctx->input` (`WLX_Input_State`):

| Field | Type | Description |
|-------|------|-------------|
| `mouse_x`, `mouse_y` | `int` | Mouse position |
| `mouse_down` | `bool` | Button currently held |
| `mouse_clicked` | `bool` | Button pressed this frame (one-shot) |
| `mouse_held` | `bool` | Button held down |
| `wheel_delta` | `float` | Scroll wheel (positive = up) |
| `keys_down[WLX_KEY_COUNT]` | `bool[]` | Current key states |
| `keys_pressed[WLX_KEY_COUNT]` | `bool[]` | Key pressed this frame (one-shot) |
| `text_input[32]` | `char[]` | Text typed this frame (for input boxes) |
