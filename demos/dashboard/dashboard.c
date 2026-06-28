// dashboard.c - Wollix dashboard showcase demo (Stitch "Mechanical Glass").
//
// One source compiled three ways via WLX_DASHBOARD_RAYLIB / _SDL3 / _WASM. The
// dashboard owns a demo-local token model and component recipes (in
// dashboard_theme.h and the local headers); it maps a baseline subset of tokens
// into WLX_Theme each frame for ordinary widgets, and provides dashboard-specific
// component recipes (glass modules, technical/glass buttons, technical input,
// status chips/badges/pips, a dense data table, a segmented progress bar,
// navigation chrome, and a responsive column grid). A shared top bar + sidebar
// wrap a scrollable main area that routes between distinct section views
// (Overview, Tokens, Components, Layouts, Theme Lab) through a section dispatch
// table; the sidebar selects the route and the selection persists across frames.
// Each section is a self-contained showcase: Tokens renders the color/type/
// spacing/radius token model, Components exercises the public widget set (label,
// button, checkbox, toggle, radio, slider, progress, input, image, widget),
// Layouts demonstrates the structural capabilities (linear, grid, flex, auto
// layout, opacity, ID stack, borders, scroll panel), and Theme Lab shows the
// component-state matrix, built-in theme presets, and the live theme geometry.
// The top-bar palette icon toggles dark/light. The Overview's hero cards show
// live, library-honest metrics (backend frame rate, and -- in a WLX_PERF build --
// draw-call count and wollix arena memory, plus desktop process RSS where
// measurable); the Integrations rows and sidebar Documentation/Support links open
// real repository/doc URLs through a per-backend open-URL hook.
//
// The backend-neutral body (token model, component recipes, section bodies, the
// shell) is shared; the per-backend window/context/font/atlas lifecycle and the
// entry point live in the platform block at the end of the file.
// Build: make dashboard (raylib), make dashboard_sdl3 (SDL3),
// make dashboard-wasm-site (bare WASM).

// ---------------------------------------------------------------------------
// Backend selection
// ---------------------------------------------------------------------------
#if !defined(WLX_DASHBOARD_RAYLIB) && !defined(WLX_DASHBOARD_SDL3) && !defined(WLX_DASHBOARD_WASM)
#define WLX_DASHBOARD_RAYLIB
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Platform block -- backend headers and wollix implementation. All three
// branches (raylib, SDL3, bare WASM) are populated; the responsive topbar relies
// on the min/max slot redistribution that is the wollix default.
// ---------------------------------------------------------------------------
#if defined(WLX_DASHBOARD_RAYLIB)
    #include <raylib.h>
#elif defined(WLX_DASHBOARD_SDL3)
    #include <SDL3/SDL.h>
    #include <SDL3_ttf/SDL_ttf.h>
#elif defined(WLX_DASHBOARD_WASM)
    // Bare-wasm32: no libc beyond the shim; headers come from web/libc/
#endif

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"

#if defined(WLX_DASHBOARD_RAYLIB)
    #include "wollix_raylib.h"
#elif defined(WLX_DASHBOARD_SDL3)
    #include "wollix_sdl3.h"
#elif defined(WLX_DASHBOARD_WASM)
    #include "wollix_wasm.h"
#endif

#include "dashboard_theme.h"
#include "dashboard_effects.h"
#include "dashboard_components.h"
#include "dashboard_icons.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 800
#define TARGET_FPS 60
// Frames to render before an optional WLX_DASHBOARD_SHOT capture, so caches warm
// and the live FPS reading settles to steady state before the frame is grabbed.
#define DASHBOARD_SHOT_FRAME 60

// Geist is the Stitch primary (sans) family; JetBrains Mono is the metadata
// family. Both ship under demos/assets/. The whole weight set is loaded once on
// desktop so any typography role can resolve to a real handle; faces are baked at
// a base size and scaled per role by the backend. Bare-WASM has no font files and
// resolves every face to the backend default.
// Bake the glyph atlas large enough that every rendered size downscales. The
// largest dashboard text is the hero stat value (display 36px) which the raylib
// path further scales by DASHBOARD_RAYLIB_FONT_SCALE (~47px effective); a 32px
// atlas would upscale it and the bilinear filter would only blur the result.
#define DASHBOARD_FONT_BASE_SIZE 64
#define DASHBOARD_FONT_FACE_COUNT 7

// Desktop font file set, in Dashboard_Fonts field order (sans regular/medium/
// semibold/bold, then mono regular/medium/bold). Shared by the raylib and SDL3
// loaders in the platform block.
#if defined(WLX_DASHBOARD_RAYLIB) || defined(WLX_DASHBOARD_SDL3)
static const char *dashboard_font_paths[DASHBOARD_FONT_FACE_COUNT] = {
    "demos/assets/Geist-Regular.ttf",
    "demos/assets/Geist-Medium.ttf",
    "demos/assets/Geist-SemiBold.ttf",
    "demos/assets/Geist-Bold.ttf",
    "demos/assets/JetBrainsMono-Regular.ttf",
    "demos/assets/JetBrainsMono-Medium.ttf",
    "demos/assets/JetBrainsMono-Bold.ttf",
};
#endif

// Sidebar routes. The order is the sidebar's top-to-bottom order and indexes the
// section dispatch table below; DASHBOARD_VIEW_COUNT is the route count.
typedef enum {
    DASHBOARD_VIEW_OVERVIEW = 0,
    DASHBOARD_VIEW_TOKENS,
    DASHBOARD_VIEW_COMPONENTS,
    DASHBOARD_VIEW_LAYOUTS,
    DASHBOARD_VIEW_THEME_LAB,
    DASHBOARD_VIEW_COUNT,
} Dashboard_View;

// Persistent widget + route state for the showcase views. current_view is the
// active sidebar route and persists across frames; the remaining fields hold the
// per-widget interactive state the section bodies read and write, grouped by the
// view that owns them so each route's controls keep their value across frames.
#define DASHBOARD_IDSTACK_ROWS 4
typedef struct {
    Dashboard_View current_view;  // active sidebar route (persists across frames)
    char  search[64];             // top-bar search field (filters the Overview lists)
    bool  theme_toggle;           // set by the top-bar switch; main flips dark/light

    // Overview view
    int   overview_tab;           // center-panel tab: 0 = Active State, 1 = Preview
    bool  overview_check;         // Active State live checkbox
    bool  overview_toggle;        // Active State live toggle
    float overview_slider;        // Active State live slider
    int   overview_clicks;        // Active State live button counter

    // Components view
    bool  comp_checks[3];         // checkbox group
    bool  comp_toggle_a;          // toggle switches
    bool  comp_toggle_b;
    int   comp_radio;             // radio selection
    float comp_slider;            // default slider value
    float comp_styled_slider;     // custom track/thumb slider value
    float comp_progress;          // progress / segmented bar value
    int   comp_clicks;            // button click counter
    char  comp_name[64];          // text input
    char  comp_email[64];         // text input

    // Layouts view
    bool  idstack_enabled[DASHBOARD_IDSTACK_ROWS];  // loop-generated rows
    float idstack_values[DASHBOARD_IDSTACK_ROWS];
    char  idstack_names[DASHBOARD_IDSTACK_ROWS][48];
    float opacity_widget;         // per-widget opacity control
    float opacity_stack;          // context-stack opacity control

    // Theme Lab view
    bool  lab_checks[5];          // component-state matrix checkboxes
    float lab_sliders[5];         // component-state matrix sliders
    char  lab_inputs[5][24];      // component-state matrix inputs
} Dashboard_Demo_State;

// Library-honest Overview metrics, refreshed once per frame from the backend
// frame timer and (in a WLX_PERF build) the published perf frame. The headline
// stat cards read these instead of hardcoded numbers. perf_available is false in
// a non-perf build (draw calls / wollix memory render "n/a"); rss_available is
// false where process memory is unmeasurable (bare-WASM).
typedef struct {
    bool     perf_available;  // WLX_PERF build with a published frame
    float    fps;             // smoothed frames per second from the backend timer
    uint64_t draw_calls;      // perf total command count for the prior frame
    size_t   wlx_bytes;       // summed wollix arena capacity in bytes
    bool     rss_available;   // process RSS measurable on this platform
    double   rss_mb;          // resident set size in megabytes
} Dashboard_Metrics;

// Live Overview metrics, refreshed once per frame (see dashboard_update_metrics).
static Dashboard_Metrics g_dashboard_metrics = {0};

// Monotonic seconds accumulated from backend frame time; drives time-based decor
// and stamps the in-app action log below.
static float g_dashboard_phase = 0.0f;

// In-app action log: a small ring buffer of real dashboard events (navigation,
// theme changes, deploy actions, tab switches). The Overview terminal panel renders
// it live through a scroll panel, the Deploy button appends to it, and the top-bar
// bell lights up while entries are unread. It records only real in-app actions and
// never fabricates output. File scope (like the metrics) so any chrome can append
// without threading a handle through every section.
#define DASHBOARD_LOG_CAP 64
typedef enum { DASH_LOG_OK = 0, DASH_LOG_INFO, DASH_LOG_WARN } Dashboard_Log_Severity;
typedef struct {
    uint8_t  sev;       // Dashboard_Log_Severity
    float    t;         // g_dashboard_phase seconds at emit time
    uint64_t seq;       // monotonic id; a stable widget key as the ring wraps
    char     msg[80];
} Dashboard_Log_Entry;
typedef struct {
    Dashboard_Log_Entry entry[DASHBOARD_LOG_CAP];
    int      head;      // next write slot
    int      count;     // live entries (saturates at DASHBOARD_LOG_CAP)
    uint64_t next_seq;  // next entry id
    int      unread;    // entries since the bell was last acknowledged
} Dashboard_Log;
static Dashboard_Log g_dashboard_log = {0};

// Append one entry to the action log, overwriting the oldest once full. `msg` is
// copied, so callers may format into a local buffer and pass it.
static void dashboard_log_emit(Dashboard_Log_Severity sev, const char *msg) {
    Dashboard_Log_Entry *e = &g_dashboard_log.entry[g_dashboard_log.head];
    e->sev = (uint8_t)sev;
    e->t   = g_dashboard_phase;
    e->seq = g_dashboard_log.next_seq++;
    snprintf(e->msg, sizeof(e->msg), "%s", msg ? msg : "");
    g_dashboard_log.head = (g_dashboard_log.head + 1) % DASHBOARD_LOG_CAP;
    if (g_dashboard_log.count < DASHBOARD_LOG_CAP) g_dashboard_log.count++;
    if (g_dashboard_log.unread < 999) g_dashboard_log.unread++;
}

// Entry `i` counting back from the newest (0 = most recent), or NULL past the end.
static const Dashboard_Log_Entry *dashboard_log_at(int i) {
    if (i < 0 || i >= g_dashboard_log.count) return NULL;
    int idx = (g_dashboard_log.head - 1 - i + 2 * DASHBOARD_LOG_CAP) % DASHBOARD_LOG_CAP;
    return &g_dashboard_log.entry[idx];
}

// Seed the log once with honest startup lines so the panel is not empty on first
// paint; the backend name reflects the actual build. Seed lines start acknowledged.
static void dashboard_log_seed_once(void) {
    static bool seeded = false;
    if (seeded) return;
    seeded = true;
#if defined(WLX_DASHBOARD_RAYLIB)
    dashboard_log_emit(DASH_LOG_OK, "Backend ready: raylib");
#elif defined(WLX_DASHBOARD_SDL3)
    dashboard_log_emit(DASH_LOG_OK, "Backend ready: SDL3");
#else
    dashboard_log_emit(DASH_LOG_OK, "Backend ready: wasm");
#endif
    dashboard_log_emit(DASH_LOG_INFO, "Renderer initialized");
    dashboard_log_emit(DASH_LOG_INFO, "Dashboard ready");
    g_dashboard_log.unread = 0;
}

// Case-insensitive substring test for the Basic Widgets / Structural Layouts search
// filter. An empty needle matches everything. Implemented without strstr so it stays
// available on the bare-WASM build.
static bool dashboard_str_match(const char *needle, const char *hay) {
    if (!needle || !needle[0]) return true;
    if (!hay) return false;
    char n[64], h[96];
    size_t ni = 0, hi = 0;
    for (; needle[ni] && ni + 1 < sizeof(n); ni++) {
        char c = needle[ni];
        n[ni] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
    }
    n[ni] = '\0';
    for (; hay[hi] && hi + 1 < sizeof(h); hi++) {
        char c = hay[hi];
        h[hi] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
    }
    h[hi] = '\0';
    if (ni == 0) return true;
    for (size_t s = 0; s + ni <= hi; s++) {
        size_t k = 0;
        for (; k < ni; k++) if (h[s + k] != n[k]) break;
        if (k == ni) return true;
    }
    return false;
}

// Backend hooks implemented in the per-backend platform block at the end of the
// file but called from the shared section bodies above it: an open-URL action
// (native on desktop, a host shim on WASM) and a process-RSS reader that reports
// availability so the metric card can degrade to "n/a".
static void dashboard_open_url(const char *url);
static bool dashboard_process_rss_mb(double *out_mb);

// Real Wollix repository / documentation destinations for the reference links.
#define DASHBOARD_URL_API_REFERENCE "https://github.com/dberzins/wollix/blob/main/docs/API_REFERENCE.md"
#define DASHBOARD_URL_QUICK_START   "https://github.com/dberzins/wollix#quick-start"
#define DASHBOARD_URL_GITHUB_REPO   "https://github.com/dberzins/wollix"
#define DASHBOARD_URL_DOCS          "https://github.com/dberzins/wollix/tree/main/docs"
#define DASHBOARD_URL_SUPPORT       "https://github.com/dberzins/wollix/issues"

// ============================================================================
// Section dispatch table
// ============================================================================
// Each sidebar route maps to one section render fn. The table is the single
// source of truth for the sidebar's title + icon and for which body the main
// content area renders, mirroring the gallery's section/IA dispatch as
// dashboard-local code.
typedef void (*Dashboard_Section_Fn)(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                     const Dashboard_Fonts *fonts,
                                     Dashboard_Demo_State *st, float phase);

typedef struct {
    const char           *title;
    WLX_Icon              icon;
    Dashboard_Section_Fn  render;
} Dashboard_Section;

static void section_overview(WLX_Context *ctx, const Dashboard_Tokens *tk,
                             const Dashboard_Fonts *fonts, Dashboard_Demo_State *st,
                             float phase);
static void section_tokens(WLX_Context *ctx, const Dashboard_Tokens *tk,
                           const Dashboard_Fonts *fonts, Dashboard_Demo_State *st,
                           float phase);
static void section_components(WLX_Context *ctx, const Dashboard_Tokens *tk,
                               const Dashboard_Fonts *fonts, Dashboard_Demo_State *st,
                               float phase);
static void section_layouts(WLX_Context *ctx, const Dashboard_Tokens *tk,
                            const Dashboard_Fonts *fonts, Dashboard_Demo_State *st,
                            float phase);
static void section_theme_lab(WLX_Context *ctx, const Dashboard_Tokens *tk,
                              const Dashboard_Fonts *fonts, Dashboard_Demo_State *st,
                              float phase);

static const Dashboard_Section dashboard_sections[DASHBOARD_VIEW_COUNT] = {
    [DASHBOARD_VIEW_OVERVIEW]   = { "Overview",   WLX_ICON_LAYOUT_DASHBOARD, section_overview },
    [DASHBOARD_VIEW_TOKENS]     = { "Tokens",     WLX_ICON_PALETTE,          section_tokens },
    [DASHBOARD_VIEW_COMPONENTS] = { "Components", WLX_ICON_BLOCKS,           section_components },
    [DASHBOARD_VIEW_LAYOUTS]    = { "Layouts",    WLX_ICON_LAYERS,           section_layouts },
    [DASHBOARD_VIEW_THEME_LAB]  = { "Theme Lab",  WLX_ICON_PALETTE,          section_theme_lab },
};

// ============================================================================
// Overview dashboard: a faithful rebuild of the Stitch "Wollix Explorer" screen.
// Layout and content are shared; colors adapt per mode to track the Stitch dark
// and light references (cyan vs blue accent, terminal/log treatment, overlay
// polarity). Icons are real Lucide glyphs from the shared atlas (via
// dashboard_icons.h); photographic assets (the avatar, the lab hero image) have
// no glyph equivalent and stay as a circle-user icon and a gradient placeholder.
// ============================================================================

// Mode-aware color helpers. The Stitch dark and light references differ in accent
// hue (cyan vs blue), overlay polarity (light-on-dark vs dark-on-light), and the
// terminal/log treatment, so colors are derived from the active tokens per mode
// while the content and layout stay identical.
static inline WLX_Color dash_a(WLX_Color c, uint8_t a) { c.a = a; return c; }
static inline int dash_is_light(const Dashboard_Tokens *tk) {
    return tk->mode == DASHBOARD_MODE_LIGHT;
}
// Faint hairline: a light line on dark surfaces, a dark line on light surfaces.
static inline WLX_Color dash_hairline(const Dashboard_Tokens *tk, uint8_t a) {
    return dash_is_light(tk) ? WLX_RGBA(0, 0, 0, a) : WLX_RGBA(255, 255, 255, a);
}
// Foreground accent. The light brand accent (#00d1ff) reads only as a fill or
// wash on light surfaces, so accent *content* (text, glyphs, thin indicator
// strokes) uses the deep primary (accent_strong) in light; in dark both resolve
// to the neon accent. Fills, washes, and glows keep tk->color.accent directly.
static inline WLX_Color dash_accent_fg(const Dashboard_Tokens *tk) {
    return dash_is_light(tk) ? tk->color.accent_strong : tk->color.accent;
}

// Uppercase mono panel heading in the accent color.
static void dashboard_panel_heading(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                    const Dashboard_Fonts *fonts, const char *text) {
    char up[48];
    dashboard_upper(text, up, sizeof(up));
    // Dark uses the neon accent for headings; light uses muted grey (per the
    // light reference's text-secondary headings).
    WLX_Color head = dash_is_light(tk) ? tk->color.on_surface_muted : tk->color.accent;
    wlx_label(ctx, up,
        .id = up,
        .style = dashboard_text_style(fonts, tk->type.label, head),
        .align = WLX_LEFT,
        .wrap = false,
        .vertical_metric = WLX_VMETRIC_FONT_SIZE,
        .border_width = 0);
}

