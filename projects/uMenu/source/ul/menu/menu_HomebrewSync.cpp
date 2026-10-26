#include <ul/menu/menu_HomebrewSync.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/util/util_String.hpp>
#include <ul/ul_Result.hpp>
#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <set>
#include <vector>

namespace ul::menu {

    namespace {

        constexpr const char SteamUiPath[] = "sdmc:/ulaunch/steamui";
        constexpr const char KnownHomebrewListPath[] = "sdmc:/ulaunch/steamui/hb_known.txt";
        constexpr size_t MaxScannedHomebrew = 256;

        void CollectMenuHomebrew(const std::string &path, std::set<std::string> &out_nros, const u32 depth = 0) {
            if(depth > 8) {
                return;
            }

            for(const auto &entry: LoadEntries(path)) {
                if(entry.Is<EntryType::Homebrew>()) {
                    out_nros.insert(entry.hb_info.nro_target.nro_path);
                }
                else if(entry.Is<EntryType::Folder>()) {
                    CollectMenuHomebrew(entry.GetFolderPath(), out_nros, depth + 1);
                }
            }
        }

        inline bool IsNroName(const char *name) {
            const auto len = std::strlen(name);
            return (len > 4) && (strcasecmp(name + len - 4, ".nro") == 0);
        }

        void ScanDirectory(const std::string &dir, const bool recurse_once, std::vector<std::string> &out_nros) {
            auto d = opendir(dir.c_str());
            if(d == nullptr) {
                return;
            }

            std::vector<std::string> sub_dirs;
            while(auto ent = readdir(d)) {
                if(ent->d_name[0] == '.') {
                    // Skips ".", ".." and hidden folders such as ".overlays"
                    continue;
                }

                const auto full_path = fs::JoinPath(dir, ent->d_name);
                if(ent->d_type == DT_DIR) {
                    if(recurse_once) {
                        sub_dirs.push_back(full_path);
                    }
                }
                else if(IsNroName(ent->d_name) && (out_nros.size() < MaxScannedHomebrew)) {
                    out_nros.push_back(full_path);
                }
            }
            closedir(d);

            for(const auto &sub_dir: sub_dirs) {
                ScanDirectory(sub_dir, false, out_nros);
            }
        }

        std::set<std::string> LoadKnownList() {
            std::set<std::string> known;
            auto f = std::fopen(KnownHomebrewListPath, "r");
            if(f != nullptr) {
                char line[FS_MAX_PATH + 2] = {};
                while(std::fgets(line, sizeof(line), f) != nullptr) {
                    line[std::strcspn(line, "\r\n")] = '\0';
                    if(line[0] != '\0') {
                        known.insert(line);
                    }
                }
                std::fclose(f);
            }
            return known;
        }

        void SaveKnownList(const std::set<std::string> &known) {
            fs::CreateDirectory(SteamUiPath);
            auto f = std::fopen(KnownHomebrewListPath, "w");
            if(f != nullptr) {
                for(const auto &nro: known) {
                    std::fprintf(f, "%s\n", nro.c_str());
                }
                std::fclose(f);
            }
        }

    }

    u32 SyncHomebrewEntries(const std::string &menu_path) {
        std::vector<std::string> found;
        ScanDirectory(RootHomebrewPath, true, found);
        if(found.empty()) {
            return 0;
        }

        std::set<std::string> in_menu;
        CollectMenuHomebrew(menu_path, in_menu);
        auto known = LoadKnownList();
        const auto known_count_before = known.size();

        u32 added = 0;
        for(const auto &nro: found) {
            // hbmenu and uManager are already handled by uLaunch itself
            if((nro == HbmenuPath) || (nro == ManagerPath)) {
                continue;
            }
            if((in_menu.count(nro) == 0) && (known.count(nro) == 0)) {
                CreateHomebrewEntry(menu_path, nro, nro);
                in_menu.insert(nro);
                added++;
            }
            known.insert(nro);
        }

        if(known.size() != known_count_before) {
            SaveKnownList(known);
        }

        if(added > 0) {
            UL_LOG_INFO("steamui: added %u homebrew entries to the menu", added);
        }
        return added;
    }

}
