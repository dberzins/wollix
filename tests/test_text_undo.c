// test_text_undo.c - text undo journal: the context-owned per-widget history
// the shared edit primitives record into, and the undo/redo chords that
// replay it. Recording is exercised through the inputbox and textarea: one
// entry per primitive mutation with exact ranges, removed bytes kept in the
// arena, caret pairs before and after; the staleness guard (length and
// revision); the password and unfocused exclusions; the entry and byte caps
// with whole-step eviction and oversize admission; then the chords: typed,
// Backspace and Delete runs coalesce into one step, caret moves and the
// single-step classes (Enter, paste, cut, word and selection deletes) split
// steps, undo restores bytes and the caret pair, redo mirrors it, a new
// edit clears redo, a replay ends coalescing, held chords repeat, and
// read-only, password and stale buffers reject the chords.

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
// Fixture: one inputbox at a stable call site. Password mode and the
// caller revision are fixture globals so one widget id can switch modes
// between frames.
// ============================================================================

static uint32_t tu_command_mod(void) {
#if defined(__APPLE__)
    return WLX_MOD_SUPER;
#else
    return WLX_MOD_CTRL;
#endif
}

static bool tu_password = false;
static bool tu_multiline = false;
static bool tu_read_only = false;
static uint32_t tu_revision = 0;
static bool tu_last_changed = false;   // the widget's "text changed" return

static void tu_inputbox(WLX_Context *ctx, char *buf, size_t buf_size) {
    tu_last_changed = wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .wrap = false, .border_width = 0, .password = tu_password,
            .multiline = tu_multiline, .read_only = tu_read_only,
            .revision = tu_revision),
        __FILE__, __LINE__);
}

