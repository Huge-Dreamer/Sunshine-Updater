#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <future>
#include <sstream>
#include <filesystem>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <shellapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using json = nlohmann::json;

// Window dimensions and colors
const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 400;
const COLORREF BG_COLOR = RGB(32, 32, 32);  // Dark background
const COLORREF TEXT_COLOR = RGB(240, 240, 240);  // Light text

// Control IDs
#define ID_STABLE_BUTTON 1001
#define ID_PRERELEASE_BUTTON 1002
#define ID_CHECK_BUTTON 1003
#define ID_STATUS_TEXT 1004
#define ID_PROGRESS_BAR 1005

// Global variables
HWND hMainWindow;
HWND hStatusText;
HWND hProgressBar;
bool isDownloading = false;

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitializeControls(HWND hwnd);
void UpdateStatus(const std::wstring& text);
void SetProgress(int percentage);
void DownloadAndInstall(bool isPrerelease);
std::wstring GetInstalledVersion();
std::wstring GetLatestVersion(bool isPrerelease);

// Callback for CURL to write response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Progress callback for downloads
int ProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    if (dltotal > 0) {
        int percentage = static_cast<int>((dlnow / dltotal) * 100);
        PostMessage(hMainWindow, WM_APP + 1, percentage, 0);
    }
    return 0;
}

// Custom window procedure for static controls to handle dark theme
LRESULT CALLBACK StaticProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static WNDPROC oldProc = NULL;
    if (!oldProc) {
        oldProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            SetBkColor(hdc, BG_COLOR);
            SetTextColor(hdc, TEXT_COLOR);

            // Get window text
            wchar_t text[1024];
            GetWindowTextW(hwnd, text, 1024);

            // Get client rect
            RECT rect;
            GetClientRect(hwnd, &rect);

            // Draw text
            SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
            DrawTextW(hdc, text, -1, &rect, DT_LEFT | DT_WORDBREAK);

            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return CallWindowProc(oldProc, hwnd, msg, wParam, lParam);
}

// Initialize window controls
void InitializeControls(HWND hwnd) {
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    // Create buttons with dark theme
    HWND hStableButton = CreateWindowW(L"BUTTON", L"Install Stable Version",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                20, 20, 200, 30,
                hwnd, (HMENU)ID_STABLE_BUTTON, NULL, NULL);
    SendMessage(hStableButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hPrereleaseButton = CreateWindowW(L"BUTTON", L"Install Pre-Release Version",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                20, 60, 200, 30,
                hwnd, (HMENU)ID_PRERELEASE_BUTTON, NULL, NULL);
    SendMessage(hPrereleaseButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hCheckButton = CreateWindowW(L"BUTTON", L"Check Versions",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                20, 100, 200, 30,
                hwnd, (HMENU)ID_CHECK_BUTTON, NULL, NULL);
    SendMessage(hCheckButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Create status text with dark theme
    hStatusText = CreateWindowW(L"STATIC", L"",
                             WS_VISIBLE | WS_CHILD | SS_LEFT,
                             20, 150, 560, 200,
                             hwnd, (HMENU)ID_STATUS_TEXT, NULL, NULL);
    SendMessage(hStatusText, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Subclass the static control
    WNDPROC oldProc = (WNDPROC)SetWindowLongPtr(hStatusText, GWLP_WNDPROC, (LONG_PTR)StaticProc);
    SetWindowLongPtr(hStatusText, GWLP_USERDATA, (LONG_PTR)oldProc);

    // Create progress bar
    hProgressBar = CreateWindowW(PROGRESS_CLASSW, NULL,
                              WS_VISIBLE | WS_CHILD,
                              20, 360, 560, 20,
                              hwnd, (HMENU)ID_PROGRESS_BAR, NULL, NULL);
    SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    // Set dark theme for progress bar
    SendMessage(hProgressBar, PBM_SETBARCOLOR, 0, RGB(0, 120, 215));
    SendMessage(hProgressBar, PBM_SETBKCOLOR, 0, RGB(64, 64, 64));
}

// Update status text
void UpdateStatus(const std::wstring& text) {
    SetWindowTextW(hStatusText, text.c_str());
}

// Set progress bar value
void SetProgress(int percentage) {
    SendMessage(hProgressBar, PBM_SETPOS, percentage, 0);
}

// Get installed version
std::wstring GetInstalledVersion() {
    HKEY hKey;
    std::wstring version = L"Not installed";
    
    // Try to open the registry key for Sunshine
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Sunshine", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR displayVersion[256] = {0};
        DWORD dataSize = sizeof(displayVersion);
        
        // Try to read the DisplayVersion value
        if (RegQueryValueExW(hKey, L"DisplayVersion", NULL, NULL, (LPBYTE)displayVersion, &dataSize) == ERROR_SUCCESS) {
            version = std::wstring(displayVersion);
            
            // Check if it's a pre-release version
            try {
                CURL* curl = curl_easy_init();
                if (curl) {
                    std::string response;
                    curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/LizardByte/Sunshine/releases");
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
                    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Sunshine-Updater");

                    CURLcode res = curl_easy_perform(curl);
                    curl_easy_cleanup(curl);

                    if (res == CURLE_OK) {
                        json releases = json::parse(response);
                        std::string versionStr(version.begin(), version.end());

                        for (const auto& release : releases) {
                            std::string releaseVersion = release["tag_name"].get<std::string>();
                            
                            if (versionStr == releaseVersion) {
                                bool isPrerelease = release["prerelease"].get<bool>();
                                std::wstring prefix = isPrerelease ? L"(Pre-Release) " : L"(Stable) ";
                                version = prefix + version;
                                break;
                            }
                        }
                    }
                }
            } catch (...) {
                // If we can't determine if it's a pre-release, just return the version
            }
        }
        
        RegCloseKey(hKey);
    }
    
    return version;
}

// Get latest version
std::wstring GetLatestVersion(bool isPrerelease) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/LizardByte/Sunshine/releases");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Sunshine-Updater");

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            try {
                json releases = json::parse(response);
                std::string version;
                bool found = false;

                for (const auto& release : releases) {
                    bool isPrereleaseRelease = release["prerelease"].get<bool>();
                    if (isPrerelease == isPrereleaseRelease) {
                        version = release["tag_name"].get<std::string>();
                        found = true;
                        break;
                    }
                }

                if (found) {
                    std::wstring prefix = isPrerelease ? L"(Pre-Release) " : L"(Stable) ";
                    return prefix + std::wstring(version.begin(), version.end());
                }
                return isPrerelease ? L"No pre-release found" : L"No stable release found";
            } catch (...) {
                return L"Error parsing version";
            }
        }
    }
    return L"Error fetching version";
}

