// Copyright (c) 2026 Dainis Berzins
// Licensed under the MIT License. See LICENSE file for full text.
//
// wollix_editor.h - wlx_editor: a windowed text editor widget over a
// caller-owned flat buffer with an explicit length in/out. Companion
// header to wollix.h: it is not part of the core and the core never
// includes it; include it AFTER wollix.h in every translation unit that
// uses the editor. The declarations below are always visible; the
// implementation expands under WOLLIX_IMPLEMENTATION in the same
// translation unit as the core implementation and calls core internals
// directly, like the backend adapter headers do. Exactly one TU defines
// WOLLIX_IMPLEMENTATION and includes the core, any backend adapter, and
// then this header:
//
//     #define WOLLIX_IMPLEMENTATION
//     #include "wollix.h"
//     #include "wollix_raylib.h"   // any backend adapter, if used
//     #include "wollix_editor.h"
//
// Every other TU includes wollix.h and this header for the declarations
// and links against wlx_editor_impl.
#ifndef WOLLIX_EDITOR_H_
#define WOLLIX_EDITOR_H_

#ifndef WOLLIX_H_
#error "Include wollix.h before wollix_editor.h"
#endif

// Persistent editor widget state. POD; the zero value is the default state.
typedef struct {
    WLX_Text_Edit_State caret;
    // Scroll anchor: index of the first (possibly partially) visible document
    // line, plus the fraction of that line already scrolled off the top. The
    // pair is resolution-independent: pixel scroll = (first_line + y_frac) *
    // line_h at whatever line height the current style measures.
    size_t first_line;
    float y_frac;
    // Horizontal scroll in pixels, and the widest measured line seen so far
    // (sticky across frames): the horizontal bar range is proportional to
    // this approximation rather than an exact document-wide maximum, which
    // would cost O(document) to know. On lines re-entered at a deep measure
    // origin the width is the origin's estimated-absolute x plus the
    // measured window - the same documented approximation class.
    float scroll_x;
    float max_line_w;
    // Horizontal scrollbar thumb drag gesture.
    bool dragging_hbar;
    float hb_drag_offset;
    // Thumb range frozen at drag start. The live range's reach floor
    // follows scroll_x - the drag's own output: a held thumb mapped
    // through the live range re-maps to a new scroll_x every frame and
    // the view creeps or oscillates near a long line's end.
    float hb_drag_content_w;
    // True while a visible line was width-truncated by the window build (its
    // true width is unknown): the horizontal scroll limit stays open one
    // band past the current reach. Only the limit follows this flag - the
    // thumb range never does, so the thumb cannot snap when the flag flips.
    // Budget-capped lines freeze instead.
    bool h_reach_open;
    // Right edge of the line-number gutter of the previous frame (0 = no
    // gutter): shapes the interaction rect so gutter presses never focus.
    float gutter_end_x;
    // Line-index guard snapshot: the index rebuilds when the document length
    // or the caller's revision changes (plus a sampled hard-line-start probe
    // as backstop for in-place mutations).
    bool index_seen;
    size_t guard_length;
    uint32_t guard_revision;
    // Wrapped mode: row of the anchor line scrolled to the window top
    // (0 outside wrap mode), clamped against the line's current row count
    // on every use - band width and font may change between frames. The
    // previous frame's mode detects wrap toggles: entering wrap resets
    // scroll_x, leaving zeroes first_row, both drop the sticky column
    // (its x is row-relative under wrap, line-relative otherwise).
    size_t first_row;
    bool last_wrap;
    // Wrapped bottom anchor: the (line, row, y_frac) that bottom-aligns
    // the document's last row against the band, cached across frames.
    // It ends the vertical thumb's range (its pseudo scroll) and lands
    // the window build's structural bottom clamp. Stored end-relative so
    // edits and line-count changes before the anchor line keep it valid
    // (the start byte shifts with them); an edit reaching the anchor
    // line, a measurement-environment change (band width, font, line
    // height), or a change in the band's row need invalidates it, and
    // the next use walks at most a band of hard lines backward from the
    // document end.
    bool bottom_valid;
    size_t bottom_need;           // band rows the anchor fills: ceil(band.h / line_h)
    size_t bottom_lines_from_end; // line_count - 1 - anchor line
    size_t bottom_row;            // anchor row within that line
    size_t bottom_rows;           // that line's row count
    float bottom_y_frac;
    size_t bottom_start_off;      // anchor line's start byte, edit-shifted
} WLX_Editor_State;

// Editor widget: a windowed text editor over a caller-owned flat buffer
// with an explicit length in/out. The widget renders and edits only the
// visible window, so frame cost is O(viewport) regardless of the document
// size; geometry rests on a context-owned per-id line index that is
// rebuilt by a newline scan when the document changes. Non-wrapping by
// default (one visual line per hard line, horizontal scrolling); .wrap
// breaks hard lines into band-wide rows instead.
typedef struct {
    // Placement
    WLX_LAYOUT_SLOT_FIELDS;

    // Sizing
    WLX_WIDGET_SIZING_FIELDS;
    WLX_CONTENT_PADDING_FIELDS;

    // State
    WLX_WIDGET_STATE_FIELDS;

    // Typography (text is always top-left; align places the optional
    // leading label)
    WLX_TEXT_TYPOGRAPHY_FIELDS;

    // Wrapped mode: hard lines break into rows at the band width; the
    // horizontal scrollbar disappears (nothing overflows sideways) and
    // the vertical thumb maps hard lines - a documented approximation,
    // since exact wrapped height would cost O(document) to measure.
    // Vertical motion and hit-tests work in visual rows.
    WLX_TEXT_WRAP_FIELDS;

    // Styles
    WLX_TEXT_COLOR_FIELDS;
    WLX_BORDER_FIELDS;
    WLX_Color border_focus_color;
    WLX_Color cursor_color;
    WLX_Color selection_color;   // {0} -> theme->input.selection

    // Optional out-param: receives this frame's focus state.
    bool *out_focused;

    // Read-only mode: focus, selection, and copy keep working; every
    // mutation is rejected.
    bool read_only;

    // Draw draggable scrollbars while the content overflows the band
    // (vertical: exact from the line count; horizontal: proportional to the
    // widest line seen so far).
    bool show_scrollbar;

    // Line-number gutter on the leading edge. Gutter presses never touch
    // caret, selection, or focus.
    bool line_numbers;

    // Tab-stop width in space-advance columns for rendering '\t'.
    int tab_columns;

    // External-mutation guard: bump after mutating the buffer outside the
    // widget (same length included); the widget rebuilds its line index when
    // the revision or the document length changes.
    uint32_t revision;

    // Explicit string ID (NULL = auto from call-site)
    const char *id;
} WLX_Editor_Opt;

#define wlx_default_editor_opt(...) \
    (WLX_Editor_Opt) { \
        /* Placement */ \
        WLX_LAYOUT_SLOT_DEFAULTS, \
        /* Sizing */ \
        WLX_WIDGET_SIZING_DEFAULTS, \
        WLX_CONTENT_PADDING_DEFAULTS, \
        /* State */ \
        WLX_WIDGET_STATE_DEFAULTS, \
        /* Typography */ \
        WLX_TEXT_TYPOGRAPHY_DEFAULTS, \
        WLX_TEXT_WRAP_DEFAULTS, \
        /* Styles */ \
        WLX_TEXT_COLOR_DEFAULTS, \
        WLX_BORDER_DEFAULTS, \
        .border_focus_color = {0}, \
        .cursor_color = {0}, \
        .selection_color = {0}, \
        .out_focused = NULL, \
        .read_only = false, \
        .show_scrollbar = true, \
        .line_numbers = false, \
        .tab_columns = 4, \
        .revision = 0, \
        __VA_ARGS__ \
    }

// buffer/length contract: *length is the authoritative document length in
// and out (the buffer need not be NUL-terminated); buffer_cap bounds every
// insert. The widget maintains a trailing NUL opportunistically when the
// capacity allows. Returns true when the text changed this frame.
WLXDEF bool wlx_editor_impl(WLX_Context *ctx, const char *label, char *buffer, size_t buffer_cap,
    size_t *length, WLX_Editor_Opt opt, const char *file, int line);

// The editor's defaults by value, exactly as wlx_default_editor_opt installs
// them (the core's wlx_*_opt_defaults family).
WLXDEF WLX_Editor_Opt wlx_editor_opt_defaults(void);
#define wlx_editor(ctx, label, buffer, buffer_cap, length, ...) \
    wlx_editor_impl((ctx), (label), (buffer), (buffer_cap), (length), \
        wlx_default_editor_opt(__VA_ARGS__), __FILE__, __LINE__)

#ifdef WOLLIX_IMPLEMENTATION

// No-wrap reach horizon, in band widths: the editor's scroll limit and
// build width extend to scroll_x plus this many bands, so measurement
// leads the view by one band past its right edge.
#define WLX_EDITOR_MEASURE_LOOKAHEAD_BANDS 2.0f

// ============================================================================
// Editor widget: windowed view over a caller-owned flat buffer
// ============================================================================

// Overscan line records built beyond the visible band so lines partially
// revealed at the bottom edge are already positioned.
#ifndef WLX_EDITOR_OVERSCAN_LINES
#define WLX_EDITOR_OVERSCAN_LINES 2
#endif

// Half-row tolerance for the wrapped row-boundary comparisons (bottom
// clamp, track-end drag, caret-follow): row geometry accumulates through
// float fractions, so exact comparisons would flicker at boundaries.
static const float WLX_EDITOR_ROW_EPSILON = 0.5f;
// Caret-follow far-jump slack in rows past the band: a caret within this
// many rows below the window still walks the row counts; anything farther
// re-anchors through the backward fill.
static const size_t WLX_EDITOR_FAR_JUMP_SLACK_ROWS = 2;
// Fraction of the last row a track-end drag anchors past, so the window
// build's structural bottom clamp lands the exact bottom anchor.
static const float WLX_EDITOR_BOTTOM_ANCHOR_ROW_FRAC = 0.9f;

// Per-frame text-band geometry: the band derivation from the field
// interior, the line-number gutter strip, the scrollbar strips, and the
// tracks, resolved in one deterministic pass.
typedef struct {
    WLX_Rect band;        // text band after the gutter and scrollbar strips
    WLX_Rect gutter_rect; // gutter strip (zero when absent); height = band.h
    float    gutter_w;
    float    gutter_pad;
    float    sb_w;
    WLX_Rect sb_track;
    WLX_Rect hb_track;
    bool     sb_visible;
    bool     hb_visible;
    bool     wrap_overflow;
    // Band height before the horizontal strip: the window build sizes
    // itself from it (see the comment at the strip decision).
    float    win_band_h;
} WLX_Editor_Band;

// The editor frame: the read-mostly per-frame values every downstream
// phase consumes, built once the edit, index-rebuild, and band-resolve
// stages have fixed them (edits mutate the buffer before any geometry,
// the index rebuild follows the edit, the band needs the rebuilt index,
// and the wrap inputs need the final band width - so the frame cannot
// exist earlier; the prologue keeps its explicit staged dataflow).
// Field writers after construction: content_h is finalized by the
// scroll-limits phase (wrapped: the bottom anchor's pseudo scroll plus
// the band); state and
// wrap_inputs are pointees the phases mutate under their own rules -
// draw phases must treat both as read-only. Everything else is frozen.
typedef struct WLX_Editor_Frame {
    WLX_Context *ctx;
    const WLX_Editor_Opt *opt;
    WLX_Editor_State *state;
    const WLX_Editor_Line_Index *idx;
    WLX_Text_Geom_Store *geom;
    const char *doc;
    size_t doc_len;
    WLX_Text_Style ts;
    float line_h;
    float tab_advance;
    WLX_Rect input_rect;
    WLX_Interaction inter;
    WLX_Editor_Band eb;
    WLX_Text_Build_Inputs *wrap_inputs; // frame-stack wrap inputs; meaningful with wrap_geom
    bool wrap_geom;
    size_t line_count;
    float content_h;
} WLX_Editor_Frame;

// The frame's scroll transition: derived by the scroll-limits phase,
// mutated by the wheel and thumb gestures, mouse auto-scroll, linear
// caret-follow, and the wrapped build's bottom clamp. scroll_y is a
// derived view of the anchor: every gesture that moves it decomposes
// back into the anchor at its own site (wlx_editor_anchor_set_scroll_y),
// so the anchor stays the one authoritative scroll state. Passed
// explicitly beside the frame - never inside it - so each phase's
// writer set stays visible in its signature. anchor_rows is consumed
// by the derive and the wheel; it is not refreshed by later anchor
// writers and must not be read after the scroll-resolve phase.
typedef struct WLX_Editor_Scroll {
    float scroll_y;        // frame-local pixel scroll (pseudo-pixels wrapped)
    float max_scroll_y;
    float max_scroll_x;
    float h_content_w;     // horizontal thumb mapping range
    size_t anchor_rows;    // the anchor line's current row count (wrapped)
} WLX_Editor_Scroll;

// Find or create the line index for a widget id. Returns NULL only on
// allocation failure.
static WLX_Editor_Line_Index *wlx_editor_index_get(WLX_Context *ctx, size_t id) {
    WLX_Editor_Line_Index_Cache *cache = &ctx->editor_indices;
    for (size_t i = 0; i < cache->count; i++) {
        if (cache->items[i].id == id) return &cache->items[i];
    }
    if (cache->count == cache->capacity) {
        size_t new_cap = cache->capacity == 0 ? 4 : cache->capacity * 2;
        WLX_Editor_Line_Index *grown = (WLX_Editor_Line_Index *)wlx_realloc(
            cache->items, new_cap * sizeof(WLX_Editor_Line_Index));
        if (grown == NULL) return NULL;
        cache->items = grown;
        cache->capacity = new_cap;
    }
    WLX_Editor_Line_Index *idx = &cache->items[cache->count++];
    wlx_zero_struct(*idx);
    idx->id = id;
    return idx;
}

// Append one hard-line-start offset to the index, growing the array
// geometrically. Returns false on allocation failure (the caller
// abandons the rebuild).
static bool wlx_editor_index_push(WLX_Editor_Line_Index *idx, size_t offset) {
    if (idx->count == idx->cap) {
        size_t new_cap = idx->cap == 0 ? 64 : idx->cap * 2;
        size_t *grown = (size_t *)wlx_realloc(idx->offsets, new_cap * sizeof(size_t));
        if (grown == NULL) return false;
        idx->offsets = grown;
        idx->cap = new_cap;
    }
    idx->offsets[idx->count++] = offset;
    return true;
}

