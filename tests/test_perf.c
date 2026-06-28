// test_perf.c - WLX_PERF instrumentation tests.
// Included from test_main.c (single TU build).
//
// Disabled-build tests always run: they assert ctx->perf == NULL and that
// the normal test suite is unaffected by the absence of WLX_PERF.
//
// WLX_PERF-guarded tests compile only when WLX_PERF is defined.  They cover
// command histogram accumulation, zero-command frame snapshots, text
// measurement and text command counters, and arena high-water values.

// ============================================================================
// Disabled-build coverage (runs in every build)
// ============================================================================

// After a full frame cycle, ctx->perf is NULL unless WLX_PERF is defined.
TEST(perf_disabled_ptr_null) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ASSERT_TRUE(ctx.perf == NULL);
    test_frame_begin(&ctx, 0, 0, false, false);
    test_frame_end(&ctx);
#ifndef WLX_PERF
    ASSERT_TRUE(ctx.perf == NULL);
#endif
    wlx_context_destroy(&ctx);
}

// Normal test frame produces no perf side effects visible from the mock
// backend (no output written, no assertions fired).  This is a compile-time
// and smoke check only - the test passes if it reaches the end.
TEST(perf_disabled_frame_clean) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_rect(&ctx, wlx_rect(0, 0, 100, 100), WLX_WHITE);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// WLX_PERF-guarded tests
// ============================================================================

#ifdef WLX_PERF

// Helper: run one frame that records N rect commands and returns the snapshot.
// ctx must have been initialised with test_ctx_init already.
static const WLX_Perf_Frame *perf_run_rect_frame(WLX_Context *ctx, int n) {
    test_frame_begin(ctx, 0, 0, false, false);
    for (int i = 0; i < n; i++) {
        wlx_draw_rect(ctx, wlx_rect(0, 0, 10, 10), WLX_WHITE);
    }
    test_frame_end(ctx);
    return wlx_perf_get_last_frame(ctx);
}

// After a frame the perf context is allocated and the last-frame snapshot is
// reachable through wlx_perf_get_last_frame().
TEST(perf_get_last_frame_non_null) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);
    test_frame_end(&ctx);
    ASSERT_TRUE(wlx_perf_get_last_frame(&ctx) != NULL);
    wlx_context_destroy(&ctx);
}

// A zero-command frame still publishes a coherent snapshot: frame_index > 0,
// timer_available == false (no timer installed), and all counters are zero.
TEST(perf_zero_command_frame_coherent) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);
    test_frame_end(&ctx);

    const WLX_Perf_Frame *f = wlx_perf_get_last_frame(&ctx);
    ASSERT_TRUE(f != NULL);
    ASSERT_TRUE(f->frame_index >= 1);
    ASSERT_FALSE(f->timer_available);
    ASSERT_EQ_INT((int)f->commands.total_commands, 0);
    ASSERT_EQ_INT((int)f->commands.command_ranges, 0);
    ASSERT_EQ_INT((int)f->text.measure_calls, 0);
    ASSERT_EQ_INT((int)f->text.emitted_text_commands, 0);

    wlx_context_destroy(&ctx);
}

// frame_index increments with each completed frame.
TEST(perf_frame_index_increments) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    test_frame_end(&ctx);
    uint64_t f1 = wlx_perf_get_last_frame(&ctx)->frame_index;

    test_frame_begin(&ctx, 0, 0, false, false);
    test_frame_end(&ctx);
    uint64_t f2 = wlx_perf_get_last_frame(&ctx)->frame_index;

    ASSERT_TRUE(f2 == f1 + 1);

    wlx_context_destroy(&ctx);
}

// Rect commands are counted in both total_commands and by_type[WLX_CMD_RECT].
TEST(perf_cmd_histogram_rect) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    const WLX_Perf_Frame *f = perf_run_rect_frame(&ctx, 3);
    ASSERT_TRUE(f != NULL);
    ASSERT_EQ_INT((int)f->commands.total_commands, 3);
    ASSERT_EQ_INT((int)f->commands.by_type[WLX_CMD_RECT], 3);
    // Other bucket must be zero
    ASSERT_EQ_INT((int)f->commands.by_type[WLX_CMD_TEXT], 0);

    wlx_context_destroy(&ctx);
}

// Mixed command types each land in the correct histogram bucket.
TEST(perf_cmd_histogram_mixed) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_rect(&ctx, wlx_rect(0, 0, 100, 20), WLX_WHITE);      // 1 RECT
    wlx_draw_rect(&ctx, wlx_rect(0, 20, 100, 20), WLX_WHITE);     // 2 RECT
    wlx_cmd_record_rect_lines(&ctx, wlx_rect(0, 0, 50, 50), 1.0f, WLX_BLACK); // 1 RECT_LINES
    wlx_cmd_record_text(&ctx, "hi", 0, 0, (WLX_Text_Style){.font_size = 10}); // 1 TEXT
    test_frame_end(&ctx);

    const WLX_Perf_Frame *f = wlx_perf_get_last_frame(&ctx);
    ASSERT_TRUE(f != NULL);
    ASSERT_EQ_INT((int)f->commands.total_commands, 4);
    ASSERT_EQ_INT((int)f->commands.by_type[WLX_CMD_RECT], 2);
    ASSERT_EQ_INT((int)f->commands.by_type[WLX_CMD_RECT_LINES], 1);
    ASSERT_EQ_INT((int)f->commands.by_type[WLX_CMD_TEXT], 1);

    wlx_context_destroy(&ctx);
}

