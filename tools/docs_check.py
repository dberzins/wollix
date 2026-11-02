#!/usr/bin/env python3
"""docs_check.py - the documentation gate (`make docs-check`).

Run from the repository root with Python 3 and nothing beyond the standard
library. Six checks, each reporting a count; the exit status is non-zero
when any check finds a problem.

  blocks      Every fenced `c` and `cpp` block of README.md and docs/*.md
              compiles with -fsyntax-only (gcc and g++, in parallel) against
              the headers. A block is a fragment: a prelude of the standard
              headers, wollix.h and wollix_editor.h (plus a backend's
              headers when the block names it) comes first, then the block,
              wrapped in a function when it is statements. A block that
              includes wollix.h itself keeps its lines up to that #include
              ahead of the prelude (its #define WLX_SHORT_NAMES must come
              first), and a doc listed in IMPLEMENTATION_DOCS compiles with
              WOLLIX_IMPLEMENTATION, since its examples call internals. The reader's own variables
              stay undeclared, so only two kinds of diagnostic fail: the
              API-shape classes (an unknown field or member, an undeclared
              wlx_ or WLX_ name) and any fatal error, such as a header that
              cannot be found, which would otherwise leave the block
              unchecked.
  pair        README's "Using from C++" pair compiles and links (Raylib).
  anchors     Every [text](#anchor) and [text](FILE.md#anchor) in README.md,
              CHANGELOG.md and docs/*.md names a heading or an anchor.
  names       Every wlx_ or WLX_ name in README.md and docs/*.md, outside a
              link target, exists in the headers, the web host or the build
              files, or is on ALLOWED_NAMES below with its reason.
  records     No tracked file except .gitignore and .graphifyignore names a
              decision record number, a path under the development records
              directory, or a plan decision id: those records are not
              published, so a public file states its rule or links a public
              doc instead.
  registries  The default-option macros, the defaults functions and the
              rows of tests/option_structs.h are one set; so are the
              WLX_SHORT_NAMES aliases and the calls between the
              short-names markers of tests/test_config_override.c.

Environment:
  DOCS_CC, DOCS_CXX   compilers (default gcc, g++)
  DOCS_INCLUDES       include directories for the backend headers, separated
                      by spaces or colons (default: ~/opt/raylib/include,
                      ~/opt/sdl3/include, ~/opt/sdl3_ttf/include)
  DOCS_LINK           link flags for the README pair (default: Raylib from
                      ~/opt/raylib/lib, as the Makefile links it)
  DOCS_JOBS           parallel compiles (default: the CPU count)
"""

import concurrent.futures
import glob
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = os.getcwd()
HOME = os.path.expanduser("~")
CC = os.environ.get("DOCS_CC", "gcc")
CXX = os.environ.get("DOCS_CXX", "g++")
_inc = os.environ.get("DOCS_INCLUDES")
INCLUDES = ([p for p in re.split(r"[\s:]+", _inc) if p] if _inc is not None else
            [HOME + "/opt/raylib/include", HOME + "/opt/sdl3/include", HOME + "/opt/sdl3_ttf/include"])
LINK = shlex.split(os.environ.get(
    "DOCS_LINK", "-L%s/opt/raylib/lib -lraylib -lGL -lm -ldl -lpthread -lrt -lX11" % HOME))
JOBS = int(os.environ.get("DOCS_JOBS", "0")) or os.cpu_count() or 4

DOC_FILES = ["README.md"] + sorted(glob.glob("docs/*.md"))
HEADERS = ["wollix.h", "wollix_editor.h", "wollix_raylib.h", "wollix_sdl3.h", "wollix_wasm.h"]

