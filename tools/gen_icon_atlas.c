// Atlas packer and header emitter for the gallery icon set.
// Manual regeneration tool. Driven by tools/gen_icon_atlas.sh; not on
// any normal build path.

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;       // file name stem (no extension)
    const char *enum_id;    // identifier suffix (prefixed with WLX_ICON_)
} IconEntry;

// Icon list in enum order. Must stay in sync with
// demos/assets/icons/lucide-atlas.manifest:icon_names and the
// ICONS array in tools/gen_icon_atlas.sh.
static const IconEntry icons[] = {
    {"check",                          "CHECK"                         },
    {"x",                              "X"                             },
    {"info",                           "INFO"                          },
    {"triangle-alert",                 "TRIANGLE_ALERT"                },
    {"image",                          "IMAGE"                         },
    {"palette",                        "PALETTE"                       },
    {"save",                           "SAVE"                          },
    {"rotate-ccw",                     "ROTATE_CCW"                    },
    {"settings",                       "SETTINGS"                      },
    {"play",                           "PLAY"                          },
    {"square",                         "SQUARE"                        },
    {"square-check",                   "SQUARE_CHECK"                  },
    {"app-window",                     "APP_WINDOW"                    },
    {"chevron-right",                  "CHEVRON_RIGHT"                 },
    {"house",                          "HOUSE"                         },
    {"blocks",                         "BLOCKS"                        },
    {"layout-dashboard",               "LAYOUT_DASHBOARD"              },
    {"route",                          "ROUTE"                         },
    {"sliders-horizontal",             "SLIDERS_HORIZONTAL"            },
    {"type",                           "TYPE"                          },
    {"mouse-pointer-click",            "MOUSE_POINTER_CLICK"           },
    {"text-cursor-input",              "TEXT_CURSOR_INPUT"             },
    {"component",                      "COMPONENT"                     },
    {"align-horizontal-space-between", "ALIGN_HORIZONTAL_SPACE_BETWEEN"},
    {"grid-3x3",                       "GRID_3X3"                      },
    {"stretch-horizontal",             "STRETCH_HORIZONTAL"            },
    {"layout-template",                "LAYOUT_TEMPLATE"               },
    {"scroll-text",                    "SCROLL_TEXT"                   },
    {"layers",                         "LAYERS"                        },
    {"blend",                          "BLEND"                         },
    {"square-dashed",                  "SQUARE_DASHED"                 },
    {"toggle-left",                    "TOGGLE_LEFT"                   },
};

#define ICON_COUNT ((int)(sizeof(icons) / sizeof(icons[0])))

// Tier sizes in ascending order. Cell stride within a row is MAX_TIER so
// the per-icon x coordinate is constant across tiers. Right-padding
// columns (when tier_px < MAX_TIER) stay transparent.
static const int TIERS[] = {16, 24, 32, 48};
#define TIER_COUNT ((int)(sizeof(TIERS) / sizeof(TIERS[0])))
#define MAX_TIER   48

static int read_icon_png(const char *png_dir,
                         int tier_px,
                         const char *name,
                         unsigned char *out_rgba) {
    char path[1024];
    int n = snprintf(path, sizeof(path), "%s/tier_%d/%s.png", png_dir, tier_px, name);
    if (n < 0 || n >= (int)sizeof(path)) {
        fprintf(stderr, "error: png path too long for %s\n", name);
        return 1;
    }
    int w = 0, h = 0, channels = 0;
    unsigned char *data = stbi_load(path, &w, &h, &channels, 4);
    if (!data) {
        fprintf(stderr, "error: stbi_load failed for %s: %s\n", path, stbi_failure_reason());
        return 1;
    }
    if (w != tier_px || h != tier_px) {
        fprintf(stderr, "error: %s has size %dx%d, expected %dx%d\n",
                path, w, h, tier_px, tier_px);
        stbi_image_free(data);
        return 1;
    }
    memcpy(out_rgba, data, (size_t)tier_px * tier_px * 4);
    stbi_image_free(data);
    return 0;
}

