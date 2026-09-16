#pragma once

/**
 * @file native.hpp
 * @brief Native escape hatch to underlying Borealis backend objects.
 *
 * @warning This API is backend-specific and not covered by NXDev UI API stability guarantees.
 * Direct use of brls:: types binds application code directly to Borealis and should only be
 * used for custom rendering or advanced low-level integrations.
 */

#include <nxdev/ui/view.hpp>
#include <nxdev/ui/application.hpp>
#include <nxdev/ui/dialog.hpp>

// Forward declaration of Borealis types without including full header
namespace brls {
    class View;
    class Application;
    class Dialog;
}

namespace nxdev::ui::borealis {

/**
 * @brief Retrieves raw pointer to underlying brls::View from an nxdev::ui::View.
 */
inline brls::View* get_native_view(const View* view) {
    if (!view) return nullptr;
    return reinterpret_cast<brls::View*>(view->native_handle());
}

/**
 * @brief Retrieves raw pointer to underlying brls::Dialog from an nxdev::ui::Dialog.
 */
inline brls::Dialog* get_native_dialog(const Dialog* dialog) {
    if (!dialog) return nullptr;
    return reinterpret_cast<brls::Dialog*>(dialog->native_handle());
}

} // namespace nxdev::ui::borealis
