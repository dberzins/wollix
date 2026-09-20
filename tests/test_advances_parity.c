// test_advances_parity.c - measure_text_advances backend parity: window
// build records, caret x, hit-test offsets, selection spans, and whole-frame
// draw command streams must be identical with the callback present (chunked
// advances fills) and absent (per-unit prefix measuring) over a pinned
// corpus of ASCII, multibyte UTF-8, malformed bytes, tabs, and CRLF, in
// both wrap modes.
//
// Geometry model (mock backend): char width = font_size/2 = 5px at the
// fixture's font_size 10, line height = 10. The mock advances implement the
// same per-byte model, so any divergence is a defect in the batching walk
// (chunk splices, tab stops, unit policy), not in the model.

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
// Fixture: a context pair running identical frames, one per measure path
// ============================================================================

static unsigned long long ap_slice_calls;
static unsigned long long ap_adv_calls;

static void ap_measure_slice(const char *text, size_t len, WLX_Text_Style style,
                             float *out_w, float *out_h, void *user) {
    (void)user;
    (void)text;
    ap_slice_calls++;
    int fs = style.font_size > 0 ? style.font_size : 10;
    if (out_w) *out_w = (float)len * (float)fs * 0.5f;
    if (out_h) *out_h = (float)fs;
}

static size_t ap_measure_advances(const char *text, size_t len,
        WLX_Text_Style style, const size_t *unit_ends, size_t unit_count,
        float *out_advances, void *user) {
    (void)user;
    ap_adv_calls++;
    return mock_measure_text_advances(text, len, style, unit_ends, unit_count,
        out_advances, user);
}

typedef struct {
    WLX_Context fallback;  // per-unit prefix measuring only
    WLX_Context advances;  // + chunked advances callback
} AP_Pair;

static void ap_pair_init(AP_Pair *p, float w, float h) {
    test_ctx_init(&p->fallback, w, h);
    p->fallback.backend.measure_text_slice = ap_measure_slice;
    test_ctx_init(&p->advances, w, h);
    p->advances.backend.measure_text_slice = ap_measure_slice;
    p->advances.backend.measure_text_advances = ap_measure_advances;
    ap_slice_calls = 0;
    ap_adv_calls = 0;
}

static void ap_pair_destroy(AP_Pair *p) {
    wlx_context_destroy(&p->fallback);
    wlx_context_destroy(&p->advances);
}

// One editor frame on one context; mirrors the geom-cache fixture.
static void ap_frame(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
    bool wrap, int mx, int my, bool clicked, float wheel, uint32_t mods,
    const bool *keys_pressed, const char *text)
{
    test_frame_begin_full(ctx, mx, my, clicked, clicked, clicked, wheel,
        NULL, keys_pressed, NULL, mods, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    (void)wlx_editor_impl(ctx, NULL, buf, cap, len,
        wlx_default_editor_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .wrap = wrap),
        "ap_editor", 1);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static WLX_Editor_Line_Index *ap_index(WLX_Context *ctx) {
    return ctx->editor_indices.count > 0 ? &ctx->editor_indices.items[0] : NULL;
}

static WLX_Editor_State *ap_state(WLX_Context *ctx) {
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        WLX_State_Map_Slot *slot = &ctx->states.slots[i];
        if (slot->id != 0 && slot->data_size == sizeof(WLX_Editor_State))
            return (WLX_Editor_State *)slot->data;
    }
    return NULL;
}

// ============================================================================
// Whole-frame draw parity: every command of the frame identical per field
// ============================================================================

static void ap_assert_colors_equal(WLX_Color a, WLX_Color b) {
    ASSERT_EQ_INT(a.r, b.r);
    ASSERT_EQ_INT(a.g, b.g);
    ASSERT_EQ_INT(a.b, b.b);
    ASSERT_EQ_INT(a.a, b.a);
}

static void ap_assert_rects_equal(WLX_Rect a, WLX_Rect b) {
    ASSERT_EQ_F(a.x, b.x, 0.0f);
    ASSERT_EQ_F(a.y, b.y, 0.0f);
    ASSERT_EQ_F(a.w, b.w, 0.0f);
    ASSERT_EQ_F(a.h, b.h, 0.0f);
}

