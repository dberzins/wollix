# Performance Diagnostics

`WLX_PERF` is Wollix's opt-in performance diagnostics channel. When enabled,
the core publishes a `WLX_Perf_Frame` snapshot after each completed frame, and
the backend adapters publish matching backend-local snapshots.

The diagnostics are compile-time opt-in. If `WLX_PERF` is not defined, the
perf API is not emitted and the normal build keeps its default behavior.

## Enabling `WLX_PERF`

Enable the diagnostics either by defining `WLX_PERF` before including
`wollix.h` in the translation unit that owns `WOLLIX_IMPLEMENTATION`, or by
passing `-DWLX_PERF` to the compiler.

```c
#define WLX_PERF
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
```

```bash
clang -DWLX_PERF -I. -o my_app my_app.c ...
```

### Built-in targets

The repository includes ready-made validation targets:

```bash
make perf-test
make gallery_perf
make gallery_sdl3_perf
```

- `make perf-test` builds the full unit test suite with `WLX_PERF` enabled.
- `make gallery_perf` builds the Raylib gallery benchmark.
- `make gallery_sdl3_perf` builds the SDL3 gallery benchmark.

### Timers

Counters work as soon as `WLX_PERF` is enabled. Timing fields require a
monotonic timestamp callback.

- `wlx_context_init_raylib()` installs a Raylib-backed timer automatically.
- `wlx_context_init_sdl3()` installs an SDL3-backed timer automatically.
- `wlx_context_init_wasm()` installs a timer only when
  `WLX_WASM_PERF_TIMESTAMP` is also defined.
- Custom backends can call `wlx_perf_set_timer()` directly.

If no timer is installed, `timer_available` is `false` and timing fields stay
at zero, but command, text, arena, and allocator counters still update.

## Reading snapshots in your app

Call `wlx_perf_get_last_frame()` after `wlx_end()` to read the last completed
core snapshot.

```c
wlx_begin(&ctx, root, wlx_process_raylib_input);
/* build UI */
wlx_end(&ctx);

const WLX_Perf_Frame *perf = wlx_perf_get_last_frame(&ctx);
if (perf && perf->timer_available) {
    printf("frame=%llu total_ms=%.3f dispatch_ms=%.3f text_cmds=%llu\n",
        (unsigned long long)perf->frame_index,
        (double)perf->timings.total_ns / 1000000.0,
        (double)perf->timings.dispatch_ns / 1000000.0,
        (unsigned long long)perf->text.emitted_text_commands);
}
```

Use `wlx_perf_reset()` before a benchmark window if you want to discard warmup
frames and start from a fresh snapshot and counter state.

Command histograms are available programmatically through
`WLX_Perf_Frame.commands.by_type[WLX_CMD_*]`. The gallery CSV exposes the same
data as `cmd_*` columns.

## Running the Gallery Benchmark

The shared gallery is the reference perf consumer for Raylib and SDL3.

### CLI controls

All perf-enabled gallery builds support the same runtime controls:

- `--perf`: enable benchmark capture.
- `--perf-warmup N`: number of warmup frames before measurement.
- `--perf-frames N`: number of measured frames to capture.
- `--perf-width N`: desktop benchmark window width.
- `--perf-height N`: desktop benchmark window height.
- `--perf-theme NAME`: theme name or index.
- `--perf-group NAME`: gallery group name or index.
- `--perf-section NAME`: section name or index.
- `--perf-csv PATH`: write measured frame rows to `PATH`; use `-` for stdout.
- `--perf-help`: print the available perf flags.

The human-readable summary is always printed to stdout after measured frames
complete. CSV output is optional and is written only when `--perf-csv` is set.

### Reproducible comparison protocol

When comparing backends, keep the following fixed across runs:

- warmup frame count
- measured frame count
- viewport size
- theme
- active gallery group
- active gallery section

The first recorded baseline in this repo used:

- `--perf-warmup 120`
- `--perf-frames 600`
- `--perf-width 1280`
- `--perf-height 720`
- `--perf-theme dark`
- `--perf-group Components`
- `--perf-section Button`

