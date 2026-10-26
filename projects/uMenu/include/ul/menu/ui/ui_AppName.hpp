
#pragma once
#include <switch.h>
#include <string>

namespace ul::menu::ui {

    // Name of an installed game from its control data ("" if unavailable)
    std::string LoadApplicationName(const u64 app_id);

}
