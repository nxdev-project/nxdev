#pragma once

#include <nxdev/types.hpp>
#include <nxdev/result.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace nxdev::account {

namespace results {
    inline constexpr ResultCode NoUserSelected{0x00030001};
    inline constexpr ResultCode ProfileNotFound{0x00030002};
    inline constexpr ResultCode ServiceInitFailed{0x00030003};
}

/**
 * @brief 128-bit Horizon OS User Identifier.
 */
struct UserId {
    u64 raw[2]{0, 0};

    constexpr UserId() noexcept = default;
    constexpr UserId(u64 low, u64 high) noexcept : raw{low, high} {}

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return raw[0] != 0 || raw[1] != 0;
    }

    constexpr explicit operator bool() const noexcept {
        return is_valid();
    }

    constexpr bool operator==(const UserId& other) const noexcept {
        return raw[0] == other.raw[0] && raw[1] == other.raw[1];
    }

    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] static UserId from_string(std::string_view hex_str);

    [[nodiscard]] const u64* native() const noexcept {
        return raw;
    }
};

/**
 * @brief User Profile information.
 */
struct Profile {
    UserId user_id{};
    std::string nickname;

    [[nodiscard]] bool is_valid() const noexcept {
        return user_id.is_valid();
    }
};

/**
 * @brief RAII Manager for the Horizon OS Account Service.
 */
class AccountService {
public:
    AccountService();
    ~AccountService();

    AccountService(const AccountService&) = delete;
    AccountService& operator=(const AccountService&) = delete;

    AccountService(AccountService&& other) noexcept;
    AccountService& operator=(AccountService&& other) noexcept;

    /**
     * @brief Factory creating an initialized AccountService.
     */
    [[nodiscard]] static Result<AccountService> create();

    /**
     * @brief Returns a list of all local user IDs.
     */
    [[nodiscard]] Result<std::vector<UserId>> users() const;

    /**
     * @brief Returns the total number of local user profiles on the system.
     */
    [[nodiscard]] Result<s32> user_count() const;

    /**
     * @brief Returns the user ID of the last active/opened user.
     */
    [[nodiscard]] Result<UserId> get_last_opened_user() const;

    /**
     * @brief Returns the user ID selected by the profile selector applet if available.
     */
    [[nodiscard]] Result<UserId> get_preselected_user() const;

    /**
     * @brief Retrieves the profile metadata for the specified user ID.
     */
    [[nodiscard]] Result<Profile> get_profile(UserId uid) const;

    // Simulation methods for unit testing
    void simulate_users(std::vector<Profile> profiles);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Convenience free function to list all users.
 */
[[nodiscard]] Result<std::vector<UserId>> users();

/**
 * @brief Convenience free function to get a user profile.
 */
[[nodiscard]] Result<Profile> get_profile(UserId uid);

/**
 * @brief Convenience free function to get the last opened user ID.
 */
[[nodiscard]] Result<UserId> get_last_opened_user();

} // namespace nxdev::account
