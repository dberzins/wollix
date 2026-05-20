// disabled_demo.c - Demo showing the disabled-state model
// Each interactive widget is shown enabled (left column) and disabled
// (right column). Switch between the dark / light / glass presets via
// the radio group at the top. Hover over the right column to verify
// that disabled widgets do not respond to hover or click input.

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

#define WINDOW_WIDTH  900
#define WINDOW_HEIGHT 720
#define TARGET_FPS    60

typedef struct {
    int    preset_index;        // 0 = dark, 1 = light, 2 = glass
    bool   checkbox_enabled;
    bool   checkbox_disabled;
    bool   toggle_enabled;
    bool   toggle_disabled;
    int    radio_enabled;
    int    radio_disabled;
    float  slider_enabled;
    float  slider_disabled;
    char   input_enabled[64];
    char   input_disabled[64];
    int    click_count;         // bumps each time the enabled button fires
} App_State;

static App_State app = {
    .preset_index      = 0,
    .checkbox_enabled  = true,
    .checkbox_disabled = true,
    .toggle_enabled    = true,
    .toggle_disabled   = true,
    .radio_enabled     = 1,
    .radio_disabled    = 1,
    .slider_enabled    = 0.6f,
    .slider_disabled   = 0.6f,
    .input_enabled     = "editable",
    .input_disabled    = "read only",
    .click_count       = 0,
};

static const WLX_Theme *preset_themes[3];
static const char      *preset_names[3]   = { "Dark", "Light", "Glass" };

static void draw_preset_picker(WLX_Context *ctx) {
    wlx_layout_begin(ctx, 4, WLX_HORZ, .padding = 6, .gap = 8);
        wlx_label(ctx, "Preset:", .height = 28, .font_size = 14);
        for (int i = 0; i < 3; i++) {
            wlx_push_id(ctx, (size_t)(i + 1));
            wlx_radio(ctx, preset_names[i], &app.preset_index, i,
                .height = 28, .font_size = 14);
            wlx_pop_id(ctx);
        }
    wlx_layout_end(ctx);
}

// One column of widgets sharing a `disabled` value. Each widget call site
// is kept on a stable line so its ID stays constant across frames.
static void draw_widget_column(WLX_Context *ctx, bool disabled) {
    wlx_layout_begin(ctx, 7, WLX_VERT, .padding = 6, .gap = 6);

        const char *label = disabled ? "Disabled" : "Enabled";
        wlx_label(ctx, label,
            .height = 24, .font_size = 14, .align = WLX_CENTER);

        char btn_label[40];
        snprintf(btn_label, sizeof(btn_label),
            disabled ? "Save (off)" : "Save (clicks: %d)", app.click_count);
        if (wlx_button(ctx, btn_label,
                .height = 32, .font_size = 13,
                .widget_align = WLX_CENTER,
                .disabled = disabled)) {
            app.click_count++;
        }

        wlx_checkbox(ctx,
            disabled ? "Locked option" : "Enable feature",
            disabled ? &app.checkbox_disabled : &app.checkbox_enabled,
            .height = 24, .font_size = 13,
            .disabled = disabled);

        wlx_toggle(ctx,
            disabled ? "Beta channel" : "Notifications",
            disabled ? &app.toggle_disabled : &app.toggle_enabled,
            .height = 26, .font_size = 13,
            .disabled = disabled);

        wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 8, .padding = 0);
            int *active = disabled ? &app.radio_disabled : &app.radio_enabled;
            wlx_radio(ctx, "A", active, 0,
                .height = 26, .font_size = 13, .disabled = disabled);
            wlx_radio(ctx, "B", active, 1,
                .height = 26, .font_size = 13, .disabled = disabled);
            wlx_radio(ctx, "C", active, 2,
                .height = 26, .font_size = 13, .disabled = disabled);
        wlx_layout_end(ctx);

        wlx_slider(ctx, "Volume",
            disabled ? &app.slider_disabled : &app.slider_enabled,
            .min_value = 0.0f, .max_value = 1.0f,
            .height = 28, .font_size = 12,
            .disabled = disabled);

        wlx_inputbox(ctx,
            "Name",
            disabled ? app.input_disabled : app.input_enabled,
            disabled ? sizeof(app.input_disabled) : sizeof(app.input_enabled),
            .height = 32, .font_size = 13,
            .disabled = disabled);

    wlx_layout_end(ctx);
}

int main(void) {
    printf("Disabled-state demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Disabled Demo - Phase 1");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    preset_themes[0] = &wlx_theme_dark;
    preset_themes[1] = &wlx_theme_light;
    preset_themes[2] = &wlx_theme_glass;

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();
        WLX_Rect root = {0, 0, w, h};

        const WLX_Theme *theme = preset_themes[app.preset_index];
        ctx->theme = theme;

        wlx_begin(ctx, root, wlx_process_raylib_input);
        BeginDrawing();

            ClearBackground((Color){
                theme->background.r,
                theme->background.g,
                theme->background.b,
                theme->background.a,
            });

            wlx_layout_begin(ctx, 4, WLX_VERT,
                .sizes = (WLX_Slot_Size[]){
                    WLX_SLOT_PX(36),
                    WLX_SLOT_PX(36),
                    WLX_SLOT_PX(28),
                    WLX_SLOT_FLEX(1),
                });

                wlx_label(ctx, "Disabled State Model",
                    .font_size = 22, .align = WLX_CENTER,
                    .front_color = theme->foreground);

                draw_preset_picker(ctx);

                wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 8, .padding = 0);
                    wlx_label(ctx, "Enabled",
                        .font_size = 15, .align = WLX_CENTER);
                    wlx_label(ctx, "Disabled (hover-tint suppressed)",
                        .font_size = 15, .align = WLX_CENTER);
                wlx_layout_end(ctx);

                wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 12, .padding = 8);
                    wlx_push_id(ctx, 11);
                    draw_widget_column(ctx, false);
                    wlx_pop_id(ctx);

                    wlx_push_id(ctx, 22);
                    draw_widget_column(ctx, true);
                    wlx_pop_id(ctx);
                wlx_layout_end(ctx);

            wlx_layout_end(ctx);

            wlx_end(ctx);
        EndDrawing();
    }

    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
