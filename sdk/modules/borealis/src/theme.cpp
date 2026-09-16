#include <nxdev/ui/theme.hpp>
#include "internal.hpp"

namespace nxdev::ui {

static ThemeVariant s_theme_variant = ThemeVariant::Auto;

void Theme::set_variant(ThemeVariant variant) {
    s_theme_variant = variant;
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    brls::ThemeVariant bv = brls::ThemeVariant::AUTO;
    switch (variant) {
        case ThemeVariant::Auto: bv = brls::ThemeVariant::AUTO; break;
        case ThemeVariant::Light: bv = brls::ThemeVariant::LIGHT; break;
        case ThemeVariant::Dark: bv = brls::ThemeVariant::DARK; break;
    }
    brls::Application::getTheme().setThemeVariant(bv);
#endif
}

ThemeVariant Theme::variant() {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    auto bv = brls::Application::getTheme().getThemeVariant();
    switch (bv) {
        case brls::ThemeVariant::LIGHT: return ThemeVariant::Light;
        case brls::ThemeVariant::DARK: return ThemeVariant::Dark;
        default: return ThemeVariant::Auto;
    }
#endif
    return s_theme_variant;
}

Color Theme::background() {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    return detail::from_nvg_color(brls::Application::getTheme().getColor("brls/background"));
#endif
    return Color::from_rgb_hex(0x2d2d2d);
}

Color Theme::foreground() {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    return detail::from_nvg_color(brls::Application::getTheme().getColor("brls/text"));
#endif
    return Color::from_rgb_hex(0xffffff);
}

Color Theme::accent() {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    return detail::from_nvg_color(brls::Application::getTheme().getColor("brls/accent"));
#endif
    return Color::from_rgb_hex(0x0080ff);
}

Color Theme::muted() {
    return Color::from_rgba(180, 180, 180, 200);
}

Color Theme::highlight() {
#if defined(NXDEV_PLATFORM_SWITCH) || defined(ENABLE_BOREALIS_BACKEND)
    return detail::from_nvg_color(brls::Application::getTheme().getColor("brls/highlight/color"));
#endif
    return Color::from_rgb_hex(0x00d4aa);
}

} // namespace nxdev::ui
