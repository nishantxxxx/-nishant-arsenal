@echo off
title NIshxnt Arsenal - Localhost C++ Engine
cd /d "%~dp0"

echo ========================================================================
echo       ARXSHNAL ARCADE // LOCALHOST TEST & DEBUG LAUNCHER (C++)
echo ========================================================================
echo.
echo [+] Mode: Host ONLY for this device (127.0.0.1:8080)
echo [+] Background Music: Harleys in Hawaii (True Autoplay & Refresh Active)
echo [+] Anti-DevTools Guardian: ACTIVE
echo [+] Opening browser: http://localhost:8080
echo.

start "" cmd /c "timeout /t 1 /nobreak >nul & if exist "%ProgramFiles%\Google\Chrome\Application\chrome.exe" ("%ProgramFiles%\Google\Chrome\Application\chrome.exe" --autoplay-policy=no-user-gesture-required --user-data-dir="%TEMP%\chrome_arxshnal" "http://localhost:8080") else if exist "%ProgramFiles(x86)%\Google\Chrome\Application\chrome.exe" ("%ProgramFiles(x86)%\Google\Chrome\Application\chrome.exe" --autoplay-policy=no-user-gesture-required --user-data-dir="%TEMP%\chrome_arxshnal" "http://localhost:8080") else (start "" "http://localhost:8080")"

if exist "hehe.exe" (
    hehe.exe 1
) else if exist "x64\Release\hehe.exe" (
    "x64\Release\hehe.exe" 1
) else (
    echo Error: hehe.exe not found! Building now...
    pause
)
