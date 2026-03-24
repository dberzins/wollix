// gallery.c — Widget gallery demo for wollix
// Showcases every widget, layout mode, and theming option with live tweaking.
// Build: make gallery

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_raylib.h"

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 800
#define TARGET_FPS    60

#define ROW_H 40

// ---------------------------------------------------------------------------
// Section forward declarations
// ---------------------------------------------------------------------------
typedef struct Gallery_State Gallery_State;
typedef void (*SectionFn)(WLX_Context *ctx, Gallery_State *g);

typedef struct {
    const char *name;
    SectionFn   render;
} Section;

static void section_label(WLX_Context *ctx, Gallery_State *g);
static void section_button(WLX_Context *ctx, Gallery_State *g);
static void section_checkbox(WLX_Context *ctx, Gallery_State *g);
static void section_slider(WLX_Context *ctx, Gallery_State *g);
static void section_inputbox(WLX_Context *ctx, Gallery_State *g);
static void section_scroll_panel(WLX_Context *ctx, Gallery_State *g);
static void section_widget(WLX_Context *ctx, Gallery_State *g);
static void section_layout_linear(WLX_Context *ctx, Gallery_State *g);
static void section_layout_grid(WLX_Context *ctx, Gallery_State *g);
static void section_layout_flex(WLX_Context *ctx, Gallery_State *g);
static void section_theming(WLX_Context *ctx, Gallery_State *g);
static void section_opacity(WLX_Context *ctx, Gallery_State *g);
static void section_id_stack(WLX_Context *ctx, Gallery_State *g);

static const Section sections[] = {
    { "Label",        section_label },
    { "Button",         section_button },
    { "Checkbox",       section_checkbox },
    { "Slider",         section_slider },
    { "Input Box",      section_inputbox },
    { "Scroll Panel",   section_scroll_panel },
    { "Widget",         section_widget },
    { "Linear Layout",  section_layout_linear },
    { "Grid Layout",    section_layout_grid },
    { "Flex & Sizing",  section_layout_flex },
    { "Theming",        section_theming },
    { "Opacity",        section_opacity },
    { "ID Stack",       section_id_stack },
};
#define SECTION_COUNT ((int)(sizeof(sections) / sizeof(sections[0])))

// ---------------------------------------------------------------------------
// Application state
// ---------------------------------------------------------------------------
struct Gallery_State {
    int active_section;
    bool dark_mode;

    // Label
    float label_font_size;
    bool  label_boxed;
    float label_opacity;

    // Button
    float button_font_size;
    float button_height;
    float button_r, button_g, button_b;
    int   button_click_count;

    // Checkbox
    bool  checkboxes[6];
    float checkbox_font_size;
    bool  checkbox_boxed;
    bool  checkbox_tex_val;

    // Slider
    float slider_demo_value;
    float slider_int_value;
    float color_r, color_g, color_b;
    float slider_track_height;
    float slider_thumb_width;
    bool  slider_show_label;
    float slider_min, slider_max;

    // Input Box
    char  inputs[4][128];
    float input_font_size;
    float input_height;
    float input_focus_r, input_focus_g, input_focus_b;

    // Scroll Panel
    float scroll_sb_width;
    float scroll_wheel_speed;
    bool  scroll_show_scrollbar;

    // Linear layouts
    float layout_slot_count;
    float layout_padding;

    // Grid
    float grid_rows;
    float grid_cols;

    // Flex
    float flex_weight_a, flex_weight_b;
    float flex_min, flex_max;

    // Theme customization
    WLX_Theme custom_theme;
    float theme_bg_r, theme_bg_g, theme_bg_b;
    float theme_fg_r, theme_fg_g, theme_fg_b;
    float theme_surface_r, theme_surface_g, theme_surface_b;
    float theme_accent_r, theme_accent_g, theme_accent_b;
    float theme_hover_brightness;
    float theme_padding;
    float theme_roundness;

    // Opacity
    float opacity_control;

    // Dynamic widgets
    float dynamic_panel_count;
    float dynamic_sliders[10];
    bool  dynamic_toggles[10];
    char  dynamic_names[10][64];
};

