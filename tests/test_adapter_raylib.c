// test_adapter_raylib.c - the Raylib adapter's input path,
// wlx_raylib_wait_frame and its text draw against a Raylib stand-in.
//
// Raylib's PollInputEvents erases what the poll before it brought: press
// edges, repeat flags, the wheel, the character queue, a close request, a
// resize. A helper that polls while it waits therefore has to stop at the
// first sign of input, and that cannot be watched from outside a real
// window. This TU compiles the adapter against the real raylib.h but links
// no Raylib: it defines the Raylib functions the input path calls, over a
// scripted event list and a clock it owns, with the real poll's erasing
// behaviour. No window and no display.
//
// Build: make test-raylib (needs raylib.h on the include path, no library).
//
// Never reference wlx_backend_raylib() or wlx_context_init_raylib() here:
// they take the address of every draw callback and would pull the rest of
// Raylib into the link.

#include <raylib.h>
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_raylib.h"
#include "tests.h"
#include "test_mock_backend.h"

#include <string.h>

// ============================================================================
// Raylib stand-in
// ============================================================================

enum {
    EV_KEY_DOWN, EV_KEY_UP, EV_KEY_REPEAT, EV_CHAR, EV_BUTTON_DOWN, EV_BUTTON_UP,
    EV_MOVE, EV_WHEEL, EV_CLOSE, EV_RESIZE, EV_FOCUS, EV_SCALE,
    EV_OTHER,   // a window event Wollix has no use for (an expose, a move)
};

typedef struct {
    double at;      // seconds on the stand-in's clock
    int    kind;
    int    a;       // key, button, codepoint or flag
    float  x, y;
} RL_Event;

#define RL_KEYS    512
#define RL_BUTTONS 8
#define RL_EVENTS  96
#define RL_FRAME   (1.0 / 60.0)

static struct {
    double   clock;
    RL_Event events[RL_EVENTS];
    int      event_count, event_next;
    bool     key_down[RL_KEYS], key_prev[RL_KEYS], key_repeat[RL_KEYS];
    bool     button_down[RL_BUTTONS], button_prev[RL_BUTTONS];
    Vector2  mouse, mouse_prev, wheel;
    int      chars[16];
    int      char_count, char_read;
    bool     should_close, resized, focused, high_dpi, event_waiting;
    float    dpi;
    float    frame_time;      // GetFrameTime's answer; fixed unless frame_time_live
    bool     frame_time_live; // GetFrameTime follows EndDrawing, as Raylib's does
    double   last_end;        // the clock at the previous EndDrawing
    int      polls, blocking_polls;
    bool     stuck;           // a blocking poll with nothing left to wake it
    double   longest_sleep;
} rl;

static void rl_reset(void) {
    memset(&rl, 0, sizeof rl);
    rl.focused = true;
    rl.dpi = 1.0f;
    rl.frame_time = (float)RL_FRAME;
    memset(&g_wlx_raylib_frame, 0, sizeof g_wlx_raylib_frame);
}

static void rl_push(double at, int kind, int a, float x, float y) {
    if (rl.event_count >= RL_EVENTS) { fprintf(stderr, "RL_EVENTS too small\n"); exit(2); }
    // The script is written in time order; the poll delivers from the front.
    if (rl.event_count > 0 && at < rl.events[rl.event_count - 1].at) {
        fprintf(stderr, "stand-in events out of order\n"); exit(2);
    }
    rl.events[rl.event_count++] = (RL_Event){ at, kind, a, x, y };
}

static void rl_deliver(const RL_Event *e) {
    switch (e->kind) {
        case EV_KEY_DOWN:    rl.key_down[e->a] = true; break;
        case EV_KEY_UP:      rl.key_down[e->a] = false; break;
        case EV_KEY_REPEAT:  rl.key_repeat[e->a] = true; break;
        case EV_CHAR:        if (rl.char_count < 16) rl.chars[rl.char_count++] = e->a; break;
        case EV_BUTTON_DOWN: rl.button_down[e->a] = true; break;
        case EV_BUTTON_UP:   rl.button_down[e->a] = false; break;
        case EV_MOVE:        rl.mouse = (Vector2){ e->x, e->y }; break;
        case EV_WHEEL:       rl.wheel = (Vector2){ e->x, e->y }; break;   // Raylib keeps the last one
        case EV_CLOSE:       rl.should_close = true; break;
        case EV_RESIZE:      rl.resized = true; break;
        case EV_FOCUS:       rl.focused = e->a != 0; break;
        case EV_SCALE:       rl.dpi = e->x; break;
        default: break;
    }
}

// What the real poll does first: everything the previous poll brought goes.
void PollInputEvents(void) {
    rl.polls++;
    memcpy(rl.key_prev, rl.key_down, sizeof rl.key_prev);
    memset(rl.key_repeat, 0, sizeof rl.key_repeat);
    memcpy(rl.button_prev, rl.button_down, sizeof rl.button_prev);
    rl.mouse_prev = rl.mouse;
    rl.wheel = (Vector2){ 0.0f, 0.0f };
    rl.char_count = rl.char_read = 0;
    rl.resized = false;
    rl.should_close = false;

    if (rl.event_waiting) {
        rl.blocking_polls++;
        if (rl.event_next >= rl.event_count) { rl.stuck = true; return; }
        if (rl.events[rl.event_next].at > rl.clock) rl.clock = rl.events[rl.event_next].at;
    }
    while (rl.event_next < rl.event_count && rl.events[rl.event_next].at <= rl.clock) {
        rl_deliver(&rl.events[rl.event_next++]);
    }
}

static bool rl_key_ok(int key) { return key > 0 && key < RL_KEYS; }
static bool rl_button_ok(int b) { return b >= 0 && b < RL_BUTTONS; }

