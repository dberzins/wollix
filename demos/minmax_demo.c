#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define WLX_SLOT_MINMAX_REDISTRIBUTE
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_raylib.h"

#define WINDOW_WIDTH 1100
#define WINDOW_HEIGHT 700

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Min/Max Constraints Demo");
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

            wlx_label(ctx, "Min/Max Constraints Demo  —  resize the window to see clamping",
                .height = 50, .font_size = 24,
                .back_color = (Color){35, 30, 45, 255},
                .align = WLX_CENTER, .boxed = true);

            // Examples stacked vertically
            wlx_layout_begin(ctx, 12, WLX_VERT, .padding = 4,
                .sizes = (WLX_Slot_Size[]){
                    WLX_SLOT_PX(22),  // label 1
                    WLX_SLOT_PX(55),  // example 1
                    WLX_SLOT_PX(22),  // label 2
                    WLX_SLOT_PX(55),  // example 2
                    WLX_SLOT_PX(22),  // label 3
                    WLX_SLOT_PX(55),  // example 3
                    WLX_SLOT_PX(22),  // label 4
                    WLX_SLOT_PX(55),  // example 4
                    WLX_SLOT_PX(22),  // label 5
                    WLX_SLOT_PX(55),  // example 5
                    WLX_SLOT_PX(22),  // label 6
                    WLX_SLOT_AUTO,    // example 6
                });

                // =============================================================
                // Slot-level min/max
                // =============================================================

                // --- Example 1: Sidebar with min width ---
                wlx_label(ctx, "1) Slot min: Sidebar FLEX(1) min=150  +  Content FLEX(3)  —  sidebar stops shrinking at 150px",
                    .height = 22, .font_size = 13,
                    .back_color = (Color){30, 30, 40, 255},
                    .align = WLX_LEFT, .boxed = true);

                wlx_layout_begin(ctx, 2, WLX_HORZ,
                    .sizes = (WLX_Slot_Size[]){
                        WLX_SLOT_FLEX_MIN(1, 150),
                        WLX_SLOT_FLEX(3),
                    });
                    wlx_label(ctx, "Sidebar FLEX(1) min=150",
                        .back_color = (Color){50, 35, 35, 255},
                        .align = WLX_CENTER, .boxed = true);
                    wlx_label(ctx, "Content FLEX(3)",
                        .back_color = (Color){35, 35, 55, 255},
                        .align = WLX_CENTER, .boxed = true);
                wlx_layout_end(ctx);

                // --- Example 2: Columns with max width ---
                wlx_label(ctx, "2) Slot max: Three FLEX(1) columns, each max=250  —  columns cap at 250px as window grows",
                    .height = 22, .font_size = 13,
                    .back_color = (Color){30, 30, 40, 255},
                    .align = WLX_LEFT, .boxed = true);

                wlx_layout_begin(ctx, 3, WLX_HORZ,
                    .sizes = (WLX_Slot_Size[]){
                        WLX_SLOT_FLEX_MAX(1, 250),
                        WLX_SLOT_FLEX_MAX(1, 250),
                        WLX_SLOT_FLEX_MAX(1, 250),
                    });
                    wlx_label(ctx, "FLEX(1) max=250",
                        .back_color = (Color){35, 50, 35, 255},
                        .align = WLX_CENTER, .boxed = true);
                    wlx_label(ctx, "FLEX(1) max=250",
                        .back_color = (Color){50, 45, 30, 255},
                        .align = WLX_CENTER, .boxed = true);
                    wlx_label(ctx, "FLEX(1) max=250",
                        .back_color = (Color){30, 45, 50, 255},
                        .align = WLX_CENTER, .boxed = true);
                wlx_layout_end(ctx);

                // --- Example 3: App layout with min+max sidebar ---
                wlx_label(ctx, "3) Slot min+max: Nav PX(50) + Sidebar FLEX(1) 120..300 + Content FLEX(3)",
                    .height = 22, .font_size = 13,
                    .back_color = (Color){30, 30, 40, 255},
                    .align = WLX_LEFT, .boxed = true);

                wlx_layout_begin(ctx, 3, WLX_HORZ,
                    .sizes = (WLX_Slot_Size[]){
                        WLX_SLOT_PX(50),
                        WLX_SLOT_FLEX_MINMAX(1, 120, 300),
                        WLX_SLOT_FLEX(3),
                    });
                    wlx_label(ctx, "Nav 50px",
                        .back_color = (Color){55, 40, 40, 255},
                        .align = WLX_CENTER, .font_size = 12, .boxed = true);
                    wlx_label(ctx, "Sidebar FLEX(1) 120..300",
                        .back_color = (Color){40, 55, 40, 255},
                        .align = WLX_CENTER, .boxed = true);
                    wlx_label(ctx, "Content FLEX(3)",
                        .back_color = (Color){35, 35, 55, 255},
                        .align = WLX_CENTER, .boxed = true);
                wlx_layout_end(ctx);

                // =============================================================
                // Widget-level min/max
                // =============================================================

                // --- Example 4: Button with max_width ---
                wlx_label(ctx, "4) Widget max: Button max_width=200 inside wide FLEX slot  —  button caps, slot unchanged",
                    .height = 22, .font_size = 13,
                    .back_color = (Color){30, 30, 40, 255},
                    .align = WLX_LEFT, .boxed = true);

                wlx_layout_begin(ctx, 2, WLX_HORZ,
                    .sizes = (WLX_Slot_Size[]){
                        WLX_SLOT_FLEX(1),
                        WLX_SLOT_FLEX(1),
                    });
                    // Button capped at 200px wide, centered in its slot
                    wlx_button(ctx, "max_width=200",
                        .max_width = 200, .height = 40,
                        .widget_align = WLX_CENTER,
                        .back_color = (Color){60, 35, 35, 255},
                        .boxed = true);
                    // Normal wlx_button fills its slot for comparison
                    wlx_button(ctx, "No constraint (fills slot)",
                        .height = 40,
                        .widget_align = WLX_CENTER,
                        .back_color = (Color){35, 35, 60, 255},
                        .boxed = true);
                wlx_layout_end(ctx);

                // --- Example 5: Widget min_width on inputs ---
                wlx_label(ctx, "5) Widget min: Inputs with min_width=120 in shrinking FLEX slots  —  inputs won't go below 120px",
                    .height = 22, .font_size = 13,
                    .back_color = (Color){30, 30, 40, 255},
                    .align = WLX_LEFT, .boxed = true);

                wlx_layout_begin(ctx, 4, WLX_HORZ,
                    .sizes = (WLX_Slot_Size[]){
                        WLX_SLOT_PX(80),
                        WLX_SLOT_FLEX(1),
                        WLX_SLOT_PX(80),
                        WLX_SLOT_FLEX(1),
                    });
                    wlx_label(ctx, "Label A:",
                        .height = 30,
                        .back_color = (Color){40, 40, 40, 255},
                        .widget_align = WLX_CENTER,
                        .align = WLX_CENTER, .boxed = true);
                    wlx_label(ctx, "min_width=120",
                        .min_width = 120, .height = 30,
                        .widget_align = WLX_LEFT,
                        .back_color = (Color){35, 55, 35, 255},
                        .align = WLX_CENTER, .boxed = true);
                    wlx_label(ctx, "Label B:",
                        .height = 30,
                        .back_color = (Color){40, 40, 40, 255},
                        .widget_align = WLX_CENTER,
                        .align = WLX_CENTER, .boxed = true);
                    wlx_label(ctx, "min_width=120",
                        .min_width = 120, .height = 60,
                        .widget_align = WLX_LEFT,
                        .back_color = (Color){55, 35, 55, 255},
                        .align = WLX_CENTER, .boxed = true);
                wlx_layout_end(ctx);

                // =============================================================
                // Iterative redistribution (WLX_SLOT_MINMAX_REDISTRIBUTE)
                // =============================================================

                // --- Example 6: Redistribute surplus from capped slots ---
                wlx_label(ctx, "6) Redistribute: FLEX(1) max=150 + FLEX(1) + FLEX(1) — surplus from capped slot fills siblings",
                    .height = 22, .font_size = 13,
                    .back_color = (Color){30, 30, 40, 255},
                    .align = WLX_LEFT, .boxed = true);

                wlx_layout_begin(ctx, 3, WLX_HORZ,
                    .sizes = (WLX_Slot_Size[]){
                        WLX_SLOT_FLEX_MAX(1, 150),
                        WLX_SLOT_FLEX(1),
                        WLX_SLOT_FLEX(1),
                    });
                    wlx_label(ctx, "FLEX(1) max=150",
                        .back_color = (Color){50, 35, 55, 255},
                        .align = WLX_CENTER, .boxed = true);
                    wlx_label(ctx, "FLEX(1) gets surplus",
                        .back_color = (Color){35, 55, 45, 255},
                        .align = WLX_CENTER, .boxed = true);
                    wlx_label(ctx, "FLEX(1) gets surplus",
                        .back_color = (Color){55, 45, 35, 255},
                        .align = WLX_CENTER, .boxed = true);
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
