@echo off
echo Checking Visual Studio installation...

REM Check for Visual Studio installation directory
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC" (
    echo Found Visual Studio 2022 Community
    set VS_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC" (
    echo Found Visual Studio 2022 Professional
    set VS_PATH="C:\Program Files\Microsoft Visual Studio\2022\Professional"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC" (
    echo Found Visual Studio 2022 Enterprise
    set VS_PATH="C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
) else (
    echo Visual Studio 2022 not found in standard locations
    echo Please ensure Visual Studio is installed with C++ development tools
    pause
    exit /b 1
)

REM Check for vcvarsall.bat
if exist %VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat (
    echo Found vcvarsall.bat
) else (
    echo Error: vcvarsall.bat not found
    echo Please ensure C++ development tools are installed
    pause
    exit /b 1
)

echo.
echo Visual Studio installation appears to be correct
echo You can now proceed with building the project
pause 