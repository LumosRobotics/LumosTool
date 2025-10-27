cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# Lumos CLI Tool

set(LUMOS_SOURCES
    main.cpp
    cli.cpp
    config/project_config.cpp
    commands/project_command.cpp
    commands/app_command.cpp
)

add_executable(lumos ${LUMOS_SOURCES})

target_include_directories(lumos
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link with required libraries
# On macOS, std::filesystem is part of the standard library
# On Linux, you may need to link with stdc++fs
if(NOT APPLE)
    target_link_libraries(lumos stdc++fs)
endif()
