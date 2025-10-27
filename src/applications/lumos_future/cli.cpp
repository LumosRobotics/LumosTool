#include "cli.h"
#include <iostream>
#include <algorithm>

namespace Lumos {
namespace CLI {

// CommandRegistry Implementation
CommandRegistry& CommandRegistry::Instance() {
    static CommandRegistry instance;
    return instance;
}

void CommandRegistry::RegisterCommand(Command* cmd) {
    commands_[cmd->GetName()] = cmd;
}

Command* CommandRegistry::GetCommand(const std::string& name) {
    auto it = commands_.find(name);
    if (it != commands_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<Command*> CommandRegistry::GetAllCommands() {
    std::vector<Command*> result;
    for (auto& pair : commands_) {
        result.push_back(pair.second);
    }
    return result;
}

// CLIParser Implementation
CLIParser::ParsedCommand CLIParser::Parse(int argc, char** argv) {
    ParsedCommand result;

    if (argc < 2) {
        return result;
    }

    int idx = 1;

    // First argument is the main command
    result.command = argv[idx++];

    // Check if there's a subcommand (doesn't start with --)
    if (idx < argc && argv[idx][0] != '-') {
        result.subcommand = argv[idx++];
    }

    // Parse remaining arguments and options
    while (idx < argc) {
        std::string arg = argv[idx];

        if (arg.substr(0, 2) == "--") {
            // Long option
            std::string option = arg.substr(2);
            size_t eq_pos = option.find('=');

            if (eq_pos != std::string::npos) {
                // --option=value
                std::string key = option.substr(0, eq_pos);
                std::string value = option.substr(eq_pos + 1);
                result.options[key] = value;
            } else {
                // --option (check if next arg is value)
                if (idx + 1 < argc && argv[idx + 1][0] != '-') {
                    result.options[option] = argv[++idx];
                } else {
                    result.options[option] = "true";
                }
            }
        } else if (arg[0] == '-' && arg.length() > 1) {
            // Short option(s)
            for (size_t i = 1; i < arg.length(); ++i) {
                std::string key(1, arg[i]);
                result.options[key] = "true";
            }
        } else {
            // Positional argument
            result.args.push_back(arg);
        }

        ++idx;
    }

    return result;
}

void CLIParser::PrintHelp() {
    std::cout << "Lumos - Embedded Distributed Application Tool\n\n";
    std::cout << "Usage: lumos <command> [subcommand] [options] [args]\n\n";
    std::cout << "Commands:\n";

    auto& registry = CommandRegistry::Instance();
    auto commands = registry.GetAllCommands();

    // Sort commands by name
    std::sort(commands.begin(), commands.end(),
              [](Command* a, Command* b) { return a->GetName() < b->GetName(); });

    for (auto* cmd : commands) {
        std::cout << "  " << cmd->GetName() << "\t\t" << cmd->GetDescription() << "\n";
    }

    std::cout << "\nUse 'lumos <command> --help' for more information about a command.\n";
}

void CLIParser::PrintVersion() {
    std::cout << "Lumos version 1.0.0\n";
}

} // namespace CLI
} // namespace Lumos