// Word-at-a-time scan to the next separator candidate byte ('\n' or '\r' -
// exactly the bytes the shared newline predicate recognizes). Returns NULL
// when the range has none. Byte-order independent: each XOR-fold tests
// every byte lane for zero symmetrically, so the hit mask needs no
// endian-specific extraction (the per-byte re-scan finds the position).
static inline const char *wlx_editor_scan_newline(const char *p, const char *end) {
    while (p + 8 <= end) {
        uint64_t w;
        memcpy(&w, p, 8);
        uint64_t lf = w ^ 0x0A0A0A0A0A0A0A0AULL;
        uint64_t cr = w ^ 0x0D0D0D0D0D0D0D0DULL;
        uint64_t hit = ((lf - 0x0101010101010101ULL) & ~lf & 0x8080808080808080ULL)
                     | ((cr - 0x0101010101010101ULL) & ~cr & 0x8080808080808080ULL);
        if (hit != 0) {
            for (int i = 0; i < 8; i++) {
                if (p[i] == '\n' || p[i] == '\r') return p + i;
            }
        }
        p += 8;
    }
    while (p < end) {
        if (*p == '\n' || *p == '\r') return p;
        p++;
    }
    return NULL;
}

// Rebuild the hard-line-start index with one newline scan. The word scan
// only locates candidate bytes; wlx_text_newline_at resolves the separator
// itself (CRLF is one separator), so the scan and the line build can never
// disagree about where lines start.
static bool wlx_editor_index_rebuild(WLX_Editor_Line_Index *idx, const char *text, size_t length) {
    idx->count = 0;
    idx->rebuilds++;
    if (!wlx_editor_index_push(idx, 0)) return false;
    if (text == NULL || length == 0) return true;

    const char *end = text + length;
    const char *p = text;
    while (p < end) {
        const char *sep = wlx_editor_scan_newline(p, end);
        if (sep == NULL) break;

        size_t sep_end = 0;
        wlx_text_newline_at(text, length, (size_t)(sep - text), &sep_end);
        if (idx->count < idx->cap) {
            idx->offsets[idx->count++] = sep_end;
        } else if (!wlx_editor_index_push(idx, sep_end)) {
            return false;
        }
        p = text + sep_end;
    }
    return true;
}

// A window record leaves the horizontal reach open when bytes exist between
// its visible end and its separator (or end-of-text): the line continues
// past the build width and its true width is unknown until scrolled to.
// The unit budget does not close the reach - a budget-capped record
// re-enters at a deeper measure origin as the view approaches, so every
// byte of the line stays reachable.
static bool wlx_editor_record_reach_open(const WLX_Text_Line_Record *line) {
    size_t tail_end = line->ended_by_newline ? line->separator_start : line->source_end;
    return tail_end > line->visible_end;
}

// End of a line's text before its newline separator (the caret's END target).
static size_t wlx_editor_line_text_end(const char *doc, size_t len,
    const WLX_Editor_Line_Index *idx, size_t line) {
    if (line + 1 >= idx->count) return len;
    size_t next = idx->offsets[line + 1];
    size_t sep_start = 0;
    if (wlx_text_separator_before(doc, len, next, &sep_start)) return sep_start;
    return next;
}

// Caret x in line-content space (pixels from the line start, tab
// stops applied) for one document line. A no-wrap retained entry
// answers through wlx_text_geom_x_at: anchor true is the anchored-
// consumer mode (motion, caret-follow - may move the measure origin
// to reach any byte, on the origin's estimated-absolute x where the
// line start is unmeasurably far), anchor false the passive mode (the
// per-frame caret draw - never moves a settled origin; 0 behind it
// and the coverage edge past it cull against the band). Without a
// store, or when the entry cannot answer, the fallback measures a
// budget-bounded prefix from the line start. The full policy is the
// decision table at wlx_text_geom_x_at.
static float wlx_editor_caret_x(WLX_Context *ctx, const char *doc, size_t len, WLX_Text_Style ts,
    float tab_advance, WLX_Text_Geom_Store *geom, size_t line_next,
    size_t line_start, size_t caret, bool anchor) {
    if (caret <= line_start) return 0.0f;
    if (geom != NULL && !geom->env.wrap && line_next > line_start && line_next <= len) {
        WLX_Text_Geom_Entry *e = wlx_text_geom_acquire(geom, line_start, line_next);
        if (e != NULL) {
            WLX_Text_Measure_Args margs = { ctx, doc, len, ts, tab_advance,
                geom->env.line_h };
            float x = 0.0f;
            if (wlx_text_geom_x_at(&margs, geom, e, caret - line_start, anchor, &x))
                return x;
        }
    }
    size_t end = line_start;
    size_t units = 0;
    while (end < caret && units < (size_t)WLX_EDITOR_MAX_LINE_UNITS) {
        size_t next = wlx_text_unit_next(doc, len, end);
        if (next > caret) next = caret;
        end = next;
        units++;
    }
    float w = 0.0f, h = 0.0f;
    if (!wlx_text_measure_prefix_tabs(ctx, doc, len, line_start, end, ts, tab_advance, &w, &h)) w = 0.0f;
    return w;
}

// Nearest caret offset at a content-space x within one line record
// (x measured from the record start, tab stops applied), by the
// midpoint rule. A retained entry answers through
// wlx_text_geom_offset_at_x over stored advances - the record's start
// (line start unwrapped, row start wrapped) matches the entry's
// advance frame by construction; any boundary miss falls back to the
// per-unit measuring walk below.
static size_t wlx_editor_offset_in_record(WLX_Context *ctx, const char *text, size_t length,
    WLX_Text_Style ts, float tab_advance, WLX_Text_Geom_Store *geom, size_t line_next,
    const WLX_Text_Line_Record *line, float content_x)
{
    if (text == NULL || length == 0) return 0;
    if (line->empty_visual) return line->cursor_start;

    if (geom != NULL && line_next > line->visible_start && line_next <= length) {
        WLX_Text_Geom_Entry *e = wlx_text_geom_find_containing(geom,
            line->visible_start, line_next);
        if (e != NULL) {
            size_t off = 0;
            if (wlx_text_geom_offset_at_x(e, line->visible_start,
                    line->visible_end, content_x, &off))
                return off;
        }
    }

    size_t off = line->visible_start;
    float prev_w = 0.0f;
    while (off < line->visible_end) {
        size_t next = wlx_text_unit_next(text, length, off);
        if (next > line->visible_end) next = line->visible_end;
        float w = 0.0f, h = 0.0f;
        if (!wlx_text_measure_prefix_tabs(ctx, text, length, line->visible_start, next, ts, tab_advance, &w, &h)) break;
        if (content_x < (prev_w + w) * 0.5f) return off;
        prev_w = w;
        off = next;
    }
    return off;
}

// Nearest caret offset at a content-space x on one document line, resolved
// through a single on-demand line record (window-independent, so motion and
// hit-tests work on lines outside the built window too). view_w > 0 lets
// the record re-enter a budget-deep line at an origin near the target x
// (a pseudo-view centered on it - a target inside the built window then
// resolves through the window's own settled origin); the hit x maps into
// the record's origin-relative frame before the midpoint walk.
static size_t wlx_editor_offset_at_x(WLX_Context *ctx, const char *doc, size_t len, WLX_Text_Style ts,
    float line_h, float tab_advance, const WLX_Editor_Line_Index *idx,
    WLX_Text_Geom_Store *geom, size_t line,
    float virtual_w, float view_w, float content_x) {
    if (idx == NULL || idx->count == 0) return 0;
    if (line >= idx->count) line = idx->count - 1;
    if (geom != NULL && geom->env.wrap) geom = NULL; // linear lookups only

    float view_x = view_w > 0.0f
        ? content_x - view_w * WLX_TEXT_GEOM_ORIGIN_VIEW_SLACK : 0.0f;
    if (view_x < 0.0f) view_x = 0.0f;
    if (virtual_w < view_x + view_w) virtual_w = view_x + view_w;

    size_t line_next = wlx_editor_line_next(idx, line, len);
    WLX_Text_Line_Record rec;
    WLX_Text_Build_Inputs inputs = {
        .ctx = ctx,
        .text = doc,
        .length = len,
        .style = ts,
        .rect = { 0, 0, virtual_w, line_h },
        .wrap = false,
        .line_h = line_h,
        .text_unit_cap = WLX_EDITOR_MAX_LINE_UNITS,
        .truncate_continue = true,
        .tab_advance = tab_advance,
        .known_line_next = line_next,
        .geom = geom,
        .view_x = view_x,
        .view_w = view_w,
    };
    WLX_Text_Build_Cursor cursor = { .text_unit_count = 0 };
    size_t n = wlx_text_build_lines_from(&inputs, &cursor, idx->offsets[line], &rec, 1);
    if (n == 0) return idx->offsets[line];
    if (geom != NULL && rec.visible_start > rec.source_start) {
        WLX_Text_Geom_Entry *e = wlx_text_geom_find(geom, rec.source_start, line_next);
        if (e != NULL && rec.visible_start == wlx_text_geom_origin_abs(e)) {
            content_x -= e->origin_x;
            if (content_x < 0.0f) content_x = 0.0f;
        }
    }
    return wlx_editor_offset_in_record(ctx, doc, len, ts, tab_advance, geom, line_next,
        &rec, content_x);
}

// Build inputs for wrapped editor geometry: band-width rows, per-line unit
// budget, tab stops. One constructor keeps every consumer (window build,
// row counting, anchor stepping, caret geometry) wrapping identically.
static WLX_Text_Build_Inputs wlx_editor_wrap_build_inputs(WLX_Context *ctx, const char *doc,
    size_t len, WLX_Text_Style ts, float band_w, float line_h, float tab_advance)
{
    return (WLX_Text_Build_Inputs){
        .ctx = ctx,
        .text = doc,
        .length = len,
        .style = ts,
        .rect = { 0, 0, band_w > 1.0f ? band_w : 1.0f, 0 },
        .wrap = true,
        .line_h = line_h,
        .text_unit_cap = WLX_EDITOR_MAX_LINE_UNITS,
        .truncate_continue = true,
        .tab_advance = tab_advance,
    };
}

// Start of the hard line after line (the text length for the last line):
// the tail-skip hint that keeps every per-line wrap walk free of newline
// scans.
static size_t wlx_editor_wrap_line_next(const WLX_Text_Build_Inputs *inputs,
    const WLX_Editor_Line_Index *idx, size_t line)
{
    return wlx_editor_line_next(idx, line, inputs->length);
}

// Streaming row iterator over one hard line: init at the line's start
// with its tail-skip hint, then next() yields one row record per call.
// After the line's last row - the row ended by its newline, made no
// forward progress, or reached the text end - `done` is set and further
// calls return false, so "stop on the last row" is the explicit
// `it.done` test at the call sites instead of three hand-copied
// stop-condition lists.
typedef struct WLX_Editor_Row_Iter {
    WLX_Text_Build_Inputs in;
    WLX_Text_Build_Cursor cursor;
    size_t off;
    bool done;
} WLX_Editor_Row_Iter;

static void wlx_editor_row_iter_init(WLX_Editor_Row_Iter *it,
    const WLX_Text_Build_Inputs *inputs, size_t line_start, size_t line_next)
{
    it->in = *inputs;
    it->in.known_line_next = line_next;
    it->cursor = (WLX_Text_Build_Cursor){ .text_unit_count = 0 };
    it->off = line_start;
    it->done = line_start >= inputs->length;
}

static bool wlx_editor_row_iter_next(WLX_Editor_Row_Iter *it,
    WLX_Text_Line_Record *out)
{
    if (it->done) return false;
    WLX_Text_Build_Step step = wlx_text_build_step(&it->in, &it->cursor,
        it->off, false);
    if (!step.produced) { it->done = true; return false; }
    *out = step.line;
    if (step.line.ended_by_newline || step.next_offset <= it->off
        || step.next_offset >= it->in.length) it->done = true;
    it->off = step.next_offset;
    return true;
}

// Wrapped row count of one hard line (>= 1: empty and trailing lines are
// one row). Streams build steps with a single stack record; cost is
// bounded by the per-line unit budget.
static size_t wlx_editor_wrap_row_count(const WLX_Text_Build_Inputs *inputs,
    size_t line_start, size_t line_next)
{
    WLX_Editor_Row_Iter it;
    wlx_editor_row_iter_init(&it, inputs, line_start, line_next);
    WLX_Text_Line_Record rec;
    size_t rows = 0;
    while (wlx_editor_row_iter_next(&it, &rec)) rows++;
    return rows > 0 ? rows : 1;
}

// Row count of one hard line, memoized per frame: a single wrapped frame
// asks for the anchor line at up to five sites plus a viewport of lines in
// caret-follow, and each recount streams the line's bytes. The memo is
// safe because the band width, style, and document are all fixed for the
// frame by the time it exists (see WLX_Wrap_Row_Memo).
static size_t wlx_editor_wrap_line_rows(const WLX_Text_Build_Inputs *inputs,
    const WLX_Editor_Line_Index *idx, size_t line)
{
    WLX_Wrap_Row_Memo *memo = inputs->row_memo;
    size_t slot = line & (WLX_WRAP_ROW_MEMO_SLOTS - 1);
    if (memo != NULL && memo->rows[slot] != 0 && memo->line[slot] == line) {
#ifdef WLX_DEBUG
        assert(memo->rows[slot] == wlx_editor_wrap_row_count(inputs,
            idx->offsets[line], wlx_editor_wrap_line_next(inputs, idx, line))
            && "wrap row memo out of sync");
#endif
        return memo->rows[slot];
    }
    // A complete retained entry knows its row table; answering from it
    // skips the streaming recount entirely. Find-only: a miss keeps the
    // streaming path below, which populates the store as before, and a
    // partial (budget-frozen) entry is complete with the same row count
    // the stream produces - both stop at WLX_EDITOR_MAX_LINE_UNITS.
    // Empty lines are rows == 0 entries and fall through (the stream
    // counts them as one row).
    if (inputs->geom != NULL) {
        size_t known = wlx_text_geom_rows_of(inputs->geom,
            idx->offsets[line], wlx_editor_wrap_line_next(inputs, idx, line));
        if (known > 0) {
#ifdef WLX_DEBUG
            assert(known == wlx_editor_wrap_row_count(inputs,
                idx->offsets[line], wlx_editor_wrap_line_next(inputs, idx, line))
                && "geom row count out of sync with streaming count");
#endif
            if (memo != NULL) { memo->line[slot] = line; memo->rows[slot] = known; }
            return known;
        }
    }
    size_t rows = wlx_editor_wrap_row_count(inputs, idx->offsets[line],
        wlx_editor_wrap_line_next(inputs, idx, line));
    if (memo != NULL) { memo->line[slot] = line; memo->rows[slot] = rows; }
    return rows;
}

