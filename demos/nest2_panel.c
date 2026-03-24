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
#define TARGET_FPS 60

// ---------------------------------------------------------------------------
// Nested 2 scroll panel demonstration
// ---------------------------------------------------------------------------
typedef struct {
    // Outer panel data
    char outer_notes[10][128];
    int outer_note_count;
    
    // Inner panel data - multiple nested panels
    char inner_text[3][256];
    float inner_sliders[3];
    bool inner_toggles[3];
} App_State;

static App_State app = {
    .outer_note_count = 5,
    .inner_sliders = { 0.3f, 0.5f, 0.7f },
    .inner_toggles = { true, false, true },
};

int main(void) {
    printf("Nested 2 Scroll Panel Demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Nested Scroll Panel Demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    // Initialize outer notes
    for (int i = 0; i < app.outer_note_count; i++) {
        snprintf(app.outer_notes[i], sizeof(app.outer_notes[i]), 
            "Outer panel item %d - This shows scrollable content in the outer panel", i + 1);
    }

    // Initialize inner text
    for (int i = 0; i < 3; i++) {
        snprintf(app.inner_text[i], sizeof(app.inner_text[i]), 
            "Inner panel %d text", i + 1);
    }

    while (!WindowShouldClose()) {
        float w = GetRenderWidth();
        float h = GetRenderHeight();
        WLX_Rect r = {.x = 0, .y = 0, .w = w, .h = h};

        wlx_begin(ctx, r, wlx_process_raylib_input);
            BeginDrawing();
                ClearBackground(WLX_BACKGROUND_COLOR);

                wlx_layout_begin(ctx, 2, WLX_VERT);

                // wlx_label(ctx, "Outer Scroll Panel Content",
                //     .height = 40, .font_size = 20,
                //     .back_color = (Color){30, 35, 42, 255},
                //     .align = LEFT);
                
                // wlx_label(ctx, "Outer Scroll Panel Content",
                //     .height = 40, .font_size = 20,
                //     .back_color = (Color){30, 35, 42, 255},
                //     .align = LEFT);

                wlx_scroll_panel_begin(ctx, -1,
                    .back_color = (Color){22, 24, 28, 255},
                    .scrollbar_color = (Color){60, 50, 70, 255},
                );

                    wlx_layout_begin(ctx, 2, WLX_VERT);
                        wlx_button(ctx, "MAIN PANEL",
                            .widget_align = WLX_CENTER, .font_size = 20, .height = 200, .back_color = RED, .spacing = 5, .align = WLX_CENTER
                        );
                        wlx_scroll_panel_begin(ctx, -1,
                            .back_color = (Color){22, 24, 28, 255},
                            .scrollbar_color = (Color){60, 50, 70, 255},
                        );

                        wlx_button(ctx, "NESTED PANEL",
                            .widget_align = WLX_CENTER, .font_size = 20, .height = 200, .back_color = BLUE, .spacing = 5, .align = WLX_CENTER
                        );
                        wlx_scroll_panel_end(ctx);

                        // wlx_scroll_panel_begin(ctx, -1,
                        //     .back_color = (Color){22, 24, 28, 255},
                        //     .scrollbar_color = (Color){60, 50, 70, 255},
                        //     .span = 5
                        // );

                        //     wlx_layout_begin(ctx, 11, VERT);
                        //         for (int i = 0; i<10; i++) {
                        //             wlx_label(ctx, "yyyYYYYYYYYYYYYYYYYYY",
                        //                 .height = 40, .font_size = 20,
                        //                 .back_color = (Color){30, 35, 42, 255},
                        //                 .align = LEFT);
                        //         }
                        //     wlx_layout_end(ctx);
                        // wlx_scroll_panel_end(ctx);


                    wlx_layout_end(ctx);
                wlx_scroll_panel_end(ctx);

                wlx_button(ctx, "BOTTOM",
                    .widget_align = WLX_CENTER, .font_size = 20, .back_color = GREEN, .spacing = 5, .align = WLX_CENTER
                );

                wlx_layout_end(ctx);

            EndDrawing();
        wlx_end(ctx);
    }

    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
