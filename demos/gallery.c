// gallery.c - Widget gallery demo for wollix.
// Showcases every widget, layout mode, and theming option with live tweaking.
// One source compiled three ways via WLX_GALLERY_RAYLIB / _SDL3 / _WASM.
// Build: make gallery (Raylib), make gallery_sdl3 (SDL3), make wasm-site (WASM).

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ---------------------------------------------------------------------------
// Backend selection
// ---------------------------------------------------------------------------
#if !defined(WLX_GALLERY_RAYLIB) && !defined(WLX_GALLERY_SDL3) && !defined(WLX_GALLERY_WASM)
#define WLX_GALLERY_RAYLIB
#endif

// ---------------------------------------------------------------------------
// Platform block -- backend headers and wollix implementation.
// All three branches (Raylib, SDL3, WASM) are populated.
// ---------------------------------------------------------------------------
#if defined(WLX_GALLERY_RAYLIB)
    #include <raylib.h>
#elif defined(WLX_GALLERY_SDL3)
    #include <SDL3/SDL.h>
    #include <SDL3_ttf/SDL_ttf.h>
#elif defined(WLX_GALLERY_WASM)
    // Bare-wasm32: no libc beyond the shim; headers come from web/libc/
#else
    #error "No gallery backend selected"
#endif

#define WLX_SLOT_MINMAX_REDISTRIBUTE
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"

#if defined(WLX_GALLERY_RAYLIB)
    #include "wollix_raylib.h"
#elif defined(WLX_GALLERY_SDL3)
    #include "wollix_sdl3.h"
#elif defined(WLX_GALLERY_WASM)
    #include "wollix_wasm.h"
#endif

#include "assets/wlx_icons.h"

// ---------------------------------------------------------------------------
// Platform contract (implemented in the platform block at end of file)
// ---------------------------------------------------------------------------
typedef struct Gallery_State Gallery_State;

typedef enum {
    GALLERY_FONT_TITLE,
    GALLERY_FONT_BODY,
    GALLERY_FONT_SMALL,
} Gallery_Font_Slot;

static bool     gallery_platform_init(Gallery_State *g);
static void     gallery_platform_shutdown(Gallery_State *g);
static bool     gallery_platform_begin_frame(Gallery_State *g, WLX_Rect *out_root);
static void     gallery_platform_end_frame(Gallery_State *g);
static void     gallery_platform_clear(WLX_Color bg);
static int      gallery_platform_fps(void);
static WLX_Font gallery_platform_font(Gallery_Font_Slot slot);

static void        gallery_icon_atlas_create(WLX_Context *ctx);
static void        gallery_icon_atlas_destroy(void);
static WLX_Texture gallery_icon_atlas_texture(void);
static bool        gallery_icon_atlas_ready(void);

#define ROW_H 40

// ---------------------------------------------------------------------------
// Gallery-level color constants (backend-neutral, match Raylib's RED/GREEN/BLUE)
// ---------------------------------------------------------------------------
#define GALLERY_RED   WLX_RGBA(230, 41,  55,  255)
#define GALLERY_GREEN WLX_RGBA(0,   228, 48,  255)
#define GALLERY_WARNING WLX_RGBA(255, 184, 0, 255)
#define GALLERY_BLUE  WLX_RGBA(0,   121, 241, 255)

// ---------------------------------------------------------------------------
// Backend name and version strings
// ---------------------------------------------------------------------------
#if defined(WLX_GALLERY_RAYLIB)
    #define GALLERY_BACKEND "Raylib"
#elif defined(WLX_GALLERY_SDL3)
    #define GALLERY_BACKEND "SDL3"
#elif defined(WLX_GALLERY_WASM)
    #define GALLERY_BACKEND "WASM"
#else
    #define GALLERY_BACKEND "Unknown"
#endif

#define GALLERY_VERSION WOLLIX_VERSION
#define GALLERY_FONT_PATH "demos/assets/LiberationSans-Regular.ttf"

// ---------------------------------------------------------------------------
// Section forward declarations
// ---------------------------------------------------------------------------
typedef void (*SectionFn)(WLX_Context *ctx, Gallery_State *g);

static void section_overview(WLX_Context *ctx, Gallery_State *st);
static void section_tokens(WLX_Context *ctx, Gallery_State *st);

typedef struct {
    const char *name;
    SectionFn   render;
    WLX_Icon    icon;
} Section;

static void section_label(WLX_Context *ctx, Gallery_State *g);
static void section_button(WLX_Context *ctx, Gallery_State *g);
static void section_checkbox(WLX_Context *ctx, Gallery_State *g);
static void section_image(WLX_Context *ctx, Gallery_State *g);
static void section_slider(WLX_Context *ctx, Gallery_State *g);
static void section_inputbox(WLX_Context *ctx, Gallery_State *g);
static void section_scroll_panel(WLX_Context *ctx, Gallery_State *g);
static void section_widget(WLX_Context *ctx, Gallery_State *g);
static void section_layout_linear(WLX_Context *ctx, Gallery_State *g);
static void section_layout_grid(WLX_Context *ctx, Gallery_State *g);
static void section_layout_flex(WLX_Context *ctx, Gallery_State *g);
static void section_theme_lab(WLX_Context *ctx, Gallery_State *g);
static void section_opacity(WLX_Context *ctx, Gallery_State *g);
static void section_id_stack(WLX_Context *ctx, Gallery_State *g);
static void section_borders(WLX_Context *ctx, Gallery_State *g);
static void section_auto_layout(WLX_Context *ctx, Gallery_State *g);
static void section_progress_toggle_radio(WLX_Context *ctx, Gallery_State *g);

static const Section section_tokens_entry = { "Semantic Tokens", section_tokens, WLX_ICON_PALETTE };
static const Section section_label_entry = { "Label", section_label, WLX_ICON_TYPE };
static const Section section_button_entry = { "Button", section_button, WLX_ICON_MOUSE_POINTER_CLICK };
static const Section section_checkbox_entry = { "Checkbox", section_checkbox, WLX_ICON_SQUARE_CHECK };
static const Section section_image_entry = { "Image", section_image, WLX_ICON_IMAGE };
static const Section section_slider_entry = { "Slider", section_slider, WLX_ICON_SLIDERS_HORIZONTAL };
static const Section section_inputbox_entry = { "Input Box", section_inputbox, WLX_ICON_TEXT_CURSOR_INPUT };
static const Section section_scroll_panel_entry = { "Scroll Panel", section_scroll_panel, WLX_ICON_SCROLL_TEXT };
static const Section section_widget_entry = { "Widget", section_widget, WLX_ICON_COMPONENT };
static const Section section_layout_linear_entry = { "Linear Layout", section_layout_linear, WLX_ICON_ALIGN_HORIZONTAL_SPACE_BETWEEN };
static const Section section_layout_grid_entry = { "Grid Layout", section_layout_grid, WLX_ICON_GRID_3X3 };
static const Section section_layout_flex_entry = { "Flex & Sizing", section_layout_flex, WLX_ICON_STRETCH_HORIZONTAL };
static const Section section_theme_lab_entry = { "Theme Lab", section_theme_lab, WLX_ICON_PALETTE };
static const Section section_opacity_entry = { "Opacity", section_opacity, WLX_ICON_BLEND };
static const Section section_id_stack_entry = { "ID Stack", section_id_stack, WLX_ICON_LAYERS };
static const Section section_borders_entry = { "Borders", section_borders, WLX_ICON_SQUARE_DASHED };
static const Section section_auto_layout_entry = { "Auto Layout", section_auto_layout, WLX_ICON_LAYOUT_TEMPLATE };
static const Section section_progress_toggle_radio_entry = { "Progress/Toggle/Radio", section_progress_toggle_radio, WLX_ICON_TOGGLE_LEFT };

// ---------------------------------------------------------------------------
// IA Group structure: direct section pointers for the final information architecture.
// Trailing icon field is the group-level sidebar icon; WLX_ICON_COUNT means
// "no icon" and triggers the gallery_icon_button text-only fallback.
// ---------------------------------------------------------------------------
typedef struct {
    const char    *name;
    const Section *sections[12];
    int         section_count;
    int         default_section;
    WLX_Icon    icon;
} Group;

// Group ordering: 0=Overview, 1=Tokens, 2=Components, 3=Layouts, 4=Patterns, 5=Theme Lab
static const Group groups[] = {
    { "Overview",   { NULL }, 0, 0, WLX_ICON_HOUSE },
    { "Tokens",     { &section_tokens_entry }, 1, 0, WLX_ICON_PALETTE },
    { "Components", { &section_label_entry, &section_button_entry, &section_checkbox_entry,
                      &section_image_entry, &section_slider_entry, &section_inputbox_entry,
                      &section_widget_entry, &section_progress_toggle_radio_entry }, 8, 0,
                    WLX_ICON_BLOCKS },
    { "Layouts",    { &section_layout_linear_entry, &section_layout_grid_entry,
                      &section_layout_flex_entry, &section_auto_layout_entry }, 4, 0,
                    WLX_ICON_LAYOUT_DASHBOARD },
    { "Patterns",   { &section_scroll_panel_entry, &section_id_stack_entry,
                      &section_opacity_entry, &section_borders_entry }, 4, 0,
                    WLX_ICON_ROUTE },
    { "Theme Lab",  { &section_theme_lab_entry }, 1, 0, WLX_ICON_PALETTE },
};
#define GROUP_COUNT ((int)(sizeof(groups) / sizeof(groups[0])))
#define GROUP_OVERVIEW 0

// ---------------------------------------------------------------------------
// Application state
// ---------------------------------------------------------------------------
typedef struct { float r, g, b; } ColorF;

struct Gallery_State {
    int active_group;
    int active_section_in_group;
    int theme_mode;  // 0=dark, 1=light, 2=glass, 3=brand, 4=custom

    // Label
    float label_font_size;
    bool  label_show_bg;
    float label_opacity;

    // Button
    float button_font_size;
    float button_height;
    ColorF button_color;
    int   button_click_count;

    // Checkbox
    bool  checkboxes[6];
    float checkbox_font_size;
    bool  checkbox_tex_val;

    // Slider
    float slider_demo_value;
    float slider_int_value;
    ColorF slider_color;
    float slider_track_height;
    float slider_thumb_width;
    bool  slider_show_label;
    float slider_min, slider_max;

    // Input Box
    char  inputs[4][256];
    float input_font_size;
    float input_height;
    ColorF input_focus_color;

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
    ColorF theme_bg;
    ColorF theme_fg;
    ColorF theme_surface;
    ColorF theme_accent;
    ColorF theme_border;
    float theme_hover_brightness;
    float theme_padding;
    float theme_roundness;

    // Opacity
    float opacity_control;
    float theme_opacity;
    float stack_opacity;

    // Borders
    float border_width;
    ColorF border_color;
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
    float  progress_value;
    bool   toggle_a;
    bool   toggle_b;
    int    radio_choice;
};

#define GALLERY_THEME_DARK   0
#define GALLERY_THEME_LIGHT  1
#define GALLERY_THEME_GLASS  2
#define GALLERY_THEME_BRAND  3
#define GALLERY_THEME_CUSTOM 4
#define GALLERY_THEME_COUNT  5

static const char *theme_names[GALLERY_THEME_COUNT] = { "Dark", "Light", "Glass", "Brand", "Custom" };

