// test_hard_assert.c - release-build (NDEBUG) coverage for WLX_HARD_ASSERT.
//
// Compiled as its own translation unit with -DNDEBUG (it is NOT part of the
// single-TU test_runner). Plain assert() is compiled out here, so these checks
// prove the memory-safety guards still fire in release builds:
//   - wlx_get_state_impl aborts on a state-size collision
//   - wlx_scissor_scope_begin aborts on scissor-stack overflow
//   - wlx_prepare_content_sizes clamps (warn + no CONTENT tracking) instead of
//     smashing the stack when a CONTENT layout exceeds WLX_CONTENT_SLOTS_MAX
//
// Death checks fork: the child triggers the guard and must die with SIGABRT.

#ifndef NDEBUG
#error "compile this with -DNDEBUG"
#endif

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"

#include "tests.h"
#include "test_mock_backend.h"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

// Run fn in a fork with stderr silenced; report how the child exited.
typedef enum { CHILD_EXITED_CLEAN, CHILD_ABORTED, CHILD_OTHER } Child_Result;

static Child_Result run_in_child(void (*fn)(void)) {
    pid_t pid = fork();
    if (pid == 0) {
        freopen("/dev/null", "w", stderr);
        fn();
        _exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
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
    RUN_TEST(plain_assert_is_inert_under_ndebug);
}

int main(void) {
    RUN_SUITE(hard_assert);
    return test_summary();
}
