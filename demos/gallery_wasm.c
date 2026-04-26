// gallery_wasm.c - Widget gallery demo for the bare-wasm32 backend.
// Adapted from gallery.c: all raylib types replaced with WLX_Color,
// texture section stubbed, FPS computed from frame time.
// Build: make wasm-bare

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#define WLX_SLOT_MINMAX_REDISTRIBUTE
#define WOLLIX_IMPLEMENTATION
#ifndef NDEBUG
#define NDEBUG
#endif
#include "wollix.h"
#include "wollix_wasm.h"

WLX_Input_State wlx_wasm_input_state = {0};

#define ROW_H 40

// Raylib color constants used by the original gallery
#define WASM_RED   WLX_RGBA(230, 41,  55,  255)
#define WASM_GREEN WLX_RGBA(0,   228, 48,  255)
#define WASM_BLUE  WLX_RGBA(0,   121, 241, 255)

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
static void section_borders(WLX_Context *ctx, Gallery_State *g);
static void section_auto_layout(WLX_Context *ctx, Gallery_State *g);
static void section_progress_toggle_radio(WLX_Context *ctx, Gallery_State *g);

static const Section sections[] = {
    { "Label",          section_label },
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
    { "Borders",        section_borders },
    { "Auto Layout",    section_auto_layout },
    { "Progress/Toggle/Radio", section_progress_toggle_radio },
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
    bool  label_show_bg;
    float label_opacity;

    // Button
    float button_font_size;
    float button_height;
    float button_r, button_g, button_b;
    int   button_click_count;

    // Checkbox
    bool  checkboxes[6];
    float checkbox_font_size;
    bool  checkbox_show_bg;

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
    float theme_opacity;
    float stack_opacity;

    // Borders
    float border_width;
    float border_r, border_g, border_b;
    bool  border_checkbox;
    float border_slider;
    char  border_input[128];

    // Dynamic widgets
    float dynamic_panel_count;
    float dynamic_sliders[10];
    bool  dynamic_toggles[10];
    char  dynamic_names[10][64];

    // Auto layout
    float auto_item_count;
    float auto_item_height;
    float auto_header_pct;
    float auto_sidebar_pct;

    // Progress / Toggle / Radio
    float progress_value;
    bool  toggle_a;
    bool  toggle_b;
    int   radio_choice;
};

