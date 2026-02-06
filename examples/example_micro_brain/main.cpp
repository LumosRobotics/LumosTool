/**
 * @file main.cpp
 * @brief STM32G0 example testing sys and gpio modules
 *
 * This example demonstrates:
 * - GPIO class usage (object-oriented API)
 * - GPIO helper functions (Arduino-style API)
 * - System timing functions (DelayMs, DelayUs)
 * - Time tracking (GetCurrentTimeMs, GetCurrentTimeUs)
 *
 * GPIO Pins Used:
 * - PA15 (Bottom Connector Pin 2) - GPIO class demo
 * - PD2  (Bottom Connector Pin 4) - Helper functions demo
 * - PA2  (Bottom Connector Pin 13) - Additional output
 * - PA1  (Bottom Connector Pin 15) - Additional output
 */
#include "lumos.h"
#include "lumos_micro_brain.h"
#include "gpio.h"
#include "sys.h"

// GPIO objects for testing GPIO class
GPIO led1(GPIOA, GPIO_PIN_15);  // PA15
GPIO led2(GPIOA, GPIO_PIN_2);   // PA2

// Global variables for timing tests
static uint64_t start_time_ms = 0;
static uint64_t start_time_us = 0;
static uint32_t loop_counter = 0;

/**
 * @brief Setup function - called once at startup
 *
 * Initializes GPIO pins and timing subsystem.
 */
void setup(void)
{
    // Initialize microsecond timing (enables DWT if available)
    InitMicrosecondTiming();

    // Configure GPIO using GPIO class (object-oriented API)
    led1.mode(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    led2.mode(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);

    // Configure GPIO using helper functions (Arduino-style API)
    pinMode(GPIOD, GPIO_PIN_2, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);   // PD2
    pinMode(GPIOA, GPIO_PIN_1, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);   // PA1

    // Initialize all pins to LOW
    led1.low();
    led2.low();
    digitalWrite(GPIOD, GPIO_PIN_2, false);
    digitalWrite(GPIOA, GPIO_PIN_1, false);

    // Record start time
    start_time_ms = GetCurrentTimeMs();
    start_time_us = GetCurrentTimeUs();

    loop_counter = 0;
}

/**
 * @brief Loop function - called repeatedly
 *
 * Demonstrates various GPIO and timing operations.
 */
void loop(void)
{
    loop_counter++;

    // Test 1: GPIO class methods - toggle PA15 (led1)
    led1.toggle();

    // Test 2: Millisecond delay
    DelayMs(100);

    // Test 3: GPIO class write - toggle PA2 (led2)
    led2.write(!led2.read());

    // Test 4: Microsecond delay
    DelayUs(50);

    // Test 5: Helper functions - toggle PD2
    bool pd2_state = digitalRead(GPIOD, GPIO_PIN_2);
    digitalWrite(GPIOD, GPIO_PIN_2, !pd2_state);

    // Test 6: Helper function toggle - toggle PA1
    togglePin(GPIOA, GPIO_PIN_1);

    // Test 7: More delays to create visible blinking pattern
    DelayMs(100);

    // Test 8: High/Low convenience methods
    if (loop_counter % 10 == 0) {
        led1.high();
        DelayMs(50);
        led1.low();
    }

    // Test 9: Time tracking every 50 loops (~10 seconds)
    if (loop_counter % 50 == 0) {
        uint64_t elapsed_ms = GetCurrentTimeMs() - start_time_ms;
        uint64_t elapsed_us = GetCurrentTimeUs() - start_time_us;

        // Flash all LEDs to indicate timing milestone
        led1.high();
        led2.high();
        digitalWrite(GPIOD, GPIO_PIN_2, true);
        digitalWrite(GPIOA, GPIO_PIN_1, true);

        DelayMs(200);

        led1.low();
        led2.low();
        digitalWrite(GPIOD, GPIO_PIN_2, false);
        digitalWrite(GPIOA, GPIO_PIN_1, false);

        // Timing values are calculated but not displayed (no serial output in this example)
        // In a real application, you would send elapsed_ms and elapsed_us over UART
        (void)elapsed_ms;  // Suppress unused variable warning
        (void)elapsed_us;
    }

    // Main delay between loop iterations
    DelayMs(100);
}
