#include <nxdev/result.hpp>

namespace nxdev {

std::string_view Result::get_error_name() const noexcept {
    if (is_success()) {
        return "Success";
    }
    if (raw_code_ == results::NotInitialized.get_raw()) {
        return "NotInitialized";
    }
    if (raw_code_ == results::AlreadyInitialized.get_raw()) {
        return "AlreadyInitialized";
    }
    if (raw_code_ == results::InvalidArgument.get_raw()) {
        return "InvalidArgument";
    }
    if (raw_code_ == results::OutOfMemory.get_raw()) {
        return "OutOfMemory";
    }
    return "UnknownError";
}

} // namespace nxdev
