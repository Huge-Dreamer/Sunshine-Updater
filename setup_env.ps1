# Function to find Visual Studio installation
function Find-VisualStudio {
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsPath) {
            return $vsPath
        }
    }
    return $null
}

# Find Visual Studio installation
$vsPath = Find-VisualStudio
if (-not $vsPath) {
    Write-Host "Error: Visual Studio with C++ development tools not found" -ForegroundColor Red
    Write-Host "Please install Visual Studio Build Tools with 'Desktop development with C++' workload" -ForegroundColor Yellow
    Write-Host "Download from: https://visualstudio.microsoft.com/visual-cpp-build-tools/" -ForegroundColor Yellow
    pause
    exit 1
}

Write-Host "Found Visual Studio at: $vsPath" -ForegroundColor Green

# Set up environment variables
$vcvarsall = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
if (Test-Path $vcvarsall) {
    Write-Host "Setting up Visual Studio environment..." -ForegroundColor Yellow
    cmd /c "`"$vcvarsall`" x64 && set" | ForEach-Object {
        if ($_ -match "=") {
            $name, $value = $_ -split "=", 2
            Set-Item -Path "Env:$name" -Value $value
        }
    }
    Write-Host "Environment setup complete" -ForegroundColor Green
} else {
    Write-Host "Error: vcvarsall.bat not found" -ForegroundColor Red
    Write-Host "Please ensure Visual Studio is properly installed" -ForegroundColor Yellow
    pause
    exit 1
}

# Verify cl.exe is available
$clPath = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($clPath) {
    Write-Host "Visual C++ compiler (cl.exe) is now available" -ForegroundColor Green
} else {
    Write-Host "Error: cl.exe still not found after environment setup" -ForegroundColor Red
    Write-Host "Please try running this script in a new PowerShell window" -ForegroundColor Yellow
    pause
    exit 1
}

Write-Host "`nYou can now run build.ps1 to build the project" -ForegroundColor Green
pause 