static void tu_frame_ex(WLX_Context *ctx, char *buf, size_t buf_size,
                        int mx, int my, bool down, bool clicked,
                        WLX_Key_Code key, uint32_t mods, const char *text) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    if (key != WLX_KEY_NONE) keys_pressed[key] = true;
    test_frame_begin_full(ctx, mx, my, down, clicked, down, 0.0f, NULL,
        key != WLX_KEY_NONE ? keys_pressed : NULL, NULL, mods, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    tu_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void tu_focus(WLX_Context *ctx, char *buf, size_t buf_size) {
    tu_frame_ex(ctx, buf, buf_size, 200, 150, true, true, WLX_KEY_NONE, 0, NULL);
}

static void tu_idle(WLX_Context *ctx, char *buf, size_t buf_size) {
    tu_frame_ex(ctx, buf, buf_size, 200, 150, false, false, WLX_KEY_NONE, 0, NULL);
}

static void tu_type(WLX_Context *ctx, char *buf, size_t buf_size, const char *text) {
    tu_frame_ex(ctx, buf, buf_size, 200, 150, false, false, WLX_KEY_NONE, 0, text);
}

static void tu_key(WLX_Context *ctx, char *buf, size_t buf_size,
                   WLX_Key_Code key, uint32_t mods) {
    tu_frame_ex(ctx, buf, buf_size, 200, 150, false, false, key, mods, NULL);
}

// Two keys in one frame, e.g. select-all plus Backspace, so the whole
// buffer goes in a single handler pass: between frames the inputbox pins
// a caret beyond its visible-width text run back to the run's end, so a
// two-frame sequence would delete only the visible prefix of a long line.
static void tu_keys2(WLX_Context *ctx, char *buf, size_t buf_size,
                     WLX_Key_Code key1, WLX_Key_Code key2, uint32_t mods) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[key1] = true;
    keys_pressed[key2] = true;
    test_frame_begin_full(ctx, 200, 150, false, false, false, 0.0f, NULL,
        keys_pressed, NULL, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    tu_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// The fixture widget's journal (the context holds one widget), or NULL.
static WLX_Text_Undo_Journal *tu_journal(WLX_Context *ctx) {
    return ctx->text_undo.count > 0 ? &ctx->text_undo.items[0] : NULL;
}

// Structural invariant of one stack: removed bytes lie in entry order,
// contiguous from the arena start, and account for every byte in use.
static bool tu_stack_consistent(const WLX_Text_Undo_Stack *s) {
    size_t off = 0;
    for (size_t i = 0; i < s->count; i++) {
        if (s->entries[i].arena_off != off) return false;
        off += s->entries[i].removed_len;
    }
    return off == s->arena_used && s->arena_used <= s->arena_cap
        && s->count <= s->cap;
}

static void tu_reset_fixture(void) {
    tu_password = false;
    tu_multiline = false;
    tu_read_only = false;
    tu_revision = 0;
    tu_last_changed = false;
}

// A held chord: the key arrives as an OS repeat, not a fresh press.
static void tu_key_repeat(WLX_Context *ctx, char *buf, size_t buf_size,
                          WLX_Key_Code key, uint32_t mods) {
    bool keys_repeated[WLX_KEY_COUNT] = {0};
    keys_repeated[key] = true;
    test_frame_begin_full(ctx, 200, 150, false, false, false, 0.0f, NULL,
        NULL, keys_repeated, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    tu_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// The fixture widget's persistent state (the context holds one widget).
static WLX_Inputbox_State *tu_state(WLX_Context *ctx) {
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        if (ctx->states.slots[i].id != 0) {
            return (WLX_Inputbox_State *)ctx->states.slots[i].data;
        }
    }
    return NULL;
}

static uint32_t tu_undo_mod(void) { return tu_command_mod(); }
static uint32_t tu_redo_mod(void) { return tu_command_mod() | WLX_MOD_SHIFT; }

// ============================================================================
// Recording
// ============================================================================

TEST(undo_journal_records_one_entry_per_primitive) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "";

    tu_focus(&ctx, buf, sizeof(buf));
    tu_type(&ctx, buf, sizeof(buf), "ab");
    ASSERT_EQ_STR(buf, "ab");
    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(1, (int)j->undo.count);
    ASSERT_EQ_INT(0, (int)j->undo.entries[0].start);
    ASSERT_EQ_INT(0, (int)j->undo.entries[0].removed_len);
    ASSERT_EQ_INT(2, (int)j->undo.entries[0].inserted_len);
    ASSERT_EQ_INT(0, (int)j->undo.entries[0].caret_before);
    ASSERT_EQ_INT(0, (int)j->undo.entries[0].anchor_before);
    ASSERT_EQ_INT(2, (int)j->undo.entries[0].caret_after);
    ASSERT_EQ_INT(2, (int)j->undo.entries[0].anchor_after);
    ASSERT_EQ_INT(2, (int)j->expected_len);

    // A second typing frame at the run's end joins the step in place.
    tu_type(&ctx, buf, sizeof(buf), "c");
    ASSERT_EQ_STR(buf, "abc");
    ASSERT_EQ_INT(1, (int)j->undo.count);
    ASSERT_EQ_INT(3, (int)j->undo.entries[0].inserted_len);
    ASSERT_EQ_INT(3, (int)j->undo.entries[0].caret_after);
    ASSERT_EQ_INT(3, (int)j->expected_len);

    // A caret move in between makes the next primitive its own record.
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, 0);
    tu_type(&ctx, buf, sizeof(buf), "d");
    ASSERT_EQ_STR(buf, "abdc");
    ASSERT_EQ_INT(2, (int)j->undo.count);
    ASSERT_EQ_INT(2, (int)j->undo.entries[1].start);
    ASSERT_EQ_INT(0, (int)j->undo.entries[1].removed_len);
    ASSERT_EQ_INT(1, (int)j->undo.entries[1].inserted_len);
    ASSERT_TRUE(j->undo.entries[1].group != j->undo.entries[0].group);
    ASSERT_EQ_INT(4, (int)j->expected_len);
    ASSERT_EQ_INT(0, (int)j->redo.count);
    ASSERT_TRUE(tu_stack_consistent(&j->undo));

    // Idle focused frames record nothing.
    tu_idle(&ctx, buf, sizeof(buf));
    ASSERT_EQ_INT(2, (int)j->undo.count);
    wlx_context_destroy(&ctx);
}

TEST(undo_journal_keeps_removed_bytes_and_caret_pairs) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "hello";

    tu_focus(&ctx, buf, sizeof(buf));                       // caret at 5
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);  // [3,5) selected
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "hel");

    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(1, (int)j->undo.count);
    const WLX_Text_Undo_Entry *e = &j->undo.entries[0];
    ASSERT_EQ_INT(3, (int)e->start);
    ASSERT_EQ_INT(2, (int)e->removed_len);
    ASSERT_EQ_INT(0, (int)e->inserted_len);
    ASSERT_EQ_INT(3, (int)e->caret_before);
    ASSERT_EQ_INT(5, (int)e->anchor_before);
    ASSERT_EQ_INT(3, (int)e->caret_after);
    ASSERT_EQ_INT(3, (int)e->anchor_after);
    ASSERT_EQ_INT(2, (int)j->undo.arena_used);
    ASSERT_TRUE(memcmp(j->undo.arena, "lo", 2) == 0);

    // Typing over a selection is a delete record followed by an insert
    // record; the removed bytes append to the arena.
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);  // [1,3) selected
    tu_type(&ctx, buf, sizeof(buf), "X");
    ASSERT_EQ_STR(buf, "hX");
    ASSERT_EQ_INT(3, (int)j->undo.count);
    e = &j->undo.entries[1];
    ASSERT_EQ_INT(1, (int)e->start);
    ASSERT_EQ_INT(2, (int)e->removed_len);
    ASSERT_EQ_INT(0, (int)e->inserted_len);
    ASSERT_EQ_INT(1, (int)e->caret_before);
    ASSERT_EQ_INT(3, (int)e->anchor_before);
    ASSERT_EQ_INT(1, (int)e->caret_after);
    ASSERT_EQ_INT(1, (int)e->anchor_after);
    e = &j->undo.entries[2];
    ASSERT_EQ_INT(1, (int)e->start);
    ASSERT_EQ_INT(0, (int)e->removed_len);
    ASSERT_EQ_INT(1, (int)e->inserted_len);
    ASSERT_EQ_INT(1, (int)e->caret_before);
    ASSERT_EQ_INT(1, (int)e->anchor_before);
    ASSERT_EQ_INT(2, (int)e->caret_after);
    ASSERT_EQ_INT(2, (int)e->anchor_after);
    ASSERT_EQ_INT(4, (int)j->undo.arena_used);
    ASSERT_TRUE(memcmp(j->undo.arena, "loel", 4) == 0);
    ASSERT_TRUE(tu_stack_consistent(&j->undo));
    ASSERT_EQ_INT(2, (int)j->expected_len);
    wlx_context_destroy(&ctx);
}

