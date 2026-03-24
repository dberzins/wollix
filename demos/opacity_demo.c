// opacity_demo.c — Demo showing per-wlx_widget opacity support
// Renders the same widgets at different opacity levels side-by-side.

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
#define WINDOW_HEIGHT 700
#define TARGET_FPS    60

typedef struct {
    bool   checkbox_val;
    float  slider_val;
    char   input_buf[128];
    float  opacity_control;
} App_State;

static App_State app = {
    .checkbox_val    = true,
    .slider_val      = 0.5f,
    .input_buf       = "Hello, opacity!",
    .opacity_control = 0.5f,
};

int main(void) {
    printf("Opacity demo — widgets at varying transparency levels\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Opacity Demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();
        WLX_Rect root = {0, 0, w, h};

        wlx_begin(ctx, root, wlx_process_raylib_input);
        BeginDrawing();

            ClearBackground((Color){30, 60, 90, 255});  // blue-ish background to show transparency

            // Outer vertical layout: title + opacity wlx_slider + grid of demos
            wlx_layout_begin(ctx, 3, WLX_VERT,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(50), WLX_SLOT_PX(50), WLX_SLOT_FLEX(1) }
            );

                // ── Title ────────────────────────────────────────
                wlx_label(ctx, "Opacity / Alpha Demo",
                    .font_size = 24, .align = WLX_CENTER,
                    .front_color = WLX_WHITE
                );

                // ── Opacity control wlx_slider ──────────────────────
                wlx_layout_begin(ctx, 1, WLX_HORZ, .padding = 10);
                    wlx_slider(ctx, "Dynamic opacity:", &app.opacity_control,
                        .min_value = 0.0f, .max_value = 1.0f,
                        .height = 30, .font_size = 16,
                        .label_color = WLX_WHITE
                    );
                wlx_layout_end(ctx);

                // ── 5 columns at fixed opacity levels ───────────
                wlx_layout_begin(ctx, 5, WLX_HORZ, .padding = 5);

                    float opacities[] = { 0.2f, 0.4f, 0.6f, 0.8f, 1.0f };

                    for (int col = 0; col < 5; col++) {
                        float op = opacities[col];
                        wlx_push_id(ctx, (size_t)(col + 1));

                        wlx_layout_begin(ctx, 8, WLX_VERT, .padding = 5);

                            // Column header with opacity value
                            char label[32];
                            snprintf(label, sizeof(label), "%.0f%%", op * 100);
                            wlx_label(ctx, label,
                                .font_size = 18, .align = WLX_CENTER,
                                .front_color = WLX_WHITE,
                                .opacity = op
                            );

                            // Button
                            wlx_button(ctx, "Button",
                                .font_size = 14, .height = 35,
                                .widget_align = WLX_CENTER,
                                .opacity = op
                            );

                            // Label (boxed)
                            wlx_label(ctx, "Boxed text",
                                .font_size = 14, .height = 30,
                                .boxed = true,
                                .widget_align = WLX_CENTER,
                                .opacity = op
                            );

                            // Checkbox
                            bool cb = app.checkbox_val;
                            wlx_checkbox(ctx, "Check", &cb,
                                .font_size = 14, .height = 25,
                                .opacity = op
                            );

                            // Slider
                            float sv = app.slider_val;
                            wlx_slider(ctx, "", &sv,
                                .min_value = 0.0f, .max_value = 1.0f,
                                .height = 30, .font_size = 12,
                                .show_label = false,
                                .opacity = op
                            );

                            // Inputbox
                            char buf[64];
                            strncpy(buf, app.input_buf, sizeof(buf) - 1);
                            buf[sizeof(buf) - 1] = '\0';
                            wlx_inputbox(ctx, "", buf, sizeof(buf),
                                .font_size = 12, .height = 35,
                                .opacity = op
                            );

                            // Plain text
                            wlx_label(ctx, "Plain text",
                                .font_size = 12, .align = WLX_CENTER,
                                .front_color = WLX_WHITE,
                                .opacity = op
                            );

                            // Spacer
                            wlx_label(ctx, "", .font_size = 1);

                        wlx_layout_end(ctx);

                        wlx_pop_id(ctx);
                    }

                wlx_layout_end(ctx);

            wlx_layout_end(ctx);

            // ── Dynamic opacity row at the bottom ────────────────
            // Draw a row of widgets using the wlx_slider-controlled opacity value
            DrawRectangle(0, (int)h - 100, (int)w, 100, (Color){20, 20, 20, 200});

            {
                WLX_Rect bottom = {10, h - 90, w - 20, 80};
                WLX_Layout bl = wlx_create_layout(ctx, bottom, 4, WLX_HORZ);
                wlx_da_append(&ctx->layouts, bl);

                    wlx_label(ctx, "Dynamic:",
                        .font_size = 16, .align = WLX_CENTER,
                        .front_color = WLX_WHITE,
                        .opacity = app.opacity_control
                    );

                    wlx_button(ctx, "Click me",
                        .font_size = 14, .height = 35,
                        .widget_align = WLX_CENTER,
                        .opacity = app.opacity_control
                    );

                    bool cb2 = true;
                    wlx_checkbox(ctx, "Toggle", &cb2,
                        .font_size = 14, .height = 30,
                        .opacity = app.opacity_control
                    );

                    float sv2 = 0.7f;
                    wlx_slider(ctx, "", &sv2,
                        .height = 30, .font_size = 12,
                        .show_label = false,
                        .opacity = app.opacity_control
                    );

                wlx_layout_end(ctx);
            }

        EndDrawing();
        wlx_end(ctx);
    }

    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
