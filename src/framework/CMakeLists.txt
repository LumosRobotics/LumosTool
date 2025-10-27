cmake_minimum_required(VERSION 3.14)

# Lumos Framework Library

set(FRAMEWORK_SOURCES
    application.cpp
)

set(FRAMEWORK_HEADERS
    application.h
)

# Create static library
add_library(LumosFramework STATIC ${FRAMEWORK_SOURCES} ${FRAMEWORK_HEADERS})

# Set C++ standard
target_compile_features(LumosFramework PUBLIC cxx_std_17)

# Include directories
target_include_directories(LumosFramework
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

# Compile options
target_compile_options(LumosFramework PRIVATE
    -Wall
    -Wextra
    -pedantic
)

# Installation (optional)
install(TARGETS LumosFramework
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${FRAMEWORK_HEADERS}
    DESTINATION include/framework
)
