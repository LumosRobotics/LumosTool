#include "timer.h"
#include "gpio.h"
#include <algorithm>

// Static storage for timer callbacks (to handle IRQs)
static Timer* timer_instances[20] = {nullptr};  // Support up to 20 timers

// Helper to map timer instance to index
static int getTimerIndex(TIM_TypeDef* timer)
{
#ifdef TIM1
    if (timer == TIM1) return 0;
#endif
#ifdef TIM2
    if (timer == TIM2) return 1;
#endif
#ifdef TIM3
    if (timer == TIM3) return 2;
#endif
#ifdef TIM4
    if (timer == TIM4) return 3;
#endif
#ifdef TIM5
    if (timer == TIM5) return 4;
#endif
#ifdef TIM6
    if (timer == TIM6) return 5;
#endif
#ifdef TIM7
    if (timer == TIM7) return 6;
#endif
#ifdef TIM8
    if (timer == TIM8) return 7;
#endif
#ifdef TIM12
    if (timer == TIM12) return 8;
#endif
#ifdef TIM13
    if (timer == TIM13) return 9;
#endif
#ifdef TIM14
    if (timer == TIM14) return 10;
#endif
#ifdef TIM15
    if (timer == TIM15) return 11;
#endif
#ifdef TIM16
    if (timer == TIM16) return 12;
#endif
#ifdef TIM17
    if (timer == TIM17) return 13;
#endif
    return -1;
}

// Timer Class Implementation

Timer::Timer(TIM_TypeDef* timer)
    : timer_(timer),
      initialized_(false),
      frequency_hz_(0)
{
    tim_handle_.Instance = timer;

    for (int i = 0; i < 4; i++) {
        channel_enabled_[i] = false;
    }

    // Register this instance
    int idx = getTimerIndex(timer);
    if (idx >= 0 && idx < 20) {
        timer_instances[idx] = this;
    }

    enableTimerClock();
}

Timer::~Timer()
{
    if (initialized_) {
        stop();
        HAL_TIM_Base_DeInit(&tim_handle_);
    }

    // Unregister instance
    int idx = getTimerIndex(timer_);
    if (idx >= 0 && idx < 20) {
        timer_instances[idx] = nullptr;
    }
}

void Timer::enableTimerClock()
{
#ifdef TIM1
    if (timer_ == TIM1) __HAL_RCC_TIM1_CLK_ENABLE();
#endif
#ifdef TIM2
    if (timer_ == TIM2) __HAL_RCC_TIM2_CLK_ENABLE();
#endif
#ifdef TIM3
    if (timer_ == TIM3) __HAL_RCC_TIM3_CLK_ENABLE();
#endif
#ifdef TIM4
    if (timer_ == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();
#endif
#ifdef TIM5
    if (timer_ == TIM5) __HAL_RCC_TIM5_CLK_ENABLE();
#endif
#ifdef TIM6
    if (timer_ == TIM6) __HAL_RCC_TIM6_CLK_ENABLE();
#endif
#ifdef TIM7
    if (timer_ == TIM7) __HAL_RCC_TIM7_CLK_ENABLE();
#endif
#ifdef TIM8
    if (timer_ == TIM8) __HAL_RCC_TIM8_CLK_ENABLE();
#endif
#ifdef TIM12
    if (timer_ == TIM12) __HAL_RCC_TIM12_CLK_ENABLE();
#endif
#ifdef TIM13
    if (timer_ == TIM13) __HAL_RCC_TIM13_CLK_ENABLE();
#endif
#ifdef TIM14
    if (timer_ == TIM14) __HAL_RCC_TIM14_CLK_ENABLE();
#endif
#ifdef TIM15
    if (timer_ == TIM15) __HAL_RCC_TIM15_CLK_ENABLE();
#endif
#ifdef TIM16
    if (timer_ == TIM16) __HAL_RCC_TIM16_CLK_ENABLE();
#endif
#ifdef TIM17
    if (timer_ == TIM17) __HAL_RCC_TIM17_CLK_ENABLE();
#endif
}

bool Timer::initPWM(uint32_t frequency_hz, float duty_cycle)
{
    if (initialized_) {
        return false;
    }

    frequency_hz_ = frequency_hz;

    // Get timer clock
    uint32_t timer_clock = getTimerClock(timer_);

    // Calculate prescaler and period
    uint32_t prescaler, period;
    if (!calculateTimerConfig(timer_clock, frequency_hz, prescaler, period)) {
        return false;
    }

    // Configure timer base
    tim_handle_.Init.Prescaler = prescaler - 1;
    tim_handle_.Init.CounterMode = TIM_COUNTERMODE_UP;
    tim_handle_.Init.Period = period - 1;
    tim_handle_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim_handle_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&tim_handle_) != HAL_OK) {
        return false;
    }

    initialized_ = true;
    return true;
}

