// perf_editor.c - wlx_editor perf gate: frame cost must be flat with the
// document size (100 KB vs 10 MB / 1M lines), idle frames must run no
// O(document) work (index rebuild counter stays put), and scroll cost is
// O(viewport). Both modes are gated: no-wrap and wrapped, plus a wrapped
// pathological single-hard-line document whose per-line budget bounds the
// frame. Build: make perf-editor. Prints per-phase frame averages and
// exits non-zero when a structural bound is violated.
//
// A second, deterministic gate counts backend measure traffic (calls and
// bytes per frame) over prose-shaped and giant-single-line documents in
// both modes, per frame class (cold, idle, vertical scroll, horizontal
// scroll, typing, END on the giant line, and the giant-line envelope:
// wheel and steady cost at ~6,000 px vs ~600,000 px of horizontal depth).
// Traffic counts are exact and machine-independent, so their bounds are
// tight where the wall-clock bounds must stay generous. Each traffic
// workload also runs with every word a colour span through the editor's
// span-colour hook, against the same bounds: colouring places pieces
// from the stored advances and must cost no measure.

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_editor.h"
#include "test_mock_backend.h"

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

// Raw floor of the edit envelope: the unavoidable byte work of a keystroke
// at offset 0 - one full-buffer memmove plus one document newline scan -
// timed in this process right before the check that consumes it. An
// absolute wall-clock bound proved load- and machine-dependent (a busy
// machine trips it with no regression); the relative form scales with
// whatever slows this process down, while any added O(document) pass in
// the edit frame still inflates the frame by at least another scan and
// blows the band. Each rep is two shifts plus one scan, so dividing by
// 2*reps yields one shift plus half a scan - a deliberately low floor.
static double edit_floor_ms(char *buf, size_t cap, size_t len, int reps) {
    double t0 = now_ms();
    volatile size_t sink = 0;
    for (int r = 0; r < reps; r++) {
        memmove(buf + 1, buf, len);        // insert shift
        memmove(buf, buf + 1, len);        // undo shift (buffer restored)
        size_t nl = 0;
        for (size_t i = 0; i < len; i++) nl += (size_t)(buf[i] == '\n');
        sink += nl;
    }
    (void)sink; (void)cap;
    return (now_ms() - t0) / (double)(2 * reps);
}

// ~11 bytes per line, matching the demo generator's shape.
static size_t gen_doc(char *buf, size_t cap, size_t target_lines) {
    size_t off = 0;
    for (size_t i = 0; i < target_lines && off + 16 < cap; i++) {
        off += (size_t)snprintf(buf + off, cap - off, "%06zu abc", i + 1);
        buf[off++] = '\n';
    }
    buf[off] = '\0';
    return off;
}

// Every-word colouring for the highlighted passes: a span ends at the next
// whitespace transition from the offset, words alternate two colours by
// the parity of their start, whitespace draws in the base colour. O(span)
// per call and stateless, so a highlighted pass measures the drawer's
// cost and nothing of a tokenizer's.
static bool hl_is_ws(char c) { return c == ' ' || c == '\t'; }

static WLX_Color hl_span_color(const WLX_Text_Span_Query *q, size_t *span_end, void *user) {
    (void)user;
    bool ws = hl_is_ws(q->text[q->offset]);
    size_t p = q->offset;
    while (p < q->limit && hl_is_ws(q->text[p]) == ws) p++;
    *span_end = p;
    WLX_Color zero = {0};
    WLX_Color a = { 200, 80, 80, 255 };
    WLX_Color b = { 80, 200, 80, 255 };
    if (ws) return zero;
    return (q->offset & 2) ? a : b;
}

// Frames pass hl_span_color to the editor while set.
static bool g_highlight;

typedef struct {
    double first_frame_ms; // includes the initial index build
    double idle_avg_ms;
    double scroll_avg_ms;
    double edit_avg_ms;    // keystroke at offset 0: memmove + index rescan
    uint32_t rebuilds_after_idle;
} Perf_Result;

