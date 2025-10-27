#include "project_config.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace Lumos {
namespace Config {

// ApplicationConfig Implementation
json ApplicationConfig::ToJson() const {
    return json{
        {"name", name},
        {"target", target},
        {"rate_hz", rate_hz},
        {"priority", priority}
    };
}

ApplicationConfig ApplicationConfig::FromJson(const json& j) {
    ApplicationConfig config;
    config.name = j.value("name", "");
    config.target = j.value("target", "");
    config.rate_hz = j.value("rate_hz", 10);
    config.priority = j.value("priority", 5);
    return config;
}

// TransportConfig Implementation
json TransportConfig::ToJson() const {
    return json{
        {"type", type},
        {"from", from},
        {"to", to},
        {"config", config}
    };
}

TransportConfig TransportConfig::FromJson(const json& j) {
    TransportConfig config;
    config.type = j.value("type", "");
    config.from = j.value("from", "");
    config.to = j.value("to", "");
    config.config = j.value("config", json::object());
    return config;
}

// ProjectInfo Implementation
json ProjectInfo::ToJson() const {
    return json{
        {"name", name},
        {"version", version}
    };
}

ProjectInfo ProjectInfo::FromJson(const json& j) {
    ProjectInfo info;
    info.name = j.value("name", "");
    info.version = j.value("version", "1.0.0");
    return info;
}

// ProjectConfig Implementation
ProjectConfig::ProjectConfig() {
    project_info_.name = "UnnamedProject";
    project_info_.version = "1.0.0";
}

ProjectConfig::ProjectConfig(const std::string& name, const std::string& version) {
    project_info_.name = name;
    project_info_.version = version;
}

bool ProjectConfig::Load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << filepath << std::endl;
        return false;
    }

    try {
        json j;
        file >> j;

        // Parse project info
        if (j.contains("project")) {
            project_info_ = ProjectInfo::FromJson(j["project"]);
        }

        // Parse applications
        applications_.clear();
        if (j.contains("applications")) {
            for (const auto& app_json : j["applications"]) {
                applications_.push_back(ApplicationConfig::FromJson(app_json));
            }
        }

        // Parse interfaces
        interfaces_.clear();
        if (j.contains("interfaces")) {
            for (const auto& iface : j["interfaces"]) {
                interfaces_.push_back(iface.get<std::string>());
            }
        }

        // Parse transports
        transports_.clear();
        if (j.contains("transports")) {
            for (const auto& transport_json : j["transports"]) {
                transports_.push_back(TransportConfig::FromJson(transport_json));
            }
        }

        return true;
    } catch (const json::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return false;
    }
}

bool ProjectConfig::Save(const std::string& filepath) const {
    json j;

    // Serialize project info
    j["project"] = project_info_.ToJson();

    // Serialize applications
    json apps_json = json::array();
    for (const auto& app : applications_) {
        apps_json.push_back(app.ToJson());
    }
    j["applications"] = apps_json;

    // Serialize interfaces
    j["interfaces"] = interfaces_;

    // Serialize transports
    json transports_json = json::array();
    for (const auto& transport : transports_) {
        transports_json.push_back(transport.ToJson());
    }
    j["transports"] = transports_json;

    // Write to file
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not create config file: " << filepath << std::endl;
        return false;
    }

    file << j.dump(2) << std::endl;
    return true;
}

void ProjectConfig::AddApplication(const ApplicationConfig& app) {
    // Check if application already exists
    for (auto& existing_app : applications_) {
        if (existing_app.name == app.name) {
            existing_app = app;
            return;
        }
    }
    applications_.push_back(app);
}

void ProjectConfig::RemoveApplication(const std::string& name) {
    applications_.erase(
        std::remove_if(applications_.begin(), applications_.end(),
                      [&name](const ApplicationConfig& app) { return app.name == name; }),
        applications_.end()
    );
}

void ProjectConfig::AddInterface(const std::string& idl_path) {
    // Check if interface already exists
    for (const auto& iface : interfaces_) {
        if (iface == idl_path) {
            return;
        }
    }
    interfaces_.push_back(idl_path);
}

void ProjectConfig::AddTransport(const TransportConfig& transport) {
    transports_.push_back(transport);
}

} // namespace Config
} // namespace Lumos
