// label_image.c - Showcase image-capable wlx_label: text-only, text + image,
// and image-only as a supported edge case. Demonstrates the four image
// placements (LEFT/RIGHT/TOP/BOTTOM) and the four texture scale modes
// (STRETCH/FIT/FILL/NONE). Labels are non-interactive; a row with
// show_background = true exercises chrome and hover-brightness alongside
// image content.
//
// For pure image-only content, prefer wlx_image. The image-only label
// branch is shown here only to document the edge case.

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
#define WINDOW_HEIGHT 760
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

// Square 64x64 checkmark glyph on a saturated tile.
static Texture2D create_check_texture(void) {
    int s = 64;
    Image img = GenImageColor(s, s, (Color){40, 110, 70, 255});

    ImageDrawRectangle(&img, 0, 0, s, 4, (Color){80, 170, 110, 255});
    ImageDrawRectangle(&img, 0, s - 4, s, 4, (Color){80, 170, 110, 255});
    ImageDrawRectangle(&img, 0, 0, 4, s, (Color){80, 170, 110, 255});
    ImageDrawRectangle(&img, s - 4, 0, 4, s, (Color){80, 170, 110, 255});

    Color stroke = (Color){240, 250, 240, 255};
    int x0 = s / 4, y0 = s / 2;
    int x1 = s / 2 - 2, y1 = s - s / 4;
    int x2 = s - s / 4, y2 = s / 4;
    for (int t = -2; t <= 2; t++) {
        ImageDrawLine(&img, x0, y0 + t, x1, y1 + t, stroke);
        ImageDrawLine(&img, x1, y1 + t, x2, y2 + t, stroke);
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// Square 64x64 info glyph: white "i" on a blue tile.
static Texture2D create_info_texture(void) {
    int s = 64;
    Image img = GenImageColor(s, s, (Color){40, 90, 160, 255});

    ImageDrawRectangle(&img, 0, 0, s, 4, (Color){90, 140, 220, 255});
    ImageDrawRectangle(&img, 0, s - 4, s, 4, (Color){90, 140, 220, 255});
    ImageDrawRectangle(&img, 0, 0, 4, s, (Color){90, 140, 220, 255});
    ImageDrawRectangle(&img, s - 4, 0, 4, s, (Color){90, 140, 220, 255});

    Color glyph = (Color){240, 245, 250, 255};
    int cx = s / 2;
    ImageDrawCircle(&img, cx, s / 4, 5, glyph);
    ImageDrawRectangle(&img, cx - 4, s / 4 + 10, 8, s / 2 - 4, glyph);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// Non-square 128x64 landscape texture so STRETCH / FIT / FILL / NONE are
// visibly different inside the same label slot.
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

int main(void) {
    printf("Wollix Image-capable Label Demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix Image-capable Label Demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    Texture2D tex_check = create_check_texture();
    Texture2D tex_info  = create_info_texture();
    Texture2D tex_land  = create_landscape_texture();

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();
        WLX_Rect root = { 0, 0, w, h };

        wlx_begin(ctx, root, wlx_process_raylib_input);
        BeginDrawing();
            ClearBackground((Color){18, 22, 32, 255});

            wlx_layout_begin(ctx, 12, WLX_VERT,
                .sizes = (WLX_Slot_Size[]){
                    WLX_SLOT_PX(36),     // title
                    WLX_SLOT_PX(22),     // section heading
                    WLX_SLOT_PX(56),     // modes row
                    WLX_SLOT_PX(22),     // section heading
                    WLX_SLOT_PX(64),     // placement row
                    WLX_SLOT_PX(22),     // section heading
                    WLX_SLOT_PX(96),     // scale row
                    WLX_SLOT_PX(22),     // section heading
                    WLX_SLOT_PX(48),     // hover row
                    WLX_SLOT_PX(22),     // section heading
                    WLX_SLOT_PX(56),     // content padding row
                    WLX_SLOT_FLEX(1),    // note
                },
                .padding = 12, .gap = 8);

                wlx_label(ctx, "wlx_label: text-only, text + image, image-only edge case",
                    .font_size = 22, .align = WLX_CENTER,
                    .front_color = WLX_WHITE);

                // -- Modes -------------------------------------------------
                wlx_label(ctx, "Modes",
                    .font_size = 14, .align = WLX_LEFT,
                    .front_color = (Color){180, 200, 220, 255});

                wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 12);
                    wlx_label(ctx, "Saved",
                        .font_size = 18, .align = WLX_CENTER,
                        .front_color = WLX_WHITE);

                    wlx_label(ctx, "Saved",
                        .texture = tex_check,
                        .image_size = 28, .image_text_gap = 8,
                        .font_size = 18, .align = WLX_CENTER,
                        .front_color = WLX_WHITE);

                    // Image-only edge case. Prefer wlx_image for pure image content.
                    wlx_label(ctx, "",
                        .texture = tex_check, .image_size = 36,
                        .align = WLX_CENTER);
                wlx_layout_end(ctx);

                // -- Placements --------------------------------------------
                wlx_label(ctx, "Image placement (text + image)",
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
                        wlx_label(ctx, placement_label(places[i]),
                            .texture = tex_info,
                            .image_placement = places[i],
                            .image_size = 24, .image_text_gap = 6,
                            .font_size = 16, .align = WLX_CENTER,
                            .front_color = WLX_WHITE);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

                // -- Scales ------------------------------------------------
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
                            wlx_label(ctx, "",
                                .texture = tex_land,
                                .texture_scale = scales[i],
                                .align = WLX_CENTER);
                        wlx_layout_end(ctx);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

                // -- Hover background --------------------------------------
                wlx_label(ctx, "show_background = true (hover brightens the fill, tint unchanged)",
                    .font_size = 14, .align = WLX_LEFT,
                    .front_color = (Color){180, 200, 220, 255});

                wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 12);
                    wlx_label(ctx, "Confirmed",
                        .texture = tex_check, .image_size = 22, .image_text_gap = 8,
                        .font_size = 16, .align = WLX_CENTER,
                        .front_color = WLX_WHITE,
                        .show_background = true,
                        .back_color = (Color){40, 70, 50, 255});

                    wlx_label(ctx, "More info",
                        .texture = tex_info, .image_size = 22, .image_text_gap = 8,
                        .image_placement = WLX_IMAGE_PLACEMENT_RIGHT,
                        .font_size = 16, .align = WLX_CENTER,
                        .front_color = WLX_WHITE,
                        .show_background = true,
                        .back_color = (Color){40, 60, 90, 255});
                wlx_layout_end(ctx);

                // -- Content padding ---------------------------------------
                wlx_label(ctx, "Content padding (text + image inset; chrome unchanged)",
                    .font_size = 14, .align = WLX_LEFT,
                    .front_color = (Color){180, 200, 220, 255});

                wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 12);
                    wlx_label(ctx, "padding_left = 32",
                        .texture = tex_info, .image_size = 22, .image_text_gap = 8,
                        .font_size = 16, .align = WLX_LEFT,
                        .front_color = WLX_WHITE,
                        .show_background = true,
                        .back_color = (Color){40, 60, 90, 255},
                        .content_padding_left = 32);

                    wlx_label(ctx, "USE_THEME",
                        .texture = tex_check, .image_size = 22, .image_text_gap = 8,
                        .font_size = 16, .align = WLX_CENTER,
                        .front_color = WLX_WHITE,
                        .show_background = true,
                        .back_color = (Color){40, 70, 50, 255},
                        .content_padding = WLX_PADDING_USE_THEME);
                wlx_layout_end(ctx);

                // -- Note --------------------------------------------------
                wlx_label(ctx,
                    "Labels are non-interactive. For pure image-only content "
                    "prefer wlx_image; the image-only label branch above is "
                    "shown only as a supported edge case.",
                    .font_size = 14, .align = WLX_LEFT, .wrap = true,
                    .front_color = (Color){200, 220, 240, 255});

            wlx_layout_end(ctx);

        wlx_end(ctx);
        EndDrawing();
    }

    UnloadTexture(tex_check);
    UnloadTexture(tex_info);
    UnloadTexture(tex_land);
    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
