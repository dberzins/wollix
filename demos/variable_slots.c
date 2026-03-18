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

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define TARGET_FPS 60

typedef struct {
    int sidebar_clicks;
    int content_clicks;
    int inspector_clicks;
} App_State;

static App_State app = {0};

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Variable-size rows/columns demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();
        WLX_Rect root = { .x = 0, .y = 0, .w = w, .h = h };

        char title[128];
        snprintf(title, sizeof(title), "Window %.0fx%.0f — vertical: 80px / auto / 18%%", w, h);

        char content_info[256];
        snprintf(content_info, sizeof(content_info),
            "Horizontal content sizes: 220px sidebar | auto main area | 25%% inspector. "
            "Resize the window to see percent + auto slots react.");

        char stats[160];
        snprintf(stats, sizeof(stats),
            "Clicks — sidebar: %d, main: %d, inspector: %d",
            app.sidebar_clicks, app.content_clicks, app.inspector_clicks);

        wlx_begin(ctx, root, wlx_process_raylib_input);
        BeginDrawing();
            ClearBackground(WLX_BACKGROUND_COLOR);

            wlx_layout_begin(ctx, 3, WLX_VERT,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(80), WLX_SLOT_AUTO, WLX_SLOT_PCT(18) }
            );
                wlx_layout_begin(ctx, 3, WLX_HORZ,
                    .padding = 10,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(260), WLX_SLOT_AUTO, WLX_SLOT_PX(180) }
                );
                    wlx_textbox(ctx, "Variable Slots Demo",
                        .boxed = true,
                        .back_color = WLX_RGBA(45, 55, 80, 255),
                        .font_size = 26,
                        .align = WLX_CENTER,
                        .widget_align = WLX_CENTER
                    );

                    wlx_textbox(ctx, title,
                        .boxed = true,
                        .back_color = WLX_RGBA(30, 30, 38, 255),
                        .font_size = 18,
                        .align = WLX_CENTER,
                        .widget_align = WLX_CENTER
                    );

                    if (wlx_button(ctx, "Reset Clicks",
                        .back_color = WLX_RGBA(80, 45, 45, 255),
                        .font_size = 18,
                        .align = WLX_CENTER,
                        .widget_align = WLX_CENTER,
                        .height = 44
                    )) {
                        app.sidebar_clicks = 0;
                        app.content_clicks = 0;
                        app.inspector_clicks = 0;
                    }
                wlx_layout_end(ctx);

                wlx_layout_begin(ctx, 3, WLX_HORZ,
                    .padding = 12,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(220), WLX_SLOT_AUTO, WLX_SLOT_PCT(25) }
                );
                    wlx_layout_begin(ctx, 5, WLX_VERT, .padding = 6);
                        wlx_textbox(ctx, "Sidebar — fixed 220px",
                            .boxed = true,
                            .back_color = WLX_RGBA(55, 40, 70, 255),
                            .font_size = 22,
                            .align = WLX_CENTER,
                            .widget_align = WLX_CENTER
                        );

                        if (wlx_button(ctx, "Sidebar action",
                            .back_color = WLX_RGBA(90, 50, 120, 255),
                            .font_size = 18,
                            .align = WLX_CENTER,
                            .widget_align = WLX_CENTER,
                            .height = 44
                        )) {
                            app.sidebar_clicks += 1;
                        }

                        wlx_textbox(ctx, "Useful for nav rails, tools, or a fixed-width property list.",
                            .boxed = true,
                            .back_color = WLX_RGBA(28, 22, 34, 255),
                            .font_size = 18,
                            .align = WLX_LEFT,
                            .wrap = true,
                            .padding = 6
                        );

                        wlx_textbox(ctx, "This slot stays 220 px wide even while the window resizes.",
                            .boxed = true,
                            .back_color = WLX_RGBA(28, 22, 34, 255),
                            .font_size = 18,
                            .align = WLX_LEFT,
                            .wrap = true,
                            .padding = 6
                        );

                        wlx_textbox(ctx, stats,
                            .boxed = true,
                            .back_color = WLX_RGBA(40, 28, 50, 255),
                            .font_size = 17,
                            .align = WLX_LEFT,
                            .wrap = true,
                            .padding = 6
                        );
                    wlx_layout_end(ctx);

                    wlx_layout_begin(ctx, 4, WLX_VERT, .padding = 6);
                        wlx_textbox(ctx, "Main content — AUTO remainder",
                            .boxed = true,
                            .back_color = WLX_RGBA(35, 60, 75, 255),
                            .font_size = 24,
                            .align = WLX_CENTER,
                            .widget_align = WLX_CENTER
                        );

                        wlx_textbox(ctx, content_info,
                            .boxed = true,
                            .back_color = WLX_RGBA(20, 28, 34, 255),
                            .font_size = 20,
                            .align = WLX_LEFT,
                            .wrap = true,
                            .padding = 8
                        );

                        if (wlx_button(ctx, "Main area button",
                            .back_color = WLX_RGBA(45, 100, 110, 255),
                            .font_size = 20,
                            .align = WLX_CENTER,
                            .widget_align = WLX_CENTER,
                            .height = 48
                        )) {
                            app.content_clicks += 1;
                        }

                        wlx_textbox(ctx, "Because this slot is AUTO, it absorbs whatever space remains after fixed and percent slots are resolved.",
                            .boxed = true,
                            .back_color = WLX_RGBA(20, 28, 34, 255),
                            .font_size = 18,
                            .align = WLX_LEFT,
                            .wrap = true,
                            .padding = 8
                        );
                    wlx_layout_end(ctx);

                    wlx_layout_begin(ctx, 5, WLX_VERT, .padding = 6);
                        wlx_textbox(ctx, "Inspector — 25%",
                            .boxed = true,
                            .back_color = WLX_RGBA(70, 55, 35, 255),
                            .font_size = 22,
                            .align = WLX_CENTER,
                            .widget_align = WLX_CENTER
                        );

                        if (wlx_button(ctx, "Inspector action",
                            .back_color = WLX_RGBA(120, 85, 45, 255),
                            .font_size = 18,
                            .align = WLX_CENTER,
                            .widget_align = WLX_CENTER,
                            .height = 44
                        )) {
                            app.inspector_clicks += 1;
                        }

                        wlx_textbox(ctx, "This column scales with the window because it uses a percentage size.",
                            .boxed = true,
                            .back_color = WLX_RGBA(35, 28, 18, 255),
                            .font_size = 18,
                            .align = WLX_LEFT,
                            .wrap = true,
                            .padding = 6
                        );

                        wlx_textbox(ctx, "Great for secondary panes that should track overall window size.",
                            .boxed = true,
                            .back_color = WLX_RGBA(35, 28, 18, 255),
                            .font_size = 18,
                            .align = WLX_LEFT,
                            .wrap = true,
                            .padding = 6
                        );

                        wlx_textbox(ctx, "Try resizing horizontally to see this slot grow and shrink proportionally.",
                            .boxed = true,
                            .back_color = WLX_RGBA(35, 28, 18, 255),
                            .font_size = 17,
                            .align = WLX_LEFT,
                            .wrap = true,
                            .padding = 6
                        );
                    wlx_layout_end(ctx);
                wlx_layout_end(ctx);

                wlx_layout_begin(ctx, 4, WLX_HORZ,
                    .padding = 10,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_AUTO, WLX_SLOT_PX(220), WLX_SLOT_PX(180), WLX_SLOT_PX(180) }
                );
                    wlx_textbox(ctx, "Footer uses 18% of window height. Left slot is AUTO; the three right slots are fixed pixel widths.",
                        .boxed = true,
                        .back_color = WLX_RGBA(24, 24, 28, 255),
                        .font_size = 18,
                        .align = WLX_LEFT,
                        .wrap = true,
                        .padding = 8
                    );

                    wlx_textbox(ctx, "220 px", 
                        .boxed = true,
                        .back_color = WLX_RGBA(48, 48, 72, 255),
                        .font_size = 22,
                        .align = WLX_CENTER,
                        .widget_align = WLX_CENTER
                    );

                    wlx_textbox(ctx, "180 px", 
                        .boxed = true,
                        .back_color = WLX_RGBA(60, 48, 48, 255),
                        .font_size = 22,
                        .align = WLX_CENTER,
                        .widget_align = WLX_CENTER
                    );

                    wlx_textbox(ctx, "180 px", 
                        .boxed = true,
                        .back_color = WLX_RGBA(48, 60, 48, 255),
                        .font_size = 22,
                        .align = WLX_CENTER,
                        .widget_align = WLX_CENTER
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