static Gallery_State g = {
    .active_group = GROUP_OVERVIEW,
    .active_section_in_group = 0,
    .theme_mode = 0,

    .label_font_size = 16.0f,
    .label_show_bg = false,
    .label_opacity = 1.0f,

    .button_font_size = 18.0f,
    .button_height = 40.0f,
    .button_color = { 0.24f, 0.12f, 0.12f },
    .button_click_count = 0,

    .checkboxes = { true, false, true, false, false, false },
    .checkbox_font_size = 18.0f,
    .checkbox_tex_val = false,

    .slider_demo_value = 0.5f,
    .slider_int_value = 50.0f,
    .slider_color = { 0.2f, 0.5f, 0.8f },
    .slider_track_height = 6.0f,
    .slider_thumb_width = 14.0f,
    .slider_show_label = true,
    .slider_min = 0.0f, .slider_max = 1.0f,

    .inputs = { "Hello!", "", "", "" },
    .input_font_size = 16.0f,
    .input_height = 40.0f,
    .input_focus_color = { 0.35f, 0.55f, 0.82f },

    .scroll_sb_width = 10.0f,
    .scroll_wheel_speed = 20.0f,
    .scroll_show_scrollbar = true,

    .layout_slot_count = 4.0f,
    .layout_padding = 4.0f,

    .grid_rows = 3.0f,
    .grid_cols = 4.0f,

    .flex_weight_a = 1.0f, .flex_weight_b = 3.0f,
    .flex_min = 100.0f, .flex_max = 400.0f,

    .theme_bg = { 0.07f, 0.07f, 0.07f },
    .theme_fg = { 0.88f, 0.88f, 0.88f },
    .theme_surface = { 0.16f, 0.16f, 0.16f },
    .theme_accent = { 0.0f, 0.47f, 1.0f },
    .theme_border = { 0.24f, 0.24f, 0.24f },
    .theme_hover_brightness = 0.75f,
    .theme_padding = 4.0f,
    .theme_roundness = 0.0f,

    .opacity_control = 0.5f,
    .theme_opacity = 1.0f,
    .stack_opacity = 1.0f,

    .border_width = 2.0f,
    .border_color = { 0.0f, 0.75f, 1.0f },
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

static int gallery_group_default_index(const Group *group) {
    if (!group || group->section_count <= 0) return 0;
    if (group->default_section < 0 || group->default_section >= group->section_count) return 0;
    return group->default_section;
}

static void gallery_select_group(Gallery_State *st, int group_index) {
    if (!st) return;
    if (group_index < 0 || group_index >= GROUP_COUNT) group_index = GROUP_OVERVIEW;
    st->active_group = group_index;
    st->active_section_in_group = gallery_group_default_index(&groups[group_index]);
}

static void gallery_select_section(Gallery_State *st, int group_index, int section_index) {
    if (!st) return;
    if (group_index < 0 || group_index >= GROUP_COUNT) group_index = GROUP_OVERVIEW;
    const Group *group = &groups[group_index];
    if (section_index < 0 || section_index >= group->section_count) {
        section_index = gallery_group_default_index(group);
    }
    st->active_group = group_index;
    st->active_section_in_group = section_index;
}

static const Section *gallery_active_section(const Gallery_State *st) {
    if (!st) return NULL;
    if (st->active_group < 0 || st->active_group >= GROUP_COUNT) return NULL;
    const Group *group = &groups[st->active_group];
    if (group->section_count <= 0) return NULL;
    int section_index = st->active_section_in_group;
    if (section_index < 0 || section_index >= group->section_count) {
        section_index = gallery_group_default_index(group);
    }
    return group->sections[section_index];
}

// ---------------------------------------------------------------------------
// Optional gallery benchmark controls and reporting (WLX_PERF)
// ---------------------------------------------------------------------------
#include "gallery_perf.h"

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

typedef struct {
    WLX_Color color_app_bg;
    WLX_Color color_surface_1;
    WLX_Color color_surface_2;
    WLX_Color color_surface_3;
    WLX_Color color_text_1;
    WLX_Color color_text_2;
    WLX_Color color_text_muted;
    WLX_Color color_border;
    WLX_Color color_border_strong;
    WLX_Color color_accent;
    WLX_Color color_focus;
    WLX_Color color_selection;
    WLX_Color color_success;
    WLX_Color color_warning;
    WLX_Color color_danger;
} Gallery_Semantic_Theme;

static unsigned char gallery_clamp_byte(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (unsigned char)value;
}

static bool gallery_color_is_zero(WLX_Color color) {
    return color.r == 0 && color.g == 0 && color.b == 0 && color.a == 0;
}

static bool gallery_theme_is_dark(const WLX_Theme *theme) {
    return (((int)theme->background.r + theme->background.g + theme->background.b) / 3) < 128;
}

static WLX_Color gallery_shift_with_theme(WLX_Color color, int amount, bool theme_is_dark) {
    int direction = theme_is_dark ? 1 : -1;
    return (WLX_Color){
        gallery_clamp_byte((int)color.r + direction * amount),
        gallery_clamp_byte((int)color.g + direction * amount),
        gallery_clamp_byte((int)color.b + direction * amount),
        color.a ? color.a : 255,
    };
}

static WLX_Color gallery_blend_color(WLX_Color base, WLX_Color mix, unsigned char mix_weight) {
    int base_weight = 255 - (int)mix_weight;
    return (WLX_Color){
        (unsigned char)(((int)base.r * base_weight + (int)mix.r * mix_weight + 127) / 255),
        (unsigned char)(((int)base.g * base_weight + (int)mix.g * mix_weight + 127) / 255),
        (unsigned char)(((int)base.b * base_weight + (int)mix.b * mix_weight + 127) / 255),
        (unsigned char)(((int)base.a * base_weight + (int)mix.a * mix_weight + 127) / 255),
    };
}

static WLX_Color gallery_on_color(WLX_Color color) {
    int luminance = ((int)color.r * 299 + (int)color.g * 587 + (int)color.b * 114) / 1000;
    if (luminance < 140) return (WLX_Color){244, 244, 238, 255};
    return (WLX_Color){24, 28, 32, 255};
}

static Gallery_Semantic_Theme gallery_semantic_theme(const WLX_Theme *theme) {
    bool theme_is_dark = gallery_theme_is_dark(theme);
    WLX_Color surface_2 = gallery_shift_with_theme(theme->surface, 12, theme_is_dark);
    WLX_Color surface_3 = gallery_shift_with_theme(theme->surface, 24, theme_is_dark);

    return (Gallery_Semantic_Theme){
        .color_app_bg = theme->background,
        .color_surface_1 = theme->surface,
        .color_surface_2 = surface_2,
        .color_surface_3 = surface_3,
        .color_text_1 = theme->foreground,
        .color_text_2 = gallery_blend_color(theme->foreground, surface_2, 56),
        .color_text_muted = gallery_blend_color(theme->foreground, surface_2, 104),
        .color_border = theme->border,
        .color_border_strong = gallery_blend_color(theme->border, theme->foreground, 88),
        .color_accent = theme->accent,
        .color_focus = gallery_color_is_zero(theme->input.border_focus)
            ? theme->accent
            : theme->input.border_focus,
        .color_selection = gallery_blend_color(surface_2, theme->accent, 92),
        .color_success = GALLERY_GREEN,
        .color_warning = GALLERY_WARNING,
        .color_danger = GALLERY_RED,
    };
}

static WLX_Theme gallery_theme_copy_with_font(WLX_Theme theme, WLX_Font font) {
    theme.font = font;
    theme.slider.track = gallery_semantic_theme(&theme).color_surface_3;
    return theme;
}

typedef enum {
    GALLERY_ICON_ROLE_TEXT,
    GALLERY_ICON_ROLE_MUTED,
    GALLERY_ICON_ROLE_ACCENT,
    GALLERY_ICON_ROLE_SUCCESS,
    GALLERY_ICON_ROLE_WARNING,
    GALLERY_ICON_ROLE_DANGER,
    GALLERY_ICON_ROLE_FOCUS,
} Gallery_Icon_Role;

static WLX_Rect gallery_icon_src_for(WLX_Icon id, float target_px) {
    if ((int)id < 0 || (int)id >= WLX_ICON_COUNT) return (WLX_Rect){0};
    int t = (target_px <= 0.0f) ? 0 : (int)ceilf(target_px);
    int chosen = WLX_ICON_TIER_COUNT - 1;
    for (int i = 0; i < WLX_ICON_TIER_COUNT; i++) {
        if (wlx_icon_tier_sizes[i] >= t) { chosen = i; break; }
    }
    WLX_Icon_Rect r = wlx_icon_rects_tiered[chosen][id];
    return (WLX_Rect){ (float)r.x, (float)r.y, (float)r.w, (float)r.h };
}

__attribute__((unused))
static WLX_Rect gallery_icon_src(WLX_Icon id) {
    return gallery_icon_src_for(id, 16.0f);
}

static WLX_Color gallery_icon_tint(const Gallery_Semantic_Theme *sem,
                                   Gallery_Icon_Role role) {
    switch (role) {
    case GALLERY_ICON_ROLE_TEXT:    return sem->color_text_1;
    case GALLERY_ICON_ROLE_MUTED:   return sem->color_text_muted;
    case GALLERY_ICON_ROLE_ACCENT:  return sem->color_accent;
    case GALLERY_ICON_ROLE_SUCCESS: return sem->color_success;
    case GALLERY_ICON_ROLE_WARNING: return sem->color_warning;
    case GALLERY_ICON_ROLE_DANGER:  return sem->color_danger;
    case GALLERY_ICON_ROLE_FOCUS:   return sem->color_focus;
    }
    return sem->color_text_1;
}


// Low-level tint helper for preview blocks that still want bespoke color ramps.
static WLX_Color heading_color(const WLX_Theme *t, int tr, int tg, int tb) {
    int s = gallery_theme_is_dark(t) ? 1 : -1;
    int avg = (tr + tg + tb) / 3;
    return (WLX_Color){
        gallery_clamp_byte((int)t->background.r + s * avg),
        gallery_clamp_byte((int)t->background.g + s * avg),
        gallery_clamp_byte((int)t->background.b + s * avg),
        255,
    };
}

static void color_sliders(WLX_Context *ctx, const char *prefix,
    ColorF *c, int h, int fs) {
    char lr[32], lg[32], lb[32];
    snprintf(lr, 32, "%s R ", prefix);
    snprintf(lg, 32, "%s G ", prefix);
    snprintf(lb, 32, "%s B ", prefix);
    wlx_push_id(ctx, 0);
    wlx_slider(ctx, lr, &c->r, .height = h, .font_size = fs, .thumb_color = GALLERY_RED);
    wlx_pop_id(ctx);
    wlx_push_id(ctx, 1);
    wlx_slider(ctx, lg, &c->g, .height = h, .font_size = fs, .thumb_color = GALLERY_GREEN);
    wlx_pop_id(ctx);
    wlx_push_id(ctx, 2);
    wlx_slider(ctx, lb, &c->b, .height = h, .font_size = fs, .thumb_color = GALLERY_BLUE);
    wlx_pop_id(ctx);
}

// Height constants
#define HEADING_H      48
#define SUB_HEADING_H  36
#define DESC_H         30
#define SMALL_H        24
#define PREVIEW_H      220
#define SIDEBAR_W      210
#define SIDEBAR_GROUP_CONTENT_PAD_L    10
#define SIDEBAR_SECTION_CONTENT_PAD_L  20
#define SIDEBAR_CONTENT_PAD_R           6

// Font size constants
#define SECTION_FS  26
#define MEDIUM_FS   16
#define SMALL_FS    14
#define TINY_FS     13
#define DEMO_FS     18

// Option-panel constants (sidebar widgets)
#define OPT_H   32
#define OPT_FS  14

// Resolve the pixel size used to pick an atlas tier for chrome helpers.
// Falls back to row-height minus inset when the caller did not pass
// .image_size; clamps into the atlas tier range.
static float gallery_chrome_icon_size(float image_size, float row_height) {
    float target = image_size > 0.0f ? image_size : row_height - 12.0f;
    if (target < 16.0f) target = 16.0f;
    if (target > 48.0f) target = 48.0f;
    return target;
}

static bool gallery_icon_button_helper(WLX_Context *ctx, const char *text,
                                       WLX_Icon icon, Gallery_Icon_Role role,
                                       WLX_Button_Opt opt,
                                       const char *file, int line) {
    if (icon != WLX_ICON_COUNT && gallery_icon_atlas_ready()) {
        Gallery_Semantic_Theme sem = gallery_semantic_theme(ctx->theme);
        float target = gallery_chrome_icon_size(opt.image_size, opt.height);
        opt.texture = gallery_icon_atlas_texture();
        opt.texture_src = gallery_icon_src_for(icon, target);
        if (opt.texture_tint.a == 0) {
            opt.texture_tint = gallery_icon_tint(&sem, role);
        }
        if (opt.image_size <= 0.0f) opt.image_size = target;
        if (opt.image_text_gap < 0) opt.image_text_gap = 8;
    }
    return wlx_button_impl(ctx, text, opt, file, line);
}

#define gallery_icon_button(ctx, text, icon, role, ...) \
    gallery_icon_button_helper((ctx), (text), (icon), (role), \
        wlx_default_button_opt(__VA_ARGS__), __FILE__, __LINE__)

static void gallery_icon_label_helper(WLX_Context *ctx, const char *text,
                                      WLX_Icon icon, Gallery_Icon_Role role,
                                      WLX_Label_Opt opt,
                                      const char *file, int line) {
    if (icon != WLX_ICON_COUNT && gallery_icon_atlas_ready()) {
        Gallery_Semantic_Theme sem = gallery_semantic_theme(ctx->theme);
        float target = gallery_chrome_icon_size(opt.image_size, opt.height);
        opt.texture = gallery_icon_atlas_texture();
        opt.texture_src = gallery_icon_src_for(icon, target);
        if (opt.texture_tint.a == 0) {
            opt.texture_tint = gallery_icon_tint(&sem, role);
        }
        if (opt.image_size <= 0.0f) opt.image_size = target;
        if (opt.image_text_gap < 0) opt.image_text_gap = 8;
    }
    wlx_label_impl(ctx, text, opt, file, line);
}

#define gallery_icon_label(ctx, text, icon, role, ...) \
    gallery_icon_label_helper((ctx), (text), (icon), (role), \
        wlx_default_label_opt(__VA_ARGS__), __FILE__, __LINE__)

// Render an icon-backed heading row as the first body row of a panel.
// The accompanying panel must be created without a built-in title so this
// row stands in for it. Font size matches the prior panel-title default
// (18 px) so converted panels keep their existing visual weight. Tint
// role is applied to the icon; the label uses the theme's primary text
// color.
static void gallery_panel_heading_helper(WLX_Context *ctx,
                                         WLX_Icon icon,
                                         Gallery_Icon_Role role,
                                         const char *text,
                                         int height,
                                         const char *file,
                                         int line) {
    Gallery_Semantic_Theme sem = gallery_semantic_theme(ctx->theme);
    gallery_icon_label_helper(ctx, text, icon, role, wlx_default_label_opt(
        .height = height,
        .font_size = 18,
        .align = WLX_LEFT,
        .show_background = true,
        .back_color = sem.color_surface_3,
        .front_color = sem.color_text_1,
        .image_size = (float)height * 0.45f,
        .image_text_gap = 10),
        file,
        line);
}

#define gallery_panel_heading(ctx, icon, role, text, height) \
    gallery_panel_heading_helper((ctx), (icon), (role), (text), (height), \
        __FILE__, __LINE__)

// Gallery-local semantic role lookups.
#define GALLERY_ROLE(ctx, role) (gallery_semantic_theme((ctx)->theme).role)
#define SPLIT_BG(ctx)      GALLERY_ROLE(ctx, color_surface_2)
#define OPTIONS_BG(ctx)    GALLERY_ROLE(ctx, color_surface_3)
#define SECTION_BG(ctx)    GALLERY_ROLE(ctx, color_surface_2)
#define CONTENT_BG(ctx)    GALLERY_ROLE(ctx, color_surface_3)
#define DANGER_BG(ctx)     GALLERY_ROLE(ctx, color_danger)
#define BORDER_STRONG(ctx) GALLERY_ROLE(ctx, color_border_strong)

// Section heading macros - colors adapt to the active theme
#define PANEL_TITLE_DEFAULTS \
    .title_font_size = SECTION_FS, .title_height = HEADING_H, .padding = 0

#define SECTION_HEADING(ctx, title) \
    wlx_label(ctx, title, \
        .font_size = SECTION_FS, .align = WLX_CENTER, .height = HEADING_H, \
        .show_background = true, .back_color = CONTENT_BG(ctx))

#define SUB_HEADING(ctx, title) \
    wlx_label(ctx, title, \
        .font_size = WLX_STYLE_HEADING_FONT_SIZE, .align = WLX_LEFT, .height = SUB_HEADING_H, \
        .show_background = true, .back_color = SECTION_BG(ctx))

// ---------------------------------------------------------------------------
// Section template: left-panel (controls) / right-panel (preview) split.
// Every section should use SECTION_BEGIN / SECTION_NEXT / SECTION_END for
// consistent layout and semantic background colors.
// Usage:
//   SECTION_BEGIN(ctx);
//     wlx_panel_begin(ctx, .padding = 0);
//       gallery_panel_heading(ctx, WLX_ICON_SLIDERS_HORIZONTAL,
//                             GALLERY_ICON_ROLE_TEXT, "Options", HEADING_H);
//       // ... controls ...
//     wlx_panel_end(ctx);
//   SECTION_NEXT(ctx);
//     // ... preview content ...
//   SECTION_END(ctx);
// ---------------------------------------------------------------------------
#define SECTION_BEGIN(ctx) \
    wlx_split_begin(ctx, .padding = 0, .gap = 4, .first_back_color = SPLIT_BG(ctx))

#define SECTION_NEXT(ctx) \
    wlx_split_next(ctx)

#define SECTION_END(ctx) \
    wlx_split_end(ctx)

// ---------------------------------------------------------------------------
// Section: Label
// ---------------------------------------------------------------------------
static void section_label(WLX_Context *ctx, Gallery_State *st) {
    int fs = (int)st->label_font_size;

    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


            wlx_slider(ctx, "Font Size  ", &st->label_font_size,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 8.0f, .max_value = 40.0f);
            wlx_checkbox(ctx, "Boxed", &st->label_show_bg,
                .height = OPT_H, .font_size = OPT_FS);
            wlx_slider(ctx, "Opacity    ", &st->label_opacity,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 0.0f, .max_value = 1.0f);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

        // ======== RIGHT: Content ========
        wlx_panel_begin(ctx, .title = "Label",
            PANEL_TITLE_DEFAULTS,
            .title_back_color = SECTION_BG(ctx));

        wlx_label(ctx, "A simple static label with default options.",
            .font_size = fs, .height = ROW_H);

        wlx_label(ctx, "Centered Heading (boxed)",
            .font_size = fs + 4, .align = WLX_CENTER, .height = HEADING_H,
            .show_background = st->label_show_bg,
            .back_color = heading_color(ctx->theme, 22, 22, 42),
            .opacity = st->label_opacity);

        wlx_label(ctx, "Right-aligned with custom color",
            .font_size = fs, .align = WLX_RIGHT, .height = ROW_H,
            .front_color = (WLX_Color){100, 200, 255, 255},
            .opacity = st->label_opacity);

        wlx_label(ctx, "This is a longer paragraph of text to demonstrate word wrapping behavior. "
            "When the text exceeds the available width, it should wrap to the next line automatically. "
            "The label widget supports this via the .wrap option which is true by default.",
            .font_size = fs, .height = 80, .wrap = true,
            .opacity = st->label_opacity);

        SUB_HEADING(ctx, "Alignment Showcase");

        // Show alignments in a 3x2 grid. Tall rows make TOP/BOTTOM
        // alignment visually distinct from one another.
        static const WLX_Slot_Size align_rows[] = { WLX_SLOT_PX(60), WLX_SLOT_PX(60), WLX_SLOT_PX(60) };
        wlx_grid_begin(ctx, 3, 3, .padding = 0, .padding_top = 4, .gap = 4, .row_sizes = align_rows);
            const char *align_names[] = { "TOP_LEFT", "TOP_CENTER", "TOP_RIGHT",
                                           "LEFT", "CENTER", "RIGHT",
                                           "BOTTOM_LEFT", "BOTTOM_CENTER", "BOTTOM_RIGHT" };
            WLX_Align aligns[] = { WLX_TOP_LEFT, WLX_TOP_CENTER, WLX_TOP_RIGHT,
                                    WLX_LEFT, WLX_CENTER, WLX_RIGHT,
                                    WLX_BOTTOM_LEFT, WLX_BOTTOM_CENTER, WLX_BOTTOM_RIGHT };
            for (int i = 0; i < 9; i++) {
                wlx_push_id(ctx, (size_t)i);
                wlx_label(ctx, align_names[i],
                    .font_size = SMALL_FS, .align = aligns[i],
                    .back_color = heading_color(ctx->theme, 12, 17, 27), .show_background = true, .wrap = false);
                wlx_pop_id(ctx);
            }
        wlx_layout_end(ctx);

        SUB_HEADING(ctx, "Status Row (icon-backed)");

        if (gallery_icon_atlas_ready()) {
            Gallery_Semantic_Theme sem = gallery_semantic_theme(ctx->theme);
            WLX_Texture atlas = gallery_icon_atlas_texture();
            float icon_size = (float)ROW_H - 16.0f;

            wlx_layout_begin(ctx, 4, WLX_HORZ, .gap = 8);
                wlx_label(ctx, "Saved",
                    .height = ROW_H, .font_size = fs,
                    .texture = atlas,
                    .texture_src = gallery_icon_src_for(WLX_ICON_CHECK, icon_size),
                    .texture_tint = gallery_icon_tint(&sem, GALLERY_ICON_ROLE_SUCCESS),
                    .image_size = icon_size);

                wlx_label(ctx, "Details",
                    .height = ROW_H, .font_size = fs,
                    .texture = atlas,
                    .texture_src = gallery_icon_src_for(WLX_ICON_INFO, icon_size),
                    .texture_tint = gallery_icon_tint(&sem, GALLERY_ICON_ROLE_ACCENT),
                    .image_size = icon_size);

                wlx_label(ctx, "Heads up",
                    .height = ROW_H, .font_size = fs,
                    .texture = atlas,
                    .texture_src = gallery_icon_src_for(WLX_ICON_TRIANGLE_ALERT, icon_size),
                    .texture_tint = gallery_icon_tint(&sem, GALLERY_ICON_ROLE_WARNING),
                    .image_size = icon_size);

                wlx_label(ctx, "12 items",
                    .height = ROW_H, .font_size = fs,
                    .texture = atlas,
                    .texture_src = gallery_icon_src_for(WLX_ICON_IMAGE, icon_size),
                    .texture_tint = gallery_icon_tint(&sem, GALLERY_ICON_ROLE_MUTED),
                    .front_color = sem.color_text_muted,
                    .image_size = icon_size);
            wlx_layout_end(ctx);
        } else {
            wlx_label(ctx, "Icon atlas unavailable on this host yet.",
                .height = ROW_H, .font_size = fs,
                .show_background = true,
                .back_color = GALLERY_ROLE(ctx, color_surface_2),
                .front_color = GALLERY_ROLE(ctx, color_text_2));
        }

        wlx_panel_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Button
// ---------------------------------------------------------------------------
static void section_button(WLX_Context *ctx, Gallery_State *st) {
    int fs = (int)st->button_font_size;
    int bh = (int)st->button_height;
    int image_bh = bh;
    int stacked_image_bh = (int)((float)bh * 0.5f + (float)fs * 1.5f + 6.0f);
    if (image_bh < stacked_image_bh) image_bh = stacked_image_bh;
    WLX_Color bc = to_color(st->button_color.r, st->button_color.g, st->button_color.b, 1.0f);

    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


            wlx_slider(ctx, "Font Size  ", &st->button_font_size,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 10.0f, .max_value = 36.0f);
            wlx_slider(ctx, "Height     ", &st->button_height,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 24.0f, .max_value = 80.0f);
            color_sliders(ctx, "", &st->button_color, OPT_H, OPT_FS);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

        // ======== RIGHT: Content ========
        wlx_panel_begin(ctx, .title = "Button",
            PANEL_TITLE_DEFAULTS,
            .title_back_color = SECTION_BG(ctx));

        wlx_label(ctx, "Clickable button. Returns true on the frame clicked.",
            .height = DESC_H);

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

        SUB_HEADING(ctx, "Icon-Backed Buttons");

        if (gallery_icon_atlas_ready()) {
            Gallery_Semantic_Theme sem = gallery_semantic_theme(ctx->theme);
            WLX_Texture atlas = gallery_icon_atlas_texture();
            float icon_size = (float)bh - 16.0f;

            wlx_layout_begin(ctx, 4, WLX_HORZ, .gap = 8);
                if (wlx_button(ctx, "",
                    .height = image_bh, .align = WLX_CENTER,
                    .texture = atlas,
                    .texture_src = gallery_icon_src_for(WLX_ICON_PLAY, icon_size),
                    .texture_tint = gallery_icon_tint(&sem, GALLERY_ICON_ROLE_ACCENT),
                    .image_size = icon_size)) {
                    st->button_click_count++;
                }
                if (wlx_button(ctx, "Save",
                    .height = image_bh, .align = WLX_CENTER, .font_size = fs,
                    .texture = atlas,
                    .texture_src = gallery_icon_src_for(WLX_ICON_SAVE, icon_size),
                    .texture_tint = gallery_icon_tint(&sem, GALLERY_ICON_ROLE_TEXT),
                    .image_placement = WLX_IMAGE_PLACEMENT_LEFT,
                    .image_size = icon_size,
                    .image_text_gap = 8)) {
                    st->button_click_count++;
                }
                if (wlx_button(ctx, "Confirm",
                    .height = image_bh, .align = WLX_CENTER, .font_size = fs,
                    .texture = atlas,
                    .texture_src = gallery_icon_src_for(WLX_ICON_CHECK, (float)bh * 0.5f),
                    .texture_tint = gallery_icon_tint(&sem, GALLERY_ICON_ROLE_SUCCESS),
                    .image_placement = WLX_IMAGE_PLACEMENT_TOP,
                    .image_size = (float)bh * 0.5f)) {
                    st->button_click_count++;
                }
                if (wlx_button(ctx, "Settings",
                    .height = image_bh, .align = WLX_CENTER, .font_size = fs,
                    .texture = atlas,
                    .texture_src = gallery_icon_src_for(WLX_ICON_SETTINGS, icon_size),
                    .texture_tint = gallery_icon_tint(&sem, GALLERY_ICON_ROLE_TEXT),
                    .image_placement = WLX_IMAGE_PLACEMENT_RIGHT,
                    .image_size = icon_size,
                    .image_text_gap = 8)) {
                    st->button_click_count++;
                }
            wlx_layout_end(ctx);
        } else {
            wlx_label(ctx, "Icon atlas unavailable on this host yet.",
                .height = ROW_H, .font_size = fs,
                .show_background = true,
                .back_color = GALLERY_ROLE(ctx, color_surface_2),
                .front_color = GALLERY_ROLE(ctx, color_text_2));
        }

        char click_buf[64];
        snprintf(click_buf, sizeof(click_buf), "Click count: %d", st->button_click_count);
        wlx_label(ctx, click_buf,
            .font_size = DEMO_FS, .height = ROW_H, .align = WLX_CENTER);

        {
            Gallery_Semantic_Theme sem = gallery_semantic_theme(ctx->theme);
            WLX_Texture atlas = gallery_icon_atlas_texture();
            bool atlas_ready = gallery_icon_atlas_ready();

            if (wlx_button(ctx, "Reset Counter",
                .height = ROW_H, .align = WLX_CENTER,
                .back_color = DANGER_BG(ctx),
                .texture = atlas_ready ? atlas : (WLX_Texture){0},
                .texture_src = atlas_ready ? gallery_icon_src_for(WLX_ICON_ROTATE_CCW, (float)ROW_H - 14.0f) : (WLX_Rect){0},
                .texture_tint = atlas_ready
                    ? gallery_icon_tint(&sem, GALLERY_ICON_ROLE_TEXT)
                    : (WLX_Color){0},
                .image_placement = WLX_IMAGE_PLACEMENT_LEFT,
                .image_size = (float)ROW_H - 14.0f,
                .image_text_gap = 8)) {
                st->button_click_count = 0;
            }
        }

        wlx_panel_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Checkbox
// ---------------------------------------------------------------------------
static void section_checkbox(WLX_Context *ctx, Gallery_State *st) {
    int fs = (int)st->checkbox_font_size;

    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


            wlx_slider(ctx, "Font Size  ", &st->checkbox_font_size,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 12.0f, .max_value = 30.0f);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

        // ======== RIGHT: Content ========
        wlx_panel_begin(ctx, .title = "Checkbox",
            PANEL_TITLE_DEFAULTS,
            .title_back_color = SECTION_BG(ctx));

    wlx_label(ctx, "Toggle checkbox with label. Returns true when state changes.",
        .height = DESC_H);

    wlx_checkbox(ctx, "Enable audio", &st->checkboxes[0],
        .height = ROW_H, .font_size = fs);
    wlx_checkbox(ctx, "Enable V-Sync", &st->checkboxes[1],
        .height = ROW_H, .font_size = fs);
    wlx_checkbox(ctx, "Fullscreen mode", &st->checkboxes[2],
        .height = ROW_H, .font_size = fs);

    SUB_HEADING(ctx, "Custom Colors");

    wlx_checkbox(ctx, "Green check mark", &st->checkboxes[3],
        .height = ROW_H, .font_size = fs,
        .check_color = (WLX_Color){0, 200, 0, 255});
    wlx_checkbox(ctx, "Red border", &st->checkboxes[4],
        .height = ROW_H, .font_size = fs,
        .border_color = (WLX_Color){200, 0, 0, 255});

    SUB_HEADING(ctx, "Atlas Texture Checkbox");

    if (gallery_icon_atlas_ready()) {
        Gallery_Semantic_Theme sem = gallery_semantic_theme(ctx->theme);
        WLX_Texture atlas = gallery_icon_atlas_texture();
        wlx_checkbox(ctx, "Atlas-backed toggle", &st->checkbox_tex_val,
            .height = ROW_H, .font_size = fs,
            .tex_checked       = atlas,
            .tex_unchecked     = atlas,
            .tex_checked_src   = gallery_icon_src_for(WLX_ICON_SQUARE_CHECK, (float)fs),
            .tex_unchecked_src = gallery_icon_src_for(WLX_ICON_SQUARE, (float)fs),
            .tex_checked_tint  = gallery_icon_tint(&sem, GALLERY_ICON_ROLE_ACCENT),
            .tex_unchecked_tint = gallery_icon_tint(&sem, GALLERY_ICON_ROLE_MUTED));
    } else {
        wlx_label(ctx, "Icon atlas unavailable on this host yet.",
            .height = ROW_H, .font_size = fs,
            .show_background = true,
            .back_color = GALLERY_ROLE(ctx, color_surface_2),
            .front_color = GALLERY_ROLE(ctx, color_text_2));
    }

        wlx_panel_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Image
// ---------------------------------------------------------------------------
static void section_image(WLX_Context *ctx, Gallery_State *st) {
    (void)st;
    wlx_panel_begin(ctx, .title = "Image",
        PANEL_TITLE_DEFAULTS,
        .title_back_color = SECTION_BG(ctx));

    wlx_label(ctx,
        "wlx_image renders one cell of the icon atlas via texture_src, with FIT/NONE scale and semantic tinting.",
        .height = DESC_H, .wrap = true);

    if (!gallery_icon_atlas_ready()) {
        wlx_label(ctx, "Icon atlas unavailable on this platform.",
            .height = ROW_H,
            .show_background = true,
            .back_color = GALLERY_ROLE(ctx, color_surface_2),
            .front_color = GALLERY_ROLE(ctx, color_text_2));
        wlx_panel_end(ctx);
        return;
    }

    Gallery_Semantic_Theme sem = gallery_semantic_theme(ctx->theme);
    WLX_Texture atlas = gallery_icon_atlas_texture();
    WLX_Rect src_image   = gallery_icon_src_for(WLX_ICON_IMAGE, (float)PREVIEW_H);
    WLX_Rect src_palette = gallery_icon_src_for(WLX_ICON_PALETTE, (float)PREVIEW_H);

    static const struct {
        const char       *label;
        WLX_Image_Scale   scale;
    } columns[] = {
        { "FIT",         WLX_IMAGE_SCALE_FIT  },
        { "NONE",        WLX_IMAGE_SCALE_NONE },
        { "FIT + tint",  WLX_IMAGE_SCALE_FIT  },
    };
    const int column_count = (int)(sizeof(columns) / sizeof(columns[0]));

    wlx_layout_begin(ctx, column_count, WLX_HORZ, .padding = 4, .gap = 6);
    for (int i = 0; i < column_count; i++) {
        wlx_push_id(ctx, (size_t)(i + 1));
        wlx_layout_begin(ctx, 2, WLX_VERT,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(20), WLX_SLOT_PX(PREVIEW_H) },
            .padding = 2);
            wlx_label(ctx, columns[i].label,
                .font_size = 13, .align = WLX_CENTER);
            if (i < 2) {
                wlx_image(ctx, atlas,
                    .src = src_image,
                    .scale = columns[i].scale,
                    .tint = gallery_icon_tint(&sem, GALLERY_ICON_ROLE_TEXT),
                    .align = WLX_CENTER);
            } else {
                wlx_image(ctx, atlas,
                    .src = src_palette,
                    .scale = columns[i].scale,
                    .tint = gallery_icon_tint(&sem, GALLERY_ICON_ROLE_ACCENT),
                    .align = WLX_CENTER);
            }
        wlx_layout_end(ctx);
        wlx_pop_id(ctx);
    }
    wlx_layout_end(ctx);

    wlx_panel_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Slider
// ---------------------------------------------------------------------------
static void section_slider(WLX_Context *ctx, Gallery_State *st) {
    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


            wlx_slider(ctx, "Min Value  ", &st->slider_min,
                .height = OPT_H, .min_value = 0.0f, .max_value = 0.5f, .font_size = OPT_FS);
            wlx_slider(ctx, "Max Value  ", &st->slider_max,
                .height = OPT_H, .min_value = 0.5f, .max_value = 2.0f, .font_size = OPT_FS);
            wlx_slider(ctx, "Track H    ", &st->slider_track_height,
                .height = OPT_H, .min_value = 2.0f, .max_value = 24.0f, .font_size = OPT_FS);
            wlx_slider(ctx, "Thumb W    ", &st->slider_thumb_width,
                .height = OPT_H, .min_value = 4.0f, .max_value = 40.0f, .font_size = OPT_FS);
            wlx_checkbox(ctx, "Show Label", &st->slider_show_label,
                .height = OPT_H, .font_size = OPT_FS);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

        // ======== RIGHT: Content ========
        wlx_panel_begin(ctx, .title = "Slider",
            PANEL_TITLE_DEFAULTS,
            .title_back_color = SECTION_BG(ctx));

            wlx_label(ctx, "Horizontal slider for float values. Click/drag the thumb or track.",
                .height = DESC_H);

            SUB_HEADING(ctx, "Default (0 - 1)");
            wlx_slider(ctx, "Value      ", &st->slider_demo_value,
                .height = ROW_H, .font_size = DEMO_FS,
                .min_value = st->slider_min, .max_value = st->slider_max,
                .show_label = st->slider_show_label);

            SUB_HEADING(ctx, "Integer Range (0 - 100)");
            wlx_slider(ctx, "Percent    ", &st->slider_int_value,
                .height = ROW_H, .font_size = DEMO_FS,
                .min_value = 0.0f, .max_value = 100.0f,
                .show_label = st->slider_show_label);

            SUB_HEADING(ctx, "Color Mixer");
            color_sliders(ctx, "", &st->slider_color, ROW_H, DEMO_FS);
            {
                WLX_Color preview = to_color(st->slider_color.r, st->slider_color.g, st->slider_color.b, 1.0f);
                wlx_widget(ctx,
                    .widget_align = WLX_CENTER, .width = -1, .height = 40,
                    .color = preview);
            }

            SUB_HEADING(ctx, "Custom Track & Thumb");
            {
                static float dummy = 0.6f;
                wlx_slider(ctx, "Styled     ", &dummy,
                    .height = ROW_H, .font_size = DEMO_FS,
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
    WLX_Color fc = to_color(st->input_focus_color.r, st->input_focus_color.g, st->input_focus_color.b, 1.0f);

    SECTION_BEGIN(ctx);

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


            wlx_slider(ctx, "Font Size  ", &st->input_font_size,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 10.0f, .max_value = 30.0f);
            wlx_slider(ctx, "Height     ", &st->input_height,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 28.0f, .max_value = 80.0f);
            color_sliders(ctx, "Focus", &st->input_focus_color, OPT_H, OPT_FS);

        wlx_panel_end(ctx);

    SECTION_NEXT(ctx);

        // ======== RIGHT: Content ========
        wlx_panel_begin(ctx, .title = "Input Box",
            PANEL_TITLE_DEFAULTS,
            .title_back_color = SECTION_BG(ctx));

        wlx_label(ctx, "Single-line text input. Click to focus, type to edit, Enter/Esc to unfocus.",
            .height = DESC_H);

        wlx_inputbox(ctx, "Name: ", st->inputs[0], sizeof(st->inputs[0]),
            .height = ih, .font_size = fs);
        wlx_inputbox(ctx, "Email: ", st->inputs[1], sizeof(st->inputs[1]),
            .height = ih, .font_size = fs);
        wlx_inputbox(ctx, "Note:  ", st->inputs[2], sizeof(st->inputs[2]),
            .height = ih*3, .font_size = fs, .align = WLX_TOP_LEFT);

        SUB_HEADING(ctx, "Custom Focus Color");
        wlx_inputbox(ctx, "Styled: ", st->inputs[3], sizeof(st->inputs[3]),
            .height = ih, .font_size = fs,
            .border_focus_color = fc,
            .cursor_color = fc);

        if (wlx_button(ctx, "Clear All Inputs",
            .height = ROW_H, .align = WLX_CENTER,
            .back_color = DANGER_BG(ctx))) {
            for (int i = 0; i < 4; i++) st->inputs[i][0] = '\0';
        }

        wlx_panel_end(ctx);

    SECTION_END(ctx);
}

// ---------------------------------------------------------------------------
// Section: Scroll Panel
// ---------------------------------------------------------------------------
static void section_scroll_panel(WLX_Context *ctx, Gallery_State *st) {
    Gallery_Semantic_Theme semantic = gallery_semantic_theme(ctx->theme);

    SECTION_BEGIN(ctx);

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


            wlx_slider(ctx, "Scrollbar W", &st->scroll_sb_width,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 4.0f, .max_value = 30.0f);
            wlx_slider(ctx, "Wheel Speed", &st->scroll_wheel_speed,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 5.0f, .max_value = 100.0f);
            wlx_checkbox(ctx, "Show Scrollbar", &st->scroll_show_scrollbar,
                .height = OPT_H, .font_size = OPT_FS);

        wlx_panel_end(ctx);

    SECTION_NEXT(ctx);

        // ======== RIGHT: Content ========
        wlx_panel_begin(ctx, .title = "Scroll Panel",
            PANEL_TITLE_DEFAULTS,
            .title_back_color = SECTION_BG(ctx));

        wlx_label(ctx, "Scrollable container with auto or fixed content height.",
            .height = DESC_H,
            .front_color = semantic.color_text_2);

        SUB_HEADING(ctx, "Auto-Height (20 items)");
        wlx_scroll_panel_begin(ctx, -1,
            .height = 200,
            .back_color = semantic.color_surface_2,
            .scrollbar_color = semantic.color_surface_3,
            .scrollbar_width = st->scroll_sb_width,
            .wheel_scroll_speed = st->scroll_wheel_speed,
            .show_scrollbar = st->scroll_show_scrollbar);
            // Demonstrate wlx_get_scroll_panel_viewport - show viewport dims
            WLX_Rect sp_vp = wlx_get_scroll_panel_viewport(ctx);
            char vp_buf[64];
            snprintf(vp_buf, sizeof(vp_buf), "  Viewport: %.0f x %.0f", sp_vp.w, sp_vp.h);

            wlx_layout_begin_auto(ctx, WLX_VERT, 30);
                wlx_label(ctx, vp_buf,
                    .height = SMALL_H, .font_size = TINY_FS, .align = WLX_LEFT,
                    .front_color = semantic.color_text_muted);
                for (int i = 0; i < 20; i++) {
                    wlx_push_id(ctx, (size_t)i);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "  Item %d", i + 1);
                    WLX_Color row_bg = (i == 5)
                        ? semantic.color_selection
                        : ((i % 2 == 0) ? semantic.color_surface_1 : semantic.color_surface_2);
                    WLX_Color row_fg = (i == 5)
                        ? gallery_on_color(row_bg)
                        : ((i % 2 == 0) ? semantic.color_text_2 : semantic.color_text_1);
                    wlx_label(ctx, buf,
                        .height = DESC_H, .font_size = 15,
                        .back_color = row_bg, .front_color = row_fg,
                        .align = WLX_LEFT);
                    wlx_pop_id(ctx);
                }
            wlx_layout_end(ctx);
        wlx_scroll_panel_end(ctx);

        SUB_HEADING(ctx, "Nested Scroll Panels");
        wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 0, .gap = 4);
            // Outer panel left
            wlx_scroll_panel_begin(ctx, -1,
                .height = 180,
                .back_color = semantic.color_surface_2,
                .scrollbar_color = semantic.color_border_strong);
                wlx_layout_begin_auto(ctx, WLX_VERT, 28);
                    for (int i = 0; i < 15; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        char buf[32];
                        snprintf(buf, sizeof(buf), "  Outer %d", i + 1);
                        WLX_Color row_bg = (i == 2) ? semantic.color_selection : semantic.color_surface_1;
                        WLX_Color row_fg = (i == 2) ? gallery_on_color(row_bg) : semantic.color_text_2;
                        wlx_label(ctx, buf, .height = 28, .font_size = SMALL_FS,
                            .back_color = row_bg, .front_color = row_fg);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);
            wlx_scroll_panel_end(ctx);

            // Inner panel right
            wlx_scroll_panel_begin(ctx, -1,
                .height = 180,
                .back_color = semantic.color_surface_3,
                .scrollbar_color = semantic.color_border_strong);
                wlx_layout_begin_auto(ctx, WLX_VERT, 28);
                    for (int i = 0; i < 15; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        char buf[32];
                        snprintf(buf, sizeof(buf), "  Inner %d", i + 1);
                        WLX_Color row_bg = (i == 8) ? semantic.color_selection : semantic.color_surface_2;
                        WLX_Color row_fg = (i == 8) ? gallery_on_color(row_bg) : semantic.color_text_1;
                        wlx_label(ctx, buf, .height = 28, .font_size = SMALL_FS,
                            .back_color = row_bg, .front_color = row_fg);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);
            wlx_scroll_panel_end(ctx);
        wlx_layout_end(ctx);

        wlx_panel_end(ctx);

    SECTION_END(ctx);
}

// ---------------------------------------------------------------------------
// Section: Widget (Colored Rect)
// ---------------------------------------------------------------------------
static void section_widget(WLX_Context *ctx, Gallery_State *st) {
    (void)st;

    SECTION_BEGIN(ctx);

        // ======== LEFT: Info ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


            wlx_label(ctx, "No adjustable options.",
                .font_size = OPT_FS, .height = OPT_H);
            wlx_label(ctx, "wlx_widget() draws a colored rect for swatches, spacers, and dividers.",
                .font_size = OPT_FS, .height = 60, .wrap = true);

        wlx_panel_end(ctx);

    SECTION_NEXT(ctx);

        // ======== RIGHT: Content ========
        wlx_panel_begin(ctx, .title = "Widget (Colored Rect)",
            PANEL_TITLE_DEFAULTS,
            .title_back_color = SECTION_BG(ctx));

        wlx_label(ctx, "Low-level colored rectangle for swatches, spacers, and dividers.",
            .height = DESC_H);

        SUB_HEADING(ctx, "Color Swatch");
        wlx_widget(ctx,
            .widget_align = WLX_CENTER, .width = 120, .height = 40,
            .color = (WLX_Color){0, 120, 255, 255});

        SUB_HEADING(ctx, "Divider / Spacer");
        wlx_widget(ctx,
            .widget_align = WLX_CENTER, .width = -1, .height = 2,
            .color = (WLX_Color){80, 80, 80, 255});

        SUB_HEADING(ctx, "Alignment Grid");
        wlx_label(ctx, "12 alignment values shown as 30x30 swatches inside 100x200 grid cells:",
            .font_size = SMALL_FS, .height = SMALL_H);

        static const WLX_Slot_Size align_grid_rows[] = {
            WLX_SLOT_PX(100), WLX_SLOT_PX(100), WLX_SLOT_PX(100), WLX_SLOT_PX(100)
        };
        static const WLX_Slot_Size align_grid_cols[] = {
            WLX_SLOT_PX(200), WLX_SLOT_PX(200), WLX_SLOT_PX(200)
        };
        wlx_grid_begin(ctx, 4, 3,
            .padding = 0, .gap = 4,
            .row_sizes = align_grid_rows,
            .col_sizes = align_grid_cols,
            .slot_border_color = (WLX_Color){60, 60, 60, 255}, .slot_border_width = 1);
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

    SECTION_END(ctx);
}

// ---------------------------------------------------------------------------
// Section: Linear Layout
// ---------------------------------------------------------------------------
static void section_layout_linear(WLX_Context *ctx, Gallery_State *st) {
    int slots = (int)st->layout_slot_count;
    if (slots < 2) slots = 2;
    if (slots > 10) slots = 10;

    SECTION_BEGIN(ctx);

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


            static float layout_padding = 0;

            wlx_slider(ctx, "Slot Count ", &st->layout_slot_count,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 2.0f, .max_value = 10.0f);
            wlx_slider(ctx, "Padding    ", &layout_padding,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 0.0f, .max_value = 20.0f);

        wlx_panel_end(ctx);

    SECTION_NEXT(ctx);

        // ======== RIGHT: Content ========
            wlx_layout_begin_s(ctx, WLX_VERT,
                WLX_SIZES(
                    WLX_SLOT_CONTENT,  // heading
                    WLX_SLOT_CONTENT,  // sub heading: horizontal
                    WLX_SLOT_PX(50),   // horizontal layout
                    WLX_SLOT_CONTENT,  // sub heading: vertical
                    WLX_SLOT_CONTENT,  // vertical layout
                    WLX_SLOT_CONTENT,  // sub heading: variable sizes
                    WLX_SLOT_PX(50),   // variable sizes layout
                    WLX_SLOT_CONTENT,  // sub heading: auto-sizing
                    WLX_SLOT_CONTENT   // auto-sizing layout
                ));

                SECTION_HEADING(ctx, "Linear Layouts");

                SUB_HEADING(ctx, "Horizontal (equal slots)");
                wlx_layout_begin(ctx, (size_t)slots, WLX_HORZ, .padding = layout_padding);
                    for (int i = 0; i < slots; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        WLX_Color c = { (unsigned char)(60 + i * 25), 60, (unsigned char)(200 - i * 15), 255 };
                        char buf[16];
                        snprintf(buf, sizeof(buf), "%d", i);
                        wlx_label(ctx, buf,
                            .font_size = DEMO_FS, .align = WLX_CENTER,
                            .back_color = c, .show_background = true);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Vertical (equal slots)");
                wlx_layout_begin(ctx, (size_t)slots, WLX_VERT, .padding = layout_padding);
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
                    .padding = 0);
                    wlx_label(ctx, "PX(100)",
                        .height = 50, .font_size = SMALL_FS, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 62, 22, 22), .show_background = true);
                    wlx_label(ctx, "PCT(30)",
                        .height = 50, .font_size = SMALL_FS, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 22, 62, 22), .show_background = true);
                    wlx_label(ctx, "FLEX(1)",
                        .height = 50, .font_size = SMALL_FS, .align = WLX_CENTER,
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

    SECTION_END(ctx);
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

    SECTION_BEGIN(ctx);

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


            wlx_slider(ctx, "Rows       ", &st->grid_rows,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 1.0f, .max_value = 8.0f);
            wlx_slider(ctx, "Cols       ", &st->grid_cols,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 1.0f, .max_value = 8.0f);

        wlx_panel_end(ctx);

    SECTION_NEXT(ctx);

        // ======== RIGHT: Content ========
            wlx_layout_begin_s(ctx, WLX_VERT,
                WLX_SIZES(
                    WLX_SLOT_CONTENT,  // heading
                    WLX_SLOT_CONTENT,  // sub: fixed grid
                    WLX_SLOT_PX(120),  // fixed grid
                    WLX_SLOT_CONTENT,  // sub: content-sized rows
                    WLX_SLOT_PX(100),  // content-sized rows grid
                    WLX_SLOT_CONTENT,  // sub: auto grid
                    WLX_SLOT_PX(140),  // auto grid (3 rows x 40 + padding)
                    WLX_SLOT_CONTENT,  // sub: auto tile grid
                    WLX_SLOT_PX(140),  // auto tile grid (2 rows x 60 + padding)
                    WLX_SLOT_CONTENT,  // sub: cell positioning
                    WLX_SLOT_PX(120)   // cell positioning grid
                ));

                SECTION_HEADING(ctx, "Grid Layouts");

                SUB_HEADING(ctx, "Fixed Grid (wlx_grid_begin)");
                wlx_grid_begin(ctx, (size_t)rows, (size_t)cols);
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
                                .height = -1, .font_size = SMALL_FS, .align = WLX_CENTER,
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
                    wlx_grid_begin(ctx, 3, 3, .row_sizes = rsizes);
                        for (int c = 0; c < 3; c++) {
                            wlx_push_id(ctx, (size_t)c);
                            WLX_Color clr = { (unsigned char)(50 + c * 30), 70, 140, 255 };
                            wlx_label(ctx, "Tall", .height = 40, .font_size = TINY_FS,
                                .align = WLX_CENTER, .back_color = clr,
                                .show_background = true);
                            wlx_pop_id(ctx);
                        }
                        for (int c = 0; c < 3; c++) {
                            wlx_push_id(ctx, (size_t)(3 + c));
                            WLX_Color clr = { 70, (unsigned char)(50 + c * 30), 140, 255 };
                            wlx_label(ctx, "Min 20", .height = 20, .font_size = TINY_FS,
                                .align = WLX_CENTER, .back_color = clr,
                                .show_background = true);
                            wlx_pop_id(ctx);
                        }
                        for (int c = 0; c < 3; c++) {
                            wlx_push_id(ctx, (size_t)(6 + c));
                            WLX_Color clr = { 70, 140, (unsigned char)(50 + c * 30), 255 };
                            wlx_label(ctx, "Max 30", .height = 35, .font_size = TINY_FS,
                                .align = WLX_CENTER, .back_color = clr,
                                .show_background = true);
                            wlx_pop_id(ctx);
                        }
                    wlx_layout_end(ctx);
                }

                SUB_HEADING(ctx, "Auto Grid (wlx_grid_begin_auto)");
                wlx_grid_begin_auto(ctx, 4, 40);
                    for (int i = 0; i < 12; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        char buf[16];
                        snprintf(buf, sizeof(buf), "Cell %d", i);
                        WLX_Color clr = { (unsigned char)(60 + i * 15), 80, (unsigned char)(180 - i * 10), 255 };
                        wlx_label(ctx, buf,
                            .height = -1, .font_size = TINY_FS, .align = WLX_CENTER,
                            .back_color = clr, .show_background = true);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Auto Tile Grid (wlx_grid_begin_auto_tile)");
                wlx_grid_begin_auto_tile(ctx, 100, 60);
                    for (int i = 0; i < 10; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        char buf[16];
                        snprintf(buf, sizeof(buf), "Tile %d", i);
                        WLX_Color clr = { (unsigned char)(80 + i * 10), (unsigned char)(60 + i * 15), 120, 255 };
                        wlx_label(ctx, buf,
                            .height = -1, .font_size = TINY_FS, .align = WLX_CENTER,
                            .back_color = clr, .show_background = true);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Grid Cell Positioning (wlx_grid_cell)");
                wlx_grid_begin(ctx, 3, 3);
                    wlx_grid_cell(ctx, 0, 0, .col_span = 2);
                    wlx_label(ctx, "Span 2 cols",
                        .height = -1, .font_size = SMALL_FS, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 62, 22, 42), .show_background = true);

                    wlx_grid_cell(ctx, 0, 2, .row_span = 2);
                    wlx_label(ctx, "Span 2 rows",
                        .height = -1, .font_size = SMALL_FS, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 22, 62, 42), .show_background = true);

                    wlx_grid_cell(ctx, 1, 0);
                    wlx_label(ctx, "1,0",
                        .height = -1, .font_size = SMALL_FS, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 42, 42, 62), .show_background = true);

                    wlx_grid_cell(ctx, 1, 1);
                    wlx_label(ctx, "1,1",
                        .height = -1, .font_size = SMALL_FS, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 42, 62, 42), .show_background = true);

                    wlx_grid_cell(ctx, 2, 0, .col_span = 3);
                    wlx_label(ctx, "Full width (span 3 cols)",
                        .height = -1, .font_size = SMALL_FS, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 42, 42, 42), .show_background = true);
                wlx_layout_end(ctx);

        wlx_layout_end(ctx);

    SECTION_END(ctx);
}

