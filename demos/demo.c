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

// ---------------------------------------------------------------------------
// Application state  – every wlx_widget's data lives here
// ---------------------------------------------------------------------------
#define PANEL_COUNT 5

typedef struct {
    // Buttons
    int button_click_count;
    int active_tab;           // 0 = Controls, 1 = Settings, 2 = Dynamic, 3 = About

    // Checkboxes
    bool enable_audio;
    bool enable_vsync;
    bool fullscreen;
    bool show_fps;
    bool dark_mode;

    // Sliders
    float volume;
    float brightness;
    float r, g, b;
    float font_scale;

    // Input fields
    char name[128];
    char search[256];
    char note[512];

    // Log messages (for the nested scroll panel)
    char log_messages[50][128];
    int  log_count;

    // Dynamic tab – loop-generated panels with wlx_push_id/wlx_pop_id
    char panel_names[PANEL_COUNT][64];
    float panel_sliders[PANEL_COUNT];
    bool  panel_toggles[PANEL_COUNT];
} App_State;

static App_State app = {
    .active_tab    = 0,
    .enable_audio  = true,
    .enable_vsync  = true,
    .fullscreen    = false,
    .show_fps      = true,
    .dark_mode     = true,
    .volume        = 0.7f,
    .brightness    = 1.0f,
    .r = 0.2f, .g = 0.5f, .b = 0.8f,
    .font_scale    = 1.0f,
    .log_count     = 0,
    .panel_sliders = { 0.2f, 0.4f, 0.6f, 0.8f, 1.0f },
    .panel_toggles = { true, false, true, false, true },
};

static void add_log(const char *fmt, ...) {
    if (app.log_count >= 50) {
        // Shift logs up
        memmove(&app.log_messages[0], &app.log_messages[1], sizeof(app.log_messages[0]) * 49);
        app.log_count = 49;
    }
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(app.log_messages[app.log_count], sizeof(app.log_messages[0]), fmt, ap);
    va_end(ap);
    app.log_count++;
}

// ---------------------------------------------------------------------------
// Tab content renderers
// ---------------------------------------------------------------------------

