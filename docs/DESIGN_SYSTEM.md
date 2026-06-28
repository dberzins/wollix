# Design System

Wollix's design system has two layers: a small, stable **core theme contract**
(`WLX_Theme`) that every backend and widget honors, and the **"Mechanical Glass"**
showcase token layer the canonical dashboard derives on top of it. This document
is the canonical reference for both -- the foundational contract first, then the
showcase token model, component recipes, and effects.

For per-widget option fields see [WIDGETS.md](WIDGETS.md); for the layout model
see [LAYOUT_MODEL.md](LAYOUT_MODEL.md); for the sentinel rules that make default
inheritance work see [SENTINEL.md](SENTINEL.md). The gallery keeps a secondary,
application-local design layer documented in
[GALLERY_DESIGN_SYSTEM.md](GALLERY_DESIGN_SYSTEM.md).

---

## Core Theme Contract (`WLX_Theme`)

Wollix keeps its core design system small and close to the C API. Themes provide
the base values, widget options inherit those values by default, and application
code can derive richer semantic roles when it needs a larger UI surface (the
"Mechanical Glass" showcase below is one such derivation). These sections are the
foundational, public theme contract everything else is built on.

### Theme Model

`WLX_Theme` is the active style source for a frame. It contains global colors,
text defaults, geometry defaults, interaction feedback, opacity, and a small
set of widget-specific overrides for inputs, sliders, checkboxes, toggles,
radios, progress bars, and scrollbars.

Callers usually assign `ctx.theme` before `wlx_begin()`:

```c
ctx.theme = dark_mode ? &wlx_theme_dark : &wlx_theme_light;
wlx_begin(&ctx, root, wlx_process_raylib_input);
```

If `ctx.theme` is `NULL`, `wlx_begin()` selects `&wlx_theme_dark` for the
frame. Widget resolve functions then fill omitted option fields from the
active theme. For example, a button without `.front_color`, `.back_color`,
`.font_size`, or `.roundness` uses the current theme values, while explicit
option values still override them at the call site.

`WLX_Theme` is intentionally flat enough to copy and edit. Application code can
start from a built-in preset, override the fields it cares about, and assign
the resulting theme pointer each frame.

### Built-in Presets

Wollix ships three public theme presets:

| Preset | Description |
|--------|-------------|
| `wlx_theme_dark` | Neutral dark default with low visual weight, blue accent states, and a darkened disabled treatment (`disabled_brightness = -0.35f`, `disabled_opacity = 0.55f`). |
| `wlx_theme_light` | Bright preset with dark text, pale surfaces, darker hover feedback, and a lightened disabled treatment (`disabled_brightness = +0.30f`, `disabled_opacity = 0.55f`). |
| `wlx_theme_glass` | Dark translucent preset for layered surfaces, stronger focus borders, and a softly darkened disabled treatment (`disabled_brightness = -0.25f`, `disabled_opacity = 0.55f`). |

The live dashboard includes a Theme Lab that compares the built-in presets,
their semantic token ramps, and common component states:
[https://dberzins.github.io/wollix/](https://dberzins.github.io/wollix/).

### Theme Inheritance

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

### Component States

Interactive widgets pass through a small set of canonical states. The names
below are the design-system vocabulary; the runtime fields live on
`WLX_Interaction` (and one widget option) and follow the same shape:

| State | Origin | Default appearance |
|-------|--------|--------------------|
| Default  | No interaction | Resolved theme colors as-is |
| Hover    | `WLX_Interaction.hover` is true | Background shifted by `theme->hover_brightness` |
| Focused  | `WLX_Interaction.focused` is true (FOCUS-mode widgets) | Focus border (`theme->input.border_focus`, falling back to accent) |
| Active   | `WLX_Interaction.active` is true (pressed, dragged, or focused) | Per-widget treatment where implemented; sliders tint the active thumb |
| Disabled | `.disabled = true` on the option (mirrored as `WLX_Interaction.disabled`) | Colors shifted by `theme->disabled_brightness`; alpha multiplied by `theme->disabled_opacity`; hover tint suppressed |

The disabled-state model exposes only `disabled` as a public option field; the
other states remain implicit interaction flags. If you need a stricter state
ramp (per-widget hover/focused/active colors), derive it locally the same way
the showcase layers derive semantic color roles. A typed `WLX_State_Flags` enum
is a future direction once the component-recipe work has settled which roles
repeat in practice.

The disabled treatment composes with the existing opacity stack: the
effective alpha is `theme->disabled_opacity * widget.opacity *
theme->opacity * ctx_stack_opacity`. Stacking `wlx_push_opacity(0.5f)` over
an already-disabled widget compounds the dim and is treated as caller
responsibility (no implicit readability floor).

### Custom Themes

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

ctx.theme = &app_theme;
wlx_begin(&ctx, root, wlx_process_raylib_input);
```

The standalone [../demos/theme_demo.c](../demos/theme_demo.c) shows runtime
preset switching, editable custom themes built from a preset, widget-specific
theme fields, and per-widget overrides.

---

## "Mechanical Glass" Showcase

The dashboard is the canonical Wollix showcase. It is a high-fidelity technical
interface for power users, engineers, and data analysts. Its "Mechanical Glass"
aesthetic fuses the precision of industrial hardware with the depth of layered
digital overlays, in both a high-contrast dark and a high-luminance light mode.
It is built **on top of** the core `WLX_Theme` contract above: the public theme
provides the base values, and the dashboard derives a richer application-local
token layer from them -- exactly the pattern the core contract prescribes for
larger UI surfaces.

The token model and recipes live as demo-local headers under `demos/dashboard/`
(`dashboard_theme.h`, `dashboard_components.h`, `dashboard_effects.h`,
`dashboard_icons.h`). This section is their conceptual reference.

### Brand & Aesthetic

The brand personality is clinical, sophisticated, and authoritative -- a
high-tech laboratory or command center that carries high information density
without sacrificing clarity. Hierarchy comes from layering, transparency, and
refined luminescence rather than flat color blocks.

**Style pillars**

- **Technological depth.** Semi-transparent surfaces and layered panels create a
  multi-level workspace. (True backdrop blur is an aspiration, not yet shipped --
  see [Effects](#effects).)
- **Precision.** Sharp geometry and tight, 4px-grid spacing reflect a
  hardware-inspired origin.
- **Luminosity.** Essential data and primary actions use vibrant accents to cut
  through the surface and guide the eye.

### Two-Tier Model: Theme Contract + Token Layer

Wollix keeps a deliberately compact public theme (`WLX_Theme`). The dashboard
needs more vocabulary -- surface-elevation ramps, on-color roles, status hues,
glass edges, a typography scale -- so it owns an **application-local token
bundle** (`Dashboard_Tokens`) and maps a baseline subset back into `WLX_Theme`
each frame for ordinary widgets.

```
Dashboard_Tokens (dark | light)        WLX_Theme (per frame, ordinary widgets)
  color / ramp / status / table   --->   background, foreground, surface, border,
  glass / type / spacing / radius        accent, font, padding, roundness,
  effects                                 input/slider/checkbox/toggle/radio/
                                          progress/scrollbar overrides
```

- `dashboard_tokens(mode)` selects the active bundle (`dashboard_tokens_dark` or
  `dashboard_tokens_light`).
- `dashboard_wlx_theme(mode, fonts)` builds the `WLX_Theme` from those tokens
  (see [WLX_Theme Mapping](#wlx_theme-mapping)).
- Component recipes draw directly from `Dashboard_Tokens` for chrome the core
  theme does not model (glass panels, two-tone bevels, status chips, tables).

This keeps the core API stable while giving the dashboard enough vocabulary for
its own navigation, panels, tables, and status states.

### Token Model

`Dashboard_Tokens` bundles: `mode`, `color`, `ramp`, `status`, `table`, `glass`,
`type`, `spacing`, `radius`, `effects`. Both modes populate every role; roles a
mode's export leaves unspecified are derived to stay legible on that mode's base
surface and are re-tunable without changing the token shape.

#### Color roles

| Role | Purpose | Dark | Light |
|------|---------|------|-------|
| `background` | Window base surface | `#0f1419` | `#f7f9ff` |
| `surface` | Default module/panel surface | `#1b2025` | `#eaeef6` |
| `surface_variant` | Alternate surface (tracks) | `#30353b` | `#dee3ea` |
| `field` | Input/control background | `rgb(5,7,10)` | `#ffffff` |
| `field_border` | Resting input/control border | `#1a242f` | `#bbc9cf` |
| `outline` | Default border | `#859399` | `#6c797f` |
| `outline_variant` | Subtle divider border | `#3c494e` | `#bbc9cf` |
| `accent` | Neon primary accent (fills/washes/glows) | `#00d1ff` | `#00d1ff` |
| `accent_strong` | Emphasized accent / accent **text** | `#4cd6ff` | `#00677f` |
| `tertiary` | Industrial-orange callouts | `#eb8104` | `#eb8104` |
| `on_surface` | Primary text/icon | `#dee3ea` | `#171c21` |
| `on_surface_muted` | Secondary/muted text | `#bbc9cf` | `#3c494e` |
| `on_accent` | Text/icon on the accent fill | `#0a0f14` | `#001f28` |

> **Accent polarity.** The bright electric blue (`accent`) reads as a fill/wash
> on both modes, but on **light** surfaces it washes out as *content*, so accent
> text/glyphs/thin strokes use `accent_strong` (the deep `#00677f`). In **dark**
> both resolve to the neon accent. The dashboard expresses this with
> `dash_accent_fg(tk)`.

#### Surface elevation ramp

Five steps, lowest -> highest, with strictly monotonic luminance (increasing in
dark, decreasing in light). Used for nested surfaces, bars, and zebra rows.

| Step | Dark | Light |
|------|------|-------|
| `lowest` | `#0a0f14` | `#ffffff` |
| `low` | `#171c21` | `#f0f4fb` |
| `base` | `#1b2025` | `#eaeef6` |
| `high` | `#252a30` | `#e4e8f0` |
| `highest` | `#30353b` | `#dee3ea` |

#### Status colors

| Role | Dark | Light | Meaning |
|------|------|-------|---------|
| `success` | `#00b36e` | `#00b36e` | Clinical mint |
| `warning` | `#eb8104` | `#eb8104` | Industrial orange |
| `error` | `#cf2c2c` | `#d32f2f` | Signal red |
| `info` | `#00d1ff` | `#00677f` | Accent (mode-appropriate) |

#### Data-table colors

| Role | Dark | Light |
|------|------|-------|
| `header_bg` | `surface-container-high` | `surface-container-high` |
| `row_bg` | `surface-container` | `surface-container-lowest` |
| `zebra_bg` | `surface-container-low` | `surface-container-low` |
| `grid_line` | white @ 15% | black @ 10% |

#### Glass (edges, fill, glow)

Glass panels are a translucent fill bounded by a **two-tone bevel** -- a light
top/left "light leak" and a darker bottom/right fall -- plus a low-alpha accent
glow source.

| Role | Dark | Light |
|------|------|-------|
| `edge_light` (top/left) | white @ 0.15 | white @ 0.80 |
| `edge_dark` (bottom/right) | black @ 0.20 | black @ 0.05 |
| `fill` | `#1a242f` @ ~70% | white @ ~65% |
| `glow` | `#00d1ff` @ ~5% | `#00d1ff` @ ~15% |

The bevel is expressed through Wollix **per-side container decor** (per-side
border colors/widths on `wlx_layout_begin`), so the chrome stays inside the
container's draw range. Per-side strokes are sharp spans and do **not** follow
the corner arc (only the fill rounds -- see [Elevation & Depth](#elevation--depth)).

#### Typography

Dual-font system: **Geist** (sans) for display/headers/body, **JetBrains Mono**
for labels and metadata (numerical data aligns in tables and readouts). Headings
use tight tracking; small labels are uppercased.

| Role | Family | Size | Weight | Line-height | Tracking |
|------|--------|------|--------|-------------|----------|
| `display` | Geist | 48 | Bold (700) | 56 | -1 |
| `headline_lg` | Geist | 32 | SemiBold (600) | 40 | 0 |
| `headline_md` | Geist | 20 | SemiBold | 28 | 0 |
| `headline_sm` | Geist | 20 (dark) / 18 (light) | SemiBold | 28 / 24 | 0 |
| `body_lg` | Geist | 16 | Regular (400) | 24 | 0 |
| `body_md` | Geist | 14 | Regular | 20 | 0 |
| `body_sm` | Geist | 12 | Regular | 16 | 0 |
| `label` | JetBrains Mono | 12 | Medium (500) | 16 | +1 |
| `mono` | JetBrains Mono | 13 | Medium | 16 | 0 |

- A role names **intent** (family + size + weight + line-height + tracking); the
  live `WLX_Font` handle is resolved at runtime from `(family, weight)` via
  `dashboard_font_resolve` / `dashboard_type_font`.
- Rendered sizes are scaled by `DASHBOARD_TEXT_SCALE` (default `1.14`) through
  `dashboard_type_px(role)`, so token sizes stay at their design values while the
  on-screen size is tuned in one place.

#### Spacing

A strict 4px grid. All values are whole multiples of `unit`.

| Token | px | Token | px |
|-------|----|-------|----|
| `unit` | 4 | `xl` | 24 |
| `xs` | 4 | `xxl` | 32 |
| `sm` | 8 | `gutter` | 16 (module gutter) |
| `md` | 12 | `margin` | 32 (page margin) |
| `lg` | 16 | `panel_padding` | 12 (inner module) |
|  |  | `container_max` | 1440 (max content width) |

#### Radius

Ascending pixel radii; `full` is the pill/circle sentinel.

| Token | px | Typical use |
|-------|----|-------------|
| `sm` | 2 | Hairline chips |
| `base` | 4 | Buttons, inputs, small cards (`rounded-sm`) |
| `md` | 6 |  |
| `lg` | 8 | Main containers, modules, modals (`rounded-lg`) |
| `xl` | 12 |  |
| `full` | 9999 | Pills, circles |

> Radii are declared as **absolute pixels** on container decor via
> `.corner_radius = (float)tk->radius.lg` (8 px), so differently sized panels
> share one corner regardless of size. See the `corner_radius` field in
> [WIDGETS.md](WIDGETS.md) / [LAYOUT_MODEL.md](LAYOUT_MODEL.md).

#### Effects

Each visual treatment carries an **intent**: `APPROXIMATED` (realizable with
existing Wollix primitives) or `CORE_REQUIRED` (needs an escalated core change
before it can ship faithfully).

| Effect | Intent | Realization today |
|--------|--------|-------------------|
| `glass_edge` | Approximated | Per-side two-tone border decor |
| `glow` | Approximated | `WLX_GLOW_FIELDS` decor |
| `shadow` | Approximated | `WLX_SHADOW_FIELDS` decor |
| `gradient` | Approximated | `WLX_GRADIENT_FIELDS` vertical gradient |
| `scanline` | Approximated | `dashboard_draw_scanlines` overlay |
| `backdrop_blur` | **Core required** | Not yet shipped; glass fill uses a flat translucent surface instead |

### WLX_Theme Mapping

`dashboard_wlx_theme(mode, fonts)` maps tokens into the public theme so ordinary
Wollix widgets (label, button, inputbox, slider, checkbox, toggle, radio,
progress, scrollbar) inherit the dashboard look with no per-call styling.

| `WLX_Theme` field | Source |
|-------------------|--------|
| `background` | `color.background` |
| `foreground` | `color.on_surface` |
| `surface` | `color.surface` |
| `border` | `color.outline_variant` |
| `border_width` | `1.0` |
| `accent` | `color.accent` (drives every active/focused affordance) |
| `font` | Geist Regular |
| `font_size` | `type.body_md.size` (14) |
| `padding` | `spacing.sm` (8) -- used when a widget opts in with `WLX_PADDING_USE_THEME` |
| `roundness` | `0.15` (~ the 4px base radius on typical controls) |
| `min_rounded_segments` | `16` |
| `hover_brightness` | `+0.08` dark / `-0.08` light |
| `disabled_brightness` | `-0.35` dark / `+0.30` light |
| `disabled_opacity` | `WLX_DEFAULT_DISABLED_OPACITY` |
| `input` | `border_focus = accent`, `cursor = on_surface`, `border_width = 1` |
| `slider` | `track = surface_variant`, `thumb = accent`, `label = on_surface`, `track_height = 6`, `thumb_width = 14` |
| `checkbox` | `check = accent`, `border = outline_variant`, `border_width = 1` |
| `toggle` | `track = ramp.low`, `track_active = accent`, `thumb = on_surface` |
| `radio` | `ring = outline_variant`, `fill = accent`, `label = on_surface`, `border_width = 1.5` |
| `progress` | `track = surface_variant`, `fill = accent`, `track_height = 6` |
| `scrollbar` | `bar = outline_variant`, `width = 10` |

Per the public theme contract, any widget option set at the call site still
overrides these defaults; zero/sentinel values mean "use the theme".

### Component Recipes

Recipes compose Wollix layouts/widgets into the dashboard's named components.
They are pure UI builders over `Dashboard_Tokens` and the loaded fonts; none
modify the core. Definitions live in `demos/dashboard/` demo-local headers and
`dashboard.c`.

| Recipe | Builds | Key Wollix primitives |
|--------|--------|-----------------------|
| `dashboard_module_begin` / `_end` | Glass module surface (translucent fill + two-tone bevel + 8px corner) | `wlx_layout_begin` per-side decor + `corner_radius` |
| `dashboard_heading` / `dashboard_panel_heading` | Uppercase mono section heading | `wlx_label` (mono `label` role) |
| `dashboard_stat_card` | Hero metric card: label + icon, big value + unit, segmented usage bar (`fraction`, `segments`); fed the Overview's live metrics | module + nested layouts + `dashboard_segmented_progress` |
| `dashboard_button_primary` | Solid accent button (dark on-accent ink) | `wlx_button` |
| `dashboard_status_chip` | LED-dot + mono text status chip | `wlx_layout_begin` border + `wlx_widget` disc + `wlx_label` |
| `dashboard_badge` | Compact status badge | bordered layout + label |
| `dashboard_status_pip` | Small status indicator dot | `wlx_widget` (roundness 1.0 disc) |
| `dashboard_segmented_progress` | Digital "readout" segmented bar | grid/row of accent vs muted segments |
| `dashboard_table` | Dense data table: header + zebra rows + grid lines | layouts + `table` tokens + `dashboard_table_row_rect` |
| `dashboard_sidebar` / `dashboard_nav_item` | Nav rail; active item gets accent wash + left accent bar | `dashboard_icon_button` + per-side decor |
| `dashboard_list_row` / `dashboard_icon_row` | Widget-list / structural-layout rows | nested layout + `wlx_label` + chevron icon |
| `dashboard_feature_card` | Icon-chip + title + subtitle card | module recipe + `dashboard_icon_chip` |
| `dashboard_integration_row` | Outlined icon chip + label + chevron; a click target that opens a repo/doc URL via the open-URL hook (accent hover) | icon chip + label + icon + `wlx_get_interaction` |
| `dashboard_footer_link` | Sidebar Documentation/Support link; click target opening a URL via the open-URL hook (accent hover) | `dashboard_icon_label` + `wlx_get_interaction` |
| `dashboard_icon_chip` | Fixed-size filled/outlined glyph box | layout decor + `dashboard_icon_image` |
| `dashboard_topbar` | App bar: logo/version, search, theme switch, icons, identity | layouts + `wlx_inputbox` + icon buttons |
| `dashboard_tab` | Tab-bar button | `wlx_button` |
| `dashboard_responsive_grid_begin` / `_end` | Column-count-responsive grid | `wlx_grid_begin` with `dashboard_grid_columns(width)` |
| `dashboard_icon_image` / `_button` / `_label` | Atlas glyph as image / button image / label image | `wlx_image` / `wlx_button` / `wlx_label` + icon atlas |

Pure helpers: `dashboard_text_style` (role -> `WLX_Text_Style`), `dashboard_upper`
(ASCII uppercase), `dashboard_grid_columns`, `dashboard_pulse_alpha`,
`dashboard_table_row_rect`. Visual-effect helpers (`dashboard_effects.h`):
`dashboard_color_lerp`, `dashboard_desaturate`, `dashboard_blur_tint`,
`dashboard_glow_ring_alpha`, `dashboard_draw_scanlines`.

### Elevation & Depth

Depth comes from optical layering, not heavy shadows.

- **Level 0 -- base.** `background` / `ramp.lowest` (the "void").
- **Level 1 -- panels.** Glass modules: translucent `glass.fill` with a two-tone
  bevel (`edge_light` top/left, `edge_dark` bottom/right) and an 8px corner.
- **Level 2 -- overlays.** Higher-contrast surfaces with a low-alpha accent
  `glass.glow` (dark) or a defined contact treatment (light).

Mechanics:

- **Two-tone bevel** via per-side container decor (see [LAYOUT_MODEL.md
  Per-side container decor](LAYOUT_MODEL.md)). Borders are sharp spans; only the
  fill follows the corner radius.
- **Absolute pixel radius** (`corner_radius`) keeps every panel at a
  uniform 8px corner regardless of size.
- **Glow / shadow / gradient** are first-class decor fields.
- **Scanlines** are a dark-mode-only overlay on the terminal/log surface.
- **Backdrop blur** is the one treatment that is *not* yet expressible in the
  core; the glass fill approximates it with a flat translucent surface.

### Dark vs Light

Layout and content are identical across modes; only color derivation differs.

- **Accent content** uses `accent_strong` in light (legible deep primary) and the
  neon `accent` in dark -- `dash_accent_fg(tk)`.
- **Hairlines** are a light line on dark surfaces, a dark line on light --
  `dash_hairline(tk, alpha)`.
- **Overlay polarity** (terminal log tags, window dots, metric ink) flips per
  mode while keeping the same hue intent.
- The mode is switched at runtime by the top-bar palette icon (no key binding).

---

## References

- Public API: [WIDGETS.md](WIDGETS.md), [LAYOUT_MODEL.md](LAYOUT_MODEL.md),
  [API_REFERENCE.md](API_REFERENCE.md), [SENTINEL.md](SENTINEL.md),
  [OPACITY.md](OPACITY.md).
- Gallery (secondary) reference:
  [GALLERY_DESIGN_SYSTEM.md](GALLERY_DESIGN_SYSTEM.md) documents the gallery's
  application-local semantic layer.
- Decor primitives: per-side border, glow/shadow, gradient fill, and absolute
  corner radius -- all documented in [WIDGETS.md](WIDGETS.md).
- Implementation: `demos/dashboard/dashboard_theme.h` (tokens + `WLX_Theme`
  mapper), `dashboard_components.h`, `dashboard_effects.h`, `dashboard_icons.h`,
  `dashboard.c`.
