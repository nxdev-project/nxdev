#include <nxdev/packages/resolver.hpp>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

namespace nxdev::packages {

DependencyResolver::DependencyResolver(const PackageRegistry& registry)
    : registry_(registry) {}

static std::string escape_json_str(std::string_view s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << c;
    }
    return o.str();
}

ResolutionResult DependencyResolver::resolve(const std::vector<std::string>& requested_ids) const {
    ResolutionResult result;
    result.requested_ids = requested_ids;

    if (requested_ids.empty()) {
        result.success = true;
        return result;
    }

    // 1. Verify existence of all requested IDs and canonicalize them
    std::vector<std::string> canonical_requested;
    for (const auto& raw_id : requested_ids) {
        const auto* pkg = registry_.find(raw_id);
        if (!pkg) {
            result.success = false;
            result.errors.push_back("Unknown dependency: '" + raw_id + "'. Run 'nxdev package list' to see available modules.");
        } else {
            canonical_requested.push_back(pkg->id);
        }
    }

    if (!result.success) {
        return result;
    }

    // 2. Perform graph traversal and topological sort
    // State: 0 = unvisited, 1 = visiting (in recursion stack), 2 = visited
    std::unordered_map<std::string, int> visit_state;
    std::vector<std::string> sorted_ids;
    std::vector<std::string> current_path;

    auto visit = [&](auto& self, const std::string& node_id) -> bool {
        int state = visit_state[node_id];
        if (state == 1) {
            // Cycle detected!
            std::string cycle_str;
            for (const auto& n : current_path) {
                cycle_str += n + " -> ";
            }
            cycle_str += node_id;
            result.errors.push_back("Circular dependency detected: " + cycle_str);
            return false;
        }
        if (state == 2) {
            return true; // Already processed
        }

        visit_state[node_id] = 1;
        current_path.push_back(node_id);

        const auto* pkg = registry_.find(node_id);
        if (pkg) {
            for (const auto& dep_id : pkg->dependencies) {
                const auto* dep_pkg = registry_.find(dep_id);
                if (!dep_pkg) {
                    result.errors.push_back("Module '" + node_id + "' depends on unknown package '" + dep_id + "'");
                    return false;
                }
                if (!self(self, dep_pkg->id)) {
                    return false;
                }
            }
        }

        current_path.pop_back();
        visit_state[node_id] = 2;
        sorted_ids.push_back(node_id);
        return true;
    };

    for (const auto& id : canonical_requested) {
        if (visit_state[id] == 0) {
            if (!visit(visit, id)) {
                result.success = false;
                return result;
            }
        }
    }

    result.ordered_module_ids = sorted_ids;

    // 3. Populate result collections
    std::unordered_set<std::string> seen_dkp_pkgs;
    std::unordered_set<std::string> seen_cmake_tgts;

    for (const auto& mod_id : result.ordered_module_ids) {
        const auto* pkg = registry_.find(mod_id);
        if (!pkg) continue;

        result.resolved_packages.push_back(*pkg);

        if (pkg->is_builtin()) {
            result.builtin_modules.push_back(pkg->id);
        } else if (pkg->is_devkitpro()) {
            result.devkitpro_modules.push_back(pkg->id);
            for (const auto& dkp_name : pkg->devkitpro.packages) {
                if (seen_dkp_pkgs.insert(dkp_name).second) {
                    result.devkitpro_packages.push_back(dkp_name);
                }
            }
        }

        for (const auto& tgt : pkg->cmake.targets) {
            if (seen_cmake_tgts.insert(tgt).second) {
                result.cmake_targets.push_back(tgt);
            }
        }
    }

    result.success = true;
    return result;
}

ResolutionResult DependencyResolver::resolve_manifest(const manifest::Manifest& manifest) const {
    std::vector<std::string> dep_names;
    for (const auto& dep : manifest.dependencies()) {
        dep_names.push_back(dep.name);
    }
    return resolve(dep_names);
}

std::string ResolutionResult::to_json() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"success\": " << (success ? "true" : "false") << ",\n";

    ss << "  \"requested\": [";
    for (size_t i = 0; i < requested_ids.size(); ++i) {
        ss << "\"" << escape_json_str(requested_ids[i]) << "\"" << (i + 1 < requested_ids.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"ordered_modules\": [";
    for (size_t i = 0; i < ordered_module_ids.size(); ++i) {
        ss << "\"" << escape_json_str(ordered_module_ids[i]) << "\"" << (i + 1 < ordered_module_ids.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"builtin_modules\": [";
    for (size_t i = 0; i < builtin_modules.size(); ++i) {
        ss << "\"" << escape_json_str(builtin_modules[i]) << "\"" << (i + 1 < builtin_modules.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"devkitpro_modules\": [";
    for (size_t i = 0; i < devkitpro_modules.size(); ++i) {
        ss << "\"" << escape_json_str(devkitpro_modules[i]) << "\"" << (i + 1 < devkitpro_modules.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"devkitpro_packages\": [";
    for (size_t i = 0; i < devkitpro_packages.size(); ++i) {
        ss << "\"" << escape_json_str(devkitpro_packages[i]) << "\"" << (i + 1 < devkitpro_packages.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"cmake_targets\": [";
    for (size_t i = 0; i < cmake_targets.size(); ++i) {
        ss << "\"" << escape_json_str(cmake_targets[i]) << "\"" << (i + 1 < cmake_targets.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"errors\": [";
    for (size_t i = 0; i < errors.size(); ++i) {
        ss << "\"" << escape_json_str(errors[i]) << "\"" << (i + 1 < errors.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"warnings\": [";
    for (size_t i = 0; i < warnings.size(); ++i) {
        ss << "\"" << escape_json_str(warnings[i]) << "\"" << (i + 1 < warnings.size() ? ", " : "");
    }
    ss << "]\n";

    ss << "}";
    return ss.str();
}

} // namespace nxdev::packages
