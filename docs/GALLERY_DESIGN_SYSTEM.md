# Gallery Design System

> **Secondary reference for the gallery.** The canonical Wollix design system --
> the core `WLX_Theme` contract and the "Mechanical Glass" showcase -- lives in
> [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md). This document covers only the gallery's
> application-local semantic layer (`Gallery_Semantic_Theme`,
> `Gallery_Semantic_Spacing`) and its Brand reference.

The gallery derives an application-local semantic layer on top of the core
`WLX_Theme` contract documented in [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md): it
extends the compact public theme with the nested surfaces, muted text, status
colors, and spacing roles its larger UI surface needs. The patterns below are
gallery-local, not public API -- treat them as a worked example of
application-level theme derivation.

For per-widget option fields, see [WIDGETS.md](WIDGETS.md). For the sentinel
rules that make default inheritance work, see [SENTINEL.md](SENTINEL.md).

---

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
| Disabled background | `theme->surface` shifted by `theme->disabled_brightness` |
| Disabled foreground | `theme->foreground` shifted by `theme->disabled_brightness` |
| Disabled border | `theme->border` shifted by `theme->disabled_brightness` |
| Warning, danger, success | Application-owned status colors |

In [../demos/gallery.c](../demos/gallery.c), these roles live in
`Gallery_Semantic_Theme` and are derived from the active `WLX_Theme`:

```c
static Gallery_Semantic_Theme gallery_semantic_theme(const WLX_Theme *theme) {
    bool theme_is_dark = gallery_theme_is_dark(theme);
    WLX_Color surface_2 = gallery_shift_with_theme(theme->surface, 12, theme_is_dark);
    WLX_Color surface_3 = gallery_shift_with_theme(theme->surface, 24, theme_is_dark);

    float disabled_b = wlx_is_float_unset(theme->disabled_brightness)
        ? 0.0f : theme->disabled_brightness;

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
        .color_disabled_bg     = wlx_color_brightness(theme->surface,    disabled_b),
        .color_disabled_fg     = wlx_color_brightness(theme->foreground, disabled_b),
        .color_disabled_border = wlx_color_brightness(theme->border,     disabled_b),
        .color_success = GALLERY_GREEN,
        .color_warning = GALLERY_WARNING,
        .color_danger = GALLERY_RED,
    };
}
```

Application code should follow the same pattern: keep `WLX_Theme` as the public
theme contract, then derive app-specific semantic roles locally. That keeps the
core API stable while giving each product enough vocabulary for its own chrome,
navigation, tables, panels, and status states. The core contract these roles
build on is documented in [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md).

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

## Brand Direction

Wollix should feel compact, mechanical, clear, composable, and quietly
expressive. The design system favors dense information, direct controls,
visible structure, and small moments of polish over decorative shell work.

The gallery includes a gallery-local Brand theme as an example of that
direction. It is not a public built-in preset. Treat it as a reference for
application-level theme derivation, not as part of the stable API surface. The
dashboard expresses the same principle through its "Mechanical Glass" token
layer; see [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md).
