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

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define TARGET_FPS 60

typedef struct {
    char username[64];
    char password[64];
    char email[128];
    char phone[32];
    char address[256];
    char comments[512];
} App_State;

static App_State app = {0};

int main(void) {
    printf("Wollix Input Widget Demo\n");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wollix Input Widget Demo");
    SetTargetFPS(TARGET_FPS);

    WLX_Context *ctx = malloc(sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    wlx_context_init_raylib(ctx);

    // Initialize with some default values
    strcpy(app.username, "user123");
    strcpy(app.email, "user@example.com");

    bool show_submit = false;

    while (!WindowShouldClose()) {
        float w = GetRenderWidth();
        float h = GetRenderHeight();
        WLX_Rect r = {.x = 0, .y = 0, .w = w, .h = h};

        wlx_begin(ctx, r, wlx_process_raylib_input);
            BeginDrawing();
                ClearBackground(WLX_BACKGROUND_COLOR);

                wlx_layout_begin(ctx, 1, WLX_HORZ);
                    wlx_layout_begin(ctx, 11, WLX_VERT);

                        // Title
                        wlx_label(ctx, "User registration form",
                            .widget_align = WLX_TOP_LEFT, .height = 50, .font_size = 32, .back_color = WLX_BACKGROUND_COLOR, .align = WLX_LEFT
                        );

                        // Instructions
                        wlx_label(ctx, "Click on a field to edit. Press ENTER or ESC to finish editing.",
                            .widget_align = WLX_LEFT, .height = 30, .font_size = 16, .back_color = WLX_BACKGROUND_COLOR, .align = WLX_LEFT
                        );

                        // Username input
                        static bool username_was_focused = false;
                        bool username_focused = wlx_inputbox(ctx, "Username:", app.username, sizeof(app.username), .height = 45, .wrap = true);

                        if (username_focused && !username_was_focused) {
                            printf("Username field focused! Active ID: %zu. Type to enter text.\n", ctx->interaction.active_id);
                        }
                        if (!username_focused && username_was_focused) {
                            printf("Username field unfocused. Content: '%s'\n", app.username);
                        }
                        username_was_focused = username_focused;

                        // Password input (note: this is just a demo, real password fields would hide characters)
                        bool password_focused = wlx_inputbox(ctx, "Password:", app.password, sizeof(app.password), .height = 45);
                        if (password_focused && ctx->input.text_input[0] != '\0') {
                            printf("Text input this frame: '%s' | Password buffer: '%s'\n", ctx->input.text_input, app.password);
                        }

                        // Email input
                        wlx_inputbox(ctx, "Email:", app.email, sizeof(app.email), .height = 45);

                        // Phone input
                        wlx_inputbox(ctx, "Phone:", app.phone, sizeof(app.phone), .height = 45);

                        // Address input
                        wlx_inputbox(ctx, "Address:", app.address, sizeof(app.address), .height = 45);

                        // Comments input (larger field)
                        wlx_inputbox(ctx, "Comments:", app.comments, sizeof(app.comments), .height = 300, .wrap = true);

                        // Submit wlx_button
                        if (wlx_button(ctx, "Submit Form",
                            .widget_align = WLX_CENTER, .font_size = 22, .width = 200, .height = 50, .back_color = MAROON, .align = WLX_CENTER
                            // .widget_align = CENTER, .width = 300, .height = 40, .align = CENTER, .font_size = 20, .show_background = true
                        )) {
                            printf("\n=== FORM SUBMITTED ===\n");
                            printf("Username: %s\n", app.username);
                            printf("Password: %s\n", app.password);
                            printf("Email: %s\n", app.email);
                            printf("Phone: %s\n", app.phone);
                            printf("Address: %s\n", app.address);
                            printf("Comments: %s\n", app.comments);
                            printf("======================\n\n");
                            show_submit = true;
                        }

                        // Status message
                        if (show_submit) {
                            wlx_label(ctx, "Form submitted! Check console for details.",
                                .widget_align = WLX_CENTER, .height = 35, .font_size = 18, .back_color = WLX_BACKGROUND_COLOR, .align = WLX_LEFT
                            );
                        }

                        // Footer info
                        char info_text[256];
                        snprintf(info_text, sizeof(info_text),
                            "Fields filled: %d/6 | Active field ID: %zu",
                            (app.username[0] ? 1 : 0) + (app.password[0] ? 1 : 0) +
                            (app.email[0] ? 1 : 0) + (app.phone[0] ? 1 : 0) +
                            (app.address[0] ? 1 : 0) + (app.comments[0] ? 1 : 0),
                            ctx->interaction.active_id
                        );

                        wlx_label(ctx, info_text,
                            .widget_align = WLX_BOTTOM_LEFT, .height = 30, .font_size = 16, .back_color = WLX_BACKGROUND_COLOR, .align = WLX_LEFT
                        );

                    wlx_layout_end(ctx);
                wlx_layout_end(ctx);

            EndDrawing();
        wlx_end(ctx);
    }

    CloseWindow();
    wlx_context_destroy(ctx);
    free(ctx);
    return 0;
}
