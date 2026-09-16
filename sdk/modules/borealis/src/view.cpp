#include <nxdev/ui/view.hpp>
#include <nxdev/ui/container.hpp>
#include "internal.hpp"
#include <algorithm>

namespace nxdev::ui {

View::View() : impl_(std::make_unique<Impl>()) {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    // Create base brls::Box / brls::View if needed
    auto* bv = new brls::Box();
    impl_->native_view = bv;
#endif
}

View::~View() = default;

View::View(View&&) noexcept = default;
View& View::operator=(View&&) noexcept = default;

std::shared_ptr<View> View::create() {
    return std::make_shared<View>();
}

void View::set_width(float width) {
    impl_->width = width;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setWidth(width);
    }
#endif
}

void View::set_height(float height) {
    impl_->height = height;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setHeight(height);
    }
#endif
}

void View::set_size(float width, float height) {
    set_width(width);
    set_height(height);
}

void View::set_size(Size size) {
    set_size(size.width, size.height);
}

float View::width() const {
    return impl_->width;
}

float View::height() const {
    return impl_->height;
}

Size View::size() const {
    return Size(impl_->width, impl_->height);
}

void View::set_min_width(float width) {
    impl_->min_width = width;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setMinWidth(width);
    }
#endif
}

void View::set_min_height(float height) {
    impl_->min_height = height;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setMinHeight(height);
    }
#endif
}

void View::set_max_width(float width) {
    impl_->max_width = width;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setMaxWidth(width);
    }
#endif
}

void View::set_max_height(float height) {
    impl_->max_height = height;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setMaxHeight(height);
    }
#endif
}

void View::set_grow(float factor) {
    impl_->grow = factor;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setGrow(factor);
    }
#endif
}

void View::set_shrink(float factor) {
    impl_->shrink = factor;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setShrink(factor);
    }
#endif
}

void View::set_margin(Insets insets) {
    impl_->margin = insets;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setMarginTop(insets.top);
        v->setMarginRight(insets.right);
        v->setMarginBottom(insets.bottom);
        v->setMarginLeft(insets.left);
    }
#endif
}

void View::set_margin(float all) {
    set_margin(Insets(all));
}

void View::set_padding(Insets insets) {
    impl_->padding = insets;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setPadding(insets.top, insets.right, insets.bottom, insets.left);
    }
#endif
}

void View::set_padding(float all) {
    set_padding(Insets(all));
}

void View::set_background_color(Color color) {
    impl_->bg_color = color;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setBackgroundColor(detail::to_nvg_color(color));
    }
#endif
}

void View::set_corner_radius(float radius) {
    impl_->corner_radius = radius;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setCornerRadius(radius);
    }
#endif
}

void View::set_border_width(float width) {
    impl_->border_width = width;
}

void View::set_border_color(Color color) {
    impl_->border_color = color;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setBorderColor(detail::to_nvg_color(color));
    }
#endif
}

void View::set_alpha(float alpha) {
    impl_->alpha = alpha;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setAlpha(alpha);
    }
#endif
}

float View::alpha() const {
    return impl_->alpha;
}

void View::set_visible(bool visible) {
    impl_->visible = visible;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setVisibility(visible ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }
#endif
}

bool View::is_visible() const {
    return impl_->visible;
}

void View::set_focusable(bool focusable) {
    impl_->focusable = focusable;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->setFocusable(focusable);
    }
#endif
}

bool View::is_focusable() const {
    return impl_->focusable;
}

void View::focus() {
    impl_->focused = true;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        brls::Application::giveFocus(v);
    }
#endif
}

bool View::has_focus() const {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        return v->isFocused();
    }
#endif
    return impl_->focused;
}

void View::on_click(ClickCallback callback) {
    impl_->click_cb = std::move(callback);
    set_focusable(true);
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->registerClickAction([this](brls::View*) {
            if (impl_->click_cb) {
                impl_->click_cb();
            }
            return true;
        });
    }
#endif
}

void View::register_action(Action action, ActionCallback callback, std::string hint) {
    (void)hint;
    impl_->actions.emplace_back(action, callback);
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* v = static_cast<brls::View*>(impl_->native_view)) {
        v->registerAction(hint, detail::to_brls_key(action), [callback](brls::View*) {
            return callback ? callback() : false;
        });
    }
#endif
}

std::shared_ptr<Container> View::parent() const {
    return impl_->parent.lock();
}

void* View::native_handle() const {
    return impl_->native_view;
}

} // namespace nxdev::ui
