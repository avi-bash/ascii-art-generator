@echo off
setlocal
cd /d "%~dp0"

echo ASCII Art Generator setup
echo =========================
echo.

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup.ps1"
set "EXIT_CODE=%ERRORLEVEL%"

echo.
if not "%EXIT_CODE%"=="0" (
    echo Setup failed. Read the message above for the missing requirement.
) else (
    echo Setup finished. The executable is in build\Debug\ascii-translation.exe
)
pause
exit /b %EXIT_CODE%