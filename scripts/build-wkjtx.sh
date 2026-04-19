#!/usr/bin/env bash
# ======================================================================
# WKjTX build script (runs inside MSYS2 MINGW64)
#
# Invoked by build.bat. Can also be run directly in an MSYS2 MINGW64
# shell from the repo root:  ./scripts/build-wkjtx.sh [mode]
#
# Modes:
#   full    (default)  dep check + hamlib + wkjtx + tests
#   quick              skip dep check and hamlib (faster iteration)
#   clean              wipe build-* dirs and do a fresh full build
#   tests              only run unit tests (assumes prior successful build)
# ======================================================================
set -euo pipefail

MODE="${1:-full}"
cd "$(dirname "$0")/.."
ROOT="$(pwd)"

# IMPORTANT: autotools/libtool inside jtdxhamlib does not properly escape
# paths that contain spaces, and the project root "Claude Local" has a
# space in it. Installing Hamlib under the project root reliably breaks
# with "/usr/bin/install: target '...' is not a directory" because the
# space splits the argument.
#
# Workaround: install Hamlib into a fixed no-space path. WKjTX's CMake
# finds it via CMAKE_PREFIX_PATH; the Hamlib location is an internal
# build detail — not visible to the end user or the WKjTX source tree.
#
# CMake itself handles spaces fine, so the WKjTX binary and installer
# can stay under the spaced project root.
HAMLIB_NOSPACE_ROOT="/c/wkjtx-build"
HAMLIB_SRC="$ROOT/third-party/jtdxhamlib"
HAMLIB_PREFIX="$HAMLIB_NOSPACE_ROOT/hamlib"
WKJTX_SRC="$ROOT/jtdx-source"
WKJTX_BUILD="$WKJTX_SRC/build-wkjtx"
WKJTX_INSTALL="$ROOT/build-artifacts/wkjtx"

log () { printf "\n[build] %s\n" "$*"; }
fail () { printf "\n[build][ERROR] %s\n" "$*" >&2; exit 1; }

# ---------------------------------------------------------------- clean
if [ "$MODE" = "clean" ]; then
  log "Wiping build directories..."
  rm -rf "$WKJTX_BUILD" "$HAMLIB_SRC/build" "$HAMLIB_PREFIX" "$WKJTX_INSTALL"
  # Also remove the old in-tree hamlib dir from pre-fix runs, if any.
  rm -rf "$ROOT/build-artifacts/hamlib"
  log "Clean done — falling through to full build."
  MODE="full"
fi

# ------------------------------------------------------------ tests-only
if [ "$MODE" = "tests" ]; then
  if [ ! -d "$WKJTX_BUILD" ]; then
    fail "No build directory at $WKJTX_BUILD. Run a full build first."
  fi
  log "Running ctest..."
  cd "$WKJTX_BUILD"
  if [ -d wkjtx/tests ]; then
    ctest --output-on-failure --test-dir wkjtx/tests
  else
    log "No tests built. Rebuild with -DWKJTX_BUILD_TESTS=ON first."
    exit 1
  fi
  exit 0
fi

