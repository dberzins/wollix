// gallery.c - Widget gallery demo for wollix.
// Showcases every widget, layout mode, and theming option with live tweaking.
// One source compiled three ways via WLX_GALLERY_RAYLIB / _SDL3 / _WASM.
// Build: make gallery (Raylib), make gallery_sdl3 (SDL3), make wasm-site (WASM).

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

typedef struct {
    WLX_Texture checked;
    WLX_Texture unchecked;
} Gallery_Checkbox_Textures;

static bool gallery_has_texture_assets(void);
static Gallery_Checkbox_Textures gallery_texture_checkbox_assets(void);

#define ROW_H 40

// ---------------------------------------------------------------------------
// Gallery-level color constants (backend-neutral, match Raylib's RED/GREEN/BLUE)
// ---------------------------------------------------------------------------
#define GALLERY_RED   WLX_RGBA(230, 41,  55,  255)
#define GALLERY_GREEN WLX_RGBA(0,   228, 48,  255)
#define GALLERY_BLUE  WLX_RGBA(0,   121, 241, 255)

// ---------------------------------------------------------------------------
// Section forward declarations
// ---------------------------------------------------------------------------
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
    { "Borders",        section_borders },
    { "Auto Layout",    section_auto_layout },
    { "Progress/Toggle/Radio", section_progress_toggle_radio },
};
#define SECTION_COUNT ((int)(sizeof(sections) / sizeof(sections[0])))

// ---------------------------------------------------------------------------
// Application state
// ---------------------------------------------------------------------------
typedef struct { float r, g, b; } ColorF;

