#include "adc.h"
#include "gpio.h"
#include <algorithm>

// Static storage for AnalogInput instances (for interrupt handling)
static AnalogInput* adc_instances[5] = {nullptr};

// Helper to map ADC instance to index
static int getADCIndex(ADC_TypeDef* adc)
{
#ifdef ADC1
    if (adc == ADC1) return 0;
#endif
#ifdef ADC2
    if (adc == ADC2) return 1;
#endif
#ifdef ADC3
    if (adc == ADC3) return 2;
#endif
#ifdef ADC4
    if (adc == ADC4) return 3;
#endif
#ifdef ADC5
    if (adc == ADC5) return 4;
#endif
    return -1;
}

// AnalogInput Class Implementation

AnalogInput::AnalogInput(ADC_TypeDef* adc)
    : adc_(adc),
      initialized_(false),
      resolution_(ADC_RESOLUTION_12B),
      vref_voltage_(3.3f),
      num_configured_channels_(0),
      current_channel_(0),
      latest_value_(0),
      continuous_mode_(false)
{
    adc_handle_.Instance = adc;

    for (int i = 0; i < MAX_CHANNELS; i++) {
        configured_channels_[i] = 0;
        stored_sampling_times_[i] = 0;
    }

    // Register this instance
    int idx = getADCIndex(adc);
    if (idx >= 0 && idx < 5) {
        adc_instances[idx] = this;
    }

    enableADCClock();
}

AnalogInput::~AnalogInput()
{
    if (initialized_) {
        stop();
        HAL_ADC_DeInit(&adc_handle_);
    }

    // Unregister instance
    int idx = getADCIndex(adc_);
    if (idx >= 0 && idx < 5) {
        adc_instances[idx] = nullptr;
    }
}

void AnalogInput::enableADCClock()
{
#ifdef ADC1
    if (adc_ == ADC1) __HAL_RCC_ADC_CLK_ENABLE();
#endif
#ifdef ADC2
    if (adc_ == ADC2) __HAL_RCC_ADC_CLK_ENABLE();
#endif
#ifdef ADC3
    if (adc_ == ADC3) __HAL_RCC_ADC_CLK_ENABLE();
#endif
#ifdef ADC4
    if (adc_ == ADC4) {
#if defined(STM32G4)
        __HAL_RCC_ADC345_CLK_ENABLE();
#endif
    }
#endif
#ifdef ADC5
    if (adc_ == ADC5) {
#if defined(STM32G4)
        __HAL_RCC_ADC345_CLK_ENABLE();
#endif
    }
#endif
}

bool AnalogInput::init(uint32_t resolution, bool continuous, float vref_voltage)
{
    if (initialized_) {
        return false;
    }

    resolution_ = resolution;
    vref_voltage_ = vref_voltage;
    continuous_mode_ = continuous;

    // Common ADC configuration
    adc_handle_.Init.Resolution = resolution;
    adc_handle_.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc_handle_.Init.ScanConvMode = ADC_SCAN_DISABLE;
    adc_handle_.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    adc_handle_.Init.ContinuousConvMode = continuous ? ENABLE : DISABLE;
    adc_handle_.Init.NbrOfConversion = 1;
    adc_handle_.Init.DiscontinuousConvMode = DISABLE;
    adc_handle_.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    adc_handle_.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;

#if defined(STM32H7) || defined(STM32H5)
    // H7/H5-specific settings
    adc_handle_.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;
    adc_handle_.Init.LowPowerAutoWait = DISABLE;
    adc_handle_.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    adc_handle_.Init.OversamplingMode = DISABLE;
    adc_handle_.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
    adc_handle_.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;

#elif defined(STM32G0)
    // G0-specific settings
    adc_handle_.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_1CYCLE_5;
    adc_handle_.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_1CYCLE_5;
    adc_handle_.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
    adc_handle_.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    adc_handle_.Init.LowPowerAutoWait = DISABLE;
    adc_handle_.Init.LowPowerAutoPowerOff = DISABLE;

#elif defined(STM32G4)
    // G4-specific settings
    adc_handle_.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;
    adc_handle_.Init.LowPowerAutoWait = DISABLE;
    adc_handle_.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    adc_handle_.Init.OversamplingMode = DISABLE;

#elif defined(STM32F4)
    // F4-specific settings
    adc_handle_.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    adc_handle_.Init.DMAContinuousRequests = DISABLE;

#endif

    if (HAL_ADC_Init(&adc_handle_) != HAL_OK) {
        return false;
    }

    initialized_ = true;
    return true;
}

