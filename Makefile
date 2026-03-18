CC = clang
CFLAGS = -Wall -Wextra -std=c11 -ggdb -Wno-initializer-overrides
INCLUDES = -I. -I$(HOME)/opt/raylib/include -I$(HOME)/code/ext/raylib/src/external -I$(HOME)/code/ext/raylib/src
LIBS = -L$(HOME)/opt/raylib/lib -lraylib -lGL -lm -ldl -lpthread -lrt -lX11

SDL3_CFLAGS = -Wall -Wextra -std=c11 -ggdb -Wno-initializer-overrides
SDL3_INCLUDES = -I. -I$(HOME)/opt/sdl3/include
SDL3_LDFLAGS = -L$(HOME)/opt/sdl3/lib -Wl,-rpath,$(HOME)/opt/sdl3/lib
SDL3_LIBS = -lSDL3 -lm

DEMO_DIR = demos
RAYLIB_DEMOS = layout button text checkbox checkbox_tex input scroll_panel slider demo widget_size variable_slots nest2_panel nested_panel grid grid_auto flex_demo minmax_demo theme_demo font_demo opacity_demo
SDL3_DEMOS = sdl3_demo
DEMO_NAMES = $(RAYLIB_DEMOS) $(SDL3_DEMOS)
TARGETS = $(addprefix $(DEMO_DIR)/,$(DEMO_NAMES))
DEFAULT_TARGETS = $(addprefix $(DEMO_DIR)/,$(RAYLIB_DEMOS))

.PHONY: all clean debug release help test test-demos $(DEMO_NAMES)

all: $(DEFAULT_TARGETS)

$(DEMO_NAMES): %: $(DEMO_DIR)/%

$(DEMO_DIR):
	mkdir -p $@

$(DEMO_DIR)/%: $(DEMO_DIR)/%.c wollix.h wollix_raylib.h | $(DEMO_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

$(DEMO_DIR)/sdl3_demo: $(DEMO_DIR)/sdl3_demo.c wollix.h wollix_sdl3.h | $(DEMO_DIR)
	$(CC) $(SDL3_CFLAGS) $(SDL3_INCLUDES) -o $@ $< $(SDL3_LDFLAGS) $(SDL3_LIBS)

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
	$(CC) -Wall -Wextra -Wno-initializer-overrides -std=c11 -ggdb -I. -o $@ $(TEST_DIR)/test_main.c -lm

test-demos: $(DEFAULT_TARGETS)
	@echo "All demos built successfully."

clean:
	rm -f $(TARGETS) $(TEST_BIN)

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build all Raylib demos into ./$(DEMO_DIR)/ (default)"
	@echo "  debug        - Build ./$(DEMO_DIR)/layout with debug flags"
	@echo "  release      - Build ./$(DEMO_DIR)/layout with release optimization"
	@echo "  clean        - Remove built demo executables from ./$(DEMO_DIR)/"
	@echo "  <demo-name>  - Build a specific demo into ./$(DEMO_DIR)/<demo-name>"
	@echo "  sdl3_demo    - Build the SDL3 backend demo into ./$(DEMO_DIR)/sdl3_demo"
	@echo "  test         - Build and run the unit test suite"
	@echo "  test-demos   - Build all Raylib demos and verify exit codes"
	@echo "  help         - Show this help message"