static void ap_assert_frames_equal(WLX_Context *ca, WLX_Context *cb) {
    size_t na = ca->arena.commands.count;
    size_t nb = cb->arena.commands.count;
    ASSERT_EQ_INT((long)na, (long)nb);
    WLX_Cmd *A = wlx_pool_commands(ca);
    WLX_Cmd *B = wlx_pool_commands(cb);
    for (size_t i = 0; i < na && i < nb; i++) {
        ASSERT_EQ_INT((long)A[i].type, (long)B[i].type);
        switch (A[i].type) {
        case WLX_CMD_RECT:
            ap_assert_rects_equal(A[i].data.rect.rect, B[i].data.rect.rect);
            ap_assert_colors_equal(A[i].data.rect.color, B[i].data.rect.color);
            break;
        case WLX_CMD_RECT_LINES:
            ap_assert_rects_equal(A[i].data.rect_lines.rect, B[i].data.rect_lines.rect);
            ASSERT_EQ_F(A[i].data.rect_lines.thick, B[i].data.rect_lines.thick, 0.0f);
            break;
        case WLX_CMD_RECT_ROUNDED:
            ap_assert_rects_equal(A[i].data.rect_rounded.rect, B[i].data.rect_rounded.rect);
            break;
        case WLX_CMD_RECT_ROUNDED_LINES:
            ap_assert_rects_equal(A[i].data.rect_rounded_lines.rect,
                B[i].data.rect_rounded_lines.rect);
            break;
        case WLX_CMD_LINE:
            ASSERT_EQ_F(A[i].data.line.x1, B[i].data.line.x1, 0.0f);
            ASSERT_EQ_F(A[i].data.line.y1, B[i].data.line.y1, 0.0f);
            ASSERT_EQ_F(A[i].data.line.x2, B[i].data.line.x2, 0.0f);
            ASSERT_EQ_F(A[i].data.line.y2, B[i].data.line.y2, 0.0f);
            break;
        case WLX_CMD_TEXT: {
            ASSERT_EQ_F(A[i].data.text.x, B[i].data.text.x, 0.0f);
            ASSERT_EQ_F(A[i].data.text.y, B[i].data.text.y, 0.0f);
            ASSERT_EQ_INT((long)A[i].data.text.text_len, (long)B[i].data.text.text_len);
            const char *ta = (const char *)&wlx_pool_scratch(ca)[A[i].data.text.text_off];
            const char *tb = (const char *)&wlx_pool_scratch(cb)[B[i].data.text.text_off];
            ASSERT_TRUE(memcmp(ta, tb, A[i].data.text.text_len) == 0);
            break;
        }
        case WLX_CMD_SCISSOR_BEGIN:
            ap_assert_rects_equal(A[i].data.scissor_begin.rect,
                B[i].data.scissor_begin.rect);
            break;
        default:
            break;
        }
    }
}

static void ap_assert_states_equal(WLX_Context *ca, WLX_Context *cb) {
    WLX_Editor_State *sa = ap_state(ca);
    WLX_Editor_State *sb = ap_state(cb);
    ASSERT_TRUE(sa != NULL && sb != NULL);
    ASSERT_EQ_INT((long)sa->caret.cursor_pos, (long)sb->caret.cursor_pos);
    ASSERT_EQ_INT((long)sa->caret.selection_anchor, (long)sb->caret.selection_anchor);
    ASSERT_EQ_INT((long)sa->first_line, (long)sb->first_line);
    ASSERT_EQ_INT((long)sa->first_row, (long)sb->first_row);
    ASSERT_EQ_F(sa->y_frac, sb->y_frac, 0.0f);
    ASSERT_EQ_F(sa->scroll_x, sb->scroll_x, 0.0f);
    ASSERT_EQ_F(sa->max_line_w, sb->max_line_w, 0.0f);
}

