#pragma once

#include <nxdev/ui/types.hpp>
#include <nxdev/result.hpp>
#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace nxdev::ui {

class Container;

/**
 * @brief Base class for all NXDev UI components.
 *
 * Encapsulates layout, appearance, lifecycle, focus, and input handling
 * without exposing internal backend structures.
 */
class View : public std::enable_shared_from_this<View> {
public:
    View();
    virtual ~View();

    View(const View&) = delete;
    View& operator=(const View&) = delete;
    View(View&&) noexcept;
    View& operator=(View&&) noexcept;

    // Factory
    static std::shared_ptr<View> create();

    // Layout Dimensions
    void set_width(float width);
    void set_height(float height);
    void set_size(float width, float height);
    void set_size(Size size);
    [[nodiscard]] float width() const;
    [[nodiscard]] float height() const;
    [[nodiscard]] Size size() const;

    void set_min_width(float width);
    void set_min_height(float height);
    void set_max_width(float width);
    void set_max_height(float height);

    // Flexbox Factors
    void set_grow(float factor);
    void set_shrink(float factor);

    // Margins and Padding
    void set_margin(Insets insets);
    void set_margin(float all);
    void set_padding(Insets insets);
    void set_padding(float all);

    // Appearance
    void set_background_color(Color color);
    void set_corner_radius(float radius);
    void set_border_width(float width);
    void set_border_color(Color color);
    void set_alpha(float alpha);
    [[nodiscard]] float alpha() const;

    // Visibility
    void set_visible(bool visible);
    [[nodiscard]] bool is_visible() const;

    // Focus & Navigation
    void set_focusable(bool focusable);
    [[nodiscard]] bool is_focusable() const;
    void focus();
    [[nodiscard]] bool has_focus() const;

    // Input & Actions
    using ClickCallback = std::function<void()>;
    using ActionCallback = std::function<bool()>;

    void on_click(ClickCallback callback);
    void register_action(Action action, ActionCallback callback, std::string hint = "");

    // Hierarchy
    [[nodiscard]] std::shared_ptr<Container> parent() const;

    // Backend Native Handle (unstable escape hatch)
    [[nodiscard]] void* native_handle() const;

protected:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    friend class Container;
    friend class Application;
    friend class Dialog;
    friend class ScrollView;
    friend class List;
};

} // namespace nxdev::ui
