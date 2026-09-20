# Widget Reference

Complete reference for every widget in the library. Each section includes a
minimal code example, a full option-field table, and common override patterns.

> **Convention:** every widget is a macro that wraps a `_impl` function.  
> Options use C99 designated initializers — any field you omit gets a sensible
> default from the active theme. Pass overrides as trailing arguments:
>
> ```c
> wlx_button(ctx, "OK", .font_size = 24, .back_color = RED);
> ```

---

## Shared option fields

All leaf-widget option structs share placement and sizing fields. Many also
reuse the typography, text-color, and border groups below, so those defaults
are documented once here.

### Placement fields (`WLX_LAYOUT_SLOT_FIELDS`)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `pos` | `int` | `WLX_UNSET` | Target slot index in the parent layout. unset = next sequential slot |
| `span` | `size_t` | `1` | Number of consecutive slots to occupy |
| `overflow` | `bool` | `false` | Allow the widget rect to exceed slot bounds using explicit width/height |
| `padding` | `float` | `0` | Uniform inset applied to the slot before the widget uses it |

### Sizing fields (`WLX_WIDGET_SIZING_FIELDS`)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `slot_align` | `WLX_Align` | `WLX_LEFT` | Where to place the widget rect inside its slot when it is smaller than the slot. `widget_align` is the deprecated pre-0.9 name (same storage, removed in the first minor after 0.9) |
| `width` | `float` | `WLX_UNSET` | Widget width in pixels. unset = fill parent width |
| `height` | `float` | `WLX_UNSET` | Widget height in pixels. unset = fill parent height |
| `min_width` | `float` | `0` | Minimum width constraint. `0` = unconstrained |
| `min_height` | `float` | `0` | Minimum height constraint. `0` = unconstrained |
| `max_width` | `float` | `0` | Maximum width constraint. `0` = unconstrained |
| `max_height` | `float` | `0` | Maximum height constraint. `0` = unconstrained |
| `opacity` | `float` | `WLX_UNSET` | Opacity multiplier. Negative = inherit theme and opacity stack; `0.0`–`1.0` = explicit alpha |

### State fields (`WLX_WIDGET_STATE_FIELDS`)

Embedded by every interactive widget (`wlx_button`, `wlx_checkbox`,
`wlx_inputbox`, `wlx_slider`, `wlx_toggle`, `wlx_radio`). Compound and
non-interactive widgets (`wlx_label`, `wlx_panel`, `wlx_split`,
`wlx_scroll_panel`) do not currently expose this field.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `disabled` | `bool` | `false` | When `true`, the widget skips active-state interaction (click/press/focus/drag), suppresses hover tint, shifts its resolved colors by `theme->disabled_brightness`, and multiplies its effective opacity by `theme->disabled_opacity`. Hover is still reported for tooltip anchoring. The same flag is mirrored on `WLX_Interaction.disabled` so widget impls can branch consistently. |

### Typography fields (`WLX_TEXT_TYPOGRAPHY_FIELDS`)

Used by `label`, `button`, `checkbox`, `inputbox`, `toggle`, and `radio`.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `font` | `WLX_Font` | `WLX_FONT_DEFAULT` | Font handle. `0` = backend default / theme font |
| `font_size` | `int` | `0` | Font size in pixels. `0` = use theme default |
| `content_align` | `WLX_Align` | `WLX_LEFT` | Text alignment within the widget rect. `align` is the deprecated pre-0.9 name (same storage, removed in the first minor after 0.9) |
| `wrap` | `bool` | varies | Enable fitted multi-line wrapping for long text. Width wrapping breaks at word boundaries: after the last space or tab that fits the row, inside a word only when it is wider than the row, with overflowing whitespace hanging on its row and left out of the row's alignment width; explicit `\n`, `\r\n`, and `\r` always break lines |
| `spacing` | `int` | `0` | Opt-in extra tracking. `0` = natural backend spacing |

Fitted text is measured and drawn as whole visible lines/runs. Horizontal
alignment is applied per visual line, and wrapped inputbox cursors use the
same fitted line layout.

Text spacing is backend-dependent. Raylib honors nonzero `.spacing` in both
draw and measure. SDL3 custom-font spacing remains gated in this slice: the
installed SDL_ttf exposes `TTF_SetFontCharSpacing`, but the backend still
keeps natural spacing until safe font-variant handling and explicit SDL3
spacing support land. SDL3 debug font and WASM ignore spacing in both draw
and measure.

For backend authors, `draw_text_slice` and `measure_text_slice` are the
preferred text callbacks. Widgets measure and draw through byte spans
internally; legacy `draw_text` and `measure_text` remain compatibility
fallbacks for downstream backends that still expect NUL-terminated text.

### Color fields (`WLX_TEXT_COLOR_FIELDS`)

Used by `label`, `button`, `checkbox`, `inputbox`, `toggle`, and `radio`.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `front_color` | `WLX_Color` | `{0}` | Text/foreground color. `{0}` = use theme foreground |
| `back_color` | `WLX_Color` | `{0}` | Background fill color. `{0}` = use theme surface |

### Border fields (`WLX_BORDER_FIELDS`)

Used by `widget`, `label`, `button`, `inputbox`, `slider`, `progress`,
`toggle`, and `scroll panels`. `checkbox` exposes the same field names
directly even though it does not use the macro.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `border_color` | `WLX_Color` | `{0}` | Border color. `{0}` = theme or widget-specific border fallback |
| `border_width` | `float` | `WLX_UNSET` | Border width. unset = theme/widget default, `0` = no border |
| `roundness` | `float` | `WLX_UNSET` | Corner roundness (fraction of the shorter side). unset = theme default |
| `corner_radius` | `float` | `0` | Absolute corner radius in **pixels**. `> 0` overrides `roundness`; `0` = unset |
| `rounded_segments` | `int` | `WLX_UNSET` | Segment count for rounded drawing. unset = theme default |
| `rounded_corners` | `int` | `0` | `WLX_CORNERS_*` mask selecting which corners use the radius. `0` = all four |
| `border_color_top` / `_right` / `_bottom` / `_left` | `WLX_Color` | `{0}` | Per-side border color. `{0}` inherits `border_color` |
| `border_width_top` / `_right` / `_bottom` / `_left` | `float` | `WLX_UNSET` | Per-side border width. `< 0` inherits `border_width`; `0` switches that edge off |

#### Absolute corner radius (`corner_radius`)

`roundness` is a fraction of the element's shorter side, so a single constant
rounds differently sized elements by different pixel amounts. `corner_radius`
expresses the radius in **pixels** instead, for design systems that specify a
fixed corner (e.g. an 8 px `rounded-lg`). It is resolved centrally at draw time
against the element's final rect using `clamp(2 * corner_radius / min(w,h), 0, 1)`,
so two differently sized panels with the same `corner_radius` get the same pixel
corner.

Precedence and sentinel:

- `corner_radius > 0` -> pixel mode: the resolved fraction replaces `roundness`
  (and the widget `WLX_UNSET` theme-roundness fallback) for that element.
- `corner_radius == 0` (default) -> unset: current behavior exactly; `roundness`
  (or its theme fallback) drives rounding. A literal sharp corner stays
  `roundness = 0`; `0` here only gates the pixel feature off.
- A `corner_radius` larger than `min(w,h)/2` clamps to a full pill/capsule
  (fraction `1.0`) and never overshoots.

#### Per-corner rounding (`rounded_corners`)

`corner_radius` / `roundness` set the radius; `rounded_corners` selects **which**
corners use it. It is a bitmask of `WLX_CORNER_TOP_LEFT`, `WLX_CORNER_TOP_RIGHT`,
`WLX_CORNER_BOTTOM_RIGHT`, `WLX_CORNER_BOTTOM_LEFT`, with the convenience masks
`WLX_CORNERS_ALL` / `_TOP` / `_BOTTOM` / `_LEFT` / `_RIGHT`. A flush header band on
a rounded panel, for example, uses `rounded_corners = WLX_CORNERS_TOP` so the top
follows the panel and the bottom stays square against the body.

- `rounded_corners == 0` (default) -> all four corners rounded; output is identical
  to before the feature existed.
- Corners absent from the mask are squared off by overdrawing their corner box with
  the fill color (no backend change). This applies to the **solid fill only**;
  gradient fills and rounded **borders** stay uniform. For an edge accent on a
  squared corner, use the per-side straight border (`border_*_bottom`, etc.).

corner_radius` is honored on everything drawn through the shared
decor path: containers (`wlx_layout_begin`, grids), panels, scroll panels, and
the rectangular chrome of `widget`, `label`, `button`, and `inputbox`. It is
accepted but **ignored** on widget primitives that draw their own rounded
shapes - `checkbox`, `slider`, `progress`, `scrollbar`, and `radio` - which keep
their fractional `roundness` (mostly circular) behavior. Only the fill follows
the pixel radius; per-side (two-tone) borders stay sharp spans (see below).

#### Per-side borders

The eight per-side fields express a two-tone bevel (per-side color) or a
single-edge accent bar (per-side width) without raw command emitters. Sentinels:
a per-side color `{0}` inherits the uniform `border_color`; a per-side width
`< 0` inherits `border_width`; an explicit `0` switches that edge off (express a
transparent edge with width `0`, not a zero color); a width `> 0` wins.

When all four edges resolve equal, the border draws through the uniform
rounded/sharp path (no behavior change). When any edge differs, the fill draws
as usual and each active edge draws as an independent **sharp** rectangle span —
per-side strokes never follow corner roundness, so a rounded fill may show small
corner gaps under a bevel or bar.

Per-side colors receive the **same hover tint and disabled treatment** as the
uniform `border_color`, so a beveled or single-edge widget keeps its interaction
feedback. Per-side fields apply only to a widget's **rectangular bounding
chrome**: pill/ring/circular indicators (the toggle thumb track, the radio ring,
and the circular checkbox mark) keep their existing non-rectangular draw path
and ignore these fields. `slider` and `toggle` draw no rectangular chrome
through the shared border path, so the fields are inert there.

### Glow and shadow fields (`WLX_GLOW_FIELDS` / `WLX_SHADOW_FIELDS`)

A soft outer glow and a soft drop shadow are first-class decoration fields. They
ride on the same `WLX_BORDER_FIELDS` group as the border (and on the matching
container decor group), so any widget or container that already carries those
fields can request an effect through options alone — no raw command emitters.
Both effects draw **before** (behind) the element's fill, share its corner
roundness, and are clipped by whatever scissor the element rides under.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `shadow_color` | `WLX_Color` | `{0}` | Shadow color. `{0}` disables the shadow (enable gate). |
| `shadow_offset_x` | `float` | `0` | Shadow shift right, in px. A literal `0` means no horizontal shift. |
| `shadow_offset_y` | `float` | `0` | Shadow shift down, in px. A literal `0` means no vertical shift. |
| `shadow_blur` | `float` | `0` | Approx blur radius (px). `<= 0` resolves to the theme value, then `8`. Consumed by a native `draw_shadow` callback only; the software fallback does not grow the shadow by `blur` this cut. |
| `shadow_layers` | `int` | `0` | Software-fallback layer count. `0` resolves to the theme value, then `4`. |
| `glow_color` | `WLX_Color` | `{0}` | Glow color. `{0}` disables the glow (enable gate). |
| `glow_spread` | `float` | `0` | Outward growth per ring (px). `<= 0` resolves to the theme value, then `4`. |
| `glow_rings` | `int` | `0` | Software-fallback ring count. `0` resolves to the theme value, then `3`. |

**Enable gate.** An effect is emitted only when its color is non-zero
(`shadow_color` / `glow_color`). All fields zero-initialize, so a widget that
sets neither color is bit-for-bit unchanged and pays no cost.

**Numeric defaults.** Each numeric knob (`shadow_blur`, `shadow_layers`,
`glow_spread`, `glow_rings`) resolves `<= 0` / `== 0` to the matching
[`WLX_Theme.shadow` / `WLX_Theme.glow`](API_REFERENCE.md) sub-struct value, then
to a hard fallback (`8` px blur, `4` layers, `4` px spread, `3` rings). The
built-in presets leave these zero, so the hard fallbacks are active by default.

**Rendering.** When the backend provides the optional `draw_shadow` /
`draw_glow` callback the effect is one native call; otherwise a software
fallback draws the shadow as `layers` translucent offset rounded rects (each
fainter, the most-offset layer most opaque) and the glow as `rings` concentric
rounded outlines expanding by `glow_spread` per ring (innermost strongest). All
in-tree backends use the fallback this cut. Effect colors flow through the same
effective-opacity premultiply as the fill (see [OPACITY.md](OPACITY.md)).

**Applicability.** Emission is wired at the single full-element chrome box of
`widget`, `label`, `button`, `checkbox`, `inputbox`, and the scroll panel, plus
the layout/grid container decor path. `slider`, `progress`, and `toggle` have no
single full-element chrome box, so they decorate the semantically active
sub-rect instead: the slider and toggle render the effect behind the **thumb**,
continuous `progress` renders it behind the **track**, and segmented `progress`
renders it behind the **bounding run of filled cells** (only when at least one
cell is filled). `wlx_panel` declares its decor fields explicitly (not via the
macros) and so carries no effect fields.

```c
// A button with a soft accent glow and a drop shadow.
wlx_button(ctx, "Launch", .height = 44,
    .glow_color   = WLX_RGBA(80, 160, 255, 120),  // {0} would disable the glow
    .glow_spread  = 5,                             // 0 -> theme, then 4
    .glow_rings   = 3,
    .shadow_color = WLX_RGBA(0, 0, 0, 90),
    .shadow_offset_y = 4,                          // 4 px down, no x shift
    .shadow_layers   = 4);                         // 0 -> theme, then 4
```

### Gradient fill fields (`WLX_GRADIENT_FIELDS`)

A vertical two-stop gradient fill. Like glow/shadow it rides on the same
`WLX_BORDER_FIELDS` group (and the container decor group), so any widget or
container can request it through options alone. The gradient draws **in place of**
the solid fill — after any glow/shadow, before the border — and shares the
element's corner roundness and scissor.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `gradient_top` | `WLX_Color` | `{0}` | Top stop. `{0}` disables the gradient (enable gate); the element renders its solid `back_color` / `color` fill instead. |
| `gradient_bottom` | `WLX_Color` | `{0}` | Bottom stop. `{0}` is treated as equal to `gradient_top`, giving a uniform fill through the gradient path. |

**Enable gate.** The gradient is emitted only when `gradient_top` is non-zero.
All fields zero-initialize, so an element that sets neither stop is bit-for-bit
unchanged and pays no cost. When active, the gradient replaces the solid fill;
the border still draws on top.

**Rendering.** When the backend provides the optional `draw_gradient_v` callback
the gradient is one native call (Raylib uses `DrawRectangleGradientEx` for sharp
rects); otherwise — and for rounded rects on Raylib — a software fallback slices
the rect into `max(1, rect.h / 4)` solid bands interpolating the two stops. The
band approximation is intentional; SDL3 and WASM render through it.
Gradient stops flow through the same effective-opacity premultiply as the fill on
the widget path (see [OPACITY.md](OPACITY.md)).

```c
// A panel container filled with a deep cyan -> dark vertical gradient.
wlx_layout_begin(ctx, 1, WLX_VERT,
    .gradient_top    = WLX_RGBA(0, 209, 255, 60),
    .gradient_bottom = WLX_RGBA(10, 15, 20, 255),
    .roundness = 0.06f, .rounded_segments = 8,
    .border_color = WLX_RGBA(255, 255, 255, 13), .border_width = 1.0f);