// Move the wrap anchor by a signed number of visual rows, stopping at the
// first and last document rows. Returns the rows left undone (negative:
// hit the top, positive: hit the last row).
static long wlx_editor_wrap_anchor_step(const WLX_Text_Build_Inputs *inputs,
    const WLX_Editor_Line_Index *idx, size_t *first_line, size_t *first_row, long rows)
{
    while (rows > 0) {
        size_t count = wlx_editor_wrap_line_rows(inputs, idx, *first_line);
        if (*first_row + 1 < count) (*first_row)++;
        else if (*first_line + 1 < idx->count) { (*first_line)++; *first_row = 0; }
        else break;
        rows--;
    }
    while (rows < 0) {
        if (*first_row > 0) (*first_row)--;
        else if (*first_line > 0) {
            (*first_line)--;
            *first_row = wlx_editor_wrap_line_rows(inputs, idx, *first_line) - 1;
        } else break;
        rows++;
    }
    return rows;
}

// Anchor (line, row) placing rows_needed rows between it and the end row
// inclusive, walking hard lines backward and summing row counts - at most
// rows_needed lines, each budget-bounded. Returns the rows actually
// available when the document holds fewer than rows_needed.
static size_t wlx_editor_wrap_fill_backward(const WLX_Text_Build_Inputs *inputs,
    const WLX_Editor_Line_Index *idx, size_t end_line, size_t end_row,
    size_t rows_needed, size_t *out_line, size_t *out_row)
{
    size_t have = end_row + 1;
    size_t line = end_line;
    while (have < rows_needed && line > 0) {
        line--;
        have += wlx_editor_wrap_line_rows(inputs, idx, line);
    }
    *out_line = line;
    *out_row = have >= rows_needed ? have - rows_needed : 0;
    return have < rows_needed ? have : rows_needed;
}

// True when the caret offset belongs to this wrapped row record. An
// offset exactly at a wrap break belongs to the row it STARTS:
//
//   row k    [ a  b  c  d )
//   row k+1               [ e  f  g  h )
//                         ^
//                         the break offset opens row k+1
//
// With the opposite affinity, vertical motion aiming at column zero
// could never cross a break. Rows that end their hard line keep their
// separator (and any frozen-tail) coverage, so line-end and pinned
// carets stay addressable.
static bool wlx_editor_wrap_caret_on_row(const WLX_Text_Line_Record *rec,
    size_t caret, size_t doc_len)
{
    if (caret < rec->source_start) return false;
    if (rec->ended_by_newline) return caret < rec->separator_end;
    if (rec->source_end >= doc_len) return caret <= rec->source_end;
    return caret < rec->visible_end;
}

// Caret row within one hard line: the first streamed row that owns the
// caret under the break-belongs-to-the-next-row convention above; a caret
// inside a frozen tail pins to the covering row. Leaves the row's record
// in *out_rec.
static size_t wlx_editor_wrap_caret_row(const WLX_Text_Build_Inputs *inputs,
    const WLX_Editor_Line_Index *idx, size_t line, size_t caret,
    WLX_Text_Line_Record *out_rec)
{
    size_t line_start = idx->offsets[line];
    size_t line_next = wlx_editor_wrap_line_next(inputs, idx, line);
    wlx_zero_struct(*out_rec);
    if (line_start >= inputs->length) {
        // Trailing empty line: one empty row at the document end.
        WLX_Text_Build_Inputs in = *inputs;
        in.known_line_next = line_next;
        WLX_Text_Build_Cursor cursor = { .text_unit_count = 0 };
        WLX_Text_Build_Step step = wlx_text_build_step(&in, &cursor, line_start, true);
        *out_rec = step.line;
        return 0;
    }
    WLX_Editor_Row_Iter it;
    wlx_editor_row_iter_init(&it, inputs, line_start, line_next);
    size_t row = 0;
    while (wlx_editor_row_iter_next(&it, out_rec)) {
        if (wlx_editor_wrap_caret_on_row(out_rec, caret, inputs->length)) break;
        if (it.done) break; // the line's last row keeps the caret
        row++;
    }
    return row;
}

// Row-relative caret x: tab-aware prefix measure from the row start,
// pinned to the row's visible end (a caret inside a frozen tail keeps the
// frozen edge x). line_next keys the retained-entry lookup; 0 (or a caret
// off any measured unit boundary) measures directly.
static float wlx_editor_wrap_caret_x(const WLX_Text_Build_Inputs *inputs,
    const WLX_Text_Line_Record *rec, size_t line_next, size_t caret)
{
    size_t cx_end = caret < rec->visible_end ? caret : rec->visible_end;
    if (cx_end <= rec->visible_start) return 0.0f;
    if (inputs->geom != NULL && line_next > rec->visible_start
        && line_next <= inputs->length) {
        WLX_Text_Geom_Entry *e = wlx_text_geom_find_containing(inputs->geom,
            rec->visible_start, line_next);
        if (e != NULL) {
            float adv = 0.0f;
            if (wlx_text_geom_advance_at(e, cx_end - e->line_start, &adv))
                return adv;
        }
    }
    float w = 0.0f, h = 0.0f;
    if (!wlx_text_measure_prefix_tabs(inputs->ctx, inputs->text, inputs->length,
            rec->visible_start, cx_end, inputs->style, inputs->tab_advance, &w, &h)) {
        w = 0.0f;
    }
    return w;
}

// Nearest caret offset at a row-relative x on one row of a hard line,
// resolved through on-demand row records (window-independent, so motion
// and hit-tests work on rows outside the built window too). A row past
// the line's last row clamps to the last row.
static size_t wlx_editor_wrap_offset_at_row_x(const WLX_Text_Build_Inputs *inputs,
    const WLX_Editor_Line_Index *idx, size_t line, size_t row, float content_x)
{
    if (idx->count == 0) return 0;
    if (line >= idx->count) line = idx->count - 1;
    size_t line_start = idx->offsets[line];
    if (line_start >= inputs->length) return inputs->length;
    size_t line_next = wlx_editor_wrap_line_next(inputs, idx, line);
    WLX_Text_Line_Record rec;
    wlx_zero_struct(rec);
    WLX_Editor_Row_Iter it;
    wlx_editor_row_iter_init(&it, inputs, line_start, line_next);
    size_t r = 0;
    while (wlx_editor_row_iter_next(&it, &rec)) {
        if (r == row || it.done) break; // target row, or clamp to the last
        r++;
    }
    return wlx_editor_offset_in_record(inputs->ctx, inputs->text, inputs->length,
        inputs->style, inputs->tab_advance, inputs->geom, line_next,
        &rec, content_x);
}

// Window-relative wrapped hit: the pointer y is a row step down from the
// anchor (clamped to the document's last row by the stepper), the pointer
// x a column on that row's record.
static size_t wlx_editor_wrap_hit(const WLX_Text_Build_Inputs *inputs,
    const WLX_Editor_Line_Index *idx, size_t first_line, size_t first_row,
    float y_frac, WLX_Rect band, float line_h, float mx, float hit_y)
{
    long wrow = line_h > 0.0f
        ? (long)((hit_y - band.y + y_frac * line_h) / line_h) : 0;
    if (wrow < 0) wrow = 0;
    size_t hl = first_line, hr = first_row;
    wlx_editor_wrap_anchor_step(inputs, idx, &hl, &hr, wrow);
    float content_x = mx - band.x;
    if (content_x < 0.0f) content_x = 0.0f;
    return wlx_editor_wrap_offset_at_row_x(inputs, idx, hl, hr, content_x);
}

// Draw the window records, tab-aware, clipped to the band (window
// content routinely overflows it on both axes). Per visible record
// the cheapest authoritative tier wins:
//
//   1. no tab in the record        -> one draw call for the whole run
//   2. a covering entry validates
//      every tab boundary          -> segments drawn at the stored
//                                     advances (zero backend measures)
//   3. otherwise                   -> per-segment measure + draw
//
// Tab presence resolves from a covering entry's first-tab fact when
// one is authoritative, else a byte scan; segment x replays in the
// record-relative frame both modes share. A passive consumer: reads
// via find only, never moves a measure origin.
static void wlx_editor_draw_window(const WLX_Editor_Frame *f,
    const WLX_Text_Line_Record *lines, size_t count)
{
    WLX_Context *ctx = f->ctx;
    WLX_Rect band = f->eb.band;
    const char *doc = f->doc;
    size_t len = f->doc_len;
    WLX_Text_Style ts = f->ts;
    float tab_advance = f->tab_advance;
    const WLX_Editor_Line_Index *idx = f->idx;
    WLX_Text_Geom_Store *geom = f->geom;
    if (lines == NULL || count == 0 || len == 0) return;

    WLX_Scissor_Scope sc = wlx_scissor_scope_begin(ctx, band);
    for (size_t i = 0; i < count; i++) {
        const WLX_Text_Line_Record *line = &lines[i];
        if (line->empty_visual) continue;
        if (!(line->origin_y + line->line_h > band.y && line->origin_y < band.y + band.h)) continue;

        // Covering entry: must span the record's range within its measured
        // span ([origin_rel, scan_rel)) or its tab answer and advances are
        // not authoritative for this record.
        WLX_Text_Geom_Entry *e = NULL;
        if (geom != NULL && idx != NULL && idx->count > 0) {
            size_t li = wlx_editor_index_line_of(idx, line->visible_start);
            e = wlx_text_geom_covering(geom, line->visible_start,
                line->visible_end, wlx_editor_line_next(idx, li, len));
        }

        bool has_tab = false;
        if (tab_advance > 0.0f) {
            if (e != NULL) {
                // Definitive absence when the line's first measured tab is
                // missing or at/after the record's end. A first tab before
                // the record start (an earlier wrapped row) leaves presence
                // unknown; the replay walk below resolves it and
                // degenerates to the single-run draw on a tab-free record.
                has_tab = wlx_text_pen_has_tab(wlx_text_geom_first_tab_abs(e),
                    line->visible_end);
            } else {
                for (size_t b = line->visible_start; b < line->visible_end; b++) {
                    if (doc[b] == '\t') { has_tab = true; break; }
                }
            }
        }
        if (!has_tab) {
            wlx_draw_text_range(ctx, doc, len, line->visible_start, line->visible_end,
                line->origin_x, line->origin_y, ts);
            continue;
        }

        bool replay = e != NULL;
        if (replay) {
            // Every segment's leading boundary must be a measured unit end
            // before anything draws; a mismatch falls back wholesale.
            for (size_t b = line->visible_start; b < line->visible_end && replay; b++) {
                if (doc[b] == '\t' && wlx_text_geom_unit_index(e,
                        b + 1 - e->line_start) == SIZE_MAX)
                    replay = false;
            }
        }
        if (replay) {
            // Segment x from the stored advances: a tab is a single-byte
            // unit, so the advance at its unit end is the following
            // segment's tab-stop start in the record's frame.
            size_t pos = line->visible_start;
            float x = 0.0f;
            WLX_Text_Tab_Seg seg;
            while (wlx_text_tab_seg_next(doc, line->visible_end, &pos, &seg)) {
                if (seg.end > seg.start) {
                    wlx_draw_text_range(ctx, doc, len, seg.start, seg.end,
                        line->origin_x + x, line->origin_y, ts);
                }
                if (seg.tab_after) {
                    // The validation loop above proved every tab's unit
                    // boundary, so this lookup cannot miss.
                    wlx_text_geom_advance_at(e, seg.end + 1 - e->line_start, &x);
                }
            }
            continue;
        }

        float x = 0.0f;
        size_t pos = line->visible_start;
        WLX_Text_Tab_Seg seg;
        while (wlx_text_tab_seg_next(doc, line->visible_end, &pos, &seg)) {
            if (seg.end > seg.start) {
                float seg_w = 0.0f, seg_h = 0.0f;
                wlx_measure_text_range(ctx, doc, len, seg.start, seg.end, ts, &seg_w, &seg_h);
                wlx_draw_text_range(ctx, doc, len, seg.start, seg.end,
                    line->origin_x + x, line->origin_y, ts);
                x += seg_w;
            }
            if (seg.tab_after) x = wlx_tab_stop_next(x, tab_advance);
        }
    }
    wlx_scissor_scope_end(ctx, sc);
}

// Cheap staleness backstop behind the length/revision guard: sampled indexed
// offsets must still be hard line starts of the current buffer. Catches
// in-place mutations that keep both the length and the revision.
static bool wlx_editor_index_probe_ok(const WLX_Editor_Line_Index *idx, const char *text, size_t length) {
    if (idx->count == 0) return false;
    size_t mid = idx->offsets[idx->count / 2];
    if (mid > length || !wlx_text_hard_line_start_at(text, length, mid)) return false;
    size_t last = idx->offsets[idx->count - 1];
    return last <= length && wlx_text_hard_line_start_at(text, length, last);
}

