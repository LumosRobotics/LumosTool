#include "app_command.h"
#include "../config/project_config.h"
#include <iostream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace Lumos {
namespace Commands {

AppCommand::AppCommand() {
    CLI::CommandRegistry::Instance().RegisterCommand(this);
}

std::string AppCommand::GetUsage() const {
    return R"(Usage: lumos app <subcommand> [options] [args]

Subcommands:
  create <name>    Create a new application
  list             List all applications in the project
  remove <name>    Remove an application from the project

Options:
  --target <mcu>   Target microcontroller (default: host)
  --rate <hz>      Update rate in Hz (default: 10)
  --priority <n>   Priority level (default: 5)
  --help           Show this help message

Examples:
  lumos app create MotorController
  lumos app create SensorReader --target stm32f407 --rate 100
  lumos app list
  lumos app remove OldApp
)";
}

void AppCommand::PrintHelp() {
    std::cout << GetUsage() << std::endl;
}

int AppCommand::Execute(const CLI::CommandContext& ctx) {
    if (ctx.args.empty()) {
        std::cerr << "Error: No subcommand specified\n" << std::endl;
        PrintHelp();
        return 1;
    }

    const std::string& subcommand = ctx.args[0];

    if (subcommand == "create") {
        return CreateApp(ctx);
    } else if (subcommand == "list") {
        return ListApps(ctx);
    } else if (subcommand == "remove") {
        return RemoveApp(ctx);
    } else if (subcommand == "--help" || subcommand == "-h") {
        PrintHelp();
        return 0;
    } else {
        std::cerr << "Error: Unknown subcommand '" << subcommand << "'\n" << std::endl;
        PrintHelp();
        return 1;
    }
}

