// test_fuzz_text.c - multi-key op-sequence fuzz over the text widgets: the
// single-line inputbox (plain and password), the textarea and the editor
// (each with wrap off and on).
//
// An op is one or more frames: typed text (ASCII, 2-4 byte codepoints,
// combining-mark clusters, a ZWJ emoji, regional-indicator pairs);
// Backspace and Delete, plain and with Ctrl; Left and Right with Shift,
// Ctrl and both; Up, Down, PageUp, PageDown; Home and End with Shift and
// Ctrl; Enter; Tab; select all; copy, cut and paste; the undo and redo
// chords; a press, a drag or a Shift-click at a random point; the wheel;
// and a composition that updates, then commits or cancels, within the op.
// About one frame in four carries a second key, or text beside a key, and
// some rounds run a 48-byte buffer so the truncation paths run.
//
// The oracle checks states, not a reference model (which would be a second
// editor with the first one's mistakes). Every frame: the length is within
// capacity, and the inputbox's buffer keeps its NUL; the document is valid
// UTF-8 (every input is); the caret pair is inside the document and on
// text-unit boundaries; the editor's line index equals a fresh scan, and
// after the first frame it is patched, never rebuilt (every change is the
// widget's own edit, so a rebuild means a frame's edit span missed one).
// Between ops no composition is held. At the end of a round, undo runs to
// the history's base and redo to its tip, bytes pinned at both: the base
// is the document whenever the undo stack was last empty, the tip the
// document whenever the redo stack was last empty.
//
// Fixed seeds gate, with a coverage self-check that every op class (and a
// second key, text beside a key, and the small buffer) is drawn in every
// mode across the table, so the generator cannot narrow silently. A
// regression table follows: seeds a failing round printed, run as printed.
// Then one time-seeded round per mode; it only adds coverage and prints
// its seed when it fails, so the seed can join the regression table (the
// test_fuzz.c pattern).
//
// Included from test_main.c (single TU build) after test_ime.c; uses
// tue_index_consistent (test_text_undo.c) and ev_index (test_editor_view.c).

// ============================================================================
// Modes, op classes, the generator
// ============================================================================

typedef enum {
    FT_INPUTBOX, FT_PASSWORD, FT_TEXTAREA, FT_TEXTAREA_WRAP, FT_EDITOR, FT_EDITOR_WRAP,
    FT_MODE_COUNT
} FT_Mode;

static const char *const ft_mode_names[FT_MODE_COUNT] = {
    "inputbox", "inputbox (password)", "textarea", "textarea (wrap)", "editor", "editor (wrap)"
};

typedef enum {
    FT_OP_TYPE, FT_OP_BACKSPACE, FT_OP_DELETE, FT_OP_WORD_DELETE, FT_OP_HORZ, FT_OP_VERT,
    FT_OP_HOME_END, FT_OP_ENTER, FT_OP_TAB, FT_OP_SELECT_ALL, FT_OP_COPY, FT_OP_CUT,
    FT_OP_PASTE, FT_OP_UNDO, FT_OP_REDO, FT_OP_PRESS, FT_OP_DRAG, FT_OP_SHIFT_CLICK,
    FT_OP_WHEEL, FT_OP_COMPOSE_COMMIT, FT_OP_COMPOSE_CANCEL,
    FT_OP_COUNT,
    // Coverage-only classes: frame shapes and round shapes, then effects the
    // ops must reach (a full buffer, a held composition, an undo replay).
    FT_COV_SECOND_KEY = FT_OP_COUNT, FT_COV_TEXT_BESIDE_KEY, FT_COV_SMALL_BUFFER,
    FT_COV_FULL_BUFFER, FT_COV_COMPOSING, FT_COV_REPLAY,
    FT_COV_COUNT
} FT_Class;

static const char *const ft_class_names[FT_COV_COUNT] = {
    "type", "backspace", "delete", "word delete", "left/right", "up/down/page",
    "home/end", "enter", "tab", "select all", "copy", "cut", "paste", "undo", "redo",
    "press", "drag", "shift-click", "wheel", "compose+commit", "compose+cancel",
    "second key", "text beside a key", "48-byte buffer",
    "a buffer within 4 bytes of full", "a held composition", "an undo replay",
};

