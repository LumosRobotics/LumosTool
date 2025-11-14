/**
 * @file lumos.h
 * @brief Lumos framework header - provides setup/loop interface
 *
 * Include this header in your user code to use the setup/loop pattern.
 * This header automatically handles C/C++ linkage, so you don't need
 * to use extern "C" in your code.
 */

#ifndef LUMOS_H
#define LUMOS_H

// Include STM32 HAL for the board
#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Setup function - called once at startup
 *
 * This function is called once after board initialization (HAL, clocks, etc.)
 * Use this to initialize your peripherals, variables, and application state.
 */
void setup(void);

/**
 * @brief Loop function - called repeatedly
 *
 * This function is called continuously in an infinite loop after setup().
 * Put your main application logic here.
 */
void loop(void);

#ifdef __cplusplus
}
#endif

#endif // LUMOS_H
