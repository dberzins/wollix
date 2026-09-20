// test_editor_geom_cache.c - retained editor line geometry: record replay
// equivalence against the measuring build (both modes, ascending and
// descending reach, unit-budget cap), the invalidation matrix (typing,
// newline split, backspace join, select-all replace, external mutation via
// .revision, font and wrap-mode environment changes), entry-key shifting
// across edits, LRU bounds, and steady-state traffic (idle frames measure
// nothing beyond the frame's reference measure).
//
// Geometry model (mock backend): char width = font_size/2 = 5px at the
// fixture's font_size 10, line height = 10. Fixture: 400x100 context,
// content_padding 4, border 0.

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

static unsigned long long gc_calls;

static void gc_counting_measure(const char *text, size_t len, WLX_Text_Style style,
                                float *out_w, float *out_h, void *user) {
    (void)user;
    (void)text;
    gc_calls++;
    int fs = style.font_size > 0 ? style.font_size : 10;
    if (out_w) *out_w = (float)len * (float)fs * 0.5f;
    if (out_h) *out_h = (float)fs;
}

static void gc_ctx_init(WLX_Context *ctx, float w, float h) {
    test_ctx_init(ctx, w, h);
    ctx->backend.measure_text_slice = gc_counting_measure;
    gc_calls = 0;
}

// One editor frame with full input control; returns this frame's measure
// call count. font_size parametrized for the environment-epoch tests.
static unsigned long long gc_frame_opt(WLX_Context *ctx, char *buf, size_t cap,
    size_t *len, uint32_t rev, bool wrap, int font_size,
    int mx, int my, bool clicked, float wheel, uint32_t mods,
    const bool *keys_pressed, const char *text)
{
    gc_calls = 0;
    test_frame_begin_full(ctx, mx, my, clicked, clicked, clicked, wheel,
        NULL, keys_pressed, NULL, mods, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    (void)wlx_editor_impl(ctx, NULL, buf, cap, len,
        wlx_default_editor_opt(.content_padding = 4, .font_size = font_size,
            .border_width = 0, .revision = rev, .wrap = wrap),
        "gc_editor", 1);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return gc_calls;
}

static unsigned long long gc_frame(WLX_Context *ctx, char *buf, size_t cap,
    size_t *len, uint32_t rev, bool wrap)
{
    return gc_frame_opt(ctx, buf, cap, len, rev, wrap, 10, 0, 0, false, 0.0f, 0, NULL, NULL);
}

static unsigned long long gc_frame_click(WLX_Context *ctx, char *buf, size_t cap,
    size_t *len, uint32_t rev, bool wrap, int mx, int my)
{
    return gc_frame_opt(ctx, buf, cap, len, rev, wrap, 10, mx, my, true, 0.0f, 0, NULL, NULL);
}

static unsigned long long gc_frame_text(WLX_Context *ctx, char *buf, size_t cap,
    size_t *len, uint32_t rev, bool wrap, const char *text)
{
    return gc_frame_opt(ctx, buf, cap, len, rev, wrap, 10, 0, 0, false, 0.0f, 0, NULL, text);
}

static unsigned long long gc_frame_key(WLX_Context *ctx, char *buf, size_t cap,
    size_t *len, uint32_t rev, bool wrap, WLX_Key_Code key, uint32_t mods)
{
    bool keys[WLX_KEY_COUNT] = {0};
    keys[key] = true;
    return gc_frame_opt(ctx, buf, cap, len, rev, wrap, 10, 0, 0, false, 0.0f, mods, keys, NULL);
}

static WLX_Editor_Line_Index *gc_index(WLX_Context *ctx) {
    return ctx->editor_indices.count > 0 ? &ctx->editor_indices.items[0] : NULL;
}

static WLX_Editor_State *gc_state(WLX_Context *ctx) {
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        WLX_State_Map_Slot *slot = &ctx->states.slots[i];
        if (slot->id != 0 && slot->data_size == sizeof(WLX_Editor_State))
            return (WLX_Editor_State *)slot->data;
    }
    return NULL;
}