static Gallery_State g = {
    .active_section = 0,
    .dark_mode = true,

    .label_font_size = 18.0f,
    .label_boxed = false,
    .label_opacity = 1.0f,

    .button_font_size = 18.0f,
    .button_height = 40.0f,
    .button_r = 0.24f, .button_g = 0.12f, .button_b = 0.12f,
    .button_click_count = 0,

    .checkboxes = { true, false, true, false, false, false },
    .checkbox_font_size = 18.0f,
    .checkbox_boxed = false,
    .checkbox_tex_val = false,

    .slider_demo_value = 0.5f,
    .slider_int_value = 50.0f,
    .color_r = 0.2f, .color_g = 0.5f, .color_b = 0.8f,
    .slider_track_height = 6.0f,
    .slider_thumb_width = 14.0f,
    .slider_show_label = true,
    .slider_min = 0.0f, .slider_max = 1.0f,

    .inputs = { "Hello!", "", "", "" },
    .input_font_size = 16.0f,
    .input_height = 40.0f,
    .input_focus_r = 0.0f, .input_focus_g = 0.47f, .input_focus_b = 1.0f,

    .scroll_sb_width = 10.0f,
    .scroll_wheel_speed = 20.0f,
    .scroll_show_scrollbar = true,

    .layout_slot_count = 4.0f,
    .layout_padding = 4.0f,

    .grid_rows = 3.0f,
    .grid_cols = 4.0f,

    .flex_weight_a = 1.0f, .flex_weight_b = 3.0f,
    .flex_min = 100.0f, .flex_max = 400.0f,

    .theme_bg_r = 0.07f, .theme_bg_g = 0.07f, .theme_bg_b = 0.07f,
    .theme_fg_r = 0.88f, .theme_fg_g = 0.88f, .theme_fg_b = 0.88f,
    .theme_surface_r = 0.16f, .theme_surface_g = 0.16f, .theme_surface_b = 0.16f,
    .theme_accent_r = 0.0f, .theme_accent_g = 0.47f, .theme_accent_b = 1.0f,
    .theme_hover_brightness = 0.75f,
    .theme_padding = 4.0f,
    .theme_roundness = 0.0f,

    .opacity_control = 0.5f,

    .dynamic_panel_count = 3.0f,
    .dynamic_sliders = { 0.2f, 0.4f, 0.6f, 0.8f, 1.0f, 0.3f, 0.5f, 0.7f, 0.9f, 0.1f },
    .dynamic_toggles = { true, false, true, false, true, false, true, false, true, false },
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static Color to_color(float r01, float g01, float b01, float a) {
    return (Color){
        (unsigned char)(r01 * 255),
        (unsigned char)(g01 * 255),
        (unsigned char)(b01 * 255),
        (unsigned char)(a * 255),
    };
}

// Derive a tinted heading color from the current theme background.
// In dark themes the tint is added; in light themes it is subtracted.
static Color heading_color(const WLX_Theme *t, int tr, int tg, int tb) {
    int lum = ((int)t->background.r + t->background.g + t->background.b) / 3;
    int s = (lum < 128) ? 1 : -1;
    int r = (int)t->background.r + s * tr;
    int g = (int)t->background.g + s * tg;
    int b = (int)t->background.b + s * tb;
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
}

// Section heading macros — colors adapt to the active theme
#define SECTION_HEADING(ctx, title) \
    wlx_label(ctx, title, \
        .font_size = 26, .align = WLX_CENTER, .height = 48, \
        .boxed = true, .back_color = heading_color(ctx->theme, 17, 12, 32))

#define SUB_HEADING(ctx, title) \
    wlx_label(ctx, title, \
        .font_size = 20, .align = WLX_LEFT, .height = 36, \
        .boxed = true, .back_color = heading_color(ctx->theme, 12, 12, 20))

#define OPTIONS_HEADING(ctx) \
    wlx_label(ctx, "Options", \
        .font_size = 20, .align = WLX_LEFT, .height = 36, \
        .boxed = true, .back_color = heading_color(ctx->theme, 20, 12, 12))

// ---------------------------------------------------------------------------
// Section: Label
// ---------------------------------------------------------------------------
static void section_label(WLX_Context *ctx, Gallery_State *st) {
    int fs = (int)st->label_font_size;

    wlx_layout_begin(ctx, 11, WLX_VERT);

    SECTION_HEADING(ctx, "Label");

    wlx_label(ctx, "A simple static label with default options.",
        .font_size = fs, .height = ROW_H);

    wlx_label(ctx, "Centered Heading (boxed)",
        .font_size = fs + 4, .align = WLX_CENTER, .height = 48,
        .boxed = st->label_boxed,
        .back_color = heading_color(ctx->theme, 22, 22, 42),
        .opacity = st->label_opacity);

    wlx_label(ctx, "Right-aligned with custom color",
        .font_size = fs, .align = WLX_RIGHT, .height = ROW_H,
        .front_color = (Color){100, 200, 255, 255},
        .opacity = st->label_opacity);

    wlx_label(ctx, "This is a longer paragraph of text to demonstrate word wrapping behavior. "
        "When the text exceeds the available width, it should wrap to the next line automatically. "
        "The label widget supports this via the .wrap option which is true by default.",
        .font_size = fs, .height = 80, .wrap = true,
        .opacity = st->label_opacity);

    SUB_HEADING(ctx, "Alignment Showcase");

    // Show alignments in a 3x2 grid
    wlx_grid_begin(ctx, 2, 3, .padding = 2);
        const char *align_names[] = { "TOP_LEFT", "TOP_CENTER", "TOP_RIGHT",
                                       "BOTTOM_LEFT", "BOTTOM_CENTER", "BOTTOM_RIGHT" };
        WLX_Align aligns[] = { WLX_TOP_LEFT, WLX_TOP_CENTER, WLX_TOP_RIGHT,
                                WLX_BOTTOM_LEFT, WLX_BOTTOM_CENTER, WLX_BOTTOM_RIGHT };
        for (int i = 0; i < 6; i++) {
            wlx_push_id(ctx, (size_t)i);
            wlx_label(ctx, align_names[i],
                .font_size = 14, .widget_align = aligns[i],
                .back_color = heading_color(ctx->theme, 12, 17, 27), .boxed = true);
            wlx_pop_id(ctx);
        }
    wlx_layout_end(ctx);

    OPTIONS_HEADING(ctx);

    wlx_slider(ctx, "Font Size  ", &st->label_font_size,
        .height = ROW_H, .min_value = 8.0f, .max_value = 40.0f, .font_size = 16);
    wlx_checkbox(ctx, "Boxed", &st->label_boxed,
        .height = ROW_H, .font_size = 16);
    wlx_slider(ctx, "Opacity    ", &st->label_opacity,
        .height = ROW_H, .min_value = 0.1f, .max_value = 1.0f, .font_size = 16);

    wlx_layout_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Button
// ---------------------------------------------------------------------------
static void section_button(WLX_Context *ctx, Gallery_State *st) {
    int fs = (int)st->button_font_size;
    int bh = (int)st->button_height;
    Color bc = to_color(st->button_r, st->button_g, st->button_b, 1.0f);

    wlx_layout_begin(ctx, 14, WLX_VERT);

    SECTION_HEADING(ctx, "Button");

    wlx_label(ctx, "Clickable button. Returns true on the frame clicked.",
        .font_size = 16, .height = 30);

    if (wlx_button(ctx, "Default Button",
        .height = bh, .font_size = fs, .align = WLX_CENTER)) {
        st->button_click_count++;
    }

    if (wlx_button(ctx, "Custom Color Button",
        .height = bh, .font_size = fs, .align = WLX_CENTER,
        .back_color = bc)) {
        st->button_click_count++;
    }

    if (wlx_button(ctx, "Boxed Button",
        .height = bh, .font_size = fs, .align = WLX_CENTER,
        .boxed = true)) {
        st->button_click_count++;
    }

    if (wlx_button(ctx, "Center Aligned (200x50)",
        .widget_align = WLX_CENTER, .width = 200, .height = 50,
        .font_size = fs, .align = WLX_CENTER)) {
        st->button_click_count++;
    }

    char click_buf[64];
    snprintf(click_buf, sizeof(click_buf), "Click count: %d", st->button_click_count);
    wlx_label(ctx, click_buf,
        .font_size = 18, .height = ROW_H, .align = WLX_CENTER);

    if (wlx_button(ctx, "Reset Counter",
        .height = 36, .font_size = 16, .align = WLX_CENTER,
        .back_color = heading_color(ctx->theme, 42, 12, 12))) {
        st->button_click_count = 0;
    }

    OPTIONS_HEADING(ctx);

    wlx_slider(ctx, "Font Size  ", &st->button_font_size,
        .height = ROW_H, .min_value = 10.0f, .max_value = 36.0f, .font_size = 16);
    wlx_slider(ctx, "Height     ", &st->button_height,
        .height = ROW_H, .min_value = 24.0f, .max_value = 80.0f, .font_size = 16);
    wlx_slider(ctx, "Red        ", &st->button_r,
        .height = ROW_H, .thumb_color = RED, .font_size = 16);
    wlx_slider(ctx, "Green      ", &st->button_g,
        .height = ROW_H, .thumb_color = GREEN, .font_size = 16);
    wlx_slider(ctx, "Blue       ", &st->button_b,
        .height = ROW_H, .thumb_color = BLUE, .font_size = 16);

    wlx_layout_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Checkbox
// ---------------------------------------------------------------------------
static void section_checkbox(WLX_Context *ctx, Gallery_State *st) {
    int fs = (int)st->checkbox_font_size;

    wlx_layout_begin(ctx, 13, WLX_VERT);

    SECTION_HEADING(ctx, "Checkbox");

    wlx_label(ctx, "Toggle checkbox with label. Returns true when state changes.",
        .font_size = 16, .height = 30);

    wlx_checkbox(ctx, "Enable audio", &st->checkboxes[0],
        .height = ROW_H, .font_size = fs, .boxed = st->checkbox_boxed);
    wlx_checkbox(ctx, "Enable V-Sync", &st->checkboxes[1],
        .height = ROW_H, .font_size = fs, .boxed = st->checkbox_boxed);
    wlx_checkbox(ctx, "Fullscreen mode", &st->checkboxes[2],
        .height = ROW_H, .font_size = fs, .boxed = st->checkbox_boxed);

    SUB_HEADING(ctx, "Custom Colors");

    wlx_checkbox(ctx, "Green check mark", &st->checkboxes[3],
        .height = ROW_H, .font_size = fs,
        .check_color = (Color){0, 200, 0, 255});
    wlx_checkbox(ctx, "Red border", &st->checkboxes[4],
        .height = ROW_H, .font_size = fs,
        .border_color = (Color){200, 0, 0, 255});

    SUB_HEADING(ctx, "Texture Checkbox (checkbox_tex)");

    // Create simple runtime textures for checkbox_tex demo
    {
        static bool tex_init = false;
        static Texture2D tex_on, tex_off;
        if (!tex_init) {
            Image img_on = GenImageColor(24, 24, GREEN);
            Image img_off = GenImageColor(24, 24, (Color){80, 80, 80, 255});
            tex_on = LoadTextureFromImage(img_on);
            tex_off = LoadTextureFromImage(img_off);
            UnloadImage(img_on);
            UnloadImage(img_off);
            tex_init = true;
        }

        wlx_checkbox_tex(ctx, "Texture toggle", &st->checkbox_tex_val,
            .height = ROW_H, .font_size = fs,
            .tex_checked = tex_on,
            .tex_unchecked = tex_off);
    }

    OPTIONS_HEADING(ctx);

    wlx_slider(ctx, "Font Size  ", &st->checkbox_font_size,
        .height = ROW_H, .min_value = 12.0f, .max_value = 30.0f, .font_size = 16);
    wlx_checkbox(ctx, "Boxed", &st->checkbox_boxed,
        .height = ROW_H, .font_size = 16);

    wlx_layout_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Slider
// ---------------------------------------------------------------------------
static void section_slider(WLX_Context *ctx, Gallery_State *st) {
    wlx_layout_begin(ctx, 19, WLX_VERT);

    SECTION_HEADING(ctx, "Slider");

    wlx_label(ctx, "Horizontal slider for float values. Click/drag the thumb or track.",
        .font_size = 16, .height = 30);

    SUB_HEADING(ctx, "Default (0 - 1)");
    wlx_slider(ctx, "Value      ", &st->slider_demo_value,
        .height = ROW_H, .font_size = 18,
        .min_value = st->slider_min, .max_value = st->slider_max,
        .show_label = st->slider_show_label);

    SUB_HEADING(ctx, "Integer Range (0 - 100)");
    wlx_slider(ctx, "Percent    ", &st->slider_int_value,
        .height = ROW_H, .font_size = 18,
        .min_value = 0.0f, .max_value = 100.0f,
        .show_label = st->slider_show_label);

    SUB_HEADING(ctx, "Color Mixer");
    wlx_slider(ctx, "Red        ", &st->color_r,
        .height = ROW_H, .font_size = 18,
        .thumb_color = RED);
    wlx_slider(ctx, "Green      ", &st->color_g,
        .height = ROW_H, .font_size = 18,
        .thumb_color = GREEN);
    wlx_slider(ctx, "Blue       ", &st->color_b,
        .height = ROW_H, .font_size = 18,
        .thumb_color = BLUE);
    {
        Color preview = to_color(st->color_r, st->color_g, st->color_b, 1.0f);
        wlx_widget(ctx,
            .widget_align = WLX_CENTER, .width = -1, .height = 40,
            .color = preview);
    }

    SUB_HEADING(ctx, "Custom Track & Thumb");
    {
        float dummy = 0.6f;
        wlx_slider(ctx, "Styled     ", &dummy,
            .height = ROW_H, .font_size = 18,
            .track_height = st->slider_track_height,
            .thumb_width = st->slider_thumb_width,
            .roundness = 0.5f, .rounded_segments = 4);
    }

    OPTIONS_HEADING(ctx);

    wlx_slider(ctx, "Min Value  ", &st->slider_min,
        .height = ROW_H, .min_value = 0.0f, .max_value = 0.5f, .font_size = 16);
    wlx_slider(ctx, "Max Value  ", &st->slider_max,
        .height = ROW_H, .min_value = 0.5f, .max_value = 2.0f, .font_size = 16);
    wlx_slider(ctx, "Track H    ", &st->slider_track_height,
        .height = ROW_H, .min_value = 2.0f, .max_value = 24.0f, .font_size = 16);
    wlx_slider(ctx, "Thumb W    ", &st->slider_thumb_width,
        .height = ROW_H, .min_value = 4.0f, .max_value = 40.0f, .font_size = 16);
    wlx_checkbox(ctx, "Show Label", &st->slider_show_label,
        .height = ROW_H, .font_size = 16);

    wlx_layout_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Input Box
// ---------------------------------------------------------------------------
static void section_inputbox(WLX_Context *ctx, Gallery_State *st) {
    int fs = (int)st->input_font_size;
    int ih = (int)st->input_height;
    Color fc = to_color(st->input_focus_r, st->input_focus_g, st->input_focus_b, 1.0f);

    wlx_layout_begin(ctx, 14, WLX_VERT);

    SECTION_HEADING(ctx, "Input Box");

    wlx_label(ctx, "Single-line text input. Click to focus, type to edit, Enter/Esc to unfocus.",
        .font_size = 16, .height = 30);

    wlx_inputbox(ctx, "Name:  ", st->inputs[0], sizeof(st->inputs[0]),
        .height = ih, .font_size = fs);
    wlx_inputbox(ctx, "Email: ", st->inputs[1], sizeof(st->inputs[1]),
        .height = ih, .font_size = fs);
    wlx_inputbox(ctx, "Note:  ", st->inputs[2], sizeof(st->inputs[2]),
        .height = ih, .font_size = fs);

    SUB_HEADING(ctx, "Custom Focus Color");
    wlx_inputbox(ctx, "Styled:", st->inputs[3], sizeof(st->inputs[3]),
        .height = ih, .font_size = fs,
        .border_focus_color = fc,
        .cursor_color = fc);

    if (wlx_button(ctx, "Clear All Inputs",
        .height = 36, .font_size = 16, .align = WLX_CENTER,
        .back_color = heading_color(ctx->theme, 42, 12, 12))) {
        for (int i = 0; i < 4; i++) st->inputs[i][0] = '\0';
    }

    OPTIONS_HEADING(ctx);

    wlx_slider(ctx, "Font Size  ", &st->input_font_size,
        .height = ROW_H, .min_value = 10.0f, .max_value = 30.0f, .font_size = 16);
    wlx_slider(ctx, "Height     ", &st->input_height,
        .height = ROW_H, .min_value = 28.0f, .max_value = 60.0f, .font_size = 16);
    wlx_slider(ctx, "Focus R    ", &st->input_focus_r,
        .height = ROW_H, .thumb_color = RED, .font_size = 16);
    wlx_slider(ctx, "Focus G    ", &st->input_focus_g,
        .height = ROW_H, .thumb_color = GREEN, .font_size = 16);
    wlx_slider(ctx, "Focus B    ", &st->input_focus_b,
        .height = ROW_H, .thumb_color = BLUE, .font_size = 16);

    wlx_layout_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Scroll Panel
// ---------------------------------------------------------------------------
static void section_scroll_panel(WLX_Context *ctx, Gallery_State *st) {
    wlx_layout_begin(ctx, 10, WLX_VERT);

    SECTION_HEADING(ctx, "Scroll Panel");

    wlx_label(ctx, "Scrollable container with auto or fixed content height.",
        .font_size = 16, .height = 30);

    SUB_HEADING(ctx, "Auto-Height (20 items)");
    wlx_scroll_panel_begin(ctx, -1,
        .height = 200,
        .back_color = heading_color(ctx->theme, 4, 4, 10),
        .scrollbar_color = heading_color(ctx->theme, 37, 37, 47),
        .scrollbar_width = st->scroll_sb_width,
        .wheel_scroll_speed = st->scroll_wheel_speed,
        .show_scrollbar = st->scroll_show_scrollbar);
        wlx_layout_begin_auto(ctx, WLX_VERT, 30);
            for (int i = 0; i < 20; i++) {
                wlx_push_id(ctx, (size_t)i);
                char buf[32];
                snprintf(buf, sizeof(buf), "  Item %d", i + 1);
                Color row_bg = (i % 2 == 0)
                    ? heading_color(ctx->theme, 7, 7, 14)
                    : heading_color(ctx->theme, 12, 12, 20);
                wlx_label(ctx, buf,
                    .height = 30, .font_size = 15,
                    .back_color = row_bg, .align = WLX_LEFT);
                wlx_pop_id(ctx);
            }
        wlx_layout_end(ctx);
    wlx_scroll_panel_end(ctx);

    SUB_HEADING(ctx, "Nested Scroll Panels");
    wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 4);
        // Outer panel left
        wlx_scroll_panel_begin(ctx, -1,
            .height = 180,
            .back_color = heading_color(ctx->theme, 2, 6, 10),
            .scrollbar_color = heading_color(ctx->theme, 32, 42, 52));
            wlx_layout_begin_auto(ctx, WLX_VERT, 28);
                for (int i = 0; i < 15; i++) {
                    wlx_push_id(ctx, (size_t)i);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "  Outer %d", i + 1);
                    wlx_label(ctx, buf, .height = 28, .font_size = 14,
                        .back_color = heading_color(ctx->theme, 4, 8, 12));
                    wlx_pop_id(ctx);
                }
            wlx_layout_end(ctx);
        wlx_scroll_panel_end(ctx);

        // Inner panel right
        wlx_scroll_panel_begin(ctx, -1,
            .height = 180,
            .back_color = heading_color(ctx->theme, 6, 2, 10),
            .scrollbar_color = heading_color(ctx->theme, 42, 32, 52));
            wlx_layout_begin_auto(ctx, WLX_VERT, 28);
                for (int i = 0; i < 15; i++) {
                    wlx_push_id(ctx, (size_t)i);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "  Inner %d", i + 1);
                    wlx_label(ctx, buf, .height = 28, .font_size = 14,
                        .back_color = heading_color(ctx->theme, 8, 4, 12));
                    wlx_pop_id(ctx);
                }
            wlx_layout_end(ctx);
        wlx_scroll_panel_end(ctx);
    wlx_layout_end(ctx);

    OPTIONS_HEADING(ctx);

    wlx_slider(ctx, "Scrollbar W", &st->scroll_sb_width,
        .height = ROW_H, .min_value = 4.0f, .max_value = 30.0f, .font_size = 16);
    wlx_slider(ctx, "Wheel Speed", &st->scroll_wheel_speed,
        .height = ROW_H, .min_value = 5.0f, .max_value = 100.0f, .font_size = 16);
    wlx_checkbox(ctx, "Show Scrollbar", &st->scroll_show_scrollbar,
        .height = ROW_H, .font_size = 16);

    wlx_layout_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Widget (Colored Rect)
// ---------------------------------------------------------------------------
static void section_widget(WLX_Context *ctx, Gallery_State *st) {
    (void)st;

    wlx_layout_begin(ctx, 9, WLX_VERT);

    SECTION_HEADING(ctx, "Widget (Colored Rect)");

    wlx_label(ctx, "Low-level colored rectangle for swatches, spacers, and dividers.",
        .font_size = 16, .height = 30);

    SUB_HEADING(ctx, "Color Swatch");
    wlx_widget(ctx,
        .widget_align = WLX_CENTER, .width = 120, .height = 40,
        .color = (Color){0, 120, 255, 255});

    SUB_HEADING(ctx, "Divider / Spacer");
    wlx_widget(ctx,
        .widget_align = WLX_CENTER, .width = -1, .height = 2,
        .color = (Color){80, 80, 80, 255});

    SUB_HEADING(ctx, "Alignment Grid");
    wlx_label(ctx, "12 alignment values shown as 30x30 swatches inside grid cells:",
        .font_size = 14, .height = 24);

    wlx_grid_begin(ctx, 4, 3, .padding = 2);
        const WLX_Align all_aligns[] = {
            WLX_TOP_LEFT, WLX_TOP_CENTER, WLX_TOP_RIGHT,
            WLX_LEFT, WLX_CENTER, WLX_RIGHT,
            WLX_BOTTOM_LEFT, WLX_BOTTOM_CENTER, WLX_BOTTOM_RIGHT,
            WLX_ALIGN_NONE, WLX_TOP, WLX_BOTTOM,
        };
        Color swatch_colors[] = {
            {200, 60, 60, 255}, {60, 200, 60, 255}, {60, 60, 200, 255},
            {200, 200, 60, 255}, {200, 60, 200, 255}, {60, 200, 200, 255},
            {200, 120, 60, 255}, {60, 200, 120, 255}, {120, 60, 200, 255},
            {150, 150, 150, 255}, {200, 180, 60, 255}, {60, 180, 200, 255},
        };
        for (int i = 0; i < 12; i++) {
            wlx_push_id(ctx, (size_t)i);
            wlx_widget(ctx,
                .widget_align = all_aligns[i], .width = 30, .height = 30,
                .color = swatch_colors[i]);
            wlx_pop_id(ctx);
        }
    wlx_layout_end(ctx);

    wlx_layout_end(ctx); // section layout
}

// ---------------------------------------------------------------------------
// Section: Linear Layout
// ---------------------------------------------------------------------------
static void section_layout_linear(WLX_Context *ctx, Gallery_State *st) {
    int slots = (int)st->layout_slot_count;
    if (slots < 2) slots = 2;
    if (slots > 10) slots = 10;

    wlx_layout_begin(ctx, 12, WLX_VERT);

    SECTION_HEADING(ctx, "Linear Layouts");

    SUB_HEADING(ctx, "Horizontal (equal slots)");
    wlx_layout_begin(ctx, (size_t)slots, WLX_HORZ, .padding = st->layout_padding);
        for (int i = 0; i < slots; i++) {
            wlx_push_id(ctx, (size_t)i);
            Color c = { (unsigned char)(60 + i * 25), 60, (unsigned char)(200 - i * 15), 255 };
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", i);
            wlx_label(ctx, buf,
                .height = 50, .font_size = 18, .align = WLX_CENTER,
                .back_color = c, .boxed = true);
            wlx_pop_id(ctx);
        }
    wlx_layout_end(ctx);

    SUB_HEADING(ctx, "Vertical (equal slots)");
    wlx_layout_begin(ctx, (size_t)slots, WLX_VERT, .padding = st->layout_padding);
        for (int i = 0; i < slots; i++) {
            wlx_push_id(ctx, (size_t)i);
            Color c = { 60, (unsigned char)(60 + i * 25), (unsigned char)(200 - i * 15), 255 };
            char buf[16];
            snprintf(buf, sizeof(buf), "Row %d", i);
            wlx_label(ctx, buf,
                .height = 30, .font_size = 16, .align = WLX_LEFT,
                .back_color = c, .boxed = true);
            wlx_pop_id(ctx);
        }
    wlx_layout_end(ctx);

    SUB_HEADING(ctx, "Variable Sizes (PX + PCT + FLEX)");
    wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 2,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(100), WLX_SLOT_PCT(30), WLX_SLOT_FLEX(1) });
        wlx_label(ctx, "PX(100)",
            .height = 50, .font_size = 14, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 62, 22, 22), .boxed = true);
        wlx_label(ctx, "PCT(30)",
            .height = 50, .font_size = 14, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 22, 62, 22), .boxed = true);
        wlx_label(ctx, "FLEX(1)",
            .height = 50, .font_size = 14, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 22, 22, 62), .boxed = true);
    wlx_layout_end(ctx);

    SUB_HEADING(ctx, "Auto-Sizing Layout");
    wlx_layout_begin_auto(ctx, WLX_VERT, 32);
        for (int i = 0; i < 5; i++) {
            wlx_push_id(ctx, (size_t)i);
            char buf[32];
            snprintf(buf, sizeof(buf), "Auto item %d", i + 1);
            wlx_label(ctx, buf,
                .height = 32, .font_size = 15,
                .back_color = heading_color(ctx->theme, 12, 12, 24), .boxed = true);
            wlx_pop_id(ctx);
        }
    wlx_layout_end(ctx);

    OPTIONS_HEADING(ctx);

    wlx_slider(ctx, "Slot Count ", &st->layout_slot_count,
        .height = ROW_H, .min_value = 2.0f, .max_value = 10.0f, .font_size = 16);
    wlx_slider(ctx, "Padding    ", &st->layout_padding,
        .height = ROW_H, .min_value = 0.0f, .max_value = 20.0f, .font_size = 16);

    wlx_layout_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Grid Layout
