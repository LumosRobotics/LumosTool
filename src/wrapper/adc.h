#pragma once

#include <cstdint>
#include <functional>

// Platform-specific HAL headers
#if defined(STM32H7)
    #include "stm32h7xx_hal.h"
    #include "stm32h7xx_hal_adc.h"
#elif defined(STM32G0)
    #include "stm32g0xx_hal.h"
    #include "stm32g0xx_hal_adc.h"
#elif defined(STM32G4)
    #include "stm32g4xx_hal.h"
    #include "stm32g4xx_hal_adc.h"
#elif defined(STM32F4)
    #include "stm32f4xx_hal.h"
    #include "stm32f4xx_hal_adc.h"
#elif defined(STM32H5)
    #include "stm32h5xx_hal.h"
    #include "stm32h5xx_hal_adc.h"
#else
    #error "Unsupported STM32 platform. Define STM32H7, STM32G0, STM32G4, STM32F4, or STM32H5."
#endif

// AnalogInput Class - Analog to Digital Converter
// Note: Class is named AnalogInput to avoid conflict with ADC macro in STM32 headers
//
// Usage Examples:
//
//   1. Single Channel Reading:
//      AnalogInput adc(ADC1);
//      adc.init();
//      adc.configureChannel(1, GPIOA, GPIO_PIN_0);  // Channel 1 on PA0
//      uint16_t value = adc.read();  // 12-bit value (0-4095)
//      float voltage = adc.readVoltage();  // Convert to voltage
//
//   2. Multiple Channels:
//      AnalogInput adc(ADC1);
//      adc.init();
//      adc.configureChannel(1, GPIOA, GPIO_PIN_0);
//      adc.configureChannel(2, GPIOA, GPIO_PIN_1);
//      uint16_t ch1 = adc.read(1);
//      uint16_t ch2 = adc.read(2);
//
//   3. Continuous Mode:
//      AnalogInput adc(ADC1);
//      adc.init(ADC_RESOLUTION_12B, true);  // Continuous mode
//      adc.configureChannel(1, GPIOA, GPIO_PIN_0);
//      adc.startContinuous();
//      uint16_t value = adc.getValue();  // Get latest value
//      adc.stop();
//
//   4. Internal Channels (Temperature, VRef):
//      AnalogInput adc(ADC1);
//      adc.init();
//      float temp = adc.readTemperature();
//      float vref = adc.readVRef();
//
class AnalogInput
{
private:
    ADC_HandleTypeDef adc_handle_;
    ADC_TypeDef* adc_;
    bool initialized_;
    uint32_t resolution_;
    float vref_voltage_;  // Reference voltage (default 3.3V)

    // Channel configuration
    static constexpr uint8_t MAX_CHANNELS = 18;
    uint32_t configured_channels_[MAX_CHANNELS];
    uint8_t num_configured_channels_;
    uint8_t current_channel_;

    // Continuous mode storage
    uint16_t latest_value_;
    bool continuous_mode_;

public:
    AnalogInput() = delete;
    AnalogInput(ADC_TypeDef* adc);
    ~AnalogInput();

    // ===== Initialization =====

    /**
     * @brief Initialize ADC with default settings
     * @param resolution ADC resolution (ADC_RESOLUTION_12B, ADC_RESOLUTION_10B, etc.)
     * @param continuous Enable continuous conversion mode
     * @param vref_voltage Reference voltage in volts (default 3.3V)
     * @return true if successful
     *
     * Example: adc.init(ADC_RESOLUTION_12B, false, 3.3f);
     */
    bool init(uint32_t resolution = ADC_RESOLUTION_12B,
              bool continuous = false,
              float vref_voltage = 3.3f);

    /**
     * @brief Calibrate ADC (recommended before first use)
     * @return true if successful
     *
     * Call this once after init() for better accuracy.
     */
    bool calibrate();

    // ===== Channel Configuration =====

