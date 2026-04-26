// auth.c - AuthWollix-style login card demo for wollix
// Demonstrates colors, borders, transparency, font styling, and centered layout.
// Build: make auth

#include <stddef.h>
#include <stdio.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_raylib.h"

#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 720
#define TARGET_FPS    60

#define CARD_W 420
#define CARD_H 530
#define CARD_PAD 32

static char email_buf[128] = "";

static void draw_background(float w, float h) {
    // Subtle radial glow behind center
    DrawCircleGradient((int)(w * 0.5f), (int)(h * 0.45f), 420,
        (Color){20, 25, 55, 80}, (Color){0, 0, 0, 0});

    // Left glass card (partially visible)
    float lw = 310, lh = 380;
    float lx = w * 0.5f - CARD_W * 0.5f - 140;
    float ly = h * 0.5f - lh * 0.35f;
    DrawRectangleRounded((Rectangle){lx, ly, lw, lh}, 0.05f, 8,
        (Color){18, 24, 45, 90});
    DrawRectangleRoundedLinesEx((Rectangle){lx, ly, lw, lh}, 0.05f, 8, 1.0f,
        (Color){55, 60, 90, 70});

    // Right glass card
    float rw = 310, rh = 380;
    float rx = w * 0.5f + CARD_W * 0.5f - 150;
    float ry = h * 0.5f - rh * 0.3f;
    DrawRectangleRounded((Rectangle){rx, ry, rw, rh}, 0.05f, 8,
        (Color){18, 24, 45, 90});
    DrawRectangleRoundedLinesEx((Rectangle){rx, ry, rw, rh}, 0.05f, 8, 1.0f,
        (Color){55, 60, 90, 70});

    // Decorative X marks
    Color xc = {50, 55, 80, 80};
    float x1 = w * 0.22f, y1 = h * 0.18f;
    DrawLineEx((Vector2){x1 - 12, y1 - 12}, (Vector2){x1 + 12, y1 + 12}, 1.5f, xc);
    DrawLineEx((Vector2){x1 + 12, y1 - 12}, (Vector2){x1 - 12, y1 + 12}, 1.5f, xc);
    float x2 = w * 0.78f, y2 = h * 0.22f;
    DrawLineEx((Vector2){x2 - 12, y2 - 12}, (Vector2){x2 + 12, y2 + 12}, 1.5f, xc);
    DrawLineEx((Vector2){x2 + 12, y2 - 12}, (Vector2){x2 - 12, y2 + 12}, 1.5f, xc);

    // Small dots
    Color dc = {55, 60, 85, 90};
    DrawCircleV((Vector2){w * 0.35f, h * 0.82f}, 2.0f, dc);
    DrawCircleV((Vector2){w * 0.65f, h * 0.85f}, 2.0f, dc);
    DrawCircleV((Vector2){w * 0.18f, h * 0.55f}, 2.0f, dc);
    DrawCircleV((Vector2){w * 0.82f, h * 0.60f}, 2.0f, dc);
}

// static void draw_or_lines - replaced by wlx_separator

static void draw_icon(float w, float h) {
    float cx = w * 0.5f;
    float cy = h * 0.5f - CARD_H * 0.5f + 44;
    DrawCircleV((Vector2){cx, cy}, 18, (Color){25, 30, 55, 200});
    DrawCircleLinesV((Vector2){cx, cy}, 18, (Color){70, 80, 130, 180});
    Vector2 pts[4] = {
        {cx, cy - 9}, {cx + 7, cy}, {cx, cy + 9}, {cx - 7, cy}
    };
    for (int i = 0; i < 4; i++)
        DrawLineEx(pts[i], pts[(i + 1) % 4], 1.5f, (Color){140, 150, 190, 200});
}

static void draw_hero_text(float w, float h) {
    // "Introducing" above the card
    Color intro_c = {120, 140, 180, 120};
    const char *intro = "Introducing";
    int iw = MeasureText(intro, 16);
    DrawText(intro, (int)(w * 0.5f) - iw / 2,
             (int)(h * 0.5f - CARD_H * 0.5f - 90), 16, intro_c);

    // "AuthWollix" large ghost text
    Color hero_c = {180, 190, 220, 80};
    const char *hero = "AuthWollix";
    int hw = MeasureText(hero, 60);
    DrawText(hero, (int)(w * 0.5f) - hw / 2,
             (int)(h * 0.5f - CARD_H * 0.5f - 68), 60, hero_c);

    // Subtitle below the card
    Color sub_c = {120, 125, 155, 100};
    const char *s1 = "Themed login box.";
    const char *s2 = "Powered by Wollix.";
    int s1w = MeasureText(s1, 14);
    int s2w = MeasureText(s2, 14);
    DrawText(s1, (int)(w * 0.5f) - s1w / 2,
             (int)(h * 0.5f + CARD_H * 0.5f + 18), 14, sub_c);
    DrawText(s2, (int)(w * 0.5f) - s2w / 2,
             (int)(h * 0.5f + CARD_H * 0.5f + 38), 14, sub_c);
}

