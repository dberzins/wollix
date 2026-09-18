// test_editor_windowed_origin.c - no-wrap windowed horizontal origin: a
// line wider than the unit budget re-enters at a measure origin near the
// view, and caret, hit-test, selection, and draw all agree on that origin
// (content-space x is origin_x + stored advance everywhere). Covers seam
// agreement on a deep-scrolled giant line, origin hysteresis (held views
// and small wheel moves never re-anchor), stitching continuity (scroll
// right then left reproduces identical geometry on the additive mock),
// tab stops restarting at the origin, END reaching the true end of a
// budget-deep line with the view following, editing at far offsets with
// the caret visible, and the near-content regime keeping every origin at
// the line start.
//
// Geometry model (mock backend): char width = font_size/2 = 5px at the
// fixture's font_size 10, line height = 10, tab advance 20. Fixture:
// 400x100 context, content_padding 4, border 0 -> band {9,4,383,92}
// before strips; the giant single-line documents show the horizontal bar
// only, so the working band is {9,4,383,82}. Unit budget 1024 units =
// 5120px of coverage per origin. The mock is additive, so every origin
// estimate is exact: content x == 5 * byte offset is asserted literally.

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
// Fixture
// ============================================================================

enum { WO_BAND_X_ = 9, WO_BAND_W_ = 383 };
#define WO_UNIT_W_ 5.0f

static unsigned long long wo_calls;

static void wo_counting_measure(const char *text, size_t len, WLX_Text_Style style,
                                float *out_w, float *out_h, void *user) {
    (void)user;
    (void)text;
    wo_calls++;
    int fs = style.font_size > 0 ? style.font_size : 10;
    if (out_w) *out_w = (float)len * (float)fs * 0.5f;
    if (out_h) *out_h = (float)fs;
}

#define WO_MAX_TEXTS_ 32
static struct { char text[80]; float x; float y; size_t len; } wo_texts[WO_MAX_TEXTS_];
static int wo_text_count;

static void wo_capture_draw_text(const char *text, float x, float y, WLX_Text_Style style, void *user) {
    (void)user;
    (void)style;
    if (wo_text_count < WO_MAX_TEXTS_) {
        const char *src = text ? text : "";
        size_t len = strlen(src);
        wo_texts[wo_text_count].len = len;
        if (len >= sizeof(wo_texts[0].text)) len = sizeof(wo_texts[0].text) - 1;
        memcpy(wo_texts[wo_text_count].text, src, len);
        wo_texts[wo_text_count].text[len] = '\0';
        wo_texts[wo_text_count].x = x;
        wo_texts[wo_text_count].y = y;
        wo_text_count++;
    }
}

#define WO_MAX_RECTS_ 64
static WLX_Rect wo_rects[WO_MAX_RECTS_];
static int wo_rect_count;

static void wo_capture_draw_rect(WLX_Rect r, WLX_Color c, void *user) {
    (void)user;
    (void)c;
    if (wo_rect_count < WO_MAX_RECTS_) wo_rects[wo_rect_count++] = r;
}

static void wo_reset_captures(void) {
    memset(wo_texts, 0, sizeof(wo_texts));
    wo_text_count = 0;
    memset(wo_rects, 0, sizeof(wo_rects));
    wo_rect_count = 0;
}

// advances true installs the batched measure callback; both measurement
// paths must produce identical origin geometry on the shared mock model.
static void wo_ctx_init_w(WLX_Context *ctx, bool advances, float ctx_w) {
    test_ctx_init(ctx, ctx_w, 100);
    ctx->backend.measure_text_slice = wo_counting_measure;
    ctx->backend.draw_text = wo_capture_draw_text;
    if (advances) test_install_mock_advances(ctx);
    wo_calls = 0;
    wo_reset_captures();
}

static void wo_ctx_init(WLX_Context *ctx, bool advances) {
    wo_ctx_init_w(ctx, advances, 400);
}