// Text commands are counted via emitted_text_commands and emitted_text_bytes.
TEST(perf_text_command_counter) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_cmd_record_text(&ctx, "hello", 0, 0, (WLX_Text_Style){.font_size = 12});
    test_frame_end(&ctx);

    const WLX_Perf_Frame *f = wlx_perf_get_last_frame(&ctx);
    ASSERT_TRUE(f != NULL);
    ASSERT_EQ_INT((int)f->text.emitted_text_commands, 1);
    // "hello" is 5 bytes
    ASSERT_TRUE(f->text.emitted_text_bytes >= 5);

    wlx_context_destroy(&ctx);
}

// Text measurement calls are counted via measure_calls and measured_bytes.
TEST(perf_text_measure_counter) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    float w = 0, h = 0;
    ctx.backend.measure_text("test", (WLX_Text_Style){.font_size = 10}, &w, &h);
    WLX_PERF_HOOK(text_measure, &ctx, 4); // "test" = 4 bytes
    test_frame_end(&ctx);

    const WLX_Perf_Frame *f = wlx_perf_get_last_frame(&ctx);
    ASSERT_TRUE(f != NULL);
    ASSERT_TRUE(f->text.measure_calls >= 1);
    ASSERT_TRUE(f->text.measured_bytes >= 4); // "test" = 4 bytes

    wlx_context_destroy(&ctx);
}

// wlx_label emits text measurement and a text command, both reflected in
// the perf snapshot.
TEST(perf_label_increments_text_counters) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // Frame 1: seed CONTENT height
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(60)));
    wlx_label(&ctx, "Hello", .height = 60);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: stable layout - now check counters
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(60)));
    wlx_label(&ctx, "Hello", .height = 60);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    const WLX_Perf_Frame *f = wlx_perf_get_last_frame(&ctx);
    ASSERT_TRUE(f != NULL);
    // At least one text command should have been recorded
    ASSERT_TRUE(f->text.emitted_text_commands >= 1);
    // total_commands must be positive (background rect + text)
    ASSERT_TRUE(f->commands.total_commands >= 1);

    wlx_context_destroy(&ctx);
}

// Arena high-water for commands sub-arena is >= count after recording commands.
TEST(perf_arena_highwater_commands) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    const WLX_Perf_Frame *f = perf_run_rect_frame(&ctx, 5);
    ASSERT_TRUE(f != NULL);
    // high_water for the commands group must be >= 5 after recording 5 rects
    ASSERT_TRUE(f->arena[WLX_ARENA_COMMANDS].high_water >= 5);
    // count reflects how many commands were in the buffer at publish time
    ASSERT_TRUE(f->arena[WLX_ARENA_COMMANDS].count >= 5);

    wlx_context_destroy(&ctx);
}

// High-water is non-decreasing: a frame with fewer commands does not reduce it.
TEST(perf_arena_highwater_nondecreasing) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // First frame: 10 rects
    perf_run_rect_frame(&ctx, 10);
    size_t hw_after_10 = wlx_perf_get_last_frame(&ctx)->arena[WLX_ARENA_COMMANDS].high_water;

    // Second frame: 2 rects - high_water must not drop below the previous value
    perf_run_rect_frame(&ctx, 2);
    size_t hw_after_2 = wlx_perf_get_last_frame(&ctx)->arena[WLX_ARENA_COMMANDS].high_water;

    ASSERT_TRUE(hw_after_2 >= hw_after_10);

    wlx_context_destroy(&ctx);
}

// wlx_perf_reset() clears the last frame but preserves the context pointer.
TEST(perf_reset_clears_snapshot) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    perf_run_rect_frame(&ctx, 5);
    ASSERT_TRUE(wlx_perf_get_last_frame(&ctx)->commands.total_commands == 5);

    wlx_perf_reset(&ctx);
    const WLX_Perf_Frame *f = wlx_perf_get_last_frame(&ctx);
    // After reset, the context still exists but the last frame is zero
    ASSERT_TRUE(f != NULL);
    ASSERT_EQ_INT((int)f->commands.total_commands, 0);
    ASSERT_EQ_INT((int)f->frame_index, 0);

    wlx_context_destroy(&ctx);
}

