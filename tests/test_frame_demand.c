// test_frame_demand.c - wlx_frame_delay, wlx_request_frame and
// wlx_request_frame_after: when the library says the next frame is needed.
//
// After wlx_end the delay is 0 when the frame's input differs from the
// previous frame's or carries an event, when the frame's backend draw calls
// differ from the previous frame's, or when a frame was requested for now;
// a positive number of seconds when the earliest request is a timed one;
// negative when nothing is scheduled. The draw calls are compared through a
// digest folded at the replay with resolved offsets, so these tests read
// ctx.frame_digest directly where the comparison itself is the subject.
//
// Mock backend: char width = font_size/2, line height = font_size, frame
// time 1/60 s unless a test installs its own.

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif
#ifndef TEST_MOCK_BACKEND_H_
#include "test_mock_backend.h"
#endif

// ============================================================================
// Fixtures
// ============================================================================

static float _fd_dt = 1.0f / 60.0f;
static float _fd_get_frame_time(void *user) {
    (void)user; return _fd_dt; }

static const char *_fd_label_text = "static";
static WLX_Color   _fd_button_color = { 40, 60, 90, 255 };
static float       _fd_request = -1.0f;    // >= 0: the UI asks for a frame after this long

// The UI of one frame: a label and a button, 40 px each, at the top of a
// 400x300 root.
static void _fd_ui(WLX_Context *ctx) {
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_PX(40)), .padding = 0);
    wlx_label(ctx, _fd_label_text, .height = 40);
    (void)wlx_button(ctx, "press", .height = 40, .back_color = _fd_button_color);
    if (_fd_request >= 0.0f) wlx_request_frame_after(ctx, _fd_request);
    wlx_layout_end(ctx);
}

static void _fd_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    _fd_ui(ctx);
    test_frame_end(ctx);
}

static void _fd_reset(WLX_Context *ctx) {
    _fd_label_text = "static";
    _fd_button_color = (WLX_Color){ 40, 60, 90, 255 };
    _fd_request = -1.0f;
    _fd_dt = 1.0f / 60.0f;
    test_ctx_init(ctx, 400, 300);
    ctx->backend.get_frame_time = _fd_get_frame_time;
}

// Run untouched frames with the pointer parked at (mx, my) until the delay
// is no longer 0; returns how many frames that took (limit + 1 = never).
static int _fd_settle(WLX_Context *ctx, int mx, int my, int limit) {
    for (int i = 1; i <= limit; i++) {
        _fd_frame(ctx, mx, my, false, false);
        if (wlx_frame_delay(ctx) != 0.0f) return i;
    }
    return limit + 1;
}

// ============================================================================
// The signal
// ============================================================================

TEST(frame_delay_reads_zero_before_the_first_end) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);
    wlx_context_destroy(&ctx);
}

TEST(frame_delay_static_ui_settles_to_unset) {
    WLX_Context ctx;
    _fd_reset(&ctx);

    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);          // the first frame
    ASSERT_TRUE(_fd_settle(&ctx, 300, 250, 3) <= 3);
    ASSERT_TRUE(wlx_frame_delay(&ctx) < 0.0f);               // nothing scheduled

    // A pointer move over nothing is input: one frame now, then rest again.
    _fd_frame(&ctx, 310, 255, false, false);
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);
    ASSERT_TRUE(_fd_settle(&ctx, 310, 255, 3) <= 3);
    ASSERT_TRUE(wlx_frame_delay(&ctx) < 0.0f);

    wlx_context_destroy(&ctx);
}

TEST(frame_digest_equal_for_identical_frames) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    test_stream_install(&ctx);
    _fd_frame(&ctx, 300, 250, false, false);
    _fd_frame(&ctx, 300, 250, false, false);

    test_stream_reset();
    _fd_frame(&ctx, 300, 250, false, false);
    uint64_t a = ctx.frame_digest;
    Test_Stream sa = test_stream_take();
    _fd_frame(&ctx, 300, 250, false, false);
    uint64_t b = ctx.frame_digest;
    Test_Stream sb = test_stream_take();

    ASSERT_TRUE(sa.count > 0);
    ASSERT_TRUE(test_stream_equal(&sa, &sb));     // the backend saw the same calls
    ASSERT_TRUE(a == b);                          // and the digest says so
    wlx_context_destroy(&ctx);
}

// A command the replay moves by its range offset is a different draw call,
// though the recorded command is the same.
TEST(frame_digest_follows_the_range_offset) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    WLX_Cmd c = { .type = WLX_CMD_RECT,
                  .data.rect = { { 10, 20, 100, 30 }, { 200, 10, 10, 255 } } };

    ctx.frame_digest = WLX_FRAME_DIGEST_SEED;
    wlx_frame_digest_cmd(&ctx, &c, 0.0f, 0.0f);
    uint64_t at_rest = ctx.frame_digest;
    ctx.frame_digest = WLX_FRAME_DIGEST_SEED;
    wlx_frame_digest_cmd(&ctx, &c, 0.0f, 5.0f);
    uint64_t moved_y = ctx.frame_digest;
    ctx.frame_digest = WLX_FRAME_DIGEST_SEED;
    wlx_frame_digest_cmd(&ctx, &c, 5.0f, 0.0f);
    uint64_t moved_x = ctx.frame_digest;
    ctx.frame_digest = WLX_FRAME_DIGEST_SEED;
    wlx_frame_digest_cmd(&ctx, &c, 0.0f, 0.0f);

    ASSERT_TRUE(ctx.frame_digest == at_rest);     // the fold is a function of its input
    ASSERT_TRUE(moved_y != at_rest);
    ASSERT_TRUE(moved_x != at_rest);
    ASSERT_TRUE(moved_x != moved_y);
    wlx_context_destroy(&ctx);
}

