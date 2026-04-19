# Changelog

All notable changes to WKjTX are documented in this file.

The format is based on Keep a Changelog. This project follows
semantic versioning adjusted for a fork: the `2.2.x` in the
upstream JTDX version is preserved in internal Versions.cmake
for code compatibility, but public releases are tagged
`v0.1.0` → `v1.0.0` reflecting WKjTX's own delivery phases.

## [v0.1.0] — 2026-04 — Baseline rebrand

Initial public release of WKjTX. Pure rebrand of JTDX 2.2.159
(SourceForge `p/jtdx/code` master HEAD `2a0e2bea`) with no
functional changes.

### Changed

- Executable renamed `jtdx` → `wkjtx` (binary, CMake target,
  install target, CPack package).
- CMake project name and `PROJECT_NAME` set to `WKjTX`.
- Qt application identity (`setApplicationName`,
  `setOrganizationName`, `setOrganizationDomain`) set to
  `WKjTX` / `wkjtx.local`.
- AppData path is now `%LOCALAPPDATA%\WKjTX\` (was
  `%LOCALAPPDATA%\JTDX\`).
- Main window title (`program_title()`): drops the JTDX
  "by HF community" phrasing; reads
  "WKjTX v<version> <rev> — fork of JTDX, derivative of
  WSJT-X by K1JT".
- About dialog title: "About WKjTX".
- About dialog body: expanded with WKjTX independent-fork
  disclaimer, full WSJT-X credits preserved, full JTDX credits
  preserved, WKjTX additions credit line for IU2VWK.
- NSIS installer: install path `C:\WKjTX64` (was `C:\JTDX64`),
  executable `wkjtx.exe`, desktop link `wkjtx`.

### Unchanged (on purpose)

- All Fortran decoder code under `lib/` (LDPC, OSD, Costas sync,
  modulators, FT4/FT8/JT65/JT9/T10/WSPR decoders).
- Audio, modulator, decoder, and Hamlib wrapper core logic.
- All 11 Impostazioni tabs: Generali, Radio, Audio, Sequenza,
  Macro TX, Segnalazioni, Frequenze, Notifiche, Filtri,
  Programmazione, Avanzate.
- All operating modes: FT8, FT4, JT65, JT9+JT65, JT9, T10,
  WSPR-2.
- AutoSeq, DXpedition, Super Fox, filters, highlighting rules,
  band scheduler — all preserved exactly as JTDX ships them.
- All translation files (`translations/jtdx_*.ts`) — filenames
  intact, content intact; label strings not remapped.
- Icon artwork — JTDX icon files reused byte-for-byte under
  their existing filenames. Visual rebranding deferred to a
  later release.
- Internal helper executables: `jtdxjt9`, `wsprd_jtdx`,
  `ft4sim`, `ft8sim`, `jt65sim`, `jt9sim`, `jt10sim`,
  `jt65code`, `jt9code` — left with original names because they
  are not user-visible and changing them would ripple into many
  CMake and resource references for no functional gain in v0.1.

### Fixed

Nothing. This release introduces no bug fixes relative to
JTDX 2.2.159.

### Security

Nothing new.

---

## Upcoming (not released)

### [v0.2.0] — 5-profile quick switch

Planned: add a persistent 5-slot profile toolbar with F1–F5
hotkeys, per-profile CAT/audio/UDP/log/macros, safe-switch
protocol.

### [v0.3.0] — Auto-call

Planned: port the auto-call feature from FT8 Card Pro
(PySide6 Python) natively to C++/Qt inside WKjTX, with seven
trigger categories, 120 s per-callsign cooldown, 3 auto-calls
per 60 s global rate limit, flashing badge, first-enable
confirmation dialog.

### [v1.0.0] — Per-profile log routing, qrz.com upload, polish

Planned: per-profile log path routing (shared vs. dedicated
ADIF), qrz.com Logbook API upload (ADIF HTTP POST), polished
NSIS installer, USER-GUIDE.md.

### [v2.0.0] — Cross-platform

Deferred: Linux (`.deb`, AppImage) and macOS (`.dmg` unsigned)
builds via GitHub Actions CI. Windows primary, others
best-effort.
