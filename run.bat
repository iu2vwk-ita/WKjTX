@echo off
REM Launch WKjTX — run from within the build directory so Qt DLLs
REM (copied alongside the exe) are found via Windows DLL search order.
cd /d "%~dp0jtdx-source\build-wkjtx"
start "" "wkjtx.exe"
exit /b 0