// ---------------------------------------------------------------------------
// Section: Flex & Sizing
// ---------------------------------------------------------------------------
static void section_layout_flex(WLX_Context *ctx, Gallery_State *st) {
    SECTION_BEGIN(ctx);

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


            wlx_slider(ctx, "Weight A   ", &st->flex_weight_a,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 0.1f, .max_value = 5.0f);
            wlx_slider(ctx, "Weight B   ", &st->flex_weight_b,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 0.1f, .max_value = 5.0f);
            wlx_slider(ctx, "Min        ", &st->flex_min,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 0.0f, .max_value = 400.0f);
            wlx_slider(ctx, "Max        ", &st->flex_max,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 100.0f, .max_value = 800.0f);

        wlx_panel_end(ctx);

    SECTION_NEXT(ctx);

        // ======== RIGHT: Content ========
            wlx_layout_begin_s(ctx, WLX_VERT,
                WLX_SIZES(
                    WLX_SLOT_CONTENT,  // heading
                    WLX_SLOT_CONTENT,  // sub: flex weight
                    WLX_SLOT_CONTENT,  // description
                    WLX_SLOT_PX(60),   // flex weight demo
                    WLX_SLOT_CONTENT,  // sub: px + flex
                    WLX_SLOT_PX(50),   // px + flex demo
                    WLX_SLOT_CONTENT,  // sub: content-fit
                    WLX_SLOT_PX(130),  // content-fit demo
                    WLX_SLOT_CONTENT,  // sub: min/max constraints
                    WLX_SLOT_PX(50),   // min/max demo
                    WLX_SLOT_CONTENT,  // sub: widget min/max
                    WLX_SLOT_PX(40),   // widget min/max demo
                    WLX_SLOT_CONTENT,  // sub: overflow
                    WLX_SLOT_PX(40)    // overflow demo
                ));

                SECTION_HEADING(ctx, "Flex & Sizing");

                SUB_HEADING(ctx, "Flex Weight Distribution");
                wlx_label(ctx, "Sidebar + Content pattern using FLEX weights:",
                    .font_size = SMALL_FS, .height = SMALL_H);

                {
                    float wa = st->flex_weight_a;
                    float wb = st->flex_weight_b;
                    if (wa < 0.1f) wa = 0.1f;
                    if (wb < 0.1f) wb = 0.1f;

                    wlx_layout_begin_s(ctx, WLX_HORZ,
                        WLX_SIZES(WLX_SLOT_FLEX(wa), WLX_SLOT_FLEX(wb)));
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
                    WLX_SIZES(WLX_SLOT_PX(120), WLX_SLOT_FLEX(1), WLX_SLOT_PX(120)));
                    wlx_label(ctx, "PX(120)",
                        .height = 50, .font_size = SMALL_FS, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 62, 32, 32), .show_background = true);
                    wlx_label(ctx, "FLEX(1)",
                        .height = 50, .font_size = SMALL_FS, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 32, 62, 32), .show_background = true);
                    wlx_label(ctx, "PX(120)",
                        .height = 50, .font_size = SMALL_FS, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 32, 32, 62), .show_background = true);
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Content-Fit Slots (CONTENT)");
                wlx_layout_begin_s(ctx, WLX_VERT,
                    WLX_SIZES(WLX_SLOT_PX(150)));
                    wlx_layout_begin_s(ctx, WLX_VERT,
                        WLX_SIZES(WLX_SLOT_CONTENT, WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1)));
                        wlx_label(ctx, "CONTENT (h=30)",
                            .height = DESC_H, .font_size = SMALL_FS, .align = WLX_CENTER,
                            .back_color = heading_color(ctx->theme, 32, 52, 32), .show_background = true);
                        wlx_label(ctx, "CONTENT (h=50)",
                            .height = 50, .font_size = SMALL_FS, .align = WLX_CENTER,
                            .back_color = heading_color(ctx->theme, 52, 32, 32), .show_background = true);
                        wlx_label(ctx, "FLEX(1) (remaining)",
                            .font_size = SMALL_FS, .align = WLX_CENTER,
                            .back_color = heading_color(ctx->theme, 32, 32, 52), .show_background = true);
                    wlx_layout_end(ctx);
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Min/Max Constraints");
                {
                    float lo = st->flex_min;
                    float hi = st->flex_max;
                    if (hi < lo) hi = lo;

                    wlx_layout_begin_s(ctx, WLX_HORZ,
                        WLX_SIZES(WLX_SLOT_FLEX_MINMAX(1, lo, hi), WLX_SLOT_FLEX(1)));
                        char buf[64];
                        snprintf(buf, sizeof(buf), "FLEX_MINMAX(1, %.0f, %.0f)", lo, hi);
                        wlx_label(ctx, buf,
                            .height = 50, .font_size = TINY_FS, .align = WLX_CENTER,
                            .back_color = heading_color(ctx->theme, 52, 32, 52), .show_background = true);
                        wlx_label(ctx, "FLEX(1)",
                            .height = 50, .font_size = TINY_FS, .align = WLX_CENTER,
                            .back_color = heading_color(ctx->theme, 32, 52, 52), .show_background = true);
                    wlx_layout_end(ctx);
                }

                SUB_HEADING(ctx, "Widget-Level Min/Max");
                wlx_layout_begin(ctx, 1, WLX_HORZ);
                    wlx_label(ctx, "min_w=100, max_w=300, centered",
                        .widget_align = WLX_CENTER,
                        .min_width = 100, .max_width = 300,
                        .height = 40, .font_size = SMALL_FS, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 42, 42, 62), .show_background = true);
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Overflow");
                wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 10, .border_color = heading_color(ctx->theme, 62, 42, 22), .border_width = 1 );
                    wlx_label(ctx, "overflow=true, width=500 (exceeds slot if narrow)",
                        .overflow = true, .width = 500,
                        .height = 40, .font_size = SMALL_FS, .align = WLX_CENTER,
                        .back_color = heading_color(ctx->theme, 62, 42, 22), .show_background = true);
                wlx_layout_end(ctx);

        wlx_layout_end(ctx);

    SECTION_END(ctx);
}

