/**
 * @file test_tick_counter.cpp
 * @brief Example demonstrating arbitrary tick period timers
 *
 * This example shows how to use timers as free-running counters
 * with specific tick periods.
 */
#include "lumos.h"
#include "lumos_micro_brain.h"
#include "stm32g0xx_hal_tim.h"
#include "timer.h"
#include "gpio.h"
#include "sys.h"

// Timer objects for different tick periods
Timer us_timer(TIM2);     // Microsecond counter
Timer ms_timer(TIM3);     // Millisecond counter
Timer custom_timer(TIM14); // Custom 100μs tick period

// Status LED
GPIO led(GPIOD, GPIO_PIN_2);

void setup(void)
{
    // Configure status LED
    led.mode(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    led.low();

    // ===== Example 1: Microsecond counter =====
    // Timer counts in microseconds - getCounter() returns μs
    if (us_timer.initMicrosecondCounter()) {
        us_timer.start();
    }

    // ===== Example 2: Millisecond counter =====
    // Timer counts in milliseconds - getCounter() returns ms
    if (ms_timer.initMillisecondCounter()) {
        ms_timer.start();
    }

    // ===== Example 3: Custom tick period (100μs) =====
    // Timer increments every 100μs
    if (custom_timer.initTickPeriod(100)) {
        custom_timer.start();
    }

    DelayMs(100);  // Let timers stabilize
}

void loop(void)
{
    // Read microsecond counter
    uint32_t us_elapsed = us_timer.getCounter();

    // Read millisecond counter
    uint32_t ms_elapsed = ms_timer.getCounter();

    // Read custom timer (counts in 100μs units)
    uint32_t custom_ticks = custom_timer.getCounter();
    uint32_t custom_us = custom_ticks * 100;  // Convert to microseconds

    // Toggle LED every ~500ms based on millisecond counter
    static uint32_t last_toggle = 0;
    if (ms_elapsed - last_toggle >= 500) {
        led.toggle();
        last_toggle = ms_elapsed;
    }

    // Small delay to avoid excessive polling
    DelayMs(10);

    // Usage notes:
    // - us_elapsed: microseconds since timer started
    // - ms_elapsed: milliseconds since timer started
    // - custom_ticks: number of 100μs periods since timer started
    // - Counter wraps at 65535 for 16-bit timers
    // - Calculate wrap time: 65535 * tick_period_us microseconds
}