static unsigned long long wo_frame_full(WLX_Context *ctx, char *buf, size_t cap,
    size_t *len, int mx, int my, bool clicked, float wheel, uint32_t mods,
    const bool *keys_pressed, const char *text)
{
    wo_calls = 0;
    wo_reset_captures();
    test_frame_begin_full(ctx, mx, my, clicked, clicked, clicked, wheel,
        NULL, keys_pressed, NULL, mods, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    (void)wlx_editor_impl(ctx, NULL, buf, cap, len,
        wlx_default_editor_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0),
        "wo_editor", 1);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return wo_calls;
}

static unsigned long long wo_frame(WLX_Context *ctx, char *buf, size_t cap, size_t *len) {
    return wo_frame_full(ctx, buf, cap, len, 0, 0, false, 0.0f, 0, NULL, NULL);
}

static unsigned long long wo_frame_key(WLX_Context *ctx, char *buf, size_t cap,
    size_t *len, WLX_Key_Code key, uint32_t mods)
{
    bool keys[WLX_KEY_COUNT] = {0};
    keys[key] = true;
    return wo_frame_full(ctx, buf, cap, len, 200, 50, false, 0.0f, mods, keys, NULL);
}

static WLX_Editor_State *wo_state(WLX_Context *ctx) {
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        WLX_State_Map_Slot *slot = &ctx->states.slots[i];
        if (slot->id != 0 && slot->data_size == sizeof(WLX_Editor_State))
            return (WLX_Editor_State *)slot->data;
    }
    return NULL;
}

static WLX_Editor_Line_Index *wo_index(WLX_Context *ctx) {
    return ctx->editor_indices.count > 0 ? &ctx->editor_indices.items[0] : NULL;
}

static WLX_Text_Geom_Entry *wo_entry(WLX_Context *ctx, size_t line_start) {
    WLX_Editor_Line_Index *idx = wo_index(ctx);
    if (idx == NULL) return NULL;
    for (size_t i = 0; i < idx->geom.count; i++) {
        WLX_Text_Geom_Entry *e = &idx->geom.entries[i];
        if (e->used && e->line_start == line_start) return e;
    }
    return NULL;
}

static float wo_caret_x(WLX_Context *ctx, char *buf, size_t len, size_t caret) {
    WLX_Editor_Line_Index *idx = wo_index(ctx);
    WLX_Text_Style ts = { .font_size = 10 };
    return wlx_editor_caret_x(ctx, buf, len, ts, 20.0f, &idx->geom, len, 0, caret, true);
}

static size_t wo_offset_at_x(WLX_Context *ctx, char *buf, size_t len,
    float scroll_x, float content_x)
{
    WLX_Editor_Line_Index *idx = wo_index(ctx);
    WLX_Text_Style ts = { .font_size = 10 };
    return wlx_editor_offset_at_x(ctx, buf, len, ts, 10.0f, 20.0f, idx,
        &idx->geom, 0, scroll_x + (float)WO_BAND_W_, (float)WO_BAND_W_, content_x);
}

// One giant hard line of 'a' (no newline): 8000 units = 40000px, nearly
// eight budget windows wide.
enum { WO_GIANT_ = 8000 };

static size_t wo_fill_giant(char *buf, size_t cap) {
    size_t n = WO_GIANT_ < cap - 1 ? WO_GIANT_ : cap - 1;
    memset(buf, 'a', n);
    buf[n] = '\0';
    return n;
}

// The additive-mock exactness invariant for tab-free content: an origin's
// frozen x always equals 5px times its byte offset, whether it was set at
// the line start, stitched, or estimated across a far jump.
static void wo_assert_origin_exact(WLX_Context *ctx, size_t line_start) {
    WLX_Text_Geom_Entry *e = wo_entry(ctx, line_start);
    ASSERT_TRUE(e != NULL);
    ASSERT_EQ_F(WO_UNIT_W_ * (float)e->origin_rel, e->origin_x, 0.01f);
}

