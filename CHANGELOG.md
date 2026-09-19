# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Changed (Breaking) - v0.9 coordinated API group

One release, one group, and the last one. **v0.9 is the final release
that changes the meaning of an existing option field or the shape of the
backend contract**; from the next minor on, existing names, defaults and
signatures keep working. Renamed fields, constants and types keep a
deprecated alias of the same storage or value for **one minor version
after 0.9**, so existing code compiles unchanged; the aliases themselves
(`.align`, `.widget_align`, tooltip `.padding`, `WLX_FLOAT_UNSET`,
`WLX_Backend_V1`, `wlx_backend_from_v1`) are removed in the first minor
release after 0.9, exactly as the v0.6 aliases were removed in 0.7. Default
*meanings* cannot be aliased: each one below names its migration.

- **Backend contract v2: every callback carries an instance pointer.** All
  22 `WLX_Backend` callbacks take a trailing `void *user`, the table's new
  `user` member, passed by the core on every call and never read by it, so
  an adapter can reach per-instance state instead of file-scope globals
  (the in-tree adapters still use their globals this release and pass
  `NULL`). The table also carries `contract_version`, which must be
  `WLX_BACKEND_CONTRACT_VERSION` (`2`): `wlx_begin` checks it in every
  build, because a v0.8 table compiles with warnings on compilers older
  than GCC 14 and calling it through the v2 signatures is memory-unsafe.
  Migration, one line: `ctx->backend = wlx_backend_from_v1(&my_table);`
  with the v0.8 table typed `WLX_Backend_V1` (the shim and the type are
  removed in the first minor after 0.9); or, for good, append `void *user`
  to each callback and set `.contract_version`. An application that
  overrides individual callbacks of an adapter-installed table forwards
  `user` unchanged and never repoints `backend.user`.
