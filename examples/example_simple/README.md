# STM32H7 Simple Example

This is the most basic example project for the Lumos build tool. It demonstrates the minimum code structure using the Arduino-style `setup()` and `loop()` pattern.

## What It Does

This example:
- Implements `setup()` - called once after board initialization
- Implements `loop()` - called repeatedly in an infinite loop
- Increments a counter with delays

**No peripherals are used** - no UART, no GPIO, no LEDs, nothing. Just the core CPU running.

The board support files (from `/src/boards/lumos_brain/`) automatically handle:
- HAL initialization
- System clock configuration @ 550 MHz
- MPU configuration
- USB and SD card initialization

## Why This Example?

This example is useful for:
- **Testing the build system** - Verify that Lumos can build a minimal project
- **Learning the basics** - Understand the minimum code required for STM32
- **Starting point** - Use as a template for your own projects
- **Debugging** - Simplest possible code to verify hardware is working

## Project Configuration

The `project.yaml` is minimal:

```yaml
sources:
  - main.cpp
board: LumosBrain
```

No `hal_modules` are specified because we don't use any peripherals. The build tool automatically includes the core HAL drivers (RCC, GPIO, Cortex, etc.).

## Building

From the `example_simple` directory:

```bash
lumos build
```

### Build Output

Creates:
- `build/firmware.elf` - Executable with debug symbols
- `build/firmware.bin` - Raw binary for flashing
- `build/firmware.map` - Memory map

Expected binary size: ~10-11 KB (minimal footprint)

## Hardware Requirements

- **Any STM32H7 board** with a 25 MHz external crystal (HSE)
- **Debugger** (ST-Link V2/V3) for flashing and debugging

## Flashing

Flash the firmware using your preferred tool:

### Using ST-Link
```bash
st-flash write build/firmware.bin 0x8000000
```

### Using OpenOCD
```bash
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
  -c "program build/firmware.elf verify reset exit"
```

## What Happens After Flashing?

The microcontroller will:
1. Execute the reset handler (from startup code)
2. Initialize HAL library
3. Configure system clock to 550 MHz
4. Enter the main loop
5. Increment a counter forever

**You won't see any visible output** because no peripherals are initialized. However, the CPU is running and executing code.

## Debugging

To verify the code is running, you can:

1. **Use a debugger (GDB)**:
   ```bash
   openocd -f interface/stlink.cfg -f target/stm32h7x.cfg
   ```

   In another terminal:
   ```bash
   arm-none-eabi-gdb build/firmware.elf
   (gdb) target remote localhost:3333
   (gdb) monitor reset halt
   (gdb) break main
   (gdb) continue
   (gdb) print counter
   ```

2. **Use STM32CubeIDE** - Import the ELF file and set breakpoints

3. **Measure power consumption** - Running at 550 MHz should draw more power than in stop mode

## Code Structure

### main.cpp (User Code)

**setup()**
- Called once after board initialization
- Use this to initialize your application state
- Configure peripherals, set up variables, etc.

**loop()**
- Called repeatedly in an infinite loop
- Contains your main application logic
- Runs at full CPU speed (no built-in delays)

**SimpleDelay()**
- Software delay function for demonstration
- Not precise, just for demonstration

### Board Support Files (Automatic)

The build system automatically includes these from `/src/boards/lumos_brain/`:
- **main.c** - Contains the actual `main()` that calls `setup()` once, then `loop()` repeatedly
- **system_stm32h7xx.c** - System and clock initialization
- **stm32h7xx_hal_msp.c** - HAL MSP callbacks
- **stm32h7xx_it.c** - Interrupt handlers
- **syscalls.c** - Newlib syscalls
- **sysmem.c** - Memory management

## User Code Structure

Your code only needs to include `lumos.h` and define two functions:

```cpp
#include "lumos.h"

void setup(void)
{
    // Initialize peripherals
    // Set up GPIO, UART, etc.
}

void loop(void)
{
    // Your application code here
    // This runs repeatedly
}
```

**That's it!** No `extern "C"`, no manual includes - `lumos.h` handles everything.

## Customization

### Change Clock Speed

The clock is configured in the board's `main.c`. To modify:
1. Edit `/src/boards/lumos_brain/main.c`
2. Modify the `SystemClock_Config()` function
3. Rebuild (all projects using LumosBrain will use the new clock)

### Add Peripherals

To use peripherals, see other examples:
- `example_uart` - UART communication
- `example_i2c` - I2C peripheral
- `example_spi` - SPI communication

## Next Steps

1. **Add LED blinking** - Initialize a GPIO pin and toggle it
2. **Add UART output** - See `example_uart` for serial communication
3. **Use the Framework** - See `example_layer` for ApplicationBase usage
4. **Add more sources** - Create additional .cpp files and list them in `project.yaml`

## Technical Details

### Memory Usage
- **Flash**: ~10-11 KB (out of 1 MB available)
- **RAM**: Minimal (just stack)

### Clock Configuration
- **HSE**: 25 MHz (external crystal)
- **PLL VCO**: 1100 MHz
- **SYSCLK**: 550 MHz
- **HCLK**: 275 MHz

### Included HAL Drivers

Even without specifying `hal_modules`, these core drivers are always included:
- `stm32h7xx_hal.c` - Core HAL functions
- `stm32h7xx_hal_cortex.c` - Cortex-M7 functions
- `stm32h7xx_hal_rcc.c` - Clock control
- `stm32h7xx_hal_gpio.c` - GPIO (for clock pins)
- `stm32h7xx_hal_pwr.c` - Power management
- `stm32h7xx_hal_dma.c` - DMA support

## License

This example is provided as-is for demonstration purposes.
