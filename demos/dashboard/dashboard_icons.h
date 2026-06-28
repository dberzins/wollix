// dashboard_icons.h - Dashboard demo local icon-atlas instrumentation.
//
// Included only by dashboard.c, alongside dashboard_theme.h and
// dashboard_components.h, following the demo-local header convention (never a
// shared cross-demo source file). This header owns the dashboard's consumption
// of the shared Lucide icon atlas (demos/assets/wlx_icons.h): the per-backend
// atlas lifecycle, atlas-tier selection, token-driven tint roles, and the
// render helpers that draw standalone icons (wlx_image) and icon+text controls
// (wlx_label / wlx_button).
//
// The atlas itself is a shared asset, not dashboard-local; this header is only a
// second consumer of it (the gallery is the first). All three backends are wired:
// each drops its own create/destroy/texture/ready into the marked section,
// guarded by its backend macro, without touching the neutral render helpers.
// Every render helper degrades gracefully when the atlas is not ready, falling
// back to a faint geometric box (standalone icons) or text-only controls
// (icon+text) so a missing atlas never crashes or leaves a blank rect. No core
// (wollix.h) or backend header is modified.

#ifndef WLX_DASHBOARD_ICONS_H
#define WLX_DASHBOARD_ICONS_H

#include "../assets/wlx_icons.h"

// ============================================================================
// Tint roles
// ============================================================================

// Semantic tint roles for dashboard icons. Each maps to a token color so an
// icon's color tracks the active mode the same way the surrounding text does.
typedef enum {
    DASHBOARD_ICON_ROLE_ON_SURFACE,    // primary on-surface (avatar, strong)
    DASHBOARD_ICON_ROLE_MUTED,         // secondary/muted chrome
    DASHBOARD_ICON_ROLE_ACCENT,        // neon primary accent
    DASHBOARD_ICON_ROLE_ACCENT_STRONG, // emphasized accent
    DASHBOARD_ICON_ROLE_TERTIARY,      // tertiary highlight (industrial orange)
    DASHBOARD_ICON_ROLE_ON_ACCENT,     // on the accent fill
    DASHBOARD_ICON_ROLE_SUCCESS,       // status success
    DASHBOARD_ICON_ROLE_WARNING,       // status warning
    DASHBOARD_ICON_ROLE_ERROR,         // status error
} Dashboard_Icon_Role;

// Resolve a tint role to a token color for the active mode.
static WLX_Color dashboard_icon_tint(const Dashboard_Tokens *tk,
                                     Dashboard_Icon_Role role) {
    switch (role) {
    case DASHBOARD_ICON_ROLE_ON_SURFACE:    return tk->color.on_surface;
    case DASHBOARD_ICON_ROLE_MUTED:         return tk->color.on_surface_muted;
    // Accent glyphs are foreground content, so they track accent text: the deep
    // primary on light (where the bright brand accent washes out), the neon
    // accent on dark. Bright tk->color.accent stays for fills/washes/glows.
    case DASHBOARD_ICON_ROLE_ACCENT:        return tk->mode == DASHBOARD_MODE_LIGHT
                                                ? tk->color.accent_strong : tk->color.accent;
    case DASHBOARD_ICON_ROLE_ACCENT_STRONG: return tk->color.accent_strong;
    // Tertiary highlight is the same industrial orange in both modes; it reads on
    // light and dark surfaces alike, so it needs no light/dark split.
    case DASHBOARD_ICON_ROLE_TERTIARY:      return tk->color.tertiary;
    case DASHBOARD_ICON_ROLE_ON_ACCENT:     return tk->color.on_accent;
    case DASHBOARD_ICON_ROLE_SUCCESS:       return tk->status.success;
    case DASHBOARD_ICON_ROLE_WARNING:       return tk->status.warning;
    case DASHBOARD_ICON_ROLE_ERROR:         return tk->status.error;
    }
    return tk->color.on_surface;
}

// ============================================================================
// Atlas-tier selection
// ============================================================================

