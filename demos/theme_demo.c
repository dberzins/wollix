// theme_demo.c - Runtime theme builder for wollix
// Demonstrates copying a built-in WLX_Theme, overriding fields, and assigning
// ctx->theme before wlx_begin() so widgets inherit the active theme.

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

#define WINDOW_WIDTH  1120
#define WINDOW_HEIGHT 760
#define TARGET_FPS    60

#define PANEL_BORDER_WIDTH 0.5f
#define ROW_H 34.0f
#define SMALL_ROW_H 28.0f
#define PREVIEW_CARD_H 174.0f
#define SWATCH_ROW_H 68.0f

typedef enum {
    THEME_MODE_DARK = 0,
    THEME_MODE_LIGHT,
    THEME_MODE_GLASS,
    THEME_MODE_CUSTOM,
} Theme_Mode;

typedef enum {
    THEME_BASE_DARK = 0,
    THEME_BASE_LIGHT,
    THEME_BASE_GLASS,
} Theme_Base;

typedef struct {
    float r;
    float g;
    float b;
} Color_Edit;

typedef struct {
    Theme_Mode active_mode;
    Theme_Base custom_base;
    Color_Edit background;
    Color_Edit foreground;
    Color_Edit surface;
    Color_Edit border;
    Color_Edit accent;
    float border_width;
    float roundness;
    float hover_brightness;
    float padding;
    bool checkbox_value;
    bool toggle_value;
    int radio_value;
    float slider_value;
    float progress_value;
    char input_buf[128];
} App_State;

static App_State app;

static float clamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static uint8_t channel_from_float(float value) {
    return (uint8_t)(clamp01(value) * 255.0f + 0.5f);
}

static Color_Edit color_edit_from_color(WLX_Color color) {
    return (Color_Edit){
        .r = (float)color.r / 255.0f,
        .g = (float)color.g / 255.0f,
        .b = (float)color.b / 255.0f,
    };
}

static WLX_Color color_from_edit(Color_Edit color) {
    return (WLX_Color){
        channel_from_float(color.r),
        channel_from_float(color.g),
        channel_from_float(color.b),
        255,
    };
}

static WLX_Color blend_color(WLX_Color a, WLX_Color b, uint8_t amount) {
    int inverse = 255 - amount;
    return (WLX_Color){
        (uint8_t)(((int)a.r * inverse + (int)b.r * amount) / 255),
        (uint8_t)(((int)a.g * inverse + (int)b.g * amount) / 255),
        (uint8_t)(((int)a.b * inverse + (int)b.b * amount) / 255),
        (uint8_t)(((int)a.a * inverse + (int)b.a * amount) / 255),
    };
}

static WLX_Color text_on_color(WLX_Color color) {
    int luminance = (int)color.r * 299 + (int)color.g * 587 + (int)color.b * 114;
    return luminance > 145000
        ? (WLX_Color){20, 24, 30, 255}
        : (WLX_Color){246, 248, 252, 255};
}

static const WLX_Theme *theme_base_ptr(Theme_Base base) {
    switch (base) {
        case THEME_BASE_LIGHT: return &wlx_theme_light;
        case THEME_BASE_GLASS: return &wlx_theme_glass;
        case THEME_BASE_DARK:
        default: return &wlx_theme_dark;
    }
}

static const char *theme_mode_name(Theme_Mode mode) {
    switch (mode) {
        case THEME_MODE_LIGHT: return "Light";
        case THEME_MODE_GLASS: return "Glass";
        case THEME_MODE_CUSTOM: return "Custom";
        case THEME_MODE_DARK:
        default: return "Dark";
    }
}

static void load_custom_from_theme(App_State *state, Theme_Base base) {
    const WLX_Theme *theme = theme_base_ptr(base);
    state->custom_base = base;
    state->background = color_edit_from_color(theme->background);
    state->foreground = color_edit_from_color(theme->foreground);
    state->surface = color_edit_from_color(theme->surface);
    state->border = color_edit_from_color(theme->border);
    state->accent = color_edit_from_color(theme->accent);
    state->border_width = theme->border_width > 0.0f ? theme->border_width : 0.5f;
    state->roundness = theme->roundness;
    state->hover_brightness = theme->hover_brightness;
    state->padding = theme->padding;
}