// Download and install
void DownloadAndInstall(bool isPrerelease) {
    UpdateStatus(L"Fetching latest version information...");
    std::wstring fullVersion = GetLatestVersion(isPrerelease);
    
    if (fullVersion.find(L"Error") != std::wstring::npos || fullVersion.find(L"No") != std::wstring::npos) {
        UpdateStatus(L"Failed to get version information");
        isDownloading = false;
        return;
    }

    // Extract version number from the full version string
    size_t start = fullVersion.find(L") ") + 2;
    std::wstring version = fullVersion.substr(start);
    std::string versionStr(version.begin(), version.end());
    
    // Create temp directory for download
    std::wstring tempPath = L"C:\\Windows\\Temp\\SunshineSetup.exe";
    
    // Construct download URL
    std::string downloadUrl = "https://github.com/LizardByte/Sunshine/releases/download/" + versionStr + "/sunshine-windows-installer.exe";
    
    UpdateStatus(L"Downloading version " + version + L"...");
    
    CURL* curl = curl_easy_init();
    if (curl) {
        FILE* fp;
        errno_t err = _wfopen_s(&fp, tempPath.c_str(), L"wb");
        if (err != 0 || !fp) {
            UpdateStatus(L"Failed to create temporary file");
            curl_easy_cleanup(curl);
            isDownloading = false;
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, downloadUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, ProgressCallback);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Sunshine-Updater");

        CURLcode res = curl_easy_perform(curl);
        fclose(fp);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            UpdateStatus(L"Download failed");
            isDownloading = false;
            return;
        }

        UpdateStatus(L"Download complete. Installing silently...");

        // Create process info structure
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        // Prepare command line for silent installation
        std::wstring cmdLine = L"\"" + tempPath + L"\" /S";
        
        // Create the process with elevated privileges
        if (CreateProcessW(NULL, (LPWSTR)cmdLine.c_str(), NULL, NULL, FALSE, 
                         CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            // Wait for the installation to complete
            WaitForSingleObject(pi.hProcess, INFINITE);
            
            // Get the exit code
            DWORD exitCode;
            if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
                if (exitCode == 0) {
                    UpdateStatus(L"Installation completed successfully!");
                } else {
                    UpdateStatus(L"Installation completed with errors. Exit code: " + std::to_wstring(exitCode));
                }
            } else {
                UpdateStatus(L"Installation completed but couldn't get exit code");
            }

            // Clean up process handles
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        } else {
            UpdateStatus(L"Failed to start installer");
        }
    } else {
        UpdateStatus(L"Failed to initialize download");
    }

    isDownloading = false;
}

// Main window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            InitializeControls(hwnd);
            return 0;

        case WM_CTLCOLORBTN:
            SetBkColor((HDC)wParam, BG_COLOR);
            SetTextColor((HDC)wParam, TEXT_COLOR);
            return (LRESULT)CreateSolidBrush(BG_COLOR);

        case WM_CTLCOLORSTATIC:
            SetBkColor((HDC)wParam, BG_COLOR);
            SetTextColor((HDC)wParam, TEXT_COLOR);
            return (LRESULT)CreateSolidBrush(BG_COLOR);

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_STABLE_BUTTON:
                    if (!isDownloading) {
                        isDownloading = true;
                        std::thread(DownloadAndInstall, false).detach();
                    }
                    return 0;

                case ID_PRERELEASE_BUTTON:
                    if (!isDownloading) {
                        isDownloading = true;
                        std::thread(DownloadAndInstall, true).detach();
                    }
                    return 0;

                case ID_CHECK_BUTTON:
                    if (!isDownloading) {
                        std::wstring installed = GetInstalledVersion();
                        std::wstring latestStable = GetLatestVersion(false);
                        std::wstring latestPrerelease = GetLatestVersion(true);
                        UpdateStatus(L"Installed: " + installed + L"\nLatest Stable: " + latestStable + L"\nLatest Pre-Release: " + latestPrerelease);
                    }
                    return 0;
            }
            break;

        case WM_APP + 1:
            SetProgress(static_cast<int>(wParam));
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"SunshineUpdater";
    wc.hbrBackground = CreateSolidBrush(BG_COLOR);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    if (!RegisterClassExW(&wc)) {
        return 1;
    }

    // Create window
    hMainWindow = CreateWindowExW(
        0,
        L"SunshineUpdater",
        L"Sunshine Updater",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hMainWindow) {
        return 1;
    }

    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Show window
    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    curl_global_cleanup();
    return 0;
} 