static Gallery_State g = {
    .active_section = 0,
    .dark_mode = true,

    .label_font_size = 18.0f,
    .label_show_bg = false,
    .label_opacity = 1.0f,

    .button_font_size = 18.0f,
    .button_height = 40.0f,
    .button_r = 0.24f, .button_g = 0.12f, .button_b = 0.12f,
    .button_click_count = 0,

    .checkboxes = { true, false, true, false, false, false },
    .checkbox_font_size = 18.0f,
    .checkbox_show_bg = false,

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
    .theme_opacity = 1.0f,
    .stack_opacity = 1.0f,

    .border_width = 2.0f,
    .border_r = 0.0f, .border_g = 0.75f, .border_b = 1.0f,
    .border_checkbox = true,
    .border_slider = 0.5f,
    .border_input = "Hello!",

    .auto_item_count = 4.0f,
    .auto_item_height = 36.0f,
    .auto_header_pct = 15.0f,
    .auto_sidebar_pct = 30.0f,

    .progress_value = 0.35f,
    .toggle_a = false,
    .toggle_b = true,
    .radio_choice = 0,

    .dynamic_panel_count = 3.0f,
    .dynamic_sliders = { 0.2f, 0.4f, 0.6f, 0.8f, 1.0f, 0.3f, 0.5f, 0.7f, 0.9f, 0.1f },
    .dynamic_toggles = { true, false, true, false, true, false, true, false, true, false },
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static WLX_Color to_color(float r01, float g01, float b01, float a) {
    return (WLX_Color){
        (unsigned char)(r01 * 255),
        (unsigned char)(g01 * 255),
        (unsigned char)(b01 * 255),
        (unsigned char)(a * 255),
    };
}

static WLX_Color heading_color(const WLX_Theme *t, int tr, int tg, int tb) {
    int lum = ((int)t->background.r + t->background.g + t->background.b) / 3;
    int s = (lum < 128) ? 1 : -1;
    int r = (int)t->background.r + s * tr;
    int gg = (int)t->background.g + s * tg;
    int b = (int)t->background.b + s * tb;
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (gg < 0) gg = 0; if (gg > 255) gg = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return (WLX_Color){ (unsigned char)r, (unsigned char)gg, (unsigned char)b, 255 };
}

#define SECTION_HEADING(ctx, title) \
    wlx_label(ctx, title, \
        .font_size = 26, .align = WLX_CENTER, .height = 48, \
        .show_background = true, .back_color = heading_color(ctx->theme, 17, 12, 32))

#define SUB_HEADING(ctx, title) \
    wlx_label(ctx, title, \
        .font_size = 20, .align = WLX_LEFT, .height = 36, \
        .show_background = true, .back_color = heading_color(ctx->theme, 12, 12, 20))

#define OPTIONS_HEADING(ctx) \
    wlx_label(ctx, "Options", \
        .font_size = 20, .align = WLX_LEFT, .height = 36, \
        .show_background = true, .back_color = heading_color(ctx->theme, 20, 12, 12))

// ---------------------------------------------------------------------------
// Section: Label
// ---------------------------------------------------------------------------
static void section_label(WLX_Context *ctx, Gallery_State *st) {
    int fs = (int)st->label_font_size;

    wlx_panel_begin(ctx, .title = "Label",
        .title_font_size = 26, .title_height = 48,
        .title_back_color = heading_color(ctx->theme, 12, 12, 20));

    wlx_label(ctx, "A simple static label with default options.",
        .font_size = fs, .height = ROW_H);

    wlx_label(ctx, "Centered Heading (boxed)",
        .font_size = fs + 4, .align = WLX_CENTER, .height = 48,
        .show_background = st->label_show_bg,
        .back_color = heading_color(ctx->theme, 22, 22, 42),
        .opacity = st->label_opacity);

    wlx_label(ctx, "Right-aligned with custom color",
        .font_size = fs, .align = WLX_RIGHT, .height = ROW_H,
        .front_color = WLX_RGBA(100, 200, 255, 255),
        .opacity = st->label_opacity);

    wlx_label(ctx, "This is a longer paragraph of text to demonstrate word wrapping behavior. "
        "When the text exceeds the available width, it should wrap to the next line automatically. "
        "The label widget supports this via the .wrap option which is true by default.",
        .font_size = fs, .height = 80, .wrap = true,
        .opacity = st->label_opacity);

    SUB_HEADING(ctx, "Alignment Showcase");

    wlx_grid_begin(ctx, 2, 3, .padding = 2);
        const char *align_names[] = { "TOP_LEFT", "TOP_CENTER", "TOP_RIGHT",
                                       "BOTTOM_LEFT", "BOTTOM_CENTER", "BOTTOM_RIGHT" };
        WLX_Align aligns[] = { WLX_TOP_LEFT, WLX_TOP_CENTER, WLX_TOP_RIGHT,
                                WLX_BOTTOM_LEFT, WLX_BOTTOM_CENTER, WLX_BOTTOM_RIGHT };
        for (int i = 0; i < 6; i++) {
            wlx_push_id(ctx, (size_t)i);
            wlx_label(ctx, align_names[i],
                .font_size = 14, .widget_align = aligns[i],
                .back_color = heading_color(ctx->theme, 12, 17, 27), .show_background = true);
            wlx_pop_id(ctx);
        }
    wlx_layout_end(ctx);

    OPTIONS_HEADING(ctx);

    wlx_slider(ctx, "Font Size  ", &st->label_font_size,
        .height = ROW_H, .min_value = 8.0f, .max_value = 40.0f, .font_size = 16);
    wlx_checkbox(ctx, "Boxed", &st->label_show_bg,
        .height = ROW_H, .font_size = 16);
    wlx_slider(ctx, "Opacity    ", &st->label_opacity,
        .height = ROW_H, .min_value = 0.1f, .max_value = 1.0f, .font_size = 16);

    wlx_panel_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Button
// ---------------------------------------------------------------------------
static void section_button(WLX_Context *ctx, Gallery_State *st) {
    int fs = (int)st->button_font_size;
    int bh = (int)st->button_height;
    WLX_Color bc = to_color(st->button_r, st->button_g, st->button_b, 1.0f);

    wlx_panel_begin(ctx, .title = "Button",
        .title_font_size = 26, .title_height = 48,
        .title_back_color = heading_color(ctx->theme, 12, 12, 20));

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
        .height = bh, .font_size = fs, .align = WLX_CENTER)) {
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
        .height = ROW_H, .thumb_color = WASM_RED, .font_size = 16);
    wlx_slider(ctx, "Green      ", &st->button_g,
        .height = ROW_H, .thumb_color = WASM_GREEN, .font_size = 16);
    wlx_slider(ctx, "Blue       ", &st->button_b,
        .height = ROW_H, .thumb_color = WASM_BLUE, .font_size = 16);

    wlx_panel_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Checkbox
// ---------------------------------------------------------------------------
static void section_checkbox(WLX_Context *ctx, Gallery_State *st) {
    int fs = (int)st->checkbox_font_size;

    wlx_layout_begin(ctx, 11, WLX_VERT);

    SECTION_HEADING(ctx, "Checkbox");

    wlx_label(ctx, "Toggle checkbox with label. Returns true when state changes.",
        .font_size = 16, .height = 30);

    wlx_checkbox(ctx, "Enable audio", &st->checkboxes[0],
        .height = ROW_H, .font_size = fs);
    wlx_checkbox(ctx, "Enable V-Sync", &st->checkboxes[1],
        .height = ROW_H, .font_size = fs);
    wlx_checkbox(ctx, "Fullscreen mode", &st->checkboxes[2],
        .height = ROW_H, .font_size = fs);

    SUB_HEADING(ctx, "Custom Colors");

    wlx_checkbox(ctx, "Green check mark", &st->checkboxes[3],
        .height = ROW_H, .font_size = fs,
        .check_color = WLX_RGBA(0, 200, 0, 255));
    wlx_checkbox(ctx, "Red border", &st->checkboxes[4],
        .height = ROW_H, .font_size = fs,
        .border_color = WLX_RGBA(200, 0, 0, 255));

    OPTIONS_HEADING(ctx);

    wlx_slider(ctx, "Font Size  ", &st->checkbox_font_size,
        .height = ROW_H, .min_value = 12.0f, .max_value = 30.0f, .font_size = 16);
    wlx_checkbox(ctx, "Boxed", &st->checkbox_show_bg,
        .height = ROW_H, .font_size = 16);

    wlx_layout_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Slider
// ---------------------------------------------------------------------------
static void section_slider(WLX_Context *ctx, Gallery_State *st) {
    wlx_split_begin(ctx,
        .first_back_color = heading_color(ctx->theme, 8, 8, 16));

        wlx_panel_begin(ctx, .title = "Options",
            .title_back_color = heading_color(ctx->theme, 20, 12, 12));

            wlx_slider(ctx, "Min Value  ", &st->slider_min,
                .height = 36, .min_value = 0.0f, .max_value = 0.5f, .font_size = 14);
            wlx_slider(ctx, "Max Value  ", &st->slider_max,
                .height = 36, .min_value = 0.5f, .max_value = 2.0f, .font_size = 14);
            wlx_slider(ctx, "Track H    ", &st->slider_track_height,
                .height = 36, .min_value = 2.0f, .max_value = 24.0f, .font_size = 14);
            wlx_slider(ctx, "Thumb W    ", &st->slider_thumb_width,
                .height = 36, .min_value = 4.0f, .max_value = 40.0f, .font_size = 14);
            wlx_checkbox(ctx, "Show Label", &st->slider_show_label,
                .height = 30, .font_size = 14);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

        wlx_panel_begin(ctx, .title = "Slider",
            .title_font_size = 26, .title_height = 48,
            .title_back_color = heading_color(ctx->theme, 17, 12, 32),
            .padding = 0);

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
                .thumb_color = WASM_RED);
            wlx_slider(ctx, "Green      ", &st->color_g,
                .height = ROW_H, .font_size = 18,
                .thumb_color = WASM_GREEN);
            wlx_slider(ctx, "Blue       ", &st->color_b,
                .height = ROW_H, .font_size = 18,
                .thumb_color = WASM_BLUE);
            {
                WLX_Color preview = to_color(st->color_r, st->color_g, st->color_b, 1.0f);
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

        wlx_panel_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Input Box
// ---------------------------------------------------------------------------
static void section_inputbox(WLX_Context *ctx, Gallery_State *st) {
    int fs = (int)st->input_font_size;
    int ih = (int)st->input_height;
    WLX_Color fc = to_color(st->input_focus_r, st->input_focus_g, st->input_focus_b, 1.0f);

    wlx_panel_begin(ctx, .title = "Input Box",
        .title_font_size = 26, .title_height = 48,
        .title_back_color = heading_color(ctx->theme, 17, 12, 32),
        .padding = 0);

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
        .height = ROW_H, .min_value = 28.0f, .max_value = 80.0f, .font_size = 16);
    wlx_slider(ctx, "Focus R    ", &st->input_focus_r,
        .height = ROW_H, .thumb_color = WASM_RED, .font_size = 16);
    wlx_slider(ctx, "Focus G    ", &st->input_focus_g,
        .height = ROW_H, .thumb_color = WASM_GREEN, .font_size = 16);
    wlx_slider(ctx, "Focus B    ", &st->input_focus_b,
        .height = ROW_H, .thumb_color = WASM_BLUE, .font_size = 16);

    wlx_panel_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Scroll Panel
// ---------------------------------------------------------------------------
static void section_scroll_panel(WLX_Context *ctx, Gallery_State *st) {
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(
            WLX_SLOT_CONTENT,
            WLX_SLOT_CONTENT,
            WLX_SLOT_CONTENT,
            WLX_SLOT_PX(200),
            WLX_SLOT_CONTENT,
            WLX_SLOT_PX(180),
            WLX_SLOT_CONTENT,
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H)
        ));

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
        WLX_Rect sp_vp = wlx_get_scroll_panel_viewport(ctx);
        char vp_buf[64];
        snprintf(vp_buf, sizeof(vp_buf), "  Viewport: %.0f x %.0f", sp_vp.w, sp_vp.h);

        wlx_layout_begin_auto(ctx, WLX_VERT, 30);
            wlx_label(ctx, vp_buf,
                .height = 24, .font_size = 13, .align = WLX_LEFT,
                .front_color = heading_color(ctx->theme, 80, 80, 120));
            for (int i = 0; i < 20; i++) {
                wlx_push_id(ctx, (size_t)i);
                char buf[32];
                snprintf(buf, sizeof(buf), "  Item %d", i + 1);
                WLX_Color row_bg = (i % 2 == 0)
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

    wlx_panel_begin(ctx, .title = "Widget (Colored Rect)",
        .title_font_size = 26, .title_height = 48,
        .title_back_color = heading_color(ctx->theme, 12, 12, 20));

    wlx_label(ctx, "Low-level colored rectangle for swatches, spacers, and dividers.",
        .font_size = 16, .height = 30);

    SUB_HEADING(ctx, "Color Swatch");
    wlx_widget(ctx,
        .widget_align = WLX_CENTER, .width = 120, .height = 40,
        .color = WLX_RGBA(0, 120, 255, 255));

    SUB_HEADING(ctx, "Divider / Spacer");
    wlx_widget(ctx,
        .widget_align = WLX_CENTER, .width = -1, .height = 2,
        .color = WLX_RGBA(80, 80, 80, 255));

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
        WLX_Color swatch_colors[] = {
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

    wlx_panel_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Linear Layout
// ---------------------------------------------------------------------------
static void section_layout_linear(WLX_Context *ctx, Gallery_State *st) {
    int slots = (int)st->layout_slot_count;
    if (slots < 2) slots = 2;
    if (slots > 10) slots = 10;

    wlx_split_begin(ctx,
        .first_back_color = heading_color(ctx->theme, 8, 8, 16));

        wlx_panel_begin(ctx, .title = "Options",
            .title_back_color = heading_color(ctx->theme, 20, 12, 12));

            wlx_slider(ctx, "Slot Count ", &st->layout_slot_count,
                .height = ROW_H, .min_value = 2.0f, .max_value = 10.0f, .font_size = 16);
            wlx_slider(ctx, "Padding    ", &st->layout_padding,
                .height = ROW_H, .min_value = 0.0f, .max_value = 20.0f, .font_size = 16);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

            wlx_layout_begin_s(ctx, WLX_VERT,
                WLX_SIZES(
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(50),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(50),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_CONTENT
                ));

                SECTION_HEADING(ctx, "Linear Layouts");

                SUB_HEADING(ctx, "Horizontal (equal slots)");
                wlx_layout_begin(ctx, (size_t)slots, WLX_HORZ, .padding = st->layout_padding);
                    for (int i = 0; i < slots; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        WLX_Color c = { (unsigned char)(60 + i * 25), 60, (unsigned char)(200 - i * 15), 255 };
                        char buf[16];
                        snprintf(buf, sizeof(buf), "%d", i);
                        wlx_label(ctx, buf,
                            .font_size = 18, .align = WLX_CENTER,
                            .back_color = c, .show_background = true);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Vertical (equal slots)");
                wlx_layout_begin(ctx, (size_t)slots, WLX_VERT, .padding = st->layout_padding);
                    for (int i = 0; i < slots; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        WLX_Color c = { 60, (unsigned char)(60 + i * 25), (unsigned char)(200 - i * 15), 255 };
                        char buf[16];
                        snprintf(buf, sizeof(buf), "Row %d", i);
                        wlx_label(ctx, buf,
                            .font_size = 16, .align = WLX_LEFT,
                            .back_color = c, .show_background = true);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Variable Sizes (PX + PCT + FLEX)");
                wlx_layout_begin_s(ctx, WLX_HORZ,
                    WLX_SIZES(WLX_SLOT_PX(100), WLX_SLOT_PCT(30), WLX_SLOT_FLEX(1)),
                    .padding = 2);
                    wlx_label(ctx, "PX(100)",
                        .height = 50, .font_size = 14, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 62, 22, 22), .show_background = true);
                    wlx_label(ctx, "PCT(30)",
                        .height = 50, .font_size = 14, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 22, 62, 22), .show_background = true);
                    wlx_label(ctx, "FLEX(1)",
                        .height = 50, .font_size = 14, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 22, 22, 62), .show_background = true);
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Auto-Sizing Layout");
                wlx_layout_begin_auto(ctx, WLX_VERT, 32);
                    for (int i = 0; i < 5; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        char buf[32];
                        snprintf(buf, sizeof(buf), "Auto item %d", i + 1);
                        wlx_label(ctx, buf,
                            .height = 32, .font_size = 15,
                            .back_color = heading_color(ctx->theme, 12, 12, 24), .show_background = true);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

        wlx_layout_end(ctx);

    wlx_split_end(ctx);
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

    wlx_split_begin(ctx,
        .first_back_color = heading_color(ctx->theme, 8, 8, 16));

        wlx_panel_begin(ctx, .title = "Options",
            .title_back_color = heading_color(ctx->theme, 20, 12, 12));

            wlx_slider(ctx, "Rows       ", &st->grid_rows,
                .height = ROW_H, .min_value = 1.0f, .max_value = 8.0f, .font_size = 16);
            wlx_slider(ctx, "Cols       ", &st->grid_cols,
                .height = ROW_H, .min_value = 1.0f, .max_value = 8.0f, .font_size = 16);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

            wlx_layout_begin_s(ctx, WLX_VERT,
                WLX_SIZES(
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(200),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(130),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(130),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(130),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(180)
                ));

                SECTION_HEADING(ctx, "Grid Layouts");

                SUB_HEADING(ctx, "Fixed Grid (wlx_grid_begin)");
                wlx_grid_begin(ctx, (size_t)rows, (size_t)cols, .padding = 2);
                    for (int r = 0; r < rows; r++) {
                        for (int c = 0; c < cols; c++) {
                            wlx_push_id(ctx, (size_t)(r * cols + c));
                            WLX_Color clr = {
                                (unsigned char)(40 + c * 40),
                                (unsigned char)(40 + r * 40),
                                (unsigned char)(120),
                                255
                            };
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%d,%d", r, c);
                            wlx_label(ctx, buf,
                                .height = -1, .font_size = 14, .align = WLX_CENTER,
                                .back_color = clr, .show_background = true);
                            wlx_pop_id(ctx);
                        }
                    }
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Content-Sized Rows (row_sizes)");
                {
                    WLX_Slot_Size rsizes[] = {
                        WLX_SLOT_CONTENT,
                        WLX_SLOT_CONTENT_MIN(20),
                        WLX_SLOT_CONTENT_MAX(30),
                    };
                    wlx_grid_begin(ctx, 3, 3, .padding = 2, .row_sizes = rsizes);
                        for (int c = 0; c < 3; c++) {
                            wlx_push_id(ctx, (size_t)c);
                            WLX_Color clr = { (unsigned char)(50 + c * 30), 70, 140, 255 };
                            wlx_label(ctx, "Tall", .height = 40, .font_size = 13,
                                .align = WLX_CENTER, .back_color = clr,
                                .show_background = true);
                            wlx_pop_id(ctx);
                        }
                        for (int c = 0; c < 3; c++) {
                            wlx_push_id(ctx, (size_t)(3 + c));
                            WLX_Color clr = { 70, (unsigned char)(50 + c * 30), 140, 255 };
                            wlx_label(ctx, "Min 20", .height = 20, .font_size = 13,
                                .align = WLX_CENTER, .back_color = clr,
                                .show_background = true);
                            wlx_pop_id(ctx);
                        }
                        for (int c = 0; c < 3; c++) {
                            wlx_push_id(ctx, (size_t)(6 + c));
                            WLX_Color clr = { 70, 140, (unsigned char)(50 + c * 30), 255 };
                            wlx_label(ctx, "Max 30", .height = 35, .font_size = 13,
                                .align = WLX_CENTER, .back_color = clr,
                                .show_background = true);
                            wlx_pop_id(ctx);
                        }
                    wlx_layout_end(ctx);
                }

                SUB_HEADING(ctx, "Auto Grid (wlx_grid_begin_auto)");
                wlx_grid_begin_auto(ctx, 4, 40, .padding = 2);
                    for (int i = 0; i < 12; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        char buf[16];
                        snprintf(buf, sizeof(buf), "Cell %d", i);
                        WLX_Color clr = { (unsigned char)(60 + i * 15), 80, (unsigned char)(180 - i * 10), 255 };
                        wlx_label(ctx, buf,
                            .height = -1, .font_size = 13, .align = WLX_CENTER,
                            .back_color = clr, .show_background = true);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Auto Tile Grid (wlx_grid_begin_auto_tile)");
                wlx_grid_begin_auto_tile(ctx, 100, 60, .padding = 2);
                    for (int i = 0; i < 10; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        char buf[16];
                        snprintf(buf, sizeof(buf), "Tile %d", i);
                        WLX_Color clr = { (unsigned char)(80 + i * 10), (unsigned char)(60 + i * 15), 120, 255 };
                        wlx_label(ctx, buf,
                            .height = -1, .font_size = 13, .align = WLX_CENTER,
                            .back_color = clr, .show_background = true);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Grid Cell Positioning (wlx_grid_cell)");
                wlx_grid_begin(ctx, 3, 3, .padding = 2);
                    wlx_grid_cell(ctx, 0, 0, .col_span = 2);
                    wlx_label(ctx, "Span 2 cols",
                        .height = -1, .font_size = 14, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 62, 22, 42), .show_background = true);

                    wlx_grid_cell(ctx, 0, 2, .row_span = 2);
                    wlx_label(ctx, "Span 2 rows",
                        .height = -1, .font_size = 14, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 22, 62, 42), .show_background = true);

                    wlx_grid_cell(ctx, 1, 0);
                    wlx_label(ctx, "1,0",
                        .height = -1, .font_size = 14, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 42, 42, 62), .show_background = true);

                    wlx_grid_cell(ctx, 1, 1);
                    wlx_label(ctx, "1,1",
                        .height = -1, .font_size = 14, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 42, 62, 42), .show_background = true);

                    wlx_grid_cell(ctx, 2, 0, .col_span = 3);
                    wlx_label(ctx, "Full width (span 3 cols)",
                        .height = -1, .font_size = 14, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 42, 42, 42), .show_background = true);
                wlx_layout_end(ctx);

        wlx_layout_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Flex & Sizing
// ---------------------------------------------------------------------------
static void section_layout_flex(WLX_Context *ctx, Gallery_State *st) {
    wlx_split_begin(ctx,
        .first_back_color = heading_color(ctx->theme, 8, 8, 16));

        wlx_panel_begin(ctx, .title = "Options",
            .title_back_color = heading_color(ctx->theme, 20, 12, 12));

            wlx_slider(ctx, "Weight A   ", &st->flex_weight_a,
                .height = ROW_H, .min_value = 0.1f, .max_value = 5.0f, .font_size = 16);
            wlx_slider(ctx, "Weight B   ", &st->flex_weight_b,
                .height = ROW_H, .min_value = 0.1f, .max_value = 5.0f, .font_size = 16);
            wlx_slider(ctx, "Min        ", &st->flex_min,
                .height = ROW_H, .min_value = 0.0f, .max_value = 400.0f, .font_size = 16);
            wlx_slider(ctx, "Max        ", &st->flex_max,
                .height = ROW_H, .min_value = 100.0f, .max_value = 800.0f, .font_size = 16);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

            wlx_layout_begin_s(ctx, WLX_VERT,
                WLX_SIZES(
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(60),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(50),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(130),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(50),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(40),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(40)
                ));

                SECTION_HEADING(ctx, "Flex & Sizing");

                SUB_HEADING(ctx, "Flex Weight Distribution");
                wlx_label(ctx, "Sidebar + Content pattern using FLEX weights:",
                    .font_size = 14, .height = 24);

                {
                    float wa = st->flex_weight_a;
                    float wb = st->flex_weight_b;
                    if (wa < 0.1f) wa = 0.1f;
                    if (wb < 0.1f) wb = 0.1f;

                    wlx_layout_begin_s(ctx, WLX_HORZ,
                        WLX_SIZES(WLX_SLOT_FLEX(wa), WLX_SLOT_FLEX(wb)),
                        .padding = 2);
                        char buf_a[32], buf_b[32];
                        snprintf(buf_a, sizeof(buf_a), "FLEX(%.1f)", wa);
                        snprintf(buf_b, sizeof(buf_b), "FLEX(%.1f)", wb);
                        wlx_label(ctx, buf_a,
                            .height = 60, .font_size = 16, .align = WLX_CENTER,
                            .back_color = heading_color(ctx->theme, 42, 22, 62), .show_background = true);
                        wlx_label(ctx, buf_b,
                            .height = 60, .font_size = 16, .align = WLX_CENTER,
                            .back_color = heading_color(ctx->theme, 22, 42, 62), .show_background = true);
                    wlx_layout_end(ctx);
                }

                SUB_HEADING(ctx, "PX Fixed + FLEX Fill");
                wlx_layout_begin_s(ctx, WLX_HORZ,
                    WLX_SIZES(WLX_SLOT_PX(120), WLX_SLOT_FLEX(1), WLX_SLOT_PX(120)),
                    .padding = 2);
                    wlx_label(ctx, "PX(120)",
                        .height = 50, .font_size = 14, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 62, 32, 32), .show_background = true);
                    wlx_label(ctx, "FLEX(1)",
                        .height = 50, .font_size = 14, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 32, 62, 32), .show_background = true);
                    wlx_label(ctx, "PX(120)",
                        .height = 50, .font_size = 14, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 32, 32, 62), .show_background = true);
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Content-Fit Slots (CONTENT)");
                wlx_layout_begin_s(ctx, WLX_VERT,
                    WLX_SIZES(WLX_SLOT_PX(150)),
                    .padding = 0);
                    wlx_layout_begin_s(ctx, WLX_VERT,
                        WLX_SIZES(WLX_SLOT_CONTENT, WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1)),
                        .padding = 2);
                        wlx_label(ctx, "CONTENT (h=30)",
                            .height = 30, .font_size = 14, .align = WLX_CENTER,
                            .back_color = heading_color(ctx->theme, 32, 52, 32), .show_background = true);
                        wlx_label(ctx, "CONTENT (h=50)",
                            .height = 50, .font_size = 14, .align = WLX_CENTER,
                            .back_color = heading_color(ctx->theme, 52, 32, 32), .show_background = true);
                        wlx_label(ctx, "FLEX(1) (remaining)",
                            .font_size = 14, .align = WLX_CENTER,
                            .back_color = heading_color(ctx->theme, 32, 32, 52), .show_background = true);
                    wlx_layout_end(ctx);
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Min/Max Constraints");
                {
                    float lo = st->flex_min;
                    float hi = st->flex_max;
                    if (hi < lo) hi = lo;

                    wlx_layout_begin_s(ctx, WLX_HORZ,
                        WLX_SIZES(WLX_SLOT_FLEX_MINMAX(1, lo, hi), WLX_SLOT_FLEX(1)),
                        .padding = 2);
                        char buf[64];
                        snprintf(buf, sizeof(buf), "FLEX_MINMAX(1, %.0f, %.0f)", lo, hi);
                        wlx_label(ctx, buf,
                            .height = 50, .font_size = 13, .align = WLX_CENTER,
                            .back_color = heading_color(ctx->theme, 52, 32, 52), .show_background = true);
                        wlx_label(ctx, "FLEX(1)",
                            .height = 50, .font_size = 13, .align = WLX_CENTER,
                            .back_color = heading_color(ctx->theme, 32, 52, 52), .show_background = true);
                    wlx_layout_end(ctx);
                }

                SUB_HEADING(ctx, "Widget-Level Min/Max");
                wlx_layout_begin(ctx, 1, WLX_HORZ);
                    wlx_label(ctx, "min_w=100, max_w=300, centered",
                        .widget_align = WLX_CENTER,
                        .min_width = 100, .max_width = 300,
                        .height = 40, .font_size = 14, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 42, 42, 62), .show_background = true);
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Overflow");
                wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 10);
                    wlx_label(ctx, "overflow=true, width=500 (exceeds slot if narrow)",
                        .overflow = true, .width = 500,
                        .height = 40, .font_size = 14, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 62, 42, 22), .show_background = true);
                wlx_layout_end(ctx);

        wlx_layout_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Theming
