// test_utf8.c — unit tests for UTF-8 helper functions in wollix.h

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

// ============================================================================
// wlx_utf8_char_len
// ============================================================================

TEST(utf8_char_len_ascii) {
    ASSERT_EQ_INT(1, wlx_utf8_char_len("A"));
    ASSERT_EQ_INT(1, wlx_utf8_char_len(" "));
    ASSERT_EQ_INT(1, wlx_utf8_char_len("~"));
}

TEST(utf8_char_len_2byte) {
    // ö = 0xC3 0xB6
    ASSERT_EQ_INT(2, wlx_utf8_char_len("\xC3\xB6"));
    // ā = 0xC4 0x81
    ASSERT_EQ_INT(2, wlx_utf8_char_len("\xC4\x81"));
}

TEST(utf8_char_len_3byte) {
    // € = 0xE2 0x82 0xAC
    ASSERT_EQ_INT(3, wlx_utf8_char_len("\xE2\x82\xAC"));
}

TEST(utf8_char_len_4byte) {
    // 𐐀 = 0xF0 0x90 0x90 0x80
    ASSERT_EQ_INT(4, wlx_utf8_char_len("\xF0\x90\x90\x80"));
}

TEST(utf8_char_len_invalid) {
    // 0xFF is not a valid UTF-8 lead byte — fallback to 1
    ASSERT_EQ_INT(1, wlx_utf8_char_len("\xFF"));
}

// ============================================================================
// wlx_utf8_decode
// ============================================================================

TEST(utf8_decode_ascii) {
    uint32_t cp = 0;
    size_t n = wlx_utf8_decode("A", &cp);
    ASSERT_EQ_INT(1, n);
    ASSERT_EQ_INT(0x41, cp);
}

TEST(utf8_decode_2byte) {
    uint32_t cp = 0;
    // ö = U+00F6
    size_t n = wlx_utf8_decode("\xC3\xB6", &cp);
    ASSERT_EQ_INT(2, n);
    ASSERT_EQ_INT(0x00F6, cp);
}

TEST(utf8_decode_3byte) {
    uint32_t cp = 0;
    // € = U+20AC
    size_t n = wlx_utf8_decode("\xE2\x82\xAC", &cp);
    ASSERT_EQ_INT(3, n);
    ASSERT_EQ_INT(0x20AC, cp);
}

TEST(utf8_decode_4byte) {
    uint32_t cp = 0;
    // 𐐀 = U+10400
    size_t n = wlx_utf8_decode("\xF0\x90\x90\x80", &cp);
    ASSERT_EQ_INT(4, n);
    ASSERT_EQ_INT(0x10400, cp);
}

TEST(utf8_decode_invalid_lead) {
    uint32_t cp = 0;
    size_t n = wlx_utf8_decode("\xFF", &cp);
    ASSERT_EQ_INT(1, n);
    ASSERT_EQ_INT(0xFFFD, cp);
}

TEST(utf8_decode_truncated_2byte) {
    // 2-byte lead with no continuation byte — next byte is NUL
    uint32_t cp = 0;
    size_t n = wlx_utf8_decode("\xC3", &cp);
    ASSERT_EQ_INT(1, n);
    ASSERT_EQ_INT(0xFFFD, cp);
}

TEST(utf8_decode_truncated_3byte) {
    // 3-byte lead with only 1 continuation
    uint32_t cp = 0;
    size_t n = wlx_utf8_decode("\xE2\x82", &cp);
    ASSERT_EQ_INT(1, n);
    ASSERT_EQ_INT(0xFFFD, cp);
}

// ============================================================================
// wlx_utf8_encode
// ============================================================================

TEST(utf8_encode_ascii) {
    char out[4] = {0};
    size_t n = wlx_utf8_encode(0x41, out);
    ASSERT_EQ_INT(1, n);
    ASSERT_EQ_INT('A', out[0]);
}

TEST(utf8_encode_2byte) {
    char out[4] = {0};
    // ö = U+00F6 → 0xC3 0xB6
    size_t n = wlx_utf8_encode(0x00F6, out);
    ASSERT_EQ_INT(2, n);
    ASSERT_EQ_INT((char)0xC3, out[0]);
    ASSERT_EQ_INT((char)0xB6, out[1]);
}