// ---------------------------------------------------------------------------
// Section: Theme Lab
// ---------------------------------------------------------------------------

static void gallery_update_custom_theme(Gallery_State *st, WLX_Font font) {
    st->custom_theme = wlx_theme_dark;
    st->custom_theme.background = to_color(st->theme_bg.r, st->theme_bg.g, st->theme_bg.b, 1.0f);
    st->custom_theme.foreground = to_color(st->theme_fg.r, st->theme_fg.g, st->theme_fg.b, 1.0f);
    st->custom_theme.surface = to_color(st->theme_surface.r, st->theme_surface.g, st->theme_surface.b, 1.0f);
    st->custom_theme.border = to_color(st->theme_border.r, st->theme_border.g, st->theme_border.b, 1.0f);
    st->custom_theme.accent = to_color(st->theme_accent.r, st->theme_accent.g, st->theme_accent.b, 1.0f);
    st->custom_theme.hover_brightness = st->theme_hover_brightness;
    st->custom_theme.padding = st->theme_padding;
    st->custom_theme.roundness = st->theme_roundness;
    st->custom_theme.font = font;
    st->custom_theme.input.border_focus = st->custom_theme.accent;
    st->custom_theme.input.cursor = st->custom_theme.foreground;
    st->custom_theme.slider.track = gallery_semantic_theme(&st->custom_theme).color_surface_3;
    st->custom_theme.slider.thumb = st->custom_theme.foreground;
    st->custom_theme.slider.label = st->custom_theme.foreground;
    st->custom_theme.checkbox.border = st->custom_theme.border;
    st->custom_theme.checkbox.check = st->custom_theme.accent;
    st->custom_theme.toggle.track_active = st->custom_theme.accent;
    st->custom_theme.radio.fill = st->custom_theme.accent;
    st->custom_theme.progress.fill = st->custom_theme.accent;
    st->custom_theme.progress.track = st->custom_theme.slider.track;
    st->custom_theme.scrollbar.bar = st->custom_theme.slider.track;
}

static WLX_Theme gallery_brand_theme(WLX_Font font) {
    WLX_Theme theme = wlx_theme_dark;
    theme.background = (WLX_Color){18, 20, 20, 255};
    theme.foreground = (WLX_Color){230, 233, 226, 255};
    theme.surface = (WLX_Color){35, 39, 39, 255};
    theme.border = (WLX_Color){58, 68, 64, 255};
    theme.accent = (WLX_Color){96, 190, 174, 255};
    theme.border_width = 0.5f;
    theme.padding = 2.0f;
    theme.roundness = 0.0f;
    theme.rounded_segments = 0;
    theme.hover_brightness = 0.07f;
    theme.font = font;
    theme.input.border_focus = theme.accent;
    theme.input.cursor = theme.foreground;
    theme.slider.track = (WLX_Color){53, 60, 58, 255};
    theme.slider.thumb = (WLX_Color){230, 233, 226, 255};
    theme.slider.label = theme.foreground;
    theme.checkbox.border = theme.border;
    theme.checkbox.check = theme.accent;
    theme.toggle.track_active = theme.accent;
    theme.radio.fill = theme.accent;
    theme.progress.track = theme.slider.track;
    theme.progress.fill = theme.accent;
    theme.scrollbar.bar = (WLX_Color){57, 64, 62, 255};
    return theme;
}

static void theme_preview_widgets(WLX_Context *ctx) {
    static bool preview_check = true;
    static float preview_slider = 0.58f;

    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(28), WLX_SLOT_PX(28), WLX_SLOT_PX(28)),
        .padding = 4, .gap = 4);
        wlx_button(ctx, "Button", .height = 28, .font_size = SMALL_FS, .align = WLX_CENTER);
        wlx_slider(ctx, "Value", &preview_slider, .height = 28, .font_size = TINY_FS);
        wlx_checkbox(ctx, "Check", &preview_check, .height = 28, .font_size = SMALL_FS);
    wlx_layout_end(ctx);
}

static void theme_sample_widgets(WLX_Context *ctx) {
    static bool t_check = true;
    static bool t_toggle = true;
    static float t_slider = 0.5f;
    static float t_progress = 0.64f;
    static char t_input[64] = "Edit me";

    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H)
        ),
        .padding = 6, .gap = 4);
        wlx_button(ctx, "Sample Button", .height = ROW_H, .align = WLX_CENTER);
        wlx_checkbox(ctx, "Sample Checkbox", &t_check, .height = ROW_H);
        wlx_slider(ctx, "Sample  ", &t_slider, .height = ROW_H);
        wlx_inputbox(ctx, "Input:", t_input, sizeof(t_input), .height = ROW_H);
        wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 0, .gap = 4);
            wlx_toggle(ctx, "Toggle", &t_toggle, .height = ROW_H);
            wlx_progress(ctx, t_progress, .height = ROW_H);
        wlx_layout_end(ctx);
    wlx_layout_end(ctx);
}

static void theme_lab_preview_card(WLX_Context *ctx, const char *name, const char *tag,
    const WLX_Theme *theme, bool active, size_t id) {
    const WLX_Theme *saved = ctx->theme;
    Gallery_Semantic_Theme preview = gallery_semantic_theme(theme);
    WLX_Color header_bg = active ? preview.color_selection : preview.color_surface_2;

    ctx->theme = theme;
    wlx_push_id(ctx, id);
        wlx_layout_begin_s(ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_PX(26), WLX_SLOT_PX(18), WLX_SLOT_FLEX(1)),
            .padding = 5, .gap = 4,
            .back_color = preview.color_app_bg,
            .border_color = active ? preview.color_selection : preview.color_border_strong,
            .border_width = active ? 1.0f : 0.5f);
            wlx_label(ctx, name,
                .height = 26, .font_size = DEMO_FS, .align = WLX_CENTER,
                .back_color = header_bg,
                .front_color = gallery_on_color(header_bg));
            wlx_label(ctx, tag,
                .height = 18, .font_size = TINY_FS, .align = WLX_CENTER,
                .front_color = preview.color_text_muted);
            theme_preview_widgets(ctx);
        wlx_layout_end(ctx);
    wlx_pop_id(ctx);
    ctx->theme = saved;
}

static void theme_lab_control_label(WLX_Context *ctx, const char *label) {
    wlx_label(ctx, label,
        .font_size = OPT_FS, .height = SMALL_H, .align = WLX_LEFT,
        .show_background = true,
        .back_color = SECTION_BG(ctx),
        .front_color = GALLERY_ROLE(ctx, color_text_1));
}

enum {
    THEME_LAB_TOKEN_COUNT = 15,
    THEME_LAB_PRESET_COUNT = 5,
};

#define THEME_LAB_TOKEN_ROW_H 76.0f
#define THEME_LAB_TOKEN_GRID_PADDING 6.0f
#define THEME_LAB_TOKEN_GRID_GAP 6.0f
#define THEME_LAB_PRESET_ROW_H 166.0f
#define THEME_LAB_PRESET_GRID_GAP 4.0f

static int gallery_responsive_columns(float available_width, int preferred_columns, float min_cell_width) {
    if (preferred_columns < 1) return 1;
    if (min_cell_width <= 0.0f) return preferred_columns;

    int column_count = (int)(available_width / min_cell_width);
    if (column_count < 1) column_count = 1;
    if (column_count > preferred_columns) column_count = preferred_columns;
    return column_count;
}

static float gallery_auto_grid_height(int item_count, int column_count,
    float row_height, float padding, float gap) {
    if (item_count < 1) return padding * 2.0f;
    if (column_count < 1) column_count = 1;

    int row_count = (item_count + column_count - 1) / column_count;
    float gap_height = (row_count > 1) ? (float)(row_count - 1) * gap : 0.0f;
    return padding * 2.0f + (float)row_count * row_height + gap_height;
}

static int theme_lab_token_columns(float available_width) {
    return gallery_responsive_columns(available_width, 3, 132.0f);
}

static float theme_lab_token_grid_height(int column_count) {
    return gallery_auto_grid_height(THEME_LAB_TOKEN_COUNT, column_count,
        THEME_LAB_TOKEN_ROW_H, THEME_LAB_TOKEN_GRID_PADDING, THEME_LAB_TOKEN_GRID_GAP);
}

static int theme_lab_preset_columns(float available_width) {
    return gallery_responsive_columns(available_width, 3, 150.0f);
}

static float theme_lab_preset_grid_height(int column_count) {
    return gallery_auto_grid_height(THEME_LAB_PRESET_COUNT, column_count,
        THEME_LAB_PRESET_ROW_H, 0.0f, THEME_LAB_PRESET_GRID_GAP);
}

static void theme_lab_token_card(WLX_Context *ctx, const Gallery_Semantic_Theme *semantic,
    const char *name, WLX_Color dark_color, WLX_Color light_color, size_t id) {
    wlx_push_id(ctx, id);
        wlx_layout_begin_s(ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_PX(18), WLX_SLOT_PX(30), WLX_SLOT_PX(16)),
            .padding = 4, .gap = 3,
            .back_color = semantic->color_surface_1,
            .border_color = semantic->color_border,
            .border_width = 1.0f);
            wlx_label(ctx, name,
                .height = 18, .font_size = TINY_FS, .align = WLX_CENTER,
                .front_color = semantic->color_text_1);
            wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 0, .gap = 3);
                wlx_widget(ctx, .height = 30, .color = dark_color,
                    .border_color = semantic->color_border_strong, .border_width = 1.0f);
                wlx_widget(ctx, .height = 30, .color = light_color,
                    .border_color = semantic->color_border_strong, .border_width = 1.0f);
            wlx_layout_end(ctx);
            wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 0, .gap = 3);
                wlx_label(ctx, "dark", .height = 16, .font_size = TINY_FS, .align = WLX_CENTER,
                    .front_color = semantic->color_text_muted);
                wlx_label(ctx, "light", .height = 16, .font_size = TINY_FS, .align = WLX_CENTER,
                    .front_color = semantic->color_text_muted);
            wlx_layout_end(ctx);
        wlx_layout_end(ctx);
    wlx_pop_id(ctx);
}