// Run one identical frame on both contexts and assert full parity.
static void ap_step(AP_Pair *p, char *buf_a, char *buf_b, size_t cap,
    size_t *len_a, size_t *len_b, bool wrap, int mx, int my, bool clicked,
    float wheel, uint32_t mods, const bool *keys_pressed, const char *text)
{
    ap_frame(&p->fallback, buf_a, cap, len_a, wrap, mx, my, clicked, wheel,
        mods, keys_pressed, text);
    ap_frame(&p->advances, buf_b, cap, len_b, wrap, mx, my, clicked, wheel,
        mods, keys_pressed, text);
    ASSERT_EQ_INT((long)*len_a, (long)*len_b);
    ASSERT_TRUE(memcmp(buf_a, buf_b, *len_a) == 0);
    ap_assert_frames_equal(&p->fallback, &p->advances);
    ap_assert_states_equal(&p->fallback, &p->advances);
}

static void ap_step_key(AP_Pair *p, char *buf_a, char *buf_b, size_t cap,
    size_t *len_a, size_t *len_b, bool wrap, WLX_Key_Code key, uint32_t mods)
{
    bool keys[WLX_KEY_COUNT] = {0};
    keys[key] = true;
    ap_step(p, buf_a, buf_b, cap, len_a, len_b, wrap, 0, 0, false, 0.0f,
        mods, keys, NULL);
}

// ============================================================================
// Pinned corpus
// ============================================================================

// Short mixed-content lines plus long lines that cross the advances chunk
// cap (a 1200-unit ASCII line, a tabbed line, a multibyte UTF-8 line), so
// chunk splices, tab-stop rounding, and the unit policy all engage.
static size_t ap_gen_corpus(char *buf, size_t cap) {
    static const char head[] =
        "plain ascii line\n"
        "\n"
        "tabs\tbetween\tsegments here\n"
        "\t\tleading and doubled\ttabs\n"
        "utf8 \xC3\xA9\xC3\xA9 mixed \xE4\xB8\xAD\xE6\x96\x87 tail\n"
        "bad \xFF\xFE bytes \x80 stray \xE2\x82 truncated\n"
        "crlf line\r\n"
        "prose  with  doubled  spaces  between  words  that  wrap  around  the  "
        "band  more  than  once  over  and  over  again  until  the  line  ends\n"
        "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
        " tail after a token that fills the band\n"
        "short zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
        "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz after an over-wide word\n"
        "            \n"
        "ends with a space \n"
        "clusters " GR_EACUTE " " GR_FLAG " " GR_FAMILY " " GR_HEART " tail\n";
    size_t off = sizeof(head) - 1;
    if (off >= cap) return 0;
    memcpy(buf, head, off);
    for (int i = 0; i < 1200 && off + 2 < cap; i++)
        buf[off++] = (char)('a' + i % 26);
    buf[off++] = '\n';
    for (int i = 0; i < 900 && off + 3 < cap; i++)
        buf[off++] = (i % 23 == 22) ? '\t' : (char)('A' + i % 26);
    buf[off++] = '\n';
    for (int i = 0; i < 400 && off + 4 < cap; i++) {
        buf[off++] = '\xC3';
        buf[off++] = (char)('\xA0' + i % 20);
    }
    buf[off++] = '\n';
    memcpy(buf + off, "ends without newline", 20);
    off += 20;
    buf[off] = '\0';
    return off;
}

// ============================================================================
// Record-stream parity (both geom paths and both measuring scans)
// ============================================================================

static void ap_assert_records_equal(const WLX_Text_Line_Record *a,
    const WLX_Text_Line_Record *b)
{
    ASSERT_EQ_INT((long)a->source_start, (long)b->source_start);
    ASSERT_EQ_INT((long)a->source_end, (long)b->source_end);
    ASSERT_EQ_INT((long)a->visible_start, (long)b->visible_start);
    ASSERT_EQ_INT((long)a->visible_end, (long)b->visible_end);
    ASSERT_EQ_INT((long)a->cursor_start, (long)b->cursor_start);
    ASSERT_EQ_INT((long)a->cursor_end, (long)b->cursor_end);
    ASSERT_EQ_INT((long)a->separator_start, (long)b->separator_start);
    ASSERT_EQ_INT((long)a->separator_end, (long)b->separator_end);
    ASSERT_EQ_F(a->measured_w, b->measured_w, 0.0f);
    ASSERT_EQ_F(a->measured_h, b->measured_h, 0.0f);
    ASSERT_EQ_F(a->advance_w, b->advance_w, 0.0f);
    ASSERT_EQ_F(a->line_h, b->line_h, 0.0f);
    ASSERT_TRUE(a->ended_by_newline == b->ended_by_newline);
    ASSERT_TRUE(a->empty_visual == b->empty_visual);
}

