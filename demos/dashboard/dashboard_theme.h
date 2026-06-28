// dashboard_theme.h - Dashboard demo local design-token model and theme surface.
//
// Included only by dashboard.c, following the gallery_perf.h local-header
// convention: demo-local helper code that never becomes a shared cross-demo
// translation unit. This header owns the dashboard's Stitch-derived token model
// for dark and light modes - the color roles, surface-container ramp, on-color
// roles, status and table colors, glass-edge and glow colors, typography roles,
// spacing and radius scales, and per-effect intent. It is pure data plus pure
// helpers: no rendering, and no dependency beyond WLX_Color and WLX_Font. The
// per-frame WLX_Theme mapper and the component recipes land here later.
//
// Color and metric values are transcribed by hand from the Stitch design
// exports and cross-checked against the reference renders. Roles the exports
// leave unspecified for a mode are derived to stay legible on that mode's base
// surface; those derivations are re-tunable without changing the token shape.
// No core (wollix.h) or backend header is modified.

#ifndef WLX_DASHBOARD_THEME_H
#define WLX_DASHBOARD_THEME_H

// ============================================================================
// Enums
// ============================================================================

// Stitch ships a dark and a light dashboard mode. Dark is the primary target.
typedef enum {
    DASHBOARD_MODE_DARK = 0,
    DASHBOARD_MODE_LIGHT = 1,
} Dashboard_Mode;

// Font families in the dashboard set. SANS is Geist; MONO is JetBrains Mono.
typedef enum {
    DASHBOARD_FAMILY_SANS = 0,
    DASHBOARD_FAMILY_MONO = 1,
} Dashboard_Font_Family;

// Weight intent. Values match common CSS numeric weights; each maps to one of
// the vendored faces at resolve time.
typedef enum {
    DASHBOARD_WEIGHT_REGULAR  = 400,
    DASHBOARD_WEIGHT_MEDIUM   = 500,
    DASHBOARD_WEIGHT_SEMIBOLD = 600,
    DASHBOARD_WEIGHT_BOLD     = 700,
} Dashboard_Font_Weight;

// How an effect is realized. APPROXIMATED uses existing primitives; CORE_REQUIRED
// marks an effect that needs an escalated core change before it can ship.
typedef enum {
    DASHBOARD_EFFECT_NONE = 0,
    DASHBOARD_EFFECT_APPROXIMATED = 1,
    DASHBOARD_EFFECT_CORE_REQUIRED = 2,
} Dashboard_Effect_Intent;

// ============================================================================
// Leaf token structs
// ============================================================================

// One typography role expressed as intent: family + size + weight, plus
// line-height and integer tracking (letter spacing in pixels). The live font
// handle is resolved at runtime from family + weight; this struct stays pure.
typedef struct {
    Dashboard_Font_Family family;
    int                   size;         // px
    Dashboard_Font_Weight weight;       // weight intent
    int                   line_height;  // px
    int                   tracking;     // integer letter spacing in px (may be < 0)
} Dashboard_Type_Role;

// Canonical role set. Both modes populate every role; a mode whose export lacks
// a role fills it from the nearest export value or a faithful derivation.
typedef struct {
    Dashboard_Type_Role display;
    Dashboard_Type_Role headline_lg;
    Dashboard_Type_Role headline_md;
    Dashboard_Type_Role headline_sm;
    Dashboard_Type_Role body_lg;
    Dashboard_Type_Role body_md;
    Dashboard_Type_Role body_sm;
    Dashboard_Type_Role label;  // small caps/label face
    Dashboard_Type_Role mono;   // monospace data face
} Dashboard_Type_Scale;

// Surface-container elevation ramp, ordered lowest to highest. Luminance is
// strictly monotonic across the ramp: increasing in dark mode, decreasing in
// light mode.
typedef struct {
    WLX_Color lowest;
    WLX_Color low;
    WLX_Color base;
    WLX_Color high;
    WLX_Color highest;
} Dashboard_Surface_Ramp;

