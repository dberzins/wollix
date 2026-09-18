// test_wasm_clipboard.c - the WASM clipboard transport against a host
// stand-in. Off wasm the import declarations in wollix_wasm.h are plain
// prototypes, so this TU defines the two clipboard imports itself and
// drives wlx_wasm_clipboard_get's grow loop with kilobyte texts. The cap
// override below applies only to the runner's copy of the header and must
// precede the header's first include, which is why test_main.c lists this
// file before test_wasm_pool.c.
//
// Never reference wlx_backend_wasm() here: its initializer takes the address
// of every import wrapper and would pull all the undefined imports into the
// link.

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

// 4096 -> 8192 -> 12001 reaches the geometric grow and the cap short-circuit
// with kilobyte texts (the default is 16 MiB).
#define WLX_WASM_CLIPBOARD_MAX 12000u
#include "wollix_wasm.h"

// ============================================================================
// Host stand-in
// ============================================================================

static const char *g_clip_host_text      = "";
static size_t      g_clip_host_len       = 0;
static int         g_clip_host_calls     = 0;
static char        g_clip_host_set_buf[64];
static size_t      g_clip_host_set_len   = 0;
static int         g_clip_host_set_calls = 0;

// Mirrors clipboard_get_into in web/wollix_wasm.js: copy up to cap bytes and
// never split a UTF-8 sequence at the cap.
uint32_t wlx_wasm_import_clipboard_get_into(char *buf, uint32_t cap) {
    g_clip_host_calls++;
    if (buf == NULL || cap == 0) return 0;
    size_t n = g_clip_host_len < cap ? g_clip_host_len : cap;
    while (n > 0 && n < g_clip_host_len
           && ((unsigned char)g_clip_host_text[n] & 0xC0) == 0x80) {
        n--;
    }
    memcpy(buf, g_clip_host_text, n);
    return (uint32_t)n;
}

void wlx_wasm_import_clipboard_set(const char *text, uint32_t len) {
    g_clip_host_set_calls++;
    g_clip_host_set_len = len < sizeof(g_clip_host_set_buf)
        ? len : sizeof(g_clip_host_set_buf) - 1;
    memcpy(g_clip_host_set_buf, text, g_clip_host_set_len);
    g_clip_host_set_buf[g_clip_host_set_len] = '\0';
}

static void clip_host(const char *text, size_t len) {
    g_clip_host_text  = text;
    g_clip_host_len   = len;
    g_clip_host_calls = 0;
}

// Free the receive buffer so a test starts on the first-fetch path. The
// buffer and its cap are file-scope statics of wollix_wasm.h in this TU.
static void clip_reset(void) {
    if (g_wlx_wasm_clipboard_buf != NULL) wlx_free(g_wlx_wasm_clipboard_buf);
    g_wlx_wasm_clipboard_buf = NULL;
    g_wlx_wasm_clipboard_cap = 0;
}

// A text of `len` bytes. ASCII: a-z cycling. Two-byte: one ASCII byte, then
// 2-byte sequences, so every even cut index >= 2 lands inside a sequence.
static char *clip_make_text(size_t len, bool two_byte) {
    char *t = (char *)malloc(len + 1);
    for (size_t i = 0; i < len; i++) {
        if (!two_byte)      t[i] = (char)('a' + (i % 26));
        else if (i == 0)    t[i] = 'a';
        else                t[i] = (i % 2 == 1) ? (char)0xC3 : (char)0xA9;
    }
    t[len] = '\0';
    return t;
}

// ============================================================================
// wlx_wasm_clipboard_get
// ============================================================================

// A short text fits the first-fetch buffer: one host call, no grow.
TEST(wasm_clipboard_short_text_fetches_once) {
    clip_reset();
    clip_host("hello", 5);
    const char *s = wlx_wasm_clipboard_get(NULL);
    ASSERT_EQ_STR(s, "hello");
    ASSERT_EQ_INT(g_clip_host_calls, 1);
    ASSERT_EQ_INT((int)g_wlx_wasm_clipboard_cap, 4096);
}