bool Timer::initPeriodic(uint32_t period_ms, TimerCallback callback)
{
    if (initialized_) {
        return false;
    }

    callback_ = callback;

    // Convert period to frequency
    uint32_t frequency_hz = 1000 / period_ms;  // Hz
    frequency_hz_ = frequency_hz;

    // Get timer clock
    uint32_t timer_clock = getTimerClock(timer_);

    // Calculate prescaler and period for desired frequency
    uint32_t prescaler, period;
    if (!calculateTimerConfig(timer_clock, frequency_hz, prescaler, period)) {
        return false;
    }

    // Configure timer base
    tim_handle_.Init.Prescaler = prescaler - 1;
    tim_handle_.Init.CounterMode = TIM_COUNTERMODE_UP;
    tim_handle_.Init.Period = period - 1;
    tim_handle_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim_handle_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_Base_Init(&tim_handle_) != HAL_OK) {
        return false;
    }

    initialized_ = true;
    return true;
}

bool Timer::initFrequency(uint32_t frequency_hz)
{
    if (initialized_) {
        return false;
    }

    frequency_hz_ = frequency_hz;

    // Get timer clock
    uint32_t timer_clock = getTimerClock(timer_);

    // Calculate prescaler and period
    uint32_t prescaler, period;
    if (!calculateTimerConfig(timer_clock, frequency_hz, prescaler, period)) {
        return false;
    }

    // Configure timer base
    tim_handle_.Init.Prescaler = prescaler - 1;
    tim_handle_.Init.CounterMode = TIM_COUNTERMODE_UP;
    tim_handle_.Init.Period = period - 1;
    tim_handle_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim_handle_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_Base_Init(&tim_handle_) != HAL_OK) {
        return false;
    }

    initialized_ = true;
    return true;
}

bool Timer::initTickPeriod(uint32_t tick_period_us)
{
    if (initialized_) {
        return false;
    }

    if (tick_period_us == 0) {
        return false;
    }

    // Get timer clock
    uint32_t timer_clock = getTimerClock(timer_);

    // Calculate prescaler to achieve desired tick period
    // Timer increments at: timer_clock / prescaler
    // We want: 1 tick every tick_period_us microseconds
    // So: timer_clock / prescaler = 1,000,000 / tick_period_us
    // Therefore: prescaler = timer_clock * tick_period_us / 1,000,000

    uint64_t prescaler_calc = ((uint64_t)timer_clock * tick_period_us) / 1000000;

    if (prescaler_calc == 0 || prescaler_calc > 65536) {
        // Prescaler out of range
        return false;
    }

    uint32_t prescaler = (uint32_t)prescaler_calc;
    uint32_t frequency_hz = 1000000 / tick_period_us;
    frequency_hz_ = frequency_hz;

    // Determine maximum period based on timer type
    // Check if this is a 32-bit timer (TIM2, TIM5 on most platforms)
    uint32_t max_period = 0xFFFF;  // Default 16-bit

#ifdef TIM2
    if (timer_ == TIM2) max_period = 0xFFFFFFFF;  // 32-bit
#endif
#ifdef TIM5
    if (timer_ == TIM5) max_period = 0xFFFFFFFF;  // 32-bit
#endif

    // Configure timer base for maximum period (free-running counter)
    tim_handle_.Init.Prescaler = prescaler - 1;
    tim_handle_.Init.CounterMode = TIM_COUNTERMODE_UP;
    tim_handle_.Init.Period = max_period;  // Maximum period for free-running
    tim_handle_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim_handle_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&tim_handle_) != HAL_OK) {
        return false;
    }

    initialized_ = true;
    return true;
}

bool Timer::initMicrosecondCounter()
{
    return initTickPeriod(1);  // 1μs per tick
}

bool Timer::initMillisecondCounter()
{
    return initTickPeriod(1000);  // 1ms per tick
}

