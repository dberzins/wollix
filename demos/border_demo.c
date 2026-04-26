// border_demo.c - Demo showcasing the border system on various widgets
// Demonstrates border_color and border_width on labels, buttons, widgets,
// checkboxes, inputboxes, sliders, and scroll panels.

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

#define WINDOW_WIDTH  1000
#define WINDOW_HEIGHT 750
#define TARGET_FPS    60

typedef struct {
    bool   checkbox_vals[3];
    float  slider_val;
    char   input_buf[128];
    float  border_width;
    float  border_r, border_g, border_b;
} App_State;

static App_State app = {
    .checkbox_vals = { true, false, true },
    .slider_val    = 0.5f,
    .input_buf     = "Hello, borders!",
    .border_width  = 2.0f,
    .border_r      = 0.0f,
    .border_g      = 0.75f,
    .border_b      = 1.0f,
};

int main(void) {
    printf("Border demo - showcasing border_color and border_width\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Border Demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();
        WLX_Rect root = {0, 0, w, h};

        WLX_Color bdr = {
            (uint8_t)(app.border_r * 255),
            (uint8_t)(app.border_g * 255),
            (uint8_t)(app.border_b * 255),
            255
        };
        float bw = app.border_width;

        wlx_begin(ctx, root, wlx_process_raylib_input);
        BeginDrawing();
        ClearBackground((Color){24, 24, 24, 255});

        // Root: title row + controls + demo area
        wlx_layout_begin(ctx, 3, WLX_VERT,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(50), WLX_SLOT_PX(160), WLX_SLOT_FLEX(1) });

            // ── Title ──
            wlx_label(ctx, "Border System Demo",
                .font_size = 26, .align = WLX_CENTER,
                .show_background = true,
                .back_color = (WLX_Color){30, 30, 50, 255},
                .border_color = bdr,
                .border_width = bw);

            // ── Controls ──
            wlx_layout_begin(ctx, 4, WLX_VERT, .padding = 4);
                wlx_label(ctx, "Border Options",
                    .font_size = 20, .align = WLX_LEFT, .height = 32,
                    .show_background = true,
                    .back_color = (WLX_Color){40, 30, 30, 255});

                wlx_slider(ctx, "Border Width ", &app.border_width,
                    .height = 34, .font_size = 16,
                    .min_value = 0.0f, .max_value = 8.0f);

                wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 4);
                    wlx_slider(ctx, "R ", &app.border_r,
                        .height = 30, .font_size = 14, .thumb_color = RED);
                    wlx_slider(ctx, "G ", &app.border_g,
                        .height = 30, .font_size = 14, .thumb_color = GREEN);
                    wlx_slider(ctx, "B ", &app.border_b,
                        .height = 30, .font_size = 14, .thumb_color = BLUE);
                wlx_layout_end(ctx);

                // Color preview
                wlx_widget(ctx,
                    .height = 30, .color = bdr,
                    .border_color = WLX_WHITE, .border_width = 1);

            wlx_layout_end(ctx);

            // ── Demo widgets ──
            wlx_scroll_panel_begin(ctx, -1,
                .border_color = bdr, .border_width = bw);
                wlx_layout_begin(ctx, 12, WLX_VERT);

                    // --- Labels ---
                    wlx_label(ctx, "Labels", .font_size = 20, .height = 36,
                        .show_background = true,
                        .back_color = (WLX_Color){35, 35, 55, 255});

                    wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 6);
                        wlx_label(ctx, "Default border",
                            .font_size = 16, .height = 40,
                            .show_background = true,
                            .border_color = bdr, .border_width = bw);
                        wlx_label(ctx, "Thick red border",
                            .font_size = 16, .height = 40,
                            .show_background = true,
                            .border_color = (WLX_Color){255, 60, 60, 255},
                            .border_width = 3);
                        wlx_label(ctx, "No border (width = 0)",
                            .font_size = 16, .height = 40,
                            .show_background = true,
                            .border_width = 0);
                    wlx_layout_end(ctx);

                    // --- Buttons ---
                    wlx_label(ctx, "Buttons", .font_size = 20, .height = 36,
                        .show_background = true,
                        .back_color = (WLX_Color){35, 35, 55, 255});

                    wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 6);
                        wlx_button(ctx, "Bordered",
                            .font_size = 16, .height = 44,
                            .align = WLX_CENTER,
                            .border_color = bdr, .border_width = bw);
                        wlx_button(ctx, "Green outline",
                            .font_size = 16, .height = 44,
                            .align = WLX_CENTER,
                            .border_color = (WLX_Color){0, 200, 0, 255},
                            .border_width = 2);
                        wlx_button(ctx, "Thick yellow",
                            .font_size = 16, .height = 44,
                            .align = WLX_CENTER,
                            .border_color = (WLX_Color){255, 220, 0, 255},
                            .border_width = 4);
                    wlx_layout_end(ctx);

                    // --- Widgets (colored rects) ---
                    wlx_label(ctx, "Widgets (colored rects)", .font_size = 20, .height = 36,
                        .show_background = true,
                        .back_color = (WLX_Color){35, 35, 55, 255});

                    wlx_layout_begin(ctx, 4, WLX_HORZ, .padding = 6);
                        wlx_widget(ctx, .height = 60,
                            .color = (WLX_Color){60, 20, 20, 255},
                            .border_color = (WLX_Color){255, 100, 100, 255},
                            .border_width = bw);
                        wlx_widget(ctx, .height = 60,
                            .color = (WLX_Color){20, 60, 20, 255},
                            .border_color = (WLX_Color){100, 255, 100, 255},
                            .border_width = bw);
                        wlx_widget(ctx, .height = 60,
                            .color = (WLX_Color){20, 20, 60, 255},
                            .border_color = (WLX_Color){100, 100, 255, 255},
                            .border_width = bw);
                        wlx_widget(ctx, .height = 60,
                            .color = (WLX_Color){50, 50, 50, 255},
                            .border_color = bdr, .border_width = bw);
                    wlx_layout_end(ctx);

                    // --- Checkboxes ---
                    wlx_label(ctx, "Checkboxes", .font_size = 20, .height = 36,
                        .show_background = true,
                        .back_color = (WLX_Color){35, 35, 55, 255});

                    wlx_layout_begin(ctx, 3, WLX_VERT, .padding = 2);
                        wlx_checkbox(ctx, "Default border (theme)", &app.checkbox_vals[0],
                            .font_size = 16, .height = 34);
                        wlx_checkbox(ctx, "Custom thick border", &app.checkbox_vals[1],
                            .font_size = 16, .height = 34,
                            .border_color = bdr, .border_width = bw);
                        wlx_checkbox(ctx, "Red border, width 3", &app.checkbox_vals[2],
                            .font_size = 16, .height = 34,
                            .border_color = (WLX_Color){255, 80, 80, 255},
                            .border_width = 3);
                    wlx_layout_end(ctx);

                    // --- Input box ---
                    wlx_label(ctx, "Input Box", .font_size = 20, .height = 36,
                        .show_background = true,
                        .back_color = (WLX_Color){35, 35, 55, 255});

                    wlx_layout_begin(ctx, 2, WLX_VERT, .padding = 4);
                        wlx_inputbox(ctx, "Default:", app.input_buf, sizeof(app.input_buf),
                            .height = 44, .font_size = 16);
                        wlx_inputbox(ctx, "Thick:", app.input_buf, sizeof(app.input_buf),
                            .height = 44, .font_size = 16,
                            .border_width = bw,
                            .border_color = bdr);
                    wlx_layout_end(ctx);

                    // --- Slider ---
                    wlx_label(ctx, "Sliders", .font_size = 20, .height = 36,
                        .show_background = true,
                        .back_color = (WLX_Color){35, 35, 55, 255});

                    wlx_layout_begin(ctx, 2, WLX_VERT, .padding = 4);
                        wlx_slider(ctx, "No border   ", &app.slider_val,
                            .height = 36, .font_size = 16);
                        wlx_slider(ctx, "With border ", &app.slider_val,
                            .height = 36, .font_size = 16,
                            .border_color = bdr, .border_width = bw);
                    wlx_layout_end(ctx);

                wlx_layout_end(ctx);
            wlx_scroll_panel_end(ctx);

        wlx_layout_end(ctx);

        wlx_end(ctx);
        EndDrawing();
    }

    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
