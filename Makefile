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
RAYLIB_DEMOS = layout button text checkbox checkbox_tex input scroll_panel slider demo widget_size variable_slots nest2_panel nested_panel grid grid_auto flex_demo minmax_demo theme_demo font_demo opacity_demo border_demo gallery auth
SDL3_DEMOS = sdl3_demo gallery_sdl3
DEMO_NAMES = $(RAYLIB_DEMOS) $(SDL3_DEMOS)
TARGETS = $(addprefix $(DEMO_DIR)/,$(DEMO_NAMES))
DEFAULT_TARGETS = $(addprefix $(DEMO_DIR)/,$(RAYLIB_DEMOS))
WASM_SITE_TARGETS = $(WASM_SITE_DIR)/gallery.wasm $(WASM_SITE_DIR)/index.html $(WASM_SITE_DIR)/wollix_wasm.js

.PHONY: all clean debug release help test test-demos wasm-bare wasm-site $(DEMO_NAMES)

all: $(DEFAULT_TARGETS)

$(DEMO_NAMES): %: $(DEMO_DIR)/%

$(DEMO_DIR):
	mkdir -p $@

$(DEMO_DIR)/%: $(DEMO_DIR)/%.c wollix.h wollix_raylib.h | $(DEMO_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

$(DEMO_DIR)/sdl3_demo: $(DEMO_DIR)/sdl3_demo.c wollix.h wollix_sdl3.h | $(DEMO_DIR)
	$(CC) $(SDL3_CFLAGS) $(SDL3_INCLUDES) -o $@ $< $(SDL3_LDFLAGS) $(SDL3_LIBS)

$(DEMO_DIR)/gallery_sdl3: $(DEMO_DIR)/gallery.c wollix.h wollix_sdl3.h | $(DEMO_DIR)
	$(CC) $(SDL3_CFLAGS) $(SDL3_INCLUDES) -DWLX_GALLERY_SDL3 -o $@ $< $(SDL3_LDFLAGS) $(SDL3_LIBS)

# ── Bare WASM ────────────────────────────────────────────────────────────────
wasm-bare: wasm-site

wasm-site: $(WASM_SITE_TARGETS)

$(WASM_SITE_DIR):
	mkdir -p $@

$(WASM_SITE_DIR)/gallery.wasm: demos/gallery.c $(WASM_SRC_DIR)/wlx_libc_shim.c wollix.h wollix_wasm.h | $(WASM_SITE_DIR)
	$(WASM_CC) $(WASM_BARE_CFLAGS) -DWLX_GALLERY_WASM -o $@ demos/gallery.c $(WASM_SRC_DIR)/wlx_libc_shim.c

$(WASM_SITE_DIR)/index.html: $(WASM_SRC_DIR)/wollix_wasm.html | $(WASM_SITE_DIR)
	cp $< $@

$(WASM_SITE_DIR)/wollix_wasm.js: $(WASM_SRC_DIR)/wollix_wasm.js | $(WASM_SITE_DIR)
	cp $< $@

debug: CFLAGS += -DDEBUG -O0
debug: layout

release: CFLAGS += -DNDEBUG -O3
release: layout

# ── Tests ────────────────────────────────────────────────────────────────────
TEST_DIR = tests
TEST_BIN = $(TEST_DIR)/test_runner

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_DIR)/test_main.c $(wildcard $(TEST_DIR)/*.c) $(wildcard $(TEST_DIR)/*.h) wollix.h
	$(CC) $(BASE_CFLAGS) -I. -o $@ $(TEST_DIR)/test_main.c -lm

test-demos: $(DEFAULT_TARGETS)
	@echo "All demos built successfully."

clean:
	rm -f $(TARGETS) $(TEST_BIN) $(WASM_SRC_DIR)/gallery.wasm $(WASM_SRC_DIR)/index.html
	rm -rf $(WASM_SITE_DIR)

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build all Raylib demos into ./$(DEMO_DIR)/ (default)"
	@echo "  debug        - Build ./$(DEMO_DIR)/layout with debug flags"
	@echo "  release      - Build ./$(DEMO_DIR)/layout with release optimization"
	@echo "  clean        - Remove built demo executables from ./$(DEMO_DIR)/"
	@echo "  <demo-name>  - Build a specific demo into ./$(DEMO_DIR)/<demo-name>"
	@echo "  sdl3_demo    - Build the SDL3 backend demo into ./$(DEMO_DIR)/sdl3_demo"
	@echo "  gallery_sdl3 - Build the SDL3 widget gallery into ./$(DEMO_DIR)/gallery_sdl3"
	@echo "  wasm-site    - Package the bare-wasm gallery demo into ./$(WASM_SITE_DIR)/"
	@echo "  wasm-bare    - Alias for wasm-site"
	@echo "  test         - Build and run the unit test suite"
	@echo "  test-demos   - Build all Raylib demos and verify exit codes"
	@echo "  help         - Show this help message"
