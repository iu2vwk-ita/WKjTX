# WKjTX — Design Document

**Status**: Draft — awaiting user review
**Author**: designed with user IU2VWK
**Date**: 2026-04-19
**License inheritance**: GPL-3.0 (mandatory from upstream)

---

## 1. Goals and non-goals

### Goals (v1.0)

WKjTX is a freeware, standalone fork of JTDX rc160 targeting the Windows amateur radio
community. It is the author's flagship product. The v1.0 feature set, on top of what JTDX
rc160 already provides, is:

1. **Rebrand** to WKjTX (binary name, icons, window titles, config paths, installer).
2. **5 radio profiles** recallable via a permanent toolbar and F1–F5 hotkeys. Each
   profile is a full set of JTDX settings (radio, audio, UDP ports, log, macros,
   notifications, filters, auto-call) that can be switched one-at-a-time.
3. **Per-profile log routing**: each profile can use the shared global log or its own
   dedicated ADIF file.
4. **Auto-call feature** ported from FT8 Card Pro: seven trigger categories, hardcoded
   safeguards (120 s per-call cooldown, 3 auto-calls per 60 s global), flashing AUTO-CALL
   badge, confirmation dialog on enable.
5. **qrz.com ADIF upload** (verify if rc160 already has it, extend if not).

### Non-goals (v1.0)

- No integration with the user's other apps (hamlog, dxmap, potamax, FT8 Card Pro) —
  explicit user rule.
- No changes to the Fortran decoder core (LDPC, OSD, Costas sync, demodulators). They
  are left byte-for-byte identical to JTDX rc160.
- No new digital modes beyond those JTDX already supports.
- No cross-platform builds in v1.0. Windows 10/11 64-bit only. Linux/macOS deferred to v2.
- No installer code signing (no paid certificate).
- No parallel multi-radio operation (rejected in favor of fast profile switching).
- No fourth UDP/TCP endpoint (JTDX's three — primary UDP, secondary UDP, TCP ADIF —
  are sufficient per user confirmation).

### Deferred to v2 (not promised)

- Linux (.deb, AppImage) and macOS (.dmg unsigned) builds via GitHub Actions CI.
- Profile import/export as a portable `.wkjtx-profile` file.
- Dark/light theme refinement.

---

## 2. Base and licensing

- **Upstream**: JTDX rc160 (released 2025-12-17).
- **Source of truth**: SourceForge git `https://sourceforge.net/p/jtdx/code/` tagged rc160.
  The GitHub mirror `jtdx-project/jtdx` is stale at rc155 (2024-04-22) and must not be
  used as the fork base.
- **License**: GPL-3.0, inherited. All credits to WSJT-X (K1JT et al.) and JTDX (HF
  community) must remain visible in `About`, `README`, and source headers. WKjTX's own
  additions are also GPL-3.0.
- **Trademark**: WKjTX is a distinct name from WSJT-X and JTDX. The About dialog states
  "WKjTX is an independent fork and is not endorsed by the WSJT-X or JTDX projects."

---

## 3. High-level architecture

WKjTX is an additive fork: everything in JTDX rc160 is preserved, new code is added in
isolated modules, and changes to existing files are kept minimal and localized.

### 3.1 Preserved as-is (no modifications)

- All Fortran code under `lib/` (decoders, modulators, sync).
- Audio engine, decoder engine, modulator engine core.
- JTDX/Hamlib wrapper core (only the profile-driven reinit path is new).
- All existing tabs in the Settings dialog: Generali, Radio, Audio, Sequenza,
  Macro TX, Segnalazioni, Frequenze, Notifiche, Filtri, Programmazione, Avanzate.
- Operating modes: FT8, FT4, JT65, JT9+JT65, JT9, T10, WSPR-2.
- AutoSeq, DXpedition mode, Super Fox mode.
- Italian translation and all other locales shipped by JTDX.
- The existing three network endpoints (primary UDP, secondary UDP, TCP ADIF) in the
  Segnalazioni tab.

### 3.2 Modified (minimal, localized changes)

- `Configuration.cpp/hpp`: extend the existing `Configurations` system to store
  per-profile slot index, hotkey, log path override, auto-call config.
- `mainwindow.cpp/ui`: insert the 5-slot profile toolbar above the decode list; add the
  flashing AUTO-CALL badge in the top bar.
