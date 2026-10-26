// Menu preset -> sys-clk's real Config::SetProfiles (writes config.ini) -> fresh Config (as after reboot)
// -> GetProfiles -> the menu detects the same preset; sections written by oc_profile.py are left intact.
#include "config.h"
#include <ul/menu/ui/ui_PerformancePresets.hpp>
#include <cstdio>
#include <fstream>
#include <sstream>
using namespace ul::menu::ui::perf;
int main(int, char **argv){
    int fails = 0; const std::uint64_t T1 = 0x0100152000022000ULL, T2 = 0x01007EF00011E000ULL;
    {   Config c(argv[1]); c.SetEnabled(true); c.Refresh();
        for(auto pr: { Preset::Performance, Preset::Balanced }) {
            SysClkTitleProfileList l; FillProfiles(pr, l);
            if(!c.SetProfiles(pr == Preset::Performance ? 0x0100ABCD00000000ULL : 0x0100ABCD00001000ULL, &l, true)) { std::printf("  FAIL  SetProfiles\n"); fails++; }
        }
        SysClkTitleProfileList stock; FillProfiles(Preset::Stock, stock); c.SetProfiles(T1, &stock, true);
    }
    Config fresh(argv[1]); fresh.SetEnabled(true); fresh.Refresh();
    auto check = [&](std::uint64_t tid, int want, const char *what){
        SysClkTitleProfileList got; fresh.GetProfiles(tid, &got); int d = DetectPreset(got);
        std::printf("  %s  %s -> %s\n", d == want ? "ok  " : "FAIL", what, d < 0 ? "Custom" : GetPresetName((Preset)d)); fails += d != want; };
    check(0x0100ABCD00000000ULL, (int)Preset::Performance, "set Performance in the menu, read back after reboot");
    check(0x0100ABCD00001000ULL, (int)Preset::Balanced, "set Balanced in the menu, read back after reboot");
    check(T1, (int)Preset::Stock, "set Stock in the menu (removes the overclock)");
    check(T2, (int)Preset::Balanced, "profile written earlier by oc_profile.py still intact");
    std::printf("\n%s\n", fails ? "FAILED" : "ALL PASSED"); return fails;
}