// One hero stat card: label + icon, big value + unit, segmented usage bar. The
// usage bar shows `fraction` (0..1, a relative load indicator against a soft
// reference) across `segments` cells; the headline figure is `value`/`unit`.
static void dashboard_stat_card(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                const Dashboard_Fonts *fonts, const char *label,
                                const char *value, const char *unit,
                                float fraction, int segments, WLX_Icon icon) {
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;
    dashboard_module_begin(ctx, tk);
        // Scope child ids by the card label so repeated value/unit strings (e.g.
        // "n/a") across cards stay distinct.
        wlx_layout_begin(ctx, 3, WLX_VERT, .id = label, .padding = 24, .gap = 12,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(16), WLX_SLOT_FLEX(1), WLX_SLOT_PX(6) });
            // Label (label-sm uppercase, muted) + accent icon, top-right.
            wlx_layout_begin(ctx, 2, WLX_HORZ,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_PX(20) });
                Dashboard_Type_Role lbl = tk->type.label;
                lbl.size = 11;
                char up[32];
                dashboard_upper(label, up, sizeof(up));
                wlx_label(ctx, up,
                    .id = up,
                    .style = dashboard_text_style(fonts, lbl, tk->color.on_surface_muted),
                    .align = WLX_LEFT,
                    .wrap = false,
                    .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                    .border_width = 0);
                dashboard_icon_image(ctx, tk, icon, DASHBOARD_ICON_ROLE_ACCENT, 18.0f,
                    .id = label, .width = 18, .height = 18, .widget_align = WLX_TOP_RIGHT);
            wlx_layout_end(ctx);
            // Value (display 36) + unit (label-md, accent). Both slots are flex
            // so the value and unit shrink with the card instead of holding a
            // fixed width that would overflow the module when the card narrows.
            // The 2:1 split leaves the unit room for "/frame" at the four-card
            // width while the value still holds the widest figure.
            wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 6,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(2), WLX_SLOT_FLEX(1) });
                Dashboard_Type_Role big = tk->type.display;
                big.size = 36;
                // Light renders the metric in primary blue; dark keeps it on-surface.
                WLX_Color val = dash_is_light(tk) ? tk->color.accent_strong : tk->color.on_surface;
                wlx_label(ctx, value,
                    .id = value,
                    .style = dashboard_text_style(fonts, big, val),
                    .align = WLX_LEFT,
                    .wrap = false,
                    .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                    .border_width = 0);
                wlx_label(ctx, unit,
                    .id = unit,
                    .style = dashboard_text_style(fonts, tk->type.label, dash_accent_fg(tk)),
                    .align = WLX_LEFT,
                    .wrap = false,
                    .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                    .border_width = 0);
            wlx_layout_end(ctx);
            // Segmented usage bar.
            dashboard_segmented_progress(ctx, tk, fraction, segments);
        wlx_layout_end(ctx);
    dashboard_module_end(ctx);
}

// Sidebar nav link (interactive). Active link gets an accent wash, accent text,
// and a left accent bar. Returns nonzero on click.
static int dashboard_nav_item(WLX_Context *ctx, const Dashboard_Tokens *tk,
                              const Dashboard_Fonts *fonts, const char *label,
                              int is_active, WLX_Icon icon) {
    WLX_Color bg = is_active ? dash_a(tk->color.accent, 26) : tk->ramp.low;
    WLX_Color fg = is_active ? dash_accent_fg(tk) : tk->color.on_surface_muted;
    // The leading glyph tints to match the link text (accent when active).
    int clicked = dashboard_icon_button(ctx, tk, label, icon, DASHBOARD_ICON_ROLE_MUTED,
        .back_color = bg, .front_color = fg, .roundness = 0.12f, .border_width = 0,
        .align = WLX_LEFT, .content_padding_left = 14,
        .image_placement = WLX_IMAGE_PLACEMENT_LEFT, .image_size = 20, .image_text_gap = 10,
        .texture_tint = fg,
        .font = dashboard_font_resolve(fonts, DASHBOARD_FAMILY_SANS,
                    is_active ? DASHBOARD_WEIGHT_BOLD : DASHBOARD_WEIGHT_MEDIUM),
        .font_size = dashboard_type_px(tk->type.body_md), .height = 40, .id = label);
    return clicked;
}

// A sidebar footer link (Documentation / Support): leading glyph + label, the
// whole slot a click target that opens `url` through the open-URL hook. The text
// and glyph brighten to the accent on hover.
static void dashboard_footer_link(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                  const Dashboard_Fonts *fonts, const char *text,
                                  WLX_Icon icon, const char *url) {
    WLX_Interaction it = {0};
    wlx_layout_begin(ctx, 1, WLX_VERT, .id = text,
        .interact = WLX_INTERACT_CLICK | WLX_INTERACT_HOVER, .interact_out = &it);
        WLX_Color foot = it.hover ? dash_accent_fg(tk) : dash_a(tk->color.on_surface_muted, 150);
        dashboard_icon_label(ctx, tk, text, icon, DASHBOARD_ICON_ROLE_MUTED,
            .id = text,
            .style = dashboard_text_style(fonts, tk->type.label, foot),
            .align = WLX_LEFT, .wrap = false,
            .image_size = 16, .image_text_gap = 8, .texture_tint = foot,
            .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
    wlx_layout_end(ctx);
    if (it.clicked && url) dashboard_open_url(url);
}

// A widget-list row (Basic Widgets panel): label + trailing chevron. The whole row
// is a click target that deep-links into the Components section and brightens to the
// accent on hover. Returns nonzero on click. `dim` fades the row to a non-interactive
// state when an active search query does not match it. The wash background and the
// accent left bar are the container's own hover-variant chrome (the bar is a constant
// 2px side toggled transparent -> accent on hover); a dimmed row passes interact = 0
// so it stays inert.
static int dashboard_list_row(WLX_Context *ctx, const Dashboard_Tokens *tk,
                              const Dashboard_Fonts *fonts, const char *text, bool dim) {
    WLX_Interaction it = {0};
    uint32_t flags = dim ? 0u : (uint32_t)(WLX_INTERACT_CLICK | WLX_INTERACT_HOVER);
    uint8_t txt_a  = dim ? 70 : 255;
    wlx_layout_begin(ctx, 1, WLX_VERT, .id = text,
        .interact = flags, .interact_out = &it,
        .roundness = 0.1f, .rounded_segments = 6,
        .back_color = (WLX_Color){0}, .hover_back_color = dash_a(tk->color.accent, 26),
        .border_width_left = 2, .border_color_left = (WLX_Color){0},
        .hover_border_color_left = dash_accent_fg(tk),
        .border_width_top = 0, .border_width_right = 0, .border_width_bottom = 0);
        bool active = it.hover;
        WLX_Color fg = active ? dash_accent_fg(tk) : dash_a(tk->color.on_surface_muted, txt_a);
        WLX_Text_Style st = dashboard_text_style(fonts, tk->type.body_md, fg);
        WLX_Color cv = active ? dash_accent_fg(tk)
                              : dash_a(tk->color.on_surface_muted, dim ? 40 : 80);
        wlx_layout_begin(ctx, 2, WLX_HORZ, .id = text, .padding_left = 8, .gap = 0,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_PX(14) });
            wlx_label(ctx, text, .id = text, .style = st, .align = WLX_LEFT,
                .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
            dashboard_icon_image(ctx, tk, WLX_ICON_CHEVRON_RIGHT, DASHBOARD_ICON_ROLE_MUTED, 14.0f,
                .id = "chevron", .width = 14, .height = 14, .widget_align = WLX_LEFT, .tint = cv);
        wlx_layout_end(ctx);
    wlx_layout_end(ctx);
    return it.clicked ? 1 : 0;
}

// A structural-layout row: leading icon + label. The whole row is a click target
// that deep-links into the Layouts section and brightens to the accent on hover.
// Returns nonzero on click. `dim` fades the row when an active search query does not
// match it; a dimmed row passes interact = 0 so it stays inert. The leading icon box
// and label colors are derived from the row's own hover result.
static int dashboard_icon_row(WLX_Context *ctx, const Dashboard_Tokens *tk,
                              const Dashboard_Fonts *fonts, const char *text,
                              WLX_Icon icon, bool dim) {
    WLX_Interaction it = {0};
    uint32_t flags = dim ? 0u : (uint32_t)(WLX_INTERACT_CLICK | WLX_INTERACT_HOVER);
    uint8_t a = dim ? 70 : 255;
    // Leading icon box + label as a horizontal split: a fixed icon slot hosting the
    // glyph, then a flex text slot. The 32 px icon slot insets its 18 px box by 4 px,
    // so the text still starts at x + 32. The whole split is the hit region.
    wlx_layout_begin(ctx, 2, WLX_HORZ, .id = text, .gap = 0,
        .interact = flags, .interact_out = &it,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(32), WLX_SLOT_FLEX(1) });
        bool active = it.hover;
        WLX_Color fg = active ? dash_accent_fg(tk) : dash_a(tk->color.on_surface_muted, a);
        WLX_Text_Style st = dashboard_text_style(fonts, tk->type.body_md, fg);
        dashboard_icon_image(ctx, tk, icon, DASHBOARD_ICON_ROLE_MUTED, 18.0f,
            .id = "icon", .width = 18, .height = 18,
            .widget_align = WLX_LEFT, .padding_left = 4, .tint = fg);
        wlx_label(ctx, text,
            .id = text,
            .style = st,
            .align = WLX_LEFT,
            .vertical_metric = WLX_VMETRIC_FONT_SIZE,
            .border_width = 0);
    wlx_layout_end(ctx);
    return it.clicked ? 1 : 0;
}

// A fixed-size icon chip: a `box`x`box` rounded decor square (filled and/or
// outlined) with the glyph centered inside, left-aligned and vertically
// centered within the current slot. Feature cards use the filled variant; the
// integration rows use the outlined variant. The box is constrained to `box`
// via a horizontal split (box + trailing flex) and vertically centered via a
// flex / fixed / flex column, so it keeps its exact dimensions regardless of
// the row height.
static void dashboard_icon_chip(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                WLX_Icon icon, Dashboard_Icon_Role role,
                                float box, float glyph,
                                WLX_Color fill, WLX_Color border, float border_w) {
    wlx_layout_begin(ctx, 2, WLX_HORZ, .id = "chip", .gap = 0,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(box), WLX_SLOT_FLEX(1) });
        wlx_layout_begin(ctx, 3, WLX_VERT, .gap = 0,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_PX(box), WLX_SLOT_FLEX(1) });
            // Top flex spacer (transparent) centers the box vertically.
            wlx_widget(ctx, .id = "chip-pad", .back_color = (WLX_Color){0}, .border_width = 0);
            wlx_layout_begin(ctx, 1, WLX_VERT,
                .back_color = fill,
                .border_color = border, .border_width = border_w,
                .roundness = 0.2f, .rounded_segments = 8);
                dashboard_icon_image(ctx, tk, icon, role, glyph,
                    .id = "chip-icon", .width = glyph, .height = glyph,
                    .widget_align = WLX_CENTER);
            wlx_layout_end(ctx);
            // Trailing flex spacer left empty.
        wlx_layout_end(ctx);
        // Trailing flex slot left empty.
    wlx_layout_end(ctx);
}

// A feature card in the center tab panel: icon box + title + subtitle. The whole
// card is a click target that deep-links into the Layouts section, where the
// advertised capability (scroll panel, opacity, id stack, borders) has a live demo;
// its outline brightens to the accent on hover via the card's own hover-variant
// border color. Returns nonzero on click.
static int dashboard_feature_card(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                  const Dashboard_Fonts *fonts, const char *title,
                                  const char *subtitle, WLX_Color icon_bg,
                                  WLX_Icon icon, Dashboard_Icon_Role role) {
    WLX_Text_Style ts = dashboard_text_style(fonts, tk->type.body_md, tk->color.on_surface);
    Dashboard_Type_Role small = tk->type.body_sm;
    WLX_Text_Style ss = dashboard_text_style(fonts, small, dash_a(tk->color.on_surface_muted, 150));
    // Slots use the role line-height (not the font size) so descenders in the
    // title and subtitle are not clipped by the card's rounded clip. body_sm's
    // design line-height is tight, and SDL3 renders descenders a touch past the
    // nominal line box, so the subtitle slot gets a small extra descender margin.
    int title_h = dashboard_type_line_px(tk->type.body_md);
    int sub_h   = dashboard_type_line_px(small) + 4;
    // Title sits above its subtitle with a small fixed gap.
    float row_gap = 4.0f;
    WLX_Interaction it = {0};
    wlx_layout_begin(ctx, 2, WLX_VERT, .id = title, .padding = 14, .gap = 10, .clip = true,
        .interact = WLX_INTERACT_CLICK | WLX_INTERACT_HOVER, .interact_out = &it,
        .back_color = tk->glass.fill,
        .corner_radius = (float)tk->radius.lg, .rounded_segments = 8,
        .border_color = dash_hairline(tk, 13), .border_width = 1.0f,
        .hover_border_color = dash_a(dash_accent_fg(tk), 130),
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(40), WLX_SLOT_FLEX(1) });
        dashboard_icon_chip(ctx, tk, icon, role, 40.0f, 22.0f,
            icon_bg, (WLX_Color){0}, 0.0f);
        wlx_layout_begin(ctx, 2, WLX_VERT, .id = subtitle, .gap = row_gap,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX((float)title_h),
                                        WLX_SLOT_PX((float)sub_h) });
            wlx_label(ctx, title, .id = title, .style = ts, .align = WLX_LEFT,
                .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
            wlx_label(ctx, subtitle, .id = subtitle, .style = ss, .align = WLX_LEFT,
                .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
        wlx_layout_end(ctx);
    wlx_layout_end(ctx);
    return it.clicked ? 1 : 0;
}

// An integrations row: bordered icon box + label + chevron. The whole row is a
// click target; a click opens `url` through the per-backend open-URL hook. The
// label and chevron brighten to the accent on hover to signal the affordance.
static void dashboard_integration_row(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                      const Dashboard_Fonts *fonts, const char *label,
                                      WLX_Icon icon, const char *url) {
    // Leading outlined icon chip + label + chevron as a nested horizontal split:
    // a fixed icon slot hosting the 32 px outlined chip, then a flex label slot
    // and a fixed chevron slot. The 44 px icon slot keeps the label at x + 44.
    WLX_Interaction it = {0};
    wlx_layout_begin(ctx, 3, WLX_HORZ, .id = label, .gap = 0,
        .interact = WLX_INTERACT_CLICK | WLX_INTERACT_HOVER, .interact_out = &it,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(44), WLX_SLOT_FLEX(1), WLX_SLOT_PX(14) });
        WLX_Color fg = it.hover ? dash_accent_fg(tk) : tk->color.on_surface_muted;
        WLX_Text_Style st = dashboard_text_style(fonts, tk->type.body_md, fg);
        dashboard_icon_chip(ctx, tk, icon, DASHBOARD_ICON_ROLE_MUTED, 32.0f, 18.0f,
            (WLX_Color){0}, dash_hairline(tk, 26), 1.0f);
        wlx_label(ctx, label, .id = label, .style = st, .align = WLX_LEFT,
            .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
        dashboard_icon_image(ctx, tk, WLX_ICON_CHEVRON_RIGHT, DASHBOARD_ICON_ROLE_MUTED, 14.0f,
            .id = "chevron", .width = 14, .height = 14, .widget_align = WLX_LEFT,
            .tint = it.hover ? dash_accent_fg(tk) : dash_a(tk->color.on_surface_muted, 90));
    wlx_layout_end(ctx);
    if (it.clicked && url) dashboard_open_url(url);
}

// A solid accent call-to-action button with centered text and a clearly visible
// hover (the fill darkens on hover). Brightening an already-bright accent fill is
// imperceptible, so a negative hover_brightness darkens it instead. Returns nonzero
// on click.
static int dashboard_accent_button(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                   const char *text, WLX_Font font, int font_size,
                                   float height, const char *id) {
    return wlx_button(ctx, text,
        .back_color = tk->color.accent, .front_color = tk->color.on_accent,
        .roundness = 0.12f, .border_width = 0,
        .align = WLX_CENTER, .content_padding = 12,
        .hover_brightness = -0.18f,
        .font = font, .font_size = font_size, .height = height, .id = id) ? 1 : 0;
}

// Top navigation bar.
static void dashboard_topbar(WLX_Context *ctx, const Dashboard_Tokens *tk,
                             const Dashboard_Fonts *fonts, Dashboard_Demo_State *st) {
    // Live FPS readout for the right group. The shared per-frame metric is
    // refreshed for every section (see dashboard_update_metrics), so surfacing
    // it on the always-present top bar keeps the rate visible while switching
    // sections, not just on the Overview hero card. Wollix copies label text
    // at emit time, so this stack buffer only needs to outlive the build below.
    char fps_text[16];
    snprintf(fps_text, sizeof(fps_text), "%.0f FPS", (double)g_dashboard_metrics.fps);

    // Topbar surface plus its bottom hairline expressed as the wrapper's own
    // container decor, so the chrome keys off the layout rect.
    wlx_layout_begin(ctx, 1, WLX_VERT,
        .back_color = tk->ramp.base,
        .border_color_bottom = dash_hairline(tk, 13), .border_width_bottom = 1.0f);

    // The identity block flexes down from 320 to a 160 floor so the left group
    // keeps room as the window narrows; redistribute hands the capped surplus
    // back to the left slot on wide windows (no dead strip on the right edge). The
    // 320 cap holds the full "Developer Access" line at the design text size.
    wlx_layout_begin(ctx, 2, WLX_HORZ, .padding_left = 32, .padding_right = 32,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(3), WLX_SLOT_FLEX_MINMAX(2, 160, 360) });
        // Left: logo + version, then search field. The search slot flexes so it
        // caps at 256 on wide windows but shrinks (down to a 96px floor) with the
        // left group instead of overflowing fixed-width into the identity block.
        // clip contains any residual overflow below the layout floor: instead of
        // the search bleeding over the identity text, it is cropped at the slot
        // edge (and hidden entirely once the window is too narrow to fit it).
        wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 24, .clip = true,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(180), WLX_SLOT_FLEX_MINMAX(1, 96, 256) });
            wlx_layout_begin(ctx, 2, WLX_VERT,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1) });
                wlx_label(ctx, "Wollix Explorer",
                    .id = "Wollix Explorer",
                    .style = dashboard_text_style(fonts, tk->type.headline_md, dash_accent_fg(tk)),
                    .align = WLX_LEFT,
                    .wrap = false,
                    .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                    .border_width = 0);
                Dashboard_Type_Role ver = tk->type.label;
                ver.size = 11;
                wlx_label(ctx, "V0.6.0-UNSTABLE",
                    .id = "V0.6.0-UNSTABLE",
                    .style = dashboard_text_style(fonts, ver, dash_a(tk->color.on_surface_muted, 130)),
                    .align = WLX_LEFT,
                    .wrap = false,
                    .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                    .border_width = 0);
            wlx_layout_end(ctx);
            wlx_layout_begin(ctx, 1, WLX_VERT,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(34) }, .padding_top = 15);
                // Search field with the magnifier glyph rendered inside the
                // field frame on the leading edge, so the icon reads as part of
                // the input instead of a detached column. The 18 px glyph and
                // 8 px gap keep the text on the same baseline as the field.
                dashboard_icon_inputbox(ctx, tk, WLX_ICON_SEARCH, DASHBOARD_ICON_ROLE_MUTED,
                    st->search, sizeof(st->search),
                    .id = "search",
                    .back_color = tk->color.field,
                    .border_color = tk->color.field_border, .border_width = 1.0f,
                    .roundness = 0.12f,
                    .font = dashboard_type_font(fonts, tk->type.body_md),
                    .font_size = dashboard_type_px(tk->type.body_md),
                    .content_padding = 4,
                    .height = 34,
                    .image_size = 18, .image_text_gap = 8,
                    .image_placement = WLX_IMAGE_PLACEMENT_LEFT);
            wlx_layout_end(ctx);
        wlx_layout_end(ctx);
        // Right: theme switch + notification + settings glyphs, user identity
        // (right-aligned), and a circle-user avatar. The icons and avatar are
        // fixed-width slots so they keep their size; the identity text flexes (and
        // clips first) as the identity block narrows. The view-level clip contains
        // any overflow.
        wlx_layout_begin(ctx, 6, WLX_HORZ, .gap = 14, .padding_top = 12, .padding_bottom = 12,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(64), WLX_SLOT_PX(40), WLX_SLOT_PX(40),
                                        WLX_SLOT_PX(40), WLX_SLOT_FLEX(1), WLX_SLOT_PX(34) });
            // Live FPS readout: mono face, accent foreground so it reads as a
            // metric rather than identity chrome. Right-aligned so it sits tight
            // against the theme switch as the right group narrows.
            wlx_label(ctx, fps_text,
                .id = "topbar-fps",
                .style = dashboard_text_style(fonts, tk->type.mono, dash_accent_fg(tk)),
                .align = WLX_RIGHT,
                .wrap = false,
                .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                .border_width = 0);
            // Light/dark switch: an icon button whose zero back_color resolves to
            // the theme surface - invisible at rest against the top bar, brightening
            // to a highlight on hover. Its 40px slot matches the row height so the
            // hover highlight gets even padding around the centered 22px glyph. The
            // click is recorded on the demo state and applied by the main loop next
            // frame.
            if (dashboard_icon_button(ctx, tk, "", WLX_ICON_PALETTE, DASHBOARD_ICON_ROLE_MUTED,
                    .id = "theme-toggle",
                    .border_color = (WLX_Color){0}, .border_width = 0,
                    .image_size = 22, .image_text_gap = 0,
                    .image_placement = WLX_IMAGE_PLACEMENT_LEFT,
                    .widget_align = WLX_CENTER, .align = WLX_CENTER)) {
                st->theme_toggle = true;
            }
            // Notification bell: tints to the accent while the action log holds
            // unread entries and reverts to muted once acknowledged. Clicking it
            // marks the log read.
            Dashboard_Icon_Role bell_role = g_dashboard_log.unread > 0
                ? DASHBOARD_ICON_ROLE_ACCENT : DASHBOARD_ICON_ROLE_MUTED;
            if (dashboard_icon_button(ctx, tk, "", WLX_ICON_BELL, bell_role,
                    .id = "bell",
                    .border_color = (WLX_Color){0}, .border_width = 0,
                    .image_size = 22, .image_text_gap = 0,
                    .image_placement = WLX_IMAGE_PLACEMENT_LEFT,
                    .widget_align = WLX_CENTER, .align = WLX_CENTER)) {
                g_dashboard_log.unread = 0;
            }
            // Settings opens the Theme Lab section (appearance / theme configuration).
            if (dashboard_icon_button(ctx, tk, "", WLX_ICON_SETTINGS, DASHBOARD_ICON_ROLE_MUTED,
                    .id = "settings",
                    .border_color = (WLX_Color){0}, .border_width = 0,
                    .image_size = 22, .image_text_gap = 0,
                    .image_placement = WLX_IMAGE_PLACEMENT_LEFT,
                    .widget_align = WLX_CENTER, .align = WLX_CENTER)) {
                st->current_view = DASHBOARD_VIEW_THEME_LAB;
            }
            wlx_layout_begin(ctx, 2, WLX_VERT,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1) });
                wlx_label(ctx, "Admin_User",
                    .id = "Admin_User",
                    .style = dashboard_text_style(fonts, tk->type.label, tk->color.on_surface),
                    .align = WLX_RIGHT,
                    .wrap = false,
                    .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                    .border_width = 0);
                Dashboard_Type_Role sub = tk->type.label;
                sub.size = 11;
                wlx_label(ctx, "Developer Access",
                    .id = "Developer Access",
                    .style = dashboard_text_style(fonts, sub, dash_a(tk->color.on_surface_muted, 110)),
                    .align = WLX_RIGHT,
                    .wrap = false,
                    .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                    .border_width = 0);
            wlx_layout_end(ctx);
            // Identity avatar: intentionally static demo chrome (there is no real
            // account behind it), so it stays a non-interactive glyph.
            dashboard_icon_image(ctx, tk, WLX_ICON_CIRCLE_USER, DASHBOARD_ICON_ROLE_ON_SURFACE, 34.0f,
                .id = "avatar", .width = 32, .height = 32, .widget_align = WLX_CENTER);
        wlx_layout_end(ctx);
    wlx_layout_end(ctx);
    wlx_layout_end(ctx);
}

