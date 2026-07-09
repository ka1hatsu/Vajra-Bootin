#pragma once

#include "hardware/HardwareProfile.h"

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
    bool hardware_scanned{false};
    hardware::HardwareProfile hardware{};
    int purpose_index{0};
    int experience_index{0};
};

} // namespace vajra
