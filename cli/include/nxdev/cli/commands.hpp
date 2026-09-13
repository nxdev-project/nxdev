#pragma once

#include <nxdev/cli/command.hpp>

namespace nxdev::cli {

class VersionCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "version"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Display NXDev version and system target details"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev version [--json]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class EnvCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "env"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Display host and devkitPro environment summary"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev env [--json]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class DoctorCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "doctor"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Diagnose host environment, toolchain, and project setup"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev doctor [--json] [--build] [--pack] [--deploy]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class ProjectCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "project"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Inspect project metadata or print project root directory"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev project <info|root> [--json]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class ConfigCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "config"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Inspect NXDev global and local host configuration"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev config <path|list|get <key>>"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class ManifestCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "manifest"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Validate, inspect, or retrieve schema for nxapp.yaml manifests"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev manifest <validate|inspect|schema> [path] [--json]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class ConfigureCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "configure"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Configure CMake for the current NXDev Switch homebrew project"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev configure [--profile <debug|release>] [--fresh] [--json]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class BuildCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "build"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Compile the current NXDev Switch homebrew project into an ELF binary"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev build [--profile <debug|release>] [--clean] [--verbose] [--jobs <N>] [--json]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class CleanCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "clean"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Safely remove NXDev-managed build output directory"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev clean"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class PackageCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "package"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Manage NXDev SDK modules and devkitPro portlib dependencies"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev package <list|info|status|search|install|remove> [options]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class PackCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "pack"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Package the homebrew binary into NRO or NSP format"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev pack <nro|nsp> [options]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class NewCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "new"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Create a new NXDev Switch homebrew project from template"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev new <project-name>"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class DevicesCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "devices"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Manage target Nintendo Switch development devices"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev devices <list|add|remove|show|set-default|test> [options]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class RunCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "run"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Build, package, deploy, and run homebrew application on Switch via nxlink"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev run [--profile <debug|release>] [--device <id>] [--host <ip>] [--no-build] [--no-pack] [--args <args>] [--json-stream]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class DeployCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "deploy"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Deploy pre-built homebrew NRO artifact to Switch device"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev deploy [--artifact <path>] [--device <id>] [--host <ip>] [--json]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class SymbolizeCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "symbolize"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Symbolize crash addresses against Switch ELF using addr2line"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev symbolize [--elf <path>] [--base <addr>] [--json] <addresses...>"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class LogsCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "logs"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Inspect saved NXDev runtime session logs"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev logs <list|latest|show <session-id>> [--json]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class InitCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "init"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Initialize NXDev metadata and build integration in an existing directory"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev init [path] [--name <name>] [--author <author>] [--title-id <id>] [--version <ver>] [--force] [--non-interactive] [--json]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class PrepareCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "prepare"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Install and configure devkitPro toolchain prerequisites on Linux host"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev prepare [--check] [--repair] [--yes] [--dry-run] [--json]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

class SdkCommand : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "sdk"; }
    [[nodiscard]] std::string_view description() const noexcept override { return "Inspect or package the NXDevSDK distribution archive"; }
    [[nodiscard]] std::string_view usage() const noexcept override { return "nxdev sdk <info|package> [options]"; }
    int execute(std::span<const std::string> args, CommandContext& ctx) override;
};

} // namespace nxdev::cli
