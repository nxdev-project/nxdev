#include <nxdev/result.hpp>

namespace nxdev {

std::string_view ResultCode::get_error_name() const noexcept {
    if (is_success()) {
        return "Success";
    }

    switch (raw_code_) {
        case 0x00010001: return "NotInitialized";
        case 0x00010002: return "AlreadyInitialized";
        case 0x00010003: return "InvalidArgument";
        case 0x00010004: return "OutOfMemory";
        case 0x00010005: return "NotFound";
        case 0x00010006: return "NotSupported";
        case 0x00010007: return "InvalidFormat";
        case 0x00010008: return "AlreadyExists";
        case 0x00010009: return "FileNotFound";
        default: break;
    }

    u32 mod = module();
    switch (mod) {
        case 1: return "KernelError";
        case 2: return "FileSystemError";
        case 3: return "OsError";
        case 5: return "NcmError";
        case 9: return "HipcError";
        case 11: return "SmError";
        case 15: return "PmError";
        case 16: return "SplError";
        case 116: return "NifmError";
        case 124: return "SslError";
        case 128: return "AppletError";
        case 140: return "SettingsError";
        case 143: return "PscError";
        case 800: return "NXDevError";
        default: return "HorizonError";
    }
}

} // namespace nxdev