bool IsKeyDown(int key)          { return rl_key_ok(key) && rl.key_down[key]; }
bool IsKeyPressed(int key)       { return rl_key_ok(key) && rl.key_down[key] && !rl.key_prev[key]; }
bool IsKeyReleased(int key)      { return rl_key_ok(key) && !rl.key_down[key] && rl.key_prev[key]; }
bool IsKeyPressedRepeat(int key) { return rl_key_ok(key) && rl.key_repeat[key]; }
int  GetCharPressed(void)        { return rl.char_read < rl.char_count ? rl.chars[rl.char_read++] : 0; }

bool IsMouseButtonDown(int b)     { return rl_button_ok(b) && rl.button_down[b]; }
bool IsMouseButtonPressed(int b)  { return rl_button_ok(b) && rl.button_down[b] && !rl.button_prev[b]; }
bool IsMouseButtonReleased(int b) { return rl_button_ok(b) && !rl.button_down[b] && rl.button_prev[b]; }
Vector2 GetMousePosition(void)    { return rl.mouse; }
Vector2 GetMouseDelta(void) {
    return (Vector2){ rl.mouse.x - rl.mouse_prev.x, rl.mouse.y - rl.mouse_prev.y };
}
Vector2 GetMouseWheelMoveV(void)  { return rl.wheel; }

// EndDrawing: the frame's work and its pacing, Raylib's frame time, then
// Raylib's own poll. Raylib measures its frame time here, from one
// EndDrawing to the next, so a wait made after EndDrawing reaches it only at
// the following one.
static void rl_end_drawing(void) {
    rl.clock += RL_FRAME;
    if (rl.frame_time_live) {
        if (rl.last_end > 0.0) rl.frame_time = (float)(rl.clock - rl.last_end);
        rl.last_end = rl.clock;
    }
    PollInputEvents();
}

bool WindowShouldClose(void)           { return rl.should_close; }
bool IsWindowResized(void)             { return rl.resized; }
bool IsWindowFocused(void)             { return rl.focused; }
bool IsWindowState(unsigned int flag)  { return flag == FLAG_WINDOW_HIGHDPI && rl.high_dpi; }
Vector2 GetWindowScaleDPI(void)        { return (Vector2){ rl.dpi, rl.dpi }; }

double GetTime(void)             { return rl.clock; }
float  GetFrameTime(void)        { return rl.frame_time; }
void   EnableEventWaiting(void)  { rl.event_waiting = true; }
void   DisableEventWaiting(void) { rl.event_waiting = false; }
void   WaitTime(double seconds) {
    if (seconds > rl.longest_sleep) rl.longest_sleep = seconds;
    rl.clock += seconds;
}

// ----------------------------------------------------------------------------
// Text: what the adapter's text path calls. The glyph search counts its
// calls, and the two draw calls record what they were asked to draw.
// ----------------------------------------------------------------------------

#define RT_QUADS 64

typedef struct {
    Rectangle    src, dst;
    Color        tint;
    unsigned int texture;
} RT_Quad;

static struct {
    Font    default_font;
    RT_Quad quads[RT_QUADS];
    int     quad_count;
    int     lookups;            // GetGlyphIndex calls
    int     draw_text_ex;       // DrawTextEx calls
    char    draw_text_ex_text[64];
} rt;

Font GetFontDefault(void) { return rt.default_font; }

// Raylib's search: the first glyph of that codepoint, else its '?', else 0.
int GetGlyphIndex(Font font, int codepoint) {
    int index = 0, fallback = 0;
    rt.lookups++;
    if (font.baseSize <= 0 || font.glyphCount <= 0 || font.recs == NULL || font.glyphs == NULL) return 0;
    for (int i = 0; i < font.glyphCount; i++) {
        if (font.glyphs[i].value == 63) fallback = i;
        if (font.glyphs[i].value == codepoint) { index = i; break; }
    }
    if (index == 0 && font.glyphs[0].value != codepoint) index = fallback;
    return index;
}

// Raylib's decoder: '?' and one byte for a sequence it cannot read.
int GetCodepointNext(const char *text, int *codepointSize) {
    const unsigned char *p = (const unsigned char *)text;
    *codepointSize = 1;
    if ((p[0] & 0xf8) == 0xf0) {
        if ((p[1] & 0xc0) != 0x80 || (p[2] & 0xc0) != 0x80 || (p[3] & 0xc0) != 0x80) return 0x3f;
        *codepointSize = 4;
        return ((p[0] & 0x07) << 18) | ((p[1] & 0x3f) << 12) | ((p[2] & 0x3f) << 6) | (p[3] & 0x3f);
    }
    if ((p[0] & 0xf0) == 0xe0) {
        if ((p[1] & 0xc0) != 0x80 || (p[2] & 0xc0) != 0x80) return 0x3f;
        *codepointSize = 3;
        return ((p[0] & 0x0f) << 12) | ((p[1] & 0x3f) << 6) | (p[2] & 0x3f);
    }
    if ((p[0] & 0xe0) == 0xc0) {
        if ((p[1] & 0xc0) != 0x80) return 0x3f;
        *codepointSize = 2;
        return ((p[0] & 0x1f) << 6) | (p[1] & 0x3f);
    }
    return (p[0] & 0x80) == 0 ? p[0] : 0x3f;
}

void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
    (void)font; (void)position; (void)fontSize; (void)spacing; (void)tint;
    rt.draw_text_ex++;
    snprintf(rt.draw_text_ex_text, sizeof rt.draw_text_ex_text, "%s", text != NULL ? text : "");
}

void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
    (void)origin; (void)rotation;
    if (rt.quad_count < RT_QUADS) rt.quads[rt.quad_count] = (RT_Quad){ source, dest, tint, texture.id };
    rt.quad_count++;
}

