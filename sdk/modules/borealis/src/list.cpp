#include <nxdev/ui/list.hpp>
#include <nxdev/ui/label.hpp>
#include <nxdev/ui/button.hpp>
#include "internal.hpp"

namespace nxdev::ui {

List::List() {
    container_ = Container::column();
    container_->set_grow(1.0f);
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    impl_->native_view = container_->native_handle();
#endif
}

List::~List() = default;

std::shared_ptr<List> List::create() {
    return std::make_shared<List>();
}

void List::add_item(std::shared_ptr<View> item) {
    if (!item) return;
    items_.push_back(item);
    container_->add(item);
}

void List::add_item(std::string title, std::function<void()> on_selected, std::string subtitle) {
    (void)subtitle;
    auto btn = Button::create(title);
    btn->set_margin(Insets(4.0f, 8.0f));
    btn->set_padding(Insets(8.0f, 16.0f));
    if (on_selected) {
        btn->on_pressed(std::move(on_selected));
    }
    add_item(btn);
}

void List::clear() {
    items_.clear();
    container_->clear();
}

void List::set_data_source(std::shared_ptr<ListDataSource> data_source) {
    data_source_ = std::move(data_source);
    reload_data();
}

void List::reload_data() {
    clear();
    if (!data_source_) return;
    size_t count = data_source_->count();
    for (size_t i = 0; i < count; ++i) {
        auto view = data_source_->create_view(i);
        if (view) {
            data_source_->bind_view(view, i);
            add_item(view);
        }
    }
}

size_t List::item_count() const {
    return items_.size();
}

const std::vector<std::shared_ptr<View>>& List::items() const {
    return items_;
}

} // namespace nxdev::ui
