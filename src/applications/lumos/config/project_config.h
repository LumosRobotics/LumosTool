#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace Lumos {
namespace Config {

struct ApplicationConfig {
    std::string name;
    std::string target;
    uint32_t rate_hz;
    uint8_t priority;

    nlohmann::json ToJson() const;
    static ApplicationConfig FromJson(const nlohmann::json& j);
};

struct TransportConfig {
    std::string type;
    std::string from;
    std::string to;
    nlohmann::json config;

    nlohmann::json ToJson() const;
    static TransportConfig FromJson(const nlohmann::json& j);
};

struct ProjectInfo {
    std::string name;
    std::string version;

    nlohmann::json ToJson() const;
    static ProjectInfo FromJson(const nlohmann::json& j);
};

class ProjectConfig {
public:
    ProjectConfig();
    ProjectConfig(const std::string& name, const std::string& version = "1.0.0");

    bool Load(const std::string& filepath);
    bool Save(const std::string& filepath) const;

    void AddApplication(const ApplicationConfig& app);
    void RemoveApplication(const std::string& name);
    void AddInterface(const std::string& idl_path);
    void AddTransport(const TransportConfig& transport);

    const ProjectInfo& GetProjectInfo() const { return project_info_; }
    const std::vector<ApplicationConfig>& GetApplications() const { return applications_; }
    const std::vector<std::string>& GetInterfaces() const { return interfaces_; }
    const std::vector<TransportConfig>& GetTransports() const { return transports_; }

    void SetProjectInfo(const ProjectInfo& info) { project_info_ = info; }

private:
    ProjectInfo project_info_;
    std::vector<ApplicationConfig> applications_;
    std::vector<std::string> interfaces_;
    std::vector<TransportConfig> transports_;
};

} // namespace Config
} // namespace Lumos
