# vcpkg overlay port

`ports/wollix/` is the vcpkg port for Wollix, kept in the repository as an
overlay so it can be used before (and independently of) the public
registry, and copied verbatim into a `microsoft/vcpkg` pull request when a
release is tagged.

Use it from any vcpkg checkout:

```bash
vcpkg install wollix --overlay-ports=/path/to/wollix/ports
```

or, in a manifest project, add the directory to `overlay-ports` in
`vcpkg-configuration.json`. The installed package provides
`find_package(wollix CONFIG REQUIRED)` and the target `wollix::wollix`.

Two values in `portfile.cmake` are tied to a release tag:

- `REF` names the tag (`v0.9.0`).
- `SHA512` is the hash of the GitHub source archive for that tag. It is `0`
  until the tag exists; the first `vcpkg install` against the real tag
  prints the actual hash, which is then pasted in.

`vcpkg.json`'s `version` matches the tag and the `WOLLIX_VERSION` macro in
`wollix.h`. The port builds nothing: tests and demos are off, only the
headers, the CMake config and the licence are installed.
