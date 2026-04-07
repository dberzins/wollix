# Opacity

Control the transparency of widgets, layout regions, and the entire UI
through three composable layers. Each layer is optional and defaults to fully
opaque when omitted.

---

## Concepts

Opacity is a `float` in the range `0.0` (fully transparent) to `1.0` (fully
opaque). Wollix provides three independent layers that multiply together to
produce the final alpha applied to every colour a widget draws:

```
final_opacity = widget_opacity * theme_opacity * context_stack_opacity
```

| Layer | How to set it | Typical use |
|-------|---------------|-------------|
| **Widget** | `.opacity = 0.5f` on any widget call | Fade a single widget (disabled look, ghost buttons) |
| **Theme** | `theme.opacity = 0.8f` on `WLX_Theme` | Global UI transparency slider |
| **Context stack** | `wlx_push_opacity()` / `wlx_pop_opacity()` | Region or panel fade (overlays, inactive groups) |

All three layers are independently optional. Omitting any layer costs
nothing and behaves as `1.0`.

---

## Per-Widget Opacity

Every widget option struct includes an `opacity` field. Set it with
a designated initializer:

```c
wlx_button(ctx, "Save", .opacity = 0.5f);
wlx_label(ctx, "Ghost text", .opacity = 0.3f);
wlx_slider(ctx, "Volume", &vol, .opacity = 0.7f);
```

The sentinel value is negative (`-1`). When you omit `.opacity`, it defaults
to `-1`, which resolves to `1.0` (fully opaque). Setting `.opacity = 0.0f`
makes the widget fully transparent.

Opacity is applied after colour resolution and hover brightening. The
ordering is:

1. Resolve theme colours (fill missing fields from the active theme).
2. Apply hover brightness (modifies RGB, preserves alpha).
3. Multiply the three opacity layers.
4. Scale every colour's alpha channel by the result.

This means a hovered semi-transparent button brightens correctly while
staying at the requested transparency.

---

## Theme Opacity

`WLX_Theme` has an `opacity` field that acts as a global multiplier for
every widget drawn under that theme:

```c
WLX_Theme theme = wlx_theme_dark;
theme.opacity = 0.6f;           // entire UI at 60% opacity
ctx->theme = &theme;
```

The built-in presets (`wlx_theme_dark`, `wlx_theme_light`) set
`opacity = -1` (sentinel for fully opaque). To use theme opacity at runtime,
work with a mutable copy of the preset.

A common pattern is to expose theme opacity as a user-facing "UI
transparency" setting:

```c
wlx_slider(ctx, "UI Opacity:", &my_theme.opacity,
    .min_value = 0.0f, .max_value = 1.0f);
```

---

## Context Opacity Stack

The context opacity stack lets you make entire layout regions
semi-transparent without touching individual widgets:

```c
wlx_push_opacity(ctx, 0.4f);

    wlx_button(ctx, "Faded A", .height = 30);
    wlx_button(ctx, "Faded B", .height = 30);
    wlx_checkbox(ctx, "Faded check", &val);

wlx_pop_opacity(ctx);
```

Every widget drawn between `push` and `pop` receives the pushed opacity as
the context-stack factor.

### API

```c
void  wlx_push_opacity(WLX_Context *ctx, float opacity);
void  wlx_pop_opacity(WLX_Context *ctx);
float wlx_get_opacity(const WLX_Context *ctx);
```

- **`wlx_push_opacity`** multiplies `opacity` with the current stack top and
  pushes the result. This means nested pushes compound automatically.
- **`wlx_pop_opacity`** removes the top entry. Every push must have a
  matching pop within the same frame.
- **`wlx_get_opacity`** returns the current stack top, or `1.0` when the
  stack is empty.

The stack resets to empty at the start of every frame (`wlx_begin`).

### Nested Pushes

Pushes nest and multiply. In the example below, widgets inside the inner
push see a combined context opacity of `0.5 * 0.5 = 0.25`:

```c
wlx_push_opacity(ctx, 0.5f);           // context = 0.5

    wlx_button(ctx, "Half", .height = 30); // drawn at 0.5

    wlx_push_opacity(ctx, 0.5f);       // context = 0.25
        wlx_button(ctx, "Quarter", .height = 30);
    wlx_pop_opacity(ctx);              // back to 0.5

wlx_pop_opacity(ctx);                  // back to 1.0
```

### Per-Widget Overrides Inside a Push

Set `.opacity` on individual widgets to override or adjust within a pushed
region. The factors still multiply:

```c
wlx_push_opacity(ctx, 0.4f);

    // This button is 0.4 (stack) * 1.0 (widget default) = 0.4
    wlx_button(ctx, "Normal", .height = 30);

    // This button is 0.4 (stack) * 0.5 (widget) = 0.2
    wlx_button(ctx, "Extra faded", .height = 30, .opacity = 0.5f);

wlx_pop_opacity(ctx);
```

---

## How the Layers Combine

The three layers always multiply. Some examples:

| Widget `.opacity` | `theme.opacity` | Stack | Final | Result |
|-------------------|-----------------|-------|-------|--------|
| omitted (`-1`) | omitted (`-1`) | empty | `1.0` | Fully opaque |
| `0.5` | omitted | empty | `0.5` | Half transparent |
| omitted | `0.8` | empty | `0.8` | Slight global fade |
| omitted | omitted | `0.4` (pushed) | `0.4` | Region fade |
| `0.5` | `0.8` | `0.4` | `0.16` | All three compound |
| `0.0` | any | any | `0.0` | Fully transparent |

Because the layers multiply, setting all three to moderate values produces
a more transparent result than any one alone. For example,
`0.5 * 0.5 * 0.5 = 0.125`.

---

## Configuration

### Sentinel Values

| Location | Sentinel | Meaning |
|----------|----------|---------|
| Widget `.opacity` | `< 0` (default `-1`) | Unset, treated as `1.0` |
| `WLX_Theme.opacity` | `< 0` (default `-1`) | Unset, treated as `1.0` |
| Context stack | empty stack | `wlx_get_opacity` returns `1.0` |

### Backend Requirements

None. Both the Raylib and SDL3 backends already pass `WLX_Color.a` through
to the renderer. Alpha blending works out of the box.

---

## Edge Cases

- **Zero opacity** (`.opacity = 0.0f`): the widget is drawn fully
  transparent. It still occupies layout space and can still receive
  interaction (hover, click). To skip interaction, do not draw the widget.
- **Negative values**: any negative value is treated as the "unset" sentinel
  and resolves to `1.0`.
- **Values above 1.0**: not clamped. Values greater than `1.0` behave the
  same as `1.0` because alpha is clamped to `[0, 255]` at the colour level.
- **Unmatched push/pop**: `wlx_pop_opacity` asserts that the stack is
  non-empty. An unmatched pop will trigger an assertion failure in debug
  builds. Unmatched pushes are harmless within a single frame because the
  stack resets in `wlx_begin`.
- **`wlx_widget()` (raw rectangle)**: the low-level `wlx_widget` function
  takes a raw `WLX_Color` and does not go through the resolve pipeline.
  To apply opacity to a raw widget, reduce the colour alpha manually or use
  `wlx_color_apply_opacity`:
  ```c
  WLX_Color c = wlx_color_apply_opacity(MY_COLOR, 0.5f);
  wlx_widget(ctx, c);
  ```

---

## Examples

### Disabled-looking button

```c
wlx_button(ctx, "Disabled", .opacity = 0.4f, .height = 36);
```

### Global UI transparency setting

```c
WLX_Theme theme = wlx_theme_dark;
// ... in your frame loop:
wlx_slider(ctx, "UI Opacity:", &theme.opacity,
    .min_value = 0.0f, .max_value = 1.0f);
```

### Faded overlay panel

```c
wlx_push_opacity(ctx, 0.5f);

    wlx_layout_begin(ctx, 4, WLX_VERT, .padding = 8);
        wlx_label(ctx, "Overlay Panel", .font_size = 18);
        wlx_button(ctx, "Action A", .height = 32);
        wlx_button(ctx, "Action B", .height = 32);
        wlx_button(ctx, "Close", .height = 32, .opacity = 1.0f);  // fully opaque
    wlx_layout_end(ctx);

wlx_pop_opacity(ctx);
```

In this example the close button overrides the stack fade because its
per-widget opacity is `1.0`. The final opacity for the close button is
`1.0 * theme * 1.0` (only the theme layer applies, if set).

Note: the per-widget `.opacity = 1.0f` only overrides the widget layer. The
context stack and theme layers still multiply in. To make a widget truly
immune to all layers, you would also need to ensure `theme.opacity` is `1.0`
and pop the context stack.

---

## Limitations

- Opacity does not affect layout. A fully transparent widget still occupies
  its slot and receives input events.
- There is no built-in `hidden` flag that removes a widget from both layout
  and interaction. This is planned for a future release.
- Deeply nested context stack pushes compound multiplicatively, which can
  result in near-zero opacity faster than expected. There is no automatic
  clamping of the running product.
- `wlx_scroll_panel_begin` does not accept an opacity field directly. Wrap
  it in `wlx_push_opacity` / `wlx_pop_opacity` to fade a scroll panel
  region.
