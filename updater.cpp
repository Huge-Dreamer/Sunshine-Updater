#include <iostream>
#include <string>
#include <filesystem>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <shlwapi.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <sstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

// Forward declarations
bool InstallUpdate(const std::string& tempPath);
bool StopSunshineService();
bool StartSunshineService();

// Embedded PowerShell script
const char* PS_SCRIPT = R"ps(
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
        Start-Process powershell -Verb RunAs -ArgumentList "-NoProfile -ExecutionPolicy Bypass -Command `"$PSCommandPath`""
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
    Write-Host "1. Download and install latest Stable build"
    Write-Host "2. Download and install latest Pre-Release build"
    Write-Host "3. Check latest releases"
    Write-Host "4. Exit"
    Write-Host "`nEnter your choice (1-4): " -NoNewline
}

# Function to check if Sunshine is installed
function Get-InstalledVersion {
    $installPath = "C:\Program Files\Sunshine\sunshine.exe"
    if (Test-Path $installPath) {
        $version = (Get-Item $installPath).VersionInfo.FileVersion
        try {
            $response = Invoke-RestMethod -Uri "https://api.github.com/repos/LizardByte/Sunshine/releases"
            
            # Check if version matches any release
            foreach ($release in $response) {
                $releaseVersion = $release.tag_name -replace '^v', ''
                if ($version -eq $releaseVersion) {
                    if ($release.prerelease) {
                        return "(Pre-Release) $version"
                    } else {
                        return "(Stable) $version"
                    }
                }
            }
            
            # If version not found in releases, return just the version number
            return $version
        } catch {
            return $version
        }
    }
    return "Not installed"
}

# Function to get latest releases
function Get-LatestReleases {
    Write-Host "`nFetching latest releases..." -ForegroundColor Yellow
    
    try {
        $response = Invoke-RestMethod -Uri "https://api.github.com/repos/LizardByte/Sunshine/releases"
        $installedVersion = Get-InstalledVersion
        
        Write-Host "`nCurrently installed version: $installedVersion" -ForegroundColor Cyan
        Write-Host "`nLatest releases:" -ForegroundColor Green
        
        $stableRelease = $response | Where-Object { -not $_.prerelease } | Select-Object -First 1
        $prerelease = $response | Where-Object { $_.prerelease } | Select-Object -First 1
        
        if ($stableRelease) {
            Write-Host "`nStable:" -ForegroundColor Green
            Write-Host "Version: $($stableRelease.tag_name)"
            Write-Host "Published: $($stableRelease.published_at)"
        }
        
        if ($prerelease) {
            Write-Host "`nPre-Release:" -ForegroundColor Yellow
            Write-Host "Version: $($prerelease.tag_name)"
            Write-Host "Published: $($prerelease.published_at)"
        }
    } catch {
        Write-Host "Error fetching releases: $_" -ForegroundColor Red
    }
    
    Write-Host "`nPress Enter to continue..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
}

# Function to download and install a release
function Install-Release {
    param (
        [bool]$IncludePrerelease
    )
    
    Write-Host "`nFetching latest release information..." -ForegroundColor Yellow
    
    try {
        $response = Invoke-RestMethod -Uri "https://api.github.com/repos/LizardByte/Sunshine/releases"
        $release = $response | Where-Object { $_.prerelease -eq $IncludePrerelease } | Select-Object -First 1
        
        if ($release) {
            $downloadUrl = $release.assets | Where-Object { $_.name -like "*windows*.exe" } | Select-Object -First 1 -ExpandProperty browser_download_url
            
            if ($downloadUrl) {
                $installedVersion = Get-InstalledVersion
                if ($installedVersion -ne "Not installed") {
                    $response = Read-Host "Sunshine is already installed. Do you want to reinstall it? (y/n)"
                    if ($response -ne 'y' -and $response -ne 'Y') {
                        return
                    }
                }
                
                Write-Host "`nStarting download and installation..." -ForegroundColor Yellow
                $tempFile = Join-Path $env:TEMP "sunshine_update.exe"
                
                # Download the file
                Invoke-WebRequest -Uri $downloadUrl -OutFile $tempFile
                
                # Start the installer silently
                Start-Process -FilePath $tempFile -ArgumentList "/S" -Wait
                
                # Clean up
                Remove-Item $tempFile -Force
                
                Write-Host "Installation completed!" -ForegroundColor Green
            } else {
                Write-Host "Could not find a suitable download URL!" -ForegroundColor Red
            }
        } else {
            Write-Host "No suitable release found!" -ForegroundColor Red
        }
    } catch {
        Write-Host "Error during installation: $_" -ForegroundColor Red
    }
    
    Write-Host "`nPress Enter to continue..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
}

