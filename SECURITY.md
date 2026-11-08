# Security Policy

## Supported versions

Wollix is pre-1.0, and security fixes are made for the latest minor release
only. Upgrading is the fix for an older version.

| Version | Supported |
|---------|-----------|
| Latest minor release | Yes |
| Older releases | No |

## Reporting a vulnerability

Please do not open a public issue for a security problem. Email
daberzinsgh@gmail.com instead, with:

- The Wollix version (`WOLLIX_VERSION`) or commit
- The backend (Raylib, SDL3, bare WASM or your own), compiler and build flags
- A minimal snippet or input that reproduces the problem
- What you believe the impact is

Wollix has one maintainer, so response times are best effort. Please keep the
report private until a fix has been released.

## Scope

Wollix processes text, clipboard contents and input events that can come from
untrusted sources. Memory errors reachable through that data, such as
out-of-bounds access or use after free, are in scope, in the library and in
the web host.

These are ordinary bugs, so please file them as issues:

- A problem that needs the application itself to misuse the API, such as an
  unbalanced begin and end
- Anything in the demos that is not library behaviour. The passwords and
  tokens the demos show are placeholder values
