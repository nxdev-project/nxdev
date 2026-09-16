#include <nxdev/ui/scroll_view.hpp>
#include "internal.hpp"

namespace nxdev::ui {

ScrollView::ScrollView() : ScrollView(Direction::Column) {}

ScrollView::ScrollView(Direction direction) : direction_(direction) {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    auto* sf = new brls::ScrollingFrame();
    impl_->native_view = sf;
#endif
}

ScrollView::~ScrollView() = default;

std::shared_ptr<ScrollView> ScrollView::create(Direction direction) {
    return std::make_shared<ScrollView>(direction);
}

void ScrollView::set_content(std::shared_ptr<View> content) {
    content_ = std::move(content);
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* sf = static_cast<brls::ScrollingFrame*>(impl_->native_view)) {
        if (content_) {
            if (auto* cv = static_cast<brls::View*>(content_->native_handle())) {
                sf->setContentView(cv);
            }
        }
    }
#endif
}

std::shared_ptr<View> ScrollView::content() const {
    return content_;
}

void ScrollView::set_direction(Direction direction) {
    direction_ = direction;
}

Direction ScrollView::direction() const {
    return direction_;
}

void ScrollView::scroll_to_top(bool animated) {
    (void)animated;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* sf = static_cast<brls::ScrollingFrame*>(impl_->native_view)) {
        sf->scrollTo(brls::FocusDirection::UP, animated);
    }
#endif
}

void ScrollView::scroll_to_bottom(bool animated) {
    (void)animated;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* sf = static_cast<brls::ScrollingFrame*>(impl_->native_view)) {
        sf->scrollTo(brls::FocusDirection::DOWN, animated);
    }
#endif
}

void ScrollView::scroll_to(std::shared_ptr<View> target, bool animated) {
    (void)target;
    (void)animated;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* sf = static_cast<brls::ScrollingFrame*>(impl_->native_view)) {
        if (target) {
            if (auto* tv = static_cast<brls::View*>(target->native_handle())) {
                // Borealis scrolls automatically to focused views or target bounds
                (void)tv;
            }
        }
    }
#endif
}

} // namespace nxdev::ui
