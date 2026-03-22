// font_demo.c — Demonstrates per-wlx_widget and theme-level font assignment
// using the WLX_Text_Style / WLX_Font system.
//
// Loads 2 TTF fonts via Raylib and shows how to:
//   1. Set a theme-level default font (all widgets inherit it).
//   2. Override the font per-wlx_widget via the .font option field.
//   3. Mix different fonts and sizes in the same UI.
//
// Edit FONT_PATH_* below to point at TTF files on your system.

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_raylib.h"

#define WINDOW_WIDTH  900
#define WINDOW_HEIGHT 650
#define TARGET_FPS    60

// ── Font file paths — change these to fonts available on your system ─────
#define FONT_PATH_SANS  "demos/assets/DejaVuSans.ttf"
#define FONT_PATH_MONO  "demos/assets/DejaVuSansMono.ttf"
#define FONT_PATH_BOLD  "demos/assets/DejaVuSans-Bold.ttf"

// ── Application state ────────────────────────────────────────────────────
typedef struct {
    bool   use_theme_font;   // toggle: apply sans font to theme
    bool   checkbox_a;
    bool   checkbox_b;
    float  slider_val;
    char   input_buf[128];
} App_State;

static App_State app = {
    .use_theme_font = true,
    .checkbox_a     = true,
    .checkbox_b     = false,
    .slider_val     = 0.5f,
    .input_buf      = "Type here...",
};

