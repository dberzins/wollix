// gallery_perf.h - WLX_PERF benchmark instrumentation for the gallery demo.
//
// Included by gallery.c.  Depends on symbols declared earlier in gallery.c:
//   Gallery_State, Group, Section, groups[], theme_names[],
//   GALLERY_BACKEND, GALLERY_VERSION, GALLERY_THEME_COUNT, GROUP_COUNT,
//   gallery_select_group(), gallery_select_section(), gallery_active_section().
//   Backend perf functions come from wollix_raylib.h / wollix_sdl3.h / wollix_wasm.h.

#ifdef WLX_PERF

#ifndef WLX_GALLERY_PERF_DEFAULT
#define WLX_GALLERY_PERF_DEFAULT 0
#endif

#ifndef WLX_GALLERY_PERF_WARMUP_FRAMES
#define WLX_GALLERY_PERF_WARMUP_FRAMES 120
#endif

#ifndef WLX_GALLERY_PERF_MEASURED_FRAMES
#define WLX_GALLERY_PERF_MEASURED_FRAMES 300
#endif

#ifndef WLX_GALLERY_PERF_WIDTH
#define WLX_GALLERY_PERF_WIDTH 1280
#endif

#ifndef WLX_GALLERY_PERF_HEIGHT
#define WLX_GALLERY_PERF_HEIGHT 800
#endif

typedef struct {
    uint64_t frame_index;
    bool timer_available;
    uint64_t draw_text_calls;
    uint64_t measure_text_calls;
    uint64_t draw_rect_calls;
    uint64_t draw_rect_lines_calls;
    uint64_t draw_rect_rounded_calls;
    uint64_t draw_rect_rounded_lines_calls;
    uint64_t draw_circle_calls;
    uint64_t draw_ring_calls;
    uint64_t draw_line_calls;
    uint64_t draw_texture_calls;
    uint64_t begin_scissor_calls;
    uint64_t end_scissor_calls;
    uint64_t geometry_submit_calls;
    uint64_t clip_change_calls;
    uint64_t text_draw_ns;
    uint64_t text_measure_ns;
    uint64_t geometry_ns;
    uint64_t scissor_ns;
    uint64_t texture_ns;
    uint64_t present_ns;
    uint64_t ttf_get_string_size_calls;
    uint64_t ttf_render_text_blended_calls;
    uint64_t create_texture_from_surface_calls;
    uint64_t destroy_texture_calls;
    uint64_t destroy_surface_calls;
    uint64_t render_texture_calls;
    uint64_t render_geometry_calls;
    uint64_t render_rect_calls;
    uint64_t render_line_calls;
    uint64_t set_clip_rect_calls;
    uint64_t set_draw_blend_mode_calls;
    uint64_t set_font_size_calls;
    uint64_t text_engine_create_attempts;
    uint64_t text_engine_create_successes;
    uint64_t text_engine_draw_calls;
    uint64_t ttf_text_creates;
    uint64_t ttf_text_destroys;
    uint64_t text_engine_fallback_draw_calls;
    uint64_t font_variant_lookups;
    uint64_t font_variant_hits;
    uint64_t font_variant_misses;
    uint64_t font_variant_creates;
    uint64_t font_variant_evictions;
    uint64_t font_variant_generation_invalidations;
    uint64_t font_variant_fallback_resolutions;
    uint64_t ttf_set_font_char_spacing_calls;
    uint64_t text_cache_lookups;
    uint64_t text_cache_hits;
    uint64_t text_cache_misses;
    uint64_t text_cache_creates;
    uint64_t text_cache_destroys;
    uint64_t text_cache_evictions;
    uint64_t text_cache_collision_rejections;
    uint64_t text_cache_fallback_resolutions;
    uint64_t text_cache_heap_allocs;
    uint64_t text_cache_heap_frees;
    uint64_t text_cache_color_mutations;
    uint64_t text_cache_color_skips;
    uint64_t text_cache_variant_invalidation_evictions;
} Gallery_Perf_Backend_Stats;

typedef struct {
    WLX_Perf_Frame core;
    Gallery_Perf_Backend_Stats backend;
    WLX_Rect viewport;
    int group_index;
    int section_index;
    int theme_mode;
} Gallery_Perf_Sample;

typedef struct {
    bool enabled;
    bool done;
    bool measurement_started;
    bool current_frame_active;
    bool current_frame_measured;
    int warmup_frames;
    int measured_frames;
    int completed_frames;
    int measured_count;
    int window_width;
    int window_height;
    bool csv_stdout;
    const char *csv_path;
    WLX_Rect current_viewport;
    Gallery_Perf_Sample *samples;
    size_t sample_capacity;
    size_t sample_count;
} Gallery_Perf_Run;

typedef struct {
    double avg_ns;
    uint64_t min_ns;
    uint64_t max_ns;
    uint64_t p95_ns;
} Gallery_Perf_Duration_Stats;

typedef enum {
    GALLERY_PERF_METRIC_CORE_TOTAL,
    GALLERY_PERF_METRIC_BEGIN,
    GALLERY_PERF_METRIC_INPUT,
    GALLERY_PERF_METRIC_USER_BUILD,
    GALLERY_PERF_METRIC_WLX_END,
    GALLERY_PERF_METRIC_RANGE_ACCUM,
    GALLERY_PERF_METRIC_OFFSET_LOOKUP,
    GALLERY_PERF_METRIC_DISPATCH,
    GALLERY_PERF_METRIC_BACKEND_CALLBACK,
    GALLERY_PERF_METRIC_BACKEND_TOTAL,
    GALLERY_PERF_METRIC_BACKEND_TEXT,
    GALLERY_PERF_METRIC_BACKEND_GEOMETRY,
    GALLERY_PERF_METRIC_BACKEND_TEXTURE,
    GALLERY_PERF_METRIC_PRESENT,
} Gallery_Perf_Metric;

typedef enum {
    GALLERY_PERF_COUNTER_COMMANDS,
    GALLERY_PERF_COUNTER_TEXT_MEASURE,
    GALLERY_PERF_COUNTER_TEXT_COMMANDS,
    GALLERY_PERF_COUNTER_BACKEND_DRAW_TEXT,
    GALLERY_PERF_COUNTER_BACKEND_MEASURE_TEXT,
    GALLERY_PERF_COUNTER_BACKEND_GEOMETRY,
    GALLERY_PERF_COUNTER_BACKEND_TEXTURE,
    GALLERY_PERF_COUNTER_SDL3_TTF_RENDER,
    GALLERY_PERF_COUNTER_SDL3_TEXTURE_CREATE,
    GALLERY_PERF_COUNTER_SDL3_TEXT_ENGINE_CREATE_ATTEMPTS,
    GALLERY_PERF_COUNTER_SDL3_TEXT_ENGINE_CREATE_SUCCESSES,
    GALLERY_PERF_COUNTER_SDL3_TEXT_ENGINE_DRAW,
    GALLERY_PERF_COUNTER_SDL3_TTF_TEXT_CREATES,
    GALLERY_PERF_COUNTER_SDL3_TTF_TEXT_DESTROYS,
    GALLERY_PERF_COUNTER_SDL3_TEXT_ENGINE_FALLBACK_DRAW,
    GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_LOOKUPS,
    GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_HITS,
    GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_MISSES,
    GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_CREATES,
    GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_EVICTIONS,
    GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_GENERATION_INVALIDATIONS,
    GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_FALLBACK_RESOLUTIONS,
    GALLERY_PERF_COUNTER_SDL3_SET_FONT_CHAR_SPACING,
    GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_LOOKUPS,
    GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_HITS,
    GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_MISSES,
    GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_CREATES,
    GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_DESTROYS,
    GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_EVICTIONS,
    GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_COLLISION_REJECTIONS,
    GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_FALLBACK_RESOLUTIONS,
    GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_HEAP_ALLOCS,
    GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_HEAP_FREES,
    GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_COLOR_MUTATIONS,
    GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_COLOR_SKIPS,
    GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_VARIANT_INVALIDATION_EVICTIONS,
    GALLERY_PERF_COUNTER_ARENA_GROWS,
    GALLERY_PERF_COUNTER_ALLOC_CALLS,
    GALLERY_PERF_COUNTER_REALLOC_CALLS,
} Gallery_Perf_Counter;

