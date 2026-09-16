#include <nxdev/ui/button.hpp>
#include "internal.hpp"

namespace nxdev::ui {

Button::Button() : Button("") {}

Button::Button(std::string text) : text_(std::move(text)) {
    set_focusable(true);
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    auto* btn = new brls::Button();
    btn->setText(text_);
    btn->registerClickAction([this](brls::View*) {
        if (enabled_ && pressed_callback_) {
            pressed_callback_();
        }
        return true;
    });
    impl_->native_view = btn;
#endif
}

Button::~Button() = default;

std::shared_ptr<Button> Button::create(std::string text) {
    return std::make_shared<Button>(std::move(text));
}

void Button::set_text(std::string text) {
    text_ = std::move(text);
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* btn = static_cast<brls::Button*>(impl_->native_view)) {
        btn->setText(text_);
    }
#endif
}

const std::string& Button::text() const {
    return text_;
}

void Button::on_pressed(std::function<void()> callback) {
    pressed_callback_ = std::move(callback);
    on_click(pressed_callback_);
}

void Button::set_enabled(bool enabled) {
    enabled_ = enabled;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* btn = static_cast<brls::Button*>(impl_->native_view)) {
        btn->setFocusable(enabled);
        btn->setAlpha(enabled ? 1.0f : 0.5f);
    }
#endif
}

bool Button::is_enabled() const {
    return enabled_;
}

void Button::set_custom_view(std::shared_ptr<View> view) {
    custom_view_ = std::move(view);
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* btn = static_cast<brls::Button*>(impl_->native_view)) {
        if (custom_view_) {
            if (auto* cv = static_cast<brls::View*>(custom_view_->native_handle())) {
                btn->setCustomNavigationRoute(brls::FocusDirection::NONE, cv);
            }
        }
    }
#endif
}

std::shared_ptr<View> Button::custom_view() const {
    return custom_view_;
}

} // namespace nxdev::ui
