# Building WKjTX from source (Windows 10/11 64-bit)

WKjTX v0.1.0 is a rebrand of JTDX 2.2.159 (upstream). The build
process is the same as JTDX upstream — only the binary name,
window titles, installer name, and `%LOCALAPPDATA%` folder
differ.

## 1. Prerequisites

- Windows 10 or Windows 11, 64-bit.
- ~20 GB free disk space (MSYS2 + sources + build artifacts).
- Administrator account (MSYS2 installer prompts for UAC).

## 2a. Install OmniRig (Windows-only, freeware, one-time)

JTDX/WKjTX uses OmniRig by VE3NEA as one of the available transceiver
control drivers. The build process requires OmniRig to be registered
as a COM server on Windows so `dumpcpp` can generate a C++ wrapper
from it. Without OmniRig, cmake aborts with
`You need to install OmniRig on this computer`.

1. Go to <http://dxatlas.com/Download.asp>.
2. Download **Omni-Rig 1.20** (a ~1 MB ZIP).
3. Extract it and **right-click `setup.exe` → Run as administrator**.
   Accept default settings. The "Run as administrator" step is
   essential — without admin, the installer copies files but fails
   silently to register the COM server in `HKLM`, and the WKjTX
   build will still fail.
4. Done — OmniRig registers itself with Windows automatically.

If you already ran the installer without admin, recover with:

```
# Open cmd as Administrator, then:
"C:\Program Files (x86)\Afreet\OmniRig\OmniRig.exe" /regserver
```

This registers the COM server without a reinstall.

You do not have to use OmniRig at runtime (Hamlib is the alternative
and more commonly used). But its COM registration must exist for the
build to succeed.

## 2b. Install MSYS2

Download `msys2-x86_64-YYYYMMDD.exe` from <https://www.msys2.org/>
and run with default settings. Install path: `C:\msys64`.

After install, open **MSYS2 MINGW64** (not `MSYS2 MSYS`) from
the Start menu and run:

```bash
pacman -Syu --noconfirm
```

The shell closes. Reopen `MSYS2 MINGW64` and run:

```bash
pacman -Su --noconfirm
```

## 3. Install build dependencies

In `MSYS2 MINGW64`:

```bash
pacman -S --needed --noconfirm \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-gcc-fortran \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-pkgconf \
  mingw-w64-x86_64-make \
  mingw-w64-x86_64-qt5-base \
  mingw-w64-x86_64-qt5-tools \
  mingw-w64-x86_64-qt5-multimedia \
  mingw-w64-x86_64-qt5-serialport \
  mingw-w64-x86_64-qt5-websockets \
  mingw-w64-x86_64-qt5-translations \
  mingw-w64-x86_64-qt5-svg \
  mingw-w64-x86_64-qt5-activeqt \
  mingw-w64-x86_64-qwt-qt5 \
  mingw-w64-x86_64-fftw \
  mingw-w64-x86_64-boost \
  mingw-w64-x86_64-libusb \
  mingw-w64-x86_64-nsis \
  git \
  patch \
  unzip \
  autoconf \
  automake \
  libtool
```

## 4. Obtain WKjTX sources

If you cloned from the WKjTX git repository, you already have
everything needed under `jtdx-source/` (vendored JTDX 2.2.159
with WKjTX rebrand patches applied) and `third-party/jtdxhamlib/`
(vendored JTDX-Hamlib).

If you only have the patches and need to regenerate the tree:

```bash
cd /some/working/dir
git clone https://git.code.sf.net/p/jtdx/code jtdx-source
git clone https://git.code.sf.net/p/jtdx/hamlib third-party/jtdxhamlib
# then apply WKjTX patches from the releases page
```

## 5. Build JTDX-Hamlib (static install)

```bash
cd /g/Claude\ Local/WKjTX/third-party/jtdxhamlib
./bootstrap
mkdir -p build && cd build
../configure \
  --prefix=/g/Claude\ Local/WKjTX/build-artifacts/hamlib \
  --disable-shared --enable-static \
  --without-cxx-binding --disable-winradio
make -j$(nproc)
make install
```

Expected: `libhamlib.a` appears in
`/g/Claude Local/WKjTX/build-artifacts/hamlib/lib/`.

## 6. Build WKjTX

```bash
cd /g/Claude\ Local/WKjTX/jtdx-source
mkdir build-wkjtx && cd build-wkjtx
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="/g/Claude Local/WKjTX/build-artifacts/hamlib" \
  -DCMAKE_INSTALL_PREFIX="/g/Claude Local/WKjTX/build-artifacts/wkjtx" \
  ..
ninja
```

Expected build time on a modern laptop: 10–30 minutes.

## 7. Launch

```bash
./wkjtx.exe
```

The window title should read "WKjTX v2.2.159 ... — fork of JTDX,
derivative of WSJT-X by K1JT". Help → About should show the
WKjTX-branded dialog with upstream credits preserved.

## 8. Create Windows installer

```bash
cd /g/Claude\ Local/WKjTX/jtdx-source/build-wkjtx
cpack -G NSIS
```

Produces `wkjtx-2.2.159-win64.exe` in the build directory.

## Known build gotchas

- **`character-length argument` Fortran error** during rc160-era
  builds with modern gfortran: add
  `-DCMAKE_Fortran_FLAGS="-fallow-argument-mismatch"` to the cmake
  invocation in section 6.
- **`undefined reference to png_*`**: install
  `mingw-w64-x86_64-libpng`.
- **`Qt5 not found`**: ensure you are in `MSYS2 MINGW64`, not
  `MSYS2 MSYS` or `MSYS2 UCRT64`.
- **`Could not find hamlib`**: your `CMAKE_PREFIX_PATH` does not
  point to the directory where `make install` placed hamlib in
  section 5. Re-check the path.
- **`libtool: install: target '...' is not a directory`** during
  Hamlib's `make install`: classic autotools bug when the install
  `--prefix` contains a space character. The automated `build.bat`
  works around this by installing Hamlib to `C:\wkjtx-build\hamlib`
  regardless of where the project root is. If building manually,
  use a no-space `--prefix` for the Hamlib `configure` step.
- **`You need to install OmniRig on this computer`** at the cmake
  step: the Windows transceiver control driver OmniRig is not
  registered as a COM server on your system. See section **2a**
  above for the install steps. The automated `build.bat` checks for
  this before running cmake and warns you early if missing.

## Smoke test

After first launch:

- Window title contains "WKjTX".
- Help → About shows "About WKjTX" with WSJT-X + JTDX + WKjTX
  credits.
- `%LOCALAPPDATA%\WKjTX\` directory is created (Windows Explorer,
  paste path in address bar).
- All Impostazioni tabs load without error.
