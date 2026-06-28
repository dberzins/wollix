// dashboard_components.h - Dashboard demo local component recipes.
//
// Included only by dashboard.c, alongside dashboard_theme.h, following the
// demo-local header convention (never a shared cross-demo source file). These
// are the dashboard-specific components the Stitch direction needs that the core
// widgets do not cover: glass modules with asymmetric edges, technical and glass
// buttons, a technical input, status chips/badges/pips, a dense data table, a
// segmented progress bar, navigation chrome, and a responsive column grid.
//
// Everything is built from the public Wollix surfaces (layouts, grids, panels,
// widgets, labels, and their container/widget decor) plus the dashboard token
// model - no raw command emitters. No core (wollix.h) or backend header is
// modified. Pure geometry and state helpers sit at the top so they can be unit
// tested without a backend.

#ifndef WLX_DASHBOARD_COMPONENTS_H
#define WLX_DASHBOARD_COMPONENTS_H

// ============================================================================
// Pure geometry / state helpers (no context, unit testable)
// ============================================================================

// Responsive column count for the dashboard grid: 12 (wide) -> 8 (medium) ->
// 4 (narrow), matching the Stitch 12-to-4 reflow.
static inline int dashboard_grid_columns(float width) {
    if (width >= 1200.0f) return 12;
    if (width >= 720.0f)  return 8;
    return 4;
}

// Pulse alpha for a status pip halo: a triangle wave over `phase` (seconds or
// any monotonic value) scaling `base` between 40% and 100%. Result in [0,255].
static inline uint8_t dashboard_pulse_alpha(uint8_t base, float phase) {
    float p = phase - (float)((long)phase);
    if (p < 0.0f) p += 1.0f;
    float tri = (p < 0.5f) ? (p * 2.0f) : (2.0f - p * 2.0f);
    float a = (float)base * (0.4f + 0.6f * tri);
    if (a < 0.0f) a = 0.0f;
    if (a > 255.0f) a = 255.0f;
    return (uint8_t)a;
}

// Rect of table row `index` within `table`: row 0 is the header (height
// header_h); data rows (index >= 1) each have height row_h and stack below.
static inline WLX_Rect dashboard_table_row_rect(WLX_Rect table, int index,
                                                float header_h, float row_h) {
    WLX_Rect r = { table.x, table.y, table.w, header_h };
    if (index <= 0) return r;
    r.y = table.y + header_h + (float)(index - 1) * row_h;
    r.h = row_h;
    return r;
}

