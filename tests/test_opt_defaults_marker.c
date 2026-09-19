// test_opt_defaults_marker.c - every option struct built from its defaults
// macro or defaults function carries from_defaults; a zero-initialised
// struct reaching a widget entry warns once per call site under WLX_DEBUG,
// and structs from the macro, the function, or a copy of either never do.
// Included from test_main.c (single TU build).

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef WOLLIX_EDITOR_H_
#include "wollix_editor.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif
#ifndef TEST_MOCK_BACKEND_H_
#include "test_mock_backend.h"
#endif

TEST(marker_set_by_every_defaults_function) {
    ASSERT_TRUE(wlx_slot_style_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_grid_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_grid_auto_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_layout_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_overlay_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_widget_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_label_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_button_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_dropdown_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_tooltip_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_menu_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_menu_item_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_menu_button_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_checkbox_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_inputbox_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_slider_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_separator_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_progress_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_image_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_toggle_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_radio_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_scroll_panel_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_list_clipper_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_split_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_split_next_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_panel_opt_defaults().from_defaults);
    ASSERT_TRUE(wlx_editor_opt_defaults().from_defaults);
}

TEST(marker_set_by_the_macro_and_kept_by_copies) {
    WLX_Button_Opt from_macro = wlx_default_button_opt(.width = 120);
    ASSERT_TRUE(from_macro.from_defaults);
    ASSERT_TRUE(from_macro.wrap);

    WLX_Button_Opt copy = wlx_button_opt_defaults();
    copy.content_align = WLX_CENTER;
    WLX_Button_Opt copy2 = copy;
    ASSERT_TRUE(copy2.from_defaults);
    ASSERT_EQ_INT(copy2.content_align, WLX_CENTER);

    // A zero-initialised struct is the documented trap: zeros, not defaults.
    WLX_Button_Opt zero;
    memset(&zero, 0, sizeof(zero));
    ASSERT_FALSE(zero.from_defaults);
    ASSERT_FALSE(zero.wrap);
    ASSERT_EQ_INT(zero.span, 0);
}

#ifdef WLX_DEBUG
static int _marker_warn_count = 0;
static char _marker_last_msg[256];

// Counts marker warnings only: a zeroed struct also trips unrelated debug
// checks (a NULL id shared by two calls at one site, for one), and those
// are not what this suite pins.
static void _marker_warn_cb(const char *file, int line, const char *msg, void *user_data) {
    (void)file;
    (void)line;
    (void)user_data;
    if (strstr(msg, "not built from its defaults") == NULL) return;
    _marker_warn_count++;
    snprintf(_marker_last_msg, sizeof(_marker_last_msg), "%s", msg);
}

static void _marker_capture(WLX_Context *ctx) {
    _marker_warn_count = 0;
    _marker_last_msg[0] = '\0';
    wlx_dbg_init(ctx);
    ctx->dbg->warn_cb = _marker_warn_cb;
    ctx->dbg->warn_user_data = NULL;
}

static void _marker_zero_button_site(WLX_Context *ctx) {
    WLX_Button_Opt zero;
    memset(&zero, 0, sizeof(zero));
    (void)wlx_button_impl(ctx, "zero", zero, __FILE__, __LINE__);
}

TEST(marker_zeroed_struct_warns_once_per_site) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _marker_capture(&ctx);

    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin(&ctx, 2, WLX_VERT);
        _marker_zero_button_site(&ctx);
        _marker_zero_button_site(&ctx);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }
    // One site, however many frames and calls.
    ASSERT_EQ_INT(_marker_warn_count, 1);
    ASSERT_TRUE(strstr(_marker_last_msg, "wlx_button") != NULL);
    ASSERT_TRUE(strstr(_marker_last_msg, "wlx_button_opt_defaults()") != NULL);

    // A second site warns once more.
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    {
        WLX_Button_Opt zero;
        memset(&zero, 0, sizeof(zero));
        (void)wlx_button_impl(&ctx, "zero", zero, __FILE__, __LINE__);
    }
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(_marker_warn_count, 2);

    wlx_context_destroy(&ctx);
}

TEST(marker_defaults_built_structs_never_warn) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _marker_capture(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 4, WLX_VERT);
    {
        WLX_Button_Opt bo = wlx_button_opt_defaults();
        bo.content_align = WLX_CENTER;
        (void)wlx_button_impl(&ctx, "function", bo, __FILE__, __LINE__);

        WLX_Label_Opt lo = wlx_default_label_opt(.font_size = 13);
        wlx_label_impl(&ctx, "macro", lo, __FILE__, __LINE__);

        WLX_Label_Opt copy = lo;
        wlx_label_impl(&ctx, "copy", copy, __FILE__, __LINE__);

        wlx_button(&ctx, "one-liner", .width = 80);
    }
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(_marker_warn_count, 0);

    wlx_context_destroy(&ctx);
}

TEST(marker_entry_without_call_site_keys_on_its_name) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _marker_capture(&ctx);

    for (int frame = 0; frame < 2; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_grid_begin(&ctx, 1, 1);
        {
            WLX_Slot_Style_Opt zero;
            memset(&zero, 0, sizeof(zero));
            wlx_grid_cell_impl(&ctx, 0, 0, zero);
            wlx_grid_cell_impl(&ctx, 0, 0, zero);
        }
        wlx_grid_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_EQ_INT(_marker_warn_count, 1);
    ASSERT_TRUE(strstr(_marker_last_msg, "wlx_grid_cell") != NULL);
    ASSERT_TRUE(strstr(_marker_last_msg, "wlx_slot_style_opt_defaults()") != NULL);

    wlx_context_destroy(&ctx);
}
#endif // WLX_DEBUG

SUITE(opt_defaults_marker) {
    RUN_TEST(marker_set_by_every_defaults_function);
    RUN_TEST(marker_set_by_the_macro_and_kept_by_copies);
#ifdef WLX_DEBUG
    RUN_TEST(marker_zeroed_struct_warns_once_per_site);
    RUN_TEST(marker_defaults_built_structs_never_warn);
    RUN_TEST(marker_entry_without_call_site_keys_on_its_name);
#endif
}
