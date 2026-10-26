#include <ul/menu/menu_HomebrewSync.hpp>
#include <cstdio>
#include <fstream>
#include <set>
#include <sys/stat.h>
namespace ul::menu { std::map<std::string, std::vector<Entry>> g_Menu; }
using namespace ul::menu;
static int fails = 0;
#define CHECK(c, m) do { if(c) std::printf("  ok   %s\n", m); else { std::printf("  FAIL %s\n", m); fails++; } } while(0)
static void touch(const std::string &p){ std::ofstream(p) << "x"; }
static std::set<std::string> menu_nros(){ std::set<std::string> s; for(auto &[k, v]: g_Menu) for(auto &e: v) if(e.Is<EntryType::Homebrew>()) s.insert(e.hb_info.nro_target.nro_path); return s; }
int main(){
    for(auto d: { "sdmc:", "sdmc:/switch", "sdmc:/switch/JKSV", "sdmc:/switch/nfscx", "sdmc:/switch/.overlays", "sdmc:/switch/deep", "sdmc:/switch/deep/deeper", "sdmc:/ulaunch" }) mkdir(d, 0777);
    touch("sdmc:/switch/ftpd.nro"); touch("sdmc:/switch/JKSV/JKSV.nro"); touch("sdmc:/switch/nfscx/nfscx-vulkan.NRO");
    touch("sdmc:/switch/.overlays/sys-clk.ovl"); touch("sdmc:/switch/.overlays/fake.nro"); touch("sdmc:/switch/deep/deeper/toodeep.nro");
    touch("sdmc:/switch/uManager.nro"); touch("sdmc:/switch/readme.txt");
    const std::string root = "sdmc:/ulaunch/menu/user";
    // JKSV already on the menu, inside a folder
    Entry folder{}; folder.type = EntryType::Folder; folder.folder = root + "/f1"; g_Menu[root].push_back(folder);
    Entry jk{}; jk.type = EntryType::Homebrew; std::snprintf(jk.hb_info.nro_target.nro_path, FS_MAX_PATH, "sdmc:/switch/JKSV/JKSV.nro"); g_Menu[root + "/f1"].push_back(jk);

    std::printf("== Homebrew auto-add ==\n");
    auto n = SyncHomebrewEntries(root);
    auto s = menu_nros();
    CHECK(n == 2, "first run adds the 2 new apps");
    CHECK(s.count("sdmc:/switch/ftpd.nro") && s.count("sdmc:/switch/nfscx/nfscx-vulkan.NRO"), "top-level and one-folder-deep NROs added (case-insensitive .nro)");
    CHECK(!s.count("sdmc:/switch/.overlays/fake.nro"), "hidden folders (.overlays) ignored");
    CHECK(!s.count("sdmc:/switch/deep/deeper/toodeep.nro"), "only one subfolder level scanned");
    CHECK(!s.count("sdmc:/switch/uManager.nro"), "uLaunch's own uManager not duplicated");
    int jk_count = 0; for(auto &[k, v]: g_Menu) for(auto &e: v) if(e.Is<EntryType::Homebrew>() && std::string(e.hb_info.nro_target.nro_path) == "sdmc:/switch/JKSV/JKSV.nro") jk_count++;
    CHECK(jk_count == 1, "app already in a menu folder is not added twice");
    CHECK(SyncHomebrewEntries(root) == 0, "second run adds nothing");
    // User removes ftpd from the menu -> must stay removed
    auto &r = g_Menu[root]; r.erase(std::remove_if(r.begin(), r.end(), [](auto &e){ return std::string(e.hb_info.nro_target.nro_path) == "sdmc:/switch/ftpd.nro"; }), r.end());
    CHECK(SyncHomebrewEntries(root) == 0 && !menu_nros().count("sdmc:/switch/ftpd.nro"), "removed app stays removed");
    touch("sdmc:/switch/Moonlight.nro");
    CHECK(SyncHomebrewEntries(root) == 1 && menu_nros().count("sdmc:/switch/Moonlight.nro"), "newly copied app appears on next load");
    std::remove("sdmc:/ulaunch/steamui/hb_known.txt");
    CHECK(SyncHomebrewEntries(root) == 1, "deleting hb_known.txt re-adds removed apps");
    std::printf("\n%s (%d failure%s)\n", fails ? "FAILED" : "ALL PASSED", fails, fails == 1 ? "" : "s");
    return fails;
}
