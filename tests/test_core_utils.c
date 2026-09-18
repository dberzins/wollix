// test_core_utils.c - shared core utilities the adapters also consume:
// wlx_buf_reserve (grow-and-reuse flat buffer), wlx_utf8_floor (boundary
// floor), wlx_hash_fnv1a64 (byte-content hash of the measurement caches);
// plus the small pure helpers the widget code shares (sub-pixel outline
// rule, reference line height, glyph-row block width) and the layout axis
// / CONTENT-slot predicates.

TEST(buf_reserve_grows_geometrically_and_keeps_content) {
    char *buf = NULL;
    size_t cap = 0;

    ASSERT_TRUE(wlx_buf_reserve(&buf, &cap, 10));
    ASSERT_TRUE(buf != NULL);
    ASSERT_EQ_INT(1024, (long)cap);            // first growth lands on the seed
    memcpy(buf, "hello", 6);

    char *before = buf;
    ASSERT_TRUE(wlx_buf_reserve(&buf, &cap, 100));
    ASSERT_TRUE(buf == before);                 // capacity sufficed: no-op
    ASSERT_EQ_INT(1024, (long)cap);

    ASSERT_TRUE(wlx_buf_reserve(&buf, &cap, 3000));
    ASSERT_EQ_INT(4096, (long)cap);             // 1024 -> 2048 -> 4096
    ASSERT_TRUE(memcmp(buf, "hello", 6) == 0);  // realloc preserved content

    wlx_free(buf);
}

TEST(utf8_floor_lands_on_boundaries) {
    // "a" U+00E9 (2 bytes) U+20AC (3 bytes) U+1F600 (4 bytes)
    const char s[] = "a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";
    ASSERT_EQ_INT(0, (long)wlx_utf8_floor(s, 0));    // offset 0 is a boundary
    ASSERT_EQ_INT(1, (long)wlx_utf8_floor(s, 1));    // lead byte returns itself
    ASSERT_EQ_INT(1, (long)wlx_utf8_floor(s, 2));    // continuation -> its lead
    ASSERT_EQ_INT(3, (long)wlx_utf8_floor(s, 4));    // 3-byte sequence, mid
    ASSERT_EQ_INT(3, (long)wlx_utf8_floor(s, 5));    // 3-byte sequence, last
    ASSERT_EQ_INT(6, (long)wlx_utf8_floor(s, 8));    // 4-byte sequence, mid
    ASSERT_EQ_INT(6, (long)wlx_utf8_floor(s, 9));    // 4-byte sequence, last
    ASSERT_EQ_INT(10, (long)wlx_utf8_floor(s, 10));  // NUL after the emoji
}

TEST(fnv1a64_matches_known_vectors) {
    ASSERT_TRUE(wlx_hash_fnv1a64("", 0) == 0xcbf29ce484222325ULL);
    ASSERT_TRUE(wlx_hash_fnv1a64("a", 1) == 0xaf63dc4c8601ec8cULL);
    ASSERT_TRUE(wlx_hash_fnv1a64("foobar", 6) == 0x85944171f73967e8ULL);
    // Slices hash by content, not by termination.
    const char span[] = { 'f', 'o', 'o', 'b', 'a', 'r', 'X' };
    ASSERT_TRUE(wlx_hash_fnv1a64(span, 6) == 0x85944171f73967e8ULL);
}

TEST(outline_subpixel_scales_alpha_below_one_px) {
    float thick = 0.5f;
    WLX_Color color = { 10, 20, 30, 200 };
    wlx_outline_subpixel(&thick, &color);
    ASSERT_EQ_F(1.0f, thick, 0.0001f);       // drawn 1 px wide ...
    ASSERT_EQ_INT(100, color.a);             // ... at half the alpha
    ASSERT_EQ_INT(10, color.r);
    ASSERT_EQ_INT(30, color.b);

    thick = 2.0f;
    color = (WLX_Color){ 10, 20, 30, 200 };
    wlx_outline_subpixel(&thick, &color);    // whole pixels pass through
    ASSERT_EQ_F(2.0f, thick, 0.0001f);
    ASSERT_EQ_INT(200, color.a);
}

// A measure that reports no line height, so the reference line height must
// fall back to the font size while the space advance still comes back.
static void _cu_measure_flat(const char *text, WLX_Text_Style style, float *out_w, float *out_h, void *user) {
    (void)user;
    (void)style;
    size_t len = text ? strlen(text) : 0;
    if (out_w) *out_w = (float)len * 7.0f;
    if (out_h) *out_h = 0.0f;
}