// Core color roles, including on-color roles and the input-field roles used by
// the baseline WLX_Theme mapper.
typedef struct {
    WLX_Color background;        // window base surface
    WLX_Color surface;           // default module/panel surface
    WLX_Color surface_variant;   // alternate surface
    WLX_Color field;             // input/control background
    WLX_Color field_border;      // resting input/control border
    WLX_Color outline;           // default border
    WLX_Color outline_variant;   // subtle divider border
    WLX_Color accent;            // neon primary accent
    WLX_Color accent_strong;     // emphasized accent (hover/active, glow source)
    WLX_Color tertiary;          // tertiary highlight (industrial orange callouts)
    WLX_Color on_surface;        // primary text/icon on surfaces
    WLX_Color on_surface_muted;  // secondary/muted text
    WLX_Color on_accent;         // text/icon on the accent fill
} Dashboard_Color_Tokens;

// Semantic status colors.
typedef struct {
    WLX_Color success;
    WLX_Color warning;
    WLX_Color error;
    WLX_Color info;
} Dashboard_Status_Tokens;

// Data-table colors.
typedef struct {
    WLX_Color header_bg;  // header row background
    WLX_Color row_bg;     // base data row background
    WLX_Color zebra_bg;   // alternating data row background
    WLX_Color grid_line;  // 1px row/column separators (translucent)
} Dashboard_Table_Tokens;

// Glass module edge and glow colors. Edges are paired light/dark strokes; the
// fill and glow are translucent.
typedef struct {
    WLX_Color edge_light;  // top-left light edge
    WLX_Color edge_dark;   // bottom-right dark edge
    WLX_Color fill;        // translucent module fill
    WLX_Color glow;        // accent glow (low alpha)
} Dashboard_Glass_Tokens;

// Spacing scale on a strict 4px base. All values are whole multiples of `unit`.
typedef struct {
    int unit;           // 4
    int xs;             // 4
    int sm;             // 8
    int md;             // 12
    int lg;             // 16
    int xl;             // 24
    int xxl;            // 32
    int gutter;         // module gutter
    int margin;         // outer page margin
    int panel_padding;  // inner module padding
    int container_max;  // max content width
} Dashboard_Spacing;

// Corner radius scale, ascending; `full` is the pill/circle sentinel.
typedef struct {
    int sm;
    int base;
    int md;
    int lg;
    int xl;
    int full;
} Dashboard_Radius;

// Per-effect intent for the dashboard's visual treatments.
typedef struct {
    Dashboard_Effect_Intent backdrop_blur;
    Dashboard_Effect_Intent glow;
    Dashboard_Effect_Intent shadow;
    Dashboard_Effect_Intent glass_edge;
    Dashboard_Effect_Intent scanline;
    Dashboard_Effect_Intent gradient;
} Dashboard_Effects;

// ============================================================================
// Top-level token bundle
// ============================================================================

typedef struct {
    Dashboard_Mode          mode;
    Dashboard_Color_Tokens  color;
    Dashboard_Surface_Ramp  ramp;
    Dashboard_Status_Tokens status;
    Dashboard_Table_Tokens  table;
    Dashboard_Glass_Tokens  glass;
    Dashboard_Type_Scale    type;
    Dashboard_Spacing       spacing;
    Dashboard_Radius        radius;
    Dashboard_Effects       effects;
} Dashboard_Tokens;

// ============================================================================
// Dark mode tokens (Stitch "Mechanical Glass Dark")
// ============================================================================
//
// Transcribed from docs/dev/dashboard/dark/DESIGN.md. The Material surface and
// on-color roles come straight from the export frontmatter (background #0f1419
// over a #0a0f14 void); the bright electric blue (#00d1ff) is the brand accent,
// and the deep dark ink (#0a0f14) is the on-accent text per the narrative. Status
// hues are the narrative support colors (clinical mint / industrial orange /
// signal red). Glass edges follow the elevation rule: a 0.15 white light-leak on
// top/left, a darker fall on bottom/right, over a 60-80% dark glass fill.