// ---------------------------------------------------------------------------
static void section_layout_grid(WLX_Context *ctx, Gallery_State *st) {
    int rows = (int)st->grid_rows;
    int cols = (int)st->grid_cols;
    if (rows < 1) rows = 1;
    if (rows > 8) rows = 8;
    if (cols < 1) cols = 1;
    if (cols > 8) cols = 8;

    wlx_layout_begin(ctx, 12, WLX_VERT,
        .sizes = (WLX_Slot_Size[]){
            WLX_SLOT_PX(48),   // heading
            WLX_SLOT_PX(36),   // sub
            WLX_SLOT_PX(200),  // fixed grid
            WLX_SLOT_PX(36),   // sub
            WLX_SLOT_PX(130),  // auto grid
            WLX_SLOT_PX(36),   // sub
            WLX_SLOT_PX(130),  // auto tile grid
            WLX_SLOT_PX(36),   // sub
            WLX_SLOT_PX(180),  // cell positioning grid
            WLX_SLOT_PX(36),   // options heading
            WLX_SLOT_PX(40),   // rows slider
            WLX_SLOT_PX(40),   // cols slider
        });

    SECTION_HEADING(ctx, "Grid Layouts");

    SUB_HEADING(ctx, "Fixed Grid (wlx_grid_begin)");
    wlx_grid_begin(ctx, (size_t)rows, (size_t)cols, .padding = 2);
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                wlx_push_id(ctx, (size_t)(r * cols + c));
                int idx = r * cols + c;
                Color clr = {
                    (unsigned char)(40 + c * 40),
                    (unsigned char)(40 + r * 40),
                    (unsigned char)(120),
                    255
                };
                char buf[16];
                snprintf(buf, sizeof(buf), "%d,%d", r, c);
                wlx_label(ctx, buf,
                    .height = -1, .font_size = 14, .align = WLX_CENTER,
                    .back_color = clr, .boxed = true);
                (void)idx;
                wlx_pop_id(ctx);
            }
        }
    wlx_layout_end(ctx);

    SUB_HEADING(ctx, "Auto Grid (wlx_grid_begin_auto)");
    wlx_grid_begin_auto(ctx, 4, 40, .padding = 2);
        for (int i = 0; i < 12; i++) {
            wlx_push_id(ctx, (size_t)i);
            char buf[16];
            snprintf(buf, sizeof(buf), "Cell %d", i);
            Color clr = { (unsigned char)(60 + i * 15), 80, (unsigned char)(180 - i * 10), 255 };
            wlx_label(ctx, buf,
                .height = -1, .font_size = 13, .align = WLX_CENTER,
                .back_color = clr, .boxed = true);
            wlx_pop_id(ctx);
        }
    wlx_layout_end(ctx);

    SUB_HEADING(ctx, "Auto Tile Grid (wlx_grid_begin_auto_tile)");
    wlx_grid_begin_auto_tile(ctx, 100, 60, .padding = 2);
        for (int i = 0; i < 10; i++) {
            wlx_push_id(ctx, (size_t)i);
            char buf[16];
            snprintf(buf, sizeof(buf), "Tile %d", i);
            Color clr = { (unsigned char)(80 + i * 10), (unsigned char)(60 + i * 15), 120, 255 };
            wlx_label(ctx, buf,
                .height = -1, .font_size = 13, .align = WLX_CENTER,
                .back_color = clr, .boxed = true);
            wlx_pop_id(ctx);
        }
    wlx_layout_end(ctx);

    SUB_HEADING(ctx, "Grid Cell Positioning (wlx_grid_cell)");
    wlx_grid_begin(ctx, 3, 3, .padding = 2);
        wlx_grid_cell(ctx, 0, 0, .col_span = 2);
        wlx_label(ctx, "Span 2 cols",
            .height = -1, .font_size = 14, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 62, 22, 42), .boxed = true);

        wlx_grid_cell(ctx, 0, 2, .row_span = 2);
        wlx_label(ctx, "Span 2 rows",
            .height = -1, .font_size = 14, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 22, 62, 42), .boxed = true);

        wlx_grid_cell(ctx, 1, 0);
        wlx_label(ctx, "1,0",
            .height = -1, .font_size = 14, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 42, 42, 62), .boxed = true);

        wlx_grid_cell(ctx, 1, 1);
        wlx_label(ctx, "1,1",
            .height = -1, .font_size = 14, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 42, 62, 42), .boxed = true);

        wlx_grid_cell(ctx, 2, 0, .col_span = 3);
        wlx_label(ctx, "Full width (span 3 cols)",
            .height = -1, .font_size = 14, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 42, 42, 42), .boxed = true);
    wlx_layout_end(ctx);

    OPTIONS_HEADING(ctx);

    wlx_slider(ctx, "Rows       ", &st->grid_rows,
        .height = ROW_H, .min_value = 1.0f, .max_value = 8.0f, .font_size = 16);
    wlx_slider(ctx, "Cols       ", &st->grid_cols,
        .height = ROW_H, .min_value = 1.0f, .max_value = 8.0f, .font_size = 16);

    wlx_layout_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Flex & Sizing
