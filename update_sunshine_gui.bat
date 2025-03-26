@echo off
setlocal

rem Check for admin rights
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Requesting administrator privileges...
    powershell -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

rem Launch the GUI version
start "" "Sunshine_Updater_Gui.exe"
exit /b 0 