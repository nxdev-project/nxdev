#pragma once

#include <nxdev/ui/view.hpp>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace nxdev::ui {

/**
 * @brief Modal dialog component with action buttons and dismiss behavior.
 */
class Dialog {
public:
    struct ActionOption {
        std::string label;
        std::function<void()> callback;
        bool is_cancel = false;
    };

    struct Config {
        std::string title;
        std::string message;
        std::vector<ActionOption> actions;
        bool cancelable = true;
    };

    Dialog();
    explicit Dialog(std::string title, std::string message = "");
    ~Dialog();

    // Static convenience method
    static std::shared_ptr<Dialog> show(const Config& config);

    void set_title(std::string title);
    [[nodiscard]] const std::string& title() const;

    void set_message(std::string message);
    [[nodiscard]] const std::string& message() const;

    void add_action(std::string label, std::function<void()> callback = nullptr, bool is_cancel = false);
    void add_cancel_action(std::string label = "Cancel", std::function<void()> callback = nullptr);

    void set_custom_view(std::shared_ptr<View> view);

    void open();
    void dismiss();

    [[nodiscard]] void* native_handle() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::string title_;
    std::string message_;
    std::vector<ActionOption> actions_;
    std::shared_ptr<View> custom_view_;
};

} // namespace nxdev::ui
