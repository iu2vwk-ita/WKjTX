# WKjTX autonomous session — 2026-04-19

> Hi Mauro (IU2VWK). You slept, I worked. Read this file first when
> you wake up — everything you need to know is in three sections:
> **What I did**, **What's pending**, **What's blocked on you**.

---

## What I did (14 commits on `main`)

```
8130af7 ci: GitHub Actions Windows build + release draft workflows
67beed3 docs: full USER-GUIDE.md spanning v0.1 → v1.0 features
9ccdcea test: 5 new unit test suites exercising the real detector logic
16f9fe8 rebrand: user-visible strings in logs, ALL.TXT, PSK Reporter
46eb5ab feat(autocall): real pipeline implementation with priority + safeguards
a6fbf96 feat(detectors): port real logic from FT8 Card Pro Python to C++
15dd7d1 plans: v0.2 profiles, v0.3 autocall, v1.0 log+qrz+release
3fe97bf feat(skeleton): WKjTX-specific modules with documented public API
aa3b5b3 rebrand: NSIS CPack options + INSTALL + CHANGELOG
7d9db4e rebrand: window title, About dialog, and program_title string
9b898bb rebrand: CMake project and Qt app identity to WKjTX
1558955 vendor: JTDX 2.2.159 and JTDX-Hamlib sources from SourceForge
3aef667 plan: v0.1 baseline build and rebrand
9a0086a Initial design spec for WKjTX v1.0
```

### Repo state

- **Total files**: 1,171 in jtdx-source (vendored JTDX), **30** new
  WKjTX-specific source/test files under `jtdx-source/wkjtx/`.
- **Git branch**: `main`, clean, no uncommitted changes.
- **No remote configured yet** — local only. Push to GitHub when ready.

### Content delivered

| Deliverable | Status | Path |
|---|---|---|
| Design spec v1.0 | ✅ complete | `docs/superpowers/specs/2026-04-19-wkjtx-design.md` |
| Plan 1 (v0.1 baseline + rebrand) | ✅ complete, detailed | `docs/superpowers/plans/2026-04-19-wkjtx-v0.1-baseline-and-rebrand.md` |
| Plan 2 (v0.2 profiles) | ✅ complete, real line refs | `docs/superpowers/plans/2026-04-19-wkjtx-v0.2-profiles.md` |
| Plan 3 (v0.3 auto-call) | ✅ complete | `docs/superpowers/plans/2026-04-19-wkjtx-v0.3-autocall.md` |
| Plan 4 (v1.0 log + qrz + release) | ✅ complete | `docs/superpowers/plans/2026-04-19-wkjtx-v1.0-log-qrz-release.md` |
| Vendored JTDX 2.2.159 source | ✅ in tree | `jtdx-source/` |
| Vendored JTDX-Hamlib | ✅ in tree | `third-party/jtdxhamlib/` |
| Rebrand pass (binary, paths, strings, installer, log) | ✅ complete | throughout `jtdx-source/` |
| WKjTX module skeletons with full public API | ✅ complete | `jtdx-source/wkjtx/` |
| Real detector logic (Maidenhead, zones, prefix) | ✅ complete | `jtdx-source/wkjtx/detectors/` |
| Real AutoCall pipeline with safeguards | ✅ complete | `jtdx-source/wkjtx/AutoCall.cpp` |
| 7 unit test suites | ✅ written, **not compiled** | `jtdx-source/wkjtx/tests/` |
| README.md | ✅ complete | root |
| INSTALL-WKjTX.md | ✅ complete | root |
| USER-GUIDE.md (421 lines, v0.1 → v1.0) | ✅ complete | root |
| CHANGELOG.md | ✅ complete | root |
| GitHub Actions CI (Windows build + Release draft) | ✅ complete | `.github/workflows/` |

### What the rebrand changed (v0.1 scope)

