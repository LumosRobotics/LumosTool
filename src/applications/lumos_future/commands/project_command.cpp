#include "project_command.h"
#include "../config/project_config.h"
#include <iostream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace Lumos {
namespace Commands {

ProjectCommand::ProjectCommand() {
    CLI::CommandRegistry::Instance().RegisterCommand(this);
}

std::string ProjectCommand::GetUsage() const {
    return R"(Usage: lumos project <subcommand> [options] [args]

Subcommands:
  create <name>    Create a new Lumos project
  build            Build the current project
  clean            Clean build artifacts
  list             List all projects in workspace

Options:
  --target <mcu>   Specify target microcontroller
  --help           Show this help message

Examples:
  lumos project create MyRobot
  lumos project build --target stm32f407
  lumos project clean
)";
}

void ProjectCommand::PrintHelp() {
    std::cout << GetUsage() << std::endl;
}

int ProjectCommand::Execute(const CLI::CommandContext& ctx) {
    if (ctx.args.empty()) {
        std::cerr << "Error: No subcommand specified\n" << std::endl;
        PrintHelp();
        return 1;
    }

    const std::string& subcommand = ctx.args[0];

    if (subcommand == "create") {
        return CreateProject(ctx);
    } else if (subcommand == "build") {
        return BuildProject(ctx);
    } else if (subcommand == "clean") {
        return CleanProject(ctx);
    } else if (subcommand == "list") {
        return ListProjects(ctx);
    } else if (subcommand == "--help" || subcommand == "-h") {
        PrintHelp();
        return 0;
    } else {
        std::cerr << "Error: Unknown subcommand '" << subcommand << "'\n" << std::endl;
        PrintHelp();
        return 1;
    }
}

int ProjectCommand::CreateProject(const CLI::CommandContext& ctx) {
    if (ctx.args.size() < 2) {
        std::cerr << "Error: Project name not specified" << std::endl;
        std::cerr << "Usage: lumos project create <name>" << std::endl;
        return 1;
    }

    const std::string& project_name = ctx.args[1];

    // Create project directory
    fs::path project_path = fs::path(ctx.working_directory) / project_name;

    if (fs::exists(project_path)) {
        std::cerr << "Error: Directory '" << project_name << "' already exists" << std::endl;
        return 1;
    }

    std::cout << "Creating project '" << project_name << "'..." << std::endl;

    try {
        // Create directory structure
        fs::create_directories(project_path);
        fs::create_directories(project_path / "src");
        fs::create_directories(project_path / "include");
        fs::create_directories(project_path / "interfaces");
        fs::create_directories(project_path / "apps");
        fs::create_directories(project_path / "build");

        std::cout << "  Created directory structure" << std::endl;

        // Create lumos.json configuration
        Config::ProjectConfig config(project_name);
        config.Save((project_path / "lumos.json").string());

        std::cout << "  Created lumos.json" << std::endl;

        // Create CMakeLists.txt
        std::ofstream cmake_file(project_path / "CMakeLists.txt");
        cmake_file << "cmake_minimum_required(VERSION 3.14)\n";
        cmake_file << "project(" << project_name << " C CXX)\n\n";
        cmake_file << "set(CMAKE_CXX_STANDARD 17)\n";
        cmake_file << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";
        cmake_file << "# Include Lumos framework\n";
        cmake_file << "# TODO: Add framework path\n\n";
        cmake_file << "# Add applications\n";
        cmake_file << "# Applications will be added here by 'lumos app' commands\n";
        cmake_file.close();

        std::cout << "  Created CMakeLists.txt" << std::endl;

        // Create README.md
        std::ofstream readme_file(project_path / "README.md");
        readme_file << "# " << project_name << "\n\n";
        readme_file << "A Lumos distributed embedded application project.\n\n";
        readme_file << "## Building\n\n";
        readme_file << "```bash\n";
        readme_file << "lumos project build\n";
        readme_file << "```\n\n";
        readme_file << "## Adding Applications\n\n";
        readme_file << "```bash\n";
        readme_file << "lumos app create MyApp\n";
        readme_file << "```\n";
        readme_file.close();

        std::cout << "  Created README.md" << std::endl;

        // Create .gitignore
        std::ofstream gitignore_file(project_path / ".gitignore");
        gitignore_file << "build/\n";
        gitignore_file << "*.o\n";
        gitignore_file << "*.elf\n";
        gitignore_file << "*.bin\n";
        gitignore_file << "*.hex\n";
        gitignore_file << ".vscode/\n";
        gitignore_file << ".idea/\n";
        gitignore_file.close();

        std::cout << "  Created .gitignore" << std::endl;

        std::cout << "\nProject '" << project_name << "' created successfully!" << std::endl;
        std::cout << "\nNext steps:" << std::endl;
        std::cout << "  cd " << project_name << std::endl;
        std::cout << "  lumos app create <app_name>" << std::endl;

        return 0;

    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error creating project: " << e.what() << std::endl;
        return 1;
    }
}

