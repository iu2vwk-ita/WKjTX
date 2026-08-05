# WKjTX

Qt/C++ ham radio FT8/JT65 application — fork of JTDX 2.2.159.

- **Versioning gotcha**: in `Versions.cmake`, `WSJTX_VERSION_32A` is a **boolean flag** (appends a `-32A` suffix), NOT a patch counter — do not increment it for patch releases; bump `WSJTX_VERSION_SUB` instead.
- **Release workflow**: every version tag → GitHub Release on `iu2vwk-ita/WKjTX`, notes from `CHANGELOG.md`. FTP deploy to `iu2vwk.com/wkjtx/` is manual.
- **UDP protocol**: custom message types added — SwitchProfile (type 52), EnableTx (type 53), used by the Stream Deck companion.
- **Do NOT** rebase on the WSJT-X 3.0 base — no decoder quality gain, 4–8 week rebase cost. Cherry-pick patches instead.
