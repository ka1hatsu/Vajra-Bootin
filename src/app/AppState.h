#pragma once

namespace vajra {

enum class Screen {
    Welcome,
    Scan,
    Recommendations,
    Downloads,
    Flash,
    History,
    About
};

struct AppState {
    Screen current_screen{Screen::Welcome};
    bool running{true};
};

} // namespace vajra
