#include "project_config.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

namespace Lumos {

bool ProjectConfig::Load(const std::string& yaml_path) {
    try {
        YAML::Node config = YAML::LoadFile(yaml_path);

        // Load sources
        if (config["sources"]) {
            sources = config["sources"].as<std::vector<std::string>>();
        } else {
            std::cerr << "Error: 'sources' field not found in " << yaml_path << std::endl;
            return false;
        }

        // Load board
        if (config["board"]) {
            board = config["board"].as<std::string>();
        } else {
            std::cerr << "Error: 'board' field not found in " << yaml_path << std::endl;
            return false;
        }

        // Load hal_modules (optional)
        if (config["hal_modules"]) {
            hal_modules = config["hal_modules"].as<std::vector<std::string>>();
        }

        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "Error parsing " << yaml_path << ": " << e.what() << std::endl;
        return false;
    }
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