TEST(frame_digest_sees_text_bytes_and_colour) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(_fd_settle(&ctx, 300, 250, 3) <= 3);
    uint64_t base = ctx.frame_digest;

    // Same length, one byte different: the same number of commands at the
    // same places, a different draw.
    _fd_label_text = "statid";
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(ctx.frame_digest != base);
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);
    ASSERT_TRUE(_fd_settle(&ctx, 300, 250, 3) <= 3);
    uint64_t text_changed = ctx.frame_digest;

    _fd_button_color = (WLX_Color){ 40, 60, 91, 255 };
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(ctx.frame_digest != text_changed);
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);

    wlx_context_destroy(&ctx);
}

// Every argument the backend receives is in the digest. A recording
// backend folds each call's arguments as bytes; every bit of every command
// payload is flipped in turn and the command replayed, and wherever the
// recording changes the digest must change too. A fold left out of
// wlx_frame_digest_cmd (a colour, a thickness, the text style) fails here:
// a change of that argument alone would ask for no frame. Run once with the
// backend's shadow, glow and gradient callbacks and once without, where the
// core draws them through plain rects; that pass flips only the low three
// bits of each byte, since a flipped high bit in a layer count or a height
// asks the fallback for billions of rects. The replay offsets are varied
// too.
static uint64_t _fd_rec_h;
static size_t   _fd_rec_n;
static void _fd_rec(const void *p, size_t n) {
    const uint8_t *b = (const uint8_t *)p;
    for (size_t i = 0; i < n; i++) { _fd_rec_h ^= b[i]; _fd_rec_h *= 1099511628211ull; }
    _fd_rec_n += n;
}
#define _FD_REC(v) _fd_rec(&(v), sizeof(v))
static void _fd_rec_tag(int t) { _FD_REC(t); }
static void _fd_r_rect(WLX_Rect r, WLX_Color c, void *u) {
    (void)u; _fd_rec_tag(1); _FD_REC(r); _FD_REC(c); }
static void _fd_r_rect_lines(WLX_Rect r, float t, WLX_Color c, void *u) {
    (void)u; _fd_rec_tag(2); _FD_REC(r); _FD_REC(t); _FD_REC(c); }
static void _fd_r_rounded(WLX_Rect r, float ro, int sg, WLX_Color c, void *u) {
    (void)u; _fd_rec_tag(3); _FD_REC(r); _FD_REC(ro); _FD_REC(sg); _FD_REC(c); }
static void _fd_r_rounded_lines(WLX_Rect r, float ro, int sg, float t, WLX_Color c, void *u) {
    (void)u; _fd_rec_tag(4); _FD_REC(r); _FD_REC(ro); _FD_REC(sg); _FD_REC(t); _FD_REC(c); }
static void _fd_r_circle(float x, float y, float rad, int sg, WLX_Color c, void *u) {
    (void)u; _fd_rec_tag(5); _FD_REC(x); _FD_REC(y); _FD_REC(rad); _FD_REC(sg); _FD_REC(c); }
static void _fd_r_ring(float x, float y, float a, float b, int sg, WLX_Color c, void *u) {
    (void)u; _fd_rec_tag(6); _FD_REC(x); _FD_REC(y); _FD_REC(a); _FD_REC(b); _FD_REC(sg); _FD_REC(c); }
static void _fd_r_line(float x1, float y1, float x2, float y2, float t, WLX_Color c, void *u) {
    (void)u; _fd_rec_tag(7); _FD_REC(x1); _FD_REC(y1); _FD_REC(x2); _FD_REC(y2); _FD_REC(t); _FD_REC(c); }
static void _fd_r_text(const char *txt, size_t len, float x, float y, WLX_Text_Style st, void *u) {
    (void)u; _fd_rec_tag(8); _FD_REC(len); _fd_rec(txt, len); _FD_REC(x); _FD_REC(y);
    _FD_REC(st.font); _FD_REC(st.font_size); _FD_REC(st.color); _FD_REC(st.spacing); }
static void _fd_r_texture(WLX_Texture t, WLX_Rect src, WLX_Rect dst, WLX_Color c, void *u) {
    (void)u; _fd_rec_tag(9); _FD_REC(t); _FD_REC(src); _FD_REC(dst); _FD_REC(c); }
static void _fd_r_sc_begin(WLX_Rect r, void *u) { (void)u; _fd_rec_tag(10); _FD_REC(r); }
static void _fd_r_sc_end(void *u) { (void)u; _fd_rec_tag(11); }
static void _fd_r_shadow(WLX_Rect r, WLX_Color c, float ox, float oy, float blur, int layers,
                         float ro, int sg, void *u) {
    (void)u; _fd_rec_tag(12); _FD_REC(r); _FD_REC(c); _FD_REC(ox); _FD_REC(oy); _FD_REC(blur);
    _FD_REC(layers); _FD_REC(ro); _FD_REC(sg); }
