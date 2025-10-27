#include "builder.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

void PrintUsage() {
    std::cout << "Lumos - STM32 Build Tool" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: lumos <command> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  build           Build the project in current directory" << std::endl;
    std::cout << "  --help, -h      Show this help message" << std::endl;
    std::cout << "  --version, -v   Show version" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  cd my_project" << std::endl;
    std::cout << "  lumos build" << std::endl;
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

    if (command == "build") {
        // Get current directory as project directory
        fs::path current_dir = fs::current_path();
        std::string project_dir = current_dir.string();

        // Check if project.yaml exists
        fs::path yaml_path = current_dir / "project.yaml";
        if (!fs::exists(yaml_path)) {
            std::cerr << "Error: project.yaml not found in current directory" << std::endl;
            std::cerr << "Make sure you're in a Lumos project directory" << std::endl;
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

    std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
    std::cerr << std::endl;
    PrintUsage();
    return 1;
}
