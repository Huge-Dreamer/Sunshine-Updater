# Sunshine-Updater

A Windows application for updating Sunshine streaming server with both stable and pre-release versions.

## Quick Start (Recommended)

1. Download the latest release from the [Releases](https://github.com/Huge-Dreamer/Sunshine-Updater/releases) page
2. Extract the contents of `Sunshine-Updater_v1.0.0.zip` to a folder
3. Run `update_sunshine_gui.bat` to launch the application
4. Click "Check Versions" to see your currently installed version
5. Choose between stable or pre-release updates
6. Click "Download and Install" to update Sunshine

The pre-built package includes everything you need:
- Sunshine-Updater_Gui.exe
- All required DLLs
- Launch scripts

## Features

- Graphical user interface
- Support for both stable and pre-release versions
- Automatic version detection
- Silent installation
- Progress tracking
- Administrator privileges handling

## Building from Source

If you want to build the application yourself:

1. Clone the repository:
   ```bash
   git clone https://github.com/Huge-Dreamer/Sunshine-Updater.git
   cd Sunshine-Updater
   ```

2. Clone vcpkg:
   ```bash
   git clone https://github.com/Microsoft/vcpkg.git
   ```

3. Build the project:
   - Using PowerShell: `.\build.ps1`
   - Using Command Prompt: `build.bat`

4. The built application will be in the `Sunshine_Updater_[timestamp]` folder

## Requirements for Building

- Windows 10 or later
- Visual Studio 2022 Build Tools
- CMake 3.15 or later
- vcpkg

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Sunshine](https://github.com/LizardByte/Sunshine) - The streaming server this updater is designed for
- [vcpkg](https://github.com/microsoft/vcpkg) - C++ package manager
- [libcurl](https://curl.se/) - For HTTP requests
- [nlohmann/json](https://github.com/nlohmann/json) - For JSON parsing 