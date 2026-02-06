#pragma once

#include <cstdint>

// Platform-specific HAL headers
#if defined(STM32H7)
    #include "stm32h7xx_hal.h"
#elif defined(STM32G0)
    #include "stm32g0xx_hal.h"
#elif defined(STM32G4)
    #include "stm32g4xx_hal.h"
#elif defined(STM32F4)
    #include "stm32f4xx_hal.h"
#elif defined(STM32H5)
    #include "stm32h5xx_hal.h"
#else
    #error "Unsupported STM32 platform. Define STM32H7, STM32G0, STM32G4, STM32F4, or STM32H5."
#endif

// System Utility Functions
// Usage Example:
//   DelayMs(1000);  // Delay for 1 second
//   DelayUs(100);   // Delay for 100 microseconds
//   uint64_t time = GetCurrentTimeMs();  // Get current time in milliseconds

/**
 * @brief Delay for specified milliseconds
 * @param ms Number of milliseconds to delay
 *
 * Uses HAL_Delay() which relies on SysTick timer (1ms tick)
 */
void DelayMs(uint32_t ms);

/**
 * @brief Delay for specified microseconds
 * @param us Number of microseconds to delay
 *
 * Uses cycle-accurate delay based on CPU frequency
 * Note: For very short delays (<10us), accuracy depends on CPU speed
 */
void DelayUs(uint32_t us);

/**
 * @brief Get current system time in milliseconds
 * @return uint64_t Time in milliseconds since system start
 *
 * Uses HAL_GetTick() which provides millisecond resolution
 * Automatically handles 32-bit rollover by extending to 64-bit
 */
uint64_t GetCurrentTimeMs();

/**
 * @brief Get current system time in microseconds
 * @return uint64_t Time in microseconds since system start
 *
 * Uses DWT cycle counter if available for microsecond resolution
 * Falls back to millisecond resolution if DWT not available
 */
uint64_t GetCurrentTimeUs();

/**
 * @brief Initialize microsecond timing (enables DWT if available)
 *
 * Call this once during initialization if you need microsecond timing
 * Not required for millisecond timing
 */
void InitMicrosecondTiming();