// ---------------------------------------------------------------------------

static void theme_sample_widgets(WLX_Context *ctx) {
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(
            WLX_SLOT_PX(30),
            WLX_SLOT_PX(34),
            WLX_SLOT_PX(34),
            WLX_SLOT_PX(34),
            WLX_SLOT_PX(34)
        ), .padding = 2);
        wlx_label(ctx, "Sample Label", .height = 30, .font_size = 16, .align = WLX_CENTER);
        {
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
    st->custom_theme = wlx_theme_dark;
    st->custom_theme.background = to_color(st->theme_bg_r, st->theme_bg_g, st->theme_bg_b, 1.0f);
    st->custom_theme.foreground = to_color(st->theme_fg_r, st->theme_fg_g, st->theme_fg_b, 1.0f);
    st->custom_theme.surface    = to_color(st->theme_surface_r, st->theme_surface_g, st->theme_surface_b, 1.0f);
    st->custom_theme.accent     = to_color(st->theme_accent_r, st->theme_accent_g, st->theme_accent_b, 1.0f);
    st->custom_theme.hover_brightness = st->theme_hover_brightness;
    st->custom_theme.padding    = st->theme_padding;
    st->custom_theme.roundness  = st->theme_roundness;

    wlx_split_begin(ctx,
        .first_size = WLX_SLOT_PX(300),
        .first_back_color = heading_color(ctx->theme, 8, 8, 16));

        wlx_panel_begin(ctx, .title = "Theme Options",
            .title_back_color = heading_color(ctx->theme, 20, 12, 12));

                wlx_label(ctx, "Background", .font_size = 14, .height = 24);
                wlx_slider(ctx, "BG Red  ", &st->theme_bg_r, .height = 30, .font_size = 13, .thumb_color = WASM_RED);
                wlx_slider(ctx, "BG Green", &st->theme_bg_g, .height = 30, .font_size = 13, .thumb_color = WASM_GREEN);
                wlx_slider(ctx, "BG Blue ", &st->theme_bg_b, .height = 30, .font_size = 13, .thumb_color = WASM_BLUE);

                wlx_label(ctx, "Foreground", .font_size = 14, .height = 24);
                wlx_slider(ctx, "FG Red  ", &st->theme_fg_r, .height = 30, .font_size = 13, .thumb_color = WASM_RED);
                wlx_slider(ctx, "FG Green", &st->theme_fg_g, .height = 30, .font_size = 13, .thumb_color = WASM_GREEN);
                wlx_slider(ctx, "FG Blue ", &st->theme_fg_b, .height = 30, .font_size = 13, .thumb_color = WASM_BLUE);

                wlx_label(ctx, "Surface", .font_size = 14, .height = 24);
                wlx_slider(ctx, "Srf Red ", &st->theme_surface_r, .height = 30, .font_size = 13, .thumb_color = WASM_RED);
                wlx_slider(ctx, "Srf Grn ", &st->theme_surface_g, .height = 30, .font_size = 13, .thumb_color = WASM_GREEN);
                wlx_slider(ctx, "Srf Blue", &st->theme_surface_b, .height = 30, .font_size = 13, .thumb_color = WASM_BLUE);

                wlx_label(ctx, "Accent", .font_size = 14, .height = 24);
                wlx_slider(ctx, "Acc Red ", &st->theme_accent_r, .height = 30, .font_size = 13, .thumb_color = WASM_RED);
                wlx_slider(ctx, "Acc Grn ", &st->theme_accent_g, .height = 30, .font_size = 13, .thumb_color = WASM_GREEN);
                wlx_slider(ctx, "Acc Blue", &st->theme_accent_b, .height = 30, .font_size = 13, .thumb_color = WASM_BLUE);

                wlx_label(ctx, "Other", .font_size = 14, .height = 24);
                wlx_slider(ctx, "Hover Brt", &st->theme_hover_brightness, .height = 30, .font_size = 13,
                    .min_value = 0.0f, .max_value = 2.0f);
                wlx_slider(ctx, "Padding  ", &st->theme_padding, .height = 30, .font_size = 13,
                    .min_value = 0.0f, .max_value = 20.0f);
                wlx_slider(ctx, "Roundness", &st->theme_roundness, .height = 30, .font_size = 13,
                    .min_value = 0.0f, .max_value = 1.0f);

            wlx_panel_end(ctx);

    wlx_split_next(ctx);

            wlx_layout_begin_s(ctx, WLX_VERT,
                WLX_SIZES(
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(220),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(36),
                    WLX_SLOT_PX(24),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(200)
                ));

                SECTION_HEADING(ctx, "Theming");

                wlx_label(ctx, "Built-in dark/light themes and custom theme creation.",
                    .font_size = 16, .height = 30);

                SUB_HEADING(ctx, "Side-by-Side: Dark vs Light");
                wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 4);
                    {
                        const WLX_Theme *saved = ctx->theme;
                        ctx->theme = &wlx_theme_dark;
                        wlx_push_id(ctx, 0);
                        wlx_scroll_panel_begin(ctx, 220,
                            .height = 220,
                            .back_color = wlx_theme_dark.background);
                            wlx_layout_begin_s(ctx, WLX_VERT,
                                WLX_SIZES(WLX_SLOT_PX(28), WLX_SLOT_FLEX(1)));
                                wlx_label(ctx, "Dark Theme", .height = 28, .font_size = 18, .align = WLX_CENTER);
                                theme_sample_widgets(ctx);
                            wlx_layout_end(ctx);
                        wlx_scroll_panel_end(ctx);
                        wlx_pop_id(ctx);
                        ctx->theme = saved;
                    }

                    {
                        const WLX_Theme *saved = ctx->theme;
                        ctx->theme = &wlx_theme_light;
                        wlx_push_id(ctx, 1);
                        wlx_scroll_panel_begin(ctx, 220,
                            .height = 220,
                            .back_color = wlx_theme_light.background);
                            wlx_layout_begin_s(ctx, WLX_VERT,
                                WLX_SIZES(WLX_SLOT_PX(28), WLX_SLOT_FLEX(1)));
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
                    wlx_layout_begin(ctx, 5, WLX_HORZ, .padding = 2);
                        for (int i = 0; i < 5; i++) {
                            wlx_push_id(ctx, (size_t)i);
                            wlx_widget(ctx,
                                .widget_align = WLX_CENTER, .width = 40, .height = 30,
                                .color = swatches[i].c);
                            wlx_pop_id(ctx);
                        }
                    wlx_layout_end(ctx);
                    wlx_layout_begin(ctx, 5, WLX_HORZ, .padding = 2);
                        for (int i = 0; i < 5; i++) {
                            wlx_push_id(ctx, (size_t)(i + 5));
                            wlx_label(ctx, swatches[i].name,
                                .height = 20, .font_size = 12, .align = WLX_CENTER);
                            wlx_pop_id(ctx);
                        }
                    wlx_layout_end(ctx);
                }

                SUB_HEADING(ctx, "Custom Theme Preview");
                {
                    const WLX_Theme *saved = ctx->theme;
                    ctx->theme = &st->custom_theme;
                    wlx_push_id(ctx, 2);
                        wlx_layout_begin_s(ctx, WLX_VERT,
                            WLX_SIZES(WLX_SLOT_PX(28), WLX_SLOT_FLEX(1)));
                            wlx_label(ctx, "Custom Theme Preview", .height = 28, .font_size = 18, .align = WLX_CENTER);
                            theme_sample_widgets(ctx);
                        wlx_layout_end(ctx);
                    wlx_pop_id(ctx);
                    ctx->theme = saved;
                }

        wlx_layout_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Opacity