// Stream one hard line's records (one record unwrapped, all rows wrapped).
static size_t ap_stream_line(const WLX_Text_Build_Inputs *in_base,
    const WLX_Editor_Line_Index *idx, size_t line,
    WLX_Text_Line_Record *recs, size_t cap)
{
    WLX_Text_Build_Inputs in = *in_base;
    in.known_line_next = line + 1 < idx->count ? idx->offsets[line + 1] : in.length;
    WLX_Text_Build_Cursor cursor = { .text_unit_count = 0 };
    size_t off = idx->offsets[line];
    bool trailing = off == in.length && in.length > 0;
    size_t n = 0;
    for (;;) {
        WLX_Text_Build_Step step = wlx_text_build_step(&in, &cursor, off, trailing);
        if (!step.produced) break;
        if (n < cap) recs[n++] = step.line;
        else break;
        if (!in.wrap) break;
        if (step.line.ended_by_newline || step.next_offset <= off
            || step.next_offset >= in.length) break;
        off = step.next_offset;
        trailing = false;
    }
    return n;
}

// Compare every line's record stream across all four scan paths: the
// fallback context's measuring base is the truth; its geom replay, the
// advances context's batched measuring scan, and the advances context's
// callback-filled geom replay must all reproduce it.
static void ap_assert_streams_equal(AP_Pair *p, const char *doc_a,
    const char *doc_b, size_t len, bool wrap, float w)
{
    WLX_Editor_Line_Index *ia = ap_index(&p->fallback);
    WLX_Editor_Line_Index *ib = ap_index(&p->advances);
    ASSERT_TRUE(ia != NULL && ib != NULL);
    ASSERT_EQ_INT((long)ia->count, (long)ib->count);
    WLX_Text_Style ts = { .font = 0, .font_size = 10,
        .color = {255, 255, 255, 255}, .spacing = 0 };
    WLX_Text_Build_Inputs base_a = {
        .ctx = &p->fallback, .text = doc_a, .length = len, .style = ts,
        .rect = { 0, 0, w, 1000 }, .wrap = wrap,
        .wrap_strict_ws = wrap, // the editor's rule, so the store and this scan agree
        .line_h = ia->geom.env.line_h,
        .text_unit_cap = WLX_EDITOR_MAX_LINE_UNITS,
        .truncate_continue = true,
        .tab_advance = ia->geom.env.tab_advance,
    };
    WLX_Text_Build_Inputs base_b = base_a;
    base_b.ctx = &p->advances;
    base_b.text = doc_b;
    WLX_Text_Build_Inputs geom_a = base_a;
    geom_a.geom = &ia->geom;
    WLX_Text_Build_Inputs geom_b = base_b;
    geom_b.geom = &ib->geom;

    for (size_t line = 0; line < ia->count; line++) {
        WLX_Text_Line_Record ra[96], rb[96], ga[96], gb[96];
        size_t na = ap_stream_line(&base_a, ia, line, ra, 96);
        size_t nb = ap_stream_line(&base_b, ib, line, rb, 96);
        size_t nga = ap_stream_line(&geom_a, ia, line, ga, 96);
        size_t ngb = ap_stream_line(&geom_b, ib, line, gb, 96);
        ASSERT_EQ_INT((long)na, (long)nb);
        ASSERT_EQ_INT((long)na, (long)nga);
        ASSERT_EQ_INT((long)na, (long)ngb);
        for (size_t r = 0; r < na; r++) {
            ap_assert_records_equal(&ra[r], &rb[r]);
            ap_assert_records_equal(&ra[r], &ga[r]);
            ap_assert_records_equal(&ra[r], &gb[r]);
        }
    }
}

// ============================================================================
// Tests
// ============================================================================