// Keyboard caret/selection vocabulary for a focused editor beyond the
// shared wlx_text_edit_handle_keys set: HOME/END on the caret's line (the
// command modifier stretches to the document ends), UP/DOWN and
// PageUp/PageDown with a sticky column (content-space x, resolved through
// on-demand line records so motion works across window edges). SHIFT
// extends the selection. wrap_inputs non-NULL switches vertical motion to
// visual rows with a row-relative sticky column. Returns true when the
// caret or the selection changed.
static bool wlx_editor_handle_motion_keys(const WLX_Editor_Frame *f,
    float motion_w, size_t page_lines)
{
    WLX_Context *ctx = f->ctx;
    WLX_Editor_State *state = f->state;
    const WLX_Editor_Line_Index *idx = f->idx;
    WLX_Text_Geom_Store *geom = f->geom;
    const char *doc = f->doc;
    size_t len = f->doc_len;
    WLX_Text_Style ts = f->ts;
    float line_h = f->line_h;
    float tab_advance = f->tab_advance;
    float view_w = f->eb.band.w;
    const WLX_Text_Build_Inputs *wrap_inputs = f->wrap_geom ? f->wrap_inputs : NULL;
    bool shift = wlx_mod_down(ctx, WLX_MOD_SHIFT);
    bool moved = false;

    // HOME/END jump within the caret's document line (by index arithmetic,
    // no window dependency); the command modifier jumps to the document ends.
    bool home_hit = wlx_is_key_actuated(ctx, WLX_KEY_HOME);
    bool end_hit  = wlx_is_key_actuated(ctx, WLX_KEY_END);
    if (home_hit || end_hit) {
        if (wlx_mod_command_down(ctx)) {
            state->caret.cursor_pos = end_hit ? len : 0;
        } else {
            size_t line = wlx_editor_index_line_of(idx, state->caret.cursor_pos);
            state->caret.cursor_pos = end_hit
                ? wlx_editor_line_text_end(doc, len, idx, line)
                : idx->offsets[line];
        }
        if (!shift) state->caret.selection_anchor = state->caret.cursor_pos;
        state->caret.preferred_x_valid = false;
        moved = true;
    }

    // Vertical motion: UP/DOWN step one line, PageUp/PageDown step a
    // viewport of lines, both aiming at the sticky column. The first
    // vertical move latches the caret x; the edge clamps drop the latch.
    bool up_hit   = wlx_is_key_actuated(ctx, WLX_KEY_UP);
    bool down_hit = wlx_is_key_actuated(ctx, WLX_KEY_DOWN);
    bool pgup_hit = wlx_is_key_actuated(ctx, WLX_KEY_PAGE_UP);
    bool pgdn_hit = wlx_is_key_actuated(ctx, WLX_KEY_PAGE_DOWN);
    if ((up_hit != down_hit || pgup_hit != pgdn_hit) && idx->count > 0) {
        size_t line = wlx_editor_index_line_of(idx, state->caret.cursor_pos);
        long step = 0;
        if (up_hit != down_hit) step += up_hit ? -1 : 1;
        if (pgup_hit != pgdn_hit) step += pgup_hit ? -(long)page_lines : (long)page_lines;

        if (wrap_inputs != NULL) {
            // Visual-row motion: the sticky column is row-relative, the
            // target row found by walking whole-line row counts (at most
            // a page of lines, each budget-bounded).
            WLX_Text_Line_Record rec;
            size_t row = wlx_editor_wrap_caret_row(wrap_inputs, idx, line,
                state->caret.cursor_pos, &rec);
            if (!state->caret.preferred_x_valid) {
                state->caret.preferred_x = wlx_editor_wrap_caret_x(wrap_inputs, &rec,
                    wlx_editor_wrap_line_next(wrap_inputs, idx, line),
                    state->caret.cursor_pos);
                state->caret.preferred_x_valid = true;
            }

            long target_row = (long)row + step;
            size_t cur_line = line;
            while (target_row < 0 && cur_line > 0) {
                cur_line--;
                target_row += (long)wlx_editor_wrap_line_rows(wrap_inputs, idx, cur_line);
            }
            long rows_in = (long)wlx_editor_wrap_line_rows(wrap_inputs, idx, cur_line);
            while (target_row >= rows_in && cur_line + 1 < idx->count) {
                target_row -= rows_in;
                cur_line++;
                rows_in = (long)wlx_editor_wrap_line_rows(wrap_inputs, idx, cur_line);
            }
            if (target_row < 0) {
                state->caret.cursor_pos = 0;
                state->caret.preferred_x_valid = false;
            } else if (target_row >= rows_in) {
                state->caret.cursor_pos = len;
                state->caret.preferred_x_valid = false;
            } else {
                state->caret.cursor_pos = wlx_editor_wrap_offset_at_row_x(wrap_inputs, idx,
                    cur_line, (size_t)target_row, state->caret.preferred_x);
            }
        } else {
            if (!state->caret.preferred_x_valid) {
                size_t text_end = wlx_editor_line_text_end(doc, len, idx, line);
                size_t cx_off = state->caret.cursor_pos < text_end ? state->caret.cursor_pos : text_end;
                state->caret.preferred_x = wlx_editor_caret_x(ctx, doc, len, ts, tab_advance,
                    geom, wlx_editor_line_next(idx, line, len),
                    idx->offsets[line], cx_off, true);
                state->caret.preferred_x_valid = true;
            }

            if (step < 0 && line == 0) {
                state->caret.cursor_pos = 0;
                state->caret.preferred_x_valid = false;
            } else if (step > 0 && line == idx->count - 1) {
                state->caret.cursor_pos = len;
                state->caret.preferred_x_valid = false;
            } else {
                long target = (long)line + step;
                if (target < 0) target = 0;
                if (target >= (long)idx->count) target = (long)idx->count - 1;
                float target_w = motion_w;
                if (target_w < state->caret.preferred_x + line_h) target_w = state->caret.preferred_x + line_h;
                state->caret.cursor_pos = wlx_editor_offset_at_x(ctx, doc, len, ts, line_h, tab_advance,
                    idx, geom, (size_t)target, target_w, view_w, state->caret.preferred_x);
            }
        }
        if (!shift) state->caret.selection_anchor = state->caret.cursor_pos;
        moved = true;
    }

    size_t normalized = wlx_text_normalize_cursor_offset(doc, len, state->caret.cursor_pos);
    if (normalized != state->caret.cursor_pos) moved = true;
    state->caret.cursor_pos = normalized;
    state->caret.selection_anchor = wlx_text_normalize_cursor_offset(doc, len, state->caret.selection_anchor);
    return moved;
}

// Resolve the frame's band geometry from the field interior: the
// line-number gutter strip, both scrollbar strips and their tracks,
// the wrapped-overflow probe, and the pre-strip window band height,
// in one deterministic pass. The visibility rules each encode a
// documented feedback failure (bar blink, probe feedback); see
// WLX_Editor_Band above and docs/EDITOR_MODEL.md section 5.
static WLX_Editor_Band wlx_editor_resolve_band(WLX_Context *ctx, const WLX_Editor_Opt *opt,
    WLX_Editor_State *state, const WLX_Editor_Line_Index *idx, const char *doc, size_t doc_len,
    WLX_Text_Style ts, float line_h, float tab_advance, WLX_Rect input_rect,
    float border_inset, size_t line_count, float content_h)
{
    // Text band: the interior window the document scrolls behind. Always
    // top-left anchored; a partially visible line at each edge is normal.
    WLX_Rect band = {
        input_rect.x + WLX_TEXT_FIELD_INSET,
        input_rect.y + border_inset,
        input_rect.w - WLX_TEXT_FIELD_INSET - WLX_TEXT_CARET_WIDTH - WLX_TEXT_CARET_PADDING,
        input_rect.h - border_inset * 2.0f,
    };
    if (band.w < 0.0f) band.w = 0.0f;
    if (band.h < 0.0f) band.h = 0.0f;

    // Line-number gutter on the leading edge, sized by the measured
    // width of the widest line number plus three quarters of a digit of
    // padding per side. Both the width and the padding derive from the
    // measured digits, never from the space advance: digit and space
    // metrics diverge across backends. The band (and with it every
    // scrollbar track) starts after the strip.
    float gutter_w = 0.0f;
    float gutter_pad = 2.0f;
    WLX_Rect gutter_rect = {0};
    if (opt->line_numbers && line_count > 0) {
        char max_num[24];
        int max_len = snprintf(max_num, sizeof(max_num), "%zu", line_count);
        if (max_len < 1) max_len = 1;
        float max_num_w = 0.0f, max_num_h = 0.0f;
        wlx_measure_text_slice(ctx, max_num, (size_t)max_len, ts, &max_num_w, &max_num_h);
        if (max_num_w <= 0.0f) max_num_w = (float)max_len * (float)opt->font_size * 0.5f;
        gutter_pad = (max_num_w / (float)max_len) * 0.75f;
        gutter_w = max_num_w + 2.0f * gutter_pad;
        if (gutter_w > band.w) gutter_w = band.w;
        gutter_rect = (WLX_Rect){ band.x, band.y, gutter_w, band.h };
        band.x += gutter_w;
        band.w -= gutter_w;
    }
    state->gutter_end_x = gutter_w > 0.0f ? gutter_rect.x + gutter_w : 0.0f;

    // Scrollbar strips. The vertical decision depends only on the line
    // count (wrapped: plus a bounded row probe of the visible lines);
    // the horizontal one on the sticky max-seen line width. A
    // horizontal bar shortens the band, which can newly overflow it
    // vertically - resolved in one deterministic pass, no oscillation.
    float sb_w = ctx->theme->scrollbar.width > 0.0f
        ? ctx->theme->scrollbar.width : WLX_SCROLLBAR_FALLBACK_WIDTH;
    // Wrapped content overflows when the document has more hard lines
    // than the band has rows (every line is at least one row), when
    // the view is already scrolled, or when the few visible lines
    // alone wrap past the band. Probed at the pre-strip width:
    // stripping the bar only adds rows, so the decision cannot
    // oscillate.
    bool wrap_overflow = false;
    if (opt->wrap && idx != NULL && line_count > 0 && line_h > 0.0f && band.w > 0.0f) {
        size_t band_rows = (size_t)(band.h / line_h);
        if (line_count > band_rows
            || state->first_line > 0 || state->first_row > 0 || state->y_frac > 0.0f) {
            wrap_overflow = true;
        } else {
            // The probe runs at the pre-strip band width, so it keeps
            // row_memo NULL: its counts must not seed (or read) the
            // final-width memo below. Already bounded to band_rows.
            WLX_Text_Build_Inputs probe = wlx_editor_wrap_build_inputs(ctx, doc, doc_len,
                ts, band.w, line_h, tab_advance);
            size_t rows = 0;
            for (size_t l = 0; l < line_count && rows <= band_rows; l++) {
                rows += wlx_editor_wrap_line_rows(&probe, idx, l);
            }
            wrap_overflow = rows > band_rows;
        }
    }
    bool sb_visible = opt->show_scrollbar
        && (opt->wrap ? wrap_overflow : content_h > band.h);
    if (sb_visible) {
        band.w -= sb_w;
        if (band.w < 0.0f) band.w = 0.0f;
    }
    bool hb_visible = !opt->wrap && opt->show_scrollbar
        && (state->max_line_w > band.w || state->h_reach_open);
    // The window build sizes itself from the pre-strip height: the strip's
    // visibility follows the window's reach flag, so a window sized by the
    // post-strip band would feed visibility back into its own input and
    // oscillate when a long line sits at the window's bottom edge.
    float win_band_h = band.h;
    if (hb_visible) {
        band.h -= sb_w;
        if (band.h < 0.0f) band.h = 0.0f;
        if (!sb_visible && opt->show_scrollbar && content_h > band.h) {
            sb_visible = true;
            band.w -= sb_w;
            if (band.w < 0.0f) band.w = 0.0f;
        }
    }
    WLX_Rect sb_track = { input_rect.x + border_inset, band.y,
                          input_rect.w - border_inset * 2.0f, band.h };
    WLX_Rect hb_track = { band.x, band.y + band.h, band.w, sb_w };
    gutter_rect.h = band.h;

    return (WLX_Editor_Band){
        .band = band, .gutter_rect = gutter_rect,
        .gutter_w = gutter_w, .gutter_pad = gutter_pad,
        .sb_w = sb_w, .sb_track = sb_track, .hb_track = hb_track,
        .sb_visible = sb_visible, .hb_visible = hb_visible,
        .wrap_overflow = wrap_overflow, .win_band_h = win_band_h,
    };
}

// Horizontal content extent the editor scrolls over: the sticky max-seen
// line width plus caret room past the last glyph (the margin caret-follow
// targets). An extent clamped to the glyph width parks an end-of-line
// caret just outside the band - visible only on caret-follow frames,
// culled and unreachable by scrolling on every frame after. The scroll
// limit and both thumb-range computations must share this extent: a range
// that omits the margin coincides with the limit 4px early and the thumb
// breathes at a long line's end.
static float wlx_editor_h_extent(const WLX_Editor_State *state) {
    if (state->max_line_w <= 0.0f) return 0.0f;
    return state->max_line_w + WLX_TEXT_CARET_WIDTH + WLX_TEXT_CARET_PADDING;
}

// Wrapped pseudo scroll of an anchor: hard-line pseudo pixels (the
// anchor line plus its row component as a fraction of that line's rows,
// times the line height). The single derivation shared by the scroll
// limits, the wheel, the thumb's range end (the bottom anchor), and the
// wrapped build's final scrollbar value; its inverse is
// wlx_editor_anchor_set_scroll_y below, and the pair is the whole
// scroll-to-anchor conversion surface (the scroll dataflow diagram in
// docs/TEXT_PIPELINE_MAP.md).
static float wlx_editor_pseudo_scroll(size_t line, size_t row, float y_frac,
    size_t rows, float line_h)
{
    return ((float)line + ((float)row + y_frac) / (float)rows) * line_h;
}

// The pseudo scroll of the state's scroll anchor.
static float wlx_editor_pseudo_scroll_y(const WLX_Editor_State *state,
    size_t anchor_rows, float line_h)
{
    return wlx_editor_pseudo_scroll(state->first_line, state->first_row,
        state->y_frac, anchor_rows, line_h);
}

// Decompose a pixel (wrapped: pseudo) scroll into the anchor - the
// inverse of the derivations above, and the only way scroll motion
// reaches the anchor. Each gesture that moves the scroll (wheel, thumb
// drag, mouse auto-scroll, linear caret-follow) calls it at its own
// site, and the linear scroll-limits clamp calls it when normalization
// changes the derived value, so the scroll anchor stays the one
// authoritative scroll state and the frame's scroll_y is a derived
// view. The fractional hard line converts to a row of that line under
// wrap. See the scroll dataflow diagram in docs/TEXT_PIPELINE_MAP.md.
static void wlx_editor_anchor_set_scroll_y(const WLX_Editor_Frame *f, float scroll_y)
{
    WLX_Editor_State *state = f->state;
    if (f->line_h <= 0.0f) return;
    float lines_scrolled = scroll_y / f->line_h;
    state->first_line = (size_t)lines_scrolled;
    float line_frac = lines_scrolled - (float)state->first_line;
    if (f->line_count > 0 && state->first_line >= f->line_count) {
        state->first_line = f->line_count - 1;
        line_frac = 0.0f;
    }
    if (f->wrap_geom) {
        size_t rows = wlx_editor_wrap_line_rows(f->wrap_inputs, f->idx,
            state->first_line);
        float row_pos = line_frac * (float)rows;
        state->first_row = (size_t)row_pos;
        if (state->first_row >= rows) state->first_row = rows - 1;
        state->y_frac = row_pos - (float)state->first_row;
    } else {
        state->first_row = 0;
        state->y_frac = line_frac;
    }
}

// The wrapped bottom anchor: the (line, row, y_frac) that bottom-aligns
// the document's last row against the band's bottom edge - where a
// wheel, drag, or caret-follow lands at the document end, and the end of
// the vertical thumb's range. Answered from the state's cache while it
// holds (see WLX_Editor_State); a miss fills a band of rows backward from
// the last row, touching at most a band of hard lines, each row count
// budget-bounded. A document shorter than the band anchors at the top
// with y_frac 0. Every output pointer is optional.
static void wlx_editor_wrap_bottom_anchor(const WLX_Editor_Frame *f,
    size_t *out_line, size_t *out_row, float *out_y_frac, size_t *out_rows)
{
    WLX_Editor_State *state = f->state;
    const WLX_Editor_Line_Index *idx = f->idx;
    float band_h = f->eb.band.h;
    float line_h = f->line_h;
    size_t need = (size_t)ceilf(band_h / line_h);
    if (need == 0) need = 1;
    if (!state->bottom_valid || state->bottom_need != need
        || state->bottom_lines_from_end >= f->line_count) {
        size_t last_line = f->line_count - 1;
        size_t last_rows = wlx_editor_wrap_line_rows(f->wrap_inputs, idx, last_line);
        size_t a_line = 0, a_row = 0;
        size_t got = wlx_editor_wrap_fill_backward(f->wrap_inputs, idx, last_line,
            last_rows - 1, need, &a_line, &a_row);
        float y_frac = got >= need ? ((float)need * line_h - band_h) / line_h : 0.0f;
        if (y_frac < 0.0f) y_frac = 0.0f;
        state->bottom_valid = true;
        state->bottom_need = need;
        state->bottom_lines_from_end = last_line - a_line;
        state->bottom_row = a_row;
        state->bottom_rows = a_line == last_line ? last_rows
            : wlx_editor_wrap_line_rows(f->wrap_inputs, idx, a_line);
        state->bottom_y_frac = y_frac;
        state->bottom_start_off = idx->offsets[a_line];
    }
    if (out_line != NULL) *out_line = f->line_count - 1 - state->bottom_lines_from_end;
    if (out_row != NULL) *out_row = state->bottom_row;
    if (out_y_frac != NULL) *out_y_frac = state->bottom_y_frac;
    if (out_rows != NULL) *out_rows = state->bottom_rows;
}