// Draw weights, in FT_Class order.
static const int ft_weights[FT_OP_COUNT] = {
    10, 3, 2, 2, 4, 3, 2, 2, 1, 1, 1, 1, 2, 2, 1, 2, 1, 1, 1, 1, 1,
};

static uint32_t ft_next(uint32_t *s) {
    *s = *s * 1664525u + 1013904223u;
    return *s >> 8;
}

static FT_Class ft_draw_class(uint32_t *s) {
    int total = 0;
    for (int i = 0; i < FT_OP_COUNT; i++) total += ft_weights[i];
    int r = (int)(ft_next(s) % (uint32_t)total);
    for (int i = 0; i < FT_OP_COUNT; i++) {
        if (r < ft_weights[i]) return (FT_Class)i;
        r -= ft_weights[i];
    }
    return FT_OP_TYPE;
}

static const char *const ft_typed[] = {
    "a", "b", "z", " ", "word", "x y",
    "\xC3\xA9",                                     // e acute, 2 bytes
    "\xE2\x82\xAC",                                 // euro sign, 3 bytes
    "\xF0\x9F\x98\x80",                             // grinning face, 4 bytes
    "e\xCC\x81",                                    // e + combining acute
    "a\xCC\x88\xCC\x81",                            // a + two marks
    ("\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9"  // man ZWJ woman ZWJ girl
     "\xE2\x80\x8D\xF0\x9F\x91\xA7"),
    "\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8",             // regional indicators U S
    "\xF0\x9F\x87\xAB\xF0\x9F\x87\xB7",             // regional indicators F R
};
#define FT_TYPED_COUNT (sizeof(ft_typed) / sizeof(ft_typed[0]))

static const char *const ft_clips[] = {
    "P", "pq\nr", "\xC3\xA9\xE2\x82\xAC", "line one\nline two\n",
    ("\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8" "e\xCC\x81"), "12345678901234567890",
};
#define FT_CLIPS_COUNT (sizeof(ft_clips) / sizeof(ft_clips[0]))

// Composition strings: a run of updates, then the committed text.
static const char *const ft_preedits[] = {
    "k", "\xE3\x81\x8B", ("\xE3\x81\x8B" "n"), "\xE3\x81\x8B\xE3\x81\xAA",
};
static const char *const ft_commits[] = {
    "\xE3\x81\x8B\xE3\x81\xAA", "\xE4\xBB\xAE\xE5\x90\x8D", "x",
};

// ============================================================================
// Fixture: one widget at a stable call site, the mode a fixture global
// ============================================================================

#define FT_ROOT_W 400
#define FT_ROOT_H 120

static FT_Mode ft_mode = FT_INPUTBOX;
static bool    ft_changed = false;   // the widget's "text changed" return, last frame
static bool    ft_focused = false;

static bool ft_is_inputbox(FT_Mode m) { return m <= FT_TEXTAREA_WRAP; }

static void ft_widget(WLX_Context *ctx, char *buf, size_t cap, size_t *len) {
    if (ft_is_inputbox(ft_mode)) {
        ft_changed = wlx_inputbox_impl(ctx, NULL, buf, cap,
            wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10, .border_width = 0,
                .multiline = ft_mode == FT_TEXTAREA || ft_mode == FT_TEXTAREA_WRAP,
                .wrap = ft_mode == FT_TEXTAREA_WRAP, .password = ft_mode == FT_PASSWORD,
                .out_focused = &ft_focused),
            __FILE__, __LINE__);
        *len = strlen(buf);
    } else {
        ft_changed = wlx_editor_impl(ctx, NULL, buf, cap, len,
            wlx_default_editor_opt(.content_padding = 4, .font_size = 10, .border_width = 0,
                .wrap = ft_mode == FT_EDITOR_WRAP, .out_focused = &ft_focused),
            __FILE__, __LINE__);
    }
}

typedef struct {
    int mx, my;
    bool down, clicked;
    float wheel;
    WLX_Key_Code key1, key2;
    uint32_t mods;
    const char *text;
    const char *preedit;
    int32_t preedit_cursor;
} FT_Frame;

static void ft_copy_field(char *dst, size_t cap, const char *src) {
    size_t n = src != NULL ? strlen(src) : 0;
    if (n >= cap) n = cap - 1;
    if (n > 0) memcpy(dst, src, n);
    dst[n] = '\0';
}

