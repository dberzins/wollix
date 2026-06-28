#!/usr/bin/env bash
# Regenerate demos/assets/wlx_icons.h from the committed Lucide SVG set.
# Manual invocation only. Not invoked by make, make test, or make test-demos.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT}/demos/assets/icons/lucide-svg"
LICENSE_REL="demos/assets/icons/LUCIDE_LICENSE"
OUTPUT="${ROOT}/demos/assets/wlx_icons.h"
TOOLS_DIR="${ROOT}/tools"
SCRATCH="${ROOT}/.cache/icons"

LUCIDE_VERSION="1.14.0"

# Selected icons in enum order. Must stay in sync with
# demos/assets/icons/lucide-atlas.manifest:icon_names and the icons[]
# table in tools/gen_icon_atlas.c.
ICONS=(
    check
    x
    info
    triangle-alert
    image
    palette
    save
    rotate-ccw
    settings
    play
    square
    square-check
    app-window
    chevron-right
    house
    blocks
    layout-dashboard
    route
    sliders-horizontal
    type
    mouse-pointer-click
    text-cursor-input
    component
    align-horizontal-space-between
    grid-3x3
    stretch-horizontal
    layout-template
    scroll-text
    layers
    blend
    square-dashed
    toggle-left
    search
    bell
    square-minus
    database
    square-pen
    list
    droplet
    contact
    code-xml
    zap
    folder-git-2
    flask-conical
    file-text
    circle-question-mark
    circle-user
)

# Atlas tier sizes (px). Must stay in sync with the TIERS[] table in
# tools/gen_icon_atlas.c and target_icon_size_px in lucide-atlas.manifest.
TIERS=(16 24 32 48)

if ! command -v rsvg-convert >/dev/null 2>&1; then
    cat >&2 <<EOF
error: rsvg-convert not found on PATH
install hint:
  Debian/Ubuntu   : sudo apt install librsvg2-bin
  Fedora          : sudo dnf install librsvg2-tools
  Arch            : sudo pacman -S librsvg
  macOS (Homebrew): brew install librsvg
EOF
    exit 1
fi

if ! command -v cc >/dev/null 2>&1; then
    echo "error: C compiler 'cc' not found on PATH" >&2
    exit 1
fi

if [ ! -d "${SRC_DIR}" ]; then
    echo "error: ${SRC_DIR} does not exist" >&2
    echo "       The selected Lucide SVGs must be vendored there." >&2
    exit 1
fi

RSVG_VERSION_LINE="$(rsvg-convert --version 2>&1 | head -1)"

rm -rf "${SCRATCH}"
mkdir -p "${SCRATCH}/svg" "${SCRATCH}/png"

# Substitute currentColor -> white on a working copy of each icon. The
# committed SVGs under ${SRC_DIR} are never modified in place.
for name in "${ICONS[@]}"; do
    src="${SRC_DIR}/${name}.svg"
    if [ ! -f "${src}" ]; then
        echo "error: missing source SVG ${src}" >&2
        exit 1
    fi
    sed 's/currentColor/white/g' "${src}" > "${SCRATCH}/svg/${name}.svg"
done

# Rasterize each working-copy SVG to a tier_px*tier_px RGBA PNG per tier.
for tier in "${TIERS[@]}"; do
    mkdir -p "${SCRATCH}/png/tier_${tier}"
    for name in "${ICONS[@]}"; do
        rsvg-convert -w "${tier}" -h "${tier}" \
            -o "${SCRATCH}/png/tier_${tier}/${name}.png" \
            "${SCRATCH}/svg/${name}.svg" >/dev/null
    done
done

# Build the packer.
cc -std=c11 -Wall -Wextra -O2 \
    "${TOOLS_DIR}/gen_icon_atlas.c" \
    -o "${SCRATCH}/gen_icon_atlas" \
    -lm

# Pack and emit the header.
"${SCRATCH}/gen_icon_atlas" \
    --png-dir "${SCRATCH}/png" \
    --output "${OUTPUT}" \
    --lucide-version "${LUCIDE_VERSION}" \
    --rsvg-version "${RSVG_VERSION_LINE}" \
    --license-path "${LICENSE_REL}"

echo "generated: ${OUTPUT}"
