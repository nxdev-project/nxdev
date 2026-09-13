#include <nxdev/cli/commands.hpp>
#include <nxdev/symbolize/symbolizer.hpp>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace nxdev::cli {

namespace fs = std::filesystem;

int SymbolizeCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    std::string elf_path;
    uint64_t module_base = 0;
    bool json_output = false;
    std::vector<std::string> addresses;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--elf" && i + 1 < args.size()) {
            elf_path = args[++i];
        } else if (args[i] == "--base" && i + 1 < args.size()) {
            symbolize::Symbolizer::parse_hex_address(args[++i], module_base);
        } else if (args[i] == "--json") {
            json_output = true;
        } else if (!args[i].empty() && args[i][0] != '-') {
            addresses.push_back(args[i]);
        }
    }

    if (addresses.empty()) {
        std::cerr << "Error: No addresses specified to symbolize.\n"
                  << "Usage: nxdev symbolize [--elf <path>] [--base <addr>] [--json] <0xAddress...>\n";
        return 1;
    }

    // Auto-resolve ELF path if not specified
    if (elf_path.empty() && ctx.project.has_value() && ctx.project->is_valid()) {
        std::string app_name = ctx.project->manifest().application().name;
        std::string profile = ctx.project->manifest().build().default_profile;
        fs::path p1 = fs::path(ctx.project->root_path()) / ".nxdev" / "build" / profile / "bin" / (app_name + ".elf");
        fs::path p2 = fs::path(ctx.project->root_path()) / "build" / profile / (app_name + ".elf");
        fs::path p3 = fs::path(ctx.project->root_path()) / "build" / profile / "bin" / (app_name + ".elf");
        if (fs::exists(p1)) {
            elf_path = p1.string();
        } else if (fs::exists(p2)) {
            elf_path = p2.string();
        } else if (fs::exists(p3)) {
            elf_path = p3.string();
        }
    }

    symbolize::Symbolizer symbolizer(elf_path);

    if (!symbolizer.is_tool_available()) {
        std::string err = "Tool 'aarch64-none-elf-addr2line' not found. Ensure devkitA64 is installed.";
        if (json_output) {
            std::cout << "{\"success\":false,\"error\":\"" << err << "\"}\n";
        } else {
            std::cerr << "Error: " << err << "\n";
        }
        return 1;
    }

    if (!symbolizer.is_elf_valid()) {
        std::string err = "ELF file not found at '" + elf_path + "'. Specify with --elf <path>.";
        if (json_output) {
            std::cout << "{\"success\":false,\"error\":\"" << err << "\"}\n";
        } else {
            std::cerr << "Error: " << err << "\n";
        }
        return 1;
    }

    auto frames = symbolizer.symbolize(addresses, module_base);

    if (json_output) {
        std::ostringstream oss;
        oss << "{\"success\":true,\"elf\":\"" << elf_path << "\",\"frames\":[";
        for (size_t i = 0; i < frames.size(); ++i) {
            const auto& f = frames[i];
            if (i > 0) oss << ",";
            oss << "{\"address\":\"" << f.address_hex << "\""
                << ",\"function\":\"" << f.function << "\""
                << ",\"file\":\"" << f.file << "\""
                << ",\"line\":" << f.line
                << ",\"is_valid\":" << (f.is_valid ? "true" : "false") << "}";
        }
        oss << "]}";
        std::cout << oss.str() << "\n";
        return 0;
    }

    std::cout << "Symbolized Frames (ELF: " << elf_path << "):\n\n";
    for (const auto& f : frames) {
        std::cout << "  " << f.address_hex << " -> " << f.function << "\n"
                  << "    at " << f.file << ":" << f.line << "\n";
    }

    return 0;
}

} // namespace nxdev::cli
