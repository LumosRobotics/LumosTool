#include "builder.h"
#include "cache_config.h"
#include "serial.h"
#include "stm32_communicator.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <string>
#include <limits>
#include <csignal>
#include <thread>
#include <chrono>
#include <algorithm>

namespace fs = std::filesystem;

// Global flag for signal handling
static volatile bool g_running = true;

void SignalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nInterrupt received, stopping..." << std::endl;
        g_running = false;
    }
}

void PrintUsage() {
    std::cout << "Lumos - STM32 Build Tool" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: lumos <command> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  init               Initialize a new project in current directory" << std::endl;
    std::cout << "  build              Build the project in current directory" << std::endl;
    std::cout << "  flash [port]       Flash firmware to STM32 (auto-detects port if not specified)" << std::endl;
    std::cout << "  monitor [port]     Monitor serial output from MCU" << std::endl;
    std::cout << "  ports              List available serial ports" << std::endl;
    std::cout << "  --help, -h         Show this help message" << std::endl;
    std::cout << "  --version, -v      Show version" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  mkdir my_project && cd my_project" << std::endl;
    std::cout << "  lumos init" << std::endl;
    std::cout << "  lumos build" << std::endl;
    std::cout << "  lumos flash" << std::endl;
    std::cout << "  lumos monitor" << std::endl;
}

void PrintVersion() {
    std::cout << "Lumos v0.1.0" << std::endl;
}

std::string GetLumosRoot() {
    // Try to find LUMOS_ROOT from environment or deduce from executable path
    const char* lumos_root_env = std::getenv("LUMOS_ROOT");
    if (lumos_root_env) {
        return std::string(lumos_root_env);
    }

    // Default to a reasonable guess (can be improved)
    // For now, assume we're in build/src/applications/lumos_simple/
    // and LUMOS_ROOT is 4 directories up
    return "/Users/danielpi/work/LumosTool";
}

std::string Prompt(const std::string& question, const std::vector<std::string>& options, size_t default_index = 0) {
    std::cout << question << std::endl;
    for (size_t i = 0; i < options.size(); i++) {
        std::cout << "  " << (i + 1) << ". " << options[i];
        if (i == default_index) {
            std::cout << " (default)";
        }
        std::cout << std::endl;
    }
    std::cout << "Enter choice [1-" << options.size() << "]: ";

    std::string input;
    std::getline(std::cin, input);

    // If empty, use default
    if (input.empty()) {
        return options[default_index];
    }

    // Try to parse as number
    try {
        int choice = std::stoi(input);
        if (choice >= 1 && choice <= (int)options.size()) {
            return options[choice - 1];
        }
    } catch (...) {
        // Not a number, ignore
    }

    // Invalid input, use default
    std::cout << "Invalid choice, using default: " << options[default_index] << std::endl;
    return options[default_index];
}

/**
 * @brief Get serial port with caching support
 *
 * This function handles serial port selection with the following priority:
 * 1. Use command-line argument if provided
 * 2. Use cached port if valid (exists in current port list)
 * 3. Prompt user to select from available ports and cache the selection
 *
 * @param project_dir Project directory path
 * @param explicit_port Port specified on command line (empty if not specified)
 * @return Selected serial port name, or empty string on error
 */
std::string GetSerialPortWithCache(const fs::path& project_dir, const std::string& explicit_port = "") {
    fs::path build_dir = project_dir / "build";

    // If port explicitly specified, use it and update cache
    if (!explicit_port.empty()) {
        Lumos::CacheConfig cache;
        cache.SetSerialPort(explicit_port);
        cache.Save(build_dir);
        return explicit_port;
    }

    // Get list of available ports
    auto ports = SimpleSerial::Serial::ListPorts();
    if (ports.empty()) {
        std::cerr << "Error: No serial ports found" << std::endl;
        return "";
    }

    // Try to load cached port
    Lumos::CacheConfig cache;
    cache.Load(build_dir);

    // Check if cached port is still valid
    if (cache.HasSerialPort()) {
        std::string cached_port = cache.GetSerialPort();
        auto it = std::find(ports.begin(), ports.end(), cached_port);
        if (it != ports.end()) {
            std::cout << "Using cached port: " << cached_port << std::endl;
            return cached_port;
        } else {
            std::cout << "Cached port '" << cached_port << "' no longer available" << std::endl;
        }
    }

    // Prompt user to select a port
    std::string selected_port;

    if (ports.size() == 1) {
        selected_port = ports[0];
        std::cout << "Auto-selected port: " << selected_port << std::endl;
    } else {
        std::cout << "Available serial ports:" << std::endl;
        for (size_t i = 0; i < ports.size(); i++) {
            std::cout << "  " << (i + 1) << ". " << ports[i] << std::endl;
        }
        std::cout << "Enter choice [1-" << ports.size() << "]: ";
        std::string input;
        std::getline(std::cin, input);

        try {
            int choice = std::stoi(input);
            if (choice >= 1 && choice <= (int)ports.size()) {
                selected_port = ports[choice - 1];
            } else {
                std::cerr << "Invalid choice" << std::endl;
                return "";
            }
        } catch (...) {
            std::cerr << "Invalid input" << std::endl;
            return "";
        }
    }

    // Save selected port to cache
    cache.SetSerialPort(selected_port);
    if (cache.Save(build_dir)) {
        std::cout << "Port cached for future use" << std::endl;
    }

    return selected_port;
}