# Function to get latest version
function Get-LatestVersion {
    try {
        $response = Invoke-RestMethod -Uri "https://api.github.com/repos/LizardByte/Sunshine/releases"
        $latestStable = $response | Where-Object { -not $_.prerelease } | Select-Object -First 1
        $latestPrerelease = $response | Where-Object { $_.prerelease } | Select-Object -First 1
        
        if ($latestStable) {
            return "Stable: $($latestStable.tag_name)"
        } elseif ($latestPrerelease) {
            return "Pre-Release: $($latestPrerelease.tag_name)"
        } else {
            return "No releases found"
        }
    } catch {
        return "Error fetching version: $_"
    }
}

# Main loop
try {
    while ($true) {
        Show-Menu
        $choice = Read-Host
        
        switch ($choice) {
            "1" { Install-Release -IncludePrerelease $false }
            "2" { Install-Release -IncludePrerelease $true }
            "3" { Get-LatestReleases }
            "4" { 
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
)ps";

// Version information
const std::string CURRENT_VERSION = "0.0.0"; // This should be updated with actual version
const std::string GITHUB_API_URL = "https://api.github.com/repos/LizardByte/Sunshine/releases";
const std::string APP_NAME = "Sunshine";
const std::string INSTALL_DIR = "C:\\Program Files\\Sunshine"; // Default installation directory

// Progress tracking structure
struct ProgressData {
    double progress;
    std::string status;
    bool isComplete;
};

// Callback function for CURL to write response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Progress callback for downloads
int ProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    ProgressData* progress = static_cast<ProgressData*>(clientp);
    if (dltotal > 0) {
        progress->progress = (dlnow / dltotal) * 100;
        std::cout << progress->progress << std::endl; // Output progress for PowerShell to capture
    }
    return 0;
}

// Get the download URL for the current platform
std::string GetDownloadUrl(const json& release, bool includePrerelease) {
    std::string platform = "windows";
    std::string extension = ".exe";
    
    for (const auto& asset : release["assets"]) {
        std::string name = asset["name"].get<std::string>();
        if (name.find(platform) != std::string::npos && 
            name.find(extension) != std::string::npos) {
            return asset["browser_download_url"].get<std::string>();
        }
    }
    return "";
}

// Check for updates
std::string CheckForUpdates(bool includePrerelease) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    std::string latestVersion;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, GITHUB_API_URL.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Sunshine-Updater/1.0");

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            try {
                json releases = json::parse(readBuffer);
                for (const auto& release : releases) {
                    bool isPrerelease = release["prerelease"].get<bool>();
                    if (!includePrerelease && isPrerelease) {
                        continue;
                    }
                    std::string version = release["tag_name"].get<std::string>();
                    if (version > CURRENT_VERSION) {
                        latestVersion = version;
                        break;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing JSON: " << e.what() << std::endl;
            }
        }
        curl_easy_cleanup(curl);
    }
    return latestVersion;
}

// Download and install update
bool DownloadUpdate(const std::string& version, const std::string& downloadUrl) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    // Create temporary directory for download
    fs::path tempDir = fs::temp_directory_path() / "sunshine_update";
    fs::create_directories(tempDir);
    fs::path tempFile = tempDir / "sunshine_update.exe";

    FILE* fp = fopen(tempFile.string().c_str(), "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return false;
    }

    ProgressData progress = {0, "Downloading...", false};
    curl_easy_setopt(curl, CURLOPT_URL, downloadUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, ProgressCallback);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &progress);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return false;
    }

    // Install the update
    return InstallUpdate(tempFile.string());
}

