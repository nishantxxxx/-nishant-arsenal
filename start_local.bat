@echo off
title NIshxnt Arsenal - Local Server Launcher
cd /d "%~dp0"

echo ========================================================
echo        ARXSHNAL ARCADE // LOCALHOST TEST RUNNER
echo ========================================================
echo.
echo [+] Launching C++ High-Performance Server on Port 8080...
echo [+] Background Music Engine: 50%% Default Volume (Harleys in Hawaii)
echo [+] DevTools Security Shield: ACTIVE
echo [+] Opening browser: http://localhost:8080
echo.

start "" "http://localhost:8080"

if exist "hehe.exe" (
    hehe.exe 1
) else (
    python server.py
)

pause
