CC = clang
BASE_CFLAGS = -Wall -Wextra -std=c11 -ggdb

ifeq ($(findstring gcc,$(notdir $(CC))),gcc)
BASE_CFLAGS += -Wno-override-init
else
BASE_CFLAGS += -Wno-initializer-overrides
endif

CFLAGS = $(BASE_CFLAGS) -DWLX_DEBUG
INCLUDES = -I. -I$(HOME)/opt/raylib/include
LIBS = -L$(HOME)/opt/raylib/lib -lraylib -lGL -lm -ldl -lpthread -lrt -lX11

SDL3_CFLAGS = $(BASE_CFLAGS)
SDL3_INCLUDES = -I. -I$(HOME)/opt/sdl3/include -I$(HOME)/opt/sdl3_ttf/include
SDL3_LDFLAGS = -L$(HOME)/opt/sdl3/lib -Wl,-rpath,$(HOME)/opt/sdl3/lib -L$(HOME)/opt/sdl3_ttf/lib -Wl,-rpath,$(HOME)/opt/sdl3_ttf/lib
SDL3_LIBS = -lSDL3 -lSDL3_ttf -lm

WASM_CC = clang
WASM_SRC_DIR = web
WASM_SITE_DIR = dist/wasm-demo
WASM_BARE_CFLAGS = --target=wasm32-unknown-unknown -nostdlib -nostdinc -Wno-initializer-overrides\
	-isystem $(WASM_SRC_DIR)/libc -isystem $(shell $(WASM_CC) -print-resource-dir)/include \
    -O2 -DNDEBUG -I. \
    -Wl,--no-entry \
    -Wl,--export=wlx_wasm_init \
    -Wl,--export=wlx_wasm_frame \
    -Wl,--export=wlx_wasm_pool_stats \
    -Wl,--export=wlx_wasm_input_state \
    -Wl,--export=memory \
    -Wl,--allow-undefined

