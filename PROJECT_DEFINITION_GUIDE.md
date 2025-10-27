# Lumos Project Definition Guide

## Overview

A Lumos project is a distributed embedded application that may span multiple microcontrollers, each running one or more applications that communicate with each other. This document analyzes all the aspects that need to be defined for a complete project.

## 1. Project Structure Definition

### 1.1 Project Metadata
```json
{
  "project": {
    "name": "RobotController",
    "version": "1.0.0",
    "description": "Mobile robot control system",
    "author": "Engineering Team",
    "created": "2025-10-26",
    "targets": ["stm32f407", "stm32f103", "esp32"]
  }
}
```

**Required Information:**
- Project name (unique identifier)
- Version (semantic versioning)
- Description (human-readable purpose)
- Supported hardware targets
- Creation/modification timestamps

### 1.2 Directory Structure
```
Project/
├── lumos.json              # Project configuration
├── hardware.json           # Hardware topology definition
├── communication.json      # Communication topology
├── deployment.json         # Deployment configuration
├── apps/                   # Application implementations
├── interfaces/             # IDL interface definitions
├── configs/                # Configuration files
│   ├── logging.json
│   ├── topics.json
│   └── security.json
└── scripts/                # Build and deployment scripts
```

## 2. Application Definition

### 2.1 Application Metadata
```json
{
  "name": "MotorController",
  "type": "application",
  "version": "1.0.0",
  "execution": {
    "rate_hz": 100,
    "priority": 200,
    "mode": "periodic",          // periodic, event-driven, sporadic
    "deadline_ms": 10,            // for real-time scheduling
    "wcet_ms": 8                  // worst-case execution time estimate
  },
  "resources": {
    "stack_size": 8192,
    "heap_size": 16384,
    "cpu_affinity": 0             // for multi-core targets
  },
  "deployment": {
    "target": "stm32f407",
    "node_id": "main_controller"
  }
}
```

**Key Aspects:**
- **Execution Mode**: Periodic (fixed rate), Event-driven (on message), Sporadic (bounded rate)
- **Timing Constraints**: Rate, deadline, WCET for schedulability analysis
- **Resource Requirements**: Memory footprint for deployment planning
- **Deployment Target**: Which physical node runs this app

### 2.2 Application Dependencies

```json
{
  "dependencies": {
    "provides": [
      "motor.MotorControl",       // Services this app provides
      "motor.MotorStatus"
    ],
    "requires": [
      "sensor.TemperatureSensor", // Services this app needs
      "safety.EmergencyStop"
    ],
    "optional": [
      "diagnostics.Telemetry"     // Optional services
    ]
  }
}
```

**Considerations:**
- Service discovery mechanism
- Dependency resolution at build time
- Runtime service availability checking
- Graceful degradation when optional services unavailable

## 3. Hardware Topology Definition

### 3.1 Node Definition
```json
{
  "nodes": [
    {
      "id": "main_controller",
      "type": "stm32f407",
      "role": "master",
      "hardware": {
        "cpu_freq_mhz": 168,
        "ram_kb": 192,
        "flash_kb": 1024,
        "cores": 1
      },
      "peripherals": {
        "uart": ["UART1", "UART2"],
        "spi": ["SPI1"],
        "can": ["CAN1"],
        "i2c": ["I2C1"],
        "gpio": {
          "digital_in": 16,
          "digital_out": 16,
          "analog_in": 8
        }
      },
      "connections": [
        {
          "interface": "UART1",
          "to_node": "sensor_node",
          "baudrate": 115200,
          "flow_control": "none"
        },
        {
          "interface": "CAN1",
          "to_bus": "can_bus_0",
          "bitrate": 500000
        }
      ]
    },
    {
      "id": "sensor_node",
      "type": "stm32f103",
      "role": "sensor_hub",
      "hardware": {
        "cpu_freq_mhz": 72,
        "ram_kb": 20,
        "flash_kb": 128,
        "cores": 1
      }
    }
  ]
}
```

**Key Information:**
- **Node Identification**: Unique IDs, roles, types
- **Hardware Capabilities**: CPU, memory, peripherals
- **Physical Connections**: How nodes are wired together
- **Resource Constraints**: For deployment validation

### 3.2 Bus/Network Definition
```json
{
  "buses": [
    {
      "id": "can_bus_0",
      "type": "CAN",
      "bitrate": 500000,
      "nodes": ["main_controller", "motor_driver", "battery_monitor"],
      "arbitration": "priority",
      "error_handling": "automatic_retransmit"
    },
    {
      "id": "i2c_sensors",
      "type": "I2C",
      "speed": "fast_mode",  // 400kHz
      "nodes": ["sensor_node", "display_node"],
      "addressing": "7bit"
    }
  ]
}
```

