#pragma once

#include <nxdev/ui/view.hpp>
#include <string>
#include <functional>
#include <memory>

namespace nxdev::ui {

/**
 * @brief Interactive button component supporting controller focus and activation.
 */
class Button : public View {
public:
    Button();
    explicit Button(std::string text);
    ~Button() override;

    static std::shared_ptr<Button> create(std::string text = "");

    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const;

    void on_pressed(std::function<void()> callback);

    void set_enabled(bool enabled);
    [[nodiscard]] bool is_enabled() const;

    void set_custom_view(std::shared_ptr<View> view);
    [[nodiscard]] std::shared_ptr<View> custom_view() const;

private:
    std::string text_;
    bool enabled_{true};
    std::function<void()> pressed_callback_;
    std::shared_ptr<View> custom_view_;
};

} // namespace nxdev::ui
