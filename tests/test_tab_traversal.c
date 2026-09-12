// test_tab_traversal.c - keyboard focus traversal: Tab / Shift-Tab walk the
// previous frame's focusable candidates in declaration order (wrapping at
// both ends, topmost layer only), skipping sliders, disabled and decoration
// widgets; Tab continues from a mouse-focused field and hands it the blur
// edge; a Tab-focused button activates on Enter / Space; Escape and a
// pointer press clear the ring; a focused widget that disappears is GC'd;
// the focus ring is one accent outline at frame end around the focused
// rect, on top of every layer, and absent whenever nothing holds the ring.
//
// Raw wlx_get_interaction queries with fixed rects and stable string ids,
// in the test_focus_release.c style. Every traversal resolves against the
// candidate list of the frame before, so fixtures stage a plain frame first.

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

// ============================================================================
// Fixture: button / field / slider / button, declared in that order
// ============================================================================

static const WLX_Rect tt_b1_rect     = { 0, 0,   100, 40 };
static const WLX_Rect tt_field_rect  = { 0, 60,  100, 40 };
static const WLX_Rect tt_slider_rect = { 0, 120, 100, 40 };
static const WLX_Rect tt_b2_rect     = { 0, 180, 100, 40 };

typedef struct {
    WLX_Interaction b1, field, slider, b2;
} TT_Result;

// One frame with the given pointer state, pressed keys and modifiers. The
// label widget is a decoration query (TAB_SKIP) that must never be a stop.
static TT_Result tt_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked,
                          WLX_Key_Code key, uint32_t mods, bool b2_disabled) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    if (key != WLX_KEY_NONE) keys_pressed[key] = true;
    test_frame_begin_full(ctx, mx, my, down, clicked, down, 0.0f, NULL,
        key != WLX_KEY_NONE ? keys_pressed : NULL, NULL, mods, NULL);
    TT_Result r;
    r.b1 = wlx_get_interaction(ctx, tt_b1_rect,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD, "tt_b1", 1);
    r.field = wlx_get_interaction(ctx, tt_field_rect,
        WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS, "tt_field", 1);
    (void)wlx_get_interaction(ctx, ((WLX_Rect){ 200, 0, 100, 40 }),
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD
            | WLX_INTERACT_TAB_SKIP, "tt_label", 1);
    r.slider = wlx_get_interaction(ctx, tt_slider_rect,
        WLX_INTERACT_HOVER | WLX_INTERACT_DRAG, "tt_slider", 1);
    r.b2 = wlx_get_interaction_for(ctx, tt_b2_rect,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        b2_disabled, "tt_b2", 1);
    test_frame_end(ctx);
    return r;
}

static TT_Result tt_idle(WLX_Context *ctx) {
    return tt_frame(ctx, 300, 300, false, false, WLX_KEY_NONE, 0, false);
}

static TT_Result tt_tab(WLX_Context *ctx) {
    return tt_frame(ctx, 300, 300, false, false, WLX_KEY_TAB, 0, false);
}

static TT_Result tt_shift_tab(WLX_Context *ctx) {
    return tt_frame(ctx, 300, 300, false, false, WLX_KEY_TAB, WLX_MOD_SHIFT, false);
}

// ============================================================================
// Ring order
// ============================================================================