// Pseudo scroll of the wrapped bottom anchor: the vertical thumb's range
// end. Exact at wrap factor one, where it is line_count * line_h - band.h.
static float wlx_editor_wrap_bottom_pseudo(const WLX_Editor_Frame *f)
{
    size_t line = 0, row = 0, rows = 1;
    float y_frac = 0.0f;
    wlx_editor_wrap_bottom_anchor(f, &line, &row, &y_frac, &rows);
    return wlx_editor_pseudo_scroll(line, row, y_frac, rows, f->line_h);
}

// Derive the frame's scroll state from the scroll anchor and the
// sticky max-seen width: scroll_y (pseudo-pixels under wrap), the
// vertical range, the horizontal scroll limit, and the horizontal
// thumb mapping range. A linear clamp change normalizes the anchor
// through wlx_editor_anchor_set_scroll_y.
//
// The two horizontal ranges (no-wrap) do different jobs:
//
//   x -> 0            measured reach          open extension
//        |==================|. . . . . . . . . . .|
//
//   scroll limit: the reach, extended to scroll_x + lookahead bands
//        while the open-reach flag holds, so the view can keep
//        moving into unmeasured content.
//   thumb range: max(reach, scroll_x + band w) - flag-free and
//        continuous in scroll_x, so the thumb cannot snap when the
//        flag flips at a long line's end.
//
// See docs/EDITOR_MODEL.md section 5 for why the split exists.
static void wlx_editor_scroll_limits(WLX_Editor_Frame *f, WLX_Editor_Scroll *scr)
{
    const WLX_Editor_Opt *opt = f->opt;
    WLX_Editor_State *state = f->state;
    const WLX_Editor_Band *eb = &f->eb;
    bool wrap_geom = f->wrap_geom;
    size_t anchor_rows = scr->anchor_rows;
    float line_h = f->line_h;
    float *content_h = &f->content_h;
    float *scroll_y = &scr->scroll_y;
    float *max_scroll_y = &scr->max_scroll_y;
    float *max_scroll_x = &scr->max_scroll_x;
    float *h_content_w = &scr->h_content_w;
    WLX_Rect band = eb->band;

    // Wrapped mode scrolls in hard-line pseudo pixels (as if nothing
    // wrapped): thumb geometry and drag mapping stay exact at wrap
    // factor one and continuous elsewhere, while real row motion
    // (wheel, caret-follow, clamps) works on the anchor and re-derives
    // this value. The range ends at the bottom anchor's pseudo scroll,
    // not at line_count * line_h - band.h: a band of wrapped rows holds
    // fewer hard lines than a band of unwrapped ones, so that arithmetic
    // end sits lines above the real one - the thumb reached the track
    // end early and stuck there while the view scrolled back up through
    // those lines. Ending at the anchor, the thumb hits the track end
    // exactly at the document end and leaves it on the first row back
    // up; the thumb's content height follows as range plus band.
    if (wrap_geom) {
        *max_scroll_y = wlx_editor_wrap_bottom_pseudo(f);
        *content_h = *max_scroll_y + band.h;
    } else {
        *max_scroll_y = *content_h > band.h ? *content_h - band.h : 0.0f;
    }

    // Pixel scroll from the anchor, clamped to the exact content height
    // (wrapped: the pseudo scroll is a view of the anchor and clamps at
    // draw time instead, so the anchor round-trips exactly).
    *scroll_y = wrap_geom
        ? wlx_editor_pseudo_scroll_y(state, anchor_rows, line_h)
        : ((float)state->first_line + state->y_frac) * line_h;
    if (!opt->wrap) {
        float clamped = wlx_clampf(*scroll_y, 0.0f, *max_scroll_y);
        if (clamped != *scroll_y) {
            // The anchor lay outside the scrollable range (document
            // shrink, band growth): normalize it through the clamp -
            // the one non-gesture scroll-anchor write.
            *scroll_y = clamped;
            wlx_editor_anchor_set_scroll_y(f, clamped);
        }
    }

    // Horizontal scroll limit: the sticky max-seen content extent,
    // extended one band past the current reach while a visible line is
    // still width-truncated (its true width is unknown until scrolled to).
    // Wrapped rows never overflow sideways: horizontal scroll is pinned
    // while the mode is on (the fields stay for toggling back).
    if (opt->wrap) state->scroll_x = 0.0f;
    float h_limit_w = wlx_editor_h_extent(state);
    if (state->h_reach_open && h_limit_w < state->scroll_x + band.w * WLX_EDITOR_MEASURE_LOOKAHEAD_BANDS)
        h_limit_w = state->scroll_x + band.w * WLX_EDITOR_MEASURE_LOOKAHEAD_BANDS;
    *max_scroll_x = (!opt->wrap && h_limit_w > band.w) ? h_limit_w - band.w : 0.0f;
    state->scroll_x = wlx_clampf(state->scroll_x, 0.0f, *max_scroll_x);
    // Thumb range: only the content measured so far, never the open
    // extension. A reach-dependent range snaps when the flag flips at a
    // long line's end; this one is continuous in scroll_x. The window
    // build's one-band measure lookahead keeps it a band ahead of the
    // view while a line is still truncated, so the thumb always leaves
    // drag room toward unmeasured content.
    *h_content_w = wlx_editor_h_extent(state);
    if (*h_content_w < state->scroll_x + band.w)
        *h_content_w = state->scroll_x + band.w;
}

// Frame scroll resolution: the wheel (Shift redirects to the horizontal
// axis; wrap scrolls row space through the anchor) and both scrollbar
// thumb gestures. Every gesture that moves the vertical scroll
// decomposes it into the anchor right here at its site; a wrapped drag
// to the track end anchors past the last row instead and lets the
// window build's structural backfill land the exact bottom anchor.
static void wlx_editor_scroll_resolve(const WLX_Editor_Frame *f, WLX_Editor_Scroll *scr)
{
    WLX_Context *ctx = f->ctx;
    const WLX_Editor_Opt *opt = f->opt;
    WLX_Editor_State *state = f->state;
    WLX_Interaction inter = f->inter;
    const WLX_Editor_Line_Index *idx = f->idx;
    const WLX_Editor_Band *eb = &f->eb;
    const WLX_Text_Build_Inputs *wrap_inputs = f->wrap_inputs;
    bool wrap_geom = f->wrap_geom;
    float line_h = f->line_h;
    float content_h = f->content_h;
    float max_scroll_y = scr->max_scroll_y;
    float max_scroll_x = scr->max_scroll_x;
    float h_content_w = scr->h_content_w;
    float *scroll_y = &scr->scroll_y;
    size_t *anchor_rows = &scr->anchor_rows;
    // Wheel: a hovered editor with overflow on the wheel's axis owns the
    // delta and consumes it (innermost scrollable wins); without overflow
    // the delta is left to the enclosing panel. Shift redirects the wheel
    // to the horizontal axis.
    if (inter.hover && !inter.disabled && ctx->input.wheel_delta != 0.0f
            && wlx_pointer_on_current_layer(ctx)) {
        if (wlx_mod_down(ctx, WLX_MOD_SHIFT)) {
            wlx_wheel_consume(ctx, &state->scroll_x, max_scroll_x,
                WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED);
        } else if (opt->wrap) {
            if (wrap_geom && eb->wrap_overflow) {
                // Row-space scrolling: the wheel moves visual rows, not
                // hard lines, so heavily wrapped regions scroll evenly.
                // Overshoot past the last row is left for the window
                // build's bottom clamp.
                float px = state->y_frac * line_h
                    - ctx->input.wheel_delta * WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED;
                float steps = floorf(px / line_h);
                state->y_frac = (px - steps * line_h) / line_h;
                long left = wlx_editor_wrap_anchor_step(wrap_inputs, idx,
                    &state->first_line, &state->first_row, (long)steps);
                if (left < 0) state->y_frac = 0.0f;
                *anchor_rows = wlx_editor_wrap_line_rows(wrap_inputs, idx,
                    state->first_line);
                *scroll_y = wlx_editor_pseudo_scroll_y(state, *anchor_rows, line_h);
                ctx->input.wheel_delta = 0.0f;
            }
        } else {
            float before = *scroll_y;
            wlx_wheel_consume(ctx, scroll_y, max_scroll_y,
                WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED);
            if (*scroll_y != before)
                wlx_editor_anchor_set_scroll_y(f, *scroll_y);
        }
    }

    // Horizontal wheel (trackpad tilt/swipe): same ownership rule on the
    // horizontal axis; without horizontal overflow the delta is left to
    // the enclosing panel.
    if (inter.hover && !inter.disabled && ctx->input.wheel_delta_x != 0.0f) {
        wlx_wheel_consume_x(ctx, &state->scroll_x, max_scroll_x,
            WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED);
    }

    // Scrollbar thumb drags on raw mouse primitives, resolved before
    // any caret handling so a thumb press never places the caret.
    if (eb->sb_visible && !inter.disabled) {
        WLX_Rect sb_rect = wlx_scrollbar_rect(eb->sb_track, content_h, *scroll_y, eb->sb_w);
        float drag_pos = -1.0f;
        *scroll_y = wlx_thumb_drag_update(ctx, eb->sb_track, sb_rect, true,
            max_scroll_y, *scroll_y,
            &state->caret.dragging_scrollbar, &state->caret.sb_drag_offset, &drag_pos);
        if (drag_pos >= 0.0f) {
            if (wrap_geom
                && drag_pos >= (eb->sb_track.h - sb_rect.h) - WLX_EDITOR_ROW_EPSILON) {
                // Track end always means the document end: anchor past
                // the last row and let the window build's structural
                // backfill land the exact bottom anchor.
                state->first_line = f->line_count - 1;
                state->first_row = wlx_editor_wrap_line_rows(wrap_inputs, idx,
                    f->line_count - 1) - 1;
                state->y_frac = WLX_EDITOR_BOTTOM_ANCHOR_ROW_FRAC;
            } else if (!opt->wrap || wrap_geom) {
                // The drag flows through the pixel (wrapped: pseudo)
                // scroll and decomposes into the anchor here at the
                // gesture.
                wlx_editor_anchor_set_scroll_y(f, *scroll_y);
            }
        }
    } else {
        state->caret.dragging_scrollbar = false;
    }

    if (eb->hb_visible && !inter.disabled && h_content_w > 0.0f) {
        // A live drag maps through the range frozen at the press (the
        // live range follows the drag's own output: a held thumb
        // re-mapped through it makes the view creep or oscillate near a
        // long line's end); the thumb rect follows the mapping range.
        // While not dragging both ranges agree, so the press always
        // hits the live thumb.
        float map_w = h_content_w;
        if (state->dragging_hbar) {
            map_w = state->hb_drag_content_w;
            if (map_w < eb->band.w) map_w = eb->band.w;
        }
        WLX_Rect hb_rect = wlx_scrollbar_rect_h(eb->hb_track, eb->band.w, map_w, state->scroll_x);
        bool was_dragging = state->dragging_hbar;
        state->scroll_x = wlx_thumb_drag_update(ctx, eb->hb_track, hb_rect, false,
            map_w - eb->band.w, state->scroll_x,
            &state->dragging_hbar, &state->hb_drag_offset, NULL);
        if (!was_dragging && state->dragging_hbar)
            state->hb_drag_content_w = h_content_w;
    } else {
        state->dragging_hbar = false;
    }
}

// Row-space caret-follow: a caret above the window anchors its row to the
// top; one below bottom-aligns it via the backward fill, which doubles as
// the far-jump re-anchor (Ctrl+End) - the line-distance early-out keeps
// the walk O(viewport).
static void wlx_editor_caret_follow_wrapped(const WLX_Editor_Frame *f)
{
    const WLX_Text_Build_Inputs *wrap_inputs = f->wrap_inputs;
    const WLX_Editor_Line_Index *idx = f->idx;
    WLX_Editor_State *state = f->state;
    float band_h = f->eb.band.h;
    float line_h = f->line_h;
    size_t caret_line = wlx_editor_index_line_of(idx, state->caret.cursor_pos);
    WLX_Text_Line_Record crec;
    size_t caret_row = wlx_editor_wrap_caret_row(wrap_inputs, idx, caret_line,
        state->caret.cursor_pos, &crec);
    size_t band_rows = (size_t)(band_h / line_h);
    if (band_rows < 1) band_rows = 1;

    bool above = caret_line < state->first_line
        || (caret_line == state->first_line && caret_row < state->first_row);
    if (above) {
        state->first_line = caret_line;
        state->first_row = caret_row;
        state->y_frac = 0.0f;
    } else {
        size_t rows_between = 0;
        bool far = caret_line - state->first_line > band_rows + WLX_EDITOR_FAR_JUMP_SLACK_ROWS;
        if (!far) {
            for (size_t l = state->first_line; l < caret_line; l++) {
                rows_between += wlx_editor_wrap_line_rows(wrap_inputs, idx, l);
                if (rows_between > band_rows + state->first_row + WLX_EDITOR_FAR_JUMP_SLACK_ROWS) { far = true; break; }
            }
        }
        if (!far) rows_between = rows_between + caret_row - state->first_row;
        float caret_bottom = ((float)rows_between + 1.0f - state->y_frac) * line_h;
        if (far || caret_bottom > band_h + WLX_EDITOR_ROW_EPSILON) {
            size_t need = (size_t)ceilf(band_h / line_h);
            size_t a_line = 0, a_row = 0;
            size_t got = wlx_editor_wrap_fill_backward(wrap_inputs, idx,
                caret_line, caret_row, need, &a_line, &a_row);
            if (got >= need && need > 0) {
                state->first_line = a_line;
                state->first_row = a_row;
                state->y_frac = ((float)need * line_h - band_h) / line_h;
                if (state->y_frac < 0.0f) state->y_frac = 0.0f;
            } else {
                state->first_line = 0;
                state->first_row = 0;
                state->y_frac = 0.0f;
            }
        }
    }
}

