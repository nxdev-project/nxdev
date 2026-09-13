#include <nxdev/symbolize/symbolizer.hpp>
#include <nxdev/exec/process.hpp>
#include <nxdev/env/environment.hpp>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <regex>
#include <algorithm>

namespace nxdev::symbolize {

namespace fs = std::filesystem;

Symbolizer::Symbolizer(
    const std::string& elf_path,
    const std::string& custom_addr2line_path)
    : elf_path_(elf_path) {
    if (!custom_addr2line_path.empty()) {
        tool_path_ = custom_addr2line_path;
    } else {
        tool_path_ = resolve_default_addr2line();
    }
}

std::string Symbolizer::resolve_default_addr2line() {
    auto env = env::Environment::detect(config::HostConfig::load());
    if (env.devkita64().is_valid && !env.devkita64().path.empty()) {
        fs::path p = fs::path(env.devkita64().path) / "bin" / "aarch64-none-elf-addr2line";
#if defined(_WIN32)
        p += ".exe";
#endif
        if (fs::exists(p)) {
            return p.string();
        }
    }
    // Fallback: check DEVKITPRO
    if (const char* dkp = std::getenv("DEVKITPRO")) {
        fs::path p = fs::path(dkp) / "devkitA64" / "bin" / "aarch64-none-elf-addr2line";
#if defined(_WIN32)
        p += ".exe";
#endif
        if (fs::exists(p)) {
            return p.string();
        }
    }
    return "aarch64-none-elf-addr2line";
}

bool Symbolizer::is_tool_available() const noexcept {
    if (tool_path_.empty()) return false;
    if (fs::exists(tool_path_)) return true;
    auto res = exec::ProcessExecutor::probe_version(tool_path_, "--version", 1000);
    return res.success;
}

bool Symbolizer::is_elf_valid() const noexcept {
    if (elf_path_.empty()) return false;
    return fs::exists(elf_path_) && fs::is_regular_file(elf_path_);
}

bool Symbolizer::parse_hex_address(const std::string& hex_str, uint64_t& out_address) {
    if (hex_str.empty()) return false;
    std::string clean = hex_str;
    if (clean.rfind("0x", 0) == 0 || clean.rfind("0X", 0) == 0) {
        clean = clean.substr(2);
    }
    try {
        size_t idx = 0;
        out_address = std::stoull(clean, &idx, 16);
        return (idx == clean.size());
    } catch (...) {
        return false;
    }
}

std::string Symbolizer::format_hex_address(uint64_t addr) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setw(16) << std::setfill('0') << addr;
    return oss.str();
}

std::optional<SymbolFrame> Symbolizer::symbolize_single(
    const std::string& address_str,
    uint64_t module_base) const {
    auto frames = symbolize({address_str}, module_base);
    if (!frames.empty()) {
        return frames.front();
    }
    return std::nullopt;
}