static void draw_top_bar(float w) {
    Color tc = {140, 145, 170, 200};
    int fs = 14;
    DrawText("Left", 40, 20, fs, tc);

    const char *docs = "Right";
    int dw = MeasureText(docs, fs);
    DrawRectangleRoundedLinesEx(
        (Rectangle){w - 50.0f - dw, 16, (float)(dw + 24), 28},
        0.4f, 6, 1.0f, (Color){60, 65, 90, 120});
    DrawText(docs, (int)w - 38 - dw, 22, fs, tc);

    // Center diamond icon
    float icx = w * 0.5f, icy = 28;
    Vector2 d[4] = {{icx, icy-10},{icx+8, icy},{icx, icy+10},{icx-8, icy}};
    for (int i = 0; i < 4; i++)
        DrawLineEx(d[i], d[(i+1)%4], 1.5f, tc);
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "AuthWollix Login - wollix demo");
    SetTargetFPS(TARGET_FPS);

    Font font = LoadFontEx("demos/assets/PublicSans-Regular.ttf", 32, NULL, 0);
    bool font_ok = font.glyphCount > 0;
    if (font_ok) SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    else         printf("WARNING: could not load PublicSans-Regular.ttf\n");
    WLX_Font wfont = font_ok ? wlx_font_from_raylib(&font) : WLX_FONT_DEFAULT;

    // White-card theme - transparent backgrounds so Raylib-drawn card shows through
    WLX_Theme card_theme = wlx_theme_light;
    card_theme.font             = wfont;
    card_theme.background       = (Color){255, 255, 255, 0};
    card_theme.foreground       = (Color){210, 215, 235, 255};
    card_theme.surface          = (Color){0, 0, 0, 0};
    card_theme.border           = (Color){0, 0, 0, 0};
    card_theme.accent           = (Color){210, 215, 235, 255};
    card_theme.border_width     = 0;
    card_theme.roundness        = 0.35f;
    card_theme.rounded_segments = 6;
    card_theme.padding          = 0;
    card_theme.hover_brightness = 0.05f;
    card_theme.opacity          = -1.0f;
    card_theme.input.border_focus  = (Color){90, 110, 200, 255};
    card_theme.input.cursor        = (Color){210, 215, 235, 255};
    card_theme.input.border_width  = 1.0f;

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);
    ctx->theme = &card_theme;

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();
        WLX_Rect root = { 0, 0, w, h };

        wlx_begin(ctx, root, wlx_process_raylib_input);

        BeginDrawing();
        ClearBackground((Color){6, 8, 20, 255});

        draw_background(w, h);
        draw_hero_text(w, h);
        draw_icon(w, h);

        // ---- Wollix UI: card content centered on screen ----
        wlx_layout_begin_s(ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_FLEX(1), WLX_SLOT_PX(CARD_H), WLX_SLOT_FLEX(1)));

            wlx_label(ctx, "", .height = -1);

            wlx_layout_begin_s(ctx, WLX_HORZ,
                WLX_SIZES(WLX_SLOT_FLEX(1), WLX_SLOT_PX(CARD_W), WLX_SLOT_FLEX(1)));

                wlx_label(ctx, "", .width = -1);

                // ---- Card form ----
                wlx_layout_begin_s(ctx, WLX_VERT,
                    WLX_SIZES(
                        WLX_SLOT_PX(72),   // icon area
                        WLX_SLOT_PX(36),   // heading
                        WLX_SLOT_PX(28),   // gap
                        WLX_SLOT_PX(22),   // email label
                        WLX_SLOT_PX(6),    // gap
                        WLX_SLOT_PX(54),   // input
                        WLX_SLOT_PX(18),   // gap
                        WLX_SLOT_PX(44),   // continue btn
                        WLX_SLOT_PX(36),   // OR separator
                        WLX_SLOT_PX(44),   // ms btn
                        WLX_SLOT_PX(12),   // gap
                        WLX_SLOT_PX(44),   // o365 btn
                        WLX_SLOT_PX(20),   // gap
                        WLX_SLOT_PX(22),   // footer
                        WLX_SLOT_FLEX(1)   // fill
                    ),
                    .padding = 0,
                    .back_color = (WLX_Color){14, 18, 35, 220},
                    .border_color = (WLX_Color){55, 65, 110, 140},
                    .border_width = 1.0f,
                    .roundness = 0.04f,
                    .rounded_segments = 8);

                    wlx_label(ctx, "", .height = -1);

                    wlx_label(ctx, "Sign in Wollix",
                        .font_size = 21, .align = WLX_CENTER, .height = 36,
                        .front_color = (Color){220, 225, 240, 255});

                    wlx_label(ctx, "", .height = -1);

                    // Email label
                    wlx_layout_begin_s(ctx, WLX_HORZ,
                        WLX_SIZES(WLX_SLOT_PX(CARD_PAD), WLX_SLOT_FLEX(1), WLX_SLOT_PX(CARD_PAD)));
                        wlx_label(ctx, "", .width = -1);
                        wlx_label(ctx, "Email address",
                            .font_size = 14, .height = 22, .align = WLX_LEFT,
                            .front_color = (Color){190, 195, 215, 255});
                        wlx_label(ctx, "", .width = -1);
                    wlx_layout_end(ctx);

                    wlx_label(ctx, "", .height = -1);

                    // Email input
                    wlx_layout_begin_s(ctx, WLX_HORZ,
                        WLX_SIZES(WLX_SLOT_PX(CARD_PAD), WLX_SLOT_FLEX(1), WLX_SLOT_PX(CARD_PAD)));
                        wlx_label(ctx, "", .width = -1);
                        wlx_inputbox(ctx, "", email_buf, sizeof(email_buf),
                            .height = 54, .font_size = 16,
                            .back_color = (Color){10, 14, 30, 180},
                            .front_color = (Color){210, 215, 235, 255},
                            .border_color = (Color){50, 60, 100, 180},
                            .border_focus_color = (Color){90, 110, 200, 255},
                            .border_width = 1.0f,
                            .roundness = 0.35f,
                            .content_padding = 14);
                        wlx_label(ctx, "", .width = -1);
                    wlx_layout_end(ctx);

                    wlx_label(ctx, "", .height = -1);

                    // Continue button (dark filled)
                    wlx_layout_begin_s(ctx, WLX_HORZ,
                        WLX_SIZES(WLX_SLOT_PX(CARD_PAD), WLX_SLOT_FLEX(1), WLX_SLOT_PX(CARD_PAD)));
                        wlx_label(ctx, "", .width = -1);
                        wlx_button(ctx, "Continue",
                            .height = 44, .font_size = 15, .align = WLX_CENTER,
                            .back_color = (Color){20, 28, 60, 200},
                            .front_color = (Color){210, 215, 235, 255},
                            .border_color = (Color){60, 75, 130, 180},
                            .border_width = 1.0f,
                            .roundness = 0.35f);
                        wlx_label(ctx, "", .width = -1);
                    wlx_layout_end(ctx);

                    // OR separator
                    wlx_layout_begin_s(ctx, WLX_HORZ,
                        WLX_SIZES(WLX_SLOT_PX(CARD_PAD), WLX_SLOT_FLEX(1),
                                  WLX_SLOT_PX(40), WLX_SLOT_FLEX(1), WLX_SLOT_PX(CARD_PAD)));
                        wlx_label(ctx, "", .width = -1);
                        wlx_separator(ctx, .color = (WLX_Color){60, 70, 110, 140});
                        wlx_label(ctx, "OR",
                            .font_size = 12, .align = WLX_CENTER,
                            .front_color = (Color){80, 90, 130, 200});
                        wlx_separator(ctx, .color = (WLX_Color){60, 70, 110, 140});
                        wlx_label(ctx, "", .width = -1);
                    wlx_layout_end(ctx);

                    // Continue with Google account
                    wlx_layout_begin_s(ctx, WLX_HORZ,
                        WLX_SIZES(WLX_SLOT_PX(CARD_PAD), WLX_SLOT_FLEX(1), WLX_SLOT_PX(CARD_PAD)));
                        wlx_label(ctx, "", .width = -1);
                        wlx_button(ctx, "Continue with Google account",
                            .height = 44, .font_size = 14, .align = WLX_CENTER,
                            .back_color = (Color){0, 0, 0, 0},
                            .front_color = (Color){200, 205, 225, 255},
                            .border_color = (Color){50, 60, 100, 160},
                            .border_width = 1.0f,
                            .roundness = 0.35f);
                        wlx_label(ctx, "", .width = -1);
                    wlx_layout_end(ctx);

                    wlx_label(ctx, "", .height = -1);

                    // Continue with Facebook account
                    wlx_layout_begin_s(ctx, WLX_HORZ,
                        WLX_SIZES(WLX_SLOT_PX(CARD_PAD), WLX_SLOT_FLEX(1), WLX_SLOT_PX(CARD_PAD)));
                        wlx_label(ctx, "", .width = -1);
                        wlx_button(ctx, "Continue with Facebook account",
                            .height = 44, .font_size = 14, .align = WLX_CENTER,
                            .back_color = (Color){0, 0, 0, 0},
                            .front_color = (Color){200, 205, 225, 255},
                            .border_color = (Color){50, 60, 100, 160},
                            .border_width = 1.0f,
                            .roundness = 0.35f);
                        wlx_label(ctx, "", .width = -1);
                    wlx_layout_end(ctx);

                    wlx_label(ctx, "", .height = -1);

                    // Footer
                    wlx_label(ctx, "Don't have an account?  Sign up",
                        .font_size = 13, .align = WLX_CENTER, .height = 22,
                        .front_color = (Color){100, 110, 150, 200});

                    wlx_label(ctx, "", .height = -1);

                wlx_layout_end(ctx);

                wlx_label(ctx, "", .width = -1);

            wlx_layout_end(ctx);

            wlx_label(ctx, "", .height = -1);

        wlx_layout_end(ctx);

        draw_top_bar(w);

        wlx_end(ctx);
        EndDrawing();
    }

    if (font_ok) UnloadFont(font);
    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