static void _fd_r_glow(WLX_Rect r, WLX_Color c, float spread, int rings, float ro, int sg, void *u) {
    (void)u; _fd_rec_tag(13); _FD_REC(r); _FD_REC(c); _FD_REC(spread); _FD_REC(rings);
    _FD_REC(ro); _FD_REC(sg); }
static void _fd_r_grad(WLX_Rect r, WLX_Color top, WLX_Color bot, float ro, int sg, void *u) {
    (void)u; _fd_rec_tag(14); _FD_REC(r); _FD_REC(top); _FD_REC(bot); _FD_REC(ro); _FD_REC(sg); }

// Replay one command through the recording backend; the recording is left
// in _fd_rec_h / _fd_rec_n and the command's digest is returned.
static uint64_t _fd_replay(WLX_Context *ctx, WLX_Cmd *c, float dx, float dy) {
    _fd_rec_n = 0;
    _fd_rec_h = 14695981039346656037ull;
    ctx->frame_digest = WLX_FRAME_DIGEST_SEED;
    wlx_replay_dispatch_cmd(ctx, c, dx, dy);
    return ctx->frame_digest;
}

typedef struct { int flips, effective, misses; } _Fd_Sweep;

// Flip the low `bits` bits of every byte in [lo, hi) of the command, and
// each replay offset; count the flips that change the recording without
// changing the digest.
static void _fd_sweep(WLX_Context *ctx, const char *name, WLX_Cmd base,
                      size_t lo, size_t hi, int bits, _Fd_Sweep *sw) {
    uint64_t d0 = _fd_replay(ctx, &base, 3.0f, 5.0f);
    uint64_t h0 = _fd_rec_h;
    size_t   n0 = _fd_rec_n;
    for (size_t b = lo; b < hi; b++) {
        for (int bit = 0; bit < bits; bit++) {
            WLX_Cmd m = base;
            ((uint8_t *)&m)[b] ^= (uint8_t)(1u << bit);
            uint64_t d = _fd_replay(ctx, &m, 3.0f, 5.0f);
            bool differs = _fd_rec_n != n0 || _fd_rec_h != h0;
            sw->flips++;
            if (!differs) continue;
            sw->effective++;
            if (d == d0) {
                sw->misses++;
                fprintf(stderr, "    digest misses %s: payload byte %zu bit %d\n",
                    name, b - offsetof(WLX_Cmd, data), bit);
            }
        }
    }
    float offsets[2][2] = { { 4.0f, 5.0f }, { 3.0f, 6.0f } };
    for (int k = 0; k < 2; k++) {
        uint64_t d = _fd_replay(ctx, &base, offsets[k][0], offsets[k][1]);
        bool differs = _fd_rec_n != n0 || _fd_rec_h != h0;
        if (differs && d == d0) {
            sw->misses++;
            fprintf(stderr, "    digest misses %s: the replay offset on %s\n", name, k ? "y" : "x");
        }
    }
}
#define _FD_SWEEP(ctx, name, cmd, member, sw) \
    _fd_sweep((ctx), (name), (cmd), offsetof(WLX_Cmd, data), \
        offsetof(WLX_Cmd, data) + sizeof(((WLX_Cmd *)0)->data.member), bits, (sw))