// Left sidebar: nav links packed at the top; Deploy button and footer links at
// the bottom.
static void dashboard_overview_sidebar(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                       const Dashboard_Fonts *fonts, Dashboard_Demo_State *st) {
    // Sidebar surface plus its right hairline expressed as the wrapper's own
    // container decor.
    wlx_layout_begin(ctx, 1, WLX_VERT,
        .back_color = tk->ramp.low,
        .border_color_right = dash_hairline(tk, 26), .border_width_right = 1.0f);

    // Nav region takes the flexible slot but is floored at its natural content
    // height (the table's links at 40px each plus 4px gaps) so a short window
    // cannot squeeze it below the fixed-height links and overlap them with the
    // bottom group. When the window is too short for both, the bottom group slides
    // off the bottom edge instead of overlapping the nav.
    wlx_layout_begin(ctx, 2, WLX_VERT, .padding_top = 32, .padding_bottom = 32,
        .padding_left = 16, .padding_right = 16,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX_MIN(1, 220), WLX_SLOT_PX(132) });
        // Nav links packed at the top: one per section route from the dispatch
        // table, then a trailing FLEX spacer. Clicking a link sets current_view.
        WLX_Slot_Size nav_sizes[DASHBOARD_VIEW_COUNT + 1];
        for (int i = 0; i < DASHBOARD_VIEW_COUNT; i++) nav_sizes[i] = WLX_SLOT_PX(40);
        nav_sizes[DASHBOARD_VIEW_COUNT] = WLX_SLOT_FLEX(1);
        wlx_layout_begin(ctx, DASHBOARD_VIEW_COUNT + 1, WLX_VERT, .gap = 4, .sizes = nav_sizes);
            for (int i = 0; i < DASHBOARD_VIEW_COUNT; i++) {
                bool active = i == (int)st->current_view;
                if (dashboard_nav_item(ctx, tk, fonts, dashboard_sections[i].title, active, dashboard_sections[i].icon)) {
                    st->current_view = (Dashboard_View)i;
                }
            }
            // Trailing FLEX slot is left empty to pack the links at the top.
        wlx_layout_end(ctx);
        // Bottom group: Deploy button + footer links.
        wlx_layout_begin(ctx, 3, WLX_VERT, .gap = 12,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(44), WLX_SLOT_PX(24), WLX_SLOT_PX(24) });
            // Deploy posts a real build sequence to the in-app action log (it acts
            // on demo state only -- there is no remote deploy to fabricate).
            if (dashboard_accent_button(ctx, tk, "Deploy Build",
                    dashboard_font_resolve(fonts, DASHBOARD_FAMILY_MONO, DASHBOARD_WEIGHT_MEDIUM),
                    dashboard_type_px(tk->type.label), 44, "deploy-build")) {
                dashboard_log_emit(DASH_LOG_INFO, "Deploy requested");
                dashboard_log_emit(DASH_LOG_OK, "Command buffer rebuilt");
                dashboard_log_emit(DASH_LOG_OK, "Frame committed");
            }
            dashboard_footer_link(ctx, tk, fonts, "Documentation", WLX_ICON_FILE_TEXT,
                DASHBOARD_URL_DOCS);
            dashboard_footer_link(ctx, tk, fonts, "Support", WLX_ICON_CIRCLE_QUESTION_MARK,
                DASHBOARD_URL_SUPPORT);
        wlx_layout_end(ctx);
    wlx_layout_end(ctx);
    wlx_layout_end(ctx);
}

// Left grid column: Basic Widgets and Structural Layouts panels. Each row deep-links
// into the section that demonstrates it (widgets -> Components, layouts -> Layouts).
// When the top-bar search holds a query, non-matching rows fade out and stop
// responding, so the search reads as a live filter over these lists.
static void dashboard_widgets_column(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                     const Dashboard_Fonts *fonts, Dashboard_Demo_State *st) {
    const char *q = st->search;
    wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 16,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(212), WLX_SLOT_FLEX(1) });
        dashboard_module_begin(ctx, tk);
            wlx_layout_begin(ctx, 5, WLX_VERT, .padding = 16, .gap = 4,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(28), WLX_SLOT_PX(34), WLX_SLOT_PX(34),
                                            WLX_SLOT_PX(34), WLX_SLOT_PX(34) });
                dashboard_panel_heading(ctx, tk, fonts, "Basic Widgets");
                const char *widgets[] = { "Label", "Button", "Checkbox", "TextField" };
                for (int i = 0; i < 4; i++) {
                    if (dashboard_list_row(ctx, tk, fonts, widgets[i], !dashboard_str_match(q, widgets[i])))
                        st->current_view = DASHBOARD_VIEW_COMPONENTS;
                }
            wlx_layout_end(ctx);
        dashboard_module_end(ctx);
        dashboard_module_begin(ctx, tk);
            wlx_layout_begin(ctx, 4, WLX_VERT, .padding = 16, .gap = 4,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(28), WLX_SLOT_PX(34),
                                            WLX_SLOT_PX(34), WLX_SLOT_PX(34) });
                dashboard_panel_heading(ctx, tk, fonts, "Structural Layouts");
                const char *layouts[] = { "Linear", "Grid", "Flex" };
                const WLX_Icon licons[] = { WLX_ICON_LIST, WLX_ICON_GRID_3X3, WLX_ICON_LAYOUT_TEMPLATE };
                for (int i = 0; i < 3; i++) {
                    if (dashboard_icon_row(ctx, tk, fonts, layouts[i], licons[i],
                            !dashboard_str_match(q, layouts[i])))
                        st->current_view = DASHBOARD_VIEW_LAYOUTS;
                }
            wlx_layout_end(ctx);
        dashboard_module_end(ctx);
    wlx_layout_end(ctx);
}

// One tab button in the center panel's tab bar. Returns nonzero on click.
static int dashboard_tab(WLX_Context *ctx, const Dashboard_Tokens *tk,
                         const Dashboard_Fonts *fonts, const char *text, int active) {
    WLX_Color bg = active ? dash_a(tk->color.accent, 13) : tk->ramp.base;
    WLX_Color fg = active ? dash_accent_fg(tk) : tk->color.on_surface_muted;
    return wlx_button(ctx, text, .back_color = bg, .front_color = fg, .roundness = 0,
        .border_width = 0, .align = WLX_CENTER,
        .font = dashboard_font_resolve(fonts, DASHBOARD_FAMILY_MONO, DASHBOARD_WEIGHT_MEDIUM),
        .font_size = dashboard_type_px(tk->type.label), .height = 44, .id = text);
}

// A muted caption line inside a module body; defined with the showcase helpers
// further down, forward-declared here for the Active State pane.
static void dashboard_caption(WLX_Context *ctx, const Dashboard_Tokens *tk,
                              const Dashboard_Fonts *fonts, const char *text);

// Active State tab body: a compact 2x2 of genuinely live widgets (checkbox, toggle,
// slider, click-counted button) bound to persistent demo state, so the tab is real
// interactivity rather than swapped static art.
static void dashboard_overview_active_state(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                            const Dashboard_Fonts *fonts,
                                            Dashboard_Demo_State *st) {
    WLX_Font body_font = dashboard_type_font(fonts, tk->type.body_md);
    int      body_px   = dashboard_type_px(tk->type.body_md);
    wlx_grid_begin(ctx, 2, 2, .gap = 16);
        wlx_layout_begin(ctx, 2, WLX_VERT, .id = "as-check", .gap = 8,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(18), WLX_SLOT_FLEX(1) });
            dashboard_caption(ctx, tk, fonts, "Checkbox");
            wlx_checkbox(ctx, "Enabled", &st->overview_check, .id = "as-checkbox",
                .height = 34, .font = body_font, .font_size = body_px);
        wlx_layout_end(ctx);
        wlx_layout_begin(ctx, 2, WLX_VERT, .id = "as-toggle", .gap = 8,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(18), WLX_SLOT_FLEX(1) });
            dashboard_caption(ctx, tk, fonts, "Toggle");
            wlx_toggle(ctx, "Live", &st->overview_toggle, .id = "as-toggle-sw",
                .height = 34, .font = body_font, .font_size = body_px);
        wlx_layout_end(ctx);
        wlx_layout_begin(ctx, 2, WLX_VERT, .id = "as-slider", .gap = 8,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(18), WLX_SLOT_FLEX(1) });
            dashboard_caption(ctx, tk, fonts, "Slider");
            wlx_slider(ctx, "Mix ", &st->overview_slider, .id = "as-slider-w",
                .height = 38, .font = body_font, .font_size = body_px,
                .min_value = 0.0f, .max_value = 1.0f, .show_value = true);
        wlx_layout_end(ctx);
        wlx_layout_begin(ctx, 2, WLX_VERT, .id = "as-button", .gap = 8,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(18), WLX_SLOT_FLEX(1) });
            char cap[24];
            snprintf(cap, sizeof(cap), "Button (%d)", st->overview_clicks);
            dashboard_caption(ctx, tk, fonts, cap);
            if (dashboard_accent_button(ctx, tk, "Run", body_font, body_px, 38, "as-run"))
                st->overview_clicks++;
        wlx_layout_end(ctx);
    wlx_grid_end(ctx);
}

