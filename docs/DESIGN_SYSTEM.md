# Design System

Wollix keeps its design system small and close to the C API. Themes provide
the base values, widget options inherit those values by default, and
application code can derive semantic roles when it needs a larger UI surface.

This document is the public reference for theme usage and the current design
direction. For per-widget option fields, see [WIDGETS.md](WIDGETS.md). For the
sentinel rules that make default inheritance work, see
[SENTINEL.md](SENTINEL.md).

---

## Theme Model

`WLX_Theme` is the active style source for a frame. It contains global colors,
text defaults, geometry defaults, interaction feedback, opacity, and a small
set of widget-specific overrides for inputs, sliders, checkboxes, toggles,
radios, progress bars, and scrollbars.

Callers usually assign `ctx->theme` before `wlx_begin()`:

```c
ctx->theme = dark_mode ? &wlx_theme_dark : &wlx_theme_light;
wlx_begin(&ctx, root, wlx_process_raylib_input);
```

If `ctx->theme` is `NULL`, `wlx_begin()` selects `&wlx_theme_dark` for the
frame. Widget resolve functions then fill omitted option fields from the
active theme. For example, a button without `.front_color`, `.back_color`,
`.font_size`, or `.roundness` uses the current theme values, while explicit
option values still override them at the call site.

`WLX_Theme` is intentionally flat enough to copy and edit. Application code can
start from a built-in preset, override the fields it cares about, and assign
the resulting theme pointer each frame.

## Built-in Presets

Wollix ships three public theme presets:

| Preset | Description |
|--------|-------------|
| `wlx_theme_dark` | Neutral dark default with low visual weight and blue accent states. |
| `wlx_theme_light` | Bright preset with dark text, pale surfaces, and darker hover feedback. |
| `wlx_theme_glass` | Dark translucent preset for layered surfaces and stronger focus borders. |

