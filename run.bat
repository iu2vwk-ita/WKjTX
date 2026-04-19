@echo off
REM Launch the built WKjTX executable.
REM
REM Usage: run.bat

setlocal

set MSYS2_HOME=C:\msys64
set WKJTX_ROOT=%~dp0
if "%WKJTX_ROOT:~-1%"=="\" set WKJTX_ROOT=%WKJTX_ROOT:~0,-1%

set WKJTX_EXE=%WKJTX_ROOT%\jtdx-source\build-wkjtx\wkjtx.exe

if not exist "%WKJTX_EXE%" (
    echo.
    echo [ERROR] wkjtx.exe not found at:
    echo   %WKJTX_EXE%
    echo.
    echo Run build.bat first.
    echo.
    pause
    exit /b 1
)

REM Launch with MSYS2 MINGW64 runtime DLLs on PATH so Qt and gfortran
REM dynamic libraries load correctly when running from cmd.
set PATH=%MSYS2_HOME%\mingw64\bin;%PATH%

echo Launching %WKJTX_EXE%...
start "" "%WKJTX_EXE%"
exit /b 0