TEST(frame_digest_folds_every_backend_argument) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    _Fd_Sweep sw = {0};
    for (int fallback = 0; fallback < 2; fallback++) {
        int bits = fallback ? 3 : 8;
        ctx.backend.draw_rect = _fd_r_rect;
        ctx.backend.draw_rect_lines = _fd_r_rect_lines;
        ctx.backend.draw_rect_rounded = _fd_r_rounded;
        ctx.backend.draw_rect_rounded_lines = _fd_r_rounded_lines;
        ctx.backend.draw_circle = _fd_r_circle;
        ctx.backend.draw_ring = _fd_r_ring;
        ctx.backend.draw_line = _fd_r_line;
        ctx.backend.draw_text_slice = _fd_r_text;
        ctx.backend.draw_texture = _fd_r_texture;
        ctx.backend.begin_scissor = _fd_r_sc_begin;
        ctx.backend.end_scissor = _fd_r_sc_end;
        ctx.backend.draw_shadow = fallback ? NULL : _fd_r_shadow;
        ctx.backend.draw_glow = fallback ? NULL : _fd_r_glow;
        ctx.backend.draw_gradient_v = fallback ? NULL : _fd_r_grad;

        WLX_Rect r = { 10, 20, 100, 30 };
        WLX_Color c1 = { 200, 10, 30, 255 }, c2 = { 1, 2, 3, 200 };
        WLX_Cmd c;
        memset(&c, 0, sizeof c); c.type = WLX_CMD_RECT;
        c.data.rect.rect = r; c.data.rect.color = c1;
        _FD_SWEEP(&ctx, "rect", c, rect, &sw);
        memset(&c, 0, sizeof c); c.type = WLX_CMD_RECT_LINES;
        c.data.rect_lines.rect = r; c.data.rect_lines.thick = 2; c.data.rect_lines.color = c1;
        _FD_SWEEP(&ctx, "rect_lines", c, rect_lines, &sw);
        memset(&c, 0, sizeof c); c.type = WLX_CMD_RECT_ROUNDED;
        c.data.rect_rounded.rect = r; c.data.rect_rounded.roundness = 0.3f;
        c.data.rect_rounded.segments = 8; c.data.rect_rounded.color = c1;
        _FD_SWEEP(&ctx, "rect_rounded", c, rect_rounded, &sw);
        memset(&c, 0, sizeof c); c.type = WLX_CMD_RECT_ROUNDED_LINES;
        c.data.rect_rounded_lines.rect = r; c.data.rect_rounded_lines.roundness = 0.3f;
        c.data.rect_rounded_lines.segments = 8; c.data.rect_rounded_lines.thick = 2;
        c.data.rect_rounded_lines.color = c1;
        _FD_SWEEP(&ctx, "rect_rounded_lines", c, rect_rounded_lines, &sw);
        memset(&c, 0, sizeof c); c.type = WLX_CMD_CIRCLE;
        c.data.circle.cx = 50; c.data.circle.cy = 60; c.data.circle.radius = 9;
        c.data.circle.segments = 12; c.data.circle.color = c1;
        _FD_SWEEP(&ctx, "circle", c, circle, &sw);
        memset(&c, 0, sizeof c); c.type = WLX_CMD_RING;
        c.data.ring.cx = 50; c.data.ring.cy = 60; c.data.ring.inner_r = 7; c.data.ring.outer_r = 9;
        c.data.ring.segments = 12; c.data.ring.color = c1;
        _FD_SWEEP(&ctx, "ring", c, ring, &sw);
        memset(&c, 0, sizeof c); c.type = WLX_CMD_LINE;
        c.data.line.x1 = 1; c.data.line.y1 = 2; c.data.line.x2 = 30; c.data.line.y2 = 40;
        c.data.line.thick = 2; c.data.line.color = c1;
        _FD_SWEEP(&ctx, "line", c, line, &sw);
        memset(&c, 0, sizeof c); c.type = WLX_CMD_TEXTURE;
        c.data.texture.texture.width = 64; c.data.texture.texture.height = 32;
        c.data.texture.src = (WLX_Rect){ 1, 2, 30, 20 }; c.data.texture.dst = r;
        c.data.texture.tint = c1;
        _FD_SWEEP(&ctx, "texture", c, texture, &sw);
        memset(&c, 0, sizeof c); c.type = WLX_CMD_SCISSOR_BEGIN;
        c.data.scissor_begin.rect = r;
        _FD_SWEEP(&ctx, "scissor_begin", c, scissor_begin, &sw);
        memset(&c, 0, sizeof c); c.type = WLX_CMD_SHADOW;
        c.data.shadow.rect = r; c.data.shadow.color = c1; c.data.shadow.offset_x = 3;
        c.data.shadow.offset_y = 4; c.data.shadow.blur = 6; c.data.shadow.layers = 4;
        c.data.shadow.roundness = 0.2f; c.data.shadow.rounded_segs = 6;
        _FD_SWEEP(&ctx, "shadow", c, shadow, &sw);
        memset(&c, 0, sizeof c); c.type = WLX_CMD_GLOW;
        c.data.glow.rect = r; c.data.glow.color = c1; c.data.glow.spread = 6; c.data.glow.rings = 4;
        c.data.glow.roundness = 0.2f; c.data.glow.rounded_segs = 6;
        _FD_SWEEP(&ctx, "glow", c, glow, &sw);
        memset(&c, 0, sizeof c); c.type = WLX_CMD_GRADIENT_V;
        c.data.gradient_v.rect = r; c.data.gradient_v.top = c1; c.data.gradient_v.bottom = c2;
        c.data.gradient_v.roundness = 0.2f; c.data.gradient_v.rounded_segs = 6;
        _FD_SWEEP(&ctx, "gradient_v", c, gradient_v, &sw);

        // Text: a recorded command, so its bytes live in the frame's
        // scratch. Every payload bit past the scratch span, then the bytes,
        // the length and the span's start.
        test_frame_begin(&ctx, 0, 0, false, false);
        WLX_Text_Style ts = { (WLX_Font)0x1234, 14, c1, 1 };
        wlx_draw_text(&ctx, "hello, digest", 10, 20, ts);
        WLX_Cmd t = wlx_pool_commands(&ctx)[ctx.arena.commands.count - 1];
        ASSERT_TRUE(t.type == WLX_CMD_TEXT);
        _fd_sweep(&ctx, "text", t, offsetof(WLX_Cmd, data.text.x),
                  offsetof(WLX_Cmd, data) + sizeof(t.data.text), bits, &sw);
        uint64_t d0 = _fd_replay(&ctx, &t, 0, 0);
        wlx_pool_scratch(&ctx)[t.data.text.text_off + 3] ^= 0x20;
        ASSERT_TRUE(_fd_replay(&ctx, &t, 0, 0) != d0);
        wlx_pool_scratch(&ctx)[t.data.text.text_off + 3] ^= 0x20;
        WLX_Cmd t2 = t;
        t2.data.text.text_len--;
        ASSERT_TRUE(_fd_replay(&ctx, &t2, 0, 0) != d0);
        t2 = t;
        t2.data.text.text_off++;
        t2.data.text.text_len--;
        ASSERT_TRUE(_fd_replay(&ctx, &t2, 0, 0) != d0);
        test_frame_end(&ctx);
    }
    ASSERT_EQ_INT(0, sw.misses);
    ASSERT_TRUE(sw.effective > 4000);                // the sweep reached every payload
    wlx_context_destroy(&ctx);
}

