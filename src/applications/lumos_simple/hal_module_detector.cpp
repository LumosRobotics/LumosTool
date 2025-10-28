#include "hal_module_detector.h"
#include <fstream>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <iostream>

namespace Lumos {

// Static mapping table for special case headers
const std::vector<HeaderToModuleMapping> HALModuleDetector::special_mappings_ = {
    // === USB Device ===
    {"usbd_core.h", {"pcd"}, "USB Device Core", true},
    {"usbd_cdc.h", {"pcd"}, "USB CDC Class", true},
    {"usbd_cdc_if.h", {"pcd"}, "USB CDC Interface", true},
    {"usbd_msc.h", {"pcd"}, "USB MSC Class", true},
    {"usbd_hid.h", {"pcd"}, "USB HID Class", true},
    {"usbd_conf.h", {"pcd"}, "USB Device Config", true},
    {"usbd_desc.h", {"pcd"}, "USB Device Descriptors", true},

    // === USB Host ===
    {"usbh_core.h", {"hcd"}, "USB Host Core", true},
    {"usbh_def.h", {"hcd"}, "USB Host Definitions", true},
    {"usbh_conf.h", {"hcd"}, "USB Host Config", true},

    // === Ethernet/Network ===
    {"lwip", {"eth"}, "LWIP Network Stack", false},
    {"ethernetif.h", {"eth"}, "Ethernet Interface", true},

    // === Filesystem ===
    {"ff.h", {"sdmmc"}, "FatFs Filesystem", true},
    {"diskio.h", {"sdmmc"}, "FatFs Disk I/O", true},

    // === Graphics (might need both LTDC and DMA2D) ===
    {"ltdc", {"ltdc", "dma2d"}, "LCD-TFT Display Controller", false},

    // === FreeRTOS (often uses TIM for timebase) ===
    {"FreeRTOS.h", {"tim"}, "FreeRTOS RTOS", true},
    {"cmsis_os", {"tim"}, "CMSIS-RTOS", false},
};

HALModuleDetector::HALModuleDetector() {
}

std::vector<std::string> HALModuleDetector::DetectModules(
    const std::vector<std::string>& source_files,
    const std::string& project_dir) const
{
    std::set<std::string> detected_modules;
    std::vector<std::string> all_includes;

    // Step 1: Parse includes from all source files
    for (const auto& source : source_files) {
        std::string full_path = project_dir + "/" + source;
        auto includes = ParseIncludesFromFile(full_path);
        all_includes.insert(all_includes.end(), includes.begin(), includes.end());
    }

    // Step 2: Also parse includes from headers in include/ directory
    std::string include_dir = project_dir + "/include";
    std::error_code ec;
    if (std::filesystem::exists(include_dir, ec) && std::filesystem::is_directory(include_dir, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(include_dir, ec)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                // Parse .h and .hpp files (C++17 compatible check)
                if (filename.size() >= 2 &&
                    (filename.substr(filename.size() - 2) == ".h" ||
                     (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".hpp"))) {
                    auto includes = ParseIncludesFromFile(entry.path().string());
                    all_includes.insert(all_includes.end(), includes.begin(), includes.end());
                }
            }
        }
    }

    // Step 3: Detect from standard HAL headers (pattern-based)
    auto standard_modules = DetectFromStandardHALHeaders(all_includes);
    detected_modules.insert(standard_modules.begin(), standard_modules.end());

    // Step 4: Detect from special case mappings
    auto special_modules = DetectFromSpecialHeaders(all_includes);
    detected_modules.insert(special_modules.begin(), special_modules.end());

    // Step 5: Return sorted unique list
    std::vector<std::string> result(detected_modules.begin(), detected_modules.end());
    std::sort(result.begin(), result.end());

    return result;
}

std::vector<std::string> HALModuleDetector::ParseIncludesFromFile(const std::string& file_path) const {
    std::vector<std::string> includes;
    std::ifstream file(file_path);

    if (!file.is_open()) {
        return includes;
    }

    std::string line;
    // Regex to match #include directives: #include "header.h" or #include <header.h>
    std::regex include_pattern(R"(^\s*#\s*include\s+[<"]([^>"]+)[>"])");

    while (std::getline(file, line)) {
        std::smatch match;
        if (std::regex_search(line, match, include_pattern)) {
            if (match.size() > 1) {
                includes.push_back(match[1].str());
            }
        }
    }

    return includes;
}

std::set<std::string> HALModuleDetector::DetectFromStandardHALHeaders(
    const std::vector<std::string>& includes) const
{
    std::set<std::string> modules;

    // Pattern: stm32{platform}_hal_{module}.h
    // Example: stm32h7xx_hal_uart.h → uart
    std::regex hal_pattern(R"(stm32[a-z0-9]+_hal_([a-z0-9_]+)\.h)");

    for (const auto& include : includes) {
        std::smatch match;
        if (std::regex_search(include, match, hal_pattern)) {
            if (match.size() > 1) {
                std::string module = match[1].str();

                // Filter out base files that aren't actual modules
                if (module != "hal" &&
                    module != "def" &&
                    module != "conf" &&
                    module.find("_ex") == std::string::npos) {  // Skip _ex, they're added automatically
                    modules.insert(module);
                }
            }
        }
    }

    return modules;
}

std::set<std::string> HALModuleDetector::DetectFromSpecialHeaders(
    const std::vector<std::string>& includes) const
{
    std::set<std::string> modules;

    for (const auto& include : includes) {
        for (const auto& mapping : special_mappings_) {
            bool match = false;

            if (mapping.exact_match) {
                // Exact match
                match = (include == mapping.header_pattern);
            } else {
                // Contains match
                match = (include.find(mapping.header_pattern) != std::string::npos);
            }

            if (match) {
                // Add all required modules for this header
                for (const auto& module : mapping.modules) {
                    modules.insert(module);
                }
            }
        }
    }

    return modules;
}

} // namespace Lumos
