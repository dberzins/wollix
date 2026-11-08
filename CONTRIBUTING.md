# Contributing

Wollix has not reached 1.0. v0.9 is the last release that changes the meaning
of an existing option field or the shape of the backend contract; after it,
existing names, defaults and signatures keep working, apart from the
deprecated aliases the [CHANGELOG](CHANGELOG.md) lists for removal. Internals
can change in any release.

**Code contributions (pull requests) are not being accepted before v1.0.0.**
PRs opened now will likely be closed — the internals they touch may not
survive the next release.

What IS very welcome right now, as GitHub issues:

- Bug reports with a minimal reproducing snippet
- API feedback and friction reports from real usage
- Platform reports (CI covers Linux with gcc and clang, macOS with Apple
  clang, and Windows with MSVC through CMake; other compilers, older
  toolchains and real applications are what we cannot see)

## Building and tests

Two build descriptions exist and CI runs both: the `Makefile` is the
developer tool (demos, perf gates, the WASM sites, `make test`), and
`CMakeLists.txt` is the consumer surface (`find_package(wollix CONFIG)`,
`add_subdirectory`, `FetchContent`; `-DWOLLIX_BUILD_TESTS=ON` registers the
same test binaries with CTest, and it is what the Windows/MSVC leg runs).
A test binary added to the Makefile's `test` target is added to the CMake
test list in the same commit, and vice versa.

```bash
make test                                   # the Makefile suite
cmake -B build && cmake --build build && ctest --test-dir build   # the same suite through CTest
```

On a multi-config generator (Visual Studio, the Windows/MSVC leg) the build
and test steps each take the configuration: `cmake --build build --config Debug`
and `ctest --test-dir build -C Debug`.

Full contribution guidelines (including AI-assisted-contribution disclosure)
will arrive with v1.0.0.