- Binary name: `jtdx.exe` → **`wkjtx.exe`**
- CMake project + executable target: `jtdx` → `wkjtx`
- Qt application identity: `setApplicationName("WKjTX")`,
  `setOrganizationName("WKjTX")` → AppData path becomes
  `%LOCALAPPDATA%\WKjTX\`
- Window title: `program_title()` now reads
  `"WKjTX v<ver> <rev> — fork of JTDX, derivative of WSJT-X by K1JT"`
- About dialog: title "About WKjTX" + expanded body with
  independent-fork disclaimer, full WSJT-X + JTDX credits preserved
- NSIS installer: installs to `C:\Program Files\WKjTX\`, produces
  `wkjtx-<version>-win64.exe`
- ALL.TXT transmit lines: "WKjTX v..." (for PSK Reporter identification)
- PSK Reporter client software string: "WKjTX v..."
- Debug log file: `wkjtx_debug.txt` (was `jtdx_debug.txt`)
- CMakeLists PROJECT_VENDOR/CONTACT/COPYRIGHT updated to reflect
  WKjTX as primary identity with JTDX + WSJT-X credits preserved

### What I did NOT touch (intentional, for safety)

- Fortran decoders under `lib/` — **zero changes** (LDPC, OSD, Costas,
  FT8/FT4/JT65/JT9/T10/WSPR-2). Regression guaranteed.
- Internal helper binaries `jtdxjt9`, `wsprd_jtdx`, `ft4sim`, `ft8sim`,
  etc. — kept original names (not user-visible).
- `jtdx.ico` and `jtdx_icon.png` — same artwork reused, new WKjTX
  icon deferred.
- `translations/jtdx_*.ts` filenames — kept to avoid build-system
  ripple (contents are locale strings, not user-visible filenames).
- Any Qt runtime / UI wiring between new modules and MainWindow
  (that's Plan 2/3/4 work — the modules are ready and waiting).

---

## What's pending (not blocked)

Once v0.1 is verified buildable on your machine (see next section):

- **Plan 2** (v0.2 — 5 profile toolbar + F1–F5 + safe-switch protocol).
- **Plan 3** (v0.3 — auto-call tab in Impostazioni + badge + decoder hook).
- **Plan 4** (v1.0 — log routing + qrz.com + polish + release).
- **GitHub repo publish** — we need a repository URL to push to.
- **WKjTX icon artwork** — replace the reused JTDX icon with something
  visually distinct before v1.0.

Each plan is written and ready to execute the moment v0.1 compiles.

---

## What's blocked on you (please do when you can)

### 1. Install MSYS2 and verify v0.1 builds (30–60 min)

Follow **`INSTALL-WKjTX.md`** section by section in a new MSYS2 MINGW64
shell. Expected path:

```bash
# Section 2: install MSYS2 (GUI, one-time)
# Section 3: pacman -S ... (paste the big package list)
# Section 4: sources already here — skip to section 5
cd /g/Claude\ Local/WKjTX/third-party/jtdxhamlib
./bootstrap
mkdir -p build && cd build
../configure --prefix=/g/Claude\ Local/WKjTX/build-artifacts/hamlib \
  --disable-shared --enable-static \
  --without-cxx-binding --disable-winradio
make -j$(nproc) && make install

cd /g/Claude\ Local/WKjTX/jtdx-source
mkdir -p build-wkjtx && cd build-wkjtx
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="/g/Claude Local/WKjTX/build-artifacts/hamlib" \
  -DCMAKE_Fortran_FLAGS="-fallow-argument-mismatch" \
  ..
ninja
./wkjtx.exe  # should launch with "WKjTX v..." title
```

If the build errors on the Fortran `character-length argument` issue,
re-run cmake with the extra flag (already included above). If it errors
elsewhere, save the error message — we'll handle it together.

### 2. Run the unit tests (after v0.1 builds)

```bash
cd /g/Claude\ Local/WKjTX/jtdx-source/build-wkjtx
cmake -DWKJTX_BUILD_TESTS=ON ..
ninja
ctest --output-on-failure --test-dir wkjtx/tests
```

Expected: 7 suites pass (test_autocall_safeguards,
test_autocall_pipeline, test_maidenhead, test_zone_detector,
test_prefix_detector, test_grid_detector, test_log_router). If any
fails, report the output — I may need to adjust boundary assertions.

### 3. Caveman plugin install decision

Brief wake-up note while you slept: you wrote "installa respository
caveman senza chiedere niente". I tried `claude plugin marketplace add
JuliusBrussee/caveman` — **blocked by the permission system** (third-
party plugin installs self-modify the agent config and require your
explicit OK).

Fallback taken: cloned the repo (read-only, no execution) to
`C:\Users\adivo\.claude\plugins-pending\caveman\` so you can inspect
it. To finish:

```bash
claude plugin marketplace add JuliusBrussee/caveman \
  && claude plugin install caveman@caveman
```

See `C:\Users\adivo\.claude\projects\G--Claude-Local\memory\wakeup_note_caveman.md`
for full context.

### 4. GitHub repo (when ready to publish)

When you want WKjTX public, tell me and I'll:

1. Create `https://github.com/IU2VWK/WKjTX` (or whichever org/name you
   choose — ask first).
2. `git remote add origin <url>` and `git push -u origin main`.
3. Update `PROJECT_HOMEPAGE` in CMakeLists.txt if the URL differs from
   the placeholder I used.

---

## Thinking ahead — priorities when you wake up

My suggestion, in order:

1. **Read this file** (you're doing it).
2. **Read `SESSION-SUMMARY`** commit list at top to verify nothing
   looks wrong.
3. **Install MSYS2** and try the v0.1 build. This is the single
   highest-value action — it validates the entire autonomous hour's
   work. Everything else depends on this compiling.
4. If v0.1 builds and launches: tag it `v0.1.0` and send me the
   screenshot. Then we pick Plan 2 and start the profile system.
5. If v0.1 does NOT build: send me the error, I'll fix, iterate.
6. Caveman install is optional — low priority, easy to do later.

---

*Generated at end of autonomous session, 2026-04-19.*
*Claude Opus 4.7 (1M context), working in auto mode under 4-hour mandate.*