// Terminal logs panel: a live scroll panel over the in-app action log, newest first.
// The body container carries the rounded fill and outline; the header band is a
// nested layout with its own bar fill and bottom hairline. clip contains the
// fixed-size header dots and the log text when the panel narrows below its slots.
static void dashboard_terminal_logs(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                    const Dashboard_Fonts *fonts) {
    int light = dash_is_light(tk);
    wlx_layout_begin(ctx, 1, WLX_VERT, .clip = true,
        .back_color = light ? tk->ramp.low : tk->ramp.lowest,
        .corner_radius = (float)tk->radius.lg, .rounded_segments = 8,
        .border_color = dash_a(tk->color.outline_variant, 120), .border_width = 1.0f);
        WLX_Rect r = wlx_get_parent_rect(ctx);
        Dashboard_Type_Role hr = tk->type.label;
        hr.size = 11;
        WLX_Color head_fg = light ? WLX_RGBA(240, 240, 240, 255) : tk->color.on_surface_muted;
        WLX_Text_Style ht = dashboard_text_style(fonts, hr, head_fg);
        // Scanlines only suit the dark "mechanical glass" body.
        if (!light) {
            WLX_Color scan = tk->color.accent;
            scan.a = 8;
            dashboard_draw_scanlines(ctx, (WLX_Rect){ r.x, r.y + 28.0f, r.w, r.h - 28.0f }, scan, 4.0f);
        }
        WLX_Color muted = dash_a(tk->color.on_surface_muted, 220);
        // Log tag hues: dark uses green/cyan/yellow; light uses blue/orange/red.
        WLX_Color c_ok   = light ? tk->color.accent_strong : WLX_RGBA(0, 255, 157, 255);
        WLX_Color c_info = light ? WLX_RGBA(235, 129, 4, 255) : tk->color.accent;
        WLX_Color c_warn = light ? tk->status.error : WLX_RGBA(255, 204, 0, 255);
        // Header band (fixed) over the scrolling log body (flex).
        wlx_layout_begin(ctx, 2, WLX_VERT, .id = "term", .gap = 0,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(28), WLX_SLOT_FLEX(1) });
            // Header band: technical-grey bar with a bottom hairline as the band's
            // own decor, hosting the heading and a trailing nested group of three
            // window-dot discs. It carries the container radius on its top corners
            // only, so the band follows the rounded panel at the top and stays flush
            // and square where it meets the body below.
            wlx_layout_begin(ctx, 2, WLX_HORZ,
                .back_color = light ? WLX_RGBA(62, 62, 62, 255) : tk->ramp.high,
                .corner_radius = (float)tk->radius.lg, .rounded_segments = 8,
                .rounded_corners = WLX_CORNERS_TOP,
                .border_color_bottom = dash_hairline(tk, 13), .border_width_bottom = 1.0f,
                .padding_right = 8, .gap = 0,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_PX(36) });
                wlx_label(ctx, "TERMINAL LOGS [WOLLIX_ENGINE]", .style = ht,
                    .align = WLX_TOP_LEFT, .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                    .content_padding_left = 12.0f, .content_padding_top = 9.0f,
                    .border_width = 0);
                // Window dots: bright red/yellow/green in light, faded tints in
                // dark. Square wlx_widgets with roundness 1.0 render true discs via
                // the box->circle fallback. The 8 px slots spaced by 6 px reproduce
                // the original 14 px dot-center pitch.
                WLX_Color dot_r = light ? WLX_RGBA(239, 68, 68, 255) : WLX_RGBA(255, 180, 171, 110);
                WLX_Color dot_y = light ? WLX_RGBA(234, 179, 8, 255)  : WLX_RGBA(164, 230, 255, 110);
                WLX_Color dot_g = light ? WLX_RGBA(34, 197, 94, 255)  : WLX_RGBA(0, 209, 255, 110);
                wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 6,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(8), WLX_SLOT_PX(8), WLX_SLOT_PX(8) });
                    wlx_widget(ctx, .id = "dot-r", .width = 8, .height = 8, .widget_align = WLX_CENTER,
                        .back_color = dot_r, .roundness = 1.0f, .rounded_segments = 12, .border_width = 0);
                    wlx_widget(ctx, .id = "dot-y", .width = 8, .height = 8, .widget_align = WLX_CENTER,
                        .back_color = dot_y, .roundness = 1.0f, .rounded_segments = 12, .border_width = 0);
                    wlx_widget(ctx, .id = "dot-g", .width = 8, .height = 8, .widget_align = WLX_CENTER,
                        .back_color = dot_g, .roundness = 1.0f, .rounded_segments = 12, .border_width = 0);
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
            // Scrolling body: one auto-height row per log entry, newest at the top so
            // the latest event is visible without relying on auto-scroll-to-bottom.
            // A transparent background lets the rounded container fill (and its
            // scanlines) show through, so the body is one rounded surface rather than
            // a square inner panel.
            // Virtualized log: an explicit content height feeds the clipper,
            // which builds only the rows inside the viewport so per-frame cost
            // stays flat as the log fills. ROW_H is the fixed row pitch.
            const float ROW_H = 22.0f;
            wlx_layout_begin(ctx, 1, WLX_VERT, .padding_top = 2, .padding_bottom = 2);
                wlx_scroll_panel_begin(ctx,
                    wlx_list_clipper_height(g_dashboard_log.count, ROW_H, NULL),
                    .transparent_background = true,
                    .border_width = 0, .scrollbar_color = tk->color.field_border);

                    WLX_List_Clipper logclip = wlx_list_clipper_begin(ctx,
                        g_dashboard_log.count, ROW_H, .id = "logrows", .overscan = ROW_H);
                        
                        for (int i = logclip.first; i < logclip.last; i++) {
                            const Dashboard_Log_Entry *e = dashboard_log_at(i);
                            if (!e) break;
                            WLX_Color tagcol = e->sev == DASH_LOG_OK   ? c_ok
                                             : e->sev == DASH_LOG_WARN ? c_warn : c_info;
                            const char *tagtext = e->sev == DASH_LOG_OK   ? "[OK]"
                                                : e->sev == DASH_LOG_WARN ? "[WARN]" : "[INFO]";
                            char msgbuf[96];
                            snprintf(msgbuf, sizeof(msgbuf), "+%.1fs  %s", (double)e->t, e->msg);
                            // Stable widget key by the entry's monotonic seq, so the
                            // ids do not churn as old entries drop off the ring.
                            wlx_push_id(ctx, (size_t)e->seq);
                            wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 0,
                                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(66), WLX_SLOT_FLEX(1) });
                                WLX_Text_Style tg = dashboard_text_style(fonts, hr, tagcol);
                                wlx_label(ctx, tagtext, .id = "tag", .style = tg,
                                    .align = WLX_TOP_LEFT, .wrap = false,
                                    .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                                    .content_padding_left = 12.0f, .content_padding_top = 5.0f,
                                    .border_width = 0);
                                WLX_Text_Style ms = dashboard_text_style(fonts, hr, muted);
                                wlx_label(ctx, msgbuf, .id = "msg", .style = ms,
                                    .align = WLX_TOP_LEFT, .wrap = false,
                                    .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                                    .content_padding_left = 12.0f, .content_padding_top = 5.0f,
                                    .border_width = 0);
                            wlx_layout_end(ctx);
                            wlx_pop_id(ctx);
                        }
                    wlx_list_clipper_end(ctx, &logclip);
                wlx_scroll_panel_end(ctx);
            wlx_layout_end(ctx);
        wlx_layout_end(ctx);
    wlx_layout_end(ctx);
}

// Center grid column: a tabbed panel (live Active State controls or the capability
// Preview cards) over the live terminal log.
static void dashboard_center_column(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                    const Dashboard_Fonts *fonts, Dashboard_Demo_State *st) {
    wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 16,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_PX(140) });

        // Tabbed panel.
        dashboard_module_begin(ctx, tk);
            wlx_layout_begin(ctx, 2, WLX_VERT,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(44), WLX_SLOT_FLEX(1) });
                // Tab bar with a bottom hairline. The active tab is driven by the
                // persistent overview_tab; clicking a tab switches the body below.
                wlx_layout_begin(ctx, 1, WLX_VERT);
                    wlx_layout_begin(ctx, 3, WLX_HORZ,
                        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(110), WLX_SLOT_PX(140), WLX_SLOT_FLEX(1) });
                        if (dashboard_tab(ctx, tk, fonts, "Preview", st->overview_tab == 0)) {
                            st->overview_tab = 0;
                            dashboard_log_emit(DASH_LOG_INFO, "Tab: Preview");
                        }
                        if (dashboard_tab(ctx, tk, fonts, "Active State", st->overview_tab == 1)) {
                            st->overview_tab = 1;
                            dashboard_log_emit(DASH_LOG_INFO, "Tab: Active State");
                        }
                        // Remaining FLEX slot is intentionally empty.
                    wlx_layout_end(ctx);
                wlx_layout_end(ctx);
                // Body switches on the active tab.
                wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 16);
                    if (st->overview_tab == 0) {
                        // Preview: the four capability cards; each deep-links to the
                        // Layouts section where its live demo lives.
                        wlx_grid_begin(ctx, 2, 2, .gap = 16);
                            if (dashboard_feature_card(ctx, tk, fonts, "Scroll Panel",
                                    "Dynamic overflow", dash_a(tk->color.accent, 26),
                                    WLX_ICON_MOUSE_POINTER_CLICK, DASHBOARD_ICON_ROLE_ACCENT))
                                st->current_view = DASHBOARD_VIEW_LAYOUTS;
                            if (dashboard_feature_card(ctx, tk, fonts, "Opacity Mask",
                                    "Gradient layering", dash_a(tk->color.tertiary, 45),
                                    WLX_ICON_DROPLET, DASHBOARD_ICON_ROLE_TERTIARY))
                                st->current_view = DASHBOARD_VIEW_LAYOUTS;
                            if (dashboard_feature_card(ctx, tk, fonts, "ID Stack",
                                    "Reference mapping", dash_a(tk->color.on_surface_muted, 50),
                                    WLX_ICON_CONTACT, DASHBOARD_ICON_ROLE_MUTED))
                                st->current_view = DASHBOARD_VIEW_LAYOUTS;
                            if (dashboard_feature_card(ctx, tk, fonts, "Borders & Radii",
                                    "Geometric precision", dash_a(tk->color.accent, 26),
                                    WLX_ICON_SQUARE_DASHED, DASHBOARD_ICON_ROLE_ACCENT))
                                st->current_view = DASHBOARD_VIEW_LAYOUTS;
                        wlx_grid_end(ctx);
                    } else {
                        dashboard_overview_active_state(ctx, tk, fonts, st);
                    }
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        dashboard_module_end(ctx);

        dashboard_terminal_logs(ctx, tk, fonts);
    wlx_layout_end(ctx);
}

// Right grid column: Integrations panel + Theme Lab panel.
static void dashboard_right_column(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                   const Dashboard_Fonts *fonts, Dashboard_Demo_State *st) {
    wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 16,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(204), WLX_SLOT_FLEX(1) });

        dashboard_module_begin(ctx, tk);
            wlx_layout_begin(ctx, 4, WLX_VERT, .padding = 24, .gap = 8,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(28), WLX_SLOT_PX(40),
                                            WLX_SLOT_PX(40), WLX_SLOT_PX(40) });
                dashboard_panel_heading(ctx, tk, fonts, "Integrations");
                dashboard_integration_row(ctx, tk, fonts, "API Reference", WLX_ICON_CODE_XML,
                    DASHBOARD_URL_API_REFERENCE);
                dashboard_integration_row(ctx, tk, fonts, "Quick Start", WLX_ICON_ZAP,
                    DASHBOARD_URL_QUICK_START);
                dashboard_integration_row(ctx, tk, fonts, "GitHub Repo", WLX_ICON_FOLDER_GIT_2,
                    DASHBOARD_URL_GITHUB_REPO);
            wlx_layout_end(ctx);
        dashboard_module_end(ctx);

        // Theme Lab: accent-bordered call-to-action panel whose Launch Lab button
        // opens the Theme Lab section. In dark mode light text sits over the deep
        // surface behind it; in light mode the panel reads as a light accent-bordered
        // card, so the title/subtitle flip to on-surface ink and the accent chrome
        // uses the deep primary (dash_accent_fg). The accent outline is the panel
        // container's own border decor. clip contains the fixed-size Launch Lab button
        // icon and the text when the panel narrows below their natural width.
        WLX_Color lab_accent = dash_accent_fg(tk);
        WLX_Color lab_title  = dash_is_light(tk) ? tk->color.on_surface
                                                 : WLX_RGBA(240, 243, 246, 255);
        WLX_Color lab_sub    = dash_is_light(tk) ? dash_a(tk->color.on_surface_muted, 230)
                                                 : WLX_RGBA(220, 226, 232, 190);
        wlx_layout_begin(ctx, 1, WLX_VERT, .clip = true,
            .border_color = dash_a(lab_accent, 70), .border_width = 1.0f,
            .corner_radius = (float)tk->radius.lg, .rounded_segments = 8);
            // The right column is the narrow 3-of-12 slot, so the headline_md title
            // wraps onto two lines here rather than dropping off the design scale;
            // the title slot is sized for two lines.
            int lab_title_h = 2 * dashboard_type_line_px(tk->type.headline_md);
            int lab_sub_h   = 3 * dashboard_type_line_px(tk->type.body_sm);
            wlx_layout_begin(ctx, 4, WLX_VERT, .padding = 24, .gap = 10,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX((float)lab_title_h),
                                            WLX_SLOT_PX((float)lab_sub_h),
                                            WLX_SLOT_PX(40), WLX_SLOT_FLEX(1) });
                // Explicit break at the word boundary; each line fits the column so
                // the greedy wrap never splits "Laboratory" mid-word.
                wlx_label(ctx, "Theme\nLaboratory",
                    .id = "Theme Laboratory",
                    .style = dashboard_text_style(fonts, tk->type.headline_md, lab_title),
                    .align = WLX_LEFT,
                    .wrap = true,
                    .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                    .border_width = 0);

                wlx_label(ctx, "Explore component states,\ntheme presets, and live\ntheme geometry.",
                    .id = "theme-lab-subtitle",
                    .style = dashboard_text_style(fonts, tk->type.body_sm, lab_sub),
                    .align = WLX_LEFT,
                    .wrap = true,
                    .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                    .border_width = 0);


                if (dashboard_icon_button(ctx, tk, "Launch Lab", WLX_ICON_FLASK_CONICAL,
                        DASHBOARD_ICON_ROLE_ACCENT,
                        .back_color = tk->glass.fill,
                        .front_color = lab_accent,
                        .border_color = lab_accent, .border_width = 1.0f,
                        .texture_tint = lab_accent,
                        .roundness = 0.15f,
                        .image_placement = WLX_IMAGE_PLACEMENT_LEFT, .image_size = 18, .image_text_gap = 8,
                        .font = dashboard_type_font(fonts, tk->type.body_md),
                        .font_size = dashboard_type_px(tk->type.body_md),
                        .height = 36, .id = "launch-lab",
                        .align = WLX_CENTER, .widget_align = WLX_CENTER)) {
                    st->current_view = DASHBOARD_VIEW_THEME_LAB;
                }
                // Image placeholder: a deep cyan -> dark vertical gradient with a
                // faint outline, both expressed as the container's own decor. The
                // gradient draws in place of the fill, the outline on top.
                wlx_layout_begin(ctx, 1, WLX_VERT,
                    .gradient_top = WLX_RGBA(0, 209, 255, 60),
                    .gradient_bottom = WLX_RGBA(10, 15, 20, 255),
                    .border_color = dash_hairline(tk, 13), .border_width = 1.0f,
                    .roundness = 0.06f, .rounded_segments = 8);
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        wlx_layout_end(ctx);
    wlx_layout_end(ctx);
}

// Overview section body: the hero stat row over the 12-column dashboard grid.
// Rendered inside the shell's shared scroll panel.
static void section_overview(WLX_Context *ctx, const Dashboard_Tokens *tk,
                             const Dashboard_Fonts *fonts, Dashboard_Demo_State *st,
                             float phase) {
    (void)phase;
    const Dashboard_Metrics *m = &g_dashboard_metrics;

    // Format the live metrics into per-card buffers. Wollix copies label text into
    // its own arena at emit time, so these stack buffers only need to outlive the
    // build below. The usage bars are relative indicators against soft references;
    // the headline figures are the measured values (or "n/a" where unmeasurable).
    char fps_val[16], draw_val[16], wlx_val[16], rss_val[16];
    const int   bar_segments = 8;
    const float fps_ref      = 60.0f;     // 60 fps fills the bar
    const float draw_ref     = 800.0f;    // soft draw-call reference
    const float wlx_ref_b    = 65536.0f;  // soft 64 KiB arena reference
    const float rss_ref_mb   = 512.0f;    // soft 512 MB process reference

    snprintf(fps_val, sizeof(fps_val), "%.0f", (double)m->fps);
    float fps_frac = m->fps / fps_ref;

    float draw_frac = 0.0f, wlx_frac = 0.0f;
    if (m->perf_available) {
        snprintf(draw_val, sizeof(draw_val), "%d", (int)m->draw_calls);
        snprintf(wlx_val, sizeof(wlx_val), "%.1f", (double)m->wlx_bytes / 1024.0);
        draw_frac = (float)m->draw_calls / draw_ref;
        wlx_frac  = (float)m->wlx_bytes / wlx_ref_b;
    } else {
        snprintf(draw_val, sizeof(draw_val), "n/a");
        snprintf(wlx_val, sizeof(wlx_val), "n/a");
    }

    float rss_frac = 0.0f;
    if (m->rss_available) {
        snprintf(rss_val, sizeof(rss_val), "%.0f", m->rss_mb);
        rss_frac = (float)(m->rss_mb / rss_ref_mb);
    } else {
        snprintf(rss_val, sizeof(rss_val), "n/a");
    }

    wlx_layout_begin(ctx, 2, WLX_VERT, .padding = 32, .gap = 32,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(150), WLX_SLOT_PX(520) });
        // Hero stat row: live, library-honest metrics. FPS comes from the backend
        // timer; draw calls and wollix memory come from the perf frame (or "n/a"
        // in a non-perf build); process RSS is desktop-only ("n/a" on bare-WASM).
        wlx_layout_begin(ctx, 4, WLX_HORZ, .gap = 16);
            dashboard_stat_card(ctx, tk, fonts, "FPS", fps_val, "",
                fps_frac, bar_segments, WLX_ICON_ZAP);
            // "/fr" (per frame) stays inside the unit slot at the four-card width
            // on the larger-text backends where the full "/frame" would clip.
            dashboard_stat_card(ctx, tk, fonts, "Draw Calls", draw_val, "/fr",
                draw_frac, bar_segments, WLX_ICON_SQUARE_PEN);
            dashboard_stat_card(ctx, tk, fonts, "Wlx Memory", wlx_val, "KB",
                wlx_frac, bar_segments, WLX_ICON_DATABASE);
            dashboard_stat_card(ctx, tk, fonts, "Process Rss", rss_val, "MB",
                rss_frac, bar_segments, WLX_ICON_SQUARE_MINUS);
        wlx_layout_end(ctx);
        // 12-column dashboard grid: 3 / 6 / 3.
        wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 16,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(3), WLX_SLOT_FLEX(6), WLX_SLOT_FLEX(3) });
            dashboard_widgets_column(ctx, tk, fonts, st);
            dashboard_center_column(ctx, tk, fonts, st);
            dashboard_right_column(ctx, tk, fonts, st);
        wlx_layout_end(ctx);
    wlx_layout_end(ctx);
}

// ============================================================================
// Showcase section helpers (shared by the Tokens/Components/Layouts/Theme Lab
// bodies below). Every body renders inside the shell's auto-height scroll panel,
// so each top-level block carries an explicit pixel height; the modules below
// follow the dashboard's glass-module + accent-heading recipe.
// ============================================================================

// Total height of a demo module whose interior content occupies `body_h` px:
// module padding (20 top + 20 bottom), the heading slot (20), and the 14 px gap
// between the heading and the body.
static inline float dashboard_module_h(float body_h) { return 74.0f + body_h; }

// Height of a section header block (title + subtitle stacked with an 8 px gap),
// sized to the role line-heights so descenders are never clipped.
static inline float dashboard_header_h(const Dashboard_Tokens *tk) {
    return (float)(dashboard_type_line_px(tk->type.headline_lg)
                 + dashboard_type_line_px(tk->type.body_md) + 8);
}

// Section header: a large accent title over a muted subtitle.
static void dashboard_section_header(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                     const Dashboard_Fonts *fonts,
                                     const char *title, const char *subtitle) {
    int title_h = dashboard_type_line_px(tk->type.headline_lg);
    int sub_h   = dashboard_type_line_px(tk->type.body_md);
    wlx_layout_begin(ctx, 2, WLX_VERT, .id = title, .gap = 8,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX((float)title_h), WLX_SLOT_PX((float)sub_h) });
        wlx_label(ctx, title, .id = title,
            .style = dashboard_text_style(fonts, tk->type.headline_lg, dash_accent_fg(tk)),
            .align = WLX_LEFT, .wrap = false,
            .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
        wlx_label(ctx, subtitle, .id = subtitle,
            .style = dashboard_text_style(fonts, tk->type.body_md, tk->color.on_surface_muted),
            .align = WLX_LEFT, .wrap = false,
            .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
    wlx_layout_end(ctx);
}

// Open a titled glass demo module: the glass surface + accent panel heading,
// leaving a single FLEX body slot the caller fills with one child layout. Pair
// with dashboard_demo_module_end.
static void dashboard_demo_module(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                  const Dashboard_Fonts *fonts, const char *heading) {
    dashboard_module_begin(ctx, tk);
    wlx_layout_begin(ctx, 2, WLX_VERT, .id = heading, .padding = 20, .gap = 14,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(20), WLX_SLOT_FLEX(1) });
        dashboard_panel_heading(ctx, tk, fonts, heading);
}