wlx_layout_end(ctx);
```

### Content padding (`WLX_CONTENT_PADDING_FIELDS`)

Used by `label`, `button`, `tooltip`, `split`, `panel`, `inputbox` and `editor` to inset the
widget's *inner content* independently from outer slot margin. Chrome
(background, border) and the interaction hit rect always stay at the full
widget rect.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `content_padding` | `float` | `WLX_UNSET` | Uniform inner inset. Unset resolves to `0` for leaf widgets and to the compound widget's own default (see table below). Pass `WLX_PADDING_USE_THEME` to opt into the theme's `padding` knob. |
| `content_padding_top` | `float` | `WLX_UNSET` | Top-side override. `< 0` falls back to `content_padding`. |
| `content_padding_right` | `float` | `WLX_UNSET` | Right-side override. `< 0` falls back to `content_padding`. |
| `content_padding_bottom` | `float` | `WLX_UNSET` | Bottom-side override. `< 0` falls back to `content_padding`. |
| `content_padding_left` | `float` | `WLX_UNSET` | Left-side override. `< 0` falls back to `content_padding`. |

Per-widget uniform defaults:

| Widget | `content_padding` default | Resolved to |
|--------|--------------------------|-------------|
| `wlx_label` | `WLX_UNSET` | `0` (no inset) |
| `wlx_button` | `WLX_UNSET` | `0` (no inset) |
| `wlx_checkbox` | `WLX_UNSET` | `0` (no inset) |
| `wlx_radio` | `WLX_UNSET` | `0` (no inset) |
| `wlx_toggle` | `WLX_UNSET` | `0` (no inset) |
| `wlx_slider` | `WLX_UNSET` | `0` (no inset) |
| `wlx_progress` | `WLX_UNSET` | `0` (no inset) |
| `wlx_tooltip_for` | `WLX_UNSET` | `6 px` all sides (`WLX_TOOLTIP_CONTENT_PADDING`) |
| `wlx_split_begin` | `WLX_UNSET` | `4 px` all sides (`WLX_SPLIT_CONTENT_PADDING`) |
| `wlx_panel_begin` | `WLX_UNSET` | `2 px` all sides (`WLX_PANEL_CONTENT_PADDING`) |
| `wlx_inputbox`, `wlx_editor` | `WLX_UNSET` | `10 px` outer gutter (all sides, `WLX_INPUTBOX_CONTENT_PADDING`) |

Resolution rules (same for every consumer):
- A per-side value `>= 0` wins unconditionally.
- Otherwise the side falls back to the uniform `content_padding`.
- An unset uniform (`WLX_UNSET`, the default) resolves to `0` for leaf
  widgets and to the compound widget's own default; writing `WLX_UNSET`
  explicitly means the same as omitting the field.
- `WLX_PADDING_USE_THEME` on the uniform (`-2.0f`) resolves all
  still-unset sides to the theme's `padding` knob
  (`WLX_STYLE_CONTENT_PADDING` by default).
- On tight rects the resolved padding is clamped proportionally so the
  content rect never has negative dimensions.

#### Defaults without designated initializers

Every option struct has a by-value defaults function,
`wlx_<widget>_opt_defaults()`, returning exactly what the widget's macro
installs. Callers that cannot use the defaults-then-overrides macro (a
C++ translation unit, a table-driven builder) take the defaults, assign,
and call the `_impl` function:

```c
WLX_Label_Opt o = wlx_label_opt_defaults();
o.font_size = 18;
o.border_width = 0;
wlx_label_impl(ctx, "Title", o, __FILE__, __LINE__);
```

**Inputbox-specific note:** `wlx_inputbox` uses `WLX_CONTENT_PADDING_FIELDS`
for the outer gutter (label area, input rect position, and vertical centering).
The text rect *inside* the editing box is additionally inset by the fixed
constant `WLX_INPUTBOX_TEXT_INSET` (5 px) on the x-axis only. This value
equals the pre-change default of `content_padding / 2` with `content_padding
= 10`, preserving byte-identical default visuals.

**Adding a new chrome widget:** any widget that insets inner content must
embed `WLX_CONTENT_PADDING_FIELDS` in its opt struct and resolve padding with
`wlx_resolve_content_padding`. Do not add a bespoke `padding` scalar — use
this shared shape so theme integration and per-side overrides work uniformly.

### Identity field

All leaf-widget opt structs include an optional identity field:

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `const char *` | `NULL` | An explicit string ID. When non-NULL, the string is hashed and pushed onto the ID stack for the duration of the widget call, giving it a stable identity independent of call-site order. Useful for widgets created in loops or dynamic lists. Compound widgets that keep their body open document their scope-style `id` behavior in their own sections. See [LAYOUT_MODEL § Explicit String IDs](LAYOUT_MODEL.md) for details. |

---

## Intrinsic widths (CONTENT slots in HORZ layouts)

A `WLX_SLOT_CONTENT` slot in a horizontal linear layout sizes from its
children's widths. An explicit `.width` always wins; otherwise each widget
offers its intrinsic (natural) width — always the single-line, unwrapped
measure, even when `.wrap` is set (the fit slot renders the text unwrapped;
combined with `WLX_SLOT_CONTENT_MAX` the text wraps inside the clamp while
the measurement stays stable):

| Widget | Intrinsic width |
|---|---|
| `wlx_label`, `wlx_button` | single-line measured text + resolved left/right content padding + the image band the face draws (`image_size`, else the auto band `font_size * 1.5` when text is present, else `texture_src` / texture width for image-only faces; + `image_text_gap` beside text; `TOP`/`BOTTOM` placements take the wider of image and text) |
| `wlx_checkbox`, `wlx_toggle`, `wlx_radio` | glyph box + label gap + single-line label measure + padding (checkbox uses `font_size` as the glyph's upper bound) |
| `wlx_dropdown` | the widest of the label and every option (single-line measures) + resolved left/right content padding, so the face does not resize when the selection changes |
| `wlx_menu_button_begin` | single-line measured label + resolved left/right content padding (the face; the list takes `menu_width`) |
| `wlx_image` | source sub-rect width, else texture width |
| `wlx_separator` | its `thickness` (it renders as a vertical divider) |
| `wlx_slider`, `wlx_progress`, `wlx_inputbox`, `wlx_textarea`, `wlx_editor`, `wlx_scroll_panel`, `wlx_widget` | none — these fill their slot; explicit `.width` or the slot's `MIN` clamp only |
| nested layouts | none — place the width-defining widget directly in the slot, or give the slot an explicit size or `MIN` clamp |

The measured width settles one frame after content changes (use
`WLX_SLOT_CONTENT_MIN` to avoid the first-frame pop, exactly as with
vertical CONTENT slots), and on the measuring frame the deferred replay
corrects drawn positions — HORZ CONTENT gets the same same-frame treatment
VERT layouts get vertically, scissors included. The intrinsic measure runs
only when the widget actually sits in a width-consuming slot, so layouts
that do not use the feature perform no extra text measurement.

## Keyboard operation

Every operable widget can be reached and driven without a pointer.

**Tab / Shift-Tab** move the keyboard focus ring through the widgets in
declaration order (wrapping at both ends): buttons, checkboxes, toggles,
radios, dropdowns, menu buttons, inputboxes, textareas, and the editor are
Tab stops; sliders, separators, labels, progress bars, disabled widgets, and
widgets whose hit zone is clipped away entirely (scrolled out of a panel or
cropped by a `.clip` layout) are skipped. When a popup (menu, dropdown list)
is open, Tab cycles only inside it. While a widget holds the ring, an accent
outline is drawn around its visible part, kept inside the viewport or `.clip`
layout that crops it (`theme->accent`; thickness and gap are
`WLX_FOCUS_RING_THICKNESS` / `WLX_FOCUS_RING_GAP`, overridable before
include).

**Enter / Space** activate the focused button-like widget exactly like a
click (`wlx_button` returns `true`, a checkbox flips, a dropdown opens).
Hovering a widget and pressing Enter/Space still works as before.

**Inputbox / textarea.** Tabbing onto a field focuses it for typing; Tab
again moves on (the field never swallows Tab - a form should stay
traversable). Enter in a single-line field blurs it without activating the
next widget; Escape blurs it and keeps the ring on it, so a further Tab
continues from there.

**Editor.** A focused editor keeps Tab for itself: Tab inserts `\t` with
tab-stop expansion. To leave it by keyboard, press Escape (blurs the
editor, ring stays) and then Tab. The Tab that moves the ring *onto* the
editor focuses it without inserting.

**Leaving keyboard mode.** Any pointer press drops the ring (so the outline
never shows in mouse-driven sessions); Escape drops it when no field is
focused.

Read the focused widget's id with `wlx_focused_id(ctx)` and compare it with
`WLX_Interaction.id` (for example to scroll the focused control into view).
Custom widgets built on `wlx_get_interaction` join the ring automatically
when they query `WLX_INTERACT_FOCUS` or `WLX_INTERACT_CLICK |
WLX_INTERACT_KEYBOARD`; add `WLX_INTERACT_TAB_SKIP` to opt out, or
`WLX_INTERACT_FOCUS_HOLD_TAB` to keep Tab while focused.

## Disabled state

Wollix exposes a single first-class disabled-state contract that covers
both interaction gating and visual treatment.

### Coverage matrix

The coverage rule is structural: any widget that calls
`wlx_get_interaction*` and reports `clicked`, `pressed`, `focused`, or
`active` to the caller carries `.disabled`. Purely decorative widgets do
not.

| Widget | `.disabled` field | Notes |
|--------|-------------------|-------|
| `wlx_button` | yes | Reports `clicked`; gates click/focus when disabled |
| `wlx_checkbox` | yes | Reports `clicked`; gates click when disabled |
| `wlx_inputbox` | yes | Reports text changes (focus via `.out_focused`); gates focus when disabled |
| `wlx_slider` | yes | Reports drag / `active`; gates drag when disabled |
| `wlx_toggle` | yes | Reports `clicked`; gates click when disabled |
| `wlx_radio` | yes | Reports `clicked`; gates click when disabled |
| `wlx_widget` | no | Decoration primitive; hover only for tooltip anchoring |
| `wlx_label` | no | Non-interactive text; hover only for tooltip anchoring |
| `wlx_image` | no | Pure visual; no interaction |
| `wlx_separator` | no | Pure visual; no interaction |
| `wlx_progress` | no | Read-only indicator; no interaction |
| `wlx_scroll_panel` | no | Container scroll interaction is not gated |

### Effect when `.disabled = true`

1. **Interaction:** click, focus, drag, and keyboard arbitration are
   suppressed. `WLX_Interaction.clicked`, `.pressed`, `.focused`,
   `.active`, `.just_focused`, and `.just_unfocused` are forced to
   `false`. `.hover` is still reported (so a disabled control can still
   anchor a tooltip), and `.disabled` mirrors the resolved state.
2. **Visual:** the widget's resolved opacity is multiplied by
   `theme->disabled_opacity`, and resolved disabled-state colors are
   shifted by `wlx_color_brightness(..., theme->disabled_brightness)`.
   Hover-tint is automatically suppressed (the
   `(hover && !disabled) ? brightness : c` gate at every interactive
   draw site).

### Theme knobs

| Theme field | Sentinel | Built-in presets |
|-------------|----------|------------------|
| `disabled_brightness` | `WLX_UNSET` (skip) | dark `-0.35f`, light `+0.30f`, glass `-0.25f` |
| `disabled_opacity` | `< 0` (skip multiply) | all presets `0.55f` |

Custom themes that leave both fields at their sentinel render disabled
widgets identically to enabled ones — that is the back-compat fall-back.

### Adding `.disabled` to a new widget

Use the coverage rule above as the test. If the new widget returns a
click/focus/active result and an end-user could meaningfully want it
greyed-out and inert:

1. Embed `WLX_WIDGET_STATE_FIELDS` in the option struct (adds
   `bool disabled`) and `WLX_WIDGET_STATE_DEFAULTS` in the default-opt
   macro.
2. Route the interaction call through
   `wlx_get_interaction_for(ctx, rect, flags, opt.disabled, file, line)`.
3. Use `wlx_color_hover_tint(c, inter.hover, inter.disabled, brightness)`
   for any hover tint; the helper applies the disabled gate.
4. End the resolver with
   `WLX_RESOLVE_VISUAL_STATE(ctx, opt, opt->disabled, /*colors...*/)`
   so disabled-brightness and disabled-opacity flow through the standard
   tail.

---

## `wlx_label`

Static text label. Does not return a value and is non-interactive regardless
of content mode — the only hover behavior is brightening the optional
background fill (when `show_background` is true). Wrapped labels are measured
and drawn as whole visible lines, and explicit newlines preserve empty visual
lines.

The same call supports text-only and text + image content through the same
options. Image-only labels are supported as an edge case; for pure
non-interactive image rendering prefer [`wlx_image`](#wlx_image).

### Signature

```c
void wlx_label(WLX_Context *ctx, const char *text, ...options);
```

### Minimal example

```c
wlx_label(ctx, "Hello, world!",
    .font_size = 20, .content_align = WLX_CENTER
);
```

### Content modes

| Mode | Trigger |
|------|---------|
| Text-only | `text` is non-empty, no `texture` (or `texture.width <= 0`) |
| Text + image | both `texture` valid and `text` non-empty |
| Image-only (edge case) | `texture.width > 0`, `text` is `""` (or `NULL`) — prefer `wlx_image` for pure image content |

Mode is selected purely by the inputs — there is no separate
`wlx_image_label` or `wlx_label_image` function or option struct.

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `show_background` | `bool` | `false` | Draw a filled background rectangle behind the content. When `true`, hover brightens the fill using the theme's `hover_brightness`. |
| `texture` | `WLX_Texture` | zero handle | Optional image content. `width <= 0` or `height <= 0` means no image. |
| `texture_src` | `WLX_Rect` | `{0}` | Source sub-rect. `w <= 0` or `h <= 0` means full texture. |
| `texture_scale` | `WLX_Image_Scale` | `WLX_IMAGE_SCALE_FIT` | How the texture fits its image rect (same modes as `wlx_image`). |
| `texture_tint` | `WLX_Color` | `{0}` | Tint applied to the texture. `{0}` resolves to `WLX_WHITE`. The alpha is multiplied by the opacity stack. |
| `image_placement` | `WLX_Image_Placement` | `WLX_IMAGE_PLACEMENT_LEFT` | Where the image sits relative to text (`LEFT`, `RIGHT`, `TOP`, `BOTTOM`). |
| `image_size` | `float` | `0` | Reserved square size for the image. `<= 0` is automatic: derived from `font_size` for text + image, or the full label rect for image-only. |
| `image_text_gap` | `float` | `WLX_UNSET` | Pixels between image and text. `< 0` resolves to `font_size * 0.5`. |
| `content_padding` | `float` | `WLX_UNSET` | See [Content padding](#content-padding-wlx_content_padding_fields). Default resolves to `0`. |
| `content_padding_top` | `float` | `WLX_UNSET` | Top-side override. See [Content padding](#content-padding-wlx_content_padding_fields). |
| `content_padding_right` | `float` | `WLX_UNSET` | Right-side override. |
| `content_padding_bottom` | `float` | `WLX_UNSET` | Bottom-side override. |
| `content_padding_left` | `float` | `WLX_UNSET` | Left-side override. |
| `style` | `WLX_Text_Style` | `{0}` | Optional aggregate typography (`font`, `font_size`, `color`, `spacing`). When `style.font_size > 0` the aggregate overrides the individual `font`, `font_size`, `spacing`, and `front_color` fields. A `style.color` of all-zero falls back to the resolved `front_color`. Setting both `style.font_size > 0` and `font_size > 0` is a misuse: `style` wins and `WLX_DEBUG` builds emit a once-per-site warning. |
| `vertical_metric` | `WLX_Vertical_Metric` | `WLX_VMETRIC_LINE_HEIGHT` | Vertical centering basis. `WLX_VMETRIC_LINE_HEIGHT` (default) centers using the backend's reported line height. `WLX_VMETRIC_FONT_SIZE` centers using the font size (em box), which produces consistent cap-height placement across backends whose line heights differ from the font size. |

All shared placement, sizing, typography, color, and border fields also apply.
Label content uses the same fitted line/run layout as `wlx_button`, so
centered or wrapped captions align per visible line. See the matching
[`wlx_button` image content section](#wlx_button) for the parallel button
surface — the two widgets share the same image content options and
fitting helpers.

`content_padding*` insets only the content (text + image) rect; chrome and
hit rect stay at the full label rect. See
[Content padding](#content-padding-wlx_content_padding_fields) in the
shared section for full resolution rules and the `WLX_PADDING_USE_THEME`
sentinel.

### Content rules

- **Empty texture + non-empty text** → behaves exactly like a text-only label
  (no texture draw is emitted).
- **Empty texture + empty text** → no content is drawn; the chrome (background
  and/or border) still draws when configured.
- **`content_align` vs. `image_placement`** are independent:
  `image_placement` controls *which side* of the text the image sits on
  (`LEFT`/`RIGHT`/`TOP`/`BOTTOM` of the text within the combined block);
  `content_align` then positions the *combined block* inside the label rect for
  text + image mode, or the image rect itself for image-only mode when
  `image_size > 0`.
- The texture is always centered inside its reserved image rect. Hover
  feedback applies to the optional background only — texture tint is not
  modulated by hover, and labels never intercept clicks.

### Common overrides

Centered heading with background:

```c
wlx_label(ctx, "Settings",
    .font_size = 28,
    .content_align = WLX_CENTER,
    .height = 50,
    .back_color = (WLX_Color){40, 40, 40, 255},
    .show_background = true
);
```

Right-aligned status line:

```c
wlx_label(ctx, status_text,
    .font_size = 14,
    .content_align = WLX_RIGHT,
    .height = 30,
    .front_color = (WLX_Color){150, 150, 150, 255}
);
```

Text + image with default LEFT placement:

```c
wlx_label(ctx, "Saved",
    .height = 32, .font_size = 18, .content_align = WLX_CENTER,
    .texture = check_icon
);
```

Text + image with image stacked on top:

```c
wlx_label(ctx, "Saved",
    .height = 56, .font_size = 14, .content_align = WLX_CENTER,
    .texture = check_icon,
    .image_placement = WLX_IMAGE_PLACEMENT_TOP,
    .image_size = 24, .image_text_gap = 6
);
```

Text + image with `RIGHT` and `BOTTOM` placement:

```c
wlx_label(ctx, "Reminder",
    .height = 32, .font_size = 16, .content_align = WLX_CENTER,
    .texture = info_icon,
    .image_placement = WLX_IMAGE_PLACEMENT_RIGHT,
    .image_size = 20
);