// --- Tab 0: Controls -------------------------------------------------------
static void render_controls_tab(WLX_Context *ctx, int layout_span) {
    // 2 columns: left = buttons & checkboxes, right = nested scroll panels
    wlx_layout_begin(ctx, 2, WLX_HORZ, .span = layout_span);

        // ---- LEFT column ------------------------------------------------
        // NOTE: In a vertical layout all children share equal cell height
        // (total / count). The .height param only *shrinks* within that
        // cell, never grows.  So we use a uniform ROW_H for every item.
        #define ROW_H 40
        wlx_scroll_panel_begin(ctx, -1,
            .back_color = (Color){22, 22, 28, 255},
            .scrollbar_color = (Color){55, 55, 65, 255}
        );
            wlx_layout_begin(ctx, 17, WLX_VERT);

                wlx_textbox(ctx, "Buttons",
                    .height = ROW_H, .font_size = 22,
                    .back_color = (Color){30, 30, 40, 255}, .align = WLX_CENTER);

                if (wlx_button(ctx, "Click Me!",
                    .height = ROW_H, .font_size = 18,
                    .back_color = (Color){60, 30, 30, 255}, .align = WLX_CENTER)) {
                    app.button_click_count++;
                    add_log("Button clicked %d time(s)", app.button_click_count);
                }

                char click_text[64];
                snprintf(click_text, sizeof(click_text), "Clicks: %d", app.button_click_count);
                wlx_textbox(ctx, click_text,
                    .height = ROW_H, .font_size = 16, .align = WLX_CENTER,
                    .back_color = (Color){22, 22, 28, 255});

                if (wlx_button(ctx, "Reset Counter",
                    .height = ROW_H, .font_size = 16,
                    .back_color = (Color){40, 40, 60, 255}, .align = WLX_CENTER)) {
                    app.button_click_count = 0;
                    add_log("Counter reset");
                }

                if (wlx_button(ctx, "Add Log Entry",
                    .height = ROW_H, .font_size = 16,
                    .back_color = (Color){30, 50, 30, 255}, .align = WLX_CENTER)) {
                    add_log("Manual log entry #%d", app.log_count + 1);
                }

                wlx_textbox(ctx, "Checkboxes",
                    .height = ROW_H, .font_size = 22,
                    .back_color = (Color){30, 30, 40, 255}, .align = WLX_CENTER);

                if (wlx_checkbox(ctx, "Enable audio", &app.enable_audio,
                    .height = ROW_H, .font_size = 18)) {
                    add_log("Audio %s", app.enable_audio ? "ON" : "OFF");
                }

                if (wlx_checkbox(ctx, "V-Sync", &app.enable_vsync,
                    .height = ROW_H, .font_size = 18)) {
                    add_log("V-Sync %s", app.enable_vsync ? "ON" : "OFF");
                }

                if (wlx_checkbox(ctx, "Fullscreen", &app.fullscreen,
                    .height = ROW_H, .font_size = 18)) {
                    add_log("Fullscreen %s", app.fullscreen ? "ON" : "OFF");
                }

                if (wlx_checkbox(ctx, "Show FPS", &app.show_fps,
                    .height = ROW_H, .font_size = 18)) {
                    add_log("Show FPS %s", app.show_fps ? "ON" : "OFF");
                }

                if (wlx_checkbox(ctx, "Dark mode", &app.dark_mode,
                    .height = ROW_H, .font_size = 18)) {
                    add_log("Dark mode %s", app.dark_mode ? "ON" : "OFF");
                }

                wlx_textbox(ctx, "Input Fields",
                    .height = ROW_H, .font_size = 22,
                    .back_color = (Color){30, 30, 40, 255}, .align = WLX_CENTER);

                wlx_inputbox(ctx, "Name:", app.name, sizeof(app.name),
                    .height = ROW_H, .font_size = 16);

                wlx_inputbox(ctx, "Search:", app.search, sizeof(app.search),
                    .height = ROW_H, .font_size = 16);

                wlx_inputbox(ctx, "Note:", app.note, sizeof(app.note),
                    .height = ROW_H, .font_size = 16);

                if (wlx_button(ctx, "Clear All Inputs",
                    .height = ROW_H, .font_size = 16,
                    .back_color = (Color){50, 30, 30, 255}, .align = WLX_CENTER)) {
                    app.name[0] = '\0';
                    app.search[0] = '\0';
                    app.note[0] = '\0';
                    add_log("All input fields cleared");
                }

            wlx_layout_end(ctx);
        wlx_scroll_panel_end(ctx);
        #undef ROW_H

        // ---- RIGHT column: NESTED scroll panels -------------------------
        wlx_layout_begin(ctx, 2, WLX_VERT);

            // ---- Outer scroll panel (event log) -------------------------
            wlx_textbox(ctx, "Event Log (outer scroll panel)",
                .height = 30, .font_size = 18,
                .back_color = (Color){35, 25, 25, 255}, .align = WLX_CENTER);

            wlx_scroll_panel_begin(ctx, -1,
                .back_color = (Color){20, 20, 24, 255},
                .scrollbar_color = (Color){70, 40, 40, 255}
            );
                wlx_layout_begin_auto(ctx, WLX_VERT, 28);
                    if (app.log_count == 0) {
                        wlx_textbox(ctx, "  No events yet...",
                            .height = 28, .font_size = 14,
                            .back_color = (Color){20, 20, 24, 255}, .align = WLX_LEFT);
                    }
                    for (int i = 0; i < app.log_count; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        Color row_bg = (i % 2 == 0)
                            ? (Color){25, 25, 30, 255}
                            : (Color){30, 30, 36, 255};
                        wlx_textbox(ctx, app.log_messages[i],
                            .height = 28, .font_size = 14,
                            .back_color = row_bg, .align = WLX_LEFT);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);
            wlx_scroll_panel_end(ctx);

        wlx_layout_end(ctx);

    wlx_layout_end(ctx);
}

