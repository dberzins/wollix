# Sentinel Resolution

How wollix decides whether an option field was explicitly set by the caller
or should fall back to the active theme or a library default.

---

## Background

Wollix widgets use C11 designated initializers for options. Omitted fields
are zero-filled by the compiler:

```c
wlx_button(ctx, "OK", .font_size = 24);
// border_width, roundness, hover_brightness, etc. are all 0
```

Many widget resolve functions detect unset fields and replace them with theme
defaults. A few public option structs use local defaulting rules instead.
The challenge is that `0` is a valid explicit value for many fields (e.g.
"no border", "sharp corners", "no hover effect"). Wollix uses **sentinel
default values** hidden inside the public macros to distinguish "not set"
from "explicitly zero."

---

## Sentinel Conventions

Wollix uses several unset/default patterns depending on the field type and API:

### 1. `-1` for fields where negative is never valid

Used for: `border_width`, `roundness`, `scrollbar_width`, `rounded_segments`,
`width`, `height`, `opacity`, `padding` (panel/split).

The widget macro sets the default to `-1`. The resolve function uses
`wlx_is_negative_unset()` to detect the sentinel and replaces it with the
theme value. `0` passes through as an explicit value.

```c
// Inside the macro (hidden from callers):
.border_width = -1

// In resolve:
if (wlx_is_negative_unset(opt->border_width)) opt->border_width = theme->border_width;
```

`wlx_is_negative_unset(float x)` returns `true` when `x < 0`. Use it in
resolve functions to signal intent.

> **Deprecated:** `wlx_is_unset()` is a backward-compatible alias for
> `wlx_is_negative_unset()`. New code should use `wlx_is_negative_unset()`.

**As a caller:**

```c
wlx_label(ctx, "Hello");                    // border_width = theme default
wlx_label(ctx, "Hello", .border_width = 0); // no border
wlx_label(ctx, "Hello", .border_width = 2); // 2px border
```

### 2. `WLX_FLOAT_UNSET` for fields where negative values are valid

Used for: `hover_brightness`, `thumb_hover_brightness`,
`scrollbar_hover_brightness`.

These fields accept negative values (the light theme uses `-0.08f` to darken
on hover), so `-1` cannot serve as the sentinel. Instead, wollix defines:

```c
#define WLX_FLOAT_UNSET (-1e30f)
```

The widget macro sets the default to `WLX_FLOAT_UNSET`. The resolve function
uses `wlx_is_float_unset()` and replaces with the theme value. Any real
value — positive, negative, or zero — passes through.

`0.0f` is an explicit value, not a sentinel. For hover-brightness fields it
means "apply no hover brightening or darkening," which is distinct from both
`WLX_FLOAT_UNSET` (inherit from theme) and negative values such as `-0.08f`
that intentionally darken on hover.

```c
// In resolve:
if (wlx_is_float_unset(opt->hover_brightness))
    opt->hover_brightness = theme->hover_brightness;
```

**As a caller:**

```c
wlx_slider(ctx, "Vol", &vol);                          // hover = theme default
wlx_slider(ctx, "Vol", &vol, .hover_brightness = 0.0f); // no hover effect
wlx_slider(ctx, "Vol", &vol, .hover_brightness = 0.2f); // custom hover
```

### 3. `<= 0` for fields where zero is never valid

Used for: `font_size`, `spacing`, `track_height`, `thumb_width`.

Zero is never a meaningful value for these fields (zero-size font, zero-width
thumb, etc.), so the original `<= 0` sentinel is correct and unchanged.

```c
if (opt->font_size <= 0) opt->font_size = theme->font_size;
```

### 4. `wlx_color_is_zero()` for colors

All `WLX_Color` fields use the all-zero color `{0, 0, 0, 0}` as sentinel.
The resolve function detects it with `wlx_color_is_zero()` and replaces with
the theme color.

```c
wlx_button(ctx, "OK");                              // theme colors
wlx_button(ctx, "OK", .back_color = WLX_RGBA(255, 0, 0, 255)); // red
```

For multi-level color fallbacks (e.g. toggle/radio colors that first check a
widget-specific theme field, then fall back to a generic field), use
`wlx_color_or(a, b)` which returns `a` if non-zero, otherwise `b`:

```c
opt->track_color = wlx_color_or(opt->track_color,
                       wlx_color_or(theme->toggle.track, theme->slider.track));
```

### 5. `WLX_ALIGN_NONE` as a field-specific default trigger

Used for: `WLX_Panel_Opt.title_align`.

`WLX_ALIGN_NONE` is normally a real alignment value meaning "use the raw rect"
or "do not apply alignment." In a few option paths, though, the zero-valued
enum also acts as an omitted/default trigger.

For `WLX_Panel_Opt.title_align`, the default macro leaves the field at `0`
(`WLX_ALIGN_NONE`), and `wlx_panel_begin_impl()` resolves that to
`WLX_CENTER`:

```c
if (opt.title_align == WLX_ALIGN_NONE) opt.title_align = WLX_CENTER;
```

This is a field-specific contract, not a generic align helper. It also means
that for `title_align`, an omitted value is not distinguishable from an
explicit `WLX_ALIGN_NONE`.

### 6. All-zero `WLX_Slot_Size` as a structural default sentinel

Used for: `WLX_Split_Opt.first_size`, `second_size`, `fill_size`.

`WLX_Slot_Size` is a kind/value/min/max struct rather than a plain scalar.
Some APIs use the all-zero struct `{0}` as an omitted/default sentinel and
detect it with `wlx_slot_size_is_zero()`.

For split panels, the default macro leaves all three size fields at `{0}` and
`wlx_split_begin_impl()` resolves them to field-specific defaults:

```c
if (wlx_slot_size_is_zero(opt.fill_size))
    opt.fill_size = WLX_SLOT_FILL;
if (wlx_slot_size_is_zero(opt.first_size))
    opt.first_size = WLX_SLOT_PX(280);
if (wlx_slot_size_is_zero(opt.second_size))
    opt.second_size = WLX_SLOT_FLEX(1);
```

Because `WLX_SLOT_AUTO` has the same all-zero representation, these split
fields cannot distinguish an omitted value from an explicit `WLX_SLOT_AUTO`.
This is a structural default contract, not theme inheritance.

---

## Choosing the right sentinel helper

| Situation | Helper to use |
|-----------|--------------|
| Field uses `-1` sentinel; negative values are never meaningful (border_width, roundness, opacity, padding, scrollbar_width, ...) | `wlx_is_negative_unset(x)` |
| Field uses `WLX_FLOAT_UNSET` sentinel; negative values are legitimate (hover_brightness, thumb_hover_brightness, scrollbar_hover_brightness) | `wlx_is_float_unset(x)` |
| Field uses all-zero `WLX_Slot_Size` as an omitted/default marker in a structural option path (currently split sizes) | `wlx_slot_size_is_zero(x)` |

`wlx_is_unset()` is a deprecated alias for `wlx_is_negative_unset()`. It
remains for backward compatibility but should not be used in new resolve code.

`WLX_ALIGN_NONE` does not have a generic helper-based unset contract. Treat it
as a real align mode unless a specific field documents that `WLX_ALIGN_NONE`
also triggers a local default.

---

## Layout opts are different

`WLX_Layout_Opt`, `WLX_Grid_Opt`, and `WLX_Grid_Auto_Opt` do **not** go
through resolve functions. Their `border_width`, `roundness`, and
`rounded_segments` fields default to `0` and are used as literal values.
`0` means no border / sharp corners — there is no theme inheritance for
layout containers.

```c
wlx_layout_begin(ctx, 3, WLX_VERT, .roundness = 0.3f, .back_color = bg);
// roundness = 0.3f exactly, not inherited from theme
```

---

## Summary table

| Field | Sentinel | Unset default | Zero means |
|-------|----------|---------------|------------|
| `border_width` | `-1` | theme value | no border |
| `roundness` | `-1` | theme value | sharp corners |
| `rounded_segments` | `-1` | theme value | no rounded segments |
| `scrollbar_width` | `-1` | theme value | no scrollbar |
| `opacity` | `-1` | `1.0` (fully opaque) | fully transparent |
| `hover_brightness` | `WLX_FLOAT_UNSET` | theme value | no hover effect |
| `thumb_hover_brightness` | `WLX_FLOAT_UNSET` | theme value | no hover effect |
| `scrollbar_hover_brightness` | `WLX_FLOAT_UNSET` | theme value | no hover effect |
| `font_size` | `<= 0` | theme value | *(never valid)* |
| `spacing` | `<= 0` | theme value | *(never valid)* |
| `track_height` | `<= 0` | theme value | *(never valid)* |
| `thumb_width` | `<= 0` | theme value | *(never valid)* |
| colors | `{0,0,0,0}` | theme color | transparent black is not representable in these fields |
| `title_align` (`WLX_Panel_Opt`) | `WLX_ALIGN_NONE` | `WLX_CENTER` | raw/no alignment in most other paths |
| `first_size` (`WLX_Split_Opt`) | `{0}` `WLX_Slot_Size` | `WLX_SLOT_PX(280)` | same representation as `WLX_SLOT_AUTO` |
| `second_size` (`WLX_Split_Opt`) | `{0}` `WLX_Slot_Size` | `WLX_SLOT_FLEX(1)` | same representation as `WLX_SLOT_AUTO` |
| `fill_size` (`WLX_Split_Opt`) | `{0}` `WLX_Slot_Size` | `WLX_SLOT_FILL` | same representation as `WLX_SLOT_AUTO` |
