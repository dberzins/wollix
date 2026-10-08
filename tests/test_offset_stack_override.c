// test_offset_stack_override.c - CONTENT layouts under a lowered
// WLX_OFFSET_STACK_LIMIT.
//
// Compiled as its own translation unit (it is NOT part of the single-TU
// test_runner) with WLX_OFFSET_STACK_LIMIT below WLX_CONTENT_SLOTS_MAX, so a
// CONTENT layout's offset solver takes its redistribution working block from
// the byte scratch arena, the arena that already holds the layout's copy of
// its slot sizes. A general allocator that always moves the block on growth
// and poisons the old one makes an offset taken from a pointer held across
// that allocation visible in a plain build.

#define WLX_OFFSET_STACK_LIMIT 4

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "test_mock_backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if WLX_OFFSET_STACK_LIMIT >= WLX_CONTENT_SLOTS_MAX
#error "the case needs a CONTENT layout above the offset stack limit"
#endif

#define CHECK(cond) do {                                                       \
    if (!(cond)) {                                                             \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
        return 1;                                                              \
    }                                                                          \
} while (0)

static int moves;

static void *moving_alloc(size_t size, void *user) {
    (void)user;
    void *p = malloc(size);
    if (p != NULL) memset(p, 0xA5, size);
    return p;
}

static void *moving_realloc(void *ptr, size_t old_size, size_t new_size, void *user) {
    (void)user;
    void *p = malloc(new_size);
    if (p == NULL) return NULL;
    memset(p, 0xA5, new_size);
    if (ptr != NULL) {
        memcpy(p, ptr, old_size < new_size ? old_size : new_size);
        memset(ptr, 0xA5, old_size);
        free(ptr);
    }
    moves++;
    return p;
}

static void moving_free(void *ptr, size_t size, void *user) {
    (void)size; (void)user;
    free(ptr);
}

// A PX layout's retained sizes fill the byte scratch to 96 of its initial
// 256 bytes; the CONTENT layout's copy takes it to 224 and its solver block
// (8 floats + 8 flags) crosses the capacity.
enum { PX_SLOTS = 6, CONTENT_SLOTS = 8 };

typedef struct {
    size_t cap_before;     // byte scratch capacity before the CONTENT begin
    size_t cap_after;      // and after it
    bool   copy_in_range;  // the retained copy lies inside the used scratch
    bool   copy_matches;   // and reads back as the caller's sizes
    WLX_Rect rects[CONTENT_SLOTS];
} Frame_Result;

static void run_frame(WLX_Context *ctx, const WLX_Slot_Size *content, Frame_Result *r) {
    WLX_Slot_Size px[PX_SLOTS];
    for (int i = 0; i < PX_SLOTS; i++) px[i] = WLX_SLOT_PX(10);

    test_frame_begin(ctx, 0, 0, false, false);
    wlx_layout_begin(ctx, PX_SLOTS, WLX_VERT, .sizes = px);
    wlx_layout_end(ctx);

    r->cap_before = ctx->arena.scratch.capacity;
    wlx_layout_begin(ctx, CONTENT_SLOTS, WLX_VERT, .sizes = content);
    r->cap_after = ctx->arena.scratch.capacity;
    const WLX_Layout *l = &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
    r->copy_in_range = l->has_content_sizes
        && l->content_sizes_scratch_off + CONTENT_SLOTS * sizeof(WLX_Slot_Size)
           <= ctx->arena.scratch.count;
    r->copy_matches = r->copy_in_range
        && memcmp(wlx_layout_content_sizes(ctx, l), content,
                  CONTENT_SLOTS * sizeof(WLX_Slot_Size)) == 0;
    for (size_t i = 0; i < CONTENT_SLOTS; i++) {
        r->rects[i] = wlx_calc_layout_slot_rect(ctx, l, i, 1);
    }
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

int main(void) {
    WLX_Allocator moving = {
        .alloc = moving_alloc, .realloc = moving_realloc, .free = moving_free,
    };
    WLX_Arena_Pool_Config cfg = { .general = &moving };
    WLX_Context ctx;
    wlx_context_init_ex(&ctx, &cfg);
    ctx.backend = mock_backend();
    ctx.rect = wlx_rect(0, 0, 400, 300);

    WLX_Slot_Size content[CONTENT_SLOTS];
    content[0] = WLX_SLOT_CONTENT;
    for (int i = 1; i < CONTENT_SLOTS; i++) content[i] = WLX_SLOT_FLEX_MIN(1, 10);

    Frame_Result first, second;
    run_frame(&ctx, content, &first);
    run_frame(&ctx, content, &second);

    // Precondition: the scratch grew (and moved) inside frame 1's CONTENT
    // begin, and frame 2 found it grown.
    CHECK(first.cap_before == WLX_DA_INIT_CAP);
    CHECK(first.cap_after > first.cap_before);
    CHECK(moves > 0);
    CHECK(second.cap_after == second.cap_before);

    CHECK(first.copy_in_range);
    CHECK(first.copy_matches);
    CHECK(second.copy_in_range);
    CHECK(second.copy_matches);
    for (int i = 0; i < CONTENT_SLOTS; i++) {
        CHECK(first.rects[i].x == second.rects[i].x && first.rects[i].y == second.rects[i].y
              && first.rects[i].w == second.rects[i].w && first.rects[i].h == second.rects[i].h);
    }

    wlx_context_destroy(&ctx);
    printf("test_offset_stack_override: OK (CONTENT copy survives a scratch move)\n");
    return 0;
}
