@echo off
title NIshxnt Arsenal - Localhost C++ Engine
cd /d "%~dp0"

echo ========================================================================
echo       ARXSHNAL ARCADE // LOCALHOST TEST & DEBUG LAUNCHER (C++)
echo ========================================================================
echo.
echo [+] Mode: Host ONLY for this device (127.0.0.1:8080)
echo [+] Background Music: Harleys in Hawaii (50%% Loop, Bottom-Right)
echo [+] Anti-DevTools Guardian: ACTIVE
echo [+] Opening browser: http://localhost:8080
echo.

start "" "http://localhost:8080"

if exist "hehe.exe" (
    hehe.exe 1
) else if exist "x64\Release\hehe.exe" (
    "x64\Release\hehe.exe" 1
) else (
    echo Error: hehe.exe not found! Building now...
    pause
)
