/**
 * @file main.cpp
 * @brief STM32G0 ADC (Analog-to-Digital Converter) demonstration
 *
 * This example demonstrates:
 * - Single-channel ADC reading
 * - Multi-channel ADC reading
 * - Voltage conversion
 * - Internal temperature sensor reading
 * - Internal voltage reference reading
 *
 * Hardware Connections:
 * - PA1 (Bottom Connector Pin 15) - ADC Channel 1 - Analog input 1
 * - PA2 (Bottom Connector Pin 13) - ADC Channel 2 - Analog input 2
 * - PD2 (Bottom Connector Pin 4)  - Status LED
 *
 * Test Setup:
 * - Connect a potentiometer or voltage source (0-3.3V) to PA1
 * - Connect another voltage source (0-3.3V) to PA2
 * - Observe readings via debugging or UART
 *
 * STM32G0B1 ADC Channel Mapping:
 * - PA0 = ADC_IN0
 * - PA1 = ADC_IN1
 * - PA2 = ADC_IN2
 * - PA3 = ADC_IN3
 * - PA4 = ADC_IN4
 * - PA5 = ADC_IN5
 * - PA6 = ADC_IN6
 * - PA7 = ADC_IN7
 */
#include "lumos.h"
#include "lumos_micro_brain.h"
#include "stm32g0xx_hal_adc.h"  // Force ADC HAL module detection
#include "adc.h"
#include "gpio.h"
#include "sys.h"

// ADC instance (AnalogInput class to avoid macro conflict)
AnalogInput adc(ADC1);

// Status LED for visual feedback
GPIO led(GPIOD, GPIO_PIN_2);

// Variables to store readings
uint16_t raw_value_ch1 = 0;
uint16_t raw_value_ch2 = 0;
float voltage_ch1 = 0.0f;
float voltage_ch2 = 0.0f;
float temperature = 0.0f;
float vref = 0.0f;

/**
 * @brief Setup function - called once at startup
 */
void setup(void)
{
    // Initialize timing
    InitMicrosecondTiming();

    // Configure status LED
    led.mode(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    led.low();

    // ===== Initialize ADC =====
    // 12-bit resolution, single conversion mode, 3.3V reference
    if (!adc.init(ADC_RESOLUTION_12B, false, 3.3f)) {
        // Error initializing ADC - flash LED rapidly
        while (1) {
            led.toggle();
            DelayMs(100);
        }
    }

    // Calibrate ADC for better accuracy
    adc.calibrate();

    // ===== Configure ADC Channels =====

    // Channel 1: PA1 (analog input)
    // PA1 is ADC_IN1 on STM32G0
    adc.configureChannel(1, GPIOA, GPIO_PIN_1);  // Uses default sampling time

    // Channel 2: PA2 (analog input)
    // PA2 is ADC_IN2 on STM32G0
    adc.configureChannel(2, GPIOA, GPIO_PIN_2);  // Uses default sampling time

    // Configuration complete - flash LED slowly 3 times
    for (int i = 0; i < 6; i++) {
        led.toggle();
        DelayMs(300);
    }
}

/**
 * @brief Loop function - called repeatedly
 */
void loop(void)
{
    led.high();  // LED on during measurements

    // ===== Test 1: Read Channel 1 (PA1) =====
    raw_value_ch1 = adc.read(1);              // Read raw 12-bit value (0-4095)
    voltage_ch1 = adc.readVoltage(1);         // Read and convert to voltage

    DelayMs(10);

    // ===== Test 2: Read Channel 2 (PA2) =====
    raw_value_ch2 = adc.read(2);
    voltage_ch2 = adc.readVoltage(2);

    DelayMs(10);

    // ===== Test 3: Read Internal Temperature Sensor =====
    temperature = adc.readTemperature();

    DelayMs(10);

    // ===== Test 4: Read Internal Voltage Reference =====
    vref = adc.readVRef();

    led.low();  // LED off after measurements

    // Wait before next reading cycle
    DelayMs(500);

    // Example results (values depend on connected voltages):
    // raw_value_ch1: 0-4095 (12-bit)
    // voltage_ch1:   0.0-3.3V
    // raw_value_ch2: 0-4095
    // voltage_ch2:   0.0-3.3V
    // temperature:   ~25°C (room temperature)
    // vref:          ~3.0V (internal reference)

    // In a real application, you would:
    // - Send these values over UART/USB
    // - Display on LCD/OLED
    // - Use for control algorithms
    // - Log to SD card
    // - Trigger actions based on thresholds

    // Example threshold detection:
    if (voltage_ch1 > 2.0f) {
        // High voltage detected on channel 1
        // Take some action...
    }

    if (temperature > 50.0f) {
        // Temperature warning
        // Activate cooling or reduce power...
    }
}

/**
 * Example Usage Patterns:
 *
 * 1. Reading a potentiometer:
 *    uint16_t pot_raw = adc.read(1);
 *    float pot_voltage = adc.readVoltage(1);
 *    uint8_t pot_percent = (pot_raw * 100) / 4095;
 *
 * 2. Battery voltage monitoring:
 *    float battery_voltage = adc.readVoltage(2) * 2.0f;  // If using voltage divider
 *    if (battery_voltage < 3.0f) {
 *        // Low battery warning
 *    }
 *
 * 3. Light sensor reading:
 *    uint16_t light_level = adc.read(1);
 *    if (light_level < 500) {
 *        // Dark environment - turn on lights
 *    }
 *
 * 4. Temperature monitoring:
 *    float temp = adc.readTemperature();
 *    if (temp > 60.0f) {
 *        // Overheat protection
 *    }
 *
 * 5. Analog sensor with calibration:
 *    uint16_t raw = adc.read(1);
 *    float voltage = adc.rawToVoltage(raw);
 *    float sensor_value = (voltage - 0.5f) / 0.01f;  // Example: 10mV/unit sensor
 */