// Linear caret-follow, both axes: the vertical from index arithmetic
// against the pixel scroll, the horizontal from a cap-bounded prefix
// measure of the caret's line against scroll_x.
static void wlx_editor_caret_follow_linear(const WLX_Editor_Frame *f,
    WLX_Editor_Scroll *scr)
{
    WLX_Context *ctx = f->ctx;
    WLX_Editor_State *state = f->state;
    const WLX_Editor_Line_Index *idx = f->idx;
    WLX_Text_Geom_Store *geom = f->geom;
    const char *doc = f->doc;
    size_t doc_len = f->doc_len;
    WLX_Text_Style ts = f->ts;
    float tab_advance = f->tab_advance;
    float line_h = f->line_h;
    WLX_Rect band = f->eb.band;
    float max_scroll_y = scr->max_scroll_y;
    float *scroll_y = &scr->scroll_y;
    size_t caret_line = wlx_editor_index_line_of(idx, state->caret.cursor_pos);
    float caret_top = (float)caret_line * line_h;
    if (caret_top < *scroll_y) {
        *scroll_y = caret_top;
    } else if (caret_top + line_h > *scroll_y + band.h) {
        *scroll_y = caret_top + line_h - band.h;
    }
    *scroll_y = wlx_clampf(*scroll_y, 0.0f, max_scroll_y);
    wlx_editor_anchor_set_scroll_y(f, *scroll_y);

    size_t line_start = idx->offsets[caret_line];
    size_t text_end = wlx_editor_line_text_end(doc, doc_len, idx, caret_line);
    size_t cx_off = state->caret.cursor_pos < text_end ? state->caret.cursor_pos : text_end;
    float caret_x = wlx_editor_caret_x(ctx, doc, doc_len, ts, tab_advance,
        geom, wlx_editor_line_next(idx, caret_line, doc_len),
        line_start, cx_off, true);
    float caret_margin = WLX_TEXT_CARET_WIDTH + WLX_TEXT_CARET_PADDING;
    if (caret_x < state->scroll_x) {
        state->scroll_x = caret_x;
    } else if (caret_x + caret_margin > state->scroll_x + band.w) {
        // May exceed the current max: the measured reach opens as the next
        // build sees the deeper prefix.
        state->scroll_x = caret_x + caret_margin - band.w;
    }
    if (state->scroll_x < 0.0f) state->scroll_x = 0.0f;
}

// Wrapped window build: stream rows from the anchor line's start, discard
// the rows above first_row, and fill the band plus overscan. A second pass
// runs only when the first shows the document tail ending above the band's
// bottom edge - the anchor then moves to the bottom anchor so the last row
// lands exactly on it (the bottom clamp is structural, there being no
// exact wrapped content height to clamp against). That second pass is the
// one place the anchor
// is rewritten mid-build: everything before the build consumed the old
// anchor already, and everything after (records, gutter, caret, scrollbar)
// reads the corrected one, so the correction must happen here and nowhere
// later. Returns the row count; *out_lines receives the frame's records
// and *scroll_y the final pseudo scroll for the trailing scrollbar draw.
static size_t wlx_editor_window_build_wrapped(const WLX_Editor_Frame *f,
    WLX_Editor_Scroll *scr, WLX_Text_Line_Record **out_lines)
{
    WLX_Context *ctx = f->ctx;
    WLX_Editor_State *state = f->state;
    const WLX_Editor_Line_Index *idx = f->idx;
    WLX_Text_Build_Inputs *wrap_inputs = f->wrap_inputs;
    const char *doc = f->doc;
    size_t doc_len = f->doc_len;
    WLX_Rect band = f->eb.band;
    float win_band_h = f->eb.win_band_h;
    float line_h = f->line_h;
    size_t line_count = f->line_count;
    float max_scroll_y = scr->max_scroll_y;
    float *scroll_y = &scr->scroll_y;
    size_t win_count = 0;
    bool wrap_at_bottom = false;
    size_t viewport_rows = (size_t)(win_band_h / line_h) + 2;
    size_t win_cap = viewport_rows + WLX_EDITOR_OVERSCAN_LINES;
    WLX_Text_Line_Record *lines = wlx_text_line_scratch(ctx, win_cap > 0 ? win_cap : 1);
    for (int pass = 0; lines != NULL && win_cap > 0 && pass < 2; pass++) {
        win_count = 0;
        size_t cur_line = state->first_line;
        size_t line_offset = idx->offsets[cur_line];
        bool append_empty = line_offset == doc_len && doc_len > 0
            && wlx_text_hard_line_start_at(doc, doc_len, line_offset);
        size_t to_skip = state->first_row;
        WLX_Text_Build_Cursor cursor = { .text_unit_count = 0 };
        wrap_inputs->known_line_next =
            wlx_editor_wrap_line_next(wrap_inputs, idx, cur_line);
        for (;;) {
            WLX_Text_Build_Step step = wlx_text_build_step(wrap_inputs, &cursor,
                line_offset, append_empty);
            if (!step.produced) break;
            if (to_skip > 0) to_skip--;
            else if (win_count < win_cap) lines[win_count++] = step.line;
            else break;
            line_offset = step.next_offset;
            append_empty = step.append_trailing_empty_line;
            if (step.line.ended_by_newline && cur_line + 1 < line_count) {
                cur_line++;
                wrap_inputs->known_line_next =
                    wlx_editor_wrap_line_next(wrap_inputs, idx, cur_line);
            }
        }

        float rows_h = (float)win_count * line_h - state->y_frac * line_h;
        bool tail_in = win_count > 0 && lines[win_count - 1].source_end >= doc_len;
        bool over_end = tail_in && rows_h < band.h - WLX_EDITOR_ROW_EPSILON;
        if (pass == 0 && over_end
            && !(state->first_line == 0 && state->first_row == 0
                 && state->y_frac <= 0.0f)) {
            wlx_editor_wrap_bottom_anchor(f, &state->first_line,
                &state->first_row, &state->y_frac, NULL);
            wrap_at_bottom = true;
            continue;
        }
        wrap_at_bottom = wrap_at_bottom || (tail_in && rows_h <= band.h + WLX_EDITOR_ROW_EPSILON);
        break;
    }

    float band_top = band.y - state->y_frac * line_h;
    for (size_t i = 0; i < win_count; i++) {
        lines[i].origin_x = band.x;
        lines[i].origin_y = band_top + (float)i * line_h;
    }
    state->h_reach_open = false;

    // Final pseudo scroll for the trailing scrollbar draw; the
    // bottom anchor always reads as the track end.
    size_t anchor_rows = wlx_editor_wrap_line_rows(wrap_inputs, idx, state->first_line);
    *scroll_y = wrap_at_bottom ? max_scroll_y
        : wlx_editor_pseudo_scroll_y(state, anchor_rows, line_h);
    *scroll_y = wlx_clampf(*scroll_y, 0.0f, max_scroll_y);

    *out_lines = lines;
    return win_count;
}

// Linear window build: only the visible lines (plus overscan) become line
// records, through the from-offset truncate-and-continue pipeline, then
// re-anchored to the band. The virtual width is scroll_x plus two bands:
// one band shows, the second is measured ahead so the max-seen width stays
// a band past the view while a line is still truncated - without the
// lookahead the thumb range collapses onto the reach and the thumb fills
// the track at scroll_x 0 with no room to drag. Grows the sticky max-seen
// width and rewrites the reach flag from the built records. Returns the
// record count; *out_lines receives the frame's records.
static size_t wlx_editor_window_build_linear(const WLX_Editor_Frame *f,
    WLX_Text_Line_Record **out_lines)
{
    WLX_Context *ctx = f->ctx;
    WLX_Editor_State *state = f->state;
    const WLX_Editor_Line_Index *idx = f->idx;
    WLX_Text_Geom_Store *geom = f->geom;
    const char *doc = f->doc;
    size_t doc_len = f->doc_len;
    WLX_Text_Style ts = f->ts;
    float tab_advance = f->tab_advance;
    float line_h = f->line_h;
    float content_h = f->content_h;
    WLX_Rect band = f->eb.band;
    float win_band_h = f->eb.win_band_h;
    size_t line_count = f->line_count;
    size_t win_count = 0;
    size_t viewport_lines = (size_t)(win_band_h / line_h) + 2;
    size_t win_cap = viewport_lines + WLX_EDITOR_OVERSCAN_LINES;
    size_t remaining = line_count - state->first_line;
    if (win_cap > remaining) win_cap = remaining;
    WLX_Text_Line_Record *lines = wlx_text_line_scratch(ctx, win_cap > 0 ? win_cap : 1);
    if (lines != NULL && win_cap > 0) {
        WLX_Text_Build_Inputs inputs = {
            .ctx = ctx,
            .text = doc,
            .length = doc_len,
            .style = ts,
            .rect = { 0, 0, state->scroll_x + band.w * WLX_EDITOR_MEASURE_LOOKAHEAD_BANDS, content_h },
            .wrap = false,
            .line_h = line_h,
            .text_unit_cap = WLX_EDITOR_MAX_LINE_UNITS,
            .truncate_continue = true,
            .tab_advance = tab_advance,
            .geom = geom,
            .view_x = state->scroll_x,
            .view_w = band.w,
        };
        // One record per index line: in truncate-and-continue mode every
        // step starts at a hard line start and covers the whole line, so
        // the per-line loop produces exactly the records a single
        // build-from would - and can hand each step its true next-line
        // start, which both the tail skip and the retained store key on.
        WLX_Text_Build_Cursor cursor = { .text_unit_count = 0 };
        float band_top = band.y - state->y_frac * line_h;
        float win_max_w = 0.0f;
        bool reach_open = false;
        for (size_t i = 0; i < win_cap; i++) {
            size_t line = state->first_line + i;
            size_t line_offset = idx->offsets[line];
            inputs.known_line_next = line + 1 < line_count
                ? idx->offsets[line + 1] : doc_len;
            bool trailing = line_offset == doc_len;
            WLX_Text_Build_Step step = wlx_text_build_step(&inputs, &cursor,
                line_offset, trailing);
            if (!step.produced) break;
            // A record entering its line at a deeper measure origin is
            // placed (and its width reported) at the origin's frozen
            // content-space x; everything the origin skipped counts into
            // the sticky max-seen width as that estimate, keeping the
            // monotone contract on estimated-absolute x.
            float rec_x = 0.0f;
            if (geom != NULL && step.line.visible_start > step.line.source_start) {
                WLX_Text_Geom_Entry *e = wlx_text_geom_find(geom,
                    step.line.source_start, inputs.known_line_next);
                if (e != NULL
                    && step.line.visible_start == wlx_text_geom_origin_abs(e))
                    rec_x = e->origin_x;
            }
            step.line.origin_x = band.x - state->scroll_x + rec_x;
            step.line.origin_y = band_top + (float)win_count * line_h;
            if (rec_x + step.line.measured_w > win_max_w)
                win_max_w = rec_x + step.line.measured_w;
            if (!reach_open && wlx_editor_record_reach_open(&step.line))
                reach_open = true;
            lines[win_count++] = step.line;
        }
        if (win_max_w > state->max_line_w) state->max_line_w = win_max_w;
        state->h_reach_open = reach_open;
    }
    *out_lines = lines;
    return win_count;
}

// Gutter numbers: the index position of each visible record,
// right-aligned inside the gutter padding, dimmed against the text. An
// empty document still numbers its one empty line.
static void wlx_editor_draw_gutter(const WLX_Editor_Frame *f,
    const WLX_Text_Line_Record *lines, size_t win_count)
{
    WLX_Context *ctx = f->ctx;
    const WLX_Editor_State *state = f->state;
    bool wrap = f->opt->wrap;
    const char *doc = f->doc;
    size_t doc_len = f->doc_len;
    WLX_Rect gutter_rect = f->eb.gutter_rect;
    float gutter_pad = f->eb.gutter_pad;
    WLX_Rect band = f->eb.band;
    float line_h = f->line_h;
    WLX_Text_Style gutter_ts = f->ts;
    gutter_ts.color.a = (unsigned char)(gutter_ts.color.a / 2);
    WLX_Scissor_Scope gutter_clip = wlx_scissor_scope_begin(ctx, gutter_rect);
    char num[24];
    if (win_count == 0 && doc_len == 0) {
        float num_w = 0.0f, num_h = 0.0f;
        wlx_measure_text_slice(ctx, "1", 1, gutter_ts, &num_w, &num_h);
        wlx_draw_text_range(ctx, "1", 1, 0, 1,
            gutter_rect.x + gutter_rect.w - gutter_pad - num_w, band.y, gutter_ts);
    }
    // Wrapped: only a hard line's first row carries its number;
    // continuation rows leave the gutter blank. Continuation rows
    // start mid-line, so the head test is the hard-line-start
    // predicate on the record's source start.
    size_t gutter_line = state->first_line
        + ((wrap && state->first_row > 0) ? 1 : 0);
    for (size_t i = 0; i < win_count; i++) {
        bool line_head = !wrap
            || wlx_text_hard_line_start_at(doc, doc_len, lines[i].source_start);
        size_t head_line = gutter_line;
        if (line_head) gutter_line++;
        if (!line_head) continue;
        if (!(lines[i].origin_y + line_h > band.y && lines[i].origin_y < band.y + band.h)) continue;
        int num_len = snprintf(num, sizeof(num), "%zu", head_line + 1);
        if (num_len <= 0) continue;
        float num_w = 0.0f, num_h = 0.0f;
        wlx_measure_text_slice(ctx, num, (size_t)num_len, gutter_ts, &num_w, &num_h);
        wlx_draw_text_range(ctx, num, (size_t)num_len, 0, (size_t)num_len,
            gutter_rect.x + gutter_rect.w - gutter_pad - num_w,
            lines[i].origin_y, gutter_ts);
    }
    wlx_scissor_scope_end(ctx, gutter_clip);
}