// ============================================================================
// Seam agreement: caret, hit-test, and draw share the window's origin
// ============================================================================

TEST(origin_deep_view_caret_hit_draw_agree) {
    for (int adv = 0; adv < 2; adv++) {
        WLX_Context ctx;
        wo_ctx_init(&ctx, adv == 1);
        static char buf[8200];
        size_t len = wo_fill_giant(buf, sizeof(buf));

        wo_frame(&ctx, buf, sizeof(buf), &len);
        wo_frame(&ctx, buf, sizeof(buf), &len);
        WLX_Editor_State *st = wo_state(&ctx);
        ASSERT_TRUE(st != NULL);

        // Park the view far beyond the budget's reach from the line start.
        st->scroll_x = 20000.0f;
        wo_frame(&ctx, buf, sizeof(buf), &len);
        wo_frame(&ctx, buf, sizeof(buf), &len);

        WLX_Text_Geom_Entry *e = wo_entry(&ctx, 0);
        ASSERT_TRUE(e != NULL);
        ASSERT_TRUE(e->origin_rel > 0);
        wo_assert_origin_exact(&ctx, 0);
        // The measured band covers the view: origin at or left of the
        // view's left edge, coverage past its right edge.
        ASSERT_TRUE(e->origin_x <= st->scroll_x);
        ASSERT_TRUE(e->units > 0);
        ASSERT_TRUE(e->origin_x + e->advances[e->units - 1]
            >= st->scroll_x + (float)WO_BAND_W_);

        // Caret x and hit-test round-trip at and around the origin and
        // across the view (chunk splices included on the advances path).
        size_t probes[] = { (size_t)e->origin_rel, (size_t)e->origin_rel + 1,
            (size_t)e->origin_rel + 57, 4000, 4019, 4038, 4076 };
        for (size_t i = 0; i < sizeof(probes) / sizeof(probes[0]); i++) {
            size_t b = probes[i];
            float x = wo_caret_x(&ctx, buf, len, b);
            ASSERT_EQ_F(WO_UNIT_W_ * (float)b, x, 0.01f);
            ASSERT_EQ_INT((long)b,
                (long)wo_offset_at_x(&ctx, buf, len, st->scroll_x, x + 0.1f));
        }

        // A real click through the frame: screen x 200 is content x
        // 20000 + (200 - 9) = 20191 -> between boundaries 4038 and 4039,
        // below their midpoint.
        wo_frame_full(&ctx, buf, sizeof(buf), &len, 200, 10, true, 0.0f, 0, NULL, NULL);
        ASSERT_EQ_INT(4038, (long)st->caret.cursor_pos);

        // The drawn window run starts exactly at the origin's screen x.
        wo_frame(&ctx, buf, sizeof(buf), &len);
        e = wo_entry(&ctx, 0);
        ASSERT_TRUE(e != NULL);
        float want_x = (float)WO_BAND_X_ - st->scroll_x + e->origin_x;
        bool found = false;
        for (int i = 0; i < wo_text_count; i++) {
            if (wo_texts[i].text[0] == 'a' && fabsf(wo_texts[i].x - want_x) < 0.01f) {
                found = true;
                break;
            }
        }
        ASSERT_TRUE(found);
        wlx_context_destroy(&ctx);
    }
}