    /**
     * @brief Configure an ADC channel with GPIO pin
     * @param channel ADC channel number (0-18)
     * @param port GPIO port (GPIOA, GPIOB, etc.), or nullptr for internal channels
     * @param pin GPIO pin (GPIO_PIN_0, etc.), or 0 for internal channels
     * @param sampling_time Sampling time (platform-specific constant)
     * @return true if successful
     *
     * Example: adc.configureChannel(1, GPIOA, GPIO_PIN_0);
     */
    bool configureChannel(uint32_t channel,
                         GPIO_TypeDef* port = nullptr,
                         uint16_t pin = 0,
                         uint32_t sampling_time = 0);

    /**
     * @brief Configure internal temperature sensor channel
     * @return true if successful
     */
    bool configureTemperatureChannel();

    /**
     * @brief Configure internal voltage reference channel
     * @return true if successful
     */
    bool configureVRefChannel();

    // ===== Single Conversion Reading =====

    /**
     * @brief Read ADC value from configured channel (blocking)
     * @param channel Channel to read (uses last configured if not specified)
     * @param timeout_ms Timeout in milliseconds
     * @return ADC value (0-4095 for 12-bit)
     *
     * Example: uint16_t val = adc.read();
     */
    uint16_t read(uint32_t channel = 0, uint32_t timeout_ms = 100);

    /**
     * @brief Read ADC value and convert to voltage
     * @param channel Channel to read
     * @param timeout_ms Timeout in milliseconds
     * @return Voltage in volts
     *
     * Example: float volts = adc.readVoltage();
     */
    float readVoltage(uint32_t channel = 0, uint32_t timeout_ms = 100);

    /**
     * @brief Read internal temperature sensor
     * @return Temperature in degrees Celsius
     */
    float readTemperature();

    /**
     * @brief Read internal voltage reference
     * @return VRef voltage in volts
     */
    float readVRef();

    // ===== Continuous Mode =====

    /**
     * @brief Start continuous conversion mode
     * @return true if successful
     *
     * Use getValue() to read latest value without blocking.
     */
    bool startContinuous();

    /**
     * @brief Stop continuous conversion mode
     */
    void stop();

    /**
     * @brief Get latest value from continuous mode (non-blocking)
     * @return Latest ADC value
     */
    uint16_t getValue() const { return latest_value_; }

    /**
     * @brief Get latest value as voltage (non-blocking)
     * @return Latest voltage reading
     */
    float getVoltage() const;

    // ===== Utilities =====

    /**
     * @brief Convert raw ADC value to voltage
     * @param raw_value Raw ADC reading
     * @return Voltage in volts
     */
    float rawToVoltage(uint16_t raw_value) const;

    /**
     * @brief Get maximum ADC value for current resolution
     * @return Maximum value (e.g., 4095 for 12-bit)
     */
    uint16_t getMaxValue() const;

    /**
     * @brief Set reference voltage (if different from default)
     * @param vref_voltage Reference voltage in volts
     */
    void setVRef(float vref_voltage) { vref_voltage_ = vref_voltage; }

    /**
     * @brief Check if ADC is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }

    /**
     * @brief Get ADC handle (for advanced usage)
     * @return Pointer to ADC_HandleTypeDef
     */
    ADC_HandleTypeDef* getHandle() { return &adc_handle_; }

    /**
     * @brief Handle ADC interrupt (called from IRQ handler)
     */
    void handleInterrupt();

private:
    // Helper functions
    void enableADCClock();
    uint32_t getADCChannel(uint32_t channel);
    void configureGPIOAnalog(GPIO_TypeDef* port, uint16_t pin);
    bool selectChannel(uint32_t channel, uint32_t sampling_time = 0);
    uint32_t stored_sampling_times_[MAX_CHANNELS];
};

// ===== Helper Functions =====

/**
 * @brief Get ADC clock frequency
 * @param adc ADC instance
 * @return Clock frequency in Hz
 */
uint32_t getADCClock(ADC_TypeDef* adc);