// ---------------------------------------------------------------------------
static void section_opacity(WLX_Context *ctx, Gallery_State *st) {
    wlx_split_begin(ctx,
        .first_back_color = heading_color(ctx->theme, 8, 8, 16));

        wlx_panel_begin(ctx, .title = "Options",
            .title_back_color = heading_color(ctx->theme, 20, 12, 12));

            wlx_slider(ctx, "Widget      ", &st->opacity_control,
                .height = 36, .min_value = 0.0f, .max_value = 1.0f, .font_size = 14);
            wlx_slider(ctx, "Theme       ", &st->theme_opacity,
                .height = 36, .min_value = 0.0f, .max_value = 1.0f, .font_size = 14);
            wlx_slider(ctx, "Stack       ", &st->stack_opacity,
                .height = 36, .min_value = 0.0f, .max_value = 1.0f, .font_size = 14);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

            wlx_layout_begin_s(ctx, WLX_VERT,
                WLX_SIZES(
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(100),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(40),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(70),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(70)
                ));

                SECTION_HEADING(ctx, "Opacity");

                wlx_label(ctx, "Three-layer model: per-widget * theme * context stack.",
                    .font_size = 16, .height = 26);

                SUB_HEADING(ctx, "Fixed Per-Widget Levels");
                wlx_layout_begin(ctx, 5, WLX_HORZ, .padding = 4);
                    float levels[] = { 0.2f, 0.4f, 0.6f, 0.8f, 1.0f };
                    for (int i = 0; i < 5; i++) {
                        wlx_push_id(ctx, (size_t)(i + 100));
                        wlx_layout_begin(ctx, 4, WLX_VERT, .padding = 1);
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%.0f%%", levels[i] * 100);
                            wlx_label(ctx, buf,
                                .height = 20, .font_size = 13, .align = WLX_CENTER,
                                .opacity = levels[i]);
                            wlx_button(ctx, "Btn",
                                .height = 24, .font_size = 13, .align = WLX_CENTER,
                                .opacity = levels[i]);
                            {
                                static bool checks[5] = { true, true, true, true, true };
                                wlx_checkbox(ctx, "Chk", &checks[i],
                                    .height = 24, .font_size = 13,
                                    .opacity = levels[i]);
                            }
                            wlx_widget(ctx,
                                .widget_align = WLX_CENTER, .width = -1, .height = 20,
                                .color = WLX_RGBA(0, 120, 255, (unsigned char)(levels[i] * 255)));
                        wlx_layout_end(ctx);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Interactive (per-widget slider)");
                wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 4);
                    wlx_label(ctx, "Dynamic text", .height = 40, .font_size = 16,
                        .align = WLX_CENTER, .show_background = true,
                        .back_color = heading_color(ctx->theme, 42, 22, 62),
                        .opacity = st->opacity_control);
                    wlx_button(ctx, "Dynamic button", .height = 40, .font_size = 16,
                        .align = WLX_CENTER,
                        .opacity = st->opacity_control);
                    wlx_widget(ctx,
                        .widget_align = WLX_CENTER, .width = -1, .height = 40,
                        .color = WLX_RGBA(200, 80, 60, (unsigned char)(st->opacity_control * 255)));
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Context Stack (push_opacity)");
                wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 4);

                    wlx_push_id(ctx, 200);
                    wlx_push_opacity(ctx, st->stack_opacity);
                    wlx_layout_begin(ctx, 2, WLX_VERT, .padding = 2);
                        wlx_label(ctx, "Single push", .height = 20, .font_size = 12,
                            .align = WLX_CENTER);
                        wlx_button(ctx, "Faded", .height = 30, .font_size = 13,
                            .align = WLX_CENTER);
                    wlx_layout_end(ctx);
                    wlx_pop_opacity(ctx);
                    wlx_pop_id(ctx);

                    wlx_push_id(ctx, 201);
                    wlx_push_opacity(ctx, st->stack_opacity);
                    wlx_push_opacity(ctx, 0.5f);
                    wlx_layout_begin(ctx, 2, WLX_VERT, .padding = 2);
                        wlx_label(ctx, "Nested (slider * 0.5)", .height = 20, .font_size = 12,
                            .align = WLX_CENTER);
                        wlx_button(ctx, "Double-faded", .height = 30, .font_size = 13,
                            .align = WLX_CENTER);
                    wlx_layout_end(ctx);
                    wlx_pop_opacity(ctx);
                    wlx_pop_opacity(ctx);
                    wlx_pop_id(ctx);

                    wlx_push_id(ctx, 202);
                    wlx_push_opacity(ctx, st->stack_opacity);
                    wlx_layout_begin(ctx, 2, WLX_VERT, .padding = 2);
                        wlx_button(ctx, ".opacity=1 override", .height = 30, .font_size = 13,
                            .align = WLX_CENTER, .opacity = 1.0f);
                        wlx_label(ctx, "Stays opaque", .height = 20, .font_size = 12,
                            .align = WLX_CENTER);
                    wlx_layout_end(ctx);
                    wlx_pop_opacity(ctx);
                    wlx_pop_id(ctx);

                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "All Three Combined");
                {
                    float saved_theme_opacity = ctx->theme->opacity;
                    ((WLX_Theme *)ctx->theme)->opacity = st->theme_opacity;

                    wlx_push_opacity(ctx, st->stack_opacity);
                    wlx_layout_begin(ctx, 4, WLX_HORZ, .padding = 4);
                        wlx_push_id(ctx, 300);
                        wlx_label(ctx, "w*t*s", .height = 50, .font_size = 14,
                            .align = WLX_CENTER, .show_background = true,
                            .back_color = heading_color(ctx->theme, 42, 22, 62),
                            .opacity = st->opacity_control);
                        wlx_button(ctx, "Combined", .height = 50, .font_size = 14,
                            .align = WLX_CENTER,
                            .opacity = st->opacity_control);
                        {
                            static bool cb = true;
                            wlx_checkbox(ctx, "All 3", &cb, .height = 50, .font_size = 14,
                                .opacity = st->opacity_control);
                        }
                        {
                            static float sv = 0.5f;
                            wlx_slider(ctx, "", &sv, .height = 50, .font_size = 12,
                                .show_label = false,
                                .opacity = st->opacity_control);
                        }
                        wlx_pop_id(ctx);
                    wlx_layout_end(ctx);
                    wlx_pop_opacity(ctx);

                    ((WLX_Theme *)ctx->theme)->opacity = saved_theme_opacity;
                }

        wlx_layout_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: ID Stack & Dynamic Widgets
