# example_new

A Lumos project for LumosBrain.

## Configuration

- **Board**: LumosBrain
- **Language**: C++

## Building

```bash
lumos build
```

## Flashing

```bash
st-flash write build/firmware.bin 0x8000000
```

## Project Structure

- `main.cpp` - Main application code
- `project.yaml` - Project configuration
- `build/` - Build output directory (generated)
