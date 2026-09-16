#pragma once

#include <nxdev/ui/view.hpp>
#include <string>
#include <memory>

namespace nxdev::ui {

/**
 * @brief Text display component.
 */
class Label : public View {
public:
    Label();
    explicit Label(std::string text);
    ~Label() override;

    static std::shared_ptr<Label> create(std::string text = "");

    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const;

    void set_font_size(float size);
    [[nodiscard]] float font_size() const;

    void set_text_color(Color color);
    [[nodiscard]] Color text_color() const;

    void set_alignment(TextAlignment alignment);
    [[nodiscard]] TextAlignment alignment() const;

    void set_word_wrap(bool wrap);
    [[nodiscard]] bool word_wrap() const;

private:
    std::string text_;
    float font_size_{24.0f};
    Color text_color_{Color::White()};
    TextAlignment alignment_{TextAlignment::Left};
    bool word_wrap_{false};
};

} // namespace nxdev::ui
