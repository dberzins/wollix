// test_frame_time.c - per-frame backend time sampling contract.
//
// The core samples WLX_Backend.get_frame_time exactly once per frame (in
// wlx_begin) and serves every wlx_get_frame_time read from that cached
// sample: reads within one frame agree, the next frame re-samples, and an
// adapter is therefore free to measure time since its own previous call.

static int ft_calls = 0;

static float ft_counting_get_frame_time(void *user) {
    (void)user;
    ft_calls++;
    return 0.010f + 0.001f * (float)ft_calls;  // distinct value per call
}

TEST(frame_time_sampled_once_per_frame) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    ctx.backend.get_frame_time = ft_counting_get_frame_time;
    ft_calls = 0;

    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_TRUE(ft_calls == 1);
    float a = wlx_get_frame_time(&ctx);
    float b = wlx_get_frame_time(&ctx);
    float c = wlx_get_frame_time(&ctx);
    ASSERT_TRUE(a == b && b == c);   // reads within one frame agree
    ASSERT_TRUE(ft_calls == 1);      // reads hit the cache, not the backend
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_TRUE(ft_calls == 2);      // exactly one fresh sample per frame
    float d = wlx_get_frame_time(&ctx);
    ASSERT_TRUE(d != a);             // the new frame sees the new sample
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

TEST(frame_time_immediate_mode_samples_too) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    ctx.backend.get_frame_time = ft_counting_get_frame_time;
    ft_calls = 0;

    wlx_begin_immediate(&ctx, wlx_rect(0, 0, 800, 600), _test_input_handler);
    ASSERT_TRUE(ft_calls == 1);
    ASSERT_TRUE(wlx_get_frame_time(&ctx) == wlx_get_frame_time(&ctx));
    wlx_end(&ctx);

    wlx_context_destroy(&ctx);
}

SUITE(frame_time) {
    RUN_TEST(frame_time_sampled_once_per_frame);
    RUN_TEST(frame_time_immediate_mode_samples_too);
}