static Gallery_Perf_Run gallery_perf = {
    .enabled = WLX_GALLERY_PERF_DEFAULT != 0,
    .warmup_frames = WLX_GALLERY_PERF_WARMUP_FRAMES,
    .measured_frames = WLX_GALLERY_PERF_MEASURED_FRAMES,
    .window_width = WLX_GALLERY_PERF_WIDTH,
    .window_height = WLX_GALLERY_PERF_HEIGHT,
};

static bool gallery_perf_is_separator(char c) {
    return c == ' ' || c == '-' || c == '_';
}

static char gallery_perf_lower_char(char c) {
    if (c >= 'A' && c <= 'Z') return (char)(c - 'A' + 'a');
    return c;
}

static bool gallery_perf_str_eq(const char *a, const char *b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (*a != *b) return false;
        a++;
        b++;
    }
    return *a == *b;
}

static bool gallery_perf_name_matches(const char *name, const char *query) {
    if (!name || !query) return false;
    while (*name || *query) {
        while (gallery_perf_is_separator(*name)) name++;
        while (gallery_perf_is_separator(*query)) query++;
        if (gallery_perf_lower_char(*name) != gallery_perf_lower_char(*query)) return false;
        if (*name == '\0' || *query == '\0') return *name == *query;
        name++;
        query++;
    }
    return true;
}

static bool gallery_perf_parse_int(const char *text, int min_value, int max_value, int *out) {
    long value = 0;
    bool negative = false;
    if (!text || !out) return false;
    if (*text == '-') {
        negative = true;
        text++;
    }
    if (*text == '\0') return false;
    while (*text) {
        int digit;
        if (*text < '0' || *text > '9') return false;
        digit = *text - '0';
        value = value * 10 + digit;
        if (value > (long)max_value + 1L) return false;
        text++;
    }
    if (negative) value = -value;
    if (value < (long)min_value || value > (long)max_value) return false;
    *out = (int)value;
    return true;
}

static const char *gallery_perf_next_arg(int argc, char **argv, int *index, const char *name) {
    if (*index + 1 >= argc) {
        fprintf(stderr, "%s requires a value\n", name);
        return NULL;
    }
    *index += 1;
    return argv[*index];
}

static int gallery_perf_find_theme(const char *name) {
    int index;
    if (gallery_perf_parse_int(name, 0, GALLERY_THEME_COUNT - 1, &index)) return index;
    for (index = 0; index < GALLERY_THEME_COUNT; index++) {
        if (gallery_perf_name_matches(theme_names[index], name)) return index;
    }
    return -1;
}

static int gallery_perf_find_group(const char *name) {
    int index;
    if (gallery_perf_parse_int(name, 0, GROUP_COUNT - 1, &index)) return index;
    for (index = 0; index < GROUP_COUNT; index++) {
        if (gallery_perf_name_matches(groups[index].name, name)) return index;
    }
    return -1;
}

static int gallery_perf_find_section_in_group(int group_index, const char *name) {
    int index;
    const Group *group;
    if (group_index < 0 || group_index >= GROUP_COUNT) return -1;
    group = &groups[group_index];
    if (gallery_perf_parse_int(name, 0, group->section_count - 1, &index)) return index;
    for (index = 0; index < group->section_count; index++) {
        const Section *section = group->sections[index];
        if (section && gallery_perf_name_matches(section->name, name)) return index;
    }
    return -1;
}

static bool gallery_perf_find_section_any(const char *name, int *out_group, int *out_section) {
    int group_index;
    for (group_index = 0; group_index < GROUP_COUNT; group_index++) {
        int section_index = gallery_perf_find_section_in_group(group_index, name);
        if (section_index >= 0) {
            *out_group = group_index;
            *out_section = section_index;
            return true;
        }
    }
    return false;
}

static const char *gallery_perf_section_name_for(const Gallery_State *st) {
    const Section *section = gallery_active_section(st);
    if (section) return section->name;
    if (!st || st->active_group < 0 || st->active_group >= GROUP_COUNT) return "Unknown";
    return groups[st->active_group].name;
}

static void gallery_perf_print_usage(void) {
    printf("Wollix gallery perf options require a -DWLX_PERF build.\n");
    printf("  --perf                     enable benchmark capture\n");
    printf("  --perf-warmup N            warmup frame count (default %d)\n", WLX_GALLERY_PERF_WARMUP_FRAMES);
    printf("  --perf-frames N            measured frame count (default %d)\n", WLX_GALLERY_PERF_MEASURED_FRAMES);
    printf("  --perf-width N             benchmark window width (desktop backends)\n");
    printf("  --perf-height N            benchmark window height (desktop backends)\n");
    printf("  --perf-theme NAME          dark, light, glass, brand, custom, or index\n");
    printf("  --perf-group NAME          overview, tokens, components, layouts, patterns, theme-lab, or index\n");
    printf("  --perf-section NAME        section name or index within the active group\n");
    printf("  --perf-csv PATH            write measured frame rows after the run; '-' prints CSV to stdout\n");
}

static bool gallery_perf_prepare_samples(void) {
    if (!gallery_perf.enabled) return true;
    if (gallery_perf.measured_frames < 1) gallery_perf.measured_frames = 1;
    if (gallery_perf.warmup_frames < 0) gallery_perf.warmup_frames = 0;
    gallery_perf.sample_capacity = (size_t)gallery_perf.measured_frames;
    gallery_perf.samples = (Gallery_Perf_Sample *)calloc(gallery_perf.sample_capacity, sizeof(Gallery_Perf_Sample));
    if (!gallery_perf.samples) {
        fprintf(stderr, "could not allocate gallery perf sample buffer\n");
        return false;
    }
    return true;
}