// Stop Sunshine service
bool StopSunshineService() {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm) {
        return false;
    }

    SC_HANDLE service = OpenService(scm, "Sunshine", SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!service) {
        CloseServiceHandle(scm);
        return false;
    }

    SERVICE_STATUS_PROCESS ssp;
    DWORD bytesNeeded;
    if (QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &bytesNeeded)) {
        if (ssp.dwCurrentState != SERVICE_STOPPED) {
            SERVICE_STATUS status;
            ControlService(service, SERVICE_CONTROL_STOP, &status);
            // Wait for service to stop
            Sleep(1000);
        }
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return true;
}

// Start Sunshine service
bool StartSunshineService() {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm) {
        return false;
    }

    SC_HANDLE service = OpenService(scm, "Sunshine", SERVICE_START);
    if (!service) {
        CloseServiceHandle(scm);
        return false;
    }

    bool result = StartService(service, 0, NULL) != 0;
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return result;
}

// Install the update
bool InstallUpdate(const std::string& tempPath) {
    try {
        // Stop Sunshine service
        if (!StopSunshineService()) {
            std::cerr << "Failed to stop Sunshine service" << std::endl;
            return false;
        }

        // Create backup of current installation
        fs::path backupDir = fs::temp_directory_path() / "sunshine_backup";
        if (fs::exists(INSTALL_DIR)) {
            fs::create_directories(backupDir);
            for (const auto& entry : fs::directory_iterator(INSTALL_DIR)) {
                fs::copy(entry.path(), backupDir / entry.path().filename(),
                        fs::copy_options::overwrite_existing);
            }
        }

        // Create installation directory if it doesn't exist
        fs::create_directories(INSTALL_DIR);

        // Copy new executable
        fs::copy_file(tempPath, fs::path(INSTALL_DIR) / "sunshine.exe",
                     fs::copy_options::overwrite_existing);

        // Copy configuration files from backup if they exist
        if (fs::exists(backupDir)) {
            for (const auto& entry : fs::directory_iterator(backupDir)) {
                if (entry.path().extension() == ".conf") {
                    fs::copy_file(entry.path(), fs::path(INSTALL_DIR) / entry.path().filename(),
                                fs::copy_options::overwrite_existing);
                }
            }
        }

        // Start Sunshine service
        if (!StartSunshineService()) {
            std::cerr << "Failed to start Sunshine service" << std::endl;
            return false;
        }

        // Clean up
        fs::remove_all(fs::path(tempPath).parent_path());
        fs::remove_all(backupDir);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error during installation: " << e.what() << std::endl;
        return false;
    }
}

// Command line arguments structure
struct CommandLineArgs {
    bool check = false;
    bool update = false;
    bool prerelease = false;
    std::string version;
};

// Parse command line arguments
CommandLineArgs ParseArgs(int argc, char* argv[]) {
    CommandLineArgs args;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--check") {
            args.check = true;
        } else if (arg == "--update") {
            args.update = true;
        } else if (arg.find("--prerelease:") == 0) {
            args.prerelease = arg.substr(12) == "true";
        } else if (arg == "--version" && i + 1 < argc) {
            args.version = argv[++i];
        }
    }
    return args;
}

