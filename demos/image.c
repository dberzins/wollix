// image.c - Showcase wlx_image scale modes (STRETCH, FIT, FILL, NONE)
// alignment anchors, and opacity-stack folding with procedurally-generated
// portrait and landscape textures.

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
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

static const char *scale_label(WLX_Image_Scale s) {
    switch (s) {
        case WLX_IMAGE_SCALE_STRETCH: return "STRETCH";
        case WLX_IMAGE_SCALE_FIT:     return "FIT";
        case WLX_IMAGE_SCALE_FILL:    return "FILL";
        case WLX_IMAGE_SCALE_NONE:    return "NONE";
    }
    return "?";
}

// Landscape texture (128x64): colored border + diagonal stripes + a central
// dot, so FIT / FILL / STRETCH visibly differ when rendered into non-square
// destination slots.
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

// Portrait texture (64x128): contrasting palette so it's easy to tell which
// source is feeding which pane.
static Texture2D create_portrait_texture(void) {
    int w = 64, h = 128;
    Image img = GenImageColor(w, h, (Color){80, 30, 60, 255});

    ImageDrawRectangle(&img, 0, 0, w, 4, (Color){200, 240, 120, 255});
    ImageDrawRectangle(&img, 0, h - 4, w, 4, (Color){200, 240, 120, 255});
    ImageDrawRectangle(&img, 0, 0, 4, h, (Color){200, 240, 120, 255});
    ImageDrawRectangle(&img, w - 4, 0, 4, h, (Color){200, 240, 120, 255});

    for (int j = 0; j < h; j += 16) {
        ImageDrawRectangle(&img, 8, j + 4, w - 16, 8, (Color){180, 80, 130, 255});
    }
    ImageDrawCircle(&img, w / 2, h / 2, 10, (Color){255, 255, 255, 255});

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

static void draw_scale_pane(WLX_Context *ctx, WLX_Texture tex,
                            WLX_Image_Scale scale, const char *src_tag) {
    char title[64];
    snprintf(title, sizeof(title), "%s (%s)", scale_label(scale), src_tag);

    wlx_layout_begin(ctx, 2, WLX_VERT,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(22), WLX_SLOT_FLEX(1) },
        .padding = 4);

        wlx_label(ctx, title,
            .font_size = 14, .align = WLX_CENTER,
            .front_color = WLX_WHITE);

        wlx_image(ctx, tex, .scale = scale, .align = WLX_CENTER);

    wlx_layout_end(ctx);
}

