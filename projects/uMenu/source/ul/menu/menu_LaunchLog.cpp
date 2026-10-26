#include <ul/menu/menu_LaunchLog.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <cstdio>
#include <ctime>
#include <sys/stat.h>

namespace ul::menu {

    namespace {

        constexpr const char SteamUiPath[] = "sdmc:/ulaunch/steamui";
        constexpr const char LaunchLogPath[] = "sdmc:/ulaunch/steamui/launches.txt";
        constexpr const char LaunchLogOldPath[] = "sdmc:/ulaunch/steamui/launches.old.txt";
        constexpr long MaxLaunchLogSize = 32 * 1024;

    }

    void RecordLaunch(const std::string &key) {
        if(key.empty()) {
            return;
        }

        // Same local time the menu's clock shows (and that album capture names use)
        const auto now = std::time(nullptr);
        std::tm local = {};
        if(localtime_r(&now, &local) == nullptr) {
            return;
        }
        char stamp[32] = {};
        std::strftime(stamp, sizeof(stamp), "%Y%m%d%H%M%S", &local);

        fs::CreateDirectory(SteamUiPath);
        struct stat st;
        if((stat(LaunchLogPath, &st) == 0) && (st.st_size > MaxLaunchLogSize)) {
            std::remove(LaunchLogOldPath);
            std::rename(LaunchLogPath, LaunchLogOldPath);
        }

        if(auto f = std::fopen(LaunchLogPath, "a")) {
            std::fprintf(f, "%s %s\n", stamp, key.c_str());
            std::fclose(f);
        }
    }

}