void GenerateMainFile(const std::string& language, const fs::path& project_dir) {
    std::string filename = (language == "C") ? "main.c" : "main.cpp";
    fs::path main_path = project_dir / filename;

    std::ofstream file(main_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create " + filename);
    }

    const std::string loop_declaration = (language == "C") ? "void loop(void)" : "void loop()";
    const std::string setup_declaration = (language == "C") ? "void setup(void)" : "void setup()";

    file << "/**\n";
    file << " * Main application file\n";
    file << " * This is where the setup() and loop() functions are defined\n";
    file << " */\n\n";
    file << "/**\n";
    file << " * Setup function - called once at startup\n";
    file << " */\n";
    file << setup_declaration << "\n";
    file << "{\n";
    file << "    // Initialize your application here\n";
    file << "    // - Configure GPIO pins\n";
    file << "    // - Initialize UART, SPI, I2C, etc.\n";
    file << "    // - Set up timers\n";
    file << "}\n\n";
    file << "/**\n";
    file << " * Loop function - called repeatedly\n";
    file << " */\n";
    file << loop_declaration << "\n";
    file << "{\n";
    file << "    // Your main application logic here\n";
    file << "    // This function runs continuously\n";
    file << "}\n";

    file.close();
}

void GenerateProjectYaml(const std::string& board, const std::string& language, const fs::path& project_dir) {
    fs::path yaml_path = project_dir / "project.yaml";

    std::ofstream file(yaml_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create project.yaml");
    }

    std::string source = (language == "C") ? "main.c" : "main.cpp";

    file << "# Lumos Project Configuration\n";
    file << "# Generated by: lumos init\n\n";
    file << "# Source files to compile\n";
    file << "sources:\n";
    file << "  - " << source << "\n\n";
    file << "# Target board\n";
    file << "board: " << board << "\n\n";
    file << "# Optional: HAL modules to include (auto-detected if not specified)\n";
    file << "# hal_modules:\n";
    file << "#   - uart\n";
    file << "#   - spi\n";
    file << "#   - i2c\n";

    file.close();
}

void GenerateReadme(const std::string& board, const std::string& language, const fs::path& project_dir) {
    fs::path readme_path = project_dir / "README.md";

    std::ofstream file(readme_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create README.md");
    }

    std::string project_name = project_dir.filename().string();

    file << "# " << project_name << "\n\n";
    file << "A Lumos project for " << board << ".\n\n";
    file << "## Configuration\n\n";
    file << "- **Board**: " << board << "\n";
    file << "- **Language**: " << language << "\n\n";
    file << "## Building\n\n";
    file << "```bash\n";
    file << "lumos build\n";
    file << "```\n\n";
    file << "## Flashing\n\n";
    file << "```bash\n";
    file << "st-flash write build/firmware.bin 0x8000000\n";
    file << "```\n\n";
    file << "## Project Structure\n\n";
    file << "- `" << (language == "C" ? "main.c" : "main.cpp") << "` - Main application code\n";
    file << "- `project.yaml` - Project configuration\n";
    file << "- `build/` - Build output directory (generated)\n";

    file.close();
}