TEST(tab_walks_declaration_order_and_wraps) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    TT_Result r = tt_idle(&ctx);
    ASSERT_EQ_INT(0, (long)wlx_focused_id(&ctx));

    r = tt_tab(&ctx);                          // none -> first stop: b1
    ASSERT_EQ_INT((long)r.b1.id, (long)wlx_focused_id(&ctx));

    r = tt_tab(&ctx);                          // b1 -> field (label, slider skipped later)
    ASSERT_EQ_INT((long)r.field.id, (long)wlx_focused_id(&ctx));
    ASSERT_TRUE(r.field.focused);              // FOCUS-class target is now typing-focused
    ASSERT_TRUE(r.field.just_focused);

    r = tt_tab(&ctx);                          // field -> b2 (label + slider skipped)
    ASSERT_EQ_INT((long)r.b2.id, (long)wlx_focused_id(&ctx));
    ASSERT_FALSE(r.field.focused);             // field released
    ASSERT_TRUE(r.field.just_unfocused);

    r = tt_tab(&ctx);                          // b2 -> wrap to b1
    ASSERT_EQ_INT((long)r.b1.id, (long)wlx_focused_id(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(shift_tab_walks_backward_and_wraps) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    tt_idle(&ctx);
    TT_Result r = tt_shift_tab(&ctx);          // none -> last stop: b2
    ASSERT_EQ_INT((long)r.b2.id, (long)wlx_focused_id(&ctx));
    r = tt_shift_tab(&ctx);                    // b2 -> field
    ASSERT_EQ_INT((long)r.field.id, (long)wlx_focused_id(&ctx));
    r = tt_shift_tab(&ctx);                    // field -> b1
    ASSERT_EQ_INT((long)r.b1.id, (long)wlx_focused_id(&ctx));
    r = tt_shift_tab(&ctx);                    // b1 -> wrap to b2
    ASSERT_EQ_INT((long)r.b2.id, (long)wlx_focused_id(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(tab_skips_disabled_widget) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    tt_frame(&ctx, 300, 300, false, false, WLX_KEY_NONE, 0, true);
    TT_Result r = tt_frame(&ctx, 300, 300, false, false, WLX_KEY_TAB, 0, true);
    ASSERT_EQ_INT((long)r.b1.id, (long)wlx_focused_id(&ctx));
    r = tt_frame(&ctx, 300, 300, false, false, WLX_KEY_TAB, 0, true);
    ASSERT_EQ_INT((long)r.field.id, (long)wlx_focused_id(&ctx));
    r = tt_frame(&ctx, 300, 300, false, false, WLX_KEY_TAB, 0, true);
    // b2 is disabled (no candidate): the ring wraps straight back to b1.
    ASSERT_EQ_INT((long)r.b1.id, (long)wlx_focused_id(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(held_tab_repeats_walk_the_ring) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    tt_idle(&ctx);
    bool rep[WLX_KEY_COUNT] = {0};
    rep[WLX_KEY_TAB] = true;
    // An OS auto-repeat tick (no fresh press) still advances.
    test_frame_begin_full(&ctx, 300, 300, false, false, false, 0.0f, NULL, NULL, rep, 0, NULL);
    WLX_Interaction b1 = wlx_get_interaction(&ctx, tt_b1_rect,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD, "tt_b1", 1);
    test_frame_end(&ctx);
    ASSERT_EQ_INT((long)b1.id, (long)wlx_focused_id(&ctx));
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Continuing from a mouse-focused field
// ============================================================================

TEST(tab_continues_from_mouse_focused_field) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    tt_idle(&ctx);
    TT_Result r = tt_frame(&ctx, 50, 80, true, true, WLX_KEY_NONE, 0, false);  // click field
    ASSERT_TRUE(r.field.focused);
    ASSERT_EQ_INT(0, (long)wlx_focused_id(&ctx));   // pointer focus is not the ring
    tt_frame(&ctx, 50, 80, false, false, WLX_KEY_NONE, 0, false);

    r = tt_tab(&ctx);                                // from the field -> b2
    ASSERT_EQ_INT((long)r.b2.id, (long)wlx_focused_id(&ctx));
    ASSERT_FALSE(r.field.focused);
    ASSERT_TRUE(r.field.just_unfocused);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Activation
// ============================================================================

TEST(focused_button_activates_on_enter_and_space) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    tt_idle(&ctx);
    tt_tab(&ctx);                                    // b1 focused, pointer far away
    TT_Result r = tt_frame(&ctx, 300, 300, false, false, WLX_KEY_ENTER, 0, false);
    ASSERT_FALSE(r.b1.hover);
    ASSERT_TRUE(r.b1.clicked);
    ASSERT_FALSE(r.b2.clicked);

    r = tt_frame(&ctx, 300, 300, false, false, WLX_KEY_SPACE, 0, false);
    ASSERT_TRUE(r.b1.clicked);
    wlx_context_destroy(&ctx);
}

TEST(enter_that_blurs_field_does_not_activate_next_stop) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    tt_idle(&ctx);
    tt_tab(&ctx);                                    // b1
    tt_tab(&ctx);                                    // field (typing focus)
    // Enter blurs the field; the same press must not click anything, and
    // the ring stays on the field (active_id cleared, focus_id kept).
    TT_Result r = tt_frame(&ctx, 300, 300, false, false, WLX_KEY_ENTER, 0, false);
    ASSERT_TRUE(r.field.just_unfocused);
    ASSERT_FALSE(r.b1.clicked);
    ASSERT_FALSE(r.b2.clicked);
    ASSERT_EQ_INT((long)r.field.id, (long)wlx_focused_id(&ctx));
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Ring dismissal and GC
// ============================================================================

TEST(escape_clears_ring_when_nothing_active) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    tt_idle(&ctx);
    tt_tab(&ctx);                                    // b1 (not active: click-class)
    tt_frame(&ctx, 300, 300, false, false, WLX_KEY_ESCAPE, 0, false);
    ASSERT_EQ_INT(0, (long)wlx_focused_id(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(pointer_press_clears_ring) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    tt_idle(&ctx);
    tt_tab(&ctx);                                    // b1
    tt_frame(&ctx, 300, 300, true, true, WLX_KEY_NONE, 0, false);  // press on nothing
    ASSERT_EQ_INT(0, (long)wlx_focused_id(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(focused_widget_disappearing_is_collected) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    tt_idle(&ctx);
    TT_Result r = tt_tab(&ctx);                      // b1
    ASSERT_EQ_INT((long)r.b1.id, (long)wlx_focused_id(&ctx));

    // A frame that declares only the field: b1 was not seen, focus drops.
    test_frame_begin(&ctx, 300, 300, false, false);
    (void)wlx_get_interaction(&ctx, tt_field_rect,
        WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS, "tt_field", 1);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(0, (long)wlx_focused_id(&ctx));
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Layer scoping: an open overlay owns the ring
// ============================================================================

TEST(overlay_open_ring_cycles_top_layer_only) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    size_t base_id = 0, over_a = 0, over_b = 0;
    for (int frame = 0; frame < 4; frame++) {
        bool keys_pressed[WLX_KEY_COUNT] = {0};
        if (frame >= 1) keys_pressed[WLX_KEY_TAB] = true;
        test_frame_begin_ex(&ctx, 300, 300, false, false, false, 0.0f,
            NULL, frame >= 1 ? keys_pressed : NULL, NULL);
        wlx_layout_begin(&ctx, 1, WLX_VERT);
        WLX_Interaction base = wlx_get_interaction(&ctx, tt_b1_rect,
            WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD, "ov_base", 1);
        wlx_overlay_begin(&ctx, 1, ((WLX_Rect){ 150, 0, 100, 100 }));
        WLX_Interaction a = wlx_get_interaction(&ctx, ((WLX_Rect){ 150, 0, 100, 40 }),
            WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD, "ov_a", 1);
        WLX_Interaction b = wlx_get_interaction(&ctx, ((WLX_Rect){ 150, 50, 100, 40 }),
            WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD, "ov_b", 1);
        wlx_overlay_end(&ctx);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
        base_id = base.id; over_a = a.id; over_b = b.id;
        if (frame == 1) ASSERT_EQ_INT((long)over_a, (long)wlx_focused_id(&ctx));
        if (frame == 2) ASSERT_EQ_INT((long)over_b, (long)wlx_focused_id(&ctx));
        if (frame == 3) ASSERT_EQ_INT((long)over_a, (long)wlx_focused_id(&ctx)); // wraps within the overlay
    }
    ASSERT_TRUE(base_id != wlx_focused_id(&ctx));
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Editor: Tab stays with a focused editor; Escape-then-Tab leaves it
// ============================================================================

static WLX_Editor_State *tt_editor_state(WLX_Context *ctx) {
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        WLX_State_Map_Slot *slot = &ctx->states.slots[i];
        if (slot->id != 0 && slot->data_size == sizeof(WLX_Editor_State))
            return (WLX_Editor_State *)slot->data;
    }
    return NULL;
}

// A raw button query at y 0..40 (declared first) and a real editor filling
// the rest of a 400x300 context. The editor takes the single layout slot;
// the button rect is fixed and overlaps nothing the editor draws at
// y >= 40 because the editor sits in a 40px-inset slot.
static bool tt_editor_frame(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                            int mx, int my, bool down, bool clicked, WLX_Key_Code key,
                            WLX_Interaction *out_button) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    if (key != WLX_KEY_NONE) keys_pressed[key] = true;
    test_frame_begin_full(ctx, mx, my, down, clicked, down, 0.0f, NULL,
        key != WLX_KEY_NONE ? keys_pressed : NULL, NULL, 0, NULL);
    WLX_Interaction b = wlx_get_interaction(ctx, tt_b1_rect,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD, "tt_ed_btn", 1);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 40, .gap = 0);
    bool focused = false;
    (void)wlx_editor_impl(ctx, NULL, buf, cap, len,
        wlx_default_editor_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .out_focused = &focused),
        __FILE__, __LINE__);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    if (out_button) *out_button = b;
    return focused;
}

TEST(focused_editor_keeps_tab_as_insert) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64];
    memcpy(buf, "ab", 3);
    size_t len = 2;

    tt_editor_frame(&ctx, buf, sizeof(buf), &len, 0, 0, false, false, WLX_KEY_NONE, NULL);
    bool focused = tt_editor_frame(&ctx, buf, sizeof(buf), &len, 200, 150, true, true, WLX_KEY_NONE, NULL);
    ASSERT_TRUE(focused);
    tt_editor_frame(&ctx, buf, sizeof(buf), &len, 200, 150, false, false, WLX_KEY_NONE, NULL);

    // Tab inserts (the click above parked the caret at the end of the
    // line), focus does not move to the button.
    focused = tt_editor_frame(&ctx, buf, sizeof(buf), &len, 200, 150, false, false, WLX_KEY_TAB, NULL);
    ASSERT_TRUE(focused);
    ASSERT_EQ_INT(3, (long)len);
    ASSERT_TRUE(memcmp(buf, "ab\t", 3) == 0);
    ASSERT_EQ_INT(0, (long)wlx_focused_id(&ctx));

    // Escape blurs the editor; the next Tab traverses (to the button) and
    // does not insert.
    tt_editor_frame(&ctx, buf, sizeof(buf), &len, 200, 150, false, false, WLX_KEY_ESCAPE, NULL);
    WLX_Interaction btn;
    focused = tt_editor_frame(&ctx, buf, sizeof(buf), &len, 200, 150, false, false, WLX_KEY_TAB, &btn);
    ASSERT_FALSE(focused);
    ASSERT_EQ_INT(3, (long)len);
    ASSERT_EQ_INT((long)btn.id, (long)wlx_focused_id(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(tab_into_editor_does_not_insert) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64];
    memcpy(buf, "ab", 3);
    size_t len = 2;

    tt_editor_frame(&ctx, buf, sizeof(buf), &len, 300, 290, false, false, WLX_KEY_NONE, NULL);
    WLX_Interaction btn;
    tt_editor_frame(&ctx, buf, sizeof(buf), &len, 300, 290, false, false, WLX_KEY_TAB, &btn);   // -> button
    ASSERT_EQ_INT((long)btn.id, (long)wlx_focused_id(&ctx));
    bool focused = tt_editor_frame(&ctx, buf, sizeof(buf), &len, 300, 290, false, false, WLX_KEY_TAB, NULL); // -> editor
    ASSERT_TRUE(focused);
    ASSERT_EQ_INT(2, (long)len);                    // the entering Tab did not insert
    WLX_Editor_State *st = tt_editor_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Now focused: the next Tab is the editor's (inserts), ring unchanged.
    focused = tt_editor_frame(&ctx, buf, sizeof(buf), &len, 300, 290, false, false, WLX_KEY_TAB, NULL);
    ASSERT_TRUE(focused);
    ASSERT_EQ_INT(3, (long)len);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Focus ring: one accent outline at frame end around the focused rect
// ============================================================================

#define TT_MAX_LINES_ 64
static struct { WLX_Rect rect; float thick; WLX_Color color; } _tt_lines[TT_MAX_LINES_];
static int _tt_line_count = 0;

static void _tt_record_rect_lines(WLX_Rect rect, float thick, WLX_Color color) {
    if (_tt_line_count < TT_MAX_LINES_) {
        _tt_lines[_tt_line_count].rect = rect;
        _tt_lines[_tt_line_count].thick = thick;
        _tt_lines[_tt_line_count].color = color;
        _tt_line_count++;
    }
}

static bool _tt_color_eq(WLX_Color a, WLX_Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// The color and thickness the backend must receive for the ring: the
// configured thickness, or - below one pixel - a 1px line with the accent
// alpha scaled (the core's sub-pixel outline rule).
static WLX_Color _tt_ring_color(const WLX_Context *ctx) {
    WLX_Color c = ctx->theme->accent;
    if (WLX_FOCUS_RING_THICKNESS < 1.0f)
        c = wlx_color_apply_opacity(c, WLX_FOCUS_RING_THICKNESS);
    return c;
}

static float _tt_ring_thick(void) {
    return WLX_FOCUS_RING_THICKNESS < 1.0f ? 1.0f : WLX_FOCUS_RING_THICKNESS;
}

// Count the outlines drawn in the ring color at exactly the ring geometry
// for rect r (grown by gap + configured thickness on every side).
static int _tt_rings_around(const WLX_Context *ctx, WLX_Rect r) {
    float grow = WLX_FOCUS_RING_GAP + WLX_FOCUS_RING_THICKNESS;
    int n = 0;
    for (int i = 0; i < _tt_line_count; i++) {
        if (!_tt_color_eq(_tt_lines[i].color, _tt_ring_color(ctx))) continue;
        if (fabsf(_tt_lines[i].thick - _tt_ring_thick()) > 0.001f) continue;
        if (fabsf(_tt_lines[i].rect.x - (r.x - grow)) > 0.01f) continue;
        if (fabsf(_tt_lines[i].rect.y - (r.y - grow)) > 0.01f) continue;
        if (fabsf(_tt_lines[i].rect.w - (r.w + 2.0f * grow)) > 0.01f) continue;
        if (fabsf(_tt_lines[i].rect.h - (r.h + 2.0f * grow)) > 0.01f) continue;
        n++;
    }
    return n;
}

static int _tt_accent_lines(const WLX_Context *ctx) {
    int n = 0;
    for (int i = 0; i < _tt_line_count; i++)
        if (_tt_color_eq(_tt_lines[i].color, _tt_ring_color(ctx))) n++;
    return n;
}

TEST(focus_ring_drawn_around_focused_widget_only) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect_lines = _tt_record_rect_lines;

    _tt_line_count = 0;
    tt_idle(&ctx);
    ASSERT_EQ_INT(0, _tt_accent_lines(&ctx));          // nothing focused: no ring

    _tt_line_count = 0;
    tt_tab(&ctx);                                       // b1
    ASSERT_EQ_INT(1, _tt_rings_around(&ctx, tt_b1_rect));
    ASSERT_EQ_INT(1, _tt_accent_lines(&ctx));           // exactly one ring

    _tt_line_count = 0;
    tt_tab(&ctx);                                       // field
    ASSERT_EQ_INT(1, _tt_rings_around(&ctx, tt_field_rect));
    ASSERT_EQ_INT(0, _tt_rings_around(&ctx, tt_b1_rect));
    wlx_context_destroy(&ctx);
}

TEST(focus_ring_gone_after_press_or_escape) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect_lines = _tt_record_rect_lines;

    tt_idle(&ctx);
    tt_tab(&ctx);                                       // b1
    _tt_line_count = 0;
    tt_frame(&ctx, 300, 300, false, false, WLX_KEY_ESCAPE, 0, false);
    ASSERT_EQ_INT(0, _tt_accent_lines(&ctx));

    tt_tab(&ctx);                                       // b1 again
    _tt_line_count = 0;
    tt_frame(&ctx, 300, 300, true, true, WLX_KEY_NONE, 0, false);   // pointer press
    ASSERT_EQ_INT(0, _tt_accent_lines(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(focus_ring_follows_top_layer_in_retained_mode) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect_lines = _tt_record_rect_lines;

    // Retained mode with an overlay: real widgets record draw commands, the
    // ring must still land once, around the overlay button the ring
    // cycles to, after every layer replayed.
    WLX_Rect over_rect = { 150, 0, 100, 40 };
    for (int frame = 0; frame < 2; frame++) {
        bool keys_pressed[WLX_KEY_COUNT] = {0};
        if (frame == 1) keys_pressed[WLX_KEY_TAB] = true;
        _tt_line_count = 0;
        test_frame_begin_ex(&ctx, 300, 300, false, false, false, 0.0f,
            NULL, frame == 1 ? keys_pressed : NULL, NULL);
        wlx_layout_begin_s(&ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_FLEX(1)), .padding = 0, .gap = 0);
        wlx_button(&ctx, "base");                       // layer 0, y 0..40
        wlx_overlay_begin(&ctx, 1, over_rect);
        wlx_button(&ctx, "over");                       // layer 1, fills over_rect
        wlx_overlay_end(&ctx);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_TRUE(wlx_focused_id(&ctx) != 0);
    ASSERT_EQ_INT(1, _tt_rings_around(&ctx, over_rect));
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

// ---------------------------------------------------------------------------
// wlx_focus_next_stop on hand-built candidate lists: the ring is the
// focusable candidates on the highest layer that has any, in array order.
// ---------------------------------------------------------------------------

static WLX_Candidate_List _fns_list(WLX_Interaction_Candidate *items, size_t n) {
    return (WLX_Candidate_List){ .items = items, .count = n, .capacity = n };
}
#define _FNS(id_, layer_, focusable_) \
    (WLX_Interaction_Candidate){ .id = (id_), .layer = (layer_), .focusable = (focusable_) }

TEST(focus_next_stop_forward_and_wrap) {
    WLX_Interaction_Candidate it[] = { _FNS(1,0,true), _FNS(2,0,false), _FNS(3,0,true), _FNS(4,0,true) };
    WLX_Candidate_List l = _fns_list(it, 4);
    ASSERT_TRUE(wlx_focus_next_stop(&l, 0, false) == 1);   // no start: first member
    ASSERT_TRUE(wlx_focus_next_stop(&l, 1, false) == 3);   // skips the non-focusable 2
    ASSERT_TRUE(wlx_focus_next_stop(&l, 3, false) == 4);
    ASSERT_TRUE(wlx_focus_next_stop(&l, 4, false) == 1);   // wraps to the first
}

TEST(focus_next_stop_backward_and_wrap) {
    WLX_Interaction_Candidate it[] = { _FNS(1,0,true), _FNS(2,0,false), _FNS(3,0,true), _FNS(4,0,true) };
    WLX_Candidate_List l = _fns_list(it, 4);
    ASSERT_TRUE(wlx_focus_next_stop(&l, 0, true) == 4);    // no start: last member
    ASSERT_TRUE(wlx_focus_next_stop(&l, 4, true) == 3);
    ASSERT_TRUE(wlx_focus_next_stop(&l, 3, true) == 1);    // skips 2
    ASSERT_TRUE(wlx_focus_next_stop(&l, 1, true) == 4);    // wraps to the last
}

TEST(focus_next_stop_start_not_on_ring) {
    WLX_Interaction_Candidate it[] = { _FNS(1,0,true), _FNS(2,0,true), _FNS(9,0,false) };
    WLX_Candidate_List l = _fns_list(it, 3);
    ASSERT_TRUE(wlx_focus_next_stop(&l, 9, false) == 1);   // start is not focusable: first
    ASSERT_TRUE(wlx_focus_next_stop(&l, 9, true) == 2);    // ...or last, backward
    ASSERT_TRUE(wlx_focus_next_stop(&l, 77, false) == 1);  // start not in the list at all
}

TEST(focus_next_stop_top_layer_only) {
    WLX_Interaction_Candidate it[] = { _FNS(1,0,true), _FNS(2,1,true), _FNS(3,0,true), _FNS(4,1,true) };
    WLX_Candidate_List l = _fns_list(it, 4);
    ASSERT_TRUE(wlx_focus_next_stop(&l, 0, false) == 2);   // layer 1 hides layer 0
    ASSERT_TRUE(wlx_focus_next_stop(&l, 2, false) == 4);
    ASSERT_TRUE(wlx_focus_next_stop(&l, 4, false) == 2);   // wraps within layer 1
    ASSERT_TRUE(wlx_focus_next_stop(&l, 1, false) == 2);   // a layer-0 start is not on the ring
}

TEST(focus_next_stop_ring_of_one_returns_itself) {
    WLX_Interaction_Candidate it[] = { _FNS(5,0,false), _FNS(7,0,true) };
    WLX_Candidate_List l = _fns_list(it, 2);
    ASSERT_TRUE(wlx_focus_next_stop(&l, 7, false) == 7);
    ASSERT_TRUE(wlx_focus_next_stop(&l, 7, true) == 7);
    ASSERT_TRUE(wlx_focus_next_stop(&l, 0, true) == 7);
}

TEST(focus_next_stop_none_focusable_returns_zero) {
    WLX_Interaction_Candidate it[] = { _FNS(1,0,false), _FNS(2,2,false) };
    WLX_Candidate_List l = _fns_list(it, 2);
    ASSERT_TRUE(wlx_focus_next_stop(&l, 0, false) == 0);
    ASSERT_TRUE(wlx_focus_next_stop(&l, 1, true) == 0);
    WLX_Candidate_List empty = _fns_list(it, 0);
    ASSERT_TRUE(wlx_focus_next_stop(&empty, 0, false) == 0);
}

// A press outside a Tab-holding editor releases it at frame begin and the
// release clears its Tab claim with it: active_consumes_tab never outlives
// the holder, and a Tab arriving in the same frame traverses instead of
// being swallowed by (or inserted into) the released editor.
TEST(press_outside_field_clears_tab_claim) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64];
    memcpy(buf, "ab", 3);
    size_t len = 2;

    tt_editor_frame(&ctx, buf, sizeof(buf), &len, 0, 0, false, false, WLX_KEY_NONE, NULL);
    bool focused = tt_editor_frame(&ctx, buf, sizeof(buf), &len, 200, 150, true, true, WLX_KEY_NONE, NULL);
    ASSERT_TRUE(focused);                            // editor holds typing focus
    tt_editor_frame(&ctx, buf, sizeof(buf), &len, 200, 150, false, false, WLX_KEY_NONE, NULL);
    ASSERT_TRUE(ctx.interaction.active_consumes_tab); // ...and the Tab claim

    // Press on empty space (neither editor nor button) with Tab down the
    // same frame: no widget acquires active_id, so nothing recomputes the
    // claim - only the release itself clears it.
    WLX_Interaction btn;
    focused = tt_editor_frame(&ctx, buf, sizeof(buf), &len, 200, 280, true, true, WLX_KEY_TAB, &btn);
    ASSERT_FALSE(focused);                           // released at frame begin
    ASSERT_FALSE(ctx.interaction.active_consumes_tab); // the claim went with it
    ASSERT_EQ_INT(2, (long)len);                     // the Tab did not insert

    wlx_context_destroy(&ctx);
}

// The ring around a Tab-focused editor wraps the full widget rect (gutter
// included), not the gutter-excluded hit zone the editor queries with.
TEST(focus_ring_wraps_editor_including_gutter) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect_lines = _tt_record_rect_lines;
    char buf[64];
    memcpy(buf, "ab\ncd\nef", 9);
    size_t len = 8;

    // tt_editor_frame's layout: .padding = 40 -> editor rect {40,40,320,220}.
    WLX_Rect ed_rect = { 40, 40, 320, 220 };

    // Rebuild the harness frame with line numbers on: a local copy of
    // tt_editor_frame's body would drift, so drive the editor directly.
    for (int warm = 0; warm < 2; warm++) {
        test_frame_begin(&ctx, 300, 290, false, false);
        (void)wlx_get_interaction(&ctx, tt_b1_rect,
            WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD, "tt_rg_btn", 1);
        wlx_layout_begin(&ctx, 1, WLX_VERT, .padding = 40, .gap = 0);
        (void)wlx_editor_impl(&ctx, NULL, buf, sizeof(buf), &len,
            wlx_default_editor_opt(.content_padding = 4, .font_size = 10,
                .border_width = 0, .line_numbers = true),
            __FILE__, __LINE__);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    bool keys[WLX_KEY_COUNT] = {0};
    keys[WLX_KEY_TAB] = true;
    for (int tab = 0; tab < 2; tab++) {                 // button, then editor
        _tt_line_count = 0;
        test_frame_begin_ex(&ctx, 300, 290, false, false, false, 0.0f, NULL, keys, NULL);
        (void)wlx_get_interaction(&ctx, tt_b1_rect,
            WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD, "tt_rg_btn", 1);
        wlx_layout_begin(&ctx, 1, WLX_VERT, .padding = 40, .gap = 0);
        (void)wlx_editor_impl(&ctx, NULL, buf, sizeof(buf), &len,
            wlx_default_editor_opt(.content_padding = 4, .font_size = 10,
                .border_width = 0, .line_numbers = true),
            __FILE__, __LINE__);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    WLX_Editor_State *st = tt_editor_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_TRUE(st->gutter_end_x > ed_rect.x);          // the gutter exists
    ASSERT_EQ_INT(1, _tt_rings_around(&ctx, ed_rect));  // ring wraps the widget
    WLX_Rect hit_zone = { st->gutter_end_x, ed_rect.y,
                          ed_rect.x + ed_rect.w - st->gutter_end_x, ed_rect.h };
    ASSERT_EQ_INT(0, _tt_rings_around(&ctx, hit_zone)); // not the hit zone
    wlx_context_destroy(&ctx);
}

SUITE(tab_traversal) {
    RUN_TEST(tab_walks_declaration_order_and_wraps);
    RUN_TEST(shift_tab_walks_backward_and_wraps);
    RUN_TEST(tab_skips_disabled_widget);
    RUN_TEST(held_tab_repeats_walk_the_ring);
    RUN_TEST(tab_continues_from_mouse_focused_field);
    RUN_TEST(focused_button_activates_on_enter_and_space);
    RUN_TEST(enter_that_blurs_field_does_not_activate_next_stop);
    RUN_TEST(escape_clears_ring_when_nothing_active);
    RUN_TEST(pointer_press_clears_ring);
    RUN_TEST(focused_widget_disappearing_is_collected);
    RUN_TEST(overlay_open_ring_cycles_top_layer_only);
    RUN_TEST(focused_editor_keeps_tab_as_insert);
    RUN_TEST(tab_into_editor_does_not_insert);
    RUN_TEST(focus_ring_drawn_around_focused_widget_only);
    RUN_TEST(focus_ring_gone_after_press_or_escape);
    RUN_TEST(focus_ring_follows_top_layer_in_retained_mode);
    RUN_TEST(focus_next_stop_forward_and_wrap);
    RUN_TEST(focus_next_stop_backward_and_wrap);
    RUN_TEST(focus_next_stop_start_not_on_ring);
    RUN_TEST(focus_next_stop_top_layer_only);
    RUN_TEST(focus_next_stop_ring_of_one_returns_itself);
    RUN_TEST(focus_next_stop_none_focusable_returns_zero);
    RUN_TEST(press_outside_field_clears_tab_claim);
    RUN_TEST(focus_ring_wraps_editor_including_gutter);
}
