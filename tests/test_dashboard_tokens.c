// test_dashboard_tokens.c - integrity tests for the dashboard demo token model.
// Included from test_main.c (single TU build). Pure data checks: no backend.
//
// Validates the dashboard-local Stitch token bundles for both modes:
//   - surface-container ramp ordering is strictly monotonic in luminance,
//   - alpha channels sit in the expected opaque / translucent ranges,
//   - required color roles are populated (non-zero) in both modes,
//   - both modes expose the same role set with valid typography,
//   - spacing keeps a strict 4px grid, radius is ascending,
//   - per-effect intent matches the approximated-first / blur-deferred policy,
//   - the runtime font resolver maps (family, weight) to the right handle.

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

#include "demos/dashboard/dashboard_theme.h"

static const Dashboard_Tokens *dashboard_test_modes[2] = {
    &dashboard_tokens_dark,
    &dashboard_tokens_light,
};

static int dashboard_test_lum(WLX_Color c) {
    return (int)c.r + (int)c.g + (int)c.b;
}

static int dashboard_test_color_nonzero(WLX_Color c) {
    return c.r != 0 || c.g != 0 || c.b != 0 || c.a != 0;
}

// ----------------------------------------------------------------------------

TEST(dashboard_tokens_ramp_monotonic) {
    for (int m = 0; m < 2; m++) {
        const Dashboard_Surface_Ramp *r = &dashboard_test_modes[m]->ramp;
        int lum[5] = {
            dashboard_test_lum(r->lowest),
            dashboard_test_lum(r->low),
            dashboard_test_lum(r->base),
            dashboard_test_lum(r->high),
            dashboard_test_lum(r->highest),
        };
        int increasing = lum[1] > lum[0];
        for (int i = 1; i < 5; i++) {
            if (increasing) {
                ASSERT_TRUE(lum[i] > lum[i - 1]);
            } else {
                ASSERT_TRUE(lum[i] < lum[i - 1]);
            }
        }
    }
}

TEST(dashboard_tokens_alpha_ranges) {
    for (int m = 0; m < 2; m++) {
        const Dashboard_Tokens *t = dashboard_test_modes[m];
        // Opaque roles.
        WLX_Color opaque[] = {
            t->color.background, t->color.surface, t->color.accent,
            t->color.on_surface, t->color.on_accent,
            t->status.success, t->status.warning, t->status.error, t->status.info,
            t->table.header_bg, t->table.row_bg, t->table.zebra_bg,
            t->ramp.lowest, t->ramp.low, t->ramp.base, t->ramp.high, t->ramp.highest,
        };
        for (size_t i = 0; i < sizeof(opaque) / sizeof(opaque[0]); i++) {
            ASSERT_EQ_INT(255, opaque[i].a);
        }
        // Translucent glass roles: visible but not opaque.
        WLX_Color glass[] = {
            t->glass.edge_light, t->glass.edge_dark, t->glass.fill, t->glass.glow,
        };
        for (size_t i = 0; i < sizeof(glass) / sizeof(glass[0]); i++) {
            ASSERT_TRUE(glass[i].a > 0);
            ASSERT_TRUE(glass[i].a < 255);
        }
        // Grid line is visible (translucent in both modes: a light line on dark
        // surfaces, a 10% black rule on light surfaces).
        ASSERT_TRUE(t->table.grid_line.a > 0);
    }
}

TEST(dashboard_tokens_required_roles_nonzero) {
    for (int m = 0; m < 2; m++) {
        const Dashboard_Tokens *t = dashboard_test_modes[m];
        WLX_Color required[] = {
            t->color.background, t->color.surface, t->color.surface_variant,
            t->color.field, t->color.field_border, t->color.outline,
            t->color.outline_variant, t->color.accent, t->color.accent_strong,
            t->color.on_surface, t->color.on_surface_muted, t->color.on_accent,
            t->status.success, t->status.warning, t->status.error, t->status.info,
            t->table.header_bg, t->table.row_bg, t->table.zebra_bg, t->table.grid_line,
            t->glass.edge_light, t->glass.edge_dark, t->glass.fill, t->glass.glow,
            t->ramp.lowest, t->ramp.low, t->ramp.base, t->ramp.high, t->ramp.highest,
        };
        for (size_t i = 0; i < sizeof(required) / sizeof(required[0]); i++) {
            ASSERT_TRUE(dashboard_test_color_nonzero(required[i]));
        }
    }
}

TEST(dashboard_tokens_typography_valid) {
    for (int m = 0; m < 2; m++) {
        const Dashboard_Type_Scale *ts = &dashboard_test_modes[m]->type;
        const Dashboard_Type_Role *roles[9] = {
            &ts->display, &ts->headline_lg, &ts->headline_md, &ts->headline_sm,
            &ts->body_lg, &ts->body_md, &ts->body_sm, &ts->label, &ts->mono,
        };
        for (int i = 0; i < 9; i++) {
            const Dashboard_Type_Role *r = roles[i];
            ASSERT_TRUE(r->size > 0);
            ASSERT_TRUE(r->line_height > 0);
            ASSERT_TRUE(r->line_height >= r->size);
            ASSERT_TRUE(r->family == DASHBOARD_FAMILY_SANS ||
                        r->family == DASHBOARD_FAMILY_MONO);
            ASSERT_TRUE(r->weight == DASHBOARD_WEIGHT_REGULAR ||
                        r->weight == DASHBOARD_WEIGHT_MEDIUM ||
                        r->weight == DASHBOARD_WEIGHT_SEMIBOLD ||
                        r->weight == DASHBOARD_WEIGHT_BOLD);
        }
    }
}