static bool gallery_perf_parse_args(int argc, char **argv, Gallery_State *gs) {
    const char *theme_arg = NULL;
    const char *group_arg = NULL;
    const char *section_arg = NULL;
    int i;

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (gallery_perf_str_eq(arg, "--perf")) {
            gallery_perf.enabled = true;
        } else if (gallery_perf_str_eq(arg, "--perf-help") || gallery_perf_str_eq(arg, "--help")) {
            gallery_perf_print_usage();
            return false;
        } else if (gallery_perf_str_eq(arg, "--perf-warmup")) {
            const char *value = gallery_perf_next_arg(argc, argv, &i, arg);
            if (!value || !gallery_perf_parse_int(value, 0, 1000000, &gallery_perf.warmup_frames)) return false;
            gallery_perf.enabled = true;
        } else if (gallery_perf_str_eq(arg, "--perf-frames")) {
            const char *value = gallery_perf_next_arg(argc, argv, &i, arg);
            if (!value || !gallery_perf_parse_int(value, 1, 1000000, &gallery_perf.measured_frames)) return false;
            gallery_perf.enabled = true;
        } else if (gallery_perf_str_eq(arg, "--perf-width")) {
            const char *value = gallery_perf_next_arg(argc, argv, &i, arg);
            if (!value || !gallery_perf_parse_int(value, 320, 16384, &gallery_perf.window_width)) return false;
            gallery_perf.enabled = true;
        } else if (gallery_perf_str_eq(arg, "--perf-height")) {
            const char *value = gallery_perf_next_arg(argc, argv, &i, arg);
            if (!value || !gallery_perf_parse_int(value, 240, 16384, &gallery_perf.window_height)) return false;
            gallery_perf.enabled = true;
        } else if (gallery_perf_str_eq(arg, "--perf-theme")) {
            theme_arg = gallery_perf_next_arg(argc, argv, &i, arg);
            if (!theme_arg) return false;
            gallery_perf.enabled = true;
        } else if (gallery_perf_str_eq(arg, "--perf-group")) {
            group_arg = gallery_perf_next_arg(argc, argv, &i, arg);
            if (!group_arg) return false;
            gallery_perf.enabled = true;
        } else if (gallery_perf_str_eq(arg, "--perf-section")) {
            section_arg = gallery_perf_next_arg(argc, argv, &i, arg);
            if (!section_arg) return false;
            gallery_perf.enabled = true;
        } else if (gallery_perf_str_eq(arg, "--perf-csv")) {
            gallery_perf.csv_path = gallery_perf_next_arg(argc, argv, &i, arg);
            if (!gallery_perf.csv_path) return false;
            gallery_perf.csv_stdout = gallery_perf_str_eq(gallery_perf.csv_path, "-");
            gallery_perf.enabled = true;
        } else {
            fprintf(stderr, "unknown gallery argument: %s\n", arg);
            return false;
        }
    }

    if (theme_arg) {
        int theme = gallery_perf_find_theme(theme_arg);
        if (theme < 0) {
            fprintf(stderr, "unknown perf theme: %s\n", theme_arg);
            return false;
        }
        gs->theme_mode = theme;
    }

    if (group_arg) {
        int group = gallery_perf_find_group(group_arg);
        if (group < 0) {
            fprintf(stderr, "unknown perf group: %s\n", group_arg);
            return false;
        }
        gallery_select_group(gs, group);
    }

    if (section_arg) {
        int section = gallery_perf_find_section_in_group(gs->active_group, section_arg);
        if (section >= 0) {
            gallery_select_section(gs, gs->active_group, section);
        } else {
            int group = -1;
            if (!gallery_perf_find_section_any(section_arg, &group, &section)) {
                fprintf(stderr, "unknown perf section: %s\n", section_arg);
                return false;
            }
            gallery_select_section(gs, group, section);
        }
    }

    return gallery_perf_prepare_samples();
}

static bool gallery_perf_enabled(void) {
    return gallery_perf.enabled;
}

static bool gallery_perf_finished(void) {
    return gallery_perf.done;
}

static int gallery_perf_window_width(int fallback) {
    return gallery_perf.enabled ? gallery_perf.window_width : fallback;
}

static int gallery_perf_window_height(int fallback) {
    return gallery_perf.enabled ? gallery_perf.window_height : fallback;
}

static void gallery_perf_backend_reset(void) {
#if defined(WLX_GALLERY_RAYLIB)
    wlx_perf_raylib_reset();
#elif defined(WLX_GALLERY_SDL3)
    wlx_perf_sdl3_reset();
#elif defined(WLX_GALLERY_WASM)
    wlx_perf_wasm_reset();
#endif
}

static void gallery_perf_backend_begin_frame(void) {
    if (!gallery_perf.current_frame_active) return;
#if defined(WLX_GALLERY_RAYLIB)
    wlx_perf_raylib_begin_frame(0);
#elif defined(WLX_GALLERY_SDL3)
    wlx_perf_sdl3_begin_frame(0);
#elif defined(WLX_GALLERY_WASM)
    wlx_perf_wasm_begin_frame(0);
#endif
}

static void gallery_perf_backend_present_begin(void) {
    if (!gallery_perf.current_frame_active) return;
#if defined(WLX_GALLERY_RAYLIB)
    wlx_perf_raylib_present_begin();
#elif defined(WLX_GALLERY_SDL3)
    wlx_perf_sdl3_present_begin();
#elif defined(WLX_GALLERY_WASM)
    wlx_perf_wasm_present_begin();
#endif
}

static void gallery_perf_backend_present_end(void) {
    if (!gallery_perf.current_frame_active) return;
#if defined(WLX_GALLERY_RAYLIB)
    wlx_perf_raylib_present_end();
#elif defined(WLX_GALLERY_SDL3)
    wlx_perf_sdl3_present_end();
#elif defined(WLX_GALLERY_WASM)
    wlx_perf_wasm_present_end();
#endif
}

static void gallery_perf_backend_end_frame(WLX_Context *ctx) {
    const WLX_Perf_Frame *core;
    uint64_t frame_index;
    if (!gallery_perf.current_frame_active) return;
    core = wlx_perf_get_last_frame(ctx);
    frame_index = core ? core->frame_index : 0;
#if defined(WLX_GALLERY_RAYLIB)
    wlx_perf_raylib_end_frame(frame_index);
#elif defined(WLX_GALLERY_SDL3)
    wlx_perf_sdl3_end_frame(frame_index);
#elif defined(WLX_GALLERY_WASM)
    wlx_perf_wasm_end_frame(frame_index);
#endif
}

