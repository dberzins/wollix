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

All widgets embed two groups of shared fields. They appear first in every
option struct, so the table below applies to **every** widget unless noted
otherwise.

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
| `width` | `int16_t` | `-1` | Widget width in pixels. `-1` = fill parent width |
| `height` | `int16_t` | `-1` | Widget height in pixels. `-1` = fill parent height |
| `min_width` | `int16_t` | `0` | Minimum width constraint. `0` = unconstrained |
| `min_height` | `int16_t` | `0` | Minimum height constraint. `0` = unconstrained |
| `max_width` | `int16_t` | `0` | Maximum width constraint. `0` = unconstrained |
| `max_height` | `int16_t` | `0` | Maximum height constraint. `0` = unconstrained |
| `opacity` | `float` | `0` | Opacity multiplier. `0` = fully opaque (sentinel), `0.0001`–`1.0` = explicit alpha |

### Typography fields (`WLX_TEXT_TYPOGRAPHY_FIELDS`)

Used by `label`, `button`, `checkbox`, `checkbox_tex`, and `inputbox`.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `font` | `WLX_Font` | `WLX_FONT_DEFAULT` | Font handle. `0` = backend default / theme font |
| `font_size` | `int` | `0` | Font size in pixels. `0` = use theme default |
| `spacing` | `int` | `0` | Extra character spacing. `0` = use theme default |
| `align` | `WLX_Align` | `WLX_LEFT` | Text alignment within the widget rect |
| `wrap` | `bool` | varies | Enable word-wrapping for long text |

### Color fields (`WLX_TEXT_COLOR_FIELDS`)

Used by `label`, `button`, `checkbox`, `checkbox_tex`, and `inputbox`.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `front_color` | `WLX_Color` | `{0}` | Text/foreground color. `{0}` = use theme foreground |
| `back_color` | `WLX_Color` | `{0}` | Background fill color. `{0}` = use theme surface |

### Identity field

All widget opt structs include an optional identity field:

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `const char *` | `NULL` | An explicit string ID. When non-NULL, the string is hashed and pushed onto the ID stack for the duration of the widget call, giving it a stable identity independent of call-site order. Useful for widgets created in loops or dynamic lists. See [LAYOUT_MODEL § Explicit String IDs](LAYOUT_MODEL.md) for details. |

---

## `wlx_label`

Static text label. Does not return a value and has no interactive behavior
beyond hover highlighting (when `show_background` is true).

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

All shared fields (placement, sizing, typography, color) also apply.

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

### Widget-specific options

No widget-specific options beyond the shared fields.

All shared fields (placement, sizing, typography, color) apply.
The button always draws a filled `back_color` rectangle — hover brightens it
automatically using the theme's `hover_brightness`.

### Common overrides

Centered fixed-size button with custom color:

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

Full-width button in a vertical layout:

```c
if (wlx_button(ctx, "Save", .font_size = 18, .height = 40, .align = WLX_CENTER)) {
    save_data();
}
```

---

## `wlx_checkbox`

Toggle checkbox with a text label. Draws a small check-box indicator and the
label text beside it. Returns `true` on the frame the checked state changes.

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
| `show_background` | `bool` | `false` | Draw a filled background behind checkbox + label |
| `full_slot_hit` | `bool` | `true` | Use the full slot rect for hover/click interaction |
| `border_color` | `WLX_Color` | `{0}` | Border color of the check-box indicator. `{0}` = theme `checkbox.border` |
| `check_color` | `WLX_Color` | `{0}` | Color of the check mark. `{0}` = theme `checkbox.check` |

All shared fields (placement, sizing, typography, color) also apply.
Default `wrap` is `false` for checkbox.

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

---

## `wlx_checkbox_tex`

Texture-based checkbox — same behavior as `checkbox` but renders custom
textures for the checked/unchecked states instead of the default drawn
indicator. Useful for star ratings, custom toggle icons, etc.

### Signature

```c
bool wlx_checkbox_tex(WLX_Context *ctx, const char *text, bool *checked, ...options);
```

| Parameter | Description |
|-----------|-------------|
| `text` | Label displayed next to the texture |
| `checked` | Pointer to a `bool` — toggled automatically on click |

### Minimal example

