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
#define ITEM_COUNT 30

int main(void) {
    printf("Scroll panel demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Scroll Panel Demo");
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

                wlx_layout_begin(ctx, 2, WLX_HORZ);

                    // Left panel: scrollable list with auto content height (-1)
                    wlx_scroll_panel_begin(ctx, -1,
                        .back_color = (Color){25, 25, 25, 255},
                        .scrollbar_color = (Color){60, 60, 60, 255}
                    );
                        wlx_layout_begin(ctx, ITEM_COUNT, WLX_VERT);
                            for (int i = 0; i < ITEM_COUNT; i++) {
                                wlx_push_id(ctx, (size_t)i);
                                char label[64];
                                snprintf(label, sizeof(label), "Item %d", i + 1);

                                Color item_color = (i % 2 == 0)
                                    ? (Color){35, 35, 35, 255}
                                    : (Color){30, 30, 30, 255};

                                if (wlx_button(ctx, label,
                                    .back_color = item_color,
                                    .font_size = 18,
                                    .height = 40,
                                    .align = WLX_CENTER
                                )) {
                                    printf("Clicked: %s\n", label);
                                }
                                wlx_pop_id(ctx);
                            }
                        wlx_layout_end(ctx);
                    wlx_scroll_panel_end(ctx);

                    // Right panel: another scroll panel with auto height
                    wlx_scroll_panel_begin(ctx, -1,
                        .back_color = (Color){20, 20, 30, 255},
                        .scrollbar_color = (Color){80, 60, 60, 255}
                    );
                        wlx_layout_begin(ctx, 20, WLX_VERT);
                            for (int i = 0; i < 20; i++) {
                                wlx_push_id(ctx, (size_t)i);
                                char label[64];
                                snprintf(label, sizeof(label), "Text row %d - scrollable content", i + 1);
                                wlx_label(ctx, label,
                                    .font_size = 16,
                                    .height = 40,
                                    .align = WLX_LEFT
                                );
                                wlx_pop_id(ctx);
                            }
                        wlx_layout_end(ctx);
                    wlx_scroll_panel_end(ctx);

                wlx_layout_end(ctx);

            EndDrawing();
        wlx_end(ctx);
    }

    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