static void ft_run_frame(WLX_Context *ctx, char *buf, size_t cap, size_t *len, const FT_Frame *f) {
    WLX_Input_State in;
    memset(&in, 0, sizeof(in));
    in.mouse_x = f->mx;
    in.mouse_y = f->my;
    in.mouse_down = f->down;
    in.mouse_clicked = f->clicked;
    in.mouse_held = f->down;
    in.wheel_delta = f->wheel;
    in.modifiers = f->mods;
    if (f->key1 != WLX_KEY_NONE) { in.keys_pressed[f->key1] = true; in.keys_down[f->key1] = true; }
    if (f->key2 != WLX_KEY_NONE) { in.keys_pressed[f->key2] = true; in.keys_down[f->key2] = true; }
    ft_copy_field(in.text_input, sizeof(in.text_input), f->text);
    ft_copy_field(in.preedit, sizeof(in.preedit), f->preedit);
    in.preedit_cursor = f->preedit != NULL ? f->preedit_cursor : 0;
    in.preedit_sel_len = f->preedit != NULL ? -1 : 0;
    test_frame_begin_input(ctx, &in);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    ft_widget(ctx, buf, cap, len);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// The widget's caret state: the single state entry of the widget's size.
static WLX_Text_Edit_State *ft_caret(WLX_Context *ctx) {
    size_t want = ft_is_inputbox(ft_mode) ? sizeof(WLX_Inputbox_State) : sizeof(WLX_Editor_State);
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        WLX_State_Map_Slot *slot = &ctx->states.slots[i];
        if (slot->id == 0 || slot->data_size != want) continue;
        return ft_is_inputbox(ft_mode) ? &((WLX_Inputbox_State *)slot->data)->caret
                                       : &((WLX_Editor_State *)slot->data)->caret;
    }
    return NULL;
}

static WLX_Text_Undo_Journal *ft_journal(WLX_Context *ctx) {
    return ctx->text_undo.count > 0 ? &ctx->text_undo.items[0] : NULL;
}

// Strict UTF-8: no overlong forms, no surrogates, nothing past U+10FFFF.
static bool ft_utf8_valid(const char *s, size_t n) {
    const unsigned char *b = (const unsigned char *)s;
    size_t i = 0;
    while (i < n) {
        unsigned char c = b[i];
        size_t k;
        uint32_t cp, min;
        if (c < 0x80u) { i++; continue; }
        else if ((c & 0xE0u) == 0xC0u) { k = 1; cp = c & 0x1Fu; min = 0x80u; }
        else if ((c & 0xF0u) == 0xE0u) { k = 2; cp = c & 0x0Fu; min = 0x800u; }
        else if ((c & 0xF8u) == 0xF0u) { k = 3; cp = c & 0x07u; min = 0x10000u; }
        else return false;
        for (size_t j = 1; j <= k; j++) {
            if (i + j >= n || (b[i + j] & 0xC0u) != 0x80u) return false;
            cp = (cp << 6) | (b[i + j] & 0x3Fu);
        }
        if (cp < min || cp > 0x10FFFFu || (cp >= 0xD800u && cp <= 0xDFFFu)) return false;
        i += k + 1;
    }
    return true;
}

// ============================================================================
// One round
// ============================================================================

// Where the round is, for the failure report.
static int      ft_cur_op = -1;
static FT_Class ft_cur_class = FT_OP_TYPE;
static bool     ft_cur_small = false;

// The editor's index rebuild count once the first frame has built the
// index (UINT32_MAX before). Every later change in a round is the widget's
// own edit, which patches the index, so the count holds; a rebuild means a
// frame's edit span did not account for its edits, and the undo history
// went with the index.
static uint32_t ft_rebuilds = UINT32_MAX;

