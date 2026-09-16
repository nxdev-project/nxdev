#include <nxdev/ui/container.hpp>
#include "internal.hpp"
#include <algorithm>

namespace nxdev::ui {

Container::Container() : Container(Direction::Column) {}

Container::Container(Direction direction) {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    auto* box = new brls::Box(direction == Direction::Row ? brls::Axis::ROW : brls::Axis::COLUMN);
    impl_->native_view = box;
#endif
    set_direction(direction);
}

Container::~Container() {
    clear();
}

std::shared_ptr<Container> Container::create(Direction direction) {
    return std::make_shared<Container>(direction);
}

std::shared_ptr<Container> Container::row() {
    return std::make_shared<Container>(Direction::Row);
}

std::shared_ptr<Container> Container::column() {
    return std::make_shared<Container>(Direction::Column);
}

void Container::set_direction(Direction direction) {
    (void)direction;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* box = static_cast<brls::Box*>(impl_->native_view)) {
        box->setAxis(direction == Direction::Row ? brls::Axis::ROW : brls::Axis::COLUMN);
    }
#endif
}

Direction Container::direction() const {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* box = static_cast<brls::Box*>(impl_->native_view)) {
        return (box->getAxis() == brls::Axis::ROW) ? Direction::Row : Direction::Column;
    }
#endif
    return Direction::Column;
}

void Container::set_align_items(Align align) {
    (void)align;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* box = static_cast<brls::Box*>(impl_->native_view)) {
        brls::AlignItems item_align = brls::AlignItems::FLEX_START;
        switch (align) {
            case Align::Start: item_align = brls::AlignItems::FLEX_START; break;
            case Align::Center: item_align = brls::AlignItems::CENTER; break;
            case Align::End: item_align = brls::AlignItems::FLEX_END; break;
            case Align::Stretch: item_align = brls::AlignItems::STRETCH; break;
            default: break;
        }
        box->setAlignItems(item_align);
    }
#endif
}

void Container::set_justify_content(Align align) {
    (void)align;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* box = static_cast<brls::Box*>(impl_->native_view)) {
        brls::JustifyContent just = brls::JustifyContent::FLEX_START;
        switch (align) {
            case Align::Start: just = brls::JustifyContent::FLEX_START; break;
            case Align::Center: just = brls::JustifyContent::CENTER; break;
            case Align::End: just = brls::JustifyContent::FLEX_END; break;
            case Align::SpaceBetween: just = brls::JustifyContent::SPACE_BETWEEN; break;
            case Align::SpaceAround: just = brls::JustifyContent::SPACE_AROUND; break;
            case Align::SpaceEvenly: just = brls::JustifyContent::SPACE_EVENLY; break;
            default: break;
        }
        box->setJustifyContent(just);
    }
#endif
}

void Container::set_spacing(float spacing) {
    (void)spacing;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    // In yoga/borealis, spacing can be configured on children or via box margin/padding
#endif
}

float Container::spacing() const {
    return 0.0f;
}

void Container::add(std::shared_ptr<View> child) {
    if (!child) return;
    child->impl_->parent = std::static_pointer_cast<Container>(shared_from_this());
    children_.push_back(child);
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* box = static_cast<brls::Box*>(impl_->native_view)) {
        if (auto* cv = static_cast<brls::View*>(child->native_handle())) {
            box->addView(cv);
        }
    }
#endif
}

void Container::add(std::shared_ptr<View> child, size_t index) {
    if (!child) return;
    child->impl_->parent = std::static_pointer_cast<Container>(shared_from_this());
    if (index >= children_.size()) {
        children_.push_back(child);
    } else {
        children_.insert(children_.begin() + index, child);
    }
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* box = static_cast<brls::Box*>(impl_->native_view)) {
        if (auto* cv = static_cast<brls::View*>(child->native_handle())) {
            box->addView(cv, index);
        }
    }
#endif
}

bool Container::remove(std::shared_ptr<View> child) {
    if (!child) return false;
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        child->impl_->parent.reset();
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
        if (auto* box = static_cast<brls::Box*>(impl_->native_view)) {
            if (auto* cv = static_cast<brls::View*>(child->native_handle())) {
                box->removeView(cv);
            }
        }
#endif
        children_.erase(it);
        return true;
    }
    return false;
}

void Container::remove_at(size_t index) {
    if (index < children_.size()) {
        remove(children_[index]);
    }
}

void Container::clear() {
    for (auto& c : children_) {
        c->impl_->parent.reset();
    }
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* box = static_cast<brls::Box*>(impl_->native_view)) {
        box->clearViews();
    }
#endif
    children_.clear();
}

size_t Container::child_count() const {
    return children_.size();
}

const std::vector<std::shared_ptr<View>>& Container::children() const {
    return children_;
}

std::shared_ptr<View> Container::child_at(size_t index) const {
    if (index < children_.size()) {
        return children_[index];
    }
    return nullptr;
}

} // namespace nxdev::ui
