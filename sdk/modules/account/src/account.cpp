#include <nxdev/account.hpp>
#include <iomanip>
#include <sstream>
#include <atomic>
#include <mutex>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace nxdev::account {

std::string UserId::to_string() const {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(16) << raw[0]
       << std::setw(16) << raw[1];
    return ss.str();
}

UserId UserId::from_string(std::string_view hex_str) {
    if (hex_str.size() != 32) {
        return UserId{};
    }
    std::string s0(hex_str.substr(0, 16));
    std::string s1(hex_str.substr(16, 16));
    u64 low = std::strtoull(s0.c_str(), nullptr, 16);
    u64 high = std::strtoull(s1.c_str(), nullptr, 16);
    return UserId{low, high};
}

struct AccountServiceRefCounter {
    std::mutex mutex;
    int ref_count{0};

    bool retain() {
        std::lock_guard<std::mutex> lock(mutex);
#ifdef __SWITCH__
        if (ref_count == 0) {
            ::Result rc = accountInitialize(AccountServiceType_Application);
            if (R_FAILED(rc)) {
                return false;
            }
        }
#endif
        ref_count++;
        return true;
    }

    void release() {
        std::lock_guard<std::mutex> lock(mutex);
        if (ref_count > 0) {
            ref_count--;
#ifdef __SWITCH__
            if (ref_count == 0) {
                accountExit();
            }
#endif
        }
    }
};

static AccountServiceRefCounter g_acc_ref_counter;

struct AccountService::Impl {
    bool initialized{false};
    std::vector<Profile> simulated_profiles;
};

AccountService::AccountService() : impl_(std::make_unique<Impl>()) {
    if (g_acc_ref_counter.retain()) {
        impl_->initialized = true;
    }
#ifndef __SWITCH__
    // Default mock user profile on host
    impl_->simulated_profiles.push_back(Profile{
        .user_id = UserId{0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL},
        .nickname = "NXDev Developer"
    });
#endif
}

AccountService::~AccountService() {
    if (impl_ && impl_->initialized) {
        g_acc_ref_counter.release();
        impl_->initialized = false;
    }
}

AccountService::AccountService(AccountService&& other) noexcept = default;
AccountService& AccountService::operator=(AccountService&& other) noexcept = default;

Result<AccountService> AccountService::create() {
    AccountService svc;
    if (!svc.impl_->initialized) {
        return Result<AccountService>(results::ServiceInitFailed);
    }
    return Result<AccountService>(std::move(svc));
}

Result<std::vector<UserId>> AccountService::users() const {
    if (!impl_ || !impl_->initialized) {
        return Result<std::vector<UserId>>(results::ServiceInitFailed);
    }

#ifdef __SWITCH__
    AccountUid uids[ACC_USER_LIST_SIZE]{};
    s32 total = 0;
    ::Result rc = accountListAllUsers(uids, ACC_USER_LIST_SIZE, &total);
    if (R_FAILED(rc)) {
        return Result<std::vector<UserId>>(ResultCode(static_cast<u32>(rc)));
    }

    std::vector<UserId> result;
    result.reserve(total);
    for (s32 i = 0; i < total; ++i) {
        if (accountUidIsValid(&uids[i])) {
            result.push_back(UserId{uids[i].uid[0], uids[i].uid[1]});
        }
    }
    return Result<std::vector<UserId>>(std::move(result));
#else
    std::vector<UserId> result;
    for (const auto& p : impl_->simulated_profiles) {
        result.push_back(p.user_id);
    }
    return Result<std::vector<UserId>>(std::move(result));
#endif
}

Result<s32> AccountService::user_count() const {
    if (!impl_ || !impl_->initialized) {
        return Result<s32>(results::ServiceInitFailed);
    }

#ifdef __SWITCH__
    s32 count = 0;
    ::Result rc = accountGetUserCount(&count);
    if (R_FAILED(rc)) {
        return Result<s32>(ResultCode(static_cast<u32>(rc)));
    }
    return Result<s32>(count);
#else
    return Result<s32>(static_cast<s32>(impl_->simulated_profiles.size()));
#endif
}

Result<UserId> AccountService::get_last_opened_user() const {
    if (!impl_ || !impl_->initialized) {
        return Result<UserId>(results::ServiceInitFailed);
    }

#ifdef __SWITCH__
    AccountUid uid{};
    ::Result rc = accountGetLastOpenedUser(&uid);
    if (R_FAILED(rc) || !accountUidIsValid(&uid)) {
        return Result<UserId>(results::NoUserSelected);
    }
    return Result<UserId>(UserId{uid.uid[0], uid.uid[1]});
#else
    if (impl_->simulated_profiles.empty()) {
        return Result<UserId>(results::NoUserSelected);
    }
    return Result<UserId>(impl_->simulated_profiles.front().user_id);
#endif
}

Result<UserId> AccountService::get_preselected_user() const {
    if (!impl_ || !impl_->initialized) {
        return Result<UserId>(results::ServiceInitFailed);
    }

#ifdef __SWITCH__
    AccountUid uid{};
    ::Result rc = accountGetPreselectedUser(&uid);
    if (R_FAILED(rc) || !accountUidIsValid(&uid)) {
        return Result<UserId>(results::NoUserSelected);
    }
    return Result<UserId>(UserId{uid.uid[0], uid.uid[1]});
#else
    return get_last_opened_user();
#endif
}

Result<Profile> AccountService::get_profile(UserId uid) const {
    if (!impl_ || !impl_->initialized) {
        return Result<Profile>(results::ServiceInitFailed);
    }
    if (!uid.is_valid()) {
        return Result<Profile>(results::NoUserSelected);
    }

#ifdef __SWITCH__
    AccountUid native_uid;
    native_uid.uid[0] = uid.raw[0];
    native_uid.uid[1] = uid.raw[1];

    AccountProfile profile{};
    ::Result rc = accountGetProfile(&profile, native_uid);
    if (R_FAILED(rc)) {
        return Result<Profile>(ResultCode(static_cast<u32>(rc)));
    }

    AccountProfileBase base{};
    rc = accountProfileGet(&profile, nullptr, &base);
    accountProfileClose(&profile);

    if (R_FAILED(rc)) {
        return Result<Profile>(ResultCode(static_cast<u32>(rc)));
    }

    Profile result{
        .user_id = uid,
        .nickname = std::string(base.nickname)
    };
    return Result<Profile>(std::move(result));
#else
    for (const auto& p : impl_->simulated_profiles) {
        if (p.user_id == uid) {
            return Result<Profile>(p);
        }
    }
    return Result<Profile>(results::ProfileNotFound);
#endif
}

void AccountService::simulate_users(std::vector<Profile> profiles) {
    if (impl_) {
        impl_->simulated_profiles = std::move(profiles);
    }
}

Result<std::vector<UserId>> users() {
    AccountService svc;
    return svc.users();
}

Result<Profile> get_profile(UserId uid) {
    AccountService svc;
    return svc.get_profile(uid);
}

Result<UserId> get_last_opened_user() {
    AccountService svc;
    return svc.get_last_opened_user();
}

} // namespace nxdev::account
