// wollix_wasm.js — JS host for the wollix bare-wasm32 backend.
// Loads a .wasm module, provides Canvas 2D rendering imports, handles input,
// and drives the frame loop with requestAnimationFrame.
//
// Usage: <script type="module" src="wollix_wasm.js"></script>
//        with a <canvas id="wlx-canvas"> in the page.

const WASM_FILE = "gallery.wasm";
const CANVAS_ID = "wlx-canvas";
const TARGET_FPS = 60; // 0 = uncapped (tracks monitor refresh rate)

// ============================================================================
// WLX_Input_State layout (must match C struct on wasm32)
// ============================================================================

const INPUT_OFFSETS = {
    mouse_x:       0,   // int32
    mouse_y:       4,   // int32
    mouse_down:    8,   // bool (uint8)
    mouse_clicked: 9,   // bool (uint8)
    mouse_held:    10,  // bool (uint8)
    wheel_delta:   12,  // float32
    keys_down:     16,  // bool[46]
    keys_pressed:  62,  // bool[46]
    text_input:    108, // char[32]
};
const INPUT_SIZE = 140;
const WLX_KEY_COUNT = 46;

// WLX_Key_Code enum values (must match wollix.h)
const WLX_KEY = {
    NONE: 0, ESCAPE: 1, ENTER: 2, BACKSPACE: 3, TAB: 4, SPACE: 5,
    LEFT: 6, RIGHT: 7, UP: 8, DOWN: 9,
    A: 10, B: 11, C: 12, D: 13, E: 14, F: 15, G: 16, H: 17,
    I: 18, J: 19, K: 20, L: 21, M: 22, N: 23, O: 24, P: 25,
    Q: 26, R: 27, S: 28, T: 29, U: 30, V: 31, W: 32, X: 33,
    Y: 34, Z: 35,
    0: 36, 1: 37, 2: 38, 3: 39, 4: 40, 5: 41, 6: 42, 7: 43, 8: 44, 9: 45,
};

// Map DOM KeyboardEvent.code to WLX_Key_Code
const KEY_MAP = {
    Escape: WLX_KEY.ESCAPE, Enter: WLX_KEY.ENTER,
    Backspace: WLX_KEY.BACKSPACE, Tab: WLX_KEY.TAB, Space: WLX_KEY.SPACE,
    ArrowLeft: WLX_KEY.LEFT, ArrowRight: WLX_KEY.RIGHT,
    ArrowUp: WLX_KEY.UP, ArrowDown: WLX_KEY.DOWN,
    KeyA: WLX_KEY.A, KeyB: WLX_KEY.B, KeyC: WLX_KEY.C, KeyD: WLX_KEY.D,
    KeyE: WLX_KEY.E, KeyF: WLX_KEY.F, KeyG: WLX_KEY.G, KeyH: WLX_KEY.H,
    KeyI: WLX_KEY.I, KeyJ: WLX_KEY.J, KeyK: WLX_KEY.K, KeyL: WLX_KEY.L,
    KeyM: WLX_KEY.M, KeyN: WLX_KEY.N, KeyO: WLX_KEY.O, KeyP: WLX_KEY.P,
    KeyQ: WLX_KEY.Q, KeyR: WLX_KEY.R, KeyS: WLX_KEY.S, KeyT: WLX_KEY.T,
    KeyU: WLX_KEY.U, KeyV: WLX_KEY.V, KeyW: WLX_KEY.W, KeyX: WLX_KEY.X,
    KeyY: WLX_KEY.Y, KeyZ: WLX_KEY.Z,
    Digit0: WLX_KEY[0], Digit1: WLX_KEY[1], Digit2: WLX_KEY[2],
    Digit3: WLX_KEY[3], Digit4: WLX_KEY[4], Digit5: WLX_KEY[5],
    Digit6: WLX_KEY[6], Digit7: WLX_KEY[7], Digit8: WLX_KEY[8],
    Digit9: WLX_KEY[9],
};

// ============================================================================
// Helpers
// ============================================================================

const decoder = new TextDecoder("utf-8");
const encoder = new TextEncoder();

function unpackColor(rgba) {
    return {
        r: (rgba >>> 24) & 0xFF,
        g: (rgba >>> 16) & 0xFF,
        b: (rgba >>>  8) & 0xFF,
        a: (rgba >>>  0) & 0xFF,
    };
}

function cssColor(rgba) {
    const c = unpackColor(rgba);
    return `rgba(${c.r},${c.g},${c.b},${c.a / 255})`;
}

// ============================================================================
// Boot
// ============================================================================

(async function main() {
    const canvas = document.getElementById(CANVAS_ID);
    if (!canvas) {
        console.error(`wollix_wasm.js: no <canvas id="${CANVAS_ID}"> found`);
        return;
    }
    const ctx = canvas.getContext("2d");

    // Shared state written by DOM events, read by writeInputToWasm()
    const input = {
        mouseX: 0, mouseY: 0,
        mouseDown: false, prevMouseDown: false,
        wheelDelta: 0,
        keysDown: new Uint8Array(WLX_KEY_COUNT),
        keysPressed: new Uint8Array(WLX_KEY_COUNT),
        textInput: "",
    };

    let memory = null;    // WebAssembly.Memory, set after instantiation
    let inputPtr = 0;     // pointer into wasm memory for WLX_Input_State
    let lastTime = 0;     // for get_frame_time
    let frameTime = 0;
    let targetInterval = TARGET_FPS > 0 ? 1000 / TARGET_FPS : 0;
    let lastRenderedTime = 0;

    // ========================================================================
    // Read a NUL-terminated C string from wasm memory
    // ========================================================================
    function readCString(ptr) {
        const mem = new Uint8Array(memory.buffer);
        let end = ptr;
        while (mem[end] !== 0) end++;
        return decoder.decode(mem.subarray(ptr, end));
    }

    // ========================================================================
    // Scissor state
    // ========================================================================
    // wollix restores parent clipping explicitly after ending a nested scissor.
    // Match that contract by keeping at most one active clip in Canvas at a time.
    let scissorActive = false;

    // ========================================================================
    // "wlx" module imports — 10 rendering callbacks
    // ========================================================================
    const wlxImports = {
        draw_rect(x, y, w, h, rgba) {
            ctx.fillStyle = cssColor(rgba);
            ctx.fillRect(x, y, w, h);
        },

        draw_rect_lines(x, y, w, h, thick, rgba) {
            ctx.strokeStyle = cssColor(rgba);
            ctx.lineWidth = thick;
            ctx.strokeRect(x + thick / 2, y + thick / 2, w - thick, h - thick);
        },

        draw_rect_rounded(x, y, w, h, roundness, _segments, rgba) {
            const r = Math.min(roundness * Math.min(w, h) / 2, Math.min(w, h) / 2);
            ctx.fillStyle = cssColor(rgba);
            ctx.beginPath();
            ctx.roundRect(x, y, w, h, r);
            ctx.fill();
        },

        draw_rect_rounded_lines(x, y, w, h, roundness, _segments, thick, rgba) {
            const r = Math.min(roundness * Math.min(w, h) / 2, Math.min(w, h) / 2);
            ctx.strokeStyle = cssColor(rgba);
            ctx.lineWidth = thick;
            ctx.beginPath();
            ctx.roundRect(x, y, w, h, r);
            ctx.stroke();
        },

        draw_circle(cx, cy, radius, _segments, rgba) {
            ctx.fillStyle = cssColor(rgba);
            ctx.beginPath();
            ctx.arc(cx, cy, radius, 0, Math.PI * 2);
            ctx.fill();
        },

        draw_ring(cx, cy, innerR, outerR, _segments, rgba) {
            const lineW = outerR - innerR;
            const midR = (innerR + outerR) / 2;
            ctx.strokeStyle = cssColor(rgba);
            ctx.lineWidth = lineW;
            ctx.beginPath();
            ctx.arc(cx, cy, midR, 0, Math.PI * 2);
            ctx.stroke();
        },

        draw_line(x1, y1, x2, y2, thick, rgba) {
            ctx.strokeStyle = cssColor(rgba);
            ctx.lineWidth = thick;
            ctx.beginPath();
            ctx.moveTo(x1, y1);
            ctx.lineTo(x2, y2);
            ctx.stroke();
        },

        draw_text(textPtr, x, y, _font, fontSize, _spacing, rgba) {
            if (textPtr === 0) return;
            const text = readCString(textPtr);
            if (text.length === 0) return;
            ctx.fillStyle = cssColor(rgba);
            ctx.font = `${fontSize}px sans-serif`;
            ctx.textBaseline = "top";
            ctx.fillText(text, x, y);
        },

        measure_text(textPtr, _font, fontSize, spacing, outWPtr, outHPtr) {
            const f32 = new Float32Array(memory.buffer);
            const wIdx = outWPtr >> 2; // byte offset to f32 index (divide by 4)
            const hIdx = outHPtr >> 2;
            if (textPtr === 0) {
                f32[wIdx] = 0;
                f32[hIdx] = fontSize > 0 ? fontSize : 16;
                return;
            }
            const text = readCString(textPtr);
            ctx.font = `${fontSize}px sans-serif`;

            const metrics = ctx.measureText(text);
            let w = metrics.width;
            if (text.length > 1 && spacing > 0) {
                w += (text.length - 1) * spacing;
            }

            f32[wIdx] = w;
            f32[hIdx] = fontSize > 0 ? fontSize : 16;
        },

        draw_texture(_handle, _sx, _sy, _sw, _sh,
                     _dx, _dy, _dw, _dh, _tint) {
            // Texture support is a stub for now
        },

        begin_scissor(x, y, w, h) {
            if (scissorActive) {
                ctx.restore();
                scissorActive = false;
            }
            ctx.save();
            ctx.beginPath();
            ctx.rect(x, y, w, h);
            ctx.clip();
            scissorActive = true;
        },

        end_scissor() {
            if (scissorActive) {
                ctx.restore();
                scissorActive = false;
            }
        },

        get_frame_time() {
            return frameTime;
        },
    };

    // ========================================================================
    // "env" module imports — bare-wasm specific
    // ========================================================================
    const envImports = {
        abort() {
            throw new Error("wasm abort");
        },

        roundf(x) {
            const floorValue = Math.floor(x);
            const diff = x - floorValue;
            if (diff < 0.5) return floorValue;
            if (diff > 0.5) return floorValue + 1;
            return x < 0 ? floorValue : floorValue + 1;
        },

        floorf(x) {
            return Math.floor(x);
        },

        ceilf(x) {
            return Math.ceil(x);
        },

        fabsf(x) {
            return Math.abs(x);
        },

        sqrtf(x) {
            return Math.sqrt(x);
        },

        fmodf(x, y) {
            if (y === 0) return 0;
            return x - Math.trunc(x / y) * y;
        },

        truncf(x) {
            return Math.trunc(x);
        },
    };

    // ========================================================================
    // Instantiate
    // ========================================================================
    const { instance } = await WebAssembly.instantiateStreaming(
        fetch(WASM_FILE),
        { wlx: wlxImports, env: envImports }
    );

    memory = instance.exports.memory;
    const wasmInit  = instance.exports.wlx_wasm_init;
    const wasmFrame = instance.exports.wlx_wasm_frame;
    const getInputPtr = instance.exports.wlx_wasm_get_input_ptr;
    const inputStateExport = instance.exports.wlx_wasm_input_state;

    // Different toolchains can expose the shared input-state symbol in
    // different shapes. Accept the legacy getter and the direct symbol export.
    if (typeof getInputPtr === "function") {
        // Older builds export a helper that returns the input struct address.
        inputPtr = getInputPtr();
    } else if (typeof inputStateExport === "number") {
        // Some runtimes surface the symbol as a raw numeric address.
        inputPtr = inputStateExport;
    } else if (inputStateExport instanceof WebAssembly.Global) {
        // Others expose the symbol as a WebAssembly.Global wrapper.
        inputPtr = inputStateExport.value;
    } else {
        throw new Error("wollix_wasm.js: missing input state export");
    }

    wasmInit();

    // ========================================================================
    // Input event listeners
    // ========================================================================
    canvas.addEventListener("mousemove", (e) => {
        const rect = canvas.getBoundingClientRect();
        input.mouseX = e.clientX - rect.left;
        input.mouseY = e.clientY - rect.top;
    });

    canvas.addEventListener("mousedown", (e) => {
        if (e.button === 0) input.mouseDown = true;
    });

    canvas.addEventListener("mouseup", (e) => {
        if (e.button === 0) input.mouseDown = false;
    });

    canvas.addEventListener("wheel", (e) => {
        e.preventDefault();
        input.wheelDelta += e.deltaY < 0 ? 1 : e.deltaY > 0 ? -1 : 0;
    }, { passive: false });

    canvas.addEventListener("contextmenu", (e) => e.preventDefault());

    document.addEventListener("keydown", (e) => {
        const wlxKey = KEY_MAP[e.code];
        if (wlxKey !== undefined) {
            e.preventDefault();
            if (!input.keysDown[wlxKey]) {
                input.keysPressed[wlxKey] = 1;
            }
            input.keysDown[wlxKey] = 1;
        }
        // Collect text input from printable keys
        if (e.key.length === 1 && !e.ctrlKey && !e.metaKey) {
            input.textInput += e.key;
        }
    });

    document.addEventListener("keyup", (e) => {
        const wlxKey = KEY_MAP[e.code];
        if (wlxKey !== undefined) {
            input.keysDown[wlxKey] = 0;
        }
    });

    // ========================================================================
    // Write JS input state into wasm memory
    // ========================================================================
    function writeInputToWasm() {
        const i32 = new Int32Array(memory.buffer);
        const u8  = new Uint8Array(memory.buffer);
        const f32 = new Float32Array(memory.buffer);

        const base = inputPtr;

        i32[(base + INPUT_OFFSETS.mouse_x) >> 2] = input.mouseX;
        i32[(base + INPUT_OFFSETS.mouse_y) >> 2] = input.mouseY;

        u8[base + INPUT_OFFSETS.mouse_down]    = input.mouseDown ? 1 : 0;
        u8[base + INPUT_OFFSETS.mouse_clicked] =
            (input.mouseDown && !input.prevMouseDown) ? 1 : 0;
        u8[base + INPUT_OFFSETS.mouse_held]    = input.mouseDown ? 1 : 0;

        f32[(base + INPUT_OFFSETS.wheel_delta) >> 2] = input.wheelDelta;

        // keys_down and keys_pressed
        u8.set(input.keysDown, base + INPUT_OFFSETS.keys_down);
        u8.set(input.keysPressed, base + INPUT_OFFSETS.keys_pressed);

        // text_input (NUL-terminated, max 31 chars)
        const textBytes = encoder.encode(input.textInput);
        const maxLen = 31;
        const len = Math.min(textBytes.length, maxLen);
        u8.set(textBytes.subarray(0, len), base + INPUT_OFFSETS.text_input);
        u8[base + INPUT_OFFSETS.text_input + len] = 0;
        // Zero remaining bytes
        for (let i = len + 1; i < 32; i++) {
            u8[base + INPUT_OFFSETS.text_input + i] = 0;
        }

        // Reset per-frame state
        input.prevMouseDown = input.mouseDown;
        input.wheelDelta = 0;
        input.keysPressed.fill(0);
        input.textInput = "";
    }

    // ========================================================================
    // Frame loop
    // ========================================================================
    function onFrame(timestamp) {
        requestAnimationFrame(onFrame);

        // Skip frame if below target interval
        if (targetInterval > 0) {
            if (lastRenderedTime > 0 && (timestamp - lastRenderedTime) < targetInterval - 1) {
                return;
            }
            lastRenderedTime = timestamp;
        }

        // Compute frame time in seconds (elapsed between rendered frames)
        if (lastTime === 0) lastTime = timestamp;
        frameTime = Math.min((timestamp - lastTime) / 1000, 0.25);
        lastTime = timestamp;

        // Resize canvas to match display size
        const dpr = window.devicePixelRatio || 1;
        const displayW = canvas.clientWidth;
        const displayH = canvas.clientHeight;
        if (canvas.width !== displayW * dpr || canvas.height !== displayH * dpr) {
            canvas.width = displayW * dpr;
            canvas.height = displayH * dpr;
            ctx.scale(dpr, dpr);
        }

        // Clear
        ctx.clearRect(0, 0, displayW, displayH);

        // Push input into wasm memory
        writeInputToWasm();

        // Call the wasm frame function with canvas dimensions
        wasmFrame(displayW, displayH);
    }

    requestAnimationFrame(onFrame);
})();