// ---------------------------------------------------------------------------
static void section_id_stack(WLX_Context *ctx, Gallery_State *st) {
    int count = (int)st->dynamic_panel_count;
    if (count < 1) count = 1;
    if (count > 10) count = 10;

    wlx_split_begin(ctx,
        .first_back_color = heading_color(ctx->theme, 8, 8, 16));

        wlx_panel_begin(ctx, .title = "Options",
            .title_back_color = heading_color(ctx->theme, 20, 12, 12));

            wlx_slider(ctx, "Panel Count", &st->dynamic_panel_count,
                .height = ROW_H, .min_value = 1.0f, .max_value = 10.0f, .font_size = 16);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

            wlx_layout_begin_auto(ctx, WLX_VERT, 0);

                wlx_layout_auto_slot_px(ctx, 48);
                SECTION_HEADING(ctx, "ID Stack & Dynamic Widgets");

                wlx_layout_auto_slot_px(ctx, 30);
                wlx_label(ctx, "Loop-generated panels with wlx_push_id / wlx_pop_id for stable IDs.",
                    .font_size = 16, .height = 30);

                for (int i = 0; i < count; i++) {
                    wlx_push_id(ctx, (size_t)i);

                    WLX_Color section_bg = (i % 2 == 0)
                        ? heading_color(ctx->theme, 10, 10, 18)
                        : heading_color(ctx->theme, 14, 14, 22);

                    char header[32];
                    snprintf(header, sizeof(header), "Panel %d", i + 1);

                    wlx_layout_auto_slot_px(ctx, 36);
                    wlx_label(ctx, header,
                        .height = 36, .font_size = 20, .align = WLX_LEFT,
                        .back_color = section_bg, .show_background = true);

                    wlx_layout_auto_slot_px(ctx, ROW_H);
                    wlx_inputbox(ctx, "Name:  ", st->dynamic_names[i], sizeof(st->dynamic_names[i]),
                        .height = ROW_H, .font_size = 16);
                    wlx_layout_auto_slot_px(ctx, ROW_H);
                    wlx_slider(ctx, "Value    ", &st->dynamic_sliders[i],
                        .height = ROW_H, .font_size = 16);
                    wlx_layout_auto_slot_px(ctx, ROW_H);
                    wlx_checkbox(ctx, "Enabled", &st->dynamic_toggles[i],
                        .height = ROW_H, .font_size = 16);

                    wlx_layout_auto_slot_px(ctx, 2);
                    wlx_widget(ctx,
                        .widget_align = WLX_CENTER, .width = -1, .height = 2,
                        .color = heading_color(ctx->theme, 32, 32, 42));

                    wlx_pop_id(ctx);
                }

            wlx_layout_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Borders
// ---------------------------------------------------------------------------
static void section_borders(WLX_Context *ctx, Gallery_State *st) {
    WLX_Color bdr = to_color(st->border_r, st->border_g, st->border_b, 1.0f);
    float bw = st->border_width;

    wlx_split_begin(ctx,
        .first_back_color = heading_color(ctx->theme, 8, 8, 16));

        wlx_panel_begin(ctx, .title = "Controls",
            .title_back_color = heading_color(ctx->theme, 20, 12, 12));

                wlx_slider(ctx, "Width  ", &st->border_width,
                    .height = 36, .min_value = 0.0f, .max_value = 8.0f, .font_size = 14);

                wlx_label(ctx, "Border Color", .font_size = 14, .height = 24);
                wlx_slider(ctx, "R ", &st->border_r,
                    .height = 30, .font_size = 14, .thumb_color = WASM_RED);
                wlx_slider(ctx, "G ", &st->border_g,
                    .height = 30, .font_size = 14, .thumb_color = WASM_GREEN);
                wlx_slider(ctx, "B ", &st->border_b,
                    .height = 30, .font_size = 14, .thumb_color = WASM_BLUE);

                wlx_label(ctx, "Preview", .font_size = 14, .height = 24);
                wlx_widget(ctx, .height = 30,
                    .color = bdr, .border_color = WLX_RGBA(255, 255, 255, 80), .border_width = 1);

                wlx_slider(ctx, "Slider ", &st->border_slider,
                    .height = 36, .font_size = 14);
                wlx_checkbox(ctx, "Checkbox", &st->border_checkbox,
                    .font_size = 14, .height = 30);

            wlx_panel_end(ctx);

    wlx_split_next(ctx);

        wlx_panel_begin(ctx, .title = "Borders",
            .title_font_size = 26, .title_height = 48,
            .title_back_color = heading_color(ctx->theme, 17, 12, 32),
            .padding = 2);

                wlx_label(ctx, "Adjust controls on the left - see borders update live on the right.",
                    .font_size = 16, .height = 30);

                SUB_HEADING(ctx, "Labels");
                wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 4);
                    wlx_label(ctx, "Dynamic",
                        .font_size = 16, .height = 40, .show_background = true,
                        .border_color = bdr, .border_width = bw);
                    wlx_label(ctx, "Red, width 3",
                        .font_size = 16, .height = 40, .show_background = true,
                        .border_color = WLX_RGBA(255, 60, 60, 255), .border_width = 3);
                    wlx_label(ctx, "No border",
                        .font_size = 16, .height = 40, .show_background = true,
                        .border_width = 0);
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Buttons");
                wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 4);
                    wlx_button(ctx, "Dynamic",
                        .font_size = 16, .height = 44, .align = WLX_CENTER,
                        .border_color = bdr, .border_width = bw);
                    wlx_button(ctx, "Green outline",
                        .font_size = 16, .height = 44, .align = WLX_CENTER,
                        .border_color = WLX_RGBA(0, 200, 0, 255), .border_width = 2);
                    wlx_button(ctx, "Yellow thick",
                        .font_size = 16, .height = 44, .align = WLX_CENTER,
                        .border_color = WLX_RGBA(255, 220, 0, 255), .border_width = 4);
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Widgets");
                wlx_layout_begin(ctx, 4, WLX_HORZ, .padding = 4);
                    wlx_widget(ctx, .height = 50,
                        .color = WLX_RGBA(60, 20, 20, 255),
                        .border_color = WLX_RGBA(255, 100, 100, 255), .border_width = bw);
                    wlx_widget(ctx, .height = 50,
                        .color = WLX_RGBA(20, 60, 20, 255),
                        .border_color = WLX_RGBA(100, 255, 100, 255), .border_width = bw);
                    wlx_widget(ctx, .height = 50,
                        .color = WLX_RGBA(20, 20, 60, 255),
                        .border_color = WLX_RGBA(100, 100, 255, 255), .border_width = bw);
                    wlx_widget(ctx, .height = 50,
                        .color = WLX_RGBA(50, 50, 50, 255),
                        .border_color = bdr, .border_width = bw);
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Checkboxes & Inputs");
                wlx_checkbox(ctx, "Default border (from theme)", &st->border_checkbox,
                    .font_size = 18, .height = ROW_H);
                wlx_checkbox(ctx, "Custom border", &st->border_checkbox,
                    .font_size = 18, .height = ROW_H,
                    .border_color = bdr, .border_width = bw);
                wlx_inputbox(ctx, "Default:", st->border_input, sizeof(st->border_input),
                    .height = 44, .font_size = 16);
                wlx_inputbox(ctx, "Custom: ", st->border_input, sizeof(st->border_input),
                    .height = 44, .font_size = 16,
                    .border_width = bw, .border_color = bdr);

                SUB_HEADING(ctx, "Sliders");
                wlx_slider(ctx, "Default    ", &st->border_slider,
                    .height = ROW_H, .font_size = 16);
                wlx_slider(ctx, "Bordered   ", &st->border_slider,
                    .height = ROW_H, .font_size = 16,
                    .border_color = bdr, .border_width = bw);

            wlx_panel_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Auto Layout (mixed-size dynamic layouts)