// ---------------------------------------------------------------------------
static void section_layout_flex(WLX_Context *ctx, Gallery_State *st) {
    wlx_layout_begin(ctx, 17, WLX_VERT);

    SECTION_HEADING(ctx, "Flex & Sizing");

    SUB_HEADING(ctx, "Flex Weight Distribution");
    wlx_label(ctx, "Sidebar + Content pattern using FLEX weights:",
        .font_size = 14, .height = 24);

    {
        float wa = st->flex_weight_a;
        float wb = st->flex_weight_b;
        if (wa < 0.1f) wa = 0.1f;
        if (wb < 0.1f) wb = 0.1f;

        wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 2,
            .sizes = (WLX_Slot_Size[]){
                WLX_SLOT_FLEX(wa), WLX_SLOT_FLEX(wb) });
            char buf_a[32], buf_b[32];
            snprintf(buf_a, sizeof(buf_a), "FLEX(%.1f)", wa);
            snprintf(buf_b, sizeof(buf_b), "FLEX(%.1f)", wb);
            wlx_label(ctx, buf_a,
                .height = 60, .font_size = 16, .align = WLX_CENTER,
                .back_color = heading_color(ctx->theme, 42, 22, 62), .boxed = true);
            wlx_label(ctx, buf_b,
                .height = 60, .font_size = 16, .align = WLX_CENTER,
                .back_color = heading_color(ctx->theme, 22, 42, 62), .boxed = true);
        wlx_layout_end(ctx);
    }

    SUB_HEADING(ctx, "PX Fixed + AUTO Fill");
    wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 2,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(120), WLX_SLOT_AUTO, WLX_SLOT_PX(120) });
        wlx_label(ctx, "PX(120)",
            .height = 50, .font_size = 14, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 62, 32, 32), .boxed = true);
        wlx_label(ctx, "AUTO",
            .height = 50, .font_size = 14, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 32, 62, 32), .boxed = true);
        wlx_label(ctx, "PX(120)",
            .height = 50, .font_size = 14, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 32, 32, 62), .boxed = true);
    wlx_layout_end(ctx);

    SUB_HEADING(ctx, "Min/Max Constraints");
    {
        float lo = st->flex_min;
        float hi = st->flex_max;
        if (hi < lo) hi = lo;

        wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 2,
            .sizes = (WLX_Slot_Size[]){
                WLX_SLOT_FLEX_MINMAX(1, lo, hi), WLX_SLOT_FLEX(1) });
            char buf[64];
            snprintf(buf, sizeof(buf), "FLEX_MINMAX(1, %.0f, %.0f)", lo, hi);
            wlx_label(ctx, buf,
                .height = 50, .font_size = 13, .align = WLX_CENTER,
                .back_color = heading_color(ctx->theme, 52, 32, 52), .boxed = true);
            wlx_label(ctx, "FLEX(1)",
                .height = 50, .font_size = 13, .align = WLX_CENTER,
                .back_color = heading_color(ctx->theme, 32, 52, 52), .boxed = true);
        wlx_layout_end(ctx);
    }

    SUB_HEADING(ctx, "Widget-Level Min/Max");
    wlx_layout_begin(ctx, 1, WLX_HORZ);
        wlx_label(ctx, "min_w=100, max_w=300, centered",
            .widget_align = WLX_CENTER,
            .min_width = 100, .max_width = 300,
            .height = 40, .font_size = 14, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 42, 42, 62), .boxed = true);
    wlx_layout_end(ctx);

    SUB_HEADING(ctx, "Overflow");
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 10);
        wlx_label(ctx, "overflow=true, width=500 (exceeds slot if narrow)",
            .overflow = true, .width = 500,
            .height = 40, .font_size = 14, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 62, 42, 22), .boxed = true);
    wlx_layout_end(ctx);

    OPTIONS_HEADING(ctx);

    wlx_slider(ctx, "Weight A   ", &st->flex_weight_a,
        .height = ROW_H, .min_value = 0.1f, .max_value = 5.0f, .font_size = 16);
    wlx_slider(ctx, "Weight B   ", &st->flex_weight_b,
        .height = ROW_H, .min_value = 0.1f, .max_value = 5.0f, .font_size = 16);
    wlx_slider(ctx, "Min        ", &st->flex_min,
        .height = ROW_H, .min_value = 0.0f, .max_value = 400.0f, .font_size = 16);
    wlx_slider(ctx, "Max        ", &st->flex_max,
        .height = ROW_H, .min_value = 100.0f, .max_value = 800.0f, .font_size = 16);

    wlx_layout_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Theming
