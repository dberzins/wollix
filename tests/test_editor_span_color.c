// test_editor_span_color.c - per-span colour on wlx_editor: the record
// piece walk with no callback draws exactly what the tab walk drew
// (identity pins on tabbed, wrapped and windowed-origin corpora), a
// callback's spans become pieces at the stored advances, the core clips
// and snaps every span end to a text unit boundary and guarantees
// progress, zero colour is front_color, span colours take the disabled
// shift and opacity, the callback runs only for visible records, and
// geometry and measure traffic are identical with the hook on and off.
//
// Geometry model (mock backend): char width = font_size/2 = 5 px at
// font_size 10, line height 10, tab advance 20 (tab_columns 4 x the space
// advance). Fixture: 400x100 context, content_padding 4, border 0 -> band
// {9,4,383,92} before strips, so a wrapped row holds 76 units. Reuses the
// ev_* (view), ew_* (wrap) and wo_* (windowed origin) fixtures, so it
// follows test_editor_windowed_origin.c.

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
// Fixture: piece capture through draw_text_slice (exact bytes and colour)
// ============================================================================

enum { SC_BAND_X_ = 9, SC_BAND_Y_ = 4, SC_LINE_H_ = 10 };
#define SC_UNIT_W_ 5.0f
#define SC_TAB_W_ 20.0f

#define SC_MAX_PIECES_ 128
static struct { char text[96]; size_t len; float x; float y; WLX_Color color; } sc_pieces[SC_MAX_PIECES_];
static int sc_piece_count;

static void sc_capture_draw_text_slice(const char *text, size_t len, float x, float y,
    WLX_Text_Style style, void *user)
{
    (void)user;
    if (sc_piece_count >= SC_MAX_PIECES_) return;
    size_t keep = len < sizeof(sc_pieces[0].text) - 1 ? len : sizeof(sc_pieces[0].text) - 1;
    if (keep > 0) memcpy(sc_pieces[sc_piece_count].text, text, keep);
    sc_pieces[sc_piece_count].text[keep] = '\0';
    sc_pieces[sc_piece_count].len = len;
    sc_pieces[sc_piece_count].x = x;
    sc_pieces[sc_piece_count].y = y;
    sc_pieces[sc_piece_count].color = style.color;
    sc_piece_count++;
}

static void sc_reset_pieces(void) {
    memset(sc_pieces, 0, sizeof(sc_pieces));
    sc_piece_count = 0;
}

static size_t sc_fill(char *buf, size_t cap, const char *text) {
    size_t n = strlen(text);
    if (n >= cap) n = cap - 1;
    memcpy(buf, text, n);
    buf[n] = '\0';
    return n;
}

// The piece at index i is exactly `text` at (x, y).
#define SC_EXPECT_PIECE(i, want_text, want_x, want_y) do {                    \
    ASSERT_TRUE((i) < sc_piece_count);                                         \
    ASSERT_EQ_STR(sc_pieces[(i)].text, (want_text));                           \
    ASSERT_EQ_F(sc_pieces[(i)].x, (want_x), 0.01f);                            \
    ASSERT_EQ_F(sc_pieces[(i)].y, (want_y), 0.01f);                            \
} while (0)

// ============================================================================
// Identity: no callback draws exactly the tab walk's pieces
// ============================================================================

// Tabbed no-wrap document on a steady frame: a plain line is one piece; a
// tabbed line is its tab-free segments at the next-tab-stop x from the
// stored advances (the tab at 10 px stops at 20, at 0 stops at 20, two
// tabs after 5 px stop at 40). Tiers 2 and 3 agree on the additive mock;
// the tier-3 branch is pinned directly on the drawer below.
TEST(span_color_null_identity_tabs) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_text_slice = sc_capture_draw_text_slice;

    char buf[64];
    size_t len = sc_fill(buf, sizeof(buf), "ab\tcd\n\tx\na\t\tb\nplain\n");
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    sc_reset_pieces();
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    SC_EXPECT_PIECE(0, "ab", 9.0f, 4.0f);
    SC_EXPECT_PIECE(1, "cd", 9.0f + 20.0f, 4.0f);
    SC_EXPECT_PIECE(2, "x", 9.0f + 20.0f, 14.0f);
    SC_EXPECT_PIECE(3, "a", 9.0f, 24.0f);
    SC_EXPECT_PIECE(4, "b", 9.0f + 40.0f, 24.0f);
    SC_EXPECT_PIECE(5, "plain", 9.0f, 34.0f);
    ASSERT_EQ_INT(6, sc_piece_count);
    wlx_context_destroy(&ctx);
}

