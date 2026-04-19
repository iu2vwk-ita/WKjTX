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

HAMLIB_SRC="$ROOT/third-party/jtdxhamlib"
HAMLIB_PREFIX="$ROOT/build-artifacts/hamlib"
WKJTX_SRC="$ROOT/jtdx-source"
WKJTX_BUILD="$WKJTX_SRC/build-wkjtx"
WKJTX_INSTALL="$ROOT/build-artifacts/wkjtx"

log () { printf "\n[build] %s\n" "$*"; }
fail () { printf "\n[build][ERROR] %s\n" "$*" >&2; exit 1; }

# ---------------------------------------------------------------- clean
if [ "$MODE" = "clean" ]; then
  log "Wiping build directories..."
  rm -rf "$WKJTX_BUILD" "$HAMLIB_SRC/build" "$HAMLIB_PREFIX" "$WKJTX_INSTALL"
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
fi

# ------------------------------------------------------------- hamlib
if [ "$MODE" = "full" ]; then
  if [ ! -f "$HAMLIB_PREFIX/lib/libhamlib.a" ]; then
    log "Building JTDX-Hamlib (static)..."
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