// No timer installed -> timer_available == false and all timing fields are zero.
TEST(perf_no_timer_fields_zero) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    perf_run_rect_frame(&ctx, 1);

    const WLX_Perf_Frame *f = wlx_perf_get_last_frame(&ctx);
    ASSERT_TRUE(f != NULL);
    ASSERT_FALSE(f->timer_available);
    ASSERT_EQ_INT((int)f->timings.total_ns, 0);
    ASSERT_EQ_INT((int)f->timings.begin_ns, 0);
    ASSERT_EQ_INT((int)f->timings.dispatch_ns, 0);

    wlx_context_destroy(&ctx);
}

// Timer installed -> timer_available == true; timing fields may be zero on
// very fast machines but the flag itself must be set.
static uint64_t _perf_mock_timer_counter = 0;
static uint64_t perf_mock_timer(void *user) {
    (void)user;
    _perf_mock_timer_counter += 1000; // advance 1 us each call
    return _perf_mock_timer_counter;
}

TEST(perf_timer_available_when_set) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _perf_mock_timer_counter = 0;
    wlx_perf_set_timer(&ctx, perf_mock_timer, NULL);
    perf_run_rect_frame(&ctx, 1);

    const WLX_Perf_Frame *f = wlx_perf_get_last_frame(&ctx);
    ASSERT_TRUE(f != NULL);
    ASSERT_TRUE(f->timer_available);

    wlx_context_destroy(&ctx);
}

// Immediate-mode frames (ctx->immediate_mode == true) still publish a valid
// snapshot with commands.total_commands == 0 (commands go directly to backend,
// not into the command buffer).
TEST(perf_immediate_mode_zero_commands) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // wlx_begin (called by test_frame_begin) resets immediate_mode to false.
    // Set it to true after wlx_begin so that subsequent draw calls bypass the
    // command buffer and go directly to the backend.
    test_frame_begin(&ctx, 0, 0, false, false);
    ctx.immediate_mode = true;
    wlx_draw_rect(&ctx, wlx_rect(0, 0, 50, 50), WLX_WHITE);
    test_frame_end(&ctx);

    const WLX_Perf_Frame *f = wlx_perf_get_last_frame(&ctx);
    ASSERT_TRUE(f != NULL);
    ASSERT_TRUE(f->frame_index >= 1);
    // In immediate mode, draws bypass the command buffer
    ASSERT_EQ_INT((int)f->commands.total_commands, 0);

    wlx_context_destroy(&ctx);
}

// Steady-state frames of a static UI perform zero heap allocations: the
// warm-up frames grow the arenas and seed the state map, after which every
// frame-lifetime buffer rides the frame arena. The constrained wide layout
// (slot count above WLX_OFFSET_STACK_LIMIT) forces the min/max redistribution
// pass to allocate its working buffers, which must come from the frame arena,
// not the heap.
TEST(perf_steady_state_zero_allocations) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    WLX_Perf_Allocator_Stats stats = {0};
    wlx_perf_allocator_sink = &stats;

    enum { WIDE = WLX_OFFSET_STACK_LIMIT + 8 };
    WLX_Slot_Size sizes[WIDE];
    for (size_t i = 0; i < WIDE; i++) sizes[i] = WLX_SLOT_FLEX_MIN(1, 2);

    for (int frame = 0; frame < 5; frame++) {
        if (frame == 2) {
            // Arenas and state map are at steady size; count from here.
            memset(&stats, 0, sizeof(stats));
        }
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin(&ctx, 3, WLX_VERT);
        wlx_label(&ctx, "steady");
        (void)wlx_button(&ctx, "state");
        wlx_layout_begin(&ctx, WIDE, WLX_HORZ, .sizes = sizes);
        wlx_layout_end(&ctx);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    ASSERT_EQ_INT((int)stats.alloc_calls, 0);
    ASSERT_EQ_INT((int)stats.calloc_calls, 0);
    ASSERT_EQ_INT((int)stats.realloc_calls, 0);

    wlx_perf_allocator_sink = NULL;
    wlx_context_destroy(&ctx);
}

#endif // WLX_PERF

// ============================================================================
// Suite registration
// ============================================================================

SUITE(perf) {
    RUN_TEST(perf_disabled_ptr_null);
    RUN_TEST(perf_disabled_frame_clean);
#ifdef WLX_PERF
    RUN_TEST(perf_get_last_frame_non_null);
    RUN_TEST(perf_zero_command_frame_coherent);
    RUN_TEST(perf_frame_index_increments);
    RUN_TEST(perf_cmd_histogram_rect);
    RUN_TEST(perf_cmd_histogram_mixed);
    RUN_TEST(perf_text_command_counter);
    RUN_TEST(perf_text_measure_counter);
    RUN_TEST(perf_label_increments_text_counters);
    RUN_TEST(perf_arena_highwater_commands);
    RUN_TEST(perf_arena_highwater_nondecreasing);
    RUN_TEST(perf_reset_clears_snapshot);
    RUN_TEST(perf_no_timer_fields_zero);
    RUN_TEST(perf_timer_available_when_set);
    RUN_TEST(perf_immediate_mode_zero_commands);
    RUN_TEST(perf_steady_state_zero_allocations);
#endif
}