// Wrapped document with a tab inside a continuation row: the first row is
// the 76 units the band holds (a word wider than the row breaks inside
// it), the second row's tab stops relative to its own start.
TEST(span_color_null_identity_wrapped) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_text_slice = sc_capture_draw_text_slice;

    char buf[128];
    size_t off = 0;
    for (int i = 0; i < 80; i++) buf[off++] = 'a';
    buf[off++] = '\t';
    buf[off++] = 'z';
    buf[off++] = '\n';
    buf[off] = '\0';
    size_t len = off;
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    sc_reset_pieces();
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);

    char row0[80];
    memset(row0, 'a', 76);
    row0[76] = '\0';
    SC_EXPECT_PIECE(0, row0, 9.0f, 4.0f);
    SC_EXPECT_PIECE(1, "aaaa", 9.0f, 14.0f);
    SC_EXPECT_PIECE(2, "z", 9.0f + 40.0f, 14.0f);
    ASSERT_EQ_INT(3, sc_piece_count);
    wlx_context_destroy(&ctx);
}

// A giant tabbed line viewed far past the unit budget: the record enters
// at the windowed origin, its pieces tile the tab-free segments from the
// origin in order, and every piece sits at the caret geometry's x for its
// first byte (band x minus the scroll plus the content x), the seam
// agreement the origin contract promises.
TEST(span_color_null_identity_windowed_origin) {
    WLX_Context ctx;
    wo_ctx_init(&ctx, false);
    ctx.backend.draw_text_slice = sc_capture_draw_text_slice;

    static char buf[8200];
    size_t len = 0;
    while (len + 9 < 8000) {
        memcpy(buf + len, "abcdefgh\t", 9);
        len += 9;
    }
    buf[len] = '\0';

    wo_frame(&ctx, buf, sizeof(buf), &len);
    wo_frame(&ctx, buf, sizeof(buf), &len);
    WLX_Editor_State *st = wo_state(&ctx);
    ASSERT_TRUE(st != NULL);
    st->scroll_x = 20000.0f;
    wo_frame(&ctx, buf, sizeof(buf), &len);
    sc_reset_pieces();
    wo_frame(&ctx, buf, sizeof(buf), &len);

    WLX_Text_Geom_Entry *e = wo_entry(&ctx, 0);
    ASSERT_TRUE(e != NULL);
    ASSERT_TRUE(e->origin_rel > 0);
    ASSERT_TRUE(sc_piece_count >= 10);

    // The record is cut at the measure lookahead, so its last piece is a
    // prefix of its segment; every other piece is the whole segment.
    size_t pos = (size_t)e->origin_rel;
    for (int i = 0; i < sc_piece_count; i++) {
        size_t seg_end = pos;
        while (seg_end < len && buf[seg_end] != '\t') seg_end++;
        if (i + 1 < sc_piece_count) {
            ASSERT_EQ_INT((long)(seg_end - pos), (long)sc_pieces[i].len);
        } else {
            ASSERT_TRUE(sc_pieces[i].len > 0 && sc_pieces[i].len <= seg_end - pos);
        }
        ASSERT_TRUE(memcmp(buf + pos, sc_pieces[i].text, sc_pieces[i].len) == 0);
        float want_x = (float)SC_BAND_X_ - st->scroll_x + wo_caret_x(&ctx, buf, len, pos);
        ASSERT_EQ_F(sc_pieces[i].x, want_x, 0.05f);
        ASSERT_EQ_F(sc_pieces[i].y, 4.0f, 0.01f);
        pos = seg_end + 1;
    }
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Fixture: the hook, a query recorder, and an editor frame with options
// ============================================================================

// Every query the callback receives, in order, with the end it answered.
#define SC_MAX_Q_ 256
static struct { size_t offset, limit, line, line_start, line_next, end; } sc_q[SC_MAX_Q_];
static int sc_q_count;

static void sc_q_reset(void) {
    memset(sc_q, 0, sizeof(sc_q));
    sc_q_count = 0;
}

static bool sc_is_ws(char c) { return c == ' ' || c == '\t'; }

enum {
    SC_MODE_WORDS,        // words alternate a / b, whitespace zero
    SC_MODE_EDGES,        // colour a up to the next listed edge, else the limit
    SC_MODE_STALL,        // end = offset (no progress)
    SC_MODE_BACK,         // end before the offset
    SC_MODE_PAST_RECORD,  // end one past the record (legitimate, clipped)
    SC_MODE_PAST_DOC,     // end past the document (a bug, asserted, clipped)
    SC_MODE_ZERO,         // zero colour to the limit
    SC_MODE_CONST         // colour a to the limit
};

typedef struct {
    int mode;
    const size_t *edges;
    int edge_count;
    WLX_Color a;
    WLX_Color b;
    int words;
} SC_Cb;

static WLX_Color sc_cb(const WLX_Text_Span_Query *q, size_t *end, void *user) {
    SC_Cb *c = (SC_Cb *)user;
    WLX_Color zero = {0};
    WLX_Color col = c->a;
    switch (c->mode) {
    case SC_MODE_WORDS: {
        bool ws = sc_is_ws(q->text[q->offset]);
        size_t p = q->offset;
        while (p < q->limit && sc_is_ws(q->text[p]) == ws) p++;
        *end = p;
        col = ws ? zero : ((c->words++ % 2 == 0) ? c->a : c->b);
    } break;
    case SC_MODE_EDGES:
        *end = q->limit;
        for (int i = 0; i < c->edge_count; i++) {
            if (c->edges[i] > q->offset) { *end = c->edges[i]; break; }
        }
        break;
    case SC_MODE_STALL: *end = q->offset; break;
    case SC_MODE_BACK: *end = q->offset > 0 ? q->offset - 1 : 0; break;
    case SC_MODE_PAST_RECORD: *end = q->limit + 1; break;
    case SC_MODE_PAST_DOC: *end = q->length + 100; break;
    case SC_MODE_ZERO: col = zero; break;
    case SC_MODE_CONST: break;
    }
    if (sc_q_count < SC_MAX_Q_) {
        sc_q[sc_q_count].offset = q->offset;
        sc_q[sc_q_count].limit = q->limit;
        sc_q[sc_q_count].line = q->line;
        sc_q[sc_q_count].line_start = q->line_start;
        sc_q[sc_q_count].line_next = q->line_next;
        sc_q[sc_q_count].end = *end;
        sc_q_count++;
    }
    return col;
}

static const WLX_Color SC_RED_ = { 255, 0, 0, 255 };
static const WLX_Color SC_GREEN_ = { 0, 255, 0, 255 };
static const WLX_Color SC_BLUE_ = { 10, 20, 30, 255 };

// Editor frame on the ev_* geometry with the hook and the visual-state
// options a test needs; the same id every frame so the state persists.
typedef struct {
    WLX_Text_Span_Color_Fn fn;
    void *user;
    bool wrap;
    bool disabled;
    float opacity;     // < 0 = leave the default
    WLX_Color front;   // zero = leave the default
} SC_Opts;

static void sc_frame_full(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
    const SC_Opts *o, int mx, int my, bool down, bool clicked, float wheel,
    const bool *keys_pressed, const char *text)
{
    sc_q_reset();
    wo_calls = 0;
    test_frame_begin_full(ctx, mx, my, down, clicked, down, wheel, NULL, keys_pressed, NULL, 0, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    WLX_Editor_Opt opt = wlx_default_editor_opt(.content_padding = 4, .font_size = 10,
        .border_width = 0, .wrap = o->wrap, .disabled = o->disabled,
        .span_color = o->fn, .span_color_user = o->user);
    if (o->opacity >= 0.0f) opt.opacity = o->opacity;
    if (!wlx_color_is_zero(o->front)) opt.front_color = o->front;
    (void)wlx_editor_impl(ctx, NULL, buf, cap, len, opt, "sc_editor", 1);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void sc_frame(WLX_Context *ctx, char *buf, size_t cap, size_t *len, const SC_Opts *o) {
    sc_frame_full(ctx, buf, cap, len, o, 0, 0, false, false, 0.0f, NULL, NULL);
}

static SC_Opts sc_opts(WLX_Text_Span_Color_Fn fn, void *user) {
    SC_Opts o;
    memset(&o, 0, sizeof(o));
    o.fn = fn;
    o.user = user;
    o.opacity = -1.0f;
    return o;
}

// Byte-linear x of a piece starting at doc byte `b` on an unscrolled row.
static float sc_x(size_t b) { return (float)SC_BAND_X_ + SC_UNIT_W_ * (float)b; }

// ============================================================================
// Spans become pieces at the stored advances
// ============================================================================

// Every word a span on a steady frame: pieces at the byte-linear x of
// their first byte, colours in query order, whitespace in the base colour.
TEST(span_color_every_word_pieces_at_stored_advances) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_text_slice = sc_capture_draw_text_slice;
    char buf[64];
    size_t len = sc_fill(buf, sizeof(buf), "hello world foo\nbar baz\n");

    SC_Opts off = sc_opts(NULL, NULL);
    sc_frame(&ctx, buf, sizeof(buf), &len, &off);
    sc_reset_pieces();
    sc_frame(&ctx, buf, sizeof(buf), &len, &off);
    ASSERT_EQ_INT(2, sc_piece_count);
    WLX_Color front = sc_pieces[0].color;

    SC_Cb cb = { .mode = SC_MODE_WORDS, .a = SC_RED_, .b = SC_GREEN_ };
    SC_Opts on = sc_opts(sc_cb, &cb);
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);
    cb.words = 0;
    sc_reset_pieces();
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);

    SC_EXPECT_PIECE(0, "hello", sc_x(0), 4.0f);
    ASSERT_EQ_COLOR(sc_pieces[0].color, SC_RED_);
    SC_EXPECT_PIECE(1, " ", sc_x(5), 4.0f);
    ASSERT_EQ_COLOR(sc_pieces[1].color, front);
    SC_EXPECT_PIECE(2, "world", sc_x(6), 4.0f);
    ASSERT_EQ_COLOR(sc_pieces[2].color, SC_GREEN_);
    SC_EXPECT_PIECE(3, " ", sc_x(11), 4.0f);
    SC_EXPECT_PIECE(4, "foo", sc_x(12), 4.0f);
    ASSERT_EQ_COLOR(sc_pieces[4].color, SC_RED_);
    SC_EXPECT_PIECE(5, "bar", 9.0f, 14.0f);
    ASSERT_EQ_COLOR(sc_pieces[5].color, SC_GREEN_);
    SC_EXPECT_PIECE(6, " ", 9.0f + 15.0f, 14.0f);
    SC_EXPECT_PIECE(7, "baz", 9.0f + 20.0f, 14.0f);
    ASSERT_EQ_COLOR(sc_pieces[7].color, SC_RED_);
    ASSERT_EQ_INT(8, sc_piece_count);
    ASSERT_EQ_INT(8, sc_q_count);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// The core enforces every answer
// ============================================================================

// An edge inside e + U+0301, between a flag's indicators, after a ZWJ and
// inside a codepoint snaps forward to the cluster's end: each cluster is
// one piece in one colour and the pieces tile the record's bytes.
TEST(span_color_edge_inside_cluster_snaps_forward) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_text_slice = sc_capture_draw_text_slice;
    char buf[64];
    size_t len = sc_fill(buf, sizeof(buf),
        "e\xCC\x81" "x" "\xF0\x9F\x87\xA9\xF0\x9F\x87\xAA" "x"
        "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9" "x\n");
    ASSERT_EQ_INT(26, (long)len);

    static const size_t edges1[] = { 1, 4, 8, 13, 20 };
    SC_Cb cb = { .mode = SC_MODE_EDGES, .edges = edges1, .edge_count = 5, .a = SC_RED_ };
    SC_Opts on = sc_opts(sc_cb, &cb);
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);
    sc_reset_pieces();
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);

    SC_EXPECT_PIECE(0, "e\xCC\x81", sc_x(0), 4.0f);
    SC_EXPECT_PIECE(1, "x", sc_x(3), 4.0f);
    SC_EXPECT_PIECE(2, "\xF0\x9F\x87\xA9\xF0\x9F\x87\xAA", sc_x(4), 4.0f);
    SC_EXPECT_PIECE(3, "x", sc_x(12), 4.0f);
    SC_EXPECT_PIECE(4, "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9", sc_x(13), 4.0f);
    SC_EXPECT_PIECE(5, "x", sc_x(24), 4.0f);
    ASSERT_EQ_INT(6, sc_piece_count);
    for (int i = 0; i < sc_piece_count; i++) ASSERT_EQ_COLOR(sc_pieces[i].color, SC_RED_);
    size_t tiled = 0;
    for (int i = 0; i < sc_piece_count; i++) tiled += sc_pieces[i].len;
    ASSERT_EQ_INT(25, (long)tiled);
    ASSERT_EQ_INT(0, test_span_assert_hits);

    // An edge inside a codepoint of the family lands on the family's end.
    static const size_t edges2[] = { 15 };
    cb.edges = edges2;
    cb.edge_count = 1;
    sc_reset_pieces();
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);
    ASSERT_EQ_INT(2, sc_piece_count);
    ASSERT_EQ_INT(24, (long)sc_pieces[0].len);
    ASSERT_EQ_F(sc_pieces[0].x, sc_x(0), 0.01f);
    SC_EXPECT_PIECE(1, "x", sc_x(24), 4.0f);
    wlx_context_destroy(&ctx);
}