// ASCII uppercase copy into `out` (always NUL-terminated). Returns `out`.
static inline const char *dashboard_upper(const char *s, char *out, size_t cap) {
    size_t i = 0;
    if (cap == 0) return "";
    for (; s && s[i] && i + 1 < cap; i++) {
        char c = s[i];
        out[i] = (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
    }
    out[i] = '\0';
    return out;
}

// ============================================================================
// Rendering helpers
// ============================================================================

// Text style for a typography role + explicit color.
static inline WLX_Text_Style dashboard_text_style(const Dashboard_Fonts *fonts,
                                                  Dashboard_Type_Role role,
                                                  WLX_Color color) {
    WLX_Text_Style st = {
        .font = dashboard_type_font(fonts, role),
        .font_size = dashboard_type_px(role),
        .color = color,
        .spacing = role.tracking,
    };
    return st;
}

// Glass module: claims one parent slot and fills it with the translucent glass
// surface plus an asymmetric two-tone bevel (top + left catch light, bottom +
// right fall to dark) expressed through the layout's own per-side decor, so the
// chrome stays inside the container's draw range. The single claimed slot is
// then the caller's to fill with an explicit-height content layout, so
// primitive-drawn children (which carry no measured height) get real slot
// rects. Pair with dashboard_module_end.
static inline void dashboard_module_begin(WLX_Context *ctx, const Dashboard_Tokens *tk) {
    wlx_layout_begin(ctx, 1, WLX_VERT,
        .back_color = tk->glass.fill,
        .corner_radius = (float)tk->radius.lg, .rounded_segments = 8,
        .border_color_top = tk->glass.edge_light,
        .border_color_left = tk->glass.edge_light,
        .border_color_bottom = tk->glass.edge_dark,
        .border_color_right = tk->glass.edge_dark,
        .border_width_top = 1, .border_width_left = 1,
        .border_width_bottom = 1, .border_width_right = 1,
        .clip = true
    );
}

static inline void dashboard_module_end(WLX_Context *ctx) {
    wlx_layout_end(ctx);
}

// Section heading: small uppercase label-role text on the slot path. The text
// content is the scope id, so distinct headings at one call-site stay unique.
static inline void dashboard_heading(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                     const Dashboard_Fonts *fonts, const char *text,
                                     float height) {
    (void)height;
    char up[96];
    dashboard_upper(text, up, sizeof(up));
    Dashboard_Type_Role role = tk->type.label;
    WLX_Text_Style st = dashboard_text_style(fonts, role, tk->color.on_surface_muted);
    wlx_layout_begin(ctx, 1, WLX_VERT);
        wlx_label(ctx, up,
            .id = up,
            .style = st,
            .align = WLX_LEFT,
            .vertical_metric = WLX_VMETRIC_FONT_SIZE,
            .border_width = 0);
    wlx_layout_end(ctx);
}

// Technical primary button: solid accent fill, on-accent text, tight radius.
static inline bool dashboard_button_primary(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                            const Dashboard_Fonts *fonts,
                                            const char *text, float height,
                                            const char *id) {
    Dashboard_Type_Role role = tk->type.body_md;
    return wlx_button(ctx, text,
        .back_color = tk->color.accent,
        .front_color = tk->color.on_accent,
        .roundness = 0.15f, .align = WLX_CENTER, .content_padding = 10,
        .font = dashboard_type_font(fonts, role), .font_size = dashboard_type_px(role),
        .height = height, .id = id);
}

// Status chip: pill outline in the status color, leading LED pip, mono caps text.
static inline void dashboard_status_chip(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                         const Dashboard_Fonts *fonts,
                                         const char *text, WLX_Color status) {
    // Pill: glass fill + status outline as the chip container's own decor.
    wlx_layout_begin(ctx, 1, WLX_VERT,
        .back_color = tk->glass.fill,
        .roundness = 0.5f, .rounded_segments = 12,
        .border_color = status, .border_width = 1.0f);
    WLX_Rect r = wlx_get_parent_rect(ctx);
    float pr = r.h * 0.16f;
    float pip_center = r.h * 0.5f;                 // pip center from the left edge
    float caption_inset = pip_center + pr + (float)tk->spacing.sm;
    char up[64];
    dashboard_upper(text, up, sizeof(up));
    Dashboard_Type_Role role = tk->type.label;
    WLX_Text_Style st = dashboard_text_style(fonts, role, tk->color.on_surface);
    // Leading LED pip disc + caption as a nested horizontal split. The pip slot
    // spans the caption inset; a square wlx_widget with roundness 1.0 renders the
    // disc, inset so its center lands at r.h * 0.5 from the left.
    wlx_layout_begin(ctx, 2, WLX_HORZ, .id = up, .gap = 0,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(caption_inset), WLX_SLOT_FLEX(1) });
        wlx_widget(ctx, .id = "pip", .width = pr * 2.0f, .height = pr * 2.0f,
            .widget_align = WLX_LEFT, .padding_left = pip_center - pr,
            .back_color = status, .roundness = 1.0f, .rounded_segments = 16, .border_width = 0);
        wlx_label(ctx, up,
            .id = up,
            .style = st,
            .align = WLX_LEFT,
            .vertical_metric = WLX_VMETRIC_FONT_SIZE,
            .border_width = 0);
    wlx_layout_end(ctx);
    wlx_layout_end(ctx);
}

