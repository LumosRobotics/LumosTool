#pragma once

#include "../cli.h"
#include <string>

namespace Lumos {
namespace Commands {

class ProjectCommand : public CLI::Command {
public:
    ProjectCommand();
    ~ProjectCommand() override = default;

    int Execute(const CLI::CommandContext& ctx) override;
    std::string GetName() const override { return "project"; }
    std::string GetDescription() const override { return "Manage Lumos projects"; }
    std::string GetUsage() const override;

private:
    int CreateProject(const CLI::CommandContext& ctx);
    int BuildProject(const CLI::CommandContext& ctx);
    int CleanProject(const CLI::CommandContext& ctx);
    int ListProjects(const CLI::CommandContext& ctx);

    void PrintHelp();
};

} // namespace Commands
} // namespace Lumos
