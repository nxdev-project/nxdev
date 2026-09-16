#pragma once

#include <nxdev/ui/types.hpp>

namespace nxdev::ui {

/**
 * @brief Global UI theme manager and standard color palette access.
 */
class Theme {
public:
    static void set_variant(ThemeVariant variant);
    [[nodiscard]] static ThemeVariant variant();

    [[nodiscard]] static Color background();
    [[nodiscard]] static Color foreground();
    [[nodiscard]] static Color accent();
    [[nodiscard]] static Color muted();
    [[nodiscard]] static Color highlight();
};

} // namespace nxdev::ui
