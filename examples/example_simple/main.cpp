/**
 * @file main.cpp
 * @brief Minimal STM32H7 example using setup() and loop() pattern
 *
 * This is the simplest possible example that demonstrates:
 * - setup() function called once on startup
 * - loop() function called repeatedly
 *
 * No peripherals are used - just core functionality.
 * The board's main.c handles HAL initialization and clock configuration.
 */
#include <iostream>
#include "lumos.h"
#include "source_file.h"

// Global counter variable
static uint32_t counter = 0;

/**
 * @brief Simple delay function
 * @param count Number of iterations to delay
 */
void SimpleDelay(volatile uint32_t count)
{
    while(count--) {
        __NOP();
    }
}

/**
 * @brief Setup function - called once at startup
 *
 * This function is called once after HAL initialization and clock configuration.
 * Use this to initialize your peripherals, variables, and application state.
 */
void setup(void)
{
    // Initialize your application here
    counter = 0;

    // Example: You could initialize peripherals here
    // - Configure GPIO pins
    // - Initialize UART
    // - Set up timers
    // - etc.
}

/**
 * @brief Loop function - called repeatedly
 *
 * This function is called continuously in a loop after setup() completes.
 * Put your main application logic here.
 */
void loop(void)
{
    // Increment counter
    counter++;
    char* msg = "Loop iteration\r\n";
    std::cout << msg << std::endl;

    // Simple delay (approximately 1 second at 550 MHz)
    SimpleDelay(50000000);

    // The counter just increments - no output since we're not using any peripherals
    // In a real application, you might:
    // - Toggle an LED
    // - Read sensors
    // - Process incoming data
    // - Update displays
    // - etc.
}