# Names the docs may use although no header defines them, each with why.
ALLOWED_NAMES = {
    "wlx_scroll_panel": "the family name of the wlx_scroll_panel_begin / _end pair",
    "wlx_list_clipper": "the family name of the wlx_list_clipper_begin / _step / _end calls",
    "wlx_menu": "the family name of the wlx_menu_begin / _item / _end calls",
    "wlx_image_label": "named to say that no such function exists",
    "wlx_label_image": "named to say that no such function exists",
    "wlx_image_button": "named to say that no such function exists",
    "wlx_checkbox_tex": "the removed compatibility macro, named as removed",
    "wlx_inputbox_handle_keys": "a removed internal, named in the undo model's history",
    "WLX_State_Flags": "a future enum the design system describes",
    "wlx_gallery_raylib_perf": "a perf output file name",
    "wlx_gallery_raylib_summary": "a perf output file name",
    "wlx_gallery_sdl3_perf": "a perf output file name",
    "wlx_gallery_sdl3_summary": "a perf output file name",
}

# Docs whose examples call the implementation's internal helpers.
IMPLEMENTATION_DOCS = {
    "docs/CORE_PATTERNS_GUIDE.md": "the internal guide to the implementation's patterns",
}

failures = {}


def report(check, msg):
    failures.setdefault(check, []).append(msg)
    print("%s: %s" % (check, msg))


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


# ============================================================================
# blocks
# ============================================================================

# The compilers run under LC_ALL=C, so they quote with ASCII apostrophes;
# the typographic quotes a UTF-8 locale uses are accepted too, since a
# pattern that misses its quote fails silently.
Q = "['\u2018\u2019]"
API_C = re.compile(r"has no member named|unknown field|"
                   r"implicit declaration of function " + Q + r"(wlx_|WLX_)|"
                   + Q + r"(wlx_|WLX_)\w*" + Q + r" undeclared|unknown type name " + Q + r"WLX_")
API_CXX = re.compile(r"has no member named|" + Q + r"(wlx_|WLX_)\w*" + Q + r" was not declared in this scope|"
                     + Q + r"WLX_\w*" + Q + r" does not name a type|"
                     + Q + r"(wlx_|WLX_)\w*" + Q + r" has not been declared")
COMPILER_ENV = dict(os.environ, LC_ALL="C", LANG="C")
FATAL = re.compile(r"fatal error:")
FILESCOPE = re.compile(r"^(#include|#define|typedef|struct\s|enum\s|static\s|extern\s|"
                       r"(const\s+)?[A-Za-z_][\w\s\*]*\s+\**[A-Za-z_]\w*\s*\([^;]*\)\s*\{?\s*$)",
                       re.M)
PERF = re.compile(r"wlx_perf_|WLX_Perf_|WLX_PERF\b")


def fenced_blocks(path):
    out, lines, i = [], read(path).split("\n"), 0
    while i < len(lines):
        m = re.match(r"^\s*```\s*(c|C|cpp|c\+\+)\s*$", lines[i])
        if m:
            lang = "cpp" if m.group(1).lower() in ("cpp", "c++") else "c"
            start, body = i + 1, []
            i += 1
            while i < len(lines) and not re.match(r"^\s*```\s*$", lines[i]):
                body.append(lines[i])
                i += 1
            out.append((lang, start + 1, body))
        i += 1
    return out


def split_head(body):
    """A block that includes wollix.h itself: its lines up to and including
    that #include, and the rest. Any other block: no head."""
    for k, line in enumerate(body):
        if re.match(r'^\s*#\s*include\s*[<"]wollix\.h[>"]', line):
            return body[:k + 1], body[k + 1:]
    return [], body


def prelude(src, lang, impl):
    p = ["#define WOLLIX_IMPLEMENTATION"] if impl else []
    if lang == "c":
        p += ["#include <stdio.h>", "#include <stdbool.h>", "#include <string.h>",
              "#include <stdint.h>", "#include <inttypes.h>", "#include <stdlib.h>"]
    else:
        p += ["#include <stdio.h>", "#include <string.h>", "#include <stdint.h>"]
    tail = ['#include "wollix.h"', '#include "wollix_editor.h"']
    if re.search(r"raylib|ClearBackground|DrawText|InitWindow|\bColor\b|wlx_\w*raylib", src):
        p.insert(0, '#include "raylib.h"')
        tail.append('#include "wollix_raylib.h"')
    if re.search(r"SDL_|sdl3", src):
        p.insert(0, "#include <SDL3/SDL.h>")
        p.insert(1, "#include <SDL3_ttf/SDL_ttf.h>")
        tail.append('#include "wollix_sdl3.h"')
    if re.search(r"wlx_\w*wasm|WLX_\w*Wasm|WLX_WASM", src):
        tail.append('#include "wollix_wasm.h"')
    return p, tail


