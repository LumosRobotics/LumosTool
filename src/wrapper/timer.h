#pragma once

#include <cstdint>
#include <functional>

// Platform-specific HAL headers
#if defined(STM32H7)
    #include "stm32h7xx_hal.h"
    #include "stm32h7xx_hal_tim.h"
#elif defined(STM32G0)
    #include "stm32g0xx_hal.h"
    #include "stm32g0xx_hal_tim.h"
#elif defined(STM32G4)
    #include "stm32g4xx_hal.h"
    #include "stm32g4xx_hal_tim.h"
#elif defined(STM32F4)
    #include "stm32f4xx_hal.h"
    #include "stm32f4xx_hal_tim.h"
#elif defined(STM32H5)
    #include "stm32h5xx_hal.h"
    #include "stm32h5xx_hal_tim.h"
#else
    #error "Unsupported STM32 platform. Define STM32H7, STM32G0, STM32G4, STM32F4, or STM32H5."
#endif

// Timer Class - Hardware Timer Control
//
// Usage Examples:
//
//   1. PWM Generation:
//      Timer pwm_timer(TIM2);
//      pwm_timer.initPWM(1000, 50);  // 1kHz, 50% duty cycle
//      pwm_timer.setPWMChannel(1, GPIOA, GPIO_PIN_0, GPIO_AF1_TIM2);
//      pwm_timer.start();
//      pwm_timer.setDutyCycle(1, 75.0f);  // Change to 75%
//
//   2. Periodic Callback:
//      Timer periodic(TIM3);
//      periodic.initPeriodic(100, []() { /* called every 100ms */ });
//      periodic.start();
//
//   3. Microsecond Counter:
//      Timer us_counter(TIM14);
//      us_counter.initMicrosecondCounter();  // 1μs per tick
//      us_counter.start();
//      uint32_t elapsed_us = us_counter.getCounter();
//
//   4. Millisecond Counter:
//      Timer ms_counter(TIM15);
//      ms_counter.initMillisecondCounter();  // 1ms per tick
//      ms_counter.start();
//      uint32_t elapsed_ms = ms_counter.getCounter();
//
//   5. Custom Tick Period:
//      Timer custom(TIM16);
//      custom.initTickPeriod(100);  // 100μs per tick
//      custom.start();
//      uint32_t ticks = custom.getCounter();
//
class Timer
{
public:
    // Callback type for periodic interrupts
    using TimerCallback = std::function<void()>;

private:
    TIM_HandleTypeDef tim_handle_;
    TIM_TypeDef* timer_;
    bool initialized_;
    uint32_t frequency_hz_;
    TimerCallback callback_;

    // Track which channels are configured
    bool channel_enabled_[4];  // Up to 4 channels

public:
    Timer() = delete;
    Timer(TIM_TypeDef* timer);
    ~Timer();

    // ===== Basic Timer Configuration =====

    /**
     * @brief Initialize timer for PWM generation
     * @param frequency_hz PWM frequency in Hz (e.g., 1000 = 1kHz)
     * @param duty_cycle Initial duty cycle 0-100 (percentage)
     * @return true if successful
     *
     * Example: timer.initPWM(1000, 50);  // 1kHz PWM at 50% duty cycle
     */
    bool initPWM(uint32_t frequency_hz, float duty_cycle = 50.0f);

    /**
     * @brief Initialize timer for periodic interrupts with callback
     * @param period_ms Period in milliseconds
     * @param callback Function to call on each timer interrupt
     * @return true if successful
     *
     * Example: timer.initPeriodic(100, []() { toggle_led(); });
     */
    bool initPeriodic(uint32_t period_ms, TimerCallback callback);

    /**
     * @brief Initialize timer with custom frequency
     * @param frequency_hz Timer frequency in Hz
     * @return true if successful
     *
     * Use this for free-running counters. Access counter with getCounter().
     * Example: timer.initFrequency(1000000);  // 1 MHz = 1μs per tick
     */
    bool initFrequency(uint32_t frequency_hz);

    /**
     * @brief Initialize timer with specific tick period in microseconds
     * @param tick_period_us Tick period in microseconds (e.g., 1 for 1μs per tick)
     * @return true if successful
     *
     * Creates a free-running timer that increments every tick_period_us.
     * Example: timer.initTickPeriod(10);  // Counter increments every 10μs
     */
    bool initTickPeriod(uint32_t tick_period_us);