**Considerations:**
- Bus topology (point-to-point, multi-drop, shared bus)
- Bus arbitration mechanism
- Error detection and correction
- Bandwidth allocation

## 4. Communication Topology Definition

### 4.1 Message Flow Definition
```json
{
  "message_flows": [
    {
      "name": "motor_command_flow",
      "publisher": {
        "app": "MainController",
        "node": "main_controller"
      },
      "subscribers": [
        {
          "app": "MotorDriver",
          "node": "motor_driver_node",
          "qos": "reliable"
        }
      ],
      "topic": "motor/command",
      "rate_hz": 100,
      "latency_budget_ms": 5,
      "transport": "can_bus_0"
    },
    {
      "name": "sensor_data_flow",
      "publisher": {
        "app": "SensorReader",
        "node": "sensor_node"
      },
      "subscribers": [
        {
          "app": "MainController",
          "node": "main_controller",
          "qos": "best_effort"
        },
        {
          "app": "DataLogger",
          "node": "main_controller",
          "qos": "best_effort"
        }
      ],
      "topic": "sensors/imu",
      "rate_hz": 200,
      "transport": "uart1_link"
    }
  ]
}
```

**Key Aspects:**
- **Publisher-Subscriber Model**: Who sends, who receives
- **Topic Namespace**: Hierarchical topic organization
- **Quality of Service**: Delivery guarantees
- **Timing Requirements**: Expected rate, latency bounds
- **Transport Selection**: Which physical connection to use

### 4.2 Communication Patterns

**Publish-Subscribe:**
```
Publisher → Topic → Multiple Subscribers
- One-to-many
- Decoupled
- Time-based
```

**Request-Reply (RPC):**
```
Client → Request → Server
Server → Reply → Client
- Synchronous/Asynchronous
- Timeout handling
- Error responses
```

**Event Broadcasting:**
```
Event Source → Event → All Listeners
- Emergency stop, fault conditions
- System-wide notifications
```

### 4.3 Quality of Service (QoS) Policies

```json
{
  "qos_profiles": {
    "critical_control": {
      "reliability": "reliable",        // guaranteed delivery
      "ordering": "strict",              // in-order delivery
      "deadline_ms": 10,                 // max delivery time
      "priority": 255,                   // highest priority
      "durability": "volatile",          // no persistence needed
      "history": {
        "kind": "keep_last",
        "depth": 1
      }
    },
    "sensor_data": {
      "reliability": "best_effort",     // may drop messages
      "ordering": "relaxed",             // out-of-order OK
      "deadline_ms": 50,
      "priority": 128,
      "durability": "volatile",
      "history": {
        "kind": "keep_last",
        "depth": 10
      }
    },
    "diagnostics": {
      "reliability": "reliable",
      "durability": "transient_local",   // persist in memory
      "history": {
        "kind": "keep_all"               // keep all samples
      }
    }
  }
}
```

**QoS Parameters:**
- **Reliability**: Best-effort vs. Guaranteed delivery
- **Ordering**: Strict vs. Relaxed ordering requirements
- **Deadline**: Maximum acceptable latency
- **Priority**: Message priority for scheduling
- **Durability**: Volatile vs. Persistent storage
- **History**: How many samples to keep

## 5. Topic Subscription Model

### 5.1 Topic Hierarchy
```
robot/
├── control/
│   ├── motor/command
│   ├── motor/status
│   └── steering/command
├── sensors/
│   ├── imu/raw
│   ├── imu/filtered
│   ├── gps/position
│   └── temperature/readings
├── safety/
│   ├── emergency_stop
│   ├── fault_status
│   └── heartbeat
└── diagnostics/
    ├── performance
    ├── errors
    └── telemetry
```

**Best Practices:**
- Hierarchical naming (domain/category/specific)
- Use wildcards for subscriptions: `sensors/#` (all sensors)
- Separate control, data, and diagnostic topics
- Reserve top-level namespace for safety-critical

### 5.2 Subscription Configuration
```json
{
  "subscriptions": [
    {
      "app": "MainController",
      "topics": [
        {
          "topic": "sensors/imu/filtered",
          "qos": "sensor_data",
          "callback": "OnImuData",
          "buffer_size": 10,
          "filters": {
            "rate_limit_hz": 100,        // downsample if needed
            "content_filter": "x > 0.1"  // threshold filtering
          }
        },
        {
          "topic": "safety/emergency_stop",
          "qos": "critical_control",
          "callback": "OnEmergencyStop",
          "priority": "highest"
        }
      ]
    }
  ]
}
```

