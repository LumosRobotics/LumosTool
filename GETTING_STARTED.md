# Getting Started with Lumos

## Overview

Lumos is a CLI tool for building distributed applications on embedded systems. This guide will help you get started.

## Building Lumos

```bash
mkdir build && cd build
cmake ..
make
```

The `lumos` executable will be located at `build/src/applications/lumos/lumos`.

## Installation (Optional)

For convenience, add the lumos executable to your PATH:

```bash
# Add to ~/.bashrc or ~/.zshrc
export PATH="/path/to/LumosTool/build/src/applications/lumos:$PATH"
```

Or create a symbolic link:

```bash
sudo ln -s /path/to/LumosTool/build/src/applications/lumos/lumos /usr/local/bin/lumos
```

## Quick Start

### 1. Create a New Project

```bash
lumos project create MyRobot
cd MyRobot
```

This creates a project with the following structure:
```
MyRobot/
├── lumos.json          # Project configuration
├── CMakeLists.txt      # Build configuration
├── README.md
├── .gitignore
├── src/                # Project source files
├── include/            # Project headers
├── interfaces/         # IDL interface definitions
├── apps/               # Applications directory
└── build/              # Build directory
```

### 2. Create an Application

```bash
lumos app create MotorController
```

This creates an application with:
- Source files in `apps/MotorController/src/`
- Header files in `apps/MotorController/include/`
- Application configuration in `apps/MotorController/app.json`
- CMake configuration in `apps/MotorController/CMakeLists.txt`

### 3. List Applications

```bash
lumos app list
```

Output:
```
Applications in project 'MyRobot':

Name                  Target         Rate (Hz)  Priority
------------------------------------------------------------
MotorController       host           10         5
```

### 4. Implement Your Application

Edit `apps/MotorController/src/MotorController.cpp`:

```cpp
#include "MotorController.h"
#include <iostream>

namespace MotorController {

MotorControllerApp::MotorControllerApp() {
    // Constructor
}

MotorControllerApp::~MotorControllerApp() {
    // Destructor
}

void MotorControllerApp::Init() {
    std::cout << "MotorController initializing..." << std::endl;
    // Initialize motor hardware, setup pins, etc.
}

void MotorControllerApp::Step() {
    // This runs at the configured rate (default: 10 Hz)
    // Add your control loop here
}

void MotorControllerApp::DeInit() {
    std::cout << "MotorController shutting down..." << std::endl;
    // Cleanup: stop motors, release resources
}

} // namespace MotorController
```

### 5. Build the Project

```bash
lumos project build
```

Currently, this creates the build directory. Full build integration is coming in Phase 2.

For now, you can build manually:
```bash
cd build
cmake ..
make
```

## Project Configuration

The `lumos.json` file contains project configuration:

```json
{
  "project": {
    "name": "MyRobot",
    "version": "1.0.0"
  },
  "applications": [
    {
      "name": "MotorController",
      "target": "host",
      "rate_hz": 10,
      "priority": 5
    }
  ],
  "interfaces": [],
  "transports": []
}
```

### Application Configuration

Each application has an `app.json` configuration:

```json
{
  "name": "MotorController",
  "type": "application",
  "provides": [],
  "requires": [],
  "modules": ["logging"],
  "memory": {
    "stack_size": 4096,
    "heap_size": 8192
  }
}
```

## Available Commands

### Project Commands

```bash
lumos project create <name>           # Create new project
lumos project build [--target <mcu>]  # Build project
lumos project clean                    # Clean build artifacts
lumos project list                     # List projects in directory
```

### Application Commands

```bash
lumos app create <name>                          # Create new application
lumos app create <name> --target stm32f407      # Create app for specific target
lumos app create <name> --rate 100              # Create app with 100Hz rate
lumos app list                                   # List applications
lumos app remove <name>                         # Remove application
```

### Help Commands

```bash
lumos --help                # Show general help
lumos --version             # Show version
lumos project --help        # Show project command help
lumos app --help            # Show app command help
```

## Example Workflow

Here's a complete example of creating a multi-application robot project:

```bash
# 1. Create the project
lumos project create MyRobot
cd MyRobot

# 2. Create applications
lumos app create MotorController --rate 100
lumos app create SensorReader --rate 10
lumos app create MainController --rate 50

# 3. View all applications
lumos app list

# 4. Implement application logic
# Edit files in apps/*/src/*.cpp

# 5. Build (Phase 2 feature - currently manual)
cd build
cmake ..
make
```

## What's Next?

**Phase 1 (Current)** - Foundation:
- ✅ Project creation and management
- ✅ Application scaffolding
- ✅ Configuration handling
- ⏳ Basic build system integration (partial)

**Phase 2** - Framework Core:
- Application base class with lifecycle management
- Local scheduler for rate-based execution
- Message passing between applications
- Local transport (same MCU communication)

**Phase 3** - IDL Compiler:
- Interface Definition Language parser
- Code generation for messages and services
- Type-safe communication APIs

**Phase 4** - Distributed Communication:
- UART/SPI transport layers
- Location-transparent message routing
- Remote procedure calls

**Phase 5** - Advanced Features:
- Multiple transport types (CAN, etc.)
- Real-time scheduling
- Debugging tools

**Phase 6** - Developer Experience:
- Comprehensive documentation
- Example projects
- Testing infrastructure

## Troubleshooting

### Build Errors

If you encounter build errors, ensure you have:
- CMake 3.14 or higher
- C++17 compatible compiler
- nlohmann/json library (included in third_party)

### Path Issues

If `lumos` command is not found:
```bash
# Use full path
/path/to/LumosTool/build/src/applications/lumos/lumos --version

# Or add to PATH (see Installation section)
```

## Getting Help

- Check the [Architecture Document](ARCHITECTURE.md) for system design
- Run `lumos --help` for command reference
- Run `lumos <command> --help` for specific command help

## Contributing

This is the foundation of the Lumos ecosystem. Future phases will add:
- Complete build system integration
- Framework runtime
- IDL compiler
- Distributed communication
- Hardware target support

Stay tuned for updates!
