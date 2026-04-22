# Changelog

All notable changes to WKjTX are documented in this file.

The format is based on Keep a Changelog. This project follows
semantic versioning adjusted for a fork: the `2.2.x` in the
upstream JTDX version is preserved in internal Versions.cmake
for code compatibility, but public releases are tagged
`v0.1.0` → `v1.0.0` reflecting WKjTX's own delivery phases.

## [v1.1.3] — 2026-04-22 — Multilingual UI + UPDATE DATA TLS fix

### Added

- **19 languages shipped out of the box.** The JTDX-inherited
  translations (ca_ES, da_DK, en_US, es_ES, et_EE, fr_FR, hr_HR,
  hu_HU, it_IT, ja_JP, lv_LV, nl_NL, pl_PL, pt_BR, pt_PT, ru_RU,
  sv_SE, zh_CN, zh_HK) are now compiled to `.qm` and embedded as
  Qt resources. The corresponding menubar actions already existed
  from the JTDX baseline; they now actually switch the UI without
  a loose `.qm` drop-in. Upstream sources verified byte-identical
  with `jtdx-project/jtdx` on GitHub — no translation regressions
  relative to JTDX. User-supplied translations dropped in
  `bin\translations\wkjtx_<locale>.qm` or `jtdx_<locale>.qm`
  still take priority over the bundled resource.

### Fixed