static void dashboard_demo_module_end(WLX_Context *ctx) {
    wlx_layout_end(ctx);
    dashboard_module_end(ctx);
}

// A muted caption line inside a module body (body_sm, muted). Uses the default
// line-height vertical metric (not WLX_VMETRIC_FONT_SIZE): the rendered line is
// taller than the nominal font size (leading + descenders + the backend font
// scale), and centering by font_size pushed descenders past the bottom of the
// short caption slots, clipping them. Line-height centering keeps the glyph ink
// inside the slot.
static void dashboard_caption(WLX_Context *ctx, const Dashboard_Tokens *tk,
                              const Dashboard_Fonts *fonts, const char *text) {
    wlx_label(ctx, text, .id = text,
        .style = dashboard_text_style(fonts, tk->type.body_sm, dash_a(tk->color.on_surface_muted, 190)),
        .align = WLX_LEFT, .wrap = false,
        .border_width = 0);
}

// A small demo tile: a filled rounded slot with a centered mono caption, used to
// visualize layout slots. Scope repeated tiles with wlx_push_id inside loops.
static void dashboard_demo_tile(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                const Dashboard_Fonts *fonts, const char *text,
                                WLX_Color fill, WLX_Color fg) {
    wlx_label(ctx, text, .id = text,
        .style = dashboard_text_style(fonts, tk->type.body_sm, fg),
        .align = WLX_CENTER, .wrap = false,
        .show_background = true, .back_color = fill,
        .roundness = 0.12f, .rounded_segments = 6,
        .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
}

// A labeled token swatch: a color fill over a small centered name.
static void dashboard_token_swatch(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                   const Dashboard_Fonts *fonts, const char *name,
                                   WLX_Color color) {
    wlx_layout_begin(ctx, 2, WLX_VERT, .id = name, .gap = 6,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_PX(16) });
        wlx_widget(ctx, .id = "sw", .back_color = color,
            .border_color = dash_hairline(tk, 50), .border_width = 1.0f,
            .roundness = 0.18f, .rounded_segments = 6);
        Dashboard_Type_Role r = tk->type.label;
        r.size = 11;
        wlx_label(ctx, name, .id = name,
            .style = dashboard_text_style(fonts, r, tk->color.on_surface_muted),
            .align = WLX_CENTER, .wrap = false,
            .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
    wlx_layout_end(ctx);
}

// ============================================================================
// Section: Tokens -- the design-token model (color roles, surface ramp, type
// scale, spacing and radius scales) rendered straight from the active tokens.
// ============================================================================
static void section_tokens(WLX_Context *ctx, const Dashboard_Tokens *tk,
                           const Dashboard_Fonts *fonts, Dashboard_Demo_State *st,
                           float phase) {
    (void)st;
    (void)phase;
    wlx_layout_begin(ctx, 5, WLX_VERT, .padding = 32, .gap = 24,
        .sizes = (WLX_Slot_Size[]){
            WLX_SLOT_PX(dashboard_header_h(tk)),
            WLX_SLOT_PX(dashboard_module_h(134)),
            WLX_SLOT_PX(dashboard_module_h(60)),
            WLX_SLOT_PX(dashboard_module_h(144)),
            WLX_SLOT_PX(dashboard_module_h(120)) });

        dashboard_section_header(ctx, tk, fonts, "Tokens",
            "Semantic color, type, spacing, and radius tokens.");

        // Color roles: 12 semantic swatches in a 2x6 grid.
        dashboard_demo_module(ctx, tk, fonts, "Color Roles");
            const struct { const char *name; WLX_Color color; } roles[] = {
                { "bg",        tk->color.background },
                { "surface",   tk->color.surface },
                { "variant",   tk->color.surface_variant },
                { "accent",    tk->color.accent },
                { "accent-hi", tk->color.accent_strong },
                { "tertiary",  tk->color.tertiary },
                { "on-surf",   tk->color.on_surface },
                { "on-muted",  tk->color.on_surface_muted },
                { "success",   tk->status.success },
                { "warning",   tk->status.warning },
                { "error",     tk->status.error },
                { "info",      tk->status.info },
            };
            wlx_grid_begin(ctx, 2, 6, .gap = 12);
                for (int i = 0; i < 12; i++) {
                    wlx_push_id(ctx, (size_t)i);
                    dashboard_token_swatch(ctx, tk, fonts, roles[i].name, roles[i].color);
                    wlx_pop_id(ctx);
                }
            wlx_grid_end(ctx);
        dashboard_demo_module_end(ctx);

        // Surface ramp: the five-step elevation ramp lowest..highest.
        dashboard_demo_module(ctx, tk, fonts, "Surface Ramp");
            const struct { const char *name; WLX_Color color; } ramp[] = {
                { "lowest",  tk->ramp.lowest },
                { "low",     tk->ramp.low },
                { "base",    tk->ramp.base },
                { "high",    tk->ramp.high },
                { "highest", tk->ramp.highest },
            };
            wlx_layout_begin(ctx, 5, WLX_HORZ, .gap = 12);
                for (int i = 0; i < 5; i++) {
                    wlx_push_id(ctx, (size_t)i);
                    dashboard_token_swatch(ctx, tk, fonts, ramp[i].name, ramp[i].color);
                    wlx_pop_id(ctx);
                }
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // Type scale: a sample line per role at its real size, with the role name.
        dashboard_demo_module(ctx, tk, fonts, "Type Scale");
            const struct { const char *name; Dashboard_Type_Role role; } types[] = {
                { "headline_lg", tk->type.headline_lg },
                { "headline_md", tk->type.headline_md },
                { "body_md",     tk->type.body_md },
                { "label",       tk->type.label },
            };
            WLX_Slot_Size type_rows[4];
            for (int i = 0; i < 4; i++)
                type_rows[i] = WLX_SLOT_PX((float)dashboard_type_line_px(types[i].role));
            wlx_layout_begin(ctx, 4, WLX_VERT, .gap = 8, .sizes = type_rows);
                for (int i = 0; i < 4; i++) {
                    wlx_push_id(ctx, (size_t)i);
                    wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 12,
                        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(120), WLX_SLOT_FLEX(1) });
                        wlx_label(ctx, types[i].name, .id = "role",
                            .style = dashboard_text_style(fonts, tk->type.label,
                                dash_a(tk->color.on_surface_muted, 170)),
                            .align = WLX_LEFT, .wrap = false,
                            .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                        wlx_label(ctx, "Mechanical Glass 0123", .id = "sample",
                            .style = dashboard_text_style(fonts, types[i].role, tk->color.on_surface),
                            .align = WLX_LEFT, .wrap = false,
                            .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                    wlx_layout_end(ctx);
                    wlx_pop_id(ctx);
                }
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // Spacing scale (left, proportional bars) + radius scale (right, samples).
        dashboard_demo_module(ctx, tk, fonts, "Spacing & Radius");
            wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 24,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1) });
                // Spacing: one bar per step, width proportional to the value.
                const struct { const char *name; int value; } space[] = {
                    { "xs  4",  tk->spacing.xs },  { "sm  8",  tk->spacing.sm },
                    { "md 12",  tk->spacing.md },  { "lg 16",  tk->spacing.lg },
                    { "xl 24",  tk->spacing.xl },  { "xxl 32", tk->spacing.xxl },
                };
                wlx_layout_begin(ctx, 6, WLX_VERT, .gap = 4, .id = "space-scale");
                    for (int i = 0; i < 6; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 8,
                            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(54), WLX_SLOT_FLEX(1) });
                            wlx_label(ctx, space[i].name, .id = "sp",
                                .style = dashboard_text_style(fonts, tk->type.label,
                                    tk->color.on_surface_muted),
                                .align = WLX_LEFT, .wrap = false,
                                .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                            wlx_widget(ctx, .id = "bar", .widget_align = WLX_LEFT,
                                .width = (float)space[i].value * 4.0f, .height = 12,
                                .back_color = dash_a(tk->color.accent, dash_is_light(tk) ? 70 : 130),
                                .roundness = 0.3f, .rounded_segments = 4, .border_width = 0);
                        wlx_layout_end(ctx);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);
                // Radius: a sample box per step, with an increasing corner radius.
                const struct { const char *name; int value; } radii[] = {
                    { "sm",   tk->radius.sm },   { "base", tk->radius.base },
                    { "md",   tk->radius.md },   { "lg",   tk->radius.lg },
                    { "xl",   tk->radius.xl },
                };
                wlx_layout_begin(ctx, 5, WLX_HORZ, .gap = 8, .id = "radius-scale");
                    for (int i = 0; i < 5; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 6,
                            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_PX(16) });
                            wlx_widget(ctx, .id = "rbox",
                                .back_color = dash_a(tk->color.accent, dash_is_light(tk) ? 60 : 110),
                                .border_color = dash_accent_fg(tk), .border_width = 1.0f,
                                .corner_radius = (float)radii[i].value, .rounded_segments = 8);
                            wlx_label(ctx, radii[i].name, .id = "rl",
                                .style = dashboard_text_style(fonts, tk->type.label,
                                    tk->color.on_surface_muted),
                                .align = WLX_CENTER, .wrap = false,
                                .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                        wlx_layout_end(ctx);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);
    wlx_layout_end(ctx);
}

// ============================================================================
// Section: Components -- the public widget set (label, button, selection
// controls, slider, progress, input, image, widget) under the glass recipes.
// ============================================================================
static void section_components(WLX_Context *ctx, const Dashboard_Tokens *tk,
                               const Dashboard_Fonts *fonts, Dashboard_Demo_State *st,
                               float phase) {
    (void)phase;
    WLX_Font  body_font = dashboard_type_font(fonts, tk->type.body_md);
    int       body_px   = dashboard_type_px(tk->type.body_md);

    wlx_layout_begin(ctx, 8, WLX_VERT, .padding = 32, .gap = 24,
        .sizes = (WLX_Slot_Size[]){
            WLX_SLOT_PX(dashboard_header_h(tk)),
            WLX_SLOT_PX(dashboard_module_h(160)),   // labels
            WLX_SLOT_PX(dashboard_module_h(98)),    // buttons
            WLX_SLOT_PX(dashboard_module_h(140)),   // selection
            WLX_SLOT_PX(dashboard_module_h(196)),   // sliders & progress
            WLX_SLOT_PX(dashboard_module_h(136)),   // inputs
            WLX_SLOT_PX(dashboard_module_h(110)),   // image & widget
            WLX_SLOT_PX(dashboard_module_h(40)) }); // status chips

        dashboard_section_header(ctx, tk, fonts, "Components",
            "Labels, buttons, inputs, and the rest of the widget set.");

        // -- Labels: alignment, wrapping, color, icon-backed --
        dashboard_demo_module(ctx, tk, fonts, "Label");
            wlx_layout_begin(ctx, 3, WLX_VERT, .gap = 12,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(44), WLX_SLOT_PX(52), WLX_SLOT_PX(40) });
                const char *al_names[] = { "Left", "Center", "Right" };
                WLX_Align   al_vals[]  = { WLX_LEFT, WLX_CENTER, WLX_RIGHT };
                wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 12);
                    for (int i = 0; i < 3; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        wlx_label(ctx, al_names[i], .id = "al",
                            .style = dashboard_text_style(fonts, tk->type.body_md, tk->color.on_surface),
                            .align = al_vals[i], .wrap = false, .show_background = true,
                            .back_color = dash_a(tk->color.on_surface_muted, 24),
                            .roundness = 0.12f, .rounded_segments = 6,
                            .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);
                wlx_label(ctx, "Wrapped paragraph: the label widget wraps long text to the "
                    "available width, with per-call alignment and color.", .id = "wrap",
                    .style = dashboard_text_style(fonts, tk->type.body_sm, tk->color.on_surface_muted),
                    .align = WLX_LEFT, .wrap = true,
                    .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                // Icon-backed status row.
                wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 12);
                    dashboard_icon_label(ctx, tk, "Saved", WLX_ICON_CHECK, DASHBOARD_ICON_ROLE_SUCCESS,
                        .id = "saved",
                        .style = dashboard_text_style(fonts, tk->type.body_md, tk->color.on_surface),
                        .align = WLX_LEFT, .image_size = 18, .image_text_gap = 8,
                        .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                    dashboard_icon_label(ctx, tk, "Details", WLX_ICON_INFO, DASHBOARD_ICON_ROLE_ACCENT,
                        .id = "details",
                        .style = dashboard_text_style(fonts, tk->type.body_md, tk->color.on_surface),
                        .align = WLX_LEFT, .image_size = 18, .image_text_gap = 8,
                        .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                    dashboard_icon_label(ctx, tk, "Heads up", WLX_ICON_TRIANGLE_ALERT,
                        DASHBOARD_ICON_ROLE_WARNING, .id = "warn",
                        .style = dashboard_text_style(fonts, tk->type.body_md, tk->color.on_surface),
                        .align = WLX_LEFT, .image_size = 18, .image_text_gap = 8,
                        .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Buttons: text + icon placements + a click counter --
        dashboard_demo_module(ctx, tk, fonts, "Button");
            wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 12,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(42), WLX_SLOT_PX(44) });
                wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 12);
                    if (dashboard_button_primary(ctx, tk, fonts, "Primary", 42, "btn-primary"))
                        st->comp_clicks++;
                    if (wlx_button(ctx, "Neutral", .id = "btn-neutral", .height = 42,
                            .back_color = tk->color.surface_variant, .front_color = tk->color.on_surface,
                            .roundness = 0.15f, .font = body_font, .font_size = body_px,
                            .align = WLX_CENTER))
                        st->comp_clicks++;
                    if (wlx_button(ctx, "Outline", .id = "btn-outline", .height = 42,
                            .back_color = (WLX_Color){0}, .front_color = dash_accent_fg(tk),
                            .border_color = dash_accent_fg(tk), .border_width = 1.0f,
                            .roundness = 0.15f, .font = body_font, .font_size = body_px,
                            .align = WLX_CENTER))
                        st->comp_clicks++;
                wlx_layout_end(ctx);
                wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 12);
                    if (dashboard_icon_button(ctx, tk, "Save", WLX_ICON_SAVE, DASHBOARD_ICON_ROLE_ACCENT,
                            .id = "btn-save", .height = 44, .align = WLX_CENTER,
                            .back_color = tk->color.surface_variant, .front_color = tk->color.on_surface,
                            .roundness = 0.15f, .font = body_font, .font_size = body_px,
                            .image_placement = WLX_IMAGE_PLACEMENT_LEFT, .image_size = 18))
                        st->comp_clicks++;
                    if (dashboard_icon_button(ctx, tk, "Settings", WLX_ICON_SETTINGS,
                            DASHBOARD_ICON_ROLE_MUTED, .id = "btn-settings", .height = 44, .align = WLX_CENTER,
                            .back_color = tk->color.surface_variant, .front_color = tk->color.on_surface,
                            .roundness = 0.15f, .font = body_font, .font_size = body_px,
                            .image_placement = WLX_IMAGE_PLACEMENT_RIGHT, .image_size = 18))
                        st->comp_clicks++;
                    char clicks[32];
                    snprintf(clicks, sizeof(clicks), "Clicks: %d", st->comp_clicks);
                    wlx_label(ctx, clicks, .id = "clicks",
                        .style = dashboard_text_style(fonts, tk->type.body_md, tk->color.on_surface_muted),
                        .align = WLX_CENTER, .wrap = false,
                        .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Selection: checkbox / toggle / radio in three columns --
        dashboard_demo_module(ctx, tk, fonts, "Selection Controls");
            wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 24,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1) });
                // Checkboxes.
                wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 8,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(18), WLX_SLOT_FLEX(1) });
                    dashboard_caption(ctx, tk, fonts, "Checkbox");
                    wlx_layout_begin(ctx, 3, WLX_VERT, .gap = 6, .id = "checks");
                        const char *cb[] = { "Audio", "V-Sync", "Fullscreen" };
                        for (int i = 0; i < 3; i++) {
                            wlx_push_id(ctx, (size_t)i);
                            wlx_checkbox(ctx, cb[i], &st->comp_checks[i], .id = cb[i],
                                .height = 34, .font = body_font, .font_size = body_px);
                            wlx_pop_id(ctx);
                        }
                    wlx_layout_end(ctx);
                wlx_layout_end(ctx);
                // Toggles.
                wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 8,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(18), WLX_SLOT_FLEX(1) });
                    dashboard_caption(ctx, tk, fonts, "Toggle");
                    wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 6, .id = "toggles");
                        wlx_toggle(ctx, "Notifications", &st->comp_toggle_a, .id = "tog-a",
                            .height = 34, .font = body_font, .font_size = body_px);
                        wlx_toggle(ctx, "Telemetry", &st->comp_toggle_b, .id = "tog-b",
                            .height = 34, .font = body_font, .font_size = body_px);
                    wlx_layout_end(ctx);
                wlx_layout_end(ctx);
                // Radios.
                wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 8,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(18), WLX_SLOT_FLEX(1) });
                    dashboard_caption(ctx, tk, fonts, "Radio");
                    wlx_layout_begin(ctx, 3, WLX_VERT, .gap = 6, .id = "radios");
                        const char *rb[] = { "Small", "Medium", "Large" };
                        for (int i = 0; i < 3; i++) {
                            wlx_push_id(ctx, (size_t)i);
                            wlx_radio(ctx, rb[i], &st->comp_radio, i, .id = rb[i],
                                .height = 34, .font = body_font, .font_size = body_px);
                            wlx_pop_id(ctx);
                        }
                    wlx_layout_end(ctx);
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Sliders & progress --
        dashboard_demo_module(ctx, tk, fonts, "Sliders & Progress");
            wlx_layout_begin(ctx, 6, WLX_VERT, .gap = 8,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(16), WLX_SLOT_PX(38), WLX_SLOT_PX(38),
                                            WLX_SLOT_PX(16), WLX_SLOT_PX(18), WLX_SLOT_PX(26) });
                dashboard_caption(ctx, tk, fonts, "Slider");
                wlx_slider(ctx, "Value ", &st->comp_slider, .id = "sld-value",
                    .height = 38, .font = body_font, .font_size = body_px,
                    .min_value = 0.0f, .max_value = 1.0f, .show_value = true);
                wlx_slider(ctx, "Styled", &st->comp_styled_slider, .id = "sld-styled",
                    .height = 38, .font = body_font, .font_size = body_px,
                    .min_value = 0.0f, .max_value = 1.0f, .show_value = true,
                    .track_height = 8.0f, .thumb_width = 18.0f, .roundness = 0.5f, .rounded_segments = 8);
                dashboard_caption(ctx, tk, fonts, "Progress");
                wlx_progress(ctx, st->comp_styled_slider, .id = "prog", .height = 18);
                dashboard_segmented_progress(ctx, tk, st->comp_styled_slider, 12);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Inputs --
        dashboard_demo_module(ctx, tk, fonts, "Input Box");
            wlx_layout_begin(ctx, 3, WLX_VERT, .gap = 8,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(40), WLX_SLOT_PX(40), WLX_SLOT_PX(40) });
                wlx_inputbox(ctx, "Name:  ", st->comp_name, sizeof(st->comp_name), .id = "in-name",
                    .height = 40, .content_padding = 6, .font = body_font, .font_size = body_px,
                    .back_color = tk->color.field, .border_color = tk->color.field_border,
                    .border_width = 1.0f, .roundness = 0.12f);
                wlx_inputbox(ctx, "Email: ", st->comp_email, sizeof(st->comp_email), .id = "in-email",
                    .height = 40, .content_padding = 6, .font = body_font, .font_size = body_px,
                    .back_color = tk->color.field, .border_color = tk->color.field_border,
                    .border_width = 1.0f, .roundness = 0.12f,
                    .border_focus_color = dash_accent_fg(tk), .cursor_color = dash_accent_fg(tk));
                if (wlx_button(ctx, "Clear", .id = "in-clear", .height = 40,
                        .back_color = tk->status.error, .front_color = WLX_RGBA(255, 255, 255, 255),
                        .roundness = 0.15f, .font = body_font, .font_size = body_px, .align = WLX_CENTER)) {
                    st->comp_name[0] = '\0';
                    st->comp_email[0] = '\0';
                }
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Image (atlas) & widget (colored rect) --
        dashboard_demo_module(ctx, tk, fonts, "Image & Widget");
            wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 24,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1) });
                // Image: three atlas cells, FIT and tinted.
                wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 8,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(16), WLX_SLOT_FLEX(1) });
                    dashboard_caption(ctx, tk, fonts, "Image (atlas)");
                    wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 12, .id = "img-row");
                        dashboard_icon_image(ctx, tk, WLX_ICON_IMAGE, DASHBOARD_ICON_ROLE_ON_SURFACE,
                            40.0f, .id = "img-a", .width = 40, .height = 40, .widget_align = WLX_CENTER);
                        dashboard_icon_image(ctx, tk, WLX_ICON_PALETTE, DASHBOARD_ICON_ROLE_ACCENT,
                            40.0f, .id = "img-b", .width = 40, .height = 40, .widget_align = WLX_CENTER);
                        dashboard_icon_image(ctx, tk, WLX_ICON_DATABASE, DASHBOARD_ICON_ROLE_TERTIARY,
                            40.0f, .id = "img-c", .width = 40, .height = 40, .widget_align = WLX_CENTER);
                    wlx_layout_end(ctx);
                wlx_layout_end(ctx);
                // Widget: swatch + divider.
                wlx_layout_begin(ctx, 3, WLX_VERT, .gap = 8,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(16), WLX_SLOT_PX(40), WLX_SLOT_FLEX(1) });
                    dashboard_caption(ctx, tk, fonts, "Widget (rect / divider)");
                    wlx_widget(ctx, .id = "swatch", .widget_align = WLX_LEFT, .width = 120, .height = 36,
                        .back_color = tk->color.accent, .roundness = 0.15f, .rounded_segments = 6,
                        .border_width = 0);
                    wlx_widget(ctx, .id = "divider", .widget_align = WLX_CENTER, .width = -1, .height = 2,
                        .back_color = dash_hairline(tk, 60), .border_width = 0);
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Status chips / badges / pip (dashboard recipes) --
        dashboard_demo_module(ctx, tk, fonts, "Status Chips");
            wlx_layout_begin(ctx, 4, WLX_HORZ, .gap = 12,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1),
                                            WLX_SLOT_PX(40) });
                dashboard_status_chip(ctx, tk, fonts, "Online", tk->status.success);
                dashboard_status_chip(ctx, tk, fonts, "Degraded", tk->status.warning);
                dashboard_badge(ctx, tk, fonts, "v0.6", tk->color.accent, tk->color.on_accent);
                dashboard_status_pip(ctx, tk, tk->status.success, phase);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);
    wlx_layout_end(ctx);
}

