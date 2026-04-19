@echo off
REM Package the built WKjTX into an NSIS Windows installer.
REM
REM Usage: package-installer.bat
REM Requires: a successful prior build (build.bat).

setlocal

set MSYS2_HOME=C:\msys64
set WKJTX_ROOT=%~dp0
if "%WKJTX_ROOT:~-1%"=="\" set WKJTX_ROOT=%WKJTX_ROOT:~0,-1%

if not exist "%WKJTX_ROOT%\jtdx-source\build-wkjtx\build.ninja" (
    echo.
    echo [ERROR] No CMake build directory found at:
    echo   %WKJTX_ROOT%\jtdx-source\build-wkjtx\
    echo.
    echo Run build.bat first to produce a build.
    echo.
    pause
    exit /b 1
)

set MSYSTEM=MINGW64
set CHERE_INVOKING=1

"%MSYS2_HOME%\usr\bin\bash.exe" -l -c "cd '%WKJTX_ROOT:\=/%/jtdx-source/build-wkjtx' && cpack -G NSIS"

set RC=%ERRORLEVEL%
echo.
if %RC%==0 (
    echo === INSTALLER CREATED ===
    echo Look for wkjtx-*.exe in jtdx-source\build-wkjtx\
) else (
    echo === PACKAGING FAILED, exit code %RC% ===
)
pause
exit /b %RC%