// A font whose numbers place by hand: base size 10, padding 1, texture 7.
// Glyph k is codepoint cps[k], with atlas rect {10k, 0, 4+k, 8}, offset
// (k, 2) and advance 6+k.
#define RT_GLYPHS 8

typedef struct {
    Font      font;
    GlyphInfo glyphs[RT_GLYPHS];
    Rectangle recs[RT_GLYPHS];
} RT_Font;

static void rt_font_set(RT_Font *f, const int *cps, int count) {
    memset(f->glyphs, 0, sizeof f->glyphs);
    for (int k = 0; k < count; k++) {
        f->glyphs[k] = (GlyphInfo){ .value = cps[k], .offsetX = k, .offsetY = 2, .advanceX = 6 + k };
        f->recs[k] = (Rectangle){ 10.0f * (float)k, 0.0f, 4.0f + (float)k, 8.0f };
    }
    f->font = (Font){ .baseSize = 10, .glyphCount = count, .glyphPadding = 1,
                      .texture = { .id = 7, .width = 256, .height = 16 },
                      .recs = f->recs, .glyphs = f->glyphs };
}

static void rt_reset(void) {
    memset(&rt, 0, sizeof rt);
    wlx_raylib_text_cache_clear();
}

static WLX_Text_Style rt_style(RT_Font *f) {
    return (WLX_Text_Style){ .font = wlx_font_from_raylib(&f->font), .font_size = 20,
                             .color = { 10, 20, 30, 255 }, .spacing = 3 };
}

// Draws at (100, 50), size 20 (scale 2) with 3 px of spacing.
static void rt_draw(RT_Font *f, const char *text) {
    wlx_raylib_draw_text_slice(text, strlen(text), 100.0f, 50.0f, rt_style(f), NULL);
}

// The atlas rect a quad was cut from names its glyph: src.x is 10k - 1.
static int rt_quad_glyph(int quad) {
    return (int)((rt.quads[quad].src.x + 1.0f) / 10.0f + 0.5f);
}

// ============================================================================
// The application: one loop iteration the way a Raylib program runs it
// ============================================================================

#define RA_FRAMES 512

typedef struct {
    bool   call_helper;
    float  request;          // > 0: every frame asks for one this far ahead, like a caret
    int    own_key;          // a key the application reads with IsKeyPressed itself

    int    frames;
    double frame_clock[RA_FRAMES];
    float  frame_dt[RA_FRAMES];
    char   text[256];
    size_t text_len;
    double text_clock;       // when the first typed text reached a frame
    int    presses[WLX_KEY_COUNT];
    int    repeats, clicks, right_clicks, middle_clicks, own_key_presses, resizes;
    float  wheel;
    bool   closed;
    int    helper_polls;     // polls made inside the helper
} RA_App;

static void ra_loop_once(WLX_Context *ctx, RA_App *app) {
    // The loop top, where a Raylib application looks for itself. A close
    // request ends the loop there, as while (!WindowShouldClose()) does.
    if (WindowShouldClose()) { app->closed = true; return; }
    if (IsWindowResized()) app->resizes++;
    if (app->own_key != 0 && IsKeyPressed(app->own_key)) app->own_key_presses++;

    wlx_begin(ctx, (WLX_Rect){ 0, 0, 400, 300 }, wlx_process_raylib_input);
    if (app->frames < RA_FRAMES) {
        app->frame_clock[app->frames] = rl.clock;
        app->frame_dt[app->frames] = wlx_get_frame_time(ctx);
    }
    app->frames++;

    size_t n = strlen(ctx->input.text_input);
    if (n > 0 && app->text_len + n < sizeof app->text) {
        if (app->text_len == 0) app->text_clock = rl.clock;
        memcpy(app->text + app->text_len, ctx->input.text_input, n);
        app->text_len += n;
    }
    for (int k = 1; k < WLX_KEY_COUNT; k++) {
        if (ctx->input.keys_pressed[k]) app->presses[k]++;
        if (ctx->input.keys_repeated[k]) app->repeats++;
    }
    if (ctx->input.mouse_clicked) app->clicks++;
    if (ctx->input.mouse_right_clicked) app->right_clicks++;
    if (ctx->input.mouse_middle_clicked) app->middle_clicks++;
    app->wheel += ctx->input.wheel_delta;

    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_label(ctx, "idle", .height = 40);
    wlx_layout_end(ctx);
    wlx_end(ctx);
    if (app->request > 0.0f) wlx_request_frame_after(ctx, app->request);

    rl_end_drawing();

    if (app->call_helper) {
        int before = rl.polls;
        wlx_raylib_wait_frame(ctx);
        app->helper_polls += rl.polls - before;
    }
}

// Runs the loop until the clock reaches `until`, the window closes or the
// frame budget runs out. A script ends with an event at `until`, so a
// blocking wait has something to wake it.
static void ra_run(WLX_Context *ctx, RA_App *app, double until, int max_frames) {
    while (rl.clock < until && !app->closed && app->frames < max_frames) ra_loop_once(ctx, app);
}

static void ra_ctx_init(WLX_Context *ctx) {
    test_ctx_init(ctx, 400, 300);
    ctx->backend.get_frame_time = wlx_raylib_get_frame_time;
}

// A frame began within two frame times after `at`.
static bool ra_frame_soon_after(const RA_App *app, double at) {
    for (int i = 0; i < app->frames && i < RA_FRAMES; i++) {
        if (app->frame_clock[i] >= at && app->frame_clock[i] <= at + 2.0 * RL_FRAME + 1e-9) return true;
    }
    return false;
}

// ============================================================================
// Tests
// ============================================================================

