#include "test_common.hpp"
#include <nxdev/ui.hpp>
#include <nxdev/ui/borealis/native.hpp>
#include <iostream>
#include <memory>
#include <string>

// Test ListDataSource implementation
class SimpleDataSource : public nxdev::ui::ListDataSource {
public:
    explicit SimpleDataSource(size_t count) : count_(count) {}

    [[nodiscard]] size_t count() const override {
        return count_;
    }

    std::shared_ptr<nxdev::ui::View> create_view(size_t index) override {
        return nxdev::ui::Label::create("Data Item #" + std::to_string(index));
    }

    void bind_view(std::shared_ptr<nxdev::ui::View> view, size_t index) override {
        if (auto lbl = std::dynamic_pointer_cast<nxdev::ui::Label>(view)) {
            lbl->set_text("Bound Item #" + std::to_string(index));
        }
    }

private:
    size_t count_;
};

int main() {
    std::cout << "[Test] Running NXDev UI / Borealis Module Test Suite...\n";

    // =========================================================================
    // 1. Types & Colors
    // =========================================================================
    std::cout << "  -> Testing UI Types & Colors...\n";
    {
        nxdev::ui::Color white = nxdev::ui::Color::White();
        NXDEV_TEST_ASSERT(white.r == 1.0f && white.g == 1.0f && white.b == 1.0f && white.a == 1.0f);

        nxdev::ui::Color rgba = nxdev::ui::Color::from_rgba(255, 128, 0, 255);
        NXDEV_TEST_ASSERT(rgba.r > 0.99f && rgba.g > 0.49f && rgba.g < 0.51f && rgba.b == 0.0f);

        nxdev::ui::Color hex = nxdev::ui::Color::from_rgb_hex(0xff8000);
        NXDEV_TEST_ASSERT(hex == rgba);

        nxdev::ui::Insets insets(10.0f, 20.0f);
        NXDEV_TEST_ASSERT(insets.top == 10.0f && insets.bottom == 10.0f);
        NXDEV_TEST_ASSERT(insets.left == 20.0f && insets.right == 20.0f);

        nxdev::ui::Size sz(1280.0f, 720.0f);
        NXDEV_TEST_ASSERT(sz.width == 1280.0f && sz.height == 720.0f);
    }
    std::cout << "  ✓ Types & Colors passed\n";

    // =========================================================================
    // 2. View Base Properties & Ownership
    // =========================================================================
    std::cout << "  -> Testing View Base Properties & Lifecycle...\n";
    {
        auto view = nxdev::ui::View::create();
        NXDEV_TEST_ASSERT(view != nullptr);

        view->set_size(200.0f, 100.0f);
        NXDEV_TEST_ASSERT(view->width() == 200.0f);
        NXDEV_TEST_ASSERT(view->height() == 100.0f);
        NXDEV_TEST_ASSERT(view->size().width == 200.0f && view->size().height == 100.0f);

        view->set_visible(true);
        NXDEV_TEST_ASSERT(view->is_visible());
        view->set_visible(false);
        NXDEV_TEST_ASSERT(!view->is_visible());

        view->set_alpha(0.75f);
        NXDEV_TEST_ASSERT(view->alpha() == 0.75f);

        view->set_focusable(true);
        NXDEV_TEST_ASSERT(view->is_focusable());

        bool clicked = false;
        view->on_click([&clicked]() {
            clicked = true;
        });

        // Parent should be null before adding to container
        NXDEV_TEST_ASSERT(view->parent() == nullptr);
    }
    std::cout << "  ✓ View Base Properties passed\n";

    // =========================================================================
    // 3. Container & Child Hierarchy
    // =========================================================================
    std::cout << "  -> Testing Container & Child Hierarchy...\n";
    {
        auto col = nxdev::ui::Container::column();
        NXDEV_TEST_ASSERT(col != nullptr);
        NXDEV_TEST_ASSERT(col->child_count() == 0);

        auto lbl1 = nxdev::ui::Label::create("First");
        auto lbl2 = nxdev::ui::Label::create("Second");
        auto lbl3 = nxdev::ui::Label::create("Third");

        col->add(lbl1);
        col->add(lbl2);
        NXDEV_TEST_ASSERT(col->child_count() == 2);
        NXDEV_TEST_ASSERT(lbl1->parent() == col);
        NXDEV_TEST_ASSERT(lbl2->parent() == col);
        NXDEV_TEST_ASSERT(col->child_at(0) == lbl1);
        NXDEV_TEST_ASSERT(col->child_at(1) == lbl2);

        // Insert at index 1
        col->add(lbl3, 1);
        NXDEV_TEST_ASSERT(col->child_count() == 3);
        NXDEV_TEST_ASSERT(col->child_at(1) == lbl3);

        // Remove child
        bool removed = col->remove(lbl3);
        NXDEV_TEST_ASSERT(removed);
        NXDEV_TEST_ASSERT(col->child_count() == 2);
        NXDEV_TEST_ASSERT(lbl3->parent() == nullptr);

        // Clear
        col->clear();
        NXDEV_TEST_ASSERT(col->child_count() == 0);
        NXDEV_TEST_ASSERT(lbl1->parent() == nullptr);
    }
    std::cout << "  ✓ Container Hierarchy passed\n";

    // =========================================================================
    // 4. Label & Button Components
    // =========================================================================
    std::cout << "  -> Testing Label & Button...\n";
    {
        auto label = nxdev::ui::Label::create("Hello World");
        NXDEV_TEST_ASSERT(label->text() == "Hello World");

        label->set_text("Updated");
        NXDEV_TEST_ASSERT(label->text() == "Updated");

        label->set_font_size(32.0f);
        NXDEV_TEST_ASSERT(label->font_size() == 32.0f);

        label->set_text_color(nxdev::ui::Color::Red());
        NXDEV_TEST_ASSERT(label->text_color() == nxdev::ui::Color::Red());

        auto button = nxdev::ui::Button::create("Action");
        NXDEV_TEST_ASSERT(button->text() == "Action");
        NXDEV_TEST_ASSERT(button->is_enabled());

        bool pressed = false;
        button->on_pressed([&pressed]() {
            pressed = true;
        });

        button->set_enabled(false);
        NXDEV_TEST_ASSERT(!button->is_enabled());
    }
    std::cout << "  ✓ Label & Button passed\n";

    // =========================================================================
    // 5. ScrollView, Dialog, List
    // =========================================================================
    std::cout << "  -> Testing ScrollView, Dialog & List...\n";
    {
        // ScrollView
        auto scroll = nxdev::ui::ScrollView::create(nxdev::ui::Direction::Column);
        auto inner = nxdev::ui::Container::column();
        scroll->set_content(inner);
        NXDEV_TEST_ASSERT(scroll->content() == inner);

        // Dialog
        auto dlg = std::make_shared<nxdev::ui::Dialog>("Test Title", "Test Message");
        NXDEV_TEST_ASSERT(dlg->title() == "Test Title");
        NXDEV_TEST_ASSERT(dlg->message() == "Test Message");

        dlg->add_action("OK", nullptr, false);
        dlg->add_cancel_action("Cancel", nullptr);

        // List & DataSource
        auto list = nxdev::ui::List::create();
        list->add_item("Static Item 1");
        list->add_item("Static Item 2");
        NXDEV_TEST_ASSERT(list->item_count() == 2);

        auto ds = std::make_shared<SimpleDataSource>(5);
        list->set_data_source(ds);
        NXDEV_TEST_ASSERT(list->item_count() == 5);
    }
    std::cout << "  ✓ ScrollView, Dialog & List passed\n";

    // =========================================================================
    // 6. Theme & Diagnostics
    // =========================================================================
    std::cout << "  -> Testing Theme & Diagnostics...\n";
    {
        nxdev::ui::Theme::set_variant(nxdev::ui::ThemeVariant::Dark);
        NXDEV_TEST_ASSERT(nxdev::ui::Theme::variant() == nxdev::ui::ThemeVariant::Dark);

        nxdev::ui::Theme::set_variant(nxdev::ui::ThemeVariant::Light);
        NXDEV_TEST_ASSERT(nxdev::ui::Theme::variant() == nxdev::ui::ThemeVariant::Light);

        auto info = nxdev::ui::Application::backend_info();
        NXDEV_TEST_ASSERT(info.backend_name == "borealis");
        NXDEV_TEST_ASSERT(info.revision == "5f08b286f3df737f3321d2247a6fe633fcead03c");
        NXDEV_TEST_ASSERT(info.api_version == 1);
    }
    std::cout << "  ✓ Theme & Diagnostics passed\n";

    // =========================================================================
    // 7. Application Coordinator & Navigation Stack
    // =========================================================================
    std::cout << "  -> Testing Application Coordinator & Screen Stack...\n";
    {
        auto app_res = nxdev::ui::Application::create({.name = "Unit Test App"});
        NXDEV_TEST_ASSERT(app_res.succeeded());

        auto app = std::move(app_res.value());

        auto root = nxdev::ui::Container::column();
        auto set_root_res = app.set_root(root);
        NXDEV_TEST_ASSERT(set_root_res.succeeded());

        auto screen2 = nxdev::ui::Container::column();
        auto push_res = app.push_screen(screen2);
        NXDEV_TEST_ASSERT(push_res.succeeded());

        auto pop_res = app.pop_screen();
        NXDEV_TEST_ASSERT(pop_res.succeeded());

        // Cannot pop root
        auto bad_pop = app.pop_screen();
        NXDEV_TEST_ASSERT(bad_pop.failed());

        // Dispatch task
        bool task_ran = false;
        nxdev::ui::Application::dispatch([&task_ran]() {
            task_ran = true;
        });

        // Run mock step
        auto run_res = app.run();
        NXDEV_TEST_ASSERT(run_res.succeeded());
        NXDEV_TEST_ASSERT(task_ran);
    }
    std::cout << "  ✓ Application Coordinator passed\n";

    std::cout << "[Test] All NXDev UI / Borealis Unit Tests Passed Successfully!\n";
    return 0;
}