TEST(text_line_height_falls_back_to_font_size) {
    WLX_Text_Style ts = { .font_size = 16 };

    // The mock reports height == font_size and 0.5 * font_size per byte.
    WLX_Context ctx;
    test_ctx_init(&ctx, 200, 100);
    float space_w = 0.0f;
    ASSERT_EQ_F(16.0f, wlx_text_line_height(&ctx, ts, &space_w), 0.0001f);
    ASSERT_EQ_F(8.0f, space_w, 0.0001f);
    wlx_context_destroy(&ctx);

    // No backend height: the font size stands in; the advance is still reported.
    WLX_Context flat;
    test_ctx_init(&flat, 200, 100);
    flat.backend.measure_text = _cu_measure_flat;
    space_w = 0.0f;
    ASSERT_EQ_F(16.0f, wlx_text_line_height(&flat, ts, &space_w), 0.0001f);
    ASSERT_EQ_F(7.0f, space_w, 0.0001f);
    ASSERT_EQ_F(16.0f, wlx_text_line_height(&flat, ts, NULL), 0.0001f);
    wlx_context_destroy(&flat);
}

TEST(glyph_row_block_w_reserves_or_collapses_label_space) {
    // Reserved label space keeps the gap even without a label (checkbox).
    ASSERT_EQ_F(24.0f, wlx_glyph_row_block_w(20.0f, 0.0f, 4.0f, true), 0.0001f);
    // Without reservation an empty label collapses to the glyph (toggle, radio).
    ASSERT_EQ_F(20.0f, wlx_glyph_row_block_w(20.0f, 0.0f, 4.0f, false), 0.0001f);
    // A measured label adds gap + label either way.
    ASSERT_EQ_F(54.0f, wlx_glyph_row_block_w(20.0f, 30.0f, 4.0f, false), 0.0001f);
    ASSERT_EQ_F(54.0f, wlx_glyph_row_block_w(20.0f, 30.0f, 4.0f, true), 0.0001f);
}

TEST(layout_axis_predicates) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 2, WLX_HORZ,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1) });
    WLX_Layout *h = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
    ASSERT_TRUE(wlx_layout_is_horz(h));
    ASSERT_FALSE(wlx_layout_is_vert(h));
    ASSERT_TRUE(h->rect.w > 0.0f);
    ASSERT_EQ_F(h->rect.w, wlx_layout_main_extent(h), 0.0001f);

        wlx_layout_begin(&ctx, 2, WLX_VERT,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1) });
        WLX_Layout *v = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        ASSERT_FALSE(wlx_layout_is_horz(v));
        ASSERT_TRUE(wlx_layout_is_vert(v));
        ASSERT_TRUE(v->rect.h > 0.0f);
        ASSERT_EQ_F(v->rect.h, wlx_layout_main_extent(v), 0.0001f);
        wlx_layout_end(&ctx);

        wlx_grid_begin(&ctx, 2, 2);
        WLX_Layout *g = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        ASSERT_FALSE(wlx_layout_is_horz(g));   // a grid has no main axis
        ASSERT_FALSE(wlx_layout_is_vert(g));
        wlx_grid_end(&ctx);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(layout_slot_is_content_bounds) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 2, WLX_VERT,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1) });

        // A CONTENT slot beside a PIXELS slot: only the CONTENT one answers.
        wlx_layout_begin(&ctx, 2, WLX_HORZ,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_PX(50) });
        WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        ASSERT_TRUE(l->has_content_slot_measures);
        ASSERT_TRUE(wlx_layout_slot_is_content(&ctx, l, 0));
        ASSERT_FALSE(wlx_layout_slot_is_content(&ctx, l, 1));   // PIXELS
        ASSERT_FALSE(wlx_layout_slot_is_content(&ctx, l, 2));   // past the layout's slots
        // The intrinsic-width gate reads the same answer: next slot (0) is
        // CONTENT in a HORZ parent; an explicit slot 1 is not.
        ASSERT_TRUE(wlx_parent_wants_intrinsic_width(&ctx, -1));
        ASSERT_FALSE(wlx_parent_wants_intrinsic_width(&ctx, 1));
        wlx_layout_end(&ctx);

        // No CONTENT slot: nothing is tracked, every slot answers false.
        wlx_layout_begin(&ctx, 2, WLX_HORZ,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(50), WLX_SLOT_FLEX(1) });
        WLX_Layout *plain = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        ASSERT_FALSE(plain->has_content_slot_measures);
        ASSERT_FALSE(wlx_layout_slot_is_content(&ctx, plain, 0));
        ASSERT_FALSE(wlx_parent_wants_intrinsic_width(&ctx, -1));
        wlx_layout_end(&ctx);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

SUITE(core_utils) {
    RUN_TEST(buf_reserve_grows_geometrically_and_keeps_content);
    RUN_TEST(utf8_floor_lands_on_boundaries);
    RUN_TEST(fnv1a64_matches_known_vectors);
    RUN_TEST(outline_subpixel_scales_alpha_below_one_px);
    RUN_TEST(text_line_height_falls_back_to_font_size);
    RUN_TEST(glyph_row_block_w_reserves_or_collapses_label_space);
    RUN_TEST(layout_axis_predicates);
    RUN_TEST(layout_slot_is_content_bounds);
}