std::vector<SymbolFrame> Symbolizer::symbolize(
    const std::vector<std::string>& address_strings,
    uint64_t module_base) const {
    std::vector<SymbolFrame> results;
    if (address_strings.empty()) return results;

    std::vector<std::string> query_addrs;
    std::vector<uint64_t> numeric_addrs;

    for (const auto& addr_str : address_strings) {
        SymbolFrame frame;
        frame.address_hex = addr_str;
        uint64_t raw_num = 0;
        if (parse_hex_address(addr_str, raw_num)) {
            frame.address = raw_num;
            uint64_t offset_num = raw_num;
            if (module_base > 0 && offset_num >= module_base) {
                offset_num -= module_base;
            }
            std::ostringstream oss;
            oss << "0x" << std::hex << offset_num;
            query_addrs.push_back(oss.str());
            numeric_addrs.push_back(raw_num);
        } else {
            // Invalid address string
            frame.function = "??";
            frame.file = "??";
            frame.line = 0;
            frame.is_valid = false;
            results.push_back(frame);
            continue;
        }
    }

    if (query_addrs.empty() || !is_elf_valid() || !is_tool_available()) {
        for (size_t i = 0; i < query_addrs.size(); ++i) {
            SymbolFrame frame;
            frame.address = numeric_addrs[i];
            frame.address_hex = format_hex_address(numeric_addrs[i]);
            frame.function = "??";
            frame.file = "??";
            frame.line = 0;
            frame.is_valid = false;
            results.push_back(frame);
        }
        return results;
    }

    // Invoke addr2line: addr2line -e <elf> -f -C <addr1> <addr2> ...
    std::vector<std::string> args = {"-e", elf_path_, "-f", "-C"};
    for (const auto& q : query_addrs) {
        args.push_back(q);
    }

    auto proc_res = exec::ProcessExecutor::execute(tool_path_, args, 5000);
    if (!proc_res.success) {
        for (size_t i = 0; i < query_addrs.size(); ++i) {
            SymbolFrame frame;
            frame.address = numeric_addrs[i];
            frame.address_hex = format_hex_address(numeric_addrs[i]);
            frame.function = "??";
            frame.file = "??";
            frame.line = 0;
            frame.is_valid = false;
            results.push_back(frame);
        }
        return results;
    }

    // Parse addr2line standard 2-lines per address
    std::istringstream stream(proc_res.stdout_output);
    std::string func_line;
    std::string file_line;

    size_t query_idx = 0;
    while (std::getline(stream, func_line)) {
        if (!std::getline(stream, file_line)) {
            break;
        }

        SymbolFrame frame;
        if (query_idx < numeric_addrs.size()) {
            frame.address = numeric_addrs[query_idx];
            frame.address_hex = format_hex_address(numeric_addrs[query_idx]);
            query_idx++;
        }

        // Clean up function name
        // Trim \r
        if (!func_line.empty() && func_line.back() == '\r') func_line.pop_back();
        if (!file_line.empty() && file_line.back() == '\r') file_line.pop_back();

        frame.function = func_line;
        frame.raw_output = func_line + " at " + file_line;

        // Parse file:line[:discriminator]
        // Example: "/home/user/project/src/main.cpp:42 (discriminator 1)"
        // or "??:0"
        std::string file_part = file_line;
        size_t disc_pos = file_part.find("(discriminator");
        if (disc_pos != std::string::npos) {
            file_part = file_part.substr(0, disc_pos);
            // Trim trailing spaces
            while (!file_part.empty() && file_part.back() == ' ') file_part.pop_back();
        }

        size_t colon_pos = file_part.rfind(':');
        if (colon_pos != std::string::npos) {
            frame.file = file_part.substr(0, colon_pos);
            std::string line_str = file_part.substr(colon_pos + 1);
            try {
                frame.line = std::stoi(line_str);
            } catch (...) {
                frame.line = 0;
            }
        } else {
            frame.file = file_part;
            frame.line = 0;
        }

        frame.is_valid = (frame.function != "??" && frame.file != "??" && frame.line > 0);
        results.push_back(frame);
    }

    return results;
}

std::optional<std::string> Symbolizer::extract_crash_address(const std::string& log_line) {
    if (log_line.empty()) return std::nullopt;

    // Pattern 1: NXDev Crash Line: [NXDEV-CRASH] pc=0x... or pc=...
    static const std::regex kNxdevCrashPattern(
        R"(\[NXDEV[-_]CRASH\]\s+(?:pc|addr|address)=(0x[0-9a-fA-F]+|[0-9a-fA-F]{6,16}))");
    std::smatch match;
    if (std::regex_search(log_line, match, kNxdevCrashPattern)) {
        return match[1].str();
    }

    // Pattern 2: Atmosphere / Horizon Fatal PC: "PC: 0x0000007100001040"
    static const std::regex kHorizonPcPattern(
        R"(\b(?:PC|Program\s+Counter|LR|Link\s+Register):\s*(0x[0-9a-fA-F]{6,16}))");
    if (std::regex_search(log_line, match, kHorizonPcPattern)) {
        return match[1].str();
    }

    // Pattern 3: Backtrace line: "#0 0x0000007100001040" or "0x0000007100001040 in main()"
    static const std::regex kBacktracePattern(
        R"(#\d+\s+(0x[0-9a-fA-F]{6,16}))");
    if (std::regex_search(log_line, match, kBacktracePattern)) {
        return match[1].str();
    }

    return std::nullopt;
}

} // namespace nxdev::symbolize