TEST(origin_selection_highlight_clamps_at_seam) {
    WLX_Context ctx;
    wo_ctx_init(&ctx, false);
    ctx.backend.draw_rect = wo_capture_draw_rect;
    static char buf[8200];
    size_t len = wo_fill_giant(buf, sizeof(buf));

    wo_frame(&ctx, buf, sizeof(buf), &len);
    WLX_Editor_State *st = wo_state(&ctx);
    ASSERT_TRUE(st != NULL);
    st->scroll_x = 20000.0f;
    wo_frame(&ctx, buf, sizeof(buf), &len);
    // Focus with a click inside the band, then select a span reaching
    // back across the origin seam.
    wo_frame_full(&ctx, buf, sizeof(buf), &len, 200, 10, true, 0.0f, 0, NULL, NULL);
    WLX_Text_Geom_Entry *e = wo_entry(&ctx, 0);
    ASSERT_TRUE(e != NULL && e->origin_rel > 100);
    size_t origin = e->origin_rel;
    st->caret.selection_anchor = origin - 100;
    st->caret.cursor_pos = origin + 40;
    st->caret.prev_cursor_pos = origin + 40; // no caret-follow this frame
    wo_frame(&ctx, buf, sizeof(buf), &len);

    // The highlight clamps to the record's visible start (the origin):
    // x at the origin's screen position, width covering origin..caret.
    e = wo_entry(&ctx, 0);
    ASSERT_TRUE(e != NULL);
    ASSERT_EQ_INT((long)origin, (long)e->origin_rel);
    float want_x = (float)WO_BAND_X_ - st->scroll_x + e->origin_x;
    float want_w = 40.0f * WO_UNIT_W_;
    bool found = false;
    for (int i = 0; i < wo_rect_count; i++) {
        if (fabsf(wo_rects[i].x - want_x) < 0.01f
            && fabsf(wo_rects[i].w - want_w) < 0.01f
            && fabsf(wo_rects[i].h - 10.0f) < 0.01f) {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Hysteresis: held views and small moves never re-anchor
// ============================================================================

TEST(origin_held_view_and_small_wheel_never_reanchor) {
    WLX_Context ctx;
    wo_ctx_init(&ctx, false);
    static char buf[8200];
    size_t len = wo_fill_giant(buf, sizeof(buf));

    wo_frame(&ctx, buf, sizeof(buf), &len);
    WLX_Editor_State *st = wo_state(&ctx);
    ASSERT_TRUE(st != NULL);
    st->scroll_x = 20000.0f;
    wo_frame(&ctx, buf, sizeof(buf), &len);
    WLX_Text_Geom_Entry *e = wo_entry(&ctx, 0);
    ASSERT_TRUE(e != NULL);
    uint32_t origin0 = e->origin_rel;
    ASSERT_TRUE(origin0 > 0);

    // Held view: idle frames issue only the frame's reference measure and
    // never move the origin.
    for (int i = 0; i < 6; i++) {
        unsigned long long calls = wo_frame(&ctx, buf, sizeof(buf), &len);
        ASSERT_EQ_INT(1, (long)calls);
        e = wo_entry(&ctx, 0);
        ASSERT_TRUE(e != NULL);
        ASSERT_EQ_INT((long)origin0, (long)e->origin_rel);
    }

    // One wheel notch left stays inside the origin's back margin: the
    // view moves, the origin does not, and nothing re-measures.
    unsigned long long calls = wo_frame_full(&ctx, buf, sizeof(buf), &len,
        200, 50, false, 1.0f, WLX_MOD_SHIFT, NULL, NULL);
    ASSERT_TRUE(st->scroll_x < 20000.0f && st->scroll_x > 19800.0f);
    ASSERT_EQ_INT(1, (long)calls);
    e = wo_entry(&ctx, 0);
    ASSERT_TRUE(e != NULL);
    ASSERT_EQ_INT((long)origin0, (long)e->origin_rel);

    // Wheeling left past the origin re-anchors (retreat) exactly once the
    // view's left edge crosses it - and stays exact.
    for (int i = 0; i < 8; i++) {
        wo_frame_full(&ctx, buf, sizeof(buf), &len, 200, 50, false, 2.0f,
            WLX_MOD_SHIFT, NULL, NULL);
    }
    e = wo_entry(&ctx, 0);
    ASSERT_TRUE(e != NULL);
    ASSERT_TRUE(st->scroll_x < WO_UNIT_W_ * (float)origin0);
    ASSERT_TRUE(e->origin_rel < origin0);
    wo_assert_origin_exact(&ctx, 0);
    ASSERT_TRUE(e->origin_x <= st->scroll_x);
    wlx_context_destroy(&ctx);
}

// A view wider than a budget's worth of units (a very wide band, a tiny
// font, or a narrowed WLX_EDITOR_MAX_LINE_UNITS) cannot be covered from
// any origin: the half-view back margin the advance rule wants is then
// unaffordable. The policy must still settle - aim at the view's left
// edge so the whole budget lands inside the view, and hold there. Before
// this, advance crept the origin one unit per frame (target_x behind the
// origin picked the first covering boundary) while retreat pulled it
// back, and every frame of that sawtooth re-measured the whole budget on
// a motionless view.
TEST(origin_view_wider_than_budget_settles_without_thrash) {
    for (int adv = 0; adv < 2; adv++) {
        WLX_Context ctx;
        // Band ~5983px against 1024 units * 5px = 5120px of coverage.
        wo_ctx_init_w(&ctx, adv == 1, 6000);
        static char buf[8200];
        size_t len = wo_fill_giant(buf, sizeof(buf));

        wo_frame(&ctx, buf, sizeof(buf), &len);
        WLX_Editor_State *st = wo_state(&ctx);
        ASSERT_TRUE(st != NULL);

        // At rest the origin stays at the line start: there is nothing
        // left of the view to skip, so no advance may drift off it.
        for (int i = 0; i < 4; i++) {
            unsigned long long calls = wo_frame(&ctx, buf, sizeof(buf), &len);
            WLX_Text_Geom_Entry *e = wo_entry(&ctx, 0);
            ASSERT_TRUE(e != NULL);
            ASSERT_EQ_INT(0, (long)e->origin_rel);
            ASSERT_EQ_F(0.0f, e->origin_x, 0.0f);
            ASSERT_EQ_INT(1, (long)calls);
        }

        // Parked deep, the origin settles at the view's left edge - every
        // measured unit inside the view - and stays exact.
        st->scroll_x = 20000.0f;
        for (int i = 0; i < 3; i++) wo_frame(&ctx, buf, sizeof(buf), &len);
        WLX_Text_Geom_Entry *e = wo_entry(&ctx, 0);
        ASSERT_TRUE(e != NULL && e->units > 0);
        uint32_t settled = e->origin_rel;
        ASSERT_TRUE(settled > 0);
        wo_assert_origin_exact(&ctx, 0);
        ASSERT_TRUE(e->origin_x <= st->scroll_x);
        ASSERT_TRUE(e->origin_x > st->scroll_x - WO_UNIT_W_);
        ASSERT_TRUE(e->origin_x + e->advances[e->units - 1] > st->scroll_x);

        // Held there, the view neither re-anchors nor re-measures.
        for (int i = 0; i < 5; i++) {
            unsigned long long calls = wo_frame(&ctx, buf, sizeof(buf), &len);
            e = wo_entry(&ctx, 0);
            ASSERT_TRUE(e != NULL);
            ASSERT_EQ_INT((long)settled, (long)e->origin_rel);
            ASSERT_EQ_INT(1, (long)calls);
        }

        // Caret geometry still resolves exactly through that origin.
        float x = wo_caret_x(&ctx, buf, len, (size_t)settled + 100);
        ASSERT_EQ_F(WO_UNIT_W_ * (float)(settled + 100), x, 0.01f);
        wlx_context_destroy(&ctx);
    }
}

// ============================================================================
// Stitching continuity: right then left reproduces identical geometry
// ============================================================================

TEST(origin_stitching_sweep_right_then_left_stays_exact) {
    for (int adv = 0; adv < 2; adv++) {
        WLX_Context ctx;
        wo_ctx_init(&ctx, adv == 1);
        static char buf[8200];
        size_t len = wo_fill_giant(buf, sizeof(buf));

        wo_frame(&ctx, buf, sizeof(buf), &len);
        WLX_Editor_State *st = wo_state(&ctx);
        ASSERT_TRUE(st != NULL);

        // Sweep deep in coarse jumps (each beyond the previous coverage),
        // then sweep back to zero. Every parked view must resolve carets
        // at the exact additive-mock position, whatever chain of origin
        // moves produced it.
        float stops[] = { 3000.0f, 9000.0f, 15000.0f, 24000.0f, 33000.0f,
                          24000.0f, 15000.0f, 9000.0f, 3000.0f, 0.0f };
        for (size_t i = 0; i < sizeof(stops) / sizeof(stops[0]); i++) {
            st->scroll_x = stops[i];
            wo_frame(&ctx, buf, sizeof(buf), &len);
            wo_assert_origin_exact(&ctx, 0);
            size_t b = (size_t)((stops[i] + 100.0f) / WO_UNIT_W_);
            if (b > len) b = len;
            float x = wo_caret_x(&ctx, buf, len, b);
            ASSERT_EQ_F(WO_UNIT_W_ * (float)b, x, 0.01f);
            ASSERT_EQ_INT((long)b,
                (long)wo_offset_at_x(&ctx, buf, len, stops[i], x + 0.1f));
        }
        // Back at rest the origin is the line start again, exactly.
        WLX_Text_Geom_Entry *e = wo_entry(&ctx, 0);
        ASSERT_TRUE(e != NULL);
        ASSERT_EQ_INT(0, (long)e->origin_rel);
        ASSERT_EQ_F(0.0f, e->origin_x, 0.0f);
        wlx_context_destroy(&ctx);
    }
}

// ============================================================================
// Tab stops restart at the origin
// ============================================================================

TEST(origin_tab_stops_restart_at_origin) {
    WLX_Context ctx;
    wo_ctx_init(&ctx, false);
    static char buf[8200];
    size_t len = wo_fill_giant(buf, sizeof(buf));
    // A tab every 100 bytes; tab advance is 20px at the fixture metrics.
    for (size_t i = 99; i < len; i += 100) buf[i] = '\t';

    wo_frame(&ctx, buf, sizeof(buf), &len);
    WLX_Editor_State *st = wo_state(&ctx);
    ASSERT_TRUE(st != NULL);
    st->scroll_x = 20000.0f;
    wo_frame(&ctx, buf, sizeof(buf), &len);
    wo_frame(&ctx, buf, sizeof(buf), &len);

    WLX_Text_Geom_Entry *e = wo_entry(&ctx, 0);
    ASSERT_TRUE(e != NULL && e->origin_rel > 0 && e->units > 0);
    size_t origin = e->origin_rel;
    float origin_x = e->origin_x;

    // Mirror the origin-relative tab walk: segments advance 5px per byte,
    // each tab snaps to the next 20px stop from the origin.
    size_t first_tab = origin;
    while (first_tab < len && buf[first_tab] != '\t') first_tab++;
    ASSERT_TRUE(first_tab + 1 < len);
    float before = WO_UNIT_W_ * (float)(first_tab - origin);
    float after = (floorf(before / 20.0f) + 1.0f) * 20.0f;

    float x_before = wo_caret_x(&ctx, buf, len, first_tab);
    float x_after = wo_caret_x(&ctx, buf, len, first_tab + 1);
    ASSERT_EQ_F(origin_x + before, x_before, 0.01f);
    ASSERT_EQ_F(origin_x + after, x_after, 0.01f);

    // Hit-test round-trip stays consistent with the same walk.
    ASSERT_EQ_INT((long)(first_tab + 1),
        (long)wo_offset_at_x(&ctx, buf, len, st->scroll_x, x_after + 0.1f));
    wlx_context_destroy(&ctx);
}

// ============================================================================
// END and editing at far offsets
// ============================================================================

TEST(origin_end_reaches_line_end_and_view_follows) {
    for (int adv = 0; adv < 2; adv++) {
        WLX_Context ctx;
        wo_ctx_init(&ctx, adv == 1);
        static char buf[8200];
        size_t len = wo_fill_giant(buf, sizeof(buf));

        wo_frame(&ctx, buf, sizeof(buf), &len);
        WLX_Editor_State *st = wo_state(&ctx);
        ASSERT_TRUE(st != NULL);
        // Focus at the start, then END: the caret lands on the last byte
        // and the view follows to the line's true end - the unit budget
        // caps a window's record, never the reach.
        wo_frame_full(&ctx, buf, sizeof(buf), &len, 20, 10, true, 0.0f, 0, NULL, NULL);
        wo_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, 0);

        ASSERT_EQ_INT((long)len, (long)st->caret.cursor_pos);
        float end_x = WO_UNIT_W_ * (float)len;
        ASSERT_EQ_F(end_x, wo_caret_x(&ctx, buf, len, len), 0.01f);
        // Caret-follow parks the view at caret + margin - band.
        ASSERT_EQ_F(end_x + 4.0f - (float)WO_BAND_W_, st->scroll_x, 0.5f);
        wo_assert_origin_exact(&ctx, 0);

        // The parked view is stable: idle frames keep the scroll and issue
        // only the reference measure.
        float parked = st->scroll_x;
        for (int i = 0; i < 4; i++) {
            unsigned long long calls = wo_frame(&ctx, buf, sizeof(buf), &len);
            ASSERT_EQ_F(parked, st->scroll_x, 0.001f);
            ASSERT_EQ_INT(1, (long)calls);
        }

        // Ctrl+HOME returns exactly to the line start regime.
        wo_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_HOME, WLX_MOD_CTRL);
        ASSERT_EQ_INT(0, (long)st->caret.cursor_pos);
        ASSERT_EQ_F(0.0f, st->scroll_x, 0.001f);
        wo_frame(&ctx, buf, sizeof(buf), &len);
        WLX_Text_Geom_Entry *e = wo_entry(&ctx, 0);
        ASSERT_TRUE(e != NULL);
        ASSERT_EQ_INT(0, (long)e->origin_rel);
        wlx_context_destroy(&ctx);
    }
}

TEST(origin_editing_at_far_depth_keeps_caret_visible_and_exact) {
    for (int adv = 0; adv < 2; adv++) {
        WLX_Context ctx;
        wo_ctx_init(&ctx, adv == 1);
        static char buf[8200];
        size_t len = wo_fill_giant(buf, sizeof(buf));
        size_t base = len;

        wo_frame(&ctx, buf, sizeof(buf), &len);
        WLX_Editor_State *st = wo_state(&ctx);
        ASSERT_TRUE(st != NULL);
        wo_frame_full(&ctx, buf, sizeof(buf), &len, 20, 10, true, 0.0f, 0, NULL, NULL);
        wo_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, 0);

        // Append at the line's end: every keystroke lands in the buffer,
        // the caret x stays byte-exact, and the view keeps the caret
        // inside the band.
        const char *typed = "xyz";
        for (int i = 0; i < 3; i++) {
            char one[2] = { typed[i], '\0' };
            wo_frame_full(&ctx, buf, sizeof(buf), &len, 0, 0, false, 0.0f, 0, NULL, one);
            ASSERT_EQ_INT((long)(base + (size_t)i + 1), (long)len);
            ASSERT_EQ_INT((long)len, (long)st->caret.cursor_pos);
            float cx = wo_caret_x(&ctx, buf, len, st->caret.cursor_pos);
            ASSERT_EQ_F(WO_UNIT_W_ * (float)len, cx, 0.05f);
            float screen = (float)WO_BAND_X_ - st->scroll_x + cx;
            ASSERT_TRUE(screen >= (float)WO_BAND_X_);
            ASSERT_TRUE(screen <= (float)(WO_BAND_X_ + WO_BAND_W_));
        }
        ASSERT_TRUE(memcmp(buf + base, "xyz", 3) == 0);

        // Step back into the line (99% depth) and insert: the edit lands
        // at the caret's byte, not at some frozen reach.
        for (int i = 0; i < 5; i++)
            wo_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
        size_t at = st->caret.cursor_pos;
        ASSERT_EQ_INT((long)(len - 5), (long)at);
        wo_frame_full(&ctx, buf, sizeof(buf), &len, 0, 0, false, 0.0f, 0, NULL, "Q");
        ASSERT_EQ_INT('Q', (long)buf[at]);
        ASSERT_EQ_INT((long)(at + 1), (long)st->caret.cursor_pos);
        float cx = wo_caret_x(&ctx, buf, len, st->caret.cursor_pos);
        ASSERT_EQ_F(WO_UNIT_W_ * (float)(at + 1), cx, 0.05f);

        // Deleting back out again stays visible and exact.
        wo_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
        ASSERT_EQ_INT((long)at, (long)st->caret.cursor_pos);
        ASSERT_EQ_INT('a', (long)buf[at]);
        cx = wo_caret_x(&ctx, buf, len, st->caret.cursor_pos);
        float screen = (float)WO_BAND_X_ - st->scroll_x + cx;
        ASSERT_TRUE(screen >= (float)WO_BAND_X_ - 0.01f);
        ASSERT_TRUE(screen <= (float)(WO_BAND_X_ + WO_BAND_W_) + 0.01f);
        wlx_context_destroy(&ctx);
    }
}

