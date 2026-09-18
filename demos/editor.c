// editor.c - wlx_editor demo: a windowed text editor over a caller-owned
// flat buffer. Generates synthetic documents (press 1 = ~100 KB, 2 = ~10 MB /
// 1,000,000 lines, 3 = one multi-megabyte hard line) or loads a file passed
// as argv[1]; W toggles wrapped mode. Frame cost stays O(viewport) whatever
// the document size.

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
#include "wollix_editor.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 700
#define TARGET_FPS 60

// Document buffer: caller-owned, explicit length in/out. Headroom above the
// generated size leaves room for edits.
#define DOC_HEADROOM (1u << 20)

static char *doc_buf = NULL;
static size_t doc_cap = 0;
static size_t doc_len = 0;
static uint32_t doc_revision = 0;
static bool wrap_mode = false;

// Generate target_lines synthetic lines (~11 bytes each). Every 1000th line
// gets a long tail so horizontal scrolling has something to reach.
static void generate_document(size_t target_lines) {
    size_t approx = target_lines * 12 + (target_lines / 1000 + 1) * 512;
    size_t cap = approx + DOC_HEADROOM;
    char *buf = (char *)realloc(doc_buf, cap);
    if (buf == NULL) return;
    doc_buf = buf;
    doc_cap = cap;

    size_t off = 0;
    for (size_t i = 0; i < target_lines; i++) {
        off += (size_t)snprintf(buf + off, cap - off, "%06zu abc", i + 1);
        if (i % 1000 == 999) {
            size_t tail = 500;
            if (off + tail + 1 < cap) {
                memset(buf + off, '-', tail);
                off += tail;
            }
        }
        buf[off++] = '\n';
    }
    buf[off] = '\0';
    doc_len = off;
    doc_revision++;
    printf("generated %zu lines, %zu bytes\n", target_lines, doc_len);
}

// One hard line of several megabytes: the wrapped mode's per-line budget
// freezes its tail, the no-wrap mode scrolls into it horizontally.
static void generate_mega_line(size_t target_bytes) {
    size_t cap = target_bytes + DOC_HEADROOM;
    char *buf = (char *)realloc(doc_buf, cap);
    if (buf == NULL) return;
    doc_buf = buf;
    doc_cap = cap;

    size_t off = 0;
    while (off + 8 < target_bytes) {
        off += (size_t)snprintf(buf + off, cap - off, "%07zu ", off);
    }
    buf[off++] = '\n';
    buf[off] = '\0';
    doc_len = off;
    doc_revision++;
    printf("generated one %zu-byte line\n", doc_len);
}

static void load_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) { printf("cannot open %s\n", path); return; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return; }
    size_t cap = (size_t)size + DOC_HEADROOM;
    char *buf = (char *)realloc(doc_buf, cap);
    if (buf == NULL) { fclose(f); return; }
    doc_buf = buf;
    doc_cap = cap;
    doc_len = fread(buf, 1, (size_t)size, f);
    buf[doc_len] = '\0';
    fclose(f);
    doc_revision++;
    printf("loaded %s: %zu bytes\n", path, doc_len);
}

int main(int argc, char **argv) {
    printf("Wollix Editor Demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix Editor Demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    if (argc > 1) {
        load_file(argv[1]);
    } else {
        generate_document(10000);
    }

    while (!WindowShouldClose()) {
        // Document switches (outside the widget = external mutation, so the
        // revision bump tells the editor to rebuild its line index).
        if (IsKeyPressed(KEY_ONE)) generate_document(10000);     // ~100 KB
        if (IsKeyPressed(KEY_TWO)) generate_document(1000000);   // ~10 MB / 1M lines
        if (IsKeyPressed(KEY_THREE)) generate_mega_line(4u << 20); // one ~4 MB line
        if (IsKeyPressed(KEY_W)) wrap_mode = !wrap_mode;

        float w = GetRenderWidth();
        float h = GetRenderHeight();
        WLX_Rect r = { .x = 0, .y = 0, .w = w, .h = h };

        wlx_begin(ctx, r, wlx_process_raylib_input);
            BeginDrawing();
                ClearBackground(WLX_BACKGROUND_COLOR);

                wlx_layout_begin(ctx, 3, WLX_VERT,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(30), WLX_SLOT_PX(26), WLX_SLOT_FLEX(1) });

                    char header[256];
                    snprintf(header, sizeof(header),
                        "editor: %zu bytes | %d fps | wrap %s (W) | 1 = 100 KB, 2 = 10 MB, 3 = mega-line, wheel / shift+wheel / PgUp / PgDn",
                        doc_len, GetFPS(), wrap_mode ? "on" : "off");
                    wlx_label(ctx, header, .font_size = 18,
                        .back_color = WLX_BACKGROUND_COLOR, .content_align = WLX_LEFT);

                    wlx_label(ctx, wrap_mode
                            ? "wrapped rows; the vertical thumb maps hard lines (documented approximation)"
                            : "drag the scrollbars; the vertical thumb is exact from the line count",
                        .font_size = 14, .back_color = WLX_BACKGROUND_COLOR, .content_align = WLX_LEFT);

                    if (doc_buf != NULL) {
                        wlx_editor(ctx, NULL, doc_buf, doc_cap, &doc_len,
                            .font_size = 18, .revision = doc_revision,
                            .wrap = wrap_mode, .line_numbers = wrap_mode);
                    }

                wlx_layout_end(ctx);

        wlx_end(ctx);
            EndDrawing();
    }

    CloseWindow();
    wlx_context_destroy(ctx);
    free(ctx);
    free(doc_buf);
    return 0;
}