- **`.align` is `.content_align`; `.widget_align` is `.slot_align`.** The
  two alignment fields never said what they aligned: `content_align`
  places the text or image inside the widget rect (the typography field,
  and `wlx_image`'s), `slot_align` places the widget rect inside its layout
  slot, pairing with `padding` / `content_padding`. Both old names still
  compile via anonymous-union aliases of the same storage; every demo,
  test and doc is on the new names. Migration: rename the initializers
  (`sed -E 's/\.widget_align\b/.slot_align/g; s/\.align\b/.content_align/g'`
  over your sources does it; `title_align` is untouched).
- **`WLX_UNSET` is the single unset sentinel for numeric option fields.**
  Every option field follows one of two rules, documented in SENTINEL.md:
  zero is unset only where zero can never be a meaningful value (colors,
  fonts, `font_size`, ...); everywhere zero is meaningful the macro installs
  `WLX_UNSET` (`-1`) and the resolver replaces it with the theme, parent or
  widget default. `-1` was already the value on most fields, so
  `.width = -1` and friends keep working; `WLX_FLOAT_UNSET` (`-1e30f`) stays
  as a deprecated alias that still resolves as unset. The brightness shifts
  (`hover_brightness`, `thumb_hover_brightness`, `scrollbar_hover_brightness`,
  theme `disabled_brightness`) join the same sentinel: their domain is now
  `-1 < b <= 1`, and an explicit `-1.0f` means unset rather than a full
  black shift. Migration: write `-0.999f` for a full darken; spell the
  sentinel `WLX_UNSET`.
- **Panel roundness inherits the theme.** `wlx_panel_begin` `.roundness` and
  `.rounded_segments` were literal zeros while `.border_width` inherited
  the theme (v0.6); all three now inherit like every widget's. The bundled
  themes ship `roundness = 0`, so stock rendering is unchanged. Migration:
  under a custom theme with non-zero roundness, a panel that relied on the
  literal default passes `.roundness = 0`.
- **Tooltip `.padding` is `.content_padding`.** `padding` is the slot inset
  everywhere else and the tooltip's was an inner inset; the tooltip now
  carries the shared `content_padding` field group (per-side fields
  included) with its 6 px default. `.padding` still compiles via the
  deprecated alias. Migration: rename the initializer.
- **Menu `.width` follows the unset rule.** Unset resolves to 180 as before;
  an explicit `.width = 0` is now a zero-width list instead of 180, as `0`
  is on every other `width`. Migration: drop an explicit `0`.
- **Panel `.title_align = WLX_ALIGN_NONE` is honoured.** The centred default
  moved into the macro; an explicit `WLX_ALIGN_NONE` now places the title
  at the raw rect origin instead of being resolved to `WLX_CENTER`.
  Migration: write `WLX_CENTER` if `WLX_ALIGN_NONE` was meant as "default".
- **Split sizes `WLX_SLOT_AUTO` are honoured.** `first_size`, `second_size`
  and `fill_size` default in the macro (`WLX_SLOT_PX(280)`, `WLX_SLOT_FLEX(1)`,
  `WLX_SLOT_FLEX(1)`); an explicit `WLX_SLOT_AUTO` reaches the layout as
  AUTO instead of being mistaken for the default. `wlx_slot_size_is_zero`
  is removed. Migration: none unless `WLX_SLOT_AUTO` was written to mean
  "default".
- **Compound content padding: unset means the widget default.** Writing
  `.content_padding = WLX_UNSET` on the inputbox, editor, split, panel or
  tooltip now resolves to the widget's own inset (10, 10, 4, 2, 6) exactly
  like omission, as SENTINEL.md always said, instead of `0`; the constants
  are `WLX_INPUTBOX_CONTENT_PADDING`, `WLX_SPLIT_CONTENT_PADDING`,
  `WLX_PANEL_CONTENT_PADDING`, `WLX_TOOLTIP_CONTENT_PADDING`. Omission is
  unchanged. Migration: write `0` for no inset.
- **Split `.gap` is a literal `0`** like every other container gap (its `-1`
  resolved to `0` and inherited nothing). Omission is unchanged.

#### Migrating from 0.8

In call-site order; everything compiles before you start, so migrate at
your own pace before the next minor.

1. **Align names.** `sed -E 's/\.widget_align\b/.slot_align/g; s/\.align\b/.content_align/g'`
   over your sources (`title_align` is untouched). Both old names still
   compile until the next minor.
2. **Unset spelling.** Write `WLX_UNSET` where you wrote `-1` or
   `WLX_FLOAT_UNSET` to mean "unset"; `-1` keeps working on every numeric
   field, `WLX_FLOAT_UNSET` until the next minor.
3. **Brightness `-1.0f`.** If a hover or disabled shift was exactly `-1.0f`
   (a full black shift), write `-0.999f`; `-1.0f` now means unset.
4. **Panel roundness under a custom theme.** A panel that relied on the
   literal sharp default while the theme has non-zero `roundness` passes
   `.roundness = 0`.
5. **Tooltip inset.** `.padding` on `wlx_tooltip_for` becomes
   `.content_padding` (old name compiles until the next minor).
6. **Menu `.width = 0`**, **panel `.title_align = WLX_ALIGN_NONE`**, **split
   sizes written as `WLX_SLOT_AUTO`**, **compound `.content_padding = -1`**:
   only if you wrote these to mean "the default" - drop the field instead.
7. **Backend tables.** Append `void *user` to every callback and set
   `.contract_version = WLX_BACKEND_CONTRACT_VERSION`; or, for now, type the
   table `WLX_Backend_V1` and install `wlx_backend_from_v1(&table)`. An
   application that wrapped an adapter's text callbacks to scale styles
   installs one `wlx_set_style_transform` instead.
8. **Callers without designated initializers** (C++ translation units,
   table-driven builders) take `wlx_<widget>_opt_defaults()`, assign, and
   call the `_impl` function.

### Added
- **Calling from C++.** The public half of all five headers now carries C
  linkage (`extern "C"`) and parses as ISO C++11, so a C++ translation unit
  includes the headers as-is and links against the implementation compiled
  as C11 in one C translation unit (the backend adapter lives there too;
  defining `WOLLIX_IMPLEMENTATION` from C++ is not supported). The
  documented path is `wlx_<widget>_opt_defaults()`, assign, call the
  widget's `_impl` entry; README "Using from C++" and API_REFERENCE
  "Calling from C++" carry the rules, the example and the entry table.
- **C++ spellings of the literal helpers.** `WLX_RGBA` and every
  `WLX_SLOT_*` macro expand under `__cplusplus` to a brace-initialised
  prvalue (`WLX_SLOT_LIT` picks the form per language), with the casts
  C++11 list-initialisation needs for `int` arguments; the C expansions are
  unchanged. `WLX_SIZES` stays C-only and documents the named-array form.
- **`from_defaults` marker.** Every option struct ends with
  `bool from_defaults`, set by its defaults macro and so by the defaults
  function and every copy. Under `WLX_DEBUG` a struct that reaches a widget
  entry with the flag clear (a zero-initialised struct, which holds zeros
  where the defaults are non-zero) warns once per call site, naming the
  entry and the defaults function to start from; release builds never read
  it. SENTINEL.md documents the rule.
- **C++ gate in `make test`.** `tests/test_cpp_path.cpp` is a C++11 caller
  over the public headers and the mock backend, linked against the
  implementation compiled as C11; `CXX` follows `CC` (gcc -> g++, otherwise
  clang++) so the CI matrix covers both front ends.
- **MSVC build requirement documented.** `/std:c11 /Zc:preprocessor`
  (Visual Studio 2019 16.8 or later); the conforming preprocessor is needed
  for `wlx_layout_begin_s`. A Windows CI leg follows in this cycle.
- **`wlx_set_style_transform`.** A context-level transform (function plus
  user pointer) that the core applies to the `WLX_Text_Style` immediately
  before every text callback - draw, measure and advances - and nowhere
  else, so widget options, the command buffer and retained geometry hold
  nominal styles while the backend sees only transformed ones. This
  replaces the pattern of wrapping an adapter's five text callbacks
  identically (the rule the 0.7.0 caret-drift fix documented, and which
  nothing could enforce): the dashboard and gallery Raylib font scales are
  one transform each now. Every call counts as a measurement-environment
  change, so the editor's retained geometry is rebuilt on the next frame.
- **Option defaults as values.** `wlx_<widget>_opt_defaults()` returns each
  option struct's defaults exactly as its macro installs them, one function
  per struct (26 in the core, `wlx_editor_opt_defaults` in the editor
  header). This is the call surface for code that cannot repeat designated
  initializers - take the defaults, assign, call the `_impl` function. The
  macros stay the C11 surface.
- **Undo and redo in the inputbox, textarea and editor.** Every edit made
  through a text widget lands in a per-widget undo journal in the core;
  command+Z steps back and command+Shift+Z or command+Y steps forward
  (repeating while held), restoring the bytes and the caret pair exactly,
  selection included. Typed, Backspace and Delete runs coalesce into one
  step; arrows, caret-moving clicks, Enter, Tab, paste, cut, word and
  selection deletes and an undo itself start a new one; a new edit clears
  redo; `read_only` rejects the chords and `password` fields keep no
  journal. The journal is bounded per widget by the new `#ifndef`-guarded
  `WLX_TEXT_UNDO_ENTRIES` (512) and `WLX_TEXT_UNDO_BYTES` (256 KB), whole
  oldest steps evicted first and a single oversize step still kept;
  `WLX_TEXT_UNDO_ENTRIES 0` compiles the journal out. History is dropped
  when the buffer changes outside the widget: length changes are detected,
  and `WLX_Inputbox_Opt` gains `.revision` for same-length rewrites, as the
  editor already had. The editor's undo replays through the same edit
  primitives as a keystroke, so its retained geometry, wrapped bottom
  anchor and caret-follow carry over unchanged. See WIDGETS.md "Undo and
  redo".

### Fixed
- **Layout `.clip` gates the pointer.** A `.clip` layout cropped its
  children's drawing but not their hit zones: the cropped part of a widget
  could still be hovered, pressed and dragged, and under topmost-wins
  arbitration a clipped-away widget declared later took the click from a
  visible one declared earlier. The clip walk that hit-testing and
  scroll-panel nesting share (`wlx_enclosing_clip`) now counts open `.clip`
  layouts alongside scroll panel viewports and overlay roots, so hover,
  press, drag, the focus ring and tooltip anchors stop at the clip rect; a
  scroll panel inside a clip layout is scissored to the intersection and the
  clip is re-armed after the panel ends (siblings declared after a nested
  panel were drawn unclipped). Four tests in `tests/test_layout_clip.c`.
- **Scroll panels take the wheel only where they are visible.** A panel's
  wheel ownership was raw containment on its viewport, so a panel cropped by
  a `.clip` layout or scrolled out of an outer panel scrolled through its
  invisible part and starved the visible sibling under the pointer; the
  scroll panel bar and the text-widget scrollbar thumb also lit their hover
  tint there. All three now go through the same container clip as hover and
  press.
- **Auto-counted layouts honour `.clip`.** `wlx_layout_begin_auto` accepted
  the option but never installed the scissor or joined the clip walk, so its
  overflowing children were drawn and stayed interactive; both begins now
  share one clip path. Two tests.
- **Keyboard focus skips widgets that are clipped away, and the ring stays
  inside the clip.** A widget scrolled out of its panel or cropped away by a
  `.clip` layout remained a Tab stop with no ring, so keystrokes reached an
  invisible widget; it is no longer a stop. The ring is clamped to the
  container clip around the focused widget (a scroll panel viewport, a
  `.clip` layout, or a popup's own rect), so a partially cropped widget's
  ring wraps its visible part instead of stroking over the neighbouring
  content, and a widget filling its popup gets a ring that hugs the popup
  edge. Two tests in `tests/test_tab_traversal.c`.

### Changed
- SENTINEL.md is rewritten around the two unset rules, the literal
  defaults, the request tokens and the container exception; the
  `wollix.h` option-field comment block states the same rules in short.
- Tests: every option default macro sets each designator once, pinned by
  `tests/test_defaults_once` (built with the initializer-override warning
  promoted to an error); `tests/test_sentinel.c` pins explicit zero, unset
  inheritance, the brightness domain, the padding request token and each
  default change above.
- Tests: the enabled slider's press-to-jump, drag, release and custom-range
  mapping are pinned in `tests/test_widgets.c`; the disabled case was the
  only drag-cycle test before.
- The four companion headers (`wollix_raylib.h`, `wollix_sdl3.h`,
  `wollix_wasm.h`, `wollix_editor.h`) carry the MIT copyright notice the
  core already had.

### Removed
- `build.sh`, the pre-Makefile demo build script: it built 21 of the 31
  demos (20 Raylib demos plus the SDL3 demo), lacked the SDL3_ttf flags its
  own SDL3 step needed and was referenced by nothing; `make` is the build.

## [0.8.0] - 2026-09-12

The overlay release: draw commands and interactive widgets now carry a
layer, the topmost widget under the pointer owns the press and the
hover, and the popup family — dropdowns, tooltips, and nestable
menus — rests on that primitive. Alongside it, the input contract
grows right and middle buttons and a float wheel on both axes, Tab
traversal and a focus ring make the widget set keyboard-operable,
an I-beam appears over text, and CONTENT slots size horizontally
as well as vertically.

### Added
- **Input contract v2: mouse buttons, float wheel axes, F-keys, touch on
  the web.** `WLX_Input_State` gains right/middle button state with
  one-frame press edges (`mouse_right_down/clicked`,
  `mouse_middle_down/clicked`) and a horizontal wheel axis
  (`wheel_delta_x`); `WLX_Key_Code` gains `WLX_KEY_F1..F12` and
  `WLX_KEY_INSERT` (`WLX_KEY_COUNT` 51 -> 64). New `wlx_is_mouse_right_*` /
  `wlx_is_mouse_middle_*` helpers, and `WLX_Interaction.right_clicked`: a
  right press is owned by the previous frame's topmost candidate under the
  pointer (popup content beats the base widget it covers), fires on the
  press frame, and never blurs focus or disturbs a left press or drag. The
  editor consumes the horizontal wheel into its horizontal scroll. All
  three backends land together: Raylib reports `GetMouseWheelMoveV` raw on
  both axes and maps the new keys; SDL3 accumulates raw wheel values on
  both axes (natural-scroll direction normalized) and reads right/middle
  from the mouse-state mask; the web host moves to pointer events with
  pointer capture - a drag released outside the canvas ends cleanly
  instead of sticking, right/middle arrive via `e.button`, and
  single-finger **touch drives the published demo** (`touch-action: none`
  on the canvas; `pointercancel` releases every button). F-keys are
  recorded but left to the browser (F5/F11/F12 keep their meaning). See
  API_REFERENCE.md "WLX_Input_State" and LAYOUT_MODEL.md "Input State".
- **Keyboard focus traversal and focus ring.** Tab / Shift-Tab move a
  keyboard focus ring through the operable widgets in declaration order
  (wrapping at both ends; only the topmost open layer participates, so an
  open menu or dropdown owns the ring); Enter / Space activate the focused
  button-like widget exactly like a click; tabbing onto an inputbox or
  textarea focuses it for typing and the field never swallows Tab; the
  editor keeps Tab as indent while focused and is left with Escape, then
  Tab. While a widget holds the ring, `wlx_end` draws one accent outline
  around it (`theme->accent`; `WLX_FOCUS_RING_THICKNESS` /
  `WLX_FOCUS_RING_GAP` overridable); any pointer press drops the ring, so
  mouse-driven sessions never see it. Keyboard focus is a third identity
  beside hot and active (`wlx_focused_id()` reads it); the interaction
  flags gain `WLX_INTERACT_FOCUS_HOLD_TAB` (keep Tab while focused) and
  `WLX_INTERACT_TAB_SKIP` (never a Tab stop) for custom widgets. Closes the
  documented "Tab reserved for focus traversal" promise. See WIDGETS.md
  "Keyboard operation" and LAYOUT_MODEL.md "Keyboard Focus Traversal".
- **Mouse cursor shapes: I-beam over text.** `WLX_Backend` gains an
  optional `set_cursor(WLX_Cursor_Shape)` callback (the 22nd; `NULL`
  leaves the platform cursor alone). The core resolves the shape from the
  widget under the pointer - the same topmost-wins candidate that owns
  hover - and pushes it only on change, so adapters stay stateless. The
  inputbox, textarea, and the editor's text region (line-number gutter
  excluded) show the I-beam; everything else keeps the arrow, and a text
  selection drag keeps the I-beam after leaving the field. Wired on
  Raylib, SDL3, and the web host. New `WLX_INTERACT_TEXT_CURSOR` flag for
  custom text-like widgets. See API_REFERENCE.md "WLX_Backend" and
  LAYOUT_MODEL.md "Backend Contract".
- **Overlay layers, topmost-wins input, and the popup family.** Draw
  commands and interactive widgets now carry a layer;
  `wlx_overlay_begin/end` roots an absolutely positioned subtree on the
  next layer (nesting up to `WLX_OVERLAY_MAX_LAYERS`), replayed above
  everything below it. Input follows: the previous frame's topmost hit
  candidate under the pointer owns hover (exactly one widget reports
  `hover` per frame now) and latches the press, so popup content wins
  clicks over whatever it covers, and the wheel scrolls only the
  pointer's layer — an open popup keeps the base UI still. On top of the
  primitive: `wlx_dropdown` (closed face + below-anchored scrollable
  list), `wlx_tooltip_for` (hover-delay, pointer-anchored, draw-only —
  it can never steal input), and `wlx_menu_begin/item/end`
  (caller-triggered point-anchored menu, nestable to `WLX_MENU_STACK_MAX`; a
  `.keep_open` item — the submenu trigger, or a checkable entry — clicks
  without closing the menu; `wlx_menu_button_begin` is the button-anchored
  variant whose face toggles the menu like a dropdown and anchors the list
  below itself; `wlx_submenu_begin` opens the submenu level with no
  coordinates, anchored beside its trigger item and inheriting the
  parent's styling). All
  three close on Escape or an outside press without eating that press.
  `wlx_last_rect` returns the rect of the widget just placed - the natural
  tooltip anchor with no manual geometry.
  Frames that never leave layer 0 keep the previous flat replay path.
  See LAYOUT_MODEL.md sections 8-9, WIDGETS.md, `demos/popup.c`, and the
  dashboard's Components > Popups module.
- **CONTENT slots size horizontally: intrinsic widths.** A
  `WLX_SLOT_CONTENT` slot in a HORZ linear layout now resolves from its
  children's widths — fit-to-label buttons and auto-width columns work.
  Each widget offers its natural width (always the single-line, unwrapped
  measure): label/button measure their text plus padding and the image
  band; checkbox/toggle/radio use their glyph row; image its
  sub-rect/texture width; separator its thickness. Explicit `.width` always
  wins; sliders, inputs, the editor, scroll panels, and nested layouts
  contribute explicit widths only. The width settles one frame after
  content changes (`WLX_SLOT_CONTENT_MIN` hides the pop, as with vertical
  CONTENT), and the deferred replay corrects drawn positions on the
  measuring frame itself: command ranges carry a dx offset beside dy, so
  HORZ CONTENT has full parity with the VERT correction, scissors
  included. Intrinsic measurement runs only inside width-consuming slots,
  so existing layouts measure nothing extra. Internally the per-layout
  CONTENT state now stores the main-axis extent (heights for VERT layouts,
  widths for HORZ — previously HORZ misread heights as widths and warned
  under `WLX_DEBUG`; that diagnostic is gone). See LAYOUT_MODEL.md § Layout
  Capabilities and WIDGETS.md § Intrinsic widths.

### Fixed
- **The wrapped editor's vertical thumb no longer sticks at the track end
  while scrolling back up.** The wrapped thumb maps hard-line pseudo
  pixels, and its range ended at `line_count * line_h - band.h` - the
  bottom of an unwrapped document. A band of wrapped rows holds fewer
  hard lines than that, so in prose the range end sat many lines above
  the real bottom anchor: the thumb hit the track end early and stayed
  there while the view scrolled up through those lines (the dashboard's
  prose doc parked it for a long stretch), and a drag over the last part
  of the track jumped the same distance. The range now ends at the bottom
  anchor's pseudo scroll - the anchor that bottom-aligns the last row,
  filled backward from the document end over at most a band of lines and
  cached in the editor state (end-relative, so edits before it only shift
  it) - so the thumb reaches the track end exactly at the document end,
  leaves it on the first row back up, and a drag is continuous over the
  whole track. Unwrapped documents are unchanged: both ends coincide at
  wrap factor one. The wrapped cold frame now also counts the rows of the
  band of lines at the document end, retained thereafter; steady frames
  are unaffected (`make perf-editor` cold bounds updated).
- **Raylib dashboard and gallery text no longer renders smaller than the
  SDL3/WASM builds.** Both demos scale raylib's font size up (1.31x and
  1.15x) to match the other backends, wrapping the adapter's text
  callbacks. When the raylib adapter gained `draw_text_slice` (the core
  prefers it over `draw_text`), the shims kept wrapping only `draw_text`,
  so every text command drew at nominal size against a layout measured at
  the scaled size. Both shims now wrap the slice draw as well, and the
  `WLX_Backend` comment states that a decorating app must wrap every text
  callback the adapter installs.
- **Scrollbar thumbs no longer vanish over long content.** The thumb was
  strictly proportional to the visible share of the content, so a 30k-line
  editor document in a 20-row band drew a sub-pixel thumb - invisible and
  undraggable. Every thumb (scroll panel, multiline inputbox, editor on both
  axes) now floors at 20px, or the track when shorter, and its travel maps
  the scroll range onto the remaining track: the track end still means the
  content end, and a held thumb stays under the pointer. The dashboard's
  editor page also highlights the document picker that is actually loaded
  instead of always the sample one.
- **A clamped fixed slot could land two pixels past its max in an
  overconstrained frame.** Redistribution re-read each slot's size from the
  already snapped boundaries and snapped the rebuilt offsets again, so a
  `PIXELS` / `PERCENT` / `FILL` / `CONTENT` slot clamped to a fractional max
  entered the freeze loop a pixel over and a float sum landing on .5 added
  the second. Redistribution now starts from the exact clamped sizes and
  snaps once, so every slot lands within one pixel of its clamped size; in a
  frame whose requested sizes already exceed it, one boundary may move by a
  pixel. The fuzz suite that flaked on this (about 2.4% of seeds) now gates
  on the captured seeds first and adds one time-seeded round that prints
  its seed and slots on failure.
- **A scroll panel contributes its explicit `.width` to a `CONTENT`-sized
  parent slot.** WIDGETS.md has listed the panel as contributing "explicit
  `.width` ... only" since the two-axis CONTENT wave, but the contribution
  helper sent height alone, so a scroll panel in a horizontal CONTENT slot
  collapsed to the unmeasured floor. It now contributes `opt.width` when
  set (and nothing otherwise - the viewport rect would be circular).
- **Popups declared inside a scroll panel receive input outside the panel
  viewport.** An overlay escaped the base layer's clip for drawing, but the
  walkers that intersect "all active clips" - hover and press hit-testing,
  the candidate clip, a nested scroll panel's scissor and its restore, the
  enclosing-scissor query - iterated every base scroll panel regardless of
  layer, so a dropdown, menu or raw `wlx_overlay` declared inside a scroll
  panel could not select rows past the viewport and the dropdown's inner
  list scissor was cut at it. Each layer now has its own clip context
  (`WLX_Clip_Base`): an overlay starts a fresh one at its own rect and
  `wlx_overlay_end` restores the enclosing one; one walker
  (`wlx_enclosing_clip`) serves the five sites with unchanged base-layer
  rules. Follow-ons: popup rects are culled against the overlay's own rect
  instead of being exempt from culling, and `wlx_overlay_end` re-arms the
  enclosing clip only in immediate mode (the deferred base pass never lost
  it). Seven in-panel tests plus two stream pins.
- **Drags hold on `mouse_down`.** `WLX_INTERACT_DRAG` acquired on
  `mouse_down` but kept the drag alive only while `mouse_held` was set, so a
  host that fills `mouse_down` alone (the documented contract; `mouse_held`
  is its legacy twin per ADR_040) lost every drag on the frame after the
  press. The core now reads `mouse_down` for both; `mouse_held` stays in the
  struct as the documented legacy-equal field.
- **`wlx_last_rect` after a `wlx_menu_button_begin` block is the face.**
  It reported the last item row while the menu was open, so a tooltip
  anchored right after the block jumped onto an item; the menu button now
  follows the dropdown's one-widget rule and re-publishes its face at
  `wlx_menu_end` (point-anchored `wlx_menu_begin` menus are unchanged).
- **A submenu's `*open` flag no longer outlives its parent.** Chain
  dismissal marked only the menu frames on the stack at click time, so a
  parent leaf declared *before* the submenu closed the parent but left the
  submenu's caller-owned flag set - the next open showed a stale submenu,
  and both in-tree consumers carried `if (!menu_open) submenu_open =
  false;` to hide it. `wlx_submenu_begin` now clears the flag when an
  ancestor leaf was activated earlier in the frame or when the parent is
  re-opening; the workarounds are gone from `demos/popup.c` and the
  dashboard.
- **Frame-scratch pointers no longer go stale across scratch growth
  (use-after-free under ASan).** Two classes, both in the per-frame byte
  scratch arena, which is realloc-grown: (1) a layout's retained slot-size
  array was read through a pointer captured at `layout_begin` by the
  intrinsic-width gate, the VERT fixed-slot contribution override, and the
  debug layout shadow, so a sibling recording enough text (or a nested
  CONTENT layout) between begin and the read left it dangling; the array is
  now re-derived from its offset on every read (`wlx_layout_content_sizes`)
  and the pointer field is gone. (2) `wlx_end` took its per-command dy, dx
  and layer tables as three successive scratch allocations and kept the
  earlier pointers across the later allocations - on growth frames the dy
  zero-fill and the layered-dispatch reads hit freed memory; the three
  tables now share one block. Plain runs passed by allocator luck (the old
  block usually survives); an ASan build of the suite reported six
  heap-use-after-free sites on existing tests, now zero. New
  `content_sizes_survive_scratch_growth` test pins the gate on a growth
  frame.
- **Clicking outside a focused field no longer swallows the press.** A
  focused inputbox/editor holds the interaction lock (`active_id`) across
  frames and released it only when itself queried, so a press on a button
  declared earlier in the frame silently did nothing — the user had to click
  twice, and whether the first click worked depended on declaration order of
  unrelated widgets. `wlx_begin` now releases a focus-class holder when a
  fresh press lands outside the widget's recorded rect, before any widget is
  queried, so the same press activates its real target. Drag interactions
  (sliders, scrollbars) are never touched, Escape blur is unchanged, and the
  released widget still observes its `just_unfocused` edge.
- **Clipboard transports grow instead of silently truncating at 1 KB.** All
  three adapters funneled clipboard traffic through fixed 1024-byte static
  buffers, so copying or pasting more than ~15 lines silently lost data.
  The transports are now grow-and-reuse heap buffers bounded by a 16 MB soft
  cap (`WLX_RAYLIB_CLIPBOARD_MAX` / `WLX_SDL3_CLIPBOARD_MAX` /
  `WLX_WASM_CLIPBOARD_MAX`, overridable); truncation happens only at the cap
  and never splits a UTF-8 sequence. The WASM receive path grows and
  re-fetches from the host until the text provably fits. Pinned by a
  5,000-byte round-trip test through the core clipboard helpers.
- **Backend adapters no longer read one byte past text slices.** The Raylib
  slice measure, the Raylib advances walk, and the SDL3 debug-font draw
  probed `text[len]` for an existing NUL terminator to skip a copy — an
  out-of-bounds read for a slice ending exactly at the end of an allocation
  (the slice contract explicitly forbids the probe). All three paths now
  always copy through their existing stack-or-heap temp buffers; Raylib
  measure-cache hits still skip the copy entirely.
- **SDL3 caret blink and multi-click timing: frame time is now sampled once
  per frame.** `wlx_get_frame_time` returned the backend callback's value on
  every read, but the SDL3 adapter measures time since its own previous call
  — so with several reads per frame only the first saw real time: the
  inputbox caret never blinked on SDL3, and a later widget's multi-click
  clock barely advanced (slow single clicks could register as double/triple
  clicks). `wlx_begin` now takes the frame's single backend sample into
  `WLX_Context.frame_dt` and every read serves that cache; the
  `WLX_Backend.get_frame_time` contract documents the exactly-once-per-frame
  call. Behavior is now identical across Raylib, SDL3, and WASM.
- **The documented limit macros are actually overridable.**
  `WLX_MAX_SLOT_COUNT`, `WLX_CONTENT_SLOTS_MAX`, `WLX_OFFSET_STACK_LIMIT`,
  and `WLX_DA_INIT_CAP` were `#define`d unconditionally, so a user's
  pre-include definition was silently clobbered back to the default (with a
  redefinition warning); all four now sit behind `#ifndef`, matching the
  text-pipeline macros. Pinned by a new override test binary
  (`tests/test_config_override.c`) wired into `make test`.
- **size_t-overflow guards survive release builds.** The overflow checks in
  the frame sub-arena reserve/alloc paths and the layout slot-count sanity
  check were plain `assert()`s that compiled out under NDEBUG; they are now
  `WLX_HARD_ASSERT`, so a wrapped size aborts with a `wollix fatal:` message
  instead of silently writing out of bounds. Pinned by two new NDEBUG death
  tests. The unused `wlx_da_reserve` / `wlx_da_append` macros were removed
  (`WLX_DA_INIT_CAP` stays).

### Changed
- **Intrinsic widths of image buttons without an explicit `.image_size`
  follow the face.** In a `CONTENT`-sized slot, a label/button with a
  texture and no `.image_size` used to contribute the raw texture width;
  the face, however, reserves the auto band (`font_size * 1.5`) when text
  is present - so a small icon under-reserved and a large texture
  over-widened the slot. The intrinsic now reports the same band the face
  draws; image-only faces still contribute the texture (sub-rect) width,
  and explicit `.image_size` callers are unchanged.
- **Keyboard activation now admits the keyboard-focused widget.**
  `WLX_INTERACT_KEYBOARD` previously fired on Space/Enter only while the
  widget was hovered; it now fires when the widget is hovered **or** holds
  the Tab focus ring. The hover path is unchanged, so existing code keeps
  working; a widget that was never Tab-focused behaves exactly as before.
- **Wheel values are unquantized float detents on every backend.**
  `wheel_delta` (and the new `wheel_delta_x`) carry `1.0` per notch with
  trackpad fractions preserved. Raylib previously routed the wheel through
  an encoder-bounce debounce that suppressed single-frame direction
  reversals - it also ate legitimate fast reversals and is gone; SDL3 and
  the web host previously quantized every event to +-1 and now pass raw
  values (the web host converts `deltaMode` pixels/lines to detents at
  100 px and 3 lines per detent). Scroll speed is unchanged per detent, so
  clicky wheels feel the same; trackpads scroll smoothly instead of in
  notches, and rapid reversals are no longer dropped.
- **`WLX_Input_State` grew (208 -> 252 bytes; `WLX_KEY_COUNT` 51 -> 64).**
  Append-only, so every pre-existing field keeps its meaning and the
  offsets before `keys_pressed` are unchanged; but the key arrays are
  larger, so anything after `keys_down` moves. Hosts that write the struct
  by byte offset must use the new layout - in-tree that is the WASM host,
  whose `wollix_wasm.h` static-asserts fail the build on a mismatch.
  Recompiled C callers need no changes.
- **Overlapping widgets: topmost now wins the press and the hover.**
  Previously the first widget to query an overlapping rect captured the
  press and every overlapping widget reported `hover` at once
  (first-writer capture). With ownership arbitration the visually topmost
  candidate — highest layer, then latest query — is the single owner.
  Non-overlapping layouts behave identically; code that relied on an
  underneath widget receiving the press must move that widget to a higher
  layer (or reorder it). Activation timing is unchanged for static
  geometry; a widget appearing for the first time can activate one frame
  after it appears. Interactive containers (`.interact` on a layout) are
  covered by the same rule: a `CLICK` container no longer swallows the
  press of a button declared inside it - the child (the innermost query)
  owns the press and the container reports `clicked` only on its own
  non-interactive area (the 0.7.0 "non-interactive children only" scoping
  is lifted; LAYOUT_MODEL.md / API_REFERENCE.md updated).

### Internal
- The test runner is leak-clean and the sanitizer gate runs: every test
  that initializes a context destroys it, the WASM pool tests release
  their blocks, and `make test-asan` builds the runner under ASan + UBSan
  and runs it with leak detection on. CI's sanitizer job calls that target
  and the warnings-as-errors job overrides `BASE_CFLAGS`; both jobs had
  passed `CFLAGS=`, which the test binaries never read, so neither had run
  with the flags it named.
- The WASM clipboard transport is unit-tested: `tests/test_wasm_clipboard.c`
  defines the two clipboard imports as a host stand-in (the JS back-off
  rule) and pins the first fetch, the 3-byte back-off window, the geometric
  grow, the soft cap on a UTF-8 boundary, buffer reuse, the empty host and
  `clipboard_set`'s slice; `WLX_WASM_CLIPBOARD_MAX` is overridden to 12,000
  bytes in the runner so every path is reachable with kilobyte texts.
- Consistency batch, core header and docs: the five headers' comments and
  string literals are ASCII again (theme section rules, three em dashes,
  the wasm banner); the heights -> measures rename leftovers are gone
  (`slot_measures_off`, the debug validator's measure source, the parent
  contribution comment); the `WLX_INTERACT_KEYBOARD` comment (header and
  API_REFERENCE.md) states the actual rule (Space/Enter while hot or
  keyboard-focused); the `right_press_owner` comment says when it is read;
  WIDGETS.md names the gradient call the Raylib adapter makes
  (`DrawRectangleGradientEx`). The tooltip's default delay / padding and
  flip gaps are named constants, the two doubling growers share
  `WLX_GROW_INIT_CAP`, the candidate grower carries the same overflow
  hard-assert as its siblings, a static assert pins
  `WLX_OVERLAY_MAX_LAYERS` to the byte-wide replay layer tag (an override
  above 255 is now a build error instead of a silent wrap), the tooltip
  reads frame time through `wlx_get_frame_time`, and the unused
  `WLX_TODO` macro is removed.
- Consistency batch, adapters: every file-scope global in the three
  adapters carries the `g_` prefix (`g_wlx_sdl3_text_cache`,
  `g_wlx_sdl3_font_variants`, `g_wlx_wasm_clipboard_buf` / `_cap`; all
  file-local), `wollix_sdl3.h` is free of trailing whitespace and its
  eviction note is four lines, `wollix_raylib.h`'s cache comments state
  the 2048-default slot arithmetic and its font shim follows the backend
  factory like SDL3's, `WLX_WASM_POOL_MIN_ORDER` / `_MAX_ORDER` are
  `#ifndef`-guarded knobs (API_REFERENCE.md says so), and the SDL3
  override examples in API_REFERENCE.md show the defaults.
- Idiom sweep, draw / input / text: the sub-pixel outline rule
  (`wlx_outline_subpixel`, shared by the rect outlines and the focus ring),
  the wheel consume body (`wlx_wheel_consume_axis`; the scroll panel's
  hand-rolled consume now calls `wlx_wheel_consume`), the pointer-on-layer
  gate (`wlx_pointer_on_current_layer`, formerly `wlx_wheel_on_current_layer`;
  the tooltip's inline copy calls it), the reference line-height probe
  (`wlx_text_line_height`, four sites including the editor), the viewport
  clip of the interaction query (`wlx_interaction_clip_rect`: one clip walk
  per query instead of two, and the focus-ring rect uses it), the glyph-row
  block width (`wlx_glyph_row_block_w`, intrinsic and draw paths) and the
  multi-click clock tick (`wlx_text_edit_tick_click_clock`, inputbox and
  editor) each exist once. Pure moves; `test_core_utils.c` gains direct
  tests for the pure helpers and `test_disabled_state.c` pins that disabled
  hover stays viewport-clipped. Layout axis and slot: `wlx_layout_is_horz`
  / `wlx_layout_is_vert` / `wlx_layout_main_extent` replace the repeated
  "linear and HORZ / VERT" tests and main-extent selects in the layout core
  and the debug shadow, and `wlx_layout_slot_is_content` answers the
  CONTENT-slot question for both the intrinsic-width gate and the debug
  validator (same bounds, one definition).
- Intrinsic-width prologues share one padding macro and measure through
  the perf hook. `WLX_INTRINSIC_PAD_LR(ctx, opt)` replaces the five-field
  `wlx_intrinsic_pad_lr` spread at the seven widget sites;
  `wlx_intrinsic_text_width` now routes through `wlx_measure_text_slice`
  (so intrinsic traffic shows up in the text-measure perf counter) and the
  image / glyph-row intrinsics take an explicit length, so each widget in
  a CONTENT slot runs one `strlen` instead of two. The tooltip measures
  its text with `wlx_measure_text_slice` directly instead of borrowing the
  intrinsic helper (also one `strlen`), and the toggle's track-ratio
  fallback is the named `WLX_TOGGLE_TRACK_RATIO_FALLBACK`. No behavior
  change; intrinsic values and measure traffic are unchanged (the
  per-widget measure style stays measure-only).
- One backend perf scaffold. The three adapters' byte-equivalent perf
  machinery (an identical 22-field frame prefix, the begin/end/inc/timing
  helper set, and the per-callback `#ifdef WLX_PERF` timing sandwich at
  45+ sites) now builds on core-defined pieces:
  `WLX_PERF_BACKEND_COMMON_FIELDS`, `WLX_Perf_Backend_Clock` with
  `wlx_perf_backend_*` helpers (pinned by clock unit tests under
  `make perf-test`), and `WLX_PERF_SCOPE_BEGIN/END` macros that are no-ops
  without `WLX_PERF` - and that turn a timing scope opened but never
  closed (the dropped-`time_end` bug class) into a compile-time warning.
  Adapters keep their frame extras, their timestamp source and thin typed
  wrappers; every frame field name is unchanged, so
  `demos/gallery_perf.h` compiles without edits. ~290 lines lighter
  across the three adapters; the bare-WASM perf state gains the `g_`
  prefix its siblings use. No behavior change.
- Key and cursor maps carry compile-time completeness tripwires. The Raylib
  key map flips to WLX -> platform (`wlx_raylib_key_for`, reading side by
  side with SDL3's table) as a switch with no `default`, and the build now
  runs with `-Werror=switch`, so a `WLX_Key_Code` added without a mapping
  fails the build; the per-frame scan drops from 350 platform codes to
  `WLX_KEY_COUNT`. Each adapter's `set_cursor` carries a
  `_Static_assert(WLX_CURSOR_COUNT == 2, ...)` (the WASM one points at the
  JS host's map). SDL3 and the web host no longer split a UTF-8 sequence
  at the 31-byte `text_input` cap (a codepoint that does not fit is
  dropped whole; Raylib already appended per codepoint). API_REFERENCE
  gains the per-adapter optional-callback capability table.
- Three shared utilities hoisted into the core: `wlx_buf_reserve` (the
  grow-and-reuse flat buffer the Raylib and SDL3 clipboard transports and
  the test mock each re-rolled verbatim, now with the house overflow
  guard), `wlx_utf8_floor` (the continuation-byte back-off loop, five
  sites across the core and adapters; bounded back-offs keep their own
  loops), and `wlx_hash_fnv1a64` (the FNV-1a 64 both measurement caches
  defined locally). The bare-WASM clipboard's first fetch now honors a
  `WLX_WASM_CLIPBOARD_MAX` override smaller than its 4096-byte seed. New
  `tests/test_core_utils.c` pins all three helpers directly. No behavior
  change.
- Adapters copy slices through the core's `WLX_CStr_Tmp`. The Raylib slice
  measure, the Raylib advances walk and the SDL3 debug-font slice draw each
  hand-rolled the stack-or-heap NUL-copy discipline (three different stack
  caps); all three now use the core helper, and Raylib wires a
  `draw_text_slice` built on it - the `WLX_Backend` "all in-tree backends
  implement the slice pair" note is now true (the copy simply moved from
  the core fallback into the adapter). No behavior change.
- Typing-focus release and blur each exist once.
  `wlx_interaction_release_focus_holder` serves both frame-begin release
  sites (press outside the holder, Tab away) and now clears the holder's
  Tab claim in both - previously the press-outside path left
  `active_consumes_tab` stale-true until some widget re-acquired
  `active_id`, harmless only because the Tab gate also checks the holder;
  a new test pins that the claim never outlives the holder.
  `wlx_interaction_blur` serves the three blur edges (bootstrap outside
  press, Enter, Escape) in `wlx_interaction_handle_focus`.
- `wlx_begin` is a sequencer again. The frame-begin arbitration that had
  grown inside it (owner / cursor walk, cursor push, hover rule, left and
  right press latches, ring clears, Tab traversal) moved to
  `wlx_frame_arbitrate` as a pure move, and the Tab ring's next stop is one
  function, `wlx_focus_next_stop` (two linear passes, `size_t` indices -
  the old three-pass modular walk with `long` indices is gone), pinned by
  six direct unit tests on hand-built candidate lists. No behavior change.
- Menu frames embed their resolved `WLX_Menu_Opt`. `WLX_Menu_Frame` (one open
  menu body) now holds `style`, `rect` and `row_cursor` instead of mirroring
  thirteen styling fields by hand; `wlx_menu_frame_push` takes the anchor
  point and the style, computes the panel rect itself and copies the opt by
  assignment, the submenu inherits from `parent->style`, and the three
  closed-menu early-outs share `wlx_menu_closed`. The context's menu stack
  is heap-backed (`WLX_MENU_STACK_MAX` entries, allocated on the first menu
  open, never reallocated, freed in `wlx_context_destroy`) so the frame can
  be defined with the menu implementation; `sizeof(WLX_Context)` shrinks by
  the former inline stack. No behavior change.
- One button face. `wlx_button`, the dropdown face and the menu-button face
  draw through a single `wlx_button_face` (frame, query, hover tint,
  per-side border, box, content) over an already-resolved `WLX_Button_Opt`;
  the popups build that opt with new per-field-group copy initializers
  (`WLX_*_COPY(src)`, kept beside each group's `_FIELDS` / `_DEFAULTS`).
  Popup rows, overlay chrome and the outside-press test share helpers
  (`wlx_popup_row_opt`, `wlx_overlay_chrome_opt`,
  `wlx_popup_outside_press`), the dropdown's list styling is a
  `WLX_Menu_Opt` view like the menu button's, and the three popup magic
  numbers are named (`WLX_POPUP_ROW_HEIGHT_PAD`, `WLX_MENU_DEFAULT_WIDTH`,
  `WLX_MENU_ITEM_PADDING`). Two stream-equality tests pin "dropdown /
  menu-button face == button face". No behavior change.
- Docs aligned with the landed contracts: LAYOUT_MODEL.md / API_REFERENCE.md
  describe interactive containers under topmost-wins (a child button wins
  the press; ADR_029 carries a superseded-by note), the Raylib input section
  no longer claims wheel debouncing, README and the header preamble list the
  popup family / input v2 / focus traversal / cursor callback, the preamble
  LIMITS block lists every overridable cap, WIDGETS.md tables gain the
  dropdown and menu-button intrinsic-width rows and `id` fields,
  API_REFERENCE gains a "Backend - WASM" section and documents the three
  `WLX_*_CLIPBOARD_MAX` caps, and ADR_038-042 carry their implemented
  status. `demos/gallery_wasm.c` (orphan: the WASM gallery builds from
  `gallery.c`) is removed.
- SDL3 adapter: `wlx_sdl3_measure_text`'s debug-font `len == 0` path ends
  its `WLX_PERF` timer before returning (the started sample was dropped).
- `wlx_menu_item`, `wlx_menu_end` and `wlx_submenu_begin` guard an empty
  menu stack with `WLX_HARD_ASSERT` (release-surviving) instead of a plain
  `assert`; `menu_stack[-1]` overlays the candidate buffers, so the old
  guard let an unmatched `wlx_menu_end` corrupt memory in `NDEBUG` builds.
  `test_hard_assert` gains the death case.
- CI now runs on the `dev` working branch (push and pull_request), and a new
  `perf-gate` job runs the editor measure-traffic gate (`make perf-editor`)
  and the `WLX_PERF` test build (`make perf-test`) on every push.
- Docs: the README compile line and a new "Compiler flags" preamble section
  in `wollix.h` document the required initializer-override warning
  suppressions (`-Wno-initializer-overrides` on clang, `-Wno-override-init`
  on gcc with `-Wextra`); the README toggle description no longer claims
  "animated" styling (no motion system exists); `CONTRIBUTING.md` states the
  pre-1.0 contribution policy (issues welcome, PRs after 1.0).

## [0.7.0] - 2026-08-10

The editor release: a new windowed text-editor widget shipping as the
`wollix_editor.h` companion header, a full inputbox editing overhaul
(selection, clipboard, multiline), and a retained text-geometry layer
that drops steady-frame measure traffic to zero.

### Added
- **`wlx_editor` — windowed text editor widget (`wollix_editor.h`).** A
  non-wrapping editor over a caller-owned flat buffer
  (`wlx_editor(ctx, label, buffer, cap, &length, ...)`): only the visible
  window of lines is measured and drawn, so frame cost is O(viewport) up
  to the 10 MB / 1,000,000-line envelope. Context-owned per-widget line
  index, `(first_line, y_frac)` scroll anchor, exact vertical scrollbar,
  horizontal bar + Shift+wheel, caret-follow on both axes, the full
  caret/selection/editing vocabulary (PageUp/PageDown, Ctrl/Cmd+Home/End,
  literal Tab with tab-stop expansion via `.tab_columns`), an optional
  line-number gutter, and `.read_only`. Per-line geometry budget
  `WLX_EDITOR_MAX_LINE_UNITS` (default 1024). New demo `demos/editor.c`.
- **`wlx_editor` wrapped mode (`.wrap`).** Opt-in per widget: hard lines
  break into band-wide rows at the same envelope with no O(document)
  measure work — anchor-relative row geometry (no stored row count),
  row-based caret/selection/hit-testing with a sticky column, a
  structural bottom clamp, and a vertical thumb that maps hard lines.
  Runtime-toggleable; byte-offset caret and selection survive switches.
- **Retained editor line geometry.** Each touched line's measured unit
  advances live in a context-owned, LRU-bounded per-id store: idle and
  post-scroll frames issue zero line measures, typing re-measures only
  the edited line; eviction costs traffic, never geometry. Benches:
  Raylib no-wrap idle 172 -> 6 ms, SDL3 horizontally-scrolled steady
  state 1111 -> 8 ms per frame.
- **`measure_text_advances` backend primitive.** Optional `WLX_Backend`
  callback fills one run's cumulative advances at core-supplied unit
  ends in chunked requests (`WLX_TEXT_ADVANCES_CHUNK`, default 256), so
  a retained line's geometry costs O(line) backend work; the per-unit
  measuring walk stays the parity-tested fallback. All three adapters
  implement it (SDL3 needs SDL_ttf >= 3.3.0). Measured: prose cold build
  3,821 -> 21 measure calls, typing frames Raylib ~21 -> ~7 ms and
  SDL3 ~30 -> ~4 ms.
- **Windowed horizontal origin: giant single-line documents are editable
  end-to-end.** In no-wrap mode line geometry re-enters a line at a
  measure origin near the view once the unit budget cannot reach it from
  the line start — the budget is a per-window cap, not a reach limit.
  END on a 300 KB single-line document reaches the true end (~2.46M px,
  previously frozen at ~7.4K px) and per-frame cost is independent of
  scroll depth (asserted by the perf gate). Far jumps onto unmeasured
  content estimate the origin's x from the measured average advance — a
  documented x-space approximation; byte offsets stay exact. New suite:
  `tests/test_editor_windowed_origin.c`.
- **Editor measure-traffic perf gate.** `make perf-editor` counts backend
  measure calls and bytes per frame class (cold, idle, scroll, typing,
  END on a giant line) through a deterministic counting backend, bounded
  at recorded baselines +15%, so traffic regressions trip exactly even
  on a loaded machine.
- **Multiline input (`.multiline` / `wlx_textarea`).** Enter inserts
  `\n` and keeps focus; UP/DOWN cross hard and wrapped visual lines with
  a sticky column; content taller than the field scrolls internally with
  caret-follow, wheel capture, a draggable scrollbar
  (`.show_scrollbar`), and drag-select auto-scroll. Composes with
  `.password` (forces multiline off) and `.read_only`. Own budgets:
  `WLX_INPUTBOX_MULTILINE_MAX_UNITS` / `_MAX_LINES`.
- **Inputbox text-editing overhaul** (all UTF-8 codepoint safe): forward
  DELETE, held-key auto-repeat, Ctrl/Alt word motion, HOME/END on the
  visual line, full mouse selection (click/drag, double-click word,
  triple-click all, SHIFT+click), selection highlight
  (`selection_color`), clipboard command+C/X/V/A, `.password` (masked
  render, copy/cut suppressed), and `.read_only`.
- **Core input + focus contract extension (all three backends).** New
  keycodes `WLX_KEY_DELETE/HOME/END/PAGE_UP/PAGE_DOWN`;
  `keys_repeated[]` (OS auto-repeat) and a `modifiers` bitfield with
  `wlx_is_key_actuated` / `wlx_mod_down` / `wlx_mod_command_down`; new
  `WLX_INTERACT_FOCUS_HOLD_ENTER` flag; Escape blurs any focused field;
  keyboard activation is gated while another widget owns `active_id`.
- **Clipboard transport.** Optional `WLX_Backend.clipboard_get/set`
  hooks (NULL = safe no-op) wired for Raylib, SDL3, and bare WASM
  (best-effort cached), plus public `wlx_clipboard_set_text` /
  `wlx_clipboard_get_copy` with UTF-8-boundary truncation.
- **Docs: `docs/EDITOR_MODEL.md` and `docs/TEXT_PIPELINE_MAP.md`.** The
  editor's windowed-text model split out of LINE_RUN_MODEL.md (which
  stays canonical for the shared pipeline), plus a code map of the text
  pipeline with a glossary, comment conventions, and mermaid diagrams.

### Changed
- **`wlx_editor` moves to the companion header `wollix_editor.h`.**
  Migration: add `#include "wollix_editor.h"` after `wollix.h` in every
  TU that uses `wlx_editor`; it expands under the same
  `WOLLIX_IMPLEMENTATION`, and everything else is source-compatible.
  `wlx_inputbox` / `wlx_textarea` and the text pipeline stay in the core.
- **Raylib default bitmap font renders with natural inter-glyph
  spacing.** The adapter derives an effective spacing for
  `WLX_FONT_DEFAULT` (whose glyphs store no advances):
  `font_size / baseSize`, min 1 px, added to `style.spacing` — so
  `spacing = 0` means natural backend spacing at every size instead of
  edge-to-edge glyphs. Loaded fonts are unchanged.
- **One text-edit key vocabulary and one selection-highlight loop for
  the inputbox and the editor.** The inputbox gains Ctrl/Alt word
  deletes; sticky-column invalidation is on-change-only everywhere; both
  widgets draw selections through the tab-aware prefix-difference
  measure (sub-pixel span shifts possible on shaping backends). Password
  fields keep the hard copy/cut gate, pinned by a negative test.
- **Tab-heavy editor documents stop re-measuring tab segments every
  frame.** The window draw replays tab presence and segment x positions
  from retained line geometry when an entry covers the drawn record;
  uncovered records keep the byte-scan-and-measure path.
- **Editor perf gate: relative edit-frame envelope.** The worst-case
  edit check bounds against an in-process raw byte-work floor (one
  full-buffer memmove + one newline scan) instead of an absolute
  wall-clock bound, so it holds on loaded machines; a loose 50 ms
  absolute backstop remains.

### Removed
- **The v0.6 deprecated field aliases.** As announced in the v0.6.0 notes
  (aliases kept for one minor version), the anonymous-union aliases are
  gone: `wlx_widget` / `wlx_separator` `.color` (use `.back_color`) and
  `wlx_slider` `.show_label` (use `.show_value`). Initializers still
  using the old names now fail to compile; rename the field.

### Fixed
- **End-of-line caret on long no-wrap lines was unreachable.** The
  horizontal scroll limit clamped short of the caret margin, so the
  caret at the end of any over-wide line was culled one frame after
  typing stopped. `wlx_editor_h_extent` now reserves the margin, shared
  by the scroll limit and both thumb-range computations.
- **Dashboard/gallery Raylib caret drift after typing.** The demos'
  font-size-scaling text shims did not wrap the new
  `measure_text_advances`, so retained geometry used nominal sizes while
  text drew scaled. The shims scale it now, and the `WLX_Backend`
  contract documents that decorated backends must wrap all four text
  callbacks together.
- **Column-0 caret no longer clips, vanishes, or jumps to the row end.**
  The wrapped caret measured an empty prefix that SDL_ttf's "length 0
  means NUL-terminated" convention turned into a whole-row width;
  zero-length slices now measure empty and column 0 short-circuits. The
  shared caret draw clamps the caret body inside the clip on every
  backend, and SDL3 `draw_line` now honors `thick` for axis-aligned
  lines (carets, dividers).
- **Far-jump origin gap probe capped at four unit budgets.** A single
  jump deep into a giant line walked the entire unmeasured gap counting
  units; past the cap the estimate goes byte-proportional, inside the
  documented x-space approximation class.
- **Editor caret-follow fires on anchor-only keyboard changes** (e.g.
  select-all with the caret parked offscreen).
- **Editor label honors the vertical component of `.align`** (was
  hardcoded top-left).
- **Editor horizontal scrollbar releases the max-seen width when the
  widest line is deleted**, re-growing it from the visible window on
  every index rebuild instead of staying inflated forever.

### Internal
- Memory-write entry guards stay active in release (NDEBUG) builds:
  `WLX_HARD_ASSERT` on the editor/inputbox buffer contracts and size_t
  overflow guards on scratch and store growth multiplies.
- Wrapped-mode frames run 2.5-4x faster on heavily wrapped documents
  (frame-local row-count memo; tab precheck hoisted out of the build
  loop's prefix measures), and wrapped row counts answer directly from
  complete retained entries, skipping the streaming recount.
- UTF-8 stepping consolidated on `wlx_text_unit_next` (malformed bytes
  as one-byte units, pinned by tests) and tab stops on
  `wlx_tab_stop_next`; the inputbox and editor share one key handler,
  mouse-gesture driver, thumb-drag gesture, and caret/thumb draw
  helpers; both frame functions decomposed into named phase helpers
  (editor 807 -> 249 lines, inputbox 464 -> 116), behavior-identical.
- Consistency batches V06/V07: shared text-widget constants carry the
  neutral `WLX_TEXT_*` prefix, the retained-geometry store renamed to
  the `wlx_text_geom_*` family with its policy constants named, dead
  constants and typos swept.
- `WLX_SDL3_TEXT_CACHE_CAP` stays 4096, now with a permanent sizing
  rationale (must hold one frame's distinct measured strings; 1024
  thrashed the LRU into 5x slower frames).
- The inputbox builds its line records once per frame into a shared
  scratch (previously 2-5 rebuilds), and the wrap-mode scrollbar-width
  decision predicts from last frame's visibility, saving a second build
  per steadily overflowing frame.

## [0.6.0] - 2026-06-28

### Changed
- **Design-system docs renamed and re-scoped.** `docs/DESIGN_SYSTEM.md` is now
  the canonical design reference: the core `WLX_Theme` contract folded in as a
  foundational section ahead of the "Mechanical Glass" showcase token model and
  recipes (the content formerly in `docs/DASHBOARD_DESIGN_SYSTEM.md`, now
  retired). The gallery's application-local layer (`Gallery_Semantic_Theme`,
  `Gallery_Semantic_Spacing`, Brand) moved to `docs/GALLERY_DESIGN_SYSTEM.md`.
- **Dashboard is the primary demo across build and docs.** `make` / `make all`
  now also build `demos/dashboard/dashboard`; the README leads with a dashboard
  screenshot linked to the live demo and presents the gallery as a clearly
  labeled secondary demo.

### Internal
- CI builds the dashboard (raylib + perf) and `dashboard_sdl3` + `gallery_sdl3`,
  and a new `build-wasm` job compiles the published site via `make pages-site`.
- **GitHub Pages now publishes both showcases.** A new `make pages-site` target
  assembles the dashboard at the site root and nests the gallery under
  `dist/wasm-demo/gallery/`, so the live URLs are the dashboard at
  `https://dberzins.github.io/wollix/` and the gallery at
  `https://dberzins.github.io/wollix/gallery/`. The Pages workflow auto-deploys
  this tree on path-filtered pushes to `main` while keeping the manual
  `workflow_dispatch` run.

### Changed (Breaking) — v0.6 coordinated API group (V05 R9b)

One release, four changes. Renamed fields keep a deprecated alias of the same
storage (an anonymous-union member) for **one minor version after 0.6**, so
existing designated initializers compile unchanged; migrate before 0.7.

- **`wlx_widget` / `wlx_separator`: `.color` -> `.back_color`.** The fill
  field now matches every other widget. Migration: rename the initializer;
  `.color` still compiles via the deprecated alias.
- **`wlx_slider`: `.show_label` -> `.show_value`.** The field always toggled
  the numeric value readout, never the label text; the name now says so.
  Migration: rename the initializer; `.show_label` still compiles via the
  deprecated alias.
- **`wlx_panel`: `border_width` unset sentinel is `-1` + theme inheritance.**
  An unset panel border now inherits `theme->border_width` (and a `{0}`
  `border_color` inherits `theme->border`), matching widget resolution -
  previously a panel co## [uld never inherit the theme border. All bundled theme
  presets ship `border_width = 0`, so default rendering is unchanged with
  stock themes; under a custom theme with a non-zero `border_width`, panels
  that relied on default-no-border must now pass an explicit
  `.border_width = 0`.
- **`wlx_inputbox` returns *changed*, not *focused*.** The return value is now
  "the buffer text was mutated this frame" (insert or delete; cursor-only
  movement does not count), which is what callers almost always want. Focus
  state - the old return value - is available through the new
  `.out_focused = &flag` out-param. Migration:
  `bool focused = wlx_inputbox(...)` becomes
  `bool focused; wlx_inputbox(..., .out_focused = &focused)`.

### Added
- **Interactive containers (`interact` / `interact_out`).** Container begin opts
  (`WLX_Layout_Opt`, grids, and `WLX_Panel_Opt` via `WLX_CONTAINER_DECOR_FIELDS`)
  gain `uint32_t interact` (a `WLX_Interact_Flags` mask; `0` = non-interactive,
  the default and zero-overhead) and `WLX_Interaction *interact_out` (optional).
  When `interact != 0`, `wlx_layout_begin` resolves the interaction on the
  container rect (folding in the container's scope id) **before** recording its
  chrome, writes the raw `WLX_Interaction` through `interact_out`, and applies
  hover-variant colors with replace semantics: new `hover_back_color`,
  `hover_border_color`, and `hover_border_color_top/right/bottom/left` twins each
  replace their base color while hovered (a `{0}` twin keeps the base). Color-only
  - there is no hover-variant width/gradient/shadow/glow; an *appearing* border is
  a constant width with a transparent base color toggled to an accent twin.
  Supported flags are `HOVER`, `CLICK`, `KEYBOARD` (`FOCUS`/`DRAG` are unsupported
  on containers; the auto-counted begins lack a stable call-site id and do not
  support `interact`). A `CLICK` container is queried before its children and
  captures the press first (first-writer), so keep interactive containers to
  composite regions of non-interactive content. Existing containers are bit-for-bit
  unchanged (`interact = 0`). Covered by `tests/test_interactive_container.c`. See
  `docs/LAYOUT_MODEL.md` § Interactive containers.
- **List clipper for virtualized rows, plus opt-in offscreen command culling.**
  `wlx_list_clipper_begin` / `wlx_list_clipper_end` (with helpers
  `wlx_list_clipper_height` and `wlx_list_clipper_item_height`) build only the
  rows of a long list that fall within the enclosing scroll panel's viewport,
  reserving the off-screen extent with empty spacers so the scrollbar stays
  correct. Fixed pitch or variable heights (a prefix-sum `item_offsets` array) are
  both supported, plus an `overscan` band. New `wlx_get_scroll_panel_offset`
  exposes the innermost panel's scroll offset to pair with
  `wlx_get_scroll_panel_viewport`. Rows outside the returned `[first, last)` are
  not produced (no ids, interactions, or persistent state that frame) - keep
  stateful or interactive widgets out of virtualized rows, or widen the range with
  `overscan`. Separately, `wlx_set_cull_offscreen(ctx, true)` (default off) makes
  the deferred recorder skip rect-bounded draw commands (rect, rect-lines, rounded
  fill/lines, vertical gradient, texture) that lie fully outside the active clip,
  trimming command-buffer size and backend replay; output is unchanged because
  only provably-invisible commands drop, and immediate mode is unaffected. Covered
  by `tests/test_list_clipper.c` and `tests/test_offscreen_cull.c`. See
  `docs/WIDGETS.md` and `docs/API_REFERENCE.md`.
- **Per-corner rounding (`rounded_corners`):** a new `int rounded_corners`
  decoration field on every bordered widget and container (`WLX_BORDER_FIELDS` /
  `WLX_CONTAINER_DECOR_FIELDS`, default `0`) selects **which** corners use the
  resolved `corner_radius` / `roundness`. It is a `WLX_Corner` bitmask
  (`WLX_CORNER_TOP_LEFT` ... `WLX_CORNER_BOTTOM_LEFT`) with the convenience masks
  `WLX_CORNERS_ALL` / `_TOP` / `_BOTTOM` / `_LEFT` / `_RIGHT`; `0` (unset) is
  treated as all four corners, so existing output is byte-for-byte unchanged.
  Corners absent from the mask are squared off by overdrawing their corner box
  with the fill color (no command-buffer or backend change). It applies to the
  **solid fill only** - gradient fills and rounded borders stay uniform; use a
  per-side straight border for an edge accent on a squared corner. Honored on the
  shared decor path (containers, grids, panels, scroll panels, and the chrome of
  `widget` / `label` / `button` / `inputbox`); the widget primitives that draw
  their own rounded shapes accept but ignore it. Covered by
  `tests/test_corner_radius.c`. See `docs/WIDGETS.md` § Per-corner rounding.
- **Scroll panel transparent background:** `WLX_Scroll_Panel_Opt` gains
  `bool transparent_background` (default `false`). When `true`, the panel draws no
  fill so the container behind it shows through - distinct from `back_color = {0}`,
  which resolves to the theme background. Existing panels are unchanged.
- **`wlx_button` per-call hover override:** `WLX_Button_Opt` gains
  `hover_brightness` (default `WLX_FLOAT_UNSET` -> `theme->hover_brightness`) and
  `hover_back_color` (default `{0}`), mirroring `wlx_toggle` / `wlx_radio` /
  `wlx_slider`. A non-zero `hover_back_color` replaces the fill while hovered;
  otherwise the brightness path applies. Inert by default, so existing buttons
  render unchanged.
- **`wlx_inputbox`: inner icon support.** `WLX_Inputbox_Opt` gains the
  image-capable fields (`texture`, `texture_src`, `texture_tint`,
  `image_placement`, `image_size`, `image_text_gap`) so a leading (`LEFT`,
  default) or trailing (`RIGHT`) icon renders **inside** the field frame instead
  of as a detached sibling widget. The icon is centered vertically independent of
  `align`, and the reserved band insets the text and caret so they never overlap
  the glyph. A zero `texture` keeps the field text-only with byte-identical
  geometry, so existing call sites are unaffected. The dashboard top-bar search
  now uses this to put the magnifier glyph inside the box (via a new
  `dashboard_icon_inputbox` demo helper).
- **Dashboard showcase ("Mechanical Glass") -- the canonical cross-backend demo
  (ADR_026):** `demos/dashboard/` is a single-translation-unit, demo-local
  dashboard that builds and runs on raylib, SDL3, and bare WASM. A shared top bar
  and sidebar route between distinct, persistent views -- Overview, Tokens,
  Components, Layouts, Theme Lab -- through a dashboard-local section dispatch
  table. The Overview shows library-honest metrics (backend FPS, and in a
  `WLX_PERF` build the draw-call count and wollix arena memory, plus desktop
  process RSS where measurable), and reference links open real repository/doc
  URLs through a per-backend open-URL hook (raylib `OpenURL`, SDL3 `SDL_OpenURL`,
  a `window.open` shim on WASM). Tokens/Components/Layouts/Theme Lab demonstrate
  the full public widget and layout surface (label, button, checkbox, toggle,
  radio, slider, progress, input, image, widget; linear/grid/flex/auto layout,
  opacity stack, ID stack, borders, scroll panel; tokens, theme presets, and a
  component-state matrix). The core (`wollix.h`) and backend headers are
  unchanged; the only host addition is an additive `window.open` shim in
  `web/wollix_wasm.js`. `make dashboard` / `dashboard_sdl3` / `dashboard_perf`
  build the desktop binaries. See
  `docs/dev/ADR_026_DASHBOARD_REPLACES_GALLERY.md` and
  `docs/DASHBOARD_DESIGN_SYSTEM.md`.
- **API hygiene, additive (V05 R9a):**
  - `WLX_SCROLL_AUTO_HEIGHT` names the `-1` auto-height sentinel for
    `wlx_scroll_panel_begin`; all in-tree call sites converted.
  - `wlx_rect_contains(WLX_Rect, float, float)` is the new canonical
    float-precision point-in-rect query; `wlx_point_in_rect` remains as an
    int-based shim, and internal hit tests now route through the new query.
  - `WLX_Panel_Opt` now embeds the full container decor set
    (`WLX_CONTAINER_DECOR_FIELDS` + `WLX_SLOT_DECOR_FIELDS`): panels gain
    `rounded_segments`, shadow/glow/gradient, and per-slot decor, forwarded to
    the panel body layout. All new knobs default to off; existing panels
    render unchanged (panel `border_width` keeps its 0-as-unset convention).
  - `wlx_split_next` now goes through `wlx_default_split_next_opt`, aligning
    it with the default-opt pattern.
  - `WLX_SHORT_NAMES` gains `image`, `slot_style`, `grid_cell_style`,
    `push_id`/`pop_id`, `push_opacity`/`pop_opacity`.
  - Docs: a "Widget Return Semantics" table in API_REFERENCE.md (clicked vs
    changed vs focused), and the unset-sentinel rule is written down next to
    `WLX_FLOAT_UNSET` in the header.
- **`WLX_HARD_ASSERT` - memory-safety guards survive release builds (V05 R1):**
  guards whose failure would corrupt memory no longer compile out under
  `NDEBUG`: scissor-stack push, the CONTENT-slot count in layout begin/end,
  persistent-state size collisions in `wlx_get_state_impl`, and every internal
  allocation result now abort with a `wollix fatal:` message instead of writing
  out of bounds or dereferencing NULL. Exception: a layout whose slot count
  exceeds `WLX_CONTENT_SLOTS_MAX` with CONTENT slots present clamps in release
  (warns on stderr and disables CONTENT tracking for that layout - CONTENT
  slots fall back to min size) rather than aborting. The assert-vs-hard-assert
  policy is documented in the header preamble (ASSERTION POLICY). Covered by
  the new `tests/test_hard_assert.c` NDEBUG death-test binary, wired into
  `make test`.
- **Layout over-allocation diagnostic (`WLX_DEBUG`):** a linear layout whose slot
  sizes resolve to a boundary past its own extent (so the trailing slot overflows
  onto its neighbor) now emits a one-time warning through the existing debug warn
  channel (`warn_count` / `warn_cb`). The warning is suppressed when the layout or
  an ancestor clips (the overflow is contained) and is deduplicated per call-site.
  Zero footprint in release builds. Covered by `tests/test_slot_redistribute.c`.
- **Absolute pixel corner radius (`corner_radius`):** an opt-in `float
  corner_radius` decoration field on every bordered widget and container
  (`WLX_BORDER_FIELDS` / `WLX_CONTAINER_DECOR_FIELDS`, default `0`) declares the
  corner radius in **pixels**, where `roundness` is a fraction of the shorter
  side. `corner_radius > 0` overrides `roundness`; it is resolved centrally at
  draw time inside `wlx_draw_box` to the fraction the backends already consume
  (`clamp(2 * corner_radius / min(w,h), 0, 1)`), so two differently sized
  elements share the same pixel corner and the recorded command still carries a
  fraction - no command-buffer or backend change. Existing callers are
  bit-for-bit unchanged (`0` = unset). v1 honors it on the decor path
  (containers, grids, panels, scroll panels, and the chrome of `widget` /
  `label` / `button` / `inputbox`); widget primitives that draw their own
  rounded shapes (`checkbox`, `slider`, `progress`, `scrollbar`, `radio`) accept
  but ignore it. See `docs/dev/ADR_025_ABSOLUTE_CORNER_RADIUS.md`.
- **First-class vertical gradient fill:** a two-stop vertical gradient is now a
  decoration field on every bordered widget and container. New
  `WLX_GRADIENT_FIELDS` (`gradient_top`, `gradient_bottom`) embed in
  `WLX_BORDER_FIELDS` and `WLX_CONTAINER_DECOR_FIELDS`; both zero-initialize to
  "disabled" (`gradient_top = {0}` is the enable gate, a zero `gradient_bottom`
  resolves to `gradient_top` for a uniform fill), so existing callers are
  bit-for-bit unchanged. `wlx_draw_box` emits the gradient **in place of** the
  solid fill, after soft effects and before borders, via a new command type
  `WLX_CMD_GRADIENT_V` (public `wlx_cmd_record_gradient_v`); the
  `wlx_draw_layout_decor` gate fires for a gradient-only container. At replay
  (and in immediate mode) the optional `WLX_Backend.draw_gradient_v` callback
  renders natively (Raylib: `DrawRectangleGradientV` sharp, rounded-band
  approximation for rounded rects) else a software fallback slices the rect into
  `max(1, rect.h / 4)` solid bands interpolating the stops. Widget-path stops
  carry the effective opacity (container stops pass through like `back_color`);
  `WLX_PERF` counts one command per element. New `wlx_color_lerp` helper; v1 is
  vertical two-stop only (ADR_023). Covered by `tests/test_gradient.c`. See
  `docs/WIDGETS.md` § Gradient fill fields.
- **Opt-in layout clipping:** `WLX_Layout_Opt` gains `.clip` (default `false`).
  When set, `wlx_layout_begin` clips its children to the post-padding content
  rect by beginning a scissor at begin and releasing it at end. The clip rect is
  intersected with the active clip before installing, so a `clip` layout inside a
  scroll panel (or another clip layout) never widens the visible region. The
  container's own chrome (background/border/glow/shadow) is recorded before the
  scissor and is never cropped; a child's effects are inside the clip and are
  cropped to the parent content rect. `wlx_panel_begin(.clip = true)` now routes
  through this shared path, and `wlx_layout_end` is the single owner of the
  scissor release (`wlx_panel_end` no longer ends the scissor itself); panel clip
  output is unchanged. Backends without `begin_scissor`/`end_scissor` no-op the
  recorded commands at replay. Grids do not expose `clip`; wrap a grid in a
  `clip` layout for cell containment.
- **Segmented progress self-containment:** `wlx_progress_cell_rect` now keeps
  every segmented cell inside its track at all widths. The inter-cell gap is
  compressed (down to 0) when the nominal gaps would fill the track, cell widths
  keep a 1px floor, and a trailing cell that still cannot fit is clamped to the
  track's right edge (collapsing to zero width) instead of drawing past it. The
  minimum segmented bar width drops to `segments` px (1px per cell, gap 0). This
  is an internal geometry fix; no public API or option change.
- **First-class glow and shadow:** a soft outer glow and a soft drop shadow are
  now decoration fields on every bordered widget and container. New
  `WLX_GLOW_FIELDS` (`glow_color`, `glow_spread`, `glow_rings`) and
  `WLX_SHADOW_FIELDS` (`shadow_color`, `shadow_offset_x`, `shadow_offset_y`,
  `shadow_blur`, `shadow_layers`) embed in `WLX_BORDER_FIELDS` and
  `WLX_CONTAINER_DECOR_FIELDS`; all fields zero-initialize to "effect disabled"
  (non-zero color is the enable gate), so existing callers are bit-for-bit
  unchanged. Each numeric knob resolves `<= 0` / `== 0` to the new
  `WLX_Theme.shadow` / `WLX_Theme.glow` sub-structs, then to a hard fallback
  (`8` px blur / `4` layers / `4` px spread / `3` rings); built-in presets need
  no edit. `wlx_draw_box` emits the effects before the element fill via two new
  command types `WLX_CMD_SHADOW` / `WLX_CMD_GLOW` (recorded with the public
  `wlx_cmd_record_shadow` / `wlx_cmd_record_glow`). At replay (and in immediate
  mode) the optional `WLX_Backend.draw_shadow` / `draw_glow` callbacks render
  natively when set, else a software fallback draws layered offset rounded rects
  / concentric expanding rounded rings; all in-tree backends use the fallback
  this cut. Effect colors flow through the same effective-opacity premultiply as
  fill/border. Emission is wired for `wlx_widget` / `wlx_label` / `wlx_button` /
  `wlx_checkbox` / `wlx_inputbox` / the scroll panel and the layout/grid
  container decor path. `wlx_slider` and `wlx_toggle` render the effect behind
  their thumb; continuous `wlx_progress` renders it behind the track and
  segmented `wlx_progress` behind the bounding run of filled cells. The
  dashboard demo's `dashboard_draw_glow` / `dashboard_draw_shadow` now delegate
  to the new recorders, and `dashboard_segmented_progress` expresses its neon
  run-glow through the `wlx_progress` glow options instead of a manual emitter.
- **`wlx_progress` segmented mode:** `WLX_Progress_Opt` gains `.segments`
  (`0` = continuous, the default; `> 0` = discrete cells) and `.segment_gap`
  (pixel gap between cells; `<= 0` resolves to the theme default). A new
  `WLX_Theme.progress.segment_gap` field supplies that default, falling back to
  `2px` when zero (built-in presets need no edit). In segmented mode the bar
  draws `round(value * segments)` filled cells in `fill_color` and the rest in
  `track_color`, with equal floor-rounded widths (the last cell absorbs the
  remainder) and transparent gaps - no continuous track box or border. The
  continuous path is unchanged for `segments == 0`. The dashboard demo's
  `dashboard_segmented_progress` helper now delegates its cells to
  `wlx_progress` (keeping its neon glow) and the demo-local
  `dashboard_segments_filled` helper is retired.
- **`wlx_label` aggregate text style and vertical metric:** `WLX_Label_Opt`
  gains an optional `.style` (`WLX_Text_Style`) and `.vertical_metric`
  (`WLX_Vertical_Metric`). When `.style.font_size > 0` the aggregate
  overrides the individual `font`/`font_size`/`spacing`/`front_color`
  fields (a fully-zero `style.color` falls back to the resolved
  `front_color`); otherwise existing per-field behavior is preserved.
  `.vertical_metric` defaults to `WLX_VMETRIC_LINE_HEIGHT` (no change);
  `WLX_VMETRIC_FONT_SIZE` centers fitted text on the font-size em box
  for consistent cap-height placement across backends whose line height
  exceeds the font size. Setting both `.style.font_size > 0` and the
  field `font_size > 0` emits a once-per-site warning under
  `WLX_DEBUG`. The dashboard demo's `dashboard_slot_text` helper now
  uses `wlx_label` with `.style` and `.vertical_metric = WLX_VMETRIC_FONT_SIZE`.
- **Dashboard demo scaffold (Stitch "Mechanical Glass"):** new `demos/dashboard/dashboard.c`
  plus demo-local `demos/dashboard/dashboard_theme.h` stub (included only by that TU,
  extending the `gallery_perf.h` local-header convention). Opens a themed Raylib window,
  clears to the Stitch dark background (`#0f1419`), and renders a placeholder label. New
  `make dashboard` target, wired into `make test-demos`. `wollix.h` and backends unchanged.
- **Vendored fonts:** Geist (Regular/Medium/SemiBold/Bold) and JetBrains Mono
  (Regular/Medium/Bold) under `demos/assets/` with their SIL Open Font License files, for the
  dashboard typography roles.
- **Dashboard token model (dark + light):** `demos/dashboard/dashboard_theme.h` now carries the
  Stitch-derived, dashboard-local design tokens as pure data - color roles + surface-container
  ramp, on-colors, status, table, glass-edge and glow colors, typography roles (family/weight
  intent + size + line-height + integer tracking), a strict 4px spacing scale, a radius scale,
  and per-effect intent - as `dashboard_tokens_dark` / `dashboard_tokens_light`. A runtime
  resolver (`dashboard_font_resolve` / `dashboard_type_font`) maps typography intent to loaded
  Geist / JetBrains Mono handles. New `tests/test_dashboard_tokens.c` token-integrity suite.
  `wollix.h` and backends remain unchanged.
- **Dashboard `WLX_Theme` mapper:** `dashboard_wlx_theme(mode, fonts)` in
  `demos/dashboard/dashboard_theme.h` builds a `WLX_Theme` from the active mode's tokens each
  frame - baseline subset (background, foreground, surface, border, accent, padding, roundness,
  font/size) plus widget overrides for input focus, slider, checkbox, toggle, radio, progress,
  and scrollbar. The dashboard demo now renders a token-driven title and the ordinary widgets
  under the mapped theme, switching dark/light with `M` (or a `light` argv). New
  `tests/test_dashboard_theme.c` mapper suite.
- **Dashboard component recipes:** `demos/dashboard/dashboard_components.h` adds dashboard-local
  components built from existing primitives + the token model - glass module with asymmetric
  edge strokes, technical primary / glass secondary buttons, technical input, status
  chip / badge / pulsing pip, dense data table (header bg, zebra, grid lines), segmented
  progress bar, navigation sidebar, and a responsive 12-to-4 column grid helper. The demo gains
  a components showcase view (`TAB` switches views; a `widgets` argv starts on the widgets view).
  New `tests/test_dashboard_components.c` suite covers the pure geometry/state helpers.
  `wollix.h` and backends remain unchanged.
- **Dashboard Overview screen (Stitch replica):** the dashboard demo's primary view is now a
  faithful rebuild of the Stitch dark "Wollix Explorer | Overview" reference - a top navigation
  bar, a left sidebar (nav links + Deploy Build + footer), and a scrollable main area with three
  hero stat cards (segmented usage bars) and a 12-column grid (Basic Widgets + Structural
  Layouts, a tabbed feature-card panel + a scanline Terminal Logs panel, Integrations + a
  gradient Visual Laboratory). Colors adapt per mode to track both the Stitch dark and light
  references (cyan vs blue accent, light-on-dark vs dark-on-light overlays, and the
  terminal/log treatment), with identical layout and content. Radii, borders, typography roles,
  and the scroll/clip regions track the references. Material-Symbols icons and the remote images
  are drawn as geometric placeholders. `TAB` switches to the ordinary-widgets mapping view; a `widgets` argv
  starts there. New `WLX_DASHBOARD_SHOT=<file>` env captures a screenshot for offline
  verification. `wollix.h` and backends remain unchanged.
- **Dashboard visual effects (primitive-only):** `demos/dashboard/dashboard_effects.h` adds
  layered-primitive approximations - outer glow / highlight, soft shadow, vertical gradient,
  scanline overlay, and a frosted backdrop-blur stand-in (darkened/desaturated translucent
  fill) - plus pure color helpers (lerp, desaturate, blur tint, glow falloff). Wired into the
  demo as module shadows, a page gradient backdrop, an active segmented-bar glow, and a
  dedicated "Visual effects" section. New `tests/test_dashboard_effects.c` suite. A per-effect
  fidelity assessment is recorded under `docs/dev/dashboard/`; true backdrop blur is deferred
  to an ADR-gated core change. `wollix.h` and backends remain unchanged.

### Changed
- **The published web demo and canonical showcase are now the dashboard
  (ADR_026).** `make wasm-site` now packages the bare-WASM **dashboard** into
  `dist/wasm-demo/` (the GitHub Pages artifact), so the demo URL is unchanged but
  serves the dashboard. The gallery keeps building and coexists: `make gallery`,
  `make gallery_sdl3`, and the new `make gallery-wasm-site` (into
  `dist/gallery-demo/`); `make dashboard-wasm-site` is now an alias of
  `make wasm-site`. `docs/DASHBOARD_DESIGN_SYSTEM.md` is now the canonical
  showcase design reference and supersedes `docs/DESIGN_SYSTEM.md`, which remains
  the reference for the core public theme contract.
- **Dashboard Overview elements are now functional, not decorative.** The Overview
  landing's previously static controls gained real, library-honest behavior (all
  demo-local; no core change): the Basic Widgets and Structural Layouts lists and the
  capability cards deep-link into the Components/Layouts sections; the top-bar search
  filters those lists in place; the "Active State"/"Preview" tabs switch and persist
  (Active State hosts live checkbox/toggle/slider/button controls); Terminal Logs now
  renders a live in-app action log (navigation, theme changes, deploy, tab switches)
  in a scroll panel, newest first with timestamps, replacing the hardcoded lines;
  Deploy Build posts a real build sequence to that log; the bell tints to the accent
  while the log holds unread entries and clears on click; Settings and "Launch Lab"
  open the Theme Lab section; and the "Visual Laboratory" panel's fictional
  physics/luminosity copy was rewritten to describe Theme Lab honestly. The identity
  avatar stays intentionally static (no fabricated account).
- **Image-capable `button` / `label` crop an oversized image to the widget instead
  of overflowing it.** Widget drawing is not scissored, so an explicit `image_size`
  (or a fixed image beside text) larger than a squeezed content rect previously drew
  past the control's edge. The image + text path now keeps the image at its requested
  size and crops it to the content rect via a new geometric blit-clip
  (`wlx_clip_textured_blit`: intersect the destination, then narrow the source
  proportionally), clipping the glyph at the edge rather than scaling it down; the
  image-only path sizes through `wlx_get_align_rect`, which already clamps to the
  content rect; an image fully outside the rect draws nothing. Covered by
  `tests/test_button_image.c`.
- **Per-side border forwarding behind one macro (V05 R7, internal):** the
  eight per-side border fields every chrome-drawing widget forwards into
  `wlx_border_sides_for_widget` now go through `WLX_BORDER_SIDES_ARGS(opt)`
  at all seven call sites; adding a per-side field is a one-macro change.
- **Min/max redistribution scratch rides the frame arena (V05 R4, internal):**
  the freeze-and-redistribute pass in the offset solver no longer heap-allocates
  its working buffers when a constrained layout exceeds `WLX_OFFSET_STACK_LIMIT`
  slots. The new context-aware entry `wlx_compute_offsets_ctx` takes one
  combined block from the frame byte-scratch arena (reclaimed by the per-frame
  reset); the ctx-less `wlx_compute_offsets` remains for standalone callers
  with a hard-checked heap fallback. A steady-state frame of a static UI now
  performs **zero** heap allocations, locked in by a new `WLX_PERF` test
  (`perf_steady_state_zero_allocations`). Offset math is unchanged.
- **Label/button content emission unified (V05 R5, internal):** the
  three-branch image/text block (measure, auto-size, layout, fit, draw) and
  the tint/gap resolver tail that were pasted into both widgets now live in
  `wlx_draw_widget_content` / `wlx_resolve_widget_content`. Rendering is
  unchanged; button text keeps line-height centering (the `.vertical_metric`
  cap-height knob remains label-only by decision).
- **Glyph + label row layout unified (V05 R6, internal):** checkbox, toggle,
  and radio share `wlx_layout_glyph_row` for the measure / combined-block /
  align / split sequence. Per-widget row quirks are preserved exactly
  (checkbox: always-reserved label padding, top-aligned glyph, label height
  extending to the content bottom, optional `full_slot_hit`; toggle/radio:
  collapsed empty-label space, vertically centered glyph).
  `WLX_CHECKBOX_LABEL_PADDING_FACTOR` was renamed to the shared
  `WLX_GLYPH_ROW_LABEL_PADDING_FACTOR` (internal constant).
- **Scratch allocation converged on the sub-arena primitives (V05 R3,
  internal):** `wlx_scratch_alloc`, `wlx_dyn_scratch_alloc`, and
  `wlx_scratch_alloc_bytes` are now thin wrappers over `wlx_sub_arena_alloc` /
  `_alloc_bytes`, restoring the alignment power-of-two and `size_t`-overflow
  asserts the byte path had dropped. A new `wlx_sub_arena_extend_to` primitive
  replaces the four hand-rolled reserve/count/high-water sequences in
  `wlx_create_layout_auto`, `wlx_create_grid_auto`, and both dynamic-growth
  paths in `wlx_get_slot_rect`; every bump of the arena invariant now lives
  inside `wlx_sub_arena_*`. No allocation-pattern or layout-behavior change.
- **Slot min/max redistribution is now the default.** When a slot clamps to its
  min or max, its surplus/deficit is redistributed to unfrozen flex/auto siblings
  so the resolved boundary stays at `total`. This changes resolved sizes only for
  layouts that use min/max-constrained slots; constraint-free layouts are
  byte-for-byte unchanged (gated on `has_constraints`). Define
  `WLX_SLOT_SINGLE_PASS_CLAMP` to opt out and restore the legacy single-pass
  clamp. The former opt-in `WLX_SLOT_MINMAX_REDISTRIBUTE` is now a deprecated
  no-op, accepted but ignored for source compatibility. Redistribution cannot
  remove a physical overflow when fixed + min sizes exceed the container; use an
  opt-in layout `.clip` to contain that. See `docs/LAYOUT_MODEL.md` § Redistribute
  Mode.
- **Dashboard chrome on native surfaces:** the dashboard demo no longer hand-places
  `wlx_cmd_record_*` chrome against parent rects. Topbar/sidebar/terminal fills, hairlines,
  and panel outlines moved onto container `.back_color` and (per-side) border decor; icon
  boxes, the terminal window dots, and the status-chip/status-pip discs moved onto
  slot-claiming `wlx_widget`s (square + `roundness = 1.0` renders true discs via the
  box->circle fallback); the badge pill is now a single `wlx_label` `.show_background`; the
  data table expresses its header/zebra fills and grid rules through per-row `.back_color`
  and per-cell borders; the status-pip halo uses first-class `.glow_*`. The unused
  `dashboard_draw_glow` / `dashboard_draw_shadow` / `dashboard_draw_frost` helpers were
  deleted (redundant with first-class glow/shadow decor and container tints); the pure color
  helpers and the scanline draw are retained. The live Overview render is unchanged within
  sub-pixel border-resolver tolerance. The visual-lab image gradient now uses the first-class
  `.gradient_top` / `.gradient_bottom` decor (see Added), so `dashboard.c` is free of raw
  `wlx_cmd_record_*` calls; the scanline overlay is the lone documented demo-local residual.

### Fixed
- **Slice-only backends pass the readiness check (V05 R8):**
  `wlx_backend_is_ready` required the legacy NUL-terminated `draw_text` /
  `measure_text` callbacks even though the slice entry points are the
  documented preferred contract, so a slice-only backend failed the readiness
  assert. Readiness now accepts either form per direction. Also fixed an
  `||`/`&&` precedence slip in `wlx_draw_text_slice`'s immediate-mode assert
  that bound the message to the wrong operand. Covered by the new
  `tests/test_slice_backend.c` suite (readiness matrix plus deferred and
  immediate rendering through the slice callback).
- **Toggle and radio honor `.wrap` (V05 R9a):** both option structs carried
  the wrap field but the draw call hardcoded no-wrap; the field is now passed
  through like checkbox. Default remains `false`, so behavior changes only
  for callers who set it.
- **One-byte out-of-bounds read on the legacy text fallback (V05 R2):**
  `wlx_span_measure_text` and `wlx_draw_text_span_immediate` probed
  `text[len] == '\0'` to skip the temporary copy; for a slice ending exactly at
  the end of an allocation that read one byte past the span. The legacy path
  (backends without `draw_text_slice` / `measure_text_slice`) now always copies
  via a new shared `wlx_cstr_tmp_begin/end` helper, which also replaces the
  three hand-rolled stack-or-heap copy blocks (measure, draw, deferred-replay
  dispatch). No in-tree backend is affected - all implement the slice
  callbacks.

## [0.5.0] - 2026-05-23

### Added
- **Image widget:** `wlx_image` with `WLX_Image_Scale` (`STRETCH`/`FIT`/`FILL`/`NONE`),
  `WLX_Image_Opt` (alignment, tint, source sub-rect, opacity), and `demos/image`.
- **Image-capable button:** `WLX_Button_Opt` gains `texture`, `texture_src`, `texture_scale`,
  `texture_tint`, `image_placement`, `image_size`, `image_text_gap`; new `WLX_Image_Placement`
  enum (`LEFT`/`RIGHT`/`TOP`/`BOTTOM`). New `demos/button_image` demo.
- **Image-capable label:** same image surface as button via `WLX_Label_Opt`. New `demos/label_image` demo.
- **Image-capable checkbox texture mode:** `WLX_Checkbox_Opt` gains `tex_checked_src`,
  `tex_unchecked_src`, `tex_checked_tint`, `tex_unchecked_tint`. Both tints fold through the
  opacity stack; texture mode now requires both state textures and defaults tint to `WLX_WHITE`.
- **Content padding for all widgets:** `WLX_CONTENT_PADDING_FIELDS` (`content_padding`,
  `content_padding_top/right/bottom/left`) added to every widget and chrome container;
  `WLX_PADDING_USE_THEME` sentinel opts into `theme->padding`. Defaults are zero so existing
  call sites are pixel-stable.
- **Disabled-state Phase 1:** `.disabled` option on `wlx_button`, `wlx_checkbox`, `wlx_inputbox`,
  `wlx_slider`, `wlx_toggle`, and `wlx_radio`; `theme->disabled_brightness` and
  `theme->disabled_opacity` knobs; `WLX_DEFAULT_DISABLED_OPACITY` constant (`0.55f`) shared
  across built-in presets. Coverage rule documented in ADR_018 and `docs/WIDGETS.md`.
- **WASM texture support:** `wlx_wasm_texture_create` / `wlx_wasm_texture_destroy` C helpers;
  JS host owns a per-texture `OffscreenCanvas` registry copied at upload time. Full RGB tint
  is supported via a source-space variant cache: primary path uses `ctx.filter feColorMatrix`;
  fallback uses Canvas 2D `multiply` compositing. Alpha applies per-draw via `ctx.globalAlpha`.
  See ADR_017 for the full contract.
- **Gallery Lucide icon atlas (32 icons, four tiers):** `demos/assets/wlx_icons.h` is now a
  576x120 white-alpha atlas at 16/24/32/48 px. `gallery_icon_src_for()` picks the sharpest tier
  at the call-site size. Source SVGs and manifest under `demos/assets/icons/`.

### Changed
- **Breaking:** `WLX_Split_Opt` and `WLX_Panel_Opt` replace `.padding*` fields with
  `.content_padding*`. `WLX_PADDING_FIELDS`, `WLX_PADDING_DEFAULTS`, and `WLX_RESOLVE_PADDING`
  macros removed. Rename `.padding` → `.content_padding` (and per-side variants) to migrate.
- **Breaking:** `pushed_id` fields on `WLX_Widget_Frame` and `WLX_Scroll_Panel_State` renamed to
  `bool pushed_scope`; `pushed_scope_id` on `WLX_Layout` renamed to `pushed_scope`.
- Separator now respects the opacity stack and `theme->opacity` (was silently ignored).
- Gallery spacing roles (`space_shell`, `space_panel`, `space_heading_*`, `gap_dense`, etc.)
  derived from `WLX_Theme.padding` instead of hard-coded literals.
- `wlx_widget_impl` draw rect no longer rounded with `ceilf`; matches all other widget paths.

### Fixed
- Fixed incorrect cast in `wlx_scroll_panel_frame_begin`.

### Internal
- `WLX_Checkbox_Opt` and `WLX_Inputbox_Opt` embed `WLX_BORDER_FIELDS`/`WLX_BORDER_DEFAULTS`
  instead of hand-rolled border fields.
- `wlx_resolve_content_rect_full` + `WLX_RESOLVE_CONTENT_RECT` collapse the common
  padding/clamp/inset three-liner used by label, button, checkbox, slider, progress, toggle, radio.
- `WLX_RESOLVE_VISUAL_STATE` macro unifies resolver tails; `wlx_color_hover_tint` centralizes
  the hover-tint gate across all interactive widgets.
- `wlx_scope_push`/`wlx_scope_pop` unify the `.id` → id-stack push/pop pattern used by every
  frame helper. Under `WLX_DEBUG`, `wlx_end` asserts the id-stack returns to zero each frame.
- `wlx_contribute_to_parent_layout` (`WLX_Parent_Contribution`) consolidates parent
  content-tracking for `wlx_widget_begin`, `wlx_layout_end`, and scroll-panel.
- `bool wrap` split from `WLX_TEXT_TYPOGRAPHY_FIELDS` into `WLX_TEXT_WRAP_FIELDS`; slider
  resolver aligned with shared `wlx_resolve_typography`/`wlx_resolve_border` helpers.

## [0.4.0] - 2026-05-09

### Added
- **Slice-aware text paths:** added public `wlx_measure_text_slice()` and
  `wlx_draw_text_slice()` helpers, optional `WLX_Backend.draw_text_slice` /
  `WLX_Backend.measure_text_slice` callbacks, and slice-first core/replay
  routing while preserving C-string compatibility.
- **Performance diagnostics:** added opt-in `WLX_PERF` core and backend
  snapshots, gallery perf builds with summaries/CSV output, public diagnostics
  docs, and regression tests.
- **Backend text caches:** added retained SDL3 `TTF_Text` caching with font
  variants, flush/lifecycle helpers, and counters; added Raylib text
  measurement caching with cap-zero disable, generation-bump clearing, and
  cache stats.
- **Gallery and design system updates:** added a grouped gallery shell,
  Overview, Semantic Tokens, Theme Lab, Brand theme preview, responsive section
  templates, refreshed gallery assets, and public design-system documentation.
- **Panel and typography options:** added panel body-frame controls on
  `WLX_Panel_Opt` and restored opt-in `spacing` across text style and widget
  typography options.

### Changed
- Reworked fitted text into a UTF-8-safe line/run model with explicit newline
  handling, shared cursor layout, fewer emitted text commands, and private
  builder refactors that keep public APIs stable.
- Updated SDL3 and Raylib text rendering paths for better measurement reuse,
  coordinate snapping, font scaling, and custom-font spacing behavior.
- Improved gallery layout behavior around section splits, theme controls,
  status chrome, semantic colors, and version display.
- Refined content sizing and vertical parent height accounting for more stable
  nested layout behavior.

### Fixed
- Fixed checkbox label width math, slider fill/roundness consistency, missing
  widget ID scopes, Theme Lab padding/preview sizing, and stale gallery version
  display.
- Fixed fitted-text and inputbox clipping so scissor scopes are installed only
  when needed and cursor clipping remains independent.
- Hardened SDL3 text cache teardown, renderer-change invalidation,
  font-variant fallback paths, and text rasterization stability.

## [0.3.0] - 2026-04-26

### Added
- Scoped container IDs: `WLX_Layout_Opt`, `WLX_Grid_Opt`, `WLX_Grid_Auto_Opt`,
  `WLX_Panel_Opt`, and `WLX_Split_Opt` now accept `const char *id`, pushing a
  scope ID for the full container body. `wlx_scroll_panel_begin` /
  `wlx_scroll_panel_end` now do the same for scroll-panel bodies. The identity
  model is now documented in `wollix.h` and `docs/API_REFERENCE.md`, including
  Widget ID / State ID / Scope ID roles and guidance on when
  `wlx_push_id` / `wlx_pop_id` is still needed.

  **Migration note:** callers that both pass scroll-panel `.id` and wrap the
  panel body in manual `wlx_push_id` / `wlx_pop_id` calls will see descendant
  state keys change because the panel scope now stays active for the full body.
- New widgets and styling APIs: added `wlx_progress`, `wlx_toggle`,
  `wlx_radio`, and `wlx_separator`; added rounded backgrounds and borders for
  widgets and containers; and added per-slot / per-grid-cell decoration plus
  one-shot style overrides.
- Theming additions: added built-in `wlx_theme_glass`, theme sub-structs for
  toggle, radio, and progress styling, advisory size/style constants, and
  `WLX_SHORT_NAMES` aliases for `progress`, `toggle`, and `radio`.
- Layout additions: added per-side padding across layouts, panels, grids,
  splits, and scroll panels, plus per-slot `gap` support.
- Backend and web additions: `WLX_Backend` now supports
  `draw_rect_rounded_lines`, plus optional `draw_circle` and `draw_ring`
  callbacks with widget-level fallbacks. Raylib and WASM expose native
  circle/ring support, SDL3 now includes rounded-shape tessellation helpers,
  and the bare-WASM backend/browser host gained optional FPS limiting.
- Demo and docs additions: added `demos/auth.c`, expanded the gallery with
  themed control sections and layout examples, and added `docs/CORE_PATTERNS_GUIDE.md`
  plus `docs/SENTINEL.md`.

### Changed
- `gallery.c` is now the main cross-backend gallery source for Raylib, SDL3,
  and WASM, with backend-specific setup moved into platform layers and the
  current demo layout retuned for a wider sidebar.
- `gallery.c` now derives shared demo chrome from a private gallery semantic
  helper layer, centralizing elevated surfaces, muted text, selection,
  border-emphasis, and status or destructive treatment without expanding the
  public `WLX_Theme` API.
- The same gallery semantic helpers now drive scroll-panel lists, theme-preview
  cards, helper metadata, and progress-state badges so the proving surfaces use
  the private token roles beyond shell chrome.
- Default rendering is now deferred through a per-frame command buffer
  replayed by `wlx_end`; use `wlx_begin_immediate` to opt out. Public examples
  and docs now consistently call `wlx_end(ctx)` before `EndDrawing()` /
  `SDL_RenderPresent(ren)`.
- `WLX_Context` per-frame storage now lives in `WLX_Arena_Pool`, with new
  `wlx_context_init` and `wlx_context_init_ex` entry points plus
  allocator-group configuration.
- Theme presets and demo styling were refreshed across dark, light, and glass
  modes, and toggle/radio label placement now respects the `align` field via
  `wlx_get_align_rect`.
- Text measurement, wrapping, and input/gallery sizing were refined for more
  stable two-pass layout behavior and cleaner demo ergonomics.

### Changed (Breaking)
- **Breaking:** `WLX_Theme.toggle` gained `track_to_height_ratio` (default 0 =
  use 2.0) and `thumb_inset_ratio` (default 0 = use 0.15). `WLX_Theme.radio`
  gained `selected_inset_ratio` (default 0 = use 0.25). `WLX_Theme` gained a
  global `min_rounded_segments` field (built-in presets set to 16) replacing
  the hardcoded `< 16 ? 16` clamp in toggle and radio impls. Custom themes
  that zero-initialize from struct literals gain the new fields at zero, which
  preserves previous visual output.
- **Breaking:** `WLX_Theme` gained a `progress` sub-struct with `track`,
  `fill`, and `track_height` fields. `wlx_resolve_opt_progress` now reads
  from `theme->progress` first and falls back to `theme->slider.track` /
  `theme->accent` only when the progress-specific fields are zero. Custom
  themes that zero-initialize the new sub-struct retain previous visual output.
  Built-in presets (`wlx_theme_dark`, `wlx_theme_light`, `wlx_theme_glass`)
  have been updated with dedicated progress colors.

### Fixed
- Fixed nested CONTENT bootstrap deadlocks by seeding unmeasured slots,
  improving intrinsic height contribution, and including child padding in
  linear parent measurements.
- Fixed overly noisy nested CONTENT debug warnings while preserving
  widget/content-slot diagnostics for real layout issues.
- Fixed gap handling so slot and grid rects no longer paint into visual gap
  space, and dynamic linear/grid paths now apply spacing consistently.
- Fixed dynamic-layout and nested-static-layout contention by moving dynamic
  offsets into a dedicated `dyn_offsets` arena.
- Fixed `wlx_widget` so opacity and hover brightness apply to border color as
  well as fill color.
- Fixed sentinel resolution for negative and float-valued option fields,
  including border width, roundness, scrollbar width, and hover-brightness
  fields.
- Fixed `WLX_Cmd_Range.parent_range_idx` back to `int` with
  `WLX_NO_RANGE == -1`, removing redundant casts and restoring the intended
  internal sentinel handling.
- Fixed WASM host input edge cases, SDL3 multisampling, a dangling-pointer bug
  in `wlx_grid_begin_impl`, content-based debug-warning deduplication, and
  assorted demo sizing issues.
- Fixed documentation drift around backend requirements, sentinel
  conventions, the identity model, and frame-loop ordering.

### Removed
- Deprecated `wlx_is_unset()` removed. Use `wlx_is_negative_unset()` for `-1`
  sentinel fields and `wlx_is_float_unset()` when other negative values are
  valid.
- **Breaking:** Removed `WLX_Grid_Cell_Opt` and `wlx_default_grid_cell_opt`.
  `row_span` and `col_span` now live on `WLX_Slot_Style_Opt` (defaults remain
  `1`), and `wlx_grid_cell()` now takes `WLX_Slot_Style_Opt` directly. Calls
  like `wlx_grid_cell(ctx, r, c, .row_span = 2, .col_span = 2)` still work,
  but named `WLX_Grid_Cell_Opt` variables must migrate to `WLX_Slot_Style_Opt`.
- Removed outdated `demos/gallery_new.c` after folding its content into the
  main gallery flow.

### Internal
- Introduced shared helper and macro layers for command recording, widget
  frames, layout/container decoration, padding, opacity, typography, border
  resolution, and box drawing.
- Unified the five layout-begin entry points behind shared frame/common
  helpers, extracted the CONTENT sizing lifecycle into named helpers,
  decomposed `wlx_scroll_panel_begin_impl`, and moved widget wrappers onto a
  balanced frame lifecycle.
- Refactored per-frame storage around `WLX_Arena_Pool`, `WLX_Sub_Arena`, a
  WASM page pool, `dyn_offsets`, and `WLX_ARENA_POOL_FIELDS(X)`, while
  switching layout internals to on-demand offset/content-height accessors.
- Added SDL3-local tessellation helpers and expanded regression coverage with
  new suites including `test_circle_dispatch`, `test_cmd_replay`,
  `test_container_scope`, `test_dyn_offsets`, `test_rounding`,
  `test_slot_style`, `test_sub_arena`, `test_text_layout`, `test_wasm_pool`,
  `test_widget_wrapper`, and `test_widgets`.

## [0.2.0] - 2026-04-07

### Removed
- **Breaking:** Removed `wlx_checkbox_tex` compatibility macro and
  `checkbox_tex` short alias. Use `wlx_checkbox` with `.tex_checked` /
  `.tex_unchecked` fields instead (drop-in replacement).

### Changed
- **Breaking:** Renamed `.boxed` to `.show_background` on `WLX_Label_Opt`,
  `WLX_Checkbox_Opt`, and `WLX_Checkbox_Tex_Opt`. Removed `.boxed` from
  `WLX_Button_Opt` entirely (it was dead code — buttons always draw their
  background). Checkbox/checkbox_tex opts gain a new `full_slot_hit` field
  (default `true`) that controls whether the full slot rect is used for
  hover/click interaction, decoupling hit-testing from the visual background.
  Migration: `.boxed = true` → `.show_background = true` on labels and
  checkboxes; remove `.boxed` from button calls.
- **Breaking:** Renamed `wlx_textbox` to `wlx_label` (and `WLX_Textbox_Opt`
  to `WLX_Label_Opt`). Search-replace `wlx_textbox` → `wlx_label` in your
  code to migrate.
- **Breaking:** Interaction IDs no longer include a per-frame sequential counter.
  IDs are now `hash(file, line) ^ id_stack_hash`, matching persistent state IDs.
  Loops that emit interactive widgets MUST use `wlx_push_id()`/`wlx_pop_id()`.
- **Breaking:** `wlx_resolve_opt_*` functions now take `const WLX_Context *`
  instead of `const WLX_Theme *` to access the context opacity stack.
- Layout dimensions switched from `int16_t` to `float` with pixel-aligned
  rounding for sub-pixel accuracy.
- Refactored `wlx_widget()` to use `WLX_Widget_Opt` struct with designated
  initializer syntax (e.g. `wlx_widget(ctx, .color = red)`), replacing the
  old positional color argument.
- Refactored state pool from a linear array to an open-addressing hashmap for
  faster persistent state lookups.
- Slider value area uses fixed width for consistent geometry across values.

### Added
- **Three-layer opacity model:** final opacity is
  `widget.opacity * theme.opacity * ctx_stack_opacity`. Each layer defaults
  to 1.0 (fully opaque) when omitted. See `docs/OPACITY.md`.
  - Per-widget `.opacity` field on all widget option structs (negative
    sentinel = unset).
  - `WLX_Theme.opacity` field for global UI transparency.
  - `wlx_push_opacity()` / `wlx_pop_opacity()` / `wlx_get_opacity()` for
    scoped region-level opacity. Nested pushes multiply automatically.
  - `wlx_resolve_opacity()` and `wlx_color_apply_opacity()` helpers.
- **Layout sizing:** `WLX_SIZES` macro and `wlx_layout_begin_s` for
  auto-counted slot size arrays. `wlx_layout_auto_slot` for dynamic
  mixed-size layouts. `WLX_SIZE_CONTENT` and `WLX_SIZE_FILL` slot size
  types for content-driven and viewport-relative sizing.
- **Compound panel widget:** `wlx_panel_begin` / `wlx_panel_end` — a
  capacity-based CONTENT layout with optional heading label. Eliminates
  manual slot counting for all-CONTENT layouts. Pre-allocates 32 CONTENT
  slots by default; unused slots contribute 0px. See `docs/WIDGETS.md`
  and `docs/API_REFERENCE.md`.
- **Compound split widget:** `wlx_split_begin` / `wlx_split_next` /
  `wlx_split_end` — a two-pane split layout with independent scroll panels.
  Replaces the common 6-pair begin/end sidebar+content pattern with 3 calls.
  Configurable pane sizes, gap, fill behavior, and per-pane background colors
  via `WLX_Split_Opt` designated initializers. See `docs/WIDGETS.md` and
  `docs/API_REFERENCE.md` for full documentation.
- **Border support:** `border_color` and `border_width` fields on widget
  option structs. Border thickness clamped to 1px minimum with alpha scaling
  for sub-pixel values.
- **Debug instrumentation:** `WLX_DEBUG` compile-time mode with
  `WLX_Debug_Companion` for content-slot oscillation detection, site-based
  warning system, and vertical bounding checks. Warns when the same
  call-site is hit multiple times per frame without `wlx_push_id()`.
- **Inputbox improvements:** `wlx_inputbox_handle_keys` for extracted key
  handling. Width guard guaranteeing minimum `font_size * 3` pixels.
  `content_padding` auto-shrink when slot constrains widget height.
- `wlx_parent_rect()` and `wlx_viewport_rect()` layout query helpers.
- Content height tracking for horizontal and vertical layouts.
- `WLX_Scratch_Bytes` byte-level scratch arena.
- Optional `.id` field on all widget option structs for explicit string-based
  identity (e.g. `.id = "my_button"`). When set, the string is hashed and
  pushed onto the ID stack automatically, giving widgets stable IDs independent
  of source location.
- `wlx_hash_string()` helper — djb2 hash for string-to-ID conversion.
- SDL3_ttf font support in the SDL3 backend (`wollix_sdl3.h`).
- Inter and Public Sans font assets in `demos/assets/`.
- New demos: `border_demo`, `opacity_demo` (three-layer model).
- Expanded `gallery.c` with all new features.
- Updated `font_demo.c` with new font showcases.
- New test suites: `test_auto_layout`, `test_split`, `test_panel`,
  `test_opacity_stack`.
- Expanded existing test suites: `test_layout_math`, `test_edge_cases`,
  `test_scroll_panel`, `test_interaction`.
- New documentation: `docs/OPACITY.md`.
- Expanded `docs/API_REFERENCE.md`, `docs/LAYOUT_MODEL.md`,
  `docs/WIDGETS.md`.
- CI: Raylib and SDL3 demo builds, sanitizer tests, `-Werror` test.

### Fixed
- `active_id_seen` tracking bug where stale active IDs could persist across
  frames when the originating widget was removed.
- Auto-height scroll panel content height double-counting regression.

## [0.1.0] - 2026-03-21

### Added
- Initial release of wollix
- Single-header immediate-mode GUI library
- Raylib backend (`wollix_raylib.h`)
- SDL3 backend (`wollix_sdl3.h`)
- Flexbox-inspired layout system (row, column, wrap)
- Grid layout support
- Scrollable panels
- Widget library: buttons, checkboxes, sliders, text input, labels
- Theming and opacity support
- UTF-8 text handling
- Demo programs and documentation