TEST(advances_records_match_fallback_across_widths) {
    AP_Pair p;
    ap_pair_init(&p, 400, 120);
    static char buf_a[8192], buf_b[8192];
    size_t len_a = ap_gen_corpus(buf_a, sizeof(buf_a));
    size_t len_b = ap_gen_corpus(buf_b, sizeof(buf_b));
    ASSERT_TRUE(len_a > 0 && len_a == len_b);

    ap_step(&p, buf_a, buf_b, sizeof(buf_a), &len_a, &len_b, false,
        0, 0, false, 0.0f, 0, NULL, NULL);
    ap_step(&p, buf_a, buf_b, sizeof(buf_a), &len_a, &len_b, false,
        0, 0, false, 0.0f, 0, NULL, NULL);

    // Ascending reach extends entries in place (the advances side in
    // chunk-sized steps); the descending pass refits narrower widths from
    // the extended arrays. Every pass must reproduce the measuring build.
    float widths[] = { 12.0f, 60.0f, 200.0f, 900.0f, 8000.0f, 200.0f, 12.0f };
    for (size_t i = 0; i < sizeof(widths) / sizeof(widths[0]); i++) {
        ap_assert_streams_equal(&p, buf_a, buf_b, len_a, false, widths[i]);
    }
    ASSERT_TRUE(ap_adv_calls > 0);
    ap_pair_destroy(&p);
}

TEST(advances_wrap_rows_match_fallback) {
    AP_Pair p;
    ap_pair_init(&p, 400, 120);
    static char buf_a[8192], buf_b[8192];
    size_t len_a = ap_gen_corpus(buf_a, sizeof(buf_a));
    size_t len_b = ap_gen_corpus(buf_b, sizeof(buf_b));

    ap_step(&p, buf_a, buf_b, sizeof(buf_a), &len_a, &len_b, true,
        0, 0, false, 0.0f, 0, NULL, NULL);
    ap_step(&p, buf_a, buf_b, sizeof(buf_a), &len_a, &len_b, true,
        0, 0, false, 0.0f, 0, NULL, NULL);

    WLX_Editor_Line_Index *ia = ap_index(&p.fallback);
    ASSERT_TRUE(ia != NULL);
    ap_assert_streams_equal(&p, buf_a, buf_b, len_a, true, ia->geom.env.band_w);
    ASSERT_TRUE(ap_adv_calls > 0);
    ap_pair_destroy(&p);
}

TEST(advances_caret_x_and_hit_tests_match_fallback) {
    AP_Pair p;
    ap_pair_init(&p, 400, 120);
    static char buf_a[8192], buf_b[8192];
    size_t len_a = ap_gen_corpus(buf_a, sizeof(buf_a));
    size_t len_b = ap_gen_corpus(buf_b, sizeof(buf_b));

    ap_step(&p, buf_a, buf_b, sizeof(buf_a), &len_a, &len_b, false,
        0, 0, false, 0.0f, 0, NULL, NULL);
    ap_step(&p, buf_a, buf_b, sizeof(buf_a), &len_a, &len_b, false,
        0, 0, false, 0.0f, 0, NULL, NULL);

    WLX_Editor_Line_Index *ia = ap_index(&p.fallback);
    WLX_Editor_Line_Index *ib = ap_index(&p.advances);
    ASSERT_TRUE(ia != NULL && ib != NULL);
    WLX_Text_Style ts = { .font = 0, .font_size = 10,
        .color = {255, 255, 255, 255}, .spacing = 0 };
    float tab_advance = ia->geom.env.tab_advance;
    float line_h = ia->geom.env.line_h;

    for (size_t line = 0; line < ia->count; line++) {
        size_t start = ia->offsets[line];
        size_t next = line + 1 < ia->count ? ia->offsets[line + 1] : len_a;
        if (next <= start || next > len_a) continue;
        // Caret x at every byte offset of the line: retained-entry answers
        // (with the third form pinning the measured truth) must agree.
        // The store-less truth walk stays budget-capped from the line
        // start, while store-backed answers re-anchor past the budget and
        // keep resolving - the truth pin therefore applies within the
        // budget window; the two store paths must agree everywhere.
        for (size_t caret = start; caret <= next; caret++) {
            float xa = wlx_editor_caret_x(&p.fallback, buf_a, len_a, ts,
                tab_advance, &ia->geom, next, start, caret, true);
            float xb = wlx_editor_caret_x(&p.advances, buf_b, len_b, ts,
                tab_advance, &ib->geom, next, start, caret, true);
            ASSERT_EQ_F(xa, xb, 0.0f);
            if (caret - start <= (size_t)WLX_EDITOR_MAX_LINE_UNITS) {
                float xt = wlx_editor_caret_x(&p.fallback, buf_a, len_a, ts,
                    tab_advance, NULL, next, start, caret, true);
                ASSERT_EQ_F(xt, xb, 0.0f);
            }
        }
        // Hit tests across the line's width (plus slack on both sides).
        for (float x = -7.0f; x < 6200.0f; x += 37.0f) {
            size_t oa = wlx_editor_offset_at_x(&p.fallback, buf_a, len_a, ts,
                line_h, tab_advance, ia, &ia->geom, line, 8000.0f, 0.0f, x);
            size_t ob = wlx_editor_offset_at_x(&p.advances, buf_b, len_b, ts,
                line_h, tab_advance, ib, &ib->geom, line, 8000.0f, 0.0f, x);
            ASSERT_EQ_INT((long)oa, (long)ob);
        }
    }
    ap_pair_destroy(&p);
}

