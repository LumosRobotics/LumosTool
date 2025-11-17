cmake_minimum_required(VERSION 3.14)

project(LumosSerial)

# Platform-specific serial implementation
if(WIN32)
    set(SERIAL_IMPL serial_windows.cpp)
elseif(APPLE)
    set(SERIAL_IMPL serial_mac.cpp)
else()
    set(SERIAL_IMPL serial_linux.cpp)
endif()

set(SERIAL_SOURCES
    ${SERIAL_IMPL}
    stm32_communicator.cpp
)

set(SERIAL_HEADERS
    serial.h
    stm32_communicator.h
)

# Create static library
add_library(lumos_serial STATIC ${SERIAL_SOURCES} ${SERIAL_HEADERS})

# Set C++ standard
target_compile_features(lumos_serial PUBLIC cxx_std_17)

# Export include directory for consumers
target_include_directories(lumos_serial PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