**Subscription Features:**
- Per-subscription QoS overrides
- Callback function binding
- Buffer management
- Rate limiting and filtering
- Priority-based dispatch

## 6. Data Integrity and Validation

### 6.1 Error Detection
```json
{
  "integrity": {
    "checksum": {
      "algorithm": "crc32",              // CRC-16, CRC-32, Fletcher
      "scope": "payload",                // header, payload, both
      "automatic": true                  // auto-compute and verify
    },
    "sequence_numbers": {
      "enabled": true,
      "rollover": 65535,
      "gap_detection": true
    },
    "validation": {
      "schema_check": true,              // validate against IDL
      "range_check": true,               // value bounds checking
      "timestamp_check": true            // detect stale data
    }
  }
}
```

**Error Detection Mechanisms:**
- **CRC/Checksum**: Detect bit errors in transmission
- **Sequence Numbers**: Detect lost/duplicated/reordered messages
- **Schema Validation**: Ensure data matches interface definition
- **Range Validation**: Sanity check for sensor values
- **Timestamp Validation**: Detect stale or future-dated messages

### 6.2 Error Correction
```json
{
  "error_correction": {
    "retransmission": {
      "enabled": true,
      "max_retries": 3,
      "timeout_ms": 100,
      "backoff": "exponential"
    },
    "redundancy": {
      "enabled": false,
      "method": "voting",                // majority voting
      "min_sources": 3
    },
    "fallback": {
      "use_last_known_good": true,
      "max_age_ms": 500
    }
  }
}
```

### 6.3 End-to-End Integrity
```json
{
  "e2e_protection": {
    "enabled": true,
    "profile": "ASIL_D",                 // Safety integrity level
    "mechanisms": {
      "crc": {
        "algorithm": "crc32c",
        "polynomial": "0x1EDC6F41"
      },
      "counter": {
        "bits": 16,
        "wrap_behavior": "saturate"
      },
      "alive_monitoring": {
        "enabled": true,
        "timeout_ms": 100
      }
    },
    "actions_on_error": {
      "crc_failure": "discard",
      "sequence_gap": "log_and_continue",
      "timeout": "trigger_fault_handler"
    }
  }
}
```

**AUTOSAR E2E Patterns:**
- Profile 1: CRC + Counter (simple messages)
- Profile 2: CRC + Counter + DataID (complex messages)
- Profile 4: CRC + Counter + Offset (variable length)

## 7. Timing and Synchronization

### 7.1 Timestamp Configuration
```json
{
  "timing": {
    "time_source": {
      "primary": "crystal_oscillator",   // HW timer, RTC, GPS
      "frequency_hz": 168000000,
      "resolution_us": 1,
      "drift_ppm": 20
    },
    "synchronization": {
      "enabled": true,
      "protocol": "ptp",                 // PTP, NTP, custom
      "master_node": "main_controller",
      "sync_interval_ms": 1000,
      "max_offset_us": 100
    },
    "timestamps": {
      "format": "unix_time_us",          // unix_time_us, monotonic, tai
      "include_in_messages": true,
      "timestamp_fields": ["acquired", "sent", "received"]
    }
  }
}
```

**Timestamp Types:**
- **Acquisition Time**: When data was sampled (sensor reading)
- **Send Time**: When message was transmitted
- **Receive Time**: When message was received
- **Processing Time**: When application processed the data

### 7.2 Clock Synchronization Requirements

```
Node A Clock: |-------|-------|-------|-------|
Node B Clock: |------|------|------|------|
Offset:          ↑ Δt (needs sync)
```

**Synchronization Needs:**
- Sensor fusion (combine data from different nodes)
- Event correlation (match cause and effect)
- Data logging (coherent timestamps)
- Control loops (phase alignment)

**Precision Requirements:**
- Loose: ±100ms (data logging)
- Medium: ±10ms (control coordination)
- Tight: ±100μs (sensor fusion)
- Ultra-tight: ±1μs (precision motor control)