TEST(advances_scenario_no_wrap_draws_identically) {
    AP_Pair p;
    ap_pair_init(&p, 400, 120);
    static char buf_a[8192], buf_b[8192];
    size_t len_a = ap_gen_corpus(buf_a, sizeof(buf_a));
    size_t len_b = ap_gen_corpus(buf_b, sizeof(buf_b));
    size_t cap = sizeof(buf_a);

    // Cold + idle.
    for (int i = 0; i < 3; i++)
        ap_step(&p, buf_a, buf_b, cap, &len_a, &len_b, false, 0, 0, false, 0.0f, 0, NULL, NULL);
    // Vertical wheel.
    for (int i = 0; i < 4; i++)
        ap_step(&p, buf_a, buf_b, cap, &len_a, &len_b, false, 30, 30, false, -1.0f, 0, NULL, NULL);
    // Horizontal sweep deep into the long lines (chunk splices engage).
    for (int i = 0; i < 12; i++)
        ap_step(&p, buf_a, buf_b, cap, &len_a, &len_b, false, 30, 30, false, -3.0f, WLX_MOD_SHIFT, NULL, NULL);
    // Focus, type ASCII + multibyte + tab, then END and a shift selection.
    ap_step(&p, buf_a, buf_b, cap, &len_a, &len_b, false, 60, 30, true, 0.0f, 0, NULL, NULL);
    ap_step(&p, buf_a, buf_b, cap, &len_a, &len_b, false, 0, 0, false, 0.0f, 0, NULL, "x");
    ap_step(&p, buf_a, buf_b, cap, &len_a, &len_b, false, 0, 0, false, 0.0f, 0, NULL, "\xC3\xA9");
    ap_step_key(&p, buf_a, buf_b, cap, &len_a, &len_b, false, WLX_KEY_TAB, 0);
    ap_step_key(&p, buf_a, buf_b, cap, &len_a, &len_b, false, WLX_KEY_END, 0);
    ap_step_key(&p, buf_a, buf_b, cap, &len_a, &len_b, false, WLX_KEY_LEFT, WLX_MOD_SHIFT);
    ap_step_key(&p, buf_a, buf_b, cap, &len_a, &len_b, false, WLX_KEY_LEFT, WLX_MOD_SHIFT);
    ap_step_key(&p, buf_a, buf_b, cap, &len_a, &len_b, false, WLX_KEY_HOME, WLX_MOD_SHIFT);
    // Selection span drawn from stored advances on both sides.
    for (int i = 0; i < 2; i++)
        ap_step(&p, buf_a, buf_b, cap, &len_a, &len_b, false, 0, 0, false, 0.0f, 0, NULL, NULL);
    ap_pair_destroy(&p);
}

