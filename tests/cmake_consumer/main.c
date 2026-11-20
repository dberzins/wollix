// A program built against the installed wollix package and nothing else.
// It compiles the implementation, which needs the C maths library on most
// platforms, so it links only if wollix::wollix brings that library along.
#include <stdio.h>
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_editor.h"

// Not called. It holds option-macro calls of the kind every application
// writes, so the build shows the target's compile options keep them quiet.
void consumer_ui(WLX_Context *ctx, char *buf, size_t cap, size_t *len) {
    wlx_layout_begin(ctx, 2, WLX_VERT, .padding = 8, .gap = 4);
    wlx_label(ctx, "hello", .font_size = 14, .content_align = WLX_CENTER);
    (void)wlx_editor(ctx, NULL, buf, cap, len, .line_numbers = true);
    wlx_layout_end(ctx);
}

int main(void) {
    printf("wollix %s from the installed package\n", WOLLIX_VERSION);
    return 0;
}