- Rebrand pass: strings, icons, window titles, `About` dialog, installer name, AppData
  path `%LOCALAPPDATA%\WKjTX\`.

### 3.3 New modules (added files)

- `ProfileManager.{h,cpp}`: owns the 5 slots, handles save/load/switch, hotkey
  registration, safe-switch protocol (stop TX → close CAT/audio/UDP → load → reopen).
- `AutoCall.{h,cpp}`: detection pipeline, cooldown/rate-limit state, badge updates,
  per-profile config, audit log line to ALL.TXT.
- `NewDxccDetector.{h,cpp}`, `ZoneDetector.{h,cpp}`, `GridDetector.{h,cpp}`,
  `PrefixDetector.{h,cpp}`, `CallsignDetector.{h,cpp}`: port of FT8 Card Pro's
  `dxhunter/detector.py` and `dxhunter/zones.py` to native C++.
- `QrzUploader.{h,cpp}`: HTTPS ADIF upload to qrz.com Logbook API.
- `LogRouter.{h,cpp}`: routes a QSO record either to the shared global log or to the
  active profile's dedicated log file.

### 3.4 Stack

- C++14/17, Qt 5.15, gfortran, JTDX-Hamlib, FFTW, Boost, CMake, Ninja (identical to
  JTDX upstream — no stack changes).

---

## 4. Profile system

### 4.1 Data model

A profile is a complete named configuration stored in `%LOCALAPPDATA%\WKjTX\profiles\slot<N>.ini`
(N = 1..5). Contents:

- Identity: name, slot index 1–5, F-key binding, optional color tag.
- Station: callsign, locator, IARU region.
- Radio: rig model, CAT serial port, baud, PTT method, split mode.
- Audio: input device, output device, channel mode, save folder.
- Sequencing: CQ reply limits, RR73 repeats, etc.
- Macros TX: the user's `@ TNX 73` / `@ &` / custom macros.
- Reporting: primary UDP host+port, secondary UDP host+port, TCP ADIF host+port,
  eQSL credentials, qrz.com credentials, PSK Reporter flag, DX Summit flag.
- Frequencies: custom working frequency table + calibration + station info per band.
- Notifications: highlight rules for new DXCC/zone/grid/prefix/callsign.
- Filters: hide continent/country/callsign.
- Band scheduler: hh:mm → band map.
- Advanced: JT65 and T10 decoder parameters.
- **New** — Log path: empty string means shared global log; non-empty means
  per-profile `.adi` file.
- **New** — Auto-call: 7 category flags + 5 alert callsigns + per-profile enable.

A separate `%LOCALAPPDATA%\WKjTX\global.ini` stores: active profile at startup, global
hotkey bindings, toolbar visibility, shared log path, last window geometry.

File format: identical INI layout to JTDX (Qt QSettings), so a JTDX user can import
their existing config into a WKjTX slot without manual rewriting.

### 4.2 UX

- Permanent toolbar above the decode list, 5 wide buttons labeled with the profile
  name. The active profile is highlighted (color + bold). F1..F5 trigger the same
  switch. A "blank" slot shows "Empty — click to create".
- Settings dialog gains a "Profile slot" field (1–5 / none) and an "Assign hotkey"
  field in Generali, so a user can also manage profiles from the existing Configurations
  dialog.
- "Import from JTDX" menu item reads `%LOCALAPPDATA%\JTDX\*.ini` and copies one into a
  selected slot.

### 4.3 Switch protocol

On profile switch request (click or F-key):

1. If TX active: modal dialog "Stop TX and switch profile? [Yes/No]". No → abort.
2. Persist current profile state: active frequency, mode, split state, window geometry.
3. Stop decoder. Close UDP servers. Close TCP ADIF client. Close Hamlib CAT. Stop
   audio input/output.
4. Load target profile INI. Validate: does the CAT serial port exist? Do the audio
   devices exist? Are the UDP ports free? If validation fails: rollback to previous
   profile, show a non-blocking error toast, stay usable.
5. Reopen all resources with the new profile's values.
6. Restart decoder in the new profile's mode and frequency.
7. Update profile toolbar state. Update title bar with profile name.

Target switch time: < 3 s for well-configured profiles, bounded by physical CAT and
audio device open latency.

### 4.4 Integrity rules

- Cannot delete the currently active profile.
- Cannot assign the same F-key to two slots.
- If all five slots are empty on first launch, the toolbar explains how to create the
  first profile and points to Settings → Generali → "Profile slot".
- Saving a profile with an invalid CAT port or audio device is allowed (user might
  plug hardware later), but switching to it surfaces a clear error.

---

## 5. Auto-call

### 5.1 Placement

A new "Auto-call" tab in the Impostazioni dialog, positioned after "Filtri", before
"Programmazione". All flags default OFF.

### 5.2 UI elements

- Red warning banner (same wording as FT8 Card Pro): unattended TX risk, amplifier
  risk, user responsibility. Explicitly states "Never leave the station unattended
  with auto-call ON while connected to an amplifier or timer-based power strip."
- Seven category checkboxes:
  1. Alert callsigns (5 text inputs for callsigns the user explicitly wants auto-called)
  2. NEW DXCC entity
  3. NEW CQ zone
  4. NEW ITU zone
  5. NEW grid (4-char)
  6. NEW prefix
  7. NEW callsign (never worked)
- First-enable confirmation dialog, per category.
- Hardcoded safeguards shown in small text: "Each callsign auto-called at most once
  per 120 s. Global limit: 3 auto-calls per 60 s."
- "Master enable" switch at the top — a single kill-switch that disables all categories
  at once.

### 5.3 Runtime flow

For every new decode from the JTDX decoder pipeline:

1. Pass the decode through the detector pipeline in this priority order: Alert
   callsigns > NEW DXCC > NEW CQ zone > NEW ITU zone > NEW grid > NEW prefix >
   NEW callsign. First match wins.
2. If no active category matches → return, no auto-call.
3. Check per-callsign cooldown: `_autocall_last[callsign]`. If < 120 s, skip and log
   "(cooldown)".
4. Check global rate-limit: `_autocall_recent` deque of timestamps in the last 60 s.
   If size ≥ 3, skip and log "(rate limit)".
5. Trigger the internal "call this decode" action (the same action that JTDX's
   existing double-click on a decode line performs). This stages the QSO.
6. Update `_autocall_last[callsign] = now` and push `now` to `_autocall_recent`.
7. Emit a status bar message and append a line to ALL.TXT:
   `🤖 AUTO-CALL → <callsign> (<category>)`.

### 5.4 Badge

The flashing "AUTO-CALL · N" label (N = number of active categories) appears in the
top bar whenever any category of the active profile is ON. Blink rate ~1 Hz.
Tooltip: "Auto-call active for N categories. Review in Settings → Auto-call."

### 5.5 Worked-before cache

The "NEW *" detectors need a fast worked-before lookup. On startup, parse the active
log file (`wsjtx_log.adi` or the per-profile log) into in-memory indexes:

- Set of worked DXCC prefixes
- Set of worked (CQ zone, ITU zone)
- Set of worked 4-char grids
- Set of worked callsigns

Index update is O(1) on every logged QSO.

### 5.6 Per-profile scoping

Each profile owns its own auto-call state: category flags, alert list, and cooldown/
rate-limit counters. Switching profiles atomically swaps the state. This is important:
two radios on different bands have different "new" sets.

---

## 6. Logging and export

### 6.1 Routing

At QSO log time, `LogRouter` inspects the active profile's log path field:

- Empty → write to the global shared log (JTDX default behavior).
- Non-empty → write to the per-profile ADIF file at that path.

Both paths are always valid ADIF files, importable into any logger.

### 6.2 eQSL upload

Already implemented in JTDX (Segnalazioni tab → "Abilita l'invio a eQSL" + user/pass/
nickname). Preserved as-is. Credentials are per-profile (live in the profile INI).

### 6.3 qrz.com upload

If JTDX rc160 already has a qrz.com upload (check at implementation time): preserve
and expose per-profile credentials. If not: add `QrzUploader` — an HTTPS POST of each
QSO's ADIF record to `https://logbook.qrz.com/api` with the profile's API key. Failure
modes logged to `ALL.TXT`, no retry storm.

