#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <memory>
#include <string>

using namespace ftxui;

// ============================================================================
// PAGE 1: COUNTER & PROGRESS BAR SHOWCASE
// ============================================================================
Component MakeCounterPage(std::function<void()> on_back) {
    // We use a shared_ptr for the state so it persists across renders
    // and is isolated only to this page.
    auto counter = std::make_shared<int>(0);

    auto btn_increment = Button(" + Increment ", [counter] { if (*counter < 100) *counter += 5; });
    auto btn_decrement = Button(" - Decrement ", [counter] { if (*counter > 0) *counter -= 5; });
    auto btn_back = Button(" <-- Back to Menu ", on_back);

    // Group the interactive controls
    auto controls = Container::Horizontal({
        btn_back,
        btn_decrement,
        btn_increment,
    });

    // Return a Renderer that binds the controls to the visual DOM
    return Renderer(controls, [=] {
        float progress = *counter / 100.0f;

        auto ui = vbox({
            text(" Counter & Progress ") | bold | center,
            separator(),
            hbox({
                text("Current Value: "),
                text(std::to_string(*counter)) | bold | color(Color::GreenLight),
            }) | center,
            separatorEmpty(),
            gauge(progress) | color(Color::BlueLight),
            separatorEmpty(),
            hbox({
                btn_decrement->Render(),
                text(" "),
                btn_increment->Render(),
                filler(),
                btn_back->Render(),
            }),
        }) | borderHeavy;

        return center(ui); // Center the whole box on the screen
    });
}

// ============================================================================
// PAGE 2: TEXT & BORDER STYLES SHOWCASE
// ============================================================================
Component MakeStylesPage(std::function<void()> on_back) {
    auto btn_back = Button(" <-- Back to Menu ", on_back);

    return Renderer(btn_back, [=] {
        auto styled_text = vbox({
            text(" Text Styles ") | bold | center,
            separator(),
            text("Normal text"),
            text("Bold text") | bold,
            text("Colored text") | color(Color::CyanLight),
        }) | border;

        auto border_styles = vbox({
            text(" Borders ") | bold | center,
            separator(),
            text("Heavy") | borderHeavy,
            text("Double") | borderDouble,
            text("Rounded") | borderRounded,
        }) | border;

        auto ui = vbox({
            text(" Visual Styles ") | bold | center | color(Color::YellowLight),
            separatorHeavy(),
            hbox({ styled_text, border_styles }) | center,
            separatorEmpty(),
            btn_back->Render() | center,
        }) | borderHeavy;

        return center(ui);
    });
}

// ============================================================================
// MAIN MENU
// ============================================================================
Component MakeMainMenu(std::function<void()> go_counter,
                       std::function<void()> go_styles,
                       std::function<void()> on_quit) {

    auto btn_counter = Button(" 1. Counter & Gauge App ", go_counter);
    auto btn_styles  = Button(" 2. Visual Styles App   ", go_styles);
    auto btn_quit    = Button(" 3. Quit Application    ", on_quit);

    auto menu_container = Container::Vertical({
        btn_counter,
        btn_styles,
        btn_quit,
    });

    return Renderer(menu_container, [=] {
        auto ui = vbox({
            text(" FTXUI MULTI-PAGE DASHBOARD ") | bold | center | color(Color::MagentaLight),
            separatorDouble(),
            text("Please select an application to launch:") | dim | center,
            separatorEmpty(),
            btn_counter->Render() | center,
            btn_styles->Render()  | center,
            separatorEmpty(),
            btn_quit->Render()    | center,
        }) | borderRounded;

        return center(ui);
    });
}

// ============================================================================
// APPLICATION ENTRY POINT
// ============================================================================
int main() {
    auto screen = ScreenInteractive::TerminalOutput();

    // The tab_index determines which page is currently active
    int tab_index = 0;

    // Navigation callbacks
    auto go_menu    = [&] { tab_index = 0; };
    auto go_counter = [&] { tab_index = 1; };
    auto go_styles  = [&] { tab_index = 2; };
    auto do_quit    = screen.ExitLoopClosure();

    // Build the isolated pages
    auto page_menu    = MakeMainMenu(go_counter, go_styles, do_quit);
    auto page_counter = MakeCounterPage(go_menu);
    auto page_styles  = MakeStylesPage(go_menu);

    // The Router: A Tab container that holds all pages and switches based on tab_index
    auto router = Container::Tab(
        { page_menu, page_counter, page_styles },
        &tab_index
    );

    // Run the app!
    screen.Loop(router);

    return 0;
}