static Gallery_Perf_Backend_Stats gallery_perf_backend_stats(void) {
    Gallery_Perf_Backend_Stats out = {0};
#if defined(WLX_GALLERY_RAYLIB)
    const WLX_Perf_Raylib_Frame *frame = wlx_perf_raylib_get_last_frame();
    if (!frame) return out;
    out.frame_index = frame->frame_index;
    out.timer_available = frame->timer_available;
    out.draw_text_calls = frame->draw_text_calls;
    out.measure_text_calls = frame->measure_text_calls;
    out.draw_rect_calls = frame->draw_rect_calls;
    out.draw_rect_lines_calls = frame->draw_rect_lines_calls;
    out.draw_rect_rounded_calls = frame->draw_rect_rounded_calls;
    out.draw_rect_rounded_lines_calls = frame->draw_rect_rounded_lines_calls;
    out.draw_circle_calls = frame->draw_circle_calls;
    out.draw_ring_calls = frame->draw_ring_calls;
    out.draw_line_calls = frame->draw_line_calls;
    out.draw_texture_calls = frame->draw_texture_calls;
    out.begin_scissor_calls = frame->begin_scissor_calls;
    out.end_scissor_calls = frame->end_scissor_calls;
    out.geometry_submit_calls = frame->geometry_submit_calls;
    out.clip_change_calls = frame->clip_change_calls;
    out.text_draw_ns = frame->text_draw_ns;
    out.text_measure_ns = frame->text_measure_ns;
    out.geometry_ns = frame->geometry_ns;
    out.scissor_ns = frame->scissor_ns;
    out.texture_ns = frame->texture_ns;
    out.present_ns = frame->present_ns;
    out.text_cache_lookups = frame->text_cache_lookups;
    out.text_cache_hits = frame->text_cache_hits;
    out.text_cache_misses = frame->text_cache_misses;
    out.text_cache_evictions = frame->text_cache_evictions;
    out.text_cache_collision_rejections = frame->text_cache_collision_rejections;
#elif defined(WLX_GALLERY_SDL3)
    const WLX_Perf_SDL3_Frame *frame = wlx_perf_sdl3_get_last_frame();
    if (!frame) return out;
    out.frame_index = frame->frame_index;
    out.timer_available = frame->timer_available;
    out.draw_text_calls = frame->draw_text_calls;
    out.measure_text_calls = frame->measure_text_calls;
    out.draw_rect_calls = frame->draw_rect_calls;
    out.draw_rect_lines_calls = frame->draw_rect_lines_calls;
    out.draw_rect_rounded_calls = frame->draw_rect_rounded_calls;
    out.draw_rect_rounded_lines_calls = frame->draw_rect_rounded_lines_calls;
    out.draw_circle_calls = frame->draw_circle_calls;
    out.draw_ring_calls = frame->draw_ring_calls;
    out.draw_line_calls = frame->draw_line_calls;
    out.draw_texture_calls = frame->draw_texture_calls;
    out.begin_scissor_calls = frame->begin_scissor_calls;
    out.end_scissor_calls = frame->end_scissor_calls;
    out.geometry_submit_calls = frame->geometry_submit_calls;
    out.clip_change_calls = frame->clip_change_calls;
    out.text_draw_ns = frame->text_draw_ns;
    out.text_measure_ns = frame->text_measure_ns;
    out.geometry_ns = frame->geometry_ns;
    out.scissor_ns = frame->scissor_ns;
    out.texture_ns = frame->texture_ns;
    out.present_ns = frame->present_ns;
    out.ttf_get_string_size_calls = frame->ttf_get_string_size_calls;
    out.ttf_render_text_blended_calls = frame->ttf_render_text_blended_calls;
    out.create_texture_from_surface_calls = frame->create_texture_from_surface_calls;
    out.destroy_texture_calls = frame->destroy_texture_calls;
    out.destroy_surface_calls = frame->destroy_surface_calls;
    out.render_texture_calls = frame->render_texture_calls;
    out.render_geometry_calls = frame->render_geometry_calls;
    out.render_rect_calls = frame->render_rect_calls;
    out.render_line_calls = frame->render_line_calls;
    out.set_clip_rect_calls = frame->set_clip_rect_calls;
    out.set_draw_blend_mode_calls = frame->set_draw_blend_mode_calls;
    out.set_font_size_calls = frame->set_font_size_calls;
    out.text_engine_create_attempts = frame->text_engine_create_attempts;
    out.text_engine_create_successes = frame->text_engine_create_successes;
    out.text_engine_draw_calls = frame->text_engine_draw_calls;
    out.ttf_text_creates = frame->ttf_text_creates;
    out.ttf_text_destroys = frame->ttf_text_destroys;
    out.text_engine_fallback_draw_calls = frame->text_engine_fallback_draw_calls;
    out.font_variant_lookups = frame->font_variant_lookups;
    out.font_variant_hits = frame->font_variant_hits;
    out.font_variant_misses = frame->font_variant_misses;
    out.font_variant_creates = frame->font_variant_creates;
    out.font_variant_evictions = frame->font_variant_evictions;
    out.font_variant_generation_invalidations = frame->font_variant_generation_invalidations;
    out.font_variant_fallback_resolutions = frame->font_variant_fallback_resolutions;
    out.ttf_set_font_char_spacing_calls = frame->ttf_set_font_char_spacing_calls;
    out.text_cache_lookups = frame->text_cache_lookups;
    out.text_cache_hits = frame->text_cache_hits;
    out.text_cache_misses = frame->text_cache_misses;
    out.text_cache_creates = frame->text_cache_creates;
    out.text_cache_destroys = frame->text_cache_destroys;
    out.text_cache_evictions = frame->text_cache_evictions;
    out.text_cache_collision_rejections = frame->text_cache_collision_rejections;
    out.text_cache_fallback_resolutions = frame->text_cache_fallback_resolutions;
    out.text_cache_heap_allocs = frame->text_cache_heap_allocs;
    out.text_cache_heap_frees = frame->text_cache_heap_frees;
    out.text_cache_color_mutations = frame->text_cache_color_mutations;
    out.text_cache_color_skips = frame->text_cache_color_skips;
    out.text_cache_variant_invalidation_evictions = frame->text_cache_variant_invalidation_evictions;
#elif defined(WLX_GALLERY_WASM)
    const WLX_Perf_Wasm_Frame *frame = wlx_perf_wasm_get_last_frame();
    if (!frame) return out;
    out.frame_index = frame->frame_index;
    out.timer_available = frame->timer_available;
    out.draw_text_calls = frame->draw_text_calls;
    out.measure_text_calls = frame->measure_text_calls;
    out.draw_rect_calls = frame->draw_rect_calls;
    out.draw_rect_lines_calls = frame->draw_rect_lines_calls;
    out.draw_rect_rounded_calls = frame->draw_rect_rounded_calls;
    out.draw_rect_rounded_lines_calls = frame->draw_rect_rounded_lines_calls;
    out.draw_circle_calls = frame->draw_circle_calls;
    out.draw_ring_calls = frame->draw_ring_calls;
    out.draw_line_calls = frame->draw_line_calls;
    out.draw_texture_calls = frame->draw_texture_calls;
    out.begin_scissor_calls = frame->begin_scissor_calls;
    out.end_scissor_calls = frame->end_scissor_calls;
    out.geometry_submit_calls = frame->geometry_submit_calls;
    out.clip_change_calls = frame->clip_change_calls;
    out.text_draw_ns = frame->text_draw_ns;
    out.text_measure_ns = frame->text_measure_ns;
    out.geometry_ns = frame->geometry_ns;
    out.scissor_ns = frame->scissor_ns;
    out.texture_ns = frame->texture_ns;
    out.present_ns = frame->present_ns;
#endif
    return out;
}

static uint64_t gallery_perf_backend_total_ns(const Gallery_Perf_Backend_Stats *backend) {
    return backend->text_draw_ns + backend->text_measure_ns + backend->geometry_ns +
        backend->scissor_ns + backend->texture_ns;
}

static uint64_t gallery_perf_metric_value(const Gallery_Perf_Sample *sample, Gallery_Perf_Metric metric) {
    switch (metric) {
    case GALLERY_PERF_METRIC_CORE_TOTAL: return sample->core.timings.total_ns;
    case GALLERY_PERF_METRIC_BEGIN: return sample->core.timings.begin_ns;
    case GALLERY_PERF_METRIC_INPUT: return sample->core.timings.input_ns;
    case GALLERY_PERF_METRIC_USER_BUILD: return sample->core.timings.user_build_ns;
    case GALLERY_PERF_METRIC_WLX_END: return sample->core.timings.end_ns;
    case GALLERY_PERF_METRIC_RANGE_ACCUM: return sample->core.timings.range_accum_ns;
    case GALLERY_PERF_METRIC_OFFSET_LOOKUP: return sample->core.timings.offset_lookup_ns;
    case GALLERY_PERF_METRIC_DISPATCH: return sample->core.timings.dispatch_ns;
    case GALLERY_PERF_METRIC_BACKEND_CALLBACK: return sample->core.timings.backend_callback_ns;
    case GALLERY_PERF_METRIC_BACKEND_TOTAL: return gallery_perf_backend_total_ns(&sample->backend);
    case GALLERY_PERF_METRIC_BACKEND_TEXT: return sample->backend.text_draw_ns + sample->backend.text_measure_ns;
    case GALLERY_PERF_METRIC_BACKEND_GEOMETRY: return sample->backend.geometry_ns;
    case GALLERY_PERF_METRIC_BACKEND_TEXTURE: return sample->backend.texture_ns;
    case GALLERY_PERF_METRIC_PRESENT: return sample->backend.present_ns;
    }
    return 0;
}