static const Dashboard_Tokens dashboard_tokens_dark = {
    .mode = DASHBOARD_MODE_DARK,
    .color = {
        .background       = WLX_RGBA( 15,  20,  25, 255),  // background #0f1419
        .surface          = WLX_RGBA( 27,  32,  37, 255),  // surface-container #1b2025
        .surface_variant  = WLX_RGBA( 48,  53,  59, 255),  // surface-variant #30353b
        .field            = WLX_RGBA(  5,   7,  10, 255),  // inset field, darker than the void
        .field_border     = WLX_RGBA( 26,  36,  47, 255),  // #1a242f input border
        .outline          = WLX_RGBA(133, 147, 153, 255),  // outline #859399
        .outline_variant  = WLX_RGBA( 60,  73,  78, 255),  // outline-variant #3c494e
        .accent           = WLX_RGBA(  0, 209, 255, 255),  // primary-container (electric blue)
        .accent_strong    = WLX_RGBA( 76, 214, 255, 255),  // surface-tint #4cd6ff (hover/glow)
        .tertiary         = WLX_RGBA(235, 129,   4, 255),  // #eb8104 industrial orange
        .on_surface       = WLX_RGBA(222, 227, 234, 255),  // on-surface #dee3ea
        .on_surface_muted = WLX_RGBA(187, 201, 207, 255),  // on-surface-variant #bbc9cf
        .on_accent        = WLX_RGBA( 10,  15,  20, 255),  // dark ink #0a0f14 on the accent fill
    },
    .ramp = {
        .lowest  = WLX_RGBA( 10,  15,  20, 255),  // surface-container-lowest #0a0f14
        .low     = WLX_RGBA( 23,  28,  33, 255),  // surface-container-low #171c21
        .base    = WLX_RGBA( 27,  32,  37, 255),  // surface-container #1b2025
        .high    = WLX_RGBA( 37,  42,  48, 255),  // surface-container-high #252a30
        .highest = WLX_RGBA( 48,  53,  59, 255),  // surface-container-highest #30353b
    },
    .status = {
        .success = WLX_RGBA(  0, 179, 110, 255),  // #00b36e clinical mint
        .warning = WLX_RGBA(235, 129,   4, 255),  // #eb8104 industrial orange
        .error   = WLX_RGBA(207,  44,  44, 255),  // #cf2c2c signal red
        .info    = WLX_RGBA(  0, 209, 255, 255),  // primary accent (electric blue)
    },
    .table = {
        .header_bg = WLX_RGBA( 37,  42,  48, 255),  // surface-container-high
        .row_bg    = WLX_RGBA( 27,  32,  37, 255),  // surface-container
        .zebra_bg  = WLX_RGBA( 23,  28,  33, 255),  // surface-container-low
        .grid_line = WLX_RGBA(255, 255, 255,  38),  // 1px row/column rules at 15% white
    },
    .glass = {
        .edge_light = WLX_RGBA(255, 255, 255,  38),  // top/left light leak (0.15)
        .edge_dark  = WLX_RGBA(  0,   0,   0,  51),   // bottom/right falls to dark
        .fill       = WLX_RGBA( 26,  36,  47, 179),   // #1a242f dark glass at ~70%
        .glow       = WLX_RGBA(  0, 209, 255,  13),    // electric-blue focus glow (very low)
    },
    .type = {
        .display     = { DASHBOARD_FAMILY_SANS, 48, DASHBOARD_WEIGHT_BOLD,     56, -1 },
        .headline_lg = { DASHBOARD_FAMILY_SANS, 32, DASHBOARD_WEIGHT_SEMIBOLD, 40,  0 },
        .headline_md = { DASHBOARD_FAMILY_SANS, 20, DASHBOARD_WEIGHT_SEMIBOLD, 28,  0 },
        .headline_sm = { DASHBOARD_FAMILY_SANS, 20, DASHBOARD_WEIGHT_SEMIBOLD, 28,  0 },
        .body_lg     = { DASHBOARD_FAMILY_SANS, 16, DASHBOARD_WEIGHT_REGULAR,  24,  0 },
        .body_md     = { DASHBOARD_FAMILY_SANS, 14, DASHBOARD_WEIGHT_REGULAR,  20,  0 },
        .body_sm     = { DASHBOARD_FAMILY_SANS, 12, DASHBOARD_WEIGHT_REGULAR,  16,  0 },
        .label       = { DASHBOARD_FAMILY_MONO, 12, DASHBOARD_WEIGHT_MEDIUM,   16,  1 },
        .mono        = { DASHBOARD_FAMILY_MONO, 13, DASHBOARD_WEIGHT_MEDIUM,   16,  0 },
    },
    .spacing = {
        .unit = 4, .xs = 4, .sm = 8, .md = 12, .lg = 16, .xl = 24, .xxl = 32,
        .gutter = 16, .margin = 32, .panel_padding = 12, .container_max = 1440,
    },
    .radius = { .sm = 2, .base = 4, .md = 6, .lg = 8, .xl = 12, .full = 9999 },
    .effects = {
        .backdrop_blur = DASHBOARD_EFFECT_CORE_REQUIRED,
        .glow          = DASHBOARD_EFFECT_APPROXIMATED,
        .shadow        = DASHBOARD_EFFECT_APPROXIMATED,
        .glass_edge    = DASHBOARD_EFFECT_APPROXIMATED,
        .scanline      = DASHBOARD_EFFECT_APPROXIMATED,
        .gradient      = DASHBOARD_EFFECT_APPROXIMATED,
    },
};

