# Demo Index

Each `.c` file is a self-contained program that demonstrates one or more
library features. Build all demos with `./build.sh` or compile one directly
(see the main README for flags).

---

## Widget demos

| Demo | Description |
|------|-------------|
| [button.c](button.c) | Basic button click handling with `widget_align`, `back_color`, and text alignment |
| [text.c](text.c) | `wlx_label` with every `WLX_Align` value, `show_background` mode, and Unicode text |
| [checkbox.c](checkbox.c) | Six boolean toggles showing checkbox state management |
| [checkbox_tex.c](checkbox_tex.c) | Texture-based checkboxes with programmatically generated checked/unchecked icons |
| [input.c](input.c) | Multi-field form (username, password, email, phone, address, comments) with focus tracking |
| [slider.c](slider.c) | Volume, brightness, RGB, and speed sliders with colored thumbs and a live color preview |
| [scroll_panel.c](scroll_panel.c) | Scrollable list of 30 items using auto content height (`-1`), custom scrollbar colors |

## Layout demos

| Demo | Description |
|------|-------------|
| [layout.c](layout.c) | Minimal layout nesting (VERT/HORZ) with `pos`, `span`, and `overflow` |
| [flex_demo.c](flex_demo.c) | Weight-based slot sizing — sidebar + content using `WLX_SLOT_PX`, `WLX_SLOT_AUTO`, and flex weights |
| [grid.c](grid.c) | Form-style 3×2 grid with label + input pairs using `wlx_grid_begin` |
| [grid_auto.c](grid_auto.c) | Auto-sizing grid with up to 200 dynamically filtered records |
| [variable_slots.c](variable_slots.c) | Mixed slot size types (`WLX_SLOT_PX`, `WLX_SLOT_AUTO`, `WLX_SLOT_PCT`) reacting to window resize |
| [minmax_demo.c](minmax_demo.c) | Min/max slot constraints with `WLX_SLOT_MINMAX_REDISTRIBUTE` — resize the window to see clamping |
| [widget_size.c](widget_size.c) | Widget sizing with `span` and `height = -1` (fill parent) |

## Advanced demos

| Demo | Description |
|------|-------------|
| [demo.c](demo.c) | Full showcase (~580 lines): buttons, checkboxes, sliders, inputs, scroll panels, tabs, `wlx_push_id`/`wlx_pop_id` |
| [nested_panel.c](nested_panel.c) | Nested scroll panels — outer list with three inner scrollable panels containing sliders and toggles |
| [nest2_panel.c](nest2_panel.c) | Nested scroll panel variant — outer notes + inner panels with sliders and toggles |
| [opacity_demo.c](opacity_demo.c) | Per-widget opacity — side-by-side comparison of widgets at different transparency levels |
| [font_demo.c](font_demo.c) | Loading TTF fonts (DejaVu Sans, Mono, Bold), per-widget and theme-level font assignment |
| [theme_demo.c](theme_demo.c) | Runtime dark/light theme switching with `wlx_theme_dark` / `wlx_theme_light` |
| [sdl3_demo.c](sdl3_demo.c) | SDL3 backend — same widgets (button, checkbox, slider) rendered via `wollix_sdl3.h` instead of Raylib |

---

## Suggested reading order

1. **[layout.c](layout.c)** — understand layout nesting and slots
2. **[button.c](button.c)** — first interactive widget
3. **[slider.c](slider.c)** and **[input.c](input.c)** — stateful widgets
4. **[scroll_panel.c](scroll_panel.c)** — scrollable containers
5. **[demo.c](demo.c)** — everything together
