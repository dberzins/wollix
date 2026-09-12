// wollix_wasm.js — JS host for the wollix bare-wasm32 backend.
// Loads a .wasm module, provides Canvas 2D rendering imports, handles input,
// and drives the frame loop with requestAnimationFrame.
//
// Usage: <script type="module" src="wollix_wasm.js"></script>
//        with a <canvas id="wlx-canvas"> in the page.

// The module to load. Defaults to the gallery; a host page can point the same
// JS host at another build by setting window.WLX_WASM_FILE or a ?wasm= query
// param before this module runs (used by the dashboard site).
const WASM_FILE =
    (typeof window !== "undefined" && window.WLX_WASM_FILE) ||
    (typeof location !== "undefined" &&
        new URLSearchParams(location.search).get("wasm")) ||
    "gallery.wasm";
const CANVAS_ID = "wlx-canvas";
const TARGET_FPS = 60; // 0 = uncapped (tracks monitor refresh rate)

const TINT_CACHE_PIXEL_BUDGET = 8 * 1024 * 1024;
const TINT_LARGE_TEXTURE_PIXEL_THRESHOLD = 1024 * 1024;
const TINT_FILTER_PROBE_RGB = 0x7F7F7F;

// ============================================================================
// WLX_Input_State layout (must match C struct on wasm32)
// ============================================================================

// Byte offsets into WLX_Input_State. These MUST track the C struct layout in
// wollix.h: WLX_KEY_COUNT sizes the keys_* arrays, so adding keycodes shifts
// every field after keys_down. New fields are appended at the end of the
// struct. wollix_wasm.h static-asserts the same offsets, so a mismatch fails
// the wasm build rather than the demo.
const INPUT_OFFSETS = {
    mouse_x:              0,   // int32
    mouse_y:              4,   // int32
    mouse_down:           8,   // bool (uint8)
    mouse_clicked:        9,   // bool (uint8)
    mouse_held:           10,  // bool (uint8)
    wheel_delta:          12,  // float32 (vertical detents, positive = up)
    keys_down:            16,  // bool[64]
    keys_pressed:         80,  // bool[64]
    text_input:           144, // char[32]
    keys_repeated:        176, // bool[64]
    modifiers:            240, // uint32 (4-byte aligned)
    wheel_delta_x:        244, // float32 (horizontal detents)
    mouse_right_down:     248, // bool (uint8)
    mouse_right_clicked:  249, // bool (uint8)
    mouse_middle_down:    250, // bool (uint8)
    mouse_middle_clicked: 251, // bool (uint8)
};
const INPUT_SIZE = 252;
const WLX_KEY_COUNT = 64;

// DOM wheel deltas arrive in pixels (deltaMode 0), lines (1), or pages (2).
// The input contract carries float detents (1.0 = one notch); these divisors
// convert each mode to detents without quantizing trackpad fractions.
const WHEEL_PIXELS_PER_DETENT = 100;
const WHEEL_LINES_PER_DETENT  = 3;

// WLX_Key_Mod bit flags (must match wollix.h)
const WLX_MOD = { SHIFT: 1 << 0, CTRL: 1 << 1, ALT: 1 << 2, SUPER: 1 << 3 };

// WLX_Key_Code enum values (must match wollix.h)
const WLX_KEY = {
    NONE: 0, ESCAPE: 1, ENTER: 2, BACKSPACE: 3, TAB: 4, SPACE: 5,
    LEFT: 6, RIGHT: 7, UP: 8, DOWN: 9,
    A: 10, B: 11, C: 12, D: 13, E: 14, F: 15, G: 16, H: 17,
    I: 18, J: 19, K: 20, L: 21, M: 22, N: 23, O: 24, P: 25,
    Q: 26, R: 27, S: 28, T: 29, U: 30, V: 31, W: 32, X: 33,
    Y: 34, Z: 35,
    0: 36, 1: 37, 2: 38, 3: 39, 4: 40, 5: 41, 6: 42, 7: 43, 8: 44, 9: 45,
    DELETE: 46, HOME: 47, END: 48, PAGE_UP: 49, PAGE_DOWN: 50,
    F1: 51, F2: 52, F3: 53, F4: 54, F5: 55, F6: 56,
    F7: 57, F8: 58, F9: 59, F10: 60, F11: 61, F12: 62,
    INSERT: 63,
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
    Delete: WLX_KEY.DELETE, Home: WLX_KEY.HOME, End: WLX_KEY.END,
    PageUp: WLX_KEY.PAGE_UP, PageDown: WLX_KEY.PAGE_DOWN,
    F1: WLX_KEY.F1, F2: WLX_KEY.F2, F3: WLX_KEY.F3, F4: WLX_KEY.F4,
    F5: WLX_KEY.F5, F6: WLX_KEY.F6, F7: WLX_KEY.F7, F8: WLX_KEY.F8,
    F9: WLX_KEY.F9, F10: WLX_KEY.F10, F11: WLX_KEY.F11, F12: WLX_KEY.F12,
    Insert: WLX_KEY.INSERT,
};

