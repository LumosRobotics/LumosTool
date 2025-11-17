#include "serial.h"
#include "stm32_communicator.h"
#include <iostream>
#include <string>
#include <fstream>
#include <csignal>
#include <thread>
#include <chrono>

// Global flag for signal handling
static volatile bool g_running = true;

void SignalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nInterrupt received, stopping..." << std::endl;
        g_running = false;
    }
}

void PrintUsage() {
    std::cout << "Simple Serial - Serial Port Communication Tool" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: simple_serial <command> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  list                        List available serial ports" << std::endl;
    std::cout << "  test <port>                 Test serial port connection" << std::endl;
    std::cout << "  reset <port>                Pulse DTR to reset MCU" << std::endl;
    std::cout << "  monitor <port> [baud]       Monitor serial communication (default: 115200)" << std::endl;
    std::cout << "  bootloader <port>           Enter STM32 bootloader mode" << std::endl;
    std::cout << "  flash <port> <binary_file>  Flash firmware to STM32" << std::endl;
    std::cout << "  --help, -h                  Show this help message" << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        PrintUsage();
        return 0;
    }

    std::string command = argv[1];

    if (command == "--help" || command == "-h") {
        PrintUsage();
        return 0;
    }

    if (command == "list") {
        std::cout << "Scanning for serial ports..." << std::endl;
        auto ports = SimpleSerial::Serial::ListPorts();

        if (ports.empty()) {
            std::cout << "No serial ports found." << std::endl;
        } else {
            std::cout << "Available serial ports:" << std::endl;
            for (const auto& port : ports) {
                std::cout << "  " << port << std::endl;
            }
        }
        return 0;
    }

    if (command == "test") {
        if (argc < 3) {
            std::cerr << "Error: Port name required for test command" << std::endl;
            std::cerr << "Usage: simple_serial test <port>" << std::endl;
            return 1;
        }

        std::string port_name = argv[2];

        std::cout << "Testing serial port: " << port_name << std::endl;

        SimpleSerial::Serial serial;
        SimpleSerial::SerialConfig config;
        config.baud_rate = 115200;
        config.timeout_ms = 1000;

        if (!serial.Open(port_name, config)) {
            std::cerr << "Failed to open port: " << serial.GetLastError() << std::endl;
            return 1;
        }

        std::cout << "Port opened successfully!" << std::endl;
        std::cout << "Configuration:" << std::endl;
        std::cout << "  Baud rate: " << config.baud_rate << std::endl;
        std::cout << "  Data bits: " << config.data_bits << std::endl;
        std::cout << "  Stop bits: " << config.stop_bits << std::endl;
        std::cout << "  Parity: " << config.parity << std::endl;

        // Check available bytes
        int available = serial.Available();
        if (available > 0) {
            std::cout << "Bytes available: " << available << std::endl;
        }

        serial.Close();
        std::cout << "Port closed." << std::endl;

        return 0;
    }

    if (command == "reset") {
        if (argc < 3) {
            std::cerr << "Error: Port name required for reset command" << std::endl;
            std::cerr << "Usage: simple_serial reset <port>" << std::endl;
            return 1;
        }

        std::string port_name = argv[2];

        std::cout << "Opening serial port: " << port_name << std::endl;

        SimpleSerial::Serial serial;
        SimpleSerial::SerialConfig config;
        config.baud_rate = 115200;
        config.timeout_ms = 1000;

        if (!serial.Open(port_name, config)) {
            std::cerr << "Failed to open port: " << serial.GetLastError() << std::endl;
            return 1;
        }

        std::cout << "Port opened successfully!" << std::endl;
        std::cout << "Pulsing DTR to reset MCU..." << std::endl;

        // Pulse DTR with 100ms duration, active-low (typical for reset)
        if (!serial.PulseDTR(100, true)) {
            std::cerr << "Failed to pulse DTR: " << serial.GetLastError() << std::endl;
            serial.Close();
            return 1;
        }

        std::cout << "DTR pulsed successfully!" << std::endl;
        std::cout << "MCU should be reset now." << std::endl;

        serial.Close();
        std::cout << "Port closed." << std::endl;

        return 0;
    }

    if (command == "monitor") {
        if (argc < 3) {
            std::cerr << "Error: Port name required for monitor command" << std::endl;
            std::cerr << "Usage: simple_serial monitor <port> [baud_rate]" << std::endl;
            return 1;
        }

        std::string port_name = argv[2];
        int baud_rate = 115200;

        if (argc >= 4) {
            baud_rate = std::stoi(argv[3]);
        }

        std::cout << "Opening port: " << port_name << " at " << baud_rate << " baud" << std::endl;

        SimpleSerial::STM32Communicator comm;
        if (!comm.Connect(port_name, baud_rate)) {
            std::cerr << "Failed to connect: " << comm.GetLastError() << std::endl;
            return 1;
        }

        std::cout << "Connected! Monitoring serial data (Press Ctrl+C to exit)..." << std::endl;
        std::cout << "-----------------------------------------------------------" << std::endl;

        // Set up signal handler
        signal(SIGINT, SignalHandler);

        // Start monitoring
        if (!comm.StartMonitoring()) {
            std::cerr << "Failed to start monitoring: " << comm.GetLastError() << std::endl;
            return 1;
        }

        // Wait until interrupted
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        comm.StopMonitoring();
        comm.Disconnect();
        std::cout << "\nMonitoring stopped." << std::endl;

        return 0;
    }

    if (command == "bootloader") {
        if (argc < 3) {
            std::cerr << "Error: Port name required for bootloader command" << std::endl;
            std::cerr << "Usage: simple_serial bootloader <port>" << std::endl;
            return 1;
        }

        std::string port_name = argv[2];

        std::cout << "Connecting to: " << port_name << std::endl;

        SimpleSerial::STM32Communicator comm;
        if (!comm.Connect(port_name, 115200)) {
            std::cerr << "Failed to connect: " << comm.GetLastError() << std::endl;
            return 1;
        }

        std::cout << "Entering bootloader mode..." << std::endl;

        if (!comm.EnterBootloader(true)) {
            std::cerr << "Failed to enter bootloader: " << comm.GetLastError() << std::endl;
            comm.Disconnect();
            return 1;
        }

        std::cout << "Successfully entered bootloader mode!" << std::endl;
        comm.Disconnect();

        return 0;
    }

    if (command == "flash") {
        if (argc < 4) {
            std::cerr << "Error: Port name and binary file required for flash command" << std::endl;
            std::cerr << "Usage: simple_serial flash <port> <binary_file>" << std::endl;
            return 1;
        }

        std::string port_name = argv[2];
        std::string binary_file = argv[3];

        // Read binary file
        std::ifstream file(binary_file, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Error: Failed to open binary file: " << binary_file << std::endl;
            return 1;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> firmware_data(size);
        if (!file.read(reinterpret_cast<char*>(firmware_data.data()), size)) {
            std::cerr << "Error: Failed to read binary file" << std::endl;
            return 1;
        }
        file.close();

        std::cout << "Loaded " << firmware_data.size() << " bytes from " << binary_file << std::endl;
        std::cout << "Connecting to: " << port_name << std::endl;

        SimpleSerial::STM32Communicator comm;
        if (!comm.Connect(port_name, 115200)) {
            std::cerr << "Failed to connect: " << comm.GetLastError() << std::endl;
            return 1;
        }

        std::cout << "Entering bootloader mode..." << std::endl;
        if (!comm.EnterBootloader(true)) {
            std::cerr << "Failed to enter bootloader: " << comm.GetLastError() << std::endl;
            comm.Disconnect();
            return 1;
        }

        std::cout << "Bootloader ready!" << std::endl;

        // Prepare firmware data
        SimpleSerial::FirmwareData firmware;
        firmware.start_address = 0x08000000;  // STM32 flash start address
        firmware.data = firmware_data;

        // Flash the firmware
        if (!comm.Flash(firmware, true)) {
            std::cerr << "Failed to flash firmware: " << comm.GetLastError() << std::endl;
            comm.Disconnect();
            return 1;
        }

        std::cout << "Firmware flashed successfully!" << std::endl;
        comm.Disconnect();

        return 0;
    }

    std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
    std::cerr << std::endl;
    PrintUsage();
    return 1;
}
