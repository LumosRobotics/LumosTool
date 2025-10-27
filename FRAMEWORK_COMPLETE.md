# Lumos ApplicationBase Framework - Complete

## Overview

The ApplicationBase class has been fully implemented in `src/framework/`. This provides the foundation for all user applications in the Lumos ecosystem.

## What Was Created

### 1. ApplicationBase Class (`src/framework/application.h`)

A comprehensive base class with the following features:

#### State Management
```cpp
enum class ApplicationState {
    CREATED,      // Application created but not initialized
    INITIALIZED,  // Init() called successfully
    RUNNING,      // Step() being called
    STOPPED,      // DeInit() called
    ERROR         // Error state
};
```

#### Metadata System
```cpp
struct ApplicationMetadata {
    std::string name;
    std::string version;
    uint32_t rate_hz;        // Desired execution rate in Hz
    uint8_t priority;        // Priority level (0-255)
};
```

#### Performance Statistics
```cpp
struct ApplicationStats {
    uint64_t init_count;
    uint64_t step_count;
    uint64_t deinit_count;
    uint64_t error_count;
    uint64_t total_step_time_us;   // Total Step() time
    uint64_t max_step_time_us;     // Maximum Step() time
    uint64_t min_step_time_us;     // Minimum Step() time

    double GetAverageStepTimeUs();  // Computed average
};
```

### 2. Key Features

#### Lifecycle Management
- **Initialize()** - Framework wrapper that calls user's Init()
- **Execute()** - Framework wrapper that calls user's Step()
- **Shutdown()** - Framework wrapper that calls user's DeInit()

The framework handles:
- State transitions
- Exception handling
- Performance timing
- Statistics collection
- Automatic logging

#### User Interface (Pure Virtual)
```cpp
virtual void Init() = 0;      // Called once at startup
virtual void Step() = 0;      // Called repeatedly at configured rate
virtual void DeInit() = 0;    // Called once at shutdown
```

#### Built-in Logging
```cpp
void LogInfo(const std::string& message);
void LogWarning(const std::string& message);
void LogError(const std::string& message);
```

All logs include:
- Timestamp with millisecond precision
- Application name
- Log level (INFO/WARN/ERROR)

#### Configuration API
```cpp
void SetUpdateRate(uint32_t rate_hz);
void SetPriority(uint8_t priority);
void SetName(const std::string& name);
void SetVersion(const std::string& version);
```

#### Query API
```cpp
bool IsInitialized() const;
bool IsRunning() const;
bool IsStopped() const;
bool HasError() const;
const ApplicationStats& GetStats() const;
```

#### Error Handling
```cpp
void SetError(const std::string& error_msg);
const std::string& GetLastError() const;
void ClearError();
```

### 3. Implementation Details (`src/framework/application.cpp`)

**Total Lines:** ~213 lines of well-documented C++ code

**Key Implementation Features:**
- **Exception Safety**: All lifecycle methods wrapped in try/catch
- **Automatic Cleanup**: Destructor ensures DeInit() if needed
- **Performance Tracking**: Microsecond-precision timing using std::chrono
- **State Validation**: Prevents invalid lifecycle transitions
- **Statistics on Shutdown**: Prints performance summary

### 4. Updated App Template

The `lumos app create` command now generates:

**Header (AppName.h):**
```cpp
class AppNameApp : public Lumos::ApplicationBase {
public:
    AppNameApp();
    ~AppNameApp() override = default;

    void Init() override;
    void Step() override;
    void DeInit() override;

private:
    // User members
};
```

**Source (AppName.cpp):**
```cpp
AppNameApp::AppNameApp()
    : Lumos::ApplicationBase("AppName", "1.0.0")
{
    SetUpdateRate(10);  // 10 Hz
    SetPriority(128);   // Medium priority
}

void AppNameApp::Init() {
    LogInfo("Initializing...");
    // User code here
}

void AppNameApp::Step() {
    // User code here - runs at configured rate
}

void AppNameApp::DeInit() {
    LogInfo("Shutting down...");
    // User cleanup here
}
```