// ============================================================================
// Light mode tokens (Stitch "Mechanical Glass Light")
// ============================================================================
//
// Transcribed from docs/dev/dashboard/light/DESIGN.md. The Material surface and
// on-color roles come straight from the export frontmatter; the accent system
// follows the narrative: the bright electric blue (#00d1ff, primary-container)
// is the brand accent for fills, washes, and indicators, while the deep primary
// (#00677f) carries accent text/values that must stay legible on light surfaces.
// Status hues are the narrative support colors (clinical mint / industrial
// orange / signal red). Glass edges follow the elevation rule (top/left catch
// light at 0.8, bottom/right fall to 0.05).

static const Dashboard_Tokens dashboard_tokens_light = {
    .mode = DASHBOARD_MODE_LIGHT,
    .color = {
        .background       = WLX_RGBA(247, 249, 255, 255),  // background / surface
        .surface          = WLX_RGBA(234, 238, 246, 255),  // surface-container
        .surface_variant  = WLX_RGBA(222, 227, 234, 255),  // surface-variant
        .field            = WLX_RGBA(255, 255, 255, 255),  // surface-container-lowest
        .field_border     = WLX_RGBA(187, 201, 207, 255),  // outline-variant
        .outline          = WLX_RGBA(108, 121, 127, 255),  // outline
        .outline_variant  = WLX_RGBA(187, 201, 207, 255),  // outline-variant
        .accent           = WLX_RGBA(  0, 209, 255, 255),  // primary-container (electric blue)
        .accent_strong    = WLX_RGBA(  0, 103, 127, 255),  // primary (deep, legible on light)
        .tertiary         = WLX_RGBA(235, 129,   4, 255),  // #eb8104 industrial orange
        .on_surface       = WLX_RGBA( 23,  28,  33, 255),  // on-surface
        .on_surface_muted = WLX_RGBA( 60,  73,  78, 255),  // on-surface-variant
        .on_accent        = WLX_RGBA(  0,  31,  40, 255),  // dark ink on the accent fill
    },
    .ramp = {
        .lowest  = WLX_RGBA(255, 255, 255, 255),  // surface-container-lowest
        .low     = WLX_RGBA(240, 244, 251, 255),  // surface-container-low
        .base    = WLX_RGBA(234, 238, 246, 255),  // surface-container
        .high    = WLX_RGBA(228, 232, 240, 255),  // surface-container-high
        .highest = WLX_RGBA(222, 227, 234, 255),  // surface-container-highest
    },
    .status = {
        .success = WLX_RGBA(  0, 179, 110, 255),  // #00b36e clinical mint
        .warning = WLX_RGBA(235, 129,   4, 255),  // #eb8104 industrial orange
        .error   = WLX_RGBA(211,  47,  47, 255),  // #d32f2f signal red
        .info    = WLX_RGBA(  0, 103, 127, 255),  // primary (deep)
    },
    .table = {
        .header_bg = WLX_RGBA(228, 232, 240, 255),  // surface-container-high
        .row_bg    = WLX_RGBA(255, 255, 255, 255),  // surface-container-lowest
        .zebra_bg  = WLX_RGBA(240, 244, 251, 255),  // surface-container-low
        .grid_line = WLX_RGBA(  0,   0,   0,  26),   // 1px row/column rules at 10% black
    },
    .glass = {
        .edge_light = WLX_RGBA(255, 255, 255, 204),  // top/left edge catches light (0.8)
        .edge_dark  = WLX_RGBA(  0,   0,   0,  13),   // bottom/right edge falls to dark (0.05)
        .fill       = WLX_RGBA(255, 255, 255, 166),   // semi-transparent white glass
        .glow       = WLX_RGBA(  0, 209, 255,  38),   // electric-blue accent glow
    },
    .type = {
        .display     = { DASHBOARD_FAMILY_SANS, 48, DASHBOARD_WEIGHT_BOLD,     56, -1 },
        .headline_lg = { DASHBOARD_FAMILY_SANS, 32, DASHBOARD_WEIGHT_SEMIBOLD, 40,  0 },
        .headline_md = { DASHBOARD_FAMILY_SANS, 20, DASHBOARD_WEIGHT_SEMIBOLD, 28,  0 },
        .headline_sm = { DASHBOARD_FAMILY_SANS, 18, DASHBOARD_WEIGHT_SEMIBOLD, 24,  0 },
        .body_lg     = { DASHBOARD_FAMILY_SANS, 16, DASHBOARD_WEIGHT_REGULAR,  24,  0 },
        .body_md     = { DASHBOARD_FAMILY_SANS, 14, DASHBOARD_WEIGHT_REGULAR,  20,  0 },
        .body_sm     = { DASHBOARD_FAMILY_SANS, 12, DASHBOARD_WEIGHT_REGULAR,  16,  0 },
        .label       = { DASHBOARD_FAMILY_MONO, 12, DASHBOARD_WEIGHT_MEDIUM,   16,  1 },
        .mono        = { DASHBOARD_FAMILY_MONO, 13, DASHBOARD_WEIGHT_MEDIUM,   16,  0 },
    },
    .spacing = {
        .unit = 4, .xs = 4, .sm = 8, .md = 12, .lg = 16, .xl = 24, .xxl = 32,
        .gutter = 16, .margin = 32, .panel_padding = 12, .container_max = 1440,
    },
    .radius = { .sm = 2, .base = 4, .md = 6, .lg = 8, .xl = 12, .full = 9999 },
    .effects = {
        .backdrop_blur = DASHBOARD_EFFECT_CORE_REQUIRED,
        .glow          = DASHBOARD_EFFECT_APPROXIMATED,
        .shadow        = DASHBOARD_EFFECT_APPROXIMATED,
        .glass_edge    = DASHBOARD_EFFECT_APPROXIMATED,
        .scanline      = DASHBOARD_EFFECT_APPROXIMATED,
        .gradient      = DASHBOARD_EFFECT_APPROXIMATED,
    },
};

