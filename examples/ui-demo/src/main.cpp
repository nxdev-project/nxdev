#include <nxdev/ui.hpp>
#include <nxdev/log.hpp>
#include <vector>
#include <string>

// Create second page for screen navigation testing
std::shared_ptr<nxdev::ui::Container> create_second_screen(nxdev::ui::Application& app) {
    auto screen = nxdev::ui::Container::column();
    screen->set_padding(nxdev::ui::Insets(32.0f));
    screen->set_align_items(nxdev::ui::Align::Center);
    screen->set_justify_content(nxdev::ui::Align::Center);
    screen->set_grow(1.0f);

    auto heading = nxdev::ui::Label::create("Second Screen");
    heading->set_font_size(32.0f);
    heading->set_margin(nxdev::ui::Insets(16.0f));
    screen->add(heading);

    auto desc = nxdev::ui::Label::create("Demonstrating NXDev UI screen stack navigation (push/pop).");
    desc->set_font_size(20.0f);
    desc->set_text_color(nxdev::ui::Color::from_rgba(200, 200, 200));
    desc->set_margin(nxdev::ui::Insets(12.0f));
    screen->add(desc);

    auto back_btn = nxdev::ui::Button::create("Back to Main Screen");
    back_btn->set_padding(nxdev::ui::Insets(10.0f, 24.0f));
    back_btn->set_margin(nxdev::ui::Insets(16.0f));
    back_btn->on_pressed([&app]() {
        (void)app.pop_screen();
    });
    screen->add(back_btn);

    return screen;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    nxdev::log::info("Starting NXDev UI Demo...");

    // 1. Initialize UI Application
    auto app_res = nxdev::ui::Application::create({
        .name = "NXDev UI Demo"
    });

    if (app_res.failed()) {
        nxdev::log::error("Failed to initialize NXDev UI runtime: " + std::string(app_res.error_name()));
        return 1;
    }

    auto app = std::move(app_res.value());

    // 2. Build Root Layout
    auto root = nxdev::ui::Container::column();
    root->set_padding(nxdev::ui::Insets(24.0f));
    root->set_grow(1.0f);

    // Header
    auto header = nxdev::ui::Container::row();
    header->set_align_items(nxdev::ui::Align::Center);
    header->set_justify_content(nxdev::ui::Align::SpaceBetween);
    header->set_margin(nxdev::ui::Insets(8.0f, 16.0f));

    auto title = nxdev::ui::Label::create("NXDev UI Showcase");
    title->set_font_size(28.0f);
    header->add(title);

    auto status_lbl = nxdev::ui::Label::create("Ready");
    status_lbl->set_font_size(18.0f);
    status_lbl->set_text_color(nxdev::ui::Theme::accent());
    header->add(status_lbl);

    root->add(header);

    // Scrollable Content Area
    auto scroll_view = nxdev::ui::ScrollView::create(nxdev::ui::Direction::Column);
    scroll_view->set_grow(1.0f);

    auto content = nxdev::ui::Container::column();
    content->set_padding(nxdev::ui::Insets(12.0f));

    // Section 1: Interactive Buttons & Dialogs
    auto sec1_title = nxdev::ui::Label::create("Interactive Dialogs");
    sec1_title->set_font_size(22.0f);
    sec1_title->set_margin(nxdev::ui::Insets(8.0f, 0.0f));
    content->add(sec1_title);

    auto btn_row = nxdev::ui::Container::row();
    btn_row->set_margin(nxdev::ui::Insets(8.0f, 0.0f));

    auto dialog_btn = nxdev::ui::Button::create("Open Dialog");
    dialog_btn->set_padding(nxdev::ui::Insets(8.0f, 16.0f));
    dialog_btn->set_margin(nxdev::ui::Insets(4.0f, 8.0f));
    dialog_btn->on_pressed([status_lbl]() {
        status_lbl->set_text("Dialog Opened");
        nxdev::ui::Dialog::show({
            .title = "NXDev UI Dialog",
            .message = "Controller navigation and button focus work seamlessly across platforms.",
            .actions = {
                {
                    .label = "Confirm",
                    .callback = [status_lbl]() { status_lbl->set_text("Confirmed"); },
                    .is_cancel = false
                },
                {
                    .label = "Cancel",
                    .callback = [status_lbl]() { status_lbl->set_text("Cancelled"); },
                    .is_cancel = true
                }
            }
        });
    });
    btn_row->add(dialog_btn);

    auto nav_btn = nxdev::ui::Button::create("Push Screen");
    nav_btn->set_padding(nxdev::ui::Insets(8.0f, 16.0f));
    nav_btn->set_margin(nxdev::ui::Insets(4.0f, 8.0f));
    nav_btn->on_pressed([&app, status_lbl]() {
        status_lbl->set_text("Navigated to Screen 2");
        (void)app.push_screen(create_second_screen(app));
    });
    btn_row->add(nav_btn);

    content->add(btn_row);

    // Section 2: Themes & Appearance
    auto sec2_title = nxdev::ui::Label::create("Theme Appearance");
    sec2_title->set_font_size(22.0f);
    sec2_title->set_margin(nxdev::ui::Insets(16.0f, 0.0f, 8.0f, 0.0f));
    content->add(sec2_title);

    auto theme_row = nxdev::ui::Container::row();
    theme_row->set_margin(nxdev::ui::Insets(8.0f, 0.0f));

    auto dark_btn = nxdev::ui::Button::create("Dark Theme");
    dark_btn->set_padding(nxdev::ui::Insets(8.0f, 16.0f));
    dark_btn->set_margin(nxdev::ui::Insets(4.0f, 8.0f));
    dark_btn->on_pressed([status_lbl]() {
        nxdev::ui::Theme::set_variant(nxdev::ui::ThemeVariant::Dark);
        status_lbl->set_text("Theme: Dark");
    });
    theme_row->add(dark_btn);

    auto light_btn = nxdev::ui::Button::create("Light Theme");
    light_btn->set_padding(nxdev::ui::Insets(8.0f, 16.0f));
    light_btn->set_margin(nxdev::ui::Insets(4.0f, 8.0f));
    light_btn->on_pressed([status_lbl]() {
        nxdev::ui::Theme::set_variant(nxdev::ui::ThemeVariant::Light);
        status_lbl->set_text("Theme: Light");
    });
    theme_row->add(light_btn);

    content->add(theme_row);

    // Section 3: List & Items
    auto sec3_title = nxdev::ui::Label::create("List Components");
    sec3_title->set_font_size(22.0f);
    sec3_title->set_margin(nxdev::ui::Insets(16.0f, 0.0f, 8.0f, 0.0f));
    content->add(sec3_title);

    auto list = nxdev::ui::List::create();
    for (int i = 1; i <= 6; ++i) {
        std::string item_name = "Feature Item #" + std::to_string(i);
        list->add_item(item_name, [status_lbl, item_name]() {
            status_lbl->set_text("Selected: " + item_name);
        });
    }
    content->add(list);

    // Section 4: Exit
    auto exit_btn = nxdev::ui::Button::create("Exit Demo");
    exit_btn->set_padding(nxdev::ui::Insets(10.0f, 24.0f));
    exit_btn->set_margin(nxdev::ui::Insets(24.0f, 0.0f));
    exit_btn->on_pressed([&app]() {
        app.request_exit();
    });
    content->add(exit_btn);

    scroll_view->set_content(content);
    root->add(scroll_view);

    // 3. Set Root and Run
    (void)app.set_root(root);
    auto run_res = app.run();

    nxdev::log::info("UI Demo finished with result: " + std::string(run_res.succeeded() ? "Success" : "Failure"));
    return run_res.succeeded() ? 0 : 1;
}
