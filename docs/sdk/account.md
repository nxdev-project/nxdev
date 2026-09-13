# User & Account Management (`NXDev::Account`)

The `NXDev::Account` module wraps Horizon OS local user profile and account services (`acc:*`).

## CMake Target & Manifest

In your `nxapp.yaml`:
```yaml
build:
  dependencies:
    - nxdev.core
    - nxdev.account
```

In your `CMakeLists.txt`:
```cmake
find_package(NXDev REQUIRED)
target_link_libraries(myapp PRIVATE NXDev::Account)
```

## Header

```cpp
#include <nxdev/account.hpp>
```

---

## Querying Local User Profiles

```cpp
#include <nxdev/account.hpp>
#include <iostream>

void print_active_user() {
    // Query last opened user ID:
    auto user_res = nxdev::account::get_last_opened_user();
    if (user_res.is_success()) {
        nxdev::account::UserId uid = user_res.value();
        
        // Look up nickname:
        auto prof_res = nxdev::account::get_profile(uid);
        if (prof_res.is_success()) {
            std::cout << "Active User: " << prof_res->nickname << "\n";
            std::cout << "User UID: " << uid.to_string() << "\n";
        }
    }
}

void list_all_users() {
    auto users_res = nxdev::account::users();
    if (users_res.is_success()) {
        for (const auto& uid : users_res.value()) {
            auto prof = nxdev::account::get_profile(uid);
            if (prof.is_success()) {
                std::cout << "User: " << prof->nickname << " (" << uid.to_string() << ")\n";
            }
        }
    }
}
```

---

## RAII Account Service Lifetime

When instantiating `AccountService`, the underlying Horizon OS `acc` service is initialized and reference-counted:

```cpp
auto svc_res = nxdev::account::AccountService::create();
if (svc_res.is_success()) {
    auto& svc = svc_res.value();
    auto count = svc.user_count();
}
```