bool AnalogInput::calibrate()
{
    if (!initialized_) {
        return false;
    }

#if defined(STM32H7) || defined(STM32H5) || defined(STM32G4)
    // Start calibration with differential/single-ended parameter
    if (HAL_ADCEx_Calibration_Start(&adc_handle_, ADC_SINGLE_ENDED) != HAL_OK) {
        return false;
    }
#elif defined(STM32G0)
    // G0 calibration without differential parameter
    if (HAL_ADCEx_Calibration_Start(&adc_handle_) != HAL_OK) {
        return false;
    }
#elif defined(STM32F4)
    // F4 doesn't have built-in calibration in the same way
    // Just return success
#endif

    return true;
}

bool AnalogInput::configureChannel(uint32_t channel, GPIO_TypeDef* port, uint16_t pin, uint32_t sampling_time)
{
    if (!initialized_ || channel >= MAX_CHANNELS) {
        return false;
    }

    // Configure GPIO pin as analog if provided
    if (port != nullptr && pin != 0) {
        configureGPIOAnalog(port, pin);
    }

    // Store channel configuration
    configured_channels_[num_configured_channels_] = channel;
    stored_sampling_times_[num_configured_channels_] = sampling_time;
    num_configured_channels_++;
    current_channel_ = channel;

    return true;
}

bool AnalogInput::configureTemperatureChannel()
{
#if defined(STM32H7)
    return configureChannel(ADC_CHANNEL_TEMPSENSOR, nullptr, 0, ADC_SAMPLETIME_810CYCLES_5);
#elif defined(STM32G0)
    return configureChannel(ADC_CHANNEL_TEMPSENSOR, nullptr, 0, ADC_SAMPLETIME_160CYCLES_5);
#elif defined(STM32G4)
    return configureChannel(ADC_CHANNEL_TEMPSENSOR_ADC1, nullptr, 0, ADC_SAMPLETIME_247CYCLES_5);
#elif defined(STM32F4)
    return configureChannel(ADC_CHANNEL_TEMPSENSOR, nullptr, 0, ADC_SAMPLETIME_480CYCLES);
#elif defined(STM32H5)
    return configureChannel(ADC_CHANNEL_TEMPSENSOR, nullptr, 0, ADC_SAMPLETIME_814CYCLES_5);
#else
    return false;
#endif
}

bool AnalogInput::configureVRefChannel()
{
#if defined(STM32H7)
    return configureChannel(ADC_CHANNEL_VREFINT, nullptr, 0, ADC_SAMPLETIME_810CYCLES_5);
#elif defined(STM32G0)
    return configureChannel(ADC_CHANNEL_VREFINT, nullptr, 0, ADC_SAMPLETIME_160CYCLES_5);
#elif defined(STM32G4)
    return configureChannel(ADC_CHANNEL_VREFINT, nullptr, 0, ADC_SAMPLETIME_247CYCLES_5);
#elif defined(STM32F4)
    return configureChannel(ADC_CHANNEL_VREFINT, nullptr, 0, ADC_SAMPLETIME_480CYCLES);
#elif defined(STM32H5)
    return configureChannel(ADC_CHANNEL_VREFINT, nullptr, 0, ADC_SAMPLETIME_814CYCLES_5);
#else
    return false;
#endif
}

uint16_t AnalogInput::read(uint32_t channel, uint32_t timeout_ms)
{
    if (!initialized_) {
        return 0;
    }

    // Find sampling time for the channel
    uint32_t sampling_time = 0;
    uint32_t ch = (channel != 0) ? channel : current_channel_;

    for (uint8_t i = 0; i < num_configured_channels_; i++) {
        if (configured_channels_[i] == ch) {
            sampling_time = stored_sampling_times_[i];
            break;
        }
    }

    // Select channel if specified
    if (channel != 0 && channel != current_channel_) {
        if (!selectChannel(channel, sampling_time)) {
            return 0;
        }
    } else if (current_channel_ != 0) {
        // Use current channel
        if (!selectChannel(current_channel_, sampling_time)) {
            return 0;
        }
    }

    // Start ADC conversion
    if (HAL_ADC_Start(&adc_handle_) != HAL_OK) {
        return 0;
    }

    // Wait for conversion to complete
    if (HAL_ADC_PollForConversion(&adc_handle_, timeout_ms) != HAL_OK) {
        HAL_ADC_Stop(&adc_handle_);
        return 0;
    }

    // Read value
    uint16_t value = (uint16_t)HAL_ADC_GetValue(&adc_handle_);

    // Stop ADC
    HAL_ADC_Stop(&adc_handle_);

    return value;
}