int AppCommand::CreateApp(const CLI::CommandContext& ctx) {
    if (ctx.args.size() < 2) {
        std::cerr << "Error: Application name not specified" << std::endl;
        std::cerr << "Usage: lumos app create <name>" << std::endl;
        return 1;
    }

    const std::string& app_name = ctx.args[1];

    // Check if we're in a Lumos project directory
    fs::path config_path = fs::path(ctx.working_directory) / "lumos.json";

    if (!fs::exists(config_path)) {
        std::cerr << "Error: Not in a Lumos project directory (lumos.json not found)" << std::endl;
        std::cerr << "Run this command from the root of a Lumos project." << std::endl;
        return 1;
    }

    // Load project configuration
    Config::ProjectConfig project_config;
    if (!project_config.Load(config_path.string())) {
        std::cerr << "Error: Failed to load project configuration" << std::endl;
        return 1;
    }

    std::cout << "Creating application '" << app_name << "'..." << std::endl;

    // Create application directory
    fs::path app_path = fs::path(ctx.working_directory) / "apps" / app_name;

    if (fs::exists(app_path)) {
        std::cerr << "Error: Application '" << app_name << "' already exists" << std::endl;
        return 1;
    }

    try {
        // Create directory structure
        fs::create_directories(app_path);
        fs::create_directories(app_path / "src");
        fs::create_directories(app_path / "include");

        std::cout << "  Created directory structure" << std::endl;

        // Create app.json configuration
        std::ofstream app_config_file(app_path / "app.json");
        app_config_file << "{\n";
        app_config_file << "  \"name\": \"" << app_name << "\",\n";
        app_config_file << "  \"type\": \"application\",\n";
        app_config_file << "  \"provides\": [],\n";
        app_config_file << "  \"requires\": [],\n";
        app_config_file << "  \"modules\": [\"logging\"],\n";
        app_config_file << "  \"memory\": {\n";
        app_config_file << "    \"stack_size\": 4096,\n";
        app_config_file << "    \"heap_size\": 8192\n";
        app_config_file << "  }\n";
        app_config_file << "}\n";
        app_config_file.close();

        std::cout << "  Created app.json" << std::endl;

        // Create main application header
        std::ofstream header_file(app_path / "include" / (app_name + ".h"));
        header_file << "#pragma once\n\n";
        header_file << "#include <framework/application.h>\n\n";
        header_file << "namespace " << app_name << " {\n\n";
        header_file << "class " << app_name << "App : public Lumos::ApplicationBase {\n";
        header_file << "public:\n";
        header_file << "    " << app_name << "App();\n";
        header_file << "    ~" << app_name << "App() override = default;\n\n";
        header_file << "    // Lifecycle methods - implement your application logic here\n";
        header_file << "    void Init() override;\n";
        header_file << "    void Step() override;\n";
        header_file << "    void DeInit() override;\n\n";
        header_file << "private:\n";
        header_file << "    // Add your private members here\n";
        header_file << "    // Example:\n";
        header_file << "    // int counter_;\n";
        header_file << "};\n\n";
        header_file << "} // namespace " << app_name << "\n";
        header_file.close();

        std::cout << "  Created " << app_name << ".h" << std::endl;

        // Create main application source
        std::ofstream source_file(app_path / "src" / (app_name + ".cpp"));
        source_file << "#include \"" << app_name << ".h\"\n\n";
        source_file << "namespace " << app_name << " {\n\n";
        source_file << app_name << "App::" << app_name << "App()\n";
        source_file << "    : Lumos::ApplicationBase(\"" << app_name << "\", \"1.0.0\")\n";
        source_file << "{\n";
        source_file << "    // Constructor - set application metadata here if needed\n";
        source_file << "    // SetUpdateRate(10);  // Run at 10 Hz\n";
        source_file << "    // SetPriority(128);   // Medium priority\n";
        source_file << "}\n\n";
        source_file << "void " << app_name << "App::Init() {\n";
        source_file << "    // Called once when the application starts\n";
        source_file << "    // Initialize your hardware, allocate resources, etc.\n";
        source_file << "    LogInfo(\"Initializing...\");\n\n";
        source_file << "    // TODO: Add your initialization code here\n";
        source_file << "}\n\n";
        source_file << "void " << app_name << "App::Step() {\n";
        source_file << "    // Called repeatedly at the configured rate (default: 10 Hz)\n";
        source_file << "    // This is your main application loop\n\n";
        source_file << "    // TODO: Add your application logic here\n";
        source_file << "    // Example:\n";
        source_file << "    // LogInfo(\"Step \" + std::to_string(GetStats().step_count));\n";
        source_file << "}\n\n";
        source_file << "void " << app_name << "App::DeInit() {\n";
        source_file << "    // Called once when the application is shutting down\n";
        source_file << "    // Clean up resources, close connections, etc.\n";
        source_file << "    LogInfo(\"Shutting down...\");\n\n";
        source_file << "    // TODO: Add your cleanup code here\n";
        source_file << "}\n\n";
        source_file << "} // namespace " << app_name << "\n";
        source_file.close();

        std::cout << "  Created " << app_name << ".cpp" << std::endl;

        // Create CMakeLists.txt for the application
        std::ofstream cmake_file(app_path / "CMakeLists.txt");
        cmake_file << "# " << app_name << " Application\n\n";
        cmake_file << "set(APP_NAME " << app_name << ")\n\n";
        cmake_file << "set(APP_SOURCES\n";
        cmake_file << "    src/" << app_name << ".cpp\n";
        cmake_file << ")\n\n";
        cmake_file << "set(APP_HEADERS\n";
        cmake_file << "    include/" << app_name << ".h\n";
        cmake_file << ")\n\n";
        cmake_file << "add_library(${APP_NAME} STATIC ${APP_SOURCES} ${APP_HEADERS})\n\n";
        cmake_file << "target_include_directories(${APP_NAME}\n";
        cmake_file << "    PUBLIC\n";
        cmake_file << "        ${CMAKE_CURRENT_SOURCE_DIR}/include\n";
        cmake_file << "    PRIVATE\n";
        cmake_file << "        ${CMAKE_CURRENT_SOURCE_DIR}/src\n";
        cmake_file << ")\n\n";
        cmake_file << "# Link against Lumos framework\n";
        cmake_file << "# target_link_libraries(${APP_NAME} LumosFramework)\n";
        cmake_file.close();

        std::cout << "  Created CMakeLists.txt" << std::endl;

        // Add application to project configuration
        Config::ApplicationConfig app_config;
        app_config.name = app_name;
        app_config.target = "host"; // Default to host target
        app_config.rate_hz = 10;    // Default rate
        app_config.priority = 5;    // Default priority

        project_config.AddApplication(app_config);
        project_config.Save(config_path.string());

        std::cout << "  Added to project configuration" << std::endl;

        std::cout << "\nApplication '" << app_name << "' created successfully!" << std::endl;
        std::cout << "\nImplement your application logic in:" << std::endl;
        std::cout << "  apps/" << app_name << "/src/" << app_name << ".cpp" << std::endl;

        return 0;

    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error creating application: " << e.what() << std::endl;
        return 1;
    }
}