// An end at or before the offset advances one unit per call, the walk
// terminates, and the contract check fires once per stalled answer.
TEST(span_color_end_at_or_before_offset_progresses_one_unit) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_text_slice = sc_capture_draw_text_slice;
    char buf[32];
    size_t len = sc_fill(buf, sizeof(buf), "ab\xCC\x81" "c\n");

    for (int mode = SC_MODE_STALL; mode <= SC_MODE_BACK; mode++) {
        SC_Cb cb = { .mode = mode, .a = SC_RED_ };
        SC_Opts on = sc_opts(sc_cb, &cb);
        sc_frame(&ctx, buf, sizeof(buf), &len, &on);
        test_span_assert_hits = 0;
        sc_reset_pieces();
        sc_frame(&ctx, buf, sizeof(buf), &len, &on);
        SC_EXPECT_PIECE(0, "a", sc_x(0), 4.0f);
        SC_EXPECT_PIECE(1, "b\xCC\x81", sc_x(1), 4.0f);
        SC_EXPECT_PIECE(2, "c", sc_x(4), 4.0f);
        ASSERT_EQ_INT(3, sc_piece_count);
        ASSERT_EQ_INT(3, sc_q_count);
        ASSERT_EQ_INT(3, test_span_assert_hits);
    }
    test_span_assert_hits = 0;
    wlx_context_destroy(&ctx);
}