static void write_banner(FILE *f,
                          const char *lucide_version,
                          const char *rsvg_version,
                          const char *license_path) {
    fputs("/*\n", f);
    fputs(" * Generated icon atlas. Do not edit by hand.\n", f);
    fputs(" * Include only from gallery translation units; never from wollix.h\n", f);
    fputs(" * or any backend header.\n", f);
    fputs(" *\n", f);
    fprintf(f, " * Upstream project : lucide-icons\n");
    fprintf(f, " * Upstream version : %s\n", lucide_version ? lucide_version : "unknown");
    fprintf(f, " * Upstream archive : https://github.com/lucide-icons/lucide/archive/refs/tags/%s.zip\n",
            lucide_version ? lucide_version : "unknown");
    fprintf(f, " * License notice   : %s\n", license_path ? license_path : "demos/assets/icons/LUCIDE_LICENSE");
    fprintf(f, " * Rasterizer       : %s\n", rsvg_version ? rsvg_version : "unknown");
    fprintf(f, " * Regenerate with  : bash tools/gen_icon_atlas.sh\n");
    fputs(" */\n\n", f);
}

int main(int argc, char **argv) {
    const char *png_dir = NULL;
    const char *output_path = NULL;
    const char *lucide_version = NULL;
    const char *rsvg_version = NULL;
    const char *license_path = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--png-dir") == 0 && i + 1 < argc) {
            png_dir = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_path = argv[++i];
        } else if (strcmp(argv[i], "--lucide-version") == 0 && i + 1 < argc) {
            lucide_version = argv[++i];
        } else if (strcmp(argv[i], "--rsvg-version") == 0 && i + 1 < argc) {
            rsvg_version = argv[++i];
        } else if (strcmp(argv[i], "--license-path") == 0 && i + 1 < argc) {
            license_path = argv[++i];
        } else {
            fprintf(stderr, "error: unknown argument: %s\n", argv[i]);
            return 1;
        }
    }
    if (!png_dir || !output_path) {
        fprintf(stderr,
                "usage: %s --png-dir DIR --output FILE"
                " [--lucide-version V] [--rsvg-version V] [--license-path P]\n",
                argv[0]);
        return 1;
    }

    int tier_y[TIER_COUNT];
    int atlas_h = 0;
    for (int t = 0; t < TIER_COUNT; ++t) {
        tier_y[t] = atlas_h;
        atlas_h += TIERS[t];
    }
    const int atlas_w = MAX_TIER * ICON_COUNT;
    const size_t atlas_bytes = (size_t)atlas_w * atlas_h * 4;

    unsigned char *atlas = (unsigned char *)calloc(atlas_bytes, 1);
    if (!atlas) {
        fprintf(stderr, "error: failed to allocate %zu bytes for atlas\n", atlas_bytes);
        return 1;
    }

    unsigned char *cell = (unsigned char *)malloc((size_t)MAX_TIER * MAX_TIER * 4);
    if (!cell) {
        fprintf(stderr, "error: failed to allocate cell scratch\n");
        free(atlas);
        return 1;
    }

    for (int t = 0; t < TIER_COUNT; ++t) {
        const int tier_px = TIERS[t];
        const int row_y = tier_y[t];
        for (int i = 0; i < ICON_COUNT; ++i) {
            if (read_icon_png(png_dir, tier_px, icons[i].name, cell) != 0) {
                free(cell);
                free(atlas);
                return 1;
            }
            const int dst_x = i * MAX_TIER;
            // White-alpha encoding: RGB = 255 for every pixel, preserve coverage
            // from the rasterizer's alpha channel. Tint at the call site.
            for (int y = 0; y < tier_px; ++y) {
                for (int x = 0; x < tier_px; ++x) {
                    unsigned char a = cell[(y * tier_px + x) * 4 + 3];
                    size_t dst = ((size_t)(row_y + y) * atlas_w + (dst_x + x)) * 4;
                    atlas[dst + 0] = 255;
                    atlas[dst + 1] = 255;
                    atlas[dst + 2] = 255;
                    atlas[dst + 3] = a;
                }
            }
        }
    }

    free(cell);

    FILE *out = fopen(output_path, "wb");
    if (!out) {
        fprintf(stderr, "error: cannot open %s for write\n", output_path);
        free(atlas);
        return 1;
    }

    write_banner(out, lucide_version, rsvg_version, license_path);

    fputs("#ifndef WLX_ICONS_H\n", out);
    fputs("#define WLX_ICONS_H\n\n", out);
    fprintf(out, "#define WLX_ICONS_WIDTH  %d\n", atlas_w);
    fprintf(out, "#define WLX_ICONS_HEIGHT %d\n", atlas_h);
    fprintf(out, "#define WLX_ICON_TIER_COUNT %d\n\n", TIER_COUNT);

    fputs("typedef enum {\n", out);
    for (int i = 0; i < ICON_COUNT; ++i) {
        fprintf(out, "    WLX_ICON_%s = %d,\n", icons[i].enum_id, i);
    }
    fprintf(out, "    WLX_ICON_COUNT = %d\n", ICON_COUNT);
    fputs("} WLX_Icon;\n\n", out);

    fputs("typedef struct { int x, y, w, h; } WLX_Icon_Rect;\n\n", out);

    fputs("static const int wlx_icon_tier_sizes[WLX_ICON_TIER_COUNT] = {", out);
    for (int t = 0; t < TIER_COUNT; ++t) {
        fprintf(out, "%s%d", t ? ", " : "", TIERS[t]);
    }
    fputs("};\n", out);

    fputs("static const int wlx_icon_tier_y[WLX_ICON_TIER_COUNT] = {", out);
    for (int t = 0; t < TIER_COUNT; ++t) {
        fprintf(out, "%s%d", t ? ", " : "", tier_y[t]);
    }
    fputs("};\n\n", out);

    fputs("static const WLX_Icon_Rect wlx_icon_rects_tiered[WLX_ICON_TIER_COUNT][WLX_ICON_COUNT] = {\n", out);
    for (int t = 0; t < TIER_COUNT; ++t) {
        fprintf(out, "    { /* tier %d */\n", TIERS[t]);
        for (int i = 0; i < ICON_COUNT; ++i) {
            fprintf(out, "        {%4d, %3d, %2d, %2d}, /* %s */\n",
                    i * MAX_TIER, tier_y[t], TIERS[t], TIERS[t], icons[i].name);
        }
        fputs("    },\n", out);
    }
    fputs("};\n\n", out);

    // Backwards-compatible 1D table: smallest tier, same row as tiered[0].
    fputs("static const WLX_Icon_Rect wlx_icon_rects[WLX_ICON_COUNT] = {\n", out);
    for (int i = 0; i < ICON_COUNT; ++i) {
        fprintf(out, "    {%4d, %3d, %2d, %2d}, /* %s */\n",
                i * MAX_TIER, tier_y[0], TIERS[0], TIERS[0], icons[i].name);
    }
    fputs("};\n\n", out);

    fputs("static const unsigned char wlx_icons_rgba[WLX_ICONS_WIDTH * WLX_ICONS_HEIGHT * 4] = {\n", out);
    for (size_t i = 0; i < atlas_bytes; i += 16) {
        fputs("    ", out);
        size_t end = (i + 16 < atlas_bytes) ? (i + 16) : atlas_bytes;
        for (size_t j = i; j < end; ++j) {
            fprintf(out, "0x%02x,", atlas[j]);
            if (j + 1 < end) fputc(' ', out);
        }
        fputc('\n', out);
    }
    fputs("};\n\n", out);

    fputs("#endif\n", out);

    fclose(out);
    free(atlas);

    fprintf(stdout, "%s\n", output_path);
    return 0;
}