// Every used entry must key a real line of the current index: line_start an
// indexed hard line start and line_next that line's following offset. This
// is the store's core safety invariant after any invalidation path.
static void gc_assert_store_keys_valid(WLX_Context *ctx, size_t doc_len) {
    WLX_Editor_Line_Index *idx = gc_index(ctx);
    ASSERT_TRUE(idx != NULL);
    WLX_Text_Geom_Store *s = &idx->geom;
    for (size_t i = 0; i < s->count; i++) {
        WLX_Text_Geom_Entry *e = &s->entries[i];
        if (!e->used) continue;
        size_t line = wlx_editor_index_line_of(idx, e->line_start);
        ASSERT_EQ_INT((long)idx->offsets[line], (long)e->line_start);
        size_t next = line + 1 < idx->count ? idx->offsets[line + 1] : doc_len;
        ASSERT_EQ_INT((long)next, (long)e->line_next);
    }
}

// Fill buf with n 4-byte lines "l00\n".."lNN\n", returning the length.
static size_t gc_fill_lines(char *buf, size_t cap, size_t n) {
    size_t off = 0;
    for (size_t i = 0; i < n && off + 4 < cap; i++) {
        buf[off++] = 'l';
        buf[off++] = (char)('0' + (i / 10) % 10);
        buf[off++] = (char)('0' + i % 10);
        buf[off++] = '\n';
    }
    buf[off] = '\0';
    return off;
}

// ============================================================================
// Record equivalence: replay vs measuring build
// ============================================================================

// Mixed-content corpus: ASCII, empty line, tabs, multi-byte UTF-8,
// malformed bytes, CRLF, a line without a trailing newline.
static const char gc_corpus[] =
    "plain ascii line\n"
    "\n"
    "tabs\tbetween\tsegments here\n"
    "utf8 \xC3\xA9\xC3\xA9 mixed \xE4\xB8\xAD\xE6\x96\x87 tail\n"
    "bad \xFF\xFE bytes\n"
    "crlf line\r\n"
    "ends without newline";

static void gc_assert_records_equal(const WLX_Text_Line_Record *a,
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
static size_t gc_stream_line(const WLX_Text_Build_Inputs *in_base,
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

// Compare the retained-store stream against the measuring stream for every
// line of the current document at the given no-wrap virtual width.
static void gc_assert_linear_equivalence(WLX_Context *ctx, const char *doc,
    size_t len, float w)
{
    WLX_Editor_Line_Index *idx = gc_index(ctx);
    ASSERT_TRUE(idx != NULL);
    WLX_Text_Style ts = { .font = 0, .font_size = 10, .color = {255,255,255,255}, .spacing = 0 };
    WLX_Text_Build_Inputs base = {
        .ctx = ctx, .text = doc, .length = len, .style = ts,
        .rect = { 0, 0, w, 1000 }, .wrap = false,
        .line_h = idx->geom.env.line_h,
        .text_unit_cap = WLX_EDITOR_MAX_LINE_UNITS,
        .truncate_continue = true,
        .tab_advance = idx->geom.env.tab_advance,
    };
    WLX_Text_Build_Inputs with_geom = base;
    with_geom.geom = &idx->geom;

    for (size_t line = 0; line < idx->count; line++) {
        WLX_Text_Line_Record ra, rb;
        size_t na = gc_stream_line(&with_geom, idx, line, &ra, 1);
        size_t nb = gc_stream_line(&base, idx, line, &rb, 1);
        ASSERT_EQ_INT((long)nb, (long)na);
        if (na == 1) gc_assert_records_equal(&ra, &rb);
    }
}

TEST(geom_linear_records_match_measuring_build_across_reach) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    char buf[512];
    memcpy(buf, gc_corpus, sizeof(gc_corpus));
    size_t len = sizeof(gc_corpus) - 1;

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);

    // Ascending reach extends entries in place; the descending pass refits
    // narrower widths from the extended arrays. Both must reproduce the
    // measuring build exactly.
    float widths[] = { 12.0f, 60.0f, 200.0f, 5000.0f, 60.0f, 12.0f };
    for (size_t i = 0; i < sizeof(widths) / sizeof(widths[0]); i++) {
        gc_assert_linear_equivalence(&ctx, buf, len, widths[i]);
    }
    gc_assert_store_keys_valid(&ctx, len);
    wlx_context_destroy(&ctx);
}

