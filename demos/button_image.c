// button_image.c - Showcase image-capable wlx_button: text-only, image-only,
// and image + text modes; the four image placements (LEFT/RIGHT/TOP/BOTTOM);
// and the four texture scale modes (STRETCH/FIT/FILL/NONE) on image-only
// buttons. A click counter at the bottom increments when any button is
// pressed.

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

#define WINDOW_WIDTH  1100
#define WINDOW_HEIGHT 720
#define TARGET_FPS    60

static const char *placement_label(WLX_Image_Placement p) {
    switch (p) {
        case WLX_IMAGE_PLACEMENT_LEFT:   return "LEFT";
        case WLX_IMAGE_PLACEMENT_RIGHT:  return "RIGHT";
        case WLX_IMAGE_PLACEMENT_TOP:    return "TOP";
        case WLX_IMAGE_PLACEMENT_BOTTOM: return "BOTTOM";
    }
    return "?";
}

static const char *scale_label(WLX_Image_Scale s) {
    switch (s) {
        case WLX_IMAGE_SCALE_STRETCH: return "STRETCH";
        case WLX_IMAGE_SCALE_FIT:     return "FIT";
        case WLX_IMAGE_SCALE_FILL:    return "FILL";
        case WLX_IMAGE_SCALE_NONE:    return "NONE";
    }
    return "?";
}

