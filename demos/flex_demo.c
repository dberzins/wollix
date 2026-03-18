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

#define WINDOW_WIDTH 1100
#define WINDOW_HEIGHT 700

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Flex / Weight-Based Sizing Demo");
    SetTargetFPS(60);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();

        wlx_begin(ctx, (WLX_Rect){0, 0, w, h}, wlx_process_raylib_input);
        BeginDrawing();
        ClearBackground(WLX_BACKGROUND_COLOR);

        // Root: title + examples
        wlx_layout_begin(ctx, 2, WLX_VERT,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(50), WLX_SLOT_AUTO });

            wlx_textbox(ctx, "Flex / Weight-Based Sizing Demo",
                .height = 50, .font_size = 28,
                .back_color = (Color){35, 30, 45, 255},
                .align = WLX_CENTER, .boxed = true);

            // Examples stacked vertically
            wlx_layout_begin(ctx, 6, WLX_VERT, .padding = 4,
                .sizes = (WLX_Slot_Size[]){
                    WLX_SLOT_PX(25), WLX_SLOT_PX(60),
                    WLX_SLOT_PX(25), WLX_SLOT_PX(60),
                    WLX_SLOT_PX(25), WLX_SLOT_AUTO,
                });

                // --- Example 1: Sidebar + Content ---
                wlx_textbox(ctx, "1) PX(200) + FLEX(1) sidebar + FLEX(3) content",
                    .height = 25, .font_size = 14,
                    .back_color = (Color){30, 30, 40, 255},
                    .align = WLX_LEFT, .boxed = true);

                wlx_layout_begin(ctx, 3, WLX_HORZ,
                    .sizes = (WLX_Slot_Size[]){
                        WLX_SLOT_PX(200),
                        WLX_SLOT_FLEX(1),
                        WLX_SLOT_FLEX(3),
                    });
                    wlx_textbox(ctx, "Nav (200px)",
                        .back_color = (Color){50, 35, 35, 255},
                        .align = WLX_CENTER, .boxed = true);
                    wlx_textbox(ctx, "Sidebar FLEX(1)",
                        .back_color = (Color){35, 50, 35, 255},
                        .align = WLX_CENTER, .boxed = true);
                    wlx_textbox(ctx, "Content FLEX(3)",
                        .back_color = (Color){35, 35, 55, 255},
                        .align = WLX_CENTER, .boxed = true);
                wlx_layout_end(ctx);

                // --- Example 2: Mixed AUTO and FLEX ---
                wlx_textbox(ctx, "2) AUTO + FLEX(2) + PX(100)  — AUTO acts as FLEX(1)",
                    .height = 25, .font_size = 14,
                    .back_color = (Color){30, 30, 40, 255},
                    .align = WLX_LEFT, .boxed = true);

                wlx_layout_begin(ctx, 3, WLX_HORZ,
                    .sizes = (WLX_Slot_Size[]){
                        WLX_SLOT_AUTO,
                        WLX_SLOT_FLEX(2),
                        WLX_SLOT_PX(100),
                    });
                    wlx_textbox(ctx, "AUTO = 1/3 remaining",
                        .back_color = (Color){50, 45, 30, 255},
                        .align = WLX_CENTER, .boxed = true);
                    wlx_textbox(ctx, "FLEX(2) = 2/3 remaining",
                        .back_color = (Color){30, 45, 50, 255},
                        .align = WLX_CENTER, .boxed = true);
                    wlx_textbox(ctx, "PX(100)",
                        .back_color = (Color){45, 30, 45, 255},
                        .align = WLX_CENTER, .boxed = true);
                wlx_layout_end(ctx);

                // --- Example 3: Complex app layout ---
                wlx_textbox(ctx, "3) App layout: PX(50) header, FLEX(1) body, PX(30) footer — body has weighted columns",
                    .height = 25, .font_size = 14,
                    .back_color = (Color){30, 30, 40, 255},
                    .align = WLX_LEFT, .boxed = true);

                wlx_layout_begin(ctx, 3, WLX_VERT,
                    .sizes = (WLX_Slot_Size[]){
                        WLX_SLOT_PX(35),
                        WLX_SLOT_FLEX(1),
                        WLX_SLOT_PX(25),
                    });

                    // Header
                    wlx_textbox(ctx, "Header (PX 35)",
                        .back_color = (Color){45, 35, 50, 255},
                        .align = WLX_CENTER, .boxed = true);

                    // Body: 4-column weighted layout
                    wlx_layout_begin(ctx, 4, WLX_HORZ,
                        .sizes = (WLX_Slot_Size[]){
                            WLX_SLOT_PX(40),
                            WLX_SLOT_FLEX(3),
                            WLX_SLOT_FLEX(1),
                            WLX_SLOT_PX(80),
                        });

                        // Icon sidebar
                        wlx_layout_begin_auto(ctx, WLX_VERT, 40);
                            wlx_textbox(ctx, "[F]",
                                .back_color = (Color){55, 40, 40, 255},
                                .align = WLX_CENTER, .font_size = 14, .boxed = true);
                            wlx_textbox(ctx, "[S]",
                                .back_color = (Color){55, 40, 40, 255},
                                .align = WLX_CENTER, .font_size = 14, .boxed = true);
                            wlx_textbox(ctx, "[T]",
                                .back_color = (Color){55, 40, 40, 255},
                                .align = WLX_CENTER, .font_size = 14, .boxed = true);
                        wlx_layout_end(ctx);

                        // Main editor area
                        wlx_textbox(ctx, "Editor FLEX(3)",
                            .back_color = (Color){30, 38, 48, 255},
                            .align = WLX_CENTER, .boxed = true);

                        // Side panel
                        wlx_textbox(ctx, "Panel FLEX(1)",
                            .back_color = (Color){38, 48, 30, 255},
                            .align = WLX_CENTER, .boxed = true);

                        // Minimap
                        wlx_textbox(ctx, "Map",
                            .back_color = (Color){40, 40, 40, 255},
                            .align = WLX_CENTER, .font_size = 12, .boxed = true);

                    wlx_layout_end(ctx);

                    // Footer
                    wlx_textbox(ctx, "Footer (PX 25)",
                        .back_color = (Color){35, 45, 35, 255},
                        .align = WLX_CENTER, .font_size = 12, .boxed = true);

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
