# Core Patterns Guide

Internal guide for the recurring implementation patterns in `wollix.h`.

Use this file when touching widget internals or shared resolve/draw helpers.
Each pattern names the owning helper, one canonical example, and the failure
mode it is meant to prevent.

---

## Frame Pattern

The frame pattern brackets every `_impl` with a prologue/epilogue pair that
owns scope-id push/pop, geometry resolution, and any other shared lifecycle
work.  Three families use it:

| Family | Begin helper | End helper | Used by |
|---|---|---|---|
| Widget | `wlx_widget_frame_begin()` | `wlx_widget_frame_end()` | every widget `_impl` |
| Layout/Grid | `wlx_layout_frame_begin()` | `wlx_layout_frame_end()` | `wlx_layout_end()` |
| Scroll panel | `wlx_scroll_panel_frame_begin()` | `wlx_scroll_panel_frame_end()` | `wlx_scroll_panel_end()` |

### Widget frame

**Where it lives:** `WLX_Widget_Frame`, `wlx_widget_frame_begin()`,
`wlx_widget_frame_end()` in `wollix.h`.

**What it does:**

- Brackets every widget `_impl` with one prologue/epilogue pair.
- Balances `.id` pushes and pops through the returned frame object.
- Gives the widget body one resolved rect to use for interaction and drawing.

**Canonical example:** `wlx_widget_impl()`.

```c
WLX_Widget_Frame frame = wlx_widget_frame_begin(
    ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
WLX_Rect wr = frame.rect;

WLX_Interaction inter = wlx_get_interaction(
    ctx,
    wr,
    WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
    file, line
);

// ... widget body ...

wlx_widget_frame_end(ctx, frame);
```

### Layout/Grid frame

**Where it lives:** `WLX_Layout_Frame`, `wlx_layout_frame_begin()`,
`wlx_layout_frame_end()` in `wollix.h`.

**What it does:**

- Shared prologue for all five layout/grid begin entry points
  (`wlx_layout_begin_impl`, `wlx_grid_begin_impl`, `wlx_grid_begin_auto_impl`,
  `wlx_grid_begin_auto_tile_impl`, `wlx_layout_begin_auto_impl`).
- Pushes the scope id (from `co.id`) as the very first action so that
  downstream state lookups and interactions are properly scoped.
- Selects the slot rect, opens the command range, draws the container
  background, resolves padding, captures viewport dimensions for
  `WLX_SIZE_FILL` resolution.
- Returns `frame.pushed_scope_id`; each begin impl copies it into
  `l.pushed_scope_id` so `wlx_layout_frame_end` can pop the id at end.

**Canonical example:** `wlx_layout_begin_impl()`.

```c
WLX_Layout_Frame frame = wlx_layout_frame_begin(
    ctx, WLX_LAYOUT_COMMON_OPT(opt), file, line);

WLX_Layout l = wlx_create_layout(ctx, frame.rect, count, orient, opt.gap);
wlx_layout_apply_common(&l, WLX_LAYOUT_COMMON_OPT(opt), frame);
l.viewport = (orient == WLX_HORZ) ? frame.viewport_horz : frame.viewport_vert;
// ... CONTENT pre-resolution, slot offsets ...
l.pushed_scope_id = frame.pushed_scope_id;
wlx_pool_push(&ctx->arena.layouts, WLX_Layout, l);
```

`wlx_layout_end` finishes all measurements and range work, then delegates
cleanup to `wlx_layout_frame_end`:

```c
// last lines of wlx_layout_end:
wlx_layout_frame_end(ctx, l);  // pops stack + pops id if pushed
```

### Scroll panel frame

**Where it lives:** `WLX_Scroll_Panel_Frame`, `wlx_scroll_panel_frame_begin()`,
`wlx_scroll_panel_frame_end()` in `wollix.h`.

**What it does:**

- Prologue for `wlx_scroll_panel_begin_impl`: pushes the scope id, resolves
  opts and rects, fetches persistent state, contributes to the parent layout,
  manages the auto-scroll context, detects hover, and pushes onto the scroll
  panel stack.
