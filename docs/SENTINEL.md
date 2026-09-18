# Sentinel Resolution

How wollix decides whether an option field was set by the caller or should
fall back to the theme, the parent, or a widget default.

---

## Background

Widgets take C11 designated initializers. Each option macro installs the
struct's defaults first and the caller's designators override them:

```c
wlx_button(ctx, "OK", .font_size = 24);
// every other field carries the default the macro installed
```

The challenge is that `0` is a valid explicit value for many fields ("no
border", "sharp corners", "no hover effect"). Two rules decide, per field,
what omission means and what `0` means. A designator follows the same rule
in every option struct that declares it.

---

## Rule Z: zero is unset

Used only where zero can never be a meaningful explicit value. The macro
installs `0` / `{0}`; the resolver replaces it.

| Field family | Unset value | Resolves to |
|---|---|---|
| colors (`back_color`, `border_color`, `front_color`, ...) | `{0,0,0,0}` | theme color (`wlx_color_is_zero`); transparent black is not expressible |
| `font` | `WLX_FONT_DEFAULT` (`0`) | theme font |
| `font_size`, `track_height`, `thumb_width`, `row_height`, `menu_width`, `max_list_height`, `title_font_size`, `title_height`, `capacity` | `0` (checked `<= 0`) | theme value or widget constant |
| `shadow_blur`, `shadow_layers`, `glow_spread`, `glow_rings` | `0` | theme defaults (a `{0}` shadow/glow color disables the effect) |
| `corner_radius` | `0` | pixel-radius mode off; `roundness` drives rounding |
| `rounded_corners` | `0` | all corners |
| `min_width`, `min_height`, `max_width`, `max_height` | `0` | unconstrained |
| pointers and ids (`id`, `out_focused`, `interact_out`, textures) | `NULL` / `{0}` | none |

For multi-level color fallbacks use `wlx_color_or(a, b)`: `a` if non-zero,
otherwise `b`.

---

## Rule U: `WLX_UNSET` is unset

Used for every numeric field where zero is a meaningful explicit value.
The macro installs `WLX_UNSET` (`-1`, which converts to `int` and `float`
fields alike); the resolver replaces it with the field's documented
fallback and passes every value `>= 0` through unchanged. Writing
`WLX_UNSET` explicitly means the same as omitting the field.

| Field | Resolves to | Zero means |
|---|---|---|
| `border_width` (widgets, panel) | theme `border_width` | no border |
| `border_width_top/right/bottom/left` | the uniform `border_width` | no border on that side |
| `roundness`, `rounded_segments` (widgets, panel) | theme values | sharp corners / no segments |
| `ring_border_width` (radio), `list_border_width` (dropdown, menu button) | widget-specific theme cascade | no border |
| `scrollbar_width` | theme `scrollbar.width` | no scrollbar |
| `opacity` | `1.0` (opaque) | fully transparent |
| `width`, `height` | the slot's size | zero-sized |
| `menu` `width` | 180 px | zero-width list |
| `pos` | the next sequential slot | slot 0 |
| `padding_top/right/bottom/left` (slot inset) | the uniform slot `padding` | no inset on that side |
| `content_padding` (uniform) | `0` on leaf widgets; the compound widget's own default (inputbox and editor 10, split 4, panel 2, tooltip 6) | no inset |
| `content_padding_top/right/bottom/left` | the resolved uniform | no inset on that side |
| `image_text_gap`, `item_padding`, `delay` | widget constants | zero gap / inset / delay |
| `hover_brightness`, `thumb_hover_brightness`, `scrollbar_hover_brightness`, theme `disabled_brightness` | theme `hover_brightness` (thumb: half of it; disabled: no shift) | no hover / disabled shift |

**Signed-domain note.** Brightness shifts are the one Rule U family whose
values may be negative (the light theme darkens on hover with `-0.08f`).
Their domain is `-1 < b <= 1`: `-1.0f` is the sentinel and everything above
it, negative included, is explicit. `WLX_FLOAT_UNSET` (the pre-0.9 spelling,
a large negative float) still resolves as unset and is removed in the first
minor release after 0.9.

```c
wlx_label(ctx, "Hello");                     // border_width = theme value
wlx_label(ctx, "Hello", .border_width = 0);  // no border
wlx_label(ctx, "Hello", .border_width = 2);  // 2px border
wlx_slider(ctx, "Vol", &v, .hover_brightness = 0.0f);   // no hover effect
wlx_slider(ctx, "Vol", &v, .hover_brightness = -0.1f);  // darken on hover
```

---

## Literal defaults

Every other field has no unset state: the macro installs its documented
value and the widget uses it as written. This covers `span` (1), `wrap`,
`show_value`, `show_scrollbar`, `wheel_scroll_speed`, `thickness`,
`max_value`, tooltip `offset_x` / `offset_y`, `texture_scale`,
`image_placement`, `orient`, `vertical_metric`, `clip`, the align fields,
panel `title_align` (`WLX_CENTER`; an explicit `WLX_ALIGN_NONE` is honoured)
and the split sizes (`first_size = WLX_SLOT_PX(280)`, `second_size` and
`fill_size = WLX_SLOT_FLEX(1)`; an explicit `WLX_SLOT_AUTO` is honoured).

**Containers are literal on purpose.** `WLX_Layout_Opt`, `WLX_Grid_Opt`,
`WLX_Grid_Auto_Opt` and `WLX_Slot_Style_Opt` have no theme chrome: their
`border_width`, `roundness`, `rounded_segments` and `gap` default to `0`
and are used as literal values, because a layout is invisible unless the
caller decorates it. `wlx_panel_begin` is a widget with theme chrome and
follows Rule U for the same three chrome fields; its `gap`, like every
container gap, is literal.

```c
wlx_layout_begin(ctx, 3, WLX_VERT, .roundness = 0.3f, .back_color = bg);
// roundness = 0.3f exactly, not inherited from theme
```

---

## Request tokens

Two negative constants are explicit values that request a behaviour. They
are not unset markers and never appear in a default macro.

| Token | Where | Meaning |
|---|---|---|
| `WLX_PADDING_USE_THEME` (`-2.0f`) | uniform `content_padding` | resolve all still-unset sides to the theme's `padding` knob (`WLX_STYLE_CONTENT_PADDING` by default) |
| `WLX_SCROLL_AUTO_HEIGHT` (`-1.0f`) | the `content_height` argument of `wlx_scroll_panel_begin` | measure the content during the frame |

---

## Resolver helpers

| Situation | Helper |
|---|---|
| Rule U field with a non-negative domain (`border_width`, `roundness`, `opacity`, `content_padding`, `scrollbar_width`, ...) | `wlx_is_negative_unset(x)` (`x < 0`) |
| Rule U field with a signed domain (the four brightness fields) | `wlx_is_float_unset(x)` (`x <= -1.0f`) |
| Rule Z color | `wlx_color_is_zero(c)`, `wlx_color_or(a, b)` |
| Content padding with a widget default | `WLX_RESOLVE_CONTENT_PADDING_EX(ctx, opt, widget_default)`; leaf widgets use `WLX_RESOLVE_CONTENT_PADDING(ctx, opt)` |

Callers who need a struct's defaults as a value (to assign fields instead
of writing designators) take `wlx_<widget>_opt_defaults()`, which returns
exactly what the macro installs.