// An end past the record is legitimate and clipped to it; an end past
// the document is clipped too and reported by the contract check. Either
// way the next record's first query starts at its own start.
TEST(span_color_end_clipped_to_record_and_document) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_text_slice = sc_capture_draw_text_slice;
    char buf[32];
    size_t len = sc_fill(buf, sizeof(buf), "abc\ndef\n");

    for (int mode = SC_MODE_PAST_RECORD; mode <= SC_MODE_PAST_DOC; mode++) {
        SC_Cb cb = { .mode = mode, .a = SC_RED_ };
        SC_Opts on = sc_opts(sc_cb, &cb);
        sc_frame(&ctx, buf, sizeof(buf), &len, &on);
        test_span_assert_hits = 0;
        sc_reset_pieces();
        sc_frame(&ctx, buf, sizeof(buf), &len, &on);
        SC_EXPECT_PIECE(0, "abc", sc_x(0), 4.0f);
        SC_EXPECT_PIECE(1, "def", 9.0f, 14.0f);
        ASSERT_EQ_INT(2, sc_piece_count);
        ASSERT_EQ_INT(2, sc_q_count);
        ASSERT_EQ_INT(0, (long)sc_q[0].offset);
        ASSERT_EQ_INT(3, (long)sc_q[0].limit);
        ASSERT_EQ_INT(4, (long)sc_q[1].offset);
        ASSERT_EQ_INT(7, (long)sc_q[1].limit);
        ASSERT_EQ_INT(mode == SC_MODE_PAST_DOC ? 2 : 0, test_span_assert_hits);
    }
    test_span_assert_hits = 0;
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Tabs and spans are one boundary set
// ============================================================================