- `wlx_scroll_panel_frame_end` owns only the scope pop; `wlx_scroll_panel_end`
  performs all measurement, wheel handling, clamp, and scissor restore before
  calling it.

```c
// last line of wlx_scroll_panel_end:
wlx_scroll_panel_frame_end(ctx, state_sp);  // pops id if pushed
```

**Anti-pattern to avoid (all families):**

- Calling `wlx_push_id()` / `wlx_pop_id()` manually inside a begin or end impl
  instead of delegating to the frame helper.
- Returning early from a begin impl on a path that skips copying
  `frame.pushed_scope_id` into the layout record, which would leave the id
  stack unbalanced.

---

## Resolve Pipeline

**Where it lives:** `wlx_resolve_opt_*()` helpers, plus
`wlx_resolve_typography()`, `wlx_resolve_border()`,
`wlx_resolve_opacity_for()`, and `WLX_APPLY_OPACITY(...)` in `wollix.h`.

**What it does:**

- Resolves `{0}` colors from the theme first.
- Resolves shared typography and border fields through the common helpers.
- Resolves inherited opacity from the context once.
- Applies the resolved opacity to every widget color before the draw body runs.

**Canonical example:** `wlx_resolve_opt_label()`.

```c
if (wlx_color_is_zero(opt->front_color)) opt->front_color = theme->foreground;
if (wlx_color_is_zero(opt->back_color))  opt->back_color  = theme->surface;
wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->spacing, &opt->min_height);
wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);
opt->opacity = wlx_resolve_opacity_for(ctx, opt->opacity);
WLX_APPLY_OPACITY(opt->opacity, &opt->front_color, &opt->back_color, &opt->border_color);
```

**Anti-pattern to avoid:**

- Repeating theme fallback lists in the widget draw body.
- Applying opacity before border/color resolution is complete.
- Leaving one color field out of `WLX_APPLY_OPACITY(...)` so fill and border do
  not share the same alpha contract.

Widget-specific pre-processing is still allowed before the shared helpers run.
Keep the special case explicit, then rejoin the common pipeline.

---

## Box Draw

**Where it lives:** `WLX_Box_Style` and `wlx_draw_box()` in `wollix.h`.

**What it does:**

- Centralizes the repeated fill + border draw logic.
- Keeps the rounded and non-rounded branches in one helper.
- Resolves `rounded_segments` against the theme in one place.

**Canonical example:** `wlx_widget_impl()`.

```c
wlx_draw_box(ctx, rect, (WLX_Box_Style){
    .fill             = opt.color,
    .border           = opt.border_color,
    .border_width     = opt.border_width,
    .roundness        = opt.roundness,
    .rounded_segments = opt.rounded_segments,
});
```

**Anti-pattern to avoid:**

- Re-opening the old inline `if (fill) ... if (border) ...` branches in each
  widget implementation.
- Recomputing rounded-segment fallback logic at each call site.

---

## Sentinel Doctrine

**Where it lives:** `wlx_is_negative_unset()` and `wlx_is_float_unset()` in
`wollix.h`.

**What it does:**

- Uses `wlx_is_negative_unset(x)` for fields where negative values are never
  legal and `-1`-style sentinels are acceptable.
- Uses `wlx_is_float_unset(x)` for fields where negative values are legal and
  `WLX_FLOAT_UNSET` is the real sentinel.
- Preserves explicit `0.0f` values for hover-related brightness fields.

**Canonical examples:**

```c
if (wlx_is_negative_unset(*border_width)) *border_width = theme->border_width;
if (wlx_is_float_unset(opt->hover_brightness))
    opt->hover_brightness = theme->hover_brightness;
```

**Anti-pattern to avoid:**

- Raw `x < 0` checks on public option fields that already use the named helper.
- Using `-1` as the unset test for `hover_brightness`-style fields where
  negative values are meaningful.

See also: [SENTINEL.md](../../SENTINEL.md)