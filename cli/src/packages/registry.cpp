#include <nxdev/packages/registry.hpp>
#include <nxdev/manifest/yaml.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

namespace nxdev::packages {

static std::string to_lower_str(std::string_view sv) {
    std::string s(sv);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

void PackageRegistry::rebuild_indices() {
    index_by_id_.clear();
    aliases_.clear();

    for (size_t i = 0; i < packages_.size(); ++i) {
        const auto& pkg = packages_[i];
        index_by_id_[pkg.id] = i;

        // Auto-alias: strip "nxdev." prefix (e.g. "nxdev.sdl2" -> "sdl2")
        if (pkg.id.rfind("nxdev.", 0) == 0 && pkg.id.size() > 6) {
            std::string short_name = pkg.id.substr(6);
            aliases_[short_name] = pkg.id;
        }

        // Auto-alias: devkitpro package names (e.g. "switch-sdl2" -> "nxdev.sdl2")
        for (const auto& dkp_pkg : pkg.devkitpro.packages) {
            aliases_[dkp_pkg] = pkg.id;
        }
    }
}

void PackageRegistry::add_package(PackageDefinition pkg) {
    auto it = index_by_id_.find(pkg.id);
    if (it != index_by_id_.end()) {
        packages_[it->second] = std::move(pkg);
    } else {
        packages_.push_back(std::move(pkg));
    }
    rebuild_indices();
}

const PackageDefinition* PackageRegistry::find(const std::string& id_or_alias) const {
    auto it = index_by_id_.find(id_or_alias);
    if (it != index_by_id_.end()) {
        return &packages_[it->second];
    }

    auto alias_it = aliases_.find(id_or_alias);
    if (alias_it != aliases_.end()) {
        auto target_it = index_by_id_.find(alias_it->second);
        if (target_it != index_by_id_.end()) {
            return &packages_[target_it->second];
        }
    }

    return nullptr;
}

bool PackageRegistry::contains(const std::string& id_or_alias) const {
    return find(id_or_alias) != nullptr;
}

std::vector<const PackageDefinition*> PackageRegistry::search(const std::string& query) const {
    std::string lq = to_lower_str(query);
    std::vector<const PackageDefinition*> results;

    for (const auto& pkg : packages_) {
        if (lq.empty() ||
            to_lower_str(pkg.id).find(lq) != std::string::npos ||
            to_lower_str(pkg.name).find(lq) != std::string::npos ||
            to_lower_str(pkg.description).find(lq) != std::string::npos ||
            to_lower_str(pkg.category).find(lq) != std::string::npos) {
            results.push_back(&pkg);
            continue;
        }

        bool matched_dkp = false;
        for (const auto& dkp : pkg.devkitpro.packages) {
            if (to_lower_str(dkp).find(lq) != std::string::npos) {
                matched_dkp = true;
                break;
            }
        }
        if (matched_dkp) {
            results.push_back(&pkg);
        }
    }

    return results;
}

Result<PackageRegistry> PackageRegistry::load_from_string(const std::string& content) {
    manifest::YamlParser parser;
    manifest::YamlParseError parse_err;
    auto root_opt = parser.parse(content, parse_err);

    if (!root_opt.has_value()) {
        return Result<PackageRegistry>(results::InvalidFormat);
    }

    const auto& root = root_opt.value();
    if (!root.is_mapping()) {
        return Result<PackageRegistry>(results::InvalidFormat);
    }

    PackageRegistry reg;

    if (const auto* ver_node = root.get("schema_version")) {
        reg.set_schema_version(ver_node->as_string());
    }

    const auto* pkgs_node = root.get("packages");
    if (!pkgs_node || !pkgs_node->is_sequence()) {
        return Result<PackageRegistry>(results::InvalidFormat);
    }

    for (const auto& item : pkgs_node->as_sequence()) {
        if (!item.is_mapping()) continue;

        PackageDefinition pkg;
        if (const auto* id_node = item.get("id")) pkg.id = id_node->as_string();
        if (const auto* name_node = item.get("name")) pkg.name = name_node->as_string();
        if (const auto* desc_node = item.get("description")) pkg.description = desc_node->as_string();
        if (const auto* cat_node = item.get("category")) pkg.category = cat_node->as_string();
        if (const auto* lic_node = item.get("license")) pkg.license = lic_node->as_string();
        if (const auto* url_node = item.get("upstream_url")) pkg.upstream_url = url_node->as_string();

        if (const auto* kind_node = item.get("kind")) {
            auto kind_opt = parse_package_kind(kind_node->as_string());
            if (kind_opt.has_value()) {
                pkg.kind = kind_opt.value();
            }
        }

        if (const auto* deps_node = item.get("dependencies")) {
            if (deps_node->is_sequence()) {
                for (const auto& d : deps_node->as_sequence()) {
                    pkg.dependencies.push_back(d.as_string());
                }
            }
        }

        if (const auto* dkp_node = item.get("devkitpro")) {
            if (dkp_node->is_mapping()) {
                if (const auto* pkgs = dkp_node->get("packages")) {
                    if (pkgs->is_sequence()) {
                        for (const auto& p : pkgs->as_sequence()) {
                            pkg.devkitpro.packages.push_back(p.as_string());
                        }
                    }
                }
            }
        }

        if (const auto* cmake_node = item.get("cmake")) {
            if (cmake_node->is_mapping()) {
                if (const auto* tgts = cmake_node->get("targets")) {
                    if (tgts->is_sequence()) {
                        for (const auto& t : tgts->as_sequence()) {
                            pkg.cmake.targets.push_back(t.as_string());
                        }
                    }
                }
                if (const auto* hdrs = cmake_node->get("headers")) {
                    if (hdrs->is_sequence()) {
                        for (const auto& h : hdrs->as_sequence()) {
                            pkg.cmake.headers.push_back(h.as_string());
                        }
                    }
                }
                if (const auto* libs = cmake_node->get("libraries")) {
                    if (libs->is_sequence()) {
                        for (const auto& l : libs->as_sequence()) {
                            pkg.cmake.libraries.push_back(l.as_string());
                        }
                    }
                }
            }
        }

        if (reg.contains(pkg.id)) {
            return Result<PackageRegistry>(results::AlreadyExists);
        }
        reg.add_package(std::move(pkg));
    }

    auto val_res = reg.validate();
    if (!val_res.is_success()) {
        return Result<PackageRegistry>(val_res.code());
    }

    return Result<PackageRegistry>(std::move(reg));
}

Result<PackageRegistry> PackageRegistry::load_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return Result<PackageRegistry>(results::FileNotFound);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return load_from_string(buffer.str());
}

Result<PackageRegistry> PackageRegistry::load_default(const env::Environment& env) {
    // 1. Environment variable override
    if (const char* env_path = std::getenv("NXDEV_PACKAGE_REGISTRY")) {
        if (fs::exists(env_path)) {
            return load_from_file(env_path);
        }
    }

    // 2. Look for package-registry/registry.json relative to workspace or known dirs
    std::vector<std::string> candidates = {
        "package-registry/registry.json",
        "../package-registry/registry.json",
        "../../package-registry/registry.json",
        "/usr/share/nxdev/package-registry/registry.json",
        "/opt/devkitpro/share/nxdev/package-registry/registry.json"
    };

    if (!env.host().is_wsl && !env.devkitpro().path.empty()) {
        candidates.push_back(env.devkitpro().path + "/share/nxdev/package-registry/registry.json");
    }

    for (const auto& path : candidates) {
        if (fs::exists(path)) {
            auto res = load_from_file(path);
            if (res.is_success()) {
                return res;
            }
        }
    }

    // 3. Fallback to built-in programmatic registry
    return Result<PackageRegistry>(create_default());
}

PackageRegistry PackageRegistry::create_default() {
    PackageRegistry reg;
    reg.set_schema_version("1.0");

    // Built-in modules
    reg.add_package(PackageDefinition{
        .id = "nxdev.core",
        .name = "NXDev Core SDK",
        .description = "Foundational C++ abstraction layer on top of libnx for Nintendo Switch homebrew",
        .kind = PackageKind::Builtin,
        .category = "core",
        .dependencies = {},
        .devkitpro = {},
        .cmake = {.targets = {"NXDev::Core"}, .headers = {"nxdev/nxdev.hpp", "nxdev/app.hpp", "nxdev/result.hpp"}, .libraries = {}},
        .license = "MIT",
        .upstream_url = ""
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.input",
        .name = "NXDev Input",
        .description = "Multi-player controller, handheld Joy-Con, button transitions, and multi-touch input",
        .kind = PackageKind::Builtin,
        .category = "input",
        .dependencies = {"nxdev.core"},
        .devkitpro = {},
        .cmake = {.targets = {"NXDev::Input"}, .headers = {"nxdev/input.hpp"}, .libraries = {}},
        .license = "MIT",
        .upstream_url = ""
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.filesystem",
        .name = "NXDev Filesystem",
        .description = "RomFS mounting/unmounting RAII guard, SD card paths, and robust safe file I/O",
        .kind = PackageKind::Builtin,
        .category = "filesystem",
        .dependencies = {"nxdev.core"},
        .devkitpro = {},
        .cmake = {.targets = {"NXDev::Filesystem"}, .headers = {"nxdev/filesystem.hpp"}, .libraries = {}},
        .license = "MIT",
        .upstream_url = ""
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.account",
        .name = "NXDev Account",
        .description = "Horizon OS user account discovery, 128-bit UID handling, and profile nickname metadata",
        .kind = PackageKind::Builtin,
        .category = "system",
        .dependencies = {"nxdev.core"},
        .devkitpro = {},
        .cmake = {.targets = {"NXDev::Account"}, .headers = {"nxdev/account.hpp"}, .libraries = {}},
        .license = "MIT",
        .upstream_url = ""
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.network",
        .name = "NXDev Network",
        .description = "BSD socket driver lifecycle management, IP discovery, and network status queries",
        .kind = PackageKind::Builtin,
        .category = "networking",
        .dependencies = {"nxdev.core"},
        .devkitpro = {},
        .cmake = {.targets = {"NXDev::Network"}, .headers = {"nxdev/network.hpp"}, .libraries = {}},
        .license = "MIT",
        .upstream_url = ""
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.time",
        .name = "NXDev Time",
        .description = "High-precision monotonic clock, 19.2MHz ARM hardware ticks, and calendar DateTime",
        .kind = PackageKind::Builtin,
        .category = "system",
        .dependencies = {"nxdev.core"},
        .devkitpro = {},
        .cmake = {.targets = {"NXDev::Time"}, .headers = {"nxdev/time.hpp"}, .libraries = {}},
        .license = "MIT",
        .upstream_url = ""
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.power",
        .name = "NXDev Power",
        .description = "Operation mode (Handheld/Docked), battery percentage/charging info, and applet event hooks",
        .kind = PackageKind::Builtin,
        .category = "system",
        .dependencies = {"nxdev.core"},
        .devkitpro = {},
        .cmake = {.targets = {"NXDev::Power"}, .headers = {"nxdev/power.hpp"}, .libraries = {}},
        .license = "MIT",
        .upstream_url = ""
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.display",
        .name = "NXDev Display",
        .description = "Console and basic display facilities for Nintendo Switch homebrew",
        .kind = PackageKind::Builtin,
        .category = "graphics",
        .dependencies = {"nxdev.core"},
        .devkitpro = {},
        .cmake = {.targets = {"NXDev::Display"}, .headers = {"nxdev/display.hpp"}, .libraries = {}},
        .license = "MIT",
        .upstream_url = ""
    });

    // devkitPro-backed portlibs
    reg.add_package(PackageDefinition{
        .id = "nxdev.sdl2",
        .name = "SDL2",
        .description = "Simple DirectMedia Layer 2 cross-platform multimedia library for Nintendo Switch",
        .kind = PackageKind::DevkitPro,
        .category = "graphics",
        .dependencies = {"nxdev.core"},
        .devkitpro = {.packages = {"switch-sdl2"}},
        .cmake = {.targets = {"NXDev::SDL2"}, .headers = {"SDL2/SDL.h"}, .libraries = {"libSDL2.a"}},
        .license = "zlib",
        .upstream_url = "https://www.libsdl.org/"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.sdl2-image",
        .name = "SDL2 Image",
        .description = "Image loading library supporting PNG, JPEG, GIF, WebP, and BMP for SDL2",
        .kind = PackageKind::DevkitPro,
        .category = "graphics",
        .dependencies = {"nxdev.sdl2"},
        .devkitpro = {.packages = {"switch-sdl2_image"}},
        .cmake = {.targets = {"NXDev::SDL2Image"}, .headers = {"SDL2/SDL_image.h"}, .libraries = {"libSDL2_image.a"}},
        .license = "zlib",
        .upstream_url = "https://github.com/libsdl-org/SDL_image"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.sdl2-mixer",
        .name = "SDL2 Mixer",
        .description = "Multi-channel audio playback and mixing library supporting WAV, MP3, OGG, FLAC, and MOD",
        .kind = PackageKind::DevkitPro,
        .category = "audio",
        .dependencies = {"nxdev.sdl2"},
        .devkitpro = {.packages = {"switch-sdl2_mixer"}},
        .cmake = {.targets = {"NXDev::SDL2Mixer"}, .headers = {"SDL2/SDL_mixer.h"}, .libraries = {"libSDL2_mixer.a"}},
        .license = "zlib",
        .upstream_url = "https://github.com/libsdl-org/SDL_mixer"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.sdl2-ttf",
        .name = "SDL2 TTF",
        .description = "TrueType font rendering library for SDL2 applications",
        .kind = PackageKind::DevkitPro,
        .category = "fonts",
        .dependencies = {"nxdev.sdl2", "nxdev.freetype"},
        .devkitpro = {.packages = {"switch-sdl2_ttf"}},
        .cmake = {.targets = {"NXDev::SDL2TTF"}, .headers = {"SDL2/SDL_ttf.h"}, .libraries = {"libSDL2_ttf.a"}},
        .license = "zlib",
        .upstream_url = "https://github.com/libsdl-org/SDL_ttf"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.deko3d",
        .name = "deko3d",
        .description = "Low-level, high-performance Vulkan-like 3D graphics API for Nintendo Switch Maxwell GPU",
        .kind = PackageKind::DevkitPro,
        .category = "graphics",
        .dependencies = {"nxdev.core"},
        .devkitpro = {.packages = {"deko3d"}},
        .cmake = {.targets = {"NXDev::Deko3D"}, .headers = {"deko3d.hpp", "deko3d.h"}, .libraries = {"libdeko3d.a"}},
        .license = "Zlib",
        .upstream_url = "https://github.com/devkitPro/deko3d"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.curl",
        .name = "libcurl",
        .description = "Multiprotocol file transfer and HTTP/HTTPS client library",
        .kind = PackageKind::DevkitPro,
        .category = "networking",
        .dependencies = {"nxdev.network", "nxdev.mbedtls", "nxdev.zlib"},
        .devkitpro = {.packages = {"switch-curl"}},
        .cmake = {.targets = {"NXDev::Curl"}, .headers = {"curl/curl.h"}, .libraries = {"libcurl.a"}},
        .license = "curl",
        .upstream_url = "https://curl.se/"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.mbedtls",
        .name = "mbedTLS",
        .description = "Lightweight, modular SSL/TLS and cryptographic library",
        .kind = PackageKind::DevkitPro,
        .category = "networking",
        .dependencies = {"nxdev.core"},
        .devkitpro = {.packages = {"switch-mbedtls"}},
        .cmake = {.targets = {"NXDev::MbedTLS"}, .headers = {"mbedtls/ssl.h"}, .libraries = {"libmbedtls.a", "libmbedcrypto.a", "libmbedx509.a"}},
        .license = "Apache-2.0",
        .upstream_url = "https://github.com/Mbed-TLS/mbedtls"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.freetype",
        .name = "FreeType",
        .description = "High-quality, portable font engine for rasterizing glyphs",
        .kind = PackageKind::DevkitPro,
        .category = "fonts",
        .dependencies = {"nxdev.png", "nxdev.zlib"},
        .devkitpro = {.packages = {"switch-freetype"}},
        .cmake = {.targets = {"NXDev::FreeType"}, .headers = {"ft2build.h"}, .libraries = {"libfreetype.a"}},
        .license = "FTL / GPLv2",
        .upstream_url = "https://freetype.org/"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.opus",
        .name = "Opus",
        .description = "Interactive audio and speech codec and opusfile container decoder",
        .kind = PackageKind::DevkitPro,
        .category = "audio",
        .dependencies = {"nxdev.ogg"},
        .devkitpro = {.packages = {"switch-libopus", "switch-opusfile"}},
        .cmake = {.targets = {"NXDev::Opus"}, .headers = {"opus/opus.h", "opus/opusfile.h"}, .libraries = {"libopus.a", "libopusfile.a"}},
        .license = "BSD-3-Clause",
        .upstream_url = "https://opus-codec.org/"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.zlib",
        .name = "zlib",
        .description = "Massively spiffy yet delicately unobtrusive compression library",
        .kind = PackageKind::DevkitPro,
        .category = "compression",
        .dependencies = {"nxdev.core"},
        .devkitpro = {.packages = {"switch-zlib"}},
        .cmake = {.targets = {"NXDev::Zlib"}, .headers = {"zlib.h"}, .libraries = {"libz.a"}},
        .license = "Zlib",
        .upstream_url = "https://zlib.net/"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.png",
        .name = "libpng",
        .description = "Official Portable Network Graphics reference library",
        .kind = PackageKind::DevkitPro,
        .category = "graphics",
        .dependencies = {"nxdev.zlib"},
        .devkitpro = {.packages = {"switch-libpng"}},
        .cmake = {.targets = {"NXDev::Png"}, .headers = {"png.h"}, .libraries = {"libpng.a"}},
        .license = "libpng-2.0",
        .upstream_url = "http://www.libpng.org/pub/png/libpng.html"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.jpeg",
        .name = "libjpeg-turbo",
        .description = "High-speed SIMD-accelerated JPEG image codec",
        .kind = PackageKind::DevkitPro,
        .category = "graphics",
        .dependencies = {"nxdev.core"},
        .devkitpro = {.packages = {"switch-libjpeg-turbo"}},
        .cmake = {.targets = {"NXDev::JpegTurbo"}, .headers = {"jpeglib.h"}, .libraries = {"libjpeg.a"}},
        .license = "IJG / BSD-3-Clause",
        .upstream_url = "https://libjpeg-turbo.org/"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.ogg",
        .name = "libogg",
        .description = "Ogg bitstream container multimedia format library",
        .kind = PackageKind::DevkitPro,
        .category = "audio",
        .dependencies = {"nxdev.core"},
        .devkitpro = {.packages = {"switch-libogg"}},
        .cmake = {.targets = {"NXDev::Ogg"}, .headers = {"ogg/ogg.h"}, .libraries = {"libogg.a"}},
        .license = "BSD-3-Clause",
        .upstream_url = "https://xiph.org/ogg/"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.vorbis",
        .name = "libvorbis",
        .description = "General purpose lossy audio and music compression codec",
        .kind = PackageKind::DevkitPro,
        .category = "audio",
        .dependencies = {"nxdev.ogg"},
        .devkitpro = {.packages = {"switch-libvorbis"}},
        .cmake = {.targets = {"NXDev::Vorbis"}, .headers = {"vorbis/codec.h"}, .libraries = {"libvorbis.a", "libvorbisfile.a"}},
        .license = "BSD-3-Clause",
        .upstream_url = "https://xiph.org/vorbis/"
    });

    reg.add_package(PackageDefinition{
        .id = "nxdev.physfs",
        .name = "PhysicsFS",
        .description = "Abstraction layer for archive and file system access (ZIP, 7z, ISO, etc.)",
        .kind = PackageKind::DevkitPro,
        .category = "filesystem",
        .dependencies = {"nxdev.core"},
        .devkitpro = {.packages = {"switch-physfs"}},
        .cmake = {.targets = {"NXDev::PhysFS"}, .headers = {"physfs.h"}, .libraries = {"libphysfs.a"}},
        .license = "zlib",
        .upstream_url = "https://icculus.org/physfs/"
    });

    return reg;
}

Result<void> PackageRegistry::validate() const {
    if (schema_version_ != "1.0" && schema_version_ != "1") {
        return Result<void>(results::InvalidFormat);
    }

    std::unordered_set<std::string> seen_ids;
    for (const auto& pkg : packages_) {
        if (pkg.id.empty()) {
            return Result<void>(results::InvalidFormat);
        }
        if (pkg.id.rfind("nxdev.", 0) != 0) {
            return Result<void>(results::InvalidFormat);
        }
        if (seen_ids.count(pkg.id)) {
            return Result<void>(results::AlreadyExists);
        }
        seen_ids.insert(pkg.id);

        if (pkg.kind == PackageKind::DevkitPro && pkg.devkitpro.packages.empty()) {
            return Result<void>(results::InvalidFormat);
        }
    }

    // Verify all dependencies exist and no cycles exist
    for (const auto& pkg : packages_) {
        for (const auto& dep : pkg.dependencies) {
            if (!contains(dep)) {
                return Result<void>(results::NotFound);
            }
        }
    }

    // Cycle detection using DFS coloring (0 = unvisited, 1 = visiting, 2 = visited)
    std::unordered_map<std::string, int> state;
    for (const auto& pkg : packages_) {
        state[pkg.id] = 0;
    }

    auto dfs = [&](auto& self, const std::string& current_id, std::vector<std::string>& path) -> Result<void> {
        state[current_id] = 1;
        path.push_back(current_id);

        const auto* pkg = find(current_id);
        if (pkg) {
            for (const auto& dep : pkg->dependencies) {
                if (state[dep] == 1) {
                    return Result<void>(results::InvalidFormat);
                }
                if (state[dep] == 0) {
                    auto res = self(self, dep, path);
                    if (!res.is_success()) return res;
                }
            }
        }

        path.pop_back();
        state[current_id] = 2;
        return Result<void>();
    };

    std::vector<std::string> path;
    for (const auto& pkg : packages_) {
        if (state[pkg.id] == 0) {
            auto res = dfs(dfs, pkg.id, path);
            if (!res.is_success()) return res;
        }
    }

    return Result<void>();
}

std::string PackageRegistry::to_json() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"schema_version\": \"" << schema_version_ << "\",\n";
    ss << "  \"packages\": [\n";

    for (size_t i = 0; i < packages_.size(); ++i) {
        std::string pkg_json = packages_[i].to_json();
        // Indent lines
        std::istringstream stream(pkg_json);
        std::string line;
        bool first_line = true;
        while (std::getline(stream, line)) {
            ss << (first_line ? "    " : "    ") << line << "\n";
            first_line = false;
        }
        if (i + 1 < packages_.size()) {
            // Replace trailing newline with comma
            if (!ss.str().empty() && ss.str().back() == '\n') {
                std::string current = ss.str();
                current.pop_back();
                ss.str("");
                ss << current << ",\n";
            }
        }
    }

    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

} // namespace nxdev::packages
