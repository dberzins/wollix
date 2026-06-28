// test_dashboard_theme.c - tests for the dashboard token -> WLX_Theme mapper.
// Included from test_main.c (single TU build). Pure data checks: no backend.
//
// Validates dashboard_wlx_theme(mode, fonts):
//   - baseline color subset maps from the active mode's tokens,
//   - widget overrides (input focus, slider, checkbox, toggle, radio, progress,
//     scrollbar) carry the token accent / role colors,
//   - sentinels resolve correctly (opacity unset, font falls back when NULL),
//   - the font handle resolves from the supplied set,
//   - dark and light produce distinct, mode-appropriate themes.

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

#include "demos/dashboard/dashboard_theme.h"

TEST(dashboard_theme_baseline_colors) {
    struct { Dashboard_Mode mode; const Dashboard_Tokens *tk; } cases[2] = {
        { DASHBOARD_MODE_DARK,  &dashboard_tokens_dark  },
        { DASHBOARD_MODE_LIGHT, &dashboard_tokens_light },
    };
    for (int i = 0; i < 2; i++) {
        WLX_Theme th = dashboard_wlx_theme(cases[i].mode, NULL);
        const Dashboard_Tokens *tk = cases[i].tk;
        ASSERT_EQ_COLOR(th.background, tk->color.background);
        ASSERT_EQ_COLOR(th.foreground, tk->color.on_surface);
        ASSERT_EQ_COLOR(th.surface, tk->color.surface);
        ASSERT_EQ_COLOR(th.border, tk->color.outline_variant);
        ASSERT_EQ_COLOR(th.accent, tk->color.accent);
        ASSERT_EQ_INT(tk->type.body_md.size, th.font_size);
        ASSERT_EQ_INT(tk->spacing.sm, (int)th.padding);
    }
}

TEST(dashboard_theme_widget_overrides) {
    struct { Dashboard_Mode mode; const Dashboard_Tokens *tk; } cases[2] = {
        { DASHBOARD_MODE_DARK,  &dashboard_tokens_dark  },
        { DASHBOARD_MODE_LIGHT, &dashboard_tokens_light },
    };
    for (int i = 0; i < 2; i++) {
        WLX_Theme th = dashboard_wlx_theme(cases[i].mode, NULL);
        const Dashboard_Tokens *tk = cases[i].tk;
        ASSERT_EQ_COLOR(th.input.border_focus, tk->color.accent);
        ASSERT_EQ_COLOR(th.input.cursor, tk->color.on_surface);
        ASSERT_EQ_COLOR(th.slider.track, tk->color.surface_variant);
        ASSERT_EQ_COLOR(th.slider.thumb, tk->color.accent);
        ASSERT_EQ_COLOR(th.checkbox.check, tk->color.accent);
        ASSERT_EQ_COLOR(th.toggle.track, tk->ramp.low);
        ASSERT_EQ_COLOR(th.toggle.track_active, tk->color.accent);
        ASSERT_EQ_COLOR(th.radio.fill, tk->color.accent);
        ASSERT_EQ_COLOR(th.progress.fill, tk->color.accent);
        ASSERT_EQ_COLOR(th.progress.track, tk->color.surface_variant);
        ASSERT_EQ_COLOR(th.scrollbar.bar, tk->color.outline_variant);
    }
}

TEST(dashboard_theme_sentinels) {
    WLX_Theme th = dashboard_wlx_theme(DASHBOARD_MODE_DARK, NULL);
    // Opacity stays at the negative unset sentinel -> global default applies.
    ASSERT_TRUE(wlx_is_negative_unset(th.opacity));
    // No font set supplied -> font falls back to the backend default sentinel.
    ASSERT_EQ_INT((long)WLX_FONT_DEFAULT, (long)th.font);
    // Disabled opacity carries the shared default, not a sentinel.
    ASSERT_FALSE(wlx_is_negative_unset(th.disabled_opacity));
    // A real 1px hairline border, not the zero (no-border) default.
    ASSERT_TRUE(th.border_width > 0.0f);
}

TEST(dashboard_theme_font_resolution) {
    Dashboard_Fonts fonts = {
        .sans_regular  = 11,
        .sans_medium   = 12,
        .sans_semibold = 13,
        .sans_bold     = 14,
        .mono_regular  = 15,
        .mono_medium   = 16,
        .mono_bold     = 17,
    };
    WLX_Theme th = dashboard_wlx_theme(DASHBOARD_MODE_DARK, &fonts);
    // Body text uses sans regular.
    ASSERT_EQ_INT(11, (long)th.font);
}

TEST(dashboard_theme_mode_differs) {
    WLX_Theme d = dashboard_wlx_theme(DASHBOARD_MODE_DARK, NULL);
    WLX_Theme l = dashboard_wlx_theme(DASHBOARD_MODE_LIGHT, NULL);
    // Backgrounds differ between modes.
    ASSERT_TRUE(d.background.r != l.background.r ||
                d.background.g != l.background.g ||
                d.background.b != l.background.b);
    // Hover feedback brightens in dark, darkens in light.
    ASSERT_TRUE(d.hover_brightness > 0.0f);
    ASSERT_TRUE(l.hover_brightness < 0.0f);
}

SUITE(dashboard_theme) {
    RUN_TEST(dashboard_theme_baseline_colors);
    RUN_TEST(dashboard_theme_widget_overrides);
    RUN_TEST(dashboard_theme_sentinels);
    RUN_TEST(dashboard_theme_font_resolution);
    RUN_TEST(dashboard_theme_mode_differs);
}
