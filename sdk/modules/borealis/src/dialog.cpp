#include <nxdev/ui/dialog.hpp>
#include "internal.hpp"

namespace nxdev::ui {

struct Dialog::Impl {
    void* native_dialog = nullptr;

    ~Impl() {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
        // Dialog lifecycle managed by brls stack when opened
#endif
    }
};

Dialog::Dialog() : Dialog("", "") {}

Dialog::Dialog(std::string title, std::string message)
    : impl_(std::make_unique<Impl>()), title_(std::move(title)), message_(std::move(message)) {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    auto* dlg = new brls::Dialog(title_ + "\n" + message_);
    impl_->native_dialog = dlg;
#endif
}

Dialog::~Dialog() = default;

std::shared_ptr<Dialog> Dialog::show(const Config& config) {
    auto dlg = std::make_shared<Dialog>(config.title, config.message);
    for (const auto& act : config.actions) {
        dlg->add_action(act.label, act.callback, act.is_cancel);
    }
    dlg->open();
    return dlg;
}

void Dialog::set_title(std::string title) {
    title_ = std::move(title);
}

const std::string& Dialog::title() const {
    return title_;
}

void Dialog::set_message(std::string message) {
    message_ = std::move(message);
}

const std::string& Dialog::message() const {
    return message_;
}

void Dialog::add_action(std::string label, std::function<void()> callback, bool is_cancel) {
    actions_.push_back(ActionOption{
        .label = label,
        .callback = callback,
        .is_cancel = is_cancel
    });

#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* dlg = static_cast<brls::Dialog*>(impl_->native_dialog)) {
        if (is_cancel) {
            dlg->setCancelable(true);
        }
        dlg->addButton(label, [callback]() {
            if (callback) callback();
        });
    }
#endif
}

void Dialog::add_cancel_action(std::string label, std::function<void()> callback) {
    add_action(std::move(label), std::move(callback), true);
}

void Dialog::set_custom_view(std::shared_ptr<View> view) {
    custom_view_ = std::move(view);
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* dlg = static_cast<brls::Dialog*>(impl_->native_dialog)) {
        if (custom_view_) {
            if (auto* cv = static_cast<brls::View*>(custom_view_->native_handle())) {
                dlg->setCustomView(cv);
            }
        }
    }
#endif
}

void Dialog::open() {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* dlg = static_cast<brls::Dialog*>(impl_->native_dialog)) {
        dlg->open();
    }
#endif
}

void Dialog::dismiss() {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    if (auto* dlg = static_cast<brls::Dialog*>(impl_->native_dialog)) {
        dlg->close();
    }
#endif
}

void* Dialog::native_handle() const {
    return impl_->native_dialog;
}

} // namespace nxdev::ui