// The per-frame oracle.
#define FT_CHECK_FRAME(ctx, buf, cap, len) do {                                    \
    if (ft_is_inputbox(ft_mode)) ASSERT_TRUE(strlen(buf) < (cap));                 \
    ASSERT_TRUE((len) <= (cap));                                                   \
    ASSERT_TRUE(ft_utf8_valid((buf), (len)));                                      \
    WLX_Text_Edit_State *ft_c_ = ft_caret(ctx);                                    \
    ASSERT_TRUE(ft_c_ != NULL);                                                    \
    ASSERT_TRUE(ft_c_->cursor_pos <= (len) && ft_c_->selection_anchor <= (len));   \
    ASSERT_TRUE(wlx_text_unit_boundary((buf), (len), ft_c_->cursor_pos));          \
    ASSERT_TRUE(wlx_text_unit_boundary((buf), (len), ft_c_->selection_anchor));    \
    if (!ft_is_inputbox(ft_mode)) ASSERT_TRUE(tue_index_consistent((ctx), (buf), (len))); \
    if (!ft_is_inputbox(ft_mode) && ft_rebuilds != UINT32_MAX)                     \
        ASSERT_EQ_INT((long)ft_rebuilds, (long)ev_index(ctx)->rebuilds);           \
} while (0)

// A frame, then the oracle, then the effects it reached.
#define FT_FRAME(f) do {                                                           \
    ft_run_frame(&ctx, buf, cap, &len, &(f));                                      \
    changed |= ft_changed;                                                         \
    FT_CHECK_FRAME(&ctx, buf, cap, len);                                           \
    if (cap - len <= 4) *drawn |= 1u << FT_COV_FULL_BUFFER;                        \
    if (wlx_text_composing(&ctx)) *drawn |= 1u << FT_COV_COMPOSING;                \
} while (0)

static void ft_point(uint32_t *s, int *x, int *y) {
    *x = 4 + (int)(ft_next(s) % (FT_ROOT_W - 8));
    *y = 4 + (int)(ft_next(s) % (FT_ROOT_H - 8));
}

