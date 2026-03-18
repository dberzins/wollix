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


int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix test demo");
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

                wlx_layout_begin(ctx, 3, WLX_VERT);
                    wlx_layout_begin(ctx, 5, WLX_VERT);

                    wlx_button(ctx, "BUTTON1",
                            .widget_align = WLX_CENTER, .font_size = 20, .height = 50, .back_color = RED, .spacing = 5, .align = WLX_CENTER
                        );

                    wlx_textbox(ctx, "TEXTBOX1",
                            .widget_align = WLX_CENTER, .font_size = 20, .height = -1, .back_color = GREEN, .spacing = 5, .align = WLX_CENTER,
                            .span = 3, .boxed = true
                        );
                    // wlx_button(ctx, "BUTTON2",
                    //         .widget_align = CENTER, .font_size = 20, .height = 100, .back_color = GREEN, .spacing = 5, .align = CENTER, .span = 2
                    //     );
                    wlx_button(ctx, "BUTTON3",
                            .widget_align = WLX_CENTER, .font_size = 20, .height = 50, .back_color = RED, .spacing = 5, .align = WLX_CENTER
                        );
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