// --- Tab 1: Settings -------------------------------------------------------
static void render_settings_tab(WLX_Context *ctx, int layout_span) {
    wlx_layout_begin(ctx, 2, WLX_HORZ, .span = layout_span);

        // ---- LEFT: sliders in a scroll panel ----------------------------
        #define S_ROW 40
        wlx_scroll_panel_begin(ctx, -1,
            .back_color = (Color){22, 26, 22, 255},
            .scrollbar_color = (Color){50, 70, 50, 255}
        );
            wlx_layout_begin(ctx, 11, WLX_VERT);

                wlx_textbox(ctx, "Audio & Display",
                    .height = S_ROW, .font_size = 22,
                    .back_color = (Color){30, 40, 30, 255}, .align = WLX_CENTER);

                wlx_slider(ctx, "Volume   ", &app.volume,
                    .height = S_ROW, .min_value = 0.0f, .max_value = 1.0f,
                    .font_size = 18);

                wlx_slider(ctx, "Bright   ", &app.brightness,
                    .height = S_ROW, .min_value = 0.0f, .max_value = 1.0f,
                    .font_size = 18);

                wlx_slider(ctx, "Font Scl ", &app.font_scale,
                    .height = S_ROW, .min_value = 0.5f, .max_value = 2.0f,
                    .font_size = 18);

                wlx_textbox(ctx, "Color Mixer",
                    .height = S_ROW, .font_size = 22,
                    .back_color = (Color){30, 40, 30, 255}, .align = WLX_CENTER);

                wlx_slider(ctx, "Red      ", &app.r,
                    .height = S_ROW, .min_value = 0.0f, .max_value = 1.0f,
                    .thumb_color = RED, .font_size = 18);

                wlx_slider(ctx, "Green    ", &app.g,
                    .height = S_ROW, .min_value = 0.0f, .max_value = 1.0f,
                    .thumb_color = GREEN, .font_size = 18);

                wlx_slider(ctx, "Blue     ", &app.b,
                    .height = S_ROW, .min_value = 0.0f, .max_value = 1.0f,
                    .thumb_color = BLUE, .font_size = 18);

                // Color preview
                {
                    Color preview = {
                        (unsigned char)(app.r * 255),
                        (unsigned char)(app.g * 255),
                        (unsigned char)(app.b * 255),
                        (unsigned char)(app.brightness * 255),
                    };
                    wlx_widget(ctx, .widget_align = WLX_CENTER, .width = -1, .height = S_ROW, .color = preview);
                }

                char color_hex[64];
                snprintf(color_hex, sizeof(color_hex), "#%02X%02X%02X",
                    (int)(app.r * 255), (int)(app.g * 255), (int)(app.b * 255));
                wlx_textbox(ctx, color_hex,
                    .height = S_ROW, .font_size = 16,
                    .back_color = (Color){22, 26, 22, 255}, .align = WLX_CENTER);

                if (wlx_button(ctx, "Reset Colors",
                    .height = S_ROW, .font_size = 16,
                    .back_color = (Color){50, 30, 30, 255}, .align = WLX_CENTER)) {
                    app.r = 0.2f; app.g = 0.5f; app.b = 0.8f;
                    app.brightness = 1.0f;
                    add_log("Colors reset to defaults");
                }

            wlx_layout_end(ctx);
        wlx_scroll_panel_end(ctx);
        #undef S_ROW

        // ---- RIGHT: nested scroll panel with settings summary -----------
        #define R_ROW 40
        wlx_scroll_panel_begin(ctx, -1);
            wlx_layout_begin(ctx, 10, WLX_VERT);

                wlx_textbox(ctx, "Quick Notes",
                    .height = R_ROW, .font_size = 22,
                    .align = WLX_CENTER);

                wlx_inputbox(ctx, "Note:", app.note, sizeof(app.note),
                    .height = R_ROW, .font_size = 16, .wrap = true);

                wlx_textbox(ctx, "Current Settings",
                    .height = R_ROW, .font_size = 20,
                    .align = WLX_CENTER);

                {
                    char buf[128];
                    snprintf(buf, sizeof(buf), "  Volume: %.0f%%", app.volume * 100);
                    wlx_textbox(ctx, buf, .height = R_ROW, .font_size = 16,
                        .align = WLX_LEFT);
                }
                {
                    char buf[128];
                    snprintf(buf, sizeof(buf), "  Brightness: %.0f%%", app.brightness * 100);
                    wlx_textbox(ctx, buf, .height = R_ROW, .font_size = 16,
                        .align = WLX_LEFT);
                }
                {
                    char buf[128];
                    snprintf(buf, sizeof(buf), "  Font Scale: %.1fx", app.font_scale);
                    wlx_textbox(ctx, buf, .height = R_ROW, .font_size = 16,
                        .align = WLX_LEFT);
                }
                {
                    char buf[128];
                    snprintf(buf, sizeof(buf), "  Audio: %s | VSync: %s",
                        app.enable_audio ? "ON" : "OFF",
                        app.enable_vsync ? "ON" : "OFF");
                    wlx_textbox(ctx, buf, .height = R_ROW, .font_size = 16,
                        .align = WLX_LEFT);
                }
                {
                    char buf[128];
                    snprintf(buf, sizeof(buf), "  Fullscreen: %s | FPS: %s",
                        app.fullscreen ? "ON" : "OFF",
                        app.show_fps ? "ON" : "OFF");
                    wlx_textbox(ctx, buf, .height = R_ROW, .font_size = 16,
                        .align = WLX_LEFT);
                }
                {
                    char buf[128];
                    snprintf(buf, sizeof(buf), "  Color: #%02X%02X%02X",
                        (int)(app.r * 255), (int)(app.g * 255), (int)(app.b * 255));
                    wlx_textbox(ctx, buf, .height = R_ROW, .font_size = 16,
                        .align = WLX_LEFT);
                }
                {
                    char buf[128];
                    snprintf(buf, sizeof(buf), "  Dark mode: %s", app.dark_mode ? "ON" : "OFF");
                    wlx_textbox(ctx, buf, .height = R_ROW, .font_size = 16,
                        .align = WLX_LEFT);
                }

            wlx_layout_end(ctx);
        wlx_scroll_panel_end(ctx);
        #undef R_ROW

    wlx_layout_end(ctx);
}