TEST(geom_linear_budget_cap_matches_measuring_build) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    // One hard line far beyond the unit budget, then a short line.
    static char buf[4200];
    size_t off = 0;
    for (; off < 3000; off++) buf[off] = (char)('a' + (off % 7 == 6 ? -66 : (int)(off % 26)));
    buf[off++] = '\n';
    buf[off++] = 'z';
    buf[off] = '\0';
    size_t len = off;

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    gc_assert_linear_equivalence(&ctx, buf, len, 2000000.0f);
    gc_assert_linear_equivalence(&ctx, buf, len, 150.0f);

    // The giant line's entry froze at the unit budget.
    WLX_Editor_Line_Index *idx = gc_index(&ctx);
    WLX_Text_Geom_Entry *e = wlx_text_geom_find(&idx->geom, 0, 3001);
    ASSERT_TRUE(e != NULL);
    ASSERT_EQ_INT((long)WLX_EDITOR_MAX_LINE_UNITS, (long)e->units);
    wlx_context_destroy(&ctx);
}

TEST(geom_wrap_rows_match_measuring_build) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    static char buf[2048];
    size_t off = 0;
    // Long wrapping lines with tabs and UTF-8, plus corpus-style edges.
    for (int l = 0; l < 3; l++) {
        for (int i = 0; i < 150; i++) {
            buf[off++] = (i % 17 == 16) ? '\t' : (char)('a' + (i + l) % 26);
        }
        buf[off++] = '\n';
    }
    memcpy(buf + off, gc_corpus, sizeof(gc_corpus));
    size_t len = off + sizeof(gc_corpus) - 1;

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, true);
    gc_frame(&ctx, buf, sizeof(buf), &len, 1, true);

    WLX_Editor_Line_Index *idx = gc_index(&ctx);
    ASSERT_TRUE(idx != NULL);
    WLX_Text_Style ts = { .font = 0, .font_size = 10, .color = {255,255,255,255}, .spacing = 0 };
    WLX_Text_Build_Inputs base = wlx_editor_wrap_build_inputs(&ctx, buf, len, ts,
        idx->geom.env.band_w, idx->geom.env.line_h, idx->geom.env.tab_advance);
    WLX_Text_Build_Inputs with_geom = base;
    with_geom.geom = &idx->geom;

    for (size_t line = 0; line < idx->count; line++) {
        WLX_Text_Line_Record ra[64], rb[64];
        size_t na = gc_stream_line(&with_geom, idx, line, ra, 64);
        size_t nb = gc_stream_line(&base, idx, line, rb, 64);
        ASSERT_EQ_INT((long)nb, (long)na);
        for (size_t r = 0; r < na; r++) gc_assert_records_equal(&ra[r], &rb[r]);
    }
    gc_assert_store_keys_valid(&ctx, len);
    wlx_context_destroy(&ctx);
}

static void gc_put(char *buf, size_t cap, size_t *off, const char *s) {
    size_t n = strlen(s);
    if (*off + n + 1 > cap) return;
    memcpy(buf + *off, s, n);
    *off += n;
    buf[*off] = '\0';
}

static void gc_put_rep(char *buf, size_t cap, size_t *off, char c, size_t n) {
    for (size_t i = 0; i < n && *off + 2 <= cap; i++) buf[(*off)++] = c;
    buf[*off] = '\0';
}

// Prose for the word-boundary rule: doubled spaces, a token ending exactly
// at the 373 px band followed by a space (the space opens the next row:
// the editor builds strict), the same one unit longer, tabs between words,
// a word wider than the band, a line of spaces only, a spaces-only line
// wider than the band, a trailing space before a newline, and an
// unterminated last line that wraps.
static size_t gc_fill_prose(char *buf, size_t cap) {
    size_t off = 0;
    gc_put(buf, cap, &off, "prose  with  doubled  spaces  between  every  word  of  a  line  "
        "that  wraps  past  the  band  twice  over  and  keeps  going  on  and  on\n");
    gc_put_rep(buf, cap, &off, 'x', 74);
    gc_put(buf, cap, &off, " tail after the hanging space\n");
    gc_put_rep(buf, cap, &off, 'y', 76);
    gc_put(buf, cap, &off, " tail after the cut\n");
    gc_put(buf, cap, &off, "tab\tseparated\twords\tin\ta\tline\tthat\tgoes\ton\tlong\tenough\t"
        "to\twrap\tat\ta\ttab\tstop\tsomewhere\tpast\tthe\tband\twidth\tfor\tsure\n");
    gc_put(buf, cap, &off, "short ");
    gc_put_rep(buf, cap, &off, 'z', 100);
    gc_put(buf, cap, &off, " after the over-wide word\n");
    gc_put(buf, cap, &off, "          \n");
    gc_put_rep(buf, cap, &off, ' ', 80);
    gc_put(buf, cap, &off, "\n");
    gc_put(buf, cap, &off, "ends with a space \n");
    gc_put(buf, cap, &off, "no newline at the end of this last spaced prose line that also "
        "wraps around the band width at least once more before it stops");
    return off;
}