// ---------------------------------------------------------------------------

// Helper: render a row of sample widgets under the current ctx->theme
static void theme_sample_widgets(WLX_Context *ctx) {
    wlx_layout_begin(ctx, 5, WLX_VERT, .padding = 2);
        wlx_label(ctx, "Sample Label", .height = 30, .font_size = 16, .align = WLX_CENTER);
        {
            // temp state for demo buttons - they don't need to persist
            static bool t_check = true;
            static float t_slider = 0.5f;
            static char t_input[64] = "Edit me";
            wlx_button(ctx, "Sample Button", .height = 34, .font_size = 16, .align = WLX_CENTER);
            wlx_checkbox(ctx, "Sample Checkbox", &t_check, .height = 34, .font_size = 16);
            wlx_slider(ctx, "Sample  ", &t_slider, .height = 34, .font_size = 16);
            wlx_inputbox(ctx, "Input:", t_input, sizeof(t_input), .height = 34, .font_size = 16);
        }
    wlx_layout_end(ctx);
}

static void section_theming(WLX_Context *ctx, Gallery_State *st) {
    wlx_layout_begin(ctx, 28, WLX_VERT);

    SECTION_HEADING(ctx, "Theming");

    wlx_label(ctx, "Built-in dark/light themes and custom theme creation.",
        .font_size = 16, .height = 30);

    SUB_HEADING(ctx, "Side-by-Side: Dark vs Light");
    wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 4);
        // Dark theme panel
        {
            const WLX_Theme *saved = ctx->theme;
            ctx->theme = &wlx_theme_dark;
            wlx_push_id(ctx, 0);
            wlx_scroll_panel_begin(ctx, -1,
                .height = 220,
                .back_color = wlx_theme_dark.background);
                wlx_layout_begin(ctx, 2, WLX_VERT);
                    wlx_label(ctx, "Dark Theme", .height = 28, .font_size = 18, .align = WLX_CENTER);
                    theme_sample_widgets(ctx);
                wlx_layout_end(ctx);
            wlx_scroll_panel_end(ctx);
            wlx_pop_id(ctx);
            ctx->theme = saved;
        }

        // Light theme panel
        {
            const WLX_Theme *saved = ctx->theme;
            ctx->theme = &wlx_theme_light;
            wlx_push_id(ctx, 1);
            wlx_scroll_panel_begin(ctx, -1,
                .height = 220,
                .back_color = wlx_theme_light.background);
                wlx_layout_begin(ctx, 2, WLX_VERT);
                    wlx_label(ctx, "Light Theme", .height = 28, .font_size = 18, .align = WLX_CENTER);
                    theme_sample_widgets(ctx);
                wlx_layout_end(ctx);
            wlx_scroll_panel_end(ctx);
            wlx_pop_id(ctx);
            ctx->theme = saved;
        }
    wlx_layout_end(ctx);

    SUB_HEADING(ctx, "Theme Color Swatches");
    {
        const WLX_Theme *t = ctx->theme;
        struct { const char *name; WLX_Color c; } swatches[] = {
            { "background", t->background },
            { "foreground", t->foreground },
            { "surface",    t->surface },
            { "border",     t->border },
            { "accent",     t->accent },
        };
        wlx_grid_begin(ctx, 1, 5, .padding = 2);
            for (int i = 0; i < 5; i++) {
                wlx_push_id(ctx, (size_t)i);
                wlx_layout_begin(ctx, 2, WLX_VERT);
                    wlx_widget(ctx,
                        .widget_align = WLX_CENTER, .width = 40, .height = 30,
                        .color = swatches[i].c);
                    wlx_label(ctx, swatches[i].name,
                        .height = 20, .font_size = 12, .align = WLX_CENTER);
                wlx_layout_end(ctx);
                wlx_pop_id(ctx);
            }
        wlx_layout_end(ctx);
    }

    SUB_HEADING(ctx, "Custom Theme Builder");

    // Build custom theme from sliders
    st->custom_theme = wlx_theme_dark;
    st->custom_theme.background = to_color(st->theme_bg_r, st->theme_bg_g, st->theme_bg_b, 1.0f);
    st->custom_theme.foreground = to_color(st->theme_fg_r, st->theme_fg_g, st->theme_fg_b, 1.0f);
    st->custom_theme.surface    = to_color(st->theme_surface_r, st->theme_surface_g, st->theme_surface_b, 1.0f);
    st->custom_theme.accent     = to_color(st->theme_accent_r, st->theme_accent_g, st->theme_accent_b, 1.0f);
    st->custom_theme.hover_brightness = st->theme_hover_brightness;
    st->custom_theme.padding    = st->theme_padding;
    st->custom_theme.roundness  = st->theme_roundness;

    // Preview custom theme
    {
        const WLX_Theme *saved = ctx->theme;
        ctx->theme = &st->custom_theme;
        wlx_push_id(ctx, 2);
        wlx_scroll_panel_begin(ctx, -1,
            .height = 200,
            .back_color = st->custom_theme.background);
            wlx_layout_begin(ctx, 2, WLX_VERT);
                wlx_label(ctx, "Custom Theme Preview", .height = 28, .font_size = 18, .align = WLX_CENTER);
                theme_sample_widgets(ctx);
            wlx_layout_end(ctx);
        wlx_scroll_panel_end(ctx);
        wlx_pop_id(ctx);
        ctx->theme = saved;
    }

    OPTIONS_HEADING(ctx);

    wlx_label(ctx, "Background", .font_size = 14, .height = 24);
    wlx_slider(ctx, "BG Red     ", &st->theme_bg_r, .height = 34, .font_size = 14, .thumb_color = RED);
    wlx_slider(ctx, "BG Green   ", &st->theme_bg_g, .height = 34, .font_size = 14, .thumb_color = GREEN);
    wlx_slider(ctx, "BG Blue    ", &st->theme_bg_b, .height = 34, .font_size = 14, .thumb_color = BLUE);

    wlx_label(ctx, "Foreground", .font_size = 14, .height = 24);
    wlx_slider(ctx, "FG Red     ", &st->theme_fg_r, .height = 34, .font_size = 14, .thumb_color = RED);
    wlx_slider(ctx, "FG Green   ", &st->theme_fg_g, .height = 34, .font_size = 14, .thumb_color = GREEN);
    wlx_slider(ctx, "FG Blue    ", &st->theme_fg_b, .height = 34, .font_size = 14, .thumb_color = BLUE);

    wlx_label(ctx, "Surface", .font_size = 14, .height = 24);
    wlx_slider(ctx, "Srf Red    ", &st->theme_surface_r, .height = 34, .font_size = 14, .thumb_color = RED);
    wlx_slider(ctx, "Srf Green  ", &st->theme_surface_g, .height = 34, .font_size = 14, .thumb_color = GREEN);
    wlx_slider(ctx, "Srf Blue   ", &st->theme_surface_b, .height = 34, .font_size = 14, .thumb_color = BLUE);

    wlx_label(ctx, "Accent", .font_size = 14, .height = 24);
    wlx_slider(ctx, "Acc Red    ", &st->theme_accent_r, .height = 34, .font_size = 14, .thumb_color = RED);
    wlx_slider(ctx, "Acc Green  ", &st->theme_accent_g, .height = 34, .font_size = 14, .thumb_color = GREEN);
    wlx_slider(ctx, "Acc Blue   ", &st->theme_accent_b, .height = 34, .font_size = 14, .thumb_color = BLUE);

    wlx_slider(ctx, "Hover Brt  ", &st->theme_hover_brightness, .height = 34, .font_size = 14,
        .min_value = 0.0f, .max_value = 2.0f);
    wlx_slider(ctx, "Padding    ", &st->theme_padding, .height = 34, .font_size = 14,
        .min_value = 0.0f, .max_value = 20.0f);
    wlx_slider(ctx, "Roundness  ", &st->theme_roundness, .height = 34, .font_size = 14,
        .min_value = 0.0f, .max_value = 1.0f);

    wlx_layout_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Opacity
