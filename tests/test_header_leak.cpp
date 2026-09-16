#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdlib>

namespace fs = std::filesystem;

static void assert_test(bool condition, const std::string& name) {
    if (!condition) {
        std::cerr << "[FAIL] test_header_leak: " << name << "\n";
        std::exit(1);
    }
    std::cout << "[PASS] test_header_leak: " << name << "\n";
}

int main() {
    std::cout << "=== Running NXDev UI Public Header Isolation & Anti-Leak Test ===\n";

    // Locate repository root
    fs::path repo_root;
    fs::path p = fs::current_path();
    for (int i = 0; i < 4 && p != p.root_path(); ++i) {
        if (fs::exists(p / "sdk" / "modules" / "borealis" / "include" / "nxdev" / "ui.hpp")) {
            repo_root = p;
            break;
        }
        p = p.parent_path();
    }

    assert_test(!repo_root.empty(), "Located NXDev repository root");

    fs::path ui_inc_dir = repo_root / "sdk" / "modules" / "borealis" / "include" / "nxdev";
    assert_test(fs::exists(ui_inc_dir), "nxdev include directory exists");

    std::vector<fs::path> public_headers;
    public_headers.push_back(ui_inc_dir / "ui.hpp");

    if (fs::exists(ui_inc_dir / "ui")) {
        for (const auto& entry : fs::directory_iterator(ui_inc_dir / "ui")) {
            if (entry.is_regular_file() && entry.path().extension() == ".hpp") {
                public_headers.push_back(entry.path());
            }
        }
    }

    assert_test(!public_headers.empty(), "Found public UI headers to scan");

    for (const auto& header : public_headers) {
        std::ifstream file(header);
        assert_test(file.is_open(), "Read " + header.filename().string());

        std::string line;
        int line_num = 0;
        while (std::getline(file, line)) {
            line_num++;

            // Check 1: No direct borealis.hpp includes
            if (line.find("#include <borealis") != std::string::npos ||
                line.find("#include \"borealis") != std::string::npos) {
                std::cerr << "[ERROR] Leaked Borealis include in " << header.string() << ":" << line_num << " -> " << line << "\n";
                assert_test(false, "Header " + header.filename().string() + " must not include Borealis headers");
            }

            // Check 2: No brls:: namespace types exposed
            if (line.find("brls::") != std::string::npos) {
                std::cerr << "[ERROR] Leaked brls:: type in " << header.string() << ":" << line_num << " -> " << line << "\n";
                assert_test(false, "Header " + header.filename().string() + " must not expose brls:: types");
            }

            // Check 3: No NanoVG / Yoga handle leaks
            if (line.find("NVGcontext") != std::string::npos ||
                line.find("NVGcolor") != std::string::npos ||
                line.find("YGNodeRef") != std::string::npos) {
                std::cerr << "[ERROR] Leaked low-level renderer/layout type in " << header.string() << ":" << line_num << " -> " << line << "\n";
                assert_test(false, "Header " + header.filename().string() + " must not expose internal backend types");
            }
        }

        std::cout << "  ✓ Clean header isolation: " << header.filename().string() << "\n";
    }

    std::cout << "\n✓ All NXDev UI public headers verified: 100% free of backend leaks!\n";
    return 0;
}
