# WKjTX

**Weak-signal HF digital modes for Windows.** WKjTX 1.2 is a freeware
fork of [JTDX](https://sourceforge.net/p/jtdx/), itself forked from
[WSJT-X](https://wsjt.sourceforge.io/wsjtx.html) (K1JT). It speaks
FT8, FT4, JT65, JT9, JT9+JT65, T10 and WSPR-2 — same decoders, same
operating modes as upstream.

## What's new in 1.2 — qrz.com Logbook · Auto-CQ · NTP time sync

### qrz.com Logbook upload + download

![qrz.com Logbook settings — API Key, Upload mode: Automatic, eQSL mode: Automatic](docs/screenshots/v1.2.0-qrz-settings.png)

Settings → Reporting → qrz.com Logbook. Paste your API Key, pick
**Automatic** (push every QSO on Log) or **Manual queue only**.
Failed uploads park in a persistent queue with per-row retry;
eQSL shares the same UX. Download your full qrz.com log for
Worked-B4 colouring from **File → Download log from qrz.com…**

![File menu — Upload pending QSOs / Download log from qrz.com](docs/screenshots/v1.2.0-qrz-download-menu.png)

### Auto-CQ after QSO

![AutoSeq menu — Auto-CQ after QSO (Ctrl+Shift+Q)](docs/screenshots/v1.2.0-autocq-menu.png)

**AutoSeq → Auto-CQ after QSO** (`Ctrl+Shift+Q`). After every
logged QSO, WKjTX re-arms Tx6 (CQ) and re-enables TX for a
user-configurable window (1…999 min). The window extends on every
new QSO inside it. Halt TX is always a hard stop.

### NTP time-sync badge

![NTP +11 ms badge in the menubar corner next to the IC-7300 radio profile](docs/screenshots/v1.2.0-ntp-badge.png)

Live clock drift vs `pool.ntp.org` on the menubar — green under
100 ms, amber 100–500 ms, red otherwise. Click to step the clock
via PowerShell `Set-Date` (works even with Windows Time disabled).
Right-click for optional 10-min auto-sync install.

### Also in 1.2

- **19 languages shipped out of the box** — Italian, English,
  Spanish, French, German-adjacent Dutch, Portuguese (BR/PT),
  Russian, Japanese, Chinese (simplified & traditional) and more,
  bundled as Qt resources. Pick from the *Language* menu.
- **UDP `SwitchProfile` (type 52) + `EnableTx` (type 53)** for
  companion tools (Stream Deck plugin, external keyers).
- **UPDATE DATA TLS fix** — one-click refresh works on fresh
  portable installs again.

### Earlier releases

- **v1.1.1** — Day/Night theme toggle · date-filtered ADIF export · theme-switch fix.
- **v1.1.0** — 3-slot radio profile quick-switch in the menubar corner.

## What's different from JTDX

- **Auto-call** — *File → Auto-call...* opens a dedicated dialog
  with a master switch and 7 trigger categories (Alert callsigns,
  NEW DXCC, NEW CQ zone, NEW ITU zone, NEW grid 4-char, NEW prefix,
  NEW callsign). When a decoded message matches an active category,
  WKjTX **transmits a reply automatically**. Five alert-callsign
  slots for explicit watch lists. Hardcoded safeguards (not
  user-editable): each callsign called at most once every 120 s,
  global cap of 3 auto-calls per minute. A flashing badge in the
  status bar shows when at least one category is ON, so you always
  know if the station is hot.

  ![Auto-call dialog](docs/screenshots/auto-call-dialog.png)

  > **Use responsibly.** With auto-call ON the station transmits
  > unattended. You are responsible for staying within your licence
  > and never leaving an auto-calling station hooked up to a linear
  > or a timer-driven antenna switch.

- **More reliable CQ / QSO responses**: the auto-sequencer no longer
  drops replies in marginal copy and no longer gets stuck in endless
  CQ loops. You answer the station that just answered you, and you
  stop calling CQ when nobody's coming back — the way it should
  always have worked.
- **One-click data refresh**: a single *Update data* button in
  Settings → General fetches `cty.dat`, `state_data.bin`,
  `grid_data.bin` and `lotw-user-activity.csv` from their official
  sources, with editable URL fields so you can point at a mirror.
- **Log import / export in-app**: read a third-party `.adi` file
  into the internal log (dedup on CALL + QSO\_DATE + BAND + MODE);
  export a snapshot without leaving the app.
- **Font size spinboxes** next to the existing *Application Font...*
  and *Decoded Text Font...* buttons — change point size with one
  click.
- **English-only binary**, user-supplied translations: drop your
  compiled `wkjtx_<locale>.qm` into `bin/translations/` and switch
  *Language* in `JTDX.ini`. Legacy `jtdx_<locale>.qm` names are
  accepted as-is. See
  [`bin/translations/README.txt`](bin/translations/README.txt) in
  each release.
- **5 UI themes** selectable from the *Tema* menu: **Amber Classic**
  (default warm dark), **Amber Night** (deeper black), **Amber High
  Contrast**, **Native** (OS default style) and **Dark (legacy JTDX)**.
  The active theme is persisted across sessions.
- **Hamlib updater** opens the JTDX SourceForge Hamlib directory in
  your browser — manual drop-in of `libhamlib-5.dll` with an
  automatic `_old` backup slot.

Planned in later releases: per-profile log routing, third outgoing UDP port, FT2 mode.

## Download

Latest portable build: see the
[**Releases**](https://github.com/iu2vwk-ita/WKjTX/releases) page. Each
release ships a `WKjTX-<version>-portable-win64.zip` with everything
bundled — no installer, no system changes.

## Install from source

See [INSTALL-WKjTX.md](INSTALL-WKjTX.md) for the full MSYS2 MINGW64
build pipeline (Qt5, Hamlib, FFTW, GNU Fortran). Typical build
time on a modern laptop: 10–30 minutes.

## User guide

[USER-GUIDE.md](USER-GUIDE.md) covers first-run configuration,
the *Data updates* section, Import/Export ADIF, the font spinboxes,
and language swapping via `.qm` files.

## Support the project

WKjTX is free and will stay free. If it saves you time or lands you a new DXCC,
consider buying me a beer — it keeps the coffee hot and the commits coming.

[![Buy me a beer](https://img.shields.io/badge/PayPal-Buy%20me%20a%20beer%20%F0%9F%8D%BA-0070ba?logo=paypal&logoColor=white&style=for-the-badge)](https://www.paypal.com/donate/?business=adivor%40gmail.com&currency_code=EUR&item_name=Buy+IU2VWK+a+beer+%F0%9F%8D%BA)

## License

GPL-3.0, inherited from WSJT-X and JTDX. Full text in [COPYING](jtdx-source/COPYING).

## Credits

- **WSJT-X** — Joe Taylor K1JT and the WSJT Development Group
  (FT8/FT4/JT65/JT9/WSPR decoder heritage).
- **JTDX** — Igor Chernikov UA3DJY, Arvo Järve ES1JA and the JTDX
  community (HF-focused fork, auto-sequencer, decoder tuning).
- **WKjTX additions** — IU2VWK.

WKjTX is an **independent** fork and is **not endorsed** by the
WSJT-X or JTDX projects. Bug reports for WKjTX-specific code should
be filed here, not upstream.
