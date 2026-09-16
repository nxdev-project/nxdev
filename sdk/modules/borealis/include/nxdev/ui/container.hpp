#pragma once

#include <nxdev/ui/view.hpp>
#include <vector>
#include <memory>

namespace nxdev::ui {

/**
 * @brief Flexbox layout container that arranges child views in a row or column.
 */
class Container : public View {
public:
    Container();
    explicit Container(Direction direction);
    ~Container() override;

    // Static factories
    static std::shared_ptr<Container> create(Direction direction = Direction::Column);
    static std::shared_ptr<Container> row();
    static std::shared_ptr<Container> column();

    // Layout configuration
    void set_direction(Direction direction);
    [[nodiscard]] Direction direction() const;

    void set_align_items(Align align);
    void set_justify_content(Align align);
    void set_spacing(float spacing);
    [[nodiscard]] float spacing() const;

    // Child management
    void add(std::shared_ptr<View> child);
    void add(std::shared_ptr<View> child, size_t index);
    bool remove(std::shared_ptr<View> child);
    void remove_at(size_t index);
    void clear();

    [[nodiscard]] size_t child_count() const;
    [[nodiscard]] const std::vector<std::shared_ptr<View>>& children() const;
    [[nodiscard]] std::shared_ptr<View> child_at(size_t index) const;

private:
    std::vector<std::shared_ptr<View>> children_;
};

} // namespace nxdev::ui
