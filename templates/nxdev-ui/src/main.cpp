#include <nxdev/ui.hpp>
#include <nxdev/log.hpp>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // Initialize UI application runtime
    auto app_res = nxdev::ui::Application::create({
        .name = "{{PROJECT_NAME}}"
    });

    if (app_res.failed()) {
        nxdev::log::error("Failed to initialize NXDev UI runtime: " + std::string(app_res.error_name()));
        return 1;
    }

    auto app = std::move(app_res.value());

    // Build UI layout
    auto root = nxdev::ui::Container::column();
    root->set_align_items(nxdev::ui::Align::Center);
    root->set_justify_content(nxdev::ui::Align::Center);
    root->set_grow(1.0f);

    auto title = nxdev::ui::Label::create("{{PROJECT_NAME}}");
    title->set_font_size(36.0f);
    title->set_margin(nxdev::ui::Insets(16.0f));
    root->add(title);

    auto subtitle = nxdev::ui::Label::create("Built with NXDev UI & Borealis");
    subtitle->set_font_size(22.0f);
    subtitle->set_text_color(nxdev::ui::Color::from_rgba(180, 180, 180));
    subtitle->set_margin(nxdev::ui::Insets(8.0f));
    root->add(subtitle);

    auto dialog_btn = nxdev::ui::Button::create("Open Dialog");
    dialog_btn->set_margin(nxdev::ui::Insets(12.0f));
    dialog_btn->set_padding(nxdev::ui::Insets(10.0f, 24.0f));
    dialog_btn->on_pressed([]() {
        nxdev::ui::Dialog::show({
            .title = "NXDev UI",
            .message = "Hello from the NXDev UI abstraction framework!",
            .actions = {
                {.label = "OK", .callback = nullptr, .is_cancel = false}
            }
        });
    });
    root->add(dialog_btn);

    auto exit_btn = nxdev::ui::Button::create("Exit");
    exit_btn->set_margin(nxdev::ui::Insets(12.0f));
    exit_btn->set_padding(nxdev::ui::Insets(10.0f, 24.0f));
    exit_btn->on_pressed([&app]() {
        app.request_exit();
    });
    root->add(exit_btn);

    // Set root view and start main loop
    app.set_root(root);
    auto run_res = app.run();

    return run_res.succeeded() ? 0 : 1;
}