static uint64_t gallery_perf_counter_value(const Gallery_Perf_Sample *sample, Gallery_Perf_Counter counter) {
    size_t i;
    switch (counter) {
    case GALLERY_PERF_COUNTER_COMMANDS: return sample->core.commands.total_commands;
    case GALLERY_PERF_COUNTER_TEXT_MEASURE: return sample->core.text.measure_calls;
    case GALLERY_PERF_COUNTER_TEXT_COMMANDS: return sample->core.text.emitted_text_commands;
    case GALLERY_PERF_COUNTER_BACKEND_DRAW_TEXT: return sample->backend.draw_text_calls;
    case GALLERY_PERF_COUNTER_BACKEND_MEASURE_TEXT: return sample->backend.measure_text_calls;
    case GALLERY_PERF_COUNTER_BACKEND_GEOMETRY: return sample->backend.geometry_submit_calls;
    case GALLERY_PERF_COUNTER_BACKEND_TEXTURE: return sample->backend.draw_texture_calls;
    case GALLERY_PERF_COUNTER_SDL3_TTF_RENDER: return sample->backend.ttf_render_text_blended_calls;
    case GALLERY_PERF_COUNTER_SDL3_TEXTURE_CREATE: return sample->backend.create_texture_from_surface_calls;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_ENGINE_CREATE_ATTEMPTS: return sample->backend.text_engine_create_attempts;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_ENGINE_CREATE_SUCCESSES: return sample->backend.text_engine_create_successes;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_ENGINE_DRAW: return sample->backend.text_engine_draw_calls;
    case GALLERY_PERF_COUNTER_SDL3_TTF_TEXT_CREATES: return sample->backend.ttf_text_creates;
    case GALLERY_PERF_COUNTER_SDL3_TTF_TEXT_DESTROYS: return sample->backend.ttf_text_destroys;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_ENGINE_FALLBACK_DRAW: return sample->backend.text_engine_fallback_draw_calls;
    case GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_LOOKUPS: return sample->backend.font_variant_lookups;
    case GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_HITS: return sample->backend.font_variant_hits;
    case GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_MISSES: return sample->backend.font_variant_misses;
    case GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_CREATES: return sample->backend.font_variant_creates;
    case GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_EVICTIONS: return sample->backend.font_variant_evictions;
    case GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_GENERATION_INVALIDATIONS: return sample->backend.font_variant_generation_invalidations;
    case GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_FALLBACK_RESOLUTIONS: return sample->backend.font_variant_fallback_resolutions;
    case GALLERY_PERF_COUNTER_SDL3_SET_FONT_CHAR_SPACING: return sample->backend.ttf_set_font_char_spacing_calls;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_LOOKUPS: return sample->backend.text_cache_lookups;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_HITS: return sample->backend.text_cache_hits;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_MISSES: return sample->backend.text_cache_misses;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_CREATES: return sample->backend.text_cache_creates;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_DESTROYS: return sample->backend.text_cache_destroys;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_EVICTIONS: return sample->backend.text_cache_evictions;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_COLLISION_REJECTIONS: return sample->backend.text_cache_collision_rejections;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_FALLBACK_RESOLUTIONS: return sample->backend.text_cache_fallback_resolutions;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_HEAP_ALLOCS: return sample->backend.text_cache_heap_allocs;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_HEAP_FREES: return sample->backend.text_cache_heap_frees;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_COLOR_MUTATIONS: return sample->backend.text_cache_color_mutations;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_COLOR_SKIPS: return sample->backend.text_cache_color_skips;
    case GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_VARIANT_INVALIDATION_EVICTIONS: return sample->backend.text_cache_variant_invalidation_evictions;
    case GALLERY_PERF_COUNTER_ARENA_GROWS:
        {
            uint64_t grow_count = 0;
            for (i = 0; i < WLX_ARENA_GROUP_COUNT; i++) grow_count += sample->core.arena[i].grow_count;
            return grow_count;
        }
    case GALLERY_PERF_COUNTER_ALLOC_CALLS:
        return sample->core.allocator.alloc_calls + sample->core.allocator.calloc_calls;
    case GALLERY_PERF_COUNTER_REALLOC_CALLS: return sample->core.allocator.realloc_calls;
    }
    return 0;
}

static void gallery_perf_sort_u64(uint64_t *values, size_t count) {
    size_t i;
    for (i = 1; i < count; i++) {
        uint64_t value = values[i];
        size_t j = i;
        while (j > 0 && values[j - 1] > value) {
            values[j] = values[j - 1];
            j--;
        }
        values[j] = value;
    }
}

static Gallery_Perf_Duration_Stats gallery_perf_duration_stats(Gallery_Perf_Metric metric) {
    Gallery_Perf_Duration_Stats stats = {0};
    uint64_t *values;
    uint64_t sum = 0;
    size_t i;
    size_t p95_index;

    if (gallery_perf.sample_count == 0) return stats;
    values = (uint64_t *)malloc(gallery_perf.sample_count * sizeof(uint64_t));
    if (!values) return stats;

    for (i = 0; i < gallery_perf.sample_count; i++) {
        uint64_t value = gallery_perf_metric_value(&gallery_perf.samples[i], metric);
        values[i] = value;
        sum += value;
        if (i == 0 || value < stats.min_ns) stats.min_ns = value;
        if (i == 0 || value > stats.max_ns) stats.max_ns = value;
    }

    gallery_perf_sort_u64(values, gallery_perf.sample_count);
    p95_index = ((gallery_perf.sample_count * 95 + 99) / 100);
    if (p95_index > 0) p95_index--;
    if (p95_index >= gallery_perf.sample_count) p95_index = gallery_perf.sample_count - 1;
    stats.p95_ns = values[p95_index];
    stats.avg_ns = (double)sum / (double)gallery_perf.sample_count;
    free(values);
    return stats;
}

static double gallery_perf_counter_average(Gallery_Perf_Counter counter) {
    double sum = 0.0;
    size_t i;
    if (gallery_perf.sample_count == 0) return 0.0;
    for (i = 0; i < gallery_perf.sample_count; i++) {
        sum += (double)gallery_perf_counter_value(&gallery_perf.samples[i], counter);
    }
    return sum / (double)gallery_perf.sample_count;
}

static size_t gallery_perf_arena_high_water_max(WLX_Arena_Group group) {
    size_t high_water = 0;
    size_t i;
    for (i = 0; i < gallery_perf.sample_count; i++) {
        if (gallery_perf.samples[i].core.arena[group].high_water > high_water) {
            high_water = gallery_perf.samples[i].core.arena[group].high_water;
        }
    }
    return high_water;
}

static void gallery_perf_print_duration_row(const char *name, Gallery_Perf_Metric metric) {
    Gallery_Perf_Duration_Stats stats = gallery_perf_duration_stats(metric);
    printf("%s,%.6f,%.6f,%.6f,%.6f\n",
        name,
        stats.avg_ns / 1000000.0,
        (double)stats.min_ns / 1000000.0,
        (double)stats.max_ns / 1000000.0,
        (double)stats.p95_ns / 1000000.0);
}

static const char *gallery_perf_dominant_metric(void) {
    const struct {
        const char *name;
        Gallery_Perf_Metric metric;
    } candidates[] = {
        { "user_build", GALLERY_PERF_METRIC_USER_BUILD },
        { "wlx_end", GALLERY_PERF_METRIC_WLX_END },
        { "dispatch", GALLERY_PERF_METRIC_DISPATCH },
        { "backend_text", GALLERY_PERF_METRIC_BACKEND_TEXT },
        { "backend_geometry", GALLERY_PERF_METRIC_BACKEND_GEOMETRY },
        { "backend_texture", GALLERY_PERF_METRIC_BACKEND_TEXTURE },
        { "present", GALLERY_PERF_METRIC_PRESENT },
    };
    const char *dominant = "none";
    double dominant_ns = 0.0;
    size_t i;
    for (i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        Gallery_Perf_Duration_Stats stats = gallery_perf_duration_stats(candidates[i].metric);
        if (stats.avg_ns > dominant_ns) {
            dominant_ns = stats.avg_ns;
            dominant = candidates[i].name;
        }
    }
    return dominant;
}

