/**
 * @file main.cpp
 * @brief STM32G0 Timer module demonstration
 *
 * This example demonstrates:
 * - PWM generation with variable duty cycle
 * - Periodic timer callbacks
 * - Multiple timer channels
 *
 * Hardware Connections:
 * - TIM2_CH1 on PA15 (Bottom Connector Pin 2) - PWM output with ramping duty cycle
 * - TIM3_CH2 on PA7 (I2C3_SCL pin) - PWM output at fixed 50% duty cycle
 * - TIM14 - Periodic callback every 500ms to toggle LED pattern
 *
 * Note: PA7 is shared with I2C3_SCL. Don't use I2C3 if using this PWM output.
 */
#include "lumos.h"
#include "lumos_micro_brain.h"
#include "stm32g0xx_hal_tim.h"  // Force TIM HAL module detection
#include "timer.h"
#include "gpio.h"
#include "sys.h"

// Timer objects
Timer pwm1(TIM2);   // PWM with variable duty cycle
Timer pwm2(TIM3);   // PWM at 50% duty cycle
Timer periodic(TIM14);  // Periodic callback timer

// GPIO for visual feedback
GPIO led1(GPIOD, GPIO_PIN_2);   // PD2

// Variables for PWM ramping
float duty_cycle = 0.0f;
bool duty_increasing = true;

// Periodic callback function
void periodicCallback()
{
    // Toggle LED every 500ms
    led1.toggle();
}

/**
 * @brief Setup function - called once at startup
 */
void setup(void)
{
    // Initialize timing system
    InitMicrosecondTiming();

    // Configure status LED
    led1.mode(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    led1.low();

    // ===== Configure PWM1 on TIM2 Channel 1 (PA15) =====
    // 1 kHz PWM frequency, starting at 0% duty cycle
    if (pwm1.initPWM(1000, 0.0f)) {
        // Configure PA15 as TIM2_CH1
        pwm1.setPWMChannel(1, GPIOA, GPIO_PIN_15, GPIO_AF2_TIM2);
        pwm1.start();
    }

    // ===== Configure PWM2 on TIM3 Channel 2 (PA7) =====
    // 2 kHz PWM frequency at 50% duty cycle
    if (pwm2.initPWM(2000, 50.0f)) {
        // Configure PA7 as TIM3_CH2
        pwm2.setPWMChannel(2, GPIOA, GPIO_PIN_7, GPIO_AF1_TIM3);
        pwm2.start();
    }

    // ===== Configure periodic timer (TIM14) =====
    // 500ms periodic callback
    if (periodic.initPeriodic(500, periodicCallback)) {
        periodic.start();
    }

    // Give user time to connect oscilloscope/logic analyzer
    DelayMs(1000);
}

/**
 * @brief Loop function - called repeatedly
 */
void loop(void)
{
    // Ramp PWM1 duty cycle up and down (0% to 100%)
    if (duty_increasing) {
        duty_cycle += 0.5f;  // Increase by 0.5%
        if (duty_cycle >= 100.0f) {
            duty_cycle = 100.0f;
            duty_increasing = false;
        }
    } else {
        duty_cycle -= 0.5f;  // Decrease by 0.5%
        if (duty_cycle <= 0.0f) {
            duty_cycle = 0.0f;
            duty_increasing = true;
        }
    }

    // Update PWM1 duty cycle
    pwm1.setDutyCycle(1, duty_cycle);

    // Small delay to create smooth ramping
    // At 20ms per step with 0.5% increments, full ramp takes 4 seconds
    DelayMs(20);

    // PWM2 maintains constant 50% duty cycle (no changes needed)
    // Periodic timer callback automatically toggles LED every 500ms
}
