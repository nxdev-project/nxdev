#include <nxdev/cli/commands.hpp>
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

struct TemplateInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string language;
    std::vector<std::string> tags;
    std::vector<std::string> dependencies;
};

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
    return slug.empty() ? "my-nx-app" : slug;
}

fs::path find_templates_root(const CommandContext& ctx) {
    // 1. Environment override
    if (const char* env_p = std::getenv("NXDEV_TEMPLATES_DIR"); env_p && fs::is_directory(env_p)) {
        return fs::path(env_p);
    }

    // 2. Current working directory
    fs::path cwd_cand = fs::current_path() / "templates";
    if (fs::is_directory(cwd_cand) && fs::exists(cwd_cand / "manifest.json")) {
        return cwd_cand;
    }

    // 3. Parent paths relative to current working directory
    fs::path p = fs::current_path();
    for (int i = 0; i < 4 && p != p.root_path(); ++i) {
        fs::path c = p / "templates";
        if (fs::is_directory(c) && fs::exists(c / "manifest.json")) {
            return c;
        }
        p = p.parent_path();
    }

    // 4. Project root fallback
    if (ctx.project) {
        fs::path p_cand = fs::path(ctx.project->root_path()) / "templates";
        if (fs::is_directory(p_cand) && fs::exists(p_cand / "manifest.json")) {
            return p_cand;
        }
    }

    return fs::path();
}

std::vector<TemplateInfo> load_templates(const fs::path& templates_root) {
    std::vector<TemplateInfo> list;
    // Built-in standard template definitions
    list.push_back({"console-cpp", "NXDevSDK C++ Application", "Recommended starting point using modern C++ abstractions (NXDev::Core and NXDev::Input)", "cpp", {"cpp", "nxdev", "sdk"}, {"nxdev.core", "nxdev.input"}});
    list.push_back({"minimal-c", "Minimal C (Raw libnx)", "Minimal Nintendo Switch application in C using raw libnx", "c", {"c", "libnx"}, {}});
    list.push_back({"minimal-cpp", "Minimal C++ (Raw libnx)", "Minimal Nintendo Switch application in C++20 using raw libnx", "cpp", {"cpp", "libnx"}, {}});
    list.push_back({"sdl2", "SDL2 Application", "2D graphics and sound application using devkitPro SDL2 portlib", "cpp", {"cpp", "sdl2"}, {"nxdev.core", "nxdev.sdl2"}});
    list.push_back({"deko3d", "deko3d Graphics Application (Experimental)", "Low-level 3D graphics application using deko3d API", "cpp", {"cpp", "deko3d"}, {"nxdev.core", "nxdev.deko3d"}});
    list.push_back({"nxdev-ui", "Borealis UI Application", "Modern GUI application using NXDev::Borealis abstraction layer (nxdev::ui)", "cpp", {"cpp", "ui", "borealis"}, {"nxdev.core", "nxdev.borealis"}});

    (void)templates_root;
    return list;
}

std::string replace_all(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

} // namespace

int NewCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    std::string project_name;
    std::string template_id = "console-cpp";
    std::string target_dir;
    std::string author;
    std::string version = "0.1.0";
    std::string title_id = "0100000000001000";
    bool list_only = false;
    bool as_json = false;
    bool force = false;

    for (size_t i = 0; i < args.size(); ++i) {
        const auto& arg = args[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: nxdev new [<project-name>] [options]\n\n"
                      << "Scaffold a new Nintendo Switch homebrew project.\n\n"
                      << "Arguments:\n"
                      << "  <project-name>       Human-friendly project display name (e.g. \"Super Star\")\n\n"
                      << "Options:\n"
                      << "  -t, --template <id>  Starter template (default: console-cpp)\n"
                      << "  -d, --dir <path>     Target output directory (default: ./<project-id>)\n"
                      << "  -a, --author <name>  Author name for manifest (default: current user)\n"
                      << "  --version <semver>   Initial version string (default: 0.1.0)\n"
                      << "  --title-id <hex>     16-hex Title ID (default: 0100000000001000)\n"
                      << "  -l, --list           List all available templates\n"
                      << "  -f, --force          Allow scaffolding into existing non-empty directory\n"
                      << "  --json               Output machine-readable JSON summary\n";
            return 0;
        } else if (arg == "--list" || arg == "-l") {
            list_only = true;
        } else if (arg == "--json") {
            as_json = true;
        } else if (arg == "--force" || arg == "-f") {
            force = true;
        } else if ((arg == "--template" || arg == "-t") && i + 1 < args.size()) {
            template_id = args[++i];
        } else if ((arg == "--dir" || arg == "-d") && i + 1 < args.size()) {
            target_dir = args[++i];
        } else if ((arg == "--author" || arg == "-a") && i + 1 < args.size()) {
            author = args[++i];
        } else if (arg == "--version" && i + 1 < args.size()) {
            version = args[++i];
        } else if (arg == "--title-id" && i + 1 < args.size()) {
            title_id = args[++i];
        } else if (!arg.starts_with("-") && project_name.empty()) {
            project_name = arg;
        }
    }

    fs::path templates_root = find_templates_root(ctx);
    auto available_templates = load_templates(templates_root);

    if (list_only) {
        if (as_json) {
            std::cout << "{\n  \"templates\": [\n";
            for (size_t i = 0; i < available_templates.size(); ++i) {
                const auto& t = available_templates[i];
                std::cout << "    {\n"
                          << "      \"id\": \"" << t.id << "\",\n"
                          << "      \"name\": \"" << t.name << "\",\n"
                          << "      \"description\": \"" << t.description << "\",\n"
                          << "      \"language\": \"" << t.language << "\"\n"
                          << "    }" << (i + 1 < available_templates.size() ? "," : "") << "\n";
            }
            std::cout << "  ]\n}\n";
        } else {
            std::cout << c_bold("Available NXDev Starter Templates:\n\n");
            for (const auto& t : available_templates) {
                std::cout << "  " << c_cyan(t.id)
                          << " " << c_bold(t.name) << " [" << t.language << "]\n"
                          << "                   " << t.description << "\n\n";
            }
        }
        return 0;
    }

    if (project_name.empty()) {
        project_name = "MySwitchApp";
    }

    if (author.empty()) {
        const char* user = std::getenv("USER");
        if (!user) user = std::getenv("USERNAME");
        author = user ? user : "Developer";
    }

    std::string project_id = slugify(project_name);

    fs::path out_path;
    if (!target_dir.empty()) {
        out_path = fs::absolute(target_dir);
    } else {
        out_path = fs::current_path() / project_id;
    }

    // Verify template existence
    bool found_tmpl = false;
    for (const auto& t : available_templates) {
        if (t.id == template_id) {
            found_tmpl = true;
            break;
        }
    }

    if (!found_tmpl) {
        if (as_json) {
            std::cout << "{\n  \"status\": \"error\",\n  \"error\": \"Unknown template ID: " << template_id << "\"\n}\n";
        } else {
            std::cerr << c_red("Error: ") << "Unknown template '" << template_id << "'. Run 'nxdev new --list' to see available templates.\n";
        }
        return 1;
    }

    // Check directory safety
    if (fs::exists(out_path) && !fs::is_empty(out_path) && !force) {
        if (as_json) {
            std::cout << "{\n  \"status\": \"error\",\n  \"error\": \"Target directory is not empty: " << out_path.string() << "\"\n}\n";
        } else {
            std::cerr << c_red("Error: ") << "Target directory '" << out_path.string() << "' already exists and is not empty.\n"
                      << "Use --force to overwrite existing files.\n";
        }
        return 1;
    }

    fs::create_directories(out_path);

    // Copy and substitute template files
    fs::path src_template_dir = templates_root / template_id;
    bool copied_any = false;

    if (fs::exists(src_template_dir)) {
        for (const auto& entry : fs::recursive_directory_iterator(src_template_dir)) {
            fs::path rel = fs::relative(entry.path(), src_template_dir);
            fs::path dest = out_path / rel;

            if (entry.is_directory()) {
                fs::create_directories(dest);
            } else if (entry.is_regular_file()) {
                fs::create_directories(dest.parent_path());

                std::ifstream in(entry.path());
                if (!in.is_open()) continue;
                std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

                // Apply template variables
                content = replace_all(content, "{{PROJECT_NAME}}", project_name);
                content = replace_all(content, "{{PROJECT_ID}}", project_id);
                content = replace_all(content, "{{PROJECT_AUTHOR}}", author);
                content = replace_all(content, "{{PROJECT_VERSION}}", version);
                content = replace_all(content, "{{PROJECT_TITLE_ID}}", title_id);

                std::ofstream out(dest);
                out << content;
                copied_any = true;
            }
        }
    }

    // Fallback if template files could not be read from disk directly
    if (!copied_any) {
        fs::path manifest_p = out_path / "nxapp.yaml";
        std::ofstream mf(manifest_p);
        mf << "schemaVersion: 1\n\n"
           << "application:\n"
           << "  titleId: \"" << title_id << "\"\n"
           << "  name: \"" << project_name << "\"\n"
           << "  author: \"" << author << "\"\n"
           << "  version: \"" << version << "\"\n"
           << "  description: \"Nintendo Switch application created with NXDev\"\n\n"
           << "dependencies:\n"
           << "  - nxdev.core\n"
           << "  - nxdev.input\n\n"
           << "build:\n"
           << "  language: cpp\n"
           << "  standard: \"c++20\"\n"
           << "  defaultProfile: debug\n";

        fs::path cm_p = out_path / "CMakeLists.txt";
        std::ofstream cm(cm_p);
        cm << "cmake_minimum_required(VERSION 3.20)\n"
           << "project(" << project_id << " CXX)\n\n"
           << "set(CMAKE_CXX_STANDARD 20)\n\n"
           << "find_package(NXDev REQUIRED)\n\n"
           << "add_executable(" << project_id << " src/main.cpp)\n"
           << "target_link_libraries(" << project_id << " PRIVATE NXDev::Core NXDev::Input)\n";

        fs::create_directories(out_path / "src");
        fs::path main_p = out_path / "src" / "main.cpp";
        std::ofstream mn(main_p);
        mn << "#include <nxdev/nxdev.hpp>\n"
           << "#include <nxdev/input.hpp>\n"
           << "#include <iostream>\n\n"
           << "int main(int argc, char* argv[]) {\n"
           << "    (void)argc;\n"
           << "    (void)argv;\n"
           << "    auto app_res = nxdev::App::create();\n"
           << "    if (app_res.failed()) return 1;\n"
           << "    auto& app = app_res.value();\n"
           << "    nxdev::input::Controller controller(nxdev::input::Player::Auto);\n"
           << "    while (app.is_running()) {\n"
           << "        controller.update();\n"
           << "        if (controller.is_button_down(nxdev::input::Button::Plus)) break;\n"
           << "    }\n"
           << "    return 0;\n"
           << "}\n";
    }

    if (as_json) {
        std::cout << "{\n"
                  << "  \"status\": \"success\",\n"
                  << "  \"projectName\": \"" << project_name << "\",\n"
                  << "  \"projectId\": \"" << project_id << "\",\n"
                  << "  \"template\": \"" << template_id << "\",\n"
                  << "  \"projectPath\": \"" << out_path.string() << "\",\n"
                  << "  \"manifestPath\": \"" << (out_path / "nxapp.yaml").string() << "\"\n"
                  << "}\n";
    } else {
        std::cout << c_green("✔") << " Successfully created project " << c_bold(project_name) << "!\n\n"
                  << "  Template:    " << template_id << "\n"
                  << "  Location:    " << out_path.string() << "\n"
                  << "  Manifest:    " << (out_path / "nxapp.yaml").string() << "\n\n"
                  << "To build and run your application:\n"
                  << c_cyan("  cd " + (target_dir.empty() ? project_id : target_dir)) << "\n"
                  << c_cyan("  nxdev build") << "\n"
                  << c_cyan("  nxdev run") << "\n";
    }

    return 0;
}

} // namespace nxdev::cli