static void theme_lab_token_swatches(WLX_Context *ctx, const Gallery_Semantic_Theme *semantic) {
    Gallery_Semantic_Theme dark = gallery_semantic_theme(&wlx_theme_dark);
    Gallery_Semantic_Theme light = gallery_semantic_theme(&wlx_theme_light);
    const char *names[THEME_LAB_TOKEN_COUNT] = {
        "app_bg", "surface_1", "surface_2", "surface_3", "text_1",
        "text_2", "text_muted", "border", "border_strong", "accent",
        "focus", "selection", "success", "warning", "danger",
    };
    WLX_Color dark_values[THEME_LAB_TOKEN_COUNT] = {
        dark.color_app_bg, dark.color_surface_1, dark.color_surface_2, dark.color_surface_3, dark.color_text_1,
        dark.color_text_2, dark.color_text_muted, dark.color_border, dark.color_border_strong, dark.color_accent,
        dark.color_focus, dark.color_selection, dark.color_success, dark.color_warning, dark.color_danger,
    };
    WLX_Color light_values[THEME_LAB_TOKEN_COUNT] = {
        light.color_app_bg, light.color_surface_1, light.color_surface_2, light.color_surface_3, light.color_text_1,
        light.color_text_2, light.color_text_muted, light.color_border, light.color_border_strong, light.color_accent,
        light.color_focus, light.color_selection, light.color_success, light.color_warning, light.color_danger,
    };

    int column_count = theme_lab_token_columns(wlx_get_available_width(ctx));
    wlx_grid_begin_auto(ctx, column_count, THEME_LAB_TOKEN_ROW_H,
        .padding = THEME_LAB_TOKEN_GRID_PADDING, .gap = THEME_LAB_TOKEN_GRID_GAP,
        .back_color = semantic->color_surface_2,
        .slot_border_color = semantic->color_border,
        .slot_border_width = 0.5f);
        for (int i = 0; i < THEME_LAB_TOKEN_COUNT; i++) {
            theme_lab_token_card(ctx, semantic, names[i], dark_values[i], light_values[i], (size_t)i);
        }
    wlx_layout_end(ctx);
}

static void section_tokens(WLX_Context *ctx, Gallery_State *st) {
    (void)st;
    Gallery_Semantic_Theme semantic = gallery_semantic_theme(ctx->theme);

    SECTION_BEGIN(ctx);

        wlx_panel_begin(ctx, .title = "Token Snapshot",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

            wlx_label(ctx, "Color Roles",
                .height = SUB_HEADING_H, .font_size = DEMO_FS, .align = WLX_LEFT,
                .front_color = semantic.color_text_1);
            wlx_label(ctx,
                "app_bg\nsurface_1 / surface_2 / surface_3\ntext_1 / text_2 / text_muted\nborder / border_strong\naccent / focus / selection\nstatus colors",
                .height = 150, .font_size = OPT_FS, .align = WLX_LEFT,
                .front_color = semantic.color_text_2);

            wlx_label(ctx, "Text Scale",
                .height = SUB_HEADING_H, .font_size = DEMO_FS, .align = WLX_LEFT,
                .front_color = semantic.color_text_1);
            {
                char type_buf[128];
                snprintf(type_buf, sizeof(type_buf),
                    "section %d\ndemo    %d\nsmall   %d\ntiny    %d",
                    SECTION_FS, DEMO_FS, SMALL_FS, TINY_FS);
                wlx_label(ctx, type_buf,
                    .height = 104, .font_size = OPT_FS, .align = WLX_LEFT,
                    .front_color = semantic.color_text_2);
            }

            wlx_label(ctx, "Theme Geometry",
                .height = SUB_HEADING_H, .font_size = DEMO_FS, .align = WLX_LEFT,
                .front_color = semantic.color_text_1);
            {
                char geometry_buf[160];
                snprintf(geometry_buf, sizeof(geometry_buf),
                    "padding          %.1f\nroundness        %.2f\nborder width     %.1f\nhover brightness %.2f",
                    ctx->theme->padding,
                    ctx->theme->roundness,
                    ctx->theme->border_width,
                    ctx->theme->hover_brightness);
                wlx_label(ctx, geometry_buf,
                    .height = 112, .font_size = OPT_FS, .align = WLX_LEFT,
                    .front_color = semantic.color_text_2);
            }

        wlx_panel_end(ctx);

    SECTION_NEXT(ctx);

        float token_width = wlx_get_available_width(ctx);
        float token_height = theme_lab_token_grid_height(theme_lab_token_columns(token_width));

        wlx_layout_begin_s(ctx, WLX_VERT,
            WLX_SIZES(
                WLX_SLOT_CONTENT,
                WLX_SLOT_CONTENT,
                WLX_SLOT_CONTENT,
                WLX_SLOT_PX(token_height),
                WLX_SLOT_CONTENT,
                WLX_SLOT_PX(86)
            ));

            SECTION_HEADING(ctx, "Tokens");

            wlx_label(ctx, "Semantic roles used by the gallery shell and theme previews.",
                .height = DESC_H,
                .front_color = semantic.color_text_2);

            SUB_HEADING(ctx, "Semantic Color Roles");
            theme_lab_token_swatches(ctx, &semantic);

            SUB_HEADING(ctx, "Interaction Scale");
            wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 0, .gap = 4);
                wlx_label(ctx, "Hover",
                    .height = 86, .font_size = DEMO_FS, .align = WLX_CENTER,
                    .show_background = true,
                    .back_color = semantic.color_surface_3,
                    .front_color = semantic.color_text_1);
                wlx_label(ctx, "Focus",
                    .height = 86, .font_size = DEMO_FS, .align = WLX_CENTER,
                    .show_background = true,
                    .back_color = semantic.color_focus,
                    .front_color = gallery_on_color(semantic.color_focus));
                wlx_label(ctx, "Selected",
                    .height = 86, .font_size = DEMO_FS, .align = WLX_CENTER,
                    .show_background = true,
                    .back_color = semantic.color_selection,
                    .front_color = gallery_on_color(semantic.color_selection));
            wlx_layout_end(ctx);

        wlx_layout_end(ctx);

    SECTION_END(ctx);
}

static WLX_Color theme_lab_state_back(const Gallery_Semantic_Theme *semantic, int state) {
    switch (state) {
        case 1: return semantic->color_surface_3;
        case 2: return semantic->color_selection;
        case 3: return semantic->color_surface_2;
        case 4: return semantic->color_surface_1;
        default: return semantic->color_surface_2;
    }
}

static WLX_Color theme_lab_state_front(const Gallery_Semantic_Theme *semantic, WLX_Color back, int state) {
    if (state == 2) return gallery_on_color(back);
    if (state == 4) return semantic->color_text_muted;
    return semantic->color_text_1;
}

static WLX_Color theme_lab_state_border(const Gallery_Semantic_Theme *semantic, int state) {
    if (state == 3) return semantic->color_focus;
    if (state == 2) return semantic->color_accent;
    return semantic->color_border;
}

static void theme_lab_component_matrix(WLX_Context *ctx, const Gallery_Semantic_Theme *semantic) {
    static bool matrix_checks[5] = { false, false, true, true, true };
    static float matrix_sliders[5] = { 0.35f, 0.45f, 0.72f, 0.58f, 0.25f };
    static char matrix_inputs[5][32] = { "Default", "Hover", "Active", "Focus", "Disabled" };
    static const char *states[] = { "Default", "Hover", "Active", "Focus", "Disabled" };

    // 6 rows x 46 px + 5 gaps x 4 = 296 (matches the loop below)
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(24), WLX_SLOT_PX(296)),
        .padding = 6, .gap = 4,
        .back_color = semantic->color_surface_2,
        .border_color = semantic->color_border_strong,
        .border_width = 1.0f);
        wlx_layout_begin(ctx, 5, WLX_HORZ, .padding = 0, .gap = 4);
            for (int i = 0; i < 5; i++) {
                wlx_push_id(ctx, (size_t)i);
                wlx_label(ctx, states[i],
                    .height = 24, .font_size = TINY_FS, .align = WLX_CENTER,
                    .back_color = theme_lab_state_back(semantic, i),
                    .front_color = theme_lab_state_front(semantic, theme_lab_state_back(semantic, i), i),
                    .border_color = theme_lab_state_border(semantic, i),
                    .border_width = 0.5f);
                wlx_pop_id(ctx);
            }
        wlx_layout_end(ctx);

        wlx_grid_begin_auto(ctx, 5, 46, .padding = 0, .gap = 4);
            for (int row = 0; row < 6; row++) {
                for (int state = 0; state < 5; state++) {
                    WLX_Color back = theme_lab_state_back(semantic, state);
                    WLX_Color front = theme_lab_state_front(semantic, back, state);
                    WLX_Color border = theme_lab_state_border(semantic, state);
                    float opacity = (state == 4) ? 0.45f : 1.0f;
                    float border_width = (state == 3) ? 1.5f : 0.5f;

                    wlx_push_id(ctx, (size_t)(1000 + row * 16 + state));
                    switch (row) {
                        case 0:
                            wlx_label(ctx, "Label",
                                .height = 46, .font_size = SMALL_FS, .align = WLX_CENTER,
                                .show_background = true,
                                .back_color = back, .front_color = front,
                                .border_color = border, .border_width = border_width,
                                .opacity = opacity);
                            break;
                        case 1:
                            wlx_button(ctx, "Button",
                                .height = 46, .font_size = SMALL_FS, .align = WLX_CENTER,
                                .back_color = back,
                                .front_color = front,
                                .border_color = border, .border_width = border_width,
                                .opacity = opacity);
                            break;
                        case 2:
                            wlx_checkbox(ctx, "Checkbox", &matrix_checks[state],
                                .height = 46, .font_size = SMALL_FS,
                                .back_color = back, .front_color = front,
                                .border_color = border, .border_width = border_width,
                                .check_color = semantic->color_accent,
                                .opacity = opacity);
                            break;
                        case 3:
                            wlx_slider(ctx, "Slider", &matrix_sliders[state],
                                .height = 46, .font_size = TINY_FS,
                                .track_color = (state == 2) ? semantic->color_selection : semantic->color_surface_3,
                                .thumb_color = (state == 4) ? semantic->color_text_muted : semantic->color_accent,
                                .label_color = front,
                                .border_color = border, .border_width = border_width,
                                .opacity = opacity);
                            break;
                        case 4:
                            wlx_inputbox(ctx, "Input", matrix_inputs[state], sizeof(matrix_inputs[state]),
                                .height = 46, .font_size = TINY_FS,
                                .back_color = back, .front_color = front,
                                .border_color = border, .border_width = border_width,
                                .border_focus_color = semantic->color_focus,
                                .cursor_color = semantic->color_focus,
                                .opacity = opacity);
                            break;
                        default:
                            wlx_layout_begin_s(ctx, WLX_VERT,
                                WLX_SIZES(WLX_SLOT_PX(14), WLX_SLOT_PX(24)),
                                .padding = 2, .gap = 2,
                                .back_color = back,
                                .border_color = border,
                                .border_width = border_width);
                                wlx_label(ctx, "Progress",
                                    .height = 14, .font_size = TINY_FS, .align = WLX_CENTER,
                                    .front_color = front,
                                    .opacity = opacity);
                                wlx_progress(ctx, matrix_sliders[state],
                                    .height = 24,
                                    .track_color = semantic->color_surface_3,
                                    .fill_color = (state == 4) ? semantic->color_text_muted : semantic->color_accent,
                                    .opacity = opacity);
                            wlx_layout_end(ctx);
                            break;
                    }
                    wlx_pop_id(ctx);
                }
            }
        wlx_layout_end(ctx);
    wlx_layout_end(ctx);
}

static void section_theme_lab(WLX_Context *ctx, Gallery_State *st) {
    WLX_Font custom_font = ctx->theme ? ctx->theme->font : WLX_FONT_DEFAULT;
    gallery_update_custom_theme(st, custom_font);

    Gallery_Semantic_Theme semantic = gallery_semantic_theme(ctx->theme);

    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_size = WLX_SLOT_PCT_MINMAX(30, 180, 280),
        .first_back_color = SPLIT_BG(ctx));

        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_PALETTE, GALLERY_ICON_ROLE_ACCENT,
                "Theme Controls", HEADING_H);

            wlx_push_id(ctx, 200);
            theme_lab_control_label(ctx, "Background");
            wlx_pop_id(ctx);
            wlx_push_id(ctx, 100);
            color_sliders(ctx, "BG", &st->theme_bg, OPT_H, OPT_FS);
            wlx_pop_id(ctx);

            wlx_push_id(ctx, 201);
            theme_lab_control_label(ctx, "Surface");
            wlx_pop_id(ctx);
            wlx_push_id(ctx, 101);
            color_sliders(ctx, "Srf", &st->theme_surface, OPT_H, OPT_FS);
            wlx_pop_id(ctx);

            wlx_push_id(ctx, 202);
            theme_lab_control_label(ctx, "Text");
            wlx_pop_id(ctx);
            wlx_push_id(ctx, 102);
            color_sliders(ctx, "Txt", &st->theme_fg, OPT_H, OPT_FS);
            wlx_pop_id(ctx);

            wlx_push_id(ctx, 203);
            theme_lab_control_label(ctx, "Accent");
            wlx_pop_id(ctx);
            wlx_push_id(ctx, 103);
            color_sliders(ctx, "Acc", &st->theme_accent, OPT_H, OPT_FS);
            wlx_pop_id(ctx);

            wlx_push_id(ctx, 204);
            theme_lab_control_label(ctx, "Border");
            wlx_pop_id(ctx);
            wlx_push_id(ctx, 104);
            color_sliders(ctx, "Bdr", &st->theme_border, OPT_H, OPT_FS);
            wlx_pop_id(ctx);

            wlx_push_id(ctx, 205);
            theme_lab_control_label(ctx, "Interaction");
            wlx_pop_id(ctx);
            wlx_slider(ctx, "Hover Brt", &st->theme_hover_brightness, .height = OPT_H, .font_size = OPT_FS,
                .min_value = -0.25f, .max_value = 0.85f);
            wlx_slider(ctx, "Padding  ", &st->theme_padding, .height = OPT_H, .font_size = OPT_FS,
                .min_value = 0.0f, .max_value = 20.0f);
            wlx_slider(ctx, "Roundness", &st->theme_roundness, .height = OPT_H, .font_size = OPT_FS,
                .min_value = 0.0f, .max_value = 1.0f);

            if (gallery_icon_button(ctx, "Use Custom Theme",
                WLX_ICON_CHECK, GALLERY_ICON_ROLE_SUCCESS,
                .height = OPT_H, .font_size = OPT_FS, .align = WLX_CENTER,
                .back_color = semantic.color_selection,
                .front_color = gallery_on_color(semantic.color_selection),
                .image_size = 16,
                .image_text_gap = 8)) {
                st->theme_mode = GALLERY_THEME_CUSTOM;
            }

        wlx_panel_end(ctx);

    SECTION_NEXT(ctx);

        gallery_update_custom_theme(st, custom_font);
        semantic = gallery_semantic_theme(ctx->theme);
        float theme_lab_width = wlx_get_available_width(ctx);
        int token_columns = theme_lab_token_columns(theme_lab_width);
        int preset_columns = theme_lab_preset_columns(theme_lab_width);
        float token_height = theme_lab_token_grid_height(token_columns);
        float preset_height = theme_lab_preset_grid_height(preset_columns);

        wlx_layout_begin_s(ctx, WLX_VERT,
            WLX_SIZES(
                WLX_SLOT_CONTENT,
                WLX_SLOT_CONTENT,
                WLX_SLOT_CONTENT,
                WLX_SLOT_PX(token_height),
                WLX_SLOT_CONTENT,
                WLX_SLOT_PX(preset_height),
                WLX_SLOT_CONTENT,
                WLX_SLOT_PX(276),
                WLX_SLOT_CONTENT,
                WLX_SLOT_PX(336),
                WLX_SLOT_CONTENT,
                WLX_SLOT_PX(84),
                WLX_SLOT_PX(84)
            ));

            SECTION_HEADING(ctx, "Theme Lab");

            wlx_label(ctx, "Tokens, presets, controls, states.",
                .height = DESC_H,
                .front_color = semantic.color_text_2);

            SUB_HEADING(ctx, "Semantic Token Swatches");
            theme_lab_token_swatches(ctx, &semantic);

            SUB_HEADING(ctx, "Theme Presets");
            {
                WLX_Theme dark_theme = gallery_theme_copy_with_font(wlx_theme_dark, custom_font);
                WLX_Theme light_theme = gallery_theme_copy_with_font(wlx_theme_light, custom_font);
                WLX_Theme glass_theme = gallery_theme_copy_with_font(wlx_theme_glass, custom_font);
                WLX_Theme brand_theme = gallery_brand_theme(custom_font);
                WLX_Theme custom_theme = gallery_theme_copy_with_font(st->custom_theme, custom_font);
                const struct {
                    const char *name;
                    const char *tag;
                    const WLX_Theme *theme;
                    int mode;
                } previews[] = {
                    { "Dark", "built-in", &dark_theme, GALLERY_THEME_DARK },
                    { "Light", "built-in", &light_theme, GALLERY_THEME_LIGHT },
                    { "Glass", "built-in", &glass_theme, GALLERY_THEME_GLASS },
                    { "Brand", "gallery-local", &brand_theme, GALLERY_THEME_BRAND },
                    { "Custom", "editable", &custom_theme, GALLERY_THEME_CUSTOM },
                };
                wlx_grid_begin_auto(ctx, preset_columns, THEME_LAB_PRESET_ROW_H,
                    .padding = 0, .gap = THEME_LAB_PRESET_GRID_GAP);
                    for (int i = 0; i < THEME_LAB_PRESET_COUNT; i++) {
                        theme_lab_preview_card(ctx, previews[i].name, previews[i].tag,
                            previews[i].theme, previews[i].mode == st->theme_mode, (size_t)i);
                    }
                wlx_layout_end(ctx);
            }

            SUB_HEADING(ctx, "Custom Theme Live Preview");
            {
                const WLX_Theme *saved = ctx->theme;
                Gallery_Semantic_Theme custom_semantic = gallery_semantic_theme(&st->custom_theme);
                ctx->theme = &st->custom_theme;
                wlx_push_id(ctx, 300);
                    wlx_layout_begin_s(ctx, WLX_VERT,
                        WLX_SIZES(WLX_SLOT_PX(28), WLX_SLOT_FLEX(1)),
                        .back_color = custom_semantic.color_app_bg,
                        .border_color = custom_semantic.color_border_strong,
                        .border_width = 1.0f,
                        .gap = 4);
                        wlx_label(ctx, "Custom",
                            .height = 28, .font_size = DEMO_FS, .align = WLX_CENTER,
                            .back_color = custom_semantic.color_surface_2,
                            .front_color = custom_semantic.color_text_1,
                            .border_color = custom_semantic.color_border,
                            .border_width = 0.5f);
                        theme_sample_widgets(ctx);
                    wlx_layout_end(ctx);
                wlx_pop_id(ctx);
                ctx->theme = saved;
            }

            SUB_HEADING(ctx, "Component Preview Matrix");
            theme_lab_component_matrix(ctx, &semantic);

            SUB_HEADING(ctx, "Icon Tints (cross-theme)");
            if (gallery_icon_atlas_ready()) {
                WLX_Font custom_font_local = ctx->theme ? ctx->theme->font : WLX_FONT_DEFAULT;
                WLX_Theme dark_local  = gallery_theme_copy_with_font(wlx_theme_dark,  custom_font_local);
                WLX_Theme light_local = gallery_theme_copy_with_font(wlx_theme_light, custom_font_local);
                const struct {
                    const char *name;
                    const WLX_Theme *theme;
                } theme_rows[] = {
                    { "Dark",  &dark_local  },
                    { "Light", &light_local },
                };
                const struct {
                    WLX_Icon          icon;
                    Gallery_Icon_Role role;
                    const char       *label;
                } icon_cells[] = {
                    { WLX_ICON_CHECK,          GALLERY_ICON_ROLE_SUCCESS, "OK"     },
                    { WLX_ICON_INFO,           GALLERY_ICON_ROLE_ACCENT,  "Info"   },
                    { WLX_ICON_TRIANGLE_ALERT, GALLERY_ICON_ROLE_WARNING, "Warn"   },
                    { WLX_ICON_X,              GALLERY_ICON_ROLE_DANGER,  "Error"  },
                    { WLX_ICON_PALETTE,        GALLERY_ICON_ROLE_TEXT,    "Theme"  },
                    { WLX_ICON_SETTINGS,       GALLERY_ICON_ROLE_MUTED,   "Config" },
                };
                const int icon_count = (int)(sizeof(icon_cells) / sizeof(icon_cells[0]));
                const WLX_Theme *saved = ctx->theme;
                WLX_Texture atlas = gallery_icon_atlas_texture();

                wlx_push_id(ctx, 400);
                for (int row = 0; row < (int)(sizeof(theme_rows) / sizeof(theme_rows[0])); row++) {
                    wlx_push_id(ctx, (size_t)row);
                    ctx->theme = theme_rows[row].theme;
                    Gallery_Semantic_Theme row_sem = gallery_semantic_theme(theme_rows[row].theme);
                    wlx_layout_begin_s(ctx, WLX_VERT,
                        WLX_SIZES(WLX_SLOT_PX(20), WLX_SLOT_PX(54)),
                        .padding = 4, .gap = 2,
                        .back_color = row_sem.color_surface_1,
                        .border_color = row_sem.color_border,
                        .border_width = 1.0f);
                        wlx_label(ctx, theme_rows[row].name,
                            .height = 20, .font_size = SMALL_FS,
                            .front_color = row_sem.color_text_1,
                            .back_color = row_sem.color_surface_2,
                            .show_background = true);
                        wlx_layout_begin(ctx, icon_count, WLX_HORZ, .gap = 6, .padding = 2);
                            for (int i = 0; i < icon_count; i++) {
                                wlx_push_id(ctx, (size_t)i);
                                wlx_label(ctx, icon_cells[i].label,
                                    .height = 46, .font_size = TINY_FS, .align = WLX_BOTTOM_CENTER,
                                    .front_color = row_sem.color_text_2,
                                    .texture = atlas,
                                    .texture_src = gallery_icon_src_for(icon_cells[i].icon, 24.0f),
                                    .texture_tint = gallery_icon_tint(&row_sem, icon_cells[i].role),
                                    .image_placement = WLX_IMAGE_PLACEMENT_TOP,
                                    .image_size = 24,
                                    .image_text_gap = 2);
                                wlx_pop_id(ctx);
                            }
                        wlx_layout_end(ctx);
                    wlx_layout_end(ctx);
                    wlx_pop_id(ctx);
                }
                ctx->theme = saved;
                wlx_pop_id(ctx);
            }

        wlx_layout_end(ctx);

    SECTION_END(ctx);
}

