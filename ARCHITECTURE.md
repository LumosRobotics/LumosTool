# Lumos Ecosystem Architecture

## Overview

Lumos is a comprehensive CLI-based development ecosystem for building, configuring, and deploying distributed applications on embedded systems. It provides an abstraction layer that allows applications to communicate seamlessly whether they run on the same microcontroller or across different devices, synchronously or asynchronously.

## Core Principles

1. **Location Transparency**: Applications don't need to know if peer applications are local or remote
2. **Communication Abstraction**: IDL-based communication layer handles inter-application messaging
3. **Scheduling Flexibility**: Applications can run at different rates, synchronously or asynchronously
4. **Build Orchestration**: Single tool to manage the entire lifecycle from creation to deployment
5. **Embedded-First**: Designed specifically for resource-constrained embedded systems

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         Lumos CLI Tool                          │
│  (Command Line Interface - src/applications/lumos)              │
├─────────────────────────────────────────────────────────────────┤
│  Commands:                                                      │
│  - project create/build/run/debug                              │
│  - app create/add/remove                                        │
│  - interface generate                                           │
│  - deploy configure/flash                                       │
└─────────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   Project    │    │     IDL      │    │    Build     │
│  Management  │    │   Compiler   │    │   System     │
└──────────────┘    └──────────────┘    └──────────────┘
        │                     │                     │
        │                     │                     │
        ▼                     ▼                     ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Lumos Framework                            │
│                  (src/framework)                                │
├─────────────────────────────────────────────────────────────────┤
│  • Application Runtime                                          │
│  • Communication Middleware                                     │
│  • Scheduler                                                    │
│  • Transport Layer Abstraction                                 │
└─────────────────────────────────────────────────────────────────┘
        │
        ├─────────────────┬─────────────────┬─────────────────┐
        ▼                 ▼                 ▼                 ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│ User App 1   │  │ User App 2   │  │ User App 3   │  │ User App N   │
│ (Rate: 10Hz) │  │ (Rate: 1Hz)  │  │ (Rate: 100Hz)│  │ (Async)      │
└──────────────┘  └──────────────┘  └──────────────┘  └──────────────┘
        │                 │                 │                 │
        └─────────────────┴─────────────────┴─────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   Local IPC  │    │  UART/SPI    │    │   CAN Bus    │
│  (Same MCU)  │    │  (Physical)  │    │  (Network)   │
└──────────────┘    └──────────────┘    └──────────────┘
```

## Component Breakdown

### 1. Lumos CLI Tool (`src/applications/lumos`)

The main command-line interface for the entire ecosystem.

**Responsibilities:**
- Project lifecycle management (create, configure, build, clean)
- Application management (add/remove apps to projects)
- IDL compilation and code generation
- Build orchestration using CMake and ARM toolchain
- Deployment configuration
- Debugging interface

**Subcommands:**

```bash
lumos project create <name>           # Create new project
lumos project build [--target <mcu>]  # Build project for target
lumos project clean                    # Clean build artifacts
lumos project run [--target <mcu>]    # Flash and run on target

lumos app create <name>                # Create new application
lumos app add <name> --to <project>    # Add app to project
lumos app list                         # List all applications

lumos interface generate <idl-file>    # Generate communication code
lumos interface validate <idl-file>    # Validate IDL syntax

lumos deploy configure                 # Configure deployment targets
lumos deploy flash <target>            # Flash to specific target
lumos deploy monitor <target>          # Monitor serial output

lumos debug attach <target>            # Attach debugger
```

**Key Files:**
- `main.cpp` - CLI entry point and argument parsing
- `commands/` - Individual command implementations
- `config/` - Configuration file handling
- `generators/` - Code generation templates

### 2. Lumos Framework (`src/framework`)

Core runtime framework that gets linked with user applications.

**Components:**

#### a. Application Runtime
```cpp
class Application {
public:
    virtual void Init() = 0;
    virtual void Step() = 0;
    virtual void DeInit() = 0;

    void SetUpdateRate(uint32_t hz);
    void RegisterInterface(Interface* iface);
};
```

#### b. Communication Middleware
- Message routing and dispatch
- Serialization/deserialization (using IDL-generated code)
- Location-transparent addressing
- Quality of Service (QoS) management

#### c. Scheduler
- Cooperative or preemptive scheduling
- Rate-based execution
- Priority management
- Deadline monitoring

#### d. Transport Abstraction
- Unified API for different transports
- Local (in-process) communication
- UART/SPI for inter-MCU
- CAN bus support
- Pluggable transport backends

**Key Files:**
- `application.h/cpp` - Base application class
- `scheduler.h/cpp` - Task scheduler
- `transport/` - Transport layer implementations
- `message.h/cpp` - Message primitives
- `router.h/cpp` - Message routing logic
- `serialization.h/cpp` - Serialization utilities

### 3. IDL Compiler (`src/idl`)

Interface Definition Language compiler for generating communication code.

**Responsibilities:**
- Parse IDL files defining messages and services
- Generate C++ code for serialization
- Generate message routing tables
- Create type-safe communication APIs

**IDL Syntax Example:**
```idl
// sensor.idl
namespace sensor {
    message TemperatureReading {
        float temperature;
        uint32_t timestamp;
        uint8_t sensor_id;
    }

