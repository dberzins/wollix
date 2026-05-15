// Validation harness for the WASM RGB tint variant cache. Loaded only when
// the page URL carries `?wlx_tint_tests=1`. Drives the production WASM-import
// functions and a small set of read-only state accessors exposed by the host
// via `window.__wlx_test_hooks__`. Results are logged to the browser console
// and surfaced in an injected DOM banner.

const hooks = window.__wlx_test_hooks__;
if (!hooks) {
    throw new Error("wollix tint tests: __wlx_test_hooks__ missing on window");
}

const PIXEL_DELTA_TOLERANCE = 2;

function makeBanner() {
    const banner = document.createElement("div");
    banner.style.cssText = [
        "position:fixed", "top:8px", "left:8px", "z-index:99999",
        "max-width:480px", "padding:8px 12px",
        "background:rgba(0,0,0,0.85)", "color:#e0e0e0",
        "font:12px/1.4 monospace", "border-radius:4px",
        "white-space:pre-wrap", "pointer-events:none",
    ].join(";");
    banner.textContent = "wollix tint tests: running...";
    document.body.appendChild(banner);
    return banner;
}

function makeCheckerboardPixels(w, h) {
    const buf = new Uint8Array(w * h * 4);
    for (let y = 0; y < h; y++) {
        for (let x = 0; x < w; x++) {
            const i = (y * w + x) * 4;
            const onCell = ((x >> 2) + (y >> 2)) & 1;
            buf[i + 0] = onCell ? 200 : 40;
            buf[i + 1] = onCell ? 150 : 70;
            buf[i + 2] = onCell ? 100 : 90;
            buf[i + 3] = onCell ? 255 : 128;
        }
    }
    return buf;
}

function maxChannelDelta(a, b) {
    if (a.length !== b.length) return Infinity;
    let m = 0;
    for (let i = 0; i < a.length; i++) {
        const d = Math.abs(a[i] - b[i]);
        if (d > m) m = d;
    }
    return m;
}

function imageDataPixelsEqual(a, b) {
    if (a.width !== b.width || a.height !== b.height) return false;
    return maxChannelDelta(a.data, b.data) === 0;
}

const TESTS = [];

function test(name, fn) {
    TESTS.push({
        name,
        fn: () => {
            // Every test runs against a freshly drained cache so eviction and
            // pixel-count assertions are not racing the gallery's variants.
            hooks.clearAllVariants();
            hooks.resetVariantGenerationCount();
            return fn();
        },
    });
}

test("pixel-comparison filter vs scratch", () => {
    // The direct filter generator silently no-ops on browsers whose Canvas 2D
    // filter implementation accepts the assignment without applying it. The
    // probe is the authoritative signal for filter usability; on a false
    // probe production never calls the filter path, so a parity check between
    // the two generators has nothing to compare and is reported as skipped.
    if (!hooks.getProbedCtxFilterSupported()) {
        return {
            skipped: "browser probe reports ctxFilterSupported=false; " +
                "filter generator is not exercised in this browser"
        };
    }

    const w = 32, h = 32;
    const pixels = makeCheckerboardPixels(w, h);
    const handle = hooks.createTextureFromPixels(pixels, w, h);
    if (handle === 0) throw new Error("createTextureFromPixels failed");

    try {
        const r = 200, g = 100, b = 50;

        const filterVariant = hooks.generateVariantFilterDirect(handle, r, g, b);
        const scratchVariant = hooks.generateVariantScratchDirect(handle, r, g, b);

        if (!filterVariant) {
            throw new Error("filter generator returned null despite probe=true");
        }
        if (!scratchVariant) {
            throw new Error("scratch generator returned null");
        }

        const fctx = filterVariant.canvas.getContext("2d");
        const sctx = scratchVariant.canvas.getContext("2d");
        const fImg = fctx.getImageData(0, 0, w, h);
        const sImg = sctx.getImageData(0, 0, w, h);
        const delta = maxChannelDelta(fImg.data, sImg.data);
        if (delta > PIXEL_DELTA_TOLERANCE) {
            throw new Error(
                `max channel delta ${delta} exceeds tolerance ${PIXEL_DELTA_TOLERANCE}`
            );
        }
        return { delta };
    } finally {
        hooks.destroy_texture(handle);
    }
});