// A span ending at a tab byte, a span holding two tabs, and a span after
// a tab: x after each tab is the next stop, whether the pieces come from
// the stored advances (the editor's steady frame) or from measures (the
// drawer called with no entry).
TEST(span_color_edge_on_tab_and_tab_inside_span) {
    static const size_t edges[] = { 2, 6 };
    char buf[32];
    size_t len = sc_fill(buf, sizeof(buf), "ab\tcd\tef\n");

    // Stored advances.
    {
        WLX_Context ctx;
        test_ctx_init(&ctx, 400, 100);
        ctx.backend.draw_text_slice = sc_capture_draw_text_slice;
        SC_Cb cb = { .mode = SC_MODE_EDGES, .edges = edges, .edge_count = 2, .a = SC_RED_ };
        SC_Opts on = sc_opts(sc_cb, &cb);
        sc_frame(&ctx, buf, sizeof(buf), &len, &on);
        sc_reset_pieces();
        sc_frame(&ctx, buf, sizeof(buf), &len, &on);
        SC_EXPECT_PIECE(0, "ab", 9.0f, 4.0f);
        SC_EXPECT_PIECE(1, "cd", 9.0f + 20.0f, 4.0f);
        SC_EXPECT_PIECE(2, "ef", 9.0f + 40.0f, 4.0f);
        ASSERT_EQ_INT(3, sc_piece_count);
        ASSERT_EQ_INT(3, sc_q_count);
        ASSERT_EQ_INT(2, (long)sc_q[1].offset);
        ASSERT_EQ_INT(6, (long)sc_q[2].offset);
        wlx_context_destroy(&ctx);
    }

    // Measured: the drawer on a hand-built record with no entry.
    {
        WLX_Context ctx;
        test_ctx_init(&ctx, 400, 100);
        ctx.backend.draw_text_slice = sc_capture_draw_text_slice;
        SC_Cb cb = { .mode = SC_MODE_EDGES, .edges = edges, .edge_count = 2, .a = SC_RED_ };
        sc_reset_pieces();
        sc_q_reset();
        test_frame_begin(&ctx, 0, 0, false, false);
        WLX_Text_Line_Record rec;
        memset(&rec, 0, sizeof(rec));
        rec.visible_start = 0;
        rec.visible_end = 8;
        rec.origin_x = 9.0f;
        rec.origin_y = 4.0f;
        rec.line_h = 10.0f;
        WLX_Text_Span_Draw_Args args = {
            .ctx = &ctx, .text = buf, .len = len, .line = &rec,
            .ts = { .font_size = 10, .color = { 255, 255, 255, 255 } },
            .tab_advance = 20.0f, .entry = NULL,
            .line_index = 0, .line_start = 0, .line_next = len,
            .span_color = sc_cb, .span_user = &cb,
            .theme = ctx.theme, .disabled = false, .opacity = 1.0f,
        };
        wlx_text_draw_record_spans(&args);
        test_frame_end(&ctx);
        SC_EXPECT_PIECE(0, "ab", 9.0f, 4.0f);
        SC_EXPECT_PIECE(1, "cd", 9.0f + 20.0f, 4.0f);
        SC_EXPECT_PIECE(2, "ef", 9.0f + 40.0f, 4.0f);
        ASSERT_EQ_INT(3, sc_piece_count);
        wlx_context_destroy(&ctx);
    }
}

// ============================================================================
// Rows and windows query from their own start
// ============================================================================