TEST(dashboard_tokens_spacing_grid) {
    for (int m = 0; m < 2; m++) {
        const Dashboard_Spacing *s = &dashboard_test_modes[m]->spacing;
        ASSERT_EQ_INT(4, s->unit);
        int values[] = {
            s->unit, s->xs, s->sm, s->md, s->lg, s->xl, s->xxl,
            s->gutter, s->margin, s->panel_padding, s->container_max,
        };
        for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
            ASSERT_TRUE(values[i] > 0);
            ASSERT_EQ_INT(0, values[i] % s->unit);
        }
    }
}

TEST(dashboard_tokens_radius_ascending) {
    for (int m = 0; m < 2; m++) {
        const Dashboard_Radius *r = &dashboard_test_modes[m]->radius;
        ASSERT_TRUE(r->sm < r->base);
        ASSERT_TRUE(r->base < r->md);
        ASSERT_TRUE(r->md < r->lg);
        ASSERT_TRUE(r->lg < r->xl);
        ASSERT_TRUE(r->xl < r->full);
    }
}

TEST(dashboard_tokens_effect_intent) {
    for (int m = 0; m < 2; m++) {
        const Dashboard_Effects *e = &dashboard_test_modes[m]->effects;
        ASSERT_EQ_INT(DASHBOARD_EFFECT_CORE_REQUIRED, e->backdrop_blur);
        ASSERT_EQ_INT(DASHBOARD_EFFECT_APPROXIMATED, e->glow);
        ASSERT_EQ_INT(DASHBOARD_EFFECT_APPROXIMATED, e->shadow);
        ASSERT_EQ_INT(DASHBOARD_EFFECT_APPROXIMATED, e->glass_edge);
        ASSERT_EQ_INT(DASHBOARD_EFFECT_APPROXIMATED, e->scanline);
        ASSERT_EQ_INT(DASHBOARD_EFFECT_APPROXIMATED, e->gradient);
    }
}

TEST(dashboard_tokens_mode_tag) {
    ASSERT_EQ_INT(DASHBOARD_MODE_DARK, dashboard_tokens_dark.mode);
    ASSERT_EQ_INT(DASHBOARD_MODE_LIGHT, dashboard_tokens_light.mode);
    ASSERT_EQ_INT(DASHBOARD_MODE_DARK, dashboard_tokens(DASHBOARD_MODE_DARK)->mode);
    ASSERT_EQ_INT(DASHBOARD_MODE_LIGHT, dashboard_tokens(DASHBOARD_MODE_LIGHT)->mode);
}

TEST(dashboard_tokens_font_resolution) {
    // Distinct sentinel handles per face so the resolver can be checked exactly.
    Dashboard_Fonts fonts = {
        .sans_regular  = 1,
        .sans_medium   = 2,
        .sans_semibold = 3,
        .sans_bold     = 4,
        .mono_regular  = 5,
        .mono_medium   = 6,
        .mono_bold     = 7,
    };
    ASSERT_EQ_INT(4, dashboard_font_resolve(&fonts, DASHBOARD_FAMILY_SANS, DASHBOARD_WEIGHT_BOLD));
    ASSERT_EQ_INT(3, dashboard_font_resolve(&fonts, DASHBOARD_FAMILY_SANS, DASHBOARD_WEIGHT_SEMIBOLD));
    ASSERT_EQ_INT(2, dashboard_font_resolve(&fonts, DASHBOARD_FAMILY_SANS, DASHBOARD_WEIGHT_MEDIUM));
    ASSERT_EQ_INT(1, dashboard_font_resolve(&fonts, DASHBOARD_FAMILY_SANS, DASHBOARD_WEIGHT_REGULAR));
    ASSERT_EQ_INT(7, dashboard_font_resolve(&fonts, DASHBOARD_FAMILY_MONO, DASHBOARD_WEIGHT_BOLD));
    ASSERT_EQ_INT(6, dashboard_font_resolve(&fonts, DASHBOARD_FAMILY_MONO, DASHBOARD_WEIGHT_MEDIUM));
    ASSERT_EQ_INT(5, dashboard_font_resolve(&fonts, DASHBOARD_FAMILY_MONO, DASHBOARD_WEIGHT_REGULAR));
    // Mono semibold has no dedicated face: falls back to mono regular.
    ASSERT_EQ_INT(5, dashboard_font_resolve(&fonts, DASHBOARD_FAMILY_MONO, DASHBOARD_WEIGHT_SEMIBOLD));
    // Role-level resolution: dark display is sans bold; dark label is mono medium.
    ASSERT_EQ_INT(4, dashboard_type_font(&fonts, dashboard_tokens_dark.type.display));
    ASSERT_EQ_INT(6, dashboard_type_font(&fonts, dashboard_tokens_dark.type.label));
    ASSERT_EQ_INT(0, dashboard_type_font(NULL, dashboard_tokens_dark.type.display));
}

SUITE(dashboard_tokens) {
    RUN_TEST(dashboard_tokens_ramp_monotonic);
    RUN_TEST(dashboard_tokens_alpha_ranges);
    RUN_TEST(dashboard_tokens_required_roles_nonzero);
    RUN_TEST(dashboard_tokens_typography_valid);
    RUN_TEST(dashboard_tokens_spacing_grid);
    RUN_TEST(dashboard_tokens_radius_ascending);
    RUN_TEST(dashboard_tokens_effect_intent);
    RUN_TEST(dashboard_tokens_mode_tag);
    RUN_TEST(dashboard_tokens_font_resolution);
}
