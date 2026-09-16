# NXDev UI Framework (`NXDev::Borealis`)

The **NXDev UI Framework** is an NXDev-owned, controller-first graphical user interface module providing modern, declarative-friendly abstractions (`nxdev::ui::*`) powered internally by [Borealis](https://github.com/jvrcruzGAMES/borealis).

---

## Architecture Overview

NXDev UI enforces a strict decoupling boundary between application code and underlying third-party implementation dependencies:

```text
┌────────────────────────────────────────────────────────┐
│                   NXDev Application                    │
│            (uses #include <nxdev/ui.hpp>)              │
└──────────────────────────┬─────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────┐
│                    nxdev::ui::*                        │
│   (Application, View, Container, Label, Button, etc.)  │
└──────────────────────────┬─────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────┐
│               NXDev Borealis Adapter                   │
│          (Internal backend implementation)             │
└──────────────────────────┬─────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────┐
│         jvrcruzGAMES/borealis & deko3d / NanoVG        │
└────────────────────────────────────────────────────────┘
```

### Core Design Principles

1. **Clean Abstraction**: Application code interacts exclusively with `nxdev::ui::*` types. No `brls::*` types, NanoVG contexts, or Yoga layout nodes are exposed across public headers.
2. **Controller-First Navigation**: Full support for gamepad navigation, focus traversal, confirm/back actions, and touch interactions.
3. **Automatic Framework Resources**: Shipped fonts (Material Icons), system assets, and translation files are bundled transparently into RomFS (`romfs:/nxdev/ui/` or `romfs:/`).
4. **Exception Containment**: Internal backend exceptions are captured and returned as idiomatic `nxdev::Result<T>` codes.
5. **Separation of API and Backend**: Future releases could swap or evolve the backend without requiring application rewrites.

---

## Quick Start

### 1. Declare Module Dependency

In `nxapp.yaml`:

```yaml
dependencies:
  - nxdev.core
  - nxdev.borealis
```

Or in `CMakeLists.txt`:

```cmake
target_link_libraries(my-app PRIVATE NXDev::Core NXDev::Borealis)
```

### 2. Application Entry Point

```cpp
#include <nxdev/ui.hpp>
#include <nxdev/log.hpp>

int main(int argc, char* argv[]) {
    // 1. Initialize UI application runtime
    auto app_res = nxdev::ui::Application::create({
        .name = "My Switch UI App"
    });

    if (app_res.failed()) {
        nxdev::log::error("Failed to initialize UI: " + std::string(app_res.error_name()));
        return 1;
    }

    auto app = std::move(app_res.value());

    // 2. Build view layout
    auto root = nxdev::ui::Container::column();
    root->set_align_items(nxdev::ui::Align::Center);
    root->set_justify_content(nxdev::ui::Align::Center);
    root->set_grow(1.0f);

    auto title = nxdev::ui::Label::create("Hello from NXDev UI");
    title->set_font_size(32.0f);
    title->set_margin(nxdev::ui::Insets(16.0f));
    root->add(title);

    auto btn = nxdev::ui::Button::create("Open Dialog");
    btn->set_padding(nxdev::ui::Insets(10.0f, 24.0f));
    btn->on_pressed([]() {
        nxdev::ui::Dialog::show({
            .title = "Greetings",
            .message = "Powered by Borealis backend!",
            .actions = {
                {.label = "OK", .callback = nullptr}
            }
        });
    });
    root->add(btn);

    // 3. Set root view and run main loop
    app.set_root(root);
    return app.run().succeeded() ? 0 : 1;
}
```

---

## UI Components & Concepts

### `nxdev::ui::Application`
Manages the application lifecycle, screen stack, main loop, and main-thread task dispatching:
- `Application::create(config)`
- `app.set_root(view)`
- `app.push_screen(view)` / `app.pop_screen()`
- `app.run()`
- `app.request_exit()`
- `Application::dispatch(task)`

### `nxdev::ui::Container`
Flexbox layout container arranging child views along rows or columns:
- `Container::row()` / `Container::column()`
- `container->add(child)` / `container->remove(child)` / `container->clear()`
- `container->set_align_items(align)` / `container->set_justify_content(align)`

### `nxdev::ui::Label`
Formatted text component:
- `Label::create("Text")`
- `label->set_font_size(24.0f)`
- `label->set_text_color(Color::White())`
- `label->set_alignment(TextAlignment::Center)`

### `nxdev::ui::Button`
Focusable interactive button:
- `Button::create("Click Me")`
- `button->on_pressed([]() { ... })`
- `button->set_enabled(true/false)`

### `nxdev::ui::Dialog`
Modal dialog for alerts and user confirmations:
- `Dialog::show({ .title = "...", .message = "...", .actions = { ... } })`

### `nxdev::ui::ScrollView` & `nxdev::ui::List`
Smooth scroll containers and data-driven lists:
- `ScrollView::create(Direction::Column)`
- `List::create()` / `list->set_data_source(ds)`

### `nxdev::ui::Theme`
Light/Dark theme management:
- `Theme::set_variant(ThemeVariant::Dark)`
- `Theme::accent()`, `Theme::background()`, `Theme::foreground()`

---

## Unstable Native Escape Hatch

If low-level access to the underlying `brls::*` objects is required for custom rendering, include the dedicated native header:

```cpp
#include <nxdev/ui/borealis/native.hpp>

// Warning: Direct use of brls:: types is backend-specific and not covered by API stability guarantees
brls::View* raw_view = nxdev::ui::borealis::get_native_view(my_view.get());
```

---

## RomFS Staging & Framework Resources

When `nxdev.borealis` is listed as a project dependency, NXDev's packaging pipeline automatically stages the essential Borealis framework resources (`font/`, `material/`, `i18n/`, `img/sys/`) into `.nxdev/build/<profile>/romfs/resources/`.

- **Compile Definition**: NXDev automatically configures `BRLS_RESOURCES="romfs:/resources/"` on `NXDev::Borealis`.
- **User Overrides**: You can customize or override any Borealis asset by simply placing a replacement file in your project's RomFS at `resources/...` (e.g. `romfs/resources/material/theme_light.json`).
- **No Manual Copy**: You never need to copy Borealis resource directories manually. NXDev stages everything safely prior to NRO/NSP packaging.
- For full details on layering and staging manifests, see [RomFS Staging Pipeline](file:///home/jvrcruz/Projects/NXDev/docs/build/romfs.md).
