#pragma once

#include <nxdev/types.hpp>
#include <string_view>

namespace nxdev {

/**
 * @brief Result code wrapper compatible with Horizon OS / libnx result codes.
 * Libnx Result is a 32-bit integer formatted as module (9 bits) + description (13 bits).
 */
class Result {
public:
    constexpr Result() noexcept : raw_code_(0) {}
    constexpr explicit Result(u32 raw) noexcept : raw_code_(raw) {}

    [[nodiscard]] constexpr bool is_success() const noexcept {
        return raw_code_ == 0;
    }

    [[nodiscard]] constexpr bool is_failure() const noexcept {
        return raw_code_ != 0;
    }

    [[nodiscard]] constexpr u32 get_raw() const noexcept {
        return raw_code_;
    }

    [[nodiscard]] constexpr u32 get_module() const noexcept {
        return (raw_code_ >> 0) & 0x1FF;
    }

    [[nodiscard]] constexpr u32 get_description() const noexcept {
        return (raw_code_ >> 9) & 0x1FFF;
    }

    [[nodiscard]] std::string_view get_error_name() const noexcept;

    constexpr explicit operator bool() const noexcept {
        return is_success();
    }

    constexpr bool operator==(const Result& other) const noexcept = default;

    static constexpr Result Success() noexcept {
        return Result(0);
    }

private:
    u32 raw_code_{0};
};

namespace results {
    inline constexpr Result Success{0};
    inline constexpr Result NotInitialized{0x00010001};
    inline constexpr Result AlreadyInitialized{0x00010002};
    inline constexpr Result InvalidArgument{0x00010003};
    inline constexpr Result OutOfMemory{0x00010004};
}

} // namespace nxdev
