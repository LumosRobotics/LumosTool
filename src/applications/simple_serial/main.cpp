#include "serial.h"
#include <iostream>
#include <string>

void PrintUsage() {
    std::cout << "Simple Serial - Serial Port Communication Tool" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: simple_serial <command> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  list                List available serial ports" << std::endl;
    std::cout << "  test <port>         Test serial port connection" << std::endl;
    std::cout << "  reset <port>        Pulse DTR to reset MCU" << std::endl;
    std::cout << "  --help, -h          Show this help message" << std::endl;
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

    std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
    std::cerr << std::endl;
    PrintUsage();
    return 1;
}
