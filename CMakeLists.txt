cmake_minimum_required(VERSION 3.15)
project(Sunshine_Updater)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(nlohmann_json REQUIRED)

# Add manifest file
set(MANIFEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/admin.manifest")

# Create resource file for manifest
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/app.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/app.rc"
    @ONLY
)

# Add executable
add_executable(sunshine_updater updater.cpp)

# Link libraries
target_link_libraries(sunshine_updater PRIVATE 
    CURL::libcurl
    ZLIB::ZLIB
)

# Include directories
target_include_directories(sunshine_updater PRIVATE
    ${CURL_INCLUDE_DIRS}
)

# Add compiler flags
if(MSVC)
    target_compile_options(sunshine_updater PRIVATE /W4)
else()
    target_compile_options(sunshine_updater PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Add Windows-specific libraries
if(WIN32)
    target_link_libraries(sunshine_updater PRIVATE
        advapi32  # For Windows service management
        version   # For version info functions
    )
endif()

# Add GUI executable
add_executable(sunshine_updater_gui WIN32
    gui.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/app.rc"
)

# Link libraries
target_link_libraries(sunshine_updater_gui PRIVATE
    CURL::libcurl
    ZLIB::ZLIB
    nlohmann_json::nlohmann_json
)

# Add Windows-specific libraries for GUI
if(WIN32)
    target_link_libraries(sunshine_updater_gui PRIVATE
        advapi32  # For Windows service management
        version   # For version info functions
        comctl32  # For common controls
    )
endif()

# Set output name and properties
set_target_properties(sunshine_updater_gui PROPERTIES
    OUTPUT_NAME "Sunshine_Updater_Gui"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

# Set Windows subsystem and manifest
if(MSVC)
    set_target_properties(sunshine_updater_gui PROPERTIES
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/MANIFEST:NO"
    )
endif()

# Find DLL paths
find_file(CURL_DLL_PATH libcurl.dll PATHS "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin")
find_file(ZLIB_DLL_PATH zlib1.dll PATHS "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin")
find_file(OPENSSL_SSL_DLL_PATH libssl-3-x64.dll PATHS "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin")
find_file(OPENSSL_CRYPTO_DLL_PATH libcrypto-3-x64.dll PATHS "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin")

# Copy DLLs post-build
add_custom_command(TARGET sunshine_updater_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CURL_DLL_PATH}
        ${ZLIB_DLL_PATH}
        ${OPENSSL_SSL_DLL_PATH}
        ${OPENSSL_CRYPTO_DLL_PATH}
        $<TARGET_FILE_DIR:sunshine_updater_gui>
)

# Installation
install(TARGETS sunshine_updater
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin) 