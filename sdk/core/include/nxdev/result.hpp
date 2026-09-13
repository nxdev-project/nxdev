#pragma once

#include <nxdev/types.hpp>
#include <string_view>
#include <optional>
#include <variant>
#include <utility>
#include <cassert>

namespace nxdev {

/**
 * @brief Representation of a raw Horizon OS / libnx 32-bit Result Code.
 * Layout: Module (bits 0-8, 9 bits), Description (bits 9-21, 13 bits).
 */
class ResultCode {
public:
    constexpr ResultCode() noexcept : raw_code_(0) {}
    constexpr explicit ResultCode(u32 raw) noexcept : raw_code_(raw) {}

    [[nodiscard]] constexpr bool is_success() const noexcept {
        return raw_code_ == 0;
    }

    [[nodiscard]] constexpr bool is_failure() const noexcept {
        return raw_code_ != 0;
    }

    [[nodiscard]] constexpr bool succeeded() const noexcept {
        return is_success();
    }

    [[nodiscard]] constexpr bool failed() const noexcept {
        return is_failure();
    }

    [[nodiscard]] constexpr u32 raw() const noexcept {
        return raw_code_;
    }

    [[nodiscard]] constexpr u32 get_raw() const noexcept {
        return raw_code_;
    }

    [[nodiscard]] constexpr u32 native_result() const noexcept {
        return raw_code_;
    }

    [[nodiscard]] constexpr u32 module() const noexcept {
        return (raw_code_ >> 0) & 0x1FF;
    }

    [[nodiscard]] constexpr u32 description() const noexcept {
        return (raw_code_ >> 9) & 0x1FFF;
    }

    [[nodiscard]] std::string_view get_error_name() const noexcept;
    [[nodiscard]] std::string_view error_name() const noexcept { return get_error_name(); }

    constexpr explicit operator bool() const noexcept {
        return is_success();
    }

    constexpr bool operator==(const ResultCode& other) const noexcept = default;

    static constexpr ResultCode Success() noexcept {
        return ResultCode(0);
    }

private:
    u32 raw_code_{0};
};

namespace results {
    inline constexpr ResultCode Success{0};
    inline constexpr ResultCode NotInitialized{0x00010001};
    inline constexpr ResultCode AlreadyInitialized{0x00010002};
    inline constexpr ResultCode InvalidArgument{0x00010003};
    inline constexpr ResultCode OutOfMemory{0x00010004};
    inline constexpr ResultCode NotFound{0x00010005};
    inline constexpr ResultCode NotSupported{0x00010006};
    inline constexpr ResultCode InvalidFormat{0x00010007};
    inline constexpr ResultCode AlreadyExists{0x00010008};
    inline constexpr ResultCode FileNotFound{0x00010009};
}

/**
 * @brief Primary template for typed NXDev Result<T>.
 */
template <typename T>
class [[nodiscard]] Result {
public:
    using value_type = T;

    constexpr Result(T value) : storage_(std::move(value)) {} // NOLINT
    constexpr Result(ResultCode error) noexcept : storage_(error) { // NOLINT
        assert(error.is_failure() && "Constructing a failure Result with a success code is invalid");
    }
    constexpr Result(u32 raw_error) noexcept : storage_(ResultCode(raw_error)) { // NOLINT
        assert(raw_error != 0 && "Constructing a failure Result with a 0 code is invalid");
    }

    [[nodiscard]] constexpr bool is_success() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    [[nodiscard]] constexpr bool is_failure() const noexcept {
        return std::holds_alternative<ResultCode>(storage_);
    }

    [[nodiscard]] constexpr bool succeeded() const noexcept {
        return is_success();
    }

    [[nodiscard]] constexpr bool failed() const noexcept {
        return is_failure();
    }

    constexpr explicit operator bool() const noexcept {
        return is_success();
    }

    [[nodiscard]] constexpr u32 raw() const noexcept {
        if (is_failure()) {
            return std::get<ResultCode>(storage_).raw();
        }
        return 0;
    }

    [[nodiscard]] constexpr ResultCode code() const noexcept {
        if (is_failure()) {
            return std::get<ResultCode>(storage_);
        }
        return ResultCode::Success();
    }

    [[nodiscard]] constexpr u32 native_result() const noexcept {
        return raw();
    }

