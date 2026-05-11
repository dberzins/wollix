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
| `pos` | `int` | `-1` | Target slot index in the parent layout. `-1` = next sequential slot |
| `span` | `size_t` | `1` | Number of consecutive slots to occupy |
| `overflow` | `bool` | `false` | Allow the widget rect to exceed slot bounds using explicit width/height |
| `padding` | `float` | `0` | Uniform inset applied to the slot before the widget uses it |

### Sizing fields (`WLX_WIDGET_SIZING_FIELDS`)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `widget_align` | `WLX_Align` | `WLX_LEFT` | Where to place the widget inside its slot when it is smaller than the slot |
| `width` | `float` | `-1` | Widget width in pixels. `-1` = fill parent width |
| `height` | `float` | `-1` | Widget height in pixels. `-1` = fill parent height |
| `min_width` | `float` | `0` | Minimum width constraint. `0` = unconstrained |
| `min_height` | `float` | `0` | Minimum height constraint. `0` = unconstrained |
| `max_width` | `float` | `0` | Maximum width constraint. `0` = unconstrained |
| `max_height` | `float` | `0` | Maximum height constraint. `0` = unconstrained |
| `opacity` | `float` | `-1` | Opacity multiplier. Negative = inherit theme and opacity stack; `0.0`–`1.0` = explicit alpha |

### Typography fields (`WLX_TEXT_TYPOGRAPHY_FIELDS`)

Used by `label`, `button`, `checkbox`, `inputbox`, `toggle`, and `radio`.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `font` | `WLX_Font` | `WLX_FONT_DEFAULT` | Font handle. `0` = backend default / theme font |
| `font_size` | `int` | `0` | Font size in pixels. `0` = use theme default |
| `align` | `WLX_Align` | `WLX_LEFT` | Text alignment within the widget rect |
| `wrap` | `bool` | varies | Enable fitted multi-line wrapping for long text. Width wrapping stays greedy at UTF-8 codepoint boundaries; explicit `\n`, `\r\n`, and `\r` always break lines |
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
| `border_width` | `float` | `-1` | Border width. `-1` = theme/widget default, `0` = no border |
| `roundness` | `float` | `-1` | Corner roundness. `-1` = theme default |
| `rounded_segments` | `int` | `-1` | Segment count for rounded drawing. `-1` = theme default |

### Identity field

All leaf-widget opt structs include an optional identity field:

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `const char *` | `NULL` | An explicit string ID. When non-NULL, the string is hashed and pushed onto the ID stack for the duration of the widget call, giving it a stable identity independent of call-site order. Useful for widgets created in loops or dynamic lists. Compound widgets that keep their body open document their scope-style `id` behavior in their own sections. See [LAYOUT_MODEL § Explicit String IDs](LAYOUT_MODEL.md) for details. |

---

## `wlx_label`

Static text label. Does not return a value and has no interactive behavior
beyond hover highlighting (when `show_background` is true). Wrapped labels are
measured and drawn as whole visible lines, and explicit newlines preserve empty
visual lines.

### Signature

```c
void wlx_label(WLX_Context *ctx, const char *text, ...options);
```

### Minimal example

