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

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define TARGET_FPS 60

typedef struct {
    bool option1;
    bool option2;
    bool option3;
    bool enable_feature;
    bool show_details;
    bool accept_terms;
} App_State;

static App_State app = {0};

int main() {

    printf("Wollix checkbox demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix Checkbox Demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    while (!WindowShouldClose()) {
        float w = GetRenderWidth();
        float h = GetRenderHeight();
        WLX_Rect r = {.x = 0, .y = 0, .w = w, .h = h};

        wlx_begin(ctx, r, wlx_process_raylib_input);
            BeginDrawing();
                ClearBackground(WLX_BACKGROUND_COLOR);
                wlx_layout_begin(ctx, 1, WLX_HORZ);
                    wlx_layout_begin(ctx, 8, WLX_VERT);

                        // Title
                        wlx_label(ctx, "Checkbox Demo",
                            .widget_align = WLX_TOP_LEFT, .height = 40, .font_size = 20, .back_color = WLX_BACKGROUND_COLOR, .align = WLX_TOP_LEFT);

                        // Checkbox options
                        if (wlx_checkbox(ctx, "Option 1: Enable feature A", &app.option1,
                             .font_size = 20, .wrap = true,
                             // .widget_align = RIGHT, .width = 400, .height = 60, .font_size = 20
                        )) {
                            printf("Option 1 toggled: %s\n", app.option1 ? "ON" : "OFF");
                        }

                        if (wlx_checkbox(ctx, "Option 2: Enable feature B", &app.option2,
                             .font_size = 20
                             // .widget_align = BOTTOM_RIGHT, .height = 40, .align = CENTER, .font_size = 20, .boxed = true
                        )) {
                            printf("Option 2 toggled: %s\n", app.option2 ? "ON" : "OFF");
                        }

                        if (wlx_checkbox(ctx, "Option 3: Enable feature C", &app.option3,
                             .font_size = 20
                             // .widget_align = TOP_RIGHT, .height = 40, .align = CENTER, .font_size = 20, .boxed = true
                        )) {
                            printf("Option 3 toggled: %s\n", app.option3 ? "ON" : "OFF");
                        }

                        if (wlx_checkbox(ctx, "Enable advanced mode", &app.enable_feature,
                             .font_size = 20
                             // .widget_align = TOP_RIGHT, .height = 40, .align = CENTER, .font_size = 20, .boxed = true
                        )) {
                            printf("Advanced mode toggled: %s\n", app.enable_feature ? "ON" : "OFF");
                        }

                        if (wlx_checkbox(ctx, "Show detailed information", &app.show_details,
                             .font_size = 20
                             // .widget_align = CENTER, .width = 300, .height = 40, .align = CENTER, .font_size = 20, .boxed = true
                        )) {
                            printf("Show details toggled: %s\n", app.show_details ? "ON" : "OFF");
                        }

                        if (wlx_checkbox(ctx, "I accept terms and conditions", &app.accept_terms,
                             .font_size = 20
                             // .widget_align = CENTER, .width = 300, .height = 40, .align = CENTER, .font_size = 20, .boxed = true
                        )) {
                            printf("Terms accepted toggled: %s\n", app.accept_terms ? "ON" : "OFF");
                        }

                        // Status display
                        char status_text[256];
                        snprintf(status_text, sizeof(status_text),
                            "Status: %d options enabled | Terms: %s",
                            app.option1 + app.option2 + app.option3 + app.enable_feature + app.show_details,
                            app.accept_terms ? "ACCEPTED" : "NOT ACCEPTED"
                        );
                        wlx_label(ctx, status_text,
                            .widget_align = WLX_BOTTOM_LEFT, .height = 40, .font_size = 20, .back_color = WLX_BACKGROUND_COLOR, .align = WLX_BOTTOM_LEFT);


                    wlx_layout_end(ctx);
                wlx_layout_end(ctx);

            EndDrawing();
        wlx_end(ctx);
    }
    CloseWindow();
    wlx_context_destroy(ctx);
    free(ctx);
    return 0;
}
