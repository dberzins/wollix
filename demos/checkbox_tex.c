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

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define TARGET_FPS 60

typedef struct {
    bool option1;
    bool option2;
    bool option3;
    bool enable_feature;
    bool show_details;
    bool accept_terms;
} App_State;

static App_State app = {0};

// Create a simple wlx_checkbox texture programmatically
Texture2D create_unchecked_texture(void) {
    int size = 64;
    Image img = GenImageColor(size, size, BLANK);

    // Draw border
    ImageDrawRectangle(&img, 0, 0, size, size, WLX_LIGHTGRAY);
    // ImageDrawRectangle(&img, 2, 2, size - 4, size - 4, DARKGRAY);
    // ImageDrawRectangle(&img, 4, 4, size - 8, size - 8, (Color){50, 50, 50, 255});
    ImageDrawRectangle(&img, 4, 4, size - 8, size - 8, (Color){50, 50, 50, 255});

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// Create a checked wlx_checkbox texture programmatically
Texture2D create_checked_texture(void) {
    int size = 64;
    Image img = GenImageColor(size, size, BLANK);

    // Draw border
    ImageDrawRectangle(&img, 0, 0, size, size, WLX_LIGHTGRAY);
    // ImageDrawRectangle(&img, 2, 2, size - 4, size - 4, DARKGRAY);
    // ImageDrawRectangle(&img, 4, 4, size - 8, size - 8, (Color){50, 50, 50, 255});
    ImageDrawRectangle(&img, 4, 4, size - 8, size - 8, (Color){50, 50, 50, 255});

    // Draw checkmark
    int padding = size / 4;
    int thickness = 8;

    // Draw checkmark lines
    for (int i = 0; i < thickness; i++) {
        ImageDrawLine(&img, padding + i, size / 2, size / 2, size - padding, WLX_LIGHTGRAY);
        ImageDrawLine(&img, size / 2, size - padding, size - padding - i, padding, WLX_LIGHTGRAY);
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// Create alternative textures (stars)
Texture2D create_star_empty_texture(void) {
    int size = 64;
    Image img = GenImageColor(size, size, BLANK);

    // Draw empty star outline
    int center_x = size / 2;
    int center_y = size / 2;
    int outer_radius = size / 2 - 4;


    // Draw star outline (simplified)
    ImageDrawCircle(&img, center_x, center_y, outer_radius, GRAY);
    ImageDrawCircle(&img, center_x, center_y, outer_radius - 2, (Color){40, 40, 40, 255});

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

Texture2D create_star_filled_texture(void) {
    int size = 64;
    Image img = GenImageColor(size, size, BLANK);

    // Draw filled star
    int center_x = size / 2;
    int center_y = size / 2;
    int outer_radius = size / 2 - 4;

    // Draw filled circle (simplified star)
    ImageDrawCircle(&img, center_x, center_y, outer_radius, GOLD);
    ImageDrawCircle(&img, center_x, center_y, outer_radius - 2, YELLOW);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

int main(void) {
    printf("Wollix Checkbox Texture Demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix Checkbox Texture Demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    // Create textures
    Texture2D tex_unchecked = create_unchecked_texture();
    Texture2D tex_checked = create_checked_texture();
    Texture2D tex_star_empty = create_star_empty_texture();
    Texture2D tex_star_filled = create_star_filled_texture();

    while (!WindowShouldClose()) {
        float w = GetRenderWidth();
        float h = GetRenderHeight();
        WLX_Rect r = {.x = 0, .y = 0, .w = w, .h = h};

        wlx_begin(ctx, r, wlx_process_raylib_input);
            BeginDrawing();
                ClearBackground(WLX_BACKGROUND_COLOR);

                wlx_layout_begin(ctx, 1, WLX_HORZ);
                    wlx_layout_begin(ctx, 9, WLX_VERT);

                        // Title
                        wlx_textbox(ctx, "Checkbox texure Demo",
                            .widget_align = WLX_TOP_LEFT, .height = 40, .font_size = 20, .back_color = WLX_BACKGROUND_COLOR, .spacing = 2, .align = WLX_TOP_LEFT);
                        // Subtitle
                        wlx_textbox(ctx, "Standard checkboxes with custom textures",
                            .widget_align = WLX_TOP_LEFT, .height = 40, .font_size = 30, .back_color = WLX_BACKGROUND_COLOR, .spacing = 2, .align = WLX_TOP_LEFT);

                        // Texture-based wlx_checkbox options (checkmark style)
                        if (wlx_checkbox_tex(ctx, "Option 1: Enable feature A", &app.option1,
                            .font_size = 20,
                            .tex_checked = tex_checked,
                            .tex_unchecked = tex_unchecked,
                            .widget_align = WLX_LEFT, .align = WLX_LEFT,
                            .back_color = DARKBLUE,
                            .front_color = WLX_WHITE,
                        )) {
                            printf("Option 1 toggled: %s\n", app.option1 ? "ON" : "OFF");
                        }

                        if (wlx_checkbox_tex(ctx, "Option 2: Enable feature B", &app.option2,
                            .font_size = 30,
                            .tex_checked = tex_checked,
                            .tex_unchecked = tex_unchecked,
                            .widget_align = WLX_LEFT, .align = WLX_LEFT,
                            .back_color = RED,
                            .front_color = WLX_WHITE,
                        )) {
                            printf("Option 2 toggled: %s\n", app.option2 ? "ON" : "OFF");
                        }

                        if (wlx_checkbox_tex(ctx, "Option 3: Enable feature C", &app.option3,
                            .font_size = 30,
                            .tex_checked = tex_checked,
                            .tex_unchecked = tex_unchecked,
                            .widget_align = WLX_LEFT, .align = WLX_LEFT,
                            .back_color = YELLOW,
                            .front_color = WLX_WHITE,
                        )) {
                            printf("Option 3 toggled: %s\n", app.option3 ? "ON" : "OFF");
                        }

                        // Subtitle for star-style checkboxes
                        wlx_textbox(ctx, "Star-style toggle buttons",
                            .widget_align = WLX_LEFT, .height = 40, .font_size = 30, .back_color = WLX_BACKGROUND_COLOR, .align = WLX_LEFT);

                        // Star-style texture checkboxes
                        WLX_Checkbox_Tex_Opt opt = wlx_default_checkbox_tex_opt(
                            .font_size = 30,
                            .tex_checked = tex_star_filled,
                            .tex_unchecked = tex_star_empty,
                            .widget_align = WLX_LEFT, .align = WLX_LEFT,
                            .back_color = YELLOW,
                            .front_color = WLX_WHITE,
                        );
                        
                        if (wlx_checkbox_tex_impl(ctx, "Enable advanced mode", &app.enable_feature, opt, __FILE__, __LINE__)) {
                            printf("Advanced mode toggled: %s\n", app.enable_feature ? "ON" : "OFF");

                        }
                        
                        if (wlx_checkbox_tex_impl(ctx, "Show detailed information", &app.show_details, opt, __FILE__, __LINE__)) {
                            printf("Show details toggled: %s\n", app.show_details ? "ON" : "OFF");
                        }

                        // Status display
                        char status_text[256];
                        snprintf(status_text, sizeof(status_text),
                            "Status: %d standard options enabled | %d advanced features enabled",
                            app.option1 + app.option2 + app.option3,
                            app.enable_feature + app.show_details
                        );
                        wlx_textbox(ctx, status_text,
                            .widget_align = WLX_BOTTOM_LEFT, .height = 40, .font_size = 18, .back_color = WLX_BACKGROUND_COLOR, .align = WLX_TOP_LEFT);

                    wlx_layout_end(ctx);
                wlx_layout_end(ctx);

            EndDrawing();
        wlx_end(ctx);
    }

    // Cleanup textures
    UnloadTexture(tex_unchecked);
    UnloadTexture(tex_checked);
    UnloadTexture(tex_star_empty);
    UnloadTexture(tex_star_filled);

    CloseWindow();
    wlx_context_destroy(ctx);
    free(ctx);
    return 0;
}
