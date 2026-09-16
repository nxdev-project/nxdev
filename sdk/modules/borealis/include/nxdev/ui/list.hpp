#pragma once

#include <nxdev/ui/view.hpp>
#include <nxdev/ui/container.hpp>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace nxdev::ui {

/**
 * @brief Abstract data source for dynamic or recycled list items.
 */
class ListDataSource {
public:
    virtual ~ListDataSource() = default;
    [[nodiscard]] virtual size_t count() const = 0;
    virtual std::shared_ptr<View> create_view(size_t index) = 0;
    virtual void bind_view(std::shared_ptr<View> view, size_t index) { (void)view; (void)index; }
};

/**
 * @brief Controller-friendly list view supporting static items and dynamic data sources.
 */
class List : public View {
public:
    List();
    ~List() override;

    static std::shared_ptr<List> create();

    // Static list item helpers
    void add_item(std::shared_ptr<View> item);
    void add_item(std::string title, std::function<void()> on_selected = nullptr, std::string subtitle = "");
    void clear();

    // Data source binding
    void set_data_source(std::shared_ptr<ListDataSource> data_source);
    void reload_data();

    [[nodiscard]] size_t item_count() const;
    [[nodiscard]] const std::vector<std::shared_ptr<View>>& items() const;

private:
    std::shared_ptr<ListDataSource> data_source_;
    std::vector<std::shared_ptr<View>> items_;
    std::shared_ptr<Container> container_;
};

} // namespace nxdev::ui
