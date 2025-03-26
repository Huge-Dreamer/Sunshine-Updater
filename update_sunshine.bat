@echo off
echo Starting Sunshine Updater...
echo.

REM Check if PowerShell is installed
where powershell >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: PowerShell is not installed or not in PATH
    pause
    exit /b 1
)

REM Run the PowerShell script with error handling
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0update_sunshine.ps1"
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo An error occurred while running the updater.
    echo Please check if the script file exists and has the correct permissions.
    pause
    exit /b 1
)

pause 