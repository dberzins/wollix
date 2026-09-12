// test_text_undo.c - text undo journal: the context-owned per-widget history
// the shared edit primitives record into. Recording is exercised through the
// inputbox: one entry per primitive mutation with exact ranges, removed
// bytes kept in the arena, caret pairs before and after; the staleness
// guard (length and revision); the password and unfocused exclusions; and
// the entry and byte caps with whole-step eviction and oversize admission.

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
static uint32_t tu_revision = 0;

static void tu_inputbox(WLX_Context *ctx, char *buf, size_t buf_size) {
    (void)wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .wrap = false, .border_width = 0, .password = tu_password,
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
    tu_revision = 0;
}

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

    // A second typing frame is a second record with its own step id.
    tu_type(&ctx, buf, sizeof(buf), "c");
    ASSERT_EQ_STR(buf, "abc");
    ASSERT_EQ_INT(2, (int)j->undo.count);
    ASSERT_EQ_INT(2, (int)j->undo.entries[1].start);
    ASSERT_EQ_INT(0, (int)j->undo.entries[1].removed_len);
    ASSERT_EQ_INT(1, (int)j->undo.entries[1].inserted_len);
    ASSERT_TRUE(j->undo.entries[1].group != j->undo.entries[0].group);
    ASSERT_EQ_INT(3, (int)j->expected_len);
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
    ASSERT_EQ_INT(2, (int)j->undo.count);
    ASSERT_EQ_INT(2, (int)j->undo.entries[1].start);
    ASSERT_EQ_INT(1, (int)j->undo.entries[1].inserted_len);
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
    const int typed = WLX_TEXT_UNDO_ENTRIES + 8;
    for (int i = 0; i < typed; i++) tu_type(&ctx, buf, sizeof(buf), "x");
    ASSERT_EQ_INT(typed, (int)strlen(buf));

    WLX_Text_Undo_Journal *j = tu_journal(&ctx);
    ASSERT_TRUE(j != NULL);
    ASSERT_EQ_INT(WLX_TEXT_UNDO_ENTRIES, (int)j->undo.count);
    ASSERT_EQ_INT(WLX_TEXT_UNDO_ENTRIES, (int)j->undo.cap);
    // Step ids run 1..typed; the eight oldest were evicted whole.
    ASSERT_EQ_INT(9, (int)j->undo.entries[0].group);
    ASSERT_EQ_INT(typed, (int)j->undo.entries[j->undo.count - 1].group);
    ASSERT_EQ_INT(0, (int)j->undo.arena_used);
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

SUITE(text_undo) {
    RUN_TEST(undo_journal_records_one_entry_per_primitive);
    RUN_TEST(undo_journal_keeps_removed_bytes_and_caret_pairs);
    RUN_TEST(undo_journal_insert_records_bytes_after_utf8_backoff);
    RUN_TEST(undo_journal_absent_without_focus_and_for_password_fields);
    RUN_TEST(undo_journal_guard_drops_history_on_external_change);
    RUN_TEST(undo_journal_evicts_oldest_steps_at_the_entry_cap);
    RUN_TEST(undo_journal_evicts_by_bytes_and_admits_an_oversize_step);
}
