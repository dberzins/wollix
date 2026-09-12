// test_focus_release.c - frame-begin focus release (the swallowed-click fix).
//
// A focus-class widget (inputbox/editor) holds active_id across frames and
// historically released it only when itself queried later in the frame, so a
// press on a CLICK widget declared earlier found active_id != 0 and was
// swallowed - the user had to click twice, and whether the first click worked
// depended on declaration order. wlx_begin now releases a focus-class holder
// when a fresh press lands outside its recorded rect, before any widget is
// queried. Drag holders are never touched, and the blur edge
// (just_unfocused) still reaches the released widget.

static const WLX_Rect fr_button_rect = { 0, 0, 100, 40 };
static const WLX_Rect fr_field_rect  = { 0, 100, 100, 40 };
static const WLX_Rect fr_slider_rect = { 0, 200, 100, 40 };

#define FR_BUTTON(ctx) \
    wlx_get_interaction((ctx), fr_button_rect, \
        WLX_INTERACT_CLICK | WLX_INTERACT_HOVER, "fr_button", 1)
#define FR_FIELD(ctx) \
    wlx_get_interaction((ctx), fr_field_rect, \
        WLX_INTERACT_FOCUS | WLX_INTERACT_HOVER, "fr_field", 1)
#define FR_SLIDER(ctx) \
    wlx_get_interaction((ctx), fr_slider_rect, \
        WLX_INTERACT_DRAG | WLX_INTERACT_HOVER, "fr_slider", 1)

// The review probe: button declared before a focused field. The press on the
// button must activate it on that same press, not be swallowed.
TEST(focus_release_press_on_earlier_button_fires_same_press) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 50, 120, true, true);   // click the field
    WLX_Interaction a = FR_BUTTON(&ctx);
    WLX_Interaction b = FR_FIELD(&ctx);
    ASSERT_TRUE(!a.active);
    ASSERT_TRUE(b.focused && b.just_focused);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 50, 120, false, false); // release; focus persists
    a = FR_BUTTON(&ctx);
    b = FR_FIELD(&ctx);
    ASSERT_TRUE(b.focused);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 50, 20, true, true);    // press on the button
    a = FR_BUTTON(&ctx);
    b = FR_FIELD(&ctx);
    ASSERT_TRUE(a.active && a.pressed);            // activated on THIS press
    ASSERT_TRUE(!b.focused);
    ASSERT_TRUE(b.just_unfocused);                 // blur edge still delivered
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 50, 20, false, false);  // release over the button
    a = FR_BUTTON(&ctx);
    b = FR_FIELD(&ctx);
    ASSERT_TRUE(a.clicked);                        // one press, one click
    ASSERT_TRUE(!b.focused && !b.just_unfocused);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

TEST(focus_release_click_inside_field_keeps_focus) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 50, 120, true, true);
    WLX_Interaction b = FR_FIELD(&ctx);
    ASSERT_TRUE(b.focused);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 50, 120, false, false);
    b = FR_FIELD(&ctx);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 80, 130, true, true);   // second click, inside
    b = FR_FIELD(&ctx);
    ASSERT_TRUE(b.focused);
    ASSERT_TRUE(!b.just_focused && !b.just_unfocused);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

TEST(focus_release_escape_blur_unchanged) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 50, 120, true, true);
    WLX_Interaction b = FR_FIELD(&ctx);
    ASSERT_TRUE(b.focused);
    test_frame_end(&ctx);

    bool down[WLX_KEY_COUNT] = {0};
    bool pressed[WLX_KEY_COUNT] = {0};
    down[WLX_KEY_ESCAPE] = true;
    pressed[WLX_KEY_ESCAPE] = true;
    test_frame_begin_ex(&ctx, 400, 300, false, false, false, 0.0f,
                        down, pressed, NULL);
    b = FR_FIELD(&ctx);
    ASSERT_TRUE(!b.focused);
    ASSERT_TRUE(b.just_unfocused);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

// A drag holder must never be released at frame begin, even when the pointer
// crosses other widget rects mid-drag.
TEST(focus_release_drag_holder_untouched) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 50, 220, true, true);   // press on the slider
    WLX_Interaction a = FR_BUTTON(&ctx);
    WLX_Interaction s = FR_SLIDER(&ctx);
    ASSERT_TRUE(s.active && s.pressed);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 50, 20, true, false);   // drag onto the button rect
    a = FR_BUTTON(&ctx);
    s = FR_SLIDER(&ctx);
    ASSERT_TRUE(!a.active);                        // drag still owns the mouse
    ASSERT_TRUE(s.active && s.pressed);            // holder survives the frame begin
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 50, 20, false, false);  // release
    a = FR_BUTTON(&ctx);
    s = FR_SLIDER(&ctx);
    ASSERT_TRUE(!s.active);
    ASSERT_TRUE(!a.clicked);                       // the release is not a button click
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

// One press can blur the field AND start a drag on the widget it lands on.
TEST(focus_release_press_starting_drag_blurs_and_drags) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    // A real frame declares all its widgets every frame; the slider must be
    // in the candidate list before a press can land on it.
    test_frame_begin(&ctx, 50, 120, true, true);
    WLX_Interaction b = FR_FIELD(&ctx);
    WLX_Interaction s = FR_SLIDER(&ctx);
    ASSERT_TRUE(b.focused);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 50, 120, false, false);
    b = FR_FIELD(&ctx);
    s = FR_SLIDER(&ctx);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 50, 220, true, true);   // press lands on the slider
    b = FR_FIELD(&ctx);
    s = FR_SLIDER(&ctx);
    ASSERT_TRUE(!b.focused && b.just_unfocused);
    ASSERT_TRUE(s.active && s.pressed);            // same press starts the drag
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

SUITE(focus_release) {
    RUN_TEST(focus_release_press_on_earlier_button_fires_same_press);
    RUN_TEST(focus_release_click_inside_field_keeps_focus);
    RUN_TEST(focus_release_escape_blur_unchanged);
    RUN_TEST(focus_release_drag_holder_untouched);
    RUN_TEST(focus_release_press_starting_drag_blurs_and_drags);
}
