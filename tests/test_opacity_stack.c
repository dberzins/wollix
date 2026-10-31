// test_opacity_stack.c - tests for wlx_push_opacity / wlx_pop_opacity /
// wlx_get_opacity and the three-layer opacity resolution. Popups resolve
// their rows' and the tooltip's text once, as a plain button does: row
// text at the face's alpha under a theme opacity, an item with its own
// colour at its neighbours' alpha, rows fading with the popup's own
// opacity, and the tooltip text faded while its chrome is not.
// Included from test_main.c (single TU build).

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

// ============================================================================
// wlx_get_opacity - empty stack returns 1.0
// ============================================================================

TEST(opacity_stack_empty) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 1.0f, 0.001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// wlx_push_opacity / wlx_pop_opacity
// ============================================================================

TEST(opacity_stack_push_single) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_push_opacity(&ctx, 0.5f);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 0.5f, 0.001f);
    wlx_pop_opacity(&ctx);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 1.0f, 0.001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(opacity_stack_nested) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_push_opacity(&ctx, 0.5f);
    wlx_push_opacity(&ctx, 0.5f);
    // 0.5 * 0.5 = 0.25
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 0.25f, 0.001f);
    wlx_pop_opacity(&ctx);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 0.5f, 0.001f);
    wlx_pop_opacity(&ctx);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 1.0f, 0.001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(opacity_stack_resets_each_frame) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    // Frame 1: push opacity
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_push_opacity(&ctx, 0.3f);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 0.3f, 0.001f);
    test_frame_end(&ctx);
    // Frame 2: stack should be reset
    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 1.0f, 0.001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// wlx_resolve_opacity - three-layer multiplication
// ============================================================================

TEST(resolve_opacity_all_defaults) {
    // All sentinels -> 1.0
    ASSERT_EQ_F(wlx_resolve_opacity(-1.0f, -1.0f, 1.0f), 1.0f, 0.001f);
}

TEST(resolve_opacity_widget_only) {
    ASSERT_EQ_F(wlx_resolve_opacity(0.5f, -1.0f, 1.0f), 0.5f, 0.001f);
}

TEST(resolve_opacity_theme_only) {
    ASSERT_EQ_F(wlx_resolve_opacity(-1.0f, 0.8f, 1.0f), 0.8f, 0.001f);
}

TEST(resolve_opacity_ctx_only) {
    ASSERT_EQ_F(wlx_resolve_opacity(-1.0f, -1.0f, 0.6f), 0.6f, 0.001f);
}

TEST(resolve_opacity_all_three) {
    // 0.5 * 0.8 * 0.6 = 0.24
    ASSERT_EQ_F(wlx_resolve_opacity(0.5f, 0.8f, 0.6f), 0.24f, 0.001f);
}

TEST(resolve_opacity_zero_widget) {
    // widget=0 means fully transparent, not sentinel
    ASSERT_EQ_F(wlx_resolve_opacity(0.0f, 1.0f, 1.0f), 0.0f, 0.001f);
}

// ============================================================================
// wlx_resolve_opacity_for - wrapper reads theme + ctx stack automatically
// ============================================================================

