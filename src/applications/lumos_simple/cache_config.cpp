#include "cache_config.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace Lumos {

CacheConfig::CacheConfig()
    : serial_port_("")
{
}

bool CacheConfig::Load(const fs::path& build_dir) {
    fs::path cache_path = build_dir / "cache.yaml";

    if (!fs::exists(cache_path)) {
        return false;
    }

    try {
        YAML::Node cache = YAML::LoadFile(cache_path.string());

        if (cache["serial_port"]) {
            serial_port_ = cache["serial_port"].as<std::string>();
        }

        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "Warning: Failed to load cache.yaml: " << e.what() << std::endl;
        return false;
    }
}

bool CacheConfig::Save(const fs::path& build_dir) {
    // Ensure build directory exists
    if (!fs::exists(build_dir)) {
        try {
            fs::create_directories(build_dir);
        } catch (const std::exception& e) {
            std::cerr << "Error: Failed to create build directory: " << e.what() << std::endl;
            return false;
        }
    }

    fs::path cache_path = build_dir / "cache.yaml";

    try {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Comment("Lumos Cache - Non-persistent project settings");
        out << YAML::Comment("This file is auto-generated and not meant to be version controlled");
        out << YAML::Newline;

        if (!serial_port_.empty()) {
            out << YAML::Key << "serial_port";
            out << YAML::Value << serial_port_;
            out << YAML::Comment("Last used serial port");
        }

        out << YAML::EndMap;

        std::ofstream fout(cache_path);
        if (!fout.is_open()) {
            std::cerr << "Error: Failed to open cache.yaml for writing" << std::endl;
            return false;
        }

        fout << out.c_str() << std::endl;
        fout.close();

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to save cache.yaml: " << e.what() << std::endl;
        return false;
    }
}

} // namespace Lumos