wlx_label(ctx, "Saved",
    .height = 56, .font_size = 14, .content_align = WLX_CENTER,
    .texture = check_icon,
    .image_placement = WLX_IMAGE_PLACEMENT_BOTTOM,
    .image_size = 24, .image_text_gap = 6
);
```

Image-only label (edge case — prefer `wlx_image` for non-interactive image
rendering):

```c
wlx_label(ctx, "",
    .texture = check_icon, .image_size = 32, .content_align = WLX_CENTER
);
```

Tinted icon, fades with the opacity stack:

```c
wlx_push_opacity(ctx, 0.5f);
    wlx_label(ctx, "Saved",
        .texture = check_icon,
        .texture_tint = (WLX_Color){200, 220, 255, 255},
        .image_size = 24, .font_size = 14, .content_align = WLX_CENTER);
wlx_pop_opacity(ctx);
```

Label with a left content inset (chrome stays at full width):

```c
wlx_label(ctx, "padded heading",
    .height = 32, .font_size = 18, .content_align = WLX_LEFT,
    .show_background = true,
    .back_color = (WLX_Color){40, 60, 90, 255},
    .content_padding_left = 32);
```

---

## `wlx_button`

Clickable button. Returns `true` on the frame the user clicks (press then
release while hovering) or activates via keyboard (Space/Enter when hovered).
Supports three content modes through the same call: text-only, image-only,
and image + text.

### Signature

```c
bool wlx_button(WLX_Context *ctx, const char *text, ...options);
```

### Minimal example

```c
if (wlx_button(ctx, "Click me")) {
    printf("Button was clicked!\n");
}
```

### Content modes

| Mode | Trigger |
|------|---------|
| Text-only | `text` is non-empty, no `texture` (or `texture.width <= 0`) |
| Image-only | `texture.width > 0`, `text` is `""` (or `NULL`) |
| Image + text | both `texture` valid and `text` non-empty |

Mode is selected purely by the inputs — there is no separate `wlx_image_button`
function or option struct.

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `hover_brightness` | `float` | `WLX_UNSET` | Per-call hover brightness shift for the fill. Unset uses `theme->hover_brightness`; a negative value darkens (useful when an already-bright accent fill would not visibly brighten). |
| `hover_back_color` | `WLX_Color` | `{0}` | Per-call hover fill replacement. When set, replaces `back_color` while hovered instead of using the brightness path. `{0}` = unset. |
| `texture` | `WLX_Texture` | zero handle | Optional image content. `width <= 0` or `height <= 0` means no image. |
| `texture_src` | `WLX_Rect` | `{0}` | Source sub-rect. `w <= 0` or `h <= 0` means full texture. |
| `texture_scale` | `WLX_Image_Scale` | `WLX_IMAGE_SCALE_FIT` | How the texture fits its image rect (same modes as `wlx_image`). |
| `texture_tint` | `WLX_Color` | `{0}` | Tint applied to the texture. `{0}` resolves to `WLX_WHITE`. The alpha is multiplied by the opacity stack. |
| `image_placement` | `WLX_Image_Placement` | `WLX_IMAGE_PLACEMENT_LEFT` | Where the image sits relative to text (`LEFT`, `RIGHT`, `TOP`, `BOTTOM`). |
| `image_size` | `float` | `0` | Reserved square size for the image. `<= 0` is automatic: derived from `font_size` for image+text, or full button rect for image-only. |
| `image_text_gap` | `float` | `WLX_UNSET` | Pixels between image and text. `< 0` resolves to `font_size * 0.5`. |
| `content_padding` | `float` | `WLX_UNSET` | See [Content padding](#content-padding-wlx_content_padding_fields). Default resolves to `0`. |
| `content_padding_top` | `float` | `WLX_UNSET` | Top-side override. See [Content padding](#content-padding-wlx_content_padding_fields). |
| `content_padding_right` | `float` | `WLX_UNSET` | Right-side override. |
| `content_padding_bottom` | `float` | `WLX_UNSET` | Bottom-side override. |
| `content_padding_left` | `float` | `WLX_UNSET` | Left-side override. |

Shared placement, sizing, typography, color, and border fields also apply.
The button always draws a filled `back_color` rectangle — hover brightens it
automatically using the theme's `hover_brightness`, or the per-call
`hover_brightness` / `hover_back_color` override. Button captions use the
same fitted line/run layout as `wlx_label`, so centered or wrapped captions
align per visible line.

`content_padding*` insets only the content (text + image) rect; chrome and
hit rect stay at the full button rect, so the full clickable area is
preserved. See [Content padding](#content-padding-wlx_content_padding_fields)
in the shared section for full resolution rules.

### Content rules

- **Empty texture + non-empty text** → behaves exactly like a text-only
  button (no texture draw is emitted).
- **Empty texture + empty text** → only the chrome is drawn; the button still
  returns the click contract.
- **`content_align` vs. `image_placement`** are independent:
  `image_placement` controls *which side* of the text the image sits on (image
  is always on the LEFT/RIGHT/TOP/BOTTOM of the text within the combined block);
  `content_align` then positions the *combined block* inside the button rect for
  image+text mode, or the image rect itself for image-only mode when
  `image_size > 0`.
- The texture is always centered inside its reserved image rect; hover
  feedback applies to the chrome background only — texture tint is not
  modulated by hover.

### Common overrides

Text-only button with custom color:

```c
if (wlx_button(ctx, "Submit",
    .slot_align = WLX_CENTER,
    .width = 200, .height = 50,
    .font_size = 22,
    .content_align = WLX_CENTER,
    .back_color = (WLX_Color){128, 0, 0, 255}
)) {
    submit_form();
}
```

Image-only icon button (full button rect as image target):

```c
if (wlx_button(ctx, "",
    .width = 48, .height = 48,
    .texture = save_icon, .content_align = WLX_CENTER
)) {
    save_data();
}
```

Image-only with explicit square size, anchored via `content_align`:

```c
if (wlx_button(ctx, "",
    .height = 48,
    .texture = save_icon, .image_size = 32, .content_align = WLX_CENTER
)) {
    save_data();
}
```

Image + text with default LEFT placement:

```c
if (wlx_button(ctx, "Save",
    .height = 40, .font_size = 18, .content_align = WLX_CENTER,
    .texture = save_icon
)) {
    save_data();
}
```

Image + text with image stacked on top:

```c
if (wlx_button(ctx, "Save",
    .height = 64, .font_size = 14, .content_align = WLX_CENTER,
    .texture = save_icon,
    .image_placement = WLX_IMAGE_PLACEMENT_TOP,
    .image_size = 28, .image_text_gap = 6
)) {
    save_data();
}
```

Tinted icon, fades with the opacity stack:

```c
wlx_push_opacity(ctx, 0.5f);
    wlx_button(ctx, "",
        .texture = save_icon,
        .texture_tint = (WLX_Color){200, 220, 255, 255},
        .image_size = 32, .content_align = WLX_CENTER);
wlx_pop_opacity(ctx);
```

Button with uniform content padding:

```c
wlx_button(ctx, "Save",
    .height = 56, .font_size = 18, .content_align = WLX_CENTER,
    .content_padding = 12);
```

Asymmetric padding for a wide-padded label:

```c
wlx_button(ctx, "Confirm",
    .height = 48, .font_size = 16, .content_align = WLX_CENTER,
    .content_padding_top = 6, .content_padding_bottom = 6,
    .content_padding_left = 20, .content_padding_right = 20);
```

Opt into the theme's `padding` knob (e.g. when a custom theme sets a
consistent inset across all buttons):

```c
wlx_button(ctx, "Continue",
    .height = 44, .font_size = 16, .content_align = WLX_CENTER,
    .content_padding = WLX_PADDING_USE_THEME);
```

---

## `wlx_checkbox`

Toggle checkbox with a text label. Draws a square indicator beside the label
(native mode), or replaces the indicator with a texture per state when both
`tex_checked` and `tex_unchecked` are provided (texture mode). Returns `true`
on the frame the checked state changes.

### Signature

```c
bool wlx_checkbox(WLX_Context *ctx, const char *text, bool *checked, ...options);
```

| Parameter | Description |
|-----------|-------------|
| `text` | Label displayed next to the checkbox |
| `checked` | Pointer to a `bool` — toggled automatically on click |

### Minimal example

```c
static bool dark_mode = false;