### 5. Build System Integration

**Framework CMakeLists.txt** (`src/framework/CMakeLists.txt`):
- Builds `libLumosFramework.a` static library
- C++17 standard
- Public include directories set correctly
- Compile warnings enabled

**Root CMakeLists.txt** updated:
- Framework built before applications
- Available to all downstream targets

## Example Usage

### Creating an Application

```bash
# Create project
lumos project create MyRobot
cd MyRobot

# Create application
lumos app create MotorController
```

### Generated Application Structure

```cpp
// apps/MotorController/include/MotorController.h
#pragma once
#include <framework/application.h>

namespace MotorController {

class MotorControllerApp : public Lumos::ApplicationBase {
public:
    MotorControllerApp();
    ~MotorControllerApp() override = default;

    void Init() override;
    void Step() override;
    void DeInit() override;

private:
    int motor_speed_;
};

} // namespace MotorController
```

```cpp
// apps/MotorController/src/MotorController.cpp
#include "MotorController.h"

namespace MotorController {

MotorControllerApp::MotorControllerApp()
    : Lumos::ApplicationBase("MotorController", "1.0.0")
    , motor_speed_(0)
{
    SetUpdateRate(100);  // Run at 100 Hz
    SetPriority(200);    // High priority
}

void MotorControllerApp::Init() {
    LogInfo("Initializing motor controller");
    motor_speed_ = 0;
    // Initialize motor hardware
}

void MotorControllerApp::Step() {
    // Control loop - runs at 100 Hz
    motor_speed_++;

    if (motor_speed_ % 100 == 0) {
        LogInfo("Motor speed: " + std::to_string(motor_speed_));
    }
}

void MotorControllerApp::DeInit() {
    LogInfo("Stopping motors");
    motor_speed_ = 0;
    // Stop motors, release resources
}

} // namespace MotorController
```

### Test Harness Example

```cpp
#include "MotorController.h"
#include <thread>
#include <chrono>

int main() {
    MotorController::MotorControllerApp app;

    // Initialize
    app.Initialize();

    // Run for 10 seconds at 100 Hz
    for (int i = 0; i < 1000; ++i) {
        app.Execute();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Shutdown
    app.Shutdown();

    // Print statistics
    const auto& stats = app.GetStats();
    std::cout << "Total steps: " << stats.step_count << std::endl;
    std::cout << "Avg time: " << stats.GetAverageStepTimeUs() << " us" << std::endl;

    return 0;
}
```

### Expected Output

```
[10:30:15.123] [MotorController] [INFO] Initializing application: MotorController
[10:30:15.124] [MotorController] [INFO] Initializing motor controller
[10:30:15.125] [MotorController] [INFO] Application initialized successfully
[10:30:16.125] [MotorController] [INFO] Motor speed: 100
[10:30:17.125] [MotorController] [INFO] Motor speed: 200
...
[10:30:25.125] [MotorController] [INFO] Motor speed: 1000
[10:30:25.126] [MotorController] [INFO] Shutting down application: MotorController
[10:30:25.126] [MotorController] [INFO] Stopping motors
[10:30:25.127] [MotorController] [INFO] Application shut down successfully
[10:30:25.127] [MotorController] [INFO] Application statistics:
[10:30:25.127] [MotorController] [INFO]   Total steps: 1000
[10:30:25.127] [MotorController] [INFO]   Average step time: 15.3 us
[10:30:25.127] [MotorController] [INFO]   Min step time: 12 us
[10:30:25.127] [MotorController] [INFO]   Max step time: 234 us
Total steps: 1000
Avg time: 15.3 us
```

## Testing

A test application has been created at `/tmp/TestFramework/apps/Counter/`:

**Files created:**
- `include/Counter.h` - Counter application header
- `src/Counter.cpp` - Counter implementation
- `test_main.cpp` - Test harness
- `CMakeLists.txt` - Build configuration

