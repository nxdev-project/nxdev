#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <span>

namespace nxdev::cli {

class ICommand {
public:
    virtual ~ICommand() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual std::string_view description() const noexcept = 0;
    [[nodiscard]] virtual std::string_view usage() const noexcept = 0;
    virtual int execute(std::span<const std::string> args) = 0;
};

} // namespace nxdev::cli