// An untouched UI stops producing frames: the helper blocks in one poll
// under event waiting, returns when an event ends that, and leaves event
// waiting switched off for EndDrawing's own poll.
TEST(raylib_wait_blocks_until_an_event) {
    WLX_Context ctx;
    RA_App app = { .call_helper = true };
    rl_reset();
    ra_ctx_init(&ctx);
    rl_push(5.0, EV_OTHER, 0, 0, 0);

    ra_run(&ctx, &app, 5.0, 50);

    ASSERT_TRUE(app.frames <= 3);            // the first frame and its confirmation
    ASSERT_EQ_INT(1, rl.blocking_polls);
    ASSERT_EQ_INT(1, app.helper_polls);
    ASSERT_TRUE(!rl.stuck);
    ASSERT_TRUE(!rl.event_waiting);
    ASSERT_TRUE(rl.clock >= 5.0);
    wlx_context_destroy(&ctx);
}

// Typed text with no key edge beside it (committed input-method text, a key
// Wollix does not map) arrives whole and in order through a timed wait: two
// characters in one poll, one in the next slice, a multi-byte one, and a
// burst that fills Raylib's queue.
TEST(raylib_wait_keeps_typed_text) {
    WLX_Context ctx;
    RA_App app = { .call_helper = true, .request = 0.5f };
    rl_reset();
    ra_ctx_init(&ctx);
    rl_push(1.000, EV_CHAR, 'a', 0, 0);
    rl_push(1.004, EV_CHAR, 'b', 0, 0);
    rl_push(1.030, EV_CHAR, 'c', 0, 0);
    rl_push(2.000, EV_CHAR, 0x4E2D, 0, 0);
    for (int i = 0; i < 16; i++) rl_push(3.0, EV_CHAR, 'A' + i, 0, 0);
    rl_push(4.0, EV_OTHER, 0, 0, 0);

    ra_run(&ctx, &app, 4.0, RA_FRAMES);

    app.text[app.text_len] = '\0';
    ASSERT_EQ_STR("abc\xE4\xB8\xAD" "ABCDEFGHIJKLMNOP", app.text);
    // The first character reached a frame within one slice of being typed.
    ASSERT_TRUE(app.text_clock >= 1.0 && app.text_clock <= 1.0 + RL_FRAME + 1e-9);
    ASSERT_EQ_INT(0, rl.blocking_polls);
    wlx_context_destroy(&ctx);
}

// Edges survive a timed wait: a mapped key, a key only the application
// reads, a mouse click, key repeats and wheel notches. Each would be erased
// by the poll after the one that brought it.
TEST(raylib_wait_keeps_edges) {
    WLX_Context ctx;
    RA_App app = { .call_helper = true, .request = 0.5f, .own_key = KEY_GRAVE };
    rl_reset();
    ra_ctx_init(&ctx);
    rl_push(1.00, EV_KEY_DOWN, KEY_A, 0, 0);
    rl_push(1.05, EV_KEY_UP, KEY_A, 0, 0);
    rl_push(1.50, EV_KEY_DOWN, KEY_GRAVE, 0, 0);
    rl_push(1.55, EV_KEY_UP, KEY_GRAVE, 0, 0);
    rl_push(2.00, EV_BUTTON_DOWN, MOUSE_BUTTON_LEFT, 0, 0);
    rl_push(2.05, EV_BUTTON_UP, MOUSE_BUTTON_LEFT, 0, 0);
    rl_push(2.50, EV_KEY_DOWN, KEY_X, 0, 0);
    rl_push(3.00, EV_KEY_REPEAT, KEY_X, 0, 0);
    rl_push(3.05, EV_KEY_REPEAT, KEY_X, 0, 0);
    rl_push(3.10, EV_KEY_REPEAT, KEY_X, 0, 0);
    rl_push(3.15, EV_KEY_UP, KEY_X, 0, 0);
    rl_push(3.50, EV_WHEEL, 0, 0, -1.0f);
    rl_push(3.55, EV_WHEEL, 0, 0, -1.0f);
    rl_push(3.60, EV_WHEEL, 0, 0, -1.0f);
    rl_push(4.00, EV_OTHER, 0, 0, 0);

    ra_run(&ctx, &app, 4.0, RA_FRAMES);

    ASSERT_EQ_INT(1, app.presses[WLX_KEY_A]);
    ASSERT_EQ_INT(1, app.presses[WLX_KEY_X]);
    ASSERT_EQ_INT(1, app.own_key_presses);
    ASSERT_EQ_INT(1, app.clicks);
    ASSERT_EQ_INT(3, app.repeats);
    ASSERT_EQ_F(-3.0f, app.wheel, 0.0f);
    wlx_context_destroy(&ctx);
}

// A resize flag and a close request live for one poll. The application
// still finds them at the top of its loop, after a timed wait and after a
// blocking one.
TEST(raylib_wait_keeps_resize_and_close) {
    WLX_Context ctx;
    RA_App timed = { .call_helper = true, .request = 0.5f };
    rl_reset();
    ra_ctx_init(&ctx);
    rl_push(1.0, EV_RESIZE, 0, 0, 0);
    rl_push(2.0, EV_CLOSE, 0, 0, 0);
    ra_run(&ctx, &timed, 10.0, RA_FRAMES);
    ASSERT_EQ_INT(1, timed.resizes);
    ASSERT_TRUE(timed.closed);
    ASSERT_TRUE(rl.clock < 2.0 + 2.0 * RL_FRAME);
    wlx_context_destroy(&ctx);

    RA_App blocking = { .call_helper = true };
    rl_reset();
    ra_ctx_init(&ctx);
    rl_push(1.0, EV_RESIZE, 0, 0, 0);
    rl_push(2.0, EV_CLOSE, 0, 0, 0);
    ra_run(&ctx, &blocking, 10.0, RA_FRAMES);
    ASSERT_EQ_INT(1, blocking.resizes);
    ASSERT_TRUE(blocking.closed);
    ASSERT_TRUE(!rl.stuck);
    wlx_context_destroy(&ctx);
}