TEST(geom_wrap_rows_match_measuring_build_on_prose) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    static char buf[2048];
    size_t len = gc_fill_prose(buf, sizeof(buf));
    ASSERT_TRUE(len > 600);

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, true);
    gc_frame(&ctx, buf, sizeof(buf), &len, 1, true);

    WLX_Editor_Line_Index *idx = gc_index(&ctx);
    ASSERT_TRUE(idx != NULL);
    ASSERT_EQ_INT(9, (long)idx->count);
    WLX_Text_Style ts = { .font = 0, .font_size = 10, .color = {255,255,255,255}, .spacing = 0 };
    WLX_Text_Build_Inputs base = wlx_editor_wrap_build_inputs(&ctx, buf, len, ts,
        idx->geom.env.band_w, idx->geom.env.line_h, idx->geom.env.tab_advance);
    WLX_Text_Build_Inputs with_geom = base;
    with_geom.geom = &idx->geom;

    for (size_t line = 0; line < idx->count; line++) {
        WLX_Text_Line_Record ra[64], rb[64];
        size_t na = gc_stream_line(&with_geom, idx, line, ra, 64);
        size_t nb = gc_stream_line(&base, idx, line, rb, 64);
        ASSERT_EQ_INT((long)nb, (long)na);
        for (size_t r = 0; r < na; r++) {
            gc_assert_records_equal(&ra[r], &rb[r]);
            // The editor builds strict: every row measures within the
            // band (no unit of the corpus is wider than it), and a row's
            // ink extent never exceeds its measured extent.
            ASSERT_TRUE(ra[r].measured_w <= idx->geom.env.band_w + 0.01f);
            if (r + 1 < na) ASSERT_TRUE(ra[r].advance_w <= ra[r].measured_w);
        }
    }
    // The 74-byte token fills the band and its space does not fit: with
    // no earlier opportunity the space opens the next row.
    {
        WLX_Text_Line_Record r[64];
        size_t n = gc_stream_line(&with_geom, idx, 1, r, 64);
        ASSERT_EQ_INT(2, (long)n);
        ASSERT_EQ_INT((long)(r[0].visible_end - r[0].visible_start), 74);
        ASSERT_EQ_F(r[0].measured_w, 370.0f, 0.01f);
        ASSERT_EQ_F(r[0].advance_w, 370.0f, 0.01f);
        ASSERT_TRUE(buf[r[1].visible_start] == ' ');
        ASSERT_EQ_INT((long)(r[1].visible_end - r[1].visible_start), 29);
        ASSERT_TRUE(r[1].ended_by_newline);
    }
    // The spaces-only line wider than the band cuts at its 74th space:
    // a first row of spaces with no ink, then the rest.
    {
        WLX_Text_Line_Record r[64];
        size_t n = gc_stream_line(&with_geom, idx, 6, r, 64);
        ASSERT_EQ_INT(2, (long)n);
        ASSERT_EQ_INT((long)(r[0].visible_end - r[0].visible_start), 74);
        ASSERT_EQ_F(r[0].measured_w, 370.0f, 0.01f);
        ASSERT_EQ_F(r[0].advance_w, 0.0f, 0.01f);
        ASSERT_EQ_INT((long)(r[1].visible_end - r[1].visible_start), 6);
        ASSERT_EQ_F(r[1].measured_w, 30.0f, 0.01f);
    }
    gc_assert_store_keys_valid(&ctx, len);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Steady-state traffic