// A span crossing a wrapped row boundary is asked again on the next row,
// whose query starts where the previous row's limit was, and its colour
// continues.
TEST(span_color_span_crosses_wrapped_row) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_text_slice = sc_capture_draw_text_slice;
    char buf[128];
    memset(buf, 'a', 80);
    buf[80] = '\n';
    buf[81] = '\0';
    size_t len = 81;

    SC_Cb cb = { .mode = SC_MODE_CONST, .a = SC_RED_ };
    SC_Opts on = sc_opts(sc_cb, &cb);
    on.wrap = true;
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);
    sc_reset_pieces();
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);

    ASSERT_EQ_INT(2, sc_q_count);
    ASSERT_EQ_INT(0, (long)sc_q[0].offset);
    ASSERT_EQ_INT(76, (long)sc_q[0].limit);
    ASSERT_EQ_INT(76, (long)sc_q[1].offset);
    ASSERT_EQ_INT(80, (long)sc_q[1].limit);
    ASSERT_EQ_INT(0, (long)sc_q[1].line);
    ASSERT_EQ_INT(0, (long)sc_q[1].line_start);
    ASSERT_EQ_INT(81, (long)sc_q[1].line_next);
    ASSERT_EQ_INT(2, sc_piece_count);
    ASSERT_EQ_INT(76, (long)sc_pieces[0].len);
    ASSERT_EQ_F(sc_pieces[0].x, 9.0f, 0.01f);
    ASSERT_EQ_F(sc_pieces[0].y, 4.0f, 0.01f);
    SC_EXPECT_PIECE(1, "aaaa", 9.0f, 14.0f);
    ASSERT_EQ_COLOR(sc_pieces[0].color, SC_RED_);
    ASSERT_EQ_COLOR(sc_pieces[1].color, SC_RED_);
    wlx_context_destroy(&ctx);
}

// A giant line viewed past the unit budget: the record's first query
// starts at the windowed origin with the whole line as its hard line,
// and its piece sits at x 0 in the record frame.
TEST(span_color_span_crosses_windowed_origin) {
    WLX_Context ctx;
    wo_ctx_init(&ctx, false);
    ctx.backend.draw_text_slice = sc_capture_draw_text_slice;
    static char buf[8200];
    size_t len = wo_fill_giant(buf, sizeof(buf));

    SC_Cb cb = { .mode = SC_MODE_CONST, .a = SC_RED_ };
    SC_Opts on = sc_opts(sc_cb, &cb);
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);
    WLX_Editor_State *st = wo_state(&ctx);
    ASSERT_TRUE(st != NULL);
    st->scroll_x = 20000.0f;
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);
    sc_reset_pieces();
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);

    WLX_Text_Geom_Entry *e = wo_entry(&ctx, 0);
    ASSERT_TRUE(e != NULL);
    ASSERT_TRUE(e->origin_rel > 0);
    ASSERT_EQ_INT(1, sc_q_count);
    ASSERT_EQ_INT((long)e->origin_rel, (long)sc_q[0].offset);
    ASSERT_EQ_INT(0, (long)sc_q[0].line);
    ASSERT_EQ_INT(0, (long)sc_q[0].line_start);
    ASSERT_EQ_INT((long)len, (long)sc_q[0].line_next);
    ASSERT_EQ_INT(1, sc_piece_count);
    ASSERT_EQ_F(sc_pieces[0].x, (float)SC_BAND_X_ - st->scroll_x + e->origin_x, 0.05f);
    ASSERT_EQ_COLOR(sc_pieces[0].color, SC_RED_);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Colour treatment
// ============================================================================

// A zero colour is the resolved front colour; a non-zero colour is drawn
// as the widget's own colours are at full opacity.
TEST(span_color_zero_is_front_color) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_text_slice = sc_capture_draw_text_slice;
    char buf[16];
    size_t len = sc_fill(buf, sizeof(buf), "abc\n");

    SC_Opts off = sc_opts(NULL, NULL);
    sc_frame(&ctx, buf, sizeof(buf), &len, &off);
    sc_reset_pieces();
    sc_frame(&ctx, buf, sizeof(buf), &len, &off);
    ASSERT_EQ_INT(1, sc_piece_count);
    WLX_Color front = sc_pieces[0].color;
    ASSERT_FALSE(wlx_color_is_zero(front));

    SC_Cb cb = { .mode = SC_MODE_ZERO, .a = SC_BLUE_ };
    SC_Opts on = sc_opts(sc_cb, &cb);
    sc_reset_pieces();
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);
    ASSERT_EQ_INT(1, sc_piece_count);
    ASSERT_EQ_COLOR(sc_pieces[0].color, front);

    cb.mode = SC_MODE_CONST;
    sc_reset_pieces();
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);
    ASSERT_EQ_INT(1, sc_piece_count);
    ASSERT_EQ_COLOR(sc_pieces[0].color, wlx_color_apply_opacity(SC_BLUE_, 1.0f));
    wlx_context_destroy(&ctx);
}