// A focus change and a content-scale change end a timed wait, long before
// its delay runs out.
TEST(raylib_wait_ends_on_focus_and_scale) {
    WLX_Context ctx;
    RA_App app = { .call_helper = true, .request = 10.0f };
    rl_reset();
    rl.high_dpi = true;
    ra_ctx_init(&ctx);
    rl_push(1.0, EV_FOCUS, 0, 0, 0);
    rl_push(2.0, EV_SCALE, 0, 2.0f, 0);
    rl_push(3.0, EV_MOVE, 0, 40.0f, 50.0f);
    rl_push(4.0, EV_CLOSE, 0, 0, 0);

    ra_run(&ctx, &app, 10.0, RA_FRAMES);

    ASSERT_TRUE(ra_frame_soon_after(&app, 1.0));
    ASSERT_TRUE(ra_frame_soon_after(&app, 2.0));
    ASSERT_TRUE(ra_frame_soon_after(&app, 3.0));
    ASSERT_TRUE(app.closed);
    ASSERT_TRUE(app.frames < 30);            // it rested between them
    wlx_context_destroy(&ctx);
}

// With a frame scheduled and nothing arriving, the helper returns once the
// delay has passed: not before it, and no later than one slice after. It
// sleeps in slices of at most a sixtieth of a second and never blocks.
TEST(raylib_wait_returns_when_the_delay_passes) {
    WLX_Context ctx;
    RA_App app = { .call_helper = true, .request = 0.25f };
    rl_reset();
    ra_ctx_init(&ctx);
    rl_push(3.0, EV_CLOSE, 0, 0, 0);

    ra_run(&ctx, &app, 10.0, RA_FRAMES);

    ASSERT_TRUE(app.frames >= 8 && app.frames <= 16);
    for (int i = 3; i < app.frames - 1; i++) {
        double gap = app.frame_clock[i] - app.frame_clock[i - 1];
        ASSERT_TRUE(gap >= 0.25 + RL_FRAME - 1e-9);
        ASSERT_TRUE(gap <= 0.25 + 2.0 * RL_FRAME + 1e-9);
    }
    ASSERT_TRUE(rl.longest_sleep <= RL_FRAME + 1e-9);
    ASSERT_EQ_INT(0, rl.blocking_polls);
    wlx_context_destroy(&ctx);
}

// Input that EndDrawing's own poll brought belongs to the next frame: the
// helper returns without a poll of its own, even with nothing scheduled.
TEST(raylib_wait_leaves_the_frame_poll_alone) {
    WLX_Context ctx;
    RA_App app = { .call_helper = true };
    rl_reset();
    ra_ctx_init(&ctx);
    // Frame 2 begins at one frame time and ends with nothing scheduled; its
    // EndDrawing poll, at two frame times, delivers this character.
    rl_push(1.5 * RL_FRAME, EV_CHAR, 'q', 0, 0);
    rl_push(1.0, EV_OTHER, 0, 0, 0);

    ra_run(&ctx, &app, 1.0, 50);

    app.text[app.text_len] = '\0';
    ASSERT_EQ_STR("q", app.text);
    ASSERT_TRUE(app.text_clock <= 2.0 * RL_FRAME + 1e-9);
    ASSERT_EQ_INT(1, app.helper_polls);      // only the block that followed
    ASSERT_TRUE(!rl.stuck);
    wlx_context_destroy(&ctx);
}

// In a loop that calls the helper the frame time is real time, the wait
// included, whatever GetFrameTime says: the stand-in's never moves from a
// sixtieth of a second. The sum of the frame times is the run's length.
TEST(raylib_frame_time_covers_the_wait) {
    WLX_Context ctx;
    RA_App app = { .call_helper = true, .request = 0.3f };
    rl_reset();
    ra_ctx_init(&ctx);
    rl_push(1.0, EV_KEY_DOWN, KEY_A, 0, 0);
    rl_push(1.2, EV_KEY_UP, KEY_A, 0, 0);
    rl_push(2.0, EV_OTHER, 0, 0, 0);
    ra_run(&ctx, &app, 2.0, RA_FRAMES);
    app.request = 0.0f;                      // from here it blocks
    rl_push(5.0, EV_MOVE, 0, 10.0f, 10.0f);
    rl_push(9.0, EV_CLOSE, 0, 0, 0);
    ra_run(&ctx, &app, 10.0, RA_FRAMES);

    ASSERT_TRUE(app.closed);
    ASSERT_TRUE(app.frames > 8 && app.frames < RA_FRAMES);
    double sum = 0.0;
    float longest = 0.0f;
    for (int i = 2; i < app.frames; i++) {
        ASSERT_EQ_F((float)(app.frame_clock[i] - app.frame_clock[i - 1]), app.frame_dt[i], 1e-4f);
        sum += (double)app.frame_dt[i];
        if (app.frame_dt[i] > longest) longest = app.frame_dt[i];
    }
    ASSERT_EQ_F((float)(app.frame_clock[app.frames - 1] - app.frame_clock[1]), (float)sum, 1e-3f);
    ASSERT_TRUE(longest > 2.5f);             // the blocks of about three and four seconds
    wlx_context_destroy(&ctx);
}

