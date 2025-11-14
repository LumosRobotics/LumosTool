cmake_minimum_required(VERSION 3.14)

# Simple Lumos Build Tool

set(LUMOS_SOURCES
    main.cpp
    project_config.cpp
    builder.cpp
    hal_module_detector.cpp
)

add_executable(lumos ${LUMOS_SOURCES})

# Add yaml-cpp include directory
target_include_directories(lumos
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/third_party/yaml-cpp/include
)

# Set C++ standard
target_compile_features(lumos PRIVATE cxx_std_17)

# Link with yaml-cpp and filesystem library
target_link_libraries(lumos
    ${CMAKE_SOURCE_DIR}/third_party/yaml-cpp/build/libyaml-cpp.a
)

if(NOT APPLE)
    target_link_libraries(lumos stdc++fs)
endif()
