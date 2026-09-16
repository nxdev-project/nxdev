#pragma once

#include <nxdev/ui/types.hpp>
#include <nxdev/ui/view.hpp>
#include <nxdev/ui/container.hpp>
#include <memory>
#include <string>
#include <vector>
#include <utility>

// Include Borealis headers only internally inside sdk/modules/borealis/src/
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
#include <borealis.hpp>
#endif

namespace nxdev::ui {

struct View::Impl {
    void* native_view = nullptr;
    std::weak_ptr<Container> parent;
    float width = 0.0f;
    float height = 0.0f;
    float min_width = 0.0f;
    float min_height = 0.0f;
    float max_width = 0.0f;
    float max_height = 0.0f;
    float grow = 0.0f;
    float shrink = 1.0f;
    Insets margin;
    Insets padding;
    Color bg_color = Color::Transparent();
    Color border_color = Color::Transparent();
    float border_width = 0.0f;
    float corner_radius = 0.0f;
    float alpha = 1.0f;
    bool visible = true;
    bool focusable = false;
    bool focused = false;
    ClickCallback click_cb;
    std::vector<std::pair<Action, ActionCallback>> actions;

    ~Impl() {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
        if (native_view && parent.expired()) {
            // Memory managed by container hierarchy or caller
        }
#endif
    }
};

namespace detail {

#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)

inline NVGcolor to_nvg_color(const Color& c) {
    return nvgRGBAf(c.r, c.g, c.b, c.a);
}

inline Color from_nvg_color(NVGcolor c) {
    return Color(c.r, c.g, c.b, c.a);
}

inline brls::Key to_brls_key(Action action) {
    switch (action) {
        case Action::Confirm: return brls::Key::A;
        case Action::Back: return brls::Key::B;
        case Action::Menu: return brls::Key::START;
        case Action::Context: return brls::Key::X;
        case Action::Left: return brls::Key::LEFT;
        case Action::Right: return brls::Key::RIGHT;
        case Action::Up: return brls::Key::UP;
        case Action::Down: return brls::Key::DOWN;
        case Action::Alt1: return brls::Key::L;
        case Action::Alt2: return brls::Key::R;
    }
    return brls::Key::A;
}

#endif

} // namespace detail

} // namespace nxdev::ui