// ============================================================================

TEST(geom_idle_frames_measure_nothing_beyond_reference) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    char buf[512];
    size_t len = gc_fill_lines(buf, sizeof(buf), 30);

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    unsigned long long idle = gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    // The frame's one reference measure (line height probe) only.
    ASSERT_TRUE(idle <= 2);

    // Wrapped mode idles the same way.
    gc_frame(&ctx, buf, sizeof(buf), &len, 1, true);
    unsigned long long wrap_idle = gc_frame(&ctx, buf, sizeof(buf), &len, 1, true);
    ASSERT_TRUE(wrap_idle <= 2);
    wlx_context_destroy(&ctx);
}

TEST(geom_tabbed_draw_replays_segments_from_store) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    char buf[512];
    // Tab-heavy document: every line splits into three drawn segments, so
    // a draw path that re-measures per segment shows up as steady-state
    // traffic well above the frame's reference measure. More lines than
    // the band has rows, so the wrapped scrollbar decision short-circuits
    // and the pre-strip row probe (a documented store-independent
    // residual) stays out of the counts.
    size_t len = 0;
    for (int i = 0; i < 12; i++) {
        len += (size_t)snprintf(buf + len, sizeof(buf) - len, "aa\tbb\tcc%d\n", i);
    }

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    unsigned long long idle = gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);

    // Wrapped mode replays the same way.
    gc_frame(&ctx, buf, sizeof(buf), &len, 1, true);
    unsigned long long wrap_idle = gc_frame(&ctx, buf, sizeof(buf), &len, 1, true);
    // Steady state: the reference measure only - tab segment x positions
    // replay from stored advances instead of re-measuring every frame.
    ASSERT_TRUE(idle <= 2);
    ASSERT_TRUE(wrap_idle <= 2);
    wlx_context_destroy(&ctx);
}

TEST(geom_steady_frames_allocate_and_grow_nothing) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    char buf[512];
    size_t len = gc_fill_lines(buf, sizeof(buf), 30);
    for (int i = 0; i < 3; i++) gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);

    WLX_Text_Geom_Store *s = &gc_index(&ctx)->geom;
    uint32_t *ends_before[64];
    size_t caps_before[64];
    size_t n = s->count < 64 ? s->count : 64;
    for (size_t i = 0; i < n; i++) {
        ends_before[i] = s->entries[i].unit_ends;
        caps_before[i] = s->entries[i].unit_cap;
    }
    for (int i = 0; i < 10; i++) {
        ASSERT_TRUE(gc_frame(&ctx, buf, sizeof(buf), &len, 1, false) <= 2);
    }
    for (size_t i = 0; i < n; i++) {
        ASSERT_TRUE(ends_before[i] == s->entries[i].unit_ends);
        ASSERT_EQ_INT((long)caps_before[i], (long)s->entries[i].unit_cap);
    }
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Invalidation matrix
// ============================================================================

TEST(geom_typing_remeasures_only_edited_line_and_shifts_keys) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    char buf[512];
    size_t len = gc_fill_lines(buf, sizeof(buf), 9);

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    WLX_Editor_Line_Index *idx = gc_index(&ctx);
    size_t l5 = idx->offsets[5], l5n = idx->offsets[6];
    ASSERT_TRUE(wlx_text_geom_find(&idx->geom, l5, l5n) != NULL);

    // Focus mid-line 2 (band.y 4, line_h 10 -> y 25 is line 2), then type.
    gc_frame_click(&ctx, buf, sizeof(buf), &len, 1, false, 30, 25);
    unsigned long long typed = gc_frame_text(&ctx, buf, sizeof(buf), &len, 1, false, "x");
    ASSERT_TRUE(len == 37);
    // Reference measure + caret-follow + the edited 4-5 unit line only.
    ASSERT_TRUE(typed >= 3 && typed <= 12);

    // The entry for line 5 survived the edit with its key shifted by one.
    ASSERT_TRUE(wlx_text_geom_find(&idx->geom, l5 + 1, l5n + 1) != NULL);
    gc_assert_store_keys_valid(&ctx, len);

    ASSERT_TRUE(gc_frame(&ctx, buf, sizeof(buf), &len, 1, false) <= 2);
    wlx_context_destroy(&ctx);
}