struct Gallery_State {
    int active_section;
    int theme_mode;  // 0=dark, 1=light, 2=glass

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

static const char *theme_names[] = { "Dark", "Light", "Glass" };

static Gallery_State g = {
    .active_section = 0,
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

// Derive a tinted heading color from the current theme background.
// In dark themes the tint is added; in light themes it is subtracted.
static WLX_Color heading_color(const WLX_Theme *t, int tr, int tg, int tb) {
    int lum = ((int)t->background.r + t->background.g + t->background.b) / 3;
    int s = (lum < 128) ? 1 : -1;
    int avg = (tr + tg + tb) / 3;
    int r = (int)t->background.r + s * avg;
    int g = (int)t->background.g + s * avg;
    int b = (int)t->background.b + s * avg;
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return (WLX_Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
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

// Font size constants
#define SECTION_FS  26
#define SMALL_FS    14
#define TINY_FS     13
#define DEMO_FS     18

// Option-panel constants (sidebar widgets)
#define OPT_H   32
#define OPT_FS  14

// Tint macros for heading_color
#define SPLIT_BG(ctx)     heading_color(ctx->theme, 8, 8, 16)
#define OPTIONS_BG(ctx)   heading_color(ctx->theme, 20, 12, 12)
#define SECTION_BG(ctx)   heading_color(ctx->theme, 12, 12, 20)
#define CONTENT_BG(ctx)   heading_color(ctx->theme, 17, 12, 32)

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
// Section: Label
// ---------------------------------------------------------------------------
static void section_label(WLX_Context *ctx, Gallery_State *st) {
    int fs = (int)st->label_font_size;

    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

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

        wlx_panel_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Button
// ---------------------------------------------------------------------------
static void section_button(WLX_Context *ctx, Gallery_State *st) {
    int fs = (int)st->button_font_size;
    int bh = (int)st->button_height;
    WLX_Color bc = to_color(st->button_color.r, st->button_color.g, st->button_color.b, 1.0f);

    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

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

        char click_buf[64];
        snprintf(click_buf, sizeof(click_buf), "Click count: %d", st->button_click_count);
        wlx_label(ctx, click_buf,
            .font_size = DEMO_FS, .height = ROW_H, .align = WLX_CENTER);

        if (wlx_button(ctx, "Reset Counter",
            .height = ROW_H, .align = WLX_CENTER,
            .back_color = heading_color(ctx->theme, 42, 12, 12))) {
            st->button_click_count = 0;
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
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

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

    SUB_HEADING(ctx, "Texture Checkbox (checkbox_tex)");

    if (gallery_has_texture_assets()) {
        Gallery_Checkbox_Textures textures = gallery_texture_checkbox_assets();
        wlx_checkbox(ctx, "Texture toggle", &st->checkbox_tex_val,
            .height = ROW_H, .font_size = fs,
            .tex_checked = textures.checked,
            .tex_unchecked = textures.unchecked);
    } else {
        wlx_label(ctx, "Texture assets unavailable on this host yet.",
            .height = ROW_H, .font_size = fs,
            .show_background = true,
            .back_color = heading_color(ctx->theme, 10, 10, 18));
    }

        wlx_panel_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Slider
// ---------------------------------------------------------------------------
static void section_slider(WLX_Context *ctx, Gallery_State *st) {
    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

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

    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

            wlx_slider(ctx, "Font Size  ", &st->input_font_size,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 10.0f, .max_value = 30.0f);
            wlx_slider(ctx, "Height     ", &st->input_height,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 28.0f, .max_value = 80.0f);
            color_sliders(ctx, "Focus", &st->input_focus_color, OPT_H, OPT_FS);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

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
            .back_color = heading_color(ctx->theme, 42, 12, 12))) {
            for (int i = 0; i < 4; i++) st->inputs[i][0] = '\0';
        }

        wlx_panel_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Scroll Panel
// ---------------------------------------------------------------------------
static void section_scroll_panel(WLX_Context *ctx, Gallery_State *st) {
    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

            wlx_slider(ctx, "Scrollbar W", &st->scroll_sb_width,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 4.0f, .max_value = 30.0f);
            wlx_slider(ctx, "Wheel Speed", &st->scroll_wheel_speed,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 5.0f, .max_value = 100.0f);
            wlx_checkbox(ctx, "Show Scrollbar", &st->scroll_show_scrollbar,
                .height = OPT_H, .font_size = OPT_FS);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

        // ======== RIGHT: Content ========
        wlx_panel_begin(ctx, .title = "Scroll Panel",
            PANEL_TITLE_DEFAULTS,
            .title_back_color = SECTION_BG(ctx));

        wlx_label(ctx, "Scrollable container with auto or fixed content height.",
            .height = DESC_H);

        SUB_HEADING(ctx, "Auto-Height (20 items)");
        wlx_scroll_panel_begin(ctx, -1,
            .height = 200,
            .back_color = heading_color(ctx->theme, 4, 4, 10),
            .scrollbar_color = heading_color(ctx->theme, 37, 37, 47),
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
                    .front_color = heading_color(ctx->theme, 80, 80, 120));
                for (int i = 0; i < 20; i++) {
                    wlx_push_id(ctx, (size_t)i);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "  Item %d", i + 1);
                    WLX_Color row_bg = (i % 2 == 0)
                        ? heading_color(ctx->theme, 7, 7, 14)
                        : SECTION_BG(ctx);
                    wlx_label(ctx, buf,
                        .height = DESC_H, .font_size = 15,
                        .back_color = row_bg, .align = WLX_LEFT);
                    wlx_pop_id(ctx);
                }
            wlx_layout_end(ctx);
        wlx_scroll_panel_end(ctx);

        SUB_HEADING(ctx, "Nested Scroll Panels");
        wlx_layout_begin(ctx, 2, WLX_HORZ, .padding = 0, .gap = 4);
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
                        wlx_label(ctx, buf, .height = 28, .font_size = SMALL_FS,
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
                        wlx_label(ctx, buf, .height = 28, .font_size = SMALL_FS,
                            .back_color = heading_color(ctx->theme, 8, 4, 12));
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);
            wlx_scroll_panel_end(ctx);
        wlx_layout_end(ctx);

        wlx_panel_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Widget (Colored Rect)
// ---------------------------------------------------------------------------
static void section_widget(WLX_Context *ctx, Gallery_State *st) {
    (void)st;

    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Info ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

            wlx_label(ctx, "No adjustable options.",
                .font_size = OPT_FS, .height = OPT_H);
            wlx_label(ctx, "wlx_widget() draws a colored rect for swatches, spacers, and dividers.",
                .font_size = OPT_FS, .height = 60, .wrap = true);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

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

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Linear Layout
// ---------------------------------------------------------------------------
static void section_layout_linear(WLX_Context *ctx, Gallery_State *st) {
    int slots = (int)st->layout_slot_count;
    if (slots < 2) slots = 2;
    if (slots > 10) slots = 10;

    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

            static float layout_padding = 0;
            
            wlx_slider(ctx, "Slot Count ", &st->layout_slot_count,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 2.0f, .max_value = 10.0f);
            wlx_slider(ctx, "Padding    ", &layout_padding,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 0.0f, .max_value = 20.0f);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

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
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

            wlx_slider(ctx, "Rows       ", &st->grid_rows,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 1.0f, .max_value = 8.0f);
            wlx_slider(ctx, "Cols       ", &st->grid_cols,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 1.0f, .max_value = 8.0f);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

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

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Flex & Sizing
// ---------------------------------------------------------------------------
static void section_layout_flex(WLX_Context *ctx, Gallery_State *st) {
    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

            wlx_slider(ctx, "Weight A   ", &st->flex_weight_a,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 0.1f, .max_value = 5.0f);
            wlx_slider(ctx, "Weight B   ", &st->flex_weight_b,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 0.1f, .max_value = 5.0f);
            wlx_slider(ctx, "Min        ", &st->flex_min,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 0.0f, .max_value = 400.0f);
            wlx_slider(ctx, "Max        ", &st->flex_max,
                .height = OPT_H, .font_size = OPT_FS, .min_value = 100.0f, .max_value = 800.0f);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

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

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Theming
// ---------------------------------------------------------------------------

// Helper: render a row of sample widgets under the current ctx->theme
static void theme_sample_widgets(WLX_Context *ctx) {
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H),
            WLX_SLOT_PX(ROW_H)
        ));
        
        wlx_label(ctx, "Sample Label", .height = ROW_H, .align = WLX_CENTER);
        {
            static bool t_check = true;
            static float t_slider = 0.5f;
            static char t_input[64] = "Edit me";
            wlx_button(ctx, "Sample Button", .height = ROW_H, .align = WLX_CENTER);
            wlx_checkbox(ctx, "Sample Checkbox", &t_check, .height = ROW_H);
            wlx_slider(ctx, "Sample  ", &t_slider, .height = ROW_H);
            wlx_inputbox(ctx, "Input:", t_input, sizeof(t_input), .height = ROW_H);
        }
    wlx_layout_end(ctx);
}

static void section_theming(WLX_Context *ctx, Gallery_State *st) {
    // Build custom theme from sliders (needed before rendering both panels)
    st->custom_theme = wlx_theme_dark;
    st->custom_theme.background = to_color(st->theme_bg.r, st->theme_bg.g, st->theme_bg.b, 1.0f);
    st->custom_theme.foreground = to_color(st->theme_fg.r, st->theme_fg.g, st->theme_fg.b, 1.0f);
    st->custom_theme.surface    = to_color(st->theme_surface.r, st->theme_surface.g, st->theme_surface.b, 1.0f);
    st->custom_theme.accent     = to_color(st->theme_accent.r, st->theme_accent.g, st->theme_accent.b, 1.0f);
    st->custom_theme.hover_brightness = st->theme_hover_brightness;
    st->custom_theme.padding    = st->theme_padding;
    st->custom_theme.roundness  = st->theme_roundness;

    wlx_split_begin(ctx,
        // .first_size = WLX_SLOT_PX(300),
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

                wlx_label(ctx, "Background", .font_size = OPT_FS, .height = SMALL_H);
                wlx_push_id(ctx, 100);
                color_sliders(ctx, "BG", &st->theme_bg, OPT_H, OPT_FS);
                wlx_pop_id(ctx);

                wlx_label(ctx, "Foreground", .font_size = OPT_FS, .height = SMALL_H);
                wlx_push_id(ctx, 101);
                color_sliders(ctx, "FG", &st->theme_fg, OPT_H, OPT_FS);
                wlx_pop_id(ctx);

                wlx_label(ctx, "Surface", .font_size = OPT_FS, .height = SMALL_H);
                wlx_push_id(ctx, 102);
                color_sliders(ctx, "Srf", &st->theme_surface, OPT_H, OPT_FS);
                wlx_pop_id(ctx);

                wlx_label(ctx, "Accent", .font_size = OPT_FS, .height = SMALL_H);
                wlx_push_id(ctx, 103);
                color_sliders(ctx, "Acc", &st->theme_accent, OPT_H, OPT_FS);
                wlx_pop_id(ctx);

                wlx_label(ctx, "Other", .font_size = OPT_FS, .height = SMALL_H);
                wlx_slider(ctx, "Hover Brt", &st->theme_hover_brightness, .height = OPT_H, .font_size = OPT_FS,
                    .min_value = 0.0f, .max_value = 2.0f);
                wlx_slider(ctx, "Padding  ", &st->theme_padding, .height = OPT_H, .font_size = OPT_FS,
                    .min_value = 0.0f, .max_value = 20.0f);
                wlx_slider(ctx, "Roundness", &st->theme_roundness, .height = OPT_H, .font_size = OPT_FS,
                    .min_value = 0.0f, .max_value = 1.0f);

            wlx_panel_end(ctx);

    wlx_split_next(ctx);

        // ======== RIGHT: Content ========
            wlx_layout_begin_s(ctx, WLX_VERT,
                WLX_SIZES(
                    WLX_SLOT_CONTENT,  // heading
                    WLX_SLOT_CONTENT,  // description
                    WLX_SLOT_CONTENT,  // sub heading: side-by-side
                    WLX_SLOT_PX(240),  // side-by-side panels
                    WLX_SLOT_CONTENT,  // sub heading: swatches
                    WLX_SLOT_PX(36),   // swatch color boxes
                    WLX_SLOT_PX(24),   // swatch labels
                    WLX_SLOT_CONTENT,  // sub heading: custom builder
                    WLX_SLOT_PX(200)   // custom theme preview
                ));

                SECTION_HEADING(ctx, "Theming");

                wlx_label(ctx, "Built-in dark, light, and glass themes with custom theme creation.",
        .height = DESC_H);
                SUB_HEADING(ctx, "Side-by-Side: Dark vs Light vs Glass");
                wlx_layout_begin(ctx, 3, WLX_HORZ, .padding = 0);
                {
                    const struct { const char *name; const WLX_Theme *t; } previews[] = {
                        { "Dark",  &wlx_theme_dark },
                        { "Light", &wlx_theme_light },
                        { "Glass", &wlx_theme_glass },
                    };
                    for (int i = 0; i < 3; i++) {
                        const WLX_Theme *saved = ctx->theme;
                        ctx->theme = previews[i].t;
                        wlx_push_id(ctx, (size_t)i);
                            wlx_layout_begin_s(ctx, WLX_VERT,
                                WLX_SIZES(WLX_SLOT_PX(28), WLX_SLOT_FLEX(1)),
                                .back_color = previews[i].t->background);
                                
                                wlx_label(ctx, previews[i].name, .height = 28, .font_size = DEMO_FS, .align = WLX_CENTER);
                                theme_sample_widgets(ctx);
                            wlx_layout_end(ctx);
                        wlx_pop_id(ctx);
                        ctx->theme = saved;
                    }
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
                    wlx_layout_begin(ctx, 5, WLX_HORZ, .padding = 4);
                        for (int i = 0; i < 5; i++) {
                            wlx_push_id(ctx, (size_t)i);
                            wlx_widget(ctx,
                                .widget_align = WLX_CENTER, .width = 40, .height = DESC_H,
                                .color = swatches[i].c);
                            wlx_pop_id(ctx);
                        }
                    wlx_layout_end(ctx);
                    wlx_layout_begin(ctx, 5, WLX_HORZ, .padding = 4);
                        for (int i = 0; i < 5; i++) {
                            wlx_push_id(ctx, (size_t)(i + 5));
                            wlx_label(ctx, swatches[i].name,
                                .height = 20, .font_size = 14, .align = WLX_CENTER);
                            wlx_pop_id(ctx);
                        }
                    wlx_layout_end(ctx);
                }

                SUB_HEADING(ctx, "Custom Theme Preview");
                {
                    const WLX_Theme *saved = ctx->theme;
                    ctx->theme = &st->custom_theme;
                    wlx_push_id(ctx, 3);
                        wlx_layout_begin_s(ctx, WLX_VERT,
                            WLX_SIZES(WLX_SLOT_PX(28), WLX_SLOT_FLEX(1)));
                            wlx_label(ctx, "Custom Theme Preview", .height = 28, .font_size = DEMO_FS, .align = WLX_CENTER);
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
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

            wlx_slider(ctx, "Widget      ", &st->opacity_control,
                .height = OPT_H, .min_value = 0.0f, .max_value = 1.0f, .font_size = OPT_FS);
            wlx_slider(ctx, "Theme       ", &st->theme_opacity,
                .height = OPT_H, .min_value = 0.0f, .max_value = 1.0f, .font_size = OPT_FS);
            wlx_slider(ctx, "Stack       ", &st->stack_opacity,
                .height = OPT_H, .min_value = 0.0f, .max_value = 1.0f, .font_size = OPT_FS);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

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

    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

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
                        ? heading_color(ctx->theme, 10, 10, 18)
                        : heading_color(ctx->theme, 14, 14, 22);

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
    WLX_Color bdr = to_color(st->border_color.r, st->border_color.g, st->border_color.b, 1.0f);
    // float bw = st->border_width;
    static float bw = 0.35f;

    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

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
    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

                wlx_slider(ctx, "Items      ", &st->auto_item_count,
                    .height = OPT_H, .font_size = OPT_FS, .min_value = 1.0f, .max_value = 10.0f);
                wlx_slider(ctx, "Item H     ", &st->auto_item_height,
                    .height = OPT_H, .font_size = OPT_FS, .min_value = 20.0f, .max_value = 80.0f);
                wlx_slider(ctx, "Header %   ", &st->auto_header_pct,
                    .height = OPT_H, .font_size = OPT_FS, .min_value = 5.0f, .max_value = 40.0f);
                wlx_slider(ctx, "Sidebar %  ", &st->auto_sidebar_pct,
                    .height = OPT_H, .font_size = OPT_FS, .min_value = 10.0f, .max_value = 60.0f);

            wlx_panel_end(ctx);

    wlx_split_next(ctx);

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

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Section: Progress / Toggle / Radio
// ---------------------------------------------------------------------------
static void section_progress_toggle_radio(WLX_Context *ctx, Gallery_State *st) {
    wlx_split_begin(ctx,
        .padding = 0, .gap = 4,
        .first_back_color = SPLIT_BG(ctx));

        // ======== LEFT: Options ========
        wlx_panel_begin(ctx, .title = "Options",
            .title_height = HEADING_H,
            .title_back_color = OPTIONS_BG(ctx),
            .padding = 0);

            wlx_slider(ctx, "Progress  ", &st->progress_value,
                .height = OPT_H, .font_size = OPT_FS);

        wlx_panel_end(ctx);

    wlx_split_next(ctx);

        // ======== RIGHT: Content ========
        wlx_panel_begin(ctx, .title = "Progress / Toggle / Radio",
            PANEL_TITLE_DEFAULTS,
            .title_back_color = SECTION_BG(ctx));

    wlx_label(ctx, "widgets: progress bar, toggle switch, radio button.",
        .height = DESC_H);

    // -- Progress --
    SUB_HEADING(ctx, "Progress Bar");
    wlx_progress(ctx, st->progress_value, .height = ROW_H);

    {
        char buf[64];
        snprintf(buf, sizeof(buf), "Value: %.0f%%", st->progress_value * 100.0f);
        wlx_label(ctx, buf, .height = SMALL_H, .font_size = SMALL_FS);
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
        wlx_label(ctx, buf, .height = SMALL_H, .font_size = SMALL_FS);
    }

        wlx_panel_end(ctx);

    wlx_split_end(ctx);
}

// ---------------------------------------------------------------------------
// Shared per-frame renderer -- backend-agnostic.
// ---------------------------------------------------------------------------
static void gallery_render_frame(WLX_Context *ctx, Gallery_State *gs) {
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(44), WLX_SLOT_FLEX(1), WLX_SLOT_CONTENT));

        // -- Title bar --
        wlx_layout_begin_s(ctx, WLX_HORZ,
            WLX_SIZES(WLX_SLOT_FLEX(1), WLX_SLOT_PX(160)));
            wlx_label(ctx, "  Wollix Widget Gallery",
                .font_size = 24, .align = WLX_LEFT,
                .back_color = heading_color(ctx->theme, 10, 10, 20));

            {
                char mode_label[32];
                snprintf(mode_label, sizeof(mode_label), "Theme: %s", theme_names[gs->theme_mode]);
                if (wlx_button(ctx, mode_label,
                    .align = WLX_CENTER,
                    .back_color = heading_color(ctx->theme, 32, 32, 47))) {
                    gs->theme_mode = (gs->theme_mode + 1) % 3;
                }
            }
        wlx_layout_end(ctx);

        // -- Body: sidebar + content --
        wlx_layout_begin_s(ctx, WLX_HORZ,
            WLX_SIZES(WLX_SLOT_PX(SIDEBAR_W), WLX_SLOT_FLEX(1)),
            .gap = 4);

            // -- Sidebar --
            wlx_scroll_panel_begin(ctx, -1,
                .back_color = heading_color(ctx->theme, 4, 4, 10));
                wlx_panel_begin(ctx, .title = "Sections",
                    .title_font_size = SECTION_FS,
                    .title_height = HEADING_H,
                    .title_back_color = heading_color(ctx->theme, 10, 10, 20),
                    .padding = 0);
                    for (int i = 0; i < SECTION_COUNT; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        WLX_Color btn_bg = (i == gs->active_section)
                            ? heading_color(ctx->theme, 32, 32, 57)
                            : heading_color(ctx->theme, 10, 10, 18);
                        if (wlx_button(ctx, sections[i].name,
                            .height = SUB_HEADING_H,
                            .align = WLX_LEFT,
                            .back_color = btn_bg)) {
                            gs->active_section = i;
                        }
                        wlx_pop_id(ctx);
                    }
                wlx_panel_end(ctx);
            wlx_scroll_panel_end(ctx);

            // -- Content area --
            // Each section manages its own pane scrolling via wlx_split_begin.
            if (gs->active_section >= 0 && gs->active_section < SECTION_COUNT) {
                sections[gs->active_section].render(ctx, gs);
            }

        wlx_layout_end(ctx);

        // -- Status bar --
        {
            char status[256];
            snprintf(status, sizeof(status),
                "  FPS: %d  |  Section: %s  |  Theme: %s",
                gallery_platform_fps(),
                sections[gs->active_section].name,
                theme_names[gs->theme_mode]);
            wlx_label(ctx, status,
                .font_size = TINY_FS, .height = SMALL_H, .align = WLX_LEFT,
                .back_color = heading_color(ctx->theme, 4, 4, 10));
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

static Font         g_raylib_font;
static bool         g_raylib_font_ok;
static WLX_Font     g_raylib_font_handle = WLX_FONT_DEFAULT;
static WLX_Theme    g_raylib_theme_dark;
static WLX_Theme    g_raylib_theme_light;
static WLX_Theme    g_raylib_theme_glass;
static const WLX_Theme *g_raylib_themes[3];
static WLX_Context *g_raylib_ctx = NULL;
static bool         g_raylib_checkbox_textures_ready;
static WLX_Texture  g_raylib_checkbox_tex_on;
static WLX_Texture  g_raylib_checkbox_tex_off;

static bool gallery_has_texture_assets(void) {
    return g_raylib_checkbox_textures_ready;
}

static Gallery_Checkbox_Textures gallery_texture_checkbox_assets(void) {
    Gallery_Checkbox_Textures textures = {0};
    if (!g_raylib_checkbox_textures_ready) return textures;
    textures.checked = g_raylib_checkbox_tex_on;
    textures.unchecked = g_raylib_checkbox_tex_off;
    return textures;
}

static bool gallery_platform_init(Gallery_State *gs) {
    printf("Wollix Widget Gallery\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix Widget Gallery");
    SetTargetFPS(TARGET_FPS);

    g_raylib_font = LoadFontEx("demos/assets/PublicSans-Regular.ttf", 32, NULL, 0);
    g_raylib_font_ok = g_raylib_font.glyphCount > 0;
    if (g_raylib_font_ok) SetTextureFilter(g_raylib_font.texture, TEXTURE_FILTER_BILINEAR);
    else                  printf("WARNING: could not load PublicSans-Regular.ttf\n");
    g_raylib_font_handle = g_raylib_font_ok ? wlx_font_from_raylib(&g_raylib_font) : WLX_FONT_DEFAULT;

    g_raylib_theme_dark  = wlx_theme_dark;
    g_raylib_theme_light = wlx_theme_light;
    g_raylib_theme_glass = wlx_theme_glass;
    WLX_Font body_font = gallery_platform_font(GALLERY_FONT_BODY);
    g_raylib_theme_dark.font  = body_font;
    g_raylib_theme_light.font = body_font;
    g_raylib_theme_glass.font = body_font;
    g_raylib_themes[0] = &g_raylib_theme_dark;
    g_raylib_themes[1] = &g_raylib_theme_light;
    g_raylib_themes[2] = &g_raylib_theme_glass;

    g_raylib_ctx = malloc(sizeof(*g_raylib_ctx));
    if (!g_raylib_ctx) return false;
    memset(g_raylib_ctx, 0, sizeof(*g_raylib_ctx));
    wlx_context_init_raylib(g_raylib_ctx);

    {
        Image img_on = GenImageColor(24, 24, GALLERY_GREEN);
        Image img_off = GenImageColor(24, 24, WLX_RGBA(80, 80, 80, 255));

        g_raylib_checkbox_tex_on = LoadTextureFromImage(img_on);
        g_raylib_checkbox_tex_off = LoadTextureFromImage(img_off);

        UnloadImage(img_on);
        UnloadImage(img_off);

        g_raylib_checkbox_textures_ready =
            g_raylib_checkbox_tex_on.width > 0 && g_raylib_checkbox_tex_off.width > 0;

        if (!g_raylib_checkbox_textures_ready) {
            if (g_raylib_checkbox_tex_on.width > 0) UnloadTexture(g_raylib_checkbox_tex_on);
            if (g_raylib_checkbox_tex_off.width > 0) UnloadTexture(g_raylib_checkbox_tex_off);
            g_raylib_checkbox_tex_on = (WLX_Texture){0};
            g_raylib_checkbox_tex_off = (WLX_Texture){0};
            printf("WARNING: could not create checkbox texture assets; using fallback label\n");
        }
    }

    for (int i = 0; i < 10; i++) {
        snprintf(gs->dynamic_names[i], sizeof(gs->dynamic_names[i]), "Panel %d", i + 1);
    }
    return true;
}

static void gallery_platform_shutdown(Gallery_State *gs) {
    (void)gs;
    if (g_raylib_checkbox_textures_ready) {
        UnloadTexture(g_raylib_checkbox_tex_on);
        UnloadTexture(g_raylib_checkbox_tex_off);
        g_raylib_checkbox_tex_on = (WLX_Texture){0};
        g_raylib_checkbox_tex_off = (WLX_Texture){0};
        g_raylib_checkbox_textures_ready = false;
    }
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
    g_raylib_ctx->theme = g_raylib_themes[gs->theme_mode];
    wlx_begin(g_raylib_ctx, *out_root, wlx_process_raylib_input);
    BeginDrawing();
    return true;
}

static void gallery_platform_end_frame(Gallery_State *gs) {
    (void)gs;
    wlx_end(g_raylib_ctx);
    EndDrawing();
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

int main(void) {
    if (!gallery_platform_init(&g)) return 1;
    WLX_Rect root;
    while (gallery_platform_begin_frame(&g, &root)) {
        gallery_platform_clear(g_raylib_ctx->theme->background);
        gallery_render_frame(g_raylib_ctx, &g);
        gallery_platform_end_frame(&g);
    }
    gallery_platform_shutdown(&g);
    return 0;
}

#endif // WLX_GALLERY_RAYLIB

// ===========================================================================
// SDL3 platform block
// ===========================================================================
#if defined(WLX_GALLERY_SDL3)

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 800
#define FONT_PATH     "demos/assets/LiberationSans-Regular.ttf"

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
static const WLX_Theme *g_sdl3_themes[3];
static WLX_Context  *g_sdl3_ctx;

static bool          g_sdl3_checkbox_textures_ready;
static WLX_Texture   g_sdl3_checkbox_tex_on;
static WLX_Texture   g_sdl3_checkbox_tex_off;

static Uint64        g_sdl3_frame_start;
static int           g_sdl3_fps;
static int           g_sdl3_frame_count;
static Uint64        g_sdl3_fps_timer;

static bool          g_sdl3_quit;

static bool gallery_has_texture_assets(void) {
    return g_sdl3_checkbox_textures_ready;
}

static Gallery_Checkbox_Textures gallery_texture_checkbox_assets(void) {
    Gallery_Checkbox_Textures textures = {0};
    if (!g_sdl3_checkbox_textures_ready) return textures;
    textures.checked = g_sdl3_checkbox_tex_on;
    textures.unchecked = g_sdl3_checkbox_tex_off;
    return textures;
}

static SDL_Texture *sdl3_gen_solid_texture(int w, int h, WLX_Color c) {
    SDL_Texture *tex = SDL_CreateTexture(g_sdl3_renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!tex) return NULL;
    SDL_SetRenderTarget(g_sdl3_renderer, tex);
    SDL_SetRenderDrawColor(g_sdl3_renderer, c.r, c.g, c.b, c.a);
    SDL_RenderClear(g_sdl3_renderer);
    SDL_SetRenderTarget(g_sdl3_renderer, NULL);
    return tex;
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
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
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

    g_sdl3_font_title = TTF_OpenFont(FONT_PATH, 30);
    g_sdl3_font_body  = TTF_OpenFont(FONT_PATH, 20);
    g_sdl3_font_small = TTF_OpenFont(FONT_PATH, 18);
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

    g_sdl3_theme_dark  = wlx_theme_dark;
    g_sdl3_theme_light = wlx_theme_light;
    g_sdl3_theme_glass = wlx_theme_glass;
    WLX_Font body_font = gallery_platform_font(GALLERY_FONT_BODY);
    g_sdl3_theme_dark.font  = body_font;
    g_sdl3_theme_light.font = body_font;
    g_sdl3_theme_glass.font = body_font;
    g_sdl3_themes[0] = &g_sdl3_theme_dark;
    g_sdl3_themes[1] = &g_sdl3_theme_light;
    g_sdl3_themes[2] = &g_sdl3_theme_glass;

    g_sdl3_ctx = malloc(sizeof(*g_sdl3_ctx));
    if (!g_sdl3_ctx) return false;
    memset(g_sdl3_ctx, 0, sizeof(*g_sdl3_ctx));
    wlx_context_init_sdl3(g_sdl3_ctx, g_sdl3_window, g_sdl3_renderer);

    {
        SDL_Texture *tex_on  = sdl3_gen_solid_texture(24, 24, GALLERY_GREEN);
        SDL_Texture *tex_off = sdl3_gen_solid_texture(24, 24, WLX_RGBA(80, 80, 80, 255));
        g_sdl3_checkbox_textures_ready = (tex_on != NULL && tex_off != NULL);
        if (g_sdl3_checkbox_textures_ready) {
            g_sdl3_checkbox_tex_on  = (WLX_Texture){ .handle = (uintptr_t)tex_on,  .width = 24, .height = 24 };
            g_sdl3_checkbox_tex_off = (WLX_Texture){ .handle = (uintptr_t)tex_off, .width = 24, .height = 24 };
        } else {
            if (tex_on)  SDL_DestroyTexture(tex_on);
            if (tex_off) SDL_DestroyTexture(tex_off);
            printf("WARNING: could not create checkbox texture assets; using fallback label\n");
        }
    }

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
    if (g_sdl3_checkbox_textures_ready) {
        SDL_DestroyTexture((SDL_Texture *)(uintptr_t)g_sdl3_checkbox_tex_on.handle);
        SDL_DestroyTexture((SDL_Texture *)(uintptr_t)g_sdl3_checkbox_tex_off.handle);
        g_sdl3_checkbox_textures_ready = false;
    }
    if (g_sdl3_ctx) {
        wlx_context_destroy(g_sdl3_ctx);
        free(g_sdl3_ctx);
        g_sdl3_ctx = NULL;
    }
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

    g_sdl3_ctx->theme = g_sdl3_themes[gs->theme_mode];
    wlx_begin(g_sdl3_ctx, *out_root, wlx_process_sdl3_input);

    g_sdl3_frame_start = SDL_GetTicksNS();
    return true;
}

static void gallery_platform_end_frame(Gallery_State *gs) {
    (void)gs;
    wlx_end(g_sdl3_ctx);
    SDL_RenderPresent(g_sdl3_renderer);

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

int main(void) {
    if (!gallery_platform_init(&g)) return 1;
    WLX_Rect root;
    while (gallery_platform_begin_frame(&g, &root)) {
        gallery_platform_clear(g_sdl3_ctx->theme->background);
        gallery_render_frame(g_sdl3_ctx, &g);
        gallery_platform_end_frame(&g);
    }
    gallery_platform_shutdown(&g);
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
static const WLX_Theme *g_wasm_themes[3];

// Page-pool allocator: backs the general arena group so that buffer growth
// recycles freed blocks instead of leaking them through the bump shim.
static WLX_Wasm_Pool g_wasm_pool;
static WLX_Allocator g_wasm_alloc;

static bool gallery_has_texture_assets(void) {
    return false;
}

static Gallery_Checkbox_Textures gallery_texture_checkbox_assets(void) {
    Gallery_Checkbox_Textures textures = {0};
    return textures;
}

static bool gallery_platform_init(Gallery_State *gs) {
    g_wasm_theme_dark  = wlx_theme_dark;
    g_wasm_theme_light = wlx_theme_light;
    g_wasm_theme_glass = wlx_theme_glass;
    g_wasm_themes[0] = &g_wasm_theme_dark;
    g_wasm_themes[1] = &g_wasm_theme_light;
    g_wasm_themes[2] = &g_wasm_theme_glass;

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

    for (int i = 0; i < 10; i++) {
        snprintf(gs->dynamic_names[i], sizeof(gs->dynamic_names[i]), "Panel %d", i + 1);
    }
    return true;
}

static void gallery_platform_shutdown(Gallery_State *gs) {
    (void)gs;
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

    g_wasm_ctx->theme = g_wasm_themes[g.theme_mode];

    wlx_begin(g_wasm_ctx, root, wlx_process_wasm_input);

    gallery_platform_clear(g_wasm_ctx->theme->background);
    g_wasm_ctx->backend.draw_rect(root, g_wasm_ctx->theme->background);

    gallery_render_frame(g_wasm_ctx, &g);

    wlx_end(g_wasm_ctx);
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