// ---------------------------------------------------------------------------
static void section_auto_layout(WLX_Context *ctx, Gallery_State *st) {
    wlx_split_begin(ctx,
        .first_back_color = heading_color(ctx->theme, 8, 8, 16));

        wlx_panel_begin(ctx, .title = "Options",
            .title_back_color = heading_color(ctx->theme, 20, 12, 12));

                wlx_slider(ctx, "Items      ", &st->auto_item_count,
                    .height = ROW_H, .min_value = 1.0f, .max_value = 10.0f, .font_size = 16);
                wlx_slider(ctx, "Item H     ", &st->auto_item_height,
                    .height = ROW_H, .min_value = 20.0f, .max_value = 80.0f, .font_size = 16);
                wlx_slider(ctx, "Header %   ", &st->auto_header_pct,
                    .height = ROW_H, .min_value = 5.0f, .max_value = 40.0f, .font_size = 16);
                wlx_slider(ctx, "Sidebar %  ", &st->auto_sidebar_pct,
                    .height = ROW_H, .min_value = 10.0f, .max_value = 60.0f, .font_size = 16);

            wlx_panel_end(ctx);

    wlx_split_next(ctx);

        {
                int n1 = (int)st->auto_item_count;
                if (n1 < 1) n1 = 1;
                if (n1 > 10) n1 = 10;
                float demo1_h = n1 * st->auto_item_height + 30;

            wlx_layout_begin_s(ctx, WLX_VERT,
                WLX_SIZES(
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(demo1_h),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(60),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(200),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(60),
                    WLX_SLOT_CONTENT,
                    WLX_SLOT_PX(80)
                ));

                SECTION_HEADING(ctx, "Auto Layout (Mixed Sizes)");

                wlx_label(ctx, "Dynamic layouts with wlx_layout_auto_slot() - PX, PCT, FLEX, FILL:",
                    .font_size = 14, .height = 24);

                SUB_HEADING(ctx, "PX Items + FLEX Footer");
                {
                    int n = n1;
                    float ih = st->auto_item_height;

                    wlx_layout_begin_auto(ctx, WLX_VERT, 0);
                        for (int i = 0; i < n; i++) {
                            wlx_push_id(ctx, (size_t)i);
                            char buf[32];
                            snprintf(buf, sizeof(buf), "PX(%.0f) Item %d", ih, i + 1);
                            wlx_layout_auto_slot(ctx, WLX_SLOT_PX(ih));
                            wlx_label(ctx, buf,
                                .height = -1, .font_size = 14, .align = WLX_CENTER,
                                .show_background = true,
                                .back_color = heading_color(ctx->theme,
                                    20 + i * 5, 30 + i * 3, 40 + i * 2));
                            wlx_pop_id(ctx);
                        }
                        wlx_layout_auto_slot(ctx, WLX_SLOT_FLEX_MIN(1, 30));
                        wlx_label(ctx, "FLEX_MIN(1, 30) - fills remaining",
                            .height = -1, .font_size = 16, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 22, 52, 22));
                    wlx_layout_end(ctx);
                }

                SUB_HEADING(ctx, "PCT Column Widths");
                {
                    float pct_left = st->auto_sidebar_pct;
                    float pct_right = 100.0f - pct_left;
                    if (pct_right < 5.0f) pct_right = 5.0f;

                    wlx_layout_begin_auto(ctx, WLX_HORZ, 0);
                        char lbl_l[32], lbl_r[32];
                        snprintf(lbl_l, sizeof(lbl_l), "PCT(%.0f)", pct_left);
                        snprintf(lbl_r, sizeof(lbl_r), "PCT(%.0f)", pct_right);

                        wlx_layout_auto_slot(ctx, WLX_SLOT_PCT(pct_left));
                        wlx_label(ctx, lbl_l,
                            .height = 60, .font_size = 16, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 42, 22, 52));
                        wlx_layout_auto_slot(ctx, WLX_SLOT_PCT(pct_right));
                        wlx_label(ctx, lbl_r,
                            .height = 60, .font_size = 16, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 22, 42, 52));
                    wlx_layout_end(ctx);
                }

                SUB_HEADING(ctx, "Mixed: PCT Sidebar + FLEX Content");
                {
                    float spct = st->auto_sidebar_pct;
                    char side_lbl[48], content_lbl[48];
                    snprintf(side_lbl, sizeof(side_lbl), "PCT(%.0f) Sidebar", spct);
                    snprintf(content_lbl, sizeof(content_lbl), "FLEX(1) Content");

                    wlx_layout_begin_auto(ctx, WLX_HORZ, 0);

                        wlx_layout_auto_slot(ctx, WLX_SLOT_PCT(spct));
                        wlx_label(ctx, side_lbl,
                            .height = 200, .font_size = 14, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 42, 22, 22));

                        wlx_layout_auto_slot(ctx, WLX_SLOT_FLEX(1));
                        wlx_label(ctx, content_lbl,
                            .height = 200, .font_size = 14, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 22, 42, 32));

                    wlx_layout_end(ctx);
                }

                SUB_HEADING(ctx, "FLEX with Min/Max Constraints");
                {
                    wlx_layout_begin_auto(ctx, WLX_HORZ, 0);

                        wlx_layout_auto_slot(ctx, WLX_SLOT_FLEX_MINMAX(1, 80, 250));
                        wlx_label(ctx, "FLEX_MINMAX(1, 80, 250)",
                            .height = 60, .font_size = 13, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 52, 32, 52));

                        wlx_layout_auto_slot(ctx, WLX_SLOT_PX(120));
                        wlx_label(ctx, "PX(120)",
                            .height = 60, .font_size = 13, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 32, 52, 42));

                    wlx_layout_end(ctx);
                }

                SUB_HEADING(ctx, "PCT Vertical Slots");
                {
                    float hpct = st->auto_header_pct;
                    float rest = 100.0f - hpct;
                    if (rest < 5.0f) rest = 5.0f;

                    char lbl_h[32], lbl_b[32];
                    snprintf(lbl_h, sizeof(lbl_h), "PCT(%.0f) Header", hpct);
                    snprintf(lbl_b, sizeof(lbl_b), "PCT(%.0f) Body", rest);

                    wlx_layout_begin_auto(ctx, WLX_VERT, 0);
                        wlx_layout_auto_slot(ctx, WLX_SLOT_PCT(hpct));
                        wlx_label(ctx, lbl_h,
                            .height = -1, .font_size = 14, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 42, 32, 22));
                        wlx_layout_auto_slot(ctx, WLX_SLOT_PCT(rest));
                        wlx_label(ctx, lbl_b,
                            .height = -1, .font_size = 14, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 22, 32, 42));
                    wlx_layout_end(ctx);
                }

            wlx_layout_end(ctx);
        }

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Progress / Toggle / Radio
// ---------------------------------------------------------------------------
static void section_progress_toggle_radio(WLX_Context *ctx, Gallery_State *st) {
    wlx_split_begin(ctx,
        .first_back_color = heading_color(ctx->theme, 8, 8, 16));

        wlx_panel_begin(ctx, .title = "Options",
            .title_back_color = heading_color(ctx->theme, 20, 12, 12));

            wlx_slider(ctx, "Progress  ", &st->progress_value,
                .height = 36, .font_size = 14);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

        wlx_panel_begin(ctx, .title = "Progress / Toggle / Radio",
            .title_font_size = 26, .title_height = 48,
            .title_back_color = heading_color(ctx->theme, 17, 12, 32),
            .padding = 0);

            wlx_label(ctx, "Phase 2 widgets: progress bar, toggle switch, radio button.",
                .font_size = 16, .height = 30);

            SUB_HEADING(ctx, "Progress Bar");
            wlx_progress(ctx, st->progress_value, .height = ROW_H);

            {
                char buf[64];
                snprintf(buf, sizeof(buf), "Value: %.0f%%", st->progress_value * 100.0f);
                wlx_label(ctx, buf, .height = 28, .font_size = 14);
            }

            wlx_progress(ctx, 0.0f, .height = 28);
            wlx_progress(ctx, 1.0f, .height = 28);

            SUB_HEADING(ctx, "Toggle Switch");
            wlx_toggle(ctx, "Enable notifications", &st->toggle_a, .height = ROW_H);
            wlx_toggle(ctx, "Dark mode", &st->toggle_b, .height = ROW_H);

            SUB_HEADING(ctx, "Radio Buttons");
            wlx_radio(ctx, "Small",  &st->radio_choice, 0, .height = ROW_H);
            wlx_radio(ctx, "Medium", &st->radio_choice, 1, .height = ROW_H);
            wlx_radio(ctx, "Large",  &st->radio_choice, 2, .height = ROW_H);

            {
                char buf[64];
                snprintf(buf, sizeof(buf), "Selected: %d", st->radio_choice);
                wlx_label(ctx, buf, .height = 28, .font_size = 14);
            }

        wlx_panel_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
static WLX_Context *ctx;
static WLX_Theme theme_dark;
static WLX_Theme theme_light;

// ---------------------------------------------------------------------------
// Exported: wlx_wasm_init
// ---------------------------------------------------------------------------
__attribute__((export_name("wlx_wasm_init")))
void wlx_wasm_init(void) {
    theme_dark  = wlx_theme_dark;
    theme_light = wlx_theme_light;

    ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_wasm(ctx);

    for (int i = 0; i < 10; i++) {
        snprintf(g.dynamic_names[i], sizeof(g.dynamic_names[i]), "Panel %d", i + 1);
    }
}

// ---------------------------------------------------------------------------
// Exported: wlx_wasm_frame
// ---------------------------------------------------------------------------
__attribute__((export_name("wlx_wasm_frame")))
void wlx_wasm_frame(float width, float height) {
    WLX_Rect root = { 0, 0, width, height };

    ctx->theme = g.dark_mode ? &theme_dark : &theme_light;

    wlx_begin(ctx, root, wlx_process_wasm_input);

    // Draw background
    ctx->backend.draw_rect(root, ctx->theme->background);

    // ---- Root: title + body + status bar ----
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(44), WLX_SLOT_FLEX(1), WLX_SLOT_CONTENT));

        // -- Title bar --
        wlx_layout_begin_s(ctx, WLX_HORZ,
            WLX_SIZES(WLX_SLOT_FLEX(1), WLX_SLOT_PX(160)));
            wlx_label(ctx, "  Wollix Widget Gallery",
                .font_size = 24, .align = WLX_LEFT,
                .back_color = heading_color(ctx->theme, 10, 10, 20));

            if (wlx_button(ctx, g.dark_mode ? "Light Mode" : "Dark Mode",
                .font_size = 16, .align = WLX_CENTER,
                .back_color = heading_color(ctx->theme, 32, 32, 47))) {
                g.dark_mode = !g.dark_mode;
            }
        wlx_layout_end(ctx);

        // -- Body: sidebar + content --
        wlx_layout_begin_s(ctx, WLX_HORZ,
            WLX_SIZES(WLX_SLOT_PX(200), WLX_SLOT_FLEX(1)));

            // -- Sidebar --
            wlx_scroll_panel_begin(ctx, -1,
                .back_color = heading_color(ctx->theme, 4, 4, 10));
                wlx_layout_begin_auto(ctx, WLX_VERT, 36);
                    for (int i = 0; i < SECTION_COUNT; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        WLX_Color btn_bg = (i == g.active_section)
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

            // -- Content area --
            wlx_scroll_panel_begin(ctx, -1);
                if (g.active_section >= 0 && g.active_section < SECTION_COUNT) {
                    sections[g.active_section].render(ctx, &g);
                }
            wlx_scroll_panel_end(ctx);

        wlx_layout_end(ctx);

        // -- Status bar --
        {
            float ft = ctx->backend.get_frame_time();
            static float smoothed_fps = 0.0f;
            if (ft > 0.0001f) {
                float instant_fps = 1.0f / ft;
                if (smoothed_fps <= 0.0f) {
                    smoothed_fps = instant_fps;
                } else {
                    // Smooth browser frame jitter without delaying rendering.
                    smoothed_fps += 0.02f * (instant_fps - smoothed_fps);
                }
            }

            int fps = (smoothed_fps > 0.0f) ? (int)(smoothed_fps + 0.5f) : 0;
            char status[256];
            snprintf(status, sizeof(status),
                "  Theme: %s  |  Section: %s  |  FPS: %d",
                g.dark_mode ? "Dark" : "Light",
                sections[g.active_section].name,
                fps);
            wlx_label(ctx, status,
                .font_size = 13, .height = 24, .align = WLX_LEFT,
                .back_color = heading_color(ctx->theme, 4, 4, 10));
        }

    wlx_layout_end(ctx);

    wlx_end(ctx);
}