// Runs one round and records the classes it drew in *drawn. Fails through
// the ASSERT macros, so the caller reads _test_current_failed.
static void ft_round(FT_Mode mode, uint32_t seed, int ops, uint32_t *drawn) {
    uint32_t s = seed;
    ft_mode = mode;
    ft_cur_op = -1;
    ft_rebuilds = UINT32_MAX;
    bool small = ft_next(&s) % 2 == 0;
    ft_cur_small = small;
    if (small) *drawn |= 1u << FT_COV_SMALL_BUFFER;
    size_t cap = small ? 48 : 1024;
    bool multiline = mode != FT_INPUTBOX && mode != FT_PASSWORD;
    bool journaled = mode != FT_PASSWORD;
    uint32_t cmd = test_command_mod();

    WLX_Context ctx;
    test_ctx_init(&ctx, FT_ROOT_W, FT_ROOT_H);
    char *buf = (char *)calloc(1, cap + 1);
    char *base = (char *)calloc(1, cap + 1);
    char *tip = (char *)calloc(1, cap + 1);
    ASSERT_TRUE(buf != NULL && base != NULL && tip != NULL);
    const char *seed_text = multiline ? "seed \xC3\xA9 text\nline \xF0\x9F\x91\x8D\n"
                                      : "seed \xC3\xA9 \xF0\x9F\x91\x8D";
    size_t len = strlen(seed_text);
    memcpy(buf, seed_text, len);
    memcpy(base, buf, len);
    size_t base_len = len;
    memcpy(tip, buf, len);
    size_t tip_len = len;
    bool changed = false;

    int px = FT_ROOT_W / 2, py = FT_ROOT_H / 2;   // the pointer at rest
    FT_Frame f;
    memset(&f, 0, sizeof(f));
    f.mx = px; f.my = py; f.down = true; f.clicked = true;
    FT_FRAME(f);                                   // focus
    if (!ft_is_inputbox(mode) && ev_index(&ctx) != NULL) ft_rebuilds = ev_index(&ctx)->rebuilds;
    memset(&f, 0, sizeof(f));
    f.mx = px; f.my = py;
    FT_FRAME(f);                                   // release

    for (int op = 0; op < ops; op++) {
        ft_cur_op = op;
        if (!ft_focused) {                         // Tab or Enter may have moved focus
            memset(&f, 0, sizeof(f));
            f.mx = px; f.my = py; f.down = true; f.clicked = true;
            FT_FRAME(f);
            memset(&f, 0, sizeof(f));
            f.mx = px; f.my = py;
            FT_FRAME(f);
        }
        FT_Class cls = ft_draw_class(&s);
        ft_cur_class = cls;
        *drawn |= 1u << cls;
        memset(&f, 0, sizeof(f));
        f.mx = px; f.my = py;
        bool keyed = true;   // a one-frame op that may take a second key
        switch (cls) {
        case FT_OP_TYPE:
            f.text = ft_typed[ft_next(&s) % FT_TYPED_COUNT];
            break;
        case FT_OP_BACKSPACE: f.key1 = WLX_KEY_BACKSPACE; break;
        case FT_OP_DELETE:    f.key1 = WLX_KEY_DELETE; break;
        case FT_OP_WORD_DELETE:
            f.key1 = ft_next(&s) % 2 ? WLX_KEY_BACKSPACE : WLX_KEY_DELETE;
            f.mods = WLX_MOD_CTRL;
            break;
        case FT_OP_HORZ: {
            static const uint32_t m[] = { 0, WLX_MOD_SHIFT, WLX_MOD_CTRL, WLX_MOD_SHIFT | WLX_MOD_CTRL };
            f.key1 = ft_next(&s) % 2 ? WLX_KEY_LEFT : WLX_KEY_RIGHT;
            f.mods = m[ft_next(&s) % 4];
            break;
        }
        case FT_OP_VERT: {
            static const WLX_Key_Code k[] = { WLX_KEY_UP, WLX_KEY_DOWN, WLX_KEY_PAGE_UP, WLX_KEY_PAGE_DOWN };
            f.key1 = k[ft_next(&s) % 4];
            f.mods = ft_next(&s) % 2 ? WLX_MOD_SHIFT : 0;
            break;
        }
        case FT_OP_HOME_END: {
            static const uint32_t m[] = { 0, WLX_MOD_SHIFT, WLX_MOD_CTRL, WLX_MOD_SHIFT | WLX_MOD_CTRL };
            f.key1 = ft_next(&s) % 2 ? WLX_KEY_HOME : WLX_KEY_END;
            f.mods = m[ft_next(&s) % 4];
            break;
        }
        case FT_OP_ENTER:      f.key1 = WLX_KEY_ENTER; break;
        case FT_OP_TAB:        f.key1 = WLX_KEY_TAB; break;
        case FT_OP_SELECT_ALL: f.key1 = WLX_KEY_A; f.mods = cmd; break;
        case FT_OP_COPY:       f.key1 = WLX_KEY_C; f.mods = cmd; break;
        case FT_OP_CUT:        f.key1 = WLX_KEY_X; f.mods = cmd; break;
        case FT_OP_PASTE:
            test_set_clipboard(ft_clips[ft_next(&s) % FT_CLIPS_COUNT]);
            f.key1 = WLX_KEY_V; f.mods = cmd;
            break;
        case FT_OP_UNDO: f.key1 = WLX_KEY_Z; f.mods = cmd; break;
        case FT_OP_REDO:
            if (ft_next(&s) % 2) { f.key1 = WLX_KEY_Z; f.mods = cmd | WLX_MOD_SHIFT; }
            else { f.key1 = WLX_KEY_Y; f.mods = cmd; }
            break;
        case FT_OP_PRESS:
        case FT_OP_SHIFT_CLICK:
            keyed = false;
            ft_point(&s, &f.mx, &f.my);
            f.down = true; f.clicked = true;
            if (cls == FT_OP_SHIFT_CLICK) f.mods = WLX_MOD_SHIFT;
            FT_FRAME(f);
            f.down = false; f.clicked = false;
            FT_FRAME(f);
            break;
        case FT_OP_DRAG:
            keyed = false;
            ft_point(&s, &f.mx, &f.my);
            f.down = true; f.clicked = true;
            FT_FRAME(f);
            ft_point(&s, &f.mx, &f.my);
            f.clicked = false;
            FT_FRAME(f);                           // held, moved
            f.down = false;
            FT_FRAME(f);                           // released
            break;
        case FT_OP_WHEEL:
            keyed = false;
            f.wheel = (float)((int)(ft_next(&s) % 7) - 3);
            FT_FRAME(f);
            break;
        case FT_OP_COMPOSE_COMMIT:
        case FT_OP_COMPOSE_CANCEL: {
            keyed = false;
            int updates = 1 + (int)(ft_next(&s) % 3);
            for (int u = 0; u < updates; u++) {
                memset(&f, 0, sizeof(f));
                f.mx = px; f.my = py;
                f.preedit = ft_preedits[ft_next(&s) % 4];
                f.preedit_cursor = (int32_t)(ft_next(&s) % 3);
                FT_FRAME(f);
            }
            memset(&f, 0, sizeof(f));
            f.mx = px; f.my = py;
            if (cls == FT_OP_COMPOSE_COMMIT) f.text = ft_commits[ft_next(&s) % 3];
            FT_FRAME(f);
            break;
        }
        default:
            break;
        }
        if (keyed) {
            if (ft_next(&s) % 4 == 0) {
                static const WLX_Key_Code seconds[] = {
                    WLX_KEY_BACKSPACE, WLX_KEY_DELETE, WLX_KEY_LEFT, WLX_KEY_RIGHT, WLX_KEY_UP,
                    WLX_KEY_DOWN, WLX_KEY_HOME, WLX_KEY_END, WLX_KEY_ENTER, WLX_KEY_V,
                    WLX_KEY_X, WLX_KEY_Z, WLX_KEY_Y, WLX_KEY_A,
                };
                uint32_t r = ft_next(&s) % 16;
                if (r < 14) {
                    f.key2 = seconds[r];
                    if (f.key1 == WLX_KEY_NONE) { f.key1 = f.key2; f.key2 = WLX_KEY_NONE; }
                    else *drawn |= 1u << FT_COV_SECOND_KEY;
                } else if (f.text == NULL) {
                    f.text = ft_typed[ft_next(&s) % FT_TYPED_COUNT];
                    *drawn |= 1u << FT_COV_TEXT_BESIDE_KEY;
                } else {
                    f.key1 = WLX_KEY_BACKSPACE;
                    *drawn |= 1u << FT_COV_TEXT_BESIDE_KEY;
                }
            }
            FT_FRAME(f);
        }
        // Between ops: no composition is held.
        ASSERT_FALSE(wlx_text_composing(&ctx));
        ASSERT_EQ_INT(0, (long)ft_caret(&ctx)->preedit_len);
        if (journaled) {
            WLX_Text_Undo_Journal *j = ft_journal(&ctx);
            if (j == NULL || j->undo.count == 0) { memcpy(base, buf, len); base_len = len; }
            if (j == NULL || j->redo.count == 0) { memcpy(tip, buf, len); tip_len = len; }
        }
    }
    ft_cur_op = ops;

    if (journaled) {
        // Undo to the base, then redo to the tip.
        if (!ft_focused) {
            memset(&f, 0, sizeof(f));
            f.mx = px; f.my = py; f.down = true; f.clicked = true;
            FT_FRAME(f);
            memset(&f, 0, sizeof(f));
            f.mx = px; f.my = py;
            FT_FRAME(f);
        }
        int guard = 0;
        do {
            memset(&f, 0, sizeof(f));
            f.mx = px; f.my = py; f.key1 = WLX_KEY_Z; f.mods = cmd;
            changed = false;
            FT_FRAME(f);
        } while (changed && ++guard < 10000);
        if (guard > 0) *drawn |= 1u << FT_COV_REPLAY;
        WLX_Text_Undo_Journal *j = ft_journal(&ctx);
        ASSERT_TRUE(j == NULL || j->undo.count == 0);
        ASSERT_EQ_INT((long)base_len, (long)len);
        ASSERT_TRUE(memcmp(buf, base, len) == 0);
        guard = 0;
        do {
            memset(&f, 0, sizeof(f));
            f.mx = px; f.my = py; f.key1 = WLX_KEY_Z; f.mods = cmd | WLX_MOD_SHIFT;
            changed = false;
            FT_FRAME(f);
        } while (changed && ++guard < 10000);
        ASSERT_EQ_INT((long)tip_len, (long)len);
        ASSERT_TRUE(memcmp(buf, tip, len) == 0);
    }

    wlx_context_destroy(&ctx);
    free(buf);
    free(base);
    free(tip);
}

