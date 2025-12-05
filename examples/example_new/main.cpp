/**
 * Main application file
 * This is where the setup() and loop() functions are defined
 */

#include "source_file.h"
#include <jst_shield.h>
#include <lumos_brain.h>
/**
 * Setup function - called once at startup
 */
void setup()
{
    // Initialize your application here
    // - Configure GPIO pins
    // - Initialize UART, SPI, I2C, etc.
    // - Set up timers
    someFunction();

    Serial7.begin(115200);
}

/**
 * Loop function - called repeatedly
 */
void loop()
{
    // Your main application logic here
    // This function runs continuously
}
