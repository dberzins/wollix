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
    float volume;
    float brightness;
    float r, g, b;
    float speed;
} App_State;

static App_State app = {
    .volume = 0.5f,
    .brightness = 0.75f,
    .r = 0.2f,
    .g = 0.6f,
    .b = 0.8f,
    .speed = 50.0f,
};

int main(void) {
    printf("Wollix slider demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix slider demo");
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

                        wlx_label(ctx, "Slider Demo",
                            .widget_align = WLX_CENTER, .font_size = 28, .height = 50, .align = WLX_CENTER
                        );

                        wlx_slider(ctx, "Volume  ", &app.volume,
                            .widget_align = WLX_CENTER, .width = 500, .height = 40,
                            .min_value = 0.0f, .max_value = 1.0f
                        );

                        wlx_slider(ctx, "Bright  ", &app.brightness,
                            .widget_align = WLX_CENTER, .width = 500, .height = 40,
                            .min_value = 0.0f, .max_value = 1.0f
                        );

                        wlx_slider(ctx, "Red     ", &app.r,
                            .widget_align = WLX_CENTER, .width = 500, .height = 40,
                            .min_value = 0.0f, .max_value = 1.0f,
                            .thumb_color = RED
                        );

                        wlx_slider(ctx, "Green   ", &app.g,
                            .widget_align = WLX_CENTER, .width = 500, .height = 40,
                            .min_value = 0.0f, .max_value = 1.0f,
                            .thumb_color = GREEN
                        );

                        wlx_slider(ctx, "Blue    ", &app.b,
                            .widget_align = WLX_CENTER, .width = 500, .height = 40,
                            .min_value = 0.0f, .max_value = 1.0f,
                            .thumb_color = BLUE
                        );

                        wlx_slider(ctx, "Speed   ", &app.speed,
                            .widget_align = WLX_CENTER, .width = 500, .height = 40,
                            .min_value = 0.0f, .max_value = 100.0f,
                            .font_size = 18
                        );

                        // Color preview box
                        {
                            Color preview = {
                                (unsigned char)(app.r * 255),
                                (unsigned char)(app.g * 255),
                                (unsigned char)(app.b * 255),
                                (unsigned char)(app.brightness * 255)
                            };
                            wlx_widget(ctx, .widget_align = WLX_CENTER, .width = 500, .height = 60, .color = preview);
                        }

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
