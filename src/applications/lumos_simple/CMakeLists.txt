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
        ${CMAKE_SOURCE_DIR}/third_party/yaml-cpp/include
)

# Set C++ standard
target_compile_features(lumos_target PRIVATE cxx_std_17)

# Link with yaml-cpp, serial library, and filesystem library
target_link_libraries(lumos_target
    lumos_serial
    ${CMAKE_SOURCE_DIR}/third_party/yaml-cpp/build/libyaml-cpp.a
)

if(NOT APPLE)
    target_link_libraries(lumos_target stdc++fs)
endif()