TEST(resolve_opacity_for_all_defaults) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    WLX_Theme theme = {0};
    theme.opacity = -1.0f; // sentinel -> no theme attenuation
    ctx.theme = &theme;
    test_frame_begin(&ctx, 0, 0, false, false);
    // ctx opacity stack is empty -> 1.0; theme sentinel -> 1.0; opt sentinel -> 1.0
    ASSERT_EQ_F(wlx_resolve_opacity_for(&ctx, -1.0f), 1.0f, 0.001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(resolve_opacity_for_theme_attenuates) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    WLX_Theme theme = {0};
    theme.opacity = 0.5f;
    ctx.theme = &theme;
    test_frame_begin(&ctx, 0, 0, false, false);
    // opt sentinel -> 1.0; theme 0.5; ctx 1.0 -> 0.5
    ASSERT_EQ_F(wlx_resolve_opacity_for(&ctx, -1.0f), 0.5f, 0.001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(resolve_opacity_for_stack_attenuates) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    WLX_Theme theme = {0};
    theme.opacity = -1.0f;
    ctx.theme = &theme;
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_push_opacity(&ctx, 0.4f);
    // opt 0.5; theme sentinel -> 1.0; ctx 0.4 -> 0.2
    ASSERT_EQ_F(wlx_resolve_opacity_for(&ctx, 0.5f), 0.2f, 0.001f);
    wlx_pop_opacity(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

// ============================================================================
// Popups: rows and the tooltip text resolve visual state once
// ============================================================================

static struct { char text[24]; unsigned alpha; } pvs_texts[32];
static int pvs_text_count;
static struct { WLX_Color color; } pvs_rects[64];
static int pvs_rect_count;

static void pvs_capture_text(const char *text, size_t len, float x, float y,
                             WLX_Text_Style style, void *user) {
    (void)x; (void)y; (void)user;
    if (pvs_text_count >= 32) return;
    size_t n = len < 23 ? len : 23;
    memcpy(pvs_texts[pvs_text_count].text, text, n);
    pvs_texts[pvs_text_count].text[n] = '\0';
    pvs_texts[pvs_text_count].alpha = style.color.a;
    pvs_text_count++;
}

static void pvs_capture_rect(WLX_Rect r, WLX_Color c, void *user) {
    (void)r; (void)user;
    if (pvs_rect_count < 64) pvs_rects[pvs_rect_count++].color = c;
}

// Alpha of the last drawn text equal to `text`, or -1.
static int pvs_alpha_of(const char *text) {
    int a = -1;
    for (int i = 0; i < pvs_text_count; i++)
        if (strcmp(pvs_texts[i].text, text) == 0) a = (int)pvs_texts[i].alpha;
    return a;
}

static float pvs_dt;
static float pvs_frame_time(void *user) { (void)user; return pvs_dt; }

static void pvs_ctx_init(WLX_Context *ctx, WLX_Theme *theme, float theme_opacity) {
    test_ctx_init(ctx, 400, 300);
    ctx->backend.draw_text_slice = pvs_capture_text;
    ctx->backend.draw_rect = pvs_capture_rect;
    ctx->backend.get_frame_time = pvs_frame_time;
    pvs_dt = 0.0f;
    *theme = wlx_theme_dark;
    theme->opacity = theme_opacity;
    ctx->theme = theme;
}

static void pvs_begin(WLX_Context *ctx, int mx, int my, bool click) {
    pvs_text_count = 0;
    pvs_rect_count = 0;
    test_frame_begin(ctx, mx, my, click, click);
}

// A dropdown in a 40 px slot over a plain button; frame 1 presses the
// dropdown's face (y 20), frame 2 draws it open.
static void pvs_dropdown_frames(WLX_Context *ctx, float dropdown_opacity) {
    static const char *opts[] = { "one", "two" };
    int sel = 0;
    for (int f = 0; f < 3; f++) {
        pvs_begin(ctx, 50, 20, f == 1);
        wlx_layout_begin(ctx, 2, WLX_VERT, .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(40), WLX_SLOT_PX(40) });
        wlx_dropdown(ctx, "dd", &sel, opts, 2, .opacity = dropdown_opacity);
        wlx_button(ctx, "btn");
        wlx_layout_end(ctx);
        test_frame_end(ctx);
    }
}

TEST(popup_dropdown_rows_resolve_once) {
    WLX_Context ctx;
    WLX_Theme theme;
    pvs_ctx_init(&ctx, &theme, 0.5f);
    pvs_dropdown_frames(&ctx, WLX_UNSET);
    ASSERT_EQ_INT(128, pvs_alpha_of("btn"));
    ASSERT_EQ_INT(128, pvs_alpha_of("two"));           // a row, not the face
    // The list chrome is container decor and stays unfaded.
    bool chrome_opaque = false;
    for (int i = 0; i < pvs_rect_count; i++)
        if (pvs_rects[i].color.a == 255) chrome_opaque = true;
    ASSERT_TRUE(chrome_opaque);
    wlx_context_destroy(&ctx);
}

TEST(popup_rows_fade_with_the_popup) {
    WLX_Context ctx;
    WLX_Theme theme;
    pvs_ctx_init(&ctx, &theme, WLX_UNSET);
    pvs_dropdown_frames(&ctx, 0.5f);
    ASSERT_EQ_INT(255, pvs_alpha_of("btn"));
    ASSERT_EQ_INT(128, pvs_alpha_of("two"));
    wlx_context_destroy(&ctx);
}

TEST(popup_menu_button_items_resolve_once) {
    WLX_Context ctx;
    WLX_Theme theme;
    pvs_ctx_init(&ctx, &theme, 0.5f);
    bool open = false;
    for (int f = 0; f < 3; f++) {
        pvs_begin(&ctx, 50, 15, f == 1);
        wlx_layout_begin(&ctx, 1, WLX_VERT, .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(30) });
        if (wlx_menu_button_begin(&ctx, "mb", &open, .height = 30, .row_height = 30)) {
            wlx_menu_item(&ctx, "plain");
            wlx_menu_item(&ctx, "own", .front_color = { 200, 60, 60, 255 });
            wlx_menu_end(&ctx);
        }
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_TRUE(open);
    ASSERT_EQ_INT(128, pvs_alpha_of("mb"));
    ASSERT_EQ_INT(128, pvs_alpha_of("plain"));
    ASSERT_EQ_INT(128, pvs_alpha_of("own"));
    wlx_context_destroy(&ctx);
}

TEST(popup_tooltip_text_fades_with_the_theme) {
    WLX_Context ctx;
    WLX_Theme theme;
    pvs_ctx_init(&ctx, &theme, 0.5f);
    pvs_dt = 0.2f;
    bool shown = false;
    for (int f = 0; f < 4; f++) {
        pvs_begin(&ctx, 50, 20, false);
        wlx_layout_begin(&ctx, 1, WLX_VERT, .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(40) });
        wlx_button(&ctx, "anchor", .height = 40);
        shown = wlx_tooltip_for(&ctx, ((WLX_Rect){ 0, 0, 400, 40 }), "hint", .delay = 0.3f);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_TRUE(shown);
    ASSERT_EQ_INT(128, pvs_alpha_of("anchor"));
    ASSERT_EQ_INT(128, pvs_alpha_of("hint"));
    wlx_context_destroy(&ctx);
}

SUITE(opacity_stack) {
    // Stack push/pop/get
    RUN_TEST(opacity_stack_empty);
    RUN_TEST(opacity_stack_push_single);
    RUN_TEST(popup_dropdown_rows_resolve_once);
    RUN_TEST(popup_rows_fade_with_the_popup);
    RUN_TEST(popup_menu_button_items_resolve_once);
    RUN_TEST(popup_tooltip_text_fades_with_the_theme);
    RUN_TEST(opacity_stack_nested);
    RUN_TEST(opacity_stack_resets_each_frame);

    // Three-layer resolve
    RUN_TEST(resolve_opacity_all_defaults);
    RUN_TEST(resolve_opacity_widget_only);
    RUN_TEST(resolve_opacity_theme_only);
    RUN_TEST(resolve_opacity_ctx_only);
    RUN_TEST(resolve_opacity_all_three);
    RUN_TEST(resolve_opacity_zero_widget);

    // wlx_resolve_opacity_for wrapper
    RUN_TEST(resolve_opacity_for_all_defaults);
    RUN_TEST(resolve_opacity_for_theme_attenuates);
    RUN_TEST(resolve_opacity_for_stack_attenuates);
}