// --- Tab 2: Dynamic (wlx_push_id / wlx_pop_id demo) -------------------------
static void render_dynamic_tab(WLX_Context *ctx, int layout_span) {
    #define D_ROW 80
    wlx_scroll_panel_begin(ctx, -1,
        .back_color = (Color){22, 24, 30, 255},
        .scrollbar_color = (Color){55, 60, 70, 255}, .span = layout_span
    );
        // Header + one section per panel
        wlx_layout_begin(ctx, 1 + PANEL_COUNT * 5, WLX_VERT);

            wlx_textbox(ctx, "Dynamic panels (wlx_push_id / wlx_pop_id)",
                .height = D_ROW, .font_size = 22,
                .back_color = (Color){35, 30, 45, 255}, .align = WLX_CENTER);

            // Each iteration of this loop creates stateful widgets
            // (wlx_inputbox, wlx_slider, wlx_checkbox, scroll_panel).
            // wlx_push_id(ctx, i) ensures every wlx_widget inside gets a
            // unique, frame-stable ID even though the source lines are
            // identical across iterations.
            for (int i = 0; i < PANEL_COUNT; i++) {
                wlx_push_id(ctx, (size_t)i);

                    Color section_bg = (i % 2 == 0)
                        ? (Color){28, 28, 36, 255}
                        : (Color){32, 32, 40, 255};

                    char header[64];
                    snprintf(header, sizeof(header), "Panel %d", i + 1);
                    wlx_textbox(ctx, header,
                        .height = D_ROW, .font_size = 20,
                        .back_color = section_bg, .align = WLX_LEFT);

                    wlx_inputbox(ctx, "Name:", app.panel_names[i], sizeof(app.panel_names[i]),
                        .height = D_ROW, .font_size = 16);

                    wlx_slider(ctx, "Value ", &app.panel_sliders[i],
                        .height = D_ROW, .font_size = 16,
                        .min_value = 0.0f, .max_value = 1.0f);

                    if (wlx_checkbox(ctx, "Enabled", &app.panel_toggles[i],
                        .height = D_ROW, .font_size = 16)) {
                        add_log("Panel %d toggled %s", i + 1,
                            app.panel_toggles[i] ? "ON" : "OFF");
                    }

                wlx_pop_id(ctx);
            }

        wlx_layout_end(ctx);
    wlx_scroll_panel_end(ctx);
    #undef D_ROW
}