TEST(undo_journal_insert_records_bytes_after_utf8_backoff) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[4] = "";   // capacity 3 bytes of text

    tu_focus(&ctx, buf, sizeof(buf));
    tu_type(&ctx, buf, sizeof(buf), "\xC3\xA9");            // e-acute, 2 bytes
    ASSERT_EQ_STR(buf, "\xC3\xA9");
    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(1, (int)j->undo.count);
    ASSERT_EQ_INT(2, (int)j->undo.entries[0].inserted_len);

    // One byte of room: a 3-byte codepoint backs off to nothing and leaves
    // no record; "a" followed by a 2-byte codepoint lands "a" alone.
    tu_type(&ctx, buf, sizeof(buf), "\xE2\x82\xAC");
    ASSERT_EQ_STR(buf, "\xC3\xA9");
    ASSERT_EQ_INT(1, (int)j->undo.count);
    tu_type(&ctx, buf, sizeof(buf), "a\xC3\xA9");
    ASSERT_EQ_STR(buf, "\xC3\xA9" "a");
    ASSERT_EQ_INT(1, (int)j->undo.count);      // joined the typing run
    ASSERT_EQ_INT(3, (int)j->undo.entries[0].inserted_len);
    ASSERT_EQ_INT(3, (int)j->expected_len);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Exclusions and the staleness guard
// ============================================================================

TEST(undo_journal_absent_without_focus_and_for_password_fields) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "";

    // Unfocused frames, even with text staged, never look a journal up.
    tu_idle(&ctx, buf, sizeof(buf));
    tu_frame_ex(&ctx, buf, sizeof(buf), -50, -50, false, false, WLX_KEY_NONE, 0, "zz");
    ASSERT_EQ_STR(buf, "");
    ASSERT_EQ_INT(0, (int)ctx.text_undo.count);

    // A password field edits normally but keeps no history.
    tu_password = true;
    tu_focus(&ctx, buf, sizeof(buf));
    tu_type(&ctx, buf, sizeof(buf), "ab");
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_EQ_INT(0, (int)ctx.text_undo.count);

    // Plain again: history starts; flipping back to password releases it.
    tu_password = false;
    tu_type(&ctx, buf, sizeof(buf), "c");
    ASSERT_EQ_STR(buf, "abc");
    ASSERT_EQ_INT(1, (int)ctx.text_undo.count);
    ASSERT_EQ_INT(1, (int)tu_journal(&ctx)->undo.count);
    tu_password = true;
    tu_idle(&ctx, buf, sizeof(buf));
    ASSERT_EQ_INT(0, (int)ctx.text_undo.count);
    wlx_context_destroy(&ctx);
}

