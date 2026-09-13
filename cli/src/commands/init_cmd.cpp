#include <nxdev/cli/commands.hpp>
#include <nxdev/project/project.hpp>
#include <nxdev/manifest/parser.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <iomanip>

namespace fs = std::filesystem;

namespace nxdev::cli {

namespace {

inline std::string c_green(std::string_view s) { return "\033[32m" + std::string(s) + "\033[0m"; }
inline std::string c_red(std::string_view s) { return "\033[31m" + std::string(s) + "\033[0m"; }
inline std::string c_cyan(std::string_view s) { return "\033[36m" + std::string(s) + "\033[0m"; }
inline std::string c_bold(std::string_view s) { return "\033[1m" + std::string(s) + "\033[0m"; }

std::string slugify(std::string_view name) {
    std::string slug;
    slug.reserve(name.size());
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            slug.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        } else if (c == ' ' || c == '-' || c == '_') {
            if (!slug.empty() && slug.back() != '-') {
                slug.push_back('-');
            }
        }
    }
    while (!slug.empty() && slug.back() == '-') {
        slug.pop_back();
    }
    return slug.empty() ? "nx-app" : slug;
}

bool update_gitignore(const fs::path& target_dir, std::string& err) {
    fs::path gi_path = target_dir / ".gitignore";
    std::string existing_content;
    if (fs::exists(gi_path)) {
        std::ifstream in(gi_path);
        if (in) {
            existing_content.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        }
    }

    std::vector<std::string> needed_entries = {".nxdev/", "dist/", "build/"};
    std::vector<std::string> missing_entries;
    for (const auto& entry : needed_entries) {
        if (existing_content.find(entry) == std::string::npos) {
            missing_entries.push_back(entry);
        }
    }

    if (!missing_entries.empty()) {
        std::ofstream out(gi_path, std::ios::app);
        if (!out) {
            err = "Failed to write to .gitignore";
            return false;
        }
        if (!existing_content.empty() && existing_content.back() != '\n') {
            out << "\n";
        }
        out << "\n# NXDev build & packaging artifacts\n";
        for (const auto& entry : missing_entries) {
            out << entry << "\n";
        }
    }
    return true;
}

} // anonymous namespace

int InitCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    (void)ctx;
    fs::path target_path = fs::current_path();
    std::string app_name;
    std::string app_id;
    std::string author = "Anonymous Developer";
    std::string version = "0.1.0";
    std::string title_id = "0100000000001000";
    bool force = false;
    bool non_interactive = false;
    bool json_output = false;

    // Parse CLI arguments
    for (size_t i = 0; i < args.size(); ++i) {
        const auto& arg = args[i];
        if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: nxdev init [path] [options]\n\n"
                      << "Initialize NXDev metadata (nxapp.yaml) and build integration in an existing directory.\n\n"
                      << "Options:\n"
                      << "  --name <name>         Application display name\n"
                      << "  --id <id>             Internal project identifier (slug)\n"
                      << "  --author <author>     Application author name\n"
                      << "  --version <ver>       Application version string (e.g. 0.1.0)\n"
                      << "  --title-id <id>       16-digit hexadecimal Switch Title ID\n"
                      << "  -f, --force           Reinitialize / overwrite existing nxapp.yaml\n"
                      << "  -y, --non-interactive Non-interactive mode (use defaults)\n"
                      << "  --json                Emit structured JSON output\n"
                      << "  -h, --help            Display this help message\n";
            return 0;
        } else if (arg == "--name" && i + 1 < args.size()) {
            app_name = args[++i];
        } else if (arg == "--id" && i + 1 < args.size()) {
            app_id = args[++i];
        } else if (arg == "--author" && i + 1 < args.size()) {
            author = args[++i];
        } else if (arg == "--version" && i + 1 < args.size()) {
            version = args[++i];
        } else if (arg == "--title-id" && i + 1 < args.size()) {
            title_id = args[++i];
        } else if (arg == "-f" || arg == "--force") {
            force = true;
        } else if (arg == "-y" || arg == "--non-interactive") {
            non_interactive = true;
            (void)non_interactive;
        } else if (arg == "--json") {
            json_output = true;
        } else if (arg[0] != '-') {
            target_path = fs::absolute(arg);
        }
    }

    if (!fs::exists(target_path)) {
        try {
            fs::create_directories(target_path);
        } catch (const std::exception& e) {
            if (json_output) {
                std::cout << "{\"status\":\"error\",\"message\":\"Failed to create target directory: " << e.what() << "\"}\n";
            } else {
                std::cerr << c_red("Error: ") << "Failed to create directory " << target_path << ": " << e.what() << "\n";
            }
            return 1;
        }
    }

    fs::path manifest_file = target_path / "nxapp.yaml";
    if (fs::exists(manifest_file) && !force) {
        // Validate existing manifest
        auto proj = project::NXDevProject::load_explicit(target_path.string());
        if (proj.has_value() && proj->is_valid()) {
            if (json_output) {
                std::cout << "{\"status\":\"already_initialized\",\"valid\":true,\"path\":\"" << target_path.string() << "\"}\n";
            } else {
                std::cout << c_cyan("Info: ") << "Directory is already an initialized NXDev project ("
                          << manifest_file << ")\n"
                          << "Run 'nxdev manifest validate' to verify or 'nxdev init --force' to reconfigure.\n";
            }
            return 0;
        } else {
            if (json_output) {
                std::cout << "{\"status\":\"already_initialized\",\"valid\":false,\"path\":\"" << target_path.string() << "\"}\n";
            } else {
                std::cerr << c_red("Warning: ") << "nxapp.yaml already exists in " << target_path
                          << " but contains validation errors.\n"
                          << "Run 'nxdev manifest validate' to inspect or pass '--force' to recreate.\n";
            }
            return 1;
        }
    }

    // Determine application name and ID
    if (app_name.empty()) {
        std::string dirname = target_path.filename().string();
        if (dirname.empty() || dirname == ".") {
            dirname = fs::current_path().filename().string();
        }
        app_name = dirname.empty() ? "My Switch App" : dirname;
    }
    if (app_id.empty()) {
        app_id = slugify(app_name);
    }

    // Inspect existing directory structure
    bool has_c_src = false;
    bool has_cpp_src = false;
    bool is_devkitpro_makefile = false;
    bool has_cmakelists = fs::exists(target_path / "CMakeLists.txt");
    std::string detected_main_src;

    if (fs::exists(target_path / "Makefile")) {
        std::ifstream mf(target_path / "Makefile");
        std::string mfc((std::istreambuf_iterator<char>(mf)), std::istreambuf_iterator<char>());
        if (mfc.find("DEVKITPRO") != std::string::npos || mfc.find("libnx") != std::string::npos || mfc.find("switch") != std::string::npos) {
            is_devkitpro_makefile = true;
        }
    }

    // Search for source files
    std::vector<fs::path> candidate_dirs = {target_path / "src", target_path / "source", target_path};
    for (const auto& dir : candidate_dirs) {
        if (!fs::is_directory(dir)) continue;
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            std::string stem = entry.path().stem().string();
            if (ext == ".c") {
                has_c_src = true;
                if (detected_main_src.empty() || stem == "main") {
                    detected_main_src = fs::relative(entry.path(), target_path).string();
                }
            } else if (ext == ".cpp" || ext == ".cxx" || ext == ".cc") {
                has_cpp_src = true;
                if (detected_main_src.empty() || stem == "main") {
                    detected_main_src = fs::relative(entry.path(), target_path).string();
                }
            }
        }
    }

    std::string std_lang = has_cpp_src ? "c++20" : (has_c_src ? "c11" : "c++20");
    std::string lang_id = has_cpp_src ? "cpp" : (has_c_src ? "c" : "cpp");
    std::string dep_decl = (has_cpp_src || !has_c_src) ? "  - nxdev.core\n  - nxdev.input" : "";

    // 1. Generate nxapp.yaml
    std::ostringstream mf_yaml;
    mf_yaml << "schemaVersion: 1\n\n"
            << "application:\n"
            << "  titleId: \"" << title_id << "\"\n"
            << "  name: \"" << app_name << "\"\n"
            << "  author: \"" << author << "\"\n"
            << "  version: \"" << version << "\"\n"
            << "  description: \"Nintendo Switch application initialized with NXDev\"\n\n";

    if (!dep_decl.empty()) {
        mf_yaml << "dependencies:\n" << dep_decl << "\n\n";
    } else {
        mf_yaml << "dependencies: []\n\n";
    }

    mf_yaml << "build:\n"
            << "  language: " << lang_id << "\n"
            << "  standard: \"" << std_lang << "\"\n"
            << "  defaultProfile: debug\n";

    {
        std::ofstream out(manifest_file);
        if (!out) {
            std::cerr << c_red("Error: ") << "Failed to write " << manifest_file << "\n";
            return 1;
        }
        out << mf_yaml.str();
    }

    // 2. Safe CMakeLists.txt integration
    bool cmake_generated = false;
    if (!has_cmakelists) {
        if (detected_main_src.empty()) {
            fs::create_directories(target_path / "src");
            detected_main_src = "src/main.cpp";
            std::ofstream src_out(target_path / detected_main_src);
            src_out << "#include <nxdev/nxdev.hpp>\n"
                    << "#include <nxdev/input.hpp>\n\n"
                    << "int main(int argc, char* argv[]) {\n"
                    << "    nxdev::App app(\"" << app_name << "\");\n"
                    << "    nxdev::PadState pad(nxdev::PadId::Player1);\n"
                    << "    std::cout << \"Welcome to " << app_name << "! Press PLUS to exit.\\n\";\n\n"
                    << "    while (app.is_running()) {\n"
                    << "        pad.update();\n"
                    << "        if (pad.is_pressed(nxdev::Button::Plus)) break;\n"
                    << "        app.update();\n"
                    << "    }\n"
                    << "    return 0;\n"
                    << "}\n";
        }

        std::ofstream cm_out(target_path / "CMakeLists.txt");
        cm_out << "cmake_minimum_required(VERSION 3.20)\n"
               << "project(" << app_id << " LANGUAGES C CXX)\n\n"
               << "find_package(NXDev CONFIG REQUIRED)\n\n"
               << "nxdev_add_application(" << app_id << "\n"
               << "    SOURCES\n"
               << "        " << detected_main_src << "\n"
               << ")\n";
        cmake_generated = true;
    }

    // 3. Update .gitignore
    std::string gi_err;
    update_gitignore(target_path, gi_err);

    // 4. Validate generated manifest
    auto proj = project::NXDevProject::load_explicit(target_path.string());
    bool valid = proj.has_value() && proj->is_valid();

    if (json_output) {
        std::cout << "{\n"
                  << "  \"status\": \"" << (valid ? "success" : "warning") << "\",\n"
                  << "  \"path\": \"" << target_path.string() << "\",\n"
                  << "  \"manifest\": \"" << manifest_file.string() << "\",\n"
                  << "  \"application\": {\n"
                  << "    \"id\": \"" << app_id << "\",\n"
                  << "    \"name\": \"" << app_name << "\",\n"
                  << "    \"author\": \"" << author << "\",\n"
                  << "    \"version\": \"" << version << "\",\n"
                  << "    \"titleId\": \"" << title_id << "\"\n"
                  << "  },\n"
                  << "  \"cmakeGenerated\": " << (cmake_generated ? "true" : "false") << ",\n"
                  << "  \"devkitProMakefileDetected\": " << (is_devkitpro_makefile ? "true" : "false") << "\n"
                  << "}\n";
    } else {
        std::cout << c_green("✓") << " Initialized NXDev project in " << c_bold(target_path.string()) << "\n\n"
                  << "Created:\n"
                  << "  - nxapp.yaml (Application Manifest v1)\n";
        if (cmake_generated) {
            std::cout << "  - CMakeLists.txt (NXDev build configuration)\n";
        } else if (has_cmakelists) {
            std::cout << "  - Preserved existing CMakeLists.txt\n";
        }
        if (is_devkitpro_makefile) {
            std::cout << "\n" << c_cyan("Note: ") << "Detected existing devkitPro Makefile project.\n"
                      << "NXDev metadata was initialized and existing build files were safely preserved.\n";
        }

        std::cout << "\n" << c_bold("Next Steps:") << "\n"
                  << "  1. Run diagnostics:    " << c_cyan("nxdev doctor") << "\n"
                  << "  2. Configure project:  " << c_cyan("nxdev configure") << "\n"
                  << "  3. Build ELF binary:   " << c_cyan("nxdev build") << "\n"
                  << "  4. Package NRO bundle: " << c_cyan("nxdev pack nro") << "\n";
    }

    return valid ? 0 : 1;
}

} // namespace nxdev::cli