int main(void) {
    printf("Wollix Image Widget Demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix Image Demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    Texture2D tex_land = create_landscape_texture();
    Texture2D tex_port = create_portrait_texture();

    double start = GetTime();

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();
        WLX_Rect root = { 0, 0, w, h };

        float t = (float)(GetTime() - start);
        float fade = 0.5f + 0.5f * sinf(t * 1.2f);

        wlx_begin(ctx, root, wlx_process_raylib_input);
        BeginDrawing();
            ClearBackground((Color){18, 22, 32, 255});

            wlx_layout_begin(ctx, 4, WLX_VERT,
                .sizes = (WLX_Slot_Size[]){
                    WLX_SLOT_PX(36),
                    WLX_SLOT_FLEX(1),
                    WLX_SLOT_FLEX(1),
                    WLX_SLOT_PX(180),
                },
                .padding = 8, .gap = 6);

                wlx_label(ctx, "wlx_image: scale modes, alignment, opacity",
                    .font_size = 22, .align = WLX_CENTER,
                    .front_color = WLX_WHITE);

                wlx_layout_begin(ctx, 4, WLX_HORZ, .gap = 6);
                    wlx_push_id(ctx, 1); draw_scale_pane(ctx, tex_land, WLX_IMAGE_SCALE_STRETCH, "landscape"); wlx_pop_id(ctx);
                    wlx_push_id(ctx, 2); draw_scale_pane(ctx, tex_land, WLX_IMAGE_SCALE_FIT,     "landscape"); wlx_pop_id(ctx);
                    wlx_push_id(ctx, 3); draw_scale_pane(ctx, tex_land, WLX_IMAGE_SCALE_FILL,    "landscape"); wlx_pop_id(ctx);
                    wlx_push_id(ctx, 4); draw_scale_pane(ctx, tex_land, WLX_IMAGE_SCALE_NONE,    "landscape"); wlx_pop_id(ctx);
                wlx_layout_end(ctx);

                wlx_layout_begin(ctx, 4, WLX_HORZ, .gap = 6);
                    wlx_push_id(ctx, 5); draw_scale_pane(ctx, tex_port, WLX_IMAGE_SCALE_STRETCH, "portrait"); wlx_pop_id(ctx);
                    wlx_push_id(ctx, 6); draw_scale_pane(ctx, tex_port, WLX_IMAGE_SCALE_FIT,     "portrait"); wlx_pop_id(ctx);
                    wlx_push_id(ctx, 7); draw_scale_pane(ctx, tex_port, WLX_IMAGE_SCALE_FILL,    "portrait"); wlx_pop_id(ctx);
                    wlx_push_id(ctx, 8); draw_scale_pane(ctx, tex_port, WLX_IMAGE_SCALE_NONE,    "portrait"); wlx_pop_id(ctx);
                wlx_layout_end(ctx);

                wlx_layout_begin(ctx, 4, WLX_HORZ,
                    .sizes = (WLX_Slot_Size[]){
                        WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1),
                    }, .gap = 6);

                    wlx_layout_begin(ctx, 2, WLX_VERT,
                        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(22), WLX_SLOT_FLEX(1) }, .padding = 4);
                        wlx_label(ctx, "FILL + WLX_LEFT (landscape)",
                            .font_size = 14, .align = WLX_CENTER, .front_color = WLX_WHITE);
                        wlx_image(ctx, tex_land,
                            .scale = WLX_IMAGE_SCALE_FILL, .align = WLX_LEFT);
                    wlx_layout_end(ctx);

                    wlx_layout_begin(ctx, 2, WLX_VERT,
                        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(22), WLX_SLOT_FLEX(1) }, .padding = 4);
                        wlx_label(ctx, "FILL + WLX_RIGHT (landscape)",
                            .font_size = 14, .align = WLX_CENTER, .front_color = WLX_WHITE);
                        wlx_image(ctx, tex_land,
                            .scale = WLX_IMAGE_SCALE_FILL, .align = WLX_RIGHT);
                    wlx_layout_end(ctx);

                    wlx_layout_begin(ctx, 2, WLX_VERT,
                        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(22), WLX_SLOT_FLEX(1) }, .padding = 4);
                        char fade_label[64];
                        snprintf(fade_label, sizeof(fade_label), "Stack opacity %.2f", (double)fade);
                        wlx_label(ctx, fade_label,
                            .font_size = 14, .align = WLX_CENTER, .front_color = WLX_WHITE);
                        wlx_push_opacity(ctx, fade);
                            wlx_image(ctx, tex_port,
                                .scale = WLX_IMAGE_SCALE_FIT, .align = WLX_CENTER);
                        wlx_pop_opacity(ctx);
                    wlx_layout_end(ctx);

                    wlx_layout_begin(ctx, 2, WLX_VERT,
                        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(22), WLX_SLOT_FLEX(1) }, .padding = 4);
                        wlx_label(ctx, "Tinted FIT (portrait)",
                            .font_size = 14, .align = WLX_CENTER, .front_color = WLX_WHITE);
                        wlx_image(ctx, tex_port,
                            .scale = WLX_IMAGE_SCALE_FIT, .align = WLX_CENTER,
                            .tint = WLX_RGBA(255, 180, 120, 255));
                    wlx_layout_end(ctx);

                wlx_layout_end(ctx);

            wlx_layout_end(ctx);

        wlx_end(ctx);
        EndDrawing();
    }

    UnloadTexture(tex_land);
    UnloadTexture(tex_port);
    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