### 7.3 Sampling Rate Coordination
```json
{
  "sampling_coordination": {
    "global_timebase": true,
    "sync_markers": {
      "enabled": true,
      "marker_topic": "system/sync",
      "rate_hz": 1000                    // 1kHz tick
    },
    "phase_alignment": {
      "apps": [
        {
          "name": "IMUSampler",
          "rate_hz": 1000,
          "phase_offset_us": 0           // Sample at t=0
        },
        {
          "name": "MotorController",
          "rate_hz": 1000,
          "phase_offset_us": 500         // Sample at t=500us
        }
      ]
    }
  }
}
```

## 8. Data Logging and Recording

### 8.1 Logging Configuration
```json
{
  "logging": {
    "global_settings": {
      "enabled": true,
      "default_level": "INFO",
      "timestamp_format": "iso8601_us",
      "output_format": "json"            // json, csv, binary
    },
    "destinations": [
      {
        "type": "serial",
        "interface": "UART2",
        "baudrate": 921600,
        "buffer_size": 8192
      },
      {
        "type": "sd_card",
        "path": "/logs",
        "max_file_size_mb": 100,
        "rotation": "daily"
      },
      {
        "type": "network",
        "protocol": "udp",
        "host": "192.168.1.100",
        "port": 5140
      }
    ],
    "filters": {
      "apps": ["MotorController", "SensorReader"],
      "levels": ["WARN", "ERROR"],
      "topics": ["sensors/*", "control/*"]
    }
  }
}
```

### 8.2 Data Recording (Black Box)
```json
{
  "recording": {
    "enabled": true,
    "mode": "continuous",                // continuous, triggered, circular
    "triggers": [
      {
        "type": "event",
        "condition": "safety/fault_detected",
        "pre_trigger_s": 5,              // Record 5s before event
        "post_trigger_s": 10             // Record 10s after event
      },
      {
        "type": "threshold",
        "topic": "sensors/temperature",
        "condition": "value > 80",
        "action": "start_recording"
      }
    ],
    "topics": [
      {
        "topic": "control/*",
        "rate_hz": 100,
        "compression": "none"
      },
      {
        "topic": "sensors/*",
        "rate_hz": 200,
        "compression": "lz4"
      }
    ],
    "storage": {
      "type": "circular_buffer",
      "size_mb": 512,
      "persistence": "sd_card",
      "format": "mcap"                   // MCAP, ROS bag, custom
    }
  }
}
```

**Use Cases:**
- **Flight Recorder**: Circular buffer, triggered dump on fault
- **Validation Testing**: Record all data for replay
- **Performance Analysis**: Sample key metrics
- **Compliance**: Audit trail for safety-critical systems

### 8.3 Log Levels and Categories
```json
{
  "log_levels": {
    "TRACE": 0,    // Detailed debug info
    "DEBUG": 1,    // Debug information
    "INFO": 2,     // General information
    "WARN": 3,     // Warning conditions
    "ERROR": 4,    // Error conditions
    "FATAL": 5     // Fatal errors
  },
  "categories": {
    "SYSTEM": "System-level events",
    "COMM": "Communication events",
    "APP": "Application-level logs",
    "PERF": "Performance metrics",
    "SECURITY": "Security events"
  },
  "app_log_config": [
    {
      "app": "MotorController",
      "level": "DEBUG",
      "rate_limit": {
        "enabled": true,
        "max_per_second": 100
      }
    }
  ]
}
```

## 9. Security and Authentication

### 9.1 Message Authentication
```json
{
  "security": {
    "authentication": {
      "enabled": true,
      "method": "hmac_sha256",
      "key_management": "pre_shared",    // PSK, PKI, key derivation
      "key_rotation_interval_h": 24
    },
    "encryption": {
      "enabled": false,                  // Usually not on CAN/local nets
      "algorithm": "aes128_gcm",
      "applicable_to": ["network/*"]     // Only for network topics
    },
    "access_control": {
      "enabled": true,
      "rules": [
        {
          "app": "SensorReader",
          "allowed_topics": ["sensors/*"],
          "permissions": ["publish"]
        },
        {
          "app": "MotorController",
          "allowed_topics": ["motor/command"],
          "permissions": ["subscribe", "publish"]
        }
      ]
    }
  }
}
```

**Considerations:**
- Computational overhead (crypto on MCU)
- Key storage and distribution
- Secure boot and code signing
- Protection against replay attacks

### 9.2 Fault Injection Protection
```json
{
  "fault_protection": {
    "watchdog": {
      "enabled": true,
      "timeout_ms": 500,
      "action": "reset"
    },
    "bounds_checking": {
      "enabled": true,
      "validate_all_inputs": true
    },
    "isolation": {
      "memory_protection": true,        // MPU/MMU
      "fault_containment": "app_level"
    }
  }
}
```

