### Lumos Brain
 * STM32H723VGT6
 * CPU Speed: 550 MHz
 * Memory (Kbytes)
  - Flash: 1024
  - RAM: 564
   - SRAM mapped onto AXI bus: 128
   - SRAM1 (D2 domain): 16
   - SRAM2 (D2 domain): 16
   - SRAM3 (D3 domain): 16
  - RAM shared between ITCM and AXI: 192
  - ITCM RAM (instruction): 64
  - DTCM RAM (data): 128
  - Backup SRAM: 4
 * USB
  - PA12: USB DP
  - PA11: USB DN
 * USB Serial
  - PC7: CP_TX, so should be STM32 RX
  - PC6: CP_RX, so should be STM32 TX
  - CP DTR to STM32 NRST
 * SD Card (SDMMC1)
  - PD2: CMD
  - PC12: CK
  - PC11: D3
  - PC10: D2
  - PC9: D1
  - PC8: D0
### Mini Brain
 * STM32G0B1CBT6
 * CPU Speed: 64 MHz
 * Memory (Kbytes)
  - Flash: 128
  - RAM: 128 (parity-protected), 144 (non parity-protected)
 * CAN JST-SH connector
  - 1: CAN_HIGH
  - 2: CAN_LOW
  - 3: GND
  - 4: 5V
 * Extra JST-SH connector
  - 1: PB8 (SCL)
  - 2: PB9 (SDA)
  - 3: 3.3V
  - 4: GND
 * CAN1
  - PA12 (PA10): CAN TX
  - PA11 (PA9): CAN RX
 * Bottom connector:
  - 9: PA6 (I2C3 SDA)
  - 7: PA7 (I2C3 SCL)
  - 5: GND
  - 3: PB0 (UART5 TX)
  - 1: PB1 (UART5 RX)
  - 2: 5V
  - 4: PB3 (SPI3 SCK)
  - 6: PB4 (SPI3 MISO)
  - 8: PB5 (SPI3 MOSI)
  - 10: 3.3V

### Mini ESC
 * STM32G431CBU3
 * CPU Speed: 170 MHz
 * Memory (Kbytes)
  - Flash: 128
  - RAM: 32
 * 3 phase interter pins:
  - Phase A high: PA8 (TIM1 CH1)
  - Phase A low: PC13 (TIM1 CH1N)
  - Phase B high: PA9 (TIM1 CH2)
  - Phase B low: PB0 (TIM1 CH2N)
  - Phase C high: PA10 (TIM1 CH3)
  - Phase C low: PB1 (TIM1 CH3N)
 * SPI3
  - NSS: PA4
  - SCK: PB3
  - MISO: PB4
  - MOSI: PB5
 * I2C1
  - SCL: PA15
  - SDA: PB7
 * CAN1
  - TX: PA12
  - RX: PA11
 * Free pin: PB13
 * Vin voltage sense: PA0 (ADC1_IN2)
  - Voltage divider: 10k to GND, 18k to Vin
 * BEMF Measurement pins (Voltage divided with 2.2k to GND and 10k to signal):
  - Phase A: PB12 (ADC1_IN11)
  - Phase B: PB14 (ADC1_IN5)
  - Phase C: PB15 (ADC2_IN15)
 * Current Sense pin:
  - Phase A: PA7 (ADC2_IN4)
  - Phase B: PB2 (ADC2_IN12)
  - Phase C: PB11 (ADC1_IN14)