// --- Tab 3: About ----------------------------------------------------------
static void render_about_tab(WLX_Context *ctx, int layout_span) {
    #define A_ROW 32
    wlx_scroll_panel_begin(ctx, -1, .span = layout_span);
        wlx_layout_begin(ctx, 16, WLX_VERT);

            wlx_textbox(ctx, "Wollix Library - Demo",
                .height = A_ROW, .font_size = 28,
                .align = WLX_CENTER);

            wlx_textbox(ctx, "A lightweight immediate-mode UI layout library built on raylib.",
                .height = A_ROW, .font_size = 18,
                .align = WLX_CENTER);

            wlx_textbox(ctx, "Supported Widgets:",
                .height = A_ROW, .font_size = 22,
                .align = WLX_LEFT);

            wlx_textbox(ctx, "  - Buttons (with hover highlight)",
                .height = A_ROW, .font_size = 16,
                .align = WLX_LEFT);

            wlx_textbox(ctx, "  - Checkboxes (toggle states)",
                .height = A_ROW, .font_size = 16,
                .align = WLX_LEFT);

            wlx_textbox(ctx, "  - Text boxes (static labels)",
                .height = A_ROW, .font_size = 16,
                .align = WLX_LEFT);

            wlx_textbox(ctx, "  - Input boxes (editable text fields)",
                .height = A_ROW, .font_size = 16,
                .align = WLX_LEFT);

            wlx_textbox(ctx, "  - Sliders (float value adjustment)",
                .height = A_ROW, .font_size = 16,
                .align = WLX_LEFT);

            wlx_textbox(ctx, "  - Scroll panels (with nesting support)",
                .height = A_ROW, .font_size = 16,
                .align = WLX_LEFT);

            wlx_textbox(ctx, "  - Colored widgets (rectangles)",
                .height = A_ROW, .font_size = 16,
                .align = WLX_LEFT);

            wlx_textbox(ctx, "Credits:",
                .height = A_ROW, .font_size = 22,
                .align = WLX_LEFT);

            wlx_textbox(ctx, "  Wollix library by Dainis",
                .height = A_ROW, .font_size = 16,
                .align = WLX_LEFT);

            wlx_textbox(ctx, "  Built with raylib - https://raylib.com",
                .height = A_ROW, .font_size = 16,
                .align = WLX_LEFT);

            wlx_textbox(ctx, "  Immediate mode UI paradigm",
                .height = A_ROW, .font_size = 16,
                .align = WLX_LEFT);

            wlx_textbox(ctx, "  Dynamic array macros from Alexey Kutepov",
                .height = A_ROW, .font_size = 16,
                .align = WLX_LEFT);

        wlx_layout_end(ctx);
    wlx_scroll_panel_end(ctx);
    #undef A_ROW
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(void) {
    printf("Wollix - Full Widget Demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix - Full Widget Demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    // Seed some initial log entries
    add_log("Application started");
    add_log("Window size: %dx%d", WINDOW_WIDTH, WINDOW_HEIGHT);
    strcpy(app.name, "Demo User");

    // Seed dynamic panel names
    for (int i = 0; i < PANEL_COUNT; i++) {
        snprintf(app.panel_names[i], sizeof(app.panel_names[i]), "Item %d", i + 1);
    }

    while (!WindowShouldClose()) {
        float w = GetRenderWidth();
        float h = GetRenderHeight();
        WLX_Rect r = {.x = 0, .y = 0, .w = w, .h = h};

        // Apply theme based on dark_mode toggle
        ctx->theme = app.dark_mode ? &wlx_theme_dark : &wlx_theme_light;

        wlx_begin(ctx, r, wlx_process_raylib_input);
            BeginDrawing();
                ClearBackground((Color){ctx->theme->background.r, ctx->theme->background.g, ctx->theme->background.b, ctx->theme->background.a});

                // Root layout: title bar + tab bar + content
                wlx_layout_begin(ctx, 15, WLX_VERT);

                    // ---- Title bar --------------------------------------
                        wlx_layout_begin(ctx, 1, WLX_HORZ, .span = 2);
                        wlx_textbox(ctx, "Wollix - Widget Demo",
                            .height = 44, .font_size = 26,
                            .back_color = (Color){28, 28, 38, 255},
                            .align = WLX_CENTER);
                    wlx_layout_end(ctx);

                    // ---- Tab bar ----------------------------------------
                        wlx_layout_begin(ctx, 4, WLX_HORZ, .span = 1);
                        Color tab_colors[4];
                        const char *tab_labels[4] = { "Controls", "Settings", "Dynamic", "About" };
                        for (int t = 0; t < 4; t++) {
                            wlx_push_id(ctx, (size_t)t);
                            tab_colors[t] = (t == app.active_tab)
                                ? (Color){50, 50, 70, 255}
                                : (Color){30, 30, 40, 255};
                            if (wlx_button(ctx, tab_labels[t],
                                .height = -1, .font_size = 18,
                                .back_color = tab_colors[t], .align = WLX_CENTER)) {
                                if (app.active_tab != t) {
                                    app.active_tab = t;
                                    add_log("Switched to tab: %s", tab_labels[t]);
                                }
                            }
                            wlx_pop_id(ctx);
                        }
                    wlx_layout_end(ctx);

                    // ---- Tab content ------------------------------------
                    switch (app.active_tab) {
                        case 0: render_controls_tab(ctx, 12); break;
                        case 1: render_settings_tab(ctx, 12); break;
                        case 2: render_dynamic_tab(ctx, 12);  break;
                        case 3: render_about_tab(ctx, 12);    break;
                    }

                wlx_layout_end(ctx);

                // FPS overlay
                if (app.show_fps) {
                    DrawFPS(8, 8);
                }

            EndDrawing();
        wlx_end(ctx);
    }

    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