if (wlx_checkbox(ctx, "Dark mode", &dark_mode)) {
    printf("Dark mode: %s\n", dark_mode ? "ON" : "OFF");
}
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `full_slot_hit` | `bool` | `true` | Use the full slot rect for hover/click interaction |
| `border_color` | `WLX_Color` | `{0}` | Indicator border color. `{0}` = theme `checkbox.border` |
| `border_width` | `float` | `WLX_UNSET` | Indicator border width. unset = theme `checkbox.border_width` |
| `roundness` | `float` | `WLX_UNSET` | Indicator corner roundness. unset = theme default |
| `rounded_segments` | `int` | `WLX_UNSET` | Segment count for rounded drawing. unset = theme default |
| `check_color` | `WLX_Color` | `{0}` | Checkmark color (native mode only). `{0}` = theme `checkbox.check` |
| `tex_checked` | `WLX_Texture` | zero handle | Texture used for the checked state |
| `tex_unchecked` | `WLX_Texture` | zero handle | Texture used for the unchecked state |
| `tex_checked_src` | `WLX_Rect` | `{0}` | Source rect within `tex_checked`. `{0}` = full texture |
| `tex_unchecked_src` | `WLX_Rect` | `{0}` | Source rect within `tex_unchecked`. `{0}` = full texture |
| `tex_checked_tint` | `WLX_Color` | `{0}` | Tint applied to `tex_checked`. `{0}` = `WLX_WHITE` |
| `tex_unchecked_tint` | `WLX_Color` | `{0}` | Tint applied to `tex_unchecked`. `{0}` = `WLX_WHITE` |
| `content_padding` | `float` | `WLX_UNSET` | Uniform inner inset around the compound content (indicator + label). See [Content padding](#content-padding-wlx_content_padding_fields). Default resolves to `0`. |
| `content_padding_top` | `float` | `WLX_UNSET` | Top-side override. `< 0` falls back to `content_padding`. |
| `content_padding_right` | `float` | `WLX_UNSET` | Right-side override. |
| `content_padding_bottom` | `float` | `WLX_UNSET` | Bottom-side override. |
| `content_padding_left` | `float` | `WLX_UNSET` | Left-side override. |

All shared placement, sizing, typography, and color fields also apply.
Default `wrap` is `false` for checkbox. `.back_color` fills the indicator in
native mode only; it is not used in texture mode.

`content_padding*` insets the compound content rect (indicator glyph + label
together) before alignment. The hit rect honours `full_slot_hit`: when
`true` (the default) clicks still register anywhere in the slot rect; when
`false` the hit rect tracks the inset alignment rect.

### Texture mode

Texture mode activates only when **both** `tex_checked` and `tex_unchecked`
are drawable. If either is missing, the widget falls back to native rendering
for both states; this avoids half-configured checkboxes where one state
silently disappears.

When active, texture mode follows the same contract as `wlx_label`,
`wlx_button`, and `wlx_image`:

- An unset source rect (`w <= 0 || h <= 0`) resolves to the full selected
  texture.
- An unset tint (`{0}`) resolves to `WLX_WHITE`.
- Texture tints participate in opacity resolution alongside other resolved
  colors.
- Hover brightness does **not** modulate texture tint. Hover feedback
  remains a native-mode concern; texture tint is caller-controlled.
- `check_color` only affects native rendering and is ignored in texture mode.
- No native chrome (background, border, checkmark) is drawn while texture
  mode is active — the texture is the full replacement.

### Common overrides

Styled checkbox with custom font size (native mode):

```c
if (wlx_checkbox(ctx, "Enable notifications", &notify_enabled,
    .font_size = 20,
    .check_color = (WLX_Color){0, 200, 0, 255}
)) {
    update_notifications(notify_enabled);
}
```

Texture-backed checkbox with two separate per-state textures (default white
tint draws the textures at their authored colors):

```c
if (wlx_checkbox(ctx, "Favorite", &favorited,
    .tex_checked   = star_filled_tex,
    .tex_unchecked = star_empty_tex
)) {
    printf("Favorited: %s\n", favorited ? "yes" : "no");
}
```

Shared atlas using per-state source rects and semantic tints (one texture,
two cells, two roles):

```c
WLX_Rect src_unchecked = (WLX_Rect){ 0, 0, 64, 64};
WLX_Rect src_checked   = (WLX_Rect){64, 0, 64, 64};

wlx_checkbox(ctx, "Sync over cellular", &sync_enabled,
    .tex_checked        = icon_atlas,
    .tex_unchecked      = icon_atlas,
    .tex_checked_src    = src_checked,
    .tex_unchecked_src  = src_unchecked,
    .tex_checked_tint   = semantic.color_accent,
    .tex_unchecked_tint = semantic.color_border_strong);
```

Texture mode shares the source-rect and tint contract with
[`wlx_label`](#wlx_label), [`wlx_button`](#wlx_button), and
[`wlx_image`](#wlx_image).

Use `wlx_checkbox(..., .tex_checked = ..., .tex_unchecked = ...)` instead of
the removed `wlx_checkbox_tex` compatibility macro.

---

## `wlx_inputbox`

Text input field. Click to focus, type to edit, press Enter or Escape (or
click elsewhere) to unfocus. With `.multiline = true`, Enter instead inserts
a newline and keeps focus — Escape or a click elsewhere leaves the field (see
[Multiline mode](#multiline-mode)). Returns `true` when the buffer text
changed this frame (typed or deleted) — since v0.6; focus state is reported
through the `.out_focused` out-param (the pre-v0.6 return value).

Uses persistent state internally (`WLX_Inputbox_State`) to track cursor
position, selection, and blink timer across frames. The buffer is edited as
one UTF-8 byte buffer, but when `wrap` is `true` the visible text can span
multiple fitted visual lines and the cursor, selection highlight, and mouse
hit test all follow that same wrapped layout.

### Signature

```c
bool wlx_inputbox(WLX_Context *ctx, const char *label, char *buffer, size_t buffer_size, ...options);
```

| Parameter | Description |
|-----------|-------------|
| `label` | Label drawn to the left of the input area |
| `buffer` | Writable `char` array for the current text content |
| `buffer_size` | Total size of `buffer` (must be ≥ 2) |

### Minimal example

```c
static char name[64] = "";

if (wlx_inputbox(ctx, "Name:", name, sizeof(name), .height = 40)) {
    // Field is currently focused — user is typing
}
// name[] is updated in-place as the user types
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `content_padding` | `float` | `10` | Outer gutter inset. See [Content padding](#content-padding-wlx_content_padding_fields). Default `10` applies to all sides. |
| `content_padding_top` | `float` | `WLX_UNSET` | Top-side override. `< 0` falls back to `content_padding`. |
| `content_padding_right` | `float` | `WLX_UNSET` | Right-side override. `< 0` falls back to `content_padding`. |
| `content_padding_bottom` | `float` | `WLX_UNSET` | Bottom-side override. `< 0` falls back to `content_padding`. |
| `content_padding_left` | `float` | `WLX_UNSET` | Left-side override. `< 0` falls back to `content_padding`. |
| `border_color` | `WLX_Color` | `{0}` | Border color in unfocused state. `{0}` = theme `border` |
| `border_width` | `float` | `WLX_UNSET` | Border width. unset = theme `input.border_width`, then theme `border_width` |
| `roundness` | `float` | `WLX_UNSET` | Corner roundness. unset = theme default |
| `rounded_segments` | `int` | `WLX_UNSET` | Segment count for rounded drawing. unset = theme default |
| `border_focus_color` | `WLX_Color` | `{0}` | Border color when focused. `{0}` = theme `input.border_focus` |
| `cursor_color` | `WLX_Color` | `{0}` | Blinking cursor color. `{0}` = theme `input.cursor` |
| `selection_color` | `WLX_Color` | `{0}` | Selection highlight fill. `{0}` = theme `input.selection`, then translucent accent. |
| `password` | `bool` | `false` | Masked field: one `*` per codepoint is rendered while the buffer keeps the plaintext. Forces `wrap = false`; copy/cut are suppressed. |
| `read_only` | `bool` | `false` | Rejects all edits while focus, selection, caret, and copy keep working. Distinct from `disabled` (no interaction lockout, no dimming). |
| `multiline` | `bool` | `false` | Enter inserts a newline and keeps focus; UP/DOWN move the caret by visual line with a sticky column; overflowing content scrolls internally. Excluded by `password`. See [Multiline mode](#multiline-mode). |
| `show_scrollbar` | `bool` | `true` | Draw a draggable vertical scrollbar while multiline content overflows the field. `false` keeps wheel and caret-follow scrolling without the affordance. Inert outside multiline overflow. |
| `revision` | `uint32_t` | `0` | External-mutation guard for the undo history: bump (any change of value) after editing the buffer outside the widget, same length included. A length change is detected on its own. See [Undo and redo](#undo-and-redo). |
| `texture` | `WLX_Texture` | zero handle | Optional icon drawn **inside** the field. `width <= 0` or `height <= 0` means no icon. |
| `texture_src` | `WLX_Rect` | `{0}` | Source sub-rect (e.g. an atlas cell). `w <= 0` or `h <= 0` means full texture. |
| `texture_tint` | `WLX_Color` | `{0}` | Tint applied to the icon. `{0}` resolves to `WLX_WHITE`. |
| `image_placement` | `WLX_Image_Placement` | `WLX_IMAGE_PLACEMENT_LEFT` | Which interior edge the icon sits on. Only `LEFT` and `RIGHT` are meaningful; `TOP`/`BOTTOM` are treated as `LEFT`. |
| `image_size` | `float` | `0` | Reserved square size for the icon. `<= 0` is automatic (derived from `font_size`, clamped to the field interior). |
| `image_text_gap` | `float` | `WLX_UNSET` | Gap between the icon band and the text. `< 0` is font-derived (`font_size * 0.5`). |

All shared placement, sizing, typography, and color fields also apply.
Default `wrap` is `true` for inputbox, so long buffers display over multiple
visual lines inside the widget and cursor placement stays aligned with that
wrapped rendering.

`content_padding*` controls the outer gutter: it positions the label area,
the input rect, and the vertical centering of content. The text cursor rect
*inside* the editing box is additionally inset by the fixed constant
`WLX_INPUTBOX_TEXT_INSET` (5 px) on the x-axis only. With the default
`content_padding = 10` this preserves the pre-migration visual exactly
(`10 / 2 = 5`). The text inset is intentionally fixed — callers that need
asymmetric gutters should use `content_padding_left` / `content_padding_right`
to shift the input rect, not the internal text offset.

### Editing, selection, and clipboard

All editing is UTF-8 codepoint safe (deletes, caret motion, selection
boundaries, and paste truncation never split a multibyte sequence). The
**command modifier** below is Cmd on Apple platforms and Ctrl elsewhere
(`wlx_mod_command_down`).

| Input | Action |
|-------|--------|
| BACKSPACE / DELETE | Delete backward / forward by codepoint; repeats while held |
| LEFT / RIGHT | Move caret by codepoint; repeats while held |
| Ctrl/Alt + LEFT / RIGHT | Move caret by word |
| HOME / END | Jump to the start / end of the caret's **visual line** (follows wrapping) |
| command + HOME / END | Jump to the buffer start / end |
| UP / DOWN | *(multiline only)* Move the caret to the adjacent visual line, keeping a sticky column; repeats while held |
| SHIFT + any caret motion | Extend the selection from the anchor |
| Click / drag | Place the caret / extend the selection |
| Double-click / triple-click | Select word / select all |
| SHIFT + click | Extend the selection to the click point |
| command + A / C / X / V | Select all / copy / cut / paste |
| command + Z | Undo the newest step; repeats while held |
| command + SHIFT + Z, command + Y | Redo the newest undone step; repeats while held |

Typing, paste, BACKSPACE, and DELETE replace a live selection. Copying an
empty selection is a no-op. Paste bypasses the 32-byte per-frame text ring, so
arbitrarily long clipboard content lands in one frame, truncated to
`buffer_size` on a codepoint boundary. The highlight renders in
`selection_color` behind the text, per visual line.

Backend notes: clipboard support comes from the optional
`WLX_Backend.clipboard_get` / `clipboard_set` hooks; when a backend leaves
them `NULL`, copy/cut/paste degrade to safe no-ops. On the bare-WASM backend
the clipboard is a **best-effort cached string**: copy/cut update the browser
clipboard asynchronously via the async Clipboard API, and content copied in
*other* applications only becomes pasteable after a browser paste gesture
(e.g. Ctrl+V) refreshes the cache.

### Undo and redo

Every edit made through the widget (typing, Enter, BACKSPACE/DELETE and
their word variants, cut, paste, typing over a selection) lands in a
per-widget undo journal. command+Z steps back through it and
command+SHIFT+Z or command+Y steps forward; both repeat while held. Each
step restores the bytes and the caret pair exactly, selection included:
undoing a keystroke typed over a selection brings the selection back. A
run of typed characters, a run of BACKSPACE presses or a run of DELETE
presses coalesces into one step; an arrow key, a click that moves the
caret, Enter, paste, cut, a word delete, a selection delete or an undo
itself starts a new one. Any new edit clears the redo history. A
`.read_only` field rejects the chords like any other mutation, and a
`.password` field keeps no journal at all, so plaintext is never retained.

The journal follows the widget id and is bounded per widget by
`WLX_TEXT_UNDO_ENTRIES` (default 512 entries) and `WLX_TEXT_UNDO_BYTES`
(default 256 KB of removed text), whole oldest steps evicted first. A
single step larger than either cap, such as select-all followed by DELETE
on a large document, is still kept and evicts everything older. Both caps
are overridable before including `wollix.h`; `WLX_TEXT_UNDO_ENTRIES 0`
compiles the journal out and turns the chords into no-ops. History is
dropped whenever the buffer changes outside the widget: a length change is
detected automatically, and after a same-length rewrite the caller bumps
`.revision` (any change of value), exactly as for the editor. The textarea
shares all of this; the editor adds Tab and Enter as steps of their own.
The machinery (entries, stacks, transactions, replay, eviction, the guard)
is described in [UNDO_MODEL.md](UNDO_MODEL.md).

### Password and read-only modes

`.password = true` renders one `*` per plaintext codepoint while the buffer
keeps the real text; the field is forced single-line and copy/cut are
suppressed so the plaintext can never leave the widget. Editing, paste, caret
placement, and selection still work — all geometry runs on the masked display
text and maps back to plaintext byte offsets.

`.read_only = true` keeps the field focusable, selectable, and copyable but
rejects every mutation (typing, BACKSPACE/DELETE, cut, paste). Unlike
`.disabled` it does not gate interaction or dim the rendering — use it for
copyable values like IDs or tokens.

```c
static char pw[64] = "";
wlx_inputbox(ctx, "Password:", pw, sizeof(pw), .height = 40, .password = true);

static char token[64] = "wlx-4242-...";
wlx_inputbox(ctx, "API token:", token, sizeof(token), .height = 40, .read_only = true);
```

### Multiline mode

`.multiline = true` turns the field into a plain-text multi-line editor:

- **Enter** deletes a live selection and inserts `"\n"` at the caret; the
  field **keeps focus**, and OS auto-repeat inserts further newlines while
  held. The Enter press is consumed — it never doubles as a keyboard
  activation of another widget in the same frame.
- **Escape** or a **click elsewhere** leaves the field. (Escape-blur applies
  to single-line fields too.)
- **UP / DOWN** move the caret to the adjacent visual line — hard (`\n`) and
  soft (wrapped) lines alike — aiming at a **sticky column**: the first
  vertical move latches the caret x, and later moves keep aiming at it, so
  traversing a shorter line does not lose the column. Any horizontal caret
  change (typing, LEFT/RIGHT, HOME/END, mouse click, paste) drops the latch.
  UP on the first line clamps to the line start, DOWN on the last line to the
  line end. SHIFT extends the selection as usual.
- **Composition:** `.password = true` forces `multiline` off (a masked field
  is always single-line). `.read_only = true` composes: navigation,
  selection, and copy work, Enter keeps focus but the newline insert is
  rejected.
- **Internal scrolling.** Content taller than the field scrolls instead of
  clipping. While overflowing, the run is top-anchored (the vertical
  component of `.content_align` applies again once content fits):
  - **Caret-follow**: any caret move or edit scrolls the view the minimal
    distance that keeps the caret line fully visible — typing at the bottom,
    Enter auto-repeat, UP/DOWN past the edges, HOME/END jumps, and paste all
    follow.
  - **Mouse wheel** scrolls a hovered overflowing field and consumes the
    event, so an enclosing scroll panel does not also scroll (innermost
    scrollable wins, exactly like nested panels). A field whose content fits
    leaves the wheel to the panel.
  - **Scrollbar**: a draggable thumb appears at the field's right edge while
    content overflows (`.show_scrollbar`, default `true`). Pressing or
    dragging it never moves the caret, starts a selection, or blurs the
    field. The thumb is never shorter than 20px: over long content its
    length floors and its travel maps the scroll range onto the remaining
    track.
  - **Drag-select auto-scroll**: dragging a selection past the top or bottom
    edge scrolls toward the pointer (speed grows with the overshoot), so a
    selection can span more than one viewport.

The `wlx_textarea` macro is sugar for a multiline field with a top-left text
anchor; both presets can still be overridden per call:

```c
static char notes[512] = "";
wlx_textarea(ctx, "Notes:", notes, sizeof(notes), .height = 120);
// equivalent to:
// wlx_inputbox(ctx, "Notes:", notes, sizeof(notes),
//     .multiline = true, .content_align = WLX_TOP_LEFT, .height = 120);
```

Current limits: the field is **caller-sized** (`.height`) and does not grow
with content. Multiline geometry (caret, hit-test, selection, scroll, draw)
covers at most `WLX_INPUTBOX_MULTILINE_MAX_LINES` (512) visual lines and
`WLX_INPUTBOX_MULTILINE_MAX_UNITS` (4096) measured codepoints per field —
roughly 4 KB of prose; both are compile-time overridable (`#define` before
including `wollix.h`). Past the budget, text still appends to the buffer but
the caret pins to the end of the last built line and the view cannot scroll
into the unbuilt remainder. Single-line fields keep the smaller global caps
(`WLX_TEXT_RUN_MAX_LINES` / `WLX_TEXT_RUN_MAX_UNITS`). There is no submit
signal — commit on blur (`.out_focused` transition) or an explicit button.

### Inner icon

Set `texture` to render an icon **inside** the field frame on the leading
(`LEFT`, default) or trailing (`RIGHT`) interior edge — a search glyph, a clear
affordance, etc. The icon is centered vertically within the field interior,
independent of `content_align` (a glyph is not text and does not follow a multi-line
text band). The reserved band — `image_size + image_text_gap` wide — insets the
text and caret so they never overlap the icon, and the band width is clamped so
a narrow field never produces a negative-width text band. A zero `texture`
leaves the field text-only with byte-identical geometry to before, so existing
call sites are unaffected. The icon is texture-based (a `WLX_Texture` plus a
source sub-rect and tint), matching the image content of `wlx_label`,
`wlx_button`, and `wlx_checkbox`.

```c
// Search field with a leading magnifier glyph from an atlas, inside the box.
wlx_inputbox(ctx, NULL, query, sizeof(query),
    .height = 34,
    .texture = icon_atlas, .texture_src = search_glyph_src,
    .texture_tint = (WLX_Color){150, 160, 170, 255},
    .image_placement = WLX_IMAGE_PLACEMENT_LEFT,
    .image_size = 18, .image_text_gap = 8);
```

### Common overrides

Form-style input with fixed height:

```c
wlx_inputbox(ctx, "Email:", email_buf, sizeof(email_buf),
    .height = 45,
    .font_size = 18,
    .border_focus_color = (WLX_Color){0, 120, 255, 255}
);
```

Reacting to edits and tracking focus transitions:

```c
static bool was_focused = false;
bool focused = false;
bool changed = wlx_inputbox(ctx, "Search:", query, sizeof(query),
    .height = 35, .out_focused = &focused);

if (changed) {
    update_search_results(query);   // text mutated this frame
}
if (focused && !was_focused) {
    printf("Search field gained focus\n");
}
if (!focused && was_focused) {
    printf("Search submitted: %s\n", query);
}
was_focused = focused;
```

---

## `wlx_editor`

The editor ships in the companion header `wollix_editor.h` — include it
after `wollix.h` (and any backend adapter) in every translation unit
that uses it:

```c
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_raylib.h"   // any backend adapter, if used
#include "wollix_editor.h"
```

The editor's conceptual model (windowed builds, retained geometry, the
windowed origin, wrapped rows) is documented in
[EDITOR_MODEL.md](EDITOR_MODEL.md); this section covers the widget API.

Windowed text editor over a **caller-owned flat buffer** with an explicit
length in/out. Only the visible window of lines is measured, built, and
drawn each frame, so frame cost is O(viewport) regardless of the document
size — the acceptance envelope is a 10 MB / 1,000,000-line document with no
O(document) work on idle frames. Non-wrapping by default (one visual line
per hard line, horizontal scrolling for long lines); `.wrap = true` breaks
hard lines into band-wide rows instead. Returns `true` when the text
changed this frame; focus is reported through `.out_focused`.

Use `wlx_textarea` for note-sized fields; use `wlx_editor` for code/log/
prose-sized documents that need line numbers and document-scale
performance — unwrapped for code and data, `.wrap` for prose and logs with
long messages.

One deliberate behavior asymmetry between the two: the editor **handles
Tab** — the key inserts a literal `\t` and every `\t` renders with
next-tab-stop expansion (`.tab_columns`), and the user leaves the editor
by keyboard with Escape, then Tab — while the multiline inputbox does
neither: its Tab moves the keyboard focus to the next widget (a form field
should not swallow the key; see [Keyboard operation](#keyboard-operation)),
and any `\t` already in the buffer is measured as whatever glyph the backend
gives it. Everything else in the editing vocabulary (clipboard, select-all,
word motion and word deletes, sticky-column UP/DOWN) is shared and behaves
identically.

### Signature

```c
bool wlx_editor(WLX_Context *ctx, const char *label, char *buffer,
                size_t buffer_cap, size_t *length, ...options);
```

| Parameter | Description |
|-----------|-------------|
| `label` | Optional label drawn to the left of the editor area (`NULL` = none) |
| `buffer` | Writable byte buffer holding the document (need not be NUL-terminated) |
| `buffer_cap` | Total capacity of `buffer`; bounds every insert |
| `length` | In/out: the authoritative document length in bytes |

### Buffer and length contract

- `*length` is authoritative in and out; the widget never reads past it and
  writes the new length back after edits.
- Inserts truncate at `buffer_cap` on a UTF-8 boundary, so only whole
  codepoints land; a full buffer rejects input rather than splitting a
  codepoint.
- The widget maintains a trailing NUL **opportunistically** when
  `*length < buffer_cap`; the NUL is a convenience, not part of the
  contract.
- After mutating the buffer **outside** the widget, bump `.revision` (any
  change of value) so the internal line index rebuilds and the undo history
  is dropped. Length changes are detected automatically, and a sampled
  hard-line-start probe catches most same-length mutations (one that keeps
  every hard-line start in place, such as any rewrite inside a one-line
  document, is invisible to it), but `.revision` is the reliable signal.

### Minimal example

```c
static char *doc;        // caller-owned, e.g. a loaded file + headroom
static size_t doc_len;   // authoritative length
static uint32_t doc_rev; // bump after external mutations

if (wlx_editor(ctx, NULL, doc, doc_cap, &doc_len, .revision = doc_rev,
               .line_numbers = true)) {
    // text changed this frame; doc_len is already updated
}
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `content_padding` (+ per-side) | `float` | `10` | Outer inset, as on `wlx_inputbox`. |
| `border_color` / `border_focus_color` / `cursor_color` / `selection_color` | `WLX_Color` | `{0}` | Chrome colors with the same theme fallbacks as `wlx_inputbox`. |
| `out_focused` | `bool *` | `NULL` | Receives this frame's focus state. |
| `read_only` | `bool` | `false` | Rejects all edits while focus, caret, selection, and copy keep working. |
| `wrap` | `bool` | `false` | Wrapped mode: hard lines break into rows at the band width, at word boundaries (after the last space or tab that fits; inside a word only when it is wider than the band; overflowing whitespace hangs on its row). Horizontal scrolling disappears (nothing overflows sideways); vertical motion, hit-tests, and caret-follow work in visual rows; the vertical thumb becomes an approximation (see below). Toggleable at runtime — caret and selection are byte offsets and survive the switch. |
| `show_scrollbar` | `bool` | `true` | Draw draggable scrollbars while content overflows. The vertical thumb is **exact** (from the line count) when unwrapped and an approximation under `.wrap`; the horizontal one is proportional to the widest line seen so far (see below). |
| `line_numbers` | `bool` | `false` | Line-number gutter on the leading edge, sized by the digit count of the line total. Gutter presses never touch caret, selection, or focus. |
| `tab_columns` | `int` | `4` | Tab-stop width in space-advance columns: each `\t` advances to the next multiple of `tab_columns * space_advance` in measure, hit-test, caret, selection, and draw. Under `.wrap` the tab grid restarts at each visual row's start, so a tab-heavy wrapped line renders differently than its unwrapped self at the same offset. |
| `revision` | `uint32_t` | `0` | External-mutation guard; bump after editing the buffer outside the widget. |

All shared placement, sizing, typography, and color fields also apply. Text
is always top-left anchored; `content_align` places only the label.

### Editing and navigation vocabulary

Typing, Enter (newline), Tab (literal `\t`), Backspace/Delete (word variants
on Ctrl/Alt), Ctrl/Cmd+C/X/V clipboard, Ctrl/Cmd+A select-all, Ctrl/Cmd+Z
undo and Ctrl/Cmd+Shift+Z or Ctrl/Cmd+Y redo (the inputbox's
[undo journal](#undo-and-redo): the same steps, caps and `.revision`
guard, with Tab and Enter as steps of their own). Arrows with
word motion, HOME/END on the caret's line, Ctrl/Cmd+Home/End to the document
ends, UP/DOWN with a sticky column, PageUp/PageDown move the caret by one
viewport. Mouse: click places the caret, double-click selects the word,
triple-click selects all, dragging extends the selection with auto-scroll on
both axes past the band edges. Focus follows the inputbox contract (click to
focus, Escape or click-elsewhere to blur; Enter never blurs).

Under `.wrap`, UP/DOWN and PageUp/PageDown step **visual rows** with a
row-relative sticky column, and drag auto-scroll is vertical only; HOME/END
keep hard-line semantics. A caret offset exactly at a wrap break belongs to
the row it starts, so the right edge of a wrapped row is not a caret render
position — clicking there places the caret at the next row's start.

### Scrolling model

- The scroll anchor is `(first_line, y_frac)` — resolution-independent, so
  the position survives font or size changes. Under `.wrap` it gains a row
  component (`first_row`), clamped against the line's current row count, so
  band or font changes cost nothing.
- The vertical scrollbar is exact when unwrapped: content height is
  `line_count * line_h` from the line index.
- Thumbs never draw shorter than 20px on either axis: a 30k-line document's
  proportional thumb would be a fraction of a pixel, so the length floors and
  the thumb's travel maps the scroll range onto the remaining track (the
  track end still means the document end, and a held thumb stays under the
  pointer).
- Under `.wrap` the vertical thumb is an **approximation**: it maps hard
  lines (as if nothing wrapped), so it moves at uneven speed through
  heavily wrapped regions and a drag lands on a hard line. It is
  continuous, never snaps, degenerates to exact when nothing wraps, and
  the track end always means the document end (the view bottom-aligns the
  last row exactly). Exact wrapped height would cost an O(document)
  measure and is deliberately not attempted.
- The horizontal range (unwrapped only) is an **approximation**: it tracks
  the widest line measured so far (sticky) and stays open one band past
  the current reach while a visible line is still width-truncated, so long
  lines are always reachable via Shift+wheel, the horizontal thumb, drag
  auto-scroll, or caret-follow. It never shrinks back within a session.
  On lines longer than the unit budget the tracked width rests on the
  measure window's estimated-absolute position (see the performance
  section below), so the thumb keeps mapping continuously at any depth.
- Wheel: consumed only when hovered and overflowing on the wheel's axis
  (innermost scrollable wins; Shift redirects to the horizontal axis, and
  is never consumed under `.wrap` — nothing overflows sideways); otherwise
  the delta is left to enclosing scroll panels. Under `.wrap` the wheel
  moves visual rows, so wrapped regions scroll evenly.

### Performance envelope and per-line cap

- Idle frames run no O(document) work; the only O(document) step is the
  line-index newline scan on the first frame and after each edit (measured
  at ~9 ms for a 10 MB / 1M-line document, single-digit ms per keystroke,
  flat O(viewport) frame cost otherwise).
- Line geometry is **retained across frames** in a bounded per-widget
  store: steady frames (idle, held scroll, parked caret) re-measure
  nothing, typing re-measures only the edited line, scrolling only the
  lines entering the view. On backends implementing the optional
  `measure_text_advances` callback (all three in-tree adapters), a
  line's geometry fills in a few batched calls instead of one backend
  measure per character. These are asserted bounds in
  `make perf-editor`, not tendencies.
- `WLX_EDITOR_MAX_LINE_UNITS` (`#ifndef`-overridable, default 1024
  codepoints) is a **per-record safety cap, not a reach limit**.
  Unwrapped, a line longer than the budget is measured from a window
  near the view, so giant single-line documents (minified code, log
  lines) are editable end-to-end: END on a 300 KB line lands on its
  true end, and frame cost is independent of how deep the view or the
  caret sits. Far into such a line, drawn x positions rest on a
  documented estimate (the measured average advance) — the same
  approximation family as the horizontal thumb — while byte offsets
  (caret, selection, edits) stay exact everywhere; a kern-sensitive eye
  may notice glyph spacing shift where the measure window re-enters the
  line.
- Under `.wrap` the same budget is shared by a line's rows: the line
  wraps until the budget is spent and the remaining tail is frozen out
  of geometry (the caret pins at the budget edge; with no horizontal
  scroll to enter it, the frozen tail is unreachable by caret until the
  mode is toggled off or the knob is raised). Raise the knob for
  minified-content workloads that must stay wrapped.
- Clipboard: Raylib and SDL3 round-trip multi-MB transfers uncapped
  (measured at 10 MB); paste is bounded by `buffer_cap`, truncating on a
  UTF-8 boundary.

---

## `wlx_slider`

Horizontal slider for `float` values. Click and drag the thumb, or click
anywhere on the track to jump. Returns `true` on frames where the value
changes.

### Signature

```c
bool wlx_slider(WLX_Context *ctx, const char *label, float *value, ...options);
```

| Parameter | Description |
|-----------|-------------|
| `label` | Text label drawn to the left of the track (can be `NULL`) |
| `value` | Pointer to a `float` — clamped to `[min_value, max_value]` automatically |

### Minimal example

```c
static float volume = 0.5f;

if (wlx_slider(ctx, "Volume", &volume,
    .slot_align = WLX_CENTER, .width = 400, .height = 40
)) {
    set_volume(volume);
}
```

### Widget-specific options

Slider shares `WLX_TEXT_TYPOGRAPHY_FIELDS` (`font`, `font_size`, `content_align`, `spacing`) but has its own
color field (`label_color`) instead of the shared `WLX_TEXT_COLOR_FIELDS` (`front_color` / `back_color`).
It also omits `wrap` — all slider text is single-line.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `font` | `WLX_Font` | `WLX_FONT_DEFAULT` | Font for label and value text |
| `font_size` | `int` | `0` | Font size. `0` = use theme default |
| `content_align` | `WLX_Align` | `WLX_LEFT` | Text alignment for the label |
| `spacing` | `int` | `0` | Opt-in extra tracking for label and value text. `0` = natural backend spacing |
| `show_value` | `bool` | `true` | Show the numeric value readout to the right of the track (renamed from `show_label` in v0.6; the alias was removed in v0.7) |
| `track_color` | `WLX_Color` | `{0}` | Track bar background color. `{0}` = derive from theme `slider.track` |
| `thumb_color` | `WLX_Color` | `{0}` | Thumb handle color. `{0}` = theme `slider.thumb` |
| `label_color` | `WLX_Color` | `{0}` | Text label color. `{0}` = theme `slider.label` |
| `track_height` | `float` | `0` | Height of the track bar. `0` = theme default (6) |
| `thumb_width` | `float` | `0` | Width of the thumb handle. `0` = theme default (14) |
| `border_color` | `WLX_Color` | `{0}` | Border color for track/thumb outlines. `{0}` = theme `border` |
| `border_width` | `float` | `WLX_UNSET` | Border width. unset = theme `border_width` |
| `roundness` | `float` | `WLX_UNSET` | Corner rounding for track/fill/thumb. unset = theme default |
| `rounded_segments` | `int` | `WLX_UNSET` | Segment count for rounded drawing. unset = theme default |
| `hover_brightness` | `float` | `WLX_UNSET` | Brightness shift on track hover. Unset = theme default |
| `thumb_hover_brightness` | `float` | `WLX_UNSET` | Brightness shift on thumb hover. Unset = theme default |
| `fill_inactive_brightness` | `float` | `-0.3` | Brightness offset for the filled portion of the track |
| `min_value` | `float` | `0.0` | Minimum slider value |
| `max_value` | `float` | `1.0` | Maximum slider value |
| `content_padding` | `float` | `WLX_UNSET` | Uniform inner inset around the label / track / value region. See [Content padding](#content-padding-wlx_content_padding_fields). Default resolves to `0`. |
| `content_padding_top` | `float` | `WLX_UNSET` | Top-side override. |
| `content_padding_right` | `float` | `WLX_UNSET` | Right-side override. |
| `content_padding_bottom` | `float` | `WLX_UNSET` | Bottom-side override. |
| `content_padding_left` | `float` | `WLX_UNSET` | Left-side override. |

Shared placement and sizing fields also apply.

`content_padding*` insets the label, track, thumb, and value text together.
The drag hit-rect tracks the inset track region, so a padded slider stays
interactive only inside the inset.

### Common overrides

Color channel slider with colored thumb:

```c
wlx_slider(ctx, "Red", &color_r,
    .slot_align = WLX_CENTER, .width = 500, .height = 40,
    .min_value = 0.0f, .max_value = 1.0f,
    .thumb_color = (WLX_Color){255, 60, 60, 255}
);
```

Integer-range slider (0–100) with custom font:

```c
wlx_slider(ctx, "Speed", &speed,
    .slot_align = WLX_CENTER, .width = 400, .height = 40,
    .min_value = 0.0f, .max_value = 100.0f,
    .font_size = 18
);
```

---

## `wlx_separator`

Non-interactive divider line. The separator draws horizontally when its
resolved rect is wider than it is tall, and vertically otherwise.

### Signature

```c
void wlx_separator(WLX_Context *ctx, ...options);
```

### Minimal example

```c
wlx_separator(ctx, .height = 1);
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `back_color` | `WLX_Color` | `{0}` | Divider color. `{0}` = theme `border` (renamed from `color` in v0.6; the alias was removed in v0.7) |
| `thickness` | `float` | `1.0` | Line thickness in pixels |

Shared placement and sizing fields also apply.

### Common overrides

Horizontal rule between controls:

```c
wlx_separator(ctx,
    .height = 1,
    .color = (WLX_Color){70, 70, 70, 255}
);
```

Vertical divider inside a horizontal layout:

```c
wlx_separator(ctx,
    .width = 1,
    .height = 48,
    .thickness = 1.5f
);
```

---

## `wlx_progress`

Progress bar for normalized values. The incoming `value` is clamped to the
range `[0.0f, 1.0f]` before drawing.

### Signature

```c
void wlx_progress(WLX_Context *ctx, float value, ...options);
```

### Minimal example

```c
wlx_progress(ctx, download_progress,
    .height = 24,
    .track_height = 10
);
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `track_color` | `WLX_Color` | `{0}` | Track color. `{0}` = theme `progress.track`, then `slider.track` |
| `fill_color` | `WLX_Color` | `{0}` | Fill color. `{0}` = theme `progress.fill`, then `accent` |
| `track_height` | `float` | `0` | Centered track height. `0` = theme/default progress height |
| `segments` | `int` | `0` | `0` = continuous bar (default). `> 0` = discrete segmented mode drawing this many equal cells |
| `segment_gap` | `float` | `0` | Pixel gap between cells in segmented mode. `<= 0` = theme `progress.segment_gap`, then a `2px` fallback |
| `content_padding` | `float` | `WLX_UNSET` | Uniform inner inset around the track rect. See [Content padding](#content-padding-wlx_content_padding_fields). Default resolves to `0`. |
| `content_padding_top` | `float` | `WLX_UNSET` | Top-side override. |
| `content_padding_right` | `float` | `WLX_UNSET` | Right-side override. |
| `content_padding_bottom` | `float` | `WLX_UNSET` | Bottom-side override. |
| `content_padding_left` | `float` | `WLX_UNSET` | Left-side override. |

Shared placement, sizing, and border fields also apply.

`content_padding*` insets the track rect. The rendered track height is
additionally clamped to the inset height so a tall `track_height` cannot
overflow into the padding.

### Segmented mode

Setting `segments > 0` switches the bar to discrete cells: `round(value *
segments)` cells are drawn in `fill_color` and the rest in `track_color`.
Cells have equal floor-rounded widths (the last cell absorbs the sub-pixel
remainder), separated by `segment_gap`. In this mode the bar draws **only** the
cells: there is no continuous track box and no border, so the inactive cells act
as the track and the gaps are transparent. `roundness` / `rounded_segments`
still apply per cell.

The segmented bar self-contains within its track at narrow widths: every cell
stays inside the track rect regardless of `segments` and `segment_gap`. When the
track is too narrow for the nominal layout, the gap is compressed (down to 0)
and cell widths fall to a 1px floor; cells that still cannot fit collapse to
zero width at the track's right edge rather than spilling past it. The minimum
bar width is therefore `segments` px (1px per cell, gap 0).

```c
wlx_progress(ctx, cpu_load, .segments = 16, .segment_gap = 3);
```

### Common overrides

Thin status bar:

```c
wlx_progress(ctx, task_progress,
    .height = 18,
    .track_height = 6,
    .fill_color = (WLX_Color){80, 180, 120, 255}
);
```

Rounded progress meter:

```c
wlx_progress(ctx, 0.72f,
    .height = 28,
    .track_height = 12,
    .roundness = 1.0f,
    .border_width = 1.0f
);
```

---

## `wlx_toggle`

On/off switch with an optional text label. Returns `true` on the frame the
value changes and flips `*value` automatically.

### Signature

```c
bool wlx_toggle(WLX_Context *ctx, const char *label, bool *value, ...options);
```

| Parameter | Description |
|-----------|-------------|
| `label` | Label shown to the right of the switch. Can be `NULL` |
| `value` | Pointer to a `bool` — toggled automatically on click |

### Minimal example

```c
static bool autosave = true;

if (wlx_toggle(ctx, "Autosave", &autosave, .height = 34)) {
    printf("Autosave: %s\n", autosave ? "ON" : "OFF");
}
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `track_color` | `WLX_Color` | `{0}` | Off-state track color. `{0}` = theme `toggle.track`, then `slider.track` |
| `track_active_color` | `WLX_Color` | `{0}` | On-state track color. `{0}` = theme `toggle.track_active`, then `accent` |
| `thumb_color` | `WLX_Color` | `{0}` | Thumb color. `{0}` = theme `toggle.thumb`, then `foreground` |
| `hover_brightness` | `float` | `WLX_UNSET` | Hover brightness override. Unset = theme `hover_brightness` |
| `content_padding` | `float` | `WLX_UNSET` | Uniform inner inset around the compound content (track + label). See [Content padding](#content-padding-wlx_content_padding_fields). Default resolves to `0`. |
| `content_padding_top` | `float` | `WLX_UNSET` | Top-side override. |
| `content_padding_right` | `float` | `WLX_UNSET` | Right-side override. |
| `content_padding_bottom` | `float` | `WLX_UNSET` | Bottom-side override. |
| `content_padding_left` | `float` | `WLX_UNSET` | Left-side override. |

Shared placement, sizing, typography, text-color, and border fields also
apply. Default `wrap` is `false`. `front_color` styles the label text;
`back_color` is currently unused by this widget.

`content_padding*` insets the compound content rect (track + label) before
alignment. The click/hover hit rect stays at the full slot rect.

### Common overrides

Compact toolbar toggle:

```c
wlx_toggle(ctx, "Grid", &show_grid,
    .font_size = 14,
    .height = 26,
    .slot_align = WLX_RIGHT
);
```

Accent-colored toggle with rounded outline:

```c
wlx_toggle(ctx, "Live preview", &live_preview,
    .track_active_color = (WLX_Color){40, 160, 120, 255},
    .thumb_color = (WLX_Color){245, 245, 245, 255},
    .border_width = 1.0f,
    .roundness = 1.0f
);
```

---

## `wlx_radio`

Radio button bound to an integer selection group. Returns `true` on the frame
the control is clicked and writes `index` into `*active`.

### Signature

```c
bool wlx_radio(WLX_Context *ctx, const char *label, int *active, int index, ...options);
```

| Parameter | Description |
|-----------|-------------|
| `label` | Label shown to the right of the radio control. Can be `NULL` |
| `active` | Pointer to the currently selected index |
| `index` | Index associated with this radio option |

### Minimal example

```c
static int theme_choice = 0;

wlx_radio(ctx, "Dark", &theme_choice, 0);
wlx_radio(ctx, "Light", &theme_choice, 1);
wlx_radio(ctx, "Glass", &theme_choice, 2);
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `ring_color` | `WLX_Color` | `{0}` | Ring color. `{0}` = theme `radio.ring`, then `border` |
| `fill_color` | `WLX_Color` | `{0}` | Selected fill color. `{0}` = theme `radio.fill`, then `accent` |
| `ring_border_width` | `float` | `WLX_UNSET` | Ring outline width. unset = theme `radio.border_width`, then theme `border_width` |
| `hover_brightness` | `float` | `WLX_UNSET` | Hover brightness override. Unset = theme `hover_brightness` |
| `content_padding` | `float` | `WLX_UNSET` | Uniform inner inset around the compound content (ring + label). See [Content padding](#content-padding-wlx_content_padding_fields). Default resolves to `0`. |
| `content_padding_top` | `float` | `WLX_UNSET` | Top-side override. |
| `content_padding_right` | `float` | `WLX_UNSET` | Right-side override. |
| `content_padding_bottom` | `float` | `WLX_UNSET` | Bottom-side override. |
| `content_padding_left` | `float` | `WLX_UNSET` | Left-side override. |

Shared placement, sizing, typography, and text-color fields also apply.
Default `wrap` is `false`. `front_color` styles the label text; `back_color`
is currently unused by this widget.

`content_padding*` insets the compound content rect (ring + label) before
alignment. The click/hover hit rect stays at the full slot rect.

### Common overrides

Radio row in a settings panel:

```c
wlx_radio(ctx, "CPU", &backend_mode, 0, .height = 30);
wlx_radio(ctx, "GPU", &backend_mode, 1, .height = 30);
```

Styled radio with custom ring thickness:

```c
wlx_radio(ctx, "Outline mode", &render_mode, 2,
    .ring_border_width = 2.0f,
    .fill_color = (WLX_Color){220, 180, 80, 255}
);
```

---

## `wlx_scroll_panel_begin` / `wlx_scroll_panel_end`

Scrollable container. Wraps a region of child widgets that can exceed the
visible area; a scrollbar appears automatically. Must be used as a
begin/end pair with layout content in between.

Uses persistent state internally (`WLX_Scroll_Panel_State`) to track scroll
offset, scrollbar drag state, and auto-height measurement across frames.

### Signature

```c
void wlx_scroll_panel_begin(WLX_Context *ctx, float content_height, ...options);
void wlx_scroll_panel_end(WLX_Context *ctx);
```

| Parameter | Description |
|-----------|-------------|
| `content_height` | Total height of the scrollable content in pixels. Use `WLX_SCROLL_AUTO_HEIGHT` (`-1`) for **auto-height** mode (measured from children automatically) |

### Minimal example

```c
wlx_scroll_panel_begin(ctx, -1);  // auto-height: measured from children
    wlx_layout_begin(ctx, 50, WLX_VERT);
        for (int i = 0; i < 50; i++) {
            char label[32];
            snprintf(label, sizeof(label), "Item %d", i + 1);
            wlx_label(ctx, label, .height = 30, .font_size = 16);
        }
    wlx_layout_end(ctx);
wlx_scroll_panel_end(ctx);
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `back_color` | `WLX_Color` | `{0}` | Panel background color. `{0}` = theme background |
| `transparent_background` | `bool` | `false` | `true` draws no panel fill so the container behind shows through (distinct from `back_color = {0}`, which uses the theme background) |
| `scrollbar_color` | `WLX_Color` | `{0}` | Scrollbar thumb color. `{0}` = theme `scrollbar.bar` |
| `scrollbar_hover_brightness` | `float` | `WLX_UNSET` | Brightness shift when hovering the scrollbar. Unset = theme default |
| `scrollbar_width` | `float` | `WLX_UNSET` | Width of the scrollbar. unset = theme default |
| `wheel_scroll_speed` | `float` | `20.0` | Pixels scrolled per mouse wheel tick |
| `show_scrollbar` | `bool` | `true` | Whether to draw the scrollbar |
| `border_color` | `WLX_Color` | `{0}` | Panel border color. `{0}` = theme `border` |
| `border_width` | `float` | `WLX_UNSET` | Panel border width. unset = theme `border_width` |
| `roundness` | `float` | `WLX_UNSET` | Panel corner roundness. unset = theme default |
| `rounded_segments` | `int` | `WLX_UNSET` | Segment count for rounded drawing. unset = theme default |

Shared placement and sizing fields also apply (no typography or text-color fields).
When `.id` is set, that scope stays active for the full scroll-panel body
until `wlx_scroll_panel_end(ctx)`.

The thumb is proportional to the visible share of the content and never
shorter than 20px: over tall content the length floors and the thumb's
travel maps the scroll range onto the remaining track, so the track end
still means the content end.

### Common overrides

Styled scroll panel with explicit content height:

```c
wlx_scroll_panel_begin(ctx, 2000.0f,
    .back_color = (WLX_Color){25, 25, 25, 255},
    .scrollbar_color = (WLX_Color){80, 80, 80, 255},
    .scrollbar_width = 12,
    .wheel_scroll_speed = 40.0f
);
    // ... layout with children totalling 2000px ...
wlx_scroll_panel_end(ctx);
```

Auto-height scroll panel with clickable items:

```c
wlx_scroll_panel_begin(ctx, -1,
    .back_color = (WLX_Color){20, 20, 20, 255}
);
    wlx_layout_begin(ctx, ITEM_COUNT, WLX_VERT);
        for (int i = 0; i < ITEM_COUNT; i++) {
            char label[64];
            snprintf(label, sizeof(label), "Item %d", i + 1);
            if (wlx_button(ctx, label,
                .height = 40, .font_size = 18, .content_align = WLX_CENTER,
                .back_color = (i % 2 == 0)
                    ? (WLX_Color){35, 35, 35, 255}
                    : (WLX_Color){30, 30, 30, 255}
            )) {
                printf("Clicked: %s\n", label);
            }
        }
    wlx_layout_end(ctx);
wlx_scroll_panel_end(ctx);
```

---

## `wlx_list_clipper_begin` / `wlx_list_clipper_end`

Virtualizes a long list inside a scroll panel: only rows within the viewport are
built, while spacers reserve the off-screen extent so the scrollbar geometry
stays correct. Per-frame cost (layout, text measurement, command recording,
replay) becomes proportional to *visible* rows rather than total rows, so large
or fast-growing lists stay cheap.

Drive the panel with an explicit content height from `wlx_list_clipper_height`
(not `WLX_SCROLL_AUTO_HEIGHT` — auto-height would measure only the visible slice
and break scrolling).

Fixed-pitch rows:

```c
#define ROW_H 22.0f
float h = wlx_list_clipper_height(count, ROW_H, NULL);
wlx_scroll_panel_begin(ctx, h, .id = "log");
    WLX_List_Clipper c = wlx_list_clipper_begin(ctx, count, ROW_H,
        .id = "rows", .overscan = ROW_H);
    for (int i = c.first; i < c.last; i++) {
        // build row i (e.g. a HORZ layout with labels)
    }
    wlx_list_clipper_end(ctx, &c);
wlx_scroll_panel_end(ctx);
```

Variable-height rows: pass a prefix-sum `item_offsets` array (length
`count + 1`, `[0] == 0`) and size each visible row before building it:

```c
WLX_List_Clipper c = wlx_list_clipper_begin(ctx, count, fallback_h,
    .id = "rows", .item_offsets = offsets);
for (int i = c.first; i < c.last; i++) {
    wlx_layout_auto_slot_px(ctx, wlx_list_clipper_item_height(&c, i));
    // build row i
}
wlx_list_clipper_end(ctx, &c);
```

**Options** (`WLX_List_Clipper_Opt`): `id` (content-layout id), `item_offsets`
(variable mode; `NULL` = fixed pitch), `overscan` (extra pixels of rows built
above/below the viewport — use it to keep a margin of pre-built rows).

**Caveat:** rows outside `[first, last)` are not produced, so they get no ids,
interactions, or persistent state that frame. Do not place stateful or
interactive widgets in virtualized rows expecting per-frame execution, or widen
the range with `overscan`.

To additionally trim over-drawn bounded fills/borders elsewhere, enable
`wlx_set_cull_offscreen(ctx, true)`. Note that text is never culled, so the
clipper (not culling) is what keeps a text row list cheap.

---

## `wlx_split_begin` / `wlx_split_next` / `wlx_split_end`

Compound two-pane split layout with independent scroll panels. Replaces the
recurring pattern of outer VERT wrapper + HORZ layout + two scroll panels
(6 begin/end pairs) with 3 calls. Each pane gets its own auto-height scroll
panel; the user creates their own inner layout inside each pane.

### Signature

```c
void wlx_split_begin(WLX_Context *ctx, ...options);
void wlx_split_next(WLX_Context *ctx, ...options);
void wlx_split_end(WLX_Context *ctx);
```

### Minimal example

```c
wlx_split_begin(ctx);

    // Left pane (280px sidebar by default)
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_CONTENT, WLX_SLOT_CONTENT),
        .padding = 2);
        wlx_label(ctx, "Options", .font_size = 18, .height = 32);
        wlx_slider(ctx, "Value", &val, .height = 36);
    wlx_layout_end(ctx);

wlx_split_next(ctx);

    // Right pane (flexible width)
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_CONTENT, WLX_SLOT_CONTENT));
        wlx_label(ctx, "Content", .font_size = 26, .height = 48);
        wlx_label(ctx, "Hello world", .font_size = 16, .height = 30);
    wlx_layout_end(ctx);

wlx_split_end(ctx);
```

### `wlx_split_begin` options (`WLX_Split_Opt`)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `first_size` | `WLX_Slot_Size` | `WLX_SLOT_PX(280)` | Width of the first (left) pane |
| `second_size` | `WLX_Slot_Size` | `WLX_SLOT_FLEX(1)` | Width of the second (right) pane |
| `fill_size` | `WLX_Slot_Size` | `WLX_SLOT_FLEX(1)` | Outer wrapper slot size |
| `content_padding` | `float` | `4` | Uniform inner inset. See [Content padding](#content-padding-wlx_content_padding_fields). |
| `content_padding_top` | `float` | `WLX_UNSET` | Top-side override. `< 0` falls back to `content_padding`. |
| `content_padding_right` | `float` | `WLX_UNSET` | Right-side override. `< 0` falls back to `content_padding`. |
| `content_padding_bottom` | `float` | `WLX_UNSET` | Bottom-side override. `< 0` falls back to `content_padding`. |
| `content_padding_left` | `float` | `WLX_UNSET` | Left-side override. `< 0` falls back to `content_padding`. |
| `gap` | `float` | `0` | Space between the two panes |
| `first_back_color` | `WLX_Color` | `{0}` | First pane background. `{0}` = theme default |
| `second_back_color` | `WLX_Color` | `{0}` | Second pane background. `{0}` = theme default |
| `id` | `const char *` | `NULL` | Scope ID applied to the full split body |

### `wlx_split_next` options (`WLX_Split_Next_Opt`)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `back_color` | `WLX_Color` | `{0}` | Override second pane background color |

### Common overrides

Custom sidebar width with colored left pane:

```c
wlx_split_begin(ctx,
    .first_size = WLX_SLOT_PX(300),
    .first_back_color = (WLX_Color){20, 20, 30, 255});
    // ... left pane ...
wlx_split_next(ctx);
    // ... right pane ...
wlx_split_end(ctx);
```

Constrained fill with minimum height:

```c
wlx_split_begin(ctx,
    .fill_size = WLX_SLOT_FILL_MIN(400),
    .first_size = WLX_SLOT_PX(300));
    // ...
wlx_split_next(ctx);
    // ...
wlx_split_end(ctx);
```

Nested split (split inside the right pane of another split):

```c
wlx_split_begin(ctx);
    // outer left pane
    wlx_layout_begin_auto(ctx, WLX_VERT, 30);
        wlx_label(ctx, "Sidebar", .height = 30);
    wlx_layout_end(ctx);
wlx_split_next(ctx);
    // outer right pane contains a nested split
    wlx_split_begin(ctx, .first_size = WLX_SLOT_PX(200));
        wlx_layout_begin_auto(ctx, WLX_VERT, 30);
            wlx_label(ctx, "Inner left", .height = 30);
        wlx_layout_end(ctx);
    wlx_split_next(ctx);
        wlx_layout_begin_auto(ctx, WLX_VERT, 30);
            wlx_label(ctx, "Inner right", .height = 30);
        wlx_layout_end(ctx);
    wlx_split_end(ctx);
wlx_split_end(ctx);
```

### Notes

- Each pane wraps content in an auto-height scroll panel (`content_height = -1`).
  Scrollbars appear automatically when content exceeds the pane height.
- The user must create their own inner layout inside each pane.
  The split widget manages only the outer structure (VERT + HORZ + scroll panels).
- If `.id` is set, it scopes both pane bodies until `wlx_split_end(ctx)`.
- In loops, wrap each iteration with `wlx_push_id()` / `wlx_pop_id()` as with
  any repeated widget.
- Do **not** place `wlx_split_begin` inside a dynamic auto layout
  (`wlx_layout_begin_auto`) — the compound widget's internal layouts disrupt
  the dynamic layout's contiguous offset buffer. Use a static layout as the
  parent instead.

---

## `wlx_panel_begin` / `wlx_panel_end`

Capacity-based CONTENT layout with optional heading label. Eliminates manual
slot counting for layouts where every slot uses `WLX_SLOT_CONTENT`. The panel
pre-allocates a fixed capacity of CONTENT slots — unused slots contribute 0px.
Adding or removing child widgets requires no slot-count updates.

### Signature

```c
void wlx_panel_begin(WLX_Context *ctx, ...options);
void wlx_panel_end(WLX_Context *ctx);
```

### Minimal example

```c
wlx_panel_begin(ctx, .title = "Options",
    .title_back_color = (WLX_Color){40, 24, 24, 255});

    wlx_slider(ctx, "Value", &val, .height = 36);
    wlx_checkbox(ctx, "Enable", &flag, .height = 30);

wlx_panel_end(ctx);
```

### `wlx_panel_begin` options (`WLX_Panel_Opt`)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `title` | `const char *` | `NULL` | Heading text. `NULL` = no heading |
| `title_font_size` | `int` | `18` | Heading font size |
| `title_height` | `float` | `32` | Heading slot height in pixels |
| `title_align` | `WLX_Align` | `WLX_CENTER` | Heading text alignment |
| `title_back_color` | `WLX_Color` | `{0}` | Heading background color |
| `back_color` | `WLX_Color` | `{0}` | Panel body background color. `{0}` = transparent |
| `border_color` | `WLX_Color` | `{0}` | Panel border color. `{0}` = theme `border` (v0.6) |
| `border_width` | `float` | `WLX_UNSET` | Border thickness in pixels. unset inherits theme `border_width` (v0.6); explicit `0` = borderless |
| `roundness` | `float` | `0` | Corner roundness for border/background (fraction of the shorter side). `0` = sharp |
| `corner_radius` | `float` | `0` | Absolute corner radius in **pixels**. `> 0` overrides `roundness`; `0` = unset. See [Absolute corner radius](#absolute-corner-radius-corner_radius) |
| `clip` | `bool` | `false` | Clip body content to panel bounds |
| `content_padding` | `float` | `2` | Uniform inner inset. See [Content padding](#content-padding-wlx_content_padding_fields). |
| `content_padding_top` | `float` | `WLX_UNSET` | Top-side override. `< 0` falls back to `content_padding`. |
| `content_padding_right` | `float` | `WLX_UNSET` | Right-side override. `< 0` falls back to `content_padding`. |
| `content_padding_bottom` | `float` | `WLX_UNSET` | Bottom-side override. `< 0` falls back to `content_padding`. |
| `content_padding_left` | `float` | `WLX_UNSET` | Left-side override. `< 0` falls back to `content_padding`. |
| `gap` | `float` | `0` | Gap between child widgets |
| `capacity` | `int` | `32` | Max child widgets (excl. title). Clamped to `WLX_CONTENT_SLOTS_MAX` (32). |
| `id` | `const char *` | `NULL` | Scope ID applied to the full panel body |

### Common overrides

Left pane of a split with custom heading:

```c
wlx_split_begin(ctx,
    .first_back_color = (WLX_Color){20, 20, 30, 255});

    wlx_panel_begin(ctx, .title = "Options",
        .title_back_color = (WLX_Color){40, 24, 24, 255});
        wlx_slider(ctx, "Speed", &speed, .height = 36);
        wlx_slider(ctx, "Size", &size, .height = 36);
    wlx_panel_end(ctx);

wlx_split_next(ctx);

    wlx_panel_begin(ctx, .title = "Content",
        .title_font_size = 26, .title_height = 48,
        .title_back_color = (WLX_Color){34, 24, 50, 255},
        .content_padding = 0);
        wlx_label(ctx, "Hello world", .font_size = 16, .height = 30);
    wlx_panel_end(ctx);

wlx_split_end(ctx);
```

No heading (pure CONTENT layout wrapper):

```c
wlx_panel_begin(ctx, .padding = 4);
    wlx_label(ctx, "Item 1", .height = 30);
    wlx_label(ctx, "Item 2", .height = 30);
wlx_panel_end(ctx);
```

### Notes

- The panel creates a single VERT layout with all-CONTENT slots. It does
  **not** create a scroll panel — inside a split pane the scroll panel is
  already provided by the split.
- If `.id` is set, it scopes all descendants until `wlx_panel_end(ctx)`.
- For standalone scrollable panels, wrap in
  `wlx_scroll_panel_begin` / `wlx_scroll_panel_end`.
- Unused CONTENT slots measure 0px — no visual impact from over-allocation.
- The default capacity of 32 covers most use cases. Set `.capacity = 48` or
  higher for sections with many widgets.
- Users needing mixed slot types (e.g. CONTENT + PX) should use the raw
  `wlx_layout_begin_s` API instead.

---

## `wlx_overlay_begin` / `wlx_overlay_end`

Absolutely positioned subtree on the next layer. The body is a linear
layout rooted at a window-space rect; it consumes no parent slot,
contributes nothing to content measurement, draws over everything on
lower layers this frame, and its widgets win hover and presses over
whatever they cover (see LAYOUT_MODEL.md section 9). Overlays nest: each
level draws and arbitrates one layer higher, up to
`WLX_OVERLAY_MAX_LAYERS`.

An overlay may be declared anywhere - inside a scroll panel or a `.clip`
layout included, which is where dropdowns and menus usually live. It
escapes every enclosing clip for drawing **and** input: its own rect is
the only clip on its layer (`clip = true`, the default, records it as the
body scissor; hit-testing, the inner scroll panel's scissor and offscreen
culling use the same rect), so rows that extend past the base panel's
viewport draw and take presses, while base widgets scrolled out of their
panel stay unclickable underneath. A nested overlay starts its own clip
context again and the enclosing one returns at its `wlx_overlay_end`.

Deferred mode only; in immediate mode the body draws in place at the call
position (a `WLX_DEBUG` build warns once per site).

### Signature

```c
void wlx_overlay_begin(WLX_Context *ctx, size_t count, WLX_Rect rect, ...options);
void wlx_overlay_end(WLX_Context *ctx);
```

| Parameter | Description |
|-----------|-------------|
| `count` | Body slot count (the body is a linear layout) |
| `rect` | Absolute window-space rect for the overlay |

### Minimal example

```c
wlx_overlay_begin(ctx, 2, ((WLX_Rect){ 200, 100, 300, 120 }),
    .back_color = (WLX_Color){ 30, 30, 30, 255 }, .border_width = 1);
    wlx_label(ctx, "Floating panel");
    if (wlx_button(ctx, "Close")) close_it();
wlx_overlay_end(ctx);
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `const char *` | `NULL` | Scope id for the overlay body |
| `sizes` | `const WLX_Slot_Size *` | `NULL` | Per-slot sizes (`NULL` = equal split); CONTENT is unsupported here |
| `orient` | `WLX_Orient` | `WLX_VERT` | Body orientation |
| `gap` | `float` | `0` | Gap between body slots |
| `clip` | `bool` | `true` | Scissor body content to the rect |
| `back_color` | `WLX_Color` | `{0}` | Panel fill (`{0}` = none) |
| `border_color` / `border_width` | | `{0}` / `0` | Panel border |
| `roundness` / `rounded_segments` | | `0` / `0` | Corner rounding |
| `content_padding` (+ per-side) | `float` | `WLX_UNSET` | Body inset |

## `wlx_dropdown`

Closed-face dropdown. The face styles like a button and shows
`options[*selected]` (or `label` while `*selected` is out of range);
clicking it toggles an overlay list anchored below at face width. Choosing
an option writes `*selected`, closes the list, and returns `true`. The
list closes on Escape or on a press whose owner lies outside the dropdown
(the press still reaches its own target). Lists taller than
`max_list_height` scroll.

Uses persistent state internally (`WLX_Dropdown_State`) for the open flag.
In a `CONTENT`-sized column the face width follows the widest option, so
it does not resize when the selection changes.

### Signature

```c
bool wlx_dropdown(WLX_Context *ctx, const char *label, int *selected,
                  const char **options, size_t count, ...options);
```

### Minimal example

```c
static int size = 1;
const char *sizes[] = { "Small", "Medium", "Large", "Huge" };
if (wlx_dropdown(ctx, "size", &size, sizes, 4))
    printf("size is now %s\n", sizes[size]);
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `const char *` | `NULL` | Scope id (needed for loop-generated dropdowns) |
| `row_height` | `float` | `0` | List row height. `<= 0` = `font_size + 12` |
| `max_list_height` | `float` | `0` | Open-list height cap. `<= 0` = `WLX_DROPDOWN_MAX_LIST_HEIGHT` (240) |
| `list_back_color` | `WLX_Color` | `{0}` | List panel + row fill. `{0}` = theme background |
| `list_border_color` | `WLX_Color` | `{0}` | List border. `{0}` = face border color |
| `list_border_width` | `float` | `WLX_UNSET` | List border width. unset = face border width |
| `hover_brightness` / `hover_back_color` | | unset / `{0}` | Hover treatment for face and rows (as on `wlx_button`) |

Shared placement, sizing, state, typography (no wrap), color, border, and
content-padding fields also apply to the face.

## `wlx_tooltip_for`

Pointer-anchored tooltip for an anchor rect. While the pointer rests over
the anchor with the button up — and the pointer actually belongs to the
anchor's layer, so an overlay covering the anchor suppresses its tip — a
per-id timer accumulates frame time; past the delay a single-line tip
draws near the pointer on the next layer, clamped to the window. Returns
whether the tip is showing.

The anchor test is the same viewport-clipped containment widgets use for
hover: inside a scroll panel the pointer must also be inside every
enclosing panel's viewport, so an anchor scrolled out of view does not
light its tip from under whatever covers it.

**Draw-only:** the tooltip never takes part in input, so it cannot steal
hover or a press from the widget it describes. A press hides the tip and
restarts the delay.

### Signature

```c
bool wlx_tooltip_for(WLX_Context *ctx, WLX_Rect anchor, const char *text, ...options);
```

### Minimal example

```c
wlx_button(ctx, "Save");
wlx_tooltip_for(ctx, wlx_last_rect(ctx), "Write the file to disk");
```

`wlx_last_rect` returns the rect of the widget just placed, so a tooltip
follows its anchor with no manual geometry (see API_REFERENCE.md).

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `const char *` | `NULL` | Scope id for the tip's persistent hover-delay state |
| `delay` | `float` | `WLX_UNSET` | Seconds of hover before showing. `< 0` = 0.5 |
| `offset_x` / `offset_y` | `float` | `12` / `18` | Tip origin relative to the pointer |
| `content_padding` (+ per-side) | `float` | `WLX_UNSET` | Inner text inset; unset = 6. `.padding` is the deprecated pre-0.9 name of the uniform field (same storage, removed in the first minor after 0.9) |
| `front_color` / `back_color` | `WLX_Color` | `{0}` | Text / panel colors. `{0}` = theme foreground / background |
| `border_color` / `border_width` | | `{0}` / `WLX_UNSET` | Panel border. Unset = theme |
| `roundness` / `rounded_segments` | | `WLX_UNSET` | Corner rounding. Unset = theme |

Typography fields also apply. There are no placement or sizing fields: the
tip sizes itself from the text.

## `wlx_menu_begin` / `wlx_menu_item` / `wlx_menu_end`

Point-anchored overlay menu with a caller-owned open flag. Opening is
caller-triggered — any event may set `*open` — and `wlx_menu_begin`
builds the menu while it stays true. An item click, Escape, or a press
whose owner lies outside the menu clears `*open`. A nested
`wlx_submenu_begin` inside the body opens a submenu on the next layer
(anchored beside its trigger item automatically — see below); choosing a
submenu item dismisses the whole menu chain, exactly like choosing a
top-level item (`keep_open` items are the exception). A nested
`wlx_menu_begin` also works when a submenu needs a manual position.

`wlx_menu_begin` returns whether the menu is open. Add items and call
`wlx_menu_end` **only** when it returned true. Loop-generated items need
`wlx_push_id` like any widget.

Use `wlx_menu_begin` for context menus and submenus, where the press that
summons the menu may land anywhere. For a menu opened by a dedicated
button, use `wlx_menu_button_begin` (below): a separate opener button
sits outside the menu's press scope, so pressing it while the menu is
open counts as an outside press — the menu closes on the press and the
button's click reopens it, a visible close-reopen flicker instead of a
toggle.

Uses persistent state internally (`WLX_Menu_State`); the panel chrome
takes its height from the previous frame's item count, so it adapts one
frame after the item list changes.

### Signature

```c
bool wlx_menu_begin(WLX_Context *ctx, bool *open, float x, float y, ...options);
bool wlx_menu_item(WLX_Context *ctx, const char *text, ...options);
void wlx_menu_end(WLX_Context *ctx);
```

### Minimal example

```c
static bool open = false;
if (wlx_button(ctx, "Menu")) open = true;
if (wlx_menu_begin(ctx, &open, 200, 80)) {
    if (wlx_menu_item(ctx, "Copy"))  do_copy();
    if (wlx_menu_item(ctx, "Paste")) do_paste();
    wlx_menu_end(ctx);
}
```

### Widget-specific options

`wlx_menu_begin`:

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `width` | `float` | `WLX_UNSET` | Menu width. Unset = 180; `0` is a zero-width list |
| `row_height` | `float` | `0` | Item height. `<= 0` = `font_size + 12` |
| `item_padding` | `float` | `WLX_UNSET` | Left/right text inset on items. `< 0` = 8 |
| `front_color` / `back_color` | `WLX_Color` | `{0}` | Item text / panel colors. `{0}` = theme foreground / background |
| `border_color` / `border_width` | | `{0}` / `WLX_UNSET` | Panel border. Unset = theme |
| `roundness` / `rounded_segments` | | `WLX_UNSET` | Corner rounding. Unset = theme |
| `hover_brightness` / `hover_back_color` | | unset / `{0}` | Item hover treatment |
| `id` | `const char *` | `NULL` | Scope id (needed for loop-generated menus) |

`wlx_menu_item`:

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `disabled` | `bool` | `false` | Disable the item |
| `front_color` | `WLX_Color` | `{0}` | Per-item text color. `{0}` = menu `front_color` |
| `keep_open` | `bool` | `false` | Clicking does not close the menu — for submenu triggers and checkable items |

A plain item click dismisses the whole open menu chain (the item's menu
and every ancestor) at the `wlx_menu_end` calls. A submenu trigger must
therefore pass `.keep_open = true`, otherwise its own click closes
everything before the submenu can show:

```c
if (wlx_menu_item(ctx, "More...", .keep_open = true))
    submenu_open = !submenu_open;
if (wlx_submenu_begin(ctx, &submenu_open)) { ... wlx_menu_end(ctx); }
```

Menus nest at most `WLX_MENU_STACK_MAX` (4) levels deep.

## `wlx_menu_button_begin`

Button-anchored menu: a face button (drawn every frame, consuming a
layout slot like `wlx_button`) that toggles `*open` and anchors the list
below itself — the first click opens, the second closes, exactly like the
dropdown face. The face belongs to the menu's press scope, so its press
never counts as an outside press. Items, submenus, and closing behavior
are the shared menu machinery: use `wlx_menu_item` / `wlx_menu_end` (and
`wlx_submenu_begin` for a submenu) with the same contract — body and
`wlx_menu_end` **only** when it returned true.

### Signature

```c
bool wlx_menu_button_begin(WLX_Context *ctx, const char *label, bool *open, ...options);
```

### Minimal example

```c
static bool open = false;
if (wlx_menu_button_begin(ctx, "File", &open)) {
    if (wlx_menu_item(ctx, "New"))  do_new();
    if (wlx_menu_item(ctx, "Open")) do_open();
    wlx_menu_end(ctx);
}
```

### Widget-specific options

The face takes the shared placement, sizing, state, typography (no wrap),
color, border, content-padding, and hover fields — `.width` sizes the
**face** like any widget, and in a `CONTENT`-sized column the face width
follows its label. The list adds:

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `const char *` | `NULL` | Scope id (needed for loop-generated menu buttons) |
| `menu_width` | `float` | `0` | List width. `<= 0` = the face's resolved width |
| `row_height` | `float` | `0` | Item height. `<= 0` = `font_size + 12` |
| `item_padding` | `float` | `WLX_UNSET` | Item text left/right inset. `< 0` = 8 |
| `list_back_color` | `WLX_Color` | `{0}` | List panel + item fill. `{0}` = theme background |
| `list_border_color` | `WLX_Color` | `{0}` | List border. `{0}` = face border color |
| `list_border_width` | `float` | `WLX_UNSET` | List border width. unset = face border width |

## `wlx_submenu_begin`

Submenu, valid only inside a menu body. It takes no position: the list
anchors flush to the parent panel's right edge at the row of the last
emitted item — the trigger, which should be a `.keep_open` item that
toggles `*open`. Every unset option inherits the parent's resolved
styling (width, row height, item padding, typography, colors, border), so
a submenu matches its parent by default. `content_align` and `spacing` have no
"unset" sentinel (`WLX_LEFT` and `0` are real values), so they inherit
whenever left at those defaults - a submenu under a centered or tracked
parent cannot ask for left-aligned, untracked rows explicitly. It shares the parent's press
scope: pressing anything in the parent (the trigger included) counts as
inside, so the trigger toggles the submenu cleanly. Items and closing use
the shared machinery — a leaf click dismisses the whole chain, submenu
and ancestors alike, and the submenu's `*open` flag is cleared by the core
whenever its parent closes or re-opens, whichever side of the submenu the
clicked leaf was declared on (no `if (!menu_open) sub_open = false;` in
the caller) — body and `wlx_menu_end` **only** when it returned true.

### Signature

```c
bool wlx_submenu_begin(WLX_Context *ctx, bool *open, ...options);
```

### Minimal example

```c
if (wlx_menu_item(ctx, "More...", .keep_open = true))
    sub_open = !sub_open;
if (wlx_submenu_begin(ctx, &sub_open)) {
    if (wlx_menu_item(ctx, "Rename")) do_rename();
    if (wlx_menu_item(ctx, "Delete")) do_delete();
    wlx_menu_end(ctx);
}
```

### Options

Takes the same option set as `wlx_menu_begin`; every field left unset
inherits the parent menu's resolved value instead of the theme's.

## `wlx_widget`

Low-level colored rectangle or outlined box. Use it for dividers, color
swatches, spacers, or any situation where you need a simple rect without text.

### Signature

```c
void wlx_widget(WLX_Context *ctx, ...options);
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `back_color` | `WLX_Color` | `{0}` | Fill color of the rectangle (renamed from `color` in v0.6; the alias was removed in v0.7) |

All shared placement, sizing, and border fields also apply.

### Minimal example

```c
wlx_widget(ctx,
    .slot_align = WLX_CENTER, .width = 100, .height = 4,
    .color = (WLX_Color){ 80, 80, 80, 255 }
);
```

---

## `wlx_image`

Displays a `WLX_Texture` inside a layout slot. Supports four scale modes
(`STRETCH`, `FIT`, `FILL`, `NONE`) and an image-content alignment independent
of the slot alignment. The tint color's alpha channel is multiplied by the
active opacity stack, so `wlx_push_opacity` / `wlx_pop_opacity` regions work
transparently.

### Signature

```c
void wlx_image(WLX_Context *ctx, WLX_Texture texture, ...options);
```

| Parameter | Description |
|-----------|-------------|
| `texture` | A `WLX_Texture` supplied by the host application. Must have `width > 0` and `height > 0`. |

### Minimal example

```c
wlx_image(ctx, my_texture,
    .width = 128, .height = 128
);
```

### Scale modes

| `WLX_Image_Scale` | Behavior |
|-------------------|----------|
| `WLX_IMAGE_SCALE_STRETCH` | Stretches the texture to fill the entire widget rect (default). |
| `WLX_IMAGE_SCALE_FIT` | Scales uniformly so the image fits entirely within the widget rect, preserving aspect ratio. Letterbox / pillarbox space is transparent. |
| `WLX_IMAGE_SCALE_FILL` | Crops the source rect so the visible area fills the widget rect completely, preserving aspect ratio. No transparent borders. |
| `WLX_IMAGE_SCALE_NONE` | Draws the image at its natural source size. Clips to the widget rect if the image is larger. |

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `scale` | `WLX_Image_Scale` | `WLX_IMAGE_SCALE_STRETCH` | How the texture is scaled inside the slot (see table above). |
| `content_align` | `WLX_Align` | `WLX_CENTER` | Positions the image content within the widget rect for `FIT` and `NONE`; selects the crop anchor for `FILL`. Has no effect for `STRETCH`. |
| `tint` | `WLX_Color` | `{0}` | Tint applied to the texture. `{0}` resolves to `WLX_WHITE` (no tint). The alpha component is multiplied by the opacity stack. |
| `src` | `WLX_Rect` | `{0}` | Source sub-rect within the texture. `{0}` (or `src.w <= 0`) means the full texture. Use this for spritesheet cells. |
| `id` | `const char *` | `NULL` | Optional widget ID for persistent state and scoping. |

Shared placement, sizing (`slot_align`, `width`, `height`, etc.), and border
fields also apply.

> **`slot_align` vs. `content_align`**: `slot_align` positions the *widget rect*
> within its layout slot (e.g. center a 100px widget inside a 300px column).
> `content_align` positions the *image content* within that widget rect (e.g.
> pin a natural-size image to the top-left corner). They are independent, and
> pair with `padding` (slot inset) and `content_padding` (content inset).

### Common overrides

Fit a portrait photo inside a landscape card, centered:

```c
wlx_image(ctx, portrait_photo,
    .width = 200, .height = 150,
    .scale = WLX_IMAGE_SCALE_FIT,
    .content_align = WLX_CENTER
);
```

Fill a thumbnail slot, crop from the left edge:

```c
wlx_image(ctx, banner_texture,
    .width = 80, .height = 80,
    .scale = WLX_IMAGE_SCALE_FILL,
    .content_align = WLX_LEFT
);
```

Draw a spritesheet cell at natural size, pinned to the top-left:

```c
wlx_image(ctx, spritesheet,
    .scale = WLX_IMAGE_SCALE_NONE,
    .content_align = WLX_TOP_LEFT,
    .src   = (WLX_Rect){ 0, 0, 32, 32 }
);
```

Semi-transparent icon using tint alpha (also affected by the opacity stack):

```c
wlx_image(ctx, icon_texture,
    .width = 32, .height = 32,
    .tint  = (WLX_Color){ 255, 255, 255, 128 }
);
```

### Notes

- **Empty texture**: if `texture.width <= 0` or `texture.height <= 0`, no draw
  command is emitted and the widget frame still closes cleanly so subsequent
  widgets in the layout receive the correct slots. Under `-DWLX_DEBUG` an
  assertion fires to catch unloaded textures early.
- **Full texture by default**: `src.w <= 0` (including a zero-initialized `src`)
  means "use the entire texture". Set `src` only when you need a sub-region.
- **Spritesheet pattern**: combine `scale = WLX_IMAGE_SCALE_NONE` with a `src`
  cell rect to stamp a fixed-size sprite at its natural pixel size, aligned
  within the slot.
- **Opacity stack**: `wlx_push_opacity(ctx, 0.5f)` before `wlx_image` halves
  the effective alpha of `tint`. This matches the behavior of every other
  Wollix widget that carries an alpha channel.
- Texture loading is out of scope. Pass a `Texture2D` from
  `LoadTexture()` (Raylib), a `SDL_Texture *` from `IMG_LoadTexture()` (SDL3),
  or the equivalent for your backend. Wollix never performs I/O.

---

## Theme integration

All `{0}` color defaults resolve through the active theme at render time.
Set the theme before `wlx_begin()`:

```c
ctx->theme = &wlx_theme_light;   // or &wlx_theme_dark (the default)
```

Per-widget overrides always take precedence over theme values. The resolution
order is:

1. Explicit option value (e.g. `.back_color = RED`)
2. Widget-specific theme override (e.g. `theme->slider.track`)
3. Global theme color (e.g. `theme->surface`)

See the `WLX_Theme` struct in `wollix.h` for all themeable fields.

---

## Alignment quick reference

The `WLX_Align` enum values used by `slot_align` and `content_align`:

| Value | Position |
|-------|----------|
| `WLX_ALIGN_NONE` | No alignment (use raw slot rect) |
| `WLX_LEFT` | Center vertically, left edge |
| `WLX_RIGHT` | Center vertically, right edge |
| `WLX_TOP` | Center horizontally, top edge |
| `WLX_BOTTOM` | Center horizontally, bottom edge |
| `WLX_CENTER` | Center both axes |
| `WLX_TOP_LEFT` | Top-left corner |
| `WLX_TOP_RIGHT` | Top-right corner |
| `WLX_TOP_CENTER` | Top edge, centered horizontally |
| `WLX_BOTTOM_LEFT` | Bottom-left corner |
| `WLX_BOTTOM_RIGHT` | Bottom-right corner |
| `WLX_BOTTOM_CENTER` | Bottom edge, centered horizontally |

---

## Color helper

Create colors with the `WLX_RGBA` macro:

```c
WLX_Color red   = WLX_RGBA(255, 0, 0, 255);
WLX_Color trans  = WLX_RGBA(255, 255, 255, 128);  // 50% transparent white
```

Or with a compound literal:

```c
.back_color = (WLX_Color){40, 40, 40, 255}
```

Fields: `r`, `g`, `b`, `a` — each `unsigned char` (0–255).
