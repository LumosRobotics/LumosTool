#pragma once

#include <string>
#include <vector>

namespace Lumos {

struct ProjectConfig {
    std::vector<std::string> sources;
    std::string board;
    std::vector<std::string> hal_modules;  // Optional: uart, spi, i2c, adc, etc.

    bool Load(const std::string& yaml_path);
};

struct BoardConfig {
    std::string name;
    std::string platform;  // f4, g0, g4, h7
    std::string mcu;       // STM32F407xx, etc.
    std::string cpu;       // cortex-m4, cortex-m0+, etc.
    std::string float_abi; // soft, hard
    std::string fpu;       // fpv4-sp-d16, etc.

    static BoardConfig GetConfig(const std::string& board_name);
};

} // namespace Lumos
