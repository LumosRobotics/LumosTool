cmake_minimum_required(VERSION 3.14)

# Platform-specific serial implementation
if(WIN32)
    set(SERIAL_IMPL serial_windows.cpp)
elseif(APPLE)
    set(SERIAL_IMPL serial_mac.cpp)
else()
    set(SERIAL_IMPL serial_linux.cpp)
endif()

set(SIMPLE_SERIAL_SOURCES
    main.cpp
    ${SERIAL_IMPL}
)

add_executable(simple_serial ${SIMPLE_SERIAL_SOURCES})

# Set C++ standard
target_compile_features(simple_serial PRIVATE cxx_std_17)