// ============================================================================
// Section: Layouts -- structural capabilities (linear, grid, flex, auto layout,
// opacity, ID stack, borders, scroll panel) demonstrated as glass modules.
// ============================================================================
static void section_layouts(WLX_Context *ctx, const Dashboard_Tokens *tk,
                             const Dashboard_Fonts *fonts, Dashboard_Demo_State *st,
                             float phase) {
    (void)phase;
    WLX_Font  body_font = dashboard_type_font(fonts, tk->type.body_md);
    int       body_px   = dashboard_type_px(tk->type.body_md);
    WLX_Color tile_a    = dash_a(tk->color.accent, 26);
    WLX_Color tile_a_fg = dash_accent_fg(tk);
    WLX_Color tile_n    = dash_a(tk->color.on_surface_muted, 28);
    WLX_Color tile_n_fg = tk->color.on_surface;

    wlx_layout_begin(ctx, 9, WLX_VERT, .padding = 32, .gap = 24,
        .sizes = (WLX_Slot_Size[]){
            WLX_SLOT_PX(dashboard_header_h(tk)),
            WLX_SLOT_PX(dashboard_module_h(136)),   // linear
            WLX_SLOT_PX(dashboard_module_h(264)),   // grid
            WLX_SLOT_PX(dashboard_module_h(152)),   // flex
            WLX_SLOT_PX(dashboard_module_h(140)),   // auto layout
            WLX_SLOT_PX(dashboard_module_h(184)),   // opacity
            WLX_SLOT_PX(dashboard_module_h(210)),   // id stack
            WLX_SLOT_PX(dashboard_module_h(140)),   // borders
            WLX_SLOT_PX(dashboard_module_h(170)) });// scroll panel

        dashboard_section_header(ctx, tk, fonts, "Layouts",
            "Linear, grid, flex, auto layout, opacity, ID stack, and scrolling.");

        // -- Linear: equal HORZ slots, then mixed PX/PCT/FLEX --
        dashboard_demo_module(ctx, tk, fonts, "Linear Layout");
            wlx_layout_begin(ctx, 4, WLX_VERT, .gap = 8,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(16), WLX_SLOT_PX(40),
                                            WLX_SLOT_PX(16), WLX_SLOT_PX(40) });
                dashboard_caption(ctx, tk, fonts, "Horizontal (equal slots)");
                wlx_layout_begin(ctx, 4, WLX_HORZ, .gap = 8, .id = "lin-eq");
                    for (int i = 0; i < 4; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        char buf[8];
                        snprintf(buf, sizeof(buf), "%d", i + 1);
                        dashboard_demo_tile(ctx, tk, fonts, buf,
                            (i % 2 == 0) ? tile_a : tile_n, (i % 2 == 0) ? tile_a_fg : tile_n_fg);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);
                dashboard_caption(ctx, tk, fonts, "Mixed (PX + PCT + FLEX)");
                wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 8,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(110), WLX_SLOT_PCT(30), WLX_SLOT_FLEX(1) });
                    dashboard_demo_tile(ctx, tk, fonts, "PX(110)", tile_n, tile_n_fg);
                    dashboard_demo_tile(ctx, tk, fonts, "PCT(30)", tile_a, tile_a_fg);
                    dashboard_demo_tile(ctx, tk, fonts, "FLEX(1)", tile_n, tile_n_fg);
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Grid: auto grid, then cell positioning with spans --
        dashboard_demo_module(ctx, tk, fonts, "Grid Layout");
            wlx_layout_begin(ctx, 4, WLX_VERT, .gap = 8,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(16), WLX_SLOT_PX(88),
                                            WLX_SLOT_PX(16), WLX_SLOT_PX(120) });
                dashboard_caption(ctx, tk, fonts, "Auto grid (wlx_grid_begin_auto)");
                wlx_grid_begin_auto(ctx, 4, 40, .gap = 8);
                    for (int i = 0; i < 8; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        char buf[12];
                        snprintf(buf, sizeof(buf), "Cell %d", i + 1);
                        dashboard_demo_tile(ctx, tk, fonts, buf,
                            (i % 2 == 0) ? tile_n : tile_a, (i % 2 == 0) ? tile_n_fg : tile_a_fg);
                        wlx_pop_id(ctx);
                    }
                wlx_grid_end(ctx);
                dashboard_caption(ctx, tk, fonts, "Cell positioning (wlx_grid_cell, spans)");
                wlx_grid_begin(ctx, 3, 3, .gap = 8);
                    wlx_grid_cell(ctx, 0, 0, .col_span = 2);
                    dashboard_demo_tile(ctx, tk, fonts, "span 2 cols", tile_a, tile_a_fg);
                    wlx_grid_cell(ctx, 0, 2, .row_span = 2);
                    dashboard_demo_tile(ctx, tk, fonts, "span 2 rows", tile_n, tile_n_fg);
                    wlx_grid_cell(ctx, 1, 0);
                    dashboard_demo_tile(ctx, tk, fonts, "1,0", tile_n, tile_n_fg);
                    wlx_grid_cell(ctx, 1, 1);
                    dashboard_demo_tile(ctx, tk, fonts, "1,1", tile_a, tile_a_fg);
                    wlx_grid_cell(ctx, 2, 0, .col_span = 3);
                    dashboard_demo_tile(ctx, tk, fonts, "full width (span 3)", tile_n, tile_n_fg);
                wlx_grid_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Flex & sizing: weight distribution + min/max constraints --
        dashboard_demo_module(ctx, tk, fonts, "Flex & Sizing");
            wlx_layout_begin(ctx, 4, WLX_VERT, .gap = 8,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(16), WLX_SLOT_PX(48),
                                            WLX_SLOT_PX(16), WLX_SLOT_PX(48) });
                dashboard_caption(ctx, tk, fonts, "Flex weights (1 : 3)");
                wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 8,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(3) });
                    dashboard_demo_tile(ctx, tk, fonts, "FLEX(1)", tile_a, tile_a_fg);
                    dashboard_demo_tile(ctx, tk, fonts, "FLEX(3)", tile_n, tile_n_fg);
                wlx_layout_end(ctx);
                dashboard_caption(ctx, tk, fonts, "Min/max (FLEX_MINMAX + PX)");
                wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 8,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX_MINMAX(1, 80, 250), WLX_SLOT_PX(120) });
                    dashboard_demo_tile(ctx, tk, fonts, "FLEX_MINMAX(1,80,250)", tile_n, tile_n_fg);
                    dashboard_demo_tile(ctx, tk, fonts, "PX(120)", tile_a, tile_a_fg);
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Auto layout: variable-size vertical slots + a flex footer --
        dashboard_demo_module(ctx, tk, fonts, "Auto Layout");
            wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 8,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(16), WLX_SLOT_FLEX(1) });
                dashboard_caption(ctx, tk, fonts, "wlx_layout_begin_auto + auto slots");
                // The auto layout fills the flex slot; only widgets (no nested
                // layouts) may sit inside an auto layout.
                wlx_layout_begin_auto(ctx, WLX_VERT, 0);
                    for (int i = 0; i < 3; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        char buf[20];
                        snprintf(buf, sizeof(buf), "PX(28) item %d", i + 1);
                        wlx_layout_auto_slot(ctx, WLX_SLOT_PX(28));
                        dashboard_demo_tile(ctx, tk, fonts, buf, tile_n, tile_n_fg);
                        wlx_pop_id(ctx);
                    }
                    wlx_layout_auto_slot(ctx, WLX_SLOT_FLEX_MIN(1, 28));
                    dashboard_demo_tile(ctx, tk, fonts, "FLEX_MIN(1,28) footer", tile_a, tile_a_fg);
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Opacity: per-widget control + the push/pop opacity stack --
        dashboard_demo_module(ctx, tk, fonts, "Opacity");
            wlx_layout_begin(ctx, 5, WLX_VERT, .gap = 8,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(34), WLX_SLOT_PX(16),
                                            WLX_SLOT_PX(40), WLX_SLOT_PX(16), WLX_SLOT_PX(44) });
                wlx_slider(ctx, "Widget", &st->opacity_widget, .id = "op-widget",
                    .height = 34, .font = body_font, .font_size = body_px,
                    .min_value = 0.0f, .max_value = 1.0f, .show_value = true);
                dashboard_caption(ctx, tk, fonts, "Per-widget .opacity");
                wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 8, .id = "op-row");
                    wlx_label(ctx, "Label", .id = "op-label",
                        .style = dashboard_text_style(fonts, tk->type.body_md, tk->color.on_surface),
                        .align = WLX_CENTER, .show_background = true, .back_color = tile_a,
                        .roundness = 0.12f, .rounded_segments = 6,
                        .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0,
                        .opacity = st->opacity_widget);
                    wlx_button(ctx, "Button", .id = "op-btn", .height = 40, .align = WLX_CENTER,
                        .font = body_font, .font_size = body_px, .opacity = st->opacity_widget);
                    wlx_widget(ctx, .id = "op-rect", .widget_align = WLX_CENTER, .width = -1, .height = 40,
                        .back_color = tk->color.accent, .roundness = 0.12f, .rounded_segments = 6,
                        .border_width = 0, .opacity = st->opacity_widget);
                wlx_layout_end(ctx);
                dashboard_caption(ctx, tk, fonts, "Context stack (push_opacity, single vs nested)");
                wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 8, .id = "op-stack");
                    wlx_push_id(ctx, 1);
                    wlx_push_opacity(ctx, st->opacity_widget);
                        dashboard_demo_tile(ctx, tk, fonts, "single push", tile_n, tile_n_fg);
                    wlx_pop_opacity(ctx);
                    wlx_pop_id(ctx);
                    wlx_push_id(ctx, 2);
                    wlx_push_opacity(ctx, st->opacity_widget);
                    wlx_push_opacity(ctx, 0.5f);
                        dashboard_demo_tile(ctx, tk, fonts, "nested (x 0.5)", tile_a, tile_a_fg);
                    wlx_pop_opacity(ctx);
                    wlx_pop_opacity(ctx);
                    wlx_pop_id(ctx);
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- ID stack: loop-generated rows with stable IDs --
        dashboard_demo_module(ctx, tk, fonts, "ID Stack");
            wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 8,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(16), WLX_SLOT_FLEX(1) });
                dashboard_caption(ctx, tk, fonts, "wlx_push_id / wlx_pop_id per row");
                WLX_Slot_Size id_rows[DASHBOARD_IDSTACK_ROWS];
                for (int i = 0; i < DASHBOARD_IDSTACK_ROWS; i++) id_rows[i] = WLX_SLOT_PX(40);
                wlx_layout_begin(ctx, DASHBOARD_IDSTACK_ROWS, WLX_VERT, .gap = 8, .sizes = id_rows);
                    for (int i = 0; i < DASHBOARD_IDSTACK_ROWS; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 8,
                            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(2), WLX_SLOT_FLEX(2),
                                                        WLX_SLOT_FLEX(1) });
                            wlx_inputbox(ctx, "", st->idstack_names[i], sizeof(st->idstack_names[i]),
                                .id = "name", .height = 36, .content_padding = 6,
                                .font = body_font, .font_size = body_px,
                                .back_color = tk->color.field, .border_color = tk->color.field_border,
                                .border_width = 1.0f, .roundness = 0.12f);
                            wlx_slider(ctx, "", &st->idstack_values[i], .id = "val",
                                .height = 36, .font = body_font, .font_size = body_px,
                                .min_value = 0.0f, .max_value = 1.0f, .show_value = false);
                            wlx_checkbox(ctx, "On", &st->idstack_enabled[i], .id = "on",
                                .height = 36, .font = body_font, .font_size = body_px);
                        wlx_layout_end(ctx);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Borders & radii --
        dashboard_demo_module(ctx, tk, fonts, "Borders & Radii");
            wlx_layout_begin(ctx, 4, WLX_VERT, .gap = 8,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(16), WLX_SLOT_PX(40),
                                            WLX_SLOT_PX(16), WLX_SLOT_PX(44) });
                dashboard_caption(ctx, tk, fonts, "Border width (1 / 2 / 4 px)");
                wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 12, .id = "bdr-row");
                    float widths[3] = { 1.0f, 2.0f, 4.0f };
                    const char *wl[3] = { "1px", "2px", "4px" };
                    for (int i = 0; i < 3; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        wlx_label(ctx, wl[i], .id = "bw",
                            .style = dashboard_text_style(fonts, tk->type.body_md, tk->color.on_surface),
                            .align = WLX_CENTER, .show_background = true, .back_color = tk->glass.fill,
                            .border_color = dash_accent_fg(tk), .border_width = widths[i],
                            .roundness = 0.12f, .rounded_segments = 6,
                            .vertical_metric = WLX_VMETRIC_FONT_SIZE);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);
                dashboard_caption(ctx, tk, fonts, "Corner radius (per-token)");
                wlx_layout_begin(ctx, 4, WLX_HORZ, .gap = 12, .id = "rad-row");
                    float radii[4] = { (float)tk->radius.base, (float)tk->radius.md,
                                       (float)tk->radius.lg, (float)tk->radius.xl };
                    for (int i = 0; i < 4; i++) {
                        wlx_push_id(ctx, (size_t)i);
                        wlx_widget(ctx, .id = "rb",
                            .back_color = dash_a(tk->color.accent, dash_is_light(tk) ? 60 : 110),
                            .border_color = dash_accent_fg(tk), .border_width = 1.0f,
                            .corner_radius = radii[i], .rounded_segments = 8);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Scroll panel: a nested, fixed-height scrolling list --
        dashboard_demo_module(ctx, tk, fonts, "Scroll Panel");
            wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 8,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(16), WLX_SLOT_FLEX(1) });
                dashboard_caption(ctx, tk, fonts, "Nested auto-height scroll panel (24 items)");
                wlx_scroll_panel_begin(ctx, WLX_SCROLL_AUTO_HEIGHT,
                    .back_color = dash_is_light(tk) ? tk->ramp.low : tk->ramp.lowest,
                    .scrollbar_color = tk->color.field_border);
                    wlx_layout_begin_auto(ctx, WLX_VERT, 30);
                        for (int i = 0; i < 24; i++) {
                            wlx_push_id(ctx, (size_t)i);
                            char buf[24];
                            snprintf(buf, sizeof(buf), "  Row %d", i + 1);
                            WLX_Color rbg = (i % 2 == 0) ? dash_a(tk->color.on_surface_muted, 18)
                                                         : (WLX_Color){0};
                            wlx_label(ctx, buf, .id = "row",
                                .style = dashboard_text_style(fonts, tk->type.body_sm, tk->color.on_surface),
                                .align = WLX_LEFT, .show_background = (rbg.a != 0), .back_color = rbg,
                                .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                            wlx_pop_id(ctx);
                        }
                    wlx_layout_end(ctx);
                wlx_scroll_panel_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);
    wlx_layout_end(ctx);
}

