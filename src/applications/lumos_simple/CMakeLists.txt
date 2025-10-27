cmake_minimum_required(VERSION 3.14)

# Simple Lumos Build Tool

set(LUMOS_SOURCES
    main.cpp
    project_config.cpp
    builder.cpp
)

add_executable(lumos_simple ${LUMOS_SOURCES})

target_include_directories(lumos_simple
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set C++ standard
target_compile_features(lumos_simple PRIVATE cxx_std_17)

# Link with filesystem library if needed
if(NOT APPLE)
    target_link_libraries(lumos_simple stdc++fs)
endif()