## 10. Network and Transport Configuration

### 10.1 Transport Layer Selection
```json
{
  "transports": [
    {
      "id": "uart1_link",
      "type": "UART",
      "from_node": "main_controller",
      "to_node": "sensor_node",
      "config": {
        "baudrate": 115200,
        "data_bits": 8,
        "parity": "none",
        "stop_bits": 1,
        "flow_control": "hardware"
      },
      "protocol": {
        "framing": "cobs",               // COBS, HDLC, length-prefix
        "max_frame_size": 256,
        "timeout_ms": 100
      },
      "bandwidth": {
        "total_bps": 115200,
        "allocated_pct": 80,             // Max 80% utilization
        "reserved_for": ["control", "safety"]
      }
    },
    {
      "id": "can_bus_0",
      "type": "CAN",
      "bitrate": 500000,
      "config": {
        "sample_point": 87.5,
        "sjw": 1,
        "propagation_delay_ns": 100
      },
      "protocol": {
        "addressing": "can_id_based",
        "priority_inversion": "prevent",
        "error_passive_limit": 127
      },
      "message_allocation": [
        {
          "can_id_range": "0x000-0x0FF",
          "reserved_for": "safety_critical"
        },
        {
          "can_id_range": "0x100-0x3FF",
          "reserved_for": "control"
        },
        {
          "can_id_range": "0x400-0x7FF",
          "reserved_for": "sensors"
        }
      ]
    }
  ]
}
```

### 10.2 Bandwidth Management
```json
{
  "bandwidth_allocation": {
    "bus": "can_bus_0",
    "total_bandwidth_bps": 500000,
    "overhead_pct": 20,                  // Protocol overhead
    "available_bps": 400000,
    "allocations": [
      {
        "topic": "motor/command",
        "rate_hz": 100,
        "message_size_bytes": 16,
        "bandwidth_bps": 12800,
        "utilization_pct": 3.2
      },
      {
        "topic": "sensors/imu",
        "rate_hz": 200,
        "message_size_bytes": 32,
        "bandwidth_bps": 51200,
        "utilization_pct": 12.8
      }
    ],
    "total_utilization_pct": 45.3,
    "warnings": {
      "over_utilization_threshold_pct": 80,
      "action": "reject_new_topics"
    }
  }
}
```

**Bandwidth Analysis:**
- Calculate required bandwidth per topic
- Include protocol overhead (framing, CRC, acks)
- Account for retransmissions
- Reserve margin for burstiness
- Validate against bus capacity

## 11. Deployment Configuration

### 11.1 Deployment Mapping
```json
{
  "deployment": {
    "targets": [
      {
        "node_id": "main_controller",
        "hardware": "stm32f407",
        "applications": [
          {
            "name": "MainController",
            "priority": 200,
            "stack_size": 8192
          },
          {
            "name": "DataLogger",
            "priority": 100,
            "stack_size": 16384
          }
        ],
        "resource_usage": {
          "flash_used_kb": 245,
          "flash_available_kb": 1024,
          "ram_used_kb": 67,
          "ram_available_kb": 192,
          "cpu_utilization_pct": 42
        },
        "validation": {
          "schedulability": "passed",
          "memory_check": "passed",
          "bandwidth_check": "passed"
        }
      }
    ]
  }
}
```

### 11.2 Build Configuration
```json
{
  "build": {
    "toolchain": "gcc-arm-none-eabi",
    "optimization": "O2",
    "debug_symbols": true,
    "static_analysis": true,
    "unit_tests": true,
    "targets": [
      {
        "node": "main_controller",
        "output": "build/main_controller.elf",
        "linker_script": "targets/stm32f407/linker.ld",
        "defines": ["STM32F407", "USE_HAL_DRIVER"],
        "includes": ["targets/stm32f407/include"]
      }
    ]
  }
}
```

### 11.3 Flash and Debug Configuration
```json
{
  "flash": {
    "programmer": "stlink",
    "interface": "swd",
    "speed_khz": 4000,
    "verify_after_flash": true,
    "targets": [
      {
        "node": "main_controller",
        "address": "0x08000000",
        "file": "build/main_controller.bin"
      }
    ]
  },
  "debug": {
    "enabled": true,
    "gdb_server": "openocd",
    "gdb_port": 3333,
    "telnet_port": 4444,
    "rtt": {
      "enabled": true,
      "buffer_size": 4096,
      "channels": ["console", "trace", "errors"]
    }
  }
}
```

