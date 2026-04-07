#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_raylib.h"


#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define TARGET_FPS 60

// #define FONT_PATH_SANS  "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#define FONT_PATH_SANS  "demos/assets/LiberationSans-Regular.ttf"

// Build a codepoint array covering ASCII + Latin Extended (Latvian, etc.)
static int *get_font_codepoints(int *out_count) {
    // ASCII printable: 32-126 (95 chars)
    // Latin-1 Supplement: 0x00A0-0x00FF (96 chars) - accented Latin chars
    // Latin Extended-A:   0x0100-0x017F (128 chars) - ā, č, ē, ģ, ī, ķ, ļ, ņ, š, ū, ž
    int count = (126 - 32 + 1) + (0x00FF - 0x00A0 + 1) + (0x017F - 0x0100 + 1);
    int *cps = malloc(count * sizeof(int));
    int idx = 0;
    for (int c = 32;     c <= 126;    c++) cps[idx++] = c;
    for (int c = 0x00A0; c <= 0x00FF; c++) cps[idx++] = c;
    for (int c = 0x0100; c <= 0x017F; c++) cps[idx++] = c;
    *out_count = idx;
    return cps;
}

int main(void) {
    printf("Wollix text demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix text demo");
    SetTargetFPS(TARGET_FPS);

    int cp_count = 0;
    int *codepoints = get_font_codepoints(&cp_count);
    Font font_sans = LoadFontEx(FONT_PATH_SANS, 20, codepoints, cp_count);
    free(codepoints);
    bool sans_ok = font_sans.glyphCount > 0;
    if (!sans_ok) printf("WARNING: could not load %s\n", FONT_PATH_SANS);
    if (sans_ok) SetTextureFilter(font_sans.texture, TEXTURE_FILTER_BILINEAR);
    WLX_Font h_sans = sans_ok ? wlx_font_from_raylib(&font_sans) : WLX_FONT_DEFAULT;

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);
    
    WLX_Theme theme = wlx_theme_dark;

    while (!WindowShouldClose()) {
        float w = GetRenderWidth();
        float h = GetRenderHeight();
        WLX_Rect r = {.x = 0, .y = 0, .w = w, .h = h};
        theme.font = sans_ok ? h_sans : WLX_FONT_DEFAULT;
        ctx->theme = &theme;

        wlx_begin(ctx, r, wlx_process_raylib_input);
            BeginDrawing();
                ClearBackground(WLX_BACKGROUND_COLOR);

                wlx_layout_begin(ctx, 2, WLX_HORZ);
                    wlx_layout_begin(ctx, 9, WLX_VERT);

                        wlx_label(ctx, "CENTER - Vasaras diena Rīgā.",
                            .widget_align = WLX_TOP, .height = 50, .font_size = 20, .back_color = RED, .spacing = 5, .align = WLX_CENTER, .show_background = true
                        );

                        wlx_label(ctx, "LEFT - Zaļā pļavā",
                            .widget_align = WLX_TOP, .height = 50, .font_size = 20, .back_color = GREEN, .spacing = 5, .align = WLX_LEFT, .show_background = true
                        );
                        wlx_label(ctx, "RIGHT - Mēness gaismā",
                            .widget_align = WLX_TOP, .height = 50, .font_size = 20, .back_color = BLUE, .spacing = 5, .align = WLX_RIGHT, .show_background = true
                        );

                        wlx_label(ctx, "TOP - Jūras vējš",
                            .widget_align = WLX_TOP, .height = 50, .font_size = 20, .back_color = BLUE, .spacing = 5, .align = WLX_TOP, .show_background = true
                        );

                        wlx_label(ctx, "BOTTOM - Rudenī kļavu",
                            .widget_align = WLX_TOP, .height = 50, .font_size = 20, .back_color = BLUE, .spacing = 5, .align = WLX_BOTTOM, .show_background = true
                        );
                        wlx_label(ctx, "TOP_LEFT - Agrā rītā.",
                            .widget_align = WLX_TOP, .height = 50, .font_size = 20, .back_color = YELLOW, .spacing = 5, .align = WLX_TOP_LEFT, .show_background = true
                        );
                        wlx_label(ctx, "TOP_RIGHT - Pēkšņi debesīs",
                            .widget_align = WLX_TOP, .height = 50, .font_size = 20, .back_color = MAGENTA, .spacing = 5, .align = WLX_TOP_RIGHT, .show_background = true
                        );
                        wlx_label(ctx, "BOTTOM_LEFT - Siltā tējā",
                            .widget_align = WLX_TOP, .height = 50, .font_size = 20, .back_color = MAGENTA, .spacing = 5, .align = WLX_BOTTOM_LEFT, .show_background = true
                        );
                        wlx_label(ctx, "BOTTOM_RIGHT - Brīžiem čukst",
                            .widget_align = WLX_TOP, .height = 50, .font_size = 20, .back_color = MAGENTA, .spacing = 5, .align = WLX_BOTTOM_RIGHT, .show_background = true
                        );
                    wlx_layout_end(ctx);

                    wlx_layout_begin(ctx, 5, WLX_HORZ);
                        wlx_widget(ctx, .widget_align = WLX_CENTER, .width = 50, .height = -1, .color = WLX_SCROLLBAR_COLOR);
                        wlx_widget(ctx, .widget_align = WLX_CENTER, .width = 50, .height = -1, .color = RED);
                        wlx_widget(ctx, .widget_align = WLX_CENTER, .width = 50, .height = -1, .color = BLUE);
                        wlx_widget(ctx, .widget_align = WLX_CENTER, .width = 50, .height = -1, .color = YELLOW);
                        wlx_widget(ctx, .widget_align = WLX_CENTER, .width = 50, .height = -1, .color = MAGENTA);
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