test("cache hit on identical draw", () => {
    const w = 16, h = 16;
    const handle = hooks.createTextureFromPixels(
        makeCheckerboardPixels(w, h), w, h
    );
    try {
        hooks.resetVariantGenerationCount();
        const args = [handle, 0, 0, w, h, 0, 0, w, h, 0xC8643200 | 0xFF];
        hooks.draw_texture(...args);
        const afterFirst = hooks.getState().tintVariantGenerationCount;
        hooks.draw_texture(...args);
        const afterSecond = hooks.getState().tintVariantGenerationCount;
        if (afterFirst !== 1) {
            throw new Error(`expected 1 generation after first draw, got ${afterFirst}`);
        }
        if (afterSecond !== 1) {
            throw new Error(`expected 1 generation after second draw, got ${afterSecond}`);
        }
        return { generations: afterSecond };
    } finally {
        hooks.destroy_texture(handle);
    }
});

test("LRU eviction respects budget", () => {
    const w = 16, h = 16;
    const pixelsPerVariant = w * h;
    const handleA = hooks.createTextureFromPixels(makeCheckerboardPixels(w, h), w, h);
    const handleB = hooks.createTextureFromPixels(makeCheckerboardPixels(w, h), w, h);
    try {
        // Draw A first so it is the LRU entry when eviction runs.
        hooks.draw_texture(handleA, 0, 0, w, h, 0, 0, w, h, 0xC8643200 | 0xFF);
        hooks.draw_texture(handleB, 0, 0, w, h, 0, 0, w, h, 0xC8643200 | 0xFF);

        const entryA = hooks.getTextureEntry(handleA);
        const entryB = hooks.getTextureEntry(handleB);
        if (entryA.variantCount !== 1 || entryB.variantCount !== 1) {
            throw new Error("setup failed: both textures should have one variant");
        }
        const populated = hooks.getState().tintCachePixels;
        if (populated !== 2 * pixelsPerVariant) {
            throw new Error(`expected cache=${2 * pixelsPerVariant}, got ${populated}`);
        }

        // Drive one eviction: tintCachePixels + needed > BUDGET, then after
        // one removal pixels_B + needed = BUDGET so the loop exits.
        hooks.evictLruIfNeeded(hooks.TINT_CACHE_PIXEL_BUDGET - pixelsPerVariant);

        const afterEntryA = hooks.getTextureEntry(handleA);
        const afterEntryB = hooks.getTextureEntry(handleB);
        const after = hooks.getState();

        if (afterEntryA.variantCount !== 0) {
            throw new Error(`expected A evicted, got variantCount=${afterEntryA.variantCount}`);
        }
        if (afterEntryB.variantCount !== 1) {
            throw new Error(`expected B retained, got variantCount=${afterEntryB.variantCount}`);
        }
        if (after.tintCachePixels > hooks.TINT_CACHE_PIXEL_BUDGET) {
            throw new Error(`tintCachePixels=${after.tintCachePixels} exceeds budget`);
        }
        if (after.tintCachePixels !== pixelsPerVariant) {
            throw new Error(
                `tintCachePixels mismatch: expected ${pixelsPerVariant}, got ${after.tintCachePixels}`
            );
        }
        return {
            populated,
            afterCachePixels: after.tintCachePixels,
        };
    } finally {
        hooks.destroy_texture(handleA);
        hooks.destroy_texture(handleB);
    }
});