// ============================================================================
// Pure selectors and font resolution
// ============================================================================

// Active token bundle for a mode. References both instances so neither is
// dropped as unused in any translation unit that includes this header.
static inline const Dashboard_Tokens *dashboard_tokens(Dashboard_Mode mode) {
    return (mode == DASHBOARD_MODE_LIGHT) ? &dashboard_tokens_light
                                          : &dashboard_tokens_dark;
}

// Base window/background surface for a mode.
static inline WLX_Color dashboard_background(Dashboard_Mode mode) {
    return dashboard_tokens(mode)->color.background;
}

// Runtime-loaded font handles for the dashboard set. Filled by the demo at
// startup; the token model only ever names a (family, weight) intent.
typedef struct {
    WLX_Font sans_regular;
    WLX_Font sans_medium;
    WLX_Font sans_semibold;
    WLX_Font sans_bold;
    WLX_Font mono_regular;
    WLX_Font mono_medium;
    WLX_Font mono_bold;
} Dashboard_Fonts;

#ifndef DASHBOARD_TEXT_SCALE
#define DASHBOARD_TEXT_SCALE 1.14f
#endif

static inline int dashboard_type_px(Dashboard_Type_Role role) {
    int px = (int)((float)role.size * DASHBOARD_TEXT_SCALE + 0.5f);
    return px > 0 ? px : 1;
}

// Rendered line-height px for a role (font size plus leading). Use this for the
// height of a slot that holds one text line so ascenders/descenders are not
// clipped, while the text itself is still drawn at dashboard_type_px(role).
static inline int dashboard_type_line_px(Dashboard_Type_Role role) {
    int px = (int)((float)role.line_height * DASHBOARD_TEXT_SCALE + 0.5f);
    return px > 0 ? px : 1;
}