- **UPDATE DATA failed with "TLS initialization failed"** on portable
  installs. The MSYS2-sourced OpenSSL 3 runtime that ships in `bin\`
  needs `OPENSSL_CONF`, `SSL_CERT_FILE`, and `SSL_CERT_DIR` pointing
  at its config + CA bundle, otherwise Qt's `QSslSocket` can't
  initialise and every HTTPS fetch (`state_data.bin`, `grid_data.bin`,
  `lotw-user-activity.csv`) aborts before the request leaves the app.
  `run.bat` now sets the three env vars to
  `%MSYS2_HOME%\mingw64\etc\ssl\...` before launching `wkjtx.exe`.
  UPDATE DATA works out of the box on a fresh portable unzip.
- **Incremental build broke on `OMNIRIG_EXE: unbound variable`**.
  `scripts/build-wkjtx.sh` skips the dependency block when the MSYS2
  toolchain is already present, so `OMNIRIG_EXE` was never exported
  on the second and later runs and `set -u` aborted the script
  before CMake ran. Added a default expansion
  (`: "${OMNIRIG_EXE:=/c/Program Files (x86)/Afreet/OmniRig/OmniRig.exe}"`)
  right before the CMake configure step so both cold and incremental
  builds pick up the standard OmniRig install path. No behaviour
  change when the dependency block has already set the variable.

## [v1.1.2] — 2026-04-22 — NTP clock-offset badge + one-click system resync

### Added

- **NTP time-sync badge** in the top-right menubar corner (next to the
  three radio profile buttons). Shows the live offset of the system
  clock against `pool.ntp.org`:
  - green when the drift is under 100 ms,
  - amber when between 100 ms and 500 ms,
  - red when 500 ms or more, or when NTP is unreachable.
- **One-click resync.** Clicking the badge triggers an elevated
  PowerShell `Set-Date` call that steps the system clock directly by
  the measured SNTP offset. The UAC prompt appears once per click.
  The clock is corrected even if Windows Time service (`w32tm`) is
  disabled or misconfigured — `Set-Date` goes straight to the Win32
  `SetSystemTime` API.
- **Auto-refresh** every 5 minutes (silent, no UAC, no admin) keeps
  the offset value up to date on the badge. A short re-query also
  fires 1.5 s after a manual resync so the badge immediately reflects
  the new clock state.
- **No-op guard**: if the last measured offset is within ±10 ms the
  badge no-ops on click instead of spawning a UAC prompt for a
  trivial delta.
- **Optional auto-sync every 10 minutes.** Right-click the badge →
  *Auto-sync system clock every 10 minutes (install, UAC)*. One
  elevated PowerShell call reconfigures Windows Time Service to
  poll `pool.ntp.org` every 600 s and restarts `w32time`. From then
  on the system clock stays aligned silently as SYSTEM — **no more
  UAC prompts** per sync. A second context-menu entry (*Restore
  Windows default sync*) reverts to `time.windows.com` with the
  stock 1-week interval.

## [v1.1.1] — 2026-04-22 — Theme quick-toggle, tab fix, date-filtered ADIF export

### Added

- **Day / Night quick toggle** in the *Tema* menu. Two dedicated entries at
  the top of the menu (and keyboard shortcuts `Ctrl+Shift+D` and
  `Ctrl+Shift+N`) flip the UI between the Amber Classic (Day) and Amber
  Night themes without opening Settings.
- **Date-filtered ADIF export.** The *Export ADIF log* action now opens
  a picker with five presets:
  - *Full log* — every QSO (previous behaviour).
  - *Since last export* — anchored to the date of the previous export;
    ideal for incremental LoTW or logger uploads.
  - *Last 7 days* / *Last 30 days* — common quick ranges.
  - *Custom date range* — arbitrary from/to via calendar popups.
  The output filename carries the range tag automatically (e.g.
  `wkjtx_log_since_20260415_20260422_113057.adi`) so repeated exports
  don't overwrite each other.

### Fixed

- **Tab-bar text truncation in non-default themes.** `QTabBar::tab` had
  `font-family: Cascadia Mono`, `letter-spacing: 1–2px`,
  `text-transform: uppercase`, and `padding: 6-7px 14-16px` in the amber
  themes. Combined they exceeded the Settings dialog width and Qt's
  `expanding: true` squeezed every tab to the same width, clipping the
  text on both sides (e.g. "enera" instead of "General"). Stripped all
  font / spacing / transform overrides from the themes, reduced padding
  to `3px 8px`, and added `qproperty-expanding: false` +
  `usesScrollButtons: true` so tabs keep their natural width and scroll
  when they overflow.
- **Theme switching didn't actually swap themes** after the first use.
  `Configuration::impl::set_application_font` re-applied the legacy
  QDarkStyleSheet every time it was called (startup, font change,
  Settings save), clobbering whatever amber theme was live. Added a
  `WKJTX_THEME_ACTIVE` marker to each amber QSS and an early-return in
  `set_application_font` that preserves the current sheet when the
  marker is present. `ThemeManager::applyTheme` now also keeps the
  legacy `UseDarkStyle` flag in QSettings in sync (true only for the
  *Dark (legacy JTDX)* theme) so both code paths agree.

## [v1.1.0] — 2026-04-21 — Radio profile quick-switch

### Added

- **3-slot radio profile buttons** in the top-right menubar corner (next
  to Help). Slot 1 mirrors the base app configuration. Slots 2 and 3
  are independent overlays stored under
  `%LOCALAPPDATA%/WKjTX/profiles/slot<N>.ini` — they never touch the
  main `WKjTX.ini`.
  - Left-click a named slot → immediate switch (apply radio + audio,
    reconnect transceiver).
  - Left-click an empty "+" slot → opens the compact configuration
    dialog.
  - Right-click → context menu (Configure / Rename / Hide / Clear).
  - Active slot shows an amber border, inactive stays gray.
- **RadioProfileDialog** for slots 2 and 3: three group boxes
  (General, Radio, Audio) with CAT serial port and PTT port as
  auto-detected dropdowns via `QSerialPortInfo::availablePorts()`,
  rig list pulled from the live `TransceiverFactory` (no second
  Hamlib registration), data/stop/handshake/DTR/RTS combos, audio
  input/output device pickers.
- **View → Show all profile buttons** menu action to re-display
  hidden slots.

### Fixed

- Profile switching no longer overwrites the base `WKjTX.ini`.
- Dialog no longer unregisters Hamlib globally on close (temporary
  `TransceiverFactory` eliminated).

## [v1.0.0] — 2026-04 — Auto-call system live

### Added

- **Auto-call feature (dry-run by default)**:
  - `File → Auto-call...` menu item opens a full settings dialog with
    red warning banner, master enable switch, 7 category checkboxes
    (Alert, NEW DXCC / CQ zone / ITU zone / grid / prefix / callsign),
    5 slots for alert callsigns, and a locked safeguard info panel
    (120 s per-call cooldown + 3 per minute global rate limit).
  - First-enable confirmation dialog — the first time any category
    is toggled ON, a modal dialog asks the operator to acknowledge
    the unattended-TX risk. Declining reverts the toggle.
    Acknowledgment is persisted per-install under QSettings
    `autocall/firstEnableAcknowledged`.
  - Flashing red "AUTO-CALL · N" badge in the status bar whenever
    any category is ON. N = number of active categories. Click the
    badge to open the settings dialog.
  - Pipeline: every decode displayed by JTDX's decoded text browser
    is fed through the full detector chain (prefix, grid, zones via
    ported dxhunter polygons, worked-before cache). First-match
    priority: Alert > NewDxcc > NewCqZone > NewItuZone > NewGrid >
    NewPrefix > NewCallsign.
  - Safeguards: per-callsign 120 s cooldown, 3/60 s rolling global
    rate-limit. Both locked in code, not user-configurable.
  - **Dry-run mode** for v1.0 initial release: on a trigger, the
    pipeline logs to the status bar ("🤖 AUTO-CALL armed for <call>
    (dry-run, no TX)") and to ALL.TXT, but does **NOT** send a reply
    packet. This lets the operator validate detection on real decodes
    before enabling actual TX. Full TX trigger arrives in v1.1.

- **qrz.com Logbook upload** (library ready, UI wiring in v1.1):
  - Real HTTPS POST implementation at `wkjtx::QrzUploader`.
  - URL-encoded body `KEY=...&ACTION=INSERT&ADIF=...` to
    `logbook.qrz.com/api`.
  - Parses RESULT=OK vs RESULT=FAIL&REASON=... responses.
  - One-shot (no retry storm on network failure — QSO stays in
    local ADIF).

### Deferred to v1.1

- **5-profile F1-F5 quickswitch toolbar**: skeleton in place
  (ProfileManager class), UI+safe-switch wiring requires deeper
  MainWindow refactoring than fits a single autonomous session. The
  user's "5 nominativi a scelta" requirement is already fulfilled by
  the auto-call Alert slots.
- **Per-profile log routing**: LogRouter library is built and tested;
  wire-up into `MainWindow::acceptQSO2` defers until ProfileManager
  is active (otherwise there's no "profile" context to route by).
- **qrz.com credentials UI + hook in acceptQSO2**: same reason —
  credentials are designed as per-profile. Adding global qrz.com
  upload that bypasses profiles would be rework when the profile
  system lands.

### Preserved (legal / safety)

- Upstream JTDX and WSJT-X attribution in the About dialog.
- All Fortran decoder code UNCHANGED.
- Safeguard constants (120 s / 3 per 60 s) NOT exposed to user.

---

## [v0.2.0] — 2026-04 — Amber theme + icon + full rebrand

### Added

- **Theme system** with 5 selectable presets via new "Tema" menu:
  - Amber Classic (default) — FT8 Card Pro-matched palette
  - Amber Night — dimmer amber for nighttime operating
  - Amber High Contrast — pure black + white + full-sat amber
  - Native — OS default (no stylesheet)
  - Dark (legacy JTDX) — original QDarkStyle for nostalgic users
  Choice persists across sessions via QSettings "theme/current".
  Live switching, no restart needed.
- **Custom WKjTX icon**: bold amber "W" on dark rounded square with
  signal-wave arcs in the top-right corner. Multi-size .ico (16-256
  px) generated by Pillow via `scripts/generate-icon.py` so it can
  be re-designed and regenerated without an SVG toolchain.

### Changed

- Helper executables renamed:
  - `jtdxjt9.exe` → `wkjtxjt9.exe`
  - `wsprd_jtdx.exe` → `wsprd_wkjtx.exe`
  Task Manager and Program Files show WKjTX-branded names.
- Font Chooser dialog title: "JTDX Decoded Text Font Chooser" →
  "WKjTX Decoded Text Font Chooser".
- Theme applied at app startup BEFORE MainWindow is shown so the
  first paint is already themed (no flash of default).

### Preserved (legal)

- Upstream JTDX and WSJT-X attribution in the About dialog.
  GPL-3.0 requires these and they remain visible.
- Fortran-side IPC identifiers (`mem_jtdxjt9`, `sem_jtdxjt9`,
  `attach_jtdxjt9_`, etc.): internal C-Fortran ABI, not user-visible.

---

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