// Badge: small solid pill carrying a short mono caps value.
static inline void dashboard_badge(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                   const Dashboard_Fonts *fonts, const char *text,
                                   WLX_Color fill, WLX_Color on_fill) {
    char up[32];
    dashboard_upper(text, up, sizeof(up));
    Dashboard_Type_Role role = tk->type.label;
    WLX_Text_Style st = dashboard_text_style(fonts, role, on_fill);
    // Solid pill carried by the label's own rounded background; one label holds
    // both the pill and the centered caption.
    wlx_label(ctx, up,
        .id = up,
        .style = st,
        .align = WLX_CENTER,
        .vertical_metric = WLX_VMETRIC_FONT_SIZE,
        .show_background = true,
        .back_color = fill,
        .roundness = 0.5f, .rounded_segments = 12,
        .border_width = 0);
}

// Status pip with optional pulsing halo. `phase` animates the halo alpha.
static inline void dashboard_status_pip(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                        WLX_Color color, float phase) {
    (void)tk;
    wlx_layout_begin(ctx, 1, WLX_VERT);
    WLX_Rect r = wlx_get_parent_rect(ctx);
    float pr = (r.w < r.h ? r.w : r.h) * 0.22f;
    uint8_t halo_a = dashboard_pulse_alpha(110, phase);
    WLX_Color halo = { color.r, color.g, color.b, halo_a };
    // A single centered disc whose first-class glow stands in for the former
    // translucent halo circle: .color is the dot, .glow_* the pulsing halo
    // (spread reaches the old halo radius of pr * 1.9).
    wlx_widget(ctx, .id = "pip", .width = pr * 2.0f, .height = pr * 2.0f,
        .widget_align = WLX_CENTER,
        .back_color = color, .roundness = 1.0f, .rounded_segments = 20, .border_width = 0,
        .glow_color = halo, .glow_spread = pr * 0.9f, .glow_rings = 4);
    wlx_layout_end(ctx);
}

// Segmented progress bar: discrete blocks with the active count drawn in accent.
// Cell geometry, fill counting, coloring, and the neon glow behind the active
// run are all delegated to wlx_progress; the glow is expressed through the
// widget's first-class glow options (accent at a = 90).
static inline void dashboard_segmented_progress(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                                float value, int segments) {
    if (segments < 1) segments = 1;
    wlx_layout_begin(ctx, 1, WLX_VERT);
        WLX_Rect r = wlx_get_parent_rect(ctx);
        float gap = (float)tk->spacing.xs;
        WLX_Color glow = tk->color.accent;
        glow.a = 90;
        wlx_progress(ctx, value,
            .segments         = segments,
            .segment_gap      = gap,
            .track_color      = tk->color.surface_variant,
            .fill_color       = tk->color.accent,
            .roundness        = 0.2f,
            .rounded_segments = 4,
            .track_height     = r.h,
            .glow_color       = glow,
            .glow_spread      = 3.0f,
            .glow_rings       = 2);
    wlx_layout_end(ctx);
}