// ============================================================================
// Section: Theme Lab -- the theme system shown through a component-state matrix,
// built-in preset snapshots, and a live geometry readout. (A mutable theme
// editor is out of scope for v1.)
// ============================================================================

// One built-in theme preset snapshot card: name over background/surface/accent
// swatches drawn straight from the preset's WLX_Theme.
static void dashboard_preset_card(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                  const Dashboard_Fonts *fonts, const char *name,
                                  const WLX_Theme *preset) {
    wlx_layout_begin(ctx, 2, WLX_VERT, .id = name, .padding = 12, .gap = 10,
        .back_color = tk->glass.fill, .corner_radius = (float)tk->radius.lg, .rounded_segments = 8,
        .border_color = dash_hairline(tk, 26), .border_width = 1.0f,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(18), WLX_SLOT_FLEX(1) });
        wlx_label(ctx, name, .id = name,
            .style = dashboard_text_style(fonts, tk->type.label, tk->color.on_surface),
            .align = WLX_LEFT, .wrap = false,
            .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
        wlx_layout_begin(ctx, 3, WLX_HORZ, .gap = 6, .id = "sw");
            wlx_widget(ctx, .id = "bg", .back_color = preset->background,
                .border_color = dash_hairline(tk, 50), .border_width = 1.0f,
                .roundness = 0.2f, .rounded_segments = 6);
            wlx_widget(ctx, .id = "surf", .back_color = preset->surface,
                .border_color = dash_hairline(tk, 50), .border_width = 1.0f,
                .roundness = 0.2f, .rounded_segments = 6);
            wlx_widget(ctx, .id = "acc", .back_color = preset->accent,
                .border_color = dash_hairline(tk, 50), .border_width = 1.0f,
                .roundness = 0.2f, .rounded_segments = 6);
        wlx_layout_end(ctx);
    wlx_layout_end(ctx);
}

static void section_theme_lab(WLX_Context *ctx, const Dashboard_Tokens *tk,
                              const Dashboard_Fonts *fonts, Dashboard_Demo_State *st,
                              float phase) {
    (void)phase;
    WLX_Font  body_font = dashboard_type_font(fonts, tk->type.body_md);
    int       body_px   = dashboard_type_px(tk->type.body_md);

    wlx_layout_begin(ctx, 4, WLX_VERT, .padding = 32, .gap = 24,
        .sizes = (WLX_Slot_Size[]){
            WLX_SLOT_PX(dashboard_header_h(tk)),
            WLX_SLOT_PX(dashboard_module_h(266)),   // component-state matrix
            WLX_SLOT_PX(dashboard_module_h(120)),   // presets
            WLX_SLOT_PX(dashboard_module_h(110)) });// geometry readout

        dashboard_section_header(ctx, tk, fonts, "Theme Lab",
            "Component states, built-in presets, and the live theme geometry.");

        // -- Component-state matrix: rows of widgets across illustrative states.
        // Hover/active/focus cannot be forced programmatically, so the columns
        // tint to illustrate each state; the Disabled column passes .disabled.
        dashboard_demo_module(ctx, tk, fonts, "Component States");
            const char *states[] = { "Default", "Hover", "Active", "Focus", "Disabled" };
            WLX_Color st_bg[5] = {
                tk->color.surface, tk->color.surface_variant, dash_a(tk->color.accent, 38),
                tk->color.surface, tk->color.surface };
            WLX_Color st_bd[5] = {
                tk->color.outline_variant, tk->color.outline_variant, dash_accent_fg(tk),
                dash_accent_fg(tk), tk->color.outline_variant };
            float st_bw[5] = { 1.0f, 1.0f, 1.0f, 2.0f, 1.0f };
            // 6 widget rows + 1 header row.
            wlx_layout_begin(ctx, 7, WLX_VERT, .gap = 6,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(18), WLX_SLOT_PX(34), WLX_SLOT_PX(34),
                    WLX_SLOT_PX(34), WLX_SLOT_PX(34), WLX_SLOT_PX(34), WLX_SLOT_PX(34) });
                // Header row of state names.
                wlx_layout_begin(ctx, 5, WLX_HORZ, .gap = 8, .id = "states-head");
                    for (int s = 0; s < 5; s++) {
                        wlx_push_id(ctx, (size_t)s);
                        wlx_label(ctx, states[s], .id = "h",
                            .style = dashboard_text_style(fonts, tk->type.label,
                                tk->color.on_surface_muted),
                            .align = WLX_CENTER, .wrap = false,
                            .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                        wlx_pop_id(ctx);
                    }
                wlx_layout_end(ctx);
                for (int row = 0; row < 6; row++) {
                    wlx_push_id(ctx, (size_t)(100 + row));
                    wlx_layout_begin(ctx, 5, WLX_HORZ, .gap = 8, .id = "states-row");
                        for (int s = 0; s < 5; s++) {
                            bool disabled = (s == 4);
                            wlx_push_id(ctx, (size_t)s);
                            switch (row) {
                                case 0:
                                    wlx_label(ctx, "Label", .id = "w",
                                        .style = dashboard_text_style(fonts, tk->type.body_sm,
                                            tk->color.on_surface),
                                        .align = WLX_CENTER, .show_background = true, .back_color = st_bg[s],
                                        .border_color = st_bd[s], .border_width = st_bw[s],
                                        .roundness = 0.12f, .rounded_segments = 6,
                                        .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                                        .opacity = disabled ? 0.45f : 1.0f);
                                    break;
                                case 1:
                                    wlx_button(ctx, "Button", .id = "w", .height = 30, .align = WLX_CENTER,
                                        .back_color = st_bg[s], .front_color = tk->color.on_surface,
                                        .border_color = st_bd[s], .border_width = st_bw[s],
                                        .roundness = 0.12f, .font = body_font, .font_size = body_px,
                                        .disabled = disabled);
                                    break;
                                case 2:
                                    wlx_checkbox(ctx, "Check", &st->lab_checks[s], .id = "w",
                                        .height = 30, .font = body_font, .font_size = body_px,
                                        .border_color = st_bd[s], .disabled = disabled);
                                    break;
                                case 3:
                                    wlx_slider(ctx, "", &st->lab_sliders[s], .id = "w",
                                        .height = 30, .min_value = 0.0f, .max_value = 1.0f,
                                        .show_value = false, .disabled = disabled);
                                    break;
                                case 4:
                                    wlx_inputbox(ctx, "", st->lab_inputs[s], sizeof(st->lab_inputs[s]),
                                        .id = "w", .height = 30, .content_padding = 6,
                                        .font = body_font, .font_size = body_px,
                                        .back_color = st_bg[s], .border_color = st_bd[s],
                                        .border_width = st_bw[s], .roundness = 0.12f,
                                        .disabled = disabled);
                                    break;
                                default:
                                    wlx_progress(ctx, st->lab_sliders[s], .id = "w", .height = 30,
                                        .opacity = disabled ? 0.45f : 1.0f);
                                    break;
                            }
                            wlx_pop_id(ctx);
                        }
                    wlx_layout_end(ctx);
                    wlx_pop_id(ctx);
                }
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Built-in theme presets (snapshots of the library themes) --
        dashboard_demo_module(ctx, tk, fonts, "Theme Presets");
            wlx_layout_begin(ctx, 4, WLX_HORZ, .gap = 12,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1),
                                            WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1) });
                dashboard_preset_card(ctx, tk, fonts, "Dark", &wlx_theme_dark);
                dashboard_preset_card(ctx, tk, fonts, "Light", &wlx_theme_light);
                dashboard_preset_card(ctx, tk, fonts, "Glass", &wlx_theme_glass);
                dashboard_preset_card(ctx, tk, fonts, "Active", ctx->theme);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);

        // -- Live geometry readout of the active theme --
        dashboard_demo_module(ctx, tk, fonts, "Theme Geometry");
            char geo[160];
            snprintf(geo, sizeof(geo),
                "padding %.0f    roundness %.2f    border %.1fpx    hover %.2f",
                ctx->theme->padding, ctx->theme->roundness, ctx->theme->border_width,
                ctx->theme->hover_brightness);
            wlx_layout_begin(ctx, 2, WLX_VERT, .gap = 12,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(20), WLX_SLOT_PX(40) });
                wlx_label(ctx, geo, .id = "geo",
                    .style = dashboard_text_style(fonts, tk->type.mono, tk->color.on_surface),
                    .align = WLX_LEFT, .wrap = false,
                    .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 12,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(120), WLX_SLOT_FLEX(1) });
                    wlx_widget(ctx, .id = "acc-sw", .widget_align = WLX_LEFT, .width = 120, .height = 36,
                        .back_color = ctx->theme->accent, .roundness = 0.15f, .rounded_segments = 6,
                        .border_width = 0);
                    wlx_label(ctx, "theme->accent", .id = "acc-lbl",
                        .style = dashboard_text_style(fonts, tk->type.body_sm,
                            tk->color.on_surface_muted),
                        .align = WLX_LEFT, .wrap = false,
                        .vertical_metric = WLX_VMETRIC_FONT_SIZE, .border_width = 0);
                wlx_layout_end(ctx);
            wlx_layout_end(ctx);
        dashboard_demo_module_end(ctx);
    wlx_layout_end(ctx);
}

// The dashboard shell: shared top bar + sidebar chrome around a scrollable main
// content area that dispatches to the active section's body. The sidebar sets
// st->current_view; the shell routes the body without touching the chrome.
static void dashboard_overview(WLX_Context *ctx, const Dashboard_Tokens *tk,
                               const Dashboard_Fonts *fonts, Dashboard_Demo_State *st,
                               float phase) {
    // View root: fixed 64px topbar + flexible body, body row is a fixed 256px
    // sidebar + flexible main. Several regions use fixed/min slots (the 64px
    // topbar, the 256px sidebar, the topbar identity floor, the sidebar nav
    // floor) that redistribution cannot shrink, so at extreme window sizes they
    // over-allocate. Clipping the whole view to the window contains all of them
    // in one place: every fixed/min slot crops at the window edge instead of
    // drawing past it, rather than clipping each region separately.
    wlx_layout_begin(ctx, 2, WLX_VERT, .clip = true,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(64), WLX_SLOT_FLEX(1) });

        dashboard_topbar(ctx, tk, fonts, st);

        wlx_layout_begin(ctx, 2, WLX_HORZ,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(256), WLX_SLOT_FLEX(1) });

            dashboard_overview_sidebar(ctx, tk, fonts, st);

            // Main content area: scrollable, surface-container-lowest background.
            // The active route's section body is dispatched into the scroll panel.
            wlx_layout_begin(ctx, 1, WLX_VERT);
                wlx_scroll_panel_begin(ctx, WLX_SCROLL_AUTO_HEIGHT, .back_color = (WLX_Color){0},
                    .scrollbar_color = tk->color.field_border);
                    int view = (int)st->current_view;
                    if (view < 0 || view >= DASHBOARD_VIEW_COUNT) view = DASHBOARD_VIEW_OVERVIEW;
                    dashboard_sections[view].render(ctx, tk, fonts, st, phase);
                wlx_scroll_panel_end(ctx);
            wlx_layout_end(ctx);
        wlx_layout_end(ctx);
    wlx_layout_end(ctx);
}

// ===========================================================================
// Backend-neutral app state + frame driver
// ===========================================================================
// File-scope app state so the WASM frame export (which has no main loop) and the
// desktop loops share one state. The desktop entry points seed it from argv; the
// WASM host uses the defaults.

static Dashboard_Demo_State g_dashboard_demo = {
    .current_view = DASHBOARD_VIEW_OVERVIEW,

    .overview_tab    = 0,
    .overview_check  = true,
    .overview_toggle = false,
    .overview_slider = 0.5f,

    .comp_checks       = { true, false, true },
    .comp_toggle_a     = true,
    .comp_toggle_b     = false,
    .comp_radio        = 1,
    .comp_slider       = 0.5f,
    .comp_styled_slider = 0.62f,
    .comp_progress     = 0.45f,
    .comp_name         = "Ada Lovelace",
    .comp_email        = "ada@wollix.dev",

    .idstack_enabled = { true, false, true, false },
    .idstack_values  = { 0.3f, 0.6f, 0.45f, 0.8f },
    .idstack_names   = { "node-alpha", "node-beta", "node-gamma", "node-delta" },
    .opacity_widget  = 0.7f,
    .opacity_stack   = 0.6f,

    .lab_checks  = { false, false, true, true, true },
    .lab_sliders = { 0.35f, 0.45f, 0.72f, 0.58f, 0.25f },
    .lab_inputs  = { "Default", "Hover", "Active", "Focus", "Disabled" },
};
static Dashboard_Mode g_dashboard_mode = DASHBOARD_MODE_DARK;
// Exponential moving average of the frame delta (seconds), so the FPS readout is
// stable instead of flickering on per-frame measurement jitter.
static float g_dashboard_frame_ema = 0.0f;

// Seed the initial mode/route from argv: "light" starts in light mode; a route
// name ("overview", "tokens", "components", "layouts", "theme-lab") starts on
// that section. Desktop only -- the WASM host has no command line.
#if defined(WLX_DASHBOARD_RAYLIB) || defined(WLX_DASHBOARD_SDL3)
static void dashboard_parse_args(int argc, char **argv) {
    for (int a = 1; a < argc; a++) {
        if (strcmp(argv[a], "light") == 0)           g_dashboard_mode = DASHBOARD_MODE_LIGHT;
        else if (strcmp(argv[a], "overview") == 0)   g_dashboard_demo.current_view = DASHBOARD_VIEW_OVERVIEW;
        else if (strcmp(argv[a], "tokens") == 0)     g_dashboard_demo.current_view = DASHBOARD_VIEW_TOKENS;
        else if (strcmp(argv[a], "components") == 0) g_dashboard_demo.current_view = DASHBOARD_VIEW_COMPONENTS;
        else if (strcmp(argv[a], "layouts") == 0)    g_dashboard_demo.current_view = DASHBOARD_VIEW_LAYOUTS;
        else if (strcmp(argv[a], "theme-lab") == 0 ||
                 strcmp(argv[a], "themelab") == 0)   g_dashboard_demo.current_view = DASHBOARD_VIEW_THEME_LAB;
    }
}

// Process resident-set-size provider for the Overview's process card, shared by
// both desktop backends. On Linux the resident page count from /proc/self/statm
// scaled by the page size gives RSS; other desktops report unmeasurable so the
// card renders "n/a" rather than a fabricated number.
#if defined(__linux__)
#include <unistd.h>
static bool dashboard_process_rss_mb(double *out_mb) {
    FILE *f = fopen("/proc/self/statm", "r");
    long total_pages = 0, resident_pages = 0;
    if (!f) return false;
    if (fscanf(f, "%ld %ld", &total_pages, &resident_pages) != 2) {
        fclose(f);
        return false;
    }
    fclose(f);
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) return false;
    if (out_mb) *out_mb = (double)resident_pages * (double)page_size / (1024.0 * 1024.0);
    return true;
}
#else
static bool dashboard_process_rss_mb(double *out_mb) {
    (void)out_mb;
    return false;
}
#endif
#endif

// Apply a pending theme toggle (the top-bar switch records it during the prior
// frame) and install the active theme on ctx. Call once at frame start, before
// clearing and rendering.
static void dashboard_frame_prepare(WLX_Context *ctx, const Dashboard_Fonts *fonts) {
    if (g_dashboard_demo.theme_toggle) {
        g_dashboard_mode = (g_dashboard_mode == DASHBOARD_MODE_DARK) ? DASHBOARD_MODE_LIGHT
                                                                     : DASHBOARD_MODE_DARK;
        g_dashboard_demo.theme_toggle = false;
        dashboard_log_emit(DASH_LOG_INFO,
            g_dashboard_mode == DASHBOARD_MODE_LIGHT ? "Theme set to light"
                                                     : "Theme set to dark");
    }
    // The theme must outlive the frame's draw calls; a static keeps its storage
    // valid for ctx->theme without a per-frame allocation.
    static WLX_Theme theme;
    theme = dashboard_wlx_theme(g_dashboard_mode, fonts);
    ctx->theme = &theme;
}

// Refresh the live Overview metrics for this frame. FPS is derived from the
// smoothed backend frame delta (passed in, since the SDL3 timer reports the delta
// since its previous call and must be sampled exactly once per frame); draw calls
// and wollix arena memory come from the perf frame published by the previous
// wlx_end (a WLX_PERF build only -- a one-frame lag that is imperceptible in the
// headline cards). Process RSS is read through the per-backend provider and
// degrades to "n/a" where unavailable.
static void dashboard_update_metrics(WLX_Context *ctx, float frame_dt) {
    Dashboard_Metrics m = {0};
#ifndef WLX_PERF
    (void)ctx;  // only the perf frame read below uses ctx
#endif
    if (frame_dt > 0.0f) {
        g_dashboard_frame_ema = (g_dashboard_frame_ema > 0.0f)
            ? g_dashboard_frame_ema * 0.9f + frame_dt * 0.1f
            : frame_dt;
    }
    m.fps = (g_dashboard_frame_ema > 0.0f) ? (1.0f / g_dashboard_frame_ema) : 0.0f;
#ifdef WLX_PERF
    const WLX_Perf_Frame *pf = wlx_perf_get_last_frame(ctx);
    if (pf) {
        size_t bytes = 0;
        m.perf_available = true;
        m.draw_calls = pf->commands.total_commands;
        for (int i = 0; i < WLX_ARENA_GROUP_COUNT; i++) bytes += pf->arena[i].bytes_capacity;
        m.wlx_bytes = bytes;
    }
#endif
    m.rss_available = dashboard_process_rss_mb(&m.rss_mb);
    g_dashboard_metrics = m;
}