TEST(utf8_encode_3byte) {
    char out[4] = {0};
    // € = U+20AC → 0xE2 0x82 0xAC
    size_t n = wlx_utf8_encode(0x20AC, out);
    ASSERT_EQ_INT(3, n);
    ASSERT_EQ_INT((char)0xE2, out[0]);
    ASSERT_EQ_INT((char)0x82, out[1]);
    ASSERT_EQ_INT((char)0xAC, out[2]);
}

TEST(utf8_encode_4byte) {
    char out[4] = {0};
    // 𐐀 = U+10400 → 0xF0 0x90 0x90 0x80
    size_t n = wlx_utf8_encode(0x10400, out);
    ASSERT_EQ_INT(4, n);
    ASSERT_EQ_INT((char)0xF0, out[0]);
    ASSERT_EQ_INT((char)0x90, out[1]);
    ASSERT_EQ_INT((char)0x90, out[2]);
    ASSERT_EQ_INT((char)0x80, out[3]);
}

TEST(utf8_encode_invalid) {
    char out[4] = {0};
    // Codepoint > U+10FFFF → encodes U+FFFD (3 bytes: 0xEF 0xBF 0xBD)
    size_t n = wlx_utf8_encode(0x110000, out);
    ASSERT_EQ_INT(3, n);
    ASSERT_EQ_INT((char)0xEF, out[0]);
    ASSERT_EQ_INT((char)0xBF, out[1]);
    ASSERT_EQ_INT((char)0xBD, out[2]);
}

// ============================================================================
// wlx_utf8_strlen
// ============================================================================

TEST(utf8_strlen_empty) {
    ASSERT_EQ_INT(0, wlx_utf8_strlen(""));
}

TEST(utf8_strlen_ascii) {
    ASSERT_EQ_INT(5, wlx_utf8_strlen("Hello"));
}

TEST(utf8_strlen_mixed) {
    // "Héllo" — é is 2 bytes, total 6 bytes but 5 codepoints
    ASSERT_EQ_INT(5, wlx_utf8_strlen("H\xC3\xA9llo"));
}

TEST(utf8_strlen_latvian) {
    // "āčēģ" — each 2 bytes, 8 bytes total, 4 codepoints
    ASSERT_EQ_INT(4, wlx_utf8_strlen("\xC4\x81\xC4\x8D\xC4\x93\xC4\xA3"));
}

TEST(utf8_strlen_3byte) {
    // "A€B" — € is 3 bytes, total 5 bytes, 3 codepoints
    ASSERT_EQ_INT(3, wlx_utf8_strlen("A\xE2\x82\xAC" "B"));
}

TEST(utf8_strlen_4byte) {
    // "A𐐀B" — 𐐀 is 4 bytes, total 6 bytes, 3 codepoints
    ASSERT_EQ_INT(3, wlx_utf8_strlen("A\xF0\x90\x90\x80\x42"));
}

// ============================================================================
// wlx_utf8_prev
// ============================================================================

TEST(utf8_prev_ascii) {
    const char *s = "AB";
    ASSERT_EQ_INT(1, wlx_utf8_prev(s, 2));
    ASSERT_EQ_INT(0, wlx_utf8_prev(s, 1));
    ASSERT_EQ_INT(0, wlx_utf8_prev(s, 0));
}

TEST(utf8_prev_2byte) {
    // "Aö" = 'A' (1 byte) + ö (2 bytes) = 3 bytes total
    const char *s = "A\xC3\xB6";
    ASSERT_EQ_INT(1, wlx_utf8_prev(s, 3)); // end → before ö
    ASSERT_EQ_INT(0, wlx_utf8_prev(s, 1)); // before ö → before A
}

TEST(utf8_prev_3byte) {
    // "A€" = 'A' (1 byte) + € (3 bytes) = 4 bytes total
    const char *s = "A\xE2\x82\xAC";
    ASSERT_EQ_INT(1, wlx_utf8_prev(s, 4)); // end → before €
    ASSERT_EQ_INT(0, wlx_utf8_prev(s, 1)); // before € → before A
}

TEST(utf8_prev_4byte) {
    // "A𐐀" = 'A' (1 byte) + 𐐀 (4 bytes) = 5 bytes total
    const char *s = "A\xF0\x90\x90\x80";
    ASSERT_EQ_INT(1, wlx_utf8_prev(s, 5)); // end → before 𐐀
    ASSERT_EQ_INT(0, wlx_utf8_prev(s, 1)); // before 𐐀 → before A
}