// Dense data table: header row over `nrows` zebra-striped data rows with grid
// lines. `cells` is row-major with `ncols` entries per row (nrows * ncols).
static inline void dashboard_table(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                   const Dashboard_Fonts *fonts,
                                   const char **headers, const char **cells,
                                   int ncols, int nrows) {
    if (ncols < 1) ncols = 1;
    if (nrows < 0) nrows = 0;
    const float header_h = 28.0f;
    const float row_h = 26.0f;
    const float pad = (float)tk->spacing.sm;

    Dashboard_Type_Role head_role = tk->type.label;
    Dashboard_Type_Role cell_role = tk->type.body_sm;
    WLX_Text_Style head_st = dashboard_text_style(fonts, head_role, tk->color.on_surface_muted);
    WLX_Text_Style cell_st = dashboard_text_style(fonts, cell_role, tk->color.on_surface);

    // Header fill, zebra fills, and grid rules are the rows' own container decor:
    // per-row .back_color, a per-data-row bottom hairline, and a per-cell right
    // hairline (on every column but the last) forming the vertical separators.
    WLX_Slot_Size row_sizes[1 + nrows];
    row_sizes[0] = WLX_SLOT_PX(header_h);
    for (int i = 1; i <= nrows; i++) row_sizes[i] = WLX_SLOT_PX(row_h);
    wlx_layout_begin(ctx, 1 + nrows, WLX_VERT, .id = "dash_table", .gap = 0,
        .sizes = row_sizes);
        // Header row: header background + per-cell right separators.
        wlx_layout_begin(ctx, ncols, WLX_HORZ, .id = "thead", .gap = 0,
            .back_color = tk->table.header_bg);
            for (int c = 0; c < ncols; c++) {
                char up[48];
                dashboard_upper(headers[c], up, sizeof(up));
                WLX_Color sep = (c < ncols - 1) ? tk->table.grid_line : (WLX_Color){0};
                float sep_w  = (c < ncols - 1) ? 1.0f : 0.0f;
                wlx_push_id(ctx, (size_t)c);
                wlx_label(ctx, up, .id = up, .style = head_st, .align = WLX_LEFT,
                    .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                    .content_padding_left = pad, .border_width = 0,
                    .border_color_right = sep, .border_width_right = sep_w);
                wlx_pop_id(ctx);
            }
        wlx_layout_end(ctx);
        for (int i = 1; i <= nrows; i++) {
            WLX_Color rbg = (i % 2 == 1) ? tk->table.row_bg : tk->table.zebra_bg;
            wlx_push_id(ctx, (size_t)i);
            // Data row: zebra fill + bottom grid hairline as its own decor.
            wlx_layout_begin(ctx, ncols, WLX_HORZ, .gap = 0,
                .back_color = rbg,
                .border_color_bottom = tk->table.grid_line, .border_width_bottom = 1.0f);
                for (int c = 0; c < ncols; c++) {
                    const char *txt = cells[(i - 1) * ncols + c];
                    WLX_Color sep = (c < ncols - 1) ? tk->table.grid_line : (WLX_Color){0};
                    float sep_w  = (c < ncols - 1) ? 1.0f : 0.0f;
                    wlx_push_id(ctx, (size_t)c);
                    wlx_label(ctx, txt ? txt : "", .id = "cell", .style = cell_st,
                        .align = WLX_LEFT, .vertical_metric = WLX_VMETRIC_FONT_SIZE,
                        .content_padding_left = pad, .border_width = 0,
                        .border_color_right = sep, .border_width_right = sep_w);
                    wlx_pop_id(ctx);
                }
            wlx_layout_end(ctx);
            wlx_pop_id(ctx);
        }
    wlx_layout_end(ctx);
}

// Navigation sidebar: vertical list of items with the active one highlighted.
// Returns the index that was clicked this frame, or `active` if none.
static inline int dashboard_sidebar(WLX_Context *ctx, const Dashboard_Tokens *tk,
                                    const Dashboard_Fonts *fonts,
                                    const char **items, int count, int active) {
    int result = active;
    Dashboard_Type_Role role = tk->type.body_md;
    wlx_layout_begin(ctx, (size_t)count, WLX_VERT, .gap = (float)tk->spacing.xs);
    for (int i = 0; i < count; i++) {
        int is_active = (i == active);
        WLX_Color bg = is_active ? tk->glass.fill : tk->color.background;
        WLX_Color fg = is_active ? tk->color.accent : tk->color.on_surface_muted;
        if (wlx_button(ctx, items[i],
                .back_color = bg, .front_color = fg,
                .roundness = 0.12f, .align = WLX_LEFT,
                .font = dashboard_type_font(fonts, role), .font_size = dashboard_type_px(role),
                .height = 36, .id = items[i])) {
            result = i;
        }
    }
    wlx_layout_end(ctx);
    return result;
}

// Responsive card grid: opens a single-row grid whose column count reflows with
// the available width. Writes the resolved column count to `out_cols`. Pair with
// dashboard_responsive_grid_end. The caller fills up to `out_cols` cells.
static inline void dashboard_responsive_grid_begin(WLX_Context *ctx,
                                                   const Dashboard_Tokens *tk,
                                                   int *out_cols) {
    int cols = dashboard_grid_columns(wlx_get_available_width(ctx));
    if (out_cols) *out_cols = cols;
    wlx_grid_begin(ctx, 1, (size_t)cols, .gap = (float)tk->spacing.lg);
}

static inline void dashboard_responsive_grid_end(WLX_Context *ctx) {
    wlx_grid_end(ctx);
}

#endif // WLX_DASHBOARD_COMPONENTS_H