// A count inside the 3-byte back-off window of the room may mean truncation:
// the loop grows once and fetches again, and the second fetch settles it.
TEST(wasm_clipboard_backoff_window_refetches) {
    clip_reset();
    char *t = clip_make_text(4093, false);
    clip_host(t, 4093);
    const char *s = wlx_wasm_clipboard_get(NULL);
    ASSERT_EQ_INT((int)strlen(s), 4093);
    ASSERT_TRUE(memcmp(s, t, 4093) == 0);
    ASSERT_EQ_INT(g_clip_host_calls, 2);
    ASSERT_EQ_INT((int)g_wlx_wasm_clipboard_cap, 8192);
    free(t);
}

// A text past two doublings: 4096 -> 8192 -> 12001, three host calls, the
// whole text.
TEST(wasm_clipboard_grows_geometrically_to_fit) {
    clip_reset();
    char *t = clip_make_text(10000, false);
    clip_host(t, 10000);
    const char *s = wlx_wasm_clipboard_get(NULL);
    ASSERT_EQ_INT((int)strlen(s), 10000);
    ASSERT_TRUE(memcmp(s, t, 10000) == 0);
    ASSERT_EQ_INT(g_clip_host_calls, 3);
    ASSERT_EQ_INT((int)g_wlx_wasm_clipboard_cap, 12001);
    free(t);
}

// Past the soft cap the loop stops growing and returns what fits; the host
// backs the cut off a continuation byte, so the text ends on a complete
// sequence and is NUL-terminated.
TEST(wasm_clipboard_stops_at_the_cap_on_a_utf8_boundary) {
    clip_reset();
    char *t = clip_make_text(19999, true);
    clip_host(t, 19999);
    const char *s = wlx_wasm_clipboard_get(NULL);
    ASSERT_EQ_INT((int)strlen(s), 11999);
    ASSERT_TRUE(memcmp(s, t, 11999) == 0);
    ASSERT_EQ_INT(((unsigned char)s[11998] & 0xC0), 0x80);
    ASSERT_EQ_INT(((unsigned char)s[11997] & 0xC0), 0xC0);
    ASSERT_EQ_INT(g_clip_host_calls, 3);
    ASSERT_EQ_INT((int)g_wlx_wasm_clipboard_cap, 12001);
    free(t);
}

// Once grown, the buffer is reused: a later short fetch is one host call and
// the cap stays.
TEST(wasm_clipboard_reuses_the_grown_buffer) {
    clip_reset();
    char *t = clip_make_text(10000, false);
    clip_host(t, 10000);
    (void)wlx_wasm_clipboard_get(NULL);
    ASSERT_EQ_INT((int)g_wlx_wasm_clipboard_cap, 12001);
    clip_host("reuse", 5);
    const char *s = wlx_wasm_clipboard_get(NULL);
    ASSERT_EQ_STR(s, "reuse");
    ASSERT_EQ_INT(g_clip_host_calls, 1);
    ASSERT_EQ_INT((int)g_wlx_wasm_clipboard_cap, 12001);
    free(t);
}

// An empty host clipboard yields "" without growing.
TEST(wasm_clipboard_empty_host_yields_empty_string) {
    clip_reset();
    clip_host("", 0);
    const char *s = wlx_wasm_clipboard_get(NULL);
    ASSERT_EQ_STR(s, "");
    ASSERT_EQ_INT(g_clip_host_calls, 1);
    ASSERT_EQ_INT((int)g_wlx_wasm_clipboard_cap, 4096);
}

// ============================================================================
// wlx_wasm_clipboard_set
// ============================================================================

// The slice reaches the host as (text, len): the length, not the string.
TEST(wasm_clipboard_set_passes_the_slice) {
    g_clip_host_set_calls = 0;
    wlx_wasm_clipboard_set("abc", 2, NULL);
    ASSERT_EQ_INT(g_clip_host_set_calls, 1);
    ASSERT_EQ_INT((int)g_clip_host_set_len, 2);
    ASSERT_EQ_STR(g_clip_host_set_buf, "ab");
}

// ============================================================================
// Suite
// ============================================================================

SUITE(wasm_clipboard) {
    RUN_TEST(wasm_clipboard_short_text_fetches_once);
    RUN_TEST(wasm_clipboard_backoff_window_refetches);
    RUN_TEST(wasm_clipboard_grows_geometrically_to_fit);
    RUN_TEST(wasm_clipboard_stops_at_the_cap_on_a_utf8_boundary);
    RUN_TEST(wasm_clipboard_reuses_the_grown_buffer);
    RUN_TEST(wasm_clipboard_empty_host_yields_empty_string);
    RUN_TEST(wasm_clipboard_set_passes_the_slice);
}
