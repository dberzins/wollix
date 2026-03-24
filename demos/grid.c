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

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 700

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Grid Layout Demo");
    SetTargetFPS(60);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    static char name[128] = "";
    static char email[128] = "";
    static char phone[128] = "";

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();

        wlx_begin(ctx, (WLX_Rect){0, 0, w, h}, wlx_process_raylib_input);
        BeginDrawing();
        ClearBackground(WLX_BACKGROUND_COLOR);

        // Root: two rows — top for title, bottom for content
        wlx_layout_begin(ctx, 2, WLX_VERT,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(50), WLX_SLOT_AUTO });

            wlx_label(ctx, "Grid Layout Demo",
                .height = 50, .font_size = 28,
                .back_color = (Color){35, 30, 45, 255},
                .align = WLX_CENTER);

            // Content: 2 columns
            wlx_layout_begin(ctx, 2, WLX_HORZ);

                // Left: simple 3×2 form grid
                wlx_layout_begin(ctx, 2, WLX_VERT,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(30), WLX_SLOT_AUTO });

                    wlx_label(ctx, "Form (3x2 grid, auto-advance)",
                        .height = 30, .font_size = 16,
                        .back_color = (Color){30, 40, 30, 255},
                        .align = WLX_CENTER);

                    wlx_grid_begin(ctx, 3, 2,
                        .col_sizes = (WLX_Slot_Size[]){
                            WLX_SLOT_PX(100), WLX_SLOT_AUTO
                        },
                        .row_sizes = (WLX_Slot_Size[]){
                            WLX_SLOT_PX(45), WLX_SLOT_PX(45), WLX_SLOT_PX(45)
                        }
                    );
                        wlx_label(ctx, "Name:",  .align = WLX_LEFT);
                        wlx_inputbox(ctx, "", name, sizeof(name));
                        wlx_label(ctx, "Email:", .align = WLX_LEFT);
                        wlx_inputbox(ctx, "", email, sizeof(email));
                        wlx_label(ctx, "Phone:", .align = WLX_LEFT);
                        wlx_inputbox(ctx, "", phone, sizeof(phone));
                    wlx_grid_end(ctx);
                wlx_layout_end(ctx);

                // Right: spanning demo (3×3)
                wlx_layout_begin(ctx, 2, WLX_VERT,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(30), WLX_SLOT_AUTO });

                    wlx_label(ctx, "Spanning (3x3 grid, grid_cell)",
                        .height = 30, .font_size = 16,
                        .back_color = (Color){40, 30, 30, 255},
                        .align = WLX_CENTER);

                    wlx_grid_begin(ctx, 3, 3);
                        // Big tile: 2 rows × 2 cols at (0,0)
                        wlx_grid_cell(ctx, 0, 0, .row_span = 2, .col_span = 2);
                        wlx_label(ctx, "2x2 Tile",
                            .back_color = (Color){40, 60, 90, 255},
                            .align = WLX_CENTER);

                        // Right column
                        wlx_grid_cell(ctx, 0, 2);
                        wlx_label(ctx, "R0 C2",
                            .back_color = (Color){60, 40, 40, 255},
                            .align = WLX_CENTER);

                        wlx_grid_cell(ctx, 1, 2);
                        wlx_label(ctx, "R1 C2",
                            .back_color = (Color){40, 60, 40, 255},
                            .align = WLX_CENTER);

                        // Bottom row (auto-advance from 2,0)
                        wlx_grid_cell(ctx, 2, 0);
                        wlx_label(ctx, "R2 C0",
                            .back_color = (Color){50, 50, 30, 255},
                            .align = WLX_CENTER);
                        wlx_label(ctx, "R2 C1",
                            .back_color = (Color){30, 50, 50, 255},
                            .align = WLX_CENTER);
                        wlx_label(ctx, "R2 C2",
                            .back_color = (Color){50, 30, 50, 255},
                            .align = WLX_CENTER);
                    wlx_grid_end(ctx);
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
