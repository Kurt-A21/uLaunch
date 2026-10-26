// Switching between steamui and Nintendo's HOME menu (ul_HomeMenuSwitch.hpp) on a fake SD card.
#include <ul/ul_HomeMenuSwitch.hpp>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
using namespace ul::homemenu;
static int fails = 0;
#define CHECK(c, ...) do { if(c) std::printf("  ok    "); else { std::printf("  FAIL  "); fails++; } std::printf(__VA_ARGS__); std::printf("\n"); } while(0)
static void Put(const std::string &p, const std::string &data) {
    std::string cmd = "mkdir -p '" + p.substr(0, p.rfind('/')) + "'"; if(std::system(cmd.c_str()) != 0) std::abort();
    std::ofstream(p, std::ios::binary) << data;
}
static std::string Get(const std::string &p) { std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }
static bool IsDir(const std::string &p) { struct stat st; return stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode); }
int main() {
    char tmpl[] = "/tmp/claude-0/hm.XXXXXX"; std::system("mkdir -p /tmp/claude-0");
    const std::string sd = mkdtemp(tmpl);
    const std::string exefs = sd + ActiveExefs, theme = sd + ContentsDir + "/romfs/lyt/ResidentMenu.szs";
    std::string big(300000, 'U'); for(size_t i = 0; i < big.size(); i++) big[i] = static_cast<char>(i * 31);
    Put(sd + BaseExefs, big); Put(exefs, big); Put(theme, "MY-NXTHEME");

    std::printf("== steamui -> Nintendo HOME (NXTheme installed) ==\n");
    CHECK(IsActive(sd) && HasBackup(sd), "starts with steamui active, backup present");
    CHECK(Deactivate(sd) && !IsActive(sd), "switch off: exefs.nsp removed");
    CHECK(Get(theme) == "MY-NXTHEME", "the NXTheme is still there");
    CHECK(IsDir(sd + ContentsDir), "folder kept (it holds the theme)");
    CHECK(Deactivate(sd), "switching off twice is harmless");

    std::printf("== Nintendo HOME -> steamui ==\n");
    CHECK(Activate(sd) && IsActive(sd), "switch on: exefs.nsp back");
    CHECK(Get(exefs) == big, "exact copy of the backup (%zu bytes)", big.size());
    CHECK(Get(theme) == "MY-NXTHEME", "NXTheme untouched");
    CHECK(!IsFile(exefs + ".tmp"), "no temp file left behind");
    bool stable = true;
    for(int i = 0; i < 5; i++) { stable = stable && Deactivate(sd) && !IsActive(sd) && Activate(sd) && Get(exefs) == big && Get(theme) == "MY-NXTHEME"; }
    CHECK(stable, "5 more round trips: same files every time");

    std::printf("== No NXTheme ==\n");
    std::system(("rm -rf '" + sd + ContentsDir + "/romfs'").c_str());
    CHECK(Deactivate(sd) && !IsDir(sd + ContentsDir), "switch off leaves no empty folder (stock state)");
    CHECK(Activate(sd) && IsActive(sd) && Get(exefs) == big, "switch on recreates the folder");

    std::printf("== Safety ==\n");
    std::remove((sd + BaseExefs).c_str());
    CHECK(!Deactivate(sd) && IsActive(sd), "no backup copy: refuses to switch off (there'd be no way back)");
    CHECK(!Activate(sd), "no backup copy: switch on reports failure");
    std::system(("rm -rf '" + sd + "'").c_str());
    std::printf(fails ? "FAILED: %d\n" : "ALL PASSED\n", fails);
    return fails ? 1 : 0;
}