    /**
     * @brief Initialize timer as a microsecond counter
     * @return true if successful
     *
     * Convenience method equivalent to initTickPeriod(1).
     * Counter value directly represents microseconds.
     * Example: timer.initMicrosecondCounter(); uint32_t us = timer.getCounter();
     */
    bool initMicrosecondCounter();

    /**
     * @brief Initialize timer as a millisecond counter
     * @return true if successful
     *
     * Convenience method equivalent to initTickPeriod(1000).
     * Counter value directly represents milliseconds.
     * Example: timer.initMillisecondCounter(); uint32_t ms = timer.getCounter();
     */
    bool initMillisecondCounter();

    // ===== PWM Channel Configuration =====

    /**
     * @brief Configure a PWM channel with GPIO pin
     * @param channel Timer channel (1-4)
     * @param port GPIO port (GPIOA, GPIOB, etc.)
     * @param pin GPIO pin (GPIO_PIN_0, etc.)
     * @param alternate Alternate function (GPIO_AF1_TIMx, etc.)
     * @return true if successful
     *
     * Must call initPWM() first!
     * Example: timer.setPWMChannel(1, GPIOA, GPIO_PIN_0, GPIO_AF1_TIM2);
     */
    bool setPWMChannel(uint32_t channel, GPIO_TypeDef* port, uint16_t pin, uint32_t alternate);

    /**
     * @brief Set PWM duty cycle for a channel
     * @param channel Timer channel (1-4)
     * @param duty_cycle Duty cycle 0-100 (percentage)
     *
     * Example: timer.setDutyCycle(1, 75.0f);  // 75% duty cycle on channel 1
     */
    void setDutyCycle(uint32_t channel, float duty_cycle);

    /**
     * @brief Get current PWM duty cycle for a channel
     * @param channel Timer channel (1-4)
     * @return Duty cycle 0-100 (percentage)
     */
    float getDutyCycle(uint32_t channel) const;

    // ===== Timer Control =====

    /**
     * @brief Start the timer
     * For PWM: starts PWM generation on configured channels
     * For periodic: starts interrupt generation
     */
    void start();

    /**
     * @brief Stop the timer
     */
    void stop();

    /**
     * @brief Start a specific PWM channel
     * @param channel Timer channel (1-4)
     */
    void startChannel(uint32_t channel);

    /**
     * @brief Stop a specific PWM channel
     * @param channel Timer channel (1-4)
     */
    void stopChannel(uint32_t channel);

    // ===== Timer Information =====

    /**
     * @brief Get configured frequency in Hz
     * @return Frequency in Hz
     */
    uint32_t getFrequency() const { return frequency_hz_; }

    /**
     * @brief Get current timer counter value
     * @return Counter value
     */
    uint32_t getCounter() const;

    /**
     * @brief Set timer counter value
     * @param value Counter value
     */
    void setCounter(uint32_t value);

    /**
     * @brief Check if timer is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }

    // ===== IRQ Handler (for internal use) =====

    /**
     * @brief Handle timer interrupt (called from IRQ handler)
     * This is used internally by the HAL callback system
     */
    void handleInterrupt();

    // ===== Utilities =====

    /**
     * @brief Get timer handle (for advanced usage)
     * @return Pointer to TIM_HandleTypeDef
     */
    TIM_HandleTypeDef* getHandle() { return &tim_handle_; }

private:
    // Helper functions
    void enableTimerClock();
    uint32_t channelToHAL(uint32_t channel) const;
};

// ===== Helper Functions =====

/**
 * @brief Calculate timer prescaler and period for target frequency
 * @param timer_clock Timer input clock frequency in Hz
 * @param target_freq Target output frequency in Hz
 * @param prescaler Output prescaler value
 * @param period Output period value
 * @return true if valid values found
 */
bool calculateTimerConfig(uint32_t timer_clock, uint32_t target_freq,
                         uint32_t& prescaler, uint32_t& period);

/**
 * @brief Get timer input clock frequency
 * @param timer Timer instance (TIM1, TIM2, etc.)
 * @return Clock frequency in Hz
 */
uint32_t getTimerClock(TIM_TypeDef* timer);
