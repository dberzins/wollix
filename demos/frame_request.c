// frame_request.c - a loop that rests, and the two calls that wake it.
//
// This window draws a frame only when one is needed. wlx_raylib_wait_frame,
// called after EndDrawing, waits until input arrives or a frame that was
// asked for comes due. Wollix asks for its own timers (a caret blink, a
// tooltip delay). What the application animates, the application asks for:
//
//   wlx_request_frame(ctx)            the next frame, now. The progress bar
//                                     calls it on every frame while it runs.
//   wlx_request_frame_after(ctx, s)   a frame no later than s seconds from
//                                     now. The clock calls it with the time
//                                     left to the next whole second.
//
// A request lasts one frame: whatever is still pending asks again on the
// next one. Time comes from wlx_get_frame_time, which spans a rest, so the
// clock keeps real time although only two frames a second are drawn for it:
// the tick's own, and one more in which Wollix sees that nothing else
// changed.
//
// The last line counts the frames drawn in each second. It is renewed on the
// tick: a number that changed on every frame would itself keep the loop
// drawing.
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_raylib.h"

#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 460
#define TARGET_FPS 60
#define RUN_SECONDS 3.0f

typedef struct {
    double   clock;          // seconds since the first frame
    long     second;         // the whole second the clock label shows
    float    run_left;       // seconds the progress bar still runs; 0 at rest
    int      runs;           // times the bar was started
    unsigned frames;         // frames drawn so far
    unsigned frames_at_tick; // that count at the last tick
    unsigned frames_shown;   // frames drawn in the second before the last tick
} App_State;

static App_State app = {0};

int main(void) {
    printf("Wollix frame request demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix frame request demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    const WLX_Color dim = { 170, 180, 195, 255 };

    while (!WindowShouldClose()) {
        wlx_begin(ctx, wlx_raylib_root_rect(), wlx_process_raylib_input);
        BeginDrawing();
        ClearBackground(ctx->theme->background);

        // Advance by the time since the frame before. After a rest that is
        // the whole rest, so nothing here needs a frame to keep time.
        float dt = wlx_get_frame_time(ctx);
        app.clock += dt;
        app.frames++;
        if (app.run_left > 0.0f) {
            app.run_left -= dt;
            if (app.run_left < 0.0f) app.run_left = 0.0f;
        }

        // The tick: a new whole second. The frame count is renewed here and
        // nowhere else.
        long second = (long)app.clock;
        if (second != app.second) {
            app.second = second;
            app.frames_shown = app.frames - app.frames_at_tick;
            app.frames_at_tick = app.frames;
        }

        char clock_text[48], frames_text[64], run_text[64];
        snprintf(clock_text, sizeof(clock_text), "Clock  %02ld:%02ld", second / 60, second % 60);
        snprintf(frames_text, sizeof(frames_text),
            "Frames drawn in the last second: %u", app.frames_shown);
        if (app.run_left > 0.0f) {
            snprintf(run_text, sizeof(run_text), "Running: %.1f s left", (double)app.run_left);
        } else if (app.runs > 0) {
            snprintf(run_text, sizeof(run_text), "Run %d finished. At rest.", app.runs);
        } else {
            snprintf(run_text, sizeof(run_text), "At rest.");
        }
        float progress = app.runs == 0 ? 0.0f : 1.0f - app.run_left / RUN_SECONDS;

        wlx_layout_begin(ctx, 9, WLX_VERT, .padding = 24, .gap = 8,
            .sizes = (WLX_Slot_Size[]){
                WLX_SLOT_PX(40), WLX_SLOT_PX(28), WLX_SLOT_PX(36), WLX_SLOT_PX(28),
                WLX_SLOT_PX(48), WLX_SLOT_PX(24), WLX_SLOT_PX(28), WLX_SLOT_PX(28),
                WLX_SLOT_FLEX(1) });

            wlx_label(ctx, "Frame requests", .font_size = 28, .content_align = WLX_LEFT);
            wlx_label(ctx, "This window draws a frame only when one is needed.",
                .font_size = 18, .content_align = WLX_LEFT, .front_color = dim);

            wlx_label(ctx, clock_text, .id = "clock", .font_size = 24, .content_align = WLX_LEFT);
            wlx_label(ctx, frames_text, .id = "frames", .font_size = 18, .content_align = WLX_LEFT);

            wlx_layout_begin(ctx, 2, WLX_HORZ, .gap = 16,
                .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(260), WLX_SLOT_FLEX(1) });
                if (wlx_button(ctx, "Run for three seconds",
                        .font_size = 18, .content_align = WLX_CENTER)) {
                    app.run_left = RUN_SECONDS;
                    app.runs++;
                }
                wlx_label(ctx, run_text, .id = "run", .font_size = 18, .content_align = WLX_LEFT);
            wlx_layout_end(ctx);
            wlx_progress(ctx, progress, .id = "bar");

            wlx_label(ctx, "The clock asks for one frame a second: wlx_request_frame_after.",
                .font_size = 16, .content_align = WLX_LEFT, .front_color = dim);
            wlx_label(ctx, "The bar asks for every frame while it runs: wlx_request_frame.",
                .font_size = 16, .content_align = WLX_LEFT, .front_color = dim);
            wlx_label(ctx, "Moving the pointer brings frames too: input always does.",
                .font_size = 16, .content_align = WLX_TOP_LEFT, .front_color = dim);

        wlx_layout_end(ctx);

        // The clock label changes at the next whole second: ask for a frame
        // by then. The bar moves on every frame it runs: ask for the next.
        wlx_request_frame_after(ctx, (float)((double)(second + 1) - app.clock));
        if (app.run_left > 0.0f) wlx_request_frame(ctx);

        wlx_end(ctx);
        EndDrawing();

        // Rest until input arrives or one of the requests comes due.
        wlx_raylib_wait_frame(ctx);
    }

    wlx_context_destroy(ctx);
    free(ctx);
    CloseWindow();
    return 0;
}
