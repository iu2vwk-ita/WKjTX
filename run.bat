@echo off
REM Launch WKjTX — sets minimal PATH to avoid DLL conflicts
setlocal

set "MINGW_BIN=C:\msys64\mingw64\bin"

if not exist "%MINGW_BIN%\Qt5Core.dll" (
    echo [ERROR] MSYS2 MINGW64 not found at %MINGW_BIN%
    pause
    exit /b 1
)

set "WKJTX_EXE=%~dp0jtdx-source\build-wkjtx\wkjtx.exe"

if not exist "%WKJTX_EXE%" (
    echo [ERROR] wkjtx.exe not found at: %WKJTX_EXE%
    echo Run build.bat first.
    pause
    exit /b 1
)

REM Use ONLY the MINGW64 bin path, not system PATH, to prevent DLL conflicts
set "PATH=%MINGW_BIN%;%SystemRoot%\System32;%SystemRoot%"

echo Starting WKjTX...
start "" "%~dp0jtdx-source\build-wkjtx\wkjtx.exe"

REM Wait a moment and check if it's running
timeout /t 2 /nobreak > nul
tasklist /FI "IMAGENAME eq wkjtx.exe" 2>nul | find /i "wkjtx" > nul
if errorlevel 1 (
    echo.
    echo [ERROR] WKjTX failed to start.
    echo Try launching manually from PowerShell:
    echo   $env:PATH = "C:\msys64\mingw64\bin;C:\msys64\mingw64\lib;$env:PATH"
    echo   ^& "%~dp0jtdx-source\build-wkjtx\wkjtx.exe"
    pause
)
exit /b 0
