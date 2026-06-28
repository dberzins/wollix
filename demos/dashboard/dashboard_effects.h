// dashboard_effects.h - Dashboard demo local visual-effect approximations.
//
// Included only by dashboard.c (before dashboard_components.h), following the
// demo-local header convention. These reach the Stitch "Mechanical Glass" look.
// Glow, shadow, frost, and the vertical gradient are now expressed through
// first-class decor (the core's glow/shadow command fields, container
// back_color tints, and the `.gradient_top` / `.gradient_bottom` fields), so
// their standalone draw helpers are gone. The one remaining primitive-only
// effect is the scanline overlay - no repeating-line/pattern primitive exists
// in the core. Pure color helpers sit at the top so they can be unit tested and
// are still consumed by the effect tests.
//
// Depends only on wollix.h (WLX_Color, the wlx_cmd_record_* emitters, and the
// wlx_color_* helpers). No core or backend header is modified.

#ifndef WLX_DASHBOARD_EFFECTS_H
#define WLX_DASHBOARD_EFFECTS_H

// ============================================================================
// Pure color helpers (unit testable)
// ============================================================================

static inline uint8_t dashboard_lerp_u8(uint8_t a, uint8_t b, float t) {
    float v = (float)a + ((float)b - (float)a) * t;
    if (v < 0.0f) v = 0.0f;
    if (v > 255.0f) v = 255.0f;
    return (uint8_t)(v + 0.5f);
}

// Linear interpolate two colors (all four channels). t clamps to [0,1].
static inline WLX_Color dashboard_color_lerp(WLX_Color a, WLX_Color b, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    WLX_Color r = {
        dashboard_lerp_u8(a.r, b.r, t),
        dashboard_lerp_u8(a.g, b.g, t),
        dashboard_lerp_u8(a.b, b.b, t),
        dashboard_lerp_u8(a.a, b.a, t),
    };
    return r;
}

// Pull a color toward its luma grey by `amount` in [0,1]; alpha is unchanged.
static inline WLX_Color dashboard_desaturate(WLX_Color c, float amount) {
    if (amount < 0.0f) amount = 0.0f;
    if (amount > 1.0f) amount = 1.0f;
    float luma = 0.299f * (float)c.r + 0.587f * (float)c.g + 0.114f * (float)c.b;
    uint8_t grey = (uint8_t)(luma + 0.5f);
    WLX_Color r = {
        dashboard_lerp_u8(c.r, grey, amount),
        dashboard_lerp_u8(c.g, grey, amount),
        dashboard_lerp_u8(c.b, grey, amount),
        c.a,
    };
    return r;
}

// Frosted-glass tint standing in for a blurred backdrop: desaturate, darken,
// then set an explicit (translucent) alpha.
static inline WLX_Color dashboard_blur_tint(WLX_Color base, float darken,
                                            float desat, uint8_t alpha) {
    WLX_Color c = dashboard_desaturate(base, desat);
    c = wlx_color_brightness(c, -darken);
    c.a = alpha;
    return c;
}

// Alpha for glow ring `i` of `n` (i = 0 innermost/strongest, n-1 outermost/
// faintest), scaling `base`. Result in [0,255].
static inline uint8_t dashboard_glow_ring_alpha(int i, int n, uint8_t base) {
    if (n <= 0) return 0;
    if (i < 0) i = 0;
    if (i >= n) i = n - 1;
    float f = (float)(n - i) / (float)n;
    float a = (float)base * f;
    if (a < 0.0f) a = 0.0f;
    if (a > 255.0f) a = 255.0f;
    return (uint8_t)(a + 0.5f);
}

// ============================================================================
// Primitive-only effect draws
// ============================================================================

// Scanline overlay: 1px horizontal lines every `spacing` px.
static inline void dashboard_draw_scanlines(WLX_Context *ctx, WLX_Rect r,
                                            WLX_Color color, float spacing) {
    if (spacing < 1.0f) spacing = 1.0f;
    for (float y = r.y + 0.5f; y < r.y + r.h; y += spacing) {
        wlx_cmd_record_line(ctx, r.x, y, r.x + r.w, y, 1.0f, color);
    }
}

#endif // WLX_DASHBOARD_EFFECTS_H
