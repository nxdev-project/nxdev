#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace nxdev::symbolize {

/**
 * @brief Represents a single decoded stack frame or symbol location.
 */
struct SymbolFrame {
    std::string address_hex; // Hex string e.g. "0x0000007100001040"
    uint64_t address{0};     // Parsed 64-bit address
    std::string function;    // Demangled function name
    std::string file;        // Source file path
    int line{0};             // Source line number (0 if unknown)
    int discriminator{0};    // DWARF discriminator
    bool is_valid{false};    // True if symbol and file/line were found
    std::string raw_output;  // Raw line from addr2line
};

/**
 * @brief Represents a parsed crash report or panic message.
 */
struct CrashReport {
    std::string title;
    std::string error_code;
    uint64_t program_counter{0};
    uint64_t link_register{0};
    uint64_t module_base{0};
    std::vector<SymbolFrame> backtrace;
    std::string raw_output;
};

/**
 * @brief Resolves function names, source files, and lines from Switch AArch64 ELFs using addr2line.
 */
class Symbolizer {
public:
    explicit Symbolizer(
        const std::string& elf_path = "",
        const std::string& custom_addr2line_path = "");
    ~Symbolizer() = default;

    /**
     * @brief Checks if a valid addr2line tool is available.
     */
    [[nodiscard]] bool is_tool_available() const noexcept;

    /**
     * @brief Checks if the target ELF file exists and is readable.
     */
    [[nodiscard]] bool is_elf_valid() const noexcept;

    [[nodiscard]] const std::string& elf_path() const noexcept { return elf_path_; }
    [[nodiscard]] const std::string& tool_path() const noexcept { return tool_path_; }

    /**
     * @brief Symbolizes a list of addresses in a single addr2line invocation.
     */
    [[nodiscard]] std::vector<SymbolFrame> symbolize(
        const std::vector<std::string>& address_strings,
        uint64_t module_base = 0) const;

    /**
     * @brief Symbolizes a single address.
     */
    [[nodiscard]] std::optional<SymbolFrame> symbolize_single(
        const std::string& address_str,
        uint64_t module_base = 0) const;

    /**
     * @brief Checks a runtime log line for known crash/panic patterns and extracts addresses if present.
     */
    [[nodiscard]] static std::optional<std::string> extract_crash_address(const std::string& log_line);

    /**
     * @brief Helper to parse hex string (e.g. "0x7100001040" or "7100001040") into uint64_t.
     */
    static bool parse_hex_address(const std::string& hex_str, uint64_t& out_address);

    /**
     * @brief Helper to format uint64_t as standard 16-character hex "0x0000007100001040".
     */
    static std::string format_hex_address(uint64_t addr);

    /**
     * @brief Resolves default aarch64-none-elf-addr2line tool from devkitA64.
     */
    [[nodiscard]] static std::string resolve_default_addr2line();

private:
    std::string elf_path_;
    std::string tool_path_;
};

} // namespace nxdev::symbolize
