# WKjTX User Guide

> **Version status**: this guide documents the full v1.0.0 feature set.
> Individual chapters are marked with the minimum version that implements
> them. v0.1.0 ships only the rebrand — all other features arrive in
> v0.2.0 → v1.0.0.

WKjTX is a freeware Windows build based on [JTDX](https://sourceforge.net/p/jtdx/).
Everything JTDX does, WKjTX does identically — same decoders, same
operating modes (FT8, FT4, JT65, JT9, JT9+JT65, T10, WSPR-2), same
eleven settings tabs (Generali, Radio, Audio, Sequenza, Macro TX,
Segnalazioni, Frequenze, Notifiche, Filtri, Programmazione, Avanzate).

WKjTX **adds** a set of workflow features focused on operators who
switch radios frequently and want hands-free calling for specific
triggers.

---

## Table of Contents

1. [Quick Start (v0.1.0+)](#1-quick-start-v010)
2. [Five Radio Profiles (v0.2.0+)](#2-five-radio-profiles-v020)
3. [Auto-call (v0.3.0+)](#3-auto-call-v030)
4. [Per-profile Log Routing (v1.0.0+)](#4-per-profile-log-routing-v100)
5. [qrz.com Upload (v1.0.0+)](#5-qrzcom-upload-v100)
6. [Troubleshooting](#6-troubleshooting)
7. [Upgrading from JTDX](#7-upgrading-from-jtdx)

---

## 1. Quick Start (v0.1.0+)

### Install

Download the latest `wkjtx-setup-<version>-win64.exe` from the
[releases page](https://github.com/IU2VWK/WKjTX/releases). Run the
installer. Default install directory is `C:\Program Files\WKjTX\`.

On first launch Windows may show a SmartScreen warning because the
installer is not code-signed. Click "More info" → "Run anyway". If you
prefer not to trust unsigned installers, you can build from source —
see [INSTALL-WKjTX.md](INSTALL-WKjTX.md).

### First run

Launch WKjTX from the Start menu. The window title reads:

> WKjTX v<version> <rev> — fork of JTDX, derivative of WSJT-X by K1JT

On first launch the settings dialog opens. Fill in:

- **Mio Nominativo**: your amateur radio callsign (e.g. `IU2VWK`).
- **Mio Locator**: your 4-to-6 character Maidenhead locator
  (e.g. `JN45`).
- **Regione IARU**: your IARU region (Europe = Region 1,
  Americas = Region 2, Asia/Pacific = Region 3, or `All`).
- **Radio** tab: rig model, CAT serial port, baud, PTT method.
- **Audio** tab: input device (receiver side) and output device
  (transmitter side).

Click **Conferma** to save. The waterfall and decode list should start
populating on the FT8 calling frequency of the band your rig is on.

### Files and folders

WKjTX stores configuration and logs in:

```
%LOCALAPPDATA%\WKjTX\
├── WKjTX.ini              # main settings
├── wsjtx_log.adi          # global QSO log (ADIF)
├── ALL.TXT                # decoder event log (plain text)
├── save\                  # WAV files saved by "Save .wav" option
└── profiles\              # (v0.2.0+) 5 profile INIs
```

If you need to reset WKjTX to defaults, quit the app and rename or
delete `%LOCALAPPDATA%\WKjTX\`. It will be recreated on next launch.

---

## 2. Five Radio Profiles (v0.2.0+)

A **profile** is a full set of settings — CAT, audio, UDP ports, log
path, macros, notifications, filters, auto-call config — tied to a
specific radio. You can have up to five profiles and switch between
them with one click or an F-key (F1..F5).

Only one profile is active at a time. When you switch, WKjTX closes
the active profile's hardware resources (CAT, audio, UDP), loads the
new profile's INI, and reopens with the new values.

### Creating your first profile

1. Open **Impostazioni** (the wrench icon).
2. In the **Generali** tab, find the "Profile slot" selector.
3. Pick slot **1**, give it a name like `IC-7300` and (optionally) a
   color tag.
4. Assign a hotkey: **F1**.
5. Configure all the other tabs as usual (Radio, Audio, Segnalazioni,
   etc.) for this specific rig.
6. Click **Conferma** to save. The slot 1 toolbar button now shows
   `IC-7300` in its label.

Repeat for slots 2–5 with your other radios.

### Switching profiles

- **Click** the profile button in the toolbar, or
- **Press F1..F5** — wherever your focus is in the app.

If a transmission is in progress, WKjTX asks "Fermare la trasmissione
e cambiare profilo?". Click "Sì" to stop TX and switch; "No" to stay.

The switch takes about 1–3 seconds, limited by how fast your radio's
CAT interface closes and reopens. If something fails during the switch
(wrong COM port, audio device unplugged, UDP port in use), WKjTX
rolls back to the previous profile and shows an error toast explaining
what went wrong.

### UDP ports per profile

Each profile owns its own three network endpoints in the
**Segnalazioni** tab:

- **Server UDP primario** — default `127.0.0.1:2237`.
- **2° Server UDP** — default `127.0.0.1:2236`.
- **Server TCP ADIF** — default `127.0.0.1:52001`.

Use **different ports per profile** if you run two WKjTX instances or
share your system with JTDX or WSJT-X. For example:

- Profile 1 (IC-7300 in the shack): UDP 2237, UDP 2236, TCP 52001
- Profile 2 (FT-991A portable): UDP 2247, UDP 2246, TCP 52011

External tools (GridTracker, JTAlert, N1MM+, PSK Reporter) then
connect to the specific port set matching the active profile.

### Importing from JTDX

Menu: **File → Import profile from JTDX...**

Pick an existing JTDX `.ini` file (usually under
`%LOCALAPPDATA%\JTDX\`), then choose the WKjTX slot to overwrite.
All JTDX settings are copied over and the slot becomes active.

### Rules and limits

- The currently active profile cannot be deleted. Switch to another
  slot first, then delete.
- Two different slots cannot share the same F-key. If you assign F2
  to a second slot, the first assignment is cleared.
- An empty slot shows "Empty — click to create" in the toolbar.

---

## 3. Auto-call (v0.3.0+)

> ⚠ **Safety warning read this first.**
> Auto-call transmits on air without operator intervention. It is your
> legal responsibility (under your license) to know and limit what
> your station broadcasts. **Never** leave the station unattended with
> auto-call enabled while connected to an amplifier, a linear, or a
> timer-based power strip. WKjTX provides safeguards — they reduce
> risk, they do not eliminate it.

Auto-call automatically sends a Reply to a decoded station when one of
seven categories matches. All categories are **OFF by default**.
Enabling any category requires a confirmation click.

### The seven categories

| Category | Fires when the decoded station is... |
|---|---|
| **Alert** | one of the up-to-5 callsigns you explicitly added to the watch list. |
| **NEW DXCC** | from a DXCC entity you have never logged. |
| **NEW CQ zone** | in a CQ zone you have never logged (based on grid). |
| **NEW ITU zone** | in an ITU zone you have never logged (based on grid). |
| **NEW grid (4-char)** | from a 4-char Maidenhead grid you have never logged. |
| **NEW prefix** | has an operator prefix (e.g. `IU2`, `K1`, `PY2`) you have never worked. |
| **NEW callsign** | has an exact callsign you have never worked. |

If two or more categories match the same decode, the **higher-priority
one wins**, and auto-call fires exactly once. Priority order:
Alert → NewDxcc → NewCqZone → NewItuZone → NewGrid → NewPrefix →
NewCallsign.

### Hardcoded safeguards

These cannot be changed by the user:

- **Per-callsign cooldown**: each callsign can auto-call at most once
  every 120 s. Stops runaway repeat calls during a pileup.
- **Global rate limit**: no more than 3 auto-calls in any rolling 60 s
  window. Even if many triggers fire at once, only three take effect
  per minute.

When a trigger is suppressed by a safeguard, WKjTX writes a line to
ALL.TXT (`🤖 AUTO-CALL SUPPRESSED K1ABC (cooldown 120s)`) and shows a
hint in the status bar — it never silently swallows events.

### Enabling auto-call

1. **Impostazioni → Auto-call** tab.
2. Read the red warning banner.
3. Tick the **Master enable** switch at the top.
4. Tick the category or categories you want.
5. For the **Alert** category, fill in up to 5 callsigns in the text
   boxes.
6. Confirm the first-enable dialog ("Yes, I understand the unattended
   TX risk").

While any category is ON, a flashing red **AUTO-CALL · N** badge in
the top bar tells you how many categories are armed. The badge
disappears when all categories are off or you uncheck Master enable.

### Per-profile auto-call

Each profile owns its own auto-call configuration. If profile 1 has
`NEW DXCC` ON and profile 2 has everything OFF, switching from 1 to 2
disarms auto-call immediately. The badge updates atomically with the
profile switch.

### What "NEW" means

The "NEW" categories consult your **logged** QSO history:

- If you use a **shared global log** (default), "new" means
  "never seen in `wsjtx_log.adi`".
- If the active profile has its **own log file** (v1.0.0+), "new"
  means "never seen in that file" — so NEW DXCC on the portable
  profile can differ from NEW DXCC on the shack profile.

WKjTX scans the active log on startup into an in-memory index. Every
QSO you log updates the index on the fly.

### Stopping auto-call immediately

Three options, fastest first:

- Press **F12** (or click **Halt TX** on the main window) — stops any
  in-progress transmission immediately.
- Click the flashing **AUTO-CALL** badge — opens the auto-call tab so
  you can flip off the Master enable.
- Disconnect PTT or power-off the amplifier — the physical kill
  switch. Keep it within reach.

---

## 4. Per-profile Log Routing (v1.0.0+)

By default all QSOs go to a single shared file, `wsjtx_log.adi`,
regardless of which profile is active. WKjTX lets you override this
**per profile**.

### How to enable

1. Open a profile in **Impostazioni → Generali**.
2. Scroll to the **Log path** field.
3. Leave it **empty** to use the shared global log (default behavior).
4. Fill it with an explicit `.adi` file path to give this profile its
   own dedicated log.

Example:

- Profile "IC-7300 shack" → log path empty → QSOs land in
  `%LOCALAPPDATA%\WKjTX\wsjtx_log.adi`.
- Profile "FT-991A portable" → log path
  `C:\Users\adivo\Documents\FT991-portable.adi` → only this profile's
  QSOs go there.

The global log still captures **everything** from shared-mode
profiles. Per-profile logs are **additional** destinations, not
replacements (unless you explicitly opt out of the shared log).

### When to use per-profile logs

- Field day / contest / POTA activations where you want an isolated
  log per event.
- Two separate operator identities (club call + personal call) on the
  same PC.
- Per-rig statistics without manual ADIF filtering.

### When NOT to use per-profile logs

- Daily operating. The shared log is easier — everything in one place,
  one click to upload to LoTW / ClubLog / eQSL / qrz.com.

### Auto-call worked-before caching

Remember from the auto-call chapter: "NEW DXCC" (and the other NEW
categories) consult the **active profile's** log. If you switch from
the shared log to a private log halfway through a session,
previously-worked DXCCs from the shared log no longer suppress "NEW"
triggers.

---

## 5. qrz.com Upload (v1.0.0+)

WKjTX can upload each new QSO to your qrz.com Logbook automatically.
Configured per-profile.

### Setup

1. Log into qrz.com and go to Logbook → Settings. Find your
   **API key** (a long string starting with something like
   `XXXX-XXXX-XXXX-XXXX`).
2. In WKjTX: **Impostazioni → Segnalazioni** tab.
3. Find the **qrz.com** section.
4. Paste your API key.
5. Tick **Abilita upload qrz.com**.
6. Click **Conferma**.

### How it works

Every time you log a QSO with **Log QSO** (Ctrl+Alt+L), WKjTX:

1. Serializes the QSO to ADIF.
2. POSTs to `https://logbook.qrz.com/api` with
   `KEY=<your-key>&ACTION=INSERT&ADIF=<record>`.
3. Waits for the response.
4. On `RESULT=OK` — a line appears in ALL.TXT:
   `qrz.com upload OK <call>`.
5. On `RESULT=FAIL&REASON=...` — a line appears in ALL.TXT with the
   reason, and a status-bar toast shows the failure.

WKjTX retries a network error **once**. If the second attempt fails,
the QSO is still in your local ADIF — you can re-upload manually from
qrz.com's Logbook → Upload page later.

### Per-profile credentials

Each profile can have its own API key. Use this if you have two qrz.com
accounts (e.g. personal + club). Switching profiles switches the key
used for auto-upload.

---

## 6. Troubleshooting

### WKjTX doesn't find my radio

- Check **Radio** tab: rig model must match your actual radio, baud
  must match the radio's menu setting, COM port must exist.
- Click **Testa il CAT**. Green = good. Red = check cable, USB driver,
  and that no other app has the COM port open (JTDX, MixW, Log4OM,
  DXKeeper).

### Decoder shows no decodes

- Check waterfall: you should see "noise" patterns in the frequency
  range. Flat green = audio input is silent.
- **Audio** tab: verify the input device matches your rig's USB Audio
  Codec or SignaLink device.
- Listen on the radio — are you actually on the FT8 frequency, with
  the right mode (USB / PKT-USB)?

### I enabled auto-call and it fires on callsigns I already worked

Likely the worked-before cache did not load. Causes:

- Log file path is empty or wrong for the active profile.
- The ADIF file is malformed (broken header, missing `<EOR>` markers).
- First launch after rebrand: cache rebuilds only on the first
  successful QSO log after startup; restart WKjTX to force a full
  log rescan.

### Profile switch takes too long or hangs

- CAT close-and-reopen on some rigs (particularly Icom IC-7300 on
  certain firmware) takes 2–3 seconds. Normal.
- If it hangs longer than 10 seconds, force-close WKjTX from Task
  Manager. The last good profile will be remembered on next launch.

### qrz.com upload says "FAIL REASON=invalid api key"

- Triple-check the key — copy-paste from qrz.com, no trailing spaces.
- Some plans require the Logbook API to be explicitly enabled in your
  qrz.com subscription settings.

### ALL.TXT is huge and fills the disk

JTDX (and therefore WKjTX) appends to ALL.TXT without rotation.
Normal after a few months of daily use. Solutions:

- Rename `ALL.TXT` to `ALL.TXT.old` on a monthly schedule; WKjTX
  creates a fresh one.
- Or delete when WKjTX is not running.

ALL.TXT is not the QSO log — your QSOs are in `wsjtx_log.adi` or the
per-profile log. Don't confuse the two.

---

## 7. Upgrading from JTDX

WKjTX reads a JTDX `.ini` file format-compatibly. To carry your JTDX
settings into WKjTX:

1. Quit JTDX.
2. Install WKjTX.
3. Launch WKjTX. Let it create `%LOCALAPPDATA%\WKjTX\`.
4. Quit WKjTX.
5. Copy `%LOCALAPPDATA%\JTDX\JTDX.ini` to
   `%LOCALAPPDATA%\WKjTX\WKjTX.ini` (rename on copy).
6. Launch WKjTX. Your callsign, radio, audio, macros, notifications,
   filters, band schedule, reporting endpoints — all carry over.

Or use the **File → Import profile from JTDX...** menu (v0.2.0+) to
pick a JTDX INI and drop it into a WKjTX profile slot without touching
the filesystem manually.

You can run WKjTX and JTDX **side by side** on the same PC. Just
configure different UDP/TCP ports for each in their respective
Segnalazioni tabs (so they don't collide on 2237/2236/52001).

---

*Guide last updated 2026-04-19.*