## 12. Validation and Testing

### 12.1 Schedulability Analysis
```json
{
  "scheduling_validation": {
    "algorithm": "rate_monotonic",      // RM, EDF, priority-based
    "tasks": [
      {
        "name": "MotorController.Step",
        "period_ms": 10,
        "wcet_ms": 2,
        "priority": 200
      },
      {
        "name": "SensorReader.Step",
        "period_ms": 5,
        "wcet_ms": 1,
        "priority": 250
      }
    ],
    "cpu_utilization": 0.45,
    "schedulable": true,
    "worst_case_response_times": {
      "MotorController.Step": "3.2ms",
      "SensorReader.Step": "1.1ms"
    }
  }
}
```

### 12.2 Integration Testing
```json
{
  "integration_tests": {
    "scenarios": [
      {
        "name": "motor_control_loop",
        "description": "Verify end-to-end motor control",
        "steps": [
          {
            "action": "send_motor_command",
            "app": "MainController",
            "topic": "motor/command",
            "data": {"speed": 100}
          },
          {
            "action": "expect_message",
            "app": "MotorDriver",
            "topic": "motor/status",
            "timeout_ms": 50,
            "validate": "status.speed == 100"
          }
        ],
        "assertions": [
          "latency < 20ms",
          "no_message_loss",
          "crc_valid"
        ]
      }
    ]
  }
}
```

## 13. Configuration File Summary

### Recommended File Structure

```
Project/
├── lumos.json                 # Main project config
├── hardware.json              # Hardware topology
├── communication.json         # Communication topology
├── deployment.json            # Deployment mapping
├── qos.json                   # QoS profiles
├── security.json              # Security settings
├── logging.json               # Logging configuration
├── timing.json                # Timing and sync
└── validation.json            # Test scenarios
```

### Configuration Hierarchy

```
lumos.json (root)
  ├── Project metadata
  ├── References to:
  │   ├── hardware.json
  │   ├── communication.json
  │   ├── deployment.json
  │   └── ...
  └── Application list
      └── For each app:
          ├── app.json (app-specific config)
          └── Implemented in apps/<name>/
```

## 14. Key Design Questions

When defining a Lumos project, answer these questions:

### System Architecture
1. How many microcontrollers/nodes are in the system?
2. What is the role of each node (master/slave, sensor/actuator, gateway)?
3. How are nodes physically connected (UART, CAN, SPI, Ethernet)?
4. What is the data flow (who sends what to whom)?

### Timing Requirements
5. What are the sampling rates for each sensor/actuator?
6. What are the control loop rates?
7. What are the latency requirements (max acceptable delay)?
8. Do you need time synchronization between nodes? What precision?

### Communication
9. What topics/services are needed?
10. Which communication pattern (pub/sub, RPC, events)?
11. What QoS is needed (reliable vs. best-effort)?
12. What is the expected message rate and size?
13. What is the available bandwidth per link?

### Reliability
14. What level of data integrity is required (CRC-16, CRC-32)?
15. Do you need message ordering guarantees?
16. How should the system handle lost messages?
17. Do you need redundancy (duplicate sensors, failover)?

### Safety and Security
18. Is this a safety-critical system (ISO 26262, IEC 61508)?
19. Do you need end-to-end protection (AUTOSAR E2E)?
20. Do you need authentication/encryption?
21. What should happen on fault detection?

### Resource Constraints
22. What are the memory constraints (RAM, Flash)?
23. What is the available CPU budget per application?
24. What is the power budget (battery life)?

### Logging and Diagnostics
25. What data needs to be logged?
26. Where should logs be stored (SD card, network, serial)?
27. Do you need a black box recorder?
28. What diagnostic information is needed?

### Deployment
29. How will the system be flashed/updated?
30. Do you need over-the-air updates?
31. How will the system be debugged in the field?

## Conclusion

A complete Lumos project definition requires careful consideration of:

- **Structure**: Applications, nodes, topology
- **Communication**: Topics, QoS, transport
- **Timing**: Rates, sync, deadlines
- **Integrity**: CRC, validation, E2E protection
- **Logging**: What, where, when
- **Deployment**: Build, flash, debug
- **Validation**: Schedulability, bandwidth, testing

The configuration files should capture all these aspects in a declarative, analyzable format that can be validated at build time and enforced at runtime.

The Lumos CLI tool should help users:
1. Define these configurations interactively
2. Validate configurations for consistency
3. Generate deployment code
4. Analyze resource usage and schedulability
5. Simulate the system before deployment