TEST(undo_journal_guard_drops_history_on_external_change) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "";

    tu_focus(&ctx, buf, sizeof(buf));
    tu_type(&ctx, buf, sizeof(buf), "ab");
    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(1, (int)j->undo.count);

    // A length change outside the widget drops the history and re-arms
    // the guard at the new length.
    strcat(buf, "Z");
    tu_idle(&ctx, buf, sizeof(buf));
    ASSERT_EQ_INT(0, (int)j->undo.count);
    ASSERT_EQ_INT(3, (int)j->expected_len);

    // Same length and revision keep it; a revision bump drops it.
    tu_type(&ctx, buf, sizeof(buf), "c");
    ASSERT_EQ_INT(1, (int)j->undo.count);
    tu_idle(&ctx, buf, sizeof(buf));
    ASSERT_EQ_INT(1, (int)j->undo.count);
    tu_revision = 1;
    tu_idle(&ctx, buf, sizeof(buf));
    ASSERT_EQ_INT(0, (int)j->undo.count);
    tu_type(&ctx, buf, sizeof(buf), "d");
    tu_idle(&ctx, buf, sizeof(buf));
    ASSERT_EQ_INT(1, (int)j->undo.count);
    // The caret never moved off 2, so the typed bytes landed before the
    // externally appended "Z".
    ASSERT_EQ_STR(buf, "abcdZ");
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Caps and eviction
// ============================================================================

TEST(undo_journal_evicts_oldest_steps_at_the_entry_cap) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[1024] = "";

    tu_focus(&ctx, buf, sizeof(buf));
    // Alternating classes never coalesce: every keystroke is its own step.
    const int steps = WLX_TEXT_UNDO_ENTRIES + 8;
    for (int i = 0; i < steps; i += 2) {
        tu_type(&ctx, buf, sizeof(buf), "x");
        tu_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);
    }
    ASSERT_EQ_STR(buf, "");

    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(WLX_TEXT_UNDO_ENTRIES, (int)j->undo.count);
    ASSERT_EQ_INT(WLX_TEXT_UNDO_ENTRIES, (int)j->undo.cap);
    // Step ids run 1..steps; the eight oldest were evicted whole, and half
    // of the survivors are one-byte Backspace steps.
    ASSERT_EQ_INT(9, (int)j->undo.entries[0].group);
    ASSERT_EQ_INT(steps, (int)j->undo.entries[j->undo.count - 1].group);
    ASSERT_EQ_INT(WLX_TEXT_UNDO_ENTRIES / 2, (int)j->undo.arena_used);
    ASSERT_TRUE(tu_stack_consistent(&j->undo));
    wlx_context_destroy(&ctx);
}