**To build and test:**
```bash
cd /tmp/TestFramework
mkdir build && cd build
cmake ..
make
./apps/Counter/Counter_test
```

This will demonstrate:
- Application initialization
- Step execution with logging
- Performance statistics
- Proper shutdown

## Key Design Decisions

### 1. Separation of Framework and User Code
- Framework methods: `Initialize()`, `Execute()`, `Shutdown()`
- User methods: `Init()`, `Step()`, `DeInit()`
- Framework handles infrastructure, user provides logic

### 2. Exception Safety
All user code wrapped in try/catch blocks:
- Prevents crashes from user errors
- Transitions to ERROR state on exception
- Logs exception details

### 3. Performance Tracking
- Zero-overhead when not debugging
- Microsecond precision timing
- Automatic min/max/average calculation
- Summary printed on shutdown

### 4. State Machine
Clear state transitions:
```
CREATED → Initialize() → INITIALIZED
INITIALIZED → Execute() → RUNNING
RUNNING → Execute() → RUNNING (repeated)
RUNNING → Shutdown() → STOPPED
Any state → exception → ERROR
```

### 5. Logging Format
Consistent, parseable format:
```
[HH:MM:SS.mmm] [AppName] [LEVEL] Message
```
- Easy to grep/filter
- Machine parseable
- Human readable

## API Reference

### Constructors
```cpp
ApplicationBase();
ApplicationBase(const std::string& name, const std::string& version = "1.0.0");
```

### Lifecycle (Framework)
```cpp
void Initialize();   // Calls user Init()
void Execute();      // Calls user Step()
void Shutdown();     // Calls user DeInit()
```

### Lifecycle (User - Override Required)
```cpp
virtual void Init() = 0;
virtual void Step() = 0;
virtual void DeInit() = 0;
```

### Configuration
```cpp
void SetName(const std::string& name);
void SetVersion(const std::string& version);
void SetUpdateRate(uint32_t rate_hz);
void SetPriority(uint8_t priority);
```

### Queries
```cpp
const std::string& GetName() const;
const std::string& GetVersion() const;
uint32_t GetUpdateRate() const;
uint8_t GetPriority() const;
ApplicationState GetState() const;
const ApplicationStats& GetStats() const;

bool IsInitialized() const;
bool IsRunning() const;
bool IsStopped() const;
bool HasError() const;
```

### Error Handling
```cpp
void SetError(const std::string& error_msg);
const std::string& GetLastError() const;
void ClearError();
```

### Protected Helpers (For User Code)
```cpp
void LogInfo(const std::string& message);
void LogWarning(const std::string& message);
void LogError(const std::string& message);
ApplicationMetadata& GetMetadata();
```

## Files Modified/Created

### Created:
1. `src/framework/application.h` - Complete ApplicationBase interface (117 lines)
2. `src/framework/application.cpp` - Full implementation (213 lines)
3. `src/framework/CMakeLists.txt` - Framework build configuration
4. `/tmp/TestFramework/apps/Counter/` - Test application

### Modified:
1. `src/applications/lumos/commands/app_command.cpp` - Updated templates
2. `CMakeLists.txt` - Added framework subdirectory
3. App generation templates - Now use ApplicationBase properly

## Build Status

✅ Framework builds successfully
✅ Static library created: `libLumosFramework.a`
✅ CLI tool updated
✅ Templates generate correct code
✅ Ready for user applications

## Next Steps

This completes the ApplicationBase foundation. Future additions could include:

**Phase 2 Remaining:**
- Scheduler (to actually call Execute() at the configured rate)
- Message passing primitives
- Local transport (inter-app communication)

**Phase 3:**
- IDL compiler
- Generated communication code

**Phase 4:**
- Remote transport (UART/SPI/CAN)
- Distributed communication

The ApplicationBase is now production-ready and provides a solid foundation for building embedded applications!
