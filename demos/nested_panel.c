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
// Nested scroll panel demonstration
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
    printf("Nested Scroll Panel Demo\n");
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

                // Root layout: title + content
                wlx_layout_begin(ctx, 10, WLX_VERT);

                    // ---- Title bar --------------------------------------
                    wlx_label(ctx, "Nested Scroll Panel Demo",
                        .height = 50, .font_size = 28,
                        .back_color = (Color){35, 30, 45, 255},
                        .align = WLX_CENTER, .span = 1);

                    wlx_label(ctx, "This demonstrates scroll panels nested inside other scroll panels",
                        .height = 30, .font_size = 16,
                        .back_color = (Color){30, 30, 40, 255},
                        .align = WLX_CENTER, .span = 1);

                    // ---- OUTER scroll panel -----------------------------
                    // This is the main scrollable area
                    wlx_scroll_panel_begin(ctx, -1,
                        .back_color = (Color){22, 24, 28, 255},
                        .scrollbar_color = (Color){60, 50, 70, 255},
                        .span = 8
                    );
                        // Content inside outer scroll panel
                        // Cell count: 20 single-cell items + 3 panels * 12 span = 56
                        wlx_layout_begin(ctx, 56, WLX_VERT);

                            wlx_label(ctx, "Outer Scroll Panel Content",
                                .height = 40, .font_size = 20,
                                .back_color = (Color){30, 35, 42, 255},
                                .align = WLX_LEFT);

                            // Show some items in the outer panel
                            for (int i = 0; i < app.outer_note_count; i++) {
                                Color bg = (i % 2 == 0) 
                                    ? (Color){26, 28, 32, 255} 
                                    : (Color){30, 32, 36, 255};
                                wlx_label(ctx, app.outer_notes[i],
                                    .height = 35, .font_size = 14,
                                    .back_color = bg, .align = WLX_LEFT);
                            }

                            wlx_label(ctx, "Nested Scroll Panels Below",
                                .height = 40, .font_size = 20,
                                .back_color = (Color){40, 30, 35, 255},
                                .align = WLX_LEFT);

                            // ---- 3 INNER scroll panels (nested) ---------
                            // Each inner panel is scrollable independently
                            for (int panel_idx = 0; panel_idx < 3; panel_idx++) {
                                // Use wlx_push_id for loop-generated stateful widgets
                                wlx_push_id(ctx, (size_t)panel_idx);

                                    char panel_title[64];
                                    snprintf(panel_title, sizeof(panel_title), 
                                        "Inner Panel %d (Nested - scroll me independently!)", 
                                        panel_idx + 1);
                                    
                                    wlx_label(ctx, panel_title,
                                        .height = 35, .font_size = 18,
                                        .back_color = (Color){35, 45, 35, 255},
                                        .align = WLX_LEFT);

                                    // INNER scroll panel (nested inside outer)
                                    // Fixed height so it's clearly a separate scrollable area
                                    wlx_scroll_panel_begin(ctx, 400,  // explicit content height
                                        .back_color = (Color){18, 22, 20, 255},
                                        .scrollbar_color = (Color){40, 60, 45, 255},
                                        .height = 200,  // viewport height
                                        .span = 12
                                    );
                                        wlx_layout_begin(ctx, 10, WLX_VERT);

                                            wlx_label(ctx, "This is scrollable content inside the nested panel",
                                                .height = 32, .font_size = 14,
                                                .back_color = (Color){20, 24, 22, 255},
                                                .align = WLX_LEFT);

                                            // Input field inside nested panel
                                            wlx_inputbox(ctx, "Text:", app.inner_text[panel_idx], 
                                                sizeof(app.inner_text[panel_idx]),
                                                .height = 35, .font_size = 14);

                                            // Slider inside nested panel
                                            wlx_slider(ctx, "Value ", &app.inner_sliders[panel_idx],
                                                .height = 35, .font_size = 14,
                                                .min_value = 0.0f, .max_value = 1.0f);

                                            // Checkbox inside nested panel
                                            wlx_checkbox(ctx, "Enable feature", &app.inner_toggles[panel_idx],
                                                .height = 35, .font_size = 14);

                                            // Some filler items to make scrolling necessary
                                            for (int i = 0; i < 5; i++) {
                                                char item[64];
                                                snprintf(item, sizeof(item), 
                                                    "  Nested item %d - Scroll to see all content", i + 1);
                                                wlx_label(ctx, item,
                                                    .height = 32, .font_size = 13,
                                                    .back_color = (Color){22, 26, 24, 255},
                                                    .align = WLX_LEFT);
                                            }

                                            if (wlx_button(ctx, "Button in nested panel",
                                                .height = 35, .font_size = 14,
                                                .back_color = (Color){40, 50, 45, 255},
                                                .align = WLX_CENTER)) {
                                                printf("Button clicked in nested panel %d\n", panel_idx + 1);
                                            }

                                        wlx_layout_end(ctx);
                                    wlx_scroll_panel_end(ctx);

                                wlx_pop_id(ctx);

                                // Spacing between nested panels
                                wlx_label(ctx, "",
                                    .height = 10, .font_size = 10,
                                    .back_color = (Color){22, 24, 28, 255});
                            }

                            wlx_label(ctx, "More Outer Panel Content",
                                .height = 40, .font_size = 20,
                                .back_color = (Color){30, 35, 42, 255},
                                .align = WLX_LEFT);

                            // More items after nested panels
                            for (int i = 0; i < 5; i++) {
                                char item[128];
                                snprintf(item, sizeof(item), 
                                    "Outer item %d - This content comes after the nested panels", i + 1);
                                Color bg = (i % 2 == 0) 
                                    ? (Color){26, 28, 32, 255} 
                                    : (Color){30, 32, 36, 255};
                                wlx_label(ctx, item,
                                    .height = 35, .font_size = 14,
                                    .back_color = bg, .align = WLX_LEFT);
                            }

                            wlx_label(ctx, "End of outer scroll panel",
                                .height = 40, .font_size = 16,
                                .back_color = (Color){35, 30, 35, 255},
                                .align = WLX_CENTER);

                        wlx_layout_end(ctx);
                    wlx_scroll_panel_end(ctx);

                wlx_layout_end(ctx);

                // FPS counter
                DrawFPS(8, 8);

            EndDrawing();
        wlx_end(ctx);
    }

    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