static void init_app_state(App_State *state) {
    memset(state, 0, sizeof(*state));
    state->active_mode = THEME_MODE_CUSTOM;
    state->checkbox_value = true;
    state->toggle_value = true;
    state->radio_value = 1;
    state->slider_value = 0.58f;
    state->progress_value = 0.68f;
    snprintf(state->input_buf, sizeof(state->input_buf), "Make it yours");

    load_custom_from_theme(state, THEME_BASE_DARK);
    state->background = color_edit_from_color((WLX_Color){18, 20, 20, 255});
    state->foreground = color_edit_from_color((WLX_Color){230, 233, 226, 255});
    state->surface = color_edit_from_color((WLX_Color){35, 39, 39, 255});
    state->border = color_edit_from_color((WLX_Color){58, 68, 64, 255});
    state->accent = color_edit_from_color((WLX_Color){96, 190, 174, 255});
    state->border_width = 0.5f;
    state->roundness = 0.0f;
    state->hover_brightness = 0.07f;
    state->padding = 2.0f;
}

static WLX_Theme make_custom_theme(const App_State *state, WLX_Font font) {
    WLX_Theme theme = *theme_base_ptr(state->custom_base);

    theme.background = color_from_edit(state->background);
    theme.foreground = color_from_edit(state->foreground);
    theme.surface = color_from_edit(state->surface);
    theme.border = color_from_edit(state->border);
    theme.accent = color_from_edit(state->accent);
    theme.border_width = state->border_width;
    theme.padding = state->padding;
    theme.roundness = state->roundness;
    theme.rounded_segments = state->roundness > 0.0f ? 8 : 0;
    theme.hover_brightness = state->hover_brightness;
    theme.font = font;

    theme.input.border_focus = theme.accent;
    theme.input.cursor = theme.foreground;
    theme.input.border_width = theme.border_width > 0.0f ? theme.border_width : 0.5f;

    theme.slider.track = blend_color(theme.surface, theme.background, 96);
    theme.slider.thumb = theme.foreground;
    theme.slider.label = theme.foreground;

    theme.checkbox.border = theme.border;
    theme.checkbox.check = theme.accent;
    theme.toggle.track_active = theme.accent;
    theme.radio.fill = theme.accent;
    theme.progress.track = theme.slider.track;
    theme.progress.fill = theme.accent;
    theme.scrollbar.bar = theme.slider.track;

    return theme;
}

static const WLX_Theme *active_theme_ptr(Theme_Mode mode, const WLX_Theme *custom_theme) {
    switch (mode) {
        case THEME_MODE_LIGHT: return &wlx_theme_light;
        case THEME_MODE_GLASS: return &wlx_theme_glass;
        case THEME_MODE_CUSTOM: return custom_theme;
        case THEME_MODE_DARK:
        default: return &wlx_theme_dark;
    }
}

static void theme_button(WLX_Context *ctx, App_State *state, const char *label,
    Theme_Mode mode, size_t id) {
    bool selected = state->active_mode == mode;
    WLX_Color back = selected ? ctx->theme->accent : (WLX_Color){0};
    WLX_Color front = selected ? text_on_color(ctx->theme->accent) : (WLX_Color){0};

    wlx_push_id(ctx, id);
        if (wlx_button(ctx, label,
            .height = ROW_H,
            .font_size = 15,
            .align = WLX_CENTER,
            .back_color = back,
            .front_color = front,
            .border_color = selected ? ctx->theme->accent : ctx->theme->border,
            .border_width = PANEL_BORDER_WIDTH)) {
            state->active_mode = mode;
        }
    wlx_pop_id(ctx);
}