int AppCommand::ListApps(const CLI::CommandContext& ctx) {
    // Check if we're in a Lumos project directory
    fs::path config_path = fs::path(ctx.working_directory) / "lumos.json";

    if (!fs::exists(config_path)) {
        std::cerr << "Error: Not in a Lumos project directory (lumos.json not found)" << std::endl;
        return 1;
    }

    // Load project configuration
    Config::ProjectConfig project_config;
    if (!project_config.Load(config_path.string())) {
        std::cerr << "Error: Failed to load project configuration" << std::endl;
        return 1;
    }

    const auto& apps = project_config.GetApplications();

    if (apps.empty()) {
        std::cout << "No applications in this project" << std::endl;
        std::cout << "Create one with: lumos app create <name>" << std::endl;
        return 0;
    }

    std::cout << "Applications in project '" << project_config.GetProjectInfo().name << "':\n" << std::endl;
    std::cout << "Name                  Target         Rate (Hz)  Priority" << std::endl;
    std::cout << "------------------------------------------------------------" << std::endl;

    for (const auto& app : apps) {
        std::cout << std::left;
        std::cout.width(22);
        std::cout << app.name;
        std::cout.width(15);
        std::cout << app.target;
        std::cout.width(11);
        std::cout << app.rate_hz;
        std::cout << static_cast<int>(app.priority) << std::endl;
    }

    return 0;
}

int AppCommand::RemoveApp(const CLI::CommandContext& ctx) {
    if (ctx.args.size() < 2) {
        std::cerr << "Error: Application name not specified" << std::endl;
        std::cerr << "Usage: lumos app remove <name>" << std::endl;
        return 1;
    }

    const std::string& app_name = ctx.args[1];

    // Check if we're in a Lumos project directory
    fs::path config_path = fs::path(ctx.working_directory) / "lumos.json";

    if (!fs::exists(config_path)) {
        std::cerr << "Error: Not in a Lumos project directory (lumos.json not found)" << std::endl;
        return 1;
    }

    // Load project configuration
    Config::ProjectConfig project_config;
    if (!project_config.Load(config_path.string())) {
        std::cerr << "Error: Failed to load project configuration" << std::endl;
        return 1;
    }

    std::cout << "Removing application '" << app_name << "'..." << std::endl;

    // Remove from configuration
    project_config.RemoveApplication(app_name);
    project_config.Save(config_path.string());

    std::cout << "  Removed from project configuration" << std::endl;

    // Optionally remove directory
    fs::path app_path = fs::path(ctx.working_directory) / "apps" / app_name;

    if (fs::exists(app_path)) {
        std::cout << "\nApplication directory still exists at: apps/" << app_name << std::endl;
        std::cout << "Remove manually if desired: rm -rf apps/" << app_name << std::endl;
    }

    std::cout << "\nApplication '" << app_name << "' removed from project" << std::endl;

    return 0;
}

} // namespace Commands
} // namespace Lumos
