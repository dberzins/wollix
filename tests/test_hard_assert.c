// test_hard_assert.c - release-build (NDEBUG) coverage for WLX_HARD_ASSERT.
//
// Compiled as its own translation unit with -DNDEBUG (it is NOT part of the
// single-TU test_runner). Plain assert() is compiled out here, so these checks
// prove the memory-safety guards still fire in release builds:
//   - wlx_get_state_impl aborts on a state-size collision
//   - wlx_scissor_scope_begin aborts on scissor-stack overflow
//   - wlx_prepare_content_sizes clamps (warn + no CONTENT tracking) instead of
//     smashing the stack when a CONTENT layout exceeds WLX_CONTENT_SLOTS_MAX
//   - wlx_menu_end / wlx_menu_item / wlx_submenu_begin abort on an empty
//     menu stack instead of indexing menu_stack[-1]
//   - the sub-arena size_t-overflow guards abort instead of wrapping to an
//     undersized reserve (out-of-bounds writes)
//   - with no handler installed, a contract error's default handler prints
//     once per site through WLX_ERROR_PRINT and returns (it aborts only in a
//     build without NDEBUG), and a hard assert's text reaches the sink
//     before abort()
//
// Death checks fork: the child triggers the guard and must die with SIGABRT.

#ifndef NDEBUG
#error "compile this with -DNDEBUG"
#endif

// The diagnostic sink is a compile-time macro: every line wollix would print
// in this TU (contract errors with no handler, hard asserts) lands in a
// buffer instead, so the release default handler and the once-per-site sink
// can be checked without a handler installed.
static void test_sink_capture(const char *msg);
#define WLX_ERROR_PRINT(msg) test_sink_capture(msg)

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"

#include "tests.h"
#include "test_mock_backend.h"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

static char   test_sink_buf[8192];
static size_t test_sink_len;
static int    test_sink_lines;
static int    test_sink_exit_on_fatal;   // nonzero: a "wollix fatal" line _exit()s with it

static void test_sink_capture(const char *msg) {
    size_t n = strlen(msg);
    if (test_sink_exit_on_fatal != 0 && strstr(msg, "wollix fatal") != NULL) {
        // +1 when a contract error line was already captured: proves the
        // report came before the abort.
        _exit(test_sink_exit_on_fatal + (strstr(test_sink_buf, "wollix error:") != NULL ? 1 : 0));
    }
    if (test_sink_len + n + 1 < sizeof test_sink_buf) {
        memcpy(test_sink_buf + test_sink_len, msg, n);
        test_sink_len += n;
        test_sink_buf[test_sink_len++] = '\n';
        test_sink_buf[test_sink_len] = '\0';
    }
    test_sink_lines++;
}

static void test_sink_reset(void) {
    test_sink_len = 0;
    test_sink_lines = 0;
    test_sink_buf[0] = '\0';
}

// Run fn in a fork with stderr silenced; report how the child exited. The
// raw exit status of a child that exited is kept in last_child_exit_code.
typedef enum { CHILD_EXITED_CLEAN, CHILD_ABORTED, CHILD_OTHER } Child_Result;

static int last_child_exit_code = -1;

static Child_Result run_in_child(void (*fn)(void)) {
    pid_t pid = fork();
    if (pid == 0) {
        freopen("/dev/null", "w", stderr);
        fn();
        _exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    last_child_exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT) return CHILD_ABORTED;
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) return CHILD_EXITED_CLEAN;
    return CHILD_OTHER;
}

// --- Guard triggers (each runs in a forked child) ---

static void trigger_state_size_mismatch(void) {
    WLX_Context ctx = {0};
    wlx_context_init(&ctx);
    // Same file/line + empty id stack -> same state id, different sizes.
    (void)wlx_get_state_impl(&ctx, 4, "collide", 1);
    (void)wlx_get_state_impl(&ctx, 8, "collide", 1);
}

static void trigger_scissor_stack_overflow(void) {
    WLX_Context ctx = {0};
    wlx_context_init(&ctx);
    for (int i = 0; i <= WLX_SCISSOR_STACK_MAX; i++) {
        (void)wlx_scissor_scope_begin(&ctx, (WLX_Rect){ 0, 0, 10, 10 });
    }
}