TEST(undo_journal_evicts_by_bytes_and_admits_an_oversize_step) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();

    // Byte cap: paste-and-delete cycles of a 2 KB clip make two records
    // each, a bytes-free insert and a 2 KB delete. The arena is exactly
    // full after 128 deletes; every later delete evicts the oldest steps
    // (one cycle's pair) while the arena stays within the cap.
    char clip[2049];
    memset(clip, 'q', sizeof(clip) - 1);
    clip[sizeof(clip) - 1] = '\0';
    test_set_clipboard(clip);
    char buf[4096] = "";

    tu_focus(&ctx, buf, sizeof(buf));
    const int cycles = 140;
    for (int i = 0; i < cycles; i++) {
        tu_key(&ctx, buf, sizeof(buf), WLX_KEY_V, tu_command_mod());
        ASSERT_EQ_INT(2048, (int)strlen(buf));
        tu_keys2(&ctx, buf, sizeof(buf), WLX_KEY_A, WLX_KEY_BACKSPACE, tu_command_mod());
        ASSERT_EQ_STR(buf, "");
    }
    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(WLX_TEXT_UNDO_BYTES, (long)j->undo.arena_used);
    ASSERT_EQ_INT(WLX_TEXT_UNDO_BYTES, (long)j->undo.arena_cap);
    ASSERT_EQ_INT(256, (int)j->undo.count);
    // Steps run 1..280 (two per cycle); cycles 1..12 were evicted whole.
    ASSERT_EQ_INT(25, (int)j->undo.entries[0].group);
    ASSERT_EQ_INT(2 * cycles, (int)j->undo.entries[j->undo.count - 1].group);
    ASSERT_TRUE(tu_stack_consistent(&j->undo));

    // Oversize: one step removing more than the byte cap is admitted; it
    // evicts everything older and the arena grows to hold exactly it.
    const size_t big = 300000;
    char *doc = (char *)malloc(big + 64);
    char *snap = (char *)malloc(big + 64);
    ASSERT_TRUE(doc != NULL && snap != NULL);
    memset(doc, 'x', big);
    doc[big] = '\0';
    tu_idle(&ctx, doc, big + 64);            // a new document: the guard drops the old history
    ASSERT_EQ_INT(0, (int)j->undo.count);
    tu_type(&ctx, doc, big + 64, "y");       // an older step to evict
    ASSERT_EQ_INT((long)(big + 1), (long)strlen(doc));
    ASSERT_EQ_INT(1, (int)j->undo.count);
    memcpy(snap, doc, big + 1);
    tu_keys2(&ctx, doc, big + 64, WLX_KEY_A, WLX_KEY_BACKSPACE, tu_command_mod());
    ASSERT_EQ_STR(doc, "");
    ASSERT_EQ_INT(1, (int)j->undo.count);
    ASSERT_EQ_INT(0, (int)j->undo.entries[0].start);
    ASSERT_EQ_INT((long)(big + 1), (long)j->undo.entries[0].removed_len);
    ASSERT_EQ_INT((long)(big + 1), (long)j->undo.arena_used);
    ASSERT_EQ_INT((long)(big + 1), (long)j->undo.arena_cap);
    ASSERT_TRUE(memcmp(j->undo.arena, snap, big + 1) == 0);
    ASSERT_TRUE(tu_stack_consistent(&j->undo));
    ASSERT_EQ_INT(0, (int)j->expected_len);

    wlx_context_destroy(&ctx);
    ASSERT_EQ_INT(0, (int)ctx.text_undo.count);
    ASSERT_TRUE(ctx.text_undo.items == NULL);
    free(doc);
    free(snap);
}


// ============================================================================
// Chords: coalescing, single steps, redo
// ============================================================================

TEST(undo_typing_run_is_one_step_and_redo_restores) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "";

    tu_focus(&ctx, buf, sizeof(buf));
    tu_type(&ctx, buf, sizeof(buf), "a");
    tu_type(&ctx, buf, sizeof(buf), "b");
    tu_type(&ctx, buf, sizeof(buf), "c");
    ASSERT_EQ_STR(buf, "abc");
    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(1, (int)j->undo.count);
    ASSERT_EQ_INT(3, (int)j->undo.entries[0].inserted_len);
    ASSERT_EQ_INT(0, (int)j->undo.entries[0].caret_before);
    ASSERT_EQ_INT(3, (int)j->undo.entries[0].caret_after);

    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "");
    ASSERT_TRUE(tu_last_changed);
    ASSERT_EQ_INT(0, (int)tu_state(&ctx)->caret.cursor_pos);
    ASSERT_EQ_INT(0, (int)tu_state(&ctx)->caret.selection_anchor);
    ASSERT_EQ_INT(0, (int)j->undo.count);
    ASSERT_EQ_INT(1, (int)j->redo.count);
    ASSERT_EQ_INT(3, (int)j->redo.entries[0].removed_len);
    ASSERT_TRUE(memcmp(j->redo.arena, "abc", 3) == 0);
    ASSERT_TRUE(tu_stack_consistent(&j->redo));

    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_redo_mod());
    ASSERT_EQ_STR(buf, "abc");
    ASSERT_EQ_INT(3, (int)tu_state(&ctx)->caret.cursor_pos);
    ASSERT_EQ_INT(3, (int)tu_state(&ctx)->caret.selection_anchor);
    ASSERT_EQ_INT(1, (int)j->undo.count);
    ASSERT_EQ_INT(0, (int)j->redo.count);

    // command+Y redoes exactly like command+Shift+Z; an empty redo stack
    // changes nothing and reports nothing.
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Y, tu_undo_mod());
    ASSERT_EQ_STR(buf, "abc");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Y, tu_undo_mod());
    ASSERT_EQ_STR(buf, "abc");
    ASSERT_FALSE(tu_last_changed);
    wlx_context_destroy(&ctx);
}