int InitProject() {
    fs::path current_dir = fs::current_path();
    bool yaml_exists = fs::exists(current_dir / "project.yaml");

    // Check if project.yaml already exists
    if (yaml_exists) {
        // Project already initialized, only create main file if missing
        std::cout << "\n=== Lumos Project ===\n" << std::endl;
        std::cout << "Project directory: " << current_dir << std::endl;
        std::cout << "project.yaml already exists\n" << std::endl;

        // Check if main file exists
        bool main_c_exists = fs::exists(current_dir / "main.c");
        bool main_cpp_exists = fs::exists(current_dir / "main.cpp");

        if (main_c_exists || main_cpp_exists) {
            std::cout << "Main file already exists: " << (main_c_exists ? "main.c" : "main.cpp") << std::endl;
            std::cout << "\nProject is ready. Run 'lumos build' to compile." << std::endl;
            return 0;
        }

        // No main file, prompt for language and create it
        std::vector<std::string> languages = {"C++", "C"};
        std::string language = Prompt("Select programming language:", languages, 0);
        std::cout << std::endl;

        try {
            GenerateMainFile(language, current_dir);
            std::cout << "  Created " << (language == "C" ? "main.c" : "main.cpp") << std::endl;

            std::cout << "\n✓ Main file created successfully!" << std::endl;
            std::cout << "\nNext steps:" << std::endl;
            std::cout << "  1. Edit " << (language == "C" ? "main.c" : "main.cpp") << " to add your code" << std::endl;
            std::cout << "  2. Run 'lumos build' to compile" << std::endl;
            std::cout << "  3. Run 'lumos flash' to flash firmware to MCU" << std::endl;
            std::cout << std::endl;

            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

    // New project initialization
    std::cout << "\n=== Lumos Project Initialization ===\n" << std::endl;
    std::cout << "Creating project in: " << current_dir << "\n" << std::endl;

    // Prompt for board
    std::vector<std::string> boards = {"LumosBrain", "LumosMiniBrain", "LumosEscMini"};
    std::string board = Prompt("Select target board:", boards, 0);
    std::cout << std::endl;

    // Prompt for language
    std::vector<std::string> languages = {"C++", "C"};
    std::string language = Prompt("Select programming language:", languages, 0);
    std::cout << std::endl;

    try {
        // Generate files
        std::cout << "Generating project files..." << std::endl;
        GenerateMainFile(language, current_dir);
        std::cout << "  Created " << (language == "C" ? "main.c" : "main.cpp") << std::endl;

        GenerateProjectYaml(board, language, current_dir);
        std::cout << "  Created project.yaml" << std::endl;

        GenerateReadme(board, language, current_dir);
        std::cout << "  Created README.md" << std::endl;

        std::cout << "\n✓ Project initialized successfully!" << std::endl;
        std::cout << "\nNext steps:" << std::endl;
        std::cout << "  1. Edit " << (language == "C" ? "main.c" : "main.cpp") << " to add your code" << std::endl;
        std::cout << "  2. Run 'lumos build' to compile" << std::endl;
        std::cout << "  3. Run 'lumos flash' to flash firmware to MCU" << std::endl;
        std::cout << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
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

    if (command == "--version" || command == "-v") {
        PrintVersion();
        return 0;
    }

    if (command == "init") {
        return InitProject();
    }

    if (command == "build") {
        // Get current directory as project directory
        fs::path current_dir = fs::current_path();
        std::string project_dir = current_dir.string();

        // Check if project.yaml exists
        fs::path yaml_path = current_dir / "project.yaml";
        if (!fs::exists(yaml_path)) {
            std::cerr << "Error: project.yaml not found in current directory" << std::endl;
            std::cerr << "Make sure you're in a Lumos project directory" << std::endl;
            std::cerr << "Hint: Run 'lumos init' to create a new project" << std::endl;
            return 1;
        }

        // Get Lumos root
        std::string lumos_root = GetLumosRoot();
        std::cout << "Lumos Root: " << lumos_root << std::endl;
        std::cout << std::endl;

        // Create builder and build
        Lumos::Builder builder(lumos_root);
        bool success = builder.Build(project_dir);

        return success ? 0 : 1;
    }

    if (command == "ports") {
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

    if (command == "flash") {
        // Get current directory as project directory
        fs::path current_dir = fs::current_path();

        // Check if firmware.bin exists FIRST before port selection
        fs::path firmware_path = current_dir / "build" / "firmware.bin";
        if (!fs::exists(firmware_path)) {
            std::cerr << "Error: firmware.bin not found in build directory" << std::endl;
            std::cerr << "Run 'lumos build' first to compile the firmware" << std::endl;
            return 1;
        }

        // Read firmware file early to check if it's valid
        std::ifstream file(firmware_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Error: Failed to open firmware file" << std::endl;
            return 1;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> firmware_data(size);
        if (!file.read(reinterpret_cast<char*>(firmware_data.data()), size)) {
            std::cerr << "Error: Failed to read firmware file" << std::endl;
            return 1;
        }
        file.close();

        // Get port (from command line, cache, or prompt)
        std::string explicit_port = (argc >= 3) ? argv[2] : "";
        std::string port_name = GetSerialPortWithCache(current_dir, explicit_port);

        if (port_name.empty()) {
            return 1;
        }

        std::cout << "\nFlashing firmware..." << std::endl;
        std::cout << "  Firmware: " << firmware_path << std::endl;
        std::cout << "  Size: " << firmware_data.size() << " bytes" << std::endl;
        std::cout << "  Port: " << port_name << std::endl;
        std::cout << std::endl;

        // Connect and flash
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

        std::cout << "\n✓ Firmware flashed successfully!" << std::endl;
        comm.Disconnect();

        return 0;
    }

    if (command == "monitor") {
        // Get current directory as project directory
        fs::path current_dir = fs::current_path();

        // Get port (from command line, cache, or prompt)
        std::string explicit_port = (argc >= 3) ? argv[2] : "";
        std::string port_name = GetSerialPortWithCache(current_dir, explicit_port);

        if (port_name.empty()) {
            return 1;
        }

        // Get baud rate (default: 115200)
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

    std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
    std::cerr << std::endl;
    PrintUsage();
    return 1;
}