int ProjectCommand::BuildProject(const CLI::CommandContext& ctx) {
    std::cout << "Building project..." << std::endl;

    // Check if we're in a Lumos project directory
    fs::path config_path = fs::path(ctx.working_directory) / "lumos.json";

    if (!fs::exists(config_path)) {
        std::cerr << "Error: Not in a Lumos project directory (lumos.json not found)" << std::endl;
        std::cerr << "Run this command from the root of a Lumos project." << std::endl;
        return 1;
    }

    // Load project configuration
    Config::ProjectConfig config;
    if (!config.Load(config_path.string())) {
        std::cerr << "Error: Failed to load project configuration" << std::endl;
        return 1;
    }

    std::cout << "Building project: " << config.GetProjectInfo().name << std::endl;

    // TODO: Implement actual build logic
    // For now, just create build directory and run cmake
    fs::path build_path = fs::path(ctx.working_directory) / "build";

    if (!fs::exists(build_path)) {
        fs::create_directories(build_path);
    }

    std::cout << "Build functionality will be implemented in Phase 2" << std::endl;
    std::cout << "For now, you can manually run:" << std::endl;
    std::cout << "  cd build && cmake .. && make" << std::endl;

    return 0;
}

int ProjectCommand::CleanProject(const CLI::CommandContext& ctx) {
    std::cout << "Cleaning project..." << std::endl;

    fs::path build_path = fs::path(ctx.working_directory) / "build";

    if (fs::exists(build_path)) {
        try {
            fs::remove_all(build_path);
            std::cout << "Build directory cleaned" << std::endl;
            return 0;
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error cleaning build directory: " << e.what() << std::endl;
            return 1;
        }
    } else {
        std::cout << "Nothing to clean (build directory doesn't exist)" << std::endl;
        return 0;
    }
}

int ProjectCommand::ListProjects(const CLI::CommandContext& ctx) {
    std::cout << "Listing projects in current directory..." << std::endl;

    try {
        bool found_any = false;

        for (const auto& entry : fs::directory_iterator(ctx.working_directory)) {
            if (entry.is_directory()) {
                fs::path config_path = entry.path() / "lumos.json";
                if (fs::exists(config_path)) {
                    Config::ProjectConfig config;
                    if (config.Load(config_path.string())) {
                        std::cout << "  " << config.GetProjectInfo().name
                                  << " (v" << config.GetProjectInfo().version << ")"
                                  << " - " << entry.path().filename().string() << std::endl;
                        found_any = true;
                    }
                }
            }
        }

        if (!found_any) {
            std::cout << "No Lumos projects found in current directory" << std::endl;
        }

        return 0;

    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error listing projects: " << e.what() << std::endl;
        return 1;
    }
}

} // namespace Commands
} // namespace Lumos