// Every kind of edge ends a timed wait at once: a button's release as well
// as its press, every mouse button, not only the left, and keys from 256
// up (Enter, Backspace, the arrows, the function keys), each of which a
// later poll would erase. With ten seconds scheduled, only the watch can
// end the wait early.
TEST(raylib_wait_ends_on_every_edge) {
    WLX_Context ctx;
    RA_App app = { .call_helper = true, .request = 10.0f };
    rl_reset();
    ra_ctx_init(&ctx);
    static const double at[] = { 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5,
                                 5.0, 5.5, 6.0, 6.5, 7.0, 7.5, 8.0, 8.5 };
    rl_push(at[0],  EV_BUTTON_DOWN, MOUSE_BUTTON_LEFT, 0, 0);
    rl_push(at[1],  EV_BUTTON_UP,   MOUSE_BUTTON_LEFT, 0, 0);
    rl_push(at[2],  EV_BUTTON_DOWN, MOUSE_BUTTON_RIGHT, 0, 0);
    rl_push(at[3],  EV_BUTTON_UP,   MOUSE_BUTTON_RIGHT, 0, 0);
    rl_push(at[4],  EV_BUTTON_DOWN, MOUSE_BUTTON_MIDDLE, 0, 0);
    rl_push(at[5],  EV_BUTTON_UP,   MOUSE_BUTTON_MIDDLE, 0, 0);
    rl_push(at[6],  EV_BUTTON_DOWN, MOUSE_BUTTON_SIDE, 0, 0);
    rl_push(at[7],  EV_BUTTON_UP,   MOUSE_BUTTON_SIDE, 0, 0);
    rl_push(at[8],  EV_KEY_DOWN, KEY_ENTER, 0, 0);
    rl_push(at[9],  EV_KEY_UP,   KEY_ENTER, 0, 0);
    rl_push(at[10], EV_KEY_DOWN, KEY_BACKSPACE, 0, 0);
    rl_push(at[11], EV_KEY_UP,   KEY_BACKSPACE, 0, 0);
    rl_push(at[12], EV_KEY_DOWN, KEY_RIGHT, 0, 0);
    rl_push(at[13], EV_KEY_UP,   KEY_RIGHT, 0, 0);
    rl_push(at[14], EV_KEY_DOWN, KEY_F1, 0, 0);
    rl_push(at[15], EV_KEY_UP,   KEY_F1, 0, 0);
    rl_push(9.0, EV_CLOSE, 0, 0, 0);

    ra_run(&ctx, &app, 20.0, RA_FRAMES);

    for (size_t i = 0; i < sizeof at / sizeof at[0]; i++) ASSERT_TRUE(ra_frame_soon_after(&app, at[i]));
    ASSERT_EQ_INT(1, app.clicks);
    ASSERT_EQ_INT(1, app.right_clicks);
    ASSERT_EQ_INT(1, app.middle_clicks);
    ASSERT_EQ_INT(1, app.presses[WLX_KEY_ENTER]);
    ASSERT_EQ_INT(1, app.presses[WLX_KEY_BACKSPACE]);
    ASSERT_EQ_INT(1, app.presses[WLX_KEY_RIGHT]);
    ASSERT_EQ_INT(1, app.presses[WLX_KEY_F1]);
    ASSERT_TRUE(app.closed);
    ASSERT_TRUE(app.frames < 80);            // it rested between them
    wlx_context_destroy(&ctx);
}

// A loop that calls the helper while idle and skips it while it animates
// (if (!animating) wlx_raylib_wait_frame(ctx);). Raylib's GetFrameTime
// takes a wait in at the EndDrawing after it, so a frame that fell back to
// it after the switch would report the rest a second time. From the first
// call on the frame time is the adapter's own, in every iteration: each
// frame's time is the time since the frame before, once.
TEST(raylib_frame_time_stays_real_when_the_helper_is_skipped) {
    WLX_Context ctx;
    RA_App app = { .call_helper = true };
    rl_reset();
    rl.frame_time_live = true;
    ra_ctx_init(&ctx);
    rl_push(3.0, EV_BUTTON_DOWN, MOUSE_BUTTON_LEFT, 0, 0);   // ends a rest of about 3 s
    ra_run(&ctx, &app, 3.0, 50);
    ASSERT_TRUE(rl.blocking_polls == 1);

    app.call_helper = false;                  // the click starts an animation
    ra_run(&ctx, &app, 3.5, RA_FRAMES);

    ASSERT_TRUE(app.frames > 20 && app.frames < RA_FRAMES);
    double sum = 0.0;
    for (int i = 1; i < app.frames; i++) {
        ASSERT_EQ_F((float)(app.frame_clock[i] - app.frame_clock[i - 1]), app.frame_dt[i], 1e-4f);
        sum += (double)app.frame_dt[i];
    }
    ASSERT_EQ_F((float)(app.frame_clock[app.frames - 1] - app.frame_clock[0]), (float)sum, 1e-3f);
    wlx_context_destroy(&ctx);
}

// A loop that never calls the helper gets GetFrameTime, exactly.
TEST(raylib_frame_time_is_raylibs_without_the_helper) {
    WLX_Context ctx;
    RA_App app = { .call_helper = false };
    rl_reset();
    rl.frame_time = 0.0421f;
    ra_ctx_init(&ctx);

    ra_run(&ctx, &app, 0.5, 20);

    ASSERT_EQ_INT(20, app.frames);
    for (int i = 0; i < app.frames; i++) ASSERT_EQ_F(0.0421f, app.frame_dt[i], 0.0f);
    ASSERT_EQ_INT(0, app.helper_polls);
    wlx_context_destroy(&ctx);
}

// ----------------------------------------------------------------------------
// Text draw: Raylib's DrawTextEx placement, one glyph lookup per codepoint
// ----------------------------------------------------------------------------

