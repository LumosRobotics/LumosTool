#pragma once

#include <string>
#include <filesystem>

namespace Lumos {

/**
 * @brief Cache configuration for project-specific settings
 *
 * Stores non-persistent settings like the last used serial port.
 * Cache is stored in build/cache.yaml and is not meant to be
 * version controlled or shared across different machines.
 */
class CacheConfig {
public:
    CacheConfig();

    /**
     * @brief Load cache from build/cache.yaml
     * @param build_dir Path to the build directory
     * @return true if loaded successfully, false otherwise (file may not exist yet)
     */
    bool Load(const std::filesystem::path& build_dir);

    /**
     * @brief Save cache to build/cache.yaml
     * @param build_dir Path to the build directory
     * @return true if saved successfully, false otherwise
     */
    bool Save(const std::filesystem::path& build_dir);

    /**
     * @brief Get the cached serial port
     * @return Serial port name, or empty string if not set
     */
    std::string GetSerialPort() const { return serial_port_; }

    /**
     * @brief Set the serial port to cache
     * @param port Serial port name
     */
    void SetSerialPort(const std::string& port) { serial_port_ = port; }

    /**
     * @brief Check if a serial port is cached
     * @return true if a port is cached, false otherwise
     */
    bool HasSerialPort() const { return !serial_port_.empty(); }

private:
    std::string serial_port_;
};

} // namespace Lumos