static void reset_button(WLX_Context *ctx, App_State *state, const char *label,
    Theme_Base base, size_t id) {
    bool selected = state->custom_base == base;
    WLX_Color back = selected ? blend_color(ctx->theme->surface, ctx->theme->accent, 72) : (WLX_Color){0};

    wlx_push_id(ctx, id);
        if (wlx_button(ctx, label,
            .height = SMALL_ROW_H,
            .font_size = 13,
            .align = WLX_CENTER,
            .back_color = back,
            .border_color = selected ? ctx->theme->accent : ctx->theme->border,
            .border_width = PANEL_BORDER_WIDTH)) {
            load_custom_from_theme(state, base);
            state->active_mode = THEME_MODE_CUSTOM;
        }
    wlx_pop_id(ctx);
}

static void section_label(WLX_Context *ctx, const char *text) {
    wlx_push_id(ctx, wlx_hash_string(text));
        wlx_label(ctx, text,
            .height = 24,
            .font_size = 14,
            .align = WLX_LEFT,
            .front_color = blend_color(ctx->theme->foreground, ctx->theme->surface, 56),
            .show_background = true,
            .back_color = blend_color(ctx->theme->surface, ctx->theme->background, 48));
    wlx_pop_id(ctx);
}

static void color_controls(WLX_Context *ctx, const char *name, Color_Edit *color, size_t id) {
    WLX_Color preview = color_from_edit(*color);

    wlx_push_id(ctx, id);
        wlx_label(ctx, name,
            .height = 22,
            .font_size = 13,
            .align = WLX_LEFT,
            .front_color = ctx->theme->foreground);

        wlx_layout_begin_s(ctx, WLX_HORZ,
            WLX_SIZES(WLX_SLOT_PX(42), WLX_SLOT_FLEX(1)),
            .padding = 0,
            .gap = 6);

            wlx_widget(ctx,
                .height = 70,
                .color = preview,
                .border_color = ctx->theme->border,
                .border_width = PANEL_BORDER_WIDTH);

            wlx_layout_begin(ctx, 3, WLX_VERT, .padding = 0, .gap = 2);
                wlx_slider(ctx, "R", &color->r,
                    .height = 22,
                    .font_size = 12,
                    .thumb_color = (WLX_Color){230, 88, 80, 255},
                    .show_label = false);
                wlx_slider(ctx, "G", &color->g,
                    .height = 22,
                    .font_size = 12,
                    .thumb_color = (WLX_Color){80, 190, 120, 255},
                    .show_label = false);
                wlx_slider(ctx, "B", &color->b,
                    .height = 22,
                    .font_size = 12,
                    .thumb_color = (WLX_Color){80, 130, 230, 255},
                    .show_label = false);
            wlx_layout_end(ctx);

        wlx_layout_end(ctx);
    wlx_pop_id(ctx);
}

static void controls_panel(WLX_Context *ctx, App_State *state) {
    wlx_panel_begin(ctx,
        .title = "Theme Controls",
        .title_height = 34,
        .title_font_size = 16,
        .title_back_color = blend_color(ctx->theme->surface, ctx->theme->accent, 36),
        .back_color = ctx->theme->surface,
        .border_color = ctx->theme->border,
        .border_width = PANEL_BORDER_WIDTH,
        .padding = 6,
        .gap = 5,
        .capacity = 64);

        section_label(ctx, "Active theme");
        wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 0, .gap = 4);
            theme_button(ctx, state, "Dark", THEME_MODE_DARK, 1);
            theme_button(ctx, state, "Light", THEME_MODE_LIGHT, 2);
        wlx_layout_end(ctx);
        wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 0, .gap = 4);
            theme_button(ctx, state, "Glass", THEME_MODE_GLASS, 3);
            theme_button(ctx, state, "Custom", THEME_MODE_CUSTOM, 4);
        wlx_layout_end(ctx);

        wlx_separator(ctx, .height = 8, .thickness = PANEL_BORDER_WIDTH);

        section_label(ctx, "Custom starts from");
        wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 0, .gap = 4);
            reset_button(ctx, state, "Dark", THEME_BASE_DARK, 10);
            reset_button(ctx, state, "Light", THEME_BASE_LIGHT, 11);
            reset_button(ctx, state, "Glass", THEME_BASE_GLASS, 12);
        wlx_layout_end(ctx);

        wlx_separator(ctx, .height = 8, .thickness = PANEL_BORDER_WIDTH);

        section_label(ctx, "Global colors");
        color_controls(ctx, "background", &state->background, 20);
        color_controls(ctx, "surface", &state->surface, 21);
        color_controls(ctx, "foreground", &state->foreground, 22);
        color_controls(ctx, "border", &state->border, 23);
        color_controls(ctx, "accent", &state->accent, 24);

        wlx_separator(ctx, .height = 8, .thickness = PANEL_BORDER_WIDTH);

        section_label(ctx, "Geometry and feedback");
        wlx_slider(ctx, "border", &state->border_width,
            .height = ROW_H,
            .font_size = 13,
            .min_value = 0.0f,
            .max_value = 3.0f);
        wlx_slider(ctx, "round", &state->roundness,
            .height = ROW_H,
            .font_size = 13,
            .min_value = 0.0f,
            .max_value = 0.6f);
        wlx_slider(ctx, "hover", &state->hover_brightness,
            .height = ROW_H,
            .font_size = 13,
            .min_value = -0.25f,
            .max_value = 0.35f);
        wlx_slider(ctx, "padding", &state->padding,
            .height = ROW_H,
            .font_size = 13,
            .min_value = 0.0f,
            .max_value = 12.0f);

    wlx_panel_end(ctx);
}

