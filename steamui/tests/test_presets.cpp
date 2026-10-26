// Checks the burger-menu overclock presets against oc_profile.py's PRESETS (passed in as JSON lines),
// and that sys-clk profile detection round-trips.
#include <ul/menu/ui/ui_PerformancePresets.hpp>
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
using namespace ul::menu::ui::perf;
static int fails = 0;
#define CHECK(c, ...) do { if(c) std::printf("  ok    "); else { std::printf("  FAIL  "); fails++; } std::printf(__VA_ARGS__); std::printf("\n"); } while(0)
static const char *Prof[] = { "handheld", "handheld_charging", "handheld_charging_usb", "handheld_charging_official", "docked" };
static const char *Mod[] = { "cpu", "gpu", "mem" };
int main(){
    // stdin: "<preset> <key> <mhz>" per line, from oc_profile.py
    std::map<std::string, std::map<std::string, unsigned>> py; std::string p, k; unsigned v;
    while(std::cin >> p >> k >> v) py[p][k] = v;
    py["stock"];
    for(unsigned i = 0; i < (unsigned)Preset::Count; i++) {
        auto preset = (Preset)i; std::string name = GetPresetName(preset);
        std::string lname = name; for(auto &c: lname) c = std::tolower(c);
        SysClkTitleProfileList l; FillProfiles(preset, l);
        bool same = py.count(lname) > 0; int n = 0;
        for(int pr = 0; pr < SysClkProfile_EnumMax; pr++) for(int m = 0; m < SysClkModule_EnumMax; m++) {
            std::string key = std::string(Prof[pr]) + "_" + Mod[m];
            unsigned want = py[lname].count(key) ? py[lname][key] : 0;
            if(l.mhzMap[pr][m] != want) { same = false; std::printf("        %s %s: menu %u vs oc_profile.py %u\n", name.c_str(), key.c_str(), l.mhzMap[pr][m], want); }
            n += l.mhzMap[pr][m] != 0;
        }
        CHECK(same, "%s preset identical in menu and oc_profile.py (%d values)", name.c_str(), n);
        CHECK(DetectPreset(l) == (int)i, "%s detected back from sys-clk's profile", name.c_str());
    }
    SysClkTitleProfileList custom; FillProfiles(Preset::Balanced, custom); custom.mhzMap[SysClkProfile_Docked][SysClkModule_CPU] = 1581;
    CHECK(DetectPreset(custom) == CustomPreset, "hand-tuned profile shown as Custom");
    SysClkContext ctx = {}; ctx.freqs[SysClkModule_CPU] = 1785000000; ctx.freqs[SysClkModule_GPU] = 921600000; ctx.freqs[SysClkModule_MEM] = 1600000000;
    ctx.temps[SysClkThermalSensor_SOC] = 52340; ctx.temps[SysClkThermalSensor_PCB] = 41080; ctx.profile = SysClkProfile_Docked;
    auto s = FormatStatus(ctx); std::printf("        status text: %s\n", s.c_str());
    CHECK(s.find("CPU 1785") != std::string::npos && s.find("GPU 921") != std::string::npos && s.find("52.3") != std::string::npos && s.find("Docked") != std::string::npos, "live status shows clocks, temperature and dock state");
    std::printf("\n%s (%d failure%s)\n", fails ? "FAILED" : "ALL PASSED", fails, fails == 1 ? "" : "s"); return fails;
}