# ------------------------------------------------------------ dependencies
if [ "$MODE" = "full" ]; then
  log "Checking MSYS2 MINGW64 dependencies..."

  REQUIRED_PKGS=(
    mingw-w64-x86_64-gcc
    mingw-w64-x86_64-gcc-fortran
    mingw-w64-x86_64-cmake
    mingw-w64-x86_64-ninja
    mingw-w64-x86_64-pkgconf
    mingw-w64-x86_64-make
    mingw-w64-x86_64-qt5-base
    mingw-w64-x86_64-qt5-tools
    mingw-w64-x86_64-qt5-multimedia
    mingw-w64-x86_64-qt5-serialport
    mingw-w64-x86_64-qt5-translations
    mingw-w64-x86_64-qwt-qt5
    mingw-w64-x86_64-fftw
    mingw-w64-x86_64-boost
    mingw-w64-x86_64-libusb
    mingw-w64-x86_64-nsis
    git
    patch
    unzip
    autoconf
    automake
    libtool
  )

  # Only install packages that are missing — fast when everything is already installed.
  MISSING=()
  for pkg in "${REQUIRED_PKGS[@]}"; do
    if ! pacman -Q "$pkg" >/dev/null 2>&1; then
      MISSING+=("$pkg")
    fi
  done

  if [ ${#MISSING[@]} -gt 0 ]; then
    log "Installing ${#MISSING[@]} missing package(s)..."
    pacman -S --needed --noconfirm "${MISSING[@]}"
  else
    log "All dependencies present."
  fi

  # Check for OmniRig COM registration (Windows-only runtime dep).
  # JTDX's CMakeLists uses Qt's dumpcpp to generate a C++ wrapper from
  # the OmniRig COM class at build time. If OmniRig isn't registered,
  # `dumpcpp -getfile {GUID}` returns empty and cmake errors out.
  #
  # OmniRig is a 32-bit COM local server (OmniRig.exe). On 64-bit
  # Windows it registers under HKCR\WOW6432Node\CLSID\... Check both
  # the 32-bit and 64-bit registry views.
  OMNIRIG_GUID="{4FE359C5-A58F-459D-BE95-CA559FB4F270}"
  OMNIRIG_EXE="/c/Program Files (x86)/Afreet/OmniRig/OmniRig.exe"
  OMNIRIG_REGISTERED=false
  for key in \
      "HKCR\\CLSID\\$OMNIRIG_GUID" \
      "HKCR\\WOW6432Node\\CLSID\\$OMNIRIG_GUID" \
      "HKLM\\SOFTWARE\\Classes\\CLSID\\$OMNIRIG_GUID" \
      "HKLM\\SOFTWARE\\Classes\\WOW6432Node\\CLSID\\$OMNIRIG_GUID"; do
    if reg.exe query "$key" >/dev/null 2>&1; then
      OMNIRIG_REGISTERED=true
      break
    fi
  done

  if ! $OMNIRIG_REGISTERED; then
    log "[WARN] OmniRig COM class not registered on this system."
    log "       JTDX's CMakeLists will FAIL with 'You need to install OmniRig'."
    log ""
    if [ -f "$OMNIRIG_EXE" ]; then
      log "       OmniRig files are present at:"
      log "         C:\\Program Files (x86)\\Afreet\\OmniRig\\"
      log "       but the COM server was not registered (installer likely"
      log "       run without admin privileges)."
      log ""
      log "       Fix: open cmd as Administrator and run:"
      log "         \"C:\\Program Files (x86)\\Afreet\\OmniRig\\OmniRig.exe\" /regserver"
      log ""
    else
      log "       To install OmniRig:"
      log "         1. Download from http://dxatlas.com/Download.asp"
      log "            (Omni-Rig 1.20, freeware, ~1 MB)"
      log "         2. Extract the zip and run setup.exe AS ADMINISTRATOR"
      log "            (right-click setup.exe -> Run as administrator)."
      log ""
    fi
    log "       Then re-run build.bat."
    log ""
    log "       Press Ctrl+C to abort, or Enter to continue anyway (build"
    log "       will fail at the cmake step)."
    read -r -p "> " _ignored
  fi
fi

# ------------------------------------------------------------- hamlib
if [ "$MODE" = "full" ]; then
  if [ ! -f "$HAMLIB_PREFIX/lib/libhamlib.a" ]; then
    log "Building JTDX-Hamlib (static) → $HAMLIB_PREFIX"

    # Ensure the no-space install root exists.
    mkdir -p "$HAMLIB_NOSPACE_ROOT"

    # If a previous run left a build dir with the old bad --prefix cached
    # in its Makefile, wipe it before reconfiguring. Fresh configure each
    # time the install prefix is (re)established.
    if [ -f "$HAMLIB_SRC/build/Makefile" ]; then
      if ! grep -q "prefix = $HAMLIB_PREFIX" "$HAMLIB_SRC/build/Makefile" 2>/dev/null; then
        log "Build dir has stale prefix — wiping $HAMLIB_SRC/build..."
        rm -rf "$HAMLIB_SRC/build"
      fi
    fi

    cd "$HAMLIB_SRC"
    if [ ! -f configure ]; then
      ./bootstrap
    fi
    mkdir -p build && cd build
    if [ ! -f Makefile ]; then
      ../configure \
        --prefix="$HAMLIB_PREFIX" \
        --disable-shared --enable-static \
        --without-cxx-binding --disable-winradio
    fi
    make -j"$(nproc)"
    make install
  else
    log "Hamlib already built at $HAMLIB_PREFIX — skipping."
  fi
fi

# ------------------------------------------------------------- wkjtx
log "Configuring WKjTX..."
mkdir -p "$WKJTX_BUILD"
cd "$WKJTX_BUILD"

# Always rerun CMake; it's cheap when the cache is up-to-date.
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HAMLIB_PREFIX" \
  -DCMAKE_INSTALL_PREFIX="$WKJTX_INSTALL" \
  -DWKJTX_BUILD_TESTS=ON \
  -DCMAKE_Fortran_FLAGS="-fallow-argument-mismatch" \
  "$WKJTX_SRC"

log "Building WKjTX (this takes 5-30 minutes on first run)..."
ninja

# ------------------------------------------------------------- tests
log "Running unit tests..."
if [ -d wkjtx/tests ]; then
  ctest --output-on-failure --test-dir wkjtx/tests || log "Some tests failed — see output."
else
  log "Test dir wkjtx/tests not found. If WKJTX_BUILD_TESTS=ON was honored this is unexpected."
fi

# ------------------------------------------------------------- success
log "Build finished."
log "Binary:     $WKJTX_BUILD/wkjtx.exe"
log "Install:    ninja install   (to $WKJTX_INSTALL)"
log "Installer:  cpack -G NSIS    (produces wkjtx-*-win64.exe)"
