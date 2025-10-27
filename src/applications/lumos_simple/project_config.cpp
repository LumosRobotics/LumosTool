#include "project_config.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Lumos {

bool ProjectConfig::Load(const std::string& yaml_path) {
    std::ifstream file(yaml_path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << yaml_path << std::endl;
        return false;
    }

    std::string line;
    bool in_sources = false;
    bool in_hal_modules = false;

    while (std::getline(file, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        if (line.empty() || line[0] == '#') continue;

        if (line.find("sources:") == 0) {
            in_sources = true;
            in_hal_modules = false;
            continue;
        }

        if (line.find("hal_modules:") == 0) {
            in_hal_modules = true;
            in_sources = false;
            continue;
        }

        if (line.find("board:") == 0) {
            in_sources = false;
            in_hal_modules = false;
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                board = line.substr(colon_pos + 1);
                // Trim whitespace
                size_t board_start = board.find_first_not_of(" \t");
                if (board_start != std::string::npos) {
                    board = board.substr(board_start);
                }
            }
            continue;
        }

        if (in_sources && line[0] == '-') {
            std::string source = line.substr(1);
            size_t source_start = source.find_first_not_of(" \t");
            if (source_start != std::string::npos) {
                sources.push_back(source.substr(source_start));
            }
        }

        if (in_hal_modules && line[0] == '-') {
            std::string module = line.substr(1);
            size_t module_start = module.find_first_not_of(" \t");
            if (module_start != std::string::npos) {
                hal_modules.push_back(module.substr(module_start));
            }
        }
    }

    return true;
}

BoardConfig BoardConfig::GetConfig(const std::string& board_name) {
    BoardConfig config;
    config.name = board_name;

    // Map board names to platforms
    if (board_name == "LumosBrain") {
        config.platform = "h7";
        config.mcu = "STM32H723xx";
        config.cpu = "cortex-m7";
        config.float_abi = "hard";
        config.fpu = "fpv5-d16";
    }
    // Add more boards as needed
    else {
        std::cerr << "Warning: Unknown board '" << board_name << "', defaulting to H7" << std::endl;
        config.platform = "h7";
        config.mcu = "STM32H723xx";
        config.cpu = "cortex-m7";
        config.float_abi = "hard";
        config.fpu = "fpv5-d16";
    }

    return config;
}

} // namespace Lumos