TEST(advances_scenario_wrap_draws_identically) {
    AP_Pair p;
    ap_pair_init(&p, 400, 120);
    static char buf_a[8192], buf_b[8192];
    size_t len_a = ap_gen_corpus(buf_a, sizeof(buf_a));
    size_t len_b = ap_gen_corpus(buf_b, sizeof(buf_b));
    size_t cap = sizeof(buf_a);

    for (int i = 0; i < 3; i++)
        ap_step(&p, buf_a, buf_b, cap, &len_a, &len_b, true, 0, 0, false, 0.0f, 0, NULL, NULL);
    for (int i = 0; i < 6; i++)
        ap_step(&p, buf_a, buf_b, cap, &len_a, &len_b, true, 30, 30, false, -1.0f, 0, NULL, NULL);
    ap_step(&p, buf_a, buf_b, cap, &len_a, &len_b, true, 60, 40, true, 0.0f, 0, NULL, NULL);
    ap_step(&p, buf_a, buf_b, cap, &len_a, &len_b, true, 0, 0, false, 0.0f, 0, NULL, "yy");
    ap_step_key(&p, buf_a, buf_b, cap, &len_a, &len_b, true, WLX_KEY_ENTER, 0);
    ap_step_key(&p, buf_a, buf_b, cap, &len_a, &len_b, true, WLX_KEY_BACKSPACE, 0);
    ap_step_key(&p, buf_a, buf_b, cap, &len_a, &len_b, true, WLX_KEY_END, WLX_MOD_SHIFT);
    for (int i = 0; i < 2; i++)
        ap_step(&p, buf_a, buf_b, cap, &len_a, &len_b, true, 0, 0, false, 0.0f, 0, NULL, NULL);
    ap_pair_destroy(&p);
}

TEST(advances_cold_build_batches_measure_traffic) {
    AP_Pair p;
    ap_pair_init(&p, 400, 120);
    static char buf_a[8192], buf_b[8192];
    size_t len_a = ap_gen_corpus(buf_a, sizeof(buf_a));
    size_t len_b = ap_gen_corpus(buf_b, sizeof(buf_b));

    ap_slice_calls = 0;
    ap_frame(&p.fallback, buf_a, sizeof(buf_a), &len_a, false, 0, 0, false, 0.0f, 0, NULL, NULL);
    unsigned long long fallback_cold = ap_slice_calls;

    ap_slice_calls = 0;
    ap_adv_calls = 0;
    ap_frame(&p.advances, buf_b, sizeof(buf_b), &len_b, false, 0, 0, false, 0.0f, 0, NULL, NULL);
    unsigned long long advances_cold = ap_slice_calls + ap_adv_calls;

    // The batched cold build must collapse per-unit prefix traffic by an
    // order of magnitude, not merely dent it.
    ASSERT_TRUE(ap_adv_calls > 0);
    ASSERT_TRUE(advances_cold * 10 < fallback_cold);
    ap_pair_destroy(&p);
}

// A line of 300 ZWJ families (5400 bytes, 300 units; corpus macro from
// test_grapheme.c, same TU) crosses the advances chunk cap: every stored
// unit end on both paths is a family boundary, so no chunk splice falls
// inside a family, and the paths keep answering identical caret x and hit
// tests across it.
TEST(advances_cluster_units_never_split_at_chunk) {
    AP_Pair p;
    ap_pair_init(&p, 400, 120);
    static char buf_a[8192], buf_b[8192];
    size_t len = 0;
    memcpy(buf_a, "head\n", 5);
    len = 5;
    for (int i = 0; i < 300; i++) { memcpy(buf_a + len, GR_FAMILY, 18); len += 18; }
    memcpy(buf_a + len, "\ntail", 5);
    len += 5;
    memcpy(buf_b, buf_a, len);
    size_t len_a = len, len_b = len;

    // Wrapped: the store builds the whole line's rows, so the entry holds
    // all 300 units across two advances chunks; four families (360 px)
    // fit the 383 px band per row.
    ap_step(&p, buf_a, buf_b, sizeof(buf_a), &len_a, &len_b, true,
        0, 0, false, 0.0f, 0, NULL, NULL);
    ap_step(&p, buf_a, buf_b, sizeof(buf_a), &len_a, &len_b, true,
        0, 0, false, 0.0f, 0, NULL, NULL);

    WLX_Editor_Line_Index *ia = ap_index(&p.fallback);
    WLX_Editor_Line_Index *ib = ap_index(&p.advances);
    ASSERT_TRUE(ia != NULL && ib != NULL);
    ASSERT_EQ_INT(3, (long)ib->count);
    size_t start = ib->offsets[1];
    size_t next = ib->offsets[2];
    ASSERT_EQ_INT(5, (long)start);
    ASSERT_EQ_INT(5 + 5400 + 1, (long)next);

    for (int which = 0; which < 2; which++) {
        WLX_Editor_Line_Index *idx = which == 0 ? ia : ib;
        WLX_Text_Geom_Entry *e = NULL;
        for (size_t i = 0; i < idx->geom.count; i++) {
            if (idx->geom.entries[i].used && idx->geom.entries[i].line_start == start)
                e = &idx->geom.entries[i];
        }
        ASSERT_TRUE(e != NULL);
        ASSERT_EQ_INT(300, (long)e->units);
        ASSERT_TRUE(e->units > (size_t)WLX_TEXT_ADVANCES_CHUNK);
        for (size_t u = 0; u < e->units; u++)
            ASSERT_EQ_INT(0, (long)(e->unit_ends[u] % 18));
        ASSERT_EQ_INT(75, (long)e->rows);
        for (size_t r = 0; r + 1 < e->rows; r++)
            ASSERT_EQ_INT(4, (long)(e->row_units[r + 1] - e->row_units[r]));
    }

    // Unwrapped: caret x and hit tests agree between the paths across the
    // line and answer boundaries only.
    ap_step(&p, buf_a, buf_b, sizeof(buf_a), &len_a, &len_b, false,
        0, 0, false, 0.0f, 0, NULL, NULL);
    ap_step(&p, buf_a, buf_b, sizeof(buf_a), &len_a, &len_b, false,
        0, 0, false, 0.0f, 0, NULL, NULL);
    ia = ap_index(&p.fallback);
    ib = ap_index(&p.advances);
    ASSERT_TRUE(ia != NULL && ib != NULL);
    WLX_Text_Style ts = { .font = 0, .font_size = 10,
        .color = {255, 255, 255, 255}, .spacing = 0 };
    float tab_advance = ia->geom.env.tab_advance;
    float line_h = ia->geom.env.line_h;
    for (size_t caret = start; caret <= next; caret += 6) {
        float xa = wlx_editor_caret_x(&p.fallback, buf_a, len_a, ts,
            tab_advance, &ia->geom, next, start, caret, true);
        float xb = wlx_editor_caret_x(&p.advances, buf_b, len_b, ts,
            tab_advance, &ib->geom, next, start, caret, true);
        ASSERT_EQ_F(xa, xb, 0.0f);
    }
    for (float x = -7.0f; x < 27500.0f; x += 97.0f) {
        size_t oa = wlx_editor_offset_at_x(&p.fallback, buf_a, len_a, ts,
            line_h, tab_advance, ia, &ia->geom, 1, 30000.0f, 0.0f, x);
        size_t ob = wlx_editor_offset_at_x(&p.advances, buf_b, len_b, ts,
            line_h, tab_advance, ib, &ib->geom, 1, 30000.0f, 0.0f, x);
        ASSERT_EQ_INT((long)oa, (long)ob);
        ASSERT_TRUE(wlx_text_unit_boundary(buf_a, len_a, oa));
    }
    ap_pair_destroy(&p);
}

SUITE(advances_parity) {
    RUN_TEST(advances_cluster_units_never_split_at_chunk);
    RUN_TEST(advances_records_match_fallback_across_widths);
    RUN_TEST(advances_wrap_rows_match_fallback);
    RUN_TEST(advances_caret_x_and_hit_tests_match_fallback);
    RUN_TEST(advances_scenario_no_wrap_draws_identically);
    RUN_TEST(advances_scenario_wrap_draws_identically);
    RUN_TEST(advances_cold_build_batches_measure_traffic);
}
