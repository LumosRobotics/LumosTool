#pragma once

#include <string>
#include <vector>
#include <set>

namespace Lumos {

/**
 * @brief Mapping structure for special headers to HAL modules
 *
 * This structure defines how non-standard headers (like USB middleware,
 * networking libraries, etc.) map to required HAL modules.
 */
struct HeaderToModuleMapping {
    std::string header_pattern;           // Header file name or pattern
    std::vector<std::string> modules;     // Required HAL modules
    std::string description;              // Human-readable description
    bool exact_match;                     // true = exact match, false = contains match
};

/**
 * @brief HAL Module Detector
 *
 * Automatically detects which HAL modules are needed by analyzing
 * #include directives in user source files.
 *
 * Detection works in two phases:
 * 1. Pattern-based: Standard HAL headers (stm32xxx_hal_<module>.h)
 * 2. Table-based: Special cases (USB, networking, filesystem, etc.)
 */
class HALModuleDetector {
public:
    HALModuleDetector();

    /**
     * @brief Detect required HAL modules from source files
     * @param source_files List of source file paths (relative to project)
     * @param project_dir Absolute path to project directory
     * @return Vector of detected HAL module names
     */
    std::vector<std::string> DetectModules(
        const std::vector<std::string>& source_files,
        const std::string& project_dir) const;

private:
    // Static mapping table for special cases
    static const std::vector<HeaderToModuleMapping> special_mappings_;

    /**
     * @brief Parse #include directives from a source file
     * @param file_path Path to source file
     * @return Vector of included header names
     */
    std::vector<std::string> ParseIncludesFromFile(const std::string& file_path) const;

    /**
     * @brief Detect modules from standard HAL headers using pattern matching
     * @param includes List of included headers
     * @return Set of detected module names
     */
    std::set<std::string> DetectFromStandardHALHeaders(
        const std::vector<std::string>& includes) const;

    /**
     * @brief Detect modules from special headers using mapping table
     * @param includes List of included headers
     * @return Set of detected module names
     */
    std::set<std::string> DetectFromSpecialHeaders(
        const std::vector<std::string>& includes) const;
};

} // namespace Lumos