    service TemperatureSensor {
        TemperatureReading GetReading();
        void Calibrate(float offset);
    }
}
```

**Generated Artifacts:**
- `sensor_messages.h/cpp` - Message structures and serialization
- `sensor_service.h/cpp` - Service interface and stub
- `sensor_client.h/cpp` - Client proxy class

**Key Files:**
- `parser.h/cpp` - IDL parser
- `ast.h/cpp` - Abstract syntax tree
- `generator.h/cpp` - Code generator
- `templates/` - Code generation templates

### 4. Build System (`src/build`)

Build orchestration and toolchain management.

**Responsibilities:**
- CMake configuration generation
- Toolchain selection and configuration
- Dependency management
- Multi-target builds
- Linker script management

**Key Files:**
- `toolchain_manager.h/cpp` - Toolchain detection and setup
- `cmake_generator.h/cpp` - CMake file generation
- `dependency_resolver.h/cpp` - Dependency management
- `templates/` - CMake templates

### 5. Target Support (`src/targets`)

Hardware abstraction and target-specific code.

**Directory Structure:**
```
src/targets/
├── stm32/
│   ├── stm32f4/
│   ├── stm32h7/
│   └── hal/
├── esp32/
├── nrf52/
└── common/
    ├── startup.s
    └── system.c
```

**Responsibilities:**
- Startup code
- HAL integration
- Peripheral drivers
- Memory mapping
- Interrupt handlers

### 6. Modules (`src/modules`)

Reusable modules that can be added to applications.

**Examples:**
- Logging module
- Diagnostics module
- Configuration management
- Flash storage
- Bootloader integration
- Real-time trace

**Directory Structure:**
```
src/modules/
├── logging/
├── diagnostics/
├── config/
├── flash/
└── trace/
```

### 7. Toolchains (`src/toolchains`)

Embedded toolchains for cross-compilation.

**Current:**
- `gcc-arm-none-eabi-10.3-2021.10/` - ARM Cortex-M toolchain

**Future:**
- RISC-V toolchain
- Xtensa (ESP32) toolchain

## Data Flow

### Application Development Flow

1. **Create Project**
   ```bash
   lumos project create MyRobot
   cd MyRobot
   ```

2. **Create Applications**
   ```bash
   lumos app create MotorController
   lumos app create SensorReader
   lumos app create MainController
   ```

3. **Define Interfaces (IDL)**
   ```bash
   # Edit interfaces/motor.idl
   lumos interface generate interfaces/motor.idl
   ```

4. **Implement Applications**
   - Implement Init/Step/DeInit methods
   - Use generated interface code
   - Register with framework

5. **Configure Deployment**
   ```bash
   lumos deploy configure
   # Edit lumos.json to map apps to targets
   ```

6. **Build**
   ```bash
   lumos project build --target stm32f407
   ```

7. **Deploy**
   ```bash
   lumos deploy flash stm32f407
   ```

### Runtime Communication Flow

```
[App A: MotorController] --local--> [App B: MainController]
         |                                    |
         |                                    |
         └-------- UART Transport -------> [MCU2: SensorReader]
```

1. App B calls `motorService->SetSpeed(100)`
2. Framework determines App A location (local or remote)
3. If local: Direct function call through shared memory
4. If remote: Serialize message and send via configured transport
5. Remote framework receives, deserializes, and dispatches
6. Response follows reverse path

## Project Structure

```
LumosTool/
├── CMakeLists.txt              # Root build configuration
├── ARCHITECTURE.md             # This file
├── TODO.md                     # Development roadmap
├── .gitignore
├── .gitmodules
│
├── src/
│   ├── applications/
│   │   └── lumos/              # CLI tool
│   │       ├── main.cpp
│   │       ├── commands/
│   │       ├── config/
│   │       └── generators/
│   │
│   ├── framework/              # Core runtime framework
│   │   ├── application.h/cpp
│   │   ├── scheduler.h/cpp
│   │   ├── message.h/cpp
│   │   ├── router.h/cpp
│   │   ├── transport/
│   │   │   ├── transport.h
│   │   │   ├── local_transport.cpp
│   │   │   ├── uart_transport.cpp
│   │   │   └── can_transport.cpp
│   │   └── serialization.h/cpp
│   │
│   ├── idl/                    # IDL compiler
│   │   ├── parser.h/cpp
│   │   ├── ast.h/cpp
│   │   ├── generator.h/cpp
│   │   └── templates/
│   │
│   ├── build/                  # Build system
│   │   ├── toolchain_manager.h/cpp
│   │   ├── cmake_generator.h/cpp
│   │   └── templates/
│   │
│   ├── targets/                # Target-specific code
│   │   ├── stm32/
│   │   ├── esp32/
│   │   └── common/
│   │
│   ├── modules/                # Reusable modules
│   │   ├── logging/
│   │   ├── diagnostics/
│   │   └── config/
│   │
│   └── toolchains/             # Cross-compilation toolchains
│       └── gcc-arm-none-eabi-10.3-2021.10/
│
├── third_party/                # External dependencies
│   ├── nlohmann/               # JSON library
│   └── googletest/             # Testing framework
│
├── tests/                      # Unit and integration tests
│   ├── framework/
│   ├── idl/
│   └── integration/
│
├── examples/                   # Example projects
│   ├── hello_world/
│   ├── multi_mcu/
│   └── sensor_network/
│
└── docs/                       # Documentation
    ├── getting_started.md
    ├── idl_reference.md
    ├── api_reference.md
    └── tutorials/