def compile_block(job):
    path, lang, line, body = job
    src = "\n".join(body)
    head, rest = split_head(body)
    pre, tail = prelude(src, lang, path in IMPLEMENTATION_DOCS)
    rest_src = "\n".join(rest)
    if FILESCOPE.search(rest_src) or not rest_src.strip():
        wrapped = rest_src
    else:
        wrapped = "void wlx_docs_fragment(WLX_Context *ctx) {\n" + rest_src + "\n}\n"
    text = "\n".join(pre) + "\n"
    text += '#line %d "%s"\n' % (line, path) + "\n".join(head) + "\n"
    text += "\n".join(tail) + "\n"
    text += '#line %d "%s"\n' % (line + len(head), path) + wrapped + "\n"
    suffix = ".cpp" if lang == "cpp" else ".c"
    fd, name = tempfile.mkstemp(suffix=suffix, prefix="wlx_docs_")
    with os.fdopen(fd, "w", encoding="utf-8") as t:
        t.write(text)
    flags = ["-fsyntax-only", "-I" + ROOT] + ["-I" + d for d in INCLUDES]
    if PERF.search(src):
        flags.append("-DWLX_PERF")
    if lang == "cpp":
        cmd = [CXX, "-std=c++11"] + flags + [name]
    else:
        cmd = [CC, "-std=c11"] + flags + [name]
    r = subprocess.run(cmd, capture_output=True, text=True, env=COMPILER_ENV)
    os.unlink(name)
    # A diagnostic counts when it is located in the doc, or when it is
    # located in a header inside a macro the block expands (a short-name
    # alias, say): gcc then follows it with an "in expansion of macro" note
    # located in the doc. Diagnostics the headers raise on their own (an
    # adapter included without WOLLIX_IMPLEMENTATION) are not the block's.
    api = API_CXX if lang == "cpp" else API_C
    diag = re.compile(r": (fatal )?(error|warning):")
    lines = r.stderr.split("\n")
    bad = []
    for k, d in enumerate(lines):
        if not (FATAL.search(d) or api.search(d)):
            continue
        d = d.replace(name, "%s:%d (prelude)" % (path, line))
        if d.startswith(path) or FATAL.search(d):
            bad.append(d)
            continue
        for note in lines[k + 1:]:
            if diag.search(note):
                break
            if note.startswith(path) and "in expansion of macro" in note:
                bad.append("%s (from %s)" % (note.split(": note:")[0], d))
                break
    return path, line, bad


def check_blocks():
    jobs = []
    for f in DOC_FILES:
        for lang, line, body in fenced_blocks(f):
            jobs.append((f, lang, line, body))
    with concurrent.futures.ThreadPoolExecutor(max_workers=JOBS) as ex:
        results = list(ex.map(compile_block, jobs))
    n_bad = 0
    for path, line, bad in results:
        for d in bad:
            report("blocks", d)
        n_bad += 1 if bad else 0
    langs = sum(1 for j in jobs if j[1] == "cpp")
    print("blocks: %d (%d C++), %d failing" % (len(jobs), langs, n_bad))


# ============================================================================
# pair
# ============================================================================