// ---------------------------------------------------------------------------
// Section: Opacity
// ---------------------------------------------------------------------------
static void section_opacity(WLX_Context *ctx, Gallery_State *st) {
    SECTION_BEGIN(ctx);

        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


            wlx_slider(ctx, "Widget      ", &st->opacity_control,
                .height = OPT_H, .min_value = 0.0f, .max_value = 1.0f, .font_size = OPT_FS);
            wlx_slider(ctx, "Theme       ", &st->theme_opacity,
                .height = OPT_H, .min_value = 0.0f, .max_value = 1.0f, .font_size = OPT_FS);
            wlx_slider(ctx, "Stack       ", &st->stack_opacity,
                .height = OPT_H, .min_value = 0.0f, .max_value = 1.0f, .font_size = OPT_FS);

        wlx_panel_end(ctx);

    SECTION_NEXT(ctx);

            wlx_layout_begin_s(ctx, WLX_VERT,
                WLX_SIZES(
                    WLX_SLOT_CONTENT,  // heading
                    WLX_SLOT_CONTENT,  // description
                    WLX_SLOT_CONTENT,  // sub: fixed levels
                    WLX_SLOT_PX(100),  // fixed levels grid
                    WLX_SLOT_CONTENT,  // sub: interactive
                    WLX_SLOT_PX(40),   // interactive row
                    WLX_SLOT_CONTENT,  // sub: context stack
                    WLX_SLOT_PX(70),   // stack demo
                    WLX_SLOT_CONTENT,  // sub: combined
                    WLX_SLOT_PX(70)    // combined demo
                ));

                SECTION_HEADING(ctx, "Opacity");

                wlx_label(ctx, "Three-layer model: per-widget * theme * context stack.",
                .height = 26);
                SUB_HEADING(ctx, "Fixed Per-Widget Levels");
                wlx_layout_begin(ctx, 5, WLX_HORZ);
                    float levels[] = { 0.2f, 0.4f, 0.6f, 0.8f, 1.0f };
                    for (int i = 0; i < 5; i++) {
                        wlx_push_id(ctx, (size_t)(i + 100));
                        wlx_layout_begin(ctx, 4, WLX_VERT);
                            char buf[16];
                            snprintf(buf, sizeof(buf), "%.0f%%", levels[i] * 100);
                            wlx_label(ctx, buf,
                                .height = 20, .font_size = TINY_FS, .align = WLX_CENTER,
                                .opacity = levels[i]);
                            wlx_button(ctx, "Btn",
                                .height = SMALL_H, .font_size = TINY_FS, .align = WLX_CENTER,
                                .opacity = levels[i]);
                            {
                                static bool checks[5] = { true, true, true, true, true };
                                wlx_checkbox(ctx, "Chk", &checks[i],
                                    .height = SMALL_H, .font_size = TINY_FS,
                                    .opacity = levels[i]);
                            }
                            wlx_widget(ctx,
                                .widget_align = WLX_CENTER, .width = -1, .height = 20,
                                .color = (WLX_Color){ 0, 120, 255, (unsigned char)(levels[i] * 255) });
                        wlx_layout_end(ctx);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Interactive (per-widget slider)");
                wlx_layout_begin(ctx, 3, WLX_HORZ);
                    wlx_label(ctx, "Dynamic text", .height = 40, .font_size = 16,
                        .align = WLX_CENTER, .show_background = true,
                        .back_color = heading_color(ctx->theme, 42, 22, 62),
                        .opacity = st->opacity_control);
                    wlx_button(ctx, "Dynamic button", .height = 40, .font_size = 16,
                        .align = WLX_CENTER,
                        .opacity = st->opacity_control);
                    wlx_widget(ctx,
                        .widget_align = WLX_CENTER, .width = -1, .height = 40,
                        .color = (WLX_Color){ 200, 80, 60, (unsigned char)(st->opacity_control * 255) });
                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "Context Stack (push_opacity)");
                wlx_layout_begin(ctx, 3, WLX_HORZ);

                    // Single push
                    wlx_push_id(ctx, 200);
                    wlx_push_opacity(ctx, st->stack_opacity);
                    wlx_layout_begin(ctx, 2, WLX_VERT);
                        wlx_label(ctx, "Single push", .height = 20, .font_size = TINY_FS,
                            .align = WLX_CENTER);
                        wlx_button(ctx, "Faded", .height = DESC_H, .font_size = TINY_FS,
                            .align = WLX_CENTER);
                    wlx_layout_end(ctx);
                    wlx_pop_opacity(ctx);
                    wlx_pop_id(ctx);

                    // Nested push
                    wlx_push_id(ctx, 201);
                    wlx_push_opacity(ctx, st->stack_opacity);
                    wlx_push_opacity(ctx, 0.5f);
                    wlx_layout_begin(ctx, 2, WLX_VERT);
                        wlx_label(ctx, "Nested (slider * 0.5)", .height = 20, .font_size = TINY_FS,
                            .align = WLX_CENTER);
                        wlx_button(ctx, "Double-faded", .height = DESC_H, .font_size = TINY_FS,
                            .align = WLX_CENTER);
                    wlx_layout_end(ctx);
                    wlx_pop_opacity(ctx);
                    wlx_pop_opacity(ctx);
                    wlx_pop_id(ctx);

                    // Override inside stack
                    wlx_push_id(ctx, 202);
                    wlx_push_opacity(ctx, st->stack_opacity);
                    wlx_layout_begin(ctx, 2, WLX_VERT);
                        wlx_button(ctx, ".opacity=1 override", .height = DESC_H, .font_size = TINY_FS,
                            .align = WLX_CENTER, .opacity = 1.0f);
                        wlx_label(ctx, "Stays opaque", .height = 20, .font_size = TINY_FS,
                            .align = WLX_CENTER);
                    wlx_layout_end(ctx);
                    wlx_pop_opacity(ctx);
                    wlx_pop_id(ctx);

                wlx_layout_end(ctx);

                SUB_HEADING(ctx, "All Three Combined");
                {
                    // Temporarily set theme opacity for this subsection
                    float saved_theme_opacity = ctx->theme->opacity;
                    ((WLX_Theme *)ctx->theme)->opacity = st->theme_opacity;

                    wlx_push_opacity(ctx, st->stack_opacity);
                    wlx_layout_begin(ctx, 4, WLX_HORZ);
                        wlx_push_id(ctx, 300);
                        wlx_label(ctx, "w*t*s", .height = 50, .font_size = SMALL_FS,
                            .align = WLX_CENTER, .show_background = true,
                            .back_color = heading_color(ctx->theme, 42, 22, 62),
                            .opacity = st->opacity_control);
                        wlx_button(ctx, "Combined", .height = 50, .font_size = SMALL_FS,
                            .align = WLX_CENTER,
                            .opacity = st->opacity_control);
                        {
                            static bool cb = true;
                            wlx_checkbox(ctx, "All 3", &cb, .height = 50, .font_size = SMALL_FS,
                                .opacity = st->opacity_control, .padding_left = 4);
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
    Gallery_Semantic_Theme semantic = gallery_semantic_theme(ctx->theme);

    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


            wlx_slider(ctx, "Panel Count", &st->dynamic_panel_count,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 1.0f, .max_value = 10.0f);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

        // ======== RIGHT: Content ========
            wlx_layout_begin_auto(ctx, WLX_VERT, 0);

                wlx_layout_auto_slot_px(ctx, 48);
                SECTION_HEADING(ctx, "ID Stack & Dynamic Widgets");

                wlx_layout_auto_slot_px(ctx, 30);
                wlx_label(ctx, "Loop-generated panels with wlx_push_id / wlx_pop_id for stable IDs.",
                    .height = DESC_H);

                for (int i = 0; i < count; i++) {
                    wlx_push_id(ctx, (size_t)i);

                    WLX_Color section_bg = (i % 2 == 0)
                        ? semantic.color_surface_2
                        : semantic.color_surface_3;

                    char header[32];
                    snprintf(header, sizeof(header), "Panel %d", i + 1);

                    wlx_layout_auto_slot_px(ctx, 36);
                    wlx_label(ctx, header,
                        .height = SUB_HEADING_H, .font_size = 20, .align = WLX_LEFT,
                        .back_color = section_bg, .show_background = true);

                    wlx_layout_auto_slot_px(ctx, ROW_H);
                    wlx_inputbox(ctx, "Name:  ", st->dynamic_names[i], sizeof(st->dynamic_names[i]),
                        .height = ROW_H);
                    wlx_layout_auto_slot_px(ctx, ROW_H);
                    wlx_slider(ctx, "Value    ", &st->dynamic_sliders[i],
                        .height = ROW_H);
                    wlx_layout_auto_slot_px(ctx, ROW_H);
                    wlx_checkbox(ctx, "Enabled", &st->dynamic_toggles[i],
                        .height = ROW_H);

                    // Divider
                    wlx_layout_auto_slot_px(ctx, 2);
                    wlx_widget(ctx,
                        .widget_align = WLX_CENTER, .width = -1, .height = 2,
                        .color = semantic.color_border_strong);

                    wlx_pop_id(ctx);
                }

            wlx_layout_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Borders
// ---------------------------------------------------------------------------
static void section_borders(WLX_Context *ctx, Gallery_State *st) {
    WLX_Color bdr = to_color(st->border_color.r, st->border_color.g, st->border_color.b, 1.0f);
    // float bw = st->border_width;
    static float bw = 0.35f;

    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


                wlx_slider(ctx, "Width  ", &bw,
                    .height = OPT_H, .min_value = 0.0f, .max_value = 8.0f, .font_size = OPT_FS);

                wlx_label(ctx, "Border Color", .font_size = OPT_FS, .height = SMALL_H);
                color_sliders(ctx, "", &st->border_color, OPT_H, OPT_FS);

                // Color preview swatch
                wlx_label(ctx, "Preview", .font_size = OPT_FS, .height = SMALL_H);
                wlx_widget(ctx, .height = OPT_H,
                    .color = bdr, .border_color = (WLX_Color){255,255,255,80}, .border_width = 1);

                // Demo slider/checkbox state (secondary controls)
                wlx_slider(ctx, "Slider ", &st->border_slider,
                    .height = OPT_H, .font_size = OPT_FS);
                wlx_checkbox(ctx, "Checkbox", &st->border_checkbox,
                    .font_size = OPT_FS, .height = OPT_H);

            wlx_panel_end(ctx);

    wlx_split_next(ctx);

        // ======== RIGHT: Demo widgets ========
        wlx_panel_begin(ctx, .title = "Borders",
            PANEL_TITLE_DEFAULTS,
            .title_back_color = SECTION_BG(ctx));

                wlx_label(ctx, "Adjust controls on the left - see borders update live on the right.",
                    .height = DESC_H);

                // -- Labels --
                SUB_HEADING(ctx, "Labels");
                wlx_layout_begin(ctx, 3, WLX_HORZ);
                    wlx_label(ctx, "Dynamic",
                        .font_size = 16, .height = 40, .show_background = true,
                        .border_color = bdr, .border_width = bw);
                    wlx_label(ctx, "Red, width 3",
                        .font_size = 16, .height = 40, .show_background = true,
                        .border_color = (WLX_Color){255, 60, 60, 255}, .border_width = 3);
                    wlx_label(ctx, "No border",
                        .font_size = 16, .height = 40, .show_background = true,
                        .border_width = 0);
                wlx_layout_end(ctx);

                // -- Buttons --
                SUB_HEADING(ctx, "Buttons");
                wlx_layout_begin(ctx, 3, WLX_HORZ);
                    wlx_button(ctx, "Dynamic",
                        .font_size = 16, .height = ROW_H, .align = WLX_CENTER,
                        .border_color = bdr, .border_width = bw);
                    wlx_button(ctx, "Green outline",
                        .font_size = 16, .height = ROW_H, .align = WLX_CENTER,
                        .border_color = (WLX_Color){0, 200, 0, 255}, .border_width = 2);
                    wlx_button(ctx, "Yellow thick",
                        .font_size = 16, .height = ROW_H, .align = WLX_CENTER,
                        .border_color = (WLX_Color){255, 220, 0, 255}, .border_width = 4);
                wlx_layout_end(ctx);

                // -- Widgets --
                SUB_HEADING(ctx, "Widgets");
                wlx_layout_begin(ctx, 4, WLX_HORZ);
                    wlx_widget(ctx, .height = 50,
                        .color = (WLX_Color){60, 20, 20, 255},
                        .border_color = (WLX_Color){255, 100, 100, 255}, .border_width = bw);
                    wlx_widget(ctx, .height = 50,
                        .color = (WLX_Color){20, 60, 20, 255},
                        .border_color = (WLX_Color){100, 255, 100, 255}, .border_width = bw);
                    wlx_widget(ctx, .height = 50,
                        .color = (WLX_Color){20, 20, 60, 255},
                        .border_color = (WLX_Color){100, 100, 255, 255}, .border_width = bw);
                    wlx_widget(ctx, .height = 50,
                        .color = (WLX_Color){50, 50, 50, 255},
                        .border_color = bdr, .border_width = bw);
                wlx_layout_end(ctx);

                // -- Checkboxes & Inputs --
                SUB_HEADING(ctx, "Checkboxes & Inputs");
                wlx_checkbox(ctx, "Default border (from theme)", &st->border_checkbox,
                    .font_size = DEMO_FS, .height = ROW_H);
                wlx_checkbox(ctx, "Custom border", &st->border_checkbox,
                    .font_size = DEMO_FS, .height = ROW_H,
                    .border_color = bdr, .border_width = bw);
                wlx_inputbox(ctx, "Default:", st->border_input, sizeof(st->border_input),
                    .height = ROW_H);
                wlx_inputbox(ctx, "Custom: ", st->border_input, sizeof(st->border_input),
                    .height = ROW_H,
                    .border_width = bw, .border_color = bdr);

                // -- Sliders --
                SUB_HEADING(ctx, "Sliders");
                wlx_slider(ctx, "Default    ", &st->border_slider,
                    .height = ROW_H);
                wlx_slider(ctx, "Bordered   ", &st->border_slider,
                    .height = ROW_H,
                    .border_color = bdr, .border_width = bw);

            wlx_panel_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Auto Layout (mixed-size dynamic layouts)
// ---------------------------------------------------------------------------
static void section_auto_layout(WLX_Context *ctx, Gallery_State *st) {
    SECTION_BEGIN(ctx);

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


                wlx_slider(ctx, "Items      ", &st->auto_item_count,
                    .height = OPT_H, .font_size = OPT_FS, .min_value = 1.0f, .max_value = 10.0f);
                wlx_slider(ctx, "Item H     ", &st->auto_item_height,
                    .height = OPT_H, .font_size = OPT_FS, .min_value = 20.0f, .max_value = 80.0f);
                wlx_slider(ctx, "Header %   ", &st->auto_header_pct,
                    .height = OPT_H, .font_size = OPT_FS, .min_value = 5.0f, .max_value = 40.0f);
                wlx_slider(ctx, "Sidebar %  ", &st->auto_sidebar_pct,
                    .height = OPT_H, .font_size = OPT_FS, .min_value = 10.0f, .max_value = 60.0f);

            wlx_panel_end(ctx);

    SECTION_NEXT(ctx);

        // ======== RIGHT: Content ========
        {
                // Compute demo 1 height dynamically based on item count & height
                int n1 = (int)st->auto_item_count;
                if (n1 < 1) n1 = 1;
                if (n1 > 10) n1 = 10;
                float demo1_h = n1 * st->auto_item_height + 30;

            wlx_layout_begin_s(ctx, WLX_VERT,
                WLX_SIZES(
                    WLX_SLOT_CONTENT,        // heading
                    WLX_SLOT_CONTENT,        // description
                    WLX_SLOT_CONTENT,        // sub: PX + FLEX footer
                    WLX_SLOT_PX(demo1_h),    // demo 1 (dynamic)
                    WLX_SLOT_CONTENT,        // sub: PCT columns
                    WLX_SLOT_PX(60),         // demo 2
                    WLX_SLOT_CONTENT,        // sub: mixed sidebar
                    WLX_SLOT_PX(200),        // demo 3
                    WLX_SLOT_CONTENT,        // sub: min/max
                    WLX_SLOT_PX(60),         // demo 4
                    WLX_SLOT_CONTENT,        // sub: FILL
                    WLX_SLOT_PX(80)          // demo 5
                ));

                SECTION_HEADING(ctx, "Auto Layout (Mixed Sizes)");

                wlx_label(ctx, "Dynamic layouts with wlx_layout_auto_slot() - PX, PCT, FLEX, FILL:",
                    .font_size = SMALL_FS, .height = SMALL_H);

                // -- Demo 1: N x PX items + FLEX footer --
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
                                .height = -1, .font_size = SMALL_FS, .align = WLX_CENTER,
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

                // -- Demo 2: PCT column widths --
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

                // -- Demo 3: Mixed sidebar layout --
                // Note: auto layouts can't nest wlx_layout_begin inside them
                // (scratch buffer contiguity). Use a static layout for the
                // outer split and auto layouts for variable-count sections.
                SUB_HEADING(ctx, "Mixed: PCT Sidebar + FLEX Content");
                {
                    float spct = st->auto_sidebar_pct;
                    char side_lbl[48], content_lbl[48];
                    snprintf(side_lbl, sizeof(side_lbl), "PCT(%.0f) Sidebar", spct);
                    snprintf(content_lbl, sizeof(content_lbl), "FLEX(1) Content");

                    wlx_layout_begin_auto(ctx, WLX_HORZ, 0);

                        wlx_layout_auto_slot(ctx, WLX_SLOT_PCT(spct));
                        wlx_label(ctx, side_lbl,
                            .height = 200, .font_size = SMALL_FS, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 42, 22, 22));

                        wlx_layout_auto_slot(ctx, WLX_SLOT_FLEX(1));
                        wlx_label(ctx, content_lbl,
                            .height = 200, .font_size = SMALL_FS, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 22, 42, 32));

                    wlx_layout_end(ctx);
                }

                // -- Demo 4: FLEX with min/max --
                SUB_HEADING(ctx, "FLEX with Min/Max Constraints");
                {
                    wlx_layout_begin_auto(ctx, WLX_HORZ, 0);

                        wlx_layout_auto_slot(ctx, WLX_SLOT_FLEX_MINMAX(1, 80, 250));
                        wlx_label(ctx, "FLEX_MINMAX(1, 80, 250)",
                            .height = 60, .font_size = TINY_FS, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 52, 32, 52));

                        wlx_layout_auto_slot(ctx, WLX_SLOT_PX(120));
                        wlx_label(ctx, "PX(120)",
                            .height = 60, .font_size = TINY_FS, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 32, 52, 42));

                    wlx_layout_end(ctx);
                }

                // -- Demo 5: PCT header with adaptive height --
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
                            .height = -1, .font_size = SMALL_FS, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 42, 32, 22));
                        wlx_layout_auto_slot(ctx, WLX_SLOT_PCT(rest));
                        wlx_label(ctx, lbl_b,
                            .height = -1, .font_size = SMALL_FS, .align = WLX_CENTER,
                            .show_background = true,
                            .back_color = heading_color(ctx->theme, 22, 32, 42));
                    wlx_layout_end(ctx);
                }

            wlx_layout_end(ctx);
            } // close dynamic height scope

    SECTION_END(ctx);
}