// Launch PowerShell script
bool LaunchPowerShell() {
    // Create temporary script file
    fs::path tempDir = fs::temp_directory_path();
    fs::path tempScriptPath = tempDir / "sunshine_updater.ps1";

    // Write PowerShell script to temporary file
    std::ofstream scriptFile(tempScriptPath);
    if (!scriptFile) {
        return false;
    }
    scriptFile << PS_SCRIPT;
    scriptFile.close();

    // Build PowerShell command
    std::string command = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" + 
                         tempScriptPath.string() + "\"";

    // Launch PowerShell
    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    if (CreateProcess(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        fs::remove(tempScriptPath);
        return true;
    }

    fs::remove(tempScriptPath);
    return false;
}

std::string GetInstalledVersionFromFile() {
    std::string installPath = "C:\\Program Files\\Sunshine\\sunshine.exe";
    if (!fs::exists(installPath)) {
        return "Not installed";
    }

    DWORD verHandle = 0;
    DWORD verSize = GetFileVersionInfoSize(installPath.c_str(), &verHandle);
    if (verSize == 0) {
        return "Not installed";
    }

    std::vector<BYTE> verData(verSize);
    if (!GetFileVersionInfo(installPath.c_str(), verHandle, verSize, verData.data())) {
        return "Not installed";
    }

    UINT size = 0;
    LPBYTE lpBuffer = NULL;
    if (!VerQueryValue(verData.data(), "\\", (VOID FAR* FAR*)&lpBuffer, &size)) {
        return "Not installed";
    }

    if (size == 0) {
        return "Not installed";
    }

    VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
    if (verInfo->dwSignature != 0xfeef04bd) {
        return "Not installed";
    }

    std::stringstream version;
    version << ((verInfo->dwFileVersionMS >> 16) & 0xffff) << "."
            << ((verInfo->dwFileVersionMS >> 0) & 0xffff) << "."
            << ((verInfo->dwFileVersionLS >> 16) & 0xffff);

    return version.str();
}

std::string GetInstalledVersion() {
    std::string version = GetInstalledVersionFromFile();
    if (version == "Not installed") {
        return version;
    }

    try {
        // Get GitHub releases
        CURL* curl = curl_easy_init();
        if (!curl) {
            return version;
        }

        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/LizardByte/Sunshine/releases");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Sunshine-Updater");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return version;
        }

        // Parse JSON response
        nlohmann::json releases = nlohmann::json::parse(response);
        
        // Check if version matches any release
        for (const auto& release : releases) {
            std::string releaseVersion = release["tag_name"].get<std::string>();
            releaseVersion = releaseVersion.substr(1); // Remove 'v' prefix
            
            if (version == releaseVersion) {
                bool isPrerelease = release["prerelease"].get<bool>();
                return isPrerelease ? "(Pre-Release) " + version : "(Stable) " + version;
            }
        }

        return version;
    } catch (...) {
        return version;
    }
}

int main(int argc, char* argv[]) {
    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Parse command line arguments
    CommandLineArgs args = ParseArgs(argc, argv);

    if (args.check) {
        // Check for updates
        std::string latestVersion = CheckForUpdates(args.prerelease);
        if (!latestVersion.empty()) {
            std::cout << latestVersion << std::endl;
        }
    } else if (args.update && !args.version.empty()) {
        // Get download URL for the specified version
        CURL* curl = curl_easy_init();
        std::string readBuffer;
        bool success = false;

        if (curl) {
            std::string url = GITHUB_API_URL + "/tags/" + args.version;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Sunshine-Updater/1.0");

            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK) {
                try {
                    json release = json::parse(readBuffer);
                    std::string downloadUrl = GetDownloadUrl(release, args.prerelease);
                    if (!downloadUrl.empty()) {
                        success = DownloadUpdate(args.version, downloadUrl);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing JSON: " << e.what() << std::endl;
                }
            }
            curl_easy_cleanup(curl);
        }

        return success ? 0 : 1;
    } else {
        // Launch PowerShell interface
        return LaunchPowerShell() ? 0 : 1;
    }

    // Clean up CURL
    curl_global_cleanup();
    return 0;
} 