// ============================================================================
// wlx_utf8_next
// ============================================================================

TEST(utf8_next_ascii) {
    const char *s = "AB";
    size_t len = 2;
    ASSERT_EQ_INT(1, wlx_utf8_next(s, 0, len));
    ASSERT_EQ_INT(2, wlx_utf8_next(s, 1, len));
    ASSERT_EQ_INT(2, wlx_utf8_next(s, 2, len)); // at end, stays
}

TEST(utf8_next_2byte) {
    // "öB" = ö (2 bytes) + 'B' (1 byte) = 3 bytes total
    const char *s = "\xC3\xB6\x42";
    size_t len = 3;
    ASSERT_EQ_INT(2, wlx_utf8_next(s, 0, len)); // skip ö
    ASSERT_EQ_INT(3, wlx_utf8_next(s, 2, len)); // skip B
}

TEST(utf8_next_3byte) {
    // "€B" = € (3 bytes) + 'B' (1 byte) = 4 bytes total
    const char *s = "\xE2\x82\xAC\x42";
    size_t len = 4;
    ASSERT_EQ_INT(3, wlx_utf8_next(s, 0, len)); // skip €
    ASSERT_EQ_INT(4, wlx_utf8_next(s, 3, len)); // skip B
}

TEST(utf8_next_4byte) {
    // "𐐀B" = 𐐀 (4 bytes) + 'B' (1 byte) = 5 bytes total
    const char *s = "\xF0\x90\x90\x80\x42";
    size_t len = 5;
    ASSERT_EQ_INT(4, wlx_utf8_next(s, 0, len)); // skip 𐐀
    ASSERT_EQ_INT(5, wlx_utf8_next(s, 4, len)); // skip B
}

// ============================================================================
// Round-trip: encode then decode
// ============================================================================

TEST(utf8_roundtrip) {
    uint32_t test_cps[] = { 0x41, 0xF6, 0x20AC, 0x10400, 0x0101, 0x010D };
    size_t count = wlx_array_len(test_cps);
    for (size_t i = 0; i < count; i++) {
        char buf[4] = {0};
        size_t enc_n = wlx_utf8_encode(test_cps[i], buf);
        ASSERT_TRUE(enc_n >= 1 && enc_n <= 4);

        uint32_t decoded = 0;
        size_t dec_n = wlx_utf8_decode(buf, &decoded);
        ASSERT_EQ_INT(enc_n, dec_n);
        ASSERT_EQ_INT(test_cps[i], decoded);
    }
}

// ============================================================================
// Suite
// ============================================================================

SUITE(utf8) {
    // char_len
    RUN_TEST(utf8_char_len_ascii);
    RUN_TEST(utf8_char_len_2byte);
    RUN_TEST(utf8_char_len_3byte);
    RUN_TEST(utf8_char_len_4byte);
    RUN_TEST(utf8_char_len_invalid);

    // decode
    RUN_TEST(utf8_decode_ascii);
    RUN_TEST(utf8_decode_2byte);
    RUN_TEST(utf8_decode_3byte);
    RUN_TEST(utf8_decode_4byte);
    RUN_TEST(utf8_decode_invalid_lead);
    RUN_TEST(utf8_decode_truncated_2byte);
    RUN_TEST(utf8_decode_truncated_3byte);

    // encode
    RUN_TEST(utf8_encode_ascii);
    RUN_TEST(utf8_encode_2byte);
    RUN_TEST(utf8_encode_3byte);
    RUN_TEST(utf8_encode_4byte);
    RUN_TEST(utf8_encode_invalid);

    // strlen
    RUN_TEST(utf8_strlen_empty);
    RUN_TEST(utf8_strlen_ascii);
    RUN_TEST(utf8_strlen_mixed);
    RUN_TEST(utf8_strlen_latvian);
    RUN_TEST(utf8_strlen_3byte);
    RUN_TEST(utf8_strlen_4byte);

    // prev
    RUN_TEST(utf8_prev_ascii);
    RUN_TEST(utf8_prev_2byte);
    RUN_TEST(utf8_prev_3byte);
    RUN_TEST(utf8_prev_4byte);

    // next
    RUN_TEST(utf8_next_ascii);
    RUN_TEST(utf8_next_2byte);
    RUN_TEST(utf8_next_3byte);
    RUN_TEST(utf8_next_4byte);

    // round-trip
    RUN_TEST(utf8_roundtrip);
}