#define RT_ASSERT_RECT(r, ex, ey, ew, eh) do {                                 \
    ASSERT_EQ_F((r).x, (ex), 0.0f); ASSERT_EQ_F((r).y, (ey), 0.0f);            \
    ASSERT_EQ_F((r).width, (ew), 0.0f); ASSERT_EQ_F((r).height, (eh), 0.0f);   \
} while (0)

// Every quad where DrawTextEx and DrawTextCodepoint put it, worked out by
// hand: the padding widens the atlas rect by one on each side, the offset
// and the padding scale with the size, a space advances without a quad, and
// a glyph with no advance of its own advances by its atlas width.
TEST(raylib_text_places_glyphs_like_draw_text_ex) {
    RT_Font f;
    rt_reset();
    rt_font_set(&f, (int[]){ 'A', 'b', ' ', '?', 'x' }, 5);
    f.glyphs[4].advanceX = 0;

    rt_draw(&f, "Ab xA");

    ASSERT_EQ_INT(0, rt.draw_text_ex);
    ASSERT_EQ_INT(4, rt.quad_count);
    // 'A', glyph 0, at pen 0.
    RT_ASSERT_RECT(rt.quads[0].src, -1.0f, -1.0f, 6.0f, 10.0f);
    RT_ASSERT_RECT(rt.quads[0].dst, 98.0f, 52.0f, 12.0f, 20.0f);
    // 'b', glyph 1, at pen 6*2 + 3 = 15.
    RT_ASSERT_RECT(rt.quads[1].src, 9.0f, -1.0f, 7.0f, 10.0f);
    RT_ASSERT_RECT(rt.quads[1].dst, 115.0f, 52.0f, 14.0f, 20.0f);
    // The space moved the pen from 32 to 51. 'x', glyph 4.
    RT_ASSERT_RECT(rt.quads[2].src, 39.0f, -1.0f, 10.0f, 10.0f);
    RT_ASSERT_RECT(rt.quads[2].dst, 157.0f, 52.0f, 20.0f, 20.0f);
    // 'x' advanced by its atlas width: 8*2 + 3, so the pen is at 70.
    RT_ASSERT_RECT(rt.quads[3].dst, 168.0f, 52.0f, 12.0f, 20.0f);

    ASSERT_EQ_INT(7, (int)rt.quads[0].texture);
    ASSERT_EQ_INT(10, rt.quads[0].tint.r);
    ASSERT_EQ_INT(20, rt.quads[0].tint.g);
    ASSERT_EQ_INT(30, rt.quads[0].tint.b);
    ASSERT_EQ_INT(255, rt.quads[0].tint.a);
}

// The search runs once for a codepoint, however often it is drawn.
// DrawTextEx ran it twice for every drawn glyph.
TEST(raylib_text_looks_a_codepoint_up_once) {
    RT_Font f;
    rt_reset();
    rt_font_set(&f, (int[]){ 'A', 'b', ' ' }, 3);

    rt_draw(&f, "AAAA");
    ASSERT_EQ_INT(1, rt.lookups);
    ASSERT_EQ_INT(4, rt.quad_count);

    rt_draw(&f, "Ab bA");
    ASSERT_EQ_INT(3, rt.lookups);

    for (int i = 0; i < 10; i++) rt_draw(&f, "Ab bA");
    ASSERT_EQ_INT(3, rt.lookups);
}

// A font replaced in place, its glyph array at the same address in another
// order, never draws from the old order, and no flush is needed for that.
TEST(raylib_text_table_follows_a_font_changed_in_place) {
    RT_Font f;
    rt_reset();
    rt_font_set(&f, (int[]){ 'A', 'B', 'C' }, 3);
    rt_draw(&f, "B");
    ASSERT_EQ_INT(1, rt_quad_glyph(0));

    rt_font_set(&f, (int[]){ 'C', 'A', 'B' }, 3);
    rt_draw(&f, "B");
    ASSERT_EQ_INT(2, rt_quad_glyph(1));
    rt_draw(&f, "A");
    ASSERT_EQ_INT(1, rt_quad_glyph(2));

    // Fewer glyphs than the remembered index: it is not read.
    rt_font_set(&f, (int[]){ 'B' }, 1);
    rt_draw(&f, "B");
    ASSERT_EQ_INT(0, rt_quad_glyph(3));
}

// A codepoint the font lacks draws Raylib's '?' and is searched for again
// the next time, so a glyph that appears later is found.
TEST(raylib_text_missing_codepoint_is_not_remembered) {
    RT_Font f;
    rt_reset();
    rt_font_set(&f, (int[]){ 'A', '?' }, 2);

    rt_draw(&f, "Z");
    ASSERT_EQ_INT(1, rt_quad_glyph(0));
    ASSERT_EQ_INT(1, rt.lookups);
    rt_draw(&f, "Z");
    ASSERT_EQ_INT(2, rt.lookups);

    rt_font_set(&f, (int[]){ 'A', '?', 'Z' }, 3);
    rt_draw(&f, "Z");
    ASSERT_EQ_INT(2, rt_quad_glyph(2));
}

// A codepoint past the table's range still draws, through the search.
TEST(raylib_text_codepoint_above_the_table_is_searched) {
    RT_Font f;
    rt_reset();
    ASSERT_TRUE(0x416 >= WLX_RAYLIB_GLYPH_TABLE_CODEPOINTS);
    rt_font_set(&f, (int[]){ 'A', 0x416 }, 2);

    rt_draw(&f, "\xD0\x96");            // U+0416
    ASSERT_EQ_INT(1, rt.quad_count);
    ASSERT_EQ_INT(1, rt_quad_glyph(0));
    ASSERT_EQ_INT(1, rt.lookups);
    rt_draw(&f, "\xD0\x96");
    ASSERT_EQ_INT(2, rt.lookups);
}