// F-keys are recorded for the widget but never preventDefault'd: F5 reload,
// F11 fullscreen and F12 devtools keep their browser meaning.
const BROWSER_OWNED_KEYS = new Set([
    WLX_KEY.F1, WLX_KEY.F2, WLX_KEY.F3, WLX_KEY.F4, WLX_KEY.F5, WLX_KEY.F6,
    WLX_KEY.F7, WLX_KEY.F8, WLX_KEY.F9, WLX_KEY.F10, WLX_KEY.F11, WLX_KEY.F12,
]);

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

function packTintKey(r, g, b) {
    return (r << 16) | (g << 8) | b;
}

function buildTintFilterUrl(r, g, b) {
    const fr = r / 255;
    const fg = g / 255;
    const fb = b / 255;
    const matrix =
        `${fr} 0 0 0 0 ` +
        `0 ${fg} 0 0 0 ` +
        `0 0 ${fb} 0 0 ` +
        `0 0 0 1 0`;
    const svg =
        '<svg xmlns="http://www.w3.org/2000/svg">' +
          '<filter id="t" color-interpolation-filters="sRGB">' +
            `<feColorMatrix values="${matrix}"/>` +
          '</filter>' +
        '</svg>';
    return 'data:image/svg+xml;utf8,' + encodeURIComponent(svg) + '#t';
}

// OffscreenCanvas is missing on Safari < 16.4. Fall back to a detached
// <canvas> element with the same draw semantics.
const HAS_OFFSCREEN_CANVAS = typeof OffscreenCanvas !== "undefined";

function createTextureCanvas(width, height) {
    if (HAS_OFFSCREEN_CANVAS) {
        return new OffscreenCanvas(width, height);
    }
    const canvas = document.createElement("canvas");
    canvas.width = width;
    canvas.height = height;
    return canvas;
}

