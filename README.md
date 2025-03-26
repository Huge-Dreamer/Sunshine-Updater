# Sunshine-Updater

A Windows application for updating Sunshine streaming server with both stable and pre-release versions.

## Features

- Graphical user interface
- Support for both stable and pre-release versions
- Automatic version detection
- Silent installation
- Progress tracking
- Administrator privileges handling

## Requirements

- Windows 10 or later
- Visual Studio 2022 Build Tools (for building from source)
- CMake 3.15 or later
- vcpkg (for building from source)

## Installation

### Pre-built Binary

1. Download the latest release from the [Releases](https://github.com/Huge-Dreamer/Sunshine-Updater/releases) page
2. Extract the contents to a folder
3. Run `update_sunshine_gui.bat` to launch the application

### Building from Source

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

## Usage

1. Launch the application using `update_sunshine_gui.bat`
2. Click "Check Versions" to see the currently installed version
3. Choose between stable or pre-release updates
4. Click "Download and Install" to update Sunshine

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Sunshine](https://github.com/LizardByte/Sunshine) - The streaming server this updater is designed for
- [vcpkg](https://github.com/microsoft/vcpkg) - C++ package manager
- [libcurl](https://curl.se/) - For HTTP requests
- [nlohmann/json](https://github.com/nlohmann/json) - For JSON parsing 