static void content_slots_over_max_frame(void) {
    enum { OVER_MAX = WLX_CONTENT_SLOTS_MAX + 1 };
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    WLX_Slot_Size sizes[OVER_MAX];
    for (size_t i = 0; i < OVER_MAX; i++) sizes[i] = WLX_SLOT_FLEX(1);
    sizes[0] = WLX_SLOT_CONTENT;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, OVER_MAX, WLX_VERT, .sizes = sizes);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

static void trigger_overlay_end_without_begin(void) {
    WLX_Context ctx = {0};
    wlx_context_init(&ctx);
    wlx_overlay_end(&ctx);   // empty layout stack: count - 1 would wrap
}

static void trigger_menu_stack_overflow(void) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    static bool open = true;
    for (int i = 0; i <= WLX_MENU_STACK_MAX; i++) {
        wlx_push_id(&ctx, (size_t)i + 1);
        (void)wlx_menu_begin(&ctx, &open, 10, 10);
    }
}

static void trigger_menu_end_without_begin(void) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_menu_end(&ctx);      // empty menu stack: count would go to -1
}

static void trigger_sub_arena_reserve_overflow(void) {
    // A negative count converted to size_t arrives here as a huge 'needed';
    // the capacity-doubling guard must abort before the wrap.
    WLX_Sub_Arena sa;
    wlx_sub_arena_init(&sa, sizeof(float), NULL);
    wlx_sub_arena_reserve(&sa, SIZE_MAX);
}

static void trigger_sub_arena_alloc_count_overflow(void) {
    WLX_Sub_Arena sa;
    wlx_sub_arena_init(&sa, sizeof(float), NULL);
    (void)wlx_sub_arena_alloc(&sa, 1);
    (void)wlx_sub_arena_alloc(&sa, SIZE_MAX);  // count + n wraps
}

// --- Tests ---

TEST(hard_assert_fires_on_state_size_mismatch) {
    ASSERT_TRUE(run_in_child(trigger_state_size_mismatch) == CHILD_ABORTED);
}

TEST(hard_assert_fires_on_scissor_stack_overflow) {
    ASSERT_TRUE(run_in_child(trigger_scissor_stack_overflow) == CHILD_ABORTED);
}

TEST(content_slots_over_max_clamps_instead_of_corrupting) {
    ASSERT_TRUE(run_in_child(content_slots_over_max_frame) == CHILD_EXITED_CLEAN);
}

TEST(hard_assert_fires_on_overlay_end_without_begin) {
    ASSERT_TRUE(run_in_child(trigger_overlay_end_without_begin) == CHILD_ABORTED);
}

TEST(hard_assert_fires_on_menu_stack_overflow) {
    ASSERT_TRUE(run_in_child(trigger_menu_stack_overflow) == CHILD_ABORTED);
}

TEST(hard_assert_fires_on_menu_end_without_begin) {
    ASSERT_TRUE(run_in_child(trigger_menu_end_without_begin) == CHILD_ABORTED);
}

TEST(hard_assert_fires_on_sub_arena_reserve_overflow) {
    ASSERT_TRUE(run_in_child(trigger_sub_arena_reserve_overflow) == CHILD_ABORTED);
}

TEST(hard_assert_fires_on_sub_arena_alloc_count_overflow) {
    ASSERT_TRUE(run_in_child(trigger_sub_arena_alloc_count_overflow) == CHILD_ABORTED);
}

// A backend table that never declared the v2 contract (a pre-0.9 table
// assigned field by field, or one whose author forgot the version) must
// fail at wlx_begin in every build, since calling its callbacks through
// the v2 signatures would be memory-unsafe.
static void trigger_backend_contract_mismatch(void) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.contract_version = 0;
    test_frame_begin(&ctx, 0, 0, false, false);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(hard_assert_fires_on_backend_contract_mismatch) {
    ASSERT_TRUE(run_in_child(trigger_backend_contract_mismatch) == CHILD_ABORTED);
}

// --- The release error surface: default handler and sink, no handler installed ---