// Render the dashboard shell for the active mode into the already-begun frame.
static void dashboard_render_body(WLX_Context *ctx, const Dashboard_Fonts *fonts) {
    const Dashboard_Tokens *tk = dashboard_tokens(g_dashboard_mode);
    dashboard_log_seed_once();
    // Log a route change once, on the first frame after the new section took effect
    // (the click sets current_view during the prior frame's render). One choke point
    // covers the sidebar nav, the Overview deep-links, and the search jump alike.
    static int last_view = -1;
    int view_now = (int)g_dashboard_demo.current_view;
    if (view_now != last_view) {
        if (last_view >= 0 && view_now >= 0 && view_now < DASHBOARD_VIEW_COUNT) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Navigated to %s", dashboard_sections[view_now].title);
            dashboard_log_emit(DASH_LOG_INFO, buf);
        }
        last_view = view_now;
    }
    // Sample the backend frame delta exactly once per frame (the SDL3 timer is
    // stateful and would report ~0 on a second call), then share it.
    float frame_dt = wlx_get_frame_time(ctx);
    g_dashboard_phase += frame_dt;
    dashboard_update_metrics(ctx, frame_dt);
    dashboard_overview(ctx, tk, fonts, &g_dashboard_demo, g_dashboard_phase);
}

// ===========================================================================
// Platform block -- per-backend lifecycle and entry point.
// ===========================================================================
#if defined(WLX_DASHBOARD_RAYLIB)

static WLX_Context    *g_dashboard_ctx;
static Dashboard_Fonts g_dashboard_fonts;
static Font            g_dashboard_faces[DASHBOARD_FONT_FACE_COUNT];

// ASCII printable plus Latin-1 Supplement and Latin Extended-A, matching the
// codepoint coverage the other demos load.
static int *dashboard_raylib_codepoints(int *out_count) {
    int count = (126 - 32 + 1) + (0x00FF - 0x00A0 + 1) + (0x017F - 0x0100 + 1);
    int *cps = malloc(count * sizeof(int));
    int idx = 0;
    for (int c = 32;     c <= 126;    c++) cps[idx++] = c;
    for (int c = 0x00A0; c <= 0x00FF; c++) cps[idx++] = c;
    for (int c = 0x0100; c <= 0x017F; c++) cps[idx++] = c;
    *out_count = idx;
    return cps;
}

// Load the font set into g_dashboard_faces (which must outlive use, since each
// handle wraps a raylib Font) and fill g_dashboard_fonts. A face that fails to
// load resolves to the backend default.
static void dashboard_raylib_load_fonts(void) {
    int cp_count = 0;
    int *cps = dashboard_raylib_codepoints(&cp_count);
    WLX_Font *slots[DASHBOARD_FONT_FACE_COUNT] = {
        &g_dashboard_fonts.sans_regular, &g_dashboard_fonts.sans_medium,
        &g_dashboard_fonts.sans_semibold, &g_dashboard_fonts.sans_bold,
        &g_dashboard_fonts.mono_regular, &g_dashboard_fonts.mono_medium,
        &g_dashboard_fonts.mono_bold,
    };
    for (int i = 0; i < DASHBOARD_FONT_FACE_COUNT; i++) {
        g_dashboard_faces[i] = LoadFontEx(dashboard_font_paths[i], DASHBOARD_FONT_BASE_SIZE, cps, cp_count);
        if (g_dashboard_faces[i].glyphCount <= 0) {
            printf("WARNING: could not load %s\n", dashboard_font_paths[i]);
            *slots[i] = WLX_FONT_DEFAULT;
            continue;
        }
        SetTextureFilter(g_dashboard_faces[i].texture, TEXTURE_FILTER_BILINEAR);
        *slots[i] = wlx_font_from_raylib(&g_dashboard_faces[i]);
    }
    free(cps);
}

// Cross-backend font-size normalization. At a given nominal font_size, raylib
// renders the Geist/JetBrains faces about 1.31x smaller than SDL3/WASM (measured:
// "Components"@100 -> raylib w=443.75 vs SDL3 w=581). The design system's sizes
// are the nominal sizes the other backends render, so raylib's font_size is
// scaled up to match, keeping the rendered glyph size -- and the measured metrics
// that drive layout -- consistent across backends. SDL3/WASM are left at nominal.
#define DASHBOARD_RAYLIB_FONT_SCALE 1.31f

static WLX_Text_Style dashboard_raylib_scale_style(WLX_Text_Style style) {
    if (style.font_size > 0) {
        int px = (int)((float)style.font_size * DASHBOARD_RAYLIB_FONT_SCALE + 0.5f);
        style.font_size = px > 0 ? px : 1;
    }
    return style;
}

static void dashboard_raylib_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    wlx_raylib_draw_text(text, x, y, dashboard_raylib_scale_style(style));
}
static void dashboard_raylib_measure_text(const char *text, WLX_Text_Style style,
                                          float *out_w, float *out_h) {
    wlx_raylib_measure_text(text, dashboard_raylib_scale_style(style), out_w, out_h);
}
static void dashboard_raylib_measure_text_slice(const char *text, size_t len, WLX_Text_Style style,
                                                float *out_w, float *out_h) {
    wlx_raylib_measure_text_slice(text, len, dashboard_raylib_scale_style(style), out_w, out_h);
}

// raylib has no draw_text_slice (the core falls back to draw_text), so the three
// installed text paths cover both draw and measure.
static void dashboard_raylib_install_text_scale(WLX_Context *ctx) {
    ctx->backend.draw_text          = dashboard_raylib_draw_text;
    ctx->backend.measure_text       = dashboard_raylib_measure_text;
    ctx->backend.measure_text_slice = dashboard_raylib_measure_text_slice;
}

static bool dashboard_platform_init(void) {
    printf("Wollix dashboard demo (Raylib)\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix Dashboard");
    SetTargetFPS(TARGET_FPS);

    dashboard_raylib_load_fonts();

    g_dashboard_ctx = malloc(sizeof(*g_dashboard_ctx));
    if (!g_dashboard_ctx) return false;
    memset(g_dashboard_ctx, 0, sizeof(*g_dashboard_ctx));
    wlx_context_init_raylib(g_dashboard_ctx);
    dashboard_raylib_install_text_scale(g_dashboard_ctx);
    dashboard_icon_atlas_create(g_dashboard_ctx);
    // Drop recording of bounded fills/borders that fall outside the active clip.
    wlx_set_cull_offscreen(g_dashboard_ctx, true);
    return true;
}

static void dashboard_platform_shutdown(void) {
    dashboard_icon_atlas_destroy();
    for (int i = 0; i < DASHBOARD_FONT_FACE_COUNT; i++) {
        if (g_dashboard_faces[i].glyphCount > 0) UnloadFont(g_dashboard_faces[i]);
    }
    CloseWindow();
    if (g_dashboard_ctx) {
        wlx_context_destroy(g_dashboard_ctx);
        free(g_dashboard_ctx);
        g_dashboard_ctx = NULL;
    }
}

static bool dashboard_platform_begin_frame(WLX_Rect *out_root) {
    if (WindowShouldClose()) return false;
    *out_root = (WLX_Rect){ 0, 0, (float)GetRenderWidth(), (float)GetRenderHeight() };
    wlx_begin(g_dashboard_ctx, *out_root, wlx_process_raylib_input);
    BeginDrawing();
    return true;
}

static void dashboard_platform_end_frame(void) {
    wlx_end(g_dashboard_ctx);
    EndDrawing();
}

static void dashboard_platform_clear(WLX_Color bg) {
    ClearBackground(bg);
}

// Open a URL in the user's default browser via raylib's native helper.
static void dashboard_open_url(const char *url) {
    if (url) OpenURL(url);
}

int main(int argc, char **argv) {
    dashboard_parse_args(argc, argv);
    if (!dashboard_platform_init()) return 1;

    // Optional capture: WLX_DASHBOARD_SHOT=<path> renders a few frames then writes
    // a screenshot and exits. Used for offline visual verification.
    const char *shot_path = getenv("WLX_DASHBOARD_SHOT");
    int frame = 0;

    WLX_Rect root;
    while (dashboard_platform_begin_frame(&root)) {
        dashboard_frame_prepare(g_dashboard_ctx, &g_dashboard_fonts);
        dashboard_platform_clear(dashboard_background(g_dashboard_mode));
        dashboard_render_body(g_dashboard_ctx, &g_dashboard_fonts);
        dashboard_platform_end_frame();
        if (shot_path && ++frame >= DASHBOARD_SHOT_FRAME) { TakeScreenshot(shot_path); break; }
    }

    dashboard_platform_shutdown();
    return 0;
}

#endif // WLX_DASHBOARD_RAYLIB

// ===========================================================================
// SDL3 platform block
// ===========================================================================
#if defined(WLX_DASHBOARD_SDL3)

static SDL_Window     *g_dashboard_window;
static SDL_Renderer   *g_dashboard_renderer;
static TTF_Font       *g_dashboard_ttf[DASHBOARD_FONT_FACE_COUNT];
static WLX_Context    *g_dashboard_ctx;
static Dashboard_Fonts g_dashboard_fonts;
static bool            g_dashboard_quit;

// Open the font set as TTF faces baked at the base size; the backend renders any
// requested size from these via its font-variant cache. A face that fails to
// open resolves to the backend default.
static void dashboard_sdl3_load_fonts(void) {
    WLX_Font *slots[DASHBOARD_FONT_FACE_COUNT] = {
        &g_dashboard_fonts.sans_regular, &g_dashboard_fonts.sans_medium,
        &g_dashboard_fonts.sans_semibold, &g_dashboard_fonts.sans_bold,
        &g_dashboard_fonts.mono_regular, &g_dashboard_fonts.mono_medium,
        &g_dashboard_fonts.mono_bold,
    };
    for (int i = 0; i < DASHBOARD_FONT_FACE_COUNT; i++) {
        g_dashboard_ttf[i] = TTF_OpenFont(dashboard_font_paths[i], DASHBOARD_FONT_BASE_SIZE);
        if (!g_dashboard_ttf[i]) {
            printf("WARNING: could not load %s: %s\n", dashboard_font_paths[i], SDL_GetError());
            *slots[i] = WLX_FONT_DEFAULT;
            continue;
        }
        *slots[i] = wlx_font_from_sdl3(g_dashboard_ttf[i]);
    }
}

static bool dashboard_platform_init(void) {
    printf("Wollix dashboard demo (SDL3)\n");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

    g_dashboard_window = SDL_CreateWindow("Wollix Dashboard", WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (!g_dashboard_window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    g_dashboard_renderer = SDL_CreateRenderer(g_dashboard_window, NULL);
    if (!g_dashboard_renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_dashboard_window);
        SDL_Quit();
        return false;
    }
    // Cap presentation to the display refresh (the raylib path uses SetTargetFPS),
    // so the frame loop does not spin uncapped and the FPS readout is meaningful.
    SDL_SetRenderVSync(g_dashboard_renderer, 1);
    if (!TTF_Init()) {
        fprintf(stderr, "TTF_Init failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(g_dashboard_renderer);
        SDL_DestroyWindow(g_dashboard_window);
        SDL_Quit();
        return false;
    }

    g_dashboard_ctx = malloc(sizeof(*g_dashboard_ctx));
    if (!g_dashboard_ctx) return false;
    memset(g_dashboard_ctx, 0, sizeof(*g_dashboard_ctx));
    wlx_context_init_sdl3(g_dashboard_ctx, g_dashboard_window, g_dashboard_renderer);

    // After init_sdl3 so the backend renderer is set for the atlas upload.
    dashboard_sdl3_load_fonts();
    dashboard_icon_atlas_create(g_dashboard_ctx);
    // Drop recording of bounded fills/borders that fall outside the active clip.
    wlx_set_cull_offscreen(g_dashboard_ctx, true);

    g_dashboard_quit = false;
    return true;
}

static void dashboard_platform_shutdown(void) {
    dashboard_icon_atlas_destroy();
    if (g_dashboard_ctx) {
        wlx_context_destroy(g_dashboard_ctx);
        free(g_dashboard_ctx);
        g_dashboard_ctx = NULL;
    }
    wlx_sdl3_text_cache_clear();
    for (int i = 0; i < DASHBOARD_FONT_FACE_COUNT; i++) {
        if (g_dashboard_ttf[i]) TTF_CloseFont(g_dashboard_ttf[i]);
    }
    TTF_Quit();
    if (g_dashboard_renderer) SDL_DestroyRenderer(g_dashboard_renderer);
    if (g_dashboard_window)   SDL_DestroyWindow(g_dashboard_window);
    SDL_Quit();
}

static bool dashboard_platform_begin_frame(WLX_Rect *out_root) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) { g_dashboard_quit = true; return false; }
    }
    if (g_dashboard_quit) return false;

    int rw = 0, rh = 0;
    SDL_GetRenderOutputSize(g_dashboard_renderer, &rw, &rh);
    *out_root = (WLX_Rect){ 0, 0, (float)rw, (float)rh };
    wlx_begin(g_dashboard_ctx, *out_root, wlx_process_sdl3_input);
    return true;
}

// Optional capture: WLX_DASHBOARD_SHOT=<path> writes a BMP of an early frame and
// exits. Read before present, where the backbuffer is defined.
static const char *g_dashboard_shot_path;
static int         g_dashboard_shot_frame;

static void dashboard_platform_end_frame(void) {
    wlx_end(g_dashboard_ctx);
    if (g_dashboard_shot_path && ++g_dashboard_shot_frame >= DASHBOARD_SHOT_FRAME) {
        SDL_Surface *surf = SDL_RenderReadPixels(g_dashboard_renderer, NULL);
        if (surf) {
            SDL_SaveBMP(surf, g_dashboard_shot_path);
            SDL_DestroySurface(surf);
        }
    }
    SDL_RenderPresent(g_dashboard_renderer);
}

static void dashboard_platform_clear(WLX_Color bg) {
    SDL_SetRenderDrawBlendMode(g_dashboard_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_dashboard_renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderClear(g_dashboard_renderer);
}

// Open a URL in the user's default browser via SDL3's native helper.
static void dashboard_open_url(const char *url) {
    if (url) SDL_OpenURL(url);
}

int main(int argc, char **argv) {
    dashboard_parse_args(argc, argv);
    if (!dashboard_platform_init()) return 1;
    g_dashboard_shot_path = getenv("WLX_DASHBOARD_SHOT");

    WLX_Rect root;
    while (dashboard_platform_begin_frame(&root)) {
        dashboard_frame_prepare(g_dashboard_ctx, &g_dashboard_fonts);
        dashboard_platform_clear(dashboard_background(g_dashboard_mode));
        dashboard_render_body(g_dashboard_ctx, &g_dashboard_fonts);
        dashboard_platform_end_frame();
        if (g_dashboard_shot_path && g_dashboard_shot_frame >= DASHBOARD_SHOT_FRAME) break;
    }

    dashboard_platform_shutdown();
    return 0;
}

#endif // WLX_DASHBOARD_SDL3

// ===========================================================================
// WASM platform block
// ===========================================================================
#if defined(WLX_DASHBOARD_WASM)

// Shared-memory input state exported to the JS host.
WLX_Input_State wlx_wasm_input_state = {0};

// Additive host hook: the JS host (web/wollix_wasm.js) maps this "wlx" import to
// window.open. Declared demo-locally so the backend header stays unmodified.
__attribute__((import_module("wlx"), import_name("open_url")))
extern void wlx_wasm_import_open_url(const char *url);

static void dashboard_open_url(const char *url) {
    if (url) wlx_wasm_import_open_url(url);
}

// Process RSS is not available to bare-WASM; the process card renders "n/a".
static bool dashboard_process_rss_mb(double *out_mb) {
    (void)out_mb;
    return false;
}

static WLX_Context    *g_dashboard_ctx;
// Bare WASM has no font files; the zero-initialized handles resolve every face to
// the backend default (WLX_FONT_DEFAULT == 0).
static Dashboard_Fonts g_dashboard_fonts;

// Page-pool allocator backing the general arena group so buffer growth recycles
// freed blocks instead of leaking them through the bump shim.
static WLX_Wasm_Pool g_dashboard_pool;
static WLX_Allocator g_dashboard_alloc;

static bool dashboard_platform_init(void) {
    g_dashboard_ctx = malloc(sizeof(*g_dashboard_ctx));
    if (!g_dashboard_ctx) return false;
    memset(g_dashboard_ctx, 0, sizeof(*g_dashboard_ctx));

    memset(&g_dashboard_pool, 0, sizeof(g_dashboard_pool));
    g_dashboard_alloc = wlx_wasm_allocator(&g_dashboard_pool);
    WLX_Arena_Pool_Config cfg = { .contiguous = NULL, .general = &g_dashboard_alloc };
    wlx_context_init_ex(g_dashboard_ctx, &cfg);
    wlx_context_init_wasm(g_dashboard_ctx);

    dashboard_icon_atlas_create(g_dashboard_ctx);
    // Drop recording of bounded fills/borders that fall outside the active clip.
    wlx_set_cull_offscreen(g_dashboard_ctx, true);
    return true;
}

// ---------------------------------------------------------------------------
// Exported entry points for the JS host (wollix_wasm.js)
// ---------------------------------------------------------------------------
__attribute__((export_name("wlx_wasm_init")))
void wlx_wasm_init(void) {
    dashboard_platform_init();
}

__attribute__((export_name("wlx_wasm_frame")))
void wlx_wasm_frame(float width, float height) {
    WLX_Rect root = { 0, 0, width, height };
    dashboard_frame_prepare(g_dashboard_ctx, &g_dashboard_fonts);
    wlx_begin(g_dashboard_ctx, root, wlx_process_wasm_input);
    g_dashboard_ctx->backend.draw_rect(root, g_dashboard_ctx->theme->background);
    dashboard_render_body(g_dashboard_ctx, &g_dashboard_fonts);
    wlx_end(g_dashboard_ctx);
}

// Validation hook: reports current page-pool counters so a host (or test
// harness) can assert that allocations stabilize across frames.
//   slot 0: alloc_count   slot 1: reuse_count   slot 2: free_count
//   slot 3: bytes_in_use  slot 4: high_water
__attribute__((export_name("wlx_wasm_pool_stats")))
void wlx_wasm_pool_stats(size_t *out, size_t out_len) {
    size_t values[5];
    if (out == NULL || out_len == 0) return;
    values[0] = g_dashboard_pool.alloc_count;
    values[1] = g_dashboard_pool.reuse_count;
    values[2] = g_dashboard_pool.free_count;
    values[3] = g_dashboard_pool.bytes_in_use;
    values[4] = g_dashboard_pool.high_water;
    if (out_len > 5) out_len = 5;
    for (size_t i = 0; i < out_len; i++) out[i] = values[i];
}

#endif // WLX_DASHBOARD_WASM
