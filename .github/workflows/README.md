# CI workflows

| Workflow | File | Trigger | Purpose |
|---|---|---|---|
| Build Windows | `build-windows.yml` | push to main / tag `v*` / PR / manual | MSYS2 build, unit tests, artifact |
| Create GitHub Release | `release.yml` | on successful `v*` tag build | Draft GitHub Release with installer |

## v2 (deferred)

When WKjTX adds Linux and macOS targets in v2, add:

- `build-linux.yml` — `ubuntu-latest`, apt deps, `.deb` + AppImage.
- `build-macos.yml` — `macos-latest` + `macos-14` (Apple Silicon),
  brew deps, unsigned `.dmg`.

## Local reproduction

All three workflows are equivalent to running the commands in
[INSTALL-WKjTX.md](../../INSTALL-WKjTX.md) locally. If CI breaks, the
same sequence will break locally — and vice versa. Debug locally first.
