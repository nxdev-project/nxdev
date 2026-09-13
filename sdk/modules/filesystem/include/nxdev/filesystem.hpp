#pragma once

#include <nxdev/types.hpp>
#include <nxdev/result.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <span>

namespace nxdev::filesystem {

namespace results {
    inline constexpr ResultCode FileNotFound{0x00020001};
    inline constexpr ResultCode InvalidPath{0x00020002};
    inline constexpr ResultCode AccessDenied{0x00020003};
    inline constexpr ResultCode FileTooLarge{0x00020004};
    inline constexpr ResultCode MountFailed{0x00020005};
    inline constexpr ResultCode IoError{0x00020006};
}

/**
 * @brief RAII Manager for RomFS mount and unmount lifecycles.
 */
class RomFS {
public:
    RomFS() noexcept : mounted_(false) {}
    ~RomFS();

    RomFS(const RomFS&) = delete;
    RomFS& operator=(const RomFS&) = delete;

    RomFS(RomFS&& other) noexcept;
    RomFS& operator=(RomFS&& other) noexcept;

    /**
     * @brief Mounts application embedded RomFS data.
     * @param name Device mount prefix name (defaults to "romfs").
     */
    [[nodiscard]] static Result<RomFS> mount(std::string_view name = "romfs");

    /**
     * @brief Mounts RomFS from an external file path.
     */
    [[nodiscard]] static Result<RomFS> mount_file(std::string_view path, u64 offset = 0, std::string_view name = "romfs");

    [[nodiscard]] bool is_mounted() const noexcept {
        return mounted_;
    }

    explicit operator bool() const noexcept {
        return mounted_;
    }

    [[nodiscard]] std::string_view mount_name() const noexcept {
        return mount_name_;
    }

    void unmount() noexcept;

private:
    explicit RomFS(std::string name) noexcept : mount_name_(std::move(name)), mounted_(true) {}

    std::string mount_name_{"romfs"};
    bool mounted_{false};
};

/**
 * @brief Builds a canonical romfs:/ path from a relative path.
 */
[[nodiscard]] std::string romfs_path(std::string_view relative_path, std::string_view mount_name = "romfs");

/**
 * @brief Builds a canonical sdmc:/ path from a relative path.
 */
[[nodiscard]] std::string sd_path(std::string_view relative_path);

/**
 * @brief Builds an application-specific data directory path (sdmc:/switch/<app_id>/...).
 */
[[nodiscard]] std::string app_data_path(std::string_view app_id, std::string_view relative_path = "");

/**
 * @brief Reads the entire content of a file as a UTF-8 text string.
 */
[[nodiscard]] Result<std::string> read_text(std::string_view path, size_t max_size = 64 * 1024 * 1024);

/**
 * @brief Reads the entire content of a file as a binary byte buffer.
 */
[[nodiscard]] Result<std::vector<u8>> read_bytes(std::string_view path, size_t max_size = 64 * 1024 * 1024);

/**
 * @brief Writes text content to a file (creating parent directories if needed).
 */
[[nodiscard]] Result<void> write_text(std::string_view path, std::string_view content);

/**
 * @brief Writes binary data to a file.
 */
[[nodiscard]] Result<void> write_bytes(std::string_view path, const void* data, size_t size);

/**
 * @brief Writes binary data to a file from a byte vector.
 */
[[nodiscard]] inline Result<void> write_bytes(std::string_view path, const std::vector<u8>& bytes) {
    return write_bytes(path, bytes.data(), bytes.size());
}

/**
 * @brief Checks if a path exists.
 */
[[nodiscard]] bool exists(std::string_view path) noexcept;

/**
 * @brief Checks if a path points to a regular file.
 */
[[nodiscard]] bool is_file(std::string_view path) noexcept;

/**
 * @brief Checks if a path points to a directory.
 */
[[nodiscard]] bool is_directory(std::string_view path) noexcept;

/**
 * @brief Queries the size of a file in bytes.
 */
[[nodiscard]] Result<u64> file_size(std::string_view path) noexcept;

/**
 * @brief Creates a directory and any missing parent directories.
 */
[[nodiscard]] Result<void> create_directories(std::string_view path);

/**
 * @brief Removes a file or empty directory.
 */
[[nodiscard]] Result<void> remove(std::string_view path);

} // namespace nxdev::filesystem