TEST(undo_backspace_and_delete_runs_are_single_steps) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "hello";

    tu_focus(&ctx, buf, sizeof(buf));                       // caret at 5
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "he");
    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(1, (int)j->undo.count);
    ASSERT_EQ_INT(2, (int)j->undo.entries[0].start);
    ASSERT_EQ_INT(3, (int)j->undo.entries[0].removed_len);
    ASSERT_TRUE(memcmp(j->undo.arena, "llo", 3) == 0);
    ASSERT_EQ_INT(5, (int)j->undo.entries[0].caret_before);
    ASSERT_EQ_INT(5, (int)j->undo.entries[0].anchor_before);
    ASSERT_EQ_INT(2, (int)j->undo.entries[0].caret_after);

    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "hello");
    ASSERT_EQ_INT(5, (int)tu_state(&ctx)->caret.cursor_pos);
    ASSERT_EQ_INT(5, (int)tu_state(&ctx)->caret.selection_anchor);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_redo_mod());
    ASSERT_EQ_STR(buf, "he");
    ASSERT_EQ_INT(2, (int)tu_state(&ctx)->caret.cursor_pos);

    // A Delete run from the start is one step too; the new edit empties
    // the redo history of the Backspace step.
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "hello");
    ASSERT_EQ_INT(1, (int)j->redo.count);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_HOME, 0);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_DELETE, 0);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_DELETE, 0);
    ASSERT_EQ_STR(buf, "llo");
    ASSERT_EQ_INT(0, (int)j->redo.count);
    ASSERT_EQ_INT(1, (int)j->undo.count);
    ASSERT_EQ_INT(0, (int)j->undo.entries[0].start);
    ASSERT_EQ_INT(2, (int)j->undo.entries[0].removed_len);
    ASSERT_TRUE(memcmp(j->undo.arena, "he", 2) == 0);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "hello");
    ASSERT_EQ_INT(0, (int)tu_state(&ctx)->caret.cursor_pos);
    wlx_context_destroy(&ctx);
}

TEST(undo_caret_move_between_keystrokes_splits_steps) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "";

    tu_focus(&ctx, buf, sizeof(buf));
    tu_type(&ctx, buf, sizeof(buf), "a");
    tu_type(&ctx, buf, sizeof(buf), "b");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, 0);
    tu_type(&ctx, buf, sizeof(buf), "c");
    ASSERT_EQ_STR(buf, "acb");
    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(2, (int)j->undo.count);
    ASSERT_TRUE(j->undo.entries[0].group != j->undo.entries[1].group);

    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_EQ_INT(1, (int)tu_state(&ctx)->caret.cursor_pos);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "");
    ASSERT_EQ_INT(0, (int)tu_state(&ctx)->caret.cursor_pos);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_redo_mod());
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_EQ_INT(2, (int)tu_state(&ctx)->caret.cursor_pos);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_redo_mod());
    ASSERT_EQ_STR(buf, "acb");
    ASSERT_EQ_INT(2, (int)tu_state(&ctx)->caret.cursor_pos);
    wlx_context_destroy(&ctx);
}

TEST(undo_enter_paste_and_cut_are_single_steps) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    tu_multiline = true;
    char buf[64] = "";

    // Enter stands alone between two typing runs.
    tu_focus(&ctx, buf, sizeof(buf));
    tu_type(&ctx, buf, sizeof(buf), "ab");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_ENTER, 0);
    tu_type(&ctx, buf, sizeof(buf), "cd");
    ASSERT_EQ_STR(buf, "ab\ncd");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "ab\n");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "ab");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "");

    // Paste stands alone between two typing runs.
    test_set_clipboard("XY");
    tu_type(&ctx, buf, sizeof(buf), "a");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_V, tu_command_mod());
    tu_type(&ctx, buf, sizeof(buf), "b");
    ASSERT_EQ_STR(buf, "aXYb");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "aXY");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "a");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "");

    // Cut restores the selection on undo.
    tu_type(&ctx, buf, sizeof(buf), "abcd");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);  // [2,4)
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_X, tu_command_mod());
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_EQ_STR("cd", test_get_clipboard());
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "abcd");
    ASSERT_EQ_INT(2, (int)tu_state(&ctx)->caret.cursor_pos);
    ASSERT_EQ_INT(4, (int)tu_state(&ctx)->caret.selection_anchor);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_redo_mod());
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_EQ_INT(2, (int)tu_state(&ctx)->caret.cursor_pos);
    ASSERT_EQ_INT(2, (int)tu_state(&ctx)->caret.selection_anchor);
    wlx_context_destroy(&ctx);
}

