#pragma once

#include "project_config.h"
#include "hal_module_detector.h"
#include <string>
#include <vector>

namespace Lumos {

class Builder {
public:
    Builder(const std::string& lumos_root);

    bool Build(const std::string& project_dir);

private:
    std::string lumos_root_;

    std::string GetResourceBasePath() const;  // Helper for dev vs release structure
    std::string GetToolchainPath() const;
    std::string GetPlatformPath(const std::string& platform) const;
    std::string GetBoardPath(const std::string& board_name) const;

    std::vector<std::string> GetIncludePaths(const BoardConfig& board, const std::string& project_dir) const;
    std::vector<std::string> GetDefines(const BoardConfig& board) const;
    std::vector<std::string> GetCompilerFlags(const BoardConfig& board) const;
    std::vector<std::string> GetLinkerFlags(const BoardConfig& board, const std::string& project_dir) const;

    std::string GetLinkerScript(const BoardConfig& board) const;
    std::string GetStartupFile(const BoardConfig& board) const;
    std::string GetSystemFile(const BoardConfig& board) const;
    std::vector<std::string> GetBoardSupportFiles(const BoardConfig& board) const;
    std::vector<std::string> GetRequiredHALFiles(const BoardConfig& board, const std::vector<std::string>& hal_modules) const;
    std::vector<std::string> GetUSBMiddlewareFiles(const BoardConfig& board) const;

    bool CompileFile(const std::string& source_file,
                    const std::string& output_file,
                    const BoardConfig& board,
                    const std::string& project_dir) const;

    bool LinkFiles(const std::vector<std::string>& object_files,
                  const std::string& output_elf,
                  const BoardConfig& board,
                  const std::string& project_dir) const;

    bool CreateBinary(const std::string& elf_file, const std::string& bin_file) const;

    bool RunCommand(const std::string& command) const;

    bool CheckAndCreateMainFile(const std::string& project_dir, ProjectConfig& project);
    std::string PromptLanguage() const;
    bool GenerateMainFile(const std::string& language, const std::string& project_dir) const;
};

} // namespace Lumos