static void tiny_preview_widgets(WLX_Context *ctx) {
    static bool preview_check = true;
    static float preview_slider = 0.58f;

    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(28), WLX_SLOT_PX(28), WLX_SLOT_PX(28)),
        .padding = 4,
        .gap = 4);
        wlx_button(ctx, "Button", .height = 28, .font_size = 12, .align = WLX_CENTER);
        wlx_slider(ctx, "Value", &preview_slider, .height = 28, .font_size = 12);
        wlx_checkbox(ctx, "Check", &preview_check, .height = 28, .font_size = 12);
    wlx_layout_end(ctx);
}

static void preview_card(WLX_Context *ctx, const char *name, const char *tag,
    const WLX_Theme *theme, bool selected, size_t id) {
    const WLX_Theme *saved = ctx->theme;
    WLX_Color selected_border = selected ? theme->accent : theme->border;
    WLX_Color header = selected
        ? blend_color(theme->surface, theme->accent, 96)
        : blend_color(theme->surface, theme->background, 36);

    ctx->theme = theme;
    wlx_push_id(ctx, id);
        wlx_layout_begin_s(ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_PX(28), WLX_SLOT_PX(18), WLX_SLOT_PX(24), WLX_SLOT_FLEX(1)),
            .padding = 5,
            .gap = 4,
            .back_color = theme->background,
            .border_color = selected_border,
            .border_width = selected ? 1.5f : PANEL_BORDER_WIDTH);

            wlx_label(ctx, name,
                .height = 28,
                .font_size = 15,
                .align = WLX_CENTER,
                .show_background = true,
                .back_color = header,
                .front_color = text_on_color(header));
            wlx_label(ctx, tag,
                .height = 18,
                .font_size = 11,
                .align = WLX_CENTER,
                .front_color = blend_color(theme->foreground, theme->surface, 92));
            wlx_layout_begin(ctx, 5, WLX_HORZ, .padding = 0, .gap = 3);
                wlx_widget(ctx, .height = 24, .color = theme->background,
                    .border_color = theme->border, .border_width = PANEL_BORDER_WIDTH);
                wlx_widget(ctx, .height = 24, .color = theme->surface,
                    .border_color = theme->border, .border_width = PANEL_BORDER_WIDTH);
                wlx_widget(ctx, .height = 24, .color = theme->foreground,
                    .border_color = theme->border, .border_width = PANEL_BORDER_WIDTH);
                wlx_widget(ctx, .height = 24, .color = theme->border,
                    .border_color = theme->border, .border_width = PANEL_BORDER_WIDTH);
                wlx_widget(ctx, .height = 24, .color = theme->accent,
                    .border_color = theme->border, .border_width = PANEL_BORDER_WIDTH);
            wlx_layout_end(ctx);
            tiny_preview_widgets(ctx);

        wlx_layout_end(ctx);
    wlx_pop_id(ctx);
    ctx->theme = saved;
}

