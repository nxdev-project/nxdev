#pragma once

#include <nxdev/ui/view.hpp>
#include <string>
#include <vector>
#include <memory>
#include <span>

namespace nxdev::ui {

/**
 * @brief Image display component supporting RomFS paths, filesystem paths, and memory buffers.
 */
class Image : public View {
public:
    Image();
    ~Image() override;

    static std::shared_ptr<Image> from_file(const std::string& path);
    static std::shared_ptr<Image> from_resource(const std::string& resource_path);
    static std::shared_ptr<Image> from_memory(const void* data, size_t size);
    static std::shared_ptr<Image> from_memory(std::span<const uint8_t> bytes);

    bool load_from_file(const std::string& path);
    bool load_from_resource(const std::string& resource_path);
    bool load_from_memory(const void* data, size_t size);

    void set_scale_mode(ImageScaleMode mode);
    [[nodiscard]] ImageScaleMode scale_mode() const;

    void set_tint_color(Color color);
    [[nodiscard]] Color tint_color() const;

private:
    std::string source_path_;
    ImageScaleMode scale_mode_{ImageScaleMode::Fit};
    Color tint_color_{Color::White()};
};

} // namespace nxdev::ui
