@echo off
setlocal enabledelayedexpansion

rem Set output directory to current directory
set OUTPUT_DIR=%~dp0

rem Check for Visual Studio Build Tools
call check_vs.bat
if errorlevel 1 exit /b 1

rem Check for vcpkg toolchain
if not exist "vcpkg\scripts\buildsystems\vcpkg.cmake" (
    echo Error: vcpkg not found. Please run 'git clone https://github.com/Microsoft/vcpkg.git' first.
    exit /b 1
)

rem Clean build directories
echo Cleaning build directories...
if exist "build" rmdir /s /q "build"

rem Create new build folder with timestamp
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
set timestamp=%datetime:~0,8%_%datetime:~8,6%
set BUILD_DIR=build_%timestamp%
mkdir %BUILD_DIR%
echo Creating build folder: %BUILD_DIR%

rem Configure project
echo Configuring project...
cmake -B %BUILD_DIR% -S . "-DCMAKE_TOOLCHAIN_FILE=%OUTPUT_DIR%vcpkg\scripts\buildsystems\vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release

rem Build project
echo Building project...
cmake --build %BUILD_DIR% --config Release

rem Create distribution folder
set DIST_DIR=Sunshine_Updater_%timestamp%
mkdir %DIST_DIR%

rem Copy executable
echo Copying executable...
if exist "%BUILD_DIR%\bin\Release\Sunshine_Updater_Gui.exe" (
    copy "%BUILD_DIR%\bin\Release\Sunshine_Updater_Gui.exe" "%DIST_DIR%"
    echo Copied executable successfully
) else (
    echo Warning: Could not find executable at %BUILD_DIR%\bin\Release\Sunshine_Updater_Gui.exe
)

rem Copy required DLLs
echo Copying required DLLs...
set DLLS=libcurl.dll libssl-3-x64.dll libcrypto-3-x64.dll zlib1.dll
for %%d in (%DLLS%) do (
    if exist "vcpkg\installed\x64-windows\bin\%%d" (
        copy "vcpkg\installed\x64-windows\bin\%%d" "%DIST_DIR%"
        echo Copied %%d
    ) else (
        echo Warning: %%d not found
    )
)

echo.
echo Build complete! The distribution package is located at: %DIST_DIR%
echo Please test the application from the %DIST_DIR% folder.
echo.
pause 