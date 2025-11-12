cmake_minimum_required(VERSION 3.16)

# Project configuration
project(lumos_uart_wrapper_example
    VERSION 1.0.0
    DESCRIPTION "C++ UART Wrapper Example for STM32H7"
    LANGUAGES C CXX ASM
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler settings for embedded development
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Cross compilation settings
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_RANLIB arm-none-eabi-ranlib)

# Processor-specific settings for STM32H7
set(CPU_FLAGS "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")

# Compiler flags
set(CMAKE_C_FLAGS "${CPU_FLAGS} -Wall -Wextra -Wshadow -Wundef -Wformat=2 -fdata-sections -ffunction-sections")
set(CMAKE_CXX_FLAGS "${CPU_FLAGS} -Wall -Wextra -Wshadow -Wundef -Wformat=2 -fdata-sections -ffunction-sections -fno-exceptions -fno-rtti")

# Debug flags
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -DDEBUG")

# Release flags
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CPU_FLAGS} -Wl,--gc-sections -Wl,--print-memory-usage -specs=nano.specs -specs=nosys.specs")

# STM32H7 specific definitions
add_definitions(
    -DSTM32H743xx
    -DUSE_HAL_DRIVER
    -DHSE_VALUE=25000000U
    -DHSE_STARTUP_TIMEOUT=100U
    -DLSE_STARTUP_TIMEOUT=5000U
    -DLSE_VALUE=32768U
    -DHSI_VALUE=64000000U
    -DLSI_VALUE=32000U
    -DVDD_VALUE=3300U
    -DPREFETCH_ENABLE=1U
    -DART_ACCCELERATION_ENABLE=1U
)

# Include directories
include_directories(
    # STM32H7 HAL and CMSIS
    ${CMAKE_SOURCE_DIR}/third_party/stm32h7xx_hal_driver/Inc
    ${CMAKE_SOURCE_DIR}/third_party/stm32h7xx_hal_driver/Inc/Legacy
    ${CMAKE_SOURCE_DIR}/third_party/cmsis_device_h7/Include
    ${CMAKE_SOURCE_DIR}/third_party/cmsis_core/Include
    
    # Current project
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Source files
set(SOURCES
    # Main application
    main_example.cpp
    uart_wrapper.cpp
    
    # STM32H7 HAL sources (you would need to include the actual HAL sources)
    # For now, we'll assume they're linked as a library
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries (assuming STM32H7 HAL is built as a library)
# target_link_libraries(${PROJECT_NAME} stm32h7_hal)

# Set output properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}"
    SUFFIX ".elf"
)

# Generate additional output formats
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND arm-none-eabi-objcopy -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND arm-none-eabi-size $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Generating HEX and BIN files"
)

# Print memory usage
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND arm-none-eabi-nm --print-size --size-sort --radix=d $<TARGET_FILE:${PROJECT_NAME}> > ${PROJECT_NAME}.symbols
    COMMENT "Generating symbol table"
)