// An input can change what the next frame draws without changing this one:
// the application acts on a key after the widgets that show the result were
// built. The frame that carries the key draws what the frame before drew,
// so the input alone has to ask for the frame that follows.
TEST(frame_delay_input_asks_for_the_following_frame) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(_fd_settle(&ctx, 300, 250, 3) <= 3);
    uint64_t before = ctx.frame_digest;

    bool keys[WLX_KEY_COUNT] = {0};
    keys[WLX_KEY_A] = true;
    test_frame_begin_full(&ctx, 300, 250, false, false, false, 0.0f, keys, keys, NULL, 0, NULL);
    _fd_ui(&ctx);
    test_frame_end(&ctx);
    ASSERT_TRUE(ctx.frame_digest == before);         // drawn as before
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);  // yet the next frame is due

    _fd_label_text = "statid";                       // what the key did
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(ctx.frame_digest != before);
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);
    ASSERT_TRUE(_fd_settle(&ctx, 300, 250, 3) <= 3);
    ASSERT_TRUE(wlx_frame_delay(&ctx) < 0.0f);

    // Hover over static geometry needs no such help: the arrival frame
    // already draws it, and the comparison sees the change.
    uint64_t away = ctx.frame_digest;
    _fd_frame(&ctx, 50, 60, false, false);           // onto the button
    ASSERT_TRUE(ctx.frame_digest != away);
    ASSERT_TRUE(_fd_settle(&ctx, 50, 60, 3) <= 3);
    ASSERT_TRUE(wlx_frame_delay(&ctx) < 0.0f);
    wlx_context_destroy(&ctx);
}

TEST(frame_delay_held_unmoving_button_settles) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    _fd_frame(&ctx, 50, 60, false, false);
    ASSERT_TRUE(_fd_settle(&ctx, 50, 60, 4) <= 4);

    _fd_frame(&ctx, 50, 60, true, true);             // the press is an event
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);
    int n = 0;
    for (; n < 4; n++) {                             // held, not moving
        _fd_frame(&ctx, 50, 60, true, false);
        if (wlx_frame_delay(&ctx) < 0.0f) break;
    }
    ASSERT_TRUE(n < 4);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// The core's own timers
// ============================================================================

static void _fd_field_frame(WLX_Context *ctx, char *buf, size_t cap,
                            int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    (void)wlx_inputbox(ctx, NULL, buf, cap, .height = 40);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

TEST(frame_delay_focused_field_waits_for_the_blink_edge) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    char buf[32] = "abc";
    _fd_dt = 0.0f;                                   // the blink clock stands still

    _fd_field_frame(&ctx, buf, sizeof buf, 100, 20, false, false);
    _fd_field_frame(&ctx, buf, sizeof buf, 100, 20, true, true);    // focus
    _fd_field_frame(&ctx, buf, sizeof buf, 100, 20, false, false);  // release
    for (int i = 0; i < 4; i++) _fd_field_frame(&ctx, buf, sizeof buf, 100, 20, false, false);

    // Nothing but the caret is pending: half a period to its next edge.
    ASSERT_EQ_F(0.5f, wlx_frame_delay(&ctx), 1e-5f);

    // With the clock running the wait shrinks, and stays inside (0, 0.5].
    _fd_dt = 0.1f;
    _fd_field_frame(&ctx, buf, sizeof buf, 100, 20, false, false);
    float d = wlx_frame_delay(&ctx);
    ASSERT_TRUE(d > 0.0f && d <= 0.5f);
    ASSERT_EQ_F(0.4f, d, 1e-4f);

    wlx_context_destroy(&ctx);
}

// A focused text widget whose focus, or a keystroke, arrives on the frame
// that ends a rest: that frame's time is the whole rest. The blink must
// still start at its visible edge, with a full half period ahead, as it
// does in a loop that draws every frame. Run on the inputbox and on the
// editor, which share the clock.
static char   _fd_ed_buf[64];
static size_t _fd_ed_len;
static void _fd_caret_frame(WLX_Context *ctx, int which, char *buf, size_t cap,
                            const WLX_Input_State *in) {
    test_frame_begin_input(ctx, in);
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(60)), .padding = 0);
    if (which == 0) (void)wlx_inputbox(ctx, NULL, buf, cap, .height = 60);
    else (void)wlx_editor(ctx, NULL, _fd_ed_buf, sizeof _fd_ed_buf, &_fd_ed_len, .height = 60);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

