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

start "" cmd /c "timeout /t 1 /nobreak >nul & if exist "%ProgramFiles%\Google\Chrome\Application\chrome.exe" ("%ProgramFiles%\Google\Chrome\Application\chrome.exe" --autoplay-policy=no-user-gesture-required --user-data-dir="%TEMP%\chrome_arxshnal" "http://localhost:8080") else if exist "%ProgramFiles(x86)%\Google\Chrome\Application\chrome.exe" ("%ProgramFiles(x86)%\Google\Chrome\Application\chrome.exe" --autoplay-policy=no-user-gesture-required --user-data-dir="%TEMP%\chrome_arxshnal" "http://localhost:8080") else (start "" "http://localhost:8080")"

if exist "hehe.exe" (
    hehe.exe 1
) else if exist "x64\Release\hehe.exe" (
    "x64\Release\hehe.exe" 1
) else (
    python server.py
)

pause
