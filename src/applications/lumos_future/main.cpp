#include "cli.h"
#include "commands/project_command.h"
#include "commands/app_command.h"
#include <iostream>
#include <unistd.h>
#include <limits.h>

int main(int argc, char** argv) {
    // Get current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        std::cerr << "Error: Could not get current working directory" << std::endl;
        return 1;
    }

    // Register all commands
    static Lumos::Commands::ProjectCommand project_cmd;
    static Lumos::Commands::AppCommand app_cmd;

    // Parse command line arguments
    if (argc < 2) {
        Lumos::CLI::CLIParser::PrintHelp();
        return 0;
    }

    // Handle special cases
    std::string first_arg = argv[1];
    if (first_arg == "--help" || first_arg == "-h") {
        Lumos::CLI::CLIParser::PrintHelp();
        return 0;
    }

    if (first_arg == "--version" || first_arg == "-v") {
        Lumos::CLI::CLIParser::PrintVersion();
        return 0;
    }

    auto parsed = Lumos::CLI::CLIParser::Parse(argc, argv);

    // Get the command
    auto* cmd = Lumos::CLI::CommandRegistry::Instance().GetCommand(parsed.command);

    if (!cmd) {
        std::cerr << "Error: Unknown command '" << parsed.command << "'\n" << std::endl;
        Lumos::CLI::CLIParser::PrintHelp();
        return 1;
    }

    // Prepare context
    Lumos::CLI::CommandContext ctx;
    ctx.working_directory = cwd;

    // Add subcommand if present
    if (!parsed.subcommand.empty()) {
        ctx.args.push_back(parsed.subcommand);
    }

    // Add positional arguments
    for (const auto& arg : parsed.args) {
        ctx.args.push_back(arg);
    }

    // Execute command
    return cmd->Execute(ctx);
}