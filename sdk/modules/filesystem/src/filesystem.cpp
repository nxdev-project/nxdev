#include <nxdev/filesystem.hpp>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <cstring>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace nxdev::filesystem {

namespace fs = std::filesystem;

static ResultCode errno_to_result(int err) noexcept {
    switch (err) {
        case ENOENT: return results::FileNotFound;
        case EACCES:
        case EPERM:  return results::AccessDenied;
        case ENAMETOOLONG:
        case EINVAL: return results::InvalidPath;
        case EFBIG:  return results::FileTooLarge;
        default:     return results::IoError;
    }
}

RomFS::~RomFS() {
    unmount();
}

RomFS::RomFS(RomFS&& other) noexcept
    : mount_name_(std::move(other.mount_name_)), mounted_(other.mounted_) {
    other.mounted_ = false;
}

RomFS& RomFS::operator=(RomFS&& other) noexcept {
    if (this != &other) {
        unmount();
        mount_name_ = std::move(other.mount_name_);
        mounted_ = other.mounted_;
        other.mounted_ = false;
    }
    return *this;
}

Result<RomFS> RomFS::mount(std::string_view name) {
    std::string mount_name(name.empty() ? "romfs" : name);
#ifdef __SWITCH__
    ::Result rc = romfsMountSelf(mount_name.c_str());
    if (R_FAILED(rc)) {
        return Result<RomFS>(ResultCode(static_cast<u32>(rc)));
    }
#endif
    return Result<RomFS>(RomFS(std::move(mount_name)));
}

Result<RomFS> RomFS::mount_file(std::string_view path, u64 offset, std::string_view name) {
    std::string mount_name(name.empty() ? "romfs" : name);
    std::string file_path(path);
#ifdef __SWITCH__
    ::Result rc = romfsMountFromFsdev(file_path.c_str(), offset, mount_name.c_str());
    if (R_FAILED(rc)) {
        return Result<RomFS>(ResultCode(static_cast<u32>(rc)));
    }
#else
    (void)offset;
#endif
    return Result<RomFS>(RomFS(std::move(mount_name)));
}

void RomFS::unmount() noexcept {
    if (mounted_) {
#ifdef __SWITCH__
        romfsUnmount(mount_name_.c_str());
#endif
        mounted_ = false;
    }
}

static std::string normalize_relative_path(std::string_view rel) {
    while (!rel.empty() && (rel.front() == '/' || rel.front() == '\\')) {
        rel.remove_prefix(1);
    }
    return std::string(rel);
}

std::string romfs_path(std::string_view relative_path, std::string_view mount_name) {
    std::string prefix(mount_name.empty() ? "romfs" : mount_name);
    prefix += ":/";
    prefix += normalize_relative_path(relative_path);
    return prefix;
}

std::string sd_path(std::string_view relative_path) {
    std::string prefix = "sdmc:/";
    prefix += normalize_relative_path(relative_path);
    return prefix;
}

std::string app_data_path(std::string_view app_id, std::string_view relative_path) {
    std::string base = "sdmc:/switch/";
    base += normalize_relative_path(app_id);
    if (!relative_path.empty()) {
        base += "/";
        base += normalize_relative_path(relative_path);
    }
    return base;
}

Result<std::string> read_text(std::string_view path, size_t max_size) {
    std::string path_str(path);
    std::ifstream file(path_str, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return Result<std::string>(results::FileNotFound);
    }

    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    if (length < 0) {
        return Result<std::string>(results::IoError);
    }
    if (static_cast<size_t>(length) > max_size) {
        return Result<std::string>(results::FileTooLarge);
    }

    file.seekg(0, std::ios::beg);
    std::string content;
    content.resize(static_cast<size_t>(length));
    if (length > 0) {
        file.read(content.data(), length);
        if (!file && !file.eof()) {
            return Result<std::string>(results::IoError);
        }
    }
    return Result<std::string>(std::move(content));
}

Result<std::vector<u8>> read_bytes(std::string_view path, size_t max_size) {
    std::string path_str(path);
    std::ifstream file(path_str, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return Result<std::vector<u8>>(results::FileNotFound);
    }

    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    if (length < 0) {
        return Result<std::vector<u8>>(results::IoError);
    }
    if (static_cast<size_t>(length) > max_size) {
        return Result<std::vector<u8>>(results::FileTooLarge);
    }

    file.seekg(0, std::ios::beg);
    std::vector<u8> content(static_cast<size_t>(length));
    if (length > 0) {
        file.read(reinterpret_cast<char*>(content.data()), length);
        if (!file && !file.eof()) {
            return Result<std::vector<u8>>(results::IoError);
        }
    }
    return Result<std::vector<u8>>(std::move(content));
}

Result<void> write_text(std::string_view path, std::string_view content) {
    return write_bytes(path, content.data(), content.size());
}

Result<void> write_bytes(std::string_view path, const void* data, size_t size) {
    std::string path_str(path);

    std::error_code ec;
    fs::path p(path_str);
    if (p.has_parent_path()) {
        fs::create_directories(p.parent_path(), ec);
    }

    std::ofstream file(path_str, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return Result<void>(results::IoError);
    }

    if (size > 0 && data != nullptr) {
        file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
        if (!file) {
            return Result<void>(results::IoError);
        }
    }

    file.flush();
    return Result<void>::Success();
}

bool exists(std::string_view path) noexcept {
    std::error_code ec;
    return fs::exists(fs::path(path), ec);
}

bool is_file(std::string_view path) noexcept {
    std::error_code ec;
    return fs::is_regular_file(fs::path(path), ec);
}

bool is_directory(std::string_view path) noexcept {
    std::error_code ec;
    return fs::is_directory(fs::path(path), ec);
}

Result<u64> file_size(std::string_view path) noexcept {
    std::error_code ec;
    auto size = fs::file_size(fs::path(path), ec);
    if (ec) {
        return Result<u64>(results::FileNotFound);
    }
    return Result<u64>(static_cast<u64>(size));
}

Result<void> create_directories(std::string_view path) {
    std::error_code ec;
    fs::create_directories(fs::path(path), ec);
    if (ec) {
        return Result<void>(errno_to_result(ec.value()));
    }
    return Result<void>::Success();
}

Result<void> remove(std::string_view path) {
    std::error_code ec;
    fs::remove(fs::path(path), ec);
    if (ec) {
        return Result<void>(errno_to_result(ec.value()));
    }
    return Result<void>::Success();
}

} // namespace nxdev::filesystem