```

## Configuration Files

### Project Configuration: `lumos.json`

```json
{
  "project": {
    "name": "MyRobot",
    "version": "1.0.0"
  },
  "applications": [
    {
      "name": "MotorController",
      "target": "stm32f407",
      "rate_hz": 100,
      "priority": 10
    },
    {
      "name": "SensorReader",
      "target": "stm32f103",
      "rate_hz": 10,
      "priority": 5
    },
    {
      "name": "MainController",
      "target": "stm32f407",
      "rate_hz": 50,
      "priority": 15
    }
  ],
  "interfaces": [
    "interfaces/motor.idl",
    "interfaces/sensor.idl"
  ],
  "transports": [
    {
      "type": "uart",
      "from": "stm32f407",
      "to": "stm32f103",
      "config": {
        "baudrate": 115200,
        "port": "UART1"
      }
    }
  ]
}
```

### Application Configuration: `app.json`

```json
{
  "name": "MotorController",
  "type": "application",
  "provides": ["motor.MotorService"],
  "requires": ["sensor.TemperatureSensor"],
  "modules": ["logging", "diagnostics"],
  "memory": {
    "stack_size": 4096,
    "heap_size": 8192
  }
}
```

## Development Roadmap

### Phase 1: Foundation (Current)
- [ ] Lumos CLI basic structure
- [ ] Project creation and management
- [ ] Application scaffolding
- [ ] Basic build system integration

### Phase 2: Framework Core
- [ ] Application base class and lifecycle
- [ ] Local scheduler implementation
- [ ] Message primitives
- [ ] Local transport (same MCU)

### Phase 3: IDL Compiler
- [ ] IDL parser and AST
- [ ] Code generator for messages
- [ ] Code generator for services
- [ ] Integration with build system

### Phase 4: Distributed Communication
- [ ] UART transport implementation
- [ ] Message router with location awareness
- [ ] Serialization framework
- [ ] Remote procedure call mechanism

### Phase 5: Advanced Features
- [ ] Multiple transport types (SPI, CAN)
- [ ] Quality of Service (QoS)
- [ ] Real-time scheduling
- [ ] Debugging and tracing tools

### Phase 6: Developer Experience
- [ ] Comprehensive examples
- [ ] Documentation
- [ ] Testing infrastructure
- [ ] Performance profiling tools

## Key Design Decisions

### 1. Location Transparency Implementation

Use a service registry that maintains location information:

```cpp
class ServiceRegistry {
    struct ServiceLocation {
        std::string service_name;
        bool is_local;
        Transport* transport;  // nullptr if local
        uint32_t remote_id;
    };

    ServiceLocation Lookup(const std::string& service);
};
```

### 2. Scheduling Strategy

Cooperative scheduler with rate-based execution:

```cpp
class Scheduler {
    struct Task {
        Application* app;
        uint32_t period_ms;
        uint32_t last_run;
        uint8_t priority;
    };

    void Run(); // Main loop
};
```

### 3. Message Serialization

Use efficient binary serialization with schema evolution support:
- Fixed-size fields first (for alignment)
- Variable-size fields at end
- Version field in header
- Length prefix for variable data

### 4. Transport Abstraction

```cpp
class Transport {
public:
    virtual bool Send(const Message& msg, uint32_t dest_id) = 0;
    virtual bool Receive(Message& msg, uint32_t timeout_ms) = 0;
    virtual bool IsConnected() = 0;
};
```

## Conclusion

The Lumos ecosystem provides a complete solution for building distributed embedded applications. By abstracting communication and providing location transparency, developers can focus on application logic rather than communication details. The CLI tool orchestrates the entire development workflow from creation to deployment.

Next steps: Begin implementation with Phase 1 components, focusing on the CLI tool structure and project management capabilities.
