# Wollix

[![CI](https://github.com/dberzins/wollix/actions/workflows/ci.yml/badge.svg)](https://github.com/dberzins/wollix/actions/workflows/ci.yml)

**Woven layouts for C.**

Wollix is a lightweight, header-only C library for building immediate-mode UI
layouts. Define rows, columns, and grids — widgets interlock into place.

- Single header: [wollix.h](wollix.h)
- Zero dependencies in the core library
- Backend adapters for Raylib and SDL3 included
- Built-in widgets: buttons, checkboxes, sliders, text input, scroll panels
- Current version: `WOLLIX_VERSION` = `"0.1.0"`

## Quick Start

```c
#include <raylib.h>
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_raylib.h"

int main(void) {
    InitWindow(800, 600, "Hello wollix.h");
    SetTargetFPS(60);

    WLX_Context ctx = {0};
    wlx_context_init_raylib(&ctx);

    float slider_val = 0.5f;
    bool  checked    = false;

    while (!WindowShouldClose()) {
        WLX_Rect root = {0, 0, GetRenderWidth(), GetRenderHeight()};
        wlx_begin(&ctx, root, wlx_process_raylib_input);
        BeginDrawing();
        ClearBackground((Color){18, 18, 18, 255});

            wlx_layout_begin(&ctx, 4, WLX_VERT, .padding = 8);

                wlx_textbox(&ctx, "Hello, wollix.h!", .font_size = 24, .align = WLX_CENTER);

                if (wlx_button(&ctx, "Click me", .height = 40))
                    printf("clicked!\n");

                wlx_checkbox(&ctx, "Enable feature", &checked);

                wlx_slider(&ctx, "Volume", &slider_val, .min_value = 0, .max_value = 1);

            wlx_layout_end(&ctx);

        EndDrawing();
        wlx_end(&ctx);
    }
    CloseWindow();
}
```

Compile with:
```bash
clang -I. -I ~/opt/raylib/include -o hello hello.c \
      -L ~/opt/raylib/lib -lraylib -lm -lpthread -ldl -lrt -lX11
```

## Optional short aliases

The canonical API uses `wlx_` prefixes. If you prefer terser names, define
`WLX_SHORT_NAMES` before including [wollix.h](wollix.h):

```c
#define WLX_SHORT_NAMES
#include "wollix.h"

layout_begin(&ctx, 2, WLX_VERT);
button(&ctx, "OK");
layout_end(&ctx);
```

## Project Structure

The repository contains the core `wollix.h` library, demo programs, build files,
and design/analysis notes.

```
wollix/
├── LICENSE                    # Project license
├── wollix.h                   # Core header-only UI layout library
├── wollix_raylib.h            # Raylib backend adapter
├── wollix_sdl3.h              # SDL3 backend adapter
├── Makefile                   # Main build system
├── build.sh                   # Original build script
├── README.md                  # Project overview
├── docs/                      # Documentation
│   ├── API_REFERENCE.md       # Public API reference
│   ├── LAYOUT_MODEL.md        # Layout model and sizing behavior
│   └── WIDGETS.md             # Widget usage guide
├── demos/                     # Demo sources and built demo executables
│   ├── assets/                # Bundled fonts and their licenses
    ├── demo.c                 # General demo
    ├── button.c               # Button demo
    ├── checkbox.c             # Checkbox demo
    ├── checkbox_tex.c         # Textured checkbox demo
    ├── flex_demo.c            # Flex sizing demo
    ├── font_demo.c            # Font demo
    ├── gallery.c              # Widgets gallery demo
    ├── grid.c                 # Fixed grid demo
    ├── grid_auto.c            # Auto-growing grid demo
    ├── input.c                # Text input demo
    ├── layout.c               # Layout primitives demo
    ├── minmax_demo.c          # Min/max sizing demo
    ├── nest2_panel.c          # Nested scroll panel demo
    ├── nested_panel.c         # Nested panel demo
    ├── opacity_demo.c         # Opacity and layering demo
    ├── scroll_panel.c         # Scrollable panel demo
    ├── slider.c               # Slider demo
    ├── sdl3_demo.c            # SDL3 backend demo
    ├── text.c                 # Text rendering demo
    ├── theme_demo.c           # Theme styling demo
    ├── variable_slots.c       # Variable slot sizing demo
    └── widget_size.c          # Widget sizing demo
└── tests/                     # Unit tests and test helpers
```

## Dependencies

- **Raylib**: Graphics library installed in `~/opt/raylib/`
- **SDL3** (optional): Needed only for `sdl3_demo` target
- **SDL3_ttf** (optional): Needed for TTF font rendering in the SDL3 backend

> **Note:** The build files assume Raylib is installed in `~/opt/raylib/`,
> SDL3 in `~/opt/sdl3/`, and SDL3_ttf in `~/opt/sdl3_ttf/`.
> If your installations are in different locations, update the paths in
> `Makefile` accordingly.

## Text Rendering

Both backends support real TTF font rendering:

- **Raylib**: pass a `Font*` via `wlx_font_from_raylib()`
- **SDL3**: pass a `TTF_Font*` via `wlx_font_from_sdl3()` (requires SDL3_ttf)

When no font is set (`WLX_FONT_DEFAULT`), the SDL3 backend falls back to the
built-in 8×8 debug font. The Raylib backend uses its default font.

**Limitations:** Wollix provides real TTF font rendering, not a full typography
engine. Basic Latin and UTF-8 text renders correctly with proper sizing, but
advanced features like kerning, ligatures, complex script shaping, and
bidirectional text are not supported. Text layout is codepoint-by-codepoint,
not shaped-run based.

## Available Widgets

The library includes the following widgets and layout/container primitives:

- **Button** - Clickable button widget with hover effects
- **Textbox** - Static text display with wrapping and alignment
- **Checkbox** - Toggle checkbox with text label
- **Checkbox (Texture)** - Checkbox using custom textures
- **Input Box** - Text input field with cursor and selection
- **Slider** - Value slider with label and drag interaction
- **Scrollable Panel** - Vertical scrolling container for long content
- **Linear layouts** - Horizontal and vertical slot-based layouting
- **Grid layouts** - Fixed and auto-growing grid layout helpers

## Building

### Using Makefile

```bash
make                # Build all Raylib demos (default)
make test           # Build and run the unit test suite
make test-demos     # Build all Raylib demos and verify they compile
make all            # Build all
make sdl3_demo      # Build the SDL3 backend demo
make debug          # Build demos/layout with debug flags
make release        # Build demos/layout with release optimization
make clean          # Remove all built executables
make help           # Show all available targets
```

Build a single demo by name:

```bash
make button         # → demos/button
make grid           # → demos/grid
make slider         # → demos/slider
```

All executables are written to `./demos/`.

### Using the original build script

```bash
./build.sh
```