// Source rect for icon `id` at the smallest atlas tier >= `target_px`, so a
// downscaled glyph reads crisply. Falls back to the largest tier when the
// target exceeds every tier. Returns a zero rect for an out-of-range id.
static WLX_Rect dashboard_icon_src_for(WLX_Icon id, float target_px) {
    if ((int)id < 0 || (int)id >= WLX_ICON_COUNT) return (WLX_Rect){0};
    int chosen = WLX_ICON_TIER_COUNT - 1;
    for (int i = 0; i < WLX_ICON_TIER_COUNT; i++) {
        if ((float)wlx_icon_tier_sizes[i] >= target_px) { chosen = i; break; }
    }
    WLX_Icon_Rect r = wlx_icon_rects_tiered[chosen][id];
    return (WLX_Rect){ (float)r.x, (float)r.y, (float)r.w, (float)r.h };
}

// ============================================================================
// Atlas lifecycle (per-backend)
// ============================================================================
//
// The four lifecycle functions below (create / destroy / texture / ready) are
// the only backend-specific surface. Each backend (Raylib, SDL3, bare WASM)
// supplies its own implementation, guarded by its backend macro and exposing the
// same four symbols; the neutral render helpers further down need no change.

#ifdef RAYLIB_H

static bool        s_dashboard_icon_atlas_ready;
static WLX_Texture s_dashboard_icon_atlas;