// A span colour under .disabled and an explicit opacity is exactly what
// front_color becomes under the same options: the same two transforms in
// the same order.
TEST(span_color_disabled_and_opacity_applied) {
    char buf[16];
    size_t len = sc_fill(buf, sizeof(buf), "abc\n");
    for (int variant = 0; variant < 3; variant++) {
        bool disabled = variant != 1;
        float opacity = variant != 0 ? 0.5f : -1.0f;

        WLX_Context ctx_front;
        test_ctx_init(&ctx_front, 400, 100);
        ctx_front.backend.draw_text_slice = sc_capture_draw_text_slice;
        SC_Opts as_front = sc_opts(NULL, NULL);
        as_front.disabled = disabled;
        as_front.opacity = opacity;
        as_front.front = SC_BLUE_;
        sc_frame(&ctx_front, buf, sizeof(buf), &len, &as_front);
        sc_reset_pieces();
        sc_frame(&ctx_front, buf, sizeof(buf), &len, &as_front);
        ASSERT_EQ_INT(1, sc_piece_count);
        WLX_Color want = sc_pieces[0].color;
        ASSERT_FALSE(wlx_color_is_zero(want));
        wlx_context_destroy(&ctx_front);

        WLX_Context ctx;
        test_ctx_init(&ctx, 400, 100);
        ctx.backend.draw_text_slice = sc_capture_draw_text_slice;
        SC_Cb cb = { .mode = SC_MODE_CONST, .a = SC_BLUE_ };
        SC_Opts on = sc_opts(sc_cb, &cb);
        on.disabled = disabled;
        on.opacity = opacity;
        sc_frame(&ctx, buf, sizeof(buf), &len, &on);
        sc_reset_pieces();
        sc_frame(&ctx, buf, sizeof(buf), &len, &on);
        ASSERT_EQ_INT(1, sc_piece_count);
        ASSERT_EQ_COLOR(sc_pieces[0].color, want);
        if (variant != 0) ASSERT_TRUE(sc_pieces[0].color.a < SC_BLUE_.a);
        wlx_context_destroy(&ctx);
    }
}

// ============================================================================
// Query discipline
// ============================================================================

// Only the records inside the band are asked about, an empty line never
// is, and within one record the offsets strictly increase and chain from
// each answered end.
TEST(span_color_called_only_for_visible_records) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    char buf[512];
    size_t off = 0;
    memcpy(buf + off, "ab\n\ncd\n", 7);
    off += 7;
    off += ev_fill_lines(buf + off, sizeof(buf) - off, 40);
    size_t len = off;

    SC_Cb cb = { .mode = SC_MODE_CONST, .a = SC_RED_ };
    SC_Opts on = sc_opts(sc_cb, &cb);
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);

    // Rows 0..9 sit inside the 92 px band at line height 10; "ab", the
    // empty line, "cd" and then the four-byte lines.
    ASSERT_TRUE(sc_q_count >= 8 && sc_q_count <= 10);
    for (int i = 0; i < sc_q_count; i++) {
        ASSERT_TRUE(sc_q[i].offset != 3);
        ASSERT_TRUE(sc_q[i].offset < 7 + 4 * 8);
        ASSERT_TRUE(sc_q[i].line < 10);
    }
    ASSERT_EQ_INT(0, (long)sc_q[0].offset);
    ASSERT_EQ_INT(4, (long)sc_q[1].offset);
    wlx_context_destroy(&ctx);
}

TEST(span_color_offsets_strictly_increasing) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    char buf[64];
    size_t len = sc_fill(buf, sizeof(buf), "hello world  foo\nbar\tbaz\n");

    SC_Cb cb = { .mode = SC_MODE_WORDS, .a = SC_RED_, .b = SC_GREEN_ };
    SC_Opts on = sc_opts(sc_cb, &cb);
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);
    sc_frame(&ctx, buf, sizeof(buf), &len, &on);

    ASSERT_TRUE(sc_q_count >= 8);
    for (int i = 0; i < sc_q_count; i++) {
        bool first_of_line = i == 0 || sc_q[i].line != sc_q[i - 1].line;
        if (first_of_line) {
            ASSERT_EQ_INT((long)sc_q[i].line_start, (long)sc_q[i].offset);
        } else {
            ASSERT_TRUE(sc_q[i].offset > sc_q[i - 1].offset);
            ASSERT_EQ_INT((long)sc_q[i - 1].end, (long)sc_q[i].offset);
        }
        bool last_of_line = i + 1 == sc_q_count || sc_q[i + 1].line != sc_q[i].line;
        if (last_of_line) ASSERT_EQ_INT((long)sc_q[i].limit, (long)sc_q[i].end);
    }
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Nothing but colour changes
// ============================================================================