// ---------------------------------------------------------------------------
static void section_opacity(WLX_Context *ctx, Gallery_State *st) {
    wlx_layout_begin(ctx, 7, WLX_VERT);

    SECTION_HEADING(ctx, "Opacity");

    wlx_label(ctx, "Per-widget opacity multiplier. Applies to all draw operations.",
        .font_size = 16, .height = 30);

    SUB_HEADING(ctx, "Fixed Opacity Levels");
    wlx_layout_begin(ctx, 5, WLX_HORZ, .padding = 4);
        float levels[] = { 0.2f, 0.4f, 0.6f, 0.8f, 1.0f };
        for (int i = 0; i < 5; i++) {
            wlx_push_id(ctx, (size_t)(i + 100));
            wlx_layout_begin(ctx, 4, WLX_VERT, .padding = 2);
                char buf[16];
                snprintf(buf, sizeof(buf), "%.0f%%", levels[i] * 100);
                wlx_label(ctx, buf,
                    .height = 24, .font_size = 14, .align = WLX_CENTER,
                    .opacity = levels[i]);
                wlx_button(ctx, "Btn",
                    .height = 30, .font_size = 14, .align = WLX_CENTER,
                    .opacity = levels[i]);
                {
                    static bool checks[5] = { true, true, true, true, true };
                    wlx_checkbox(ctx, "Chk", &checks[i],
                        .height = 30, .font_size = 14,
                        .opacity = levels[i]);
                }
                wlx_widget(ctx,
                    .widget_align = WLX_CENTER, .width = -1, .height = 30,
                    .color = (Color){ 0, 120, 255, (unsigned char)(levels[i] * 255) });
            wlx_layout_end(ctx);
            wlx_pop_id(ctx);
        }
    wlx_layout_end(ctx);

    SUB_HEADING(ctx, "Interactive Opacity");
    wlx_slider(ctx, "Opacity    ", &st->opacity_control,
        .height = ROW_H, .min_value = 0.05f, .max_value = 1.0f, .font_size = 16);

    wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 4);
        wlx_label(ctx, "Dynamic text", .height = 50, .font_size = 18,
            .align = WLX_CENTER, .boxed = true,
            .back_color = heading_color(ctx->theme, 42, 22, 62),
            .opacity = st->opacity_control);
        wlx_button(ctx, "Dynamic button", .height = 50, .font_size = 18,
            .align = WLX_CENTER,
            .opacity = st->opacity_control);
        wlx_widget(ctx,
            .widget_align = WLX_CENTER, .width = -1, .height = 50,
            .color = (Color){ 200, 80, 60, (unsigned char)(st->opacity_control * 255) });
    wlx_layout_end(ctx);

    wlx_layout_end(ctx); // section layout
}

