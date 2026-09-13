#pragma once

#include <nxdev/env/environment.hpp>
#include <nxdev/project/project.hpp>
#include <nxdev/config/host_config.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <optional>

namespace nxdev::cli {

struct CommandContext {
    std::optional<project::NXDevProject> project;
    env::Environment env;
    config::HostConfig config;
    std::string explicit_project_path;
    bool verbose{false};
    bool no_color{false};
};

class ICommand {
public:
    virtual ~ICommand() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual std::string_view description() const noexcept = 0;
    [[nodiscard]] virtual std::string_view usage() const noexcept = 0;
    virtual int execute(std::span<const std::string> args, CommandContext& ctx) = 0;
};

} // namespace nxdev::cli
