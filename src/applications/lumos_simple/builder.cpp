#include "builder.h"
#include <iostream>
#include <filesystem>
#include <sstream>
#include <cstdlib>

namespace fs = std::filesystem;

namespace Lumos {

Builder::Builder(const std::string& lumos_root)
    : lumos_root_(lumos_root)
{
}

std::string Builder::GetToolchainPath() const {
    return lumos_root_ + "/src/toolchains/gcc-arm-none-eabi-10.3-2021.10/bin";
}

std::string Builder::GetPlatformPath(const std::string& platform) const {
    return lumos_root_ + "/src/toolchains/platform/" + platform;
}

std::vector<std::string> Builder::GetIncludePaths(const BoardConfig& board, const std::string& project_dir) const {
    std::string platform_path = GetPlatformPath(board.platform);

    std::vector<std::string> includes;

    // Add project include directory if it exists
    std::string project_include = project_dir + "/include";
    if (fs::exists(project_include)) {
        includes.push_back(project_include);
    }

    // Add platform paths
    includes.push_back(platform_path + "/lumos_config");
    includes.push_back(platform_path + "/Drivers/CMSIS/Include");

    // Add platform-specific CMSIS device include
    if (board.platform == "f4") {
        includes.push_back(platform_path + "/Drivers/CMSIS/Device/ST/STM32F4xx/Include");
        includes.push_back(platform_path + "/Drivers/STM32F4xx_HAL_Driver/Inc");
    } else if (board.platform == "h7") {
        includes.push_back(platform_path + "/Drivers/CMSIS/Device/ST/STM32H7xx/Include");
        includes.push_back(platform_path + "/Drivers/STM32H7xx_HAL_Driver/Inc");
    } else if (board.platform == "g0") {
        includes.push_back(platform_path + "/Drivers/CMSIS/Device/ST/STM32G0xx/Include");
        includes.push_back(platform_path + "/Drivers/STM32G0xx_HAL_Driver/Inc");
    } else if (board.platform == "g4") {
        includes.push_back(platform_path + "/Drivers/CMSIS/Device/ST/STM32G4xx/Include");
        includes.push_back(platform_path + "/Drivers/STM32G4xx_HAL_Driver/Inc");
    }

    // Add USB Middleware includes if needed
    includes.push_back(platform_path + "/Middlewares/ST/STM32_USB_Device_Library/Core/Inc");
    includes.push_back(platform_path + "/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc");

    return includes;
}

std::vector<std::string> Builder::GetDefines(const BoardConfig& board) const {
    return {
        board.mcu,
        "USE_HAL_DRIVER"
    };
}

std::vector<std::string> Builder::GetCompilerFlags(const BoardConfig& board) const {
    std::vector<std::string> flags = {
        "-mcpu=" + board.cpu,
        "-mthumb",
        "-mfloat-abi=" + board.float_abi,
        "-O2",
        "-Wall",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-exceptions",  // No C++ exceptions on embedded
        "-fno-rtti"         // No RTTI on embedded
    };

    // Add FPU if using hard float
    if (board.float_abi == "hard" && !board.fpu.empty()) {
        flags.push_back("-mfpu=" + board.fpu);
    }

    return flags;
}

std::string Builder::GetLinkerScript(const BoardConfig& board) const {
    std::string platform_path = GetPlatformPath(board.platform);

    if (board.platform == "f4") {
        return platform_path + "/lumos_config/STM32F407VG_FLASH.ld";
    } else if (board.platform == "h7") {
        return platform_path + "/lumos_config/STM32H723VG_FLASH.ld";
    }

    // Default fallback
    return platform_path + "/lumos_config/STM32F407VG_FLASH.ld";
}

std::string Builder::GetStartupFile(const BoardConfig& board) const {
    std::string platform_path = GetPlatformPath(board.platform);

    if (board.platform == "f4") {
        return platform_path + "/lumos_config/startup_stm32f407xx.s";
    } else if (board.platform == "h7") {
        return platform_path + "/lumos_config/startup_stm32h723xx.s";
    }

    // Default fallback
    return platform_path + "/lumos_config/startup_stm32f407xx.s";
}

std::string Builder::GetSystemFile(const BoardConfig& board) const {
    std::string platform_path = GetPlatformPath(board.platform);

    if (board.platform == "f4") {
        return platform_path + "/lumos_config/system_stm32f4xx.c";
    } else if (board.platform == "h7") {
        return platform_path + "/lumos_config/system_stm32h7xx.c";
    }

    // Default fallback
    return platform_path + "/lumos_config/system_stm32f4xx.c";
}

std::vector<std::string> Builder::GetRequiredHALFiles(const BoardConfig& board, const std::vector<std::string>& hal_modules) const {
    std::string platform_path = GetPlatformPath(board.platform);
    std::string hal_driver_path;
    std::string prefix;

    // Set platform-specific paths and prefixes
    if (board.platform == "f4") {
        hal_driver_path = platform_path + "/Drivers/STM32F4xx_HAL_Driver/Src";
        prefix = "stm32f4xx_hal";
    } else if (board.platform == "h7") {
        hal_driver_path = platform_path + "/Drivers/STM32H7xx_HAL_Driver/Src";
        prefix = "stm32h7xx_hal";
    } else if (board.platform == "g0") {
        hal_driver_path = platform_path + "/Drivers/STM32G0xx_HAL_Driver/Src";
        prefix = "stm32g0xx_hal";
    } else if (board.platform == "g4") {
        hal_driver_path = platform_path + "/Drivers/STM32G4xx_HAL_Driver/Src";
        prefix = "stm32g4xx_hal";
    } else {
        // Default to F4
        hal_driver_path = platform_path + "/Drivers/STM32F4xx_HAL_Driver/Src";
        prefix = "stm32f4xx_hal";
    }

    // Core HAL files always needed
    std::vector<std::string> hal_files = {
        hal_driver_path + "/" + prefix + ".c",
        hal_driver_path + "/" + prefix + "_cortex.c",
        hal_driver_path + "/" + prefix + "_rcc.c",
        hal_driver_path + "/" + prefix + "_rcc_ex.c",
        hal_driver_path + "/" + prefix + "_gpio.c",
        hal_driver_path + "/" + prefix + "_pwr.c",
        hal_driver_path + "/" + prefix + "_pwr_ex.c",
        hal_driver_path + "/" + prefix + "_dma.c"
    };

    // Add requested HAL modules
    for (const auto& module : hal_modules) {
        std::string module_file = hal_driver_path + "/" + prefix + "_" + module + ".c";
        hal_files.push_back(module_file);

        // Check if there's an _ex version
        std::string module_ex_file = hal_driver_path + "/" + prefix + "_" + module + "_ex.c";
        if (fs::exists(module_ex_file)) {
            hal_files.push_back(module_ex_file);
        }

        // Special case: PCD module needs LL USB driver
        if (module == "pcd") {
            std::string ll_usb_file = hal_driver_path + "/" + prefix.substr(0, prefix.find("_hal")) + "_ll_usb.c";
            if (fs::exists(ll_usb_file)) {
                hal_files.push_back(ll_usb_file);
            }
        }
    }

    return hal_files;
}

std::vector<std::string> Builder::GetUSBMiddlewareFiles(const BoardConfig& board) const {
    std::string platform_path = GetPlatformPath(board.platform);
    std::string usb_core_path = platform_path + "/Middlewares/ST/STM32_USB_Device_Library/Core/Src";
    std::string usb_cdc_path = platform_path + "/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src";

    std::vector<std::string> usb_files = {
        // USB Device Core files
        usb_core_path + "/usbd_core.c",
        usb_core_path + "/usbd_ctlreq.c",
        usb_core_path + "/usbd_ioreq.c",
        // USB CDC Class file
        usb_cdc_path + "/usbd_cdc.c"
    };

    return usb_files;
}

std::vector<std::string> Builder::GetLinkerFlags(const BoardConfig& board, const std::string& project_dir) const {
    return {
        "-mcpu=" + board.cpu,
        "-mthumb",
        "-mfloat-abi=" + board.float_abi,
        "-T" + GetLinkerScript(board),
        "-Wl,--gc-sections",
        "-Wl,-Map=" + project_dir + "/build/firmware.map",
        "-specs=nano.specs",
        "-specs=nosys.specs",
        "-lc",
        "-lm",
        "-lnosys"
    };
}

bool Builder::RunCommand(const std::string& command) const {
    std::cout << "Running: " << command << std::endl;
    int result = system(command.c_str());
    return result == 0;
}

bool Builder::CompileFile(const std::string& source_file,
                         const std::string& output_file,
                         const BoardConfig& board,
                         const std::string& project_dir) const {
    std::string toolchain = GetToolchainPath();
    std::string compiler;

    // Helper lambda for ends_with (C++17 compatible)
    auto ends_with = [](const std::string& str, const std::string& suffix) {
        if (suffix.size() > str.size()) return false;
        return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    };

    // Choose compiler based on file extension
    if (ends_with(source_file, ".c")) {
        compiler = toolchain + "/arm-none-eabi-gcc";
    } else if (ends_with(source_file, ".cpp") || ends_with(source_file, ".cc")) {
        compiler = toolchain + "/arm-none-eabi-g++";
    } else if (ends_with(source_file, ".s") || ends_with(source_file, ".S")) {
        compiler = toolchain + "/arm-none-eabi-gcc";
    } else {
        std::cerr << "Unknown file type: " << source_file << std::endl;
        return false;
    }

    std::ostringstream cmd;
    cmd << compiler << " -c " << source_file << " -o " << output_file;

    // Add compiler flags (skip for assembly)
    if (!ends_with(source_file, ".s") && !ends_with(source_file, ".S")) {
        for (const auto& flag : GetCompilerFlags(board)) {
            cmd << " " << flag;
        }

        // Add defines
        for (const auto& define : GetDefines(board)) {
            cmd << " -D" << define;
        }

        // Add include paths
        for (const auto& include : GetIncludePaths(board, project_dir)) {
            cmd << " -I" << include;
        }
    } else {
        // Assembly files just need basic flags
        cmd << " -mcpu=" << board.cpu << " -mthumb";
    }

    return RunCommand(cmd.str());
}

bool Builder::LinkFiles(const std::vector<std::string>& object_files,
                       const std::string& output_elf,
                       const BoardConfig& board,
                       const std::string& project_dir) const {
    std::string toolchain = GetToolchainPath();
    std::string linker = toolchain + "/arm-none-eabi-g++";

    std::ostringstream cmd;
    cmd << linker;

    // Add all object files
    for (const auto& obj : object_files) {
        cmd << " " << obj;
    }

    cmd << " -o " << output_elf;

    // Add linker flags
    for (const auto& flag : GetLinkerFlags(board, project_dir)) {
        cmd << " " << flag;
    }

    return RunCommand(cmd.str());
}

bool Builder::CreateBinary(const std::string& elf_file, const std::string& bin_file) const {
    std::string toolchain = GetToolchainPath();
    std::string objcopy = toolchain + "/arm-none-eabi-objcopy";

    std::ostringstream cmd;
    cmd << objcopy << " -O binary " << elf_file << " " << bin_file;

    return RunCommand(cmd.str());
}

bool Builder::Build(const std::string& project_dir) {
    std::cout << "=== Lumos Builder ===" << std::endl;
    std::cout << "Project directory: " << project_dir << std::endl;
    std::cout << std::endl;

    // Load project configuration
    ProjectConfig project;
    std::string yaml_path = project_dir + "/project.yaml";

    if (!project.Load(yaml_path)) {
        std::cerr << "Error: Failed to load project.yaml" << std::endl;
        return false;
    }

    std::cout << "Board: " << project.board << std::endl;
    std::cout << "Sources: " << project.sources.size() << " files" << std::endl;

    // Auto-detect HAL modules if not specified
    if (project.hal_modules.empty()) {
        std::cout << "Auto-detecting HAL modules from source files..." << std::endl;
        HALModuleDetector detector;
        project.hal_modules = detector.DetectModules(project.sources, project_dir);

        if (!project.hal_modules.empty()) {
            std::cout << "Detected modules: ";
            for (size_t i = 0; i < project.hal_modules.size(); ++i) {
                std::cout << project.hal_modules[i];
                if (i < project.hal_modules.size() - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << std::endl;
        } else {
            std::cout << "No HAL modules detected (using core modules only)" << std::endl;
        }
    } else {
        std::cout << "Using manually specified HAL modules: ";
        for (size_t i = 0; i < project.hal_modules.size(); ++i) {
            std::cout << project.hal_modules[i];
            if (i < project.hal_modules.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    // Get board configuration
    BoardConfig board = BoardConfig::GetConfig(project.board);
    std::cout << "Platform: " << board.platform << std::endl;
    std::cout << "MCU: " << board.mcu << std::endl;
    std::cout << "CPU: " << board.cpu << std::endl;
    std::cout << std::endl;

    // Create build directory
    std::string build_dir = project_dir + "/build";
    fs::create_directories(build_dir);

    std::vector<std::string> object_files;

    // Compile user source files
    std::cout << "Compiling user sources..." << std::endl;
    for (const auto& source : project.sources) {
        std::string source_path = project_dir + "/" + source;
        std::string obj_name = fs::path(source).stem().string() + ".o";
        std::string obj_path = build_dir + "/" + obj_name;

        std::cout << "  " << source << " -> " << obj_name << std::endl;

        if (!CompileFile(source_path, obj_path, board, project_dir)) {
            std::cerr << "Error: Compilation failed for " << source << std::endl;
            return false;
        }

        object_files.push_back(obj_path);
    }
    std::cout << std::endl;

    // Compile HAL driver files
    std::cout << "Compiling HAL drivers..." << std::endl;
    std::vector<std::string> hal_files = GetRequiredHALFiles(board, project.hal_modules);
    for (const auto& hal_file : hal_files) {
        // Only compile if file exists
        if (!fs::exists(hal_file)) {
            std::cout << "  Skipping " << fs::path(hal_file).filename().string() << " (not found)" << std::endl;
            continue;
        }

        std::string hal_filename = fs::path(hal_file).filename().string();
        std::string obj_name = fs::path(hal_file).stem().string() + ".o";
        std::string obj_path = build_dir + "/" + obj_name;

        std::cout << "  " << hal_filename << " -> " << obj_name << std::endl;

        if (!CompileFile(hal_file, obj_path, board, project_dir)) {
            std::cerr << "Error: Failed to compile HAL file " << hal_filename << std::endl;
            return false;
        }

        object_files.push_back(obj_path);
    }
    std::cout << std::endl;

    // Compile USB middleware files if needed
    bool uses_usb = false;
    for (const auto& module : project.hal_modules) {
        if (module == "pcd" || module == "pcd_ex") {
            uses_usb = true;
            break;
        }
    }

    if (uses_usb) {
        std::cout << "Compiling USB middleware..." << std::endl;
        std::vector<std::string> usb_files = GetUSBMiddlewareFiles(board);
        for (const auto& usb_file : usb_files) {
            if (!fs::exists(usb_file)) {
                std::cout << "  Skipping " << fs::path(usb_file).filename().string() << " (not found)" << std::endl;
                continue;
            }

            std::string usb_filename = fs::path(usb_file).filename().string();
            std::string obj_name = fs::path(usb_file).stem().string() + ".o";
            std::string obj_path = build_dir + "/" + obj_name;

            std::cout << "  " << usb_filename << " -> " << obj_name << std::endl;

            if (!CompileFile(usb_file, obj_path, board, project_dir)) {
                std::cerr << "Error: Failed to compile USB file " << usb_filename << std::endl;
                return false;
            }

            object_files.push_back(obj_path);
        }
        std::cout << std::endl;
    }

    // Compile system files
    std::cout << "Compiling system files..." << std::endl;

    // Startup file
    std::string startup_file = GetStartupFile(board);
    std::string startup_filename = board.platform == "h7" ? "startup_stm32h723xx.s" : "startup_stm32f407xx.s";
    std::string startup_obj = build_dir + "/startup.o";
    std::cout << "  " << startup_filename << " -> startup.o" << std::endl;
    if (!CompileFile(startup_file, startup_obj, board, project_dir)) {
        std::cerr << "Error: Failed to compile startup file" << std::endl;
        return false;
    }
    object_files.push_back(startup_obj);

    // System file
    std::string system_file = GetSystemFile(board);
    std::string system_filename = board.platform == "h7" ? "system_stm32h7xx" : "system_stm32f4xx";
    std::string system_obj = build_dir + "/" + system_filename + ".o";
    std::cout << "  " << system_filename << ".c -> " << system_filename << ".o" << std::endl;
    if (!CompileFile(system_file, system_obj, board, project_dir)) {
        std::cerr << "Error: Failed to compile system file" << std::endl;
        return false;
    }
    object_files.push_back(system_obj);
    std::cout << std::endl;

    // Link
    std::cout << "Linking..." << std::endl;
    std::string elf_file = build_dir + "/firmware.elf";
    if (!LinkFiles(object_files, elf_file, board, project_dir)) {
        std::cerr << "Error: Linking failed" << std::endl;
        return false;
    }
    std::cout << std::endl;

    // Create binary
    std::cout << "Creating binary..." << std::endl;
    std::string bin_file = build_dir + "/firmware.bin";
    if (!CreateBinary(elf_file, bin_file)) {
        std::cerr << "Error: Failed to create binary" << std::endl;
        return false;
    }
    std::cout << std::endl;

    // Print file sizes
    std::cout << "Build complete!" << std::endl;
    std::cout << "Output files:" << std::endl;
    std::cout << "  " << elf_file << std::endl;
    std::cout << "  " << bin_file << std::endl;

    // Get binary size
    if (fs::exists(bin_file)) {
        auto bin_size = fs::file_size(bin_file);
        std::cout << "  Binary size: " << bin_size << " bytes" << std::endl;
    }

    return true;
}

} // namespace Lumos
