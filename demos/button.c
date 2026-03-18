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
    bool button1_clicked;
    bool button2_clicked;
} App_State;

static App_State app = {0};

int main(void) {
    printf("Wollix text demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix text demo");
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
                    wlx_layout_begin(ctx, 5, WLX_VERT);

                        if (wlx_button(ctx, "BUTTON1",
                            .widget_align = WLX_CENTER, .font_size = 20, .height = 50, .back_color = RED, .spacing = 5, .align = WLX_CENTER
                            // .widget_align = CENTER, .width = 300, .height = 40, .align = CENTER, .font_size = 20, .boxed = true
                        )) {
                            printf("Button1 ID: %zu, Left mouse clicked at: (%d, %d)\n", ctx->interaction.hot_id, ctx->input.mouse_x, ctx->input.mouse_y);
                            app.button1_clicked = true;
                        }

                        if (wlx_button(ctx, "BUTTON2",
                            .widget_align = WLX_CENTER, .font_size = 20, .height = 50, .back_color = RED, .spacing = 5, .align = WLX_CENTER
                            // .widget_align = CENTER, .width = 300, .height = 40, .align = CENTER, .font_size = 20, .boxed = true
                        )) {
                            printf("Button2 ID: %zu, Left mouse clicked at: (%d, %d)\n", ctx->interaction.hot_id, ctx->input.mouse_x, ctx->input.mouse_y);
                            app.button2_clicked = true;
                        }

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
