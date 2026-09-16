#include <nxdev/ui/label.hpp>
#include "internal.hpp"

namespace nxdev::ui {

Label::Label() : Label("") {}

Label::Label(std::string text) : text_(std::move(text)) {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    auto* lbl = new brls::Label();
    lbl->setText(text_);
    lbl->setFontSize(font_size_);
    impl_->native_view = lbl;
#endif
}

Label::~Label() = default;

std::shared_ptr<Label> Label::create(std::string text) {
    return std::make_shared<Label>(std::move(text));
}

void Label::set_text(std::string text) {
    text_ = std::move(text);
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* lbl = static_cast<brls::Label*>(impl_->native_view)) {
        lbl->setText(text_);
    }
#endif
}

const std::string& Label::text() const {
    return text_;
}

void Label::set_font_size(float size) {
    font_size_ = size;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* lbl = static_cast<brls::Label*>(impl_->native_view)) {
        lbl->setFontSize(size);
    }
#endif
}

float Label::font_size() const {
    return font_size_;
}

void Label::set_text_color(Color color) {
    text_color_ = color;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* lbl = static_cast<brls::Label*>(impl_->native_view)) {
        lbl->setTextColor(detail::to_nvg_color(color));
    }
#endif
}

Color Label::text_color() const {
    return text_color_;
}

void Label::set_alignment(TextAlignment alignment) {
    alignment_ = alignment;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* lbl = static_cast<brls::Label*>(impl_->native_view)) {
        brls::HorizontalAlign ha = brls::HorizontalAlign::LEFT;
        switch (alignment) {
            case TextAlignment::Left: ha = brls::HorizontalAlign::LEFT; break;
            case TextAlignment::Center: ha = brls::HorizontalAlign::CENTER; break;
            case TextAlignment::Right: ha = brls::HorizontalAlign::RIGHT; break;
        }
        lbl->setHorizontalAlign(ha);
    }
#endif
}

TextAlignment Label::alignment() const {
    return alignment_;
}

void Label::set_word_wrap(bool wrap) {
    word_wrap_ = wrap;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* lbl = static_cast<brls::Label*>(impl_->native_view)) {
        lbl->setSingleLine(!wrap);
    }
#endif
}

bool Label::word_wrap() const {
    return word_wrap_;
}

} // namespace nxdev::ui