// ============================================================================
// Near-content regime: origins stay at line starts
// ============================================================================

TEST(origin_near_content_keeps_line_start_origins) {
    WLX_Context ctx;
    wo_ctx_init(&ctx, false);
    static char buf[8400];
    // 40 lines of 200 chars: 1000px wide, well inside one budget window.
    size_t off = 0;
    for (int i = 0; i < 40; i++) {
        memset(buf + off, 'a' + (i % 26), 200);
        off += 200;
        buf[off++] = '\n';
    }
    buf[off] = '\0';
    size_t len = off;

    wo_frame(&ctx, buf, sizeof(buf), &len);
    WLX_Editor_State *st = wo_state(&ctx);
    ASSERT_TRUE(st != NULL);
    st->scroll_x = 500.0f;
    wo_frame(&ctx, buf, sizeof(buf), &len);
    wo_frame(&ctx, buf, sizeof(buf), &len);

    WLX_Editor_Line_Index *idx = wo_index(&ctx);
    ASSERT_TRUE(idx != NULL);
    size_t used = 0;
    for (size_t i = 0; i < idx->geom.count; i++) {
        WLX_Text_Geom_Entry *e = &idx->geom.entries[i];
        if (!e->used) continue;
        used++;
        ASSERT_EQ_INT(0, (long)e->origin_rel);
        ASSERT_EQ_F(0.0f, e->origin_x, 0.0f);
    }
    ASSERT_TRUE(used > 0);
    wlx_context_destroy(&ctx);
}

SUITE(editor_windowed_origin) {
    RUN_TEST(origin_deep_view_caret_hit_draw_agree);
    RUN_TEST(origin_selection_highlight_clamps_at_seam);
    RUN_TEST(origin_held_view_and_small_wheel_never_reanchor);
    RUN_TEST(origin_view_wider_than_budget_settles_without_thrash);
    RUN_TEST(origin_stitching_sweep_right_then_left_stays_exact);
    RUN_TEST(origin_tab_stops_restart_at_origin);
    RUN_TEST(origin_end_reaches_line_end_and_view_follows);
    RUN_TEST(origin_editing_at_far_depth_keeps_caret_visible_and_exact);
    RUN_TEST(origin_near_content_keeps_line_start_origins);
}
