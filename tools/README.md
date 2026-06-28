# tools/

Manual regeneration tooling for gallery and dashboard assets. Nothing
here runs on the default build, test, or demo paths. Contributors only
invoke these scripts when the underlying asset source changes.

## gen_icon_atlas.sh - icon atlas regenerator

Regenerates `demos/assets/wlx_icons.h` from the 47 committed Lucide
1.14.0 SVGs under `demos/assets/icons/lucide-svg/`. The atlas has two
consumers: the widget gallery (`demos/gallery.c`) and the Stitch
dashboard demo (`demos/dashboard/dashboard.c`, via
`demos/dashboard/dashboard_icons.h`).

The first 12 cover widget-demo icons (`check`, `x`, `info`,
`triangle-alert`, `image`, `palette`, `save`, `rotate-ccw`, `settings`,
`play`, `square`, `square-check`); the next 20 cover gallery chrome and
navigation (`app-window`, `chevron-right`, `house`, `blocks`,
`layout-dashboard`, `route`, `sliders-horizontal`, `type`,
`mouse-pointer-click`, `text-cursor-input`, `component`,
`align-horizontal-space-between`, `grid-3x3`, `stretch-horizontal`,
`layout-template`, `scroll-text`, `layers`, `blend`, `square-dashed`,
`toggle-left`); the final 15 were appended for the dashboard (`search`,
`bell`, `square-minus`, `database`, `square-pen`, `list`, `droplet`,
`contact`, `code-xml`, `zap`, `folder-git-2`, `flask-conical`,
`file-text`, `circle-question-mark`, `circle-user`). New entries are
appended after `toggle-left`, so existing enum indices stay stable.

### Prerequisites

- `rsvg-convert` from `librsvg2-bin` (or equivalent). The script
  refuses to run without it and prints platform-specific install
  hints:

  | Platform         | Command                                |
  | ---------------- | -------------------------------------- |
  | Debian / Ubuntu  | `sudo apt install librsvg2-bin`        |
  | Fedora           | `sudo dnf install librsvg2-tools`      |
  | Arch             | `sudo pacman -S librsvg`               |
  | macOS (Homebrew) | `brew install librsvg`                 |

- A C compiler reachable as `cc` (gcc or clang both work). Any
  environment that builds Wollix already satisfies this.

### Invocation

    bash tools/gen_icon_atlas.sh

Reads from `demos/assets/icons/lucide-svg/`, writes to
`demos/assets/wlx_icons.h`. Scratch files (preprocessed SVGs,
intermediate PNGs, compiled packer binary) are created under
`.cache/icons/` (gitignored).

### What it does

1. Verifies `rsvg-convert` and `cc` are on `PATH`.
2. Copies each selected SVG into a working dir, substituting
   `currentColor` -> `white` so Lucide's stroke renders as a
   white-on-transparent shape. The committed SVGs are not modified.
3. For each tier in `TIERS=(16 24 32 48)` creates a scratch
   subdirectory `.cache/icons/png/tier_<n>/` and rasterises every
   working-copy SVG into it via `rsvg-convert -w <tier> -h <tier>`.
4. Builds `tools/gen_icon_atlas.c` with
   `cc -std=c11 -Wall -Wextra -O2 ... -lm` (`-lm` is required because
   `stb_image.h` references `pow`).
5. Runs the packer, which:
   - decodes each `tier_<n>/<icon>.png` via vendored
     `tools/third_party/stb_image.h`;
   - composes a single 2256x120 RGBA atlas laid out as one row per tier
     in ascending tier order (16 px row at the top, then 24, 32, 48);
   - within each row, every icon cell occupies `tier x tier` pixels at
     `x = icon_index * 48` (the max tier), leaving transparent
     right-padding for smaller tiers so cell-x arithmetic stays
     identical across tiers;
   - rewrites every pixel's RGB to 255 and keeps the rasterizer's
     alpha channel (white-alpha encoding; color comes from the
     gallery's call-site tint);
   - emits `demos/assets/wlx_icons.h` with the banner, atlas
     dimensions (`WLX_ICONS_WIDTH = 2256`, `WLX_ICONS_HEIGHT = 120`),
     `WLX_Icon` enum (47 entries), the tier metadata
     (`WLX_ICON_TIER_COUNT`, `wlx_icon_tier_sizes[]`,
     `wlx_icon_tier_y[]`), the 2D source-rect table
     `wlx_icon_rects_tiered[TIER][ICON]`, a 1D
     `wlx_icon_rects[]` mirror for the 16 px tier (backward
     compatibility), and the 1082880-byte `wlx_icons_rgba[]` array
     (2256 * 4 bytes per source line).

### Determinism

Two runs against the same committed SVG set and the same installed
`rsvg-convert` version produce a byte-identical
`demos/assets/wlx_icons.h`. The generator does not embed wall-clock
timestamps; the banner records the Lucide version and the
`rsvg-convert --version` line instead.

To verify after regeneration:

    bash tools/gen_icon_atlas.sh
    git diff demos/assets/wlx_icons.h

An empty diff confirms the committed header matches what your
environment regenerates. A non-empty diff means either (a) the
selected icon set changed, (b) the upstream SVGs changed, or (c)
the installed `rsvg-convert` produces different anti-aliased edges
than the version that generated the committed header. Cases (a)
and (b) are intentional regenerations; case (c) is local
environment drift and the committed header should not be
overwritten without re-running on a matching environment.

### Adding or removing an icon

1. Drop or remove the source SVG under
   `demos/assets/icons/lucide-svg/`.
2. Update `demos/assets/icons/lucide-atlas.manifest:icon_names` to
   match.
3. Update both `ICONS` (in `tools/gen_icon_atlas.sh`) and the
   `icons[]` table (in `tools/gen_icon_atlas.c`) so they list the
   icons in the same order.
4. Re-run `bash tools/gen_icon_atlas.sh`.
5. Commit the regenerated header alongside the source and tool
   changes.

## third_party/

Vendored single-header dependencies used only by the regeneration
tools. Nothing here is included from `wollix.h`, any backend header,
or any gallery translation unit.

- `stb_image.h` v2.30, public domain (commit
  `2e2bef463a5b53ddf8bb788e25da6b8506314c08`). Used by
  `gen_icon_atlas.c` to decode the per-icon PNG output of
  `rsvg-convert`.