static void run_frame_input(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                            float wheel, bool clicked, const char *text, bool wrap) {
    test_frame_begin_full(ctx, 40, 40, clicked, clicked, clicked, wheel,
        NULL, NULL, NULL, 0, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    (void)wlx_editor_impl(ctx, NULL, buf, cap, len,
        wlx_default_editor_opt(.font_size = 16, .wrap = wrap,
            .span_color = g_highlight ? hl_span_color : NULL), "perf_editor", 1);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void run_frame(WLX_Context *ctx, char *buf, size_t cap, size_t *len, float wheel,
                      bool wrap) {
    run_frame_input(ctx, buf, cap, len, wheel, false, NULL, wrap);
}

static Perf_Result run_case(char *buf, size_t cap, size_t *len, int frames, bool wrap,
                            bool highlighted) {
    Perf_Result r = {0};
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    g_highlight = highlighted;

    double t0 = now_ms();
    run_frame(&ctx, buf, cap, len, 0.0f, wrap);
    r.first_frame_ms = now_ms() - t0;

    // Best-of-3 per timed phase: transient scheduler noise inflates a
    // run, never deflates it, so the minimum is the honest cost.
    r.idle_avg_ms = 1e9;
    r.scroll_avg_ms = 1e9;
    r.edit_avg_ms = 1e9;
    for (int rep = 0; rep < 3; rep++) {
        t0 = now_ms();
        for (int i = 0; i < frames; i++) run_frame(&ctx, buf, cap, len, 0.0f, wrap);
        double idle = (now_ms() - t0) / frames;
        if (idle < r.idle_avg_ms) r.idle_avg_ms = idle;

        t0 = now_ms();
        for (int i = 0; i < frames; i++) run_frame(&ctx, buf, cap, len, -1.0f, wrap);
        double scroll = (now_ms() - t0) / frames;
        if (scroll < r.scroll_avg_ms) r.scroll_avg_ms = scroll;
    }
    r.rebuilds_after_idle = ctx.editor_indices.count > 0
        ? ctx.editor_indices.items[0].rebuilds : 0;

    // Editing worst case: focus at the document start, then one typed
    // character per frame at offset 0 - every insert memmoves the whole
    // buffer and rescans the index.
    run_frame_input(&ctx, buf, cap, len, 0.0f, true, NULL, wrap);
    int edit_frames = frames / 3;
    for (int rep = 0; rep < 3; rep++) {
        t0 = now_ms();
        for (int i = 0; i < edit_frames; i++) {
            run_frame_input(&ctx, buf, cap, len, 0.0f, false, "x", wrap);
        }
        double edit = (now_ms() - t0) / edit_frames;
        if (edit < r.edit_avg_ms) r.edit_avg_ms = edit;
    }

    wlx_context_destroy(&ctx);
    g_highlight = false;
    return r;
}

// ============================================================================
// Measure-traffic gate: deterministic per-frame counting of backend text
// measurement. The counting measure keeps the mock model (char width =
// font_size * 0.5, height = font_size) so geometry matches the plain mock.
// ============================================================================

static unsigned long long g_tm_calls;
static unsigned long long g_tm_bytes;
static size_t g_tm_maxlen;

static void traffic_measure_slice(const char *text, size_t len, WLX_Text_Style style,
                                  float *out_w, float *out_h, void *user) {
    (void)user;
    (void)text;
    g_tm_calls++;
    g_tm_bytes += len;
    if (len > g_tm_maxlen) g_tm_maxlen = len;
    int fs = style.font_size > 0 ? style.font_size : 10;
    if (out_w) *out_w = (float)len * (float)fs * 0.5f;
    if (out_h) *out_h = (float)fs;
}

// Counting advances callback with the same per-byte model: installed for
// the second traffic pass so the batched path is gated exactly like the
// per-unit fallback (calls and bytes at the backend boundary).
static size_t traffic_measure_advances(const char *text, size_t len,
        WLX_Text_Style style, const size_t *unit_ends, size_t unit_count,
        float *out_advances, void *user) {
    (void)user;
    (void)text;
    g_tm_calls++;
    g_tm_bytes += len;
    if (len > g_tm_maxlen) g_tm_maxlen = len;
    int fs = style.font_size > 0 ? style.font_size : 10;
    for (size_t i = 0; i < unit_count; i++)
        out_advances[i] = (float)unit_ends[i] * (float)fs * 0.5f;
    return unit_count;
}

// Prose-shaped document, replicating the dashboard "Prose doc" generator:
// 24 paragraphs, each a single hard line of "N. " plus three sentences from
// a rotation of five, blank-line separated. ~11.5 KB.
static size_t gen_prose(char *buf, size_t cap) {
    static const char *sentences[] = {
        "Wollix wraps hard lines into band-wide rows on demand, so the scroll "
        "anchor is a line plus a row within it and nothing is measured beyond "
        "the viewport.",
        "The vertical thumb maps hard lines: exact when nothing wraps, a "
        "documented approximation elsewhere, and the track end always lands "
        "on the document's last row.",
        "Vertical motion, hit tests, and caret-follow work in visual rows "
        "with a row-relative sticky column, while HOME and END keep "
        "whole-line semantics.",
        "Resize the window and the rows reflow for free: no wrap geometry is "
        "stored anywhere, so there is nothing to invalidate.",
        "Each of these paragraphs is a single hard line in the buffer; the "
        "line-number gutter marks only its first row.",
    };
    enum { PROSE_SENTENCES = 5, PROSE_PARAGRAPHS = 24 };
    size_t off = 0;
    for (int p = 0; p < PROSE_PARAGRAPHS; p++) {
        int n = snprintf(buf + off, cap - off, "%d. ", p + 1);
        if (n <= 0 || off + (size_t)n >= cap) break;
        off += (size_t)n;
        for (int s = 0; s < 3; s++) {
            const char *sentence = sentences[(p + s) % PROSE_SENTENCES];
            size_t sentence_len = strlen(sentence);
            if (off + sentence_len + 4 >= cap) break;
            memcpy(buf + off, sentence, sentence_len);
            off += sentence_len;
            buf[off++] = ' ';
        }
        buf[off++] = '\n';
        buf[off++] = '\n';
    }
    buf[off] = '\0';
    return off;
}

// One 300 KB hard line with no newline, built from space-separated tokens so
// the unit walk sees word-shaped content rather than one repeated glyph.
static size_t gen_giant(char *buf, size_t cap) {
    size_t target = 300 * 1024;
    if (target + 1 > cap) target = cap - 1;
    size_t off = 0;
    unsigned n = 0;
    while (off + 12 <= target) {
        off += (size_t)snprintf(buf + off, cap - off, "token%04u ", n++ % 10000u);
    }
    while (off < target) buf[off++] = 'x';
    buf[off] = '\0';
    return off;
}

typedef struct {
    unsigned long long calls;
    unsigned long long bytes;
    size_t maxlen;
    unsigned long long cmds;  // commands the frame recorded (text pieces grow it)
} Traffic;

// One depth probe of the giant-line envelope: the view parked at a given
// scroll_x, then wheel-notch extension and steady frames measured there.
// Cost independence of the parking depth is the windowed-origin contract.
typedef struct {
    Traffic wheel;           // total over a short Shift+wheel burst
    Traffic steady;          // idle after the burst
    double steady_ms;        // wall clock per steady frame (counting mock)
} Traffic_Depth;

// One frame against the counting backend; counters reset per frame so the
// returned Traffic is exactly this frame's measurement work.
static Traffic traffic_frame(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                             bool wrap, bool click, float wheel, uint32_t mods,
                             int pressed_key, const char *text) {
    bool keys_down[WLX_KEY_COUNT] = {false};
    bool keys_pressed[WLX_KEY_COUNT] = {false};
    if (pressed_key >= 0) {
        keys_down[pressed_key] = true;
        keys_pressed[pressed_key] = true;
    }
    g_tm_calls = 0;
    g_tm_bytes = 0;
    g_tm_maxlen = 0;
    test_frame_begin_full(ctx, 40, 40, click, click, click, wheel,
        pressed_key >= 0 ? keys_down : NULL,
        pressed_key >= 0 ? keys_pressed : NULL,
        NULL, mods, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    (void)wlx_editor_impl(ctx, NULL, buf, cap, len,
        wlx_default_editor_opt(.font_size = 16, .wrap = wrap,
            .span_color = g_highlight ? hl_span_color : NULL),
        "perf_editor_traffic", 1);
    wlx_layout_end(ctx);
    unsigned long long cmds = ctx->arena.commands.count;
    test_frame_end(ctx);
    return (Traffic){ g_tm_calls, g_tm_bytes, g_tm_maxlen, cmds };
}

static float traffic_scroll_x(WLX_Context *ctx) {
    WLX_State st = wlx_get_state_impl(ctx, sizeof(WLX_Editor_State),
        "perf_editor_traffic", 1);
    return ((WLX_Editor_State *)st.data)->scroll_x;
}

static void traffic_set_scroll_x(WLX_Context *ctx, float scroll_x) {
    WLX_State st = wlx_get_state_impl(ctx, sizeof(WLX_Editor_State),
        "perf_editor_traffic", 1);
    ((WLX_Editor_State *)st.data)->scroll_x = scroll_x;
}

// Park the view at scroll_x, settle, then measure a wheel burst and the
// steady state that follows, plus steady wall clock over the counting
// mock. Two probes at different depths pin the envelope: neither counts
// nor wall clock may grow with the parking depth.
static Traffic_Depth traffic_depth_probe(WLX_Context *ctx, char *buf, size_t cap,
                                         size_t *len, float scroll_x) {
    Traffic_Depth d = {0};
    traffic_set_scroll_x(ctx, scroll_x);
    for (int i = 0; i < 4; i++) {
        traffic_frame(ctx, buf, cap, len, false, false, 0.0f, 0, -1, NULL);
    }
    for (int i = 0; i < 4; i++) {
        Traffic t = traffic_frame(ctx, buf, cap, len, false, false, -1.0f,
            WLX_MOD_SHIFT, -1, NULL);
        d.wheel.calls += t.calls;
        d.wheel.bytes += t.bytes;
    }
    for (int i = 0; i < 3; i++) {
        d.steady = traffic_frame(ctx, buf, cap, len, false, false, 0.0f, 0, -1, NULL);
    }
    double t0 = now_ms();
    for (int i = 0; i < 200; i++) {
        traffic_frame(ctx, buf, cap, len, false, false, 0.0f, 0, -1, NULL);
    }
    d.steady_ms = (now_ms() - t0) / 200.0;
    return d;
}

typedef struct {
    const char *name;
    Traffic cold;            // first frame ever (index build + first window)
    Traffic idle;            // steady state, no input
    Traffic vscroll;         // steady vertical wheel
    Traffic typing;          // steady one typed char per frame
    Traffic hscroll_sweep;   // total over the horizontal sweep (no-wrap only)
    Traffic hscroll_steady;  // idle after a horizontal sweep (no-wrap only)
    Traffic end_frame;       // END keypress frame (giant no-wrap only)
    Traffic idle_after_end;  // idle after END (giant no-wrap only)
    Traffic_Depth shallow;   // envelope probe at ~6,000 px (giant no-wrap)
    Traffic_Depth deep;      // envelope probe at ~600,000 px (giant no-wrap)
    float hscroll_x;         // scroll_x the sweep reached
    float end_scroll_x;      // scroll_x after END (the line's true end)
    bool has_hscroll;
    bool has_end;
} Traffic_Result;

static Traffic_Result run_traffic_case(const char *name, char *buf, size_t cap,
                                       size_t *len, bool wrap, bool with_end,
                                       bool advances, bool highlighted) {
    Traffic_Result tr = {0};
    tr.name = name;
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    ctx.backend.measure_text_slice = traffic_measure_slice;
    if (advances) ctx.backend.measure_text_advances = traffic_measure_advances;
    g_highlight = highlighted;

    tr.cold = traffic_frame(&ctx, buf, cap, len, wrap, false, 0.0f, 0, -1, NULL);
    for (int i = 0; i < 6; i++) {
        tr.idle = traffic_frame(&ctx, buf, cap, len, wrap, false, 0.0f, 0, -1, NULL);
    }
    for (int i = 0; i < 6; i++) {
        tr.vscroll = traffic_frame(&ctx, buf, cap, len, wrap, false, -1.0f, 0, -1, NULL);
    }
    // Focus with a click, then type one character per frame.
    traffic_frame(&ctx, buf, cap, len, wrap, true, 0.0f, 0, -1, NULL);
    for (int i = 0; i < 3; i++) {
        tr.typing = traffic_frame(&ctx, buf, cap, len, wrap, false, 0.0f, 0, -1, "x");
    }
    if (!wrap) {
        // Horizontal sweep: 30 Shift+wheel frames of 5 notches (100 px each),
        // then settle idle frames. The sweep total is the reach-extension
        // cost; the steady state after it models a user parked deep in a
        // long line.
        for (int i = 0; i < 30; i++) {
            Traffic t = traffic_frame(&ctx, buf, cap, len, wrap, false, -5.0f, WLX_MOD_SHIFT, -1, NULL);
            tr.hscroll_sweep.calls += t.calls;
            tr.hscroll_sweep.bytes += t.bytes;
        }
        for (int i = 0; i < 4; i++) {
            tr.hscroll_steady = traffic_frame(&ctx, buf, cap, len, wrap, false, 0.0f, 0, -1, NULL);
        }
        tr.hscroll_x = traffic_scroll_x(&ctx);
        tr.has_hscroll = true;
    }
    if (with_end && !wrap) {
        tr.end_frame = traffic_frame(&ctx, buf, cap, len, wrap, false, 0.0f, 0,
            WLX_KEY_END, NULL);
        for (int i = 0; i < 4; i++) {
            tr.idle_after_end = traffic_frame(&ctx, buf, cap, len, wrap, false, 0.0f, 0, -1, NULL);
        }
        tr.end_scroll_x = traffic_scroll_x(&ctx);
        tr.has_end = true;
        // Giant-line envelope: the same probe at two depths two orders of
        // magnitude apart (END above opened the full reach).
        tr.shallow = traffic_depth_probe(&ctx, buf, cap, len, 6000.0f);
        tr.deep = traffic_depth_probe(&ctx, buf, cap, len, 600000.0f);
    }
    wlx_context_destroy(&ctx);
    g_highlight = false;
    return tr;
}

static void traffic_print(const Traffic_Result *tr) {
    printf("%-14s cold %llu/%llu  idle %llu/%llu  vscroll %llu/%llu  typing %llu/%llu  idle-cmds %llu\n",
        tr->name,
        tr->cold.calls, tr->cold.bytes, tr->idle.calls, tr->idle.bytes,
        tr->vscroll.calls, tr->vscroll.bytes, tr->typing.calls, tr->typing.bytes,
        tr->idle.cmds);
    if (tr->has_hscroll) {
        printf("%-14s hscroll-sweep %llu/%llu  hscroll-steady %llu/%llu at scroll_x %.0f px\n", "",
            tr->hscroll_sweep.calls, tr->hscroll_sweep.bytes,
            tr->hscroll_steady.calls, tr->hscroll_steady.bytes, tr->hscroll_x);
    }
    if (tr->has_end) {
        printf("%-14s end-frame %llu/%llu  idle-after-end %llu/%llu at scroll_x %.0f px\n", "",
            tr->end_frame.calls, tr->end_frame.bytes,
            tr->idle_after_end.calls, tr->idle_after_end.bytes, tr->end_scroll_x);
        printf("%-14s envelope @6K px: wheel %llu/%llu steady %llu/%llu %.4fms"
               "  @600K px: wheel %llu/%llu steady %llu/%llu %.4fms\n", "",
            tr->shallow.wheel.calls, tr->shallow.wheel.bytes,
            tr->shallow.steady.calls, tr->shallow.steady.bytes, tr->shallow.steady_ms,
            tr->deep.wheel.calls, tr->deep.wheel.bytes,
            tr->deep.steady.calls, tr->deep.steady.bytes, tr->deep.steady_ms);
    }
}

// The giant-line envelope: frame cost at a parking depth two orders of
// magnitude deeper must match the shallow probe within the margin - both
// exact traffic and (generously banded) wall clock.
static int envelope_check(const char *doc, const Traffic_Result *tr) {
    int failures = 0;
    if (tr->deep.wheel.calls > tr->shallow.wheel.calls * 115 / 100 + 2
        || tr->deep.wheel.bytes > tr->shallow.wheel.bytes * 115 / 100 + 2) {
        fprintf(stderr,
            "FAIL: %s wheel traffic grows with depth: %llu/%llu @6K vs %llu/%llu @600K\n",
            doc, tr->shallow.wheel.calls, tr->shallow.wheel.bytes,
            tr->deep.wheel.calls, tr->deep.wheel.bytes);
        failures++;
    }
    if (tr->deep.steady.calls > tr->shallow.steady.calls + 1
        || tr->deep.steady.bytes > tr->shallow.steady.bytes + 1) {
        fprintf(stderr,
            "FAIL: %s steady traffic grows with depth: %llu/%llu @6K vs %llu/%llu @600K\n",
            doc, tr->shallow.steady.calls, tr->shallow.steady.bytes,
            tr->deep.steady.calls, tr->deep.steady.bytes);
        failures++;
    }
    if (tr->deep.steady_ms > tr->shallow.steady_ms * 3.0 + 0.05) {
        fprintf(stderr,
            "FAIL: %s steady frame wall clock grows with depth: %.4fms @6K vs %.4fms @600K\n",
            doc, tr->shallow.steady_ms, tr->deep.steady_ms);
        failures++;
    }
    // END must land the view at the line's true end (~2.4M px for the
    // 300 KB line at 8 px per byte), not at a reach cap.
    if (tr->end_scroll_x < 2000000.0f || tr->end_scroll_x > 3000000.0f) {
        fprintf(stderr, "FAIL: %s END parked at %.0f px, not the line's end\n",
            doc, tr->end_scroll_x);
        failures++;
    }
    return failures;
}

// Bound = recorded stage-0 baseline + ~15% margin; a bound of 0 skips the
// check (baseline capture mode).
static int traffic_check(const char *doc, const char *phase, Traffic t,
                         unsigned long long max_calls, unsigned long long max_bytes) {
    if (max_calls == 0 && max_bytes == 0) return 0;
    if (t.calls > max_calls || t.bytes > max_bytes) {
        fprintf(stderr,
            "FAIL: %s %s measure traffic %llu calls / %llu bytes exceeds bound %llu / %llu\n",
            doc, phase, t.calls, t.bytes, max_calls, max_bytes);
        return 1;
    }
    return 0;
}

// Bounds are the 2026-07-13 retained-geometry capture plus ~15% (stage 0
// baselines in parentheses). Steady frames replay retained line geometry
// and stop measuring: idle and post-sweep frames issue only the frame's
// reference measure; typing re-measures the edited line; vertical scroll
// the entering lines. Cold frames still measure everything once; a
// wrapped cold frame also counts the rows of the band of lines at the
// document end (the bottom anchor behind the thumb's range end), retained
// from then on. Giant-wrap steady frames carry the band-resolve overflow
// row probe, which deliberately measures outside the store at the
// pre-strip width - the recorded residual. The highlighted twins of each
// workload are checked against the same bounds: colour pieces are placed
// from the stored advances, so colouring costs no measure.
static int traffic_check_fallback(const Traffic_Result *nw, const Traffic_Result *w,
                                  const Traffic_Result *gnw, const Traffic_Result *gw) {
    int failures = 0;
    failures += traffic_check(nw->name, "cold", nw->cold, 4400, 422000);
    failures += traffic_check(nw->name, "idle", nw->idle, 2, 2);            // (3821/366721)
    failures += traffic_check(nw->name, "vscroll", nw->vscroll, 225, 21100); // (3821/366721)
    failures += traffic_check(nw->name, "typing", nw->typing, 12, 40);       // (3826/366733)
    failures += traffic_check(nw->name, "hscroll-steady", nw->hscroll_steady, 2, 2); // (8336/1748030)
    failures += traffic_check(w->name, "cold", w->cold, 6320, 296300);       // 2026-09-06: 5490/257589
    failures += traffic_check(w->name, "idle", w->idle, 2, 2);                 // (3459/162025)
    failures += traffic_check(w->name, "vscroll", w->vscroll, 2, 2);           // (3538/165937)
    failures += traffic_check(w->name, "typing", w->typing, 505, 23000);       // (3929/184867)
    failures += traffic_check(gnw->name, "cold", gnw->cold, 224, 21600);
    failures += traffic_check(gnw->name, "idle", gnw->idle, 2, 2);             // (194/18722)
    failures += traffic_check(gnw->name, "vscroll", gnw->vscroll, 2, 2);       // (194/18722)
    failures += traffic_check(gnw->name, "typing", gnw->typing, 224, 21600);
    failures += traffic_check(gnw->name, "hscroll-steady", gnw->hscroll_steady, 2, 2); // (570/161603)
    failures += traffic_check(gnw->name, "end-frame", gnw->end_frame, 600, 156000);    // (1027/526849)
    failures += traffic_check(gnw->name, "idle-after-end", gnw->idle_after_end, 2, 2); // (1026/525825)
    failures += traffic_check(gw->name, "cold", gw->cold, 2380, 114200);
    failures += traffic_check(gw->name, "idle", gw->idle, 1195, 57100);          // (3103/148831)
    failures += traffic_check(gw->name, "vscroll", gw->vscroll, 1195, 57100);    // (3103/148831)
    failures += traffic_check(gw->name, "typing", gw->typing, 2380, 114200);     // (3298/158343)
    failures += traffic_check(nw->name, "hscroll-sweep", nw->hscroll_sweep, 5300, 1589000);
    failures += traffic_check(gnw->name, "hscroll-sweep", gnw->hscroll_sweep, 466, 165000);
    return failures;
}

// Advances-pass bounds: the 2026-07-13 batched capture plus ~15%. Cold
// builds fill whole chunks per line (one call per tab-segment chunk plus
// the reference measure); steady frames match the fallback pass; typing
// and reach extension collapse to chunk counts; the giant-wrap
// band-resolve row probe walks O(rows) batched calls.
static int traffic_check_advances(const Traffic_Result *nw, const Traffic_Result *w,
                                  const Traffic_Result *gnw, const Traffic_Result *gw) {
    int failures = 0;
    failures += traffic_check(nw->name, "cold", nw->cold, 25, 5900);          // (21/5121)
    failures += traffic_check(nw->name, "idle", nw->idle, 2, 2);
    failures += traffic_check(nw->name, "vscroll", nw->vscroll, 3, 300);      // (2/257)
    failures += traffic_check(nw->name, "typing", nw->typing, 3, 5);          // (2/4)
    failures += traffic_check(nw->name, "hscroll-sweep", nw->hscroll_sweep, 58, 3800); // (50/3241)
    failures += traffic_check(nw->name, "hscroll-steady", nw->hscroll_steady, 2, 2);
    failures += traffic_check(w->name, "cold", w->cold, 75, 13600);         // 2026-09-06: 65/11813
    failures += traffic_check(w->name, "idle", w->idle, 2, 2);
    failures += traffic_check(w->name, "vscroll", w->vscroll, 2, 2);
    failures += traffic_check(w->name, "typing", w->typing, 7, 1100);        // (6/951)
    failures += traffic_check(gnw->name, "cold", gnw->cold, 3, 300);           // (2/257)
    failures += traffic_check(gnw->name, "idle", gnw->idle, 2, 2);
    failures += traffic_check(gnw->name, "vscroll", gnw->vscroll, 2, 2);
    failures += traffic_check(gnw->name, "typing", gnw->typing, 3, 300);       // (2/257)
    failures += traffic_check(gnw->name, "hscroll-sweep", gnw->hscroll_sweep, 37, 650); // (32/542)
    failures += traffic_check(gnw->name, "end-frame", gnw->end_frame, 5, 600); // (2/257)
    failures += traffic_check(gnw->name, "idle-after-end", gnw->idle_after_end, 2, 2);
    failures += traffic_check(gw->name, "cold", gw->cold, 27, 5900);          // (23/5057)
    failures += traffic_check(gw->name, "idle", gw->idle, 14, 2950);          // (12/2529)
    failures += traffic_check(gw->name, "vscroll", gw->vscroll, 14, 2950);
    failures += traffic_check(gw->name, "typing", gw->typing, 27, 5900);      // (23/5057)
    return failures;
}

int main(void) {
    const int frames = 300;
    int failures = 0;

    size_t small_lines = 10000;    // ~110 KB
    size_t large_lines = 1000000;  // ~11 MB / 1M lines
    size_t small_cap = small_lines * 12 + 64;
    size_t large_cap = large_lines * 12 + 64;

    char *small = (char *)malloc(small_cap);
    char *large = (char *)malloc(large_cap);
    if (small == NULL || large == NULL) { fprintf(stderr, "alloc failed\n"); return 1; }

    size_t small_len = gen_doc(small, small_cap, small_lines);
    size_t large_len = gen_doc(large, large_cap, large_lines);
    printf("small doc: %zu lines, %zu bytes\n", small_lines, small_len);
    printf("large doc: %zu lines, %zu bytes\n", large_lines, large_len);

    Perf_Result rs = run_case(small, small_cap, &small_len, frames, false, false);
    Perf_Result rl = run_case(large, large_cap, &large_len, frames, false, false);

    // Wrapped mode over the same documents, then a pathological document
    // that is one multi-megabyte hard line: the per-line unit budget must
    // bound its frame cost.
    small_len = gen_doc(small, small_cap, small_lines);
    large_len = gen_doc(large, large_cap, large_lines);
    Perf_Result ws = run_case(small, small_cap, &small_len, frames, true, false);
    Perf_Result wl = run_case(large, large_cap, &large_len, frames, true, false);

    size_t mega_len = large_cap - 65;
    memset(large, 'm', mega_len);
    large[mega_len] = '\0';
    Perf_Result wm = run_case(large, large_cap, &mega_len, frames, true, false);

    // Every word a colour span on the 100 KB document: the pieces come from
    // the stored advances, so the cost is the extra draw commands alone.
    // Printed beside the plain column, not gated.
    small_len = gen_doc(small, small_cap, small_lines);
    Perf_Result hs = run_case(small, small_cap, &small_len, frames, false, true);

    printf("\n%-22s %12s %12s %12s %12s %12s %12s\n", "",
        "100KB", "10MB/1Mln", "wrap 100KB", "wrap 10MB", "wrap 1line", "100KB hl");
    printf("%-22s %10.3fms %10.3fms %10.3fms %10.3fms %10.3fms %10.3fms  (includes index build)\n",
        "first frame", rs.first_frame_ms, rl.first_frame_ms,
        ws.first_frame_ms, wl.first_frame_ms, wm.first_frame_ms, hs.first_frame_ms);
    printf("%-22s %10.4fms %10.4fms %10.4fms %10.4fms %10.4fms %10.4fms\n", "idle frame avg",
        rs.idle_avg_ms, rl.idle_avg_ms, ws.idle_avg_ms, wl.idle_avg_ms, wm.idle_avg_ms,
        hs.idle_avg_ms);
    printf("%-22s %10.4fms %10.4fms %10.4fms %10.4fms %10.4fms %10.4fms\n", "scroll frame avg",
        rs.scroll_avg_ms, rl.scroll_avg_ms, ws.scroll_avg_ms, wl.scroll_avg_ms, wm.scroll_avg_ms,
        hs.scroll_avg_ms);
    printf("%-22s %10.4fms %10.4fms %10.4fms %10.4fms %10.4fms %10.4fms  (keystroke at offset 0)\n",
        "edit frame avg",
        rs.edit_avg_ms, rl.edit_avg_ms, ws.edit_avg_ms, wl.edit_avg_ms, wm.edit_avg_ms,
        hs.edit_avg_ms);
    printf("%-22s %11u %12u %12u %12u %12u %12u\n", "rebuilds after idle",
        rs.rebuilds_after_idle, rl.rebuilds_after_idle,
        ws.rebuilds_after_idle, wl.rebuilds_after_idle, wm.rebuilds_after_idle,
        hs.rebuilds_after_idle);

    // Structural gates. Idle frames must not rebuild the index (that is the
    // only O(document) step in the frame path), and steady-state frame cost
    // must be flat with document size (generous 3x band to stay robust on a
    // loaded machine; a hidden O(document) step would blow it by orders of
    // magnitude on the 100x larger document).
    if (rs.rebuilds_after_idle != 1 || rl.rebuilds_after_idle != 1
        || ws.rebuilds_after_idle != 1 || wl.rebuilds_after_idle != 1
        || wm.rebuilds_after_idle != 1) {
        fprintf(stderr, "FAIL: idle frames rebuilt the line index\n");
        failures++;
    }
    if (rl.idle_avg_ms > rs.idle_avg_ms * 3.0 + 0.05) {
        fprintf(stderr, "FAIL: idle frame cost grows with document size\n");
        failures++;
    }
    if (rl.scroll_avg_ms > rs.scroll_avg_ms * 3.0 + 0.05) {
        fprintf(stderr, "FAIL: scroll frame cost grows with document size\n");
        failures++;
    }
    if (wl.idle_avg_ms > ws.idle_avg_ms * 3.0 + 0.05) {
        fprintf(stderr, "FAIL: wrapped idle frame cost grows with document size\n");
        failures++;
    }
    if (wl.scroll_avg_ms > ws.scroll_avg_ms * 3.0 + 0.05) {
        fprintf(stderr, "FAIL: wrapped scroll frame cost grows with document size\n");
        failures++;
    }
    // The single-line document has no size peer; its bound is absolute:
    // the per-line budget keeps wrapped steady-state frames sub-frame-
    // budget even when the whole document is one hard line.
    if (wm.idle_avg_ms > 2.0 || wm.scroll_avg_ms > 2.0) {
        fprintf(stderr, "FAIL: wrapped mega-line frame cost exceeds budget bound\n");
        failures++;
    }
    // Envelope: a keystroke at offset 0 of the 10 MB document (full-buffer
    // memmove plus index rescan) may cost at most a small multiple of the
    // raw byte work measured in this same process. The multiplier leaves
    // room for the index rebuild's offset stores on top of the floor but
    // not for a second O(document) pass; a loose absolute backstop catches
    // a floor measurement gone wrong.
    double floor_ms = edit_floor_ms(large, large_cap, large_len, 4);
    double edit_bound_ms = floor_ms * 2.5 + 1.0;
    printf("edit floor (memmove + newline scan): %.4fms, bound %.4fms\n",
        floor_ms, edit_bound_ms);
    if (rl.edit_avg_ms > edit_bound_ms || wl.edit_avg_ms > edit_bound_ms) {
        fprintf(stderr,
            "FAIL: edit frame (%.4f / %.4fms) exceeds 2.5x the raw byte-work floor\n",
            rl.edit_avg_ms, wl.edit_avg_ms);
        failures++;
    }
    if (rl.edit_avg_ms > 50.0 || wl.edit_avg_ms > 50.0) {
        fprintf(stderr, "FAIL: edit frame exceeds the 50ms absolute backstop\n");
        failures++;
    }

    free(small);
    free(large);

    // ---- Measure-traffic gate ------------------------------------------
    // Deterministic counting over the pathological workloads; exact numbers,
    // so bounds sit at the recorded baseline plus ~15% and any measurement
    // regression trips them long before wall-clock notices.
    size_t prose_cap = 16 * 1024;
    size_t giant_cap = 300 * 1024 + 256;
    char *prose = (char *)malloc(prose_cap);
    char *giant = (char *)malloc(giant_cap);
    if (prose == NULL || giant == NULL) { fprintf(stderr, "alloc failed\n"); return 1; }

    size_t prose_len = gen_prose(prose, prose_cap);
    size_t giant_len = gen_giant(giant, giant_cap);
    printf("\nprose doc: %zu bytes; giant doc: %zu bytes (one hard line)\n",
        prose_len, giant_len);
    printf("measure traffic per frame (calls/bytes):\n");

    Traffic_Result tp_nw = run_traffic_case("prose no-wrap", prose, prose_cap, &prose_len, false, false, false, false);
    prose_len = gen_prose(prose, prose_cap);
    Traffic_Result tp_w = run_traffic_case("prose wrap", prose, prose_cap, &prose_len, true, false, false, false);
    Traffic_Result tg_nw = run_traffic_case("giant no-wrap", giant, giant_cap, &giant_len, false, true, false, false);
    giant_len = gen_giant(giant, giant_cap);
    Traffic_Result tg_w = run_traffic_case("giant wrap", giant, giant_cap, &giant_len, true, false, false, false);

    traffic_print(&tp_nw);
    traffic_print(&tp_w);
    traffic_print(&tg_nw);
    traffic_print(&tg_w);

    // Second pass with the counting advances callback installed: the same
    // workloads through the batched fills, gated separately. Both passes
    // stay asserted so neither path regresses unnoticed.
    printf("with measure_text_advances installed:\n");
    prose_len = gen_prose(prose, prose_cap);
    giant_len = gen_giant(giant, giant_cap);
    Traffic_Result ta_nw = run_traffic_case("prose nw +adv", prose, prose_cap, &prose_len, false, false, true, false);
    prose_len = gen_prose(prose, prose_cap);
    Traffic_Result ta_w = run_traffic_case("prose wr +adv", prose, prose_cap, &prose_len, true, false, true, false);
    Traffic_Result ta_gnw = run_traffic_case("giant nw +adv", giant, giant_cap, &giant_len, false, true, true, false);
    giant_len = gen_giant(giant, giant_cap);
    Traffic_Result ta_gw = run_traffic_case("giant wr +adv", giant, giant_cap, &giant_len, true, false, true, false);

    traffic_print(&ta_nw);
    traffic_print(&ta_w);
    traffic_print(&ta_gnw);
    traffic_print(&ta_gw);

    // Third and fourth passes: every word a colour span, on both
    // measurement paths. The bounds are the unhighlighted twins' - pieces
    // are placed from the stored advances, so colouring costs no measure;
    // only the command count (printed) grows with the pieces.
    printf("with every word a colour span (span_color):\n");
    prose_len = gen_prose(prose, prose_cap);
    giant_len = gen_giant(giant, giant_cap);
    Traffic_Result th_nw = run_traffic_case("prose nw hl", prose, prose_cap, &prose_len, false, false, false, true);
    prose_len = gen_prose(prose, prose_cap);
    Traffic_Result th_w = run_traffic_case("prose wr hl", prose, prose_cap, &prose_len, true, false, false, true);
    Traffic_Result th_gnw = run_traffic_case("giant nw hl", giant, giant_cap, &giant_len, false, true, false, true);
    giant_len = gen_giant(giant, giant_cap);
    Traffic_Result th_gw = run_traffic_case("giant wr hl", giant, giant_cap, &giant_len, true, false, false, true);

    traffic_print(&th_nw);
    traffic_print(&th_w);
    traffic_print(&th_gnw);
    traffic_print(&th_gw);

    printf("with every word a colour span and measure_text_advances installed:\n");
    prose_len = gen_prose(prose, prose_cap);
    giant_len = gen_giant(giant, giant_cap);
    Traffic_Result tha_nw = run_traffic_case("prose nw +adv hl", prose, prose_cap, &prose_len, false, false, true, true);
    prose_len = gen_prose(prose, prose_cap);
    Traffic_Result tha_w = run_traffic_case("prose wr +adv hl", prose, prose_cap, &prose_len, true, false, true, true);
    Traffic_Result tha_gnw = run_traffic_case("giant nw +adv hl", giant, giant_cap, &giant_len, false, true, true, true);
    giant_len = gen_giant(giant, giant_cap);
    Traffic_Result tha_gw = run_traffic_case("giant wr +adv hl", giant, giant_cap, &giant_len, true, false, true, true);

    traffic_print(&tha_nw);
    traffic_print(&tha_w);
    traffic_print(&tha_gnw);
    traffic_print(&tha_gw);

    failures += traffic_check_fallback(&tp_nw, &tp_w, &tg_nw, &tg_w);
    failures += traffic_check_advances(&ta_nw, &ta_w, &ta_gnw, &ta_gw);
    failures += traffic_check_fallback(&th_nw, &th_w, &th_gnw, &th_gw);
    failures += traffic_check_advances(&tha_nw, &tha_w, &tha_gnw, &tha_gw);

    // Giant-line envelope on both measurement paths, plain and coloured:
    // END reaches the line's true end and parked-depth cost is
    // scroll_x-independent.
    failures += envelope_check("giant no-wrap", &tg_nw);
    failures += envelope_check("giant nw +adv", &ta_gnw);
    failures += envelope_check("giant nw hl", &th_gnw);
    failures += envelope_check("giant nw +adv hl", &tha_gnw);

    // Reach extension through the callback must beat per-unit whole-prefix
    // measuring by an order of magnitude in bytes, not merely dent it.
    if (ta_nw.hscroll_sweep.bytes * 10 > tp_nw.hscroll_sweep.bytes) {
        fprintf(stderr, "FAIL: prose h-scroll sweep bytes did not drop 10x with advances\n");
        failures++;
    }
    if (ta_gnw.hscroll_sweep.bytes * 10 > tg_nw.hscroll_sweep.bytes) {
        fprintf(stderr, "FAIL: giant h-scroll sweep bytes did not drop 10x with advances\n");
        failures++;
    }
    if (tha_nw.hscroll_sweep.bytes * 10 > th_nw.hscroll_sweep.bytes) {
        fprintf(stderr, "FAIL: coloured prose h-scroll sweep bytes did not drop 10x with advances\n");
        failures++;
    }
    if (tha_gnw.hscroll_sweep.bytes * 10 > th_gnw.hscroll_sweep.bytes) {
        fprintf(stderr, "FAIL: coloured giant h-scroll sweep bytes did not drop 10x with advances\n");
        failures++;
    }

    free(prose);
    free(giant);
    printf("\n%s\n", failures == 0 ? "PERF GATE PASSED" : "PERF GATE FAILED");
    return failures == 0 ? 0 : 1;
}
