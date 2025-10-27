#pragma once

#include <string>
#include <vector>
#include <functional>
#include <map>

namespace Lumos {
namespace CLI {

struct CommandContext {
    std::vector<std::string> args;
    std::string working_directory;
};

class Command {
public:
    virtual ~Command() = default;
    virtual int Execute(const CommandContext& ctx) = 0;
    virtual std::string GetName() const = 0;
    virtual std::string GetDescription() const = 0;
    virtual std::string GetUsage() const = 0;
};

class CommandRegistry {
public:
    static CommandRegistry& Instance();

    void RegisterCommand(Command* cmd);
    Command* GetCommand(const std::string& name);
    std::vector<Command*> GetAllCommands();

private:
    CommandRegistry() = default;
    std::map<std::string, Command*> commands_;
};

class CLIParser {
public:
    struct ParsedCommand {
        std::string command;
        std::string subcommand;
        std::vector<std::string> args;
        std::map<std::string, std::string> options;
    };

    static ParsedCommand Parse(int argc, char** argv);
    static void PrintHelp();
    static void PrintVersion();
};

} // namespace CLI
} // namespace Lumos
