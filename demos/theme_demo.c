// theme_demo.c — Minimal demo showing before/after theme switching
// Demonstrates switching between wlx_theme_dark and wlx_theme_light at runtime.

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

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define TARGET_FPS    60

typedef struct {
    bool dark_mode;
    bool checkbox_a;
    bool checkbox_b;
    float slider_val;
    char input_buf[128];
    int  input_len;
} App_State;

static App_State app = {
    .dark_mode   = true,
    .checkbox_a  = false,
    .checkbox_b  = true,
    .slider_val  = 0.5f,
    .input_buf   = "Hello, themes!",
    .input_len   = 14,
};

int main(void) {
    printf("Theme demo — press the toggle to switch dark/light\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Theme Demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    while (!WindowShouldClose()) {
        float w = GetRenderWidth();
        float h = GetRenderHeight();
        WLX_Rect root = {0, 0, w, h};

        // Switch theme each frame based on toggle state
        ctx->theme = app.dark_mode ? &wlx_theme_dark : &wlx_theme_light;

        wlx_begin(ctx, root, wlx_process_raylib_input);
        BeginDrawing();

            Color bg = (Color){
                ctx->theme->background.r,
                ctx->theme->background.g,
                ctx->theme->background.b,
                ctx->theme->background.a,
            };
            ClearBackground(bg);

            // Outer vertical layout: title row + content
            wlx_layout_begin(ctx, 2, WLX_VERT);

                // ── Title bar ────────────────────────────────────
                wlx_layout_begin(ctx, 2, WLX_HORZ);

                    wlx_textbox(ctx, app.dark_mode ? "Dark Theme" : "Light Theme",
                        .font_size = 28, .align = WLX_LEFT,
                        .widget_align = WLX_CENTER
                    );

                    if (wlx_button(ctx, app.dark_mode ? "Switch to Light" : "Switch to Dark",
                        .font_size = 18, .height = 40,
                        .widget_align = WLX_CENTER
                    )) {
                        app.dark_mode = !app.dark_mode;
                    }

                wlx_layout_end(ctx);

                // ── Widget showcase ──────────────────────────────
                wlx_layout_begin(ctx, 2, WLX_HORZ);

                    // Left column: buttons & text
                    wlx_layout_begin(ctx, 5, WLX_VERT);

                        wlx_textbox(ctx, "Buttons",
                            .font_size = 20, .align = WLX_CENTER
                        );

                        if (wlx_button(ctx, "Default Button",
                            .font_size = 16, .height = 36,
                            .widget_align = WLX_CENTER
                        )) {
                            printf("Default button clicked\n");
                        }

                        if (wlx_button(ctx, "Accent Override",
                            .font_size = 16, .height = 36,
                            .widget_align = WLX_CENTER,
                            .back_color = ctx->theme->accent
                        )) {
                            printf("Accent button clicked\n");
                        }

                        wlx_textbox(ctx, "All colors come from the active theme.\n"
                                     "Per-widget overrides still work.",
                            .font_size = 14, .align = WLX_LEFT
                        );

                        // spacer
                        wlx_textbox(ctx, "", .font_size = 1);

                    wlx_layout_end(ctx);

                    // Right column: checkboxes, wlx_slider, input
                    wlx_layout_begin_auto(ctx, WLX_VERT, 30);

                        wlx_textbox(ctx, "Controls",
                            .font_size = 20, .align = WLX_CENTER
                        );

                        wlx_checkbox(ctx, "Option A", &app.checkbox_a,
                            .font_size = 16, .height = 30
                        );

                        wlx_checkbox(ctx, "Option B", &app.checkbox_b,
                            .font_size = 16, .height = 30
                        );

                        wlx_slider(ctx, "Volume", &app.slider_val,
                            .min_value = 0.0f, .max_value = 1.0f,
                            .height = 30, .font_size = 14
                        );
                        wlx_layout_auto_slot_px(ctx, 50.0f);

                        wlx_inputbox(ctx, "Text:", app.input_buf,
                            sizeof(app.input_buf),
                            .font_size = 16, .height = 50,
                        );

                    wlx_layout_end(ctx);

                wlx_layout_end(ctx);

            wlx_layout_end(ctx);

        EndDrawing();
        wlx_end(ctx);
    }

    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