// One-shot detection that Canvas2D `ctx.filter` honors an inline SVG
// `feColorMatrix` reference. Browsers without filter support assign silently
// and leave pixels untouched; we readback a known-zeroed channel to confirm.
function probeCtxFilterSupported() {
    try {
        const r = (TINT_FILTER_PROBE_RGB >>> 16) & 0xFF;
        const g = (TINT_FILTER_PROBE_RGB >>>  8) & 0xFF;
        const b = (TINT_FILTER_PROBE_RGB >>>  0) & 0xFF;

        const src = createTextureCanvas(1, 1);
        const srcCtx = src.getContext("2d");
        if (!srcCtx) return false;
        srcCtx.fillStyle = `rgb(${r},${g},${b})`;
        srcCtx.fillRect(0, 0, 1, 1);

        const dst = createTextureCanvas(1, 1);
        const dstCtx = dst.getContext("2d");
        if (!dstCtx) return false;

        const svg =
            '<svg xmlns="http://www.w3.org/2000/svg">' +
              '<filter id="t" color-interpolation-filters="sRGB">' +
                '<feColorMatrix values="0 0 0 0 0  0 1 0 0 0  0 0 1 0 0  0 0 0 1 0"/>' +
              '</filter>' +
            '</svg>';
        const url = 'data:image/svg+xml;utf8,' + encodeURIComponent(svg) + '#t';

        dstCtx.save();
        dstCtx.filter = `url("${url}")`;
        dstCtx.drawImage(src, 0, 0);
        dstCtx.restore();

        const px = dstCtx.getImageData(0, 0, 1, 1).data;
        return px[0] === 0;
    } catch (_e) {
        return false;
    }
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
        rightDown: false, prevRightDown: false,
        middleDown: false, prevMiddleDown: false,
        wheelDelta: 0, wheelDeltaX: 0,
        keysDown: new Uint8Array(WLX_KEY_COUNT),
        keysPressed: new Uint8Array(WLX_KEY_COUNT),
        keysRepeated: new Uint8Array(WLX_KEY_COUNT),
        modifiers: 0,
        textInput: "",
    };

    // Best-effort clipboard cache. The async Clipboard API cannot be read
    // synchronously mid-frame, so in-app copies populate this cache directly and
    // a DOM paste event refreshes it from the system clipboard when available.
    let clipboardCache = "";

    let memory = null;    // WebAssembly.Memory, set after instantiation
    let inputPtr = 0;     // pointer into wasm memory for WLX_Input_State
    let lastTime = 0;     // for get_frame_time
    let frameTime = 0;
    let targetInterval = TARGET_FPS > 0 ? 1000 / TARGET_FPS : 0;
    let lastRenderedTime = 0;

    // Texture registry: maps handle -> { canvas, w, h, variants, bypassed }.
    // Handle 0 is reserved as invalid so the C side can short-circuit on a
    // zeroed WLX_Texture.
    const textures = new Map();
    let nextTextureHandle = 1;
    let liveScratchCanvas = null;
    let liveScratchCtx = null;
    let tintCachePixels = 0;
    let tintLruCounter = 0;
    let tintVariantGenerationCount = 0;
    let ctxFilterSupported = false;

    // Destination-sized scratch canvas used by the live-tint fallback when
    // ctx.filter is unavailable. The variant cache owns its own per-texture
    // canvases and does not consult this helper.
    function getLiveScratchContext(width, height) {
        if (!liveScratchCanvas) {
            liveScratchCanvas = createTextureCanvas(width, height);
            liveScratchCtx = liveScratchCanvas.getContext("2d");
        } else if (liveScratchCanvas.width !== width || liveScratchCanvas.height !== height) {
            liveScratchCanvas.width = width;
            liveScratchCanvas.height = height;
        }
        return liveScratchCtx;
    }

    // Drain LRU variants across all texture entries until the cache can fit
    // `neededPixels` within the budget. If a single new variant alone exceeds
    // the budget the loop runs the cache to empty and exits; the caller is
    // expected to fall back to a live-draw path in that case.
    function evictLruIfNeeded(neededPixels) {
        while (tintCachePixels + neededPixels > TINT_CACHE_PIXEL_BUDGET) {
            let lruEntry = null;
            let lruKey = -1;
            let lruTick = Infinity;
            for (const entry of textures.values()) {
                for (const [key, variant] of entry.variants) {
                    if (variant.lastUsed < lruTick) {
                        lruTick = variant.lastUsed;
                        lruEntry = entry;
                        lruKey = key;
                    }
                }
            }
            if (lruEntry === null) return;
            const variant = lruEntry.variants.get(lruKey);
            lruEntry.variants.delete(lruKey);
            tintCachePixels -= variant.pixels;
        }
    }

    function generateVariantFilter(entry, r, g, b) {
        try {
            const destCanvas = createTextureCanvas(entry.w, entry.h);
            const destCtx = destCanvas.getContext("2d");
            if (!destCtx) return null;

            const url = buildTintFilterUrl(r, g, b);
            destCtx.save();
            destCtx.filter = `url("${url}")`;
            destCtx.drawImage(entry.canvas, 0, 0);
            destCtx.restore();

            tintVariantGenerationCount++;
            return {
                canvas: destCanvas,
                lastUsed: ++tintLruCounter,
                pixels: entry.w * entry.h,
            };
        } catch (_e) {
            return null;
        }
    }

    function generateVariantScratch(entry, r, g, b) {
        const destCanvas = createTextureCanvas(entry.w, entry.h);
        const destCtx = destCanvas.getContext("2d");
        if (!destCtx) return null;

        destCtx.globalAlpha = 1;
        destCtx.globalCompositeOperation = "source-over";
        destCtx.drawImage(entry.canvas, 0, 0);
        destCtx.globalCompositeOperation = "multiply";
        destCtx.fillStyle = `rgb(${r},${g},${b})`;
        destCtx.fillRect(0, 0, entry.w, entry.h);
        destCtx.globalCompositeOperation = "destination-in";
        destCtx.drawImage(entry.canvas, 0, 0);
        destCtx.globalCompositeOperation = "source-over";

        tintVariantGenerationCount++;
        return {
            canvas: destCanvas,
            lastUsed: ++tintLruCounter,
            pixels: entry.w * entry.h,
        };
    }

    function ensureTintVariant(entry, r, g, b) {
        const key = packTintKey(r, g, b);
        const existing = entry.variants.get(key);
        if (existing) {
            existing.lastUsed = ++tintLruCounter;
            return existing;
        }
        const needed = entry.w * entry.h;
        evictLruIfNeeded(needed);
        // Drained but still can't fit (e.g. TINT_CACHE_PIXEL_BUDGET=0 rollback
        // config): signal the caller to take a live-draw path.
        if (tintCachePixels + needed > TINT_CACHE_PIXEL_BUDGET) {
            return null;
        }
        let variant = null;
        if (ctxFilterSupported) {
            variant = generateVariantFilter(entry, r, g, b);
        }
        if (!variant) {
            variant = generateVariantScratch(entry, r, g, b);
        }
        if (!variant) return null;
        entry.variants.set(key, variant);
        tintCachePixels += variant.pixels;
        return variant;
    }

    // Tint live every draw. Used by the large-texture bypass and by callers
    // whose variant could not be cached. Caller is responsible for managing
    // ctx.globalAlpha around the call.
    function drawTextureLive(entry, r, g, b, sx, sy, sw, sh, dx, dy, dw, dh) {
        if (ctxFilterSupported) {
            const url = buildTintFilterUrl(r, g, b);
            ctx.save();
            ctx.filter = `url("${url}")`;
            ctx.drawImage(entry.canvas, sx, sy, sw, sh, dx, dy, dw, dh);
            ctx.restore();
            return;
        }
        const tw = Math.max(1, Math.ceil(Math.abs(dw)));
        const th = Math.max(1, Math.ceil(Math.abs(dh)));
        const tctx = getLiveScratchContext(tw, th);
        if (!tctx) {
            ctx.drawImage(entry.canvas, sx, sy, sw, sh, dx, dy, dw, dh);
            return;
        }
        tctx.clearRect(0, 0, tw, th);
        tctx.globalAlpha = 1;
        tctx.globalCompositeOperation = "source-over";
        tctx.drawImage(entry.canvas, sx, sy, sw, sh, 0, 0, tw, th);
        tctx.globalCompositeOperation = "multiply";
        tctx.fillStyle = `rgb(${r},${g},${b})`;
        tctx.fillRect(0, 0, tw, th);
        tctx.globalCompositeOperation = "destination-in";
        tctx.drawImage(entry.canvas, sx, sy, sw, sh, 0, 0, tw, th);
        tctx.globalCompositeOperation = "source-over";

        ctx.drawImage(liveScratchCanvas, 0, 0, tw, th, dx, dy, dw, dh);
    }

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
    // "wlx" module imports — rendering callbacks, texture registry,
    // scissor stack, and frame-time accessor.
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

        draw_text(textPtr, x, y, _font, fontSize, rgba) {
            if (textPtr === 0) return;
            const text = readCString(textPtr);
            if (text.length === 0) return;
            ctx.fillStyle = cssColor(rgba);
            ctx.font = `${fontSize}px sans-serif`;
            ctx.textBaseline = "top";
            ctx.fillText(text, x, y);
        },

        measure_text(textPtr, _font, fontSize, outWPtr, outHPtr) {
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
            f32[wIdx] = ctx.measureText(text).width;
            f32[hIdx] = fontSize > 0 ? fontSize : 16;
        },

        draw_text_slice(textPtr, len, x, y, _font, fontSize, rgba) {
            if (textPtr === 0 || len === 0) return;
            const mem = new Uint8Array(memory.buffer);
            const text = decoder.decode(mem.subarray(textPtr, textPtr + len));
            if (text.length === 0) return;
            ctx.fillStyle = cssColor(rgba);
            ctx.font = `${fontSize}px sans-serif`;
            ctx.textBaseline = "top";
            ctx.fillText(text, x, y);
        },

        measure_text_slice(textPtr, len, _font, fontSize, outWPtr, outHPtr) {
            const f32 = new Float32Array(memory.buffer);
            const wIdx = outWPtr >> 2;
            const hIdx = outHPtr >> 2;
            if (textPtr === 0 || len === 0) {
                f32[wIdx] = 0;
                f32[hIdx] = fontSize > 0 ? fontSize : 16;
                return;
            }
            const mem = new Uint8Array(memory.buffer);
            const text = decoder.decode(mem.subarray(textPtr, textPtr + len));
            ctx.font = `${fontSize}px sans-serif`;
            f32[wIdx] = ctx.measureText(text).width;
            f32[hIdx] = fontSize > 0 ? fontSize : 16;
        },

        // Batched cumulative advances: one crossing fills the canvas width
        // of every run prefix [0, unit_ends[i]). The prefix string grows by
        // decoding each unit's bytes in place (unit ends are codepoint
        // boundaries, so appends never split a valid scalar); measureText
        // runs on the whole accumulated prefix, so kerning matches fillText
        // of the run. Exact for valid UTF-8; malformed bytes decode one
        // U+FFFD per unit here while a whole-prefix decode may merge a
        // truncated sequence into one - a bounded, garbage-input-only
        // divergence in the documented seam class. Returns the number of
        // advances filled.
        measure_text_advances(textPtr, len, _font, fontSize, unitEndsPtr,
                              unitCount, outPtr) {
            if (textPtr === 0 || len === 0 || unitCount === 0) return 0;
            const mem = new Uint8Array(memory.buffer);
            const ends = new Uint32Array(memory.buffer, unitEndsPtr, unitCount);
            const out = new Float32Array(memory.buffer, outPtr, unitCount);
            ctx.font = `${fontSize}px sans-serif`;
            let prefix = "";
            let prev = 0;
            for (let i = 0; i < unitCount; i++) {
                const e = ends[i];
                if (e > prev && e <= len) {
                    prefix += decoder.decode(mem.subarray(textPtr + prev, textPtr + e));
                    prev = e;
                }
                out[i] = ctx.measureText(prefix).width;
            }
            return unitCount;
        },

        create_texture(rgbaPtr, width, height) {
            if (rgbaPtr === 0 || width <= 0 || height <= 0) return 0;
            const byteCount = width * height * 4;
            const bufLen = memory.buffer.byteLength;
            if (rgbaPtr < 0 || rgbaPtr + byteCount > bufLen) return 0;

            // slice() copies the bytes so the registry never references
            // memory.buffer (which detaches when WASM memory grows).
            const src = new Uint8Array(memory.buffer, rgbaPtr, byteCount);
            const pixels = new Uint8ClampedArray(byteCount);
            pixels.set(src);

            const imgData = new ImageData(pixels, width, height);
            const canvas = createTextureCanvas(width, height);
            const tctx = canvas.getContext("2d");
            if (!tctx) return 0;
            tctx.putImageData(imgData, 0, 0);

            const handle = nextTextureHandle++;
            textures.set(handle, {
                canvas,
                w: width,
                h: height,
                variants: new Map(),
                bypassed: width * height > TINT_LARGE_TEXTURE_PIXEL_THRESHOLD,
            });
            return handle;
        },

        destroy_texture(handle) {
            if (handle === 0) return;
            const entry = textures.get(handle);
            if (!entry) return;
            for (const variant of entry.variants.values()) {
                tintCachePixels -= variant.pixels;
            }
            textures.delete(handle);
        },

        // Non-white RGB tints are fully supported by this backend; do not
        // reintroduce a per-session warning here for non-white tints.
        draw_texture(handle, sx, sy, sw, sh, dx, dy, dw, dh, tint) {
            const entry = textures.get(handle);
            if (!entry) return;
            if (sw <= 0 || sh <= 0 || dw === 0 || dh === 0) return;
            const { r, g, b, a } = unpackColor(tint);
            if (a === 0) return;

            const prevAlpha = ctx.globalAlpha;
            ctx.globalAlpha = a / 255;

            if (r === 255 && g === 255 && b === 255) {
                ctx.drawImage(entry.canvas, sx, sy, sw, sh, dx, dy, dw, dh);
            } else if (entry.bypassed) {
                drawTextureLive(entry, r, g, b, sx, sy, sw, sh, dx, dy, dw, dh);
            } else {
                const variant = ensureTintVariant(entry, r, g, b);
                if (variant) {
                    ctx.drawImage(variant.canvas, sx, sy, sw, sh, dx, dy, dw, dh);
                } else {
                    drawTextureLive(entry, r, g, b, sx, sy, sw, sh, dx, dy, dw, dh);
                }
            }

            ctx.globalAlpha = prevAlpha;
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

        // Open a URL from a NUL-terminated wasm-memory C string in a new tab.
        // The dashboard's reference links route through here on the WASM backend.
        open_url(strPtr) {
            if (strPtr === 0) return;
            const url = readCString(strPtr);
            try {
                window.open(url, "_blank", "noopener");
            } catch (e) {
                console.warn("open_url failed:", e);
            }
        },

        // Clipboard transport (best-effort). clipboard_get_into copies the cached
        // clipboard string into a wasm-side buffer and returns the byte count.
        clipboard_get_into(bufPtr, cap) {
            if (bufPtr === 0 || cap === 0) return 0;
            const mem = new Uint8Array(memory.buffer);
            const bytes = encoder.encode(clipboardCache);
            let n = Math.min(bytes.length, cap);
            // Do not split a UTF-8 codepoint at the cap boundary.
            while (n > 0 && (bytes[n] & 0xC0) === 0x80) n--;
            mem.set(bytes.subarray(0, n), bufPtr);
            return n;
        },

        // clipboard_set updates the cache and fires the async Clipboard API
        // write fire-and-forget (it cannot be awaited mid-frame).
        clipboard_set(textPtr, len) {
            if (textPtr === 0) { clipboardCache = ""; }
            else {
                const mem = new Uint8Array(memory.buffer);
                clipboardCache = decoder.decode(mem.subarray(textPtr, textPtr + len));
            }
            if (navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText(clipboardCache).catch(() => {});
            }
        },

        // Cursor shape (WLX_Cursor_Shape: 0 arrow, 1 I-beam). The core calls
        // this only on change, so the style write is already rate-limited.
        // The enum is append-only and wollix_wasm.h static-asserts its size,
        // so a new shape fails the C build until this map learns it.
        set_cursor(shape) {
            canvas.style.cursor = (shape === 1) ? "text" : "default";
        },
    };

    // ========================================================================
    // "env" module imports — bare-wasm specific
    // ========================================================================
    const envImports = {
        abort() {
            throw new Error("wasm abort");
        },

        puts(strPtr) {
            if (strPtr === 0) return 0;
            console.log(readCString(strPtr));
            return 0;
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

    ctxFilterSupported = probeCtxFilterSupported();

    const queryParams = new URLSearchParams(window.location.search);

    // Debug-only hooks. Enabled by appending `?wlx_debug=1` to the page URL.
    // wlx_debug_force_bypass(handle, on) flips the per-texture bypass flag so
    // a cached-path texture can be rendered via the live-tint path for
    // side-by-side inspection.
    if (queryParams.get("wlx_debug") === "1") {
        window.wlx_debug_force_bypass = (handle, on) => {
            const entry = textures.get(handle);
            if (!entry) return false;
            entry.bypassed = !!on;
            return true;
        };
        console.log(
            "[wollix] debug hooks: wlx_debug_force_bypass(handle, on); " +
            "ctxFilterSupported=" + ctxFilterSupported
        );
    }

    // Validation harness hooks. Enabled by `?wlx_tint_tests=1`. Exposes the
    // production import functions plus minimal read-only accessors so the
    // harness can drive scenarios without re-implementing imports. The harness
    // module is fetched only when the flag is set.
    if (queryParams.get("wlx_tint_tests") === "1") {
        window.__wlx_test_hooks__ = {
            TINT_CACHE_PIXEL_BUDGET,
            TINT_LARGE_TEXTURE_PIXEL_THRESHOLD,

            destroy_texture: wlxImports.destroy_texture,
            draw_texture: wlxImports.draw_texture,

            createTextureFromPixels(pixelBytes, w, h) {
                const imgData = new ImageData(
                    new Uint8ClampedArray(pixelBytes), w, h
                );
                const tcanvas = createTextureCanvas(w, h);
                const tctx = tcanvas.getContext("2d");
                if (!tctx) return 0;
                tctx.putImageData(imgData, 0, 0);
                const handle = nextTextureHandle++;
                textures.set(handle, {
                    canvas: tcanvas,
                    w, h,
                    variants: new Map(),
                    bypassed: w * h > TINT_LARGE_TEXTURE_PIXEL_THRESHOLD,
                });
                return handle;
            },

            getMainCtx() { return ctx; },

            getState() {
                return {
                    ctxFilterSupported,
                    tintCachePixels,
                    tintLruCounter,
                    tintVariantGenerationCount,
                    textureCount: textures.size,
                };
            },

            getTextureEntry(handle) {
                const e = textures.get(handle);
                if (!e) return null;
                const variants = [];
                for (const [key, v] of e.variants) {
                    variants.push({
                        key,
                        lastUsed: v.lastUsed,
                        pixels: v.pixels,
                    });
                }
                return {
                    w: e.w, h: e.h,
                    bypassed: e.bypassed,
                    variantCount: e.variants.size,
                    variants,
                };
            },

            getVariantImageData(handle, key) {
                const e = textures.get(handle);
                if (!e) return null;
                const v = e.variants.get(key);
                if (!v) return null;
                const vctx = v.canvas.getContext("2d");
                if (!vctx) return null;
                return vctx.getImageData(0, 0, e.w, e.h);
            },

            generateVariantFilterDirect(handle, r, g, b) {
                const e = textures.get(handle);
                if (!e) return null;
                return generateVariantFilter(e, r, g, b);
            },

            generateVariantScratchDirect(handle, r, g, b) {
                const e = textures.get(handle);
                if (!e) return null;
                return generateVariantScratch(e, r, g, b);
            },

            evictLruIfNeeded,
            packTintKey,

            clearAllVariants() {
                for (const e of textures.values()) {
                    for (const v of e.variants.values()) {
                        tintCachePixels -= v.pixels;
                    }
                    e.variants.clear();
                }
            },
            setCtxFilterSupported(value) {
                ctxFilterSupported = !!value;
            },
            resetVariantGenerationCount() {
                tintVariantGenerationCount = 0;
            },
            getProbedCtxFilterSupported() {
                return probeCtxFilterSupported();
            },
        };
        import("./wollix_wasm_tint_tests.js")
            .catch(err => console.error("[wollix] tint tests load failed:", err));
    }

    wasmInit();

    // ========================================================================
    // Input event listeners
    // ========================================================================
    // Pointer events cover mouse, pen and touch with one listener set.
    // Capturing the pointer on press keeps move/up arriving while the pointer
    // is outside the canvas, so a drag released off-canvas ends cleanly
    // instead of leaving a button stuck down. touch-action: none stops the
    // browser from panning/zooming the page on touch drags.
    canvas.style.touchAction = "none";

    function updatePointerPos(e) {
        const rect = canvas.getBoundingClientRect();
        input.mouseX = e.clientX - rect.left;
        input.mouseY = e.clientY - rect.top;
    }

    function setPointerButton(button, down) {
        if (button === 0)      input.mouseDown  = down;
        else if (button === 1) input.middleDown = down;
        else if (button === 2) input.rightDown  = down;
    }

    canvas.addEventListener("pointermove", updatePointerPos);

    canvas.addEventListener("pointerdown", (e) => {
        updatePointerPos(e);
        setPointerButton(e.button, true);
        try { canvas.setPointerCapture(e.pointerId); } catch (_) { /* capture unsupported */ }
    });

    canvas.addEventListener("pointerup", (e) => {
        updatePointerPos(e);
        setPointerButton(e.button, false);
    });

    // The browser took the pointer away (touch became a scroll, the window
    // lost the device): release every button so nothing stays latched.
    canvas.addEventListener("pointercancel", () => {
        input.mouseDown = false;
        input.rightDown = false;
        input.middleDown = false;
    });

    canvas.addEventListener("wheel", (e) => {
        e.preventDefault();
        let scale;
        if (e.deltaMode === 1)      scale = 1 / WHEEL_LINES_PER_DETENT;   // lines
        else if (e.deltaMode === 2) scale = 1;                            // pages
        else                        scale = 1 / WHEEL_PIXELS_PER_DETENT;  // pixels
        // DOM: positive deltaY scrolls the page down; the contract's positive
        // wheel is up. Horizontal keeps the same sign family (positive = back
        // toward offset 0).
        input.wheelDelta  += -e.deltaY * scale;
        input.wheelDeltaX += -e.deltaX * scale;
    }, { passive: false });

    canvas.addEventListener("contextmenu", (e) => e.preventDefault());

    function readModifiers(e) {
        let mods = 0;
        if (e.shiftKey) mods |= WLX_MOD.SHIFT;
        if (e.ctrlKey)  mods |= WLX_MOD.CTRL;
        if (e.altKey)   mods |= WLX_MOD.ALT;
        if (e.metaKey)  mods |= WLX_MOD.SUPER;
        return mods;
    }

    // Render one synchronous frame outside the rAF cadence. The copy/cut path
    // uses this so the widget writes the current selection into clipboardCache
    // (and fires the async clipboard write) while the browser is still inside
    // the user gesture, before the native copy/cut event below reads the cache.
    function pumpClipboardFrame() {
        const displayW = canvas.clientWidth;
        const displayH = canvas.clientHeight;
        const savedFrameTime = frameTime;
        frameTime = 0;              // extra paint: advance no animation time
        ctx.clearRect(0, 0, displayW, displayH);
        writeInputToWasm();
        wasmFrame(displayW, displayH);
        frameTime = savedFrameTime;
    }

    document.addEventListener("keydown", (e) => {
        input.modifiers = readModifiers(e);
        const wlxKey = KEY_MAP[e.code];
        if (wlxKey !== undefined) {
            const cmd = e.ctrlKey || e.metaKey;
            // Let the browser handle command-modifier shortcuts (copy/cut/paste/
            // select-all) so the native copy/cut/paste events can carry the
            // system clipboard, and leave F-keys to the browser; still record
            // the key for the widget. Other mapped keys keep their default
            // suppressed (e.g. arrows must not scroll the page).
            if (!cmd && !BROWSER_OWNED_KEYS.has(wlxKey)) e.preventDefault();
            if (e.repeat) {
                input.keysRepeated[wlxKey] = 1;
            } else if (!input.keysDown[wlxKey]) {
                input.keysPressed[wlxKey] = 1;
            }
            input.keysDown[wlxKey] = 1;
            // Copy/cut: run the widget now so clipboardCache holds the current
            // selection before the browser's native copy/cut event fires below.
            if (cmd && (wlxKey === WLX_KEY.C || wlxKey === WLX_KEY.X)) {
                pumpClipboardFrame();
            }
        }
        // Collect text input from printable keys
        if (e.key.length === 1 && !e.ctrlKey && !e.metaKey) {
            input.textInput += e.key;
        }
    });

    document.addEventListener("keyup", (e) => {
        input.modifiers = readModifiers(e);
        const wlxKey = KEY_MAP[e.code];
        if (wlxKey !== undefined) {
            input.keysDown[wlxKey] = 0;
        }
    });

    // Best-effort cross-application paste: a real browser paste gesture refreshes
    // the cache from the system clipboard.
    document.addEventListener("paste", (e) => {
        if (e.clipboardData) clipboardCache = e.clipboardData.getData("text");
    });

    // Serve the browser's native copy/cut from our cache. The keydown above
    // pumped a frame, so clipboardCache already holds the widget's current
    // selection; writing it here happens synchronously inside the user gesture,
    // so it reaches the system clipboard even where the async Clipboard API is
    // gated (e.g. Firefox) or unavailable (non-secure context). This is the
    // authoritative path; clipboard_set's writeText remains a best-effort
    // fallback.
    function serveClipboardCopy(e) {
        if (!clipboardCache || !e.clipboardData) return;
        e.clipboardData.setData("text/plain", clipboardCache);
        e.preventDefault();
    }
    document.addEventListener("copy", serveClipboardCopy);
    document.addEventListener("cut", serveClipboardCopy);

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
        u8[base + INPUT_OFFSETS.mouse_right_down]     = input.rightDown ? 1 : 0;
        u8[base + INPUT_OFFSETS.mouse_right_clicked]  =
            (input.rightDown && !input.prevRightDown) ? 1 : 0;
        u8[base + INPUT_OFFSETS.mouse_middle_down]    = input.middleDown ? 1 : 0;
        u8[base + INPUT_OFFSETS.mouse_middle_clicked] =
            (input.middleDown && !input.prevMiddleDown) ? 1 : 0;

        f32[(base + INPUT_OFFSETS.wheel_delta) >> 2]   = input.wheelDelta;
        f32[(base + INPUT_OFFSETS.wheel_delta_x) >> 2] = input.wheelDeltaX;

        // keys_down, keys_pressed, keys_repeated
        u8.set(input.keysDown, base + INPUT_OFFSETS.keys_down);
        u8.set(input.keysPressed, base + INPUT_OFFSETS.keys_pressed);
        u8.set(input.keysRepeated, base + INPUT_OFFSETS.keys_repeated);

        // modifiers (uint32)
        const u32 = new Uint32Array(memory.buffer);
        u32[(base + INPUT_OFFSETS.modifiers) >> 2] = input.modifiers;

        // text_input (NUL-terminated, max 31 chars)
        const textBytes = encoder.encode(input.textInput);
        const maxLen = 31;
        let len = Math.min(textBytes.length, maxLen);
        // Truncation must never split a UTF-8 sequence: back off any
        // continuation bytes at the cap.
        if (len < textBytes.length) {
            while (len > 0 && (textBytes[len] & 0xC0) === 0x80) len--;
        }
        u8.set(textBytes.subarray(0, len), base + INPUT_OFFSETS.text_input);
        u8[base + INPUT_OFFSETS.text_input + len] = 0;
        // Zero remaining bytes
        for (let i = len + 1; i < 32; i++) {
            u8[base + INPUT_OFFSETS.text_input + i] = 0;
        }

        // Reset per-frame state
        input.prevMouseDown = input.mouseDown;
        input.prevRightDown = input.rightDown;
        input.prevMiddleDown = input.middleDown;
        input.wheelDelta = 0;
        input.wheelDeltaX = 0;
        input.keysPressed.fill(0);
        input.keysRepeated.fill(0);
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