TEST(geom_enter_splits_line_and_keeps_following_entries) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    char buf[512];
    size_t len = gc_fill_lines(buf, sizeof(buf), 9);

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    WLX_Editor_Line_Index *idx = gc_index(&ctx);
    size_t l6 = idx->offsets[6], l6n = idx->offsets[7];

    gc_frame_click(&ctx, buf, sizeof(buf), &len, 1, false, 15, 25); // mid line 2
    gc_frame_key(&ctx, buf, sizeof(buf), &len, 1, false, WLX_KEY_ENTER, 0);
    ASSERT_TRUE(len == 37);
    ASSERT_EQ_INT(11, (long)idx->count); // 9 lines + split + trailing empty
    ASSERT_TRUE(wlx_text_geom_find(&idx->geom, l6 + 1, l6n + 1) != NULL);
    gc_assert_store_keys_valid(&ctx, len);
    wlx_context_destroy(&ctx);
}

TEST(geom_backspace_join_drops_both_lines_and_shifts_rest) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    char buf[512];
    size_t len = gc_fill_lines(buf, sizeof(buf), 9);

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    WLX_Editor_Line_Index *idx = gc_index(&ctx);
    size_t l7 = idx->offsets[7], l7n = idx->offsets[8];

    // Caret at the start of line 3 (x at the band edge), then Backspace
    // joins lines 2 and 3 by deleting the separator.
    gc_frame_click(&ctx, buf, sizeof(buf), &len, 1, false, 10, 35);
    ASSERT_EQ_INT((long)idx->offsets[3], (long)gc_state(&ctx)->caret.cursor_pos);
    gc_frame_key(&ctx, buf, sizeof(buf), &len, 1, false, WLX_KEY_BACKSPACE, 0);
    ASSERT_TRUE(len == 35);
    ASSERT_EQ_INT(9, (long)idx->count);
    ASSERT_TRUE(wlx_text_geom_find(&idx->geom, l7 - 1, l7n - 1) != NULL);
    gc_assert_store_keys_valid(&ctx, len);

    // Joined line "l02l03": caret x is analytic under the mock model
    // (5 px per ASCII char).
    float x = wlx_editor_caret_x(&ctx, buf, len, (WLX_Text_Style){ .font_size = 10 },
        20.0f, &idx->geom, idx->offsets[3], idx->offsets[2], idx->offsets[2] + 4, true);
    ASSERT_EQ_F(20.0f, x, 0.0001f);
    wlx_context_destroy(&ctx);
}

TEST(geom_select_all_replace_stays_consistent) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    char buf[512];
    size_t len = gc_fill_lines(buf, sizeof(buf), 9);

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    gc_frame_click(&ctx, buf, sizeof(buf), &len, 1, false, 30, 25);
    gc_frame_key(&ctx, buf, sizeof(buf), &len, 1, false, WLX_KEY_A, test_command_mod());
    gc_frame_text(&ctx, buf, sizeof(buf), &len, 1, false, "z");
    ASSERT_TRUE(len == 1);
    ASSERT_TRUE(buf[0] == 'z');
    gc_assert_store_keys_valid(&ctx, len);
    ASSERT_TRUE(gc_frame(&ctx, buf, sizeof(buf), &len, 1, false) <= 2);
    wlx_context_destroy(&ctx);
}

TEST(geom_external_mutation_revision_clears_store) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    char buf[512];
    size_t len = gc_fill_lines(buf, sizeof(buf), 9);

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    // Same length, different content: only the revision says so.
    buf[0] = 'X';
    buf[4] = 'Y';
    gc_frame(&ctx, buf, sizeof(buf), &len, 2, false);
    gc_assert_store_keys_valid(&ctx, len);

    // The rebuilt geometry reflects the new content exactly.
    WLX_Editor_Line_Index *idx = gc_index(&ctx);
    float x = wlx_editor_caret_x(&ctx, buf, len, (WLX_Text_Style){ .font_size = 10 },
        20.0f, &idx->geom, idx->offsets[1], idx->offsets[0], 3, true);
    ASSERT_EQ_F(15.0f, x, 0.0001f);
    ASSERT_TRUE(gc_frame(&ctx, buf, sizeof(buf), &len, 2, false) <= 2);
    wlx_context_destroy(&ctx);
}