// Square 64x64 icon: white arrow on a saturated tile, suitable for image-only
// and image + text buttons.
static Texture2D create_icon_texture(void) {
    int s = 64;
    Image img = GenImageColor(s, s, (Color){45, 90, 160, 255});

    ImageDrawRectangle(&img, 0, 0, s, 4, (Color){80, 130, 210, 255});
    ImageDrawRectangle(&img, 0, s - 4, s, 4, (Color){80, 130, 210, 255});
    ImageDrawRectangle(&img, 0, 0, 4, s, (Color){80, 130, 210, 255});
    ImageDrawRectangle(&img, s - 4, 0, 4, s, (Color){80, 130, 210, 255});

    int cx = s / 2, cy = s / 2;
    int len = s / 3;
    for (int t = -2; t <= 2; t++) {
        ImageDrawLine(&img, cx - len, cy + t, cx + len, cy + t, (Color){240, 240, 245, 255});
        ImageDrawLine(&img, cx + len + t, cy, cx + len / 2 + t, cy - len / 2, (Color){240, 240, 245, 255});
        ImageDrawLine(&img, cx + len + t, cy, cx + len / 2 + t, cy + len / 2, (Color){240, 240, 245, 255});
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// Non-square 128x64 landscape texture so STRETCH / FIT / FILL / NONE are
// visibly different in a square-ish button slot.
static Texture2D create_landscape_texture(void) {
    int w = 128, h = 64;
    Image img = GenImageColor(w, h, (Color){30, 60, 110, 255});

    ImageDrawRectangle(&img, 0, 0, w, 4, (Color){255, 200, 60, 255});
    ImageDrawRectangle(&img, 0, h - 4, w, 4, (Color){255, 200, 60, 255});
    ImageDrawRectangle(&img, 0, 0, 4, h, (Color){255, 200, 60, 255});
    ImageDrawRectangle(&img, w - 4, 0, 4, h, (Color){255, 200, 60, 255});

    for (int i = -h; i < w; i += 12) {
        ImageDrawLine(&img, i, 0, i + h, h, (Color){90, 130, 200, 255});
    }
    ImageDrawCircle(&img, w / 2, h / 2, 8, (Color){240, 240, 240, 255});

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

typedef struct {
    int total_clicks;
    int last_click_kind;  // 0 = none, 1 = text-only, 2 = image-only, 3 = image+text
} App_State;

static const char *kind_label(int kind) {
    switch (kind) {
        case 1: return "text-only";
        case 2: return "image-only";
        case 3: return "image + text";
    }
    return "none";
}

int main(void) {
    printf("Wollix Image-capable Button Demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix Image-capable Button Demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    Texture2D tex_icon = create_icon_texture();
    Texture2D tex_land = create_landscape_texture();

    App_State app = {0};

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();
        WLX_Rect root = { 0, 0, w, h };

        wlx_begin(ctx, root, wlx_process_raylib_input);
        BeginDrawing();
            ClearBackground((Color){18, 22, 32, 255});

            wlx_layout_begin(ctx, 8, WLX_VERT,
                .sizes = (WLX_Slot_Size[]){
                    WLX_SLOT_PX(36),     // title
                    WLX_SLOT_PX(22),     // section heading
                    WLX_SLOT_PX(72),     // mode row
                    WLX_SLOT_PX(22),     // section heading
                    WLX_SLOT_PX(72),     // placement row
                    WLX_SLOT_PX(22),     // section heading
                    WLX_SLOT_PX(96),     // scale row
                    WLX_SLOT_FLEX(1),    // status
                },
                .padding = 12, .gap = 8);

                wlx_label(ctx, "wlx_button: text-only, image-only, image + text",
                    .font_size = 22, .align = WLX_CENTER,
                    .front_color = WLX_WHITE);

                // ── Modes ────────────────────────────────────────────────
                wlx_label(ctx, "Modes",
                    .font_size = 14, .align = WLX_LEFT,
                    .front_color = (Color){180, 200, 220, 255});

                wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 12);
                    if (wlx_button(ctx, "Save",
                        .font_size = 18, .align = WLX_CENTER)) {
                        app.total_clicks++; app.last_click_kind = 1;
                    }

                    if (wlx_button(ctx, "",
                        .texture = tex_icon, .align = WLX_CENTER,
                        .image_size = 48)) {
                        app.total_clicks++; app.last_click_kind = 2;
                    }

                    if (wlx_button(ctx, "Save",
                        .texture = tex_icon,
                        .image_size = 28, .image_text_gap = 10,
                        .font_size = 18, .align = WLX_CENTER)) {
                        app.total_clicks++; app.last_click_kind = 3;
                    }
                wlx_layout_end(ctx);

                // ── Placements ───────────────────────────────────────────
                wlx_label(ctx, "Image placement (image + text)",
                    .font_size = 14, .align = WLX_LEFT,
                    .front_color = (Color){180, 200, 220, 255});

                wlx_layout_begin(ctx, 4, WLX_HORZ, .gap = 12);
                    WLX_Image_Placement places[4] = {
                        WLX_IMAGE_PLACEMENT_LEFT,
                        WLX_IMAGE_PLACEMENT_RIGHT,
                        WLX_IMAGE_PLACEMENT_TOP,
                        WLX_IMAGE_PLACEMENT_BOTTOM,
                    };
                    for (int i = 0; i < 4; i++) {
                        wlx_push_id(ctx, i + 1);
                        if (wlx_button(ctx, placement_label(places[i]),
                            .texture = tex_icon,
                            .image_placement = places[i],
                            .image_size = 28, .image_text_gap = 8,
                            .font_size = 16, .align = WLX_CENTER)) {
                            app.total_clicks++; app.last_click_kind = 3;
                        }
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

                // ── Scales ───────────────────────────────────────────────
                wlx_label(ctx, "Texture scale (image-only, landscape source)",
                    .font_size = 14, .align = WLX_LEFT,
                    .front_color = (Color){180, 200, 220, 255});

                wlx_layout_begin(ctx, 4, WLX_HORZ, .gap = 12);
                    WLX_Image_Scale scales[4] = {
                        WLX_IMAGE_SCALE_STRETCH,
                        WLX_IMAGE_SCALE_FIT,
                        WLX_IMAGE_SCALE_FILL,
                        WLX_IMAGE_SCALE_NONE,
                    };
                    for (int i = 0; i < 4; i++) {
                        wlx_push_id(ctx, 100 + i);
                        wlx_layout_begin(ctx, 2, WLX_VERT,
                            .sizes = (WLX_Slot_Size[]){
                                WLX_SLOT_PX(18), WLX_SLOT_FLEX(1),
                            }, .gap = 4);
                            wlx_label(ctx, scale_label(scales[i]),
                                .font_size = 12, .align = WLX_CENTER,
                                .front_color = WLX_WHITE);
                            if (wlx_button(ctx, "",
                                .texture = tex_land,
                                .texture_scale = scales[i],
                                .align = WLX_CENTER)) {
                                app.total_clicks++; app.last_click_kind = 2;
                            }
                        wlx_layout_end(ctx);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

                // ── Status ───────────────────────────────────────────────
                char status[160];
                snprintf(status, sizeof(status),
                    "Clicks: %d   |   Last: %s",
                    app.total_clicks, kind_label(app.last_click_kind));
                wlx_label(ctx, status,
                    .font_size = 16, .align = WLX_LEFT,
                    .front_color = (Color){200, 220, 240, 255});

            wlx_layout_end(ctx);

        wlx_end(ctx);
        EndDrawing();
    }

    UnloadTexture(tex_icon);
    UnloadTexture(tex_land);
    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
