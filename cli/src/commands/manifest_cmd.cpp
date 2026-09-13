#include <nxdev/cli/commands.hpp>
#include <nxdev/manifest/parser.hpp>
#include <iostream>
#include <string>

namespace nxdev::cli {

int ManifestCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    if (args.empty()) {
        std::cout << "Usage: " << usage() << "\n\n";
        std::cout << "Subcommands:\n";
        std::cout << "  validate [path]          Validate nxapp.yaml and display diagnostics\n";
        std::cout << "  inspect [path] [--json]  Display normalized manifest details\n";
        std::cout << "  schema                   Display JSON schema information\n";
        return 0;
    }

    std::string sub = args[0];
    manifest::Parser parser;

    if (sub == "validate") {
        manifest::ParseResult result;
        if (args.size() > 1 && args[1].rfind("-", 0) != 0) {
            result = parser.parse_file(args[1]);
        } else if (ctx.project.has_value()) {
            result = parser.parse_file(ctx.project->manifest_path());
        } else {
            result = parser.load_from_discovery();
        }

        if (result.diagnostics.diagnostics().empty()) {
            std::cout << "==> Manifest validation SUCCESS: No errors or warnings found.\n";
            return 0;
        }

        result.diagnostics.print_summary(std::cout);
        return result.success ? 0 : 1;
    }

    if (sub == "inspect") {
        bool as_json = false;
        std::string path;

        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "--json") {
                as_json = true;
            } else if (path.empty() && args[i].rfind("-", 0) != 0) {
                path = args[i];
            }
        }

        manifest::ParseResult result;
        if (!path.empty()) {
            result = parser.parse_file(path);
        } else if (ctx.project.has_value()) {
            result = parser.parse_file(ctx.project->manifest_path());
        } else {
            result = parser.load_from_discovery();
        }

        if (!result.success || !result.manifest.has_value()) {
            std::cerr << "Error: Failed to load manifest for inspection.\n";
            result.diagnostics.print_summary(std::cerr);
            return 1;
        }

        if (as_json) {
            std::cout << result.manifest->to_json() << "\n";
        } else {
            std::cout << result.manifest->to_human_readable() << "\n";
        }
        return 0;
    }

    if (sub == "schema") {
        std::cout << "NXDevAppManifest v1 JSON Schema is located in 'manifest/schema/nxapp-v1.schema.json'.\n";
        std::cout << "Stable URL: https://raw.githubusercontent.com/nxdev-project/nxdev/main/manifest/schema/nxapp-v1.schema.json\n";
        return 0;
    }

    std::cerr << "Unknown manifest subcommand '" << sub << "'. Run 'nxdev manifest' for help.\n";
    return 1;
}

} // namespace nxdev::cli
