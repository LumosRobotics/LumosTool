# Minimum CMake version required
cmake_minimum_required(VERSION 3.14)

# Project name and languages
project(LumosWorkspace C CXX)

# Set the C++ standard (adjust according to your needs)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set minimum macOS version
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
endif()

# Debug options
option(ENABLE_DEBUG_PORT "Enable debug TCP port for GUI testing" ON)

# Release build option (set to ON when building official releases)
option(LUMOS_OFFICIAL_RELEASE "Build as official release" OFF)

# Optionally enable debugging support
set(CMAKE_BUILD_TYPE Debug)

# Optionally define a version
set(PROJECT_VERSION "1.0.0")

# Add compile definition for release builds
if(LUMOS_OFFICIAL_RELEASE)
    add_compile_definitions(LUMOS_OFFICIAL_RELEASE=1)
    message(STATUS "Building OFFICIAL RELEASE")
else()
    add_compile_definitions(LUMOS_OFFICIAL_RELEASE=0)
    message(STATUS "Building DEVELOPMENT version")
endif()

# Nlohmann
include_directories(${CMAKE_SOURCE_DIR}/third_party/nlohmann/include)

include_directories(${CMAKE_SOURCE_DIR}/src/)

add_compile_options(-Wall -Wextra -pedantic)

# Enable testing
enable_testing()

# Add subdirectories

# Build shared modules
add_subdirectory(src/modules/serial)

# Build simple lumos CLI tool (for STM32 builds)
add_subdirectory(src/applications/lumos_simple)
add_subdirectory(src/applications/simple_serial)

# Build Qt GUI (optional – skipped gracefully if Qt6 is not installed)
find_package(Qt6 QUIET COMPONENTS Core Gui Widgets SerialPort)
if(Qt6_FOUND)
    message(STATUS "Qt6 found – building serial_gui")
    add_subdirectory(src/applications/serial_gui)
else()
    message(STATUS "Qt6 not found – skipping serial_gui")
endif()

# ── Install rules (used by cmake --install, e.g. for future build-from-source distribution) ──
install(TARGETS lumos_target simple_serial RUNTIME DESTINATION bin)

if(Qt6_FOUND)
    install(TARGETS serial_gui
        BUNDLE  DESTINATION .    # macOS .app bundle
        RUNTIME DESTINATION bin  # Linux/Windows binary
    )
endif()