float AnalogInput::readVoltage(uint32_t channel, uint32_t timeout_ms)
{
    uint16_t raw = read(channel, timeout_ms);
    return rawToVoltage(raw);
}

float AnalogInput::readTemperature()
{
    if (!initialized_) {
        return 0.0f;
    }

    // Configure temperature channel if not already done
    configureTemperatureChannel();

    // Read raw value
    uint16_t raw = read(0, 100);

    // Temperature calculation varies by platform
#if defined(STM32H7)
    // H7 datasheet formula
    const float V25 = 0.76f;          // Voltage at 25°C
    const float Avg_Slope = 0.0025f;  // mV/°C
    float voltage = rawToVoltage(raw);
    float temp = ((V25 - voltage) / Avg_Slope) + 25.0f;
    return temp;

#elif defined(STM32G0)
    // G0 uses factory calibration values
    const uint16_t *TS_CAL1 = (uint16_t*)0x1FFF75A8;  // 30°C calibration
    const uint16_t *TS_CAL2 = (uint16_t*)0x1FFF75CA;  // 130°C calibration
    float temp = 30.0f + (130.0f - 30.0f) * (raw - *TS_CAL1) / (*TS_CAL2 - *TS_CAL1);
    return temp;

#elif defined(STM32G4)
    // G4 uses factory calibration
    const uint16_t *TS_CAL1 = (uint16_t*)0x1FFF75A8;
    const uint16_t *TS_CAL2 = (uint16_t*)0x1FFF75CA;
    float temp = 30.0f + (130.0f - 30.0f) * (raw - *TS_CAL1) / (*TS_CAL2 - *TS_CAL1);
    return temp;

#elif defined(STM32F4)
    // F4 formula
    const float V25 = 0.76f;
    const float Avg_Slope = 0.0025f;
    float voltage = rawToVoltage(raw);
    float temp = ((V25 - voltage) / Avg_Slope) + 25.0f;
    return temp;

#else
    return 0.0f;
#endif
}

float AnalogInput::readVRef()
{
    if (!initialized_) {
        return 0.0f;
    }

    configureVRefChannel();
    uint16_t raw = read(0, 100);

    // VRef calculation
#if defined(STM32G0)
    const uint16_t *VREFINT_CAL = (uint16_t*)0x1FFF75AA;
    float vref = 3.0f * (*VREFINT_CAL) / raw;
    return vref;
#elif defined(STM32G4)
    const uint16_t *VREFINT_CAL = (uint16_t*)0x1FFF75AA;
    float vref = 3.0f * (*VREFINT_CAL) / raw;
    return vref;
#else
    // For platforms without calibration, use nominal VRef
    return rawToVoltage(raw);
#endif
}

bool AnalogInput::startContinuous()
{
    if (!initialized_ || !continuous_mode_) {
        return false;
    }

    // Select current channel
    if (current_channel_ != 0) {
        selectChannel(current_channel_);
    }

    // Start continuous conversion
    if (HAL_ADC_Start_IT(&adc_handle_) != HAL_OK) {
        return false;
    }

    return true;
}

void AnalogInput::stop()
{
    if (!initialized_) {
        return;
    }

    if (continuous_mode_) {
        HAL_ADC_Stop_IT(&adc_handle_);
    } else {
        HAL_ADC_Stop(&adc_handle_);
    }
}

float AnalogInput::getVoltage() const
{
    return rawToVoltage(latest_value_);
}

float AnalogInput::rawToVoltage(uint16_t raw_value) const
{
    uint16_t max_value = getMaxValue();
    return (float)raw_value * vref_voltage_ / (float)max_value;
}