TEST(frame_delay_caret_starts_visible_after_a_rest) {
    static const float rests[] = { 0.7f, 7.7f, 0.45f };
    for (int which = 0; which < 2; which++) {
        for (int r = 0; r < 3; r++) {
            WLX_Context ctx;
            _fd_reset(&ctx);
            char buf[32] = "abc";
            strcpy(_fd_ed_buf, "abc");
            _fd_ed_len = 3;
            WLX_Input_State in = {0};
            in.mouse_x = 100; in.mouse_y = 20;
            for (int i = 0; i < 3; i++) _fd_caret_frame(&ctx, which, buf, sizeof buf, &in);

            // Focus by a press that ends a rest.
            _fd_dt = rests[r];
            in.mouse_down = in.mouse_clicked = in.mouse_held = true;
            _fd_caret_frame(&ctx, which, buf, sizeof buf, &in);
            _fd_dt = 0.0f;                           // from here the clock stands still
            in.mouse_down = in.mouse_clicked = in.mouse_held = false;
            for (int i = 0; i < 3; i++) _fd_caret_frame(&ctx, which, buf, sizeof buf, &in);
            ASSERT_EQ_F(0.5f, wlx_frame_delay(&ctx), 1e-5f);

            // A character typed after a pause.
            _fd_dt = rests[r];
            WLX_Input_State key = in;
            strcpy(key.text_input, "x");
            _fd_caret_frame(&ctx, which, buf, sizeof buf, &key);
            _fd_dt = 0.0f;
            for (int i = 0; i < 3; i++) _fd_caret_frame(&ctx, which, buf, sizeof buf, &in);
            ASSERT_TRUE(which == 0 ? strcmp(buf, "abcx") == 0 : _fd_ed_len == 4);
            ASSERT_EQ_F(0.5f, wlx_frame_delay(&ctx), 1e-5f);
            wlx_context_destroy(&ctx);
        }
    }
}

static bool _fd_tip_frame(WLX_Context *ctx, int mx, int my) {
    test_frame_begin(ctx, mx, my, false, false);
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_label(ctx, "anchor", .height = 40);
    bool vis = wlx_tooltip_for(ctx, ((WLX_Rect){ 0, 0, 400, 40 }), "hint", .delay = 0.5f);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return vis;
}

// The tip shows once the pointer has rested on the anchor for the delay.
// The frame it arrives on adds nothing: in a loop that waits, that frame
// ends a rest and its frame time is the whole rest, none of it spent over
// the anchor.
TEST(frame_delay_tooltip_waits_for_its_delay) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    _fd_dt = 0.2f;
    for (int i = 0; i < 3; i++) (void)_fd_tip_frame(&ctx, 300, 200);   // off the anchor
    ASSERT_TRUE(wlx_frame_delay(&ctx) < 0.0f);

    _fd_dt = 30.0f;                                  // the arrival ends a 30 s rest
    ASSERT_TRUE(!_fd_tip_frame(&ctx, 50, 20));       // arrives: no hover yet
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);  // the move itself
    _fd_dt = 0.2f;
    ASSERT_TRUE(!_fd_tip_frame(&ctx, 50, 20));       // 0.2 s: 0.3 s to go
    ASSERT_EQ_F(0.3f, wlx_frame_delay(&ctx), 1e-4f);
    ASSERT_TRUE(!_fd_tip_frame(&ctx, 50, 20));       // 0.4 s: 0.1 s to go
    ASSERT_EQ_F(0.1f, wlx_frame_delay(&ctx), 1e-4f);
    ASSERT_TRUE(_fd_tip_frame(&ctx, 50, 20));        // 0.6 s: the tip draws
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);  // a change
    ASSERT_TRUE(_fd_tip_frame(&ctx, 50, 20));        // shown and resting
    ASSERT_TRUE(wlx_frame_delay(&ctx) < 0.0f);
    wlx_context_destroy(&ctx);
}

static void _fd_area_frame(WLX_Context *ctx, char *buf, size_t cap,
                           int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    (void)wlx_inputbox_impl(ctx, NULL, buf, cap,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .multiline = true),
        __FILE__, __LINE__);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// A drag held below the band keeps scrolling with no pointer motion while
// the view can still move: every such frame needs the next. Once the view
// is at the end of the text the frames ask for nothing more. The first
// frame past the edge steps nothing, so a long frame time there (the frame
// that ends a rest) is no jump: the scroll still takes many frames.
TEST(frame_delay_drag_autoscroll_runs_to_the_end) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    test_ctx_init(&ctx, 400, 60);
    ctx.backend.get_frame_time = _fd_get_frame_time;
    char buf[256] = "l0\nl1\nl2\nl3\nl4\nl5\nl6\nl7\nl8\nl9\n"
                    "m0\nm1\nm2\nm3\nm4\nm5\nm6\nm7\nm8\nm9\n"
                    "n0\nn1\nn2\nn3\nn4\nn5\nn6\nn7\nn8\nn9";

    _fd_area_frame(&ctx, buf, sizeof buf, 200, 30, true, true);      // press on line 2
    _fd_dt = 5.0f;                                   // past the edge after a 5 s rest
    _fd_area_frame(&ctx, buf, sizeof buf, 200, 100, true, false);
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);

    _fd_dt = 1.0f / 60.0f;
    int asking = 0;
    for (int i = 0; i < 200; i++) {
        _fd_area_frame(&ctx, buf, sizeof buf, 200, 100, true, false);
        if (wlx_frame_delay(&ctx) != 0.0f) break;
        asking++;
    }
    ASSERT_TRUE(asking >= 5);                        // it scrolled, frame by frame
    ASSERT_TRUE(asking < 200);                       // and stopped at the end

    // At the end: held still, nothing is asked for and nothing changes.
    _fd_dt = 0.0f;                                   // the caret's blink stands still
    uint64_t at_end = ctx.frame_digest;
    for (int i = 0; i < 30; i++) {
        _fd_area_frame(&ctx, buf, sizeof buf, 200, 100, true, false);
        ASSERT_TRUE(wlx_frame_delay(&ctx) != 0.0f);
        ASSERT_TRUE(ctx.frame_digest == at_end);
    }
    _fd_area_frame(&ctx, buf, sizeof buf, 200, 100, false, false);   // release
    wlx_context_destroy(&ctx);
}

