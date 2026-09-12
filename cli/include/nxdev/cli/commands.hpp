#pragma once

#include <nxdev/cli/command.hpp>

namespace nxdev::cli {

class VersionCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "version"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Display NXDev version and system target details"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev version"; }
    int execute(std::span<const std::string> args) override;
};

class DoctorCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "doctor"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Inspect and diagnose devkitPro toolchain and host environment"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev doctor"; }
    int execute(std::span<const std::string> args) override;
};

class BuildCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "build"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Compile the current NXDev Switch homebrew project"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev build [options]"; }
    int execute(std::span<const std::string> args) override;
};

class PackCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "pack"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Package the homebrew binary into NRO or NSP format"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev pack <nro|nsp> [options]"; }
    int execute(std::span<const std::string> args) override;
};

class NewCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "new"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Create a new NXDev Switch homebrew project from template"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev new <project-name>"; }
    int execute(std::span<const std::string> args) override;
};

class ManifestCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "manifest"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Validate, inspect, or retrieve schema for nxapp.yaml manifests"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev manifest <validate|inspect|schema> [path] [options]"; }
    int execute(std::span<const std::string> args) override;
};

} // namespace nxdev::cli