// Caret and scrollbars, drawn last from the final scroll offsets. The
// caret x comes from a cap-bounded prefix measure of its line; its y from
// the window records (wrapped) or index arithmetic against the anchor
// (linear); both clipped to the band.
static void wlx_editor_draw_carets_and_bars(const WLX_Editor_Frame *f,
    const WLX_Text_Line_Record *lines, size_t win_count,
    const WLX_Editor_Scroll *scr)
{
    WLX_Context *ctx = f->ctx;
    const WLX_Editor_Opt *opt = f->opt;
    WLX_Editor_State *state = f->state;
    bool focused = f->inter.focused;
    const WLX_Editor_Line_Index *idx = f->idx;
    WLX_Text_Geom_Store *geom = f->geom;
    const char *doc = f->doc;
    size_t doc_len = f->doc_len;
    WLX_Text_Style ts = f->ts;
    float tab_advance = f->tab_advance;
    float line_h = f->line_h;
    const WLX_Editor_Band *eb = &f->eb;
    bool wrap_geom = f->wrap_geom;
    const WLX_Text_Build_Inputs *wrap_inputs = f->wrap_inputs;
    float content_h = f->content_h;
    float scroll_y = scr->scroll_y;
    WLX_Rect band = eb->band;
    if (focused) {
        state->caret.cursor_blink_time += wlx_get_frame_time(ctx);
    }
    if (focused && idx != NULL && idx->count > 0 && line_h > 0.0f && opt->wrap) {
        // Wrapped caret: its row is one of the built window records
        // (first covering record wins, the same affinity as the
        // helpers); a caret outside the window is simply not drawn.
        if (wlx_text_caret_blink_on(state->caret.cursor_blink_time)
            && wrap_geom && win_count > 0 && lines != NULL) {
            for (size_t i = 0; i < win_count; i++) {
                if (!wlx_editor_wrap_caret_on_row(&lines[i], state->caret.cursor_pos, doc_len)) continue;
                size_t caret_line = wlx_editor_index_line_of(idx, lines[i].visible_start);
                float prefix = wlx_editor_wrap_caret_x(wrap_inputs, &lines[i],
                    wlx_editor_wrap_line_next(wrap_inputs, idx, caret_line),
                    state->caret.cursor_pos);
                float caret_px = lines[i].origin_x + prefix;
                if (prefix > 0.0f) caret_px += WLX_TEXT_CARET_PADDING;
                float caret_py = lines[i].origin_y;
                if (caret_px >= band.x && caret_px <= band.x + band.w
                    && caret_py + line_h > band.y && caret_py < band.y + band.h) {
                    wlx_text_caret_draw(ctx, band, caret_px,
                        caret_py + (line_h - (float)opt->font_size) * 0.5f,
                        (float)opt->font_size, opt->cursor_color);
                }
                break;
            }
        }
    } else if (focused && idx != NULL && idx->count > 0 && line_h > 0.0f) {
        size_t caret_line = wlx_editor_index_line_of(idx, state->caret.cursor_pos);
        size_t text_end = wlx_editor_line_text_end(doc, doc_len, idx, caret_line);
        size_t cx_off = state->caret.cursor_pos < text_end ? state->caret.cursor_pos : text_end;
        // The draw is a passive consumer: it must never move a settled
        // origin, or a caret parked outside the horizontal view would
        // ping-pong the origin against the window build every frame.
        float prefix = wlx_editor_caret_x(ctx, doc, doc_len, ts, tab_advance,
            geom, wlx_editor_line_next(idx, caret_line, doc_len),
            idx->offsets[caret_line], cx_off, false);
        float caret_px = band.x - state->scroll_x + prefix;
        if (prefix > 0.0f) caret_px += WLX_TEXT_CARET_PADDING;
        float caret_py = band.y
            + ((float)((long)caret_line - (long)state->first_line) - state->y_frac) * line_h;
        if (wlx_text_caret_blink_on(state->caret.cursor_blink_time)
            && caret_px >= band.x && caret_px <= band.x + band.w
            && caret_py + line_h > band.y && caret_py < band.y + band.h) {
            wlx_text_caret_draw(ctx, band, caret_px,
                caret_py + (line_h - (float)opt->font_size) * 0.5f,
                (float)opt->font_size, opt->cursor_color);
        }
    }

    if (eb->sb_visible) {
        wlx_scrollbar_thumb_draw(ctx,
            wlx_scrollbar_rect(eb->sb_track, content_h, scroll_y, eb->sb_w),
            state->caret.dragging_scrollbar);
    }
    // The thumb range is refreshed from the post-build state: scroll_x
    // and the max-seen width may both have changed since the
    // frame-start computation, and a stale range draws the thumb one
    // frame ahead of where the next frame maps it - a visible pulse on
    // every wheel notch.
    float h_content_w = wlx_editor_h_extent(state);
    if (h_content_w < state->scroll_x + band.w)
        h_content_w = state->scroll_x + band.w;
    // A live drag draws from the gesture's frozen range so the thumb
    // stays rigid under the pointer while scroll feedback shifts the
    // live range.
    if (state->dragging_hbar && state->hb_drag_content_w > band.w)
        h_content_w = state->hb_drag_content_w;
    if (eb->hb_visible && h_content_w > 0.0f) {
        wlx_scrollbar_thumb_draw(ctx,
            wlx_scrollbar_rect_h(eb->hb_track, band.w, h_content_w, state->scroll_x),
            state->dragging_hbar);
    }
}

// Pointer-driver hooks for the editor: hits resolve by index arithmetic
// (wrap: anchor-relative row walk; linear: line from the pixel scroll plus
// an on-demand record), never through the not-yet-built window, so presses
// and drags work anywhere in the document. The hooks close over the frame
// and its scroll transition.
typedef struct {
    const WLX_Editor_Frame *f;
    WLX_Editor_Scroll *scr;
} WLX_Editor_Mouse_User;

static size_t wlx_editor_mouse_hit(void *user, float x, float y) {
    WLX_Editor_Mouse_User *u = (WLX_Editor_Mouse_User *)user;
    const WLX_Editor_Frame *f = u->f;
    WLX_Rect band = f->eb.band;
    if (f->wrap_geom) {
        return wlx_editor_wrap_hit(f->wrap_inputs, f->idx, f->state->first_line,
            f->state->first_row, f->state->y_frac, band, f->line_h, x, y);
    }
    long hit_line = f->line_h > 0.0f
        ? (long)((u->scr->scroll_y + (y - band.y)) / f->line_h) : 0;
    if (hit_line < 0) hit_line = 0;
    if (hit_line >= (long)f->idx->count) hit_line = (long)f->idx->count - 1;
    float content_x = x - (band.x - f->state->scroll_x);
    if (content_x < 0.0f) content_x = 0.0f;
    return wlx_editor_offset_at_x(f->ctx, f->doc, f->doc_len, f->ts, f->line_h,
        f->tab_advance, f->idx, f->geom, (size_t)hit_line,
        f->state->scroll_x + band.w, band.w, content_x);
}

static void wlx_editor_mouse_word_bounds(void *user, size_t hit, size_t *start, size_t *end) {
    WLX_Editor_Mouse_User *u = (WLX_Editor_Mouse_User *)user;
    wlx_text_word_bounds(u->f->doc, u->f->doc_len, hit, start, end);
}

static void wlx_editor_mouse_auto_scroll(void *user, float over_x, float over_y, float dt) {
    WLX_Editor_Mouse_User *u = (WLX_Editor_Mouse_User *)user;
    const WLX_Editor_Frame *f = u->f;
    WLX_Editor_State *state = f->state;
    if (f->wrap_geom) {
        // Row-space auto-scroll: the overshoot accumulates through the
        // anchor fraction exactly like the wheel; overshoot past the end
        // is left for the bottom clamp.
        if (over_y != 0.0f && f->eb.wrap_overflow) {
            float px = state->y_frac * f->line_h
                + over_y * WLX_TEXT_DRAG_SCROLL_GAIN * dt;
            float steps = floorf(px / f->line_h);
            state->y_frac = (px - steps * f->line_h) / f->line_h;
            long left_over = wlx_editor_wrap_anchor_step(f->wrap_inputs, f->idx,
                &state->first_line, &state->first_row, (long)steps);
            if (left_over < 0) state->y_frac = 0.0f;
        }
    } else if (over_y != 0.0f && u->scr->max_scroll_y > 0.0f) {
        u->scr->scroll_y += over_y * WLX_TEXT_DRAG_SCROLL_GAIN * dt;
        u->scr->scroll_y = wlx_clampf(u->scr->scroll_y, 0.0f, u->scr->max_scroll_y);
        wlx_editor_anchor_set_scroll_y(u->f, u->scr->scroll_y);
    }
    if (over_x != 0.0f && u->scr->max_scroll_x > 0.0f) {
        state->scroll_x += over_x * WLX_TEXT_DRAG_SCROLL_GAIN * dt;
        state->scroll_x = wlx_clampf(state->scroll_x, 0.0f, u->scr->max_scroll_x);
    }
}

// Caret and selection input for a focused editor: the geometry-dependent
// motion keys plus the pointer driver, gated by the scrollbar-strip and
// gutter press exclusions. All positions come from index arithmetic and
// on-demand single-line records, never from the (not yet built) window,
// so motion and hit-tests work anywhere in the document. Returns true
// when the caret or the selection changed.
static bool wlx_editor_caret_input(const WLX_Editor_Frame *f, WLX_Editor_Scroll *scr)
{
    WLX_Context *ctx = f->ctx;
    WLX_Editor_State *state = f->state;
    WLX_Interaction inter = f->inter;
    const WLX_Editor_Line_Index *idx = f->idx;
    size_t doc_len = f->doc_len;
    float line_h = f->line_h;
    WLX_Rect input_rect = f->input_rect;
    const WLX_Editor_Band *eb = &f->eb;
    bool caret_moved = false;
    WLX_Rect band = eb->band;
    if (inter.focused && !inter.disabled && idx != NULL && idx->count > 0) {
        if (inter.just_focused) {
            state->caret.cursor_blink_time = 0.0f;
            state->caret.preferred_x_valid = false;
        }
        // The document may have changed under a held focus.
        if (state->caret.cursor_pos > doc_len) state->caret.cursor_pos = doc_len;
        if (state->caret.selection_anchor > doc_len) state->caret.selection_anchor = doc_len;

        size_t page_lines = line_h > 0.0f ? (size_t)(band.h / line_h) : 1;
        if (page_lines < 1) page_lines = 1;
        float motion_w = state->scroll_x + band.w;
        if (wlx_editor_handle_motion_keys(f, motion_w, page_lines)) {
            caret_moved = true;
        }

        // Mouse: click places the caret at the nearest boundary of the
        // line under the pointer (view arithmetic gives the line, one
        // on-demand record gives the column); repeated clicks widen to
        // word then all; dragging while held extends the selection.
        // Presses on the scrollbar strips belong to the thumb gestures.
        float mx = (float)ctx->input.mouse_x;
        float my = (float)ctx->input.mouse_y;
        bool sb_strip_hit = eb->sb_visible && wlx_rect_contains(
            (WLX_Rect){ eb->sb_track.x + eb->sb_track.w - eb->sb_w, eb->sb_track.y,
                        eb->sb_w, eb->sb_track.h }, mx, my);
        // The horizontal strip eats presses across the full widget
        // width and down to the widget's bottom edge: the corner
        // squares beside it and the border gap below it are not text,
        // and a press falling through would hit-test at the band's
        // bottom edge.
        bool hb_strip_hit = eb->hb_visible && wlx_rect_contains(
            (WLX_Rect){ input_rect.x, eb->hb_track.y, input_rect.w,
                        input_rect.y + input_rect.h - eb->hb_track.y }, mx, my);
        bool gutter_hit = eb->gutter_w > 0.0f && wlx_rect_contains(eb->gutter_rect, mx, my);
        bool press = ctx->input.mouse_clicked && !sb_strip_hit && !hb_strip_hit
            && !gutter_hit && !state->caret.dragging_scrollbar && !state->dragging_hbar
            && wlx_rect_contains(input_rect, mx, my);
        WLX_Editor_Mouse_User mouse_user = { .f = f, .scr = scr };
        if (wlx_text_edit_handle_mouse(ctx, &state->caret, band, doc_len,
                wlx_mod_down(ctx, WLX_MOD_SHIFT), press,
                &(WLX_Text_Mouse_Ops){
                    .user = &mouse_user,
                    .hit = wlx_editor_mouse_hit,
                    .word_bounds = wlx_editor_mouse_word_bounds,
                    .auto_scroll = wlx_editor_mouse_auto_scroll,
                    .clamp_drag_x = true,
                })) {
            caret_moved = true;
        }
    } else {
        state->caret.mouse_selecting = false;
    }
    return caret_moved;
}

static void wlx_resolve_opt_editor(const WLX_Context *ctx, WLX_Editor_Opt *opt) {
    const WLX_Theme *theme = ctx->theme;

    if (wlx_color_is_zero(opt->front_color))        opt->front_color        = theme->foreground;
    if (wlx_color_is_zero(opt->back_color))         opt->back_color         = theme->surface;
    if (wlx_is_negative_unset(opt->border_width))   opt->border_width       = theme->input.border_width;
    if (wlx_color_is_zero(opt->border_focus_color)) opt->border_focus_color = theme->input.border_focus;
    if (wlx_color_is_zero(opt->cursor_color))       opt->cursor_color       = theme->input.cursor;
    if (wlx_color_is_zero(opt->selection_color))    opt->selection_color    = theme->input.selection;
    if (wlx_color_is_zero(opt->selection_color)) {
        opt->selection_color = theme->accent;
        opt->selection_color.a = 90;
    }
    if (opt->tab_columns <= 0) opt->tab_columns = 4;

    wlx_resolve_typography(theme, &opt->font, &opt->font_size, &opt->min_height);
    wlx_resolve_border(theme, &opt->border_color, &opt->border_width, &opt->roundness, &opt->rounded_segments);

    WLX_RESOLVE_VISUAL_STATE(ctx, opt, opt->disabled,
        &opt->front_color, &opt->back_color, &opt->border_color,
        &opt->border_focus_color, &opt->cursor_color, &opt->selection_color,
        &opt->shadow_color, &opt->glow_color);
}

