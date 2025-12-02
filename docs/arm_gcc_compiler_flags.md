# ARM GCC Compiler Flags Reference

Comprehensive guide to ARM GCC compiler flags for embedded development, with focus on STM32 microcontrollers.

## Table of Contents
- [ARM Architecture Flags](#arm-architecture-flags)
- [Floating-Point Flags](#floating-point-flags)
- [Optimization Flags](#optimization-flags)
- [Code Generation Flags](#code-generation-flags)
- [Linking Flags](#linking-flags)
- [Preprocessor Flags](#preprocessor-flags)
- [Warning Flags](#warning-flags)
- [STM32-Specific Configurations](#stm32-specific-configurations)

---

## ARM Architecture Flags

### `-mcpu=<name>`
Specifies the target ARM processor. This is the **most important flag** as it determines instruction set and features.

**Common STM32 Values:**
```bash
-mcpu=cortex-m0       # STM32F0, STM32G0 (ARMv6-M)
-mcpu=cortex-m0plus   # STM32L0 (ARMv6-M with some extensions)
-mcpu=cortex-m3       # STM32F1, STM32F2, STM32L1 (ARMv7-M)
-mcpu=cortex-m4       # STM32F3, STM32F4, STM32L4 (ARMv7E-M with DSP)
-mcpu=cortex-m7       # STM32F7, STM32H7 (ARMv7E-M with DSP + cache)
-mcpu=cortex-m33      # STM32L5, STM32U5 (ARMv8-M with TrustZone)
```

### `-march=<arch>`
Specifies ARM architecture version. Usually **not needed** if `-mcpu` is specified.

```bash
-march=armv6-m        # Cortex-M0/M0+
-march=armv7-m        # Cortex-M3
-march=armv7e-m       # Cortex-M4/M7
-march=armv8-m.main   # Cortex-M33
```

### `-mthumb`
Generate Thumb instruction set code. **Required for all Cortex-M processors**.

```bash
-mthumb               # Use 16/32-bit Thumb-2 instructions
```

**Note:** All modern Cortex-M processors only support Thumb mode. Never use ARM mode.

### `-mabi=<name>`
Application Binary Interface. Usually auto-selected by other flags.

```bash
-mabi=aapcs           # ARM Architecture Procedure Call Standard
```

---

## Floating-Point Flags

### `-mfloat-abi=<type>`
Specifies how floating-point values are passed between functions.

```bash
-mfloat-abi=soft      # No FPU, software emulation
-mfloat-abi=softfp    # FPU present but uses soft ABI (compatibility)
-mfloat-abi=hard      # FPU present, uses FPU registers for parameters
```

**Guidelines:**
- **Cortex-M0/M0+/M3**: Use `soft` (no FPU)
- **Cortex-M4F/M7F**: Use `hard` (best performance)
- **Cortex-M4 (no F)**: Use `soft`

### `-mfpu=<fpu>`
Specifies the FPU type. **Only use with `-mfloat-abi=softfp` or `hard`**.

```bash
# Single-precision only
-mfpu=fpv4-sp-d16     # Cortex-M4F (FPv4, single-precision)

# Double-precision
-mfpu=fpv5-d16        # Cortex-M7 (FPv5, single + double precision)
-mfpu=fpv5-sp-d16     # Cortex-M7 single-precision only
```

**Common Combinations:**
```bash
# STM32F4 with FPU
-mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16

# STM32H7 with FPU
-mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16

# STM32F1 (no FPU)
-mcpu=cortex-m3 -mfloat-abi=soft
```

---

## Optimization Flags

### `-O<level>`
Optimization level. Affects code size, speed, and debugging.

```bash
-O0                   # No optimization (debugging)
-O1                   # Basic optimization
-O2                   # Moderate optimization (recommended for production)
-O3                   # Aggressive optimization (larger code)
-Os                   # Optimize for size
-Og                   # Optimize for debugging experience
```

**Recommendations:**
- **Development**: `-O0` or `-Og` (easier debugging)
- **Production**: `-O2` or `-Os` (best balance)
- **Performance-critical**: `-O3` (larger code, faster)

### Link-Time Optimization
```bash
-flto                 # Enable link-time optimization
-flto=auto            # Parallel LTO
```

---

## Code Generation Flags

### Function/Data Sections
**Critical for embedded systems** to reduce binary size.

```bash
-ffunction-sections   # Place each function in its own section
-fdata-sections       # Place each data item in its own section
```

**Must combine with linker flag** `-Wl,--gc-sections` to remove unused sections.

### Stack Protection
```bash
-fstack-protector     # Stack overflow protection (adds overhead)
-fno-stack-protector  # Disable stack protection (embedded default)
```

### Exception Handling (C++)
```bash
-fno-exceptions       # Disable C++ exceptions (saves ~20KB)
-fno-rtti            # Disable Run-Time Type Information (saves ~5KB)
-fno-use-cxa-atexit  # Disable static destructor registration
```

**Recommended for embedded C++:** Always use `-fno-exceptions -fno-rtti`

### Inline Functions
```bash
-finline-functions    # Inline functions aggressively
-fno-inline           # Never inline functions (debugging)
```

### Short Enums
```bash
-fshort-enums         # Use smallest integer type for enums
```

---

## Linking Flags

### Linker Script
```bash
-T<script>            # Specify custom linker script
-Tlinker.ld           # Example
```

### Linker Options (via `-Wl,`)
Pass options directly to the linker:

```bash
-Wl,--gc-sections                 # Remove unused sections (CRITICAL)
-Wl,-Map=output.map              # Generate memory map file
-Wl,--print-memory-usage         # Show memory usage summary
-Wl,--cref                       # Cross-reference table
-Wl,--no-warn-mismatch           # Suppress architecture mismatch warnings
```

### Specs Files (Newlib variants)
```bash
-specs=nano.specs     # Use newlib-nano (smaller C library)
-specs=nosys.specs    # No system calls (bare metal)
-specs=rdimon.specs   # Semihosting support (debugging)
```

**Embedded standard:**
```bash
-specs=nano.specs -specs=nosys.specs
```

### Libraries
```bash
-lc                   # C standard library
-lm                   # Math library
-lnosys              # Dummy syscalls stubs
-lstdc++             # C++ standard library (if using C++)
```

---

## Preprocessor Flags

### Defines
```bash
-D<name>              # Define macro
-D<name>=<value>      # Define macro with value
```

**Common STM32 defines:**
```bash
-DSTM32F407xx         # Specific MCU model
-DSTM32F4             # MCU family
-DUSE_HAL_DRIVER      # Enable STM32 HAL
-DHSE_VALUE=8000000   # External crystal frequency
-DDEBUG               # Enable debug code
```

### Include Paths
```bash
-I<path>              # Add include directory
-isystem <path>       # Add system include directory (suppresses warnings)
-include <file>       # Force include file before compilation
```

---

## Warning Flags

### Basic Warnings
```bash
-Wall                 # Enable common warnings
-Wextra              # Enable extra warnings
-Werror              # Treat warnings as errors
-pedantic            # Strict ISO C/C++ compliance
```

### Specific Warnings
```bash
-Wunused             # Warn about unused variables
-Wcast-align         # Warn about pointer alignment issues
-Wconversion         # Warn about implicit type conversions
-Wformat=2           # Extra printf/scanf format checking
-Wshadow             # Warn about variable shadowing
-Wstrict-prototypes  # C only: require function prototypes
```

### Disable Warnings
```bash
-Wno-unused-parameter       # Disable unused parameter warnings
-Wno-write-strings          # Disable string literal warnings
```

---

## STM32-Specific Configurations

### STM32F0 Series (Cortex-M0)
```bash
arm-none-eabi-gcc \
  -mcpu=cortex-m0 \
  -mthumb \
  -mfloat-abi=soft \
  -O2 \
  -Wall \
  -ffunction-sections \
  -fdata-sections \
  -DSTM32F030x6 \
  -DUSE_HAL_DRIVER
```

### STM32F1 Series (Cortex-M3)
```bash
arm-none-eabi-gcc \
  -mcpu=cortex-m3 \
  -mthumb \
  -mfloat-abi=soft \
  -O2 \
  -Wall \
  -ffunction-sections \
  -fdata-sections \
  -DSTM32F103xB \
  -DUSE_HAL_DRIVER
```

### STM32F4 Series (Cortex-M4F)
```bash
arm-none-eabi-gcc \
  -mcpu=cortex-m4 \
  -mthumb \
  -mfloat-abi=hard \
  -mfpu=fpv4-sp-d16 \
  -O2 \
  -Wall \
  -ffunction-sections \
  -fdata-sections \
  -fno-exceptions \
  -fno-rtti \
  -DSTM32F407xx \
  -DUSE_HAL_DRIVER
```

### STM32F7 Series (Cortex-M7)
```bash
arm-none-eabi-gcc \
  -mcpu=cortex-m7 \
  -mthumb \
  -mfloat-abi=hard \
  -mfpu=fpv5-d16 \
  -O2 \
  -Wall \
  -ffunction-sections \
  -fdata-sections \
  -fno-exceptions \
  -fno-rtti \
  -DSTM32F746xx \
  -DUSE_HAL_DRIVER
```

### STM32H7 Series (Cortex-M7, high performance)
```bash
arm-none-eabi-gcc \
  -mcpu=cortex-m7 \
  -mthumb \
  -mfloat-abi=hard \
  -mfpu=fpv5-d16 \
  -O2 \
  -Wall \
  -ffunction-sections \
  -fdata-sections \
  -fno-exceptions \
  -fno-rtti \
  -DSTM32H723xx \
  -DUSE_HAL_DRIVER
```

### STM32L4 Series (Cortex-M4F, low power)
```bash
arm-none-eabi-gcc \
  -mcpu=cortex-m4 \
  -mthumb \
  -mfloat-abi=hard \
  -mfpu=fpv4-sp-d16 \
  -O2 \
  -Wall \
  -ffunction-sections \
  -fdata-sections \
  -DSTM32L476xx \
  -DUSE_HAL_DRIVER
```

---

## Complete Build Example

### Compilation Step
```bash
arm-none-eabi-gcc \
  -c main.c \
  -o main.o \
  -mcpu=cortex-m7 \
  -mthumb \
  -mfloat-abi=hard \
  -mfpu=fpv5-d16 \
  -O2 \
  -Wall \
  -Wextra \
  -ffunction-sections \
  -fdata-sections \
  -fno-exceptions \
  -fno-rtti \
  -DSTM32H723xx \
  -DUSE_HAL_DRIVER \
  -I./include \
  -I./CMSIS/Include \
  -I./STM32H7xx_HAL_Driver/Inc
```

### Linking Step
```bash
arm-none-eabi-gcc \
  main.o system.o startup.o \
  -o firmware.elf \
  -mcpu=cortex-m7 \
  -mthumb \
  -mfloat-abi=hard \
  -mfpu=fpv5-d16 \
  -T STM32H723VG_FLASH.ld \
  -Wl,--gc-sections \
  -Wl,-Map=firmware.map \
  -Wl,--print-memory-usage \
  -specs=nano.specs \
  -specs=nosys.specs \
  -lc \
  -lm \
  -lnosys
```

### Binary Creation
```bash
arm-none-eabi-objcopy \
  -O binary \
  firmware.elf \
  firmware.bin
```

---

## Quick Reference Table

| Flag | Purpose | Common Values |
|------|---------|---------------|
| `-mcpu` | Target processor | `cortex-m0`, `cortex-m3`, `cortex-m4`, `cortex-m7` |
| `-mthumb` | Use Thumb instructions | Always on for Cortex-M |
| `-mfloat-abi` | FPU calling convention | `soft`, `hard` |
| `-mfpu` | FPU type | `fpv4-sp-d16`, `fpv5-d16` |
| `-O<n>` | Optimization level | `0`, `2`, `s`, `g` |
| `-ffunction-sections` | Split functions | Always on |
| `-fdata-sections` | Split data | Always on |
| `-Wl,--gc-sections` | Remove unused code | Always on |
| `-specs=nano.specs` | Small C library | Embedded standard |
| `-T<script>` | Linker script | MCU-specific |

---

## Memory Optimization Tips

1. **Always use:**
   ```bash
   -ffunction-sections -fdata-sections -Wl,--gc-sections
   ```
   Can save 20-50% of flash usage!

2. **For size-constrained devices:**
   ```bash
   -Os -flto
   ```

3. **Disable C++ features if not needed:**
   ```bash
   -fno-exceptions -fno-rtti -fno-use-cxa-atexit
   ```

4. **Use newlib-nano:**
   ```bash
   -specs=nano.specs
   ```
   Saves ~10KB compared to standard newlib.

5. **Strip symbols from release builds:**
   ```bash
   arm-none-eabi-strip firmware.elf
   ```

---

## Debugging Flags

```bash
-g                    # Generate debug information
-g3                   # Maximum debug info (includes macros)
-ggdb                # GDB-specific debug format
-gdwarf-4            # DWARF-4 debug format
-Og                   # Optimize for debugging
```

**Debug build example:**
```bash
-O0 -g3 -ggdb -Wall -Wextra
```

---

## Common Pitfalls

### ❌ Forgetting `-mthumb`
```bash
# WRONG - will generate ARM code (incompatible with Cortex-M)
arm-none-eabi-gcc -mcpu=cortex-m4 main.c
```

### ❌ FPU mismatch
```bash
# WRONG - using hard float ABI without specifying FPU
arm-none-eabi-gcc -mcpu=cortex-m4 -mfloat-abi=hard main.c
# Missing: -mfpu=fpv4-sp-d16
```

### ❌ Missing linker garbage collection
```bash
# WRONG - compiling with sections but not removing unused
arm-none-eabi-gcc -ffunction-sections main.o -o firmware.elf
# Missing: -Wl,--gc-sections
```

### ❌ Inconsistent flags between compilation and linking
```bash
# WRONG - different CPU flags
arm-none-eabi-gcc -c -mcpu=cortex-m4 main.c      # Compile
arm-none-eabi-gcc -mcpu=cortex-m7 main.o -o fw   # Link (WRONG!)
```

---

## Further Reading

- [GCC ARM Options](https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html)
- [ARM Compiler Documentation](https://developer.arm.com/documentation/)
- [Newlib Documentation](https://sourceware.org/newlib/)
- [STM32 Naming Conventions](https://www.st.com/resource/en/application_note/an4838-introduction-to-stm32-microcontrollers-security-stmicroelectronics.pdf)