### Example commands

Raylib:

```bash
make gallery_perf
./demos/gallery_perf \
  --perf \
  --perf-warmup 120 \
  --perf-frames 600 \
  --perf-width 1280 \
  --perf-height 720 \
  --perf-theme dark \
  --perf-group Components \
  --perf-section Button \
  --perf-csv /tmp/wlx_gallery_raylib_perf.csv \
  > /tmp/wlx_gallery_raylib_summary.txt
```

SDL3:

```bash
make gallery_sdl3_perf
./demos/gallery_sdl3_perf \
  --perf \
  --perf-warmup 120 \
  --perf-frames 600 \
  --perf-width 1280 \
  --perf-height 720 \
  --perf-theme dark \
  --perf-group Components \
  --perf-section Button \
  --perf-csv /tmp/wlx_gallery_sdl3_perf.csv \
  > /tmp/wlx_gallery_sdl3_summary.txt
```

### WASM note

The WASM adapter supports the same `WLX_PERF` counters, but timing fields are
counters-only unless the host also provides `WLX_WASM_PERF_TIMESTAMP`.

## Interpreting the output

The gallery summary is organized into metadata, timing rows, counter rows, and
arena high-water marks. The CSV includes the same metadata plus per-command
histogram columns and raw nanosecond fields.

### Core phase timings

- `core_total`: total Wollix core time for the completed frame.
- `begin`: time spent inside `wlx_begin()`, including frame reset and input
  setup.
- `input`: time spent in the input handler.
- `user_build`: time between the end of input processing and the start of
  `wlx_end()`. This is the cost of your own layout and widget code.
- `wlx_end`: time spent inside `wlx_end()`.
- `range_accum`: time to accumulate command ranges before replay.
- `offset_lookup`: time to resolve command-offset lookup data.
- `dispatch`: time spent replaying the Wollix command buffer.
- `backend_callback`: the slice of dispatch spent inside backend callbacks.

`wlx_end` includes `range_accum`, `offset_lookup`, `dispatch`, and the final
snapshot publication work.

### Command histograms and text counters

- `commands` in the summary is the total recorded command count.
- `WLX_Perf_Frame.commands.by_type[]` and the CSV `cmd_*` columns break that
  total down by `WLX_CMD_*` type.
- `text_measure_calls` tracks how often the core asked the backend to measure
  text.
- `text_commands` tracks how many text draw commands were emitted.
- `text_measured_bytes`, `text_bytes`, and `text_runs` are available in the
  core snapshot and CSV for deeper text-layout analysis.

High text-command counts paired with high text-measure counts usually point to
label-heavy screens or fragmented fitted text runs.

### Backend timing and counters

The backend snapshots use a shared vocabulary where possible:

- `backend_total`: backend-owned timed work excluding present.
- `backend_text`: backend text draw plus text measure time.
- `backend_geometry`: geometry, primitive, and tessellation time.
- `backend_texture`: texture submission or upload time.
- `present`: time spent in the backend present call, if timed.
- `backend_draw_text_calls`, `backend_measure_text_calls`,
  `backend_geometry_submits`, and `backend_texture_draws` count shared backend
  activity.

The Raylib backend also tracks five text cache counters. These appear in the
shared summary and CSV under the `sdl3_text_cache_*` names because the output
format is backend-agnostic: `sdl3_text_cache_lookups`, `sdl3_text_cache_hits`,
`sdl3_text_cache_misses`, `sdl3_text_cache_evictions`, and
`sdl3_text_cache_collision_rejections`. All other `sdl3_*` counters are zero
in Raylib runs.

SDL3 adds more granular counters:

- `sdl3_ttf_get_string_size`
- `sdl3_ttf_render_text_blended`
- `sdl3_create_texture_from_surface`
- `sdl3_destroy_texture`
- `sdl3_destroy_surface`
- `sdl3_render_texture`
- `sdl3_render_geometry`
- `sdl3_render_rect`
- `sdl3_render_line`
- `sdl3_set_clip_rect`
- `sdl3_set_draw_blend_mode`
- `sdl3_set_font_size`
- `sdl3_text_engine_create_attempts`
- `sdl3_text_engine_create_successes`
- `sdl3_text_engine_draw_calls`
- `sdl3_ttf_text_creates`
- `sdl3_ttf_text_destroys`
- `sdl3_text_engine_fallback_draw_calls`
- `sdl3_font_variant_lookups`
- `sdl3_font_variant_hits`
- `sdl3_font_variant_misses`
- `sdl3_font_variant_creates`
- `sdl3_font_variant_evictions`
- `sdl3_font_variant_generation_invalidations`
- `sdl3_font_variant_fallback_resolutions`
- `sdl3_set_font_char_spacing`
- `sdl3_text_cache_lookups`
- `sdl3_text_cache_hits`
- `sdl3_text_cache_misses`
- `sdl3_text_cache_creates`
- `sdl3_text_cache_destroys`
- `sdl3_text_cache_evictions`
- `sdl3_text_cache_collision_rejections`
- `sdl3_text_cache_fallback_resolutions`
- `sdl3_text_cache_heap_allocs`
- `sdl3_text_cache_heap_frees`
- `sdl3_text_cache_color_mutations`
- `sdl3_text_cache_color_skips`
- `sdl3_text_cache_variant_invalidation_evictions`

Interpretation rules of thumb:

- High `backend_text` with high `sdl3_ttf_render_text_blended` and
  `sdl3_create_texture_from_surface` usually means text rasterization and
  transient texture creation dominate the frame.
- High `sdl3_text_engine_draw_calls` with near-zero
  `sdl3_ttf_render_text_blended` and `sdl3_create_texture_from_surface` means
  custom-font drawing is using the SDL_ttf renderer text engine instead of the
  surface-upload fallback.
- For stable custom-font workloads, `sdl3_font_variant_hits` should dominate
  after warmup, `sdl3_font_variant_evictions` should usually stay at zero, and
  `sdl3_set_font_size` should approach zero on cacheable paths. Nonzero
  `sdl3_font_variant_fallback_resolutions` means the backend fell back to the
  shared base font path, where SDL3 custom-font spacing is ignored.
- For stable custom-font workloads, `sdl3_text_cache_hits` should dominate
  after warmup, `sdl3_text_cache_misses` should approach the count of newly
  observed strings, `sdl3_text_cache_evictions` and
  `sdl3_text_cache_collision_rejections` should stay at zero, and
  `sdl3_ttf_get_string_size` should approach zero on cacheable measurement
  paths because measurement returns cached `TTF_GetTextSize` dimensions
  warmed during build phase. `sdl3_text_cache_color_skips` rising relative to
  `sdl3_text_cache_color_mutations` shows the per-draw `TTF_SetTextColor`
  redundancy skip is paying off. Nonzero
  `sdl3_text_cache_fallback_resolutions` means a retained-cache lookup
  declined and the backend fell back to the per-call `TTF_CreateText` path;
  nonzero `sdl3_text_cache_variant_invalidation_evictions` means a font
  variant was evicted while retained entries depended on it. Nonzero
  `sdl3_text_cache_heap_allocs` means at least one cached label exceeded
  `WLX_SDL3_TEXT_INLINE_CAP`. Setting `WLX_SDL3_TEXT_CACHE_CAP` to `0`
  disables the retained cache and reverts to the Stage 1 per-call path.
- High `present` with low `core_total` and low `backend_total` usually points
  to frame pacing or swap/present blocking rather than Wollix replay work.
- Non-zero `arena_grows`, `alloc_calls`, or `realloc_calls` during measured
  frames usually means the benchmark did not reach a stable steady state.

### Arena and allocator data

The summary prints the maximum measured high-water marks for the core arenas.
The CSV also records allocator call and byte counters.

- Stable measured runs should usually show zero allocator churn.
- Arena high-water growth during warmup is expected.
- Arena growth during measured frames is a signal to either increase warmup,
  reduce workload variance, or investigate resizing churn.