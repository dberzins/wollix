#include <stdio.h>
#include <raylib.h>

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_raylib.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define TARGET_FPS 60

int main(void) {
    printf("Wollix layout demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix Layout Demo");
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
                wlx_layout_begin(ctx, 4, WLX_HORZ);

                    wlx_button(ctx, "BUTTON1", .pos =1, .span = 1, .overflow = true, .width = 350, .height = 350, .back_color = BLUE, .align = WLX_LEFT);
                    wlx_button(ctx, "BUTTON2", .back_color = MAGENTA, .align = WLX_CENTER);
                    
                wlx_layout_end(ctx);
                wlx_button(ctx, "BUTTON1",.pos = 2, .back_color = GREEN, .align = WLX_CENTER);
            wlx_layout_end(ctx);
        EndDrawing();
        wlx_end(ctx);
    }
    CloseWindow();
    wlx_context_destroy(ctx);
    free(ctx);
    return 0;
}