bool Timer::setPWMChannel(uint32_t channel, GPIO_TypeDef* port, uint16_t pin, uint32_t alternate)
{
    if (!initialized_ || channel < 1 || channel > 4) {
        return false;
    }

    // Configure GPIO for alternate function
    GPIO gpio(port, pin);
    gpio.setAlternateFunction(alternate);

    // Configure PWM channel
    TIM_OC_InitTypeDef config = {0};
    config.OCMode = TIM_OCMODE_PWM1;
    config.Pulse = 0;  // Will be set by setDutyCycle
    config.OCPolarity = TIM_OCPOLARITY_HIGH;
    config.OCFastMode = TIM_OCFAST_DISABLE;

#if defined(STM32H7) || defined(STM32H5)
    config.OCIdleState = TIM_OCIDLESTATE_RESET;
    config.OCNIdleState = TIM_OCNIDLESTATE_RESET;
#endif

    uint32_t hal_channel = channelToHAL(channel);
    if (HAL_TIM_PWM_ConfigChannel(&tim_handle_, &config, hal_channel) != HAL_OK) {
        return false;
    }

    channel_enabled_[channel - 1] = true;
    return true;
}

void Timer::setDutyCycle(uint32_t channel, float duty_cycle)
{
    if (!initialized_ || channel < 1 || channel > 4) {
        return;
    }

    // Clamp duty cycle to 0-100
    duty_cycle = std::max(0.0f, std::min(100.0f, duty_cycle));

    // Calculate pulse value (CCR register value)
    uint32_t period = tim_handle_.Init.Period + 1;
    uint32_t pulse = (uint32_t)((duty_cycle / 100.0f) * period);

    // Set the compare value
    uint32_t hal_channel = channelToHAL(channel);
    __HAL_TIM_SET_COMPARE(&tim_handle_, hal_channel, pulse);
}

float Timer::getDutyCycle(uint32_t channel) const
{
    if (!initialized_ || channel < 1 || channel > 4) {
        return 0.0f;
    }

    uint32_t hal_channel = channelToHAL(channel);
    uint32_t pulse = __HAL_TIM_GET_COMPARE(&tim_handle_, hal_channel);
    uint32_t period = tim_handle_.Init.Period + 1;

    return (float)pulse / (float)period * 100.0f;
}

void Timer::start()
{
    if (!initialized_) {
        return;
    }

    // If any PWM channels are configured, start them
    bool has_pwm_channels = false;
    for (int i = 0; i < 4; i++) {
        if (channel_enabled_[i]) {
            uint32_t hal_channel = channelToHAL(i + 1);
            HAL_TIM_PWM_Start(&tim_handle_, hal_channel);
            has_pwm_channels = true;
        }
    }

    // If no PWM channels and we have a callback, start in interrupt mode
    if (!has_pwm_channels && callback_) {
        HAL_TIM_Base_Start_IT(&tim_handle_);
    } else if (!has_pwm_channels) {
        // Just start the base timer
        HAL_TIM_Base_Start(&tim_handle_);
    }
}

void Timer::stop()
{
    if (!initialized_) {
        return;
    }

    // Stop all PWM channels
    for (int i = 0; i < 4; i++) {
        if (channel_enabled_[i]) {
            uint32_t hal_channel = channelToHAL(i + 1);
            HAL_TIM_PWM_Stop(&tim_handle_, hal_channel);
        }
    }

    // Stop base timer
    if (callback_) {
        HAL_TIM_Base_Stop_IT(&tim_handle_);
    } else {
        HAL_TIM_Base_Stop(&tim_handle_);
    }
}

void Timer::startChannel(uint32_t channel)
{
    if (!initialized_ || channel < 1 || channel > 4 || !channel_enabled_[channel - 1]) {
        return;
    }

    uint32_t hal_channel = channelToHAL(channel);
    HAL_TIM_PWM_Start(&tim_handle_, hal_channel);
}

void Timer::stopChannel(uint32_t channel)
{
    if (!initialized_ || channel < 1 || channel > 4 || !channel_enabled_[channel - 1]) {
        return;
    }

    uint32_t hal_channel = channelToHAL(channel);
    HAL_TIM_PWM_Stop(&tim_handle_, hal_channel);
}

uint32_t Timer::getCounter() const
{
    if (!initialized_) {
        return 0;
    }
    return __HAL_TIM_GET_COUNTER(&tim_handle_);
}

void Timer::setCounter(uint32_t value)
{
    if (!initialized_) {
        return;
    }
    __HAL_TIM_SET_COUNTER(&tim_handle_, value);
}

void Timer::handleInterrupt()
{
    if (callback_) {
        callback_();
    }
}

uint32_t Timer::channelToHAL(uint32_t channel) const
{
    switch (channel) {
        case 1: return TIM_CHANNEL_1;
        case 2: return TIM_CHANNEL_2;
        case 3: return TIM_CHANNEL_3;
        case 4: return TIM_CHANNEL_4;
        default: return TIM_CHANNEL_1;
    }
}

