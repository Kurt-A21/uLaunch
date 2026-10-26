
#pragma once
#include <switch.h>
#include <string>

namespace ul::menu::ui {

    // steamui: "Performance" in the burger menu. Applies sys-clk overclock presets to the game that's
    // focused on the home screen (for homebrew/ports: to the take-over game they run as), shows live
    // clocks/temperatures, and can pause/resume overclocking.
    void SetPerformanceTarget(const u64 app_id, const std::string &name, const bool via_takeover);
    void ShowPerformanceMenu();

}