TEST(undo_word_and_selection_deletes_are_single_steps) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "foo bar";

    // Two word deletes never merge into one step.
    tu_focus(&ctx, buf, sizeof(buf));                       // caret at 7
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, WLX_MOD_CTRL);
    ASSERT_EQ_STR(buf, "foo ");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, WLX_MOD_CTRL);
    ASSERT_EQ_STR(buf, "");
    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(2, (int)j->undo.count);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "foo ");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "foo bar");
    ASSERT_EQ_INT(7, (int)tu_state(&ctx)->caret.cursor_pos);

    // A selection delete under Backspace is its own step, and the
    // codepoint Backspace after it starts another.
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);   // [6,7)
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "foo ba");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "foo b");
    ASSERT_EQ_INT(2, (int)j->undo.count);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "foo ba");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "foo bar");
    ASSERT_EQ_INT(6, (int)tu_state(&ctx)->caret.cursor_pos);
    ASSERT_EQ_INT(7, (int)tu_state(&ctx)->caret.selection_anchor);
    wlx_context_destroy(&ctx);
}

TEST(undo_typing_over_selection_restores_selection) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "hello";

    tu_focus(&ctx, buf, sizeof(buf));                       // caret at 5
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);  // [2,5)
    tu_type(&ctx, buf, sizeof(buf), "X");
    tu_type(&ctx, buf, sizeof(buf), "Y");                   // joins the step
    ASSERT_EQ_STR(buf, "heXY");
    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(2, (int)j->undo.count);
    ASSERT_EQ_INT(j->undo.entries[0].group, j->undo.entries[1].group);
    ASSERT_EQ_INT(2, (int)j->undo.entries[1].inserted_len);

    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "hello");
    ASSERT_EQ_INT(2, (int)tu_state(&ctx)->caret.cursor_pos);
    ASSERT_EQ_INT(5, (int)tu_state(&ctx)->caret.selection_anchor);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_redo_mod());
    ASSERT_EQ_STR(buf, "heXY");
    ASSERT_EQ_INT(4, (int)tu_state(&ctx)->caret.cursor_pos);
    ASSERT_EQ_INT(4, (int)tu_state(&ctx)->caret.selection_anchor);
    wlx_context_destroy(&ctx);
}

TEST(undo_new_edit_clears_redo_and_replay_ends_coalescing) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "";

    tu_focus(&ctx, buf, sizeof(buf));
    tu_type(&ctx, buf, sizeof(buf), "a");
    tu_type(&ctx, buf, sizeof(buf), "b");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_redo_mod());
    ASSERT_EQ_STR(buf, "ab");

    // Typing right after a redo opens a new step even though the caret
    // sits exactly where the redone run ended.
    tu_type(&ctx, buf, sizeof(buf), "c");
    ASSERT_EQ_STR(buf, "abc");
    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(2, (int)j->undo.count);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "ab");

    // A new edit after an undo empties the redo history.
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "");
    ASSERT_EQ_INT(2, (int)j->redo.count);
    tu_type(&ctx, buf, sizeof(buf), "x");
    ASSERT_EQ_INT(0, (int)j->redo.count);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Y, tu_undo_mod());
    ASSERT_EQ_STR(buf, "x");
    ASSERT_FALSE(tu_last_changed);
    wlx_context_destroy(&ctx);
}

TEST(undo_held_chord_repeats_and_empty_stack_is_noop) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "";

    tu_focus(&ctx, buf, sizeof(buf));
    tu_type(&ctx, buf, sizeof(buf), "a");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, 0);
    tu_type(&ctx, buf, sizeof(buf), "b");
    ASSERT_EQ_STR(buf, "ba");

    tu_key_repeat(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "a");
    tu_key_repeat(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "");
    ASSERT_TRUE(tu_last_changed);
    tu_key_repeat(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "");
    ASSERT_FALSE(tu_last_changed);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_FALSE(tu_last_changed);
    wlx_context_destroy(&ctx);
}

TEST(undo_multibyte_runs_restore_on_boundaries) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "";
    const char *text = "\xC3\xA9\xE2\x82\xAC" "a";             // e-acute, euro, a

    tu_focus(&ctx, buf, sizeof(buf));
    tu_type(&ctx, buf, sizeof(buf), "\xC3\xA9");
    tu_type(&ctx, buf, sizeof(buf), "\xE2\x82\xAC");
    tu_type(&ctx, buf, sizeof(buf), "a");
    ASSERT_EQ_STR(buf, text);
    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(1, (int)j->undo.count);
    ASSERT_EQ_INT(6, (int)j->undo.entries[0].inserted_len);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_redo_mod());
    ASSERT_EQ_STR(buf, text);

    // A Backspace run over the three codepoints coalesces by prepending
    // whole codepoints (a second step above the redone typing step); undo
    // puts all six bytes back, and one more undo removes the typing.
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "");
    ASSERT_EQ_INT(2, (int)j->undo.count);
    ASSERT_EQ_INT(0, (int)j->undo.entries[1].start);
    ASSERT_EQ_INT(6, (int)j->undo.entries[1].removed_len);
    ASSERT_TRUE(memcmp(j->undo.arena + j->undo.entries[1].arena_off, text, 6) == 0);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, text);
    ASSERT_EQ_INT(6, (int)tu_state(&ctx)->caret.cursor_pos);
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "");
    wlx_context_destroy(&ctx);
}

TEST(undo_read_only_rejects_chords_and_keeps_history) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "";

    tu_focus(&ctx, buf, sizeof(buf));
    tu_type(&ctx, buf, sizeof(buf), "a");
    tu_read_only = true;
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "a");
    ASSERT_FALSE(tu_last_changed);
    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(1, (int)j->undo.count);
    tu_read_only = false;
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "");
    wlx_context_destroy(&ctx);
}

TEST(undo_password_and_stale_buffers_reject_chords) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    tu_reset_fixture();
    char buf[64] = "";

    // Password: no journal, the chord is a no-op.
    tu_password = true;
    tu_focus(&ctx, buf, sizeof(buf));
    tu_type(&ctx, buf, sizeof(buf), "ab");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_FALSE(tu_last_changed);
    ASSERT_EQ_INT(0, (int)ctx.text_undo.count);

    // Stale: an external length change drops the history before the
    // chord can apply it; a revision bump does the same.
    tu_password = false;
    tu_type(&ctx, buf, sizeof(buf), "c");
    strcat(buf, "Z");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "abcZ");
    ASSERT_FALSE(tu_last_changed);
    tu_type(&ctx, buf, sizeof(buf), "d");
    tu_revision = 7;
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "abcdZ");
    ASSERT_FALSE(tu_last_changed);
    tu_type(&ctx, buf, sizeof(buf), "e");
    tu_key(&ctx, buf, sizeof(buf), WLX_KEY_Z, tu_undo_mod());
    ASSERT_EQ_STR(buf, "abcdZ");
    ASSERT_TRUE(tu_last_changed);
    wlx_context_destroy(&ctx);
}

SUITE(text_undo) {
    RUN_TEST(undo_journal_records_one_entry_per_primitive);
    RUN_TEST(undo_journal_keeps_removed_bytes_and_caret_pairs);
    RUN_TEST(undo_journal_insert_records_bytes_after_utf8_backoff);
    RUN_TEST(undo_journal_absent_without_focus_and_for_password_fields);
    RUN_TEST(undo_journal_guard_drops_history_on_external_change);
    RUN_TEST(undo_journal_evicts_oldest_steps_at_the_entry_cap);
    RUN_TEST(undo_journal_evicts_by_bytes_and_admits_an_oversize_step);
    RUN_TEST(undo_typing_run_is_one_step_and_redo_restores);
    RUN_TEST(undo_backspace_and_delete_runs_are_single_steps);
    RUN_TEST(undo_caret_move_between_keystrokes_splits_steps);
    RUN_TEST(undo_enter_paste_and_cut_are_single_steps);
    RUN_TEST(undo_word_and_selection_deletes_are_single_steps);
    RUN_TEST(undo_typing_over_selection_restores_selection);
    RUN_TEST(undo_new_edit_clears_redo_and_replay_ends_coalescing);
    RUN_TEST(undo_held_chord_repeats_and_empty_stack_is_noop);
    RUN_TEST(undo_multibyte_runs_restore_on_boundaries);
    RUN_TEST(undo_read_only_rejects_chords_and_keeps_history);
    RUN_TEST(undo_password_and_stale_buffers_reject_chords);
}