// Upload the white-alpha atlas to a GPU texture once at startup.
static void dashboard_icon_atlas_create(WLX_Context *ctx) {
    (void)ctx;
    Image img = (Image){
        .data    = (void *)wlx_icons_rgba,
        .width   = WLX_ICONS_WIDTH,
        .height  = WLX_ICONS_HEIGHT,
        .mipmaps = 1,
        .format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
    s_dashboard_icon_atlas = LoadTextureFromImage(img);
    s_dashboard_icon_atlas_ready = s_dashboard_icon_atlas.width > 0;
    if (s_dashboard_icon_atlas_ready) {
        SetTextureFilter(s_dashboard_icon_atlas, TEXTURE_FILTER_BILINEAR);
    } else {
        s_dashboard_icon_atlas = (WLX_Texture){0};
        printf("WARNING: could not create dashboard icon atlas texture\n");
    }
}

static void dashboard_icon_atlas_destroy(void) {
    if (s_dashboard_icon_atlas_ready) {
        UnloadTexture(s_dashboard_icon_atlas);
        s_dashboard_icon_atlas = (WLX_Texture){0};
        s_dashboard_icon_atlas_ready = false;
    }
}

static WLX_Texture dashboard_icon_atlas_texture(void) {
    return s_dashboard_icon_atlas_ready ? s_dashboard_icon_atlas : (WLX_Texture){0};
}

static bool dashboard_icon_atlas_ready(void) {
    return s_dashboard_icon_atlas_ready;
}

#endif // RAYLIB_H

#if defined(WLX_DASHBOARD_SDL3)

static bool        s_dashboard_icon_atlas_ready;
static WLX_Texture s_dashboard_icon_atlas;

// Upload the white-alpha atlas to an SDL texture once at startup, using the
// backend renderer set by wlx_context_init_sdl3.
static void dashboard_icon_atlas_create(WLX_Context *ctx) {
    (void)ctx;
    SDL_Texture *tex = SDL_CreateTexture(g_wlx_sdl3_renderer,
        SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
        WLX_ICONS_WIDTH, WLX_ICONS_HEIGHT);
    if (!tex) {
        printf("WARNING: could not create dashboard icon atlas texture: %s\n", SDL_GetError());
        return;
    }
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
    if (!SDL_UpdateTexture(tex, NULL, wlx_icons_rgba, WLX_ICONS_WIDTH * 4)) {
        printf("WARNING: SDL_UpdateTexture for dashboard icon atlas failed: %s\n", SDL_GetError());
        SDL_DestroyTexture(tex);
        return;
    }
    s_dashboard_icon_atlas = (WLX_Texture){
        .handle = (uintptr_t)tex,
        .width  = WLX_ICONS_WIDTH,
        .height = WLX_ICONS_HEIGHT,
    };
    s_dashboard_icon_atlas_ready = true;
}

static void dashboard_icon_atlas_destroy(void) {
    if (s_dashboard_icon_atlas_ready) {
        SDL_DestroyTexture((SDL_Texture *)(uintptr_t)s_dashboard_icon_atlas.handle);
        s_dashboard_icon_atlas = (WLX_Texture){0};
        s_dashboard_icon_atlas_ready = false;
    }
}

static WLX_Texture dashboard_icon_atlas_texture(void) {
    return s_dashboard_icon_atlas_ready ? s_dashboard_icon_atlas : (WLX_Texture){0};
}

static bool dashboard_icon_atlas_ready(void) {
    return s_dashboard_icon_atlas_ready;
}

#endif // WLX_DASHBOARD_SDL3

#if defined(WLX_DASHBOARD_WASM)

static bool        s_dashboard_icon_atlas_ready;
static WLX_Texture s_dashboard_icon_atlas;

// Hand the white-alpha atlas to the JS host's texture registry once at startup.
static void dashboard_icon_atlas_create(WLX_Context *ctx) {
    (void)ctx;
    s_dashboard_icon_atlas = wlx_wasm_texture_create(
        wlx_icons_rgba, WLX_ICONS_WIDTH, WLX_ICONS_HEIGHT);
    s_dashboard_icon_atlas_ready = s_dashboard_icon_atlas.width > 0;
}

static void dashboard_icon_atlas_destroy(void) {
    if (s_dashboard_icon_atlas_ready) {
        wlx_wasm_texture_destroy(s_dashboard_icon_atlas);
        s_dashboard_icon_atlas = (WLX_Texture){0};
        s_dashboard_icon_atlas_ready = false;
    }
}

static WLX_Texture dashboard_icon_atlas_texture(void) {
    return s_dashboard_icon_atlas_ready ? s_dashboard_icon_atlas : (WLX_Texture){0};
}

static bool dashboard_icon_atlas_ready(void) {
    return s_dashboard_icon_atlas_ready;
}

#endif // WLX_DASHBOARD_WASM

// ============================================================================
// Render helpers (backend-neutral)
// ============================================================================

// Resolve the pixel size used to pick an atlas tier for an icon+text control:
// the caller's explicit image_size when set, else a size derived from the row
// height, clamped into the atlas tier range.
static float dashboard_icon_chrome_size(float image_size, float row_height) {
    float target = image_size > 0.0f ? image_size : row_height - 16.0f;
    if (target < 16.0f) target = 16.0f;
    if (target > 48.0f) target = 48.0f;
    return target;
}

// Draw a standalone atlas icon into the current slot. `px` selects the atlas
// tier; the WLX_Image_Opt sizing/align fields size and position the glyph box,
// exactly as wlx_image. The tint defaults to the role color unless the caller
// passes an explicit .tint. When the atlas is not ready, a faint role-tinted
// box is drawn into the same slot so the layout degrades to a placeholder
// instead of a blank rect.
static void dashboard_icon_image_helper(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                        WLX_Icon icon, Dashboard_Icon_Role role,
                                        float px, WLX_Image_Opt opt,
                                        const char *file, int line) {
    WLX_Color tint = dashboard_icon_tint(tk, role);
    if (icon != WLX_ICON_COUNT && dashboard_icon_atlas_ready()) {
        opt.src = dashboard_icon_src_for(icon, px);
        if (opt.tint.a == 0) opt.tint = tint;
        if (opt.scale == WLX_IMAGE_SCALE_STRETCH) opt.scale = WLX_IMAGE_SCALE_FIT;
        wlx_image_impl(ctx, dashboard_icon_atlas_texture(), opt, file, line);
        return;
    }
    WLX_Color box = tint;
    box.a = 38;
    wlx_widget_impl(ctx, wlx_default_widget_opt(
            .pos = opt.pos, .span = opt.span, .overflow = opt.overflow,
            .padding = opt.padding, .padding_top = opt.padding_top,
            .padding_right = opt.padding_right, .padding_bottom = opt.padding_bottom,
            .padding_left = opt.padding_left,
            .widget_align = opt.widget_align,
            .width = opt.width, .height = opt.height,
            .min_width = opt.min_width, .min_height = opt.min_height,
            .max_width = opt.max_width, .max_height = opt.max_height,
            .opacity = opt.opacity,
            .color = box,
            .roundness = 0.25f, .rounded_segments = 6, .border_width = 0,
            .id = opt.id),
        file, line);
}

#define dashboard_icon_image(ctx, tk, icon, role, px, ...) \
    dashboard_icon_image_helper((ctx), (tk), (icon), (role), (px), \
        wlx_default_image_opt(__VA_ARGS__), __FILE__, __LINE__)

// Icon+text button: inject the atlas texture fields into the button options so
// the glyph renders as the control's leading image. Pass WLX_ICON_COUNT (or an
// unready atlas) to fall through to a plain text button. The tint defaults to
// the role color unless the caller sets .texture_tint.
static int dashboard_icon_button_helper(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                        const char *text, WLX_Icon icon,
                                        Dashboard_Icon_Role role, WLX_Button_Opt opt,
                                        const char *file, int line) {
    if (icon != WLX_ICON_COUNT && dashboard_icon_atlas_ready()) {
        float target = dashboard_icon_chrome_size(opt.image_size, opt.height);
        opt.texture = dashboard_icon_atlas_texture();
        opt.texture_src = dashboard_icon_src_for(icon, target);
        if (opt.texture_tint.a == 0) opt.texture_tint = dashboard_icon_tint(tk, role);
        if (opt.image_size <= 0.0f) opt.image_size = target;
        if (opt.image_text_gap < 0) opt.image_text_gap = 8;
    }
    return wlx_button_impl(ctx, text, opt, file, line);
}

#define dashboard_icon_button(ctx, tk, text, icon, role, ...) \
    dashboard_icon_button_helper((ctx), (tk), (text), (icon), (role), \
        wlx_default_button_opt(__VA_ARGS__), __FILE__, __LINE__)

// Icon+text label: same texture injection as the button helper, for static
// rows (footer links, etc.). Falls through to a plain text label when the icon
// is WLX_ICON_COUNT or the atlas is not ready.
static void dashboard_icon_label_helper(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                        const char *text, WLX_Icon icon,
                                        Dashboard_Icon_Role role, WLX_Label_Opt opt,
                                        const char *file, int line) {
    if (icon != WLX_ICON_COUNT && dashboard_icon_atlas_ready()) {
        float target = dashboard_icon_chrome_size(opt.image_size, opt.height);
        opt.texture = dashboard_icon_atlas_texture();
        opt.texture_src = dashboard_icon_src_for(icon, target);
        if (opt.texture_tint.a == 0) opt.texture_tint = dashboard_icon_tint(tk, role);
        if (opt.image_size <= 0.0f) opt.image_size = target;
        if (opt.image_text_gap < 0) opt.image_text_gap = 8;
    }
    wlx_label_impl(ctx, text, opt, file, line);
}

#define dashboard_icon_label(ctx, tk, text, icon, role, ...) \
    dashboard_icon_label_helper((ctx), (tk), (text), (icon), (role), \
        wlx_default_label_opt(__VA_ARGS__), __FILE__, __LINE__)

// Inputbox with an inner leading/trailing icon: inject the atlas texture fields
// into the inputbox options so the glyph renders inside the field frame instead
// of as a detached sibling widget. The tint defaults to the role color unless
// the caller sets .texture_tint. Falls through to a plain inputbox when the icon
// is WLX_ICON_COUNT or the atlas is not ready.
static bool dashboard_icon_inputbox_helper(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                           WLX_Icon icon, Dashboard_Icon_Role role,
                                           char *buffer, size_t buffer_size,
                                           WLX_Inputbox_Opt opt,
                                           const char *file, int line) {
    if (icon != WLX_ICON_COUNT && dashboard_icon_atlas_ready()) {
        float target = dashboard_icon_chrome_size(opt.image_size, opt.height);
        opt.texture = dashboard_icon_atlas_texture();
        opt.texture_src = dashboard_icon_src_for(icon, target);
        if (opt.texture_tint.a == 0) opt.texture_tint = dashboard_icon_tint(tk, role);
        if (opt.image_size <= 0.0f) opt.image_size = target;
        if (opt.image_text_gap < 0) opt.image_text_gap = 8;
    }
    return wlx_inputbox_impl(ctx, "", buffer, buffer_size, opt, file, line);
}

#define dashboard_icon_inputbox(ctx, tk, icon, role, buffer, buffer_size, ...) \
    dashboard_icon_inputbox_helper((ctx), (tk), (icon), (role), (buffer), (buffer_size), \
        wlx_default_inputbox_opt(__VA_ARGS__), __FILE__, __LINE__)

#endif // WLX_DASHBOARD_ICONS_H