// ---------------------------------------------------------------------------
// Section: Progress / Toggle / Radio
// ---------------------------------------------------------------------------
static void section_progress_toggle_radio(WLX_Context *ctx, Gallery_State *st) {
    Gallery_Semantic_Theme semantic = gallery_semantic_theme(ctx->theme);

    SECTION_BEGIN(ctx);

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_SLIDERS_HORIZONTAL, GALLERY_ICON_ROLE_TEXT,
                "Options", HEADING_H);


            wlx_slider(ctx, "Progress  ", &st->progress_value,
                .height = OPT_H, .font_size = OPT_FS);

        wlx_panel_end(ctx);

    SECTION_NEXT(ctx);

        // ======== RIGHT: Content ========
        wlx_panel_begin(ctx, .title = "Progress / Toggle / Radio",
            PANEL_TITLE_DEFAULTS,
            .title_back_color = SECTION_BG(ctx));

    wlx_label(ctx, "widgets: progress bar, toggle switch, radio button.",
        .height = DESC_H,
        .front_color = semantic.color_text_2);

    // -- Progress --
    SUB_HEADING(ctx, "Progress Bar");
    wlx_progress(ctx, st->progress_value, .height = ROW_H);

    {
        const char *status_label = "Blocked";
        WLX_Color status_color = semantic.color_danger;

        if (st->progress_value >= 0.85f) {
            status_label = "Ready";
            status_color = semantic.color_success;
        } else if (st->progress_value >= 0.40f) {
            status_label = "Review";
            status_color = semantic.color_warning;
        }

        char buf[64];
        snprintf(buf, sizeof(buf), "Value: %.0f%%", st->progress_value * 100.0f);
        wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 0, .gap = 4);
            wlx_label(ctx, buf,
                .height = SMALL_H, .font_size = SMALL_FS,
                .front_color = semantic.color_text_2);
            wlx_label(ctx, status_label,
                .height = SMALL_H, .font_size = SMALL_FS, .align = WLX_CENTER,
                .back_color = status_color,
                .front_color = gallery_on_color(status_color));
        wlx_layout_end(ctx);
    }

    wlx_progress(ctx, 0.0f, .height = SMALL_H);
    wlx_progress(ctx, 1.0f, .height = SMALL_H);

    // -- Toggle --
    SUB_HEADING(ctx, "Toggle Switch");
    wlx_toggle(ctx, "Enable notifications", &st->toggle_a, .height = ROW_H);
    wlx_toggle(ctx, "Dark mode", &st->toggle_b, .height = ROW_H);

    // -- Radio --
    SUB_HEADING(ctx, "Radio Buttons");
    wlx_radio(ctx, "Small",  &st->radio_choice, 0, .height = ROW_H);
    wlx_radio(ctx, "Medium", &st->radio_choice, 1, .height = ROW_H);
    wlx_radio(ctx, "Large",  &st->radio_choice, 2, .height = ROW_H);

    {
        char buf[64];
        snprintf(buf, sizeof(buf), "Selected: %d", st->radio_choice);
        wlx_label(ctx, buf,
            .height = SMALL_H, .font_size = SMALL_FS, .align = WLX_CENTER,
            .back_color = semantic.color_selection,
            .front_color = gallery_on_color(semantic.color_selection));
    }

        wlx_panel_end(ctx);

    SECTION_END(ctx);
}

// ---------------------------------------------------------------------------
// Section: Overview (proves the section template)
// ---------------------------------------------------------------------------
static void section_overview(WLX_Context *ctx, Gallery_State *st) {
    Gallery_Semantic_Theme semantic = gallery_semantic_theme(ctx->theme);

    SECTION_BEGIN(ctx);

        // ======== LEFT: Welcome panel ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_INFO, GALLERY_ICON_ROLE_ACCENT,
                "Welcome", HEADING_H);

            wlx_label(ctx, "What is Wollix?",
                .font_size = SECTION_FS, .height = HEADING_H, .align = WLX_LEFT,
                .front_color = semantic.color_text_1);

            wlx_label(ctx,
                "Wollix is a header-only, immediate-mode GUI library for C11.\n"
                "It is compact, mechanical, and clear -- designed for tools\n"
                "and overlays where every pixel counts.",
                .font_size = MEDIUM_FS, .height = 100, .align = WLX_LEFT,
                .front_color = semantic.color_text_2);

            wlx_label(ctx, "Current Stats",
                .font_size = SECTION_FS, .height = HEADING_H, .align = WLX_LEFT,
                .front_color = semantic.color_text_1);

            char stats[256];
            snprintf(stats, sizeof(stats),
                "Backend: %s\n"
                "Version: %s\n"
                "FPS:     %d\n"
                "Theme:   %s",
                GALLERY_BACKEND, GALLERY_VERSION,
                gallery_platform_fps(),
                theme_names[st->theme_mode]);
            wlx_label(ctx, stats,
                .font_size = DEMO_FS, .height = 130, .align = WLX_LEFT,
                .front_color = semantic.color_text_2);

        wlx_panel_end(ctx);

    SECTION_NEXT(ctx);

        // ======== RIGHT: Demo & quick nav ========
        wlx_panel_begin(ctx, .padding = 0);

            gallery_panel_heading(ctx,
                WLX_ICON_PLAY, GALLERY_ICON_ROLE_ACCENT,
                "Demo & Links", HEADING_H);

            // Brand trait demonstration
            wlx_layout_begin_s(ctx, WLX_HORZ,
                WLX_SIZES(WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1)),
                .gap = 1,.padding_bottom = 0);
                wlx_label(ctx, "Compact",
                    .height = 50, .font_size = DEMO_FS, .align = WLX_CENTER,
                    .show_background = true,
                    .back_color = semantic.color_surface_2);
                wlx_label(ctx, "Mechanical",
                    .height = 50, .font_size = DEMO_FS, .align = WLX_CENTER,
                    .show_background = true,
                    .back_color = semantic.color_surface_2);
                wlx_label(ctx, "Clear",
                    .height = 50, .font_size = DEMO_FS, .align = WLX_CENTER,
                    .show_background = true,
                    .back_color = semantic.color_surface_2);
            wlx_layout_end(ctx);

            // Navigation buttons to groups that have sections
            for (int gi = 1; gi < GROUP_COUNT; gi++) {
                if (groups[gi].section_count == 0) continue;
                wlx_push_id(ctx, (size_t)gi);
                char btn_label[64];
                snprintf(btn_label, sizeof(btn_label), "Explore %s", groups[gi].name);
                if (gallery_icon_button(ctx, btn_label,
                    WLX_ICON_CHEVRON_RIGHT, GALLERY_ICON_ROLE_ACCENT,
                    .height = OPT_H, .align = WLX_LEFT,
                    .font_size = MEDIUM_FS,
                    .back_color = semantic.color_surface_3,
                    .front_color = semantic.color_text_1,
                    .border_color = semantic.color_border,
                    .border_width = 0.5f,
                    .image_size = 16,
                    .image_text_gap = 8)) {
                    gallery_select_group(st, gi);
                }
                wlx_pop_id(ctx);
            }

        wlx_panel_end(ctx);

    SECTION_END(ctx);
}

// ---------------------------------------------------------------------------
// Shared per-frame renderer -- backend-agnostic.
// ---------------------------------------------------------------------------
static void gallery_render_frame(WLX_Context *ctx, Gallery_State *gs) {
    Gallery_Semantic_Theme semantic = gallery_semantic_theme(ctx->theme);

    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(44), WLX_SLOT_FLEX(1), WLX_SLOT_PX(SMALL_H)),
        .back_color = semantic.color_app_bg);

        // -- Title bar (branded) --
        wlx_layout_begin_s(ctx, WLX_HORZ,
                    WLX_SIZES(WLX_SLOT_FLEX(1), WLX_SLOT_PX(82), WLX_SLOT_PX(96)),
                    .padding = 0, .gap = 0,
                    .back_color = semantic.color_surface_2,
                    .border_color = semantic.color_border,
                    .border_width = 0.5f);
                    gallery_icon_label(ctx, "Wollix",
                        WLX_ICON_APP_WINDOW, GALLERY_ICON_ROLE_ACCENT,
                        .font_size = 24, .align = WLX_LEFT,
                        .height = 44,
                        .front_color = semantic.color_accent,
                        .image_size = 24,
                        .image_text_gap = 10);

                    {
                        char fps_label[32];
                        snprintf(fps_label, sizeof(fps_label), "FPS: %d  ", gallery_platform_fps());
                        wlx_label(ctx, fps_label,
                            .font_size = TINY_FS, .height = 44, .align = WLX_RIGHT,
                            .front_color = semantic.color_text_2);
                    }

                    {
                        char mode_label[32];
                        snprintf(mode_label, sizeof(mode_label), "%s", theme_names[gs->theme_mode]);
                        if (gallery_icon_button(ctx, mode_label,
                            WLX_ICON_PALETTE, GALLERY_ICON_ROLE_TEXT,
                            .height = 44, .align = WLX_CENTER,
                            .font_size = TINY_FS,
                            .back_color = semantic.color_surface_3,
                            .front_color = semantic.color_text_1,
                            .border_color = semantic.color_border_strong,
                            .border_width = 0.5f,
                            .image_size = 16,
                            .image_text_gap = 6)) {
                            gs->theme_mode = (gs->theme_mode + 1) % GALLERY_THEME_COUNT;
                        }
                    }
        wlx_layout_end(ctx);

        // -- Body: grouped sidebar + content --
        wlx_layout_begin_s(ctx, WLX_HORZ,
            WLX_SIZES(WLX_SLOT_PX(SIDEBAR_W), WLX_SLOT_FLEX(1)),
            .gap = 4);

            // -- Grouped sidebar --
            wlx_scroll_panel_begin(ctx, -1,
                .back_color = semantic.color_surface_2,
                .scrollbar_color = semantic.color_border_strong);
                wlx_panel_begin(ctx,
                    .padding = 0,
                    .back_color = semantic.color_surface_2,
                    .border_color = semantic.color_border,
                    .border_width = 0.5f,
                    .title_back_color = semantic.color_surface_3);
                    for (int gi = 0; gi < GROUP_COUNT; gi++) {
                        wlx_push_id(ctx, (size_t)(100 + gi));
                        const Group *group = &groups[gi];
                        bool group_selected = gi == gs->active_group;

                        // Group label button
                        WLX_Color group_bg = group_selected
                            ? semantic.color_selection
                            : semantic.color_surface_2;
                        WLX_Color group_fg = group_selected
                            ? gallery_on_color(group_bg)
                            : semantic.color_text_1;
                        if (gallery_icon_button(ctx, group->name,
                            group->icon, GALLERY_ICON_ROLE_TEXT,
                            .height = SUB_HEADING_H,
                            .align = WLX_LEFT,
                            .back_color = group_bg,
                            .front_color = group_fg,
                            .texture_tint = group_fg,
                            .border_color = group_selected ? semantic.color_focus : semantic.color_border,
                            .border_width = group_selected ? 1.0f : 0.5f,
                            .font_size = SECTION_FS,
                            .content_padding_left = SIDEBAR_GROUP_CONTENT_PAD_L,
                            .content_padding_right = SIDEBAR_CONTENT_PAD_R,
                            .image_size = 18,
                            .image_text_gap = 8)) {
                            gallery_select_group(gs, gi);
                        }

                        // Section buttons under this group
                        for (int si = 0; si < group->section_count; si++) {
                            wlx_push_id(ctx, (size_t)(200 + si));
                            const Section *section = group->sections[si];
                            bool section_selected = si == gs->active_section_in_group && group_selected;
                            WLX_Color sec_bg = section_selected
                                ? semantic.color_selection
                                : semantic.color_surface_3;
                            WLX_Color sec_fg = section_selected
                                ? gallery_on_color(sec_bg)
                                : semantic.color_text_2;
                            if (gallery_icon_button(ctx, section->name,
                                section->icon, GALLERY_ICON_ROLE_MUTED,
                                .height = OPT_H,
                                .align = WLX_LEFT,
                                .back_color = sec_bg,
                                .front_color = sec_fg,
                                .texture_tint = sec_fg,
                                .border_color = section_selected ? semantic.color_focus : semantic.color_border,
                                .border_width = section_selected ? 1.0f : 0.5f,
                                .font_size = OPT_FS,
                                .content_padding_left = SIDEBAR_SECTION_CONTENT_PAD_L,
                                .content_padding_right = SIDEBAR_CONTENT_PAD_R,
                                .image_size = 16,
                                .image_text_gap = 6)) {
                                gallery_select_section(gs, gi, si);
                            }
                            wlx_pop_id(ctx);
                        }
                        wlx_pop_id(ctx);
                    }
                wlx_panel_end(ctx);
            wlx_scroll_panel_end(ctx);

            // -- Content area --
            if (gs->active_group == GROUP_OVERVIEW) {
                section_overview(ctx, gs);
            } else {
                const Section *active_section = gallery_active_section(gs);
                if (active_section && active_section->render) {
                    active_section->render(ctx, gs);
                }
            }

        wlx_layout_end(ctx);

        // -- Status bar --
        {
            char status[256];
            const Section *active_section = gallery_active_section(gs);
            const char *section_name = active_section ? active_section->name : groups[gs->active_group].name;
            snprintf(status, sizeof(status),
                "%s  |  v%s  |  FPS: %d  |  %s / %s  |  Theme: %s",
                GALLERY_BACKEND, GALLERY_VERSION,
                gallery_platform_fps(),
                groups[gs->active_group].name,
                section_name,
                theme_names[gs->theme_mode]);
            gallery_icon_label(ctx, status,
                WLX_ICON_INFO, GALLERY_ICON_ROLE_MUTED,
                .font_size = TINY_FS, .height = SMALL_H, .align = WLX_LEFT,
                .back_color = semantic.color_surface_2,
                .front_color = semantic.color_text_muted,
                .border_color = semantic.color_border,
                .border_width = 0.5f,
                .image_size = 14,
                .image_text_gap = 8);
        }

    wlx_layout_end(ctx);
}

// ===========================================================================
// Platform block -- state, platform contract implementations, and entry point.
// ===========================================================================
#if defined(WLX_GALLERY_RAYLIB)

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 800
#define TARGET_FPS    60
#define GALLERY_RAYLIB_FONT_SCALE 1.15f

static Font         g_raylib_font;
static bool         g_raylib_font_ok;
static WLX_Font     g_raylib_font_handle = WLX_FONT_DEFAULT;
static WLX_Theme    g_raylib_theme_dark;
static WLX_Theme    g_raylib_theme_light;
static WLX_Theme    g_raylib_theme_glass;
static WLX_Theme    g_raylib_theme_brand;
static const WLX_Theme *g_raylib_themes[GALLERY_THEME_COUNT];
static WLX_Context *g_raylib_ctx = NULL;
static bool         g_raylib_icon_atlas_ready;
static WLX_Texture  g_raylib_icon_atlas;

static WLX_Text_Style gallery_raylib_scaled_text_style(WLX_Text_Style style) {
    if (style.font_size > 0) {
        int scaled = (int)((float)style.font_size * GALLERY_RAYLIB_FONT_SCALE + 0.5f);
        if (scaled < 1) scaled = 1;
        style.font_size = scaled;
    }
    return style;
}

static void gallery_raylib_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    wlx_raylib_draw_text(text, x, y, gallery_raylib_scaled_text_style(style));
}

static void gallery_raylib_measure_text(const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
    wlx_raylib_measure_text(text, gallery_raylib_scaled_text_style(style), out_w, out_h);
}

static void gallery_raylib_measure_text_slice(const char *text, size_t slice_len,
    WLX_Text_Style style, float *out_w, float *out_h) {
    wlx_raylib_measure_text_slice(text, slice_len, gallery_raylib_scaled_text_style(style), out_w, out_h);
}

static void gallery_raylib_install_text_scale(WLX_Context *ctx) {
    ctx->backend.draw_text = gallery_raylib_draw_text;
    ctx->backend.measure_text = gallery_raylib_measure_text;
    ctx->backend.measure_text_slice = gallery_raylib_measure_text_slice;
}