The live gallery includes a Theme Lab that compares the built-in presets,
their semantic token ramps, and common component states:
[https://dberzins.github.io/wollix/](https://dberzins.github.io/wollix/).

## Theme Inheritance

Most widget options are optional. The public macros seed sentinel values for
omitted fields, and the implementation resolves those sentinels against the
active theme.

Common inheritance triggers are:

| Field kind | Omitted marker | Result |
|------------|----------------|--------|
| Negative-is-never-valid floats, such as `border_width`, `roundness`, `height`, and `opacity` | `-1` | Use the theme or widget default. |
| Hover brightness fields, where negative values are valid | `WLX_FLOAT_UNSET` | Use the theme hover value. |
| Text size fields, such as `font_size` | `0` or less | Use the theme text metric. |
| Colors | all-zero `WLX_Color` | Use the theme color or the widget-specific fallback. |
| Fonts | `WLX_FONT_DEFAULT` | Use the theme font or backend default. |

As a caller, this means the shortest widget calls stay theme-aware:

```c
wlx_button(&ctx, "Save");
wlx_label(&ctx, "Ready", .front_color = ctx.theme->accent);
wlx_button(&ctx, "Flat", .border_width = 0.0f);
```

The full sentinel contract is documented in [SENTINEL.md](SENTINEL.md).

## Semantic Color System

The core theme is intentionally compact. Larger applications often need more
roles than `WLX_Theme` exposes directly: nested surfaces, muted text, stronger
borders, selected rows, focus rings, and status colors. The gallery handles
that by deriving an application-local semantic layer from the active theme.

The current gallery role set is:

| Role | Source pattern |
|------|----------------|
| App background | `theme->background` |
| Surface 1 | `theme->surface` |
| Surface 2 | Surface shifted one level for the current dark/light direction |
| Surface 3 | Surface shifted another level for nested controls |
| Text 1 | `theme->foreground` |
| Text 2 | Foreground blended toward Surface 2 |
| Text muted | Foreground blended further toward Surface 2 |
| Border | `theme->border` |
| Border strong | Border blended toward foreground |
| Accent | `theme->accent` |
| Focus | `theme->input.border_focus`, falling back to accent |
| Selection | Surface 2 blended with accent |
| Warning, danger, success | Application-owned status colors |

In [../demos/gallery.c](../demos/gallery.c), these roles live in
`Gallery_Semantic_Theme` and are derived from the active `WLX_Theme`:

```c
static Gallery_Semantic_Theme gallery_semantic_theme(const WLX_Theme *theme) {
    bool theme_is_dark = gallery_theme_is_dark(theme);
    WLX_Color surface_2 = gallery_shift_with_theme(theme->surface, 12, theme_is_dark);
    WLX_Color surface_3 = gallery_shift_with_theme(theme->surface, 24, theme_is_dark);

    return (Gallery_Semantic_Theme){
        .color_app_bg = theme->background,
        .color_surface_1 = theme->surface,
        .color_surface_2 = surface_2,
        .color_surface_3 = surface_3,
        .color_text_1 = theme->foreground,
        .color_text_2 = gallery_blend_color(theme->foreground, surface_2, 56),
        .color_text_muted = gallery_blend_color(theme->foreground, surface_2, 104),
        .color_border = theme->border,
        .color_border_strong = gallery_blend_color(theme->border, theme->foreground, 88),
        .color_accent = theme->accent,
        .color_focus = gallery_color_is_zero(theme->input.border_focus)
            ? theme->accent
            : theme->input.border_focus,
        .color_selection = gallery_blend_color(surface_2, theme->accent, 92),
        .color_success = GALLERY_GREEN,
        .color_warning = GALLERY_WARNING,
        .color_danger  = GALLERY_RED,
    };
}
```

Application code should follow the same pattern: keep `WLX_Theme` as the public
theme contract, then derive app-specific semantic roles locally. That keeps the
core API stable while giving each product enough vocabulary for its own chrome,
navigation, tables, panels, and status states.

## Semantic Spacing System

`WLX_Theme.padding` is the public base spacing value. It is intentionally one
scalar, not a full application spacing system. Leaf widgets that should follow
that base value can opt in with `.content_padding = WLX_PADDING_USE_THEME`,
while layouts and demos can still use explicit `.padding` or
`.content_padding` values when they are demonstrating raw geometry.

Larger UI surfaces should derive semantic spacing locally, the same way they
derive semantic colors. The gallery uses `Gallery_Semantic_Spacing` for that
purpose. Its current roles are:

| Role | Use |
|------|-----|
| `space_shell` | Flush or near-flush major shell panes. |
| `space_panel` | Compact panel and preview layout padding. |
| `space_heading_x` | Horizontal text inset for section and panel headings. |
| `space_heading_y` | Vertical heading inset where row height allows it. |
| `space_control_x` | Compact text inset for controls, metadata, and status rows. |
| `space_preview` | Token cards, theme cards, and component preview surfaces. |
| `gap_dense` | Dense row and cell gaps. |
| `gap_section` | Larger separation between conceptual groups. |

These roles are gallery-local, not public API. If the same spacing roles become
necessary across core widgets and public examples, they can be promoted later
through a separate theme-extension decision.

## Custom Themes

Custom themes are ordinary `WLX_Theme` values. Start with a built-in preset and
override only the fields your application needs:

```c
WLX_Theme app_theme = wlx_theme_dark;
app_theme.background = (WLX_Color){18, 20, 20, 255};
app_theme.foreground = (WLX_Color){230, 233, 226, 255};
app_theme.surface = (WLX_Color){35, 39, 39, 255};
app_theme.border = (WLX_Color){58, 68, 64, 255};
app_theme.accent = (WLX_Color){96, 190, 174, 255};
app_theme.border_width = 0.5f;
app_theme.padding = 8.0f;
app_theme.input.border_focus = app_theme.accent;
app_theme.slider.thumb = app_theme.foreground;
app_theme.progress.fill = app_theme.accent;

ctx->theme = &app_theme;
wlx_begin(&ctx, root, wlx_process_raylib_input);
```

The standalone [../demos/theme_demo.c](../demos/theme_demo.c) shows runtime
preset switching, editable custom themes built from a preset, widget-specific
theme fields, and per-widget overrides.

## Brand Direction

Wollix should feel compact, mechanical, clear, composable, and quietly
expressive. The design system favors dense information, direct controls,
visible structure, and small moments of polish over decorative shell work.

The gallery includes a gallery-local Brand theme as an example of that
direction. It is not a public built-in preset. Treat it as a reference for
application-level theme derivation, not as part of the stable API surface.