#undef FT_FRAME
#undef FT_CHECK_FRAME

// ============================================================================
// Seeds and the coverage self-check
// ============================================================================

// Fixed seeds gate every run. Each mode runs every seed, mixed with the
// mode so the modes see different streams.
static const uint32_t ft_fixed_seeds[] = {
    0x7e570111u, 0x7e570222u, 0x7e570333u, 0x7e570555u,
};
#define FT_FIXED_SEED_COUNT (sizeof(ft_fixed_seeds) / sizeof(ft_fixed_seeds[0]))

// Seeds a time-seeded round printed when it failed, run as printed (not
// mixed with the mode) in every mode.
//   0x7d478ff0  editor: a frame's edits, a forward delete past the
//               running end among them, merged into an edit span with a
//               wrapped end; the index rebuilt and the undo history was
//               dropped. Printed by CI at 120 ops; at 300 it still
//               reaches the defect, in the round trip's redo.
static const uint32_t ft_regression_seeds[] = {
    0x7d478ff0u,
};
#define FT_REGRESSION_SEED_COUNT (sizeof(ft_regression_seeds) / sizeof(ft_regression_seeds[0]))
#define FT_OPS_PER_ROUND 300

static uint32_t ft_mode_seed(uint32_t seed, FT_Mode mode) {
    return seed ^ ((uint32_t)mode * 0x9E3779B9u);
}