WLXDEF bool wlx_editor_impl(WLX_Context *ctx, const char *label, char *buffer, size_t buffer_cap,
    size_t *length, WLX_Editor_Opt opt, const char *file, int line)
{
    assert(ctx != NULL);
    assert(buffer != NULL && "editor buffer must not be NULL");
    assert(length != NULL && "editor length pointer must not be NULL");
    WLX_HARD_ASSERT(*length <= buffer_cap, "editor length exceeds buffer capacity");
    wlx_resolve_opt_editor(ctx, &opt);

    WLX_Resolved_Padding rp = WLX_RESOLVE_CONTENT_PADDING_EX(ctx, opt, WLX_INPUTBOX_CONTENT_PADDING);

    // Ensure height can fit the font plus content padding on both sides.
    float min_h = (float)opt.font_size + rp.top + rp.bottom + WLX_TEXT_FIELD_MIN_HEIGHT_SLACK;
    if (opt.height > 0 && opt.height < min_h) opt.height = min_h;

    // Prologue: compute widget frame and interaction state
    WLX_Widget_Frame frame = wlx_widget_frame_begin(ctx, opt.id, WLX_WIDGET_LAYOUT(opt), file, line);
    WLX_Rect wr = frame.rect;
    wlx_clamp_resolved_padding(&rp, wr.w, wr.h);

    WLX_State persistent = wlx_get_state_impl(ctx, sizeof(WLX_Editor_State), file, line);
    WLX_Editor_State *state = (WLX_Editor_State *)persistent.data;

    // A wrap-mode change remaps the scroll state: horizontal scroll only
    // exists unwrapped, the anchor row only wrapped, and the sticky
    // column's x is relative to a different origin in each mode.
    if (opt.wrap != state->last_wrap) {
        if (opt.wrap) state->scroll_x = 0.0f;
        else state->first_row = 0;
        state->caret.preferred_x_valid = false;
        state->last_wrap = opt.wrap;
    }

    // The interactive zone starts after the line-number gutter (previous
    // frame's width): gutter presses acquire no focus and place no caret.
    WLX_Rect inter_rect = wr;
    if (opt.line_numbers && state->gutter_end_x > inter_rect.x) {
        float cut = state->gutter_end_x - inter_rect.x;
        if (cut > inter_rect.w) cut = inter_rect.w;
        inter_rect.x += cut;
        inter_rect.w -= cut;
    }
    bool ring_seen_before = ctx->interaction.focus_id_seen;
    WLX_Interaction inter = wlx_get_interaction_for(
        ctx, inter_rect,
        WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS | WLX_INTERACT_FOCUS_HOLD_ENTER
            | WLX_INTERACT_FOCUS_HOLD_TAB | WLX_INTERACT_TEXT_CURSOR,
        opt.disabled, file, line);
    // This query holds the keyboard focus ring: recorded off the gutter-
    // excluded hit zone above, the ring would cut through the widget at the
    // gutter edge, so re-record it over the full editor rect.
    if (ctx->interaction.focus_id_seen && !ring_seen_before) {
        wlx_focus_ring_rect(ctx, wr);
    }

    wlx_text_edit_tick_click_clock(ctx, &state->caret);

    bool changed = false;
    if (opt.out_focused != NULL) *opt.out_focused = inter.focused;

    // Editing runs before any geometry so this frame's index, window, and
    // caret all reflect the mutated buffer. The widget maintains a trailing
    // NUL opportunistically when the capacity allows one. kb_caret_changed
    // carries the handler's caret/selection changes (arrows, collapse,
    // select-all) into blink reset and caret-follow below.
    bool kb_caret_changed = false;
    WLX_Text_Edit_Span edit_span = {0};
    size_t pre_edit_len = *length;
    if (inter.focused) {
        size_t pre_cursor = state->caret.cursor_pos;
        size_t pre_anchor = state->caret.selection_anchor;
        // The undo journal is found by widget id under the caller's
        // revision, so history the buffer outgrew is dropped before the
        // chords could replay it. Undo and redo replay through the same
        // primitives as a keystroke, so the edit span below covers them.
        WLX_Text_Undo_Journal *undo = wlx_text_undo_get(ctx, persistent.id, *length,
            opt.revision, false);
        changed = wlx_text_edit_handle_keys(ctx, &state->caret, buffer, buffer_cap, length,
            (WLX_Text_Edit_Caps){ .read_only = opt.read_only,
                                  .allow_newline = true, .allow_tab = true,
                                  .word_delete = true }, &edit_span, undo);
        if (changed && *length < buffer_cap) buffer[*length] = '\0';
        kb_caret_changed = pre_cursor != state->caret.cursor_pos
            || pre_anchor != state->caret.selection_anchor;
    }

    // Optional leading label, placed by the vertical component of opt.content_align
    // exactly like the inputbox's (the label never wraps).
    WLX_Text_Style ts = { .font = opt.font, .font_size = opt.font_size, .color = opt.front_color, .spacing = opt.spacing };
    WLX_Text_Field_Chrome chrome = WLX_TEXT_FIELD_CHROME(opt);
    WLX_Text_Field_Frame field = wlx_text_field_frame(ctx, wr, rp, label, ts,
        opt.content_align, false, inter.hover, inter.focused, inter.disabled, &chrome);
    WLX_Rect input_rect = field.input_rect;

    if (opt.font_size > 0) {
        const char *doc = buffer;
        size_t doc_len = *length;

        float ref_w = 0.0f;
        float line_h = wlx_text_line_height(ctx, ts, &ref_w);

        // Next-tab-stop advance: tab_columns times the space advance.
        float tab_advance = ref_w > 0.0f
            ? (float)opt.tab_columns * ref_w
            : (float)opt.tab_columns * (float)opt.font_size * 0.5f;

        float border_inset = opt.border_width > 0 ? opt.border_width + 1.0f : 0.0f;

        // Line index: rebuilt on first sight and whenever the guard detects a
        // document change; untouched on idle frames so no O(document) work
        // runs outside edits.
        WLX_Editor_Line_Index *idx = wlx_editor_index_get(ctx, persistent.id);
        WLX_Text_Geom_Store *geom = idx != NULL ? &idx->geom : NULL;
        bool doc_rebuilt = false;
        if (idx != NULL) {
            // The retained geometry may survive a rebuild only when the
            // rebuild's one cause is this frame's widget edit: the edit
            // span then shifts entry keys precisely. Any other signal
            // (first sight, external length/revision change, probe fail)
            // clears the store outright.
            bool edit_only = changed && edit_span.edited && state->index_seen
                && state->guard_length == pre_edit_len
                && state->guard_revision == opt.revision
                && pre_edit_len - (edit_span.old_end - edit_span.start)
                    + (edit_span.new_end - edit_span.start) == doc_len;
            bool stale = !state->index_seen || changed
                || state->guard_length != doc_len
                || state->guard_revision != opt.revision
                || !wlx_editor_index_probe_ok(idx, doc, doc_len);
            if (stale && wlx_editor_index_rebuild(idx, doc, doc_len)) {
                state->index_seen = true;
                state->guard_length = doc_len;
                state->guard_revision = opt.revision;
                doc_rebuilt = true;
                if (edit_only) {
                    wlx_text_geom_edit_shift(geom, edit_span.start,
                        edit_span.old_end, edit_span.new_end);
                    // The wrapped bottom anchor survives an edit ending
                    // before its line starts: the bytes from that line
                    // to the document end are untouched and its
                    // end-relative line distance still holds, so only
                    // its start byte shifts by the edit's delta.
                    if (state->bottom_valid && edit_span.old_end < state->bottom_start_off) {
                        state->bottom_start_off = state->bottom_start_off
                            - edit_span.old_end + edit_span.new_end;
                    } else {
                        state->bottom_valid = false;
                    }
                } else {
                    wlx_text_geom_clear(geom);
                    state->bottom_valid = false;
                    // A rebuild the widget's own edit did not cause means
                    // the document changed under the journal (the probe
                    // catches in-place rewrites the length guard cannot
                    // see): its history goes with the geometry.
                    wlx_text_undo_clear_if_present(ctx, persistent.id, doc_len);
                }
#ifdef WLX_DEBUG
                assert(wlx_editor_index_probe_ok(idx, doc, doc_len));
#endif
            }
        }
        size_t line_count = idx != NULL ? idx->count : 0;
        float content_h = (float)line_count * line_h;

        WLX_Editor_Band eb = wlx_editor_resolve_band(ctx, &opt, state, idx, doc, doc_len,
            ts, line_h, tab_advance, input_rect, border_inset, line_count, content_h);
        WLX_Rect band = eb.band;
        float gutter_w = eb.gutter_w;
        float win_band_h = eb.win_band_h;

        // Retained-geometry environment: checked at the final band, before
        // any consumer touches the store this frame. The sizing target is
        // twice the window so scroll and caret lookups stay resident.
        if (geom != NULL) {
            WLX_Text_Geom_Env genv;
            memset(&genv, 0, sizeof(genv));
            genv.font = ts.font;
            genv.font_size = ts.font_size;
            genv.spacing = ts.spacing;
            genv.tab_advance = tab_advance;
            genv.band_w = band.w;
            genv.line_h = line_h;
            genv.wrap = opt.wrap;
            genv.transform_generation = ctx->style_transform_generation;
            size_t vp_lines = line_h > 0.0f
                ? (size_t)(win_band_h / line_h) + 2 + WLX_EDITOR_OVERSCAN_LINES : 8;
            size_t want = vp_lines * WLX_TEXT_GEOM_STORE_SLACK < WLX_TEXT_GEOM_STORE_MIN
                ? (size_t)WLX_TEXT_GEOM_STORE_MIN
                : vp_lines * WLX_TEXT_GEOM_STORE_SLACK;
            // An environment change also drops the wrapped bottom
            // anchor: its row counts were taken under the old one. (The
            // wrapped mode implies a store - it is embedded in the index
            // the mode requires - so this is the one invalidation site.)
            if (!geom->env_seen || !wlx_text_geom_env_equal(&geom->env, &genv))
                state->bottom_valid = false;
            wlx_text_geom_env_check(geom, &genv, want);
        }

        // Wrapped geometry inputs at the final band width, with the anchor
        // clamped against the current index and the anchor line's current
        // row count - both may have changed since the anchor was written.
        // The row memo lives on this frame's stack: the strips above fixed
        // the band width and the edits ran before any geometry, so every
        // row count below sees one immutable (document, width, style).
        WLX_Wrap_Row_Memo row_memo = {0};
        WLX_Text_Build_Inputs wrap_inputs = {0};
        size_t anchor_rows = 1;
        bool wrap_geom = opt.wrap && idx != NULL && line_count > 0 && line_h > 0.0f;
        if (wrap_geom) {
            wrap_inputs = wlx_editor_wrap_build_inputs(ctx, doc, doc_len, ts, band.w,
                line_h, tab_advance);
            wrap_inputs.row_memo = &row_memo;
            wrap_inputs.geom = geom;
            if (state->first_line >= line_count) {
                state->first_line = line_count - 1;
                state->first_row = 0;
                state->y_frac = 0.0f;
            }
            anchor_rows = wlx_editor_wrap_line_rows(&wrap_inputs, idx, state->first_line);
            if (state->first_row >= anchor_rows) state->first_row = anchor_rows - 1;
        }

        // The frame: every input above is now fixed for the rest of the
        // widget, so the downstream phases read one struct. The scroll
        // transition rides beside it, phase by phase.
        WLX_Editor_Frame f = {
            .ctx = ctx, .opt = &opt, .state = state, .idx = idx, .geom = geom,
            .doc = doc, .doc_len = doc_len, .ts = ts, .line_h = line_h,
            .tab_advance = tab_advance, .input_rect = input_rect, .inter = inter,
            .eb = eb, .wrap_inputs = &wrap_inputs, .wrap_geom = wrap_geom,
            .line_count = line_count, .content_h = content_h,
        };
        WLX_Editor_Scroll scr = {0};
        scr.anchor_rows = anchor_rows;

        wlx_editor_scroll_limits(&f, &scr);

        wlx_editor_scroll_resolve(&f, &scr);

        // Caret and selection input. Seeded with the shared key handler's
        // caret changes so they reach caret-follow even when only the
        // anchor moved.
        bool caret_moved = kb_caret_changed;
        if (wlx_editor_caret_input(&f, &scr)) {
            caret_moved = true;
        }

        // Caret-follow, both axes: a caret change drags the view the minimal
        // distance that puts the caret back inside the band. It never
        // mutates the caret or the selection; wheel scrolling may park the
        // caret outside the window, and the next caret change snaps back.
        if (inter.focused && idx != NULL && idx->count > 0 && wrap_geom
            && !state->caret.dragging_scrollbar
            && (caret_moved || changed || state->caret.cursor_pos != state->caret.prev_cursor_pos)) {
            // The drag gate keeps an active thumb drag authoritative
            // over a same-frame caret change (typing mid-drag), the
            // ordering the deleted end-of-frame write-back used to
            // enforce by overwriting this follow's anchor.
            wlx_editor_caret_follow_wrapped(&f);
        } else if (inter.focused && idx != NULL && idx->count > 0 && !opt.wrap
            && (caret_moved || changed || state->caret.cursor_pos != state->caret.prev_cursor_pos)) {
            wlx_editor_caret_follow_linear(&f, &scr);
        }
        if (inter.focused) state->caret.prev_cursor_pos = state->caret.cursor_pos;
        if (caret_moved || changed) state->caret.cursor_blink_time = 0.0f;

        // The document changed this frame: release the sticky width reach
        // so a deleted widest line stops holding the horizontal bar open.
        // The release sits after the scroll clamps and gestures, which run
        // against the previous reach one final time - released any earlier,
        // the clamp would see a zeroed reach, force scroll_x to zero, and
        // snap an h-scrolled view left mid-edit while the bar blinks off
        // for the frame. The window build below re-grows the reach from the
        // visible lines immediately, so the next frame's clamp and bar
        // visibility work from re-measured widths. scroll_x itself is left
        // alone: caret-follow has already re-exposed a caret deep in a long
        // line, and re-measurement decides whether the scroll still holds.
        if (doc_rebuilt) {
            state->max_line_w = 0.0f;
            state->h_reach_open = false;
        }

        // Window build: only the visible lines (plus overscan) become line
        // records, re-anchored to the band. Frame cost is O(viewport)
        // whatever the document size.
        size_t win_count = 0;
        WLX_Text_Line_Record *lines = NULL;
        if (wrap_geom && band.w > 0.0f && band.h > 0.0f) {
            win_count = wlx_editor_window_build_wrapped(&f, &scr, &lines);
        } else if (!opt.wrap && idx != NULL && doc_len > 0 && line_count > 0
            && band.w > 0.0f && band.h > 0.0f
            && state->first_line < line_count) {
            win_count = wlx_editor_window_build_linear(&f, &lines);
        }

        // Selection highlight under the text, drawn for the visible window
        // records only (their visible ranges bound every measure).
        if (win_count > 0 && inter.focused
            && wlx_text_edit_has_selection(&state->caret)) {
            size_t sel_min = wlx_text_edit_selection_min(&state->caret);
            size_t sel_max = wlx_text_edit_selection_max(&state->caret);
            wlx_text_draw_selection(ctx, band, doc, doc_len, ts, tab_advance,
                idx, geom, lines, win_count, sel_min, sel_max, opt.selection_color);
        }

        if (win_count > 0) {
            wlx_editor_draw_window(&f, lines, win_count);
        }

        if (gutter_w > 0.0f) {
            wlx_editor_draw_gutter(&f, lines, win_count);
        }

        wlx_editor_draw_carets_and_bars(&f, lines, win_count, &scr);
    }

    // Epilogue: close widget frame
    wlx_widget_frame_end(ctx, frame);
    return changed;
}

WLXDEF WLX_Editor_Opt wlx_editor_opt_defaults(void) { return wlx_default_editor_opt(); }

#endif // WOLLIX_IMPLEMENTATION

#endif // WOLLIX_EDITOR_H_