test("destroy drains cached variant pixels", () => {
    const w = 16, h = 16;
    const startCachePixels = hooks.getState().tintCachePixels;
    const handle = hooks.createTextureFromPixels(
        makeCheckerboardPixels(w, h), w, h
    );
    hooks.draw_texture(handle, 0, 0, w, h, 0, 0, w, h, 0xC8643200 | 0xFF);
    const entry = hooks.getTextureEntry(handle);
    if (entry.variantCount !== 1) {
        hooks.destroy_texture(handle);
        throw new Error("setup failed: variant was not created");
    }
    const populated = hooks.getState().tintCachePixels;
    if (populated !== startCachePixels + w * h) {
        hooks.destroy_texture(handle);
        throw new Error(
            `tintCachePixels did not grow by w*h: start=${startCachePixels}, populated=${populated}`
        );
    }
    hooks.destroy_texture(handle);
    const after = hooks.getState();
    if (hooks.getTextureEntry(handle) !== null) {
        throw new Error("texture entry survived destroy");
    }
    if (after.tintCachePixels !== startCachePixels) {
        throw new Error(
            `tintCachePixels did not return to start: start=${startCachePixels}, after=${after.tintCachePixels}`
        );
    }
    return { startCachePixels, populated, after: after.tintCachePixels };
});

test("large-texture bypass skips the variant cache", () => {
    const w = 1100, h = 1100;
    const pixels = new Uint8Array(w * h * 4).fill(128);
    const handle = hooks.createTextureFromPixels(pixels, w, h);
    if (handle === 0) throw new Error("createTextureFromPixels failed");
    try {
        const entry = hooks.getTextureEntry(handle);
        if (!entry.bypassed) {
            throw new Error("expected bypassed=true for w*h > threshold");
        }
        const startCachePixels = hooks.getState().tintCachePixels;
        hooks.draw_texture(handle, 0, 0, w, h, 0, 0, w, h, 0xC8643200 | 0xFF);
        const after = hooks.getTextureEntry(handle);
        if (after.variantCount !== 0) {
            throw new Error(
                `bypassed texture cached a variant: variantCount=${after.variantCount}`
            );
        }
        if (hooks.getState().tintCachePixels !== startCachePixels) {
            throw new Error("bypass path advanced tintCachePixels");
        }
        return { w, h, bypassed: entry.bypassed };
    } finally {
        hooks.destroy_texture(handle);
    }
});

test("white tint skips variant generation", () => {
    const w = 16, h = 16;
    const handle = hooks.createTextureFromPixels(
        makeCheckerboardPixels(w, h), w, h
    );
    try {
        hooks.resetVariantGenerationCount();
        hooks.draw_texture(handle, 0, 0, w, h, 0, 0, w, h, 0xFFFFFFFF);
        const state = hooks.getState();
        if (state.tintVariantGenerationCount !== 0) {
            throw new Error(
                `white tint advanced counter to ${state.tintVariantGenerationCount}`
            );
        }
        const entry = hooks.getTextureEntry(handle);
        if (entry.variantCount !== 0) {
            throw new Error(`white tint populated a variant: ${entry.variantCount}`);
        }
        return { generations: state.tintVariantGenerationCount };
    } finally {
        hooks.destroy_texture(handle);
    }
});

test("alpha does not change variant pixels", () => {
    const w = 16, h = 16;
    const handle = hooks.createTextureFromPixels(
        makeCheckerboardPixels(w, h), w, h
    );
    try {
        const r = 200, g = 100, b = 50;
        const tintFullAlpha = (r << 24) | (g << 16) | (b << 8) | 0xFF;
        const tintHalfAlpha = (r << 24) | (g << 16) | (b << 8) | 0x80;

        hooks.draw_texture(handle, 0, 0, w, h, 0, 0, w, h, tintFullAlpha >>> 0);
        const key = hooks.packTintKey(r, g, b);
        const pixelsAtFull = hooks.getVariantImageData(handle, key);
        if (!pixelsAtFull) throw new Error("variant missing after full-alpha draw");

        hooks.draw_texture(handle, 0, 0, w, h, 0, 0, w, h, tintHalfAlpha >>> 0);
        const pixelsAtHalf = hooks.getVariantImageData(handle, key);
        if (!pixelsAtHalf) throw new Error("variant missing after half-alpha draw");

        if (!imageDataPixelsEqual(pixelsAtFull, pixelsAtHalf)) {
            throw new Error(
                "variant pixels differ between alpha draws (alpha leaked into cache)"
            );
        }
        return { pixelsCompared: pixelsAtFull.data.length };
    } finally {
        hooks.destroy_texture(handle);
    }
});