// A press below a field that cannot scroll, dragged and held: the frames
// ask for nothing, though the pointer stays past the edge.
TEST(frame_delay_drag_past_an_unscrollable_field_rests) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    char buf[32] = "short";
    _fd_dt = 0.0f;                                   // the caret's blink stands still

    _fd_field_frame(&ctx, buf, sizeof buf, 100, 20, false, false);
    _fd_field_frame(&ctx, buf, sizeof buf, 100, 20, true, true);      // press in the text
    _fd_field_frame(&ctx, buf, sizeof buf, 120, 250, true, false);    // drag well below
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);  // the move itself
    for (int i = 0; i < 20; i++) {
        _fd_field_frame(&ctx, buf, sizeof buf, 120, 250, true, false);
        if (i > 0) ASSERT_TRUE(wlx_frame_delay(&ctx) > 0.0f);   // the caret's edge only
    }
    _fd_field_frame(&ctx, buf, sizeof buf, 120, 250, false, false);   // release
    wlx_context_destroy(&ctx);
}

TEST(frame_request_earliest_wins) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(_fd_settle(&ctx, 300, 250, 3) <= 3);
    ASSERT_TRUE(wlx_frame_delay(&ctx) < 0.0f);

    wlx_request_frame_after(&ctx, 2.0f);             // after wlx_end: lowers the value
    ASSERT_EQ_F(2.0f, wlx_frame_delay(&ctx), 0.0f);
    wlx_request_frame_after(&ctx, 1.0f);
    ASSERT_EQ_F(1.0f, wlx_frame_delay(&ctx), 0.0f);
    wlx_request_frame_after(&ctx, 3.0f);             // a later one never raises it
    ASSERT_EQ_F(1.0f, wlx_frame_delay(&ctx), 0.0f);
    wlx_request_frame(&ctx);
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);
    wlx_request_frame_after(&ctx, 5.0f);
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);
    wlx_context_destroy(&ctx);
}

TEST(frame_request_bounds_and_lifetime) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(_fd_settle(&ctx, 300, 250, 3) <= 3);

    wlx_request_frame_after(&ctx, -4.0f);            // non-positive means now
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);

    // wlx_begin drops the request: an unchanged frame rests again.
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(wlx_frame_delay(&ctx) < 0.0f);

    volatile float zero = 0.0f;
    wlx_request_frame_after(&ctx, zero / zero);      // not a number means now
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(wlx_frame_delay(&ctx) < 0.0f);

    // A request that never comes due is no request: nothing is scheduled,
    // and a timed request already made stands.
    wlx_request_frame_after(&ctx, INFINITY);
    ASSERT_TRUE(wlx_frame_delay(&ctx) < 0.0f);
    wlx_request_frame_after(&ctx, 2.0f);
    wlx_request_frame_after(&ctx, INFINITY);
    ASSERT_EQ_F(2.0f, wlx_frame_delay(&ctx), 0.0f);
    wlx_request_frame_after(&ctx, -INFINITY);        // non-positive: now
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(wlx_frame_delay(&ctx) < 0.0f);

    // A request made during the frame is the frame's answer when nothing
    // else asks for one sooner, and it is renewed only by asking again.
    _fd_request = 0.75f;
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_EQ_F(0.75f, wlx_frame_delay(&ctx), 0.0f);
    _fd_request = -1.0f;
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(wlx_frame_delay(&ctx) < 0.0f);

    // Input beats a timed request.
    _fd_request = 0.75f;
    _fd_frame(&ctx, 320, 250, false, false);
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// The input digest
// ============================================================================

// Every field of WLX_Input_State moves the digest. The size pin is the
// reminder: a field added to the struct changes it, and belongs in
// wlx_frame_input_digest and in this list.
TEST(frame_input_digest_covers_every_field) {
    ASSERT_EQ_INT(484, (int)sizeof(WLX_Input_State));

    WLX_Input_State base;
    memset(&base, 0, sizeof base);
    base.preedit_cursor = -1;
    base.preedit_sel_len = -1;
    bool event = true;
    uint64_t h0 = wlx_frame_input_digest(&base, &event);
    ASSERT_TRUE(!event);                             // an input at rest carries none

#define FD_FIELD(mutation, expect_event) do {                          \
        WLX_Input_State in = base;                                     \
        mutation;                                                      \
        bool ev = false;                                               \
        ASSERT_TRUE(wlx_frame_input_digest(&in, &ev) != h0);           \
        ASSERT_TRUE(ev == (expect_event));                             \
    } while (0)

    FD_FIELD(in.mouse_x = 7, false);
    FD_FIELD(in.mouse_y = 7, false);
    FD_FIELD(in.mouse_down = true, false);
    FD_FIELD(in.mouse_clicked = true, true);
    FD_FIELD(in.mouse_held = true, false);
    FD_FIELD(in.wheel_delta = 0.5f, true);
    FD_FIELD(in.keys_down[WLX_KEY_A] = true, false);
    FD_FIELD(in.keys_pressed[WLX_KEY_A] = true, true);
    FD_FIELD(in.text_input[0] = 'x', true);
    FD_FIELD(in.keys_repeated[WLX_KEY_A] = true, true);
    FD_FIELD(in.modifiers = WLX_MOD_SHIFT, false);
    FD_FIELD(in.wheel_delta_x = -0.5f, true);
    FD_FIELD(in.mouse_right_down = true, false);
    FD_FIELD(in.mouse_right_clicked = true, true);
    FD_FIELD(in.mouse_middle_down = true, false);
    FD_FIELD(in.mouse_middle_clicked = true, true);
    FD_FIELD(in.preedit[0] = 'k', false);
    FD_FIELD(in.preedit_cursor = 1, false);
    FD_FIELD(in.preedit_sel_len = 1, false);