TEST(geom_external_length_change_clears_store) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    char buf[512];
    size_t len = gc_fill_lines(buf, sizeof(buf), 9);

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    len = gc_fill_lines(buf, sizeof(buf), 5); // out-of-widget shrink
    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    gc_assert_store_keys_valid(&ctx, len);
    ASSERT_TRUE(gc_frame(&ctx, buf, sizeof(buf), &len, 1, false) <= 2);
    wlx_context_destroy(&ctx);
}

TEST(geom_font_change_rebuilds_geometry) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    char buf[512];
    size_t len = gc_fill_lines(buf, sizeof(buf), 9);

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    // A font-size change re-measures everything once, then idles again.
    unsigned long long changed = gc_frame_opt(&ctx, buf, sizeof(buf), &len, 1, false, 12,
        0, 0, false, 0.0f, 0, NULL, NULL);
    ASSERT_TRUE(changed > 10);
    gc_assert_store_keys_valid(&ctx, len);

    WLX_Editor_Line_Index *idx = gc_index(&ctx);
    float x = wlx_editor_caret_x(&ctx, buf, len, (WLX_Text_Style){ .font_size = 12 },
        24.0f, &idx->geom, idx->offsets[1], idx->offsets[0], 3, true);
    ASSERT_EQ_F(18.0f, x, 0.0001f); // 3 chars at 6 px
    ASSERT_TRUE(gc_frame_opt(&ctx, buf, sizeof(buf), &len, 1, false, 12,
        0, 0, false, 0.0f, 0, NULL, NULL) <= 2);
    wlx_context_destroy(&ctx);
}

TEST(geom_wrap_toggle_clears_and_rebuilds_row_tables) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    static char buf[1024];
    size_t off = 0;
    for (int i = 0; i < 150; i++) buf[off++] = (char)('a' + i % 26);
    buf[off++] = '\n';
    // Enough lines that the wrapped-overflow decision needs no row probe
    // (the probe deliberately measures outside the store every frame).
    off += gc_fill_lines(buf + off, sizeof(buf) - off, 15);
    size_t len = off;

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    WLX_Editor_Line_Index *idx = gc_index(&ctx);
    WLX_Text_Geom_Entry *e = wlx_text_geom_find(&idx->geom, 0, 151);
    ASSERT_TRUE(e != NULL);
    ASSERT_EQ_INT(0, (long)e->rows); // no-wrap entries carry no row table

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, true);
    e = wlx_text_geom_find(&idx->geom, 0, 151);
    ASSERT_TRUE(e != NULL);
    ASSERT_TRUE(e->rows >= 2); // 150 chars * 5 px wraps in a < 400 px band
    ASSERT_TRUE(gc_frame(&ctx, buf, sizeof(buf), &len, 1, true) <= 2);

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    e = wlx_text_geom_find(&idx->geom, 0, 151);
    ASSERT_TRUE(e != NULL);
    ASSERT_EQ_INT(0, (long)e->rows);
    gc_assert_store_keys_valid(&ctx, len);
    wlx_context_destroy(&ctx);
}

TEST(geom_wrap_typing_touches_only_edited_line) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    static char buf[2048];
    size_t off = 0;
    // Enough lines that the wrapped-overflow decision needs no row probe.
    for (int l = 0; l < 12; l++) {
        for (int i = 0; i < 100; i++) buf[off++] = (char)('a' + (i + l) % 26);
        buf[off++] = '\n';
    }
    size_t len = off;

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, true);
    WLX_Editor_Line_Index *idx = gc_index(&ctx);
    size_t l3 = idx->offsets[3], l3n = idx->offsets[4];

    gc_frame_click(&ctx, buf, sizeof(buf), &len, 1, true, 30, 15);
    unsigned long long typed = gc_frame_text(&ctx, buf, sizeof(buf), &len, 1, true, "x");
    ASSERT_TRUE(len == (size_t)(12 * 101 + 1));
    // Reference measure + one ~101-unit line re-walk (with a few
    // row-boundary re-measures), nothing else.
    ASSERT_TRUE(typed >= 50 && typed <= 130);
    ASSERT_TRUE(wlx_text_geom_find(&idx->geom, l3 + 1, l3n + 1) != NULL);
    gc_assert_store_keys_valid(&ctx, len);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Round-trips and bounds
