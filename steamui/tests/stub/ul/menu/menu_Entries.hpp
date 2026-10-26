#pragma once
#include <switch.h>
#include <ul/fs/fs_Stdio.hpp>
#include <map>
#include <string>
#include <vector>
#include <cstring>
namespace ul {
    constexpr const char RootHomebrewPath[] = "sdmc:/switch";
    constexpr const char HbmenuPath[] = "sdmc:/hbmenu.nro";
    constexpr const char ManagerPath[] = "sdmc:/switch/uManager.nro";
}
namespace ul::menu {
    enum class EntryType : u32 { Invalid, Application, Homebrew, Folder };
    struct Entry {
        EntryType type; std::string entry_path;
        struct { struct { char nro_path[FS_MAX_PATH]; } nro_target; } hb_info;
        std::string folder;
        template<EntryType T> bool Is() const { return type == T; }
        std::string GetFolderPath() const { return folder; }
    };
    extern std::map<std::string, std::vector<Entry>> g_Menu;
    inline std::vector<Entry> LoadEntries(const std::string &path){ return g_Menu[path]; }
    inline Entry CreateHomebrewEntry(const std::string &base, const std::string &nro, const std::string &, const s32 = -1){
        Entry e{}; e.type = EntryType::Homebrew; std::strncpy(e.hb_info.nro_target.nro_path, nro.c_str(), FS_MAX_PATH - 1); g_Menu[base].push_back(e); return e; }
}