// One round, named on failure.
static bool ft_round_named(FT_Mode mode, uint32_t seed, uint32_t *drawn, const char *kind) {
    int failed_before = _test_current_failed;
    _test_current_failed = 0;
    ft_round(mode, seed, FT_OPS_PER_ROUND, drawn);
    bool failed = _test_current_failed != 0;
    if (failed) {
        fprintf(stderr, "    (%s round: mode %s, seed 0x%08x, %d ops, op %d (%s)%s)\n",
                kind, ft_mode_names[mode], (unsigned)seed, FT_OPS_PER_ROUND, ft_cur_op,
                ft_cur_op >= 0 && ft_cur_op < FT_OPS_PER_ROUND ? ft_class_names[ft_cur_class]
                                                                 : "setup or round trip",
                ft_cur_small ? ", 48-byte buffer" : "");
    }
    _test_current_failed |= failed_before;
    return !failed;
}

static void ft_fuzz_mode(FT_Mode mode) {
    uint32_t drawn = 0;
    for (size_t i = 0; i < FT_FIXED_SEED_COUNT; i++) {
        if (!ft_round_named(mode, ft_mode_seed(ft_fixed_seeds[i], mode), &drawn, "fixed")) return;
    }
    // The coverage self-check: every class drawn in this mode across the
    // fixed table.
    if (mode == FT_PASSWORD) drawn |= 1u << FT_COV_REPLAY;   // no journal
    for (int c = 0; c < FT_COV_COUNT; c++) {
        if (drawn & (1u << c)) continue;
        fprintf(stderr, "    (mode %s: the fixed seeds never draw \"%s\")\n",
                ft_mode_names[mode], ft_class_names[c]);
        ASSERT_TRUE(drawn & (1u << c));
    }
    for (size_t i = 0; i < FT_REGRESSION_SEED_COUNT; i++) {
        if (!ft_round_named(mode, ft_regression_seeds[i], &drawn, "regression")) return;
    }
    uint32_t timed = (uint32_t)time(NULL) ^ ((uint32_t)mode * 0x85EBCA6Bu);
    (void)ft_round_named(mode, timed, &drawn, "time-seeded");
}

TEST(fuzz_text_inputbox)          { ft_fuzz_mode(FT_INPUTBOX); }
TEST(fuzz_text_inputbox_password) { ft_fuzz_mode(FT_PASSWORD); }
TEST(fuzz_text_textarea)          { ft_fuzz_mode(FT_TEXTAREA); }
TEST(fuzz_text_textarea_wrap)     { ft_fuzz_mode(FT_TEXTAREA_WRAP); }
TEST(fuzz_text_editor)            { ft_fuzz_mode(FT_EDITOR); }
TEST(fuzz_text_editor_wrap)       { ft_fuzz_mode(FT_EDITOR_WRAP); }

SUITE(fuzz_text) {
    RUN_TEST(fuzz_text_inputbox);
    RUN_TEST(fuzz_text_inputbox_password);
    RUN_TEST(fuzz_text_textarea);
    RUN_TEST(fuzz_text_textarea_wrap);
    RUN_TEST(fuzz_text_editor);
    RUN_TEST(fuzz_text_editor_wrap);
}
