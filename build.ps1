# Build script for Sunshine Updater
Write-Host "Building Sunshine Updater..."

# Set paths
$CMAKE_PATH = "C:\Program Files\CMake\bin\cmake.exe"
$VCPKG_TOOLCHAIN = "$PSScriptRoot\vcpkg\scripts\buildsystems\vcpkg.cmake"

# Check for Visual Studio Build Tools
if (-not (Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools")) {
    Write-Host "Error: Visual Studio Build Tools not found. Please install Visual Studio 2022 Build Tools."
    exit 1
}

# Check for vcpkg toolchain
if (-not (Test-Path $VCPKG_TOOLCHAIN)) {
    Write-Host "Error: vcpkg not found. Please run 'git clone https://github.com/Microsoft/vcpkg.git' first."
    exit 1
}

# Clean build directories
Write-Host "Cleaning build directories..."
if (Test-Path "build") {
    Remove-Item -Recurse -Force "build"
}

# Create new build folder with timestamp
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$BUILD_DIR = "build_$timestamp"
New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null
Write-Host "Creating build folder: $BUILD_DIR"

# Configure project
Write-Host "Configuring project..."
& $CMAKE_PATH -B $BUILD_DIR -S . "-DCMAKE_TOOLCHAIN_FILE=$VCPKG_TOOLCHAIN" -DCMAKE_BUILD_TYPE=Release

# Build project
Write-Host "Building project..."
& $CMAKE_PATH --build $BUILD_DIR --config Release

# Create distribution folder
$DIST_DIR = "Sunshine_Updater_$timestamp"
New-Item -ItemType Directory -Path $DIST_DIR | Out-Null

# Copy executable
Write-Host "Copying executable..."
$exePath = "$BUILD_DIR\bin\Sunshine_Updater_Gui.exe"
if (Test-Path $exePath) {
    Copy-Item $exePath -Destination $DIST_DIR
    Write-Host "Copied executable successfully"
} else {
    Write-Host "Warning: Could not find executable at $exePath"
}

# Copy required DLLs
Write-Host "Copying required DLLs..."
$dlls = @(
    "libcurl.dll",
    "libssl-3-x64.dll",
    "libcrypto-3-x64.dll",
    "zlib1.dll"
)

foreach ($dll in $dlls) {
    $dllPath = "vcpkg\installed\x64-windows\bin\$dll"
    if (Test-Path $dllPath) {
        Copy-Item $dllPath -Destination $DIST_DIR
        Write-Host "Copied $dll"
    } else {
        Write-Host "Warning: $dll not found"
    }
}

Write-Host "`nBuild complete! The distribution package is located at: $DIST_DIR"
Write-Host "Please test the application from the $DIST_DIR folder."
Write-Host "`nPress Enter to continue..."
Read-Host 