---

## 7. Networking (three endpoints, per-profile)

The existing JTDX Segnalazioni tab keeps its three endpoints; ports and hosts become
**per-profile**. This is what realizes the user's "personalized UDP ports for the
other FT8 card" requirement: profile A uses 2237/2236/52001, profile B uses
2247/2246/52011, etc., so the two instances of the user's workflow never clash.

- **Primary UDP**: decode stream, status, heartbeat (default 127.0.0.1:2237). Target
  consumers: GridTracker, JTAlert, FT8 Card Pro.
- **Secondary UDP**: optional mirror (default 127.0.0.1:2236). Target: PSK Reporter,
  third-party tools.
- **TCP ADIF**: QSO logging feed (default 127.0.0.1:52001). Target: N1MM+, DXkeeper,
  Log4OM.

No fourth endpoint (user confirmed three are enough).

---

## 8. Build environment

### 8.1 Windows toolchain (v1.0)

- MSYS2 (UCRT64 or MinGW64 environment).
- Packages: `mingw-w64-x86_64-{gcc,gcc-fortran,qt5,qwt-qt5,fftw,boost,cmake,ninja}`,
  plus `libusb` for some rigs.
- JTDX-Hamlib fork built from source (JTDX-specific Icom CI-V patches).
- Build:
  ```sh
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
  ninja -C build
  ```
- First clean build realistically takes 1–3 days of debugging dependency and path
  issues — this is a known difficulty of WSJT-X/JTDX on Windows.

### 8.2 Installer

NSIS script derived from JTDX's `install/`, rebranded. Produces
`wkjtx-setup-<version>.exe`. Not code-signed (no paid certificate).