DEMO_DIR = demos
GALLERY_PERF_HEADER = $(DEMO_DIR)/gallery_perf.h
DASHBOARD_DIR = $(DEMO_DIR)/dashboard
DASHBOARD_BIN = $(DASHBOARD_DIR)/dashboard
DASHBOARD_SDL3_BIN = $(DASHBOARD_DIR)/dashboard_sdl3
DASHBOARD_PERF_BIN = $(DASHBOARD_DIR)/dashboard_perf
DASHBOARD_HEADERS = $(wildcard $(DASHBOARD_DIR)/*.h)
# The packaged site (dist/wasm-demo, the GitHub Pages artifact) is the dashboard;
# the gallery keeps building into its own coexisting site dir.
GALLERY_WASM_SITE_DIR = dist/gallery-demo
RAYLIB_DEMOS = layout button button_image label_image text checkbox checkbox_tex image input scroll_panel slider demo widget_size variable_slots nest2_panel nested_panel grid grid_auto flex_demo minmax_demo theme_demo font_demo opacity_demo border_demo disabled_demo gallery auth
SDL3_DEMOS = sdl3_demo gallery_sdl3
PERF_DEMOS = gallery_perf gallery_sdl3_perf
DEMO_NAMES = $(RAYLIB_DEMOS) $(SDL3_DEMOS)
TARGETS = $(addprefix $(DEMO_DIR)/,$(DEMO_NAMES))
PERF_TARGETS = $(addprefix $(DEMO_DIR)/,$(PERF_DEMOS))
DEFAULT_TARGETS = $(addprefix $(DEMO_DIR)/,$(RAYLIB_DEMOS))
WASM_SITE_TARGETS = $(WASM_SITE_DIR)/dashboard.wasm $(WASM_SITE_DIR)/index.html $(WASM_SITE_DIR)/wollix_wasm.js $(WASM_SITE_DIR)/wollix_wasm_tint_tests.js
GALLERY_WASM_SITE_TARGETS = $(GALLERY_WASM_SITE_DIR)/gallery.wasm $(GALLERY_WASM_SITE_DIR)/index.html $(GALLERY_WASM_SITE_DIR)/wollix_wasm.js $(GALLERY_WASM_SITE_DIR)/wollix_wasm_tint_tests.js

.PHONY: all clean debug release help test test-single-pass perf-test test-demos wasm-bare wasm-site gallery-wasm-site dashboard dashboard_sdl3 dashboard_perf dashboard-wasm-site $(DEMO_NAMES) $(PERF_DEMOS)

all: $(DEFAULT_TARGETS)

$(DEMO_NAMES): %: $(DEMO_DIR)/%

$(PERF_DEMOS): %: $(DEMO_DIR)/%

$(DEMO_DIR):
	mkdir -p $@

$(DEMO_DIR)/gallery: $(DEMO_DIR)/gallery.c $(GALLERY_PERF_HEADER) wollix.h wollix_raylib.h | $(DEMO_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

# The dashboard ships with WLX_PERF so the Overview's draw-call / wollix-memory
# metrics are real on every backend; frame time comes from the backend timer.
dashboard: $(DASHBOARD_BIN)

$(DASHBOARD_BIN): $(DASHBOARD_DIR)/dashboard.c $(DASHBOARD_HEADERS) wollix.h wollix_raylib.h | $(DEMO_DIR)
	$(CC) $(CFLAGS) -DWLX_PERF $(INCLUDES) -o $@ $< $(LIBS)

dashboard_sdl3: $(DASHBOARD_SDL3_BIN)

$(DASHBOARD_SDL3_BIN): $(DASHBOARD_DIR)/dashboard.c $(DASHBOARD_HEADERS) wollix.h wollix_sdl3.h | $(DEMO_DIR)
	$(CC) $(SDL3_CFLAGS) $(SDL3_INCLUDES) -DWLX_PERF -DWLX_DASHBOARD_SDL3 -o $@ $< $(SDL3_LDFLAGS) $(SDL3_LIBS)

# Non-debug perf build (no WLX_DEBUG) of the raylib dashboard.
dashboard_perf: $(DASHBOARD_PERF_BIN)

$(DASHBOARD_PERF_BIN): $(DASHBOARD_DIR)/dashboard.c $(DASHBOARD_HEADERS) wollix.h wollix_raylib.h | $(DEMO_DIR)
	$(CC) $(BASE_CFLAGS) -DWLX_PERF $(INCLUDES) -o $@ $< $(LIBS)

$(DEMO_DIR)/%: $(DEMO_DIR)/%.c wollix.h wollix_raylib.h | $(DEMO_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

$(DEMO_DIR)/sdl3_demo: $(DEMO_DIR)/sdl3_demo.c wollix.h wollix_sdl3.h | $(DEMO_DIR)
	$(CC) $(SDL3_CFLAGS) $(SDL3_INCLUDES) -o $@ $< $(SDL3_LDFLAGS) $(SDL3_LIBS)

$(DEMO_DIR)/gallery_sdl3: $(DEMO_DIR)/gallery.c $(GALLERY_PERF_HEADER) wollix.h wollix_sdl3.h | $(DEMO_DIR)
	$(CC) $(SDL3_CFLAGS) $(SDL3_INCLUDES) -DWLX_GALLERY_SDL3 -o $@ $< $(SDL3_LDFLAGS) $(SDL3_LIBS)

$(DEMO_DIR)/gallery_perf: $(DEMO_DIR)/gallery.c $(GALLERY_PERF_HEADER) wollix.h wollix_raylib.h | $(DEMO_DIR)
	$(CC) $(BASE_CFLAGS) -DWLX_PERF $(INCLUDES) -o $@ $< $(LIBS)

$(DEMO_DIR)/gallery_sdl3_perf: $(DEMO_DIR)/gallery.c $(GALLERY_PERF_HEADER) wollix.h wollix_sdl3.h | $(DEMO_DIR)
	$(CC) $(SDL3_CFLAGS) $(SDL3_INCLUDES) -DWLX_PERF -DWLX_GALLERY_SDL3 -o $@ $< $(SDL3_LDFLAGS) $(SDL3_LIBS)

# ── Bare WASM ────────────────────────────────────────────────────────────────
# The packaged site (dist/wasm-demo) is the dashboard -- the canonical
# cross-backend showcase and the GitHub Pages artifact. dashboard-wasm-site is a
# back-compatible alias. The gallery keeps building into its own coexisting site.
wasm-bare: wasm-site

wasm-site: $(WASM_SITE_TARGETS)

dashboard-wasm-site: wasm-site

$(WASM_SITE_DIR):
	mkdir -p $@

$(WASM_SITE_DIR)/dashboard.wasm: $(DASHBOARD_DIR)/dashboard.c $(DASHBOARD_HEADERS) $(WASM_SRC_DIR)/wlx_libc_shim.c $(WASM_SRC_DIR)/wlx_libc_shim.h wollix.h wollix_wasm.h | $(WASM_SITE_DIR)
	$(WASM_CC) $(WASM_BARE_CFLAGS) -DWLX_PERF -DWLX_DASHBOARD_WASM -o $@ $(DASHBOARD_DIR)/dashboard.c $(WASM_SRC_DIR)/wlx_libc_shim.c

$(WASM_SITE_DIR)/index.html: $(WASM_SRC_DIR)/wollix_dashboard.html | $(WASM_SITE_DIR)
	cp $< $@

$(WASM_SITE_DIR)/wollix_wasm.js: $(WASM_SRC_DIR)/wollix_wasm.js | $(WASM_SITE_DIR)
	cp $< $@

$(WASM_SITE_DIR)/wollix_wasm_tint_tests.js: $(WASM_SRC_DIR)/wollix_wasm_tint_tests.js | $(WASM_SITE_DIR)
	cp $< $@

# ── Gallery bare WASM site (coexistence; no longer the public showcase) ────────
gallery-wasm-site: $(GALLERY_WASM_SITE_TARGETS)

$(GALLERY_WASM_SITE_DIR):
	mkdir -p $@

$(GALLERY_WASM_SITE_DIR)/gallery.wasm: $(DEMO_DIR)/gallery.c $(GALLERY_PERF_HEADER) $(WASM_SRC_DIR)/wlx_libc_shim.c $(WASM_SRC_DIR)/wlx_libc_shim.h wollix.h wollix_wasm.h | $(GALLERY_WASM_SITE_DIR)
	$(WASM_CC) $(WASM_BARE_CFLAGS) -DWLX_GALLERY_WASM -o $@ demos/gallery.c $(WASM_SRC_DIR)/wlx_libc_shim.c

$(GALLERY_WASM_SITE_DIR)/index.html: $(WASM_SRC_DIR)/wollix_wasm.html | $(GALLERY_WASM_SITE_DIR)
	cp $< $@

$(GALLERY_WASM_SITE_DIR)/wollix_wasm.js: $(WASM_SRC_DIR)/wollix_wasm.js | $(GALLERY_WASM_SITE_DIR)
	cp $< $@

$(GALLERY_WASM_SITE_DIR)/wollix_wasm_tint_tests.js: $(WASM_SRC_DIR)/wollix_wasm_tint_tests.js | $(GALLERY_WASM_SITE_DIR)
	cp $< $@

debug: CFLAGS += -DDEBUG -O0
debug: layout

release: CFLAGS += -DNDEBUG -O3
release: layout

# ── Tests ────────────────────────────────────────────────────────────────────
TEST_DIR = tests
TEST_BIN = $(TEST_DIR)/test_runner
SINGLE_PASS_BIN = $(TEST_DIR)/test_single_pass
HARD_ASSERT_BIN = $(TEST_DIR)/test_hard_assert

test: $(TEST_BIN) $(SINGLE_PASS_BIN) $(HARD_ASSERT_BIN)
	./$(TEST_BIN)
	./$(SINGLE_PASS_BIN)
	./$(HARD_ASSERT_BIN)

$(TEST_BIN): $(TEST_DIR)/test_main.c $(wildcard $(TEST_DIR)/*.c) $(wildcard $(TEST_DIR)/*.h) $(DASHBOARD_HEADERS) wollix.h
	$(CC) $(BASE_CFLAGS) -I. -o $@ $(TEST_DIR)/test_main.c -lm

# Opt-out path: redistribution disabled via WLX_SLOT_SINGLE_PASS_CLAMP. Built as
# its own translation unit so the default suite keeps redistribution on.
test-single-pass: $(SINGLE_PASS_BIN)
	./$(SINGLE_PASS_BIN)

$(SINGLE_PASS_BIN): $(TEST_DIR)/test_single_pass.c wollix.h
	$(CC) $(BASE_CFLAGS) -DWLX_SLOT_SINGLE_PASS_CLAMP -I. -o $@ $(TEST_DIR)/test_single_pass.c -lm

# Release-build guard coverage: WLX_HARD_ASSERT must fire with NDEBUG defined,
# while plain assert() compiles out. Built as its own translation unit.
$(HARD_ASSERT_BIN): $(TEST_DIR)/test_hard_assert.c $(TEST_DIR)/tests.h $(TEST_DIR)/test_mock_backend.h wollix.h
	$(CC) $(BASE_CFLAGS) -DNDEBUG -I. -o $@ $(TEST_DIR)/test_hard_assert.c -lm

PERF_TEST_BIN = $(TEST_DIR)/test_runner_perf

perf-test: $(PERF_TEST_BIN)
	./$(PERF_TEST_BIN)

$(PERF_TEST_BIN): $(TEST_DIR)/test_main.c $(wildcard $(TEST_DIR)/*.c) $(wildcard $(TEST_DIR)/*.h) $(DASHBOARD_HEADERS) wollix.h
	$(CC) $(BASE_CFLAGS) -DWLX_PERF -I. -o $@ $(TEST_DIR)/test_main.c -lm

test-demos: $(DEFAULT_TARGETS) $(DASHBOARD_BIN)
	@echo "All demos built successfully."

clean:
	rm -f $(TARGETS) $(PERF_TARGETS) $(DASHBOARD_BIN) $(DASHBOARD_SDL3_BIN) $(DASHBOARD_PERF_BIN) $(TEST_BIN) $(SINGLE_PASS_BIN) $(HARD_ASSERT_BIN) $(PERF_TEST_BIN) $(WASM_SRC_DIR)/gallery.wasm $(WASM_SRC_DIR)/index.html
	rm -rf $(WASM_SITE_DIR) $(GALLERY_WASM_SITE_DIR)

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build all Raylib demos into ./$(DEMO_DIR)/ (default)"
	@echo "  debug        - Build ./$(DEMO_DIR)/layout with debug flags"
	@echo "  release      - Build ./$(DEMO_DIR)/layout with release optimization"
	@echo "  clean        - Remove built demo executables from ./$(DEMO_DIR)/"
	@echo "  <demo-name>  - Build a specific demo into ./$(DEMO_DIR)/<demo-name>"
	@echo "  dashboard    - Build the Stitch dashboard showcase (Raylib) into ./$(DASHBOARD_DIR)/dashboard"
	@echo "  dashboard_sdl3 - Build the dashboard showcase (SDL3) into ./$(DASHBOARD_DIR)/dashboard_sdl3"
	@echo "  dashboard_perf - Build the Raylib dashboard with WLX_PERF enabled"
	@echo "  sdl3_demo    - Build the SDL3 backend demo into ./$(DEMO_DIR)/sdl3_demo"
	@echo "  gallery_sdl3 - Build the SDL3 widget gallery into ./$(DEMO_DIR)/gallery_sdl3"
	@echo "  gallery_perf - Build the Raylib gallery with WLX_PERF enabled"
	@echo "  gallery_sdl3_perf - Build the SDL3 gallery with WLX_PERF enabled"
	@echo "  wasm-site    - Package the bare-WASM dashboard showcase into ./$(WASM_SITE_DIR)/ (GitHub Pages artifact)"
	@echo "  dashboard-wasm-site - Alias for wasm-site"
	@echo "  gallery-wasm-site - Package the bare-WASM gallery into ./$(GALLERY_WASM_SITE_DIR)/ (coexisting reference)"
	@echo "  wasm-bare    - Alias for wasm-site"
	@echo "  test         - Build and run the unit test suite"
	@echo "  perf-test    - Build and run tests with WLX_PERF enabled"
	@echo "  test-demos   - Build all Raylib demos and verify exit codes"
	@echo "  help         - Show this help message"