static void gallery_icon_atlas_create(WLX_Context *ctx) {
    (void)ctx;
    Image img = (Image){
        .data    = (void *)wlx_icons_rgba,
        .width   = WLX_ICONS_WIDTH,
        .height  = WLX_ICONS_HEIGHT,
        .mipmaps = 1,
        .format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
    g_raylib_icon_atlas = LoadTextureFromImage(img);
    g_raylib_icon_atlas_ready = g_raylib_icon_atlas.width > 0;
    if (g_raylib_icon_atlas_ready) {
        SetTextureFilter(g_raylib_icon_atlas, TEXTURE_FILTER_BILINEAR);
    } else {
        g_raylib_icon_atlas = (WLX_Texture){0};
        printf("WARNING: could not create icon atlas texture\n");
    }
}

static void gallery_icon_atlas_destroy(void) {
    if (g_raylib_icon_atlas_ready) {
        UnloadTexture(g_raylib_icon_atlas);
        g_raylib_icon_atlas = (WLX_Texture){0};
        g_raylib_icon_atlas_ready = false;
    }
}

static WLX_Texture gallery_icon_atlas_texture(void) {
    return g_raylib_icon_atlas_ready ? g_raylib_icon_atlas : (WLX_Texture){0};
}

static bool gallery_icon_atlas_ready(void) {
    return g_raylib_icon_atlas_ready;
}

static bool gallery_platform_init(Gallery_State *gs) {
    printf("Wollix Widget Gallery\n");
    SetConfigFlags((gallery_perf_enabled() ? 0 : FLAG_WINDOW_RESIZABLE) | FLAG_MSAA_4X_HINT);
    InitWindow(gallery_perf_window_width(WINDOW_WIDTH), gallery_perf_window_height(WINDOW_HEIGHT), "Wollix Widget Gallery");
    SetTargetFPS(TARGET_FPS);

    g_raylib_font = LoadFontEx(GALLERY_FONT_PATH, 32, NULL, 0);
    g_raylib_font_ok = g_raylib_font.glyphCount > 0;
    if (g_raylib_font_ok) SetTextureFilter(g_raylib_font.texture, TEXTURE_FILTER_BILINEAR);
    else                  printf("WARNING: could not load %s\n", GALLERY_FONT_PATH);
    g_raylib_font_handle = g_raylib_font_ok ? wlx_font_from_raylib(&g_raylib_font) : WLX_FONT_DEFAULT;

    WLX_Font body_font = gallery_platform_font(GALLERY_FONT_BODY);
    g_raylib_theme_dark = gallery_theme_copy_with_font(wlx_theme_dark, body_font);
    g_raylib_theme_light = gallery_theme_copy_with_font(wlx_theme_light, body_font);
    g_raylib_theme_glass = gallery_theme_copy_with_font(wlx_theme_glass, body_font);
    g_raylib_theme_brand = gallery_brand_theme(body_font);
    gallery_update_custom_theme(gs, body_font);
    g_raylib_themes[GALLERY_THEME_DARK] = &g_raylib_theme_dark;
    g_raylib_themes[GALLERY_THEME_LIGHT] = &g_raylib_theme_light;
    g_raylib_themes[GALLERY_THEME_GLASS] = &g_raylib_theme_glass;
    g_raylib_themes[GALLERY_THEME_BRAND] = &g_raylib_theme_brand;
    g_raylib_themes[GALLERY_THEME_CUSTOM] = &gs->custom_theme;

    g_raylib_ctx = malloc(sizeof(*g_raylib_ctx));
    if (!g_raylib_ctx) return false;
    memset(g_raylib_ctx, 0, sizeof(*g_raylib_ctx));
    wlx_context_init_raylib(g_raylib_ctx);
    gallery_raylib_install_text_scale(g_raylib_ctx);

    gallery_icon_atlas_create(g_raylib_ctx);

    for (int i = 0; i < 10; i++) {
        snprintf(gs->dynamic_names[i], sizeof(gs->dynamic_names[i]), "Panel %d", i + 1);
    }
    return true;
}

static void gallery_platform_shutdown(Gallery_State *gs) {
    (void)gs;
    gallery_icon_atlas_destroy();
    if (g_raylib_font_ok) UnloadFont(g_raylib_font);
    if (g_raylib_ctx) {
        wlx_context_destroy(g_raylib_ctx);
        free(g_raylib_ctx);
        g_raylib_ctx = NULL;
    }
    CloseWindow();
}

static bool gallery_platform_begin_frame(Gallery_State *gs, WLX_Rect *out_root) {
    if (WindowShouldClose()) return false;
    float w = (float)GetRenderWidth();
    float h = (float)GetRenderHeight();
    *out_root = (WLX_Rect){ 0, 0, w, h };
    if (gs->theme_mode < 0 || gs->theme_mode >= GALLERY_THEME_COUNT) gs->theme_mode = GALLERY_THEME_DARK;
    WLX_Font body_font = gallery_platform_font(GALLERY_FONT_BODY);
    g_raylib_theme_brand = gallery_brand_theme(body_font);
    gallery_update_custom_theme(gs, body_font);
    g_raylib_ctx->theme = g_raylib_themes[gs->theme_mode];
    gallery_perf_before_frame(g_raylib_ctx, *out_root);
    wlx_begin(g_raylib_ctx, *out_root, wlx_process_raylib_input);
    BeginDrawing();
    return true;
}

static void gallery_platform_end_frame(Gallery_State *gs) {
    wlx_end(g_raylib_ctx);
    gallery_perf_backend_present_begin();
    EndDrawing();
    gallery_perf_backend_present_end();
    gallery_perf_backend_end_frame(g_raylib_ctx);
    gallery_perf_after_frame(g_raylib_ctx, gs);
}

static void gallery_platform_clear(WLX_Color bg) {
    ClearBackground(bg);
}

static int gallery_platform_fps(void) {
    return GetFPS();
}

static WLX_Font gallery_platform_font(Gallery_Font_Slot slot) {
    (void)slot;
    return g_raylib_font_handle;
}

int main(int argc, char **argv) {
    if (!gallery_perf_parse_args(argc, argv, &g)) return 1;
    if (!gallery_platform_init(&g)) {
        gallery_perf_shutdown();
        return 1;
    }
    WLX_Rect root;
    while (gallery_platform_begin_frame(&g, &root)) {
        gallery_platform_clear(g_raylib_ctx->theme->background);
        gallery_render_frame(g_raylib_ctx, &g);
        gallery_platform_end_frame(&g);
        if (gallery_perf_finished()) break;
    }
    gallery_platform_shutdown(&g);
    gallery_perf_shutdown();
    return 0;
}


#endif // WLX_GALLERY_RAYLIB

// ===========================================================================
// SDL3 platform block
// ===========================================================================
#if defined(WLX_GALLERY_SDL3)

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 800
static SDL_Window   *g_sdl3_window;
static SDL_Renderer *g_sdl3_renderer;
static TTF_Font     *g_sdl3_font_title;
static TTF_Font     *g_sdl3_font_body;
static TTF_Font     *g_sdl3_font_small;
static WLX_Font      g_sdl3_wf_title;
static WLX_Font      g_sdl3_wf_body;
static WLX_Font      g_sdl3_wf_small;
static WLX_Theme     g_sdl3_theme_dark;
static WLX_Theme     g_sdl3_theme_light;
static WLX_Theme     g_sdl3_theme_glass;
static WLX_Theme     g_sdl3_theme_brand;
static const WLX_Theme *g_sdl3_themes[GALLERY_THEME_COUNT];
static WLX_Context  *g_sdl3_ctx;

static bool          g_sdl3_icon_atlas_ready;
static WLX_Texture   g_sdl3_icon_atlas;

static Uint64        g_sdl3_frame_start;
static int           g_sdl3_fps;
static int           g_sdl3_frame_count;
static Uint64        g_sdl3_fps_timer;

static bool          g_sdl3_quit;

static void gallery_icon_atlas_create(WLX_Context *ctx) {
    (void)ctx;
    SDL_Texture *tex = SDL_CreateTexture(g_sdl3_renderer,
        SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
        WLX_ICONS_WIDTH, WLX_ICONS_HEIGHT);
    if (!tex) {
        printf("WARNING: could not create icon atlas texture: %s\n", SDL_GetError());
        return;
    }
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
    if (!SDL_UpdateTexture(tex, NULL, wlx_icons_rgba, WLX_ICONS_WIDTH * 4)) {
        printf("WARNING: SDL_UpdateTexture for icon atlas failed: %s\n", SDL_GetError());
        SDL_DestroyTexture(tex);
        return;
    }
    g_sdl3_icon_atlas = (WLX_Texture){
        .handle = (uintptr_t)tex,
        .width  = WLX_ICONS_WIDTH,
        .height = WLX_ICONS_HEIGHT,
    };
    g_sdl3_icon_atlas_ready = true;
}

static void gallery_icon_atlas_destroy(void) {
    if (g_sdl3_icon_atlas_ready) {
        SDL_DestroyTexture((SDL_Texture *)(uintptr_t)g_sdl3_icon_atlas.handle);
        g_sdl3_icon_atlas = (WLX_Texture){0};
        g_sdl3_icon_atlas_ready = false;
    }
}

static WLX_Texture gallery_icon_atlas_texture(void) {
    return g_sdl3_icon_atlas_ready ? g_sdl3_icon_atlas : (WLX_Texture){0};
}

static bool gallery_icon_atlas_ready(void) {
    return g_sdl3_icon_atlas_ready;
}

static bool gallery_platform_init(Gallery_State *gs) {
    printf("Wollix Widget Gallery (SDL3)\n");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

    g_sdl3_window = SDL_CreateWindow("Wollix Widget Gallery",
        gallery_perf_window_width(WINDOW_WIDTH), gallery_perf_window_height(WINDOW_HEIGHT),
        (gallery_perf_enabled() ? 0 : SDL_WINDOW_RESIZABLE) | SDL_WINDOW_OPENGL);
    if (!g_sdl3_window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    g_sdl3_renderer = SDL_CreateRenderer(g_sdl3_window, NULL);
    if (!g_sdl3_renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_sdl3_window);
        SDL_Quit();
        return false;
    }

    if (!TTF_Init()) {
        fprintf(stderr, "TTF_Init failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(g_sdl3_renderer);
        SDL_DestroyWindow(g_sdl3_window);
        SDL_Quit();
        return false;
    }

    g_sdl3_font_title = TTF_OpenFont(GALLERY_FONT_PATH, 30);
    g_sdl3_font_body  = TTF_OpenFont(GALLERY_FONT_PATH, 20);
    g_sdl3_font_small = TTF_OpenFont(GALLERY_FONT_PATH, 18);
    if (!g_sdl3_font_title || !g_sdl3_font_body || !g_sdl3_font_small) {
        fprintf(stderr, "TTF_OpenFont failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_DestroyRenderer(g_sdl3_renderer);
        SDL_DestroyWindow(g_sdl3_window);
        SDL_Quit();
        return false;
    }

    g_sdl3_wf_title = wlx_font_from_sdl3(g_sdl3_font_title);
    g_sdl3_wf_body  = wlx_font_from_sdl3(g_sdl3_font_body);
    g_sdl3_wf_small = wlx_font_from_sdl3(g_sdl3_font_small);

    WLX_Font body_font = gallery_platform_font(GALLERY_FONT_BODY);
    g_sdl3_theme_dark = gallery_theme_copy_with_font(wlx_theme_dark, body_font);
    g_sdl3_theme_light = gallery_theme_copy_with_font(wlx_theme_light, body_font);
    g_sdl3_theme_glass = gallery_theme_copy_with_font(wlx_theme_glass, body_font);
    g_sdl3_theme_brand = gallery_brand_theme(body_font);
    gallery_update_custom_theme(gs, body_font);
    g_sdl3_themes[GALLERY_THEME_DARK] = &g_sdl3_theme_dark;
    g_sdl3_themes[GALLERY_THEME_LIGHT] = &g_sdl3_theme_light;
    g_sdl3_themes[GALLERY_THEME_GLASS] = &g_sdl3_theme_glass;
    g_sdl3_themes[GALLERY_THEME_BRAND] = &g_sdl3_theme_brand;
    g_sdl3_themes[GALLERY_THEME_CUSTOM] = &gs->custom_theme;

    g_sdl3_ctx = malloc(sizeof(*g_sdl3_ctx));
    if (!g_sdl3_ctx) return false;
    memset(g_sdl3_ctx, 0, sizeof(*g_sdl3_ctx));
    wlx_context_init_sdl3(g_sdl3_ctx, g_sdl3_window, g_sdl3_renderer);

    gallery_icon_atlas_create(g_sdl3_ctx);

    for (int i = 0; i < 10; i++) {
        snprintf(gs->dynamic_names[i], sizeof(gs->dynamic_names[i]), "Panel %d", i + 1);
    }

    g_sdl3_fps_timer = SDL_GetTicksNS();
    g_sdl3_frame_count = 0;
    g_sdl3_fps = 0;
    g_sdl3_quit = false;
    return true;
}

static void gallery_platform_shutdown(Gallery_State *gs) {
    (void)gs;
    gallery_icon_atlas_destroy();
    if (g_sdl3_ctx) {
        wlx_context_destroy(g_sdl3_ctx);
        free(g_sdl3_ctx);
        g_sdl3_ctx = NULL;
    }
    wlx_sdl3_text_cache_clear();
    if (g_sdl3_font_small) TTF_CloseFont(g_sdl3_font_small);
    if (g_sdl3_font_body)  TTF_CloseFont(g_sdl3_font_body);
    if (g_sdl3_font_title) TTF_CloseFont(g_sdl3_font_title);
    TTF_Quit();
    if (g_sdl3_renderer) SDL_DestroyRenderer(g_sdl3_renderer);
    if (g_sdl3_window)   SDL_DestroyWindow(g_sdl3_window);
    SDL_Quit();
}

static bool gallery_platform_begin_frame(Gallery_State *gs, WLX_Rect *out_root) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            g_sdl3_quit = true;
            return false;
        }
    }
    if (g_sdl3_quit) return false;

    int rw = 0, rh = 0;
    SDL_GetRenderOutputSize(g_sdl3_renderer, &rw, &rh);
    *out_root = (WLX_Rect){ 0, 0, (float)rw, (float)rh };

    if (gs->theme_mode < 0 || gs->theme_mode >= GALLERY_THEME_COUNT) gs->theme_mode = GALLERY_THEME_DARK;
    WLX_Font body_font = gallery_platform_font(GALLERY_FONT_BODY);
    g_sdl3_theme_brand = gallery_brand_theme(body_font);
    gallery_update_custom_theme(gs, body_font);
    g_sdl3_ctx->theme = g_sdl3_themes[gs->theme_mode];
    gallery_perf_before_frame(g_sdl3_ctx, *out_root);
    wlx_begin(g_sdl3_ctx, *out_root, wlx_process_sdl3_input);

    g_sdl3_frame_start = SDL_GetTicksNS();
    return true;
}

static void gallery_platform_end_frame(Gallery_State *gs) {
    wlx_end(g_sdl3_ctx);
    gallery_perf_backend_present_begin();
    SDL_RenderPresent(g_sdl3_renderer);
    gallery_perf_backend_present_end();
    gallery_perf_backend_end_frame(g_sdl3_ctx);
    gallery_perf_after_frame(g_sdl3_ctx, gs);

    g_sdl3_frame_count++;
    Uint64 now = SDL_GetTicksNS();
    Uint64 elapsed = now - g_sdl3_fps_timer;
    if (elapsed >= 1000000000ULL) {
        g_sdl3_fps = g_sdl3_frame_count;
        g_sdl3_frame_count = 0;
        g_sdl3_fps_timer = now;
    }
}

static void gallery_platform_clear(WLX_Color bg) {
    SDL_SetRenderDrawBlendMode(g_sdl3_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_sdl3_renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderClear(g_sdl3_renderer);
}

static int gallery_platform_fps(void) {
    return g_sdl3_fps;
}

static WLX_Font gallery_platform_font(Gallery_Font_Slot slot) {
    switch (slot) {
    case GALLERY_FONT_TITLE: return g_sdl3_wf_title;
    case GALLERY_FONT_BODY:  return g_sdl3_wf_body;
    case GALLERY_FONT_SMALL: return g_sdl3_wf_small;
    }
    return g_sdl3_wf_body;
}

int main(int argc, char **argv) {
    if (!gallery_perf_parse_args(argc, argv, &g)) return 1;
    if (!gallery_platform_init(&g)) {
        gallery_perf_shutdown();
        return 1;
    }
    WLX_Rect root;
    while (gallery_platform_begin_frame(&g, &root)) {
        gallery_platform_clear(g_sdl3_ctx->theme->background);
        gallery_render_frame(g_sdl3_ctx, &g);
        gallery_platform_end_frame(&g);
        if (gallery_perf_finished()) break;
    }
    gallery_platform_shutdown(&g);
    gallery_perf_shutdown();
    return 0;
}

#endif // WLX_GALLERY_SDL3

// ===========================================================================
// WASM platform block
// ===========================================================================
#if defined(WLX_GALLERY_WASM)

// Shared-memory input state exported to JS host.
WLX_Input_State wlx_wasm_input_state = {0};

static WLX_Context *g_wasm_ctx;
static WLX_Theme    g_wasm_theme_dark;
static WLX_Theme    g_wasm_theme_light;
static WLX_Theme    g_wasm_theme_glass;
static WLX_Theme    g_wasm_theme_brand;
static const WLX_Theme *g_wasm_themes[GALLERY_THEME_COUNT];

// Page-pool allocator: backs the general arena group so that buffer growth
// recycles freed blocks instead of leaking them through the bump shim.
static WLX_Wasm_Pool g_wasm_pool;
static WLX_Allocator g_wasm_alloc;

static bool         g_wasm_icon_atlas_ready;
static WLX_Texture  g_wasm_icon_atlas;

static void gallery_icon_atlas_create(WLX_Context *ctx) {
    (void)ctx;
    g_wasm_icon_atlas = wlx_wasm_texture_create(
        wlx_icons_rgba, WLX_ICONS_WIDTH, WLX_ICONS_HEIGHT);
    g_wasm_icon_atlas_ready = g_wasm_icon_atlas.width > 0;
    if (!g_wasm_icon_atlas_ready) {
        printf("WARNING: could not create icon atlas texture\n");
    }
}

static void gallery_icon_atlas_destroy(void) {
    if (g_wasm_icon_atlas_ready) {
        wlx_wasm_texture_destroy(g_wasm_icon_atlas);
        g_wasm_icon_atlas = (WLX_Texture){0};
        g_wasm_icon_atlas_ready = false;
    }
}

static WLX_Texture gallery_icon_atlas_texture(void) {
    return g_wasm_icon_atlas_ready ? g_wasm_icon_atlas : (WLX_Texture){0};
}

static bool gallery_icon_atlas_ready(void) {
    return g_wasm_icon_atlas_ready;
}

static bool gallery_platform_init(Gallery_State *gs) {
    WLX_Font body_font = gallery_platform_font(GALLERY_FONT_BODY);
    g_wasm_theme_dark = gallery_theme_copy_with_font(wlx_theme_dark, body_font);
    g_wasm_theme_light = gallery_theme_copy_with_font(wlx_theme_light, body_font);
    g_wasm_theme_glass = gallery_theme_copy_with_font(wlx_theme_glass, body_font);
    g_wasm_theme_brand = gallery_brand_theme(body_font);
    gallery_update_custom_theme(gs, body_font);
    g_wasm_themes[GALLERY_THEME_DARK] = &g_wasm_theme_dark;
    g_wasm_themes[GALLERY_THEME_LIGHT] = &g_wasm_theme_light;
    g_wasm_themes[GALLERY_THEME_GLASS] = &g_wasm_theme_glass;
    g_wasm_themes[GALLERY_THEME_BRAND] = &g_wasm_theme_brand;
    g_wasm_themes[GALLERY_THEME_CUSTOM] = &gs->custom_theme;

    g_wasm_ctx = malloc(sizeof(*g_wasm_ctx));
    if (!g_wasm_ctx) return false;
    memset(g_wasm_ctx, 0, sizeof(*g_wasm_ctx));

    memset(&g_wasm_pool, 0, sizeof(g_wasm_pool));
    g_wasm_alloc = wlx_wasm_allocator(&g_wasm_pool);
    WLX_Arena_Pool_Config cfg = {
        .contiguous = NULL,
        .general    = &g_wasm_alloc,
    };
    wlx_context_init_ex(g_wasm_ctx, &cfg);
    wlx_context_init_wasm(g_wasm_ctx);

    gallery_icon_atlas_create(g_wasm_ctx);

    for (int i = 0; i < 10; i++) {
        snprintf(gs->dynamic_names[i], sizeof(gs->dynamic_names[i]), "Panel %d", i + 1);
    }
    return true;
}

static void gallery_platform_shutdown(Gallery_State *gs) {
    (void)gs;
    gallery_icon_atlas_destroy();
    if (g_wasm_ctx) {
        wlx_context_destroy(g_wasm_ctx);
        free(g_wasm_ctx);
        g_wasm_ctx = NULL;
    }
}

static bool gallery_platform_begin_frame(Gallery_State *gs, WLX_Rect *out_root) {
    (void)gs; (void)out_root;
    return false;
}

static void gallery_platform_end_frame(Gallery_State *gs) {
    (void)gs;
}

static void gallery_platform_clear(WLX_Color bg) {
    (void)bg;
}

static int gallery_platform_fps(void) {
    static float smoothed_fps = 0.0f;
    float ft = g_wasm_ctx->backend.get_frame_time();
    if (ft > 0.0001f) {
        float instant = 1.0f / ft;
        if (smoothed_fps <= 0.0f) smoothed_fps = instant;
        else smoothed_fps += 0.02f * (instant - smoothed_fps);
    }
    return (smoothed_fps > 0.0f) ? (int)(smoothed_fps + 0.5f) : 0;
}

static WLX_Font gallery_platform_font(Gallery_Font_Slot slot) {
    (void)slot;
    return WLX_FONT_DEFAULT;
}

// ---------------------------------------------------------------------------
// Exported entry points for the JS host (wollix_wasm.js)
// ---------------------------------------------------------------------------
__attribute__((export_name("wlx_wasm_init")))
void wlx_wasm_init(void) {
    gallery_platform_init(&g);
}

__attribute__((export_name("wlx_wasm_frame")))
void wlx_wasm_frame(float width, float height) {
    WLX_Rect root = { 0, 0, width, height };

    if (g.theme_mode < 0 || g.theme_mode >= GALLERY_THEME_COUNT) g.theme_mode = GALLERY_THEME_DARK;
    WLX_Font body_font = gallery_platform_font(GALLERY_FONT_BODY);
    g_wasm_theme_brand = gallery_brand_theme(body_font);
    gallery_update_custom_theme(&g, body_font);
    g_wasm_ctx->theme = g_wasm_themes[g.theme_mode];

    gallery_perf_before_frame(g_wasm_ctx, root);
    wlx_begin(g_wasm_ctx, root, wlx_process_wasm_input);

    gallery_platform_clear(g_wasm_ctx->theme->background);
    g_wasm_ctx->backend.draw_rect(root, g_wasm_ctx->theme->background);

    gallery_render_frame(g_wasm_ctx, &g);

    wlx_end(g_wasm_ctx);
    gallery_perf_backend_end_frame(g_wasm_ctx);
    gallery_perf_after_frame(g_wasm_ctx, &g);
}

// Validation hook: reports current page-pool counters so a host (or test
// harness) can assert that allocations stabilize across frames.
//   slot 0: alloc_count    (fresh malloc invocations)
//   slot 1: reuse_count    (pulls from the per-class free list)
//   slot 2: free_count     (returns to the free list)
//   slot 3: bytes_in_use   (currently handed out)
//   slot 4: high_water     (peak bytes_in_use)
__attribute__((export_name("wlx_wasm_pool_stats")))
void wlx_wasm_pool_stats(size_t *out, size_t out_len) {
    size_t values[5];
    size_t i;
    if (out == NULL || out_len == 0) return;
    values[0] = g_wasm_pool.alloc_count;
    values[1] = g_wasm_pool.reuse_count;
    values[2] = g_wasm_pool.free_count;
    values[3] = g_wasm_pool.bytes_in_use;
    values[4] = g_wasm_pool.high_water;
    if (out_len > 5) out_len = 5;
    for (i = 0; i < out_len; i++) out[i] = values[i];
}

#endif // WLX_GALLERY_WASM