// Resolve a (family, weight) intent to a loaded handle. Unknown weights fall
// back to the family's regular face.
static inline WLX_Font dashboard_font_resolve(const Dashboard_Fonts *fonts,
                                              Dashboard_Font_Family family,
                                              Dashboard_Font_Weight weight) {
    if (!fonts) return WLX_FONT_DEFAULT;
    if (family == DASHBOARD_FAMILY_MONO) {
        switch (weight) {
        case DASHBOARD_WEIGHT_BOLD:     return fonts->mono_bold;
        case DASHBOARD_WEIGHT_MEDIUM:   return fonts->mono_medium;
        default:                        return fonts->mono_regular;
        }
    }
    switch (weight) {
    case DASHBOARD_WEIGHT_BOLD:     return fonts->sans_bold;
    case DASHBOARD_WEIGHT_SEMIBOLD: return fonts->sans_semibold;
    case DASHBOARD_WEIGHT_MEDIUM:   return fonts->sans_medium;
    default:                        return fonts->sans_regular;
    }
}

// Resolve a typography role to its loaded handle.
static inline WLX_Font dashboard_type_font(const Dashboard_Fonts *fonts,
                                           Dashboard_Type_Role role) {
    return dashboard_font_resolve(fonts, role.family, role.weight);
}

// ============================================================================
// WLX_Theme mapper
// ============================================================================

// Build a WLX_Theme for `mode` from the dashboard tokens, for ordinary widgets.
// `fonts` may be NULL, in which case text resolves to the backend default font.
// Designated initializers leave `opacity` at its negative unset sentinel so the
// global default (fully opaque) applies; the listed widget overrides are filled
// from token roles. The body font and base size come from the body-md role; the
// accent drives every active/focused affordance.
static inline WLX_Theme dashboard_wlx_theme(Dashboard_Mode mode,
                                            const Dashboard_Fonts *fonts) {
    const Dashboard_Tokens *t = dashboard_tokens(mode);
    int dark = (mode != DASHBOARD_MODE_LIGHT);
    WLX_Theme theme = {
        .background = t->color.background,
        .foreground = t->color.on_surface,
        .surface    = t->color.surface,
        .border     = t->color.outline_variant,
        .border_width = 1.0f,
        .accent     = t->color.accent,

        .font      = dashboard_font_resolve(fonts, DASHBOARD_FAMILY_SANS,
                                            DASHBOARD_WEIGHT_REGULAR),
        .font_size = t->type.body_md.size,

        .padding   = (float)t->spacing.sm,
        .roundness = 0.15f,  // approximates the 4px base radius on typical controls
        .rounded_segments = 0,
        .min_rounded_segments = 16,

        .hover_brightness    = dark ?  0.08f : -0.08f,
        .disabled_brightness = dark ? -0.35f :  0.30f,
        .opacity             = -1.0f,
        .disabled_opacity    = WLX_DEFAULT_DISABLED_OPACITY,

        .input = {
            .border_focus = t->color.accent,
            .cursor       = t->color.on_surface,
            .border_width = 1.0f,
        },
        .slider = {
            .track        = t->color.surface_variant,
            .thumb        = t->color.accent,
            .label        = t->color.on_surface,
            .track_height = 6.0f,
            .thumb_width  = 14.0f,
        },
        .checkbox = {
            .check        = t->color.accent,
            .border       = t->color.outline_variant,
            .border_width = 1.0f,
        },
        .toggle = {
            .track        = t->ramp.low,
            .track_active = t->color.accent,
            .thumb        = t->color.on_surface,
            .track_height = 0,
        },
        .radio = {
            .ring         = t->color.outline_variant,
            .fill         = t->color.accent,
            .label        = t->color.on_surface,
            .border_width = 1.5f,
        },
        .progress = {
            .track        = t->color.surface_variant,
            .fill         = t->color.accent,
            .track_height = 6.0f,
        },
        .scrollbar = {
            .bar   = t->color.outline_variant,
            .width = 10.0f,
        },
    };
    return theme;
}

#endif // WLX_DASHBOARD_THEME_H