#undef FD_FIELD

    // Which key is down matters, not only how many.
    WLX_Input_State a = base, b = base;
    a.keys_down[WLX_KEY_A] = true;
    b.keys_down[WLX_KEY_Z] = true;
    bool ev = false;
    ASSERT_TRUE(wlx_frame_input_digest(&a, &ev) != wlx_frame_input_digest(&b, &ev));
}

// ============================================================================
// Where there is nothing to compare, and what bypasses the replay
// ============================================================================

TEST(frame_delay_immediate_mode_always_asks) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    for (int i = 0; i < 4; i++) {
        memset(&_test_staged_input, 0, sizeof _test_staged_input);
        wlx_begin_immediate(&ctx, ctx.rect, _test_input_handler);
        wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
        wlx_label(&ctx, "static", .height = 40);
        wlx_layout_end(&ctx);
        wlx_end(&ctx);
        ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);
    }
    wlx_context_destroy(&ctx);
}

// The focus ring is drawn by a direct backend call after the replay; it
// folds itself into the digest, so a ring that moves is a change.
TEST(frame_digest_includes_the_focus_ring) {
    WLX_Context ctx;
    _fd_reset(&ctx);

    ctx.frame_digest = WLX_FRAME_DIGEST_SEED;
    wlx_focus_ring_draw(&ctx);                       // no ring held
    ASSERT_TRUE(ctx.frame_digest == WLX_FRAME_DIGEST_SEED);

    ctx.interaction.focus_id = 7;
    ctx.interaction.focus_clip_active = false;
    ctx.interaction.focus_rect = (WLX_Rect){ 10, 10, 50, 20 };
    ctx.frame_digest = WLX_FRAME_DIGEST_SEED;
    wlx_focus_ring_draw(&ctx);
    uint64_t first = ctx.frame_digest;
    ASSERT_TRUE(first != WLX_FRAME_DIGEST_SEED);

    ctx.interaction.focus_rect = (WLX_Rect){ 80, 10, 50, 20 };
    ctx.frame_digest = WLX_FRAME_DIGEST_SEED;
    wlx_focus_ring_draw(&ctx);
    ASSERT_TRUE(ctx.frame_digest != first);

    ctx.interaction.focus_id = 0;
    wlx_context_destroy(&ctx);
}

static WLX_Text_Style _fd_bigger(WLX_Text_Style style, void *user) {
    (void)user;
    style.font_size += 2;
    return style;
}

// Text leaves through the style transform: installing one changes what is
// drawn although no command changed.
TEST(frame_digest_follows_the_style_transform) {
    WLX_Context ctx;
    _fd_reset(&ctx);
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(_fd_settle(&ctx, 300, 250, 3) <= 3);
    uint64_t plain = ctx.frame_digest;

    wlx_set_style_transform(&ctx, _fd_bigger, NULL);
    _fd_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(ctx.frame_digest != plain);
    ASSERT_EQ_F(0.0f, wlx_frame_delay(&ctx), 0.0f);
    ASSERT_TRUE(_fd_settle(&ctx, 300, 250, 3) <= 3);
    ASSERT_TRUE(wlx_frame_delay(&ctx) < 0.0f);

    wlx_set_style_transform(&ctx, NULL, NULL);
    wlx_context_destroy(&ctx);
}

SUITE(frame_demand) {
    RUN_TEST(frame_delay_reads_zero_before_the_first_end);
    RUN_TEST(frame_delay_static_ui_settles_to_unset);
    RUN_TEST(frame_digest_equal_for_identical_frames);
    RUN_TEST(frame_digest_follows_the_range_offset);
    RUN_TEST(frame_digest_sees_text_bytes_and_colour);
    RUN_TEST(frame_digest_folds_every_backend_argument);
    RUN_TEST(frame_delay_input_asks_for_the_following_frame);
    RUN_TEST(frame_delay_held_unmoving_button_settles);
    RUN_TEST(frame_delay_focused_field_waits_for_the_blink_edge);
    RUN_TEST(frame_delay_caret_starts_visible_after_a_rest);
    RUN_TEST(frame_delay_tooltip_waits_for_its_delay);
    RUN_TEST(frame_delay_drag_autoscroll_runs_to_the_end);
    RUN_TEST(frame_delay_drag_past_an_unscrollable_field_rests);
    RUN_TEST(frame_request_earliest_wins);
    RUN_TEST(frame_request_bounds_and_lifetime);
    RUN_TEST(frame_input_digest_covers_every_field);
    RUN_TEST(frame_delay_immediate_mode_always_asks);
    RUN_TEST(frame_digest_includes_the_focus_ring);
    RUN_TEST(frame_digest_follows_the_style_transform);
}
