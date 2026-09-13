#include <nxdev/system.hpp>
#include <string>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace nxdev::system {

AppletType get_applet_type() noexcept {
#ifdef __SWITCH__
    ::AppletType t = appletGetAppletType();
    switch (t) {
        case AppletType_None: return AppletType::None;
        case AppletType_Default: return AppletType::Default;
        case AppletType_Application: return AppletType::Application;
        case AppletType_SystemApplet: return AppletType::SystemApplet;
        case AppletType_LibraryApplet: return AppletType::LibraryApplet;
        case AppletType_OverlayApplet: return AppletType::OverlayApplet;
        default: return AppletType::Unknown;
    }
#else
    return AppletType::Application;
#endif
}

FirmwareVersion get_firmware_version() noexcept {
    FirmwareVersion ver;
#ifdef __SWITCH__
    u32 hos = hosversionGet();
    ver.major = static_cast<u8>(HOSVER_MAJOR(hos));
    ver.minor = static_cast<u8>(HOSVER_MINOR(hos));
    ver.micro = static_cast<u8>(HOSVER_MICRO(hos));
    ver.display_version = std::to_string(ver.major) + "." + std::to_string(ver.minor) + "." + std::to_string(ver.micro);
#else
    ver.major = 17;
    ver.minor = 0;
    ver.micro = 0;
    ver.display_version = "17.0.0";
#endif
    return ver;
}

u64 get_total_memory() noexcept {
#ifdef __SWITCH__
    u64 mem = 0;
    if (R_SUCCEEDED(svcGetInfo(&mem, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0))) {
        return mem;
    }
    return 0;
#else
    return 4ULL * 1024 * 1024 * 1024; // 4 GB default mock
#endif
}

u64 get_used_memory() noexcept {
#ifdef __SWITCH__
    u64 mem = 0;
    if (R_SUCCEEDED(svcGetInfo(&mem, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0))) {
        return mem;
    }
    return 0;
#else
    return 512ULL * 1024 * 1024; // 512 MB default mock
#endif
}

} // namespace nxdev::system
