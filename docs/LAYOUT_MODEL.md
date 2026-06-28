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
6. [Layout Capabilities](#6-layout-capabilities)
7. [Alignment](#7-alignment)
8. [Widgets & Option Structs](#8-widgets--option-structs)
9. [Interaction](#9-interaction)
10. [IDs & Persistent State](#10-ids--persistent-state)
11. [Theming](#11-theming)
12. [Backend Contract](#12-backend-contract)
13. [Deferred Drawing](#13-deferred-drawing)

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
#include "wollix_raylib.h"   // or wollix_sdl3.h / wollix_wasm.h
```

---

## 2. Frame Loop

Every frame follows the same structure:

```c
wlx_begin(&ctx, root_rect, input_handler);
    // ... layouts, widgets, drawing ...
wlx_end(&ctx);
```

**`wlx_begin`** resets per-frame state (hot widget, arena pool, command buffer,
debug/perf tracking), calls the input handler to populate `ctx.input`, and sets
the root rect.

**`wlx_end`** replays the deferred command buffer (see [Deferred Drawing](#13-deferred-drawing)),
then closes the frame.

The **input handler** is a callback that reads platform input and writes it to
`ctx->input`. Backend adapters provide one (e.g. `wlx_process_raylib_input`).

### Context Lifetime

```c
WLX_Context ctx = {0};              // zero-init once
wlx_context_init_raylib(&ctx);      // attach backend
ctx.theme = &wlx_theme_dark;        // optional (dark is the default)
```

The backend init helpers (`wlx_context_init_raylib`, `wlx_context_init_sdl3`,
`wlx_context_init_wasm`) populate `ctx->backend`. A zero-initialized context is
enough for default arena settings: `wlx_begin` lazily initializes the arena pool
on the first frame.

If you want to pre-initialize the arenas or pass a custom
`WLX_Arena_Pool_Config`, call the core init function before attaching the
backend, because `wlx_context_init` and `wlx_context_init_ex` zero the context:

```c
// Optional default pre-init:
wlx_context_init(&ctx);
wlx_context_init_raylib(&ctx);

// Or, for custom arena/pool sizing:
wlx_context_init_ex(&ctx, &cfg);
wlx_context_init_raylib(&ctx);
```

When the context is no longer needed, free its buffers:

```c
wlx_context_destroy(&ctx);
```

`WLX_Context` owns dynamically-growing buffers (arena pool, command buffer,
state map, scratch offsets). Zero-init it once, then reuse across frames. The
context is not thread-safe — use one per thread if needed.

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
- `slot_px = 0` — variable mode: call `wlx_layout_auto_slot(ctx, size)` before
  each child to set its size. Any `WLX_Slot_Size` type is accepted.

### Mixed-Size Auto Layouts

Dynamic layouts support the full range of `WLX_Slot_Size` types via
`wlx_layout_auto_slot()`. Non-pixel types are resolved to pixels immediately:

```c
wlx_layout_begin_auto(ctx, WLX_VERT, 0);
    // Fixed-size items
    for (int i = 0; i < n; i++) {
        wlx_layout_auto_slot(ctx, WLX_SLOT_PX(36));
        wlx_button(ctx, items[i], .height = 36);
    }
    // Fill remaining space
    wlx_layout_auto_slot(ctx, WLX_SLOT_FLEX(1));
    wlx_label(ctx, "Footer", .height = -1);
wlx_layout_end(ctx);
```

**Supported types and resolution:**

| Type | Resolution in auto layout |
|------|---------------------------|
| `WLX_SLOT_PX(px)` | Exact pixel value |
| `WLX_SLOT_PCT(pct)` | Percentage of the layout rect |
| `WLX_SLOT_FILL` | Full viewport height/width |
| `WLX_SLOT_FILL_PCT(pct)` | Percentage of viewport |
| `WLX_SLOT_FLEX(w)` | **Greedy** — takes all remaining space |
| `WLX_SLOT_AUTO` | Same as FLEX (greedy remaining) |
| `WLX_SLOT_CONTENT` | Uses `size.value` as pre-resolved pixels (0 if unset, then normal 1px floor) |

Min/max constraints work as expected:

```c
wlx_layout_auto_slot(ctx, WLX_SLOT_FLEX_MIN(1, 50));   // at least 50px
wlx_layout_auto_slot(ctx, WLX_SLOT_PCT_MINMAX(20, 80, 300)); // 20%, clamped
```

> **FLEX is greedy, not proportional.** In a dynamic layout, a FLEX slot
> consumes all remaining space at the point it is declared. Two FLEX slots
> will *not* split evenly — the first takes everything. For proportional
> multi-flex splitting, use a static layout (`wlx_layout_begin_s`).

The pixel-only convenience function `wlx_layout_auto_slot_px(ctx, px)` is
still available and equivalent to `wlx_layout_auto_slot(ctx, WLX_SLOT_PX(px))`.

### Slot Placement Options

Most layout and widget macros that consume a slot accept optional named
arguments:

| Field     | Default | Description |
|-----------|---------|-------------|
| `pos`     | `-1`    | Target slot index (`-1` = next sequential) |
| `span`    | `1`     | Number of consecutive slots to occupy |
| `overflow`| `false` | Allow widget to exceed slot bounds |
| `padding` | `0`     | Uniform inset applied to the slot rect |
| `padding_top/right/bottom/left` | `-1` | Per-side slot inset; values `>= 0` override `padding` |

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
| `WLX_SLOT_FILL` | `WLX_SIZE_FILL` | Fill entire viewport (scroll panel or layout rect) |
| `WLX_SLOT_FILL_PCT(pct)` | `WLX_SIZE_FILL` | Fill percentage of viewport |
| `WLX_SLOT_CONTENT` | `WLX_SIZE_CONTENT` | Measured from child content (frame-delayed) |

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
WLX_SLOT_FILL_MIN(100)            // fill viewport, at least 100px
WLX_SLOT_FILL_MAX(400)            // fill viewport, at most 400px
WLX_SLOT_FILL_MINMAX(100, 400)    // fill viewport, clamped to [100, 400]
```

A `0` for min or max means unconstrained.

### Advanced: Redistribute Mode

By default, clamping uses iterative freeze-and-redistribute: when a slot clamps
to its min or max, its surplus or deficit is handed to unfrozen flex/auto
siblings so the resolved boundary stays at `total` (no gap, no overflow). Define
`WLX_SLOT_SINGLE_PASS_CLAMP` before including `wollix.h` to opt out and use the
simpler O(n) single-pass clamp, which may leave a gap or overflow when min/max
fires. (`WLX_SLOT_MINMAX_REDISTRIBUTE`, the former opt-in symbol, is now a
deprecated no-op kept for source compatibility.)

Redistribution cannot remove a *physical* floor: if the fixed and min-clamped
sizes together exceed the container, there is no slack to redistribute and the
boundary still overflows. Contain that with an opt-in layout `.clip`; under
`WLX_DEBUG` an over-allocating layout emits a one-time warning unless it (or an
ancestor) clips.

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

### Content-Sized Rows

Grid rows support `WLX_SLOT_CONTENT` sizing. Each CONTENT row measures the
tallest widget placed in that row (across all columns) and uses that height
on the next frame. This uses the same one-frame-delay persistent state
mechanism as linear CONTENT slots.

```c
WLX_Slot_Size row_sizes[] = {
    WLX_SLOT_CONTENT,           // adapts to tallest widget in row 0
    WLX_SLOT_CONTENT_MIN(50),   // at least 50px regardless of content
    WLX_SLOT_CONTENT_MAX(30),   // capped at 30px even if content is taller
    WLX_SLOT_PX(100),           // fixed rows can be mixed freely
};
wlx_grid_begin(ctx, 4, 3, .row_sizes = row_sizes);
    // ... 12 widgets ...
wlx_grid_end(ctx);
```

Min/max constraints work as expected: `WLX_SLOT_CONTENT_MIN(px)` sets a floor,
`WLX_SLOT_CONTENT_MAX(px)` sets a ceiling, and `WLX_SLOT_CONTENT_MINMAX(lo, hi)`
sets both. CONTENT rows can be mixed with PX, FLEX, PCT, and other row types
in the same grid.

> **Note:** CONTENT is supported only in `row_sizes`, not `col_sizes`.
> Max 32 CONTENT rows per grid (`WLX_CONTENT_SLOTS_MAX`).

### Widget Contribution Rule

A widget contributes to its parent's CONTENT measurement as follows:

- **Explicit `.height`:** contributes that value directly.
- **No explicit `.height`:** contributes `max(min_h, cell.h)`, where
  `min_h` is the widget's declared minimum height and `cell.h` is the
  space allocated by the parent.

Built-in widgets (label, button, checkbox, inputbox, slider, progress,
toggle, radio) default `min_height` to the resolved `font_size` when
unset.  This guarantees CONTENT slots measure to a non-zero value on
the first frame, even when nested layouts allocate zero space due to
the frame-delayed measurement model.

The generic `wlx_widget()` does not default `min_height` -- set it
explicitly if the widget lives inside a CONTENT slot without a fixed
`.height`.

### Explicit Cell Placement

Use `wlx_grid_cell()` to place a child at a specific row/column with optional spans:

```c
wlx_grid_begin(ctx, 3, 3);
    wlx_grid_cell(ctx, 0, 0, .col_span = 2);    // row 0, cols 0–1
    wlx_label(ctx, "Wide header");

    wlx_grid_cell(ctx, 1, 2, .row_span = 2);    // rows 1–2, col 2
    wlx_label(ctx, "Tall sidebar");
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

## 6. Layout Capabilities

### Sizing Support

| Layout | Axis | PX | PCT | FLEX | AUTO | FILL | CONTENT | Min/Max |
|---|---|---|---|---|---|---|---|---|
| `wlx_layout_begin` | slots (VERT/HORZ) | Yes | Yes | Yes | Yes | Yes | Yes | Yes |
| `wlx_layout_begin_s` | slots (VERT/HORZ) | Yes | Yes | Yes | Yes | Yes | Yes | Yes |
| `wlx_layout_begin_auto` (slot_px>0) | per-slot uniform | Fixed | -- | -- | -- | -- | -- | -- |
| `wlx_layout_begin_auto` (slot_px=0) | per-slot via `auto_slot` | Yes | Yes | Yes | Yes | Yes | Passthrough\* | Yes |
| `wlx_grid_begin` | rows | Yes | Yes | Yes | Yes | Yes | Yes | Yes |
| `wlx_grid_begin` | cols | Yes | Yes | Yes | Yes | Yes | No | Yes |
| `wlx_grid_begin_auto` | rows (dynamic) | Fixed | -- | -- | -- | -- | -- | -- |
| `wlx_grid_begin_auto` | cols | Yes | Yes | Yes | Yes | Yes | No | Yes |
| `wlx_grid_begin_auto_tile` | rows (dynamic) | Fixed | -- | -- | -- | -- | -- | -- |
| `wlx_grid_begin_auto_tile` | cols | Derived | -- | -- | -- | -- | -- | -- |

\* `wlx_layout_auto_slot` with CONTENT reads `size.value` as pre-resolved px.
If no value is supplied it starts at 0 and then follows the normal dynamic-slot
1px floor. No persistent measurement loop exists here - the caller must supply
the value.

### Restrictions

| Layout | Restrictions |
|---|---|
| `wlx_layout_begin` | Slot count must be known at begin time. Max 32 slots when using CONTENT (`WLX_CONTENT_SLOTS_MAX`). |
| `wlx_layout_begin_s` | Same as `wlx_layout_begin`; count derived from `WLX_SIZES()` macro. |
| `wlx_layout_begin_auto` | Sequential access only (no `pos`). No nested `wlx_layout_begin` inside body (scratch buffer corruption). When `slot_px=0`, every child must call `wlx_layout_auto_slot` before use. CONTENT via `auto_slot` is not auto-measured (caller-supplied value, or 1px after the floor). |
| `wlx_grid_begin` | Row and column count fixed at begin time. CONTENT supported in `row_sizes` (per-row max height, one-frame delay). CONTENT not supported in `col_sizes` (resolves to 0px). Max 32 CONTENT rows (`WLX_CONTENT_SLOTS_MAX`). Max `WLX_MAX_SLOT_COUNT` rows/cols. |
| `wlx_grid_begin_auto` | Column count fixed; rows grow dynamically. Requires `row_px > 0` (no zero or content-sized rows). Per-row override via `wlx_grid_auto_row_px` consumed once. CONTENT not supported in `col_sizes`. |
| `wlx_grid_begin_auto_tile` | Same as `wlx_grid_begin_auto`. Column count derived from `floor(width / tile_w)`, minimum 1. `col_sizes` in opt is accepted but column count is computed from tile width. |
| All layouts | CONTENT sizing uses one-frame delay (0px first frame, measured on second). Use `WLX_SLOT_CONTENT_MIN()` to avoid visual pop. |
| All layouts | Persistent state keyed by `__FILE__`/`__LINE__` — duplicate calls at same source location share state (e.g. layouts in loops). |

---

## 7. Alignment

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

## 8. Widgets & Option Structs

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

Widget option structs share these field groups, depending on widget type:

**Placement** (from `WLX_LAYOUT_SLOT_FIELDS`):

| Field | Default | Description |
|-------|---------|-------------|
| `pos` | `-1` | Slot index |
| `span` | `1` | Slot span |
| `overflow` | `false` | Allow exceeding slot bounds |
| `padding` | `0` | Slot inset |
| `padding_top/right/bottom/left` | `-1` | Per-side slot inset; values `>= 0` override `padding` |

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
| `opacity` | `-1` | Opacity (`-1` = fully opaque sentinel) |

**State** (interactive widgets only, from `WLX_WIDGET_STATE_FIELDS`):

| Field | Default | Description |
|-------|---------|-------------|
| `disabled` | `false` | Disable click/focus/drag activation while preserving hover arbitration |

**Typography** (text widgets; `wrap` is a separate opt-in field):

| Field | Default | Description |
|-------|---------|-------------|
| `font` | `WLX_FONT_DEFAULT` | Font handle |
| `font_size` | `0` | Font size (`0` = use theme) |
| `align` | `WLX_LEFT` | Text alignment within widget |
| `spacing` | `0` | Extra character spacing (`0` = backend default) |
| `wrap` | varies | Word-wrap long text on widgets that embed `WLX_TEXT_WRAP_FIELDS`; slider omits it |

**Colors** (from `WLX_TEXT_COLOR_FIELDS`):

| Field | Default | Description |
|-------|---------|-------------|
| `front_color` | `{0}` | Text color (`{0}` = use theme foreground) |
| `back_color` | `{0}` | Background color (`{0}` = use theme surface) |

**Content padding** (widgets that embed `WLX_CONTENT_PADDING_FIELDS`):

| Field | Default | Description |
|-------|---------|-------------|
| `content_padding` | `-1` | Uniform inset around widget content; `WLX_PADDING_USE_THEME` opts into `theme.padding` |
| `content_padding_top/right/bottom/left` | `-1` | Per-side content inset; values `>= 0` override the uniform value |

**Border** (bordered widgets, from `WLX_BORDER_FIELDS`):

| Field | Default | Description |
|-------|---------|-------------|
| `border_color` | `{0}` | Uniform border color (`{0}` = theme/widget fallback) |
| `border_width` | `-1` | Uniform border width (`-1` = theme default, `0` = none) |
| `roundness` / `rounded_segments` | `-1` | Corner roundness (fraction of shorter side) and segment count |
| `corner_radius` | `0` | Absolute corner radius in **pixels**; `> 0` overrides `roundness`, `0` = unset |
| `border_color_top/right/bottom/left` | `{0}` | Per-side border color; `{0}` inherits `border_color` |
| `border_width_top/right/bottom/left` | `-1` | Per-side border width; `< 0` inherits `border_width`, `0` switches that edge off |

### Per-side container decor

`wlx_layout_begin`, `wlx_grid_begin`, `wlx_grid_begin_auto`, and the
`WLX_Panel_Opt` body carry the same container decor (`back_color`,
`border_color`, `border_width`, `roundness`) **plus** the eight per-side fields
above. They let a container draw a two-tone bevel (per-side color) or a
single-edge accent bar (per-side width) declaratively, keeping the chrome inside
the container's own draw range (correct clip/layer/opacity ordering) instead of
reaching for raw command emitters.

Sentinels match the widget border: a per-side color `{0}` inherits
`border_color`, a per-side width `< 0` inherits `border_width`, and `0` switches
an edge off. When every edge resolves equal the uniform rounded/sharp path is
used unchanged; when any edge differs, each active edge draws as a **sharp**
span (per-side strokes do not follow corner roundness). To make a container's
own background or border react to hover, see [Interactive containers](#interactive-containers).

```c
// Glass module surface: light top/left, dark bottom/right, translucent fill.
wlx_layout_begin(ctx, 1, WLX_VERT,
    .back_color = glass_fill, .roundness = 0.10f, .rounded_segments = 8,
    .border_color_top = edge_light, .border_color_left = edge_light,
    .border_color_bottom = edge_dark, .border_color_right = edge_dark,
    .border_width_top = 1, .border_width_left = 1,
    .border_width_bottom = 1, .border_width_right = 1);
```

### Interactive containers

Containers can be made hover/click-reactive directly, without a probe widget, by
setting `interact` on the begin opt. `begin` resolves the interaction on the
container rect (using its scope id), styles its own chrome from the hover-variant
colors, and hands the raw `WLX_Interaction` back through `interact_out` for child
styling and side effects. `interact = 0` (the default) skips the query, so
non-interactive containers pay nothing.

```c
WLX_Interaction it;
wlx_layout_begin(ctx, 1, WLX_VERT, .id = row_id,
    .interact = WLX_INTERACT_CLICK | WLX_INTERACT_HOVER, .interact_out = &it,
    .back_color = (WLX_Color){0}, .hover_back_color = wash,
    .border_width_left = 2, .border_color_left = (WLX_Color){0},
    .hover_border_color_left = accent);   // bar appears on hover
    // body reads it.hover for child text / icon colors
wlx_layout_end(ctx);
if (it.clicked) navigate();
```

- **Color-only, replace semantics.** Hover twins exist for the background and
  the uniform/per-side border colors only (`hover_back_color`,
  `hover_border_color`, `hover_border_color_top/right/bottom/left`). A non-zero
  twin replaces its base color while hovered; a `{0}` twin keeps the base. There
  are no hover-variant width/gradient/shadow/glow fields — an *appearing* border
  is a constant width with a transparent base color toggled to an accent twin.
- **Supported flags.** `HOVER`, `CLICK`, `KEYBOARD`. `FOCUS`/`DRAG` are not
  supported on containers, and the auto-counted begins (`wlx_layout_begin_auto`,
  `wlx_grid_begin_auto`, `wlx_grid_begin_auto_tile`) lack a stable call-site id
  and do not support `interact`.
- **Non-interactive children only.** A `CLICK` container is queried before its
  children and captures the press first (click capture is first-writer), so a
  nested interactive child can never fire its own click. Keep interactive
  containers to composite regions of non-interactive content. For a clickable
  region that owns a button, give the *child* the click and use `HOVER` only on
  the container.

### Absolute corner radius (`corner_radius`)

`roundness` is a fraction of the container's shorter side, so a layout of
differently sized panels rounded with one constant gets a different pixel corner
per panel. `corner_radius` declares the radius in **pixels** instead. It is
resolved centrally at draw time against the container's final rect with
`clamp(2 * corner_radius / min(w,h), 0, 1)`, so panels of any size share the same
pixel corner. `corner_radius > 0` overrides `roundness` for that container; the
default `0` leaves the existing fractional behavior untouched (a sharp corner is
still `roundness = 0`). Values past `min(w,h)/2` clamp to a full pill. Only the
fill follows the radius - per-side (two-tone) borders stay sharp spans. The field
is carried by every container decor path (`wlx_layout_begin`, `wlx_grid_begin`,
`wlx_grid_begin_auto`, and the `WLX_Panel_Opt` body); widget primitives that
draw their own rounded shapes ignore it in v1.

```c
// Two differently sized modules, identical 8 px corner regardless of size.
wlx_layout_begin(ctx, 1, WLX_VERT, .back_color = glass_fill, .corner_radius = 8.0f);
```

### Widget Quick Reference

**`wlx_label`** — static text display:
```c
wlx_label(ctx, "Hello", .font_size = 20, .align = WLX_CENTER, .wrap = true);
```

**`wlx_button`** — clickable, returns `true` on click:
```c
if (wlx_button(ctx, "OK", .height = 40, .back_color = WLX_RGBA(0, 120, 200, 255)))
    do_something();
```

`wlx_button` and `wlx_label` can also render optional image content via
`.texture`, `.texture_scale`, `.image_placement`, `.image_size`, and related
image fields.

**`wlx_checkbox`** — toggle, returns `true` when state changes:
```c
bool enabled = false;
if (wlx_checkbox(ctx, "Enable", &enabled))
    printf("toggled: %d\n", enabled);
```

**`wlx_checkbox` with texture mode** — checkbox with custom textures:
```c
wlx_checkbox(ctx, "Mute", &muted,
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

Pass `content_height = -1` for auto-height mode, where the library measures
content height from the first frame and applies it on subsequent frames.

**`wlx_widget`** — generic custom widget that occupies a slot without any built-in drawing:
```c
wlx_widget(ctx, .width = 100, .height = 40);
// use wlx_get_interaction() after this to handle input
```

**`wlx_separator`** — horizontal or vertical divider line:
```c
wlx_separator(ctx);
wlx_separator(ctx, .color = WLX_RGBA(100, 100, 100, 255), .thickness = 2.0f);
```

**`wlx_progress`** — progress bar, value in [0, 1]:
```c
wlx_progress(ctx, 0.75f);
wlx_progress(ctx, value, .fill_color = WLX_RGBA(0, 200, 80, 255), .height = 12);
```

**`wlx_image`** — draw a texture region with scale, alignment, source rect, and tint:
```c
wlx_image(ctx, texture, .scale = WLX_IMAGE_SCALE_FIT, .tint = WLX_RGBA(255, 255, 255, 255));
```

**`wlx_toggle`** — on/off toggle switch, returns `true` when state changes:
```c
bool enabled = false;
if (wlx_toggle(ctx, "Dark mode", &enabled))
    apply_theme(enabled);
```

**`wlx_radio`** — radio button in a group, returns `true` when selected:
```c
int selected = 0;
wlx_radio(ctx, "Option A", &selected, 0);
wlx_radio(ctx, "Option B", &selected, 1);
wlx_radio(ctx, "Option C", &selected, 2);
```

**`wlx_split`** — two-pane split layout with independent scroll panels:
```c
wlx_split_begin(ctx, .first_size = WLX_SLOT_PX(240));
    // first pane content (left / top)
    wlx_label(ctx, "Sidebar");
wlx_split_next(ctx);
    // second pane content (right / bottom)
    wlx_label(ctx, "Main area");
wlx_split_end(ctx);
```

**`wlx_panel`** — titled container that auto-counts its children:
```c
wlx_panel_begin(ctx, .title = "Settings", .gap = 8);
    wlx_checkbox(ctx, "Enable audio", &audio);
    wlx_slider(ctx, "Volume", &vol);
wlx_panel_end(ctx);
```

The panel counts children between `wlx_panel_begin` and `wlx_panel_end`
automatically, so no slot count is needed.

---

## 9. Interaction

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

`WLX_Interaction` has these fields:

| Field | Meaning |
|-------|---------|
| `id` | Generated interaction ID |
| `hover` | Mouse is currently over the widget |
| `pressed` | Mouse is down on this widget right now |
| `clicked` | Click completed (released while hovering) or keyboard-activated |
| `focused` | Widget has focus (FOCUS mode) |
| `active` | Widget is the active widget (being pressed, dragged, or focused) |
| `just_focused` | Became focused this frame |
| `just_unfocused` | Lost focus this frame |
| `disabled` | Interaction was queried through the disabled gate; active/click/focus results are forced false |

### Hot / Active Model

The library tracks two global widget slots per frame:

- **`hot_id`** — the widget the mouse is hovering over (reset each frame).
- **`active_id`** — the widget currently being pressed, dragged, or focused
  (persists across frames until released).

Only one widget can be hot and one can be active at a time. This prevents
overlapping interactions.

---

## 10. IDs & Persistent State

### How IDs Work

Every widget gets a unique ID derived from:

1. **Call-site** — `__FILE__` and `__LINE__` of the macro expansion.
2. **ID stack** — values pushed via `wlx_push_id()`.

Both interaction IDs and persistent state IDs use the same formula:
`hash(file, line) ^ id_stack_hash`. This means two calls to `wlx_button()` on
different source lines automatically get different IDs with no user effort.

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

Persistent state IDs use the same `hash(file, line) ^ id_stack_hash` formula
as interaction IDs, so they are stable across frames. Use `wlx_push_id` when
the same state call is reached multiple times.

Built-in paths that use persistent state: `WLX_SIZE_CONTENT` layout/grid/panel
measurement, `wlx_inputbox` (cursor position), and `wlx_scroll_panel` (scroll
offset, drag state).

### Explicit String IDs

Every widget opt struct has an optional `.id` field (a `const char *`,
default `NULL`). When set, the string is hashed and pushed onto the ID stack
for the duration of that widget call, giving it a unique, stable identity
regardless of call-site:

```c
for (int i = 0; i < count; i++) {
    if (wlx_button(ctx, names[i], .id = names[i]))
        printf("clicked %s\n", names[i]);
}
```

This is equivalent to wrapping the call in `wlx_push_id` / `wlx_pop_id` with
`wlx_hash_string(names[i])`, but more concise. The string is hashed with
`wlx_hash_string()` (djb2) and the resulting value is pushed/popped
automatically inside the widget implementation.

Layouts, grids, panels, and splits also accept `.id`; for those container APIs,
the string scopes the descendants inside the layout body.

**When to use `.id`:**
- Widgets created in loops where each iteration has a meaningful string key.
- Dynamic widget lists whose order may change between frames.
- Any situation where `wlx_push_id` would be needed for a single widget.

**When `wlx_push_id` is still preferable:**
- Wrapping multiple related widgets that share a group identity.
- Using integer keys (loop indices) rather than string keys.

---

## 11. Theming

### Theme Struct

A `WLX_Theme` provides default colors, fonts, and widget-specific
overrides for the entire UI. Set it on the context:

```c
ctx.theme = &wlx_theme_dark;     // built-in dark theme (default)
ctx.theme = &wlx_theme_light;    // built-in light theme
ctx.theme = &wlx_theme_glass;    // built-in translucent theme
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
| `border_width` | Default border width (0 = no border) |
| `accent` | Active/focused accent color |
| `font` | Default font handle |
| `font_size` | Default font size |
| `padding` | Default content padding when widgets opt in with `WLX_PADDING_USE_THEME` |
| `roundness` | Corner radius |
| `rounded_segments` | Segment count for rounded drawing |
| `min_rounded_segments` | Minimum segment floor for fully-round widgets |
| `hover_brightness` | Brightness shift on hover |
| `disabled_brightness` | Brightness shift when disabled (`WLX_FLOAT_UNSET` = no shift) |
| `opacity` | Global opacity multiplier |
| `disabled_opacity` | Alpha multiplier when disabled (`< 0` = unset) |

Widget-specific overrides exist for `input` (border focus, cursor color),
`slider` (track, thumb, label), `checkbox` (check, border), `toggle` (track colors, thumb),
`radio` (ring, fill, label), `progress` (track, fill), and `scrollbar` (bar color, width).

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

## 12. Backend Contract

`wollix.h` does not render anything itself. It calls function pointers stored
in `WLX_Backend` to do drawing and frame-time queries. Backend adapters
(`wollix_raylib.h`, `wollix_sdl3.h`, `wollix_wasm.h`) populate these pointers
for specific graphics environments and provide input handlers separately.

### WLX_Backend Function Pointers

`WLX_Backend` currently exposes 15 function pointers: 11 required callbacks
checked by `wlx_backend_is_ready()`, plus 2 optional native circle helpers and
2 optional slice-aware text helpers.

| Function | Signature | Required | Purpose |
|----------|-----------|----------|---------|
| `draw_rect` | `(WLX_Rect, WLX_Color)` | Yes | Fill a rectangle |
| `draw_rect_lines` | `(WLX_Rect, float thick, WLX_Color)` | Yes | Stroke a rectangle outline |
| `draw_rect_rounded` | `(WLX_Rect, float roundness, int segments, WLX_Color)` | Yes | Fill a rounded rectangle |
| `draw_rect_rounded_lines` | `(WLX_Rect, float roundness, int segments, float thick, WLX_Color)` | Yes | Stroke a rounded rectangle outline |
| `draw_circle` | `(float cx, float cy, float radius, int segments, WLX_Color)` | No | Native circle fill; `NULL` falls back to the rounded-rect tessellation path |
| `draw_ring` | `(float cx, float cy, float inner_r, float outer_r, int segments, WLX_Color)` | No | Native ring draw; `NULL` falls back to the rounded-outline tessellation path |
| `draw_line` | `(float x1, y1, x2, y2, float thick, WLX_Color)` | Yes | Draw a line segment |
| `draw_text` | `(const char *text, float x, y, WLX_Text_Style)` | Yes | Render text at a position |
| `measure_text` | `(const char *text, WLX_Text_Style, float *w, *h)` | Yes | Measure text bounding box |
| `draw_texture` | `(WLX_Texture, WLX_Rect src, dst, WLX_Color tint)` | Yes | Draw a texture region |
| `begin_scissor` | `(WLX_Rect)` | Yes | Begin a clip rectangle |
| `end_scissor` | `(void)` | Yes | End clipping |
| `get_frame_time` | `(void) -> float` | Yes | Return frame delta time in seconds |
| `draw_text_slice` | `(const char *text, size_t len, float x, y, WLX_Text_Style)` | No | Render an explicit byte span; `NULL` falls back to `draw_text` |
| `measure_text_slice` | `(const char *text, size_t len, WLX_Text_Style, float *w, *h)` | No | Measure an explicit byte span; `NULL` falls back to `measure_text` |

All 11 required function pointers must be set before calling `wlx_begin`. The
library checks this with `wlx_backend_is_ready` and asserts on missing required
pointers. `draw_circle` and `draw_ring` are optional; if they are `NULL`, the
core routes circle and ring commands through `draw_rect_rounded` and
`draw_rect_rounded_lines` using the requested segment count.

The required text callbacks use null-terminated strings and remain the backend
compatibility floor. Slice callbacks are optional and are used for fitted-text
ranges and command replay when available; otherwise the core creates a
temporary null-terminated copy. Draw and measure must agree for the same text
and style on each backend while preserving native whole-run text rendering.
Embedded NUL bytes are not supported by the public text model and are
truncated before dispatch.

### Writing a New Backend

1. Implement all 11 required functions wrapping your graphics API.
    Optionally implement `draw_circle`, `draw_ring`, `draw_text_slice`, and
    `measure_text_slice` for native shape quality and byte-span text handling.
2. Write an init function that populates `ctx->backend`.
3. Write an input handler that reads platform input into `ctx->input`.

See `wollix_raylib.h`, `wollix_sdl3.h`, or `wollix_wasm.h` for reference
implementations.

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

---

## 13. Deferred Drawing

By default, wollix **defers** all draw calls. Every `wlx_draw_*` helper
records a command into a per-frame buffer instead of dispatching to the
backend immediately. When `wlx_end` is called, the library replays the
entire buffer, applying accumulated translation offsets so that widgets
inside `WLX_SIZE_CONTENT` slots appear at their resolved positions from
the very first frame.

### Why deferred?

`WLX_SIZE_CONTENT` slots are measured by the children they contain. In a
single-pass model, children draw at *provisional* positions before the
parent slot is finalized. Deferral lets `wlx_layout_end` compute the
delta between provisional and resolved positions and write it into a
range table. The replay pass in `wlx_end` then shifts every command by
the correct offset.

### Immediate mode opt-out

For debugging, profiling, or backwards compatibility, call
`wlx_begin_immediate` instead of `wlx_begin`:

```c
wlx_begin_immediate(&ctx, root_rect, input_handler);
    // draws dispatch directly to the backend (no deferral)
wlx_end(&ctx);
```

In immediate mode, `wlx_end` skips the replay pass. CONTENT slots
still work but may show a one-frame positional lag on the first frame
they appear or change shape.

### Demo ordering

Because `wlx_end` dispatches the draw commands, it must be called
**before** the backend presents the frame:

```c
wlx_end(&ctx);      // replay draws
EndDrawing();        // present (Raylib example)
```
