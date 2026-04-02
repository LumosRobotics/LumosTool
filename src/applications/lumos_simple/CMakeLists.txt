cmake_minimum_required(VERSION 3.14)

# Simple Lumos Build Tool

set(LUMOS_SOURCES
    main.cpp
    project_config.cpp
    builder.cpp
    hal_module_detector.cpp
    cache_config.cpp
)

# Create executable with temporary name
# ── yaml-cpp ──────────────────────────────────────────────────────────────────
# Prefer a system/prefix installation (e.g. Homebrew). Falls back to the
# in-tree submodule build for the Docker / macOS dev workflow.
find_package(yaml-cpp QUIET)

# Older distro packages (e.g. Ubuntu 22.04 yaml-cpp 0.7) create a plain
# 'yaml-cpp' target without the namespace. Alias it so the rest of the build
# can always use 'yaml-cpp::yaml-cpp'.
if(yaml-cpp_FOUND AND NOT TARGET yaml-cpp::yaml-cpp AND TARGET yaml-cpp)
    add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
endif()

if(NOT TARGET yaml-cpp::yaml-cpp)
    set(_YAML_INC "${CMAKE_SOURCE_DIR}/third_party/yaml-cpp/include")
    # Use a platform-specific build directory so macOS, Linux, and Windows
    # submodule builds don't overwrite each other.
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(_YAML_LIB "${CMAKE_SOURCE_DIR}/third_party/yaml-cpp/build_windows/libyaml-cpp.a")
    else()
        set(_YAML_LIB "${CMAKE_SOURCE_DIR}/third_party/yaml-cpp/build/libyaml-cpp.a")
    endif()

    if(EXISTS "${_YAML_LIB}")
        message(STATUS "yaml-cpp: using submodule build at ${_YAML_LIB}")
        add_library(yaml-cpp::yaml-cpp STATIC IMPORTED)
        set_target_properties(yaml-cpp::yaml-cpp PROPERTIES
            IMPORTED_LOCATION "${_YAML_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${_YAML_INC}"
            # On Windows, yaml-cpp headers use __declspec(dllimport) by default.
            # This define suppresses that so the static lib links correctly.
            INTERFACE_COMPILE_DEFINITIONS "YAML_CPP_STATIC_DEFINE"
        )
    else()
        message(FATAL_ERROR
            "yaml-cpp not found. Install it (e.g. brew install yaml-cpp) or "
            "build the submodule (run docker/setup_deps.sh).")
    endif()
endif()

add_executable(lumos_target ${LUMOS_SOURCES})

# Set binary name based on build type
if(LUMOS_OFFICIAL_RELEASE)
    # Release build: name it "lumos"
    set_target_properties(lumos_target PROPERTIES OUTPUT_NAME "lumos")
    message(STATUS "Building release binary: lumos")
else()
    # Development build: name it "lumos_dev"
    set_target_properties(lumos_target PROPERTIES OUTPUT_NAME "lumos_dev")
    message(STATUS "Building development binary: lumos_dev")
endif()

# Add yaml-cpp include directory
target_include_directories(lumos_target
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set C++ standard
target_compile_features(lumos_target PRIVATE cxx_std_17)

# Link with yaml-cpp, serial library, and filesystem library
target_link_libraries(lumos_target
    lumos_serial
    yaml-cpp::yaml-cpp
)

if(UNIX AND NOT APPLE)
    target_link_libraries(lumos_target stdc++fs)
endif()
