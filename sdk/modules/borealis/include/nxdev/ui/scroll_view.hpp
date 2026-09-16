#pragma once

#include <nxdev/ui/view.hpp>
#include <memory>

namespace nxdev::ui {

/**
 * @brief Scrollable container component supporting smooth controller and touch scrolling.
 */
class ScrollView : public View {
public:
    ScrollView();
    explicit ScrollView(Direction direction);
    ~ScrollView() override;

    static std::shared_ptr<ScrollView> create(Direction direction = Direction::Column);

    void set_content(std::shared_ptr<View> content);
    [[nodiscard]] std::shared_ptr<View> content() const;

    void set_direction(Direction direction);
    [[nodiscard]] Direction direction() const;

    void scroll_to_top(bool animated = true);
    void scroll_to_bottom(bool animated = true);
    void scroll_to(std::shared_ptr<View> target, bool animated = true);

private:
    Direction direction_{Direction::Column};
    std::shared_ptr<View> content_;
};

} // namespace nxdev::ui