    [[nodiscard]] constexpr u32 module() const noexcept {
        return code().module();
    }

    [[nodiscard]] constexpr u32 description() const noexcept {
        return code().description();
    }

    [[nodiscard]] std::string_view error_name() const noexcept {
        return code().error_name();
    }

    [[nodiscard]] T& value() & {
        assert(is_success() && "Attempted to access value() on a failed Result");
        return std::get<T>(storage_);
    }

    [[nodiscard]] const T& value() const & {
        assert(is_success() && "Attempted to access value() on a failed Result");
        return std::get<T>(storage_);
    }

    [[nodiscard]] T&& value() && {
        assert(is_success() && "Attempted to access value() on a failed Result");
        return std::get<T>(std::move(storage_));
    }

    [[nodiscard]] T* operator->() {
        return &value();
    }

    [[nodiscard]] const T* operator->() const {
        return &value();
    }

    [[nodiscard]] T& operator*() & {
        return value();
    }

    [[nodiscard]] const T& operator*() const & {
        return value();
    }

    template <typename U>
    [[nodiscard]] T value_or(U&& default_value) const & {
        if (is_success()) {
            return std::get<T>(storage_);
        }
        return static_cast<T>(std::forward<U>(default_value));
    }

    template <typename U>
    [[nodiscard]] T value_or(U&& default_value) && {
        if (is_success()) {
            return std::get<T>(std::move(storage_));
        }
        return static_cast<T>(std::forward<U>(default_value));
    }

private:
    std::variant<T, ResultCode> storage_;
};

/**
 * @brief Specialization for void results (Result<void>).
 */
template <>
class [[nodiscard]] Result<void> {
public:
    using value_type = void;

    constexpr Result() noexcept : code_(ResultCode::Success()) {}
    constexpr Result(ResultCode error) noexcept : code_(error) {} // NOLINT
    constexpr Result(u32 raw_code) noexcept : code_(ResultCode(raw_code)) {} // NOLINT

    [[nodiscard]] constexpr bool is_success() const noexcept {
        return code_.is_success();
    }

    [[nodiscard]] constexpr bool is_failure() const noexcept {
        return code_.is_failure();
    }

    [[nodiscard]] constexpr bool succeeded() const noexcept {
        return is_success();
    }

    [[nodiscard]] constexpr bool failed() const noexcept {
        return is_failure();
    }

    constexpr explicit operator bool() const noexcept {
        return is_success();
    }

    [[nodiscard]] constexpr u32 raw() const noexcept {
        return code_.raw();
    }

    [[nodiscard]] constexpr u32 get_raw() const noexcept {
        return code_.raw();
    }

    [[nodiscard]] constexpr ResultCode code() const noexcept {
        return code_;
    }

    [[nodiscard]] constexpr u32 native_result() const noexcept {
        return code_.raw();
    }

    [[nodiscard]] constexpr u32 module() const noexcept {
        return code_.module();
    }

    [[nodiscard]] constexpr u32 description() const noexcept {
        return code_.description();
    }

    [[nodiscard]] std::string_view error_name() const noexcept {
        return code_.error_name();
    }

    [[nodiscard]] std::string_view get_error_name() const noexcept {
        return code_.error_name();
    }

    constexpr bool operator==(const Result<void>& other) const noexcept = default;

    static constexpr Result<void> Success() noexcept {
        return Result<void>(ResultCode::Success());
    }

private:
    ResultCode code_;
};

/**
 * @brief Helper to convert a raw Horizon OS / libnx Result code to nxdev::Result<void>.
 */
inline constexpr Result<void> from_libnx(u32 raw_result) noexcept {
    return Result<void>(ResultCode(raw_result));
}

/**
 * @brief Helper to convert a raw Horizon OS / libnx Result code to nxdev::Result<T>.
 */
template <typename T>
inline Result<T> from_libnx(u32 raw_result, T value) {
    if (raw_result == 0) {
        return Result<T>(std::move(value));
    }
    return Result<T>(ResultCode(raw_result));
}

} // namespace nxdev

/**
 * @brief Macro for early error propagation without exceptions.
 */
#define NXDEV_TRY(expr) \
    do { \
        auto _nxdev_res = (expr); \
        if (_nxdev_res.failed()) { \
            return _nxdev_res.code(); \
        } \
    } while (0)