int main(void) {
    printf("Font demo — demonstrates per-widget and theme-level font usage\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Font Demo");
    SetTargetFPS(TARGET_FPS);

    // ── Load fonts ───────────────────────────────────────────────────────
    // LoadFontEx returns a heap-allocated Font; we keep them as locals
    // and convert their addresses to WLX_Font handles.
    Font font_sans = LoadFontEx(FONT_PATH_SANS, 32, NULL, 0);
    Font font_mono = LoadFontEx(FONT_PATH_MONO, 32, NULL, 0);
    Font font_bold = LoadFontEx(FONT_PATH_BOLD, 32, NULL, 0);

    bool sans_ok = font_sans.glyphCount > 0;
    bool mono_ok = font_mono.glyphCount > 0;
    bool bold_ok = font_bold.glyphCount > 0;

    if (!sans_ok) printf("WARNING: could not load %s\n", FONT_PATH_SANS);
    if (!mono_ok) printf("WARNING: could not load %s\n", FONT_PATH_MONO);
    if (!bold_ok) printf("WARNING: could not load %s\n", FONT_PATH_BOLD);

    // Raylib requires setting texture filter for smooth scaling
    if (sans_ok) SetTextureFilter(font_sans.texture, TEXTURE_FILTER_BILINEAR);
    if (mono_ok) SetTextureFilter(font_mono.texture, TEXTURE_FILTER_BILINEAR);
    if (bold_ok) SetTextureFilter(font_bold.texture, TEXTURE_FILTER_BILINEAR);

    // Convert to WLX_Font handles (pointer → uintptr_t)
    WLX_Font h_sans = sans_ok ? wlx_font_from_raylib(&font_sans) : WLX_FONT_DEFAULT;
    WLX_Font h_mono = mono_ok ? wlx_font_from_raylib(&font_mono) : WLX_FONT_DEFAULT;
    WLX_Font h_bold = bold_ok ? wlx_font_from_raylib(&font_bold) : WLX_FONT_DEFAULT;

    // ── UI context ───────────────────────────────────────────────────────
    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    // Make a mutable copy of the dark theme so we can swap the font field
    WLX_Theme theme = wlx_theme_dark;

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();
        WLX_Rect root = {0, 0, w, h};

        // ── Theme-level font override ────────────────────────────────────
        theme.font = app.use_theme_font ? h_sans : WLX_FONT_DEFAULT;
        ctx->theme = &theme;

        wlx_begin(ctx, root, wlx_process_raylib_input);
        BeginDrawing();

            Color bg = {theme.background.r, theme.background.g,
                        theme.background.b, theme.background.a};
            ClearBackground(bg);

            // ── Root: title + content + status ────────────────────
            wlx_layout_begin(ctx, 3, WLX_VERT,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(50), WLX_SLOT_AUTO, WLX_SLOT_PX(28) }
            );

                // ── Title row ───────────────────────────────────────────
                wlx_textbox(ctx, "Font Demo",
                    .font = h_bold, .font_size = 30,
                    .height = 50, .align = WLX_CENTER,
                    .back_color = theme.surface
                );

                // ── Content: 2 columns ──────────────────────────────────
                wlx_layout_begin(ctx, 2, WLX_HORZ);

                    // ── Left column: theme & controls ───────────────────
                    wlx_layout_begin(ctx, 8, WLX_VERT);

                        wlx_textbox(ctx, "Theme-level font",
                            .font = h_bold, .font_size = 20,
                            .height = 36, .align = WLX_LEFT
                        );

                        wlx_checkbox(ctx, "Use Sans as theme font",
                            &app.use_theme_font,
                            .font_size = 18, .height = 34
                        );

                        wlx_textbox(ctx,
                            app.use_theme_font
                                ? "Theme font: DejaVu Sans"
                                : "Theme font: Raylib default",
                            .font_size = 16, .height = 30,
                            .align = WLX_LEFT
                        );

                        wlx_textbox(ctx, "Per-widget font override",
                            .font = h_bold, .font_size = 20,
                            .height = 36, .align = WLX_LEFT
                        );

                        // These two checkboxes inherit the theme font
                        wlx_checkbox(ctx, "Checkbox A (theme font)",
                            &app.checkbox_a,
                            .font_size = 16, .height = 30
                        );

                        wlx_checkbox(ctx, "Checkbox B (theme font)",
                            &app.checkbox_b,
                            .font_size = 16, .height = 30
                        );

                        wlx_slider(ctx, "Volume", &app.slider_val,
                            .min_value = 0.0f, .max_value = 1.0f,
                            .font_size = 16, .height = 34
                        );

                        wlx_inputbox(ctx, "Input:", app.input_buf,
                            sizeof(app.input_buf),
                            .font = h_mono, .font_size = 18, .height = 45
                        );

                    wlx_layout_end(ctx);

                    // ── Right column: font showcase ─────────────────────
                    wlx_layout_begin(ctx, 7, WLX_VERT);

                        wlx_textbox(ctx, "Font showcase",
                            .font = h_bold, .font_size = 20,
                            .height = 36, .align = WLX_CENTER,
                            .back_color = theme.surface
                        );

                        wlx_textbox(ctx, "Sans — The quick brown fox",
                            .font = h_sans, .font_size = 20,
                            .height = 36, .align = WLX_LEFT
                        );

                        wlx_textbox(ctx, "Mono — The quick brown fox",
                            .font = h_mono, .font_size = 20,
                            .height = 36, .align = WLX_LEFT
                        );

                        wlx_textbox(ctx, "Bold — The quick brown fox",
                            .font = h_bold, .font_size = 20,
                            .height = 36, .align = WLX_LEFT
                        );

                        wlx_textbox(ctx, "Default — The quick brown fox",
                            .font = WLX_FONT_DEFAULT, .font_size = 20,
                            .height = 36, .align = WLX_LEFT
                        );

                        if (wlx_button(ctx, "Bold button",
                            .font = h_bold, .font_size = 18,
                            .height = 40, .widget_align = WLX_CENTER
                        )) {
                            printf("Bold button clicked\n");
                        }

                        if (wlx_button(ctx, "Mono button",
                            .font = h_mono, .font_size = 18,
                            .height = 40, .widget_align = WLX_CENTER
                        )) {
                            printf("Mono button clicked\n");
                        }

                    wlx_layout_end(ctx);

                wlx_layout_end(ctx);

                // ── Status bar ──────────────────────────────────────────
                char status[256];
                snprintf(status, sizeof(status),
                    "Fonts loaded:  Sans=%s  Mono=%s  Bold=%s",
                    sans_ok ? "yes" : "NO",
                    mono_ok ? "yes" : "NO",
                    bold_ok ? "yes" : "NO");

                wlx_textbox(ctx, status,
                    .font = h_mono, .font_size = 14,
                    .height = 28, .align = WLX_LEFT,
                    .back_color = theme.surface
                );

            wlx_layout_end(ctx);

        EndDrawing();
        wlx_end(ctx);
    }

    // ── Cleanup ──────────────────────────────────────────────────────────
    if (sans_ok) UnloadFont(font_sans);
    if (mono_ok) UnloadFont(font_mono);
    if (bold_ok) UnloadFont(font_bold);
    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
