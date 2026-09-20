// dashboard_syntax.h - per-span colours for the dashboard editor's C
// sample document: a small illustrative C tokenizer behind the editor's
// span-colour hook (.span_color). Included only by dashboard.c.
//
// The hook asks for one span at a time and walks each visible record from
// its start. This tokenizer keeps no state between calls: it re-tokenizes
// the hard line from its start until it reaches the token holding the
// offset (lines are short, so the rescan is cheap) and answers that
// token's remaining bytes and class. Strings, character literals and
// comments on one line are tracked that way; block comments spanning
// lines are not (the sample has none). Illustrative, not a language
// service: a demo of the app-side tokenizer pattern, with the state the
// application owns kept to nothing at all.

#ifndef DASHBOARD_SYNTAX_H_
#define DASHBOARD_SYNTAX_H_

typedef enum {
    DASH_SYN_PLAIN = 0,   // identifiers, punctuation, whitespace: front_color
    DASH_SYN_COMMENT,
    DASH_SYN_PREPROC,
    DASH_SYN_STRING,
    DASH_SYN_NUMBER,
    DASH_SYN_KEYWORD,
    DASH_SYN_LIBRARY      // wlx_* / WLX_* identifiers
} Dash_Syntax_Class;

static bool dash_syn_ident_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool dash_syn_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool dash_syn_ident_char(char c) {
    return dash_syn_ident_start(c) || dash_syn_digit(c);
}

// Byte-for-byte match of [s, s + n) against a NUL-terminated word.
static bool dash_syn_word_is(const char *s, size_t n, const char *word) {
    size_t i = 0;
    for (; i < n; i++) {
        if (word[i] == '\0' || word[i] != s[i]) return false;
    }
    return word[i] == '\0';
}

static bool dash_syn_is_keyword(const char *s, size_t n) {
    static const char *const words[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "inline", "int", "long", "register", "restrict", "return", "short",
        "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
        "unsigned", "void", "volatile", "while", "bool", "true", "false",
        "NULL", "size_t", "uint32_t", "uint8_t", "int32_t",
    };
    for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); i++) {
        if (dash_syn_word_is(s, n, words[i])) return true;
    }
    return false;
}

// End of the token starting at `pos` inside the line [line_start,
// line_end), and its class. `pos` is a token start: the scan below only
// ever asks at line_start or at a previous token's end.
static size_t dash_syn_token(const char *t, size_t line_start, size_t line_end,
                             size_t pos, Dash_Syntax_Class *cls)
{
    size_t p = pos;
    char c = t[p];
    *cls = DASH_SYN_PLAIN;

    if (c == '#') {
        // A preprocessor line: '#' as the first non-blank byte.
        size_t q = line_start;
        while (q < pos && (t[q] == ' ' || t[q] == '\t')) q++;
        if (q == pos) { *cls = DASH_SYN_PREPROC; return line_end; }
    }
    if (c == '/' && p + 1 < line_end && t[p + 1] == '/') {
        *cls = DASH_SYN_COMMENT;
        return line_end;
    }
    if (c == '/' && p + 1 < line_end && t[p + 1] == '*') {
        *cls = DASH_SYN_COMMENT;
        p += 2;
        while (p < line_end) {
            if (t[p] == '*' && p + 1 < line_end && t[p + 1] == '/') return p + 2;
            p++;
        }
        return line_end;
    }
    if (c == '"' || c == '\'') {
        *cls = DASH_SYN_STRING;
        p++;
        while (p < line_end && t[p] != c) {
            if (t[p] == '\\' && p + 1 < line_end) p++;
            p++;
        }
        return p < line_end ? p + 1 : line_end;
    }
    if (dash_syn_digit(c)) {
        *cls = DASH_SYN_NUMBER;
        while (p < line_end && (dash_syn_ident_char(t[p]) || t[p] == '.')) p++;
        return p;
    }
    if (dash_syn_ident_start(c)) {
        while (p < line_end && dash_syn_ident_char(t[p])) p++;
        size_t n = p - pos;
        if (n > 4 && (dash_syn_word_is(t + pos, 4, "wlx_") || dash_syn_word_is(t + pos, 4, "WLX_")))
            *cls = DASH_SYN_LIBRARY;
        else if (dash_syn_is_keyword(t + pos, n))
            *cls = DASH_SYN_KEYWORD;
        return p;
    }
    // Plain run: whitespace and punctuation up to the next byte that
    // starts a classified token.
    p++;
    while (p < line_end) {
        char d = t[p];
        if (dash_syn_ident_start(d) || dash_syn_digit(d) || d == '"' || d == '\'') break;
        if (d == '/' && p + 1 < line_end && (t[p + 1] == '/' || t[p + 1] == '*')) break;
        p++;
    }
    return p;
}

// The span-colour hook: the colour of the token holding q->offset and
// where that token ends. user is the dashboard's token bundle.
static WLX_Color dashboard_syntax_span(const WLX_Text_Span_Query *q, size_t *span_end, void *user) {
    const Dashboard_Tokens *tk = (const Dashboard_Tokens *)user;
    WLX_Color plain = {0};

    // The line's bytes end at its newline (line_next is the next line's
    // start, past the separator).
    size_t line_end = q->line_start;
    while (line_end < q->line_next && q->text[line_end] != '\n' && q->text[line_end] != '\r') line_end++;
    if (q->offset >= line_end) {
        *span_end = q->limit;
        return plain;
    }

    Dash_Syntax_Class cls = DASH_SYN_PLAIN;
    size_t pos = q->line_start;
    size_t end = pos;
    for (;;) {
        end = dash_syn_token(q->text, q->line_start, line_end, pos, &cls);
        if (end <= pos) end = pos + 1;
        if (q->offset < end) break;
        pos = end;
    }
    *span_end = end;

    switch (cls) {
    case DASH_SYN_COMMENT: return tk->color.on_surface_muted;
    case DASH_SYN_PREPROC: return tk->color.accent_strong;
    case DASH_SYN_STRING:  return tk->color.tertiary;
    case DASH_SYN_NUMBER:  return tk->status.warning;
    case DASH_SYN_KEYWORD: return tk->color.accent;
    case DASH_SYN_LIBRARY: return tk->status.info;
    case DASH_SYN_PLAIN:   break;
    }
    return plain;
}

#endif // DASHBOARD_SYNTAX_H_
