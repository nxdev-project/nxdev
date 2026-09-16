#include <nxdev/ui/image.hpp>
#include "internal.hpp"

namespace nxdev::ui {

Image::Image() {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    auto* img = new brls::Image();
    impl_->native_view = img;
#endif
}

Image::~Image() = default;

std::shared_ptr<Image> Image::from_file(const std::string& path) {
    auto img = std::make_shared<Image>();
    img->load_from_file(path);
    return img;
}

std::shared_ptr<Image> Image::from_resource(const std::string& resource_path) {
    auto img = std::make_shared<Image>();
    img->load_from_resource(resource_path);
    return img;
}

std::shared_ptr<Image> Image::from_memory(const void* data, size_t size) {
    auto img = std::make_shared<Image>();
    img->load_from_memory(data, size);
    return img;
}

std::shared_ptr<Image> Image::from_memory(std::span<const uint8_t> bytes) {
    return from_memory(bytes.data(), bytes.size());
}

bool Image::load_from_file(const std::string& path) {
    source_path_ = path;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* img = static_cast<brls::Image*>(impl_->native_view)) {
        img->setImageFromFile(path);
        return true;
    }
#endif
    return true;
}

bool Image::load_from_resource(const std::string& resource_path) {
    source_path_ = resource_path;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* img = static_cast<brls::Image*>(impl_->native_view)) {
        img->setImageFromRes(resource_path);
        return true;
    }
#endif
    return true;
}

bool Image::load_from_memory(const void* data, size_t size) {
    (void)data;
    (void)size;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* img = static_cast<brls::Image*>(impl_->native_view)) {
        img->setImageFromMem(static_cast<const unsigned char*>(data), size);
        return true;
    }
#endif
    return true;
}

void Image::set_scale_mode(ImageScaleMode mode) {
    scale_mode_ = mode;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* img = static_cast<brls::Image*>(impl_->native_view)) {
        brls::ImageScalingType st = brls::ImageScalingType::FIT;
        switch (mode) {
            case ImageScaleMode::Fit: st = brls::ImageScalingType::FIT; break;
            case ImageScaleMode::Fill: st = brls::ImageScalingType::FILL; break;
            case ImageScaleMode::Crop: st = brls::ImageScalingType::CROP; break;
            case ImageScaleMode::Center: st = brls::ImageScalingType::CENTER; break;
        }
        img->setScalingType(st);
    }
#endif
}

ImageScaleMode Image::scale_mode() const {
    return scale_mode_;
}

void Image::set_tint_color(Color color) {
    tint_color_ = color;
}

Color Image::tint_color() const {
    return tint_color_;
}

} // namespace nxdev::ui
