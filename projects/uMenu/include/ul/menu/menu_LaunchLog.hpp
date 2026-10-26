
#pragma once
#include <string>

namespace ul::menu {

    // steamui: remembers which game/homebrew was launched when, so steamui_media.py can match a
    // title-screen video capture to its game automatically (album captures are named by local time).
    //   sdmc:/ulaunch/steamui/launches.txt   one line per launch: "YYYYMMDDHHMMSS KEY"
    // KEY = 16-digit title ID, or the NRO name for homebrew. The file is kept small (rotated at 32 KB).
    void RecordLaunch(const std::string &key);

}
