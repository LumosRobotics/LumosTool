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

// GPIO Class - General Purpose Input/Output
// Usage Example:
//   GPIO led(GPIOA, GPIO_PIN_5);
//   led.mode(GPIO_MODE_OUTPUT_PP);
//   led.write(true);   // Turn on
//   led.toggle();      // Toggle state
//
//   GPIO button(GPIOB, GPIO_PIN_0);
//   button.mode(GPIO_MODE_INPUT, GPIO_PULLUP);
//   bool pressed = button.read();
class GPIO
{
private:
    GPIO_TypeDef* port_;
    uint16_t pin_;
    bool initialized_;

public:
    GPIO() = delete;
    GPIO(GPIO_TypeDef* port, uint16_t pin);

    // Configuration
    void mode(uint32_t mode, uint32_t pull = GPIO_NOPULL, uint32_t speed = GPIO_SPEED_FREQ_LOW);
    void setAlternateFunction(uint32_t alternate);

    // Digital I/O
    void write(bool value);
    bool read() const;
    void toggle();
    void high();
    void low();

    // Utilities
    GPIO_TypeDef* getPort() const { return port_; }
    uint16_t getPin() const { return pin_; }
};

// Helper functions for quick GPIO operations (Arduino-style)
// These require specifying both port and pin explicitly

/**
 * @brief Configure a GPIO pin mode
 * @param port GPIO port (GPIOA, GPIOB, etc.)
 * @param pin GPIO pin number (GPIO_PIN_0 to GPIO_PIN_15)
 * @param mode GPIO mode (GPIO_MODE_OUTPUT_PP, GPIO_MODE_INPUT, etc.)
 * @param pull Pull-up/down config (GPIO_NOPULL, GPIO_PULLUP, GPIO_PULLDOWN)
 */
void pinMode(GPIO_TypeDef* port, uint16_t pin, uint32_t mode, uint32_t pull = GPIO_NOPULL);

/**
 * @brief Write digital value to GPIO pin
 * @param port GPIO port (GPIOA, GPIOB, etc.)
 * @param pin GPIO pin number (GPIO_PIN_0 to GPIO_PIN_15)
 * @param value true for HIGH, false for LOW
 */
void digitalWrite(GPIO_TypeDef* port, uint16_t pin, bool value);

/**
 * @brief Read digital value from GPIO pin
 * @param port GPIO port (GPIOA, GPIOB, etc.)
 * @param pin GPIO pin number (GPIO_PIN_0 to GPIO_PIN_15)
 * @return true for HIGH, false for LOW
 */
bool digitalRead(GPIO_TypeDef* port, uint16_t pin);

/**
 * @brief Toggle GPIO pin state
 * @param port GPIO port (GPIOA, GPIOB, etc.)
 * @param pin GPIO pin number (GPIO_PIN_0 to GPIO_PIN_15)
 */
void togglePin(GPIO_TypeDef* port, uint16_t pin);