static int responsive_columns(WLX_Context *ctx, int max_columns, float min_cell_width) {
    float available = wlx_get_available_width(ctx);
    int columns = (int)(available / min_cell_width);
    if (columns < 1) columns = 1;
    if (columns > max_columns) columns = max_columns;
    return columns;
}

static float grid_height(int item_count, int columns, float row_height, float gap) {
    if (item_count < 1) return 0.0f;
    if (columns < 1) columns = 1;

    int rows = (item_count + columns - 1) / columns;
    float total_gap = rows > 1 ? (float)(rows - 1) * gap : 0.0f;
    return (float)rows * row_height + total_gap;
}

static void token_swatch(WLX_Context *ctx, const char *name, WLX_Color color, size_t id) {
    wlx_push_id(ctx, id);
        wlx_layout_begin_s(ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_PX(34), WLX_SLOT_PX(20)),
            .padding = 4,
            .gap = 3,
            .back_color = ctx->theme->surface,
            .border_color = ctx->theme->border,
            .border_width = PANEL_BORDER_WIDTH);
            wlx_widget(ctx,
                .height = 34,
                .color = color,
                .border_color = ctx->theme->border,
                .border_width = PANEL_BORDER_WIDTH);
            wlx_label(ctx, name,
                .height = 20,
                .font_size = 11,
                .align = WLX_CENTER,
                .front_color = ctx->theme->foreground);
        wlx_layout_end(ctx);
    wlx_pop_id(ctx);
}

static void token_swatches(WLX_Context *ctx) {
    const WLX_Theme *theme = ctx->theme;
    const struct {
        const char *name;
        WLX_Color color;
    } swatches[] = {
        { "background", theme->background },
        { "surface", theme->surface },
        { "foreground", theme->foreground },
        { "border", theme->border },
        { "accent", theme->accent },
        { "input focus", theme->input.border_focus },
        { "slider", theme->slider.track },
        { "progress", theme->progress.fill },
    };

    int count = (int)(sizeof(swatches) / sizeof(swatches[0]));
    int columns = responsive_columns(ctx, 4, 132.0f);
    wlx_grid_begin_auto(ctx, (size_t)columns, SWATCH_ROW_H,
        .padding = 0,
        .gap = 5,
        .slot_border_width = 0.0f);
        for (int i = 0; i < count; i++) {
            token_swatch(ctx, swatches[i].name, swatches[i].color, (size_t)i);
        }
    wlx_grid_end(ctx);
}

static void sample_widgets(WLX_Context *ctx, App_State *state) {
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H)
        ),
        .padding = 6,
        .gap = 5,
        .back_color = blend_color(ctx->theme->surface, ctx->theme->background, 52),
        .border_color = ctx->theme->border,
        .border_width = PANEL_BORDER_WIDTH);

        wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 0, .gap = 5);
            wlx_button(ctx, "Theme default", .height = ROW_H, .align = WLX_CENTER);
            wlx_button(ctx, "Accent override",
                .height = ROW_H,
                .align = WLX_CENTER,
                .back_color = ctx->theme->accent,
                .front_color = text_on_color(ctx->theme->accent));
        wlx_layout_end(ctx);

        wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 0, .gap = 5);
            wlx_checkbox(ctx, "Checkbox", &state->checkbox_value, .height = ROW_H);
            wlx_toggle(ctx, "Toggle", &state->toggle_value, .height = ROW_H);
        wlx_layout_end(ctx);

        wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 0, .gap = 5);
            wlx_radio(ctx, "Low", &state->radio_value, 0, .height = ROW_H);
            wlx_radio(ctx, "Mid", &state->radio_value, 1, .height = ROW_H);
            wlx_radio(ctx, "High", &state->radio_value, 2, .height = ROW_H);
        wlx_layout_end(ctx);

        wlx_slider(ctx, "Slider", &state->slider_value,
            .height = ROW_H,
            .min_value = 0.0f,
            .max_value = 1.0f);
        wlx_progress(ctx, state->progress_value, .height = ROW_H);
        wlx_inputbox(ctx, "Input:", state->input_buf, sizeof(state->input_buf), .height = ROW_H);

    wlx_layout_end(ctx);
}

static void recipe_panel(WLX_Context *ctx) {
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(
            WLX_SLOT_PX(22),
            WLX_SLOT_PX(22),
            WLX_SLOT_PX(22),
            WLX_SLOT_PX(22),
            WLX_SLOT_PX(22)
        ),
        .padding = 6,
        .gap = 3,
        .back_color = blend_color(ctx->theme->surface, ctx->theme->background, 72),
        .border_color = ctx->theme->border,
        .border_width = PANEL_BORDER_WIDTH);

        wlx_label(ctx, "WLX_Theme theme = wlx_theme_dark;",
            .height = 22,
            .font_size = 12,
            .align = WLX_LEFT,
            .front_color = ctx->theme->foreground);
        wlx_label(ctx, "theme.background = custom_background;",
            .height = 22,
            .font_size = 12,
            .align = WLX_LEFT,
            .front_color = ctx->theme->foreground);
        wlx_label(ctx, "theme.input.border_focus = theme.accent;",
            .height = 22,
            .font_size = 12,
            .align = WLX_LEFT,
            .front_color = ctx->theme->foreground);
        wlx_label(ctx, "theme.progress.fill = theme.accent;",
            .height = 22,
            .font_size = 12,
            .align = WLX_LEFT,
            .front_color = ctx->theme->foreground);
        wlx_label(ctx, "ctx->theme = &theme;",
            .height = 22,
            .font_size = 12,
            .align = WLX_LEFT,
            .front_color = ctx->theme->accent);

    wlx_layout_end(ctx);
}

static void preview_panel(WLX_Context *ctx, App_State *state, const WLX_Theme *custom_theme) {
    int preset_columns = responsive_columns(ctx, 4, 152.0f);
    int token_columns = responsive_columns(ctx, 4, 132.0f);
    float preset_grid_h = grid_height(4, preset_columns, PREVIEW_CARD_H, 5.0f);
    float token_grid_h = grid_height(8, token_columns, SWATCH_ROW_H, 5.0f);

    wlx_panel_begin(ctx,
        .title = "Live Preview",
        .title_height = 34,
        .title_font_size = 16,
        .title_back_color = blend_color(ctx->theme->surface, ctx->theme->accent, 36),
        .back_color = ctx->theme->surface,
        .border_color = ctx->theme->border,
        .border_width = PANEL_BORDER_WIDTH,
        .padding = 6,
        .gap = 8,
        .capacity = 32);

        wlx_layout_begin_s(ctx, WLX_VERT,
            WLX_SIZES(
                WLX_SLOT_PX(30),
                WLX_SLOT_PX(preset_grid_h),
                WLX_SLOT_PX(24),
                WLX_SLOT_PX(token_grid_h),
                WLX_SLOT_PX(24),
                WLX_SLOT_PX(248),
                WLX_SLOT_PX(24),
                WLX_SLOT_PX(136)
            ),
            .padding = 0,
            .gap = 6);

            {
                char active_label[96];
                snprintf(active_label, sizeof(active_label), "Active theme: %s", theme_mode_name(state->active_mode));
                wlx_label(ctx, active_label,
                    .height = 30,
                    .font_size = 15,
                    .align = WLX_LEFT,
                    .front_color = ctx->theme->foreground,
                    .show_background = true,
                    .back_color = blend_color(ctx->theme->surface, ctx->theme->background, 42));
            }

            wlx_grid_begin_auto(ctx, (size_t)preset_columns, PREVIEW_CARD_H,
                .padding = 0,
                .gap = 5);
                preview_card(ctx, "Dark", "built-in", &wlx_theme_dark,
                    state->active_mode == THEME_MODE_DARK, 100);
                preview_card(ctx, "Light", "built-in", &wlx_theme_light,
                    state->active_mode == THEME_MODE_LIGHT, 101);
                preview_card(ctx, "Glass", "built-in", &wlx_theme_glass,
                    state->active_mode == THEME_MODE_GLASS, 102);
                preview_card(ctx, "Custom", "editable", custom_theme,
                    state->active_mode == THEME_MODE_CUSTOM, 103);
            wlx_grid_end(ctx);

            section_label(ctx, "Active theme tokens");
            token_swatches(ctx);

            section_label(ctx, "Theme-aware widgets");
            sample_widgets(ctx, state);

            section_label(ctx, "Source pattern");
            recipe_panel(ctx);

        wlx_layout_end(ctx);

    wlx_panel_end(ctx);
}

int main(void) {
    printf("Theme demo - edit a custom WLX_Theme at runtime\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix Theme Demo");
    SetTargetFPS(TARGET_FPS);

    init_app_state(&app);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    while (!WindowShouldClose()) {
        float w = (float)GetRenderWidth();
        float h = (float)GetRenderHeight();
        WLX_Rect root = {0, 0, w, h};

        WLX_Font active_font = ctx->theme ? ctx->theme->font : WLX_FONT_DEFAULT;
        WLX_Theme custom_theme = make_custom_theme(&app, active_font);
        ctx->theme = active_theme_ptr(app.active_mode, &custom_theme);

        wlx_begin(ctx, root, wlx_process_raylib_input);
        BeginDrawing();

            ClearBackground((Color){
                ctx->theme->background.r,
                ctx->theme->background.g,
                ctx->theme->background.b,
                ctx->theme->background.a,
            });

            wlx_layout_begin_s(ctx, WLX_VERT,
                WLX_SIZES(WLX_SLOT_PX(68), WLX_SLOT_FLEX(1)),
                .padding = 10,
                .gap = 8,
                .back_color = ctx->theme->background);

                wlx_layout_begin_s(ctx, WLX_HORZ,
                    WLX_SIZES(WLX_SLOT_FLEX(1), WLX_SLOT_PX(190)),
                    .padding = 8,
                    .gap = 8,
                    .back_color = ctx->theme->surface,
                    .border_color = ctx->theme->border,
                    .border_width = PANEL_BORDER_WIDTH);

                    wlx_layout_begin_s(ctx, WLX_VERT,
                        WLX_SIZES(WLX_SLOT_PX(34), WLX_SLOT_PX(18)),
                        .padding = 0,
                        .gap = 2);
                        wlx_label(ctx, "Wollix Theme Builder",
                            .height = 34,
                            .font_size = 24,
                            .align = WLX_LEFT,
                            .front_color = ctx->theme->foreground);
                        wlx_label(ctx, "Copy a preset, override fields, assign ctx->theme each frame.",
                            .height = 18,
                            .font_size = 13,
                            .align = WLX_LEFT,
                            .front_color = blend_color(ctx->theme->foreground, ctx->theme->surface, 78));
                    wlx_layout_end(ctx);

                    if (wlx_button(ctx, "Use Custom Theme",
                        .height = 38,
                        .font_size = 15,
                        .align = WLX_CENTER,
                        .widget_align = WLX_CENTER,
                        .back_color = ctx->theme->accent,
                        .front_color = text_on_color(ctx->theme->accent))) {
                        app.active_mode = THEME_MODE_CUSTOM;
                    }

                wlx_layout_end(ctx);

                wlx_split_begin(ctx,
                    .first_size = WLX_SLOT_PCT_MINMAX(34, 320, 390),
                    .padding = 0,
                    .gap = 8,
                    .first_back_color = ctx->theme->background,
                    .second_back_color = ctx->theme->background);

                    controls_panel(ctx, &app);

                wlx_split_next(ctx, .back_color = ctx->theme->background);

                    preview_panel(ctx, &app, &custom_theme);

                wlx_split_end(ctx);

            wlx_layout_end(ctx);

        wlx_end(ctx);
        EndDrawing();
    }

    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
