# Error handling
$ErrorActionPreference = "Stop"
trap {
    Write-Host "`nAn error occurred: $_" -ForegroundColor Red
    Write-Host "Press Enter to exit..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    exit 1
}

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "Requesting administrator privileges..." -ForegroundColor Yellow
    try {
        Start-Process powershell -Verb RunAs -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$($MyInvocation.MyCommand.Path)`""
        exit
    } catch {
        Write-Host "Failed to request administrator privileges. Please run as administrator manually." -ForegroundColor Red
        Write-Host "Press Enter to exit..."
        $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
        exit 1
    }
}

# Set the title and clear the screen
$Host.UI.RawUI.WindowTitle = "Sunshine Updater"
Clear-Host

# Function to show the menu
function Show-Menu {
    Write-Host "`n=== Sunshine Updater ===`n" -ForegroundColor Cyan
    Write-Host "1. Check for stable updates"
    Write-Host "2. Check for pre-release updates"
    Write-Host "3. Exit"
    Write-Host "`nEnter your choice (1-3): " -NoNewline
}

# Function to check for updates
function Check-ForUpdates {
    param (
        [bool]$IncludePrerelease
    )
    
    Write-Host "`nChecking for updates..." -ForegroundColor Yellow
    
    # Check if sunshine_updater.exe exists
    if (-not (Test-Path ".\sunshine_updater.exe")) {
        Write-Host "Error: sunshine_updater.exe not found in the current directory!" -ForegroundColor Red
        return
    }
    
    try {
        $latestVersion = & .\sunshine_updater.exe --check --prerelease:$IncludePrerelease
        
        if ($latestVersion) {
            Write-Host "`nNew version $latestVersion is available!" -ForegroundColor Green
            $response = Read-Host "Would you like to update now? (y/n)"
            
            if ($response -eq 'y' -or $response -eq 'Y') {
                Write-Host "`nStarting update process..." -ForegroundColor Yellow
                $result = & .\sunshine_updater.exe --update --version $latestVersion --prerelease:$IncludePrerelease
                if ($LASTEXITCODE -ne 0) {
                    Write-Host "Update failed!" -ForegroundColor Red
                } else {
                    Write-Host "Update completed successfully!" -ForegroundColor Green
                }
            }
        } else {
            Write-Host "`nYou are running the latest version!" -ForegroundColor Green
        }
    } catch {
        Write-Host "Error checking for updates: $_" -ForegroundColor Red
    }
    
    Write-Host "`nPress Enter to continue..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
}

# Main loop
try {
    while ($true) {
        Show-Menu
        $choice = Read-Host
        
        switch ($choice) {
            "1" { Check-ForUpdates -IncludePrerelease $false }
            "2" { Check-ForUpdates -IncludePrerelease $true }
            "3" { 
                Write-Host "`nExiting..." -ForegroundColor Yellow
                exit 0 
            }
            default { 
                Write-Host "Invalid choice. Please try again." -ForegroundColor Red
                Start-Sleep -Seconds 2
            }
        }
    }
} catch {
    Write-Host "`nAn unexpected error occurred: $_" -ForegroundColor Red
    Write-Host "Press Enter to exit..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    exit 1
} 