test("cross-path parity under forced ctxFilterSupported=false", () => {
    const probed = hooks.getProbedCtxFilterSupported();
    const w = 32, h = 32;
    const handle = hooks.createTextureFromPixels(
        makeCheckerboardPixels(w, h), w, h
    );
    if (handle === 0) throw new Error("createTextureFromPixels failed");

    try {
        const r = 180, g = 90, b = 200;
        const tint = ((r << 24) | (g << 16) | (b << 8) | 0xFF) >>> 0;
        const key = hooks.packTintKey(r, g, b);

        hooks.setCtxFilterSupported(probed);
        hooks.draw_texture(handle, 0, 0, w, h, 0, 0, w, h, tint);
        const firstImg = hooks.getVariantImageData(handle, key);
        if (!firstImg) throw new Error("first variant missing");
        const firstData = new Uint8ClampedArray(firstImg.data);

        hooks.clearAllVariants();
        hooks.setCtxFilterSupported(false);
        hooks.draw_texture(handle, 0, 0, w, h, 0, 0, w, h, tint);
        const secondImg = hooks.getVariantImageData(handle, key);
        if (!secondImg) throw new Error("second variant missing");

        const delta = maxChannelDelta(firstData, secondImg.data);
        if (delta > PIXEL_DELTA_TOLERANCE) {
            throw new Error(
                `cross-path delta ${delta} exceeds tolerance ${PIXEL_DELTA_TOLERANCE}`
            );
        }
        return { probedFilterSupport: probed, delta };
    } finally {
        hooks.setCtxFilterSupported(probed);
        hooks.destroy_texture(handle);
    }
});

async function captureFrameTimings(durationMs) {
    return new Promise(resolve => {
        const deltas = [];
        let last = performance.now();
        const start = last;
        function sample(now) {
            deltas.push(now - last);
            last = now;
            if (now - start < durationMs) {
                requestAnimationFrame(sample);
            } else {
                deltas.sort((a, b) => a - b);
                resolve({
                    samples: deltas.length,
                    medianMs: deltas[Math.floor(deltas.length / 2)],
                    p95Ms: deltas[Math.floor(deltas.length * 0.95)],
                });
            }
        }
        requestAnimationFrame(sample);
    });
}

async function run() {
    const banner = makeBanner();
    const results = [];
    let pass = 0, fail = 0, skip = 0;

    for (const t of TESTS) {
        try {
            const detail = t.fn();
            if (detail && detail.skipped) {
                skip++;
                results.push({ name: t.name, status: "SKIP", detail: detail.skipped });
                console.warn(`[wollix-tint-test] SKIP ${t.name}: ${detail.skipped}`);
            } else {
                pass++;
                results.push({ name: t.name, status: "PASS", detail });
                console.log(`[wollix-tint-test] PASS ${t.name}`, detail || "");
            }
        } catch (err) {
            fail++;
            results.push({ name: t.name, status: "FAIL", error: err.message });
            console.error(`[wollix-tint-test] FAIL ${t.name}: ${err.message}`);
        }
    }

    banner.textContent = `wollix tint tests: ${pass} pass, ${fail} fail, ${skip} skip\n` +
        "capturing frame timings (3s)...";

    const timings = await captureFrameTimings(3000);
    console.log("[wollix-tint-test] frame timings:", timings);

    banner.textContent =
        `wollix tint tests: ${pass} pass, ${fail} fail, ${skip} skip\n` +
        `frame timings (raf, 3s): samples=${timings.samples} ` +
        `median=${timings.medianMs.toFixed(2)}ms p95=${timings.p95Ms.toFixed(2)}ms`;

    window.__wlx_test_results__ = { results, timings, pass, fail, skip };
}

if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", run);
} else {
    run();
}