// Helper Functions Implementation

bool calculateTimerConfig(uint32_t timer_clock, uint32_t target_freq,
                         uint32_t& prescaler, uint32_t& period)
{
    // Try to find best prescaler and period combination
    // Timer frequency = timer_clock / (prescaler * period)
    // We want: prescaler * period = timer_clock / target_freq

    uint32_t total_divisor = timer_clock / target_freq;

    // Try to balance prescaler and period for best resolution
    // Start with prescaler = 1 and increase if needed

    // Maximum values for 16-bit timers
    const uint32_t MAX_PRESCALER = 65536;
    const uint32_t MAX_PERIOD = 65536;

    // Try to keep period as large as possible for PWM resolution
    for (prescaler = 1; prescaler <= MAX_PRESCALER; prescaler++) {
        period = total_divisor / prescaler;

        if (period <= MAX_PERIOD && period > 0) {
            // Check if this gives us the exact frequency
            uint32_t actual_freq = timer_clock / (prescaler * period);

            // Allow 1% error
            if (actual_freq >= target_freq * 0.99 && actual_freq <= target_freq * 1.01) {
                return true;
            }
        }

        // If period is too small, we've gone too far
        if (period < 100 && prescaler > 1) {
            // Back up and use previous values
            prescaler--;
            period = total_divisor / prescaler;
            return true;
        }
    }

    // Fallback: use maximum period and calculate prescaler
    period = MAX_PERIOD;
    prescaler = total_divisor / period;

    if (prescaler > 0 && prescaler <= MAX_PRESCALER) {
        return true;
    }

    return false;
}

uint32_t getTimerClock(TIM_TypeDef* timer)
{
    // Get the appropriate bus clock for this timer
    uint32_t clock_freq = 0;

#if defined(STM32H7)
    // On STM32H7, timers are on APB1 or APB2
    RCC_ClkInitTypeDef clk_init;
    uint32_t flash_latency;
    HAL_RCC_GetClockConfig(&clk_init, &flash_latency);

    // Check which bus the timer is on
#ifdef TIM1
    if (timer == TIM1) {
        clock_freq = HAL_RCC_GetPCLK2Freq();
        if (clk_init.APB2CLKDivider != RCC_APB2_DIV1) clock_freq *= 2;
    }
#endif
#ifdef TIM2
    if (timer == TIM2) {
        clock_freq = HAL_RCC_GetPCLK1Freq();
        if (clk_init.APB1CLKDivider != RCC_APB1_DIV1) clock_freq *= 2;
    }
#endif
    // Add more timers as needed...

    // Default to PCLK1 if not found
    if (clock_freq == 0) {
        clock_freq = HAL_RCC_GetPCLK1Freq();
        if (clk_init.APB1CLKDivider != RCC_APB1_DIV1) clock_freq *= 2;
    }

#elif defined(STM32G0) || defined(STM32G4)
    // On STM32G0/G4, check APB timer clock
    RCC_ClkInitTypeDef clk_init;
    uint32_t flash_latency;
    HAL_RCC_GetClockConfig(&clk_init, &flash_latency);

    // Most timers on STM32G0 are on APB
    clock_freq = HAL_RCC_GetPCLK1Freq();

    // If APB prescaler != 1, timer clock is 2x APB clock
    if (clk_init.APB1CLKDivider != RCC_HCLK_DIV1) {
        clock_freq *= 2;
    }

#elif defined(STM32F4)
    RCC_ClkInitTypeDef clk_init;
    uint32_t flash_latency;
    HAL_RCC_GetClockConfig(&clk_init, &flash_latency);

    // Check APB1 vs APB2
    clock_freq = HAL_RCC_GetPCLK1Freq();
    if (clk_init.APB1CLKDivider != RCC_HCLK_DIV1) {
        clock_freq *= 2;
    }

#elif defined(STM32H5)
    RCC_ClkInitTypeDef clk_init;
    uint32_t flash_latency;
    HAL_RCC_GetClockConfig(&clk_init, &flash_latency);

    clock_freq = HAL_RCC_GetPCLK1Freq();
    if (clk_init.APB1CLKDivider != RCC_HCLK_DIV1) {
        clock_freq *= 2;
    }
#endif

    return clock_freq;
}

// HAL Callbacks (called from HAL interrupt handlers)

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // Find which timer instance this is
    for (int i = 0; i < 20; i++) {
        if (timer_instances[i] != nullptr) {
            if (timer_instances[i]->getHandle() == htim) {
                timer_instances[i]->handleInterrupt();
                break;
            }
        }
    }
}