static void gallery_perf_print_summary(const Gallery_State *gs) {
    const char *section_name = gallery_perf_section_name_for(gs);
    WLX_Rect viewport = gallery_perf.sample_count > 0 ? gallery_perf.samples[0].viewport : gallery_perf.current_viewport;

    printf("WLX_PERF_SUMMARY_BEGIN\n");
    printf("backend=%s\n", GALLERY_BACKEND);
    printf("version=%s\n", GALLERY_VERSION);
    printf("warmup_frames=%d\n", gallery_perf.warmup_frames);
    printf("measured_frames=%d\n", gallery_perf.measured_count);
    printf("viewport=%.0fx%.0f\n", viewport.w, viewport.h);
    printf("group=%s\n", groups[gs->active_group].name);
    printf("section=%s\n", section_name);
    printf("theme=%s\n", theme_names[gs->theme_mode]);
    if (gallery_perf.csv_path) printf("csv=%s\n", gallery_perf.csv_path);
    printf("dominant_avg_metric=%s\n", gallery_perf_dominant_metric());
    printf("metric,avg_ms,min_ms,max_ms,p95_ms\n");
    gallery_perf_print_duration_row("core_total", GALLERY_PERF_METRIC_CORE_TOTAL);
    gallery_perf_print_duration_row("begin", GALLERY_PERF_METRIC_BEGIN);
    gallery_perf_print_duration_row("input", GALLERY_PERF_METRIC_INPUT);
    gallery_perf_print_duration_row("user_build", GALLERY_PERF_METRIC_USER_BUILD);
    gallery_perf_print_duration_row("wlx_end", GALLERY_PERF_METRIC_WLX_END);
    gallery_perf_print_duration_row("range_accum", GALLERY_PERF_METRIC_RANGE_ACCUM);
    gallery_perf_print_duration_row("offset_lookup", GALLERY_PERF_METRIC_OFFSET_LOOKUP);
    gallery_perf_print_duration_row("dispatch", GALLERY_PERF_METRIC_DISPATCH);
    gallery_perf_print_duration_row("backend_callback", GALLERY_PERF_METRIC_BACKEND_CALLBACK);
    gallery_perf_print_duration_row("backend_total", GALLERY_PERF_METRIC_BACKEND_TOTAL);
    gallery_perf_print_duration_row("backend_text", GALLERY_PERF_METRIC_BACKEND_TEXT);
    gallery_perf_print_duration_row("backend_geometry", GALLERY_PERF_METRIC_BACKEND_GEOMETRY);
    gallery_perf_print_duration_row("backend_texture", GALLERY_PERF_METRIC_BACKEND_TEXTURE);
    gallery_perf_print_duration_row("present", GALLERY_PERF_METRIC_PRESENT);
    printf("counter,avg_per_frame\n");
    printf("commands,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_COMMANDS));
    printf("text_measure_calls,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_TEXT_MEASURE));
    printf("text_commands,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_TEXT_COMMANDS));
    printf("backend_draw_text_calls,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_BACKEND_DRAW_TEXT));
    printf("backend_measure_text_calls,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_BACKEND_MEASURE_TEXT));
    printf("backend_geometry_submits,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_BACKEND_GEOMETRY));
    printf("backend_texture_draws,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_BACKEND_TEXTURE));
    printf("sdl3_ttf_render_text_blended,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TTF_RENDER));
    printf("sdl3_create_texture_from_surface,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXTURE_CREATE));
    printf("sdl3_text_engine_create_attempts,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_ENGINE_CREATE_ATTEMPTS));
    printf("sdl3_text_engine_create_successes,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_ENGINE_CREATE_SUCCESSES));
    printf("sdl3_text_engine_draw_calls,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_ENGINE_DRAW));
    printf("sdl3_ttf_text_creates,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TTF_TEXT_CREATES));
    printf("sdl3_ttf_text_destroys,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TTF_TEXT_DESTROYS));
    printf("sdl3_text_engine_fallback_draw_calls,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_ENGINE_FALLBACK_DRAW));
    printf("sdl3_font_variant_lookups,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_LOOKUPS));
    printf("sdl3_font_variant_hits,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_HITS));
    printf("sdl3_font_variant_misses,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_MISSES));
    printf("sdl3_font_variant_creates,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_CREATES));
    printf("sdl3_font_variant_evictions,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_EVICTIONS));
    printf("sdl3_font_variant_generation_invalidations,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_GENERATION_INVALIDATIONS));
    printf("sdl3_font_variant_fallback_resolutions,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_FONT_VARIANT_FALLBACK_RESOLUTIONS));
    printf("sdl3_set_font_char_spacing,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_SET_FONT_CHAR_SPACING));
    printf("sdl3_text_cache_lookups,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_LOOKUPS));
    printf("sdl3_text_cache_hits,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_HITS));
    printf("sdl3_text_cache_misses,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_MISSES));
    printf("sdl3_text_cache_creates,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_CREATES));
    printf("sdl3_text_cache_destroys,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_DESTROYS));
    printf("sdl3_text_cache_evictions,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_EVICTIONS));
    printf("sdl3_text_cache_collision_rejections,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_COLLISION_REJECTIONS));
    printf("sdl3_text_cache_fallback_resolutions,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_FALLBACK_RESOLUTIONS));
    printf("sdl3_text_cache_heap_allocs,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_HEAP_ALLOCS));
    printf("sdl3_text_cache_heap_frees,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_HEAP_FREES));
    printf("sdl3_text_cache_color_mutations,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_COLOR_MUTATIONS));
    printf("sdl3_text_cache_color_skips,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_COLOR_SKIPS));
    printf("sdl3_text_cache_variant_invalidation_evictions,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_SDL3_TEXT_CACHE_VARIANT_INVALIDATION_EVICTIONS));
    printf("arena_grows,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_ARENA_GROWS));
    printf("alloc_calls,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_ALLOC_CALLS));
    printf("realloc_calls,%.2f\n", gallery_perf_counter_average(GALLERY_PERF_COUNTER_REALLOC_CALLS));
    printf("arena_high_water,commands=%zu,cmd_ranges=%zu,scratch=%zu,layouts=%zu,slot_offsets=%zu,dyn_offsets=%zu\n",
        gallery_perf_arena_high_water_max(WLX_ARENA_COMMANDS),
        gallery_perf_arena_high_water_max(WLX_ARENA_CMD_RANGES),
        gallery_perf_arena_high_water_max(WLX_ARENA_SCRATCH),
        gallery_perf_arena_high_water_max(WLX_ARENA_LAYOUTS),
        gallery_perf_arena_high_water_max(WLX_ARENA_SLOT_SIZE_OFFSETS),
        gallery_perf_arena_high_water_max(WLX_ARENA_DYN_OFFSETS));
    printf("WLX_PERF_SUMMARY_END\n");
}

#if !defined(WLX_GALLERY_WASM)
static void gallery_perf_write_csv_header(FILE *out) {
    fprintf(out,
        "frame,backend,viewport_w,viewport_h,group,section,theme,timer_available,"
        "core_total_ns,begin_ns,input_ns,user_build_ns,wlx_end_ns,range_accum_ns,offset_lookup_ns,dispatch_ns,backend_callback_ns,"
        "commands_total,command_ranges,cmd_rect,cmd_rect_lines,cmd_rect_rounded,cmd_rect_rounded_lines,cmd_circle,cmd_ring,cmd_line,cmd_text,cmd_texture,cmd_scissor_begin,cmd_scissor_end,"
        "text_measure_calls,text_measured_bytes,text_commands,text_bytes,text_runs,"
        "arena_commands_high_water,arena_cmd_ranges_high_water,arena_scratch_high_water,arena_layouts_high_water,arena_slot_offsets_high_water,arena_dyn_offsets_high_water,arena_grow_count,"
        "alloc_calls,calloc_calls,realloc_calls,free_calls,alloc_bytes,calloc_bytes,realloc_bytes,"
        "backend_frame,timer_backend,backend_draw_text,backend_measure_text,backend_draw_rect,backend_draw_rect_lines,backend_draw_rect_rounded,backend_draw_rect_rounded_lines,backend_draw_circle,backend_draw_ring,backend_draw_line,backend_draw_texture,backend_begin_scissor,backend_end_scissor,backend_geometry_submit,backend_clip_change,"
        "backend_text_draw_ns,backend_text_measure_ns,backend_geometry_ns,backend_scissor_ns,backend_texture_ns,present_ns,"
        "sdl3_ttf_get_string_size,sdl3_ttf_render_text_blended,sdl3_create_texture_from_surface,sdl3_destroy_texture,sdl3_destroy_surface,sdl3_render_texture,sdl3_render_geometry,sdl3_render_rect,sdl3_render_line,sdl3_set_clip_rect,sdl3_set_draw_blend_mode,sdl3_set_font_size,"
        "sdl3_text_engine_create_attempts,sdl3_text_engine_create_successes,sdl3_text_engine_draw_calls,sdl3_ttf_text_creates,sdl3_ttf_text_destroys,sdl3_text_engine_fallback_draw_calls,"
        "sdl3_font_variant_lookups,sdl3_font_variant_hits,sdl3_font_variant_misses,sdl3_font_variant_creates,sdl3_font_variant_evictions,sdl3_font_variant_generation_invalidations,sdl3_font_variant_fallback_resolutions,sdl3_set_font_char_spacing,"
        "sdl3_text_cache_lookups,sdl3_text_cache_hits,sdl3_text_cache_misses,sdl3_text_cache_creates,sdl3_text_cache_destroys,sdl3_text_cache_evictions,sdl3_text_cache_collision_rejections,sdl3_text_cache_fallback_resolutions,sdl3_text_cache_heap_allocs,sdl3_text_cache_heap_frees,sdl3_text_cache_color_mutations,sdl3_text_cache_color_skips,sdl3_text_cache_variant_invalidation_evictions\n");
}

static void gallery_perf_write_csv_row(FILE *out, const Gallery_Perf_Sample *sample) {
    uint64_t arena_grows = 0;
    size_t i;
    for (i = 0; i < WLX_ARENA_GROUP_COUNT; i++) arena_grows += sample->core.arena[i].grow_count;

    fprintf(out,
        "%llu,%s,%.0f,%.0f,%s,%s,%s,%d,"
        "%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,"
        "%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,"
        "%llu,%llu,%llu,%llu,%llu,"
        "%zu,%zu,%zu,%zu,%zu,%zu,%llu,"
        "%llu,%llu,%llu,%llu,%llu,%llu,%llu,"
        "%llu,%d,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,"
        "%llu,%llu,%llu,%llu,%llu,%llu,"
        "%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,"
        "%llu,%llu,%llu,%llu,%llu,%llu,"
        "%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,"
        "%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
        (unsigned long long)sample->core.frame_index,
        GALLERY_BACKEND,
        sample->viewport.w,
        sample->viewport.h,
        groups[sample->group_index].name,
        (sample->section_index >= 0 && sample->section_index < groups[sample->group_index].section_count)
            ? groups[sample->group_index].sections[sample->section_index]->name
            : groups[sample->group_index].name,
        theme_names[sample->theme_mode],
        sample->core.timer_available ? 1 : 0,
        (unsigned long long)sample->core.timings.total_ns,
        (unsigned long long)sample->core.timings.begin_ns,
        (unsigned long long)sample->core.timings.input_ns,
        (unsigned long long)sample->core.timings.user_build_ns,
        (unsigned long long)sample->core.timings.end_ns,
        (unsigned long long)sample->core.timings.range_accum_ns,
        (unsigned long long)sample->core.timings.offset_lookup_ns,
        (unsigned long long)sample->core.timings.dispatch_ns,
        (unsigned long long)sample->core.timings.backend_callback_ns,
        (unsigned long long)sample->core.commands.total_commands,
        (unsigned long long)sample->core.commands.command_ranges,
        (unsigned long long)sample->core.commands.by_type[WLX_CMD_RECT],
        (unsigned long long)sample->core.commands.by_type[WLX_CMD_RECT_LINES],
        (unsigned long long)sample->core.commands.by_type[WLX_CMD_RECT_ROUNDED],
        (unsigned long long)sample->core.commands.by_type[WLX_CMD_RECT_ROUNDED_LINES],
        (unsigned long long)sample->core.commands.by_type[WLX_CMD_CIRCLE],
        (unsigned long long)sample->core.commands.by_type[WLX_CMD_RING],
        (unsigned long long)sample->core.commands.by_type[WLX_CMD_LINE],
        (unsigned long long)sample->core.commands.by_type[WLX_CMD_TEXT],
        (unsigned long long)sample->core.commands.by_type[WLX_CMD_TEXTURE],
        (unsigned long long)sample->core.commands.by_type[WLX_CMD_SCISSOR_BEGIN],
        (unsigned long long)sample->core.commands.by_type[WLX_CMD_SCISSOR_END],
        (unsigned long long)sample->core.text.measure_calls,
        (unsigned long long)sample->core.text.measured_bytes,
        (unsigned long long)sample->core.text.emitted_text_commands,
        (unsigned long long)sample->core.text.emitted_text_bytes,
        (unsigned long long)sample->core.text.fitted_text_runs,
        sample->core.arena[WLX_ARENA_COMMANDS].high_water,
        sample->core.arena[WLX_ARENA_CMD_RANGES].high_water,
        sample->core.arena[WLX_ARENA_SCRATCH].high_water,
        sample->core.arena[WLX_ARENA_LAYOUTS].high_water,
        sample->core.arena[WLX_ARENA_SLOT_SIZE_OFFSETS].high_water,
        sample->core.arena[WLX_ARENA_DYN_OFFSETS].high_water,
        (unsigned long long)arena_grows,
        (unsigned long long)sample->core.allocator.alloc_calls,
        (unsigned long long)sample->core.allocator.calloc_calls,
        (unsigned long long)sample->core.allocator.realloc_calls,
        (unsigned long long)sample->core.allocator.free_calls,
        (unsigned long long)sample->core.allocator.alloc_bytes,
        (unsigned long long)sample->core.allocator.calloc_bytes,
        (unsigned long long)sample->core.allocator.realloc_bytes,
        (unsigned long long)sample->backend.frame_index,
        sample->backend.timer_available ? 1 : 0,
        (unsigned long long)sample->backend.draw_text_calls,
        (unsigned long long)sample->backend.measure_text_calls,
        (unsigned long long)sample->backend.draw_rect_calls,
        (unsigned long long)sample->backend.draw_rect_lines_calls,
        (unsigned long long)sample->backend.draw_rect_rounded_calls,
        (unsigned long long)sample->backend.draw_rect_rounded_lines_calls,
        (unsigned long long)sample->backend.draw_circle_calls,
        (unsigned long long)sample->backend.draw_ring_calls,
        (unsigned long long)sample->backend.draw_line_calls,
        (unsigned long long)sample->backend.draw_texture_calls,
        (unsigned long long)sample->backend.begin_scissor_calls,
        (unsigned long long)sample->backend.end_scissor_calls,
        (unsigned long long)sample->backend.geometry_submit_calls,
        (unsigned long long)sample->backend.clip_change_calls,
        (unsigned long long)sample->backend.text_draw_ns,
        (unsigned long long)sample->backend.text_measure_ns,
        (unsigned long long)sample->backend.geometry_ns,
        (unsigned long long)sample->backend.scissor_ns,
        (unsigned long long)sample->backend.texture_ns,
        (unsigned long long)sample->backend.present_ns,
        (unsigned long long)sample->backend.ttf_get_string_size_calls,
        (unsigned long long)sample->backend.ttf_render_text_blended_calls,
        (unsigned long long)sample->backend.create_texture_from_surface_calls,
        (unsigned long long)sample->backend.destroy_texture_calls,
        (unsigned long long)sample->backend.destroy_surface_calls,
        (unsigned long long)sample->backend.render_texture_calls,
        (unsigned long long)sample->backend.render_geometry_calls,
        (unsigned long long)sample->backend.render_rect_calls,
        (unsigned long long)sample->backend.render_line_calls,
        (unsigned long long)sample->backend.set_clip_rect_calls,
        (unsigned long long)sample->backend.set_draw_blend_mode_calls,
        (unsigned long long)sample->backend.set_font_size_calls,
        (unsigned long long)sample->backend.text_engine_create_attempts,
        (unsigned long long)sample->backend.text_engine_create_successes,
        (unsigned long long)sample->backend.text_engine_draw_calls,
        (unsigned long long)sample->backend.ttf_text_creates,
        (unsigned long long)sample->backend.ttf_text_destroys,
        (unsigned long long)sample->backend.text_engine_fallback_draw_calls,
        (unsigned long long)sample->backend.font_variant_lookups,
        (unsigned long long)sample->backend.font_variant_hits,
        (unsigned long long)sample->backend.font_variant_misses,
        (unsigned long long)sample->backend.font_variant_creates,
        (unsigned long long)sample->backend.font_variant_evictions,
        (unsigned long long)sample->backend.font_variant_generation_invalidations,
        (unsigned long long)sample->backend.font_variant_fallback_resolutions,
        (unsigned long long)sample->backend.ttf_set_font_char_spacing_calls,
        (unsigned long long)sample->backend.text_cache_lookups,
        (unsigned long long)sample->backend.text_cache_hits,
        (unsigned long long)sample->backend.text_cache_misses,
        (unsigned long long)sample->backend.text_cache_creates,
        (unsigned long long)sample->backend.text_cache_destroys,
        (unsigned long long)sample->backend.text_cache_evictions,
        (unsigned long long)sample->backend.text_cache_collision_rejections,
        (unsigned long long)sample->backend.text_cache_fallback_resolutions,
        (unsigned long long)sample->backend.text_cache_heap_allocs,
        (unsigned long long)sample->backend.text_cache_heap_frees,
        (unsigned long long)sample->backend.text_cache_color_mutations,
        (unsigned long long)sample->backend.text_cache_color_skips,
        (unsigned long long)sample->backend.text_cache_variant_invalidation_evictions);
}

static void gallery_perf_write_csv(void) {
    FILE *out;
    size_t i;
    if (!gallery_perf.csv_path) return;
    out = gallery_perf.csv_stdout ? stdout : fopen(gallery_perf.csv_path, "w");
    if (!out) {
        fprintf(stderr, "could not open perf csv path: %s\n", gallery_perf.csv_path);
        return;
    }
    gallery_perf_write_csv_header(out);
    for (i = 0; i < gallery_perf.sample_count; i++) gallery_perf_write_csv_row(out, &gallery_perf.samples[i]);
    if (!gallery_perf.csv_stdout) fclose(out);
}
#else
static void gallery_perf_write_csv(void) {
}
#endif

static void gallery_perf_report(const Gallery_State *gs) {
    gallery_perf_print_summary(gs);
    gallery_perf_write_csv();
}

static void gallery_perf_before_frame(WLX_Context *ctx, WLX_Rect viewport) {
    if (!gallery_perf.enabled || gallery_perf.done) {
        gallery_perf.current_frame_active = false;
        gallery_perf.current_frame_measured = false;
        return;
    }

    if (!gallery_perf.measurement_started && gallery_perf.completed_frames >= gallery_perf.warmup_frames) {
        wlx_perf_reset(ctx);
        gallery_perf_backend_reset();
        gallery_perf.measurement_started = true;
    }

    gallery_perf.current_viewport = viewport;
    gallery_perf.current_frame_active = gallery_perf.completed_frames < gallery_perf.warmup_frames ||
        gallery_perf.measured_count < gallery_perf.measured_frames;
    gallery_perf.current_frame_measured = gallery_perf.measurement_started &&
        gallery_perf.measured_count < gallery_perf.measured_frames;
    gallery_perf_backend_begin_frame();
}

static void gallery_perf_after_frame(WLX_Context *ctx, Gallery_State *gs) {
    if (!gallery_perf.current_frame_active) return;

    if (gallery_perf.current_frame_measured && gallery_perf.sample_count < gallery_perf.sample_capacity) {
        const WLX_Perf_Frame *core = wlx_perf_get_last_frame(ctx);
        Gallery_Perf_Sample *sample = &gallery_perf.samples[gallery_perf.sample_count++];
        if (core) sample->core = *core;
        sample->backend = gallery_perf_backend_stats();
        sample->viewport = gallery_perf.current_viewport;
        sample->group_index = gs->active_group;
        sample->section_index = gs->active_section_in_group;
        sample->theme_mode = gs->theme_mode;
        gallery_perf.measured_count++;
    }

    gallery_perf.completed_frames++;
    gallery_perf.current_frame_active = false;
    gallery_perf.current_frame_measured = false;

    if (gallery_perf.measured_count >= gallery_perf.measured_frames) {
        gallery_perf_report(gs);
        gallery_perf.done = true;
    }
}

static void gallery_perf_shutdown(void) {
    free(gallery_perf.samples);
    gallery_perf.samples = NULL;
    gallery_perf.sample_capacity = 0;
    gallery_perf.sample_count = 0;
}

#else

static bool gallery_perf_str_eq(const char *a, const char *b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (*a != *b) return false;
        a++;
        b++;
    }
    return *a == *b;
}

static bool gallery_perf_str_starts_with(const char *text, const char *prefix) {
    if (!text || !prefix) return false;
    while (*prefix) {
        if (*text != *prefix) return false;
        text++;
        prefix++;
    }
    return true;
}

static bool gallery_perf_parse_args(int argc, char **argv, Gallery_State *gs) {
    int i;
    WLX_UNUSED(gs);
    for (i = 1; i < argc; i++) {
        if (gallery_perf_str_starts_with(argv[i], "--perf")) {
            fprintf(stderr, "gallery perf controls require rebuilding with -DWLX_PERF\n");
            return false;
        }
        if (gallery_perf_str_eq(argv[i], "--help")) {
            printf("Wollix gallery has no runtime options in this build. Rebuild with -DWLX_PERF for --perf controls.\n");
            return false;
        }
        fprintf(stderr, "unknown gallery argument: %s\n", argv[i]);
        return false;
    }
    return true;
}

static bool gallery_perf_enabled(void) { return false; }
static bool gallery_perf_finished(void) { return false; }
static int gallery_perf_window_width(int fallback) { return fallback; }
static int gallery_perf_window_height(int fallback) { return fallback; }
static void gallery_perf_before_frame(WLX_Context *ctx, WLX_Rect viewport) { WLX_UNUSED(ctx); WLX_UNUSED(viewport); }
static void gallery_perf_backend_present_begin(void) {}
static void gallery_perf_backend_present_end(void) {}
static void gallery_perf_backend_end_frame(WLX_Context *ctx) { WLX_UNUSED(ctx); }
static void gallery_perf_after_frame(WLX_Context *ctx, Gallery_State *gs) { WLX_UNUSED(ctx); WLX_UNUSED(gs); }
static void gallery_perf_shutdown(void) {}

#endif
