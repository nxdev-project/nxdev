#include <nxdev/cli/commands.hpp>
#include <nxdev/config/host_config.hpp>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

namespace nxdev::cli {

namespace fs = std::filesystem;

int LogsCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    fs::path log_dir;
    if (ctx.project.has_value() && ctx.project->is_valid()) {
        log_dir = fs::path(ctx.project->root_path()) / ".nxdev" / "logs";
    } else {
        log_dir = fs::path(config::HostConfig::default_global_config_dir()) / "logs";
    }

    std::vector<fs::directory_entry> log_files;
    if (fs::exists(log_dir) && fs::is_directory(log_dir)) {
        for (const auto& entry : fs::directory_iterator(log_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".log") {
                log_files.push_back(entry);
            }
        }
    }

    std::sort(log_files.begin(), log_files.end(),
        [](const fs::directory_entry& a, const fs::directory_entry& b) {
            return a.last_write_time() > b.last_write_time(); // Most recent first
        });

    if (args.empty() || args[0] == "list") {
        bool json_output = (args.size() > 1 && args[1] == "--json");

        if (json_output) {
            std::ostringstream oss;
            oss << "{\"logs\":[";
            for (size_t i = 0; i < log_files.size(); ++i) {
                if (i > 0) oss << ",";
                oss << "{\"id\":\"" << log_files[i].path().stem().string() << "\""
                    << ",\"path\":\"" << log_files[i].path().string() << "\""
                    << ",\"size_bytes\":" << fs::file_size(log_files[i].path()) << "}";
            }
            oss << "]}";
            std::cout << oss.str() << "\n";
            return 0;
        }

        std::cout << "NXDev Runtime Session Logs (" << log_dir.string() << "):\n\n";
        if (log_files.empty()) {
            std::cout << "  (No session logs recorded yet)\n";
            return 0;
        }

        for (const auto& f : log_files) {
            std::cout << "  - " << f.path().stem().string()
                      << " (" << fs::file_size(f.path()) << " bytes)\n";
        }
        return 0;
    }

    std::string sub = args[0];

    if (sub == "latest") {
        if (log_files.empty()) {
            std::cerr << "No session logs found in " << log_dir.string() << "\n";
            return 1;
        }

        std::ifstream file(log_files.front().path());
        if (!file.is_open()) {
            std::cerr << "Error: Failed to open log file " << log_files.front().path() << "\n";
            return 1;
        }
        std::cout << file.rdbuf() << "\n";
        return 0;
    }

    if (sub == "show") {
        if (args.size() < 2) {
            std::cerr << "Error: 'logs show' requires session ID or filename.\n";
            return 1;
        }
        std::string query = args[1];
        fs::path target_path;

        if (fs::exists(query)) {
            target_path = query;
        } else {
            // Check by ID
            for (const auto& f : log_files) {
                if (f.path().stem().string() == query || f.path().filename().string() == query) {
                    target_path = f.path();
                    break;
                }
            }
        }

        if (target_path.empty() || !fs::exists(target_path)) {
            std::cerr << "Error: Session log '" << query << "' not found.\n";
            return 1;
        }

        std::ifstream file(target_path);
        if (!file.is_open()) {
            std::cerr << "Error: Failed to open log file " << target_path.string() << "\n";
            return 1;
        }
        std::cout << file.rdbuf() << "\n";
        return 0;
    }

    std::cerr << "Unknown logs subcommand: '" << sub << "'. Available: list, latest, show <id>.\n";
    return 1;
}

} // namespace nxdev::cli