// What the loop does not reproduce goes to DrawTextEx whole: a line break
// (Raylib keeps its line spacing private) and a font Raylib cannot index.
TEST(raylib_text_hands_line_breaks_and_bad_fonts_to_raylib) {
    RT_Font f;
    rt_reset();
    rt_font_set(&f, (int[]){ 'A', 'b' }, 2);

    rt_draw(&f, "A\nb");
    ASSERT_EQ_INT(1, rt.draw_text_ex);
    ASSERT_EQ_STR(rt.draw_text_ex_text, "A\nb");
    ASSERT_EQ_INT(0, rt.quad_count);

    f.font.recs = NULL;
    rt_draw(&f, "Ab");
    ASSERT_EQ_INT(2, rt.draw_text_ex);
    ASSERT_EQ_INT(0, rt.quad_count);
}

// A font without a texture draws from Raylib's default font, as DrawTextEx
// has it.
TEST(raylib_text_without_a_texture_uses_the_default_font) {
    RT_Font f, def;
    rt_reset();
    rt_font_set(&def, (int[]){ 'b', 'A' }, 2);
    def.font.texture.id = 9;
    rt.default_font = def.font;
    rt_font_set(&f, (int[]){ 'A', 'b' }, 2);
    f.font.texture.id = 0;

    rt_draw(&f, "A");
    ASSERT_EQ_INT(1, rt.quad_count);
    ASSERT_EQ_INT(9, (int)rt.quads[0].texture);
    ASSERT_EQ_INT(1, rt_quad_glyph(0));
}

// More fonts than table slots: each still draws its own glyphs, round after
// round, while the slots change hands.
TEST(raylib_text_more_fonts_than_tables) {
    enum { FONTS = WLX_RAYLIB_GLYPH_TABLE_FONTS + 3 };
    static RT_Font fonts[FONTS];
    static const int orders[3][3] = { { 'A', 'B', 'C' }, { 'C', 'A', 'B' }, { 'B', 'C', 'A' } };
    rt_reset();
    for (int i = 0; i < FONTS; i++) rt_font_set(&fonts[i], orders[i % 3], 3);

    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < FONTS; i++) {
            rt.quad_count = 0;
            rt_draw(&fonts[i], "AB");
            ASSERT_EQ_INT(2, rt.quad_count);
            ASSERT_EQ_INT(i % 3, rt_quad_glyph(0));            // 'A' is glyph 0, 1, 2
            ASSERT_EQ_INT((i % 3 + 1) % 3, rt_quad_glyph(1));  // 'B' is glyph 1, 2, 0
        }
    }
}

// wlx_raylib_text_cache_clear releases the tables with the measurements.
TEST(raylib_text_cache_clear_releases_the_tables) {
    RT_Font f;
    rt_reset();
    rt_font_set(&f, (int[]){ 'A' }, 1);

    rt_draw(&f, "A");
    rt_draw(&f, "A");
    ASSERT_EQ_INT(1, rt.lookups);
    wlx_raylib_text_cache_clear();
    rt_draw(&f, "A");
    ASSERT_EQ_INT(2, rt.lookups);
}

// The advances callback reads the same table and keeps MeasureTextEx's sum:
// the glyph advances at base size, scaled once, plus the spacing between
// codepoints.
TEST(raylib_text_advances_share_the_table) {
    RT_Font f;
    float advances[2] = { 0 };
    rt_reset();
    rt_font_set(&f, (int[]){ 'A', 'b' }, 2);
    rt_draw(&f, "Ab");
    ASSERT_EQ_INT(2, rt.lookups);

    size_t filled = wlx_raylib_measure_text_advances("Ab", 2, rt_style(&f),
            (size_t[]){ 1, 2 }, 2, advances, NULL);

    ASSERT_EQ_INT(2, (int)filled);
    ASSERT_EQ_F(advances[0], 12.0f, 0.0f);      // 6 * 2
    ASSERT_EQ_F(advances[1], 29.0f, 0.0f);      // (6 + 7) * 2 + 3
    ASSERT_EQ_INT(2, rt.lookups);
}

SUITE(adapter_raylib) {
    RUN_TEST(raylib_text_places_glyphs_like_draw_text_ex);
    RUN_TEST(raylib_text_looks_a_codepoint_up_once);
    RUN_TEST(raylib_text_table_follows_a_font_changed_in_place);
    RUN_TEST(raylib_text_missing_codepoint_is_not_remembered);
    RUN_TEST(raylib_text_codepoint_above_the_table_is_searched);
    RUN_TEST(raylib_text_hands_line_breaks_and_bad_fonts_to_raylib);
    RUN_TEST(raylib_text_without_a_texture_uses_the_default_font);
    RUN_TEST(raylib_text_more_fonts_than_tables);
    RUN_TEST(raylib_text_cache_clear_releases_the_tables);
    RUN_TEST(raylib_text_advances_share_the_table);
    RUN_TEST(raylib_wait_blocks_until_an_event);
    RUN_TEST(raylib_wait_keeps_typed_text);
    RUN_TEST(raylib_wait_keeps_edges);
    RUN_TEST(raylib_wait_ends_on_every_edge);
    RUN_TEST(raylib_wait_keeps_resize_and_close);
    RUN_TEST(raylib_wait_ends_on_focus_and_scale);
    RUN_TEST(raylib_wait_returns_when_the_delay_passes);
    RUN_TEST(raylib_wait_leaves_the_frame_poll_alone);
    RUN_TEST(raylib_frame_time_covers_the_wait);
    RUN_TEST(raylib_frame_time_stays_real_when_the_helper_is_skipped);
    RUN_TEST(raylib_frame_time_is_raylibs_without_the_helper);
}

int main(void) {
    RUN_SUITE(adapter_raylib);
    return test_summary();
}
