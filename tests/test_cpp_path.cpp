// test_cpp_path.cpp - the documented C++ path, compiled as C++11 and linked
// against the implementation compiled as C11 in its own translation unit
// (test_defaults_once_impl.c). Exercises: the headers' C linkage guards,
// options from wlx_<x>_opt_defaults() with assigned fields, the _impl
// struct-form entries, the C++ spellings of the literal helper macros, and
// a layout with a named sizes array in place of WLX_SIZES. Returns non-zero
// on the first failed check.

#include "wollix.h"
#include "wollix_editor.h"
#include "test_mock_backend.h"

// The C headers on purpose: this is a C++ caller of a C library and needs
// nothing from the C++ standard library.
#include <stdio.h>
#include <string.h>

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond) do { \
    g_checks++; \
    if (!(cond)) { \
        fprintf(stderr, "test_cpp_path: %s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
        g_failures++; \
    } \
} while (0)

// Every literal helper macro, with int arguments where C++11 narrowing rules
// would reject a bare aggregate, plus a colour from ints.
static void check_literals(void) {
    int lo = 10, hi = 200, px = 44, w = 1, pct = 50, p = 75;
    WLX_Slot_Size s[] = {
        WLX_SLOT_AUTO, WLX_SLOT_PX(px), WLX_SLOT_PCT(pct), WLX_SLOT_FLEX(w),
        WLX_SLOT_PX_MINMAX(px, lo, hi), WLX_SLOT_FLEX_MIN(w, lo), WLX_SLOT_FLEX_MAX(w, hi),
        WLX_SLOT_FLEX_MINMAX(w, lo, hi), WLX_SLOT_AUTO_MIN(lo), WLX_SLOT_AUTO_MAX(hi),
        WLX_SLOT_AUTO_MINMAX(lo, hi), WLX_SLOT_PCT_MINMAX(pct, lo, hi),
        WLX_SLOT_FILL, WLX_SLOT_FILL_PCT(p), WLX_SLOT_FILL_MIN(lo), WLX_SLOT_FILL_MAX(hi),
        WLX_SLOT_FILL_MINMAX(lo, hi), WLX_SLOT_CONTENT, WLX_SLOT_CONTENT_MIN(lo),
        WLX_SLOT_CONTENT_MAX(hi), WLX_SLOT_CONTENT_MINMAX(lo, hi), WLX_SLOT_PX(44.5),
    };
    CHECK(sizeof(s) / sizeof(s[0]) == 22);
    CHECK(s[1].kind == WLX_SIZE_PIXELS && s[1].value == 44.0f);
    CHECK(s[4].min == 10.0f && s[4].max == 200.0f);
    CHECK(s[13].kind == WLX_SIZE_FILL && s[13].value == 0.75f);
    CHECK(s[21].value == 44.5f);

    int r = 255, a = 128;
    WLX_Color c = WLX_RGBA(r, 0, 0, a);
    WLX_Color k = WLX_BLACK;
    CHECK(c.r == 255 && c.g == 0 && c.a == 128);
    CHECK(k.a == 255);
}

// Defaults functions return the defaults, not zeros, and say so.
static void check_defaults(void) {
    WLX_Button_Opt bo = wlx_button_opt_defaults();
    CHECK(bo.from_defaults);
    CHECK(bo.wrap);
    CHECK(bo.span == 1);
    CHECK(bo.width == WLX_UNSET);

    WLX_Layout_Opt lo = wlx_layout_opt_defaults();
    CHECK(lo.from_defaults);

    WLX_Editor_Opt eo = wlx_editor_opt_defaults();
    CHECK(eo.from_defaults);

    WLX_Button_Opt copy = bo;
    copy.content_align = WLX_CENTER;
    CHECK(copy.from_defaults && copy.content_align == WLX_CENTER);
}

// One frame on the mock backend through the struct-form entries.
static void check_frame(void) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // WLX_SIZES is C only; a C++ caller names the array.
    static const WLX_Slot_Size sizes[] = { WLX_SLOT_PX(44), WLX_SLOT_FLEX(1), WLX_SLOT_PX(120) };
    WLX_Layout_Opt lo = wlx_layout_opt_defaults();
    lo.sizes = sizes;
    lo.padding = 4;
    wlx_layout_begin_impl(&ctx, 3, WLX_VERT, lo, __FILE__, __LINE__);
    {
        WLX_Button_Opt bo = wlx_button_opt_defaults();
        bo.content_align = WLX_CENTER;
        bo.slot_align = WLX_CENTER;
        bo.border_width = 0;
        bo.width = 160;
        (void)wlx_button_impl(&ctx, "OK", bo, __FILE__, __LINE__);

        WLX_Panel_Opt po = wlx_panel_opt_defaults();
        po.title = "Panel";
        wlx_panel_begin_impl(&ctx, po, __FILE__, __LINE__);
        {
            WLX_Label_Opt la = wlx_label_opt_defaults();
            la.font_size = 13;
            la.content_align = WLX_LEFT;
            wlx_label_impl(&ctx, "Label", la, __FILE__, __LINE__);

            char text[64] = "typed";
            WLX_Inputbox_Opt io = wlx_inputbox_opt_defaults();
            io.content_padding = 6;
            (void)wlx_inputbox_impl(&ctx, "Name", text, sizeof(text), io, __FILE__, __LINE__);
        }
        wlx_panel_end(&ctx);

        WLX_Scroll_Panel_Opt so = wlx_scroll_panel_opt_defaults();
        so.padding = 0;
        wlx_scroll_panel_begin_impl(&ctx, 400.0f, so, __FILE__, __LINE__);
        {
            char doc[256] = "editor text\nsecond line\n";
            size_t len = strlen(doc);
            WLX_Editor_Opt eo = wlx_editor_opt_defaults();
            eo.wrap = true;
            (void)wlx_editor_impl(&ctx, "doc", doc, sizeof(doc), &len, eo, __FILE__, __LINE__);
        }
        wlx_scroll_panel_end(&ctx);
    }
    wlx_layout_end(&ctx);

    test_frame_end(&ctx);
    CHECK(ctx.arena.commands.count > 0);
    wlx_context_destroy(&ctx);
}

int main(void) {
    check_literals();
    check_defaults();
    check_frame();
    if (g_failures == 0) {
        printf("test_cpp_path: OK (C++11 caller over a C11 implementation, %d checks)\n", g_checks);
        return 0;
    }
    fprintf(stderr, "test_cpp_path: %d check(s) failed\n", g_failures);
    return 1;
}