```c
wlx_label(ctx, "Hello, world!",
    .font_size = 20, .align = WLX_CENTER
);
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `show_background` | `bool` | `false` | Draw a filled background rectangle behind the text |

All shared placement, sizing, typography, color, and border fields also apply.

### Common overrides

Centered heading with background:

```c
wlx_label(ctx, "Settings",
    .font_size = 28,
    .align = WLX_CENTER,
    .height = 50,
    .back_color = (WLX_Color){40, 40, 40, 255},
    .show_background = true
);
```

Right-aligned status line:

```c
wlx_label(ctx, status_text,
    .font_size = 14,
    .align = WLX_RIGHT,
    .height = 30,
    .front_color = (WLX_Color){150, 150, 150, 255}
);
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
| `texture` | `WLX_Texture` | zero handle | Optional image content. `width <= 0` or `height <= 0` means no image. |
| `texture_src` | `WLX_Rect` | `{0}` | Source sub-rect. `w <= 0` or `h <= 0` means full texture. |
| `texture_scale` | `WLX_Image_Scale` | `WLX_IMAGE_SCALE_FIT` | How the texture fits its image rect (same modes as `wlx_image`). |
| `texture_tint` | `WLX_Color` | `{0}` | Tint applied to the texture. `{0}` resolves to `WLX_WHITE`. The alpha is multiplied by the opacity stack. |
| `image_placement` | `WLX_Image_Placement` | `WLX_IMAGE_PLACEMENT_LEFT` | Where the image sits relative to text (`LEFT`, `RIGHT`, `TOP`, `BOTTOM`). |
| `image_size` | `float` | `0` | Reserved square size for the image. `<= 0` is automatic: derived from `font_size` for image+text, or full button rect for image-only. |
| `image_text_gap` | `float` | `-1` | Pixels between image and text. `< 0` resolves to `font_size * 0.5`. |

Shared placement, sizing, typography, color, and border fields also apply.
The button always draws a filled `back_color` rectangle — hover brightens it
automatically using the theme's `hover_brightness`. Button captions use the
same fitted line/run layout as `wlx_label`, so centered or wrapped captions
align per visible line.

### Content rules

- **Empty texture + non-empty text** → behaves exactly like a text-only
  button (no texture draw is emitted).
- **Empty texture + empty text** → only the chrome is drawn; the button still
  returns the click contract.
- **`align` vs. `image_placement`** are independent:
  `image_placement` controls *which side* of the text the image sits on (image
  is always on the LEFT/RIGHT/TOP/BOTTOM of the text within the combined block);
  `align` then positions the *combined block* inside the button rect for
  image+text mode, or the image rect itself for image-only mode when
  `image_size > 0`.
- The texture is always centered inside its reserved image rect; hover
  feedback applies to the chrome background only — texture tint is not
  modulated by hover.

### Common overrides

Text-only button with custom color:

```c
if (wlx_button(ctx, "Submit",
    .widget_align = WLX_CENTER,
    .width = 200, .height = 50,
    .font_size = 22,
    .align = WLX_CENTER,
    .back_color = (WLX_Color){128, 0, 0, 255}
)) {
    submit_form();
}
```

Image-only icon button (full button rect as image target):

```c
if (wlx_button(ctx, "",
    .width = 48, .height = 48,
    .texture = save_icon, .align = WLX_CENTER
)) {
    save_data();
}
```

Image-only with explicit square size, anchored via `align`:

```c
if (wlx_button(ctx, "",
    .height = 48,
    .texture = save_icon, .image_size = 32, .align = WLX_CENTER
)) {
    save_data();
}
```

Image + text with default LEFT placement:

```c
if (wlx_button(ctx, "Save",
    .height = 40, .font_size = 18, .align = WLX_CENTER,
    .texture = save_icon
)) {
    save_data();
}
```

Image + text with image stacked on top:

```c
if (wlx_button(ctx, "Save",
    .height = 64, .font_size = 14, .align = WLX_CENTER,
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
        .image_size = 32, .align = WLX_CENTER);
wlx_pop_opacity(ctx);
```

---

## `wlx_checkbox`

Toggle checkbox with a text label. Draws a square indicator beside the label,
or uses textures when `tex_checked` / `tex_unchecked` are provided. Returns
`true` on the frame the checked state changes.

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
| `border_width` | `float` | `-1` | Indicator border width. `-1` = theme `checkbox.border_width` |
| `roundness` | `float` | `-1` | Indicator corner roundness. `-1` = theme default |
| `rounded_segments` | `int` | `-1` | Segment count for rounded drawing. `-1` = theme default |
| `check_color` | `WLX_Color` | `{0}` | Color of the check mark. `{0}` = theme `checkbox.check` |
| `tex_checked` | `WLX_Texture` | zero handle | Texture used for the checked state |
| `tex_unchecked` | `WLX_Texture` | zero handle | Texture used for the unchecked state |