def check_pair():
    s = read("README.md")
    sec = s[s.index("## Using from C++"):]
    c = re.search(r"```c\n(.*?)```", sec, re.S).group(1)
    cpp = re.search(r"```cpp\n(.*?)```", sec, re.S).group(1)
    d = tempfile.mkdtemp(prefix="wlx_docs_pair_")
    open(os.path.join(d, "wollix_impl.c"), "w").write(c)
    open(os.path.join(d, "main.cpp"), "w").write(cpp)
    inc = ["-I" + ROOT] + ["-I" + p for p in INCLUDES]
    steps = [
        [CC, "-std=c11", "-Wall", "-Wextra", "-Wno-override-init", "-Wno-override-init-side-effects"]
        + inc + ["-c", os.path.join(d, "wollix_impl.c"), "-o", os.path.join(d, "wollix_impl.o")],
        [CXX, "-std=c++11", "-Wall", "-Wextra"] + inc
        + ["-c", os.path.join(d, "main.cpp"), "-o", os.path.join(d, "main.o")],
        [CXX, "-o", os.path.join(d, "hello"), os.path.join(d, "wollix_impl.o"),
         os.path.join(d, "main.o")] + LINK,
    ]
    for cmd in steps:
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            report("pair", "%s failed:\n%s" % (os.path.basename(cmd[0]), r.stderr.strip()))
            break
    for name in os.listdir(d):
        os.unlink(os.path.join(d, name))
    os.rmdir(d)
    print("pair: %s" % ("fails" if "pair" in failures else "compiles and links"))


# ============================================================================
# anchors
# ============================================================================

def slug(h):
    h = h.strip().lower()
    h = re.sub(r"[^\w\- ]", "", h)
    return h.replace(" ", "-")


def check_anchors():
    files = ["README.md", "CHANGELOG.md"] + sorted(glob.glob("docs/*.md"))
    anchors = {}
    for f in files:
        seen, s, fence = {}, set(), False
        for line in read(f).split("\n"):
            if line.startswith("```"):
                fence = not fence
                continue
            if fence:
                continue
            m = re.match(r"^(#{1,6})\s+(.*?)\s*#*\s*$", line)
            if m:
                b = slug(m.group(2))
                n = seen.get(b, 0)
                seen[b] = n + 1
                s.add(b if n == 0 else "%s-%d" % (b, n))
            for a in re.findall(r'<a\s+(?:name|id)="([^"]+)"', line):
                s.add(a)
        anchors[os.path.normpath(f)] = s
    links = 0
    for f in files:
        fence = False
        for i, line in enumerate(read(f).split("\n"), 1):
            if line.startswith("```"):
                fence = not fence
                continue
            if fence:
                continue
            for target in re.findall(r"\]\(([^)\s]*#[^)\s]+)\)", line):
                path, _, frag = target.partition("#")
                if path.startswith("http"):
                    continue
                links += 1
                tf = os.path.normpath(os.path.join(os.path.dirname(f), path)) if path else os.path.normpath(f)
                if tf not in anchors:
                    if not os.path.exists(tf):
                        report("anchors", "%s:%d: missing file %s" % (f, i, path))
                    continue
                if frag not in anchors[tf]:
                    report("anchors", "%s:%d: #%s is not a heading or anchor of %s" % (f, i, frag, tf))
    print("anchors: %d links, %d unresolved" % (links, len(failures.get("anchors", []))))


# ============================================================================
# names
# ============================================================================

NAME = re.compile(r"\b(?:wlx|WLX)_[A-Za-z0-9_]+")


def check_names():
    known = set()
    for f in HEADERS + glob.glob("web/*.js") + glob.glob("web/*.h") + ["Makefile", "CMakeLists.txt"]:
        if os.path.exists(f):
            with open(f, encoding="utf-8", errors="replace") as h:
                known.update(NAME.findall(h.read()))
    used = 0
    allowed_seen = set()
    for f in DOC_FILES:
        for i, line in enumerate(read(f).split("\n"), 1):
            stripped = re.sub(r"\]\([^)]*\)", "]()", line)   # link targets are the anchors check's
            for name in NAME.findall(stripped):
                if name.endswith("_"):
                    continue                                  # a prefix: WLX_SIZE_*
                used += 1
                if name in known:
                    continue
                if name in ALLOWED_NAMES:
                    allowed_seen.add(name)
                    continue
                report("names", "%s:%d: %s is in no header, host or build file" % (f, i, name))
    for name in sorted(set(ALLOWED_NAMES) - allowed_seen):
        report("names", "ALLOWED_NAMES lists %s, which no doc uses any more" % name)
    for name in sorted(set(ALLOWED_NAMES) & known):
        report("names", "ALLOWED_NAMES lists %s, which a header now defines" % name)
    print("names: %d uses, %d allowed names, %d problems" % (used, len(ALLOWED_NAMES),
                                                            len(failures.get("names", []))))


# ============================================================================
# records
# ============================================================================

# Built so that this file does not match its own patterns.
RECORD_PATTERNS = [
    (re.compile(r"\bA" + r"DR[ _-]?\d{2,3}\b"), "a decision record number"),
    (re.compile(r"\bdocs/" + r"dev\b"), "a development records path"),
    (re.compile(r"\b" + r"D\d{1,3}\b"), "a plan decision id"),
]
RECORD_EXEMPT = {".gitignore", ".graphifyignore"}


def check_records():
    r = subprocess.run(["git", "ls-files", "-z"], capture_output=True)
    if r.returncode != 0:
        report("records", "git ls-files failed: %s" % r.stderr.decode(errors="replace").strip())
        print("records: not run")
        return
    files = [f for f in r.stdout.decode().split("\0") if f and f not in RECORD_EXEMPT]
    scanned = 0
    for f in files:
        try:
            with open(f, "rb") as h:
                data = h.read()
        except OSError:
            continue
        if b"\0" in data:
            continue
        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError:
            continue
        scanned += 1
        for i, line in enumerate(text.split("\n"), 1):
            for pat, what in RECORD_PATTERNS:
                m = pat.search(line)
                if m:
                    report("records", "%s:%d: %s names %s" % (f, i, m.group(0), what))
    print("records: %d tracked text files, %d citations" % (scanned, len(failures.get("records", []))))


# ============================================================================
# registries
# ============================================================================

def check_registries():
    src = "".join(read(h) for h in HEADERS)
    macros = set(re.findall(r"#define wlx_default_(\w+)_opt\(", src))
    funcs = set(re.findall(r"WLXDEF\s+WLX_\w+_Opt\s+wlx_(\w+)_opt_defaults\s*\(\s*void\s*\)\s*;", src))
    rows = set(re.findall(r"X\((\w+),\s*WLX_\w+_Opt", read("tests/option_structs.h")))
    for name, got in (("default-option macros", macros), ("defaults functions", funcs),
                      ("option_structs.h rows", rows)):
        for stem in sorted((macros | funcs | rows) - got):
            report("registries", "%s: no entry for %s" % (name, stem))
    h = read("wollix.h")
    block = h[h.index("#ifdef WLX_SHORT_NAMES"):h.index("#endif // WLX_SHORT_NAMES")]
    aliases = set(re.findall(r"#define (\w+)\(", block))
    t = read("tests/test_config_override.c")
    body = t[t.index("// short-names:begin"):t.index("// short-names:end")]
    called = set(re.findall(r"\b(\w+)\s*\(", body))
    for a in sorted(aliases - called):
        report("registries", "WLX_SHORT_NAMES alias %s is not expanded in tests/test_config_override.c" % a)
    print("registries: %d option structs (macros %d, functions %d, rows %d), %d aliases, %d problems"
          % (len(macros | funcs | rows), len(macros), len(funcs), len(rows), len(aliases),
             len(failures.get("registries", []))))


# ============================================================================

def main():
    checks = [("blocks", check_blocks), ("pair", check_pair), ("anchors", check_anchors),
              ("names", check_names), ("records", check_records), ("registries", check_registries)]
    only = sys.argv[1:]
    for name, fn in checks:
        if only and name not in only:
            continue
        fn()
    if failures:
        print("docs-check: FAILED (%s)" % ", ".join("%s %d" % (k, len(v)) for k, v in failures.items()))
        return 1
    print("docs-check: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