uint16_t AnalogInput::getMaxValue() const
{
    switch (resolution_) {
        case ADC_RESOLUTION_12B: return 4095;
        case ADC_RESOLUTION_10B: return 1023;
        case ADC_RESOLUTION_8B:  return 255;
#if defined(STM32H7) || defined(STM32H5)
        case ADC_RESOLUTION_16B: return 65535;
        case ADC_RESOLUTION_14B: return 16383;
#endif
        default: return 4095;
    }
}

void AnalogInput::handleInterrupt()
{
    // Store latest value for continuous mode
    latest_value_ = (uint16_t)HAL_ADC_GetValue(&adc_handle_);
}

void AnalogInput::configureGPIOAnalog(GPIO_TypeDef* port, uint16_t pin)
{
    GPIO gpio(port, pin);
    gpio.mode(GPIO_MODE_ANALOG, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
}

bool AnalogInput::selectChannel(uint32_t channel, uint32_t sampling_time)
{
    ADC_ChannelConfTypeDef config = {0};

    config.Channel = getADCChannel(channel);
    config.Rank = ADC_REGULAR_RANK_1;

#if defined(STM32H7) || defined(STM32H5)
    config.SamplingTime = (sampling_time == 0) ? ADC_SAMPLETIME_8CYCLES_5 : sampling_time;
    config.SingleDiff = ADC_SINGLE_ENDED;
    config.OffsetNumber = ADC_OFFSET_NONE;
    config.Offset = 0;

#elif defined(STM32G0)
    config.SamplingTime = (sampling_time == 0) ? ADC_SAMPLETIME_1CYCLE_5 : sampling_time;

#elif defined(STM32G4)
    config.SamplingTime = (sampling_time == 0) ? ADC_SAMPLETIME_2CYCLES_5 : sampling_time;
    config.SingleDiff = ADC_SINGLE_ENDED;
    config.OffsetNumber = ADC_OFFSET_NONE;
    config.Offset = 0;

#elif defined(STM32F4)
    config.SamplingTime = (sampling_time == 0) ? ADC_SAMPLETIME_3CYCLES : sampling_time;
    config.Offset = 0;

#endif

    if (HAL_ADC_ConfigChannel(&adc_handle_, &config) != HAL_OK) {
        return false;
    }

    current_channel_ = channel;
    return true;
}

uint32_t AnalogInput::getADCChannel(uint32_t channel)
{
    // Map channel number to HAL channel constant
    switch (channel) {
        case 0:  return ADC_CHANNEL_0;
        case 1:  return ADC_CHANNEL_1;
        case 2:  return ADC_CHANNEL_2;
        case 3:  return ADC_CHANNEL_3;
        case 4:  return ADC_CHANNEL_4;
        case 5:  return ADC_CHANNEL_5;
        case 6:  return ADC_CHANNEL_6;
        case 7:  return ADC_CHANNEL_7;
        case 8:  return ADC_CHANNEL_8;
        case 9:  return ADC_CHANNEL_9;
        case 10: return ADC_CHANNEL_10;
        case 11: return ADC_CHANNEL_11;
        case 12: return ADC_CHANNEL_12;
        case 13: return ADC_CHANNEL_13;
        case 14: return ADC_CHANNEL_14;
        case 15: return ADC_CHANNEL_15;
        case 16: return ADC_CHANNEL_16;
        case 17: return ADC_CHANNEL_17;
#ifdef ADC_CHANNEL_18
        case 18: return ADC_CHANNEL_18;
#endif
        default: return ADC_CHANNEL_0;
    }
}

// Helper Functions Implementation

uint32_t getADCClock(ADC_TypeDef* adc)
{
    // Return approximate ADC clock based on system clock
    // This is platform-specific and simplified
    (void)adc;

#if defined(STM32H7)
    return HAL_RCC_GetSysClockFreq() / 2;
#elif defined(STM32G0)
    return HAL_RCC_GetSysClockFreq();
#elif defined(STM32G4)
    return HAL_RCC_GetSysClockFreq() / 2;
#elif defined(STM32F4)
    return HAL_RCC_GetPCLK2Freq() / 2;
#elif defined(STM32H5)
    return HAL_RCC_GetSysClockFreq() / 2;
#else
    return 0;
#endif
}

// HAL Callbacks

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    // Find which ADC instance this is
    for (int i = 0; i < 5; i++) {
        if (adc_instances[i] != nullptr) {
            if (adc_instances[i]->getHandle() == hadc) {
                adc_instances[i]->handleInterrupt();
                break;
            }
        }
    }
}