```c
static bool favorited = false;

if (wlx_checkbox_tex(ctx, "Favorite", &favorited,
    .tex_checked   = star_filled_tex,
    .tex_unchecked = star_empty_tex
)) {
    printf("Favorited: %s\n", favorited ? "yes" : "no");
}
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `show_background` | `bool` | `false` | Draw a filled background behind texture + label |
| `full_slot_hit` | `bool` | `true` | Use the full slot rect for hover/click interaction |
| `tex_checked` | `WLX_Texture` | — | Texture to display when checked |
| `tex_unchecked` | `WLX_Texture` | — | Texture to display when unchecked |

All shared fields (placement, sizing, typography, color) also apply.

### Common overrides

Custom styled texture checkbox:

```c
if (wlx_checkbox_tex(ctx, "Feature A", &feature_a,
    .tex_checked   = tex_on,
    .tex_unchecked = tex_off,
    .font_size = 20,
    .align = WLX_LEFT,
    .back_color = (WLX_Color){30, 30, 60, 255},
    .front_color = (WLX_Color){255, 255, 255, 255}
)) {
    toggle_feature_a();
}
```

---

## `wlx_inputbox`

Single-line text input field. Click to focus, type to edit, press Enter or
Escape (or click elsewhere) to unfocus. Returns `true` while the field is
focused.

Uses persistent state internally (`WLX_Inputbox_State`) to track cursor
position and blink timer across frames.

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
| `border_focus_color` | `WLX_Color` | `{0}` | Border color when focused. `{0}` = theme `input.border_focus` |
| `cursor_color` | `WLX_Color` | `{0}` | Blinking cursor color. `{0}` = theme `input.cursor` |

All shared fields (placement, sizing, typography, color) also apply.
Default `wrap` is `true` for inputbox.

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
| `spacing` | `int` | `0` | Character spacing. `0` = use theme default |
| `align` | `WLX_Align` | `WLX_LEFT` | Text alignment for the label |
| `show_label` | `bool` | `true` | Show the numeric value to the right of the track |
| `track_color` | `WLX_Color` | `{0}` | Track bar background color. `{0}` = derive from theme `slider.track` |
| `thumb_color` | `WLX_Color` | `{0}` | Thumb handle color. `{0}` = theme `slider.thumb` |
| `label_color` | `WLX_Color` | `{0}` | Text label color. `{0}` = theme `slider.label` |
| `track_height` | `float` | `0` | Height of the track bar. `0` = theme default (6) |
| `thumb_width` | `float` | `0` | Width of the thumb handle. `0` = theme default (14) |
| `roundness` | `float` | `0` | Corner rounding for track/fill/thumb |
| `rounded_segments` | `int` | `0` | Segment count for rounded drawing |
| `hover_brightness` | `float` | `0` | Brightness shift on track hover |
| `thumb_hover_brightness` | `float` | `0` | Brightness shift on thumb hover |
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
| `scrollbar_hover_brightness` | `float` | `0` | Brightness shift when hovering the scrollbar |
| `scrollbar_width` | `float` | `0` | Width of the scrollbar. `0` = theme default (10) |
| `wheel_scroll_speed` | `float` | `20.0` | Pixels scrolled per mouse wheel tick |
| `show_scrollbar` | `bool` | `true` | Whether to draw the scrollbar |

Shared placement and sizing fields also apply (no typography or color text fields).

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
| `fill_size` | `WLX_Slot_Size` | `WLX_SLOT_FILL` | Outer wrapper slot size |
| `padding` | `float` | `4` | Gap between panes |
| `first_back_color` | `WLX_Color` | `{0}` | First pane background. `{0}` = theme default |
| `second_back_color` | `WLX_Color` | `{0}` | Second pane background. `{0}` = theme default |

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
| `padding` | `float` | `2` | Inner layout padding |
| `capacity` | `int` | `32` | Max child widgets (excl. title). Clamped to 64. |

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
- For standalone scrollable panels, wrap in
  `wlx_scroll_panel_begin` / `wlx_scroll_panel_end`.
- Unused CONTENT slots measure 0px — no visual impact from over-allocation.
- The default capacity of 32 covers most use cases. Set `.capacity = 48` or
  higher for sections with many widgets.
- Users needing mixed slot types (e.g. CONTENT + PX) should use the raw
  `wlx_layout_begin_s` API instead.

---

## `wlx_widget`

Low-level colored rectangle. Use it for dividers, color swatches, spacers,
or any situation where you need a simple filled rect without text.

### Signature

```c
void wlx_widget(WLX_Context *ctx, ...options);
```

### Widget-specific options

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `color` | `WLX_Color` | `{0}` | Fill color of the rectangle |

All shared fields (placement, sizing) also apply.

### Minimal example

```c
wlx_widget(ctx,
    .widget_align = WLX_CENTER, .width = 100, .height = 4,
    .color = (WLX_Color){ 80, 80, 80, 255 }
);
```

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
