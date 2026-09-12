// popup.c - overlay popups over a scrollable base: wlx_dropdown,
// wlx_tooltip_for, and wlx_menu (with one submenu level).
//
// Things to try:
//   - Open the dropdown, then wheel over the base list: the wheel belongs
//     to the pointer's layer, so the base stays still under the open list.
//   - Click a base row while a popup is open: the press closes the popup
//     AND still lands on the row (no click eating).
//   - Rest the pointer on the "Hover me" button for the tooltip; press to
//     dismiss it.
//   - Open the menu, then "More" for the submenu; Escape or an outside
//     press closes.

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_raylib.h"

#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 600
#define TARGET_FPS 60

#define SIDE_W 300.0f
#define ROW_H 44.0f
#define ROW_COUNT 40

static const char *size_options[] = { "Small", "Medium", "Large", "Huge" };

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Popup Demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    int  selected_size = 1;
    bool menu_open = false, submenu_open = false;
    char status[128] = "Ready.";

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();
        WLX_Rect r = { 0, 0, w, h };

        wlx_begin(ctx, r, wlx_process_raylib_input);
        BeginDrawing();
        ClearBackground(WLX_BACKGROUND_COLOR);

        wlx_layout_begin_s(ctx, WLX_HORZ,
            WLX_SIZES(WLX_SLOT_PX(SIDE_W), WLX_SLOT_FLEX(1)));

        // Base layer: a scrollable list the popups float over.
        wlx_scroll_panel_begin(ctx, WLX_SCROLL_AUTO_HEIGHT,
            .back_color = (Color){ 25, 25, 25, 255 });
        wlx_layout_begin(ctx, ROW_COUNT, WLX_VERT);
        for (int i = 0; i < ROW_COUNT; i++) {
            wlx_push_id(ctx, (size_t)i);
            char label[32];
            snprintf(label, sizeof(label), "Base row %d", i + 1);
            if (wlx_button(ctx, label, .height = 36,
                    .back_color = (Color){ 35, 35, 35, 255 })) {
                snprintf(status, sizeof(status), "Clicked %s", label);
            }
            wlx_pop_id(ctx);
        }
        wlx_layout_end(ctx);
        wlx_scroll_panel_end(ctx);

        // Right column of fixed-height control rows.
        wlx_layout_begin_s(ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_PX(ROW_H), WLX_SLOT_PX(ROW_H),
                      WLX_SLOT_PX(ROW_H), WLX_SLOT_PX(ROW_H),
                      WLX_SLOT_FLEX(1)),
            .gap = 8, .padding = 8);

        if (wlx_dropdown(ctx, "size", &selected_size,
                size_options, 4, .row_height = 32)) {
            snprintf(status, sizeof(status), "Size set to %s",
                size_options[selected_size]);
        }

        wlx_button(ctx, "Hover me");
        wlx_tooltip_for(ctx, wlx_last_rect(ctx),
            "Tooltips draw on the next layer and never take input");

        // Button-anchored menu: the face toggles like a dropdown (a second
        // click closes) and the list hangs below it.
        if (wlx_menu_button_begin(ctx, "Open menu", &menu_open, .menu_width = 200)) {
            if (wlx_menu_item(ctx, "Copy")) {
                snprintf(status, sizeof(status), "Menu: Copy");
            }
            if (wlx_menu_item(ctx, "Paste")) {
                snprintf(status, sizeof(status), "Menu: Paste");
            }
            if (wlx_menu_item(ctx, "More...", .keep_open = true)) {
                submenu_open = !submenu_open;
            }
            // Anchors itself beside the "More..." row, styled like the
            // parent; choosing a leaf closes the whole menu chain.
            if (wlx_submenu_begin(ctx, &submenu_open)) {
                if (wlx_menu_item(ctx, "Rename")) {
                    snprintf(status, sizeof(status), "Menu: Rename");
                }
                if (wlx_menu_item(ctx, "Delete")) {
                    snprintf(status, sizeof(status), "Menu: Delete");
                }
                wlx_menu_end(ctx);
            }
            wlx_menu_end(ctx);
        }

        wlx_label(ctx, status, .font_size = 18);

        wlx_layout_end(ctx);    // right column
        wlx_layout_end(ctx);    // root

        wlx_end(ctx);
        EndDrawing();
    }

    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
