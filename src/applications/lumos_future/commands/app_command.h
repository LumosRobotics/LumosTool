#pragma once

#include "../cli.h"
#include <string>

namespace Lumos {
namespace Commands {

class AppCommand : public CLI::Command {
public:
    AppCommand();
    ~AppCommand() override = default;

    int Execute(const CLI::CommandContext& ctx) override;
    std::string GetName() const override { return "app"; }
    std::string GetDescription() const override { return "Manage applications in a project"; }
    std::string GetUsage() const override;

private:
    int CreateApp(const CLI::CommandContext& ctx);
    int ListApps(const CLI::CommandContext& ctx);
    int RemoveApp(const CLI::CommandContext& ctx);

    void PrintHelp();
};

} // namespace Commands
} // namespace Lumos
