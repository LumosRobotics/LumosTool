cmake_minimum_required(VERSION 3.14)

set(SIMPLE_SERIAL_SOURCES
    main.cpp
)

add_executable(simple_serial ${SIMPLE_SERIAL_SOURCES})

# Set C++ standard
target_compile_features(simple_serial PRIVATE cxx_std_17)

# Link with serial library
target_link_libraries(simple_serial
    lumos_serial
)