// Caret offset and anchor after a click and a drag, and every rect the
// frame draws (selection band, caret, chrome), are identical with the
// hook on and off, in both wrap modes.
TEST(span_color_geometry_identical_with_hook) {
    char buf[256];
    size_t len = sc_fill(buf, sizeof(buf),
        "hello\tworld foo bar baz\nquick brown fox jumps over the lazy dog again and again\n");
    for (int wrap = 0; wrap < 2; wrap++) {
        size_t caret[2], anchor[2];
        int rect_count[2];
        WLX_Rect rects[2][EV_MAX_RECTS_];
        for (int hook = 0; hook < 2; hook++) {
            WLX_Context ctx;
            test_ctx_init(&ctx, 400, 100);
            ctx.backend.draw_rect = _ev_capture_draw_rect;
            SC_Cb cb = { .mode = SC_MODE_WORDS, .a = SC_RED_, .b = SC_GREEN_ };
            SC_Opts o = sc_opts(hook ? sc_cb : NULL, hook ? &cb : NULL);
            o.wrap = wrap == 1;
            sc_frame(&ctx, buf, sizeof(buf), &len, &o);
            sc_frame_full(&ctx, buf, sizeof(buf), &len, &o, 60, 8, true, true, 0.0f, NULL, NULL);
            sc_frame_full(&ctx, buf, sizeof(buf), &len, &o, 150, 18, true, false, 0.0f, NULL, NULL);
            _ev_reset_rects();
            sc_frame_full(&ctx, buf, sizeof(buf), &len, &o, 150, 18, false, false, 0.0f, NULL, NULL);
            WLX_Editor_State *st = ev_state(&ctx);
            ASSERT_TRUE(st != NULL);
            caret[hook] = st->caret.cursor_pos;
            anchor[hook] = st->caret.selection_anchor;
            rect_count[hook] = _ev_rect_count;
            memcpy(rects[hook], _ev_rects, sizeof(_ev_rects));
            wlx_context_destroy(&ctx);
        }
        ASSERT_TRUE(caret[0] != anchor[0]);
        ASSERT_EQ_INT((long)caret[0], (long)caret[1]);
        ASSERT_EQ_INT((long)anchor[0], (long)anchor[1]);
        ASSERT_EQ_INT(rect_count[0], rect_count[1]);
        ASSERT_TRUE(rect_count[0] > 0);
        for (int i = 0; i < rect_count[0]; i++) ASSERT_EQ_RECT(rects[0][i], rects[1][i], 0.001f);
    }
}

// Measure traffic on cold, idle and typing frames is identical with the
// hook on and off, in both wrap modes: every piece x comes from the
// stored advances. Each run starts from the same document bytes (the
// typing frame edits the buffer).
TEST(span_color_no_extra_measures_on_steady_frame) {
    char doc[512];
    size_t off = 0;
    for (int i = 0; i < 12; i++) {
        off += sc_fill(doc + off, sizeof(doc) - off, "alpha beta\tgamma delta epsilon zeta eta\n");
    }
    size_t len0 = off;
    for (int wrap = 0; wrap < 2; wrap++) {
        unsigned long long cold[2], idle[2], typing[2];
        for (int hook = 0; hook < 2; hook++) {
            WLX_Context ctx;
            test_ctx_init(&ctx, 400, 100);
            ctx.backend.measure_text_slice = wo_counting_measure;
            char buf[512];
            memcpy(buf, doc, sizeof(buf));
            size_t len = len0;
            SC_Cb cb = { .mode = SC_MODE_WORDS, .a = SC_RED_, .b = SC_GREEN_ };
            SC_Opts o = sc_opts(hook ? sc_cb : NULL, hook ? &cb : NULL);
            o.wrap = wrap == 1;
            sc_frame(&ctx, buf, sizeof(buf), &len, &o);
            cold[hook] = wo_calls;
            sc_frame(&ctx, buf, sizeof(buf), &len, &o);
            sc_frame(&ctx, buf, sizeof(buf), &len, &o);
            idle[hook] = wo_calls;
            sc_frame_full(&ctx, buf, sizeof(buf), &len, &o, 60, 8, true, true, 0.0f, NULL, NULL);
            sc_frame_full(&ctx, buf, sizeof(buf), &len, &o, 60, 8, false, false, 0.0f, NULL, "x");
            typing[hook] = wo_calls;
            wlx_context_destroy(&ctx);
        }
        ASSERT_EQ_INT((long)cold[0], (long)cold[1]);
        ASSERT_EQ_INT((long)idle[0], (long)idle[1]);
        ASSERT_EQ_INT((long)typing[0], (long)typing[1]);
        ASSERT_TRUE(idle[0] <= 1);
    }
}

// ============================================================================
// Suite
// ============================================================================

SUITE(editor_span_color) {
    RUN_TEST(span_color_null_identity_tabs);
    RUN_TEST(span_color_null_identity_wrapped);
    RUN_TEST(span_color_null_identity_windowed_origin);
    RUN_TEST(span_color_every_word_pieces_at_stored_advances);
    RUN_TEST(span_color_edge_inside_cluster_snaps_forward);
    RUN_TEST(span_color_end_at_or_before_offset_progresses_one_unit);
    RUN_TEST(span_color_end_clipped_to_record_and_document);
    RUN_TEST(span_color_edge_on_tab_and_tab_inside_span);
    RUN_TEST(span_color_span_crosses_wrapped_row);
    RUN_TEST(span_color_span_crosses_windowed_origin);
    RUN_TEST(span_color_zero_is_front_color);
    RUN_TEST(span_color_disabled_and_opacity_applied);
    RUN_TEST(span_color_called_only_for_visible_records);
    RUN_TEST(span_color_offsets_strictly_increasing);
    RUN_TEST(span_color_geometry_identical_with_hook);
    RUN_TEST(span_color_no_extra_measures_on_steady_frame);
}
