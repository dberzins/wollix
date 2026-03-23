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

#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 600

#define MAX_RECORDS 200

typedef struct {
    char name[32];
    char value[32];
} Record;

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Auto-Sizing Grid Demo");
    SetTargetFPS(60);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    // Seed some initial records
    Record records[MAX_RECORDS] = {0};
    int record_count = 5;
    for (int i = 0; i < record_count; i++) {
        snprintf(records[i].name,  sizeof(records[i].name),  "Item %d", i + 1);
        snprintf(records[i].value, sizeof(records[i].value), "Value %d", (i + 1) * 10);
    }

    // Filter
    static char filter[64] = "";
    static bool show_filtered = false;

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();

        wlx_begin(ctx, (WLX_Rect){0, 0, w, h}, wlx_process_raylib_input);
        BeginDrawing();
        ClearBackground(WLX_BACKGROUND_COLOR);

        // Root: title + controls + content
        wlx_layout_begin(ctx, 3, WLX_VERT,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(50), WLX_SLOT_PX(45), WLX_SLOT_AUTO });

            wlx_textbox(ctx, "Auto-Sizing Grid Demo",
                .height = 50, .font_size = 28,
                .back_color = (Color){35, 30, 45, 255},
                .align = WLX_CENTER);

            // Controls row
            wlx_layout_begin(ctx, 4, WLX_HORZ,
                .padding = 4,
                .sizes = (WLX_Slot_Size[]){
                    WLX_SLOT_PX(120), WLX_SLOT_PX(120),
                    WLX_SLOT_PX(200), WLX_SLOT_PX(120)
                });

                if (wlx_button(ctx, "Add Row") && record_count < MAX_RECORDS) {
                    snprintf(records[record_count].name,
                             sizeof(records[record_count].name),
                             "Item %d", record_count + 1);
                    snprintf(records[record_count].value,
                             sizeof(records[record_count].value),
                             "Value %d", (record_count + 1) * 10);
                    record_count++;
                }

                if (wlx_button(ctx, "Add 10 Rows")) {
                    for (int i = 0; i < 10 && record_count < MAX_RECORDS; i++) {
                        snprintf(records[record_count].name,
                                 sizeof(records[record_count].name),
                                 "Item %d", record_count + 1);
                        snprintf(records[record_count].value,
                                 sizeof(records[record_count].value),
                                 "Value %d", (record_count + 1) * 10);
                        record_count++;
                    }
                }

                wlx_inputbox(ctx, "Filter...", filter, sizeof(filter));

                wlx_checkbox(ctx, "Filter", &show_filtered);

            wlx_layout_end(ctx);

            // Content: two columns — scrollable data table + tile grid
            wlx_layout_begin(ctx, 2, WLX_HORZ);

                // Left: data table inside scroll panel
                wlx_layout_begin(ctx, 2, WLX_VERT,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(25), WLX_SLOT_AUTO });

                    char header_buf[64];
                    snprintf(header_buf, sizeof(header_buf),
                             "Data Table (%d rows)", record_count);
                    wlx_textbox(ctx, header_buf,
                        .height = 25, .font_size = 14,
                        .back_color = (Color){30, 40, 30, 255},
                        .align = WLX_CENTER);

                    wlx_scroll_panel_begin(ctx, -1);
                        wlx_grid_begin_auto(ctx, 3, 32,
                            .col_sizes = (WLX_Slot_Size[]){
                                WLX_SLOT_PX(140),    // Name
                                WLX_SLOT_AUTO,       // Value
                                WLX_SLOT_PX(80),     // Action
                            }
                        );
                            for (int i = 0; i < record_count; i++) {
                                wlx_push_id(ctx, (size_t)i);
                                // Apply filter
                                if (show_filtered && filter[0] != '\0') {
                                    bool match = false;
                                    for (int j = 0; records[i].name[j] && filter[j]; j++) {
                                        if (records[i].name[j] == filter[j]) {
                                            match = true;
                                            break;
                                        }
                                    }
                                    if (!match) continue;
                                }

                                // Variable row heights: every 5th row is taller
                                if (i % 5 == 4) {
                                    wlx_grid_auto_row_px(ctx, 56);
                                }

                                Color row_bg = (i % 2 == 0)
                                    ? (Color){35, 35, 40, 255}
                                    : (Color){40, 40, 48, 255};
                                // Highlight tall rows
                                if (i % 5 == 4) {
                                    row_bg = (Color){50, 40, 30, 255};
                                }

                                wlx_textbox(ctx, records[i].name,
                                    .align = WLX_LEFT,
                                    .back_color = row_bg,
                                    .boxed = true);
                                wlx_textbox(ctx, records[i].value,
                                    .align = WLX_LEFT,
                                    .back_color = row_bg,
                                    .boxed = true);
                                if (wlx_button(ctx, "Del")) {
                                    // Remove by shifting
                                    for (int j = i; j < record_count - 1; j++) {
                                        records[j] = records[j + 1];
                                    }
                                    record_count--;
                                    i--;
                                }
                                wlx_pop_id(ctx);
                            }
                        wlx_grid_end(ctx);
                    wlx_scroll_panel_end(ctx);
                wlx_layout_end(ctx);

                // Right: tile grid (auto cols from width)
                wlx_layout_begin(ctx, 2, WLX_VERT,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(25), WLX_SLOT_AUTO });

                    wlx_textbox(ctx, "Tile Grid",
                        .height = 25, .font_size = 14,
                        .back_color = (Color){40, 30, 30, 255},
                        .align = WLX_CENTER);

                    wlx_scroll_panel_begin(ctx, -1);
                        float tile_size = 80.0f;
                        wlx_grid_begin_auto_tile(ctx, tile_size, tile_size);
                            for (int i = 0; i < record_count; i++) {
                                wlx_push_id(ctx, (size_t)i);
                                Color tile_bg = (Color){
                                    (unsigned char)(40 + (i * 17) % 60),
                                    (unsigned char)(30 + (i * 31) % 60),
                                    (unsigned char)(50 + (i * 23) % 60),
                                    255
                                };
                                char tile_label[16];
                                snprintf(tile_label, sizeof(tile_label), "%d", i + 1);
                                wlx_textbox(ctx, tile_label,
                                    .align = WLX_CENTER,
                                    .back_color = tile_bg,
                                    .font_size = 18,
                                    .boxed = true);
                                wlx_pop_id(ctx);
                            }
                        wlx_grid_end(ctx);
                    wlx_scroll_panel_end(ctx);
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
