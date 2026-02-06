#include "sys.h"

// Static variables for time tracking
static volatile uint32_t last_tick = 0;
static volatile uint32_t tick_rollovers = 0;
static volatile bool dwt_initialized = false;

void DelayMs(uint32_t ms)
{
    HAL_Delay(ms);
}

void DelayUs(uint32_t us)
{
    // Get CPU frequency in MHz
    uint32_t cpu_freq_mhz;

#if defined(STM32H7)
    cpu_freq_mhz = HAL_RCC_GetSysClockFreq() / 1000000;
#elif defined(STM32G0)
    cpu_freq_mhz = HAL_RCC_GetSysClockFreq() / 1000000;
#elif defined(STM32G4)
    cpu_freq_mhz = HAL_RCC_GetSysClockFreq() / 1000000;
#elif defined(STM32F4)
    cpu_freq_mhz = HAL_RCC_GetSysClockFreq() / 1000000;
#elif defined(STM32H5)
    cpu_freq_mhz = HAL_RCC_GetSysClockFreq() / 1000000;
#endif

    // Cycle-accurate delay
    // Each loop iteration takes approximately 3-4 cycles
    // cycles_per_us = (cpu_freq_mhz * us) / cycles_per_loop
    uint32_t cycles = (cpu_freq_mhz * us) / 4;

    for (volatile uint32_t i = 0; i < cycles; i++) {
        __NOP();
    }
}

void InitMicrosecondTiming()
{
#if defined(__CORTEX_M) && (__CORTEX_M >= 3)
    // DWT (Data Watchpoint and Trace) is available on Cortex-M3 and above
    // Enable TRC (trace) and access to DWT registers
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // Reset the cycle counter
    DWT->CYCCNT = 0;

    // Enable the cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    dwt_initialized = true;
#else
    // Cortex-M0/M0+ doesn't have DWT, fall back to millisecond timing
    dwt_initialized = false;
#endif
}

uint64_t GetCurrentTimeMs()
{
    // Handle HAL_GetTick() 32-bit rollover by detecting when it decreases
    uint32_t current_tick = HAL_GetTick();

    // Detect rollover (tick wrapped from 0xFFFFFFFF to 0)
    if (current_tick < last_tick) {
        tick_rollovers++;
    }
    last_tick = current_tick;

    // Combine rollovers and current tick into 64-bit value
    // Each rollover represents 2^32 milliseconds (~49.7 days)
    uint64_t total_ms = ((uint64_t)tick_rollovers << 32) | current_tick;

    return total_ms;
}

uint64_t GetCurrentTimeUs()
{
#if defined(__CORTEX_M) && (__CORTEX_M >= 3)
    if (dwt_initialized) {
        // Use DWT cycle counter for microsecond resolution
        uint32_t cycles = DWT->CYCCNT;
        uint32_t cpu_freq_mhz;

#if defined(STM32H7)
        cpu_freq_mhz = HAL_RCC_GetSysClockFreq() / 1000000;
#elif defined(STM32G0)
        cpu_freq_mhz = HAL_RCC_GetSysClockFreq() / 1000000;
#elif defined(STM32G4)
        cpu_freq_mhz = HAL_RCC_GetSysClockFreq() / 1000000;
#elif defined(STM32F4)
        cpu_freq_mhz = HAL_RCC_GetSysClockFreq() / 1000000;
#elif defined(STM32H5)
        cpu_freq_mhz = HAL_RCC_GetSysClockFreq() / 1000000;
#endif

        // Convert cycles to microseconds
        uint64_t us = (uint64_t)cycles / cpu_freq_mhz;

        // Add millisecond time for full 64-bit range
        uint64_t ms_time = GetCurrentTimeMs();
        return (ms_time * 1000) + (us % 1000);
    }
#endif

    // Fall back to millisecond resolution (* 1000)
    return GetCurrentTimeMs() * 1000;
}