All shared placement, sizing, typography, and color fields also apply.
Default `wrap` is `false` for checkbox. `.back_color` fills the indicator in
native mode and tints the texture in texture mode; there is no separate
row-background toggle.

### Common overrides

Styled checkbox with custom font size:

```c
if (wlx_checkbox(ctx, "Enable notifications", &notify_enabled,
    .font_size = 20,
    .check_color = (WLX_Color){0, 200, 0, 255}
)) {
    update_notifications(notify_enabled);
}
```

Texture-backed checkbox using the same API:

```c
if (wlx_checkbox(ctx, "Favorite", &favorited,
    .tex_checked   = star_filled_tex,
    .tex_unchecked = star_empty_tex,
    .back_color = (WLX_Color){255, 255, 255, 255}
)) {
    printf("Favorited: %s\n", favorited ? "yes" : "no");
}
```

Use `wlx_checkbox(..., .tex_checked = ..., .tex_unchecked = ...)` instead of
the removed `wlx_checkbox_tex` compatibility macro.

---

## `wlx_inputbox`

Text input field. Click to focus, type to edit, press Enter or Escape (or
click elsewhere) to unfocus. Returns `true` while the field is focused.

Uses persistent state internally (`WLX_Inputbox_State`) to track cursor
position and blink timer across frames. The buffer is edited as one UTF-8 byte
buffer, but when `wrap` is `true` the visible text can span multiple fitted
visual lines and the cursor follows that same wrapped layout.

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
| `content_padding` | `float` | `10` | Horizontal padding inside the text editing area |
| `border_color` | `WLX_Color` | `{0}` | Border color in unfocused state. `{0}` = theme `border` |
| `border_width` | `float` | `-1` | Border width. `-1` = theme `input.border_width`, then theme `border_width` |
| `roundness` | `float` | `-1` | Corner roundness. `-1` = theme default |
| `rounded_segments` | `int` | `-1` | Segment count for rounded drawing. `-1` = theme default |
| `border_focus_color` | `WLX_Color` | `{0}` | Border color when focused. `{0}` = theme `input.border_focus` |
| `cursor_color` | `WLX_Color` | `{0}` | Blinking cursor color. `{0}` = theme `input.cursor` |

All shared placement, sizing, typography, and color fields also apply.
Default `wrap` is `true` for inputbox, so long buffers display over multiple
visual lines inside the widget and cursor placement stays aligned with that
wrapped rendering.

### Common overrides

Form-style input with fixed height:

```c
wlx_inputbox(ctx, "Email:", email_buf, sizeof(email_buf),
    .height = 45,
    .font_size = 18,
    .border_focus_color = (WLX_Color){0, 120, 255, 255}
);
```

Tracking focus transitions:

```c
static bool was_focused = false;
bool focused = wlx_inputbox(ctx, "Search:", query, sizeof(query), .height = 35);

if (focused && !was_focused) {
    printf("Search field gained focus\n");
}
if (!focused && was_focused) {
    printf("Search submitted: %s\n", query);
}
was_focused = focused;
```

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
    .widget_align = WLX_CENTER, .width = 400, .height = 40
)) {
    set_volume(volume);
}
```

### Widget-specific options

Slider has its own typography and color fields (not the shared
`WLX_TEXT_COLOR_FIELDS` / `WLX_TEXT_TYPOGRAPHY_FIELDS`).

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `font` | `WLX_Font` | `WLX_FONT_DEFAULT` | Font for label and value text |
| `font_size` | `int` | `0` | Font size. `0` = use theme default |
| `align` | `WLX_Align` | `WLX_LEFT` | Text alignment for the label |
| `spacing` | `int` | `0` | Opt-in extra tracking for label and value text. `0` = natural backend spacing |
| `show_label` | `bool` | `true` | Show the numeric value to the right of the track |
| `track_color` | `WLX_Color` | `{0}` | Track bar background color. `{0}` = derive from theme `slider.track` |
| `thumb_color` | `WLX_Color` | `{0}` | Thumb handle color. `{0}` = theme `slider.thumb` |
| `label_color` | `WLX_Color` | `{0}` | Text label color. `{0}` = theme `slider.label` |
| `track_height` | `float` | `0` | Height of the track bar. `0` = theme default (6) |
| `thumb_width` | `float` | `0` | Width of the thumb handle. `0` = theme default (14) |
| `border_color` | `WLX_Color` | `{0}` | Border color for track/thumb outlines. `{0}` = theme `border` |
| `border_width` | `float` | `-1` | Border width. `-1` = theme `border_width` |
| `roundness` | `float` | `-1` | Corner rounding for track/fill/thumb. `-1` = theme default |
| `rounded_segments` | `int` | `-1` | Segment count for rounded drawing. `-1` = theme default |
| `hover_brightness` | `float` | `WLX_FLOAT_UNSET` | Brightness shift on track hover. Unset = theme default |
| `thumb_hover_brightness` | `float` | `WLX_FLOAT_UNSET` | Brightness shift on thumb hover. Unset = theme default |
| `fill_inactive_brightness` | `float` | `-0.3` | Brightness offset for the filled portion of the track |
| `min_value` | `float` | `0.0` | Minimum slider value |
| `max_value` | `float` | `1.0` | Maximum slider value |

Shared placement and sizing fields also apply.

### Common overrides

Color channel slider with colored thumb:

```c
wlx_slider(ctx, "Red", &color_r,
    .widget_align = WLX_CENTER, .width = 500, .height = 40,
    .min_value = 0.0f, .max_value = 1.0f,
    .thumb_color = (WLX_Color){255, 60, 60, 255}
);
```

Integer-range slider (0–100) with custom font:

```c
wlx_slider(ctx, "Speed", &speed,
    .widget_align = WLX_CENTER, .width = 400, .height = 40,
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
| `color` | `WLX_Color` | `{0}` | Divider color. `{0}` = theme `border` |
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

Shared placement, sizing, and border fields also apply.

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
| `hover_brightness` | `float` | `WLX_FLOAT_UNSET` | Hover brightness override. Unset = theme `hover_brightness` |

Shared placement, sizing, typography, text-color, and border fields also
apply. Default `wrap` is `false`. `front_color` styles the label text;
`back_color` is currently unused by this widget.

### Common overrides

Compact toolbar toggle:

```c
wlx_toggle(ctx, "Grid", &show_grid,
    .font_size = 14,
    .height = 26,
    .widget_align = WLX_RIGHT
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
| `ring_border_width` | `float` | `-1` | Ring outline width. `-1` = theme `radio.border_width`, then theme `border_width` |
| `hover_brightness` | `float` | `WLX_FLOAT_UNSET` | Hover brightness override. Unset = theme `hover_brightness` |

Shared placement, sizing, typography, and text-color fields also apply.
Default `wrap` is `false`. `front_color` styles the label text; `back_color`
is currently unused by this widget.

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
| `content_height` | Total height of the scrollable content in pixels. Use `-1` for **auto-height** mode (measured from children automatically) |

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
| `scrollbar_color` | `WLX_Color` | `{0}` | Scrollbar thumb color. `{0}` = theme `scrollbar.bar` |
| `scrollbar_hover_brightness` | `float` | `WLX_FLOAT_UNSET` | Brightness shift when hovering the scrollbar. Unset = theme default |
| `scrollbar_width` | `float` | `-1` | Width of the scrollbar. `-1` = theme default |
| `wheel_scroll_speed` | `float` | `20.0` | Pixels scrolled per mouse wheel tick |
| `show_scrollbar` | `bool` | `true` | Whether to draw the scrollbar |
| `border_color` | `WLX_Color` | `{0}` | Panel border color. `{0}` = theme `border` |
| `border_width` | `float` | `-1` | Panel border width. `-1` = theme `border_width` |
| `roundness` | `float` | `-1` | Panel corner roundness. `-1` = theme default |
| `rounded_segments` | `int` | `-1` | Segment count for rounded drawing. `-1` = theme default |

Shared placement and sizing fields also apply (no typography or text-color fields).
When `.id` is set, that scope stays active for the full scroll-panel body
until `wlx_scroll_panel_end(ctx)`.

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
                .height = 40, .font_size = 18, .align = WLX_CENTER,
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
| `padding` | `float` | `4` | Uniform padding for the split container |
| `padding_top` | `float` | `-1` | Per-side override. `-1` = inherit `padding` |
| `padding_right` | `float` | `-1` | Per-side override. `-1` = inherit `padding` |
| `padding_bottom` | `float` | `-1` | Per-side override. `-1` = inherit `padding` |
| `padding_left` | `float` | `-1` | Per-side override. `-1` = inherit `padding` |
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
| `border_color` | `WLX_Color` | `{0}` | Panel border color. `{0}` = none |
| `border_width` | `float` | `0` | Border thickness in pixels. `0` = no border |
| `roundness` | `float` | `0` | Corner roundness for border/background. `0` = sharp |
| `clip` | `bool` | `false` | Clip body content to panel bounds |
| `padding` | `float` | `2` | Inner layout padding |
| `padding_top` | `float` | `-1` | Per-side override. `-1` = inherit `padding` |
| `padding_right` | `float` | `-1` | Per-side override. `-1` = inherit `padding` |
| `padding_bottom` | `float` | `-1` | Per-side override. `-1` = inherit `padding` |
| `padding_left` | `float` | `-1` | Per-side override. `-1` = inherit `padding` |
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
        .padding = 0);
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
| `color` | `WLX_Color` | `{0}` | Fill color of the rectangle |

All shared placement, sizing, and border fields also apply.

### Minimal example

```c
wlx_widget(ctx,
    .widget_align = WLX_CENTER, .width = 100, .height = 4,
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
| `align` | `WLX_Align` | `WLX_CENTER` | Positions the image content within the slot for `FIT` and `NONE`; selects the crop anchor for `FILL`. Has no effect for `STRETCH`. |
| `tint` | `WLX_Color` | `{0}` | Tint applied to the texture. `{0}` resolves to `WLX_WHITE` (no tint). The alpha component is multiplied by the opacity stack. |
| `src` | `WLX_Rect` | `{0}` | Source sub-rect within the texture. `{0}` (or `src.w <= 0`) means the full texture. Use this for spritesheet cells. |
| `id` | `const char *` | `NULL` | Optional widget ID for persistent state and scoping. |

Shared placement, sizing (`widget_align`, `width`, `height`, etc.), and border
fields also apply.

> **`widget_align` vs. `align`**: `widget_align` positions the *widget slot*
> within the parent layout (e.g. center a 100px slot inside a 300px column).
> `align` positions the *image content* within that widget slot (e.g. pin a
> natural-size image to the top-left corner of the slot). They are independent.

### Common overrides

Fit a portrait photo inside a landscape card, centered:

```c
wlx_image(ctx, portrait_photo,
    .width = 200, .height = 150,
    .scale = WLX_IMAGE_SCALE_FIT,
    .align = WLX_CENTER
);
```

Fill a thumbnail slot, crop from the left edge:

```c
wlx_image(ctx, banner_texture,
    .width = 80, .height = 80,
    .scale = WLX_IMAGE_SCALE_FILL,
    .align = WLX_LEFT
);
```

Draw a spritesheet cell at natural size, pinned to the top-left:

```c
wlx_image(ctx, spritesheet,
    .scale = WLX_IMAGE_SCALE_NONE,
    .align = WLX_TOP_LEFT,
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

The `WLX_Align` enum values used by `widget_align` and `align`:

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