// ============================================================================

TEST(geom_caret_roundtrip_warm_matches_analytic) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    char buf[512];
    size_t len = gc_fill_lines(buf, sizeof(buf), 9);

    for (int i = 0; i < 3; i++) gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    WLX_Editor_Line_Index *idx = gc_index(&ctx);
    WLX_Text_Style ts = { .font_size = 10 };

    for (size_t line = 0; line < 8; line++) {
        size_t start = idx->offsets[line];
        size_t next = idx->offsets[line + 1];
        for (size_t k = 0; k <= 3; k++) {
            float x = wlx_editor_caret_x(&ctx, buf, len, ts, 20.0f,
                &idx->geom, next, start, start + k, true);
            ASSERT_EQ_F((float)k * 5.0f, x, 0.0001f);
            size_t back = wlx_editor_offset_at_x(&ctx, buf, len, ts, 10.0f, 20.0f,
                idx, &idx->geom, line, 1000.0f, 0.0f, x + 0.1f);
            ASSERT_EQ_INT((long)(start + k), (long)back);
        }
    }
    // The round-trips above must have replayed, not measured: only the
    // caret_x fallback for offsets beyond stored coverage would measure,
    // and none apply on 3-unit lines.
    gc_calls = 0;
    (void)wlx_editor_caret_x(&ctx, buf, len, ts, 20.0f, &idx->geom,
        idx->offsets[1], idx->offsets[0], idx->offsets[0] + 2, true);
    ASSERT_EQ_INT(0, (long)gc_calls);
    wlx_context_destroy(&ctx);
}

TEST(geom_store_stays_bounded_under_full_document_scroll) {
    WLX_Context ctx;
    gc_ctx_init(&ctx, 400, 100);
    static char buf[4096];
    size_t len = gc_fill_lines(buf, sizeof(buf), 600);

    gc_frame(&ctx, buf, sizeof(buf), &len, 1, false);
    WLX_Text_Geom_Store *s = &gc_index(&ctx)->geom;
    // Wheel through the whole document (band 92 px, ~9 lines per view).
    for (int i = 0; i < 400; i++) {
        gc_frame_opt(&ctx, buf, sizeof(buf), &len, 1, false, 10,
            30, 30, false, -1.0f, 0, NULL, NULL);
    }
    ASSERT_TRUE(s->count <= s->want_cap);
    size_t used = 0;
    for (size_t i = 0; i < s->count; i++) used += s->entries[i].used ? 1 : 0;
    ASSERT_TRUE(used <= s->count);
    ASSERT_TRUE(used > 0);
    gc_assert_store_keys_valid(&ctx, len);
    wlx_context_destroy(&ctx);
}

SUITE(editor_geom_cache) {
    RUN_TEST(geom_linear_records_match_measuring_build_across_reach);
    RUN_TEST(geom_linear_budget_cap_matches_measuring_build);
    RUN_TEST(geom_wrap_rows_match_measuring_build);
    RUN_TEST(geom_wrap_rows_match_measuring_build_on_prose);
    RUN_TEST(geom_idle_frames_measure_nothing_beyond_reference);
    RUN_TEST(geom_tabbed_draw_replays_segments_from_store);
    RUN_TEST(geom_steady_frames_allocate_and_grow_nothing);
    RUN_TEST(geom_typing_remeasures_only_edited_line_and_shifts_keys);
    RUN_TEST(geom_enter_splits_line_and_keeps_following_entries);
    RUN_TEST(geom_backspace_join_drops_both_lines_and_shifts_rest);
    RUN_TEST(geom_select_all_replace_stays_consistent);
    RUN_TEST(geom_external_mutation_revision_clears_store);
    RUN_TEST(geom_external_length_change_clears_store);
    RUN_TEST(geom_font_change_rebuilds_geometry);
    RUN_TEST(geom_wrap_toggle_clears_and_rebuilds_row_tables);
    RUN_TEST(geom_wrap_typing_touches_only_edited_line);
    RUN_TEST(geom_caret_roundtrip_warm_matches_analytic);
    RUN_TEST(geom_store_stays_bounded_under_full_document_scroll);
}
