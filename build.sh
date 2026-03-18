#!/bin/sh

set -xe

DEMO_DIR=./demos
RAYLIB_INCLUDES="-I. -I$HOME/opt/raylib/include"
RAYLIB_LIBS="-L$HOME/opt/raylib/lib -lraylib -lGL -lm -ldl -lpthread -lrt -lX11"
SDL3_INCLUDES="-I. -I$HOME/opt/sdl3/include"
SDL3_LIBS="-L$HOME/opt/sdl3/lib -Wl,-rpath,$HOME/opt/sdl3/lib -lSDL3 -lm"

mkdir -p "$DEMO_DIR"

for demo_name in \
    layout \
    text \
    button \
    checkbox \
    checkbox_tex \
    input \
    scroll_panel \
    slider \
    demo \
    widget_size \
    variable_slots \
    nest2_panel \
    nested_panel \
    grid \
    grid_auto \
    flex_demo \
    minmax_demo \
    theme_demo \
    font_demo \
    opacity_demo
do
    clang -Wall -Wextra -std=c11 -ggdb -Wno-initializer-overrides \
        $RAYLIB_INCLUDES \
        -o "$DEMO_DIR/$demo_name" "$DEMO_DIR/$demo_name.c" \
        $RAYLIB_LIBS
done

# LD_LIBRARY_PATH=$HOME/opt/sdl3/lib:$LD_LIBRARY_PATH ./demos/sdl3_demo
clang -Wall -Wextra -std=c11 -ggdb -Wno-initializer-overrides \
    $SDL3_INCLUDES \
    -o "$DEMO_DIR/sdl3_demo" "$DEMO_DIR/sdl3_demo.c" \
    $SDL3_LIBS