### 8.3 CI (phase 2)

GitHub Actions workflow with `windows-latest` runner for automated Windows builds on
tag push. Linux (`ubuntu-latest`) and macOS (`macos-latest`) workflows deferred to v2.

---

## 9. Release and distribution

- GitHub repository (public, user's account), GPL-3.0 license file present at root.
- Releases via GitHub Releases: `.exe` installer + `.zip` portable bundle.
- Static GitHub Pages site optional (v2): download page, changelog, quick start.
- `README.md`, `CHANGELOG.md`, `INSTALL-WKjTX.md`, `USER-GUIDE.md` at repo root.

---

## 10. Testing

### 10.1 What Claude can test

- Clean build on the user's Windows 11 machine.
- Launch test: app opens, no crash, all tabs accessible.
- Profile operations: create 3 mock profiles, switch F1/F2/F3, verify INI files and
  UI state update correctly.
- Auto-call in dry-run mode (no TX): injected fake decodes trigger the correct
  category, cooldown, and rate-limit behavior; status bar + ALL.TXT updated; no
  actual `Enable Tx` fired.

### 10.2 What only the user can test

- CAT control on real radios.
- Real audio flow with real soundcards.
- Decoding quality on real off-air signals.
- Profile switching with two different physical radios plugged in.
- Auto-call with TX enabled (with amplifier OFF, with caution).

### 10.3 Regression guard

For every change that touches anything downstream of the decoder, run: decode a known
reference `.wav` file in both WKjTX and JTDX rc160 upstream, compare output line-by-
line. They must match. If they don't, the change is reverted — we never silently
touch the decoder.

---

## 11. Scope boundaries (explicit)

**In scope for v1.0**:

- Rebrand.
- 5-slot profile system with F1–F5.
- Per-profile log routing.
- Auto-call feature (7 categories, safeguards, badge).
- qrz.com upload (add or verify).
- Windows 10/11 64-bit build + NSIS installer.

**Out of scope for v1.0** (deferred or excluded):

- Integration with the user's other apps.
- Rewriting or modifying Fortran decoders.
- New digital modes.
- Linux and macOS builds (deferred to v2).
- Code-signed installer.
- Parallel multi-radio operation.
- Fourth UDP/TCP endpoint.
- Mobile (Android/iOS) — permanently out, architecturally impossible without a full
  rewrite.

---

## 12. Timeline (rough, best-case)

| Phase | Working days |
|---|---|
| MSYS2 + JTDX rc160 baseline clean build on target machine | 1–3 |
| Rebrand pass (names, icons, paths, About) | 1 |
| Profile toolbar + F1–F5 + Configurations extension | 3–5 |
| Per-profile log path + per-profile auto-call wiring | 1 |
| Auto-call tab + detector port from Python to C++ | 4–6 |
| qrz.com upload (implement or verify) | 1–2 |
| NSIS installer rebrand + `README`, `INSTALL`, `USER-GUIDE` | 1–2 |
| **Total, optimistic** | **~12–20 working days, i.e. 2–3 calendar weeks** |

Real hardware testing (by the user) runs in parallel from the moment the first build
works. Bug-fix iterations are additive on top of the baseline estimate.

---

## 13. Open questions (to resolve before implementation)

1. Does JTDX rc160 already ship a working qrz.com upload? First implementation step is
   to check. If yes, we just expose credentials per-profile. If no, `QrzUploader` is
   new code.
2. Does JTDX rc160 build cleanly with the latest MSYS2 Qt 5.15.x, or does it require
   a pinned older Qt? Determined at first build.
3. Exact binary name in v1.0: `wkjtx.exe` confirmed; co-tools (`qmap`, `map65`) —
   keep and rebrand, or drop? Decision deferred to implementation.

---

## 14. Validation checklist for "v1.0 ready"

- [ ] Clean build from source on Windows 11 using documented toolchain.
- [ ] Launch test passes: app opens, all tabs visible, no crash.
- [ ] Five profile slots created, F1–F5 switch correctly, INI files round-trip.
- [ ] Switching profile with different CAT + audio reopens resources correctly.
- [ ] Per-profile log path routes correctly to shared vs. dedicated ADIF.
- [ ] Auto-call dry-run: categories trigger correctly, cooldown 120 s respected,
      rate-limit 3/60 s respected, badge flashes.
- [ ] eQSL upload works from at least one profile.
- [ ] qrz.com upload works from at least one profile.
- [ ] NSIS installer produces a working install → uninstall cycle.
- [ ] Regression guard: decode output on reference `.wav` matches JTDX rc160 upstream.
- [ ] Credits to WSJT-X and JTDX preserved in About, README, and source headers.

---

*End of design document.*
