// opacity_demo.c - Demo showing the three-layer opacity model
// Per-widget .opacity field, theme.opacity global multiplier,
// and wlx_push_opacity / wlx_pop_opacity context stack.
// All three multiply together: final = widget * theme * ctx_stack

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
    bool   checkbox_val;
    float  slider_val;
    char   input_buf[128];
    float  widget_opacity;    // per-widget slider
    float  theme_opacity;     // theme-level slider
    float  stack_opacity;     // context stack slider
} App_State;

static App_State app = {
    .checkbox_val    = true,
    .slider_val      = 0.5f,
    .input_buf       = "Hello, opacity!",
    .widget_opacity  = 0.5f,
    .theme_opacity   = 1.0f,
    .stack_opacity   = 1.0f,
};

// Draw a column of sample widgets at the given per-widget opacity.
// ctx_stack and theme opacity are applied externally.
static void draw_widget_column(WLX_Context *ctx, float op) {
    wlx_layout_begin(ctx, 7, WLX_VERT, .padding = 4);

        char label[32];
        snprintf(label, sizeof(label), "%.0f%%", op * 100);
        wlx_label(ctx, label,
            .font_size = 16, .align = WLX_CENTER,
            .front_color = WLX_WHITE, .opacity = op);

        wlx_button(ctx, "Button",
            .font_size = 13, .height = 30,
            .widget_align = WLX_CENTER, .opacity = op);

        wlx_label(ctx, "Boxed text",
            .font_size = 13, .height = 26,
            .show_background = true,
            .widget_align = WLX_CENTER, .opacity = op);

        bool cb = app.checkbox_val;
        wlx_checkbox(ctx, "Check", &cb,
            .font_size = 13, .height = 22, .opacity = op);

        float sv = app.slider_val;
        wlx_slider(ctx, "", &sv,
            .min_value = 0.0f, .max_value = 1.0f,
            .height = 26, .font_size = 11,
            .show_label = false, .opacity = op);

        char buf[64];
        strncpy(buf, app.input_buf, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        wlx_inputbox(ctx, "", buf, sizeof(buf),
            .font_size = 11, .height = 30, .opacity = op);

        wlx_label(ctx, "Plain text",
            .font_size = 11, .align = WLX_CENTER,
            .front_color = WLX_WHITE, .opacity = op);

    wlx_layout_end(ctx);
}

int main(void) {
    printf("Opacity demo - three-layer opacity model\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Opacity Demo - Three-Layer Model");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    // Mutable theme so we can adjust theme.opacity at runtime
    WLX_Theme theme = wlx_theme_dark;
    ctx->theme = &theme;

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();
        WLX_Rect root = {0, 0, w, h};

        // Apply theme opacity from slider
        theme.opacity = app.theme_opacity;

        wlx_begin(ctx, root, wlx_process_raylib_input);
        BeginDrawing();

            ClearBackground((Color){30, 60, 90, 255});

            // Main vertical layout: title + controls + 3 demo sections
            wlx_layout_begin(ctx, 5, WLX_VERT,
                .sizes = (WLX_Slot_Size[]){
                    WLX_SLOT_PX(40),   // title
                    WLX_SLOT_PX(50),   // control sliders
                    WLX_SLOT_PX(24),   // header
                    WLX_SLOT_FLEX(1),  // per-widget columns
                    WLX_SLOT_PX(200),  // stack demo
                }
            );

                // -- Title ------------------------------------------------
                wlx_label(ctx, "Three-Layer Opacity: widget * theme * context stack",
                    .font_size = 20, .align = WLX_CENTER,
                    .front_color = WLX_WHITE);

                // -- Control sliders --------------------------------------
                wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 8);

                    wlx_slider(ctx, "Widget .opacity:", &app.widget_opacity,
                        .min_value = 0.0f, .max_value = 1.0f,
                        .height = 28, .font_size = 13,
                        .label_color = WLX_WHITE);

                    wlx_slider(ctx, "theme.opacity:", &app.theme_opacity,
                        .min_value = 0.0f, .max_value = 1.0f,
                        .height = 28, .font_size = 13,
                        .label_color = WLX_WHITE);

                    wlx_slider(ctx, "push_opacity:", &app.stack_opacity,
                        .min_value = 0.0f, .max_value = 1.0f,
                        .height = 28, .font_size = 13,
                        .label_color = WLX_WHITE);

                wlx_layout_end(ctx);

                // -- Section header ---------------------------------------
                wlx_label(ctx, "Per-widget .opacity at fixed levels (theme & stack also active)",
                    .font_size = 13, .align = WLX_CENTER,
                    .front_color = (WLX_Color){180, 200, 255, 255});

                // -- 5 columns at fixed per-widget opacity ---------------
                wlx_layout_begin(ctx, 5, WLX_HORZ, .padding = 4);

                    float opacities[] = { 0.2f, 0.4f, 0.6f, 0.8f, 1.0f };

                    for (int col = 0; col < 5; col++) {
                        wlx_push_id(ctx, (size_t)(col + 1));
                        draw_widget_column(ctx, opacities[col]);
                        wlx_pop_id(ctx);
                    }

                wlx_layout_end(ctx);

                // -- Context stack demo ------------------------------------
                wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 6);

                    // Left: inside wlx_push_opacity (stack-controlled)
                    wlx_push_id(ctx, 100);
                    wlx_push_opacity(ctx, app.stack_opacity);

                        wlx_layout_begin(ctx, 2, WLX_VERT, .padding = 4);
                            wlx_label(ctx, "Inside push_opacity()",
                                .font_size = 13, .align = WLX_CENTER,
                                .front_color = (WLX_Color){150, 255, 150, 255});

                            wlx_layout_begin(ctx, 3, WLX_VERT, .padding = 4);
                                wlx_button(ctx, "Stack-faded button",
                                    .font_size = 13, .height = 30);
                                bool cb = true;
                                wlx_checkbox(ctx, "Stack-faded check", &cb,
                                    .font_size = 13, .height = 24);
                                wlx_label(ctx, "No per-widget .opacity set",
                                    .font_size = 11, .align = WLX_CENTER,
                                    .show_background = true, .height = 24,
                                    .front_color = WLX_WHITE);
                            wlx_layout_end(ctx);
                        wlx_layout_end(ctx);

                    wlx_pop_opacity(ctx);
                    wlx_pop_id(ctx);

                    // Center: nested push (stack * stack)
                    wlx_push_id(ctx, 200);
                    wlx_push_opacity(ctx, app.stack_opacity);
                    wlx_push_opacity(ctx, 0.5f);

                        wlx_layout_begin(ctx, 2, WLX_VERT, .padding = 4);
                            wlx_label(ctx, "Nested: push(slider) * push(0.5)",
                                .font_size = 13, .align = WLX_CENTER,
                                .front_color = (WLX_Color){255, 200, 150, 255});

                            wlx_layout_begin(ctx, 3, WLX_VERT, .padding = 4);
                                wlx_button(ctx, "Double-faded",
                                    .font_size = 13, .height = 30);
                                float sv = 0.6f;
                                wlx_slider(ctx, "", &sv,
                                    .height = 26, .font_size = 11,
                                    .show_label = false);
                                wlx_label(ctx, "Both stacks compound",
                                    .font_size = 11, .align = WLX_CENTER,
                                    .show_background = true, .height = 24,
                                    .front_color = WLX_WHITE);
                            wlx_layout_end(ctx);
                        wlx_layout_end(ctx);

                    wlx_pop_opacity(ctx);
                    wlx_pop_opacity(ctx);
                    wlx_pop_id(ctx);

                    // Right: stack + per-widget override
                    wlx_push_id(ctx, 300);
                    wlx_push_opacity(ctx, app.stack_opacity);

                        wlx_layout_begin(ctx, 2, WLX_VERT, .padding = 4);
                            wlx_label(ctx, "Stack + per-widget override",
                                .font_size = 13, .align = WLX_CENTER,
                                .front_color = (WLX_Color){255, 150, 200, 255});

                            wlx_layout_begin(ctx, 3, WLX_VERT, .padding = 4);
                                // This widget uses the slider-controlled per-widget opacity ON TOP of stack
                                wlx_button(ctx, "widget .opacity from slider",
                                    .font_size = 13, .height = 30,
                                    .opacity = app.widget_opacity);
                                // This widget forces full opacity, overriding stack fade
                                wlx_button(ctx, "Forced .opacity=1.0",
                                    .font_size = 13, .height = 30,
                                    .opacity = 1.0f);
                                wlx_label(ctx, "All 3 layers multiply",
                                    .font_size = 11, .align = WLX_CENTER,
                                    .show_background = true, .height = 24,
                                    .front_color = WLX_WHITE);
                            wlx_layout_end(ctx);
                        wlx_layout_end(ctx);

                    wlx_pop_opacity(ctx);
                    wlx_pop_id(ctx);

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
