# Build scripts

Convenience scripts to build WKjTX without typing MSYS2 commands manually.

## One-click flow (from repo root)

```
build.bat                  # first time: installs deps + builds hamlib + wkjtx
run.bat                    # launch the just-built wkjtx.exe
package-installer.bat      # create wkjtx-*-win64.exe installer
```

## Modes (passed to build.bat)

| Mode | What it does |
|---|---|
| `build.bat` (no arg) | Full build: check deps, build Hamlib if absent, configure + compile WKjTX, run tests |
| `build.bat quick` | Skip dep check and Hamlib — fastest iteration when you're editing source |
| `build.bat clean` | Wipe `build-*` dirs, then do a full build from scratch |
| `build.bat tests` | Only run ctest (assumes prior successful build) |

## What `build.bat` actually does

1. Verifies MSYS2 is installed at `C:\msys64`.
   - If not: prints install instructions and exits. Install MSYS2 manually from https://www.msys2.org/ then re-run.
2. Invokes `scripts/build-wkjtx.sh` inside MSYS2 MINGW64 environment.

The `.sh` script is the real work:
- Runs `pacman -Q` per package to identify missing deps, installs only the missing ones.
- Builds Hamlib only if `libhamlib.a` is not yet installed (idempotent).
- Runs `cmake -G Ninja` + `ninja`.
- Runs `ctest`.

All idempotent — re-running after a successful build is cheap.

## Manual invocation

If you want to run the build from an already-open MSYS2 MINGW64 shell:

```bash
cd /g/Claude\ Local/WKjTX
./scripts/build-wkjtx.sh full
```

Arguments are the same as `build.bat`.

## First-time path checklist

Before `build.bat` works, make sure:

- [ ] MSYS2 is installed at `C:\msys64` (default path of the installer).
- [ ] You have run `pacman -Syu` at least once after install (to update
      base packages; the MSYS2 installer notes this).
- [ ] The `WKjTX` folder is at a path **without spaces is not required**
      — `G:\Claude Local\WKjTX\` works; MSYS2 handles the space via the
      `/g/Claude Local/...` POSIX form.

If something breaks, run in cmd: `build.bat` and read the error. Common issues are in `INSTALL-WKjTX.md` → "Known build gotchas".