TEST(default_handler_returns_under_ndebug) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_sink_reset();
    wlx_error_report(&ctx, WLX_ERR_BAD_ARGUMENT, "probe message", "probe.c", 3);
    // Reached: the default handler printed and returned instead of aborting.
    ASSERT_EQ_INT(1, test_sink_lines);
    ASSERT_TRUE(strstr(test_sink_buf, "probe.c:3: wollix error: probe message") != NULL);
    ASSERT_EQ_INT(1, wlx_error_count(&ctx));
    ASSERT_EQ_INT(WLX_ERR_BAD_ARGUMENT, (int)ctx.last_error.code);
    wlx_context_destroy(&ctx);
}

TEST(default_sink_prints_once_per_site) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_sink_reset();
    for (int i = 0; i < 5; i++) {
        wlx_error_report(&ctx, WLX_ERR_SLOT_OVERRUN, "same site", "site.c", 10);
    }
    ASSERT_EQ_INT(1, test_sink_lines);
    ASSERT_EQ_INT(5, wlx_error_count(&ctx));
    wlx_error_report(&ctx, WLX_ERR_SLOT_OVERRUN, "same site", "site.c", 11);   // another line
    ASSERT_EQ_INT(2, test_sink_lines);
    wlx_error_report(&ctx, WLX_ERR_GRID_BOUNDS, "same site", "site.c", 11);    // another code
    ASSERT_EQ_INT(3, test_sink_lines);
    ASSERT_EQ_INT(7, wlx_error_count(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(default_sink_names_the_open_layout) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_sink_reset();
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_VERT);
    wlx_error_report(&ctx, WLX_ERR_SLOT_OVERRUN, "inside", NULL, 0);
    ASSERT_EQ_INT(1, test_sink_lines);
    ASSERT_TRUE(strstr(test_sink_buf, "wollix error: inside (layout begun at ") != NULL);
    ASSERT_TRUE(strstr(test_sink_buf, __FILE__) != NULL);
    ASSERT_TRUE(strncmp(test_sink_buf, "wollix error:", 13) == 0);   // no caller site: message first
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(default_sink_suppresses_after_table_full) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_sink_reset();
    for (int i = 0; i < WLX_ERROR_SITES_MAX + 3; i++) {
        wlx_error_report(&ctx, WLX_ERR_BAD_ARGUMENT, "distinct", "many.c", 100 + i);
    }
    ASSERT_EQ_INT(WLX_ERROR_SITES_MAX + 1, test_sink_lines);   // the sites, then one suppression line
    ASSERT_TRUE(strstr(test_sink_buf, "further error sites suppressed") != NULL);
    ASSERT_EQ_INT(WLX_ERROR_SITES_MAX + 3, wlx_error_count(&ctx));
    wlx_error_report(&ctx, WLX_ERR_BAD_ARGUMENT, "distinct", "many.c", 999);
    ASSERT_EQ_INT(WLX_ERROR_SITES_MAX + 1, test_sink_lines);   // silent from here on
    // A site already in the table stays silent too; an installed handler is never throttled.
    wlx_context_destroy(&ctx);
}

// --- The four release probes: with no handler, each reports and degrades ---

static void probe_slot_overrun(void) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_VERT);
    for (int i = 0; i < 6; i++) (void)wlx_button(&ctx, "child");
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    if (wlx_error_count(&ctx) != 4) _exit(3);
    wlx_context_destroy(&ctx);
}

static void probe_grid_bounds(void) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_grid_begin(&ctx, 2, 2);
    wlx_grid_cell(&ctx, 5, 7);
    for (int i = 0; i < 5; i++) (void)wlx_button(&ctx, "cell");   // the fifth overruns
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);
    if (wlx_error_count(&ctx) != 2) _exit(3);
    wlx_context_destroy(&ctx);
}

static void probe_extra_end(void) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_layout_end(&ctx);
    wlx_layout_end(&ctx);
    wlx_layout_begin(&ctx, 1, WLX_VERT);   // used to abort in the sub-arena guard
    (void)wlx_button(&ctx, "after");
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    if (wlx_error_count(&ctx) != 1) _exit(3);
    wlx_context_destroy(&ctx);
}

