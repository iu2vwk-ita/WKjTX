@echo off
REM =====================================================================
REM WKjTX build orchestrator (Windows)
REM
REM Usage: double-click build.bat or run from cmd:
REM   build.bat            full build (deps + hamlib + wkjtx + tests)
REM   build.bat quick      skip dep check and hamlib (for iterative edits)
REM   build.bat clean      wipe build artifacts and rebuild from scratch
REM   build.bat tests      only run unit tests (assumes prior build)
REM
REM Requires: MSYS2 installed at C:\msys64 (install first from
REM           https://www.msys2.org/ — this script does not install MSYS2).
REM =====================================================================

setlocal

set MSYS2_HOME=C:\msys64
set WKJTX_ROOT=%~dp0
REM Strip trailing backslash from WKJTX_ROOT if present.
if "%WKJTX_ROOT:~-1%"=="\" set WKJTX_ROOT=%WKJTX_ROOT:~0,-1%

if not exist "%MSYS2_HOME%\usr\bin\bash.exe" (
    echo.
    echo [ERROR] MSYS2 not found at %MSYS2_HOME%.
    echo.
    echo Please install MSYS2 first:
    echo   1. Download installer from https://www.msys2.org/
    echo   2. Run with default settings (install to C:\msys64^)
    echo   3. Open "MSYS2 MINGW64" from Start menu and run:
    echo        pacman -Syu --noconfirm
    echo      ^(shell closes^); reopen and run:
    echo        pacman -Su --noconfirm
    echo   4. Re-run this build.bat.
    echo.
    pause
    exit /b 1
)

set MODE=%1
if "%MODE%"=="" set MODE=full

REM Required for MSYS2 to pick the MINGW64 environment.
set MSYSTEM=MINGW64
set CHERE_INVOKING=1

echo.
echo === WKjTX build orchestrator ===
echo Mode:   %MODE%
echo Root:   %WKJTX_ROOT%
echo MSYS2:  %MSYS2_HOME%
echo.

"%MSYS2_HOME%\usr\bin\bash.exe" -l -c "cd '%WKJTX_ROOT:\=/%' && ./scripts/build-wkjtx.sh %MODE%"

set RC=%ERRORLEVEL%
echo.
if %RC%==0 (
    echo === BUILD OK ===
    echo Run wkjtx.exe with:  run.bat
) else (
    echo === BUILD FAILED, exit code %RC% ===
    echo Check the output above for the error message.
)
echo.
pause
exit /b %RC%
