#include <nxdev/symbolize/symbolizer.hpp>
#include <cassert>
#include <iostream>

void test_hex_address_parsing() {
    std::cout << "  ✓ test_hex_address_parsing..." << std::flush;

    uint64_t addr = 0;
    assert(nxdev::symbolize::Symbolizer::parse_hex_address("0x7100001040", addr));
    assert(addr == 0x7100001040ULL);

    assert(nxdev::symbolize::Symbolizer::parse_hex_address("7100001040", addr));
    assert(addr == 0x7100001040ULL);

    assert(nxdev::symbolize::Symbolizer::parse_hex_address("0x0", addr));
    assert(addr == 0);

    // Invalid
    assert(!nxdev::symbolize::Symbolizer::parse_hex_address("", addr));
    assert(!nxdev::symbolize::Symbolizer::parse_hex_address("invalid_hex", addr));
    assert(!nxdev::symbolize::Symbolizer::parse_hex_address("0xGHIJK", addr));
    (void)addr;

    std::cout << " passed\n";
}

void test_crash_address_extraction() {
    std::cout << "  ✓ test_crash_address_extraction..." << std::flush;

    // Pattern 1: NXDev structured line
    auto addr1 = nxdev::symbolize::Symbolizer::extract_crash_address("[NXDEV-CRASH] pc=0x7100001040");
    assert(addr1.has_value());
    assert(*addr1 == "0x7100001040");

    // Pattern 2: Horizon PC line
    auto addr2 = nxdev::symbolize::Symbolizer::extract_crash_address("Fatal error occurred! PC: 0x00000071000042a0");
    assert(addr2.has_value());
    assert(*addr2 == "0x00000071000042a0");

    // Pattern 3: Backtrace frame line
    auto addr3 = nxdev::symbolize::Symbolizer::extract_crash_address("Backtrace: #0 0x0000007100008800");
    assert(addr3.has_value());
    assert(*addr3 == "0x0000007100008800");

    // Non-crash lines
    assert(!nxdev::symbolize::Symbolizer::extract_crash_address("Regular log output: starting application...").has_value());
    assert(!nxdev::symbolize::Symbolizer::extract_crash_address("").has_value());

    std::cout << " passed\n";
}

void test_symbolizer_mock() {
    std::cout << "  ✓ test_symbolizer_mock..." << std::flush;

    // When tool or ELF is missing, symbolizer should return graceful frames
    nxdev::symbolize::Symbolizer sym("/path/does/not/exist.elf", "/path/to/missing_tool");
    assert(!sym.is_elf_valid());
    assert(!sym.is_tool_available());

    auto frames = sym.symbolize({"0x7100001040", "0x7100002000"});
    assert(frames.size() == 2);
    assert(!frames[0].is_valid);
    assert(frames[0].function == "??");
    assert(frames[0].file == "??");

    std::cout << " passed\n";
}

int main() {
    std::cout << "[Test] Running Symbolizer Subsystem Tests...\n";
    test_hex_address_parsing();
    test_crash_address_extraction();
    test_symbolizer_mock();
    std::cout << "[Test] All Symbolizer Subsystem tests passed!\n";
    return 0;
}