static void probe_orphan_widget(void) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);
    (void)wlx_button(&ctx, "orphan");     // used to segfault
    test_frame_end(&ctx);
    if (wlx_error_count(&ctx) != 1) _exit(3);
    wlx_context_destroy(&ctx);
}

// An empty table with the right contract version: reported through the
// channel (the default handler returns under NDEBUG), then fatal.
static void trigger_backend_not_ready(void) {
    WLX_Context ctx = {0};
    wlx_context_init(&ctx);
    ctx.backend.contract_version = WLX_BACKEND_CONTRACT_VERSION;
    test_sink_reset();
    test_sink_exit_on_fatal = 43;
    wlx_begin(&ctx, wlx_rect(0, 0, 10, 10), _test_input_handler);
}

TEST(backend_not_ready_reports_then_aborts) {
    Child_Result r = run_in_child(trigger_backend_not_ready);
    ASSERT_TRUE(r == CHILD_OTHER);
    ASSERT_EQ_INT(44, last_child_exit_code);   // 43 + 1: the error line preceded the fatal one
}

TEST(release_probe_slot_overrun_exits_clean) {
    ASSERT_TRUE(run_in_child(probe_slot_overrun) == CHILD_EXITED_CLEAN);
}

TEST(release_probe_grid_bounds_exits_clean) {
    ASSERT_TRUE(run_in_child(probe_grid_bounds) == CHILD_EXITED_CLEAN);
}

TEST(release_probe_extra_end_exits_clean) {
    ASSERT_TRUE(run_in_child(probe_extra_end) == CHILD_EXITED_CLEAN);
}

TEST(release_probe_orphan_widget_exits_clean) {
    ASSERT_TRUE(run_in_child(probe_orphan_widget) == CHILD_EXITED_CLEAN);
}

static void trigger_hard_assert_through_sink(void) {
    test_sink_reset();   // the child inherits the parent's captured lines
    test_sink_exit_on_fatal = 42;
    trigger_overlay_end_without_begin();   // WLX_HARD_ASSERT: its text reaches the sink first
}

TEST(hard_assert_prints_before_abort) {
    Child_Result r = run_in_child(trigger_hard_assert_through_sink);
    ASSERT_TRUE(r == CHILD_OTHER);
    ASSERT_EQ_INT(42, last_child_exit_code);   // the sink saw "wollix fatal" before abort()
}

TEST(plain_assert_is_inert_under_ndebug) {
    // Meta-check: this TU really is a release-style build.
    int reached = 0;
    assert(reached == 1 && "compiled out under NDEBUG");
    reached = 1;
    ASSERT_TRUE(reached == 1);
}

SUITE(hard_assert) {
    RUN_TEST(hard_assert_fires_on_state_size_mismatch);
    RUN_TEST(hard_assert_fires_on_scissor_stack_overflow);
    RUN_TEST(content_slots_over_max_clamps_instead_of_corrupting);
    RUN_TEST(hard_assert_fires_on_overlay_end_without_begin);
    RUN_TEST(hard_assert_fires_on_menu_stack_overflow);
    RUN_TEST(hard_assert_fires_on_menu_end_without_begin);
    RUN_TEST(hard_assert_fires_on_sub_arena_reserve_overflow);
    RUN_TEST(hard_assert_fires_on_sub_arena_alloc_count_overflow);
    RUN_TEST(hard_assert_fires_on_backend_contract_mismatch);
    RUN_TEST(default_handler_returns_under_ndebug);
    RUN_TEST(default_sink_prints_once_per_site);
    RUN_TEST(default_sink_names_the_open_layout);
    RUN_TEST(default_sink_suppresses_after_table_full);
    RUN_TEST(hard_assert_prints_before_abort);
    RUN_TEST(backend_not_ready_reports_then_aborts);
    RUN_TEST(release_probe_slot_overrun_exits_clean);
    RUN_TEST(release_probe_grid_bounds_exits_clean);
    RUN_TEST(release_probe_extra_end_exits_clean);
    RUN_TEST(release_probe_orphan_widget_exits_clean);
    RUN_TEST(plain_assert_is_inert_under_ndebug);
}

int main(void) {
    RUN_SUITE(hard_assert);
    return test_summary();
}
