
#pragma once
#include <ul/menu/menu_Entries.hpp>

namespace ul::menu {

    // steamui: puts every homebrew app (.nro) found in sdmc:/switch and its direct subfolders on the home screen.
    // Each NRO is only ever auto-added once (remembered in sdmc:/ulaunch/steamui/hb_known.txt), so removing an
    // entry from the menu keeps it removed. Returns how many entries were added.
    u32 SyncHomebrewEntries(const std::string &menu_path);

}
