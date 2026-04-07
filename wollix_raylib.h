#ifndef WOLLIX_RAYLIB_H_
#define WOLLIX_RAYLIB_H_

#ifndef WOLLIX_H_
#error "Include wollix.h before wollix_raylib.h"
#endif

#ifndef RAYLIB_H
#error "Include raylib.h before wollix.h when using wollix_raylib.h"
#endif

static inline WLX_Key_Code wlx_raylib_map_key(int raylib_key) {
    switch (raylib_key) {
        case KEY_ESCAPE: return WLX_KEY_ESCAPE;
        case KEY_ENTER: return WLX_KEY_ENTER;
        case KEY_BACKSPACE: return WLX_KEY_BACKSPACE;
        case KEY_TAB: return WLX_KEY_TAB;
        case KEY_SPACE: return WLX_KEY_SPACE;
        case KEY_LEFT: return WLX_KEY_LEFT;
        case KEY_RIGHT: return WLX_KEY_RIGHT;
        case KEY_UP: return WLX_KEY_UP;
        case KEY_DOWN: return WLX_KEY_DOWN;
        case KEY_A: return WLX_KEY_A;
        case KEY_B: return WLX_KEY_B;
        case KEY_C: return WLX_KEY_C;
        case KEY_D: return WLX_KEY_D;
        case KEY_E: return WLX_KEY_E;
        case KEY_F: return WLX_KEY_F;
        case KEY_G: return WLX_KEY_G;
        case KEY_H: return WLX_KEY_H;
        case KEY_I: return WLX_KEY_I;
        case KEY_J: return WLX_KEY_J;
        case KEY_K: return WLX_KEY_K;
        case KEY_L: return WLX_KEY_L;
        case KEY_M: return WLX_KEY_M;
        case KEY_N: return WLX_KEY_N;
        case KEY_O: return WLX_KEY_O;
        case KEY_P: return WLX_KEY_P;
        case KEY_Q: return WLX_KEY_Q;
        case KEY_R: return WLX_KEY_R;
        case KEY_S: return WLX_KEY_S;
        case KEY_T: return WLX_KEY_T;
        case KEY_U: return WLX_KEY_U;
        case KEY_V: return WLX_KEY_V;
        case KEY_W: return WLX_KEY_W;
        case KEY_X: return WLX_KEY_X;
        case KEY_Y: return WLX_KEY_Y;
        case KEY_Z: return WLX_KEY_Z;
        case KEY_ZERO: return WLX_KEY_0;
        case KEY_ONE: return WLX_KEY_1;
        case KEY_TWO: return WLX_KEY_2;
        case KEY_THREE: return WLX_KEY_3;
        case KEY_FOUR: return WLX_KEY_4;
        case KEY_FIVE: return WLX_KEY_5;
        case KEY_SIX: return WLX_KEY_6;
        case KEY_SEVEN: return WLX_KEY_7;
        case KEY_EIGHT: return WLX_KEY_8;
        case KEY_NINE: return WLX_KEY_9;
        default: return WLX_KEY_NONE;
    }
}

static inline void wlx_process_raylib_input(WLX_Context *ctx) {
    static bool prev_mouse_down = false;
    static float prev_wheel_dir = 0.0f;   // last non-zero wheel direction (+1 or -1)
    static bool  prev_was_zero  = true;    // was previous frame's raw wheel zero?

    Vector2 mouse_pos = GetMousePosition();
    ctx->input.mouse_x = mouse_pos.x;
    ctx->input.mouse_y = mouse_pos.y;
    ctx->input.mouse_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    ctx->input.mouse_clicked = ctx->input.mouse_down && !prev_mouse_down;
    ctx->input.mouse_held = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    // Debounce mouse wheel encoder bounce: if the direction suddenly
    // reverses for a single frame mid-scroll, suppress the spurious tick.
    float raw_wheel = GetMouseWheelMove();
    if (raw_wheel != 0.0f) {
        float dir = (raw_wheel > 0.0f) ? 1.0f : -1.0f;
        if (prev_wheel_dir != 0.0f && dir != prev_wheel_dir && !prev_was_zero) {
            // Single-frame reversal mid-scroll -> likely encoder bounce, suppress
            ctx->input.wheel_delta = 0.0f;
            // Don't update prev_wheel_dir - keep the "real" direction
        } else {
            ctx->input.wheel_delta = raw_wheel;
            prev_wheel_dir = dir;
        }
        prev_was_zero = false;
    } else {
        ctx->input.wheel_delta = 0.0f;
        prev_was_zero = true;
    }
    prev_mouse_down = ctx->input.mouse_down;

    wlx_zero_struct(ctx->input.keys_pressed);
    for (int raylib_key = 0; raylib_key < 350; raylib_key++) {
        WLX_Key_Code key = wlx_raylib_map_key(raylib_key);
        if (key != WLX_KEY_NONE) {
            bool is_down = IsKeyDown(raylib_key);
            bool was_down = ctx->input.keys_down[key];
            ctx->input.keys_down[key] = is_down;
            if (is_down && !was_down) {
                ctx->input.keys_pressed[key] = true;
            }
        }
    }

    wlx_zero_struct(ctx->input.text_input);
    int key = GetCharPressed();
    int text_len = 0;
    while (key > 0) {
        if (key >= 32) {
            char encoded[4];
            size_t enc_len = wlx_utf8_encode((uint32_t)key, encoded);
            if (text_len + (int)enc_len < (int)sizeof(ctx->input.text_input)) {
                memcpy(&ctx->input.text_input[text_len], encoded, enc_len);
                text_len += (int)enc_len;
            } else {
                break;  // buffer full
            }
        }
        key = GetCharPressed();
    }
}

static inline void wlx_raylib_draw_texture(WLX_Texture texture, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    DrawTexturePro(
        texture,
        (Rectangle){src.x, src.y, src.w, src.h},
        (Rectangle){dst.x, dst.y, dst.w, dst.h},
        (Vector2){0, 0},
        0.0f,
        tint
    );
}

static inline void wlx_raylib_draw_rect(WLX_Rect rect, WLX_Color color) {
    DrawRectangleRec((Rectangle){rect.x, rect.y, rect.w, rect.h}, color);
}

static inline void wlx_raylib_draw_rect_lines(WLX_Rect rect, float thick, WLX_Color color) {
    DrawRectangleLinesEx((Rectangle){rect.x, rect.y, rect.w, rect.h}, thick, color);
}

static inline void wlx_raylib_draw_rect_rounded(WLX_Rect rect, float roundness, int segments, WLX_Color color) {
    DrawRectangleRounded((Rectangle){rect.x, rect.y, rect.w, rect.h}, roundness, segments, color);
}

static inline void wlx_raylib_draw_line(float x1, float y1, float x2, float y2, float thick, WLX_Color color) {
    DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, thick, color);
}

static inline void wlx_raylib_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    Font font = (style.font != WLX_FONT_DEFAULT)
              ? *(Font *)(uintptr_t)style.font
              : GetFontDefault();
    DrawTextEx(font, text, (Vector2){x, y}, style.font_size, style.spacing,
               (Color){style.color.r, style.color.g, style.color.b, style.color.a});
}

static inline void wlx_raylib_measure_text(const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
    Font font = (style.font != WLX_FONT_DEFAULT)
              ? *(Font *)(uintptr_t)style.font
              : GetFontDefault();
    Vector2 size = MeasureTextEx(font, text, style.font_size, style.spacing);
    *out_w = size.x;
    *out_h = size.y;
}

static inline WLX_Font wlx_font_from_raylib(Font *font) {
    return (WLX_Font)(uintptr_t)font;
}

static inline void wlx_raylib_begin_scissor(WLX_Rect rect) {
    BeginScissorMode(rect.x, rect.y, rect.w, rect.h);
}

static inline void wlx_raylib_end_scissor(void) {
    EndScissorMode();
}

static inline float wlx_raylib_get_frame_time(void) {
    return GetFrameTime();
}

static inline WLX_Backend wlx_backend_raylib(void) {
    return (WLX_Backend){
        .draw_rect = wlx_raylib_draw_rect,
        .draw_rect_lines = wlx_raylib_draw_rect_lines,
        .draw_rect_rounded = wlx_raylib_draw_rect_rounded,
        .draw_line = wlx_raylib_draw_line,
        .draw_text = wlx_raylib_draw_text,
        .measure_text = wlx_raylib_measure_text,
        .draw_texture = wlx_raylib_draw_texture,
        .begin_scissor = wlx_raylib_begin_scissor,
        .end_scissor = wlx_raylib_end_scissor,
        .get_frame_time = wlx_raylib_get_frame_time,
    };
}

static inline void wlx_context_init_raylib(WLX_Context *ctx) {
    assert(ctx != NULL);
    ctx->backend = wlx_backend_raylib();
}

#endif // WOLLIX_RAYLIB_H_