// ---------------------------------------------------------------------------
// Section: ID Stack & Dynamic Widgets
// ---------------------------------------------------------------------------
static void section_id_stack(WLX_Context *ctx, Gallery_State *st) {
    int count = (int)st->dynamic_panel_count;
    if (count < 1) count = 1;
    if (count > 10) count = 10;

    wlx_layout_begin(ctx, (size_t)(4 + count * 5), WLX_VERT);

    SECTION_HEADING(ctx, "ID Stack & Dynamic Widgets");

    wlx_label(ctx, "Loop-generated panels with wlx_push_id / wlx_pop_id for stable IDs.",
        .font_size = 16, .height = 30);

    for (int i = 0; i < count; i++) {
        wlx_push_id(ctx, (size_t)i);

        Color section_bg = (i % 2 == 0)
            ? heading_color(ctx->theme, 10, 10, 18)
            : heading_color(ctx->theme, 14, 14, 22);

        char header[32];
        snprintf(header, sizeof(header), "Panel %d", i + 1);

        wlx_label(ctx, header,
            .height = 36, .font_size = 20, .align = WLX_LEFT,
            .back_color = section_bg, .boxed = true);

        wlx_inputbox(ctx, "Name:  ", st->dynamic_names[i], sizeof(st->dynamic_names[i]),
            .height = ROW_H, .font_size = 16);
        wlx_slider(ctx, "Value    ", &st->dynamic_sliders[i],
            .height = ROW_H, .font_size = 16);
        wlx_checkbox(ctx, "Enabled", &st->dynamic_toggles[i],
            .height = ROW_H, .font_size = 16);

        // Divider
        wlx_widget(ctx,
            .widget_align = WLX_CENTER, .width = -1, .height = 2,
            .color = heading_color(ctx->theme, 32, 32, 42));

        wlx_pop_id(ctx);
    }

    OPTIONS_HEADING(ctx);

    wlx_slider(ctx, "Panel Count", &st->dynamic_panel_count,
        .height = ROW_H, .min_value = 1.0f, .max_value = 10.0f, .font_size = 16);

    wlx_layout_end(ctx);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(void) {
    printf("Wollix Widget Gallery\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix Widget Gallery");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    // Seed dynamic panel names
    for (int i = 0; i < 10; i++) {
        snprintf(g.dynamic_names[i], sizeof(g.dynamic_names[i]), "Panel %d", i + 1);
    }

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();
        WLX_Rect root = { 0, 0, w, h };

        ctx->theme = g.dark_mode ? &wlx_theme_dark : &wlx_theme_light;

        wlx_begin(ctx, root, wlx_process_raylib_input);
        BeginDrawing();
        ClearBackground((Color){
            ctx->theme->background.r, ctx->theme->background.g,
            ctx->theme->background.b, ctx->theme->background.a });

        // ---- Root: title + body + status bar ----
        wlx_layout_begin(ctx, 3, WLX_VERT,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(44), WLX_SLOT_FLEX(1), WLX_SLOT_PX(24) });

            // ── Title bar ──
            wlx_layout_begin(ctx, 2, WLX_HORZ,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_PX(160) });
                wlx_label(ctx, "  Wollix Widget Gallery",
                    .font_size = 24, .align = WLX_LEFT,
                    .back_color = heading_color(ctx->theme, 10, 10, 20));

                if (wlx_button(ctx, g.dark_mode ? "Light Mode" : "Dark Mode",
                    .font_size = 16, .align = WLX_CENTER,
                    .back_color = heading_color(ctx->theme, 32, 32, 47))) {
                    g.dark_mode = !g.dark_mode;
                }
            wlx_layout_end(ctx);

            // ── Body: sidebar + content ──
            wlx_layout_begin(ctx, 2, WLX_HORZ,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(200), WLX_SLOT_FLEX(1) });

                // ── Sidebar ──
                wlx_scroll_panel_begin(ctx, -1,
                    .back_color = heading_color(ctx->theme, 4, 4, 10));
                    wlx_layout_begin_auto(ctx, WLX_VERT, 36);
                        for (int i = 0; i < SECTION_COUNT; i++) {
                            wlx_push_id(ctx, (size_t)i);
                            Color btn_bg = (i == g.active_section)
                                ? heading_color(ctx->theme, 32, 32, 57)
                                : heading_color(ctx->theme, 10, 10, 18);
                            if (wlx_button(ctx, sections[i].name,
                                .height = 36, .font_size = 16,
                                .align = WLX_LEFT,
                                .back_color = btn_bg)) {
                                g.active_section = i;
                            }
                            wlx_pop_id(ctx);
                        }
                    wlx_layout_end(ctx);
                wlx_scroll_panel_end(ctx);

                // ── Content area ──
                // Each section wraps itself in a fixed-count layout.
                // (Dynamic/auto layouts forbid nested layout blocks.)
                wlx_scroll_panel_begin(ctx, -1);
                    if (g.active_section >= 0 && g.active_section < SECTION_COUNT) {
                        sections[g.active_section].render(ctx, &g);
                    }
                wlx_scroll_panel_end(ctx);

            wlx_layout_end(ctx);

            // ── Status bar ──
            {
                char status[256];
                snprintf(status, sizeof(status),
                    "  FPS: %d  |  Section: %s  |  Theme: %s",
                    GetFPS(),
                    sections[g.active_section].name,
                    g.dark_mode ? "Dark" : "Light");
                wlx_label(ctx, status,
                    .font_size = 13, .align = WLX_LEFT,
                    .back_color = heading_color(ctx->theme, 4, 4, 10));
            }

        wlx_layout_end(ctx);

        EndDrawing();
        wlx_end(ctx);
    }

    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
