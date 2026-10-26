// Feeds the config.ini written by oc_profile.py into sys-clk's real Config + ClockManager code and
// records the clocks sys-clk would set in every power state, on both Switch chip types.
#define private public
#define protected public
#include "clock_manager.h"
#undef private
#undef protected
#include <cstdio>
#include <cstdlib>
#include <string>
extern SysClkProfile g_Profile; extern SysClkSocType g_Soc; extern std::uint64_t g_Tid; extern std::uint32_t g_Hz[]; extern int g_SetCalls; extern bool g_Verbose;
static int fails = 0;
#define CHECK(c, ...) do { if(c) { std::printf("  ok    "); } else { std::printf("  FAIL  "); fails++; } std::printf(__VA_ARGS__); std::printf("\n"); } while(0)
static const char *pn(SysClkProfile p){ return sysclkFormatProfile(p, true); }
struct Clocks { unsigned cpu, gpu, mem; };
static Clocks run(ClockManager &cm, std::uint64_t tid, SysClkProfile p, SysClkSocType soc){
    g_Tid = tid; g_Profile = p; g_Soc = soc; cm.Tick(); cm.Tick();
    return { g_Hz[SysClkModule_CPU] / 1000000, g_Hz[SysClkModule_GPU] / 100000, g_Hz[SysClkModule_MEM] / 100000 };
}
int main(int argc, char **argv){
    const std::uint64_t GAME = 0x0100152000022000ULL, BAL = 0x01007EF00011E000ULL, BAT = 0x0100BA0003EEA000ULL, NOPROF = 0x0100000000010000ULL;
    ClockManager cm;
    delete cm.config; cm.config = new Config(argv[1]); cm.config->SetEnabled(true); cm.config->Refresh();
    cm.context->enabled = true; g_Verbose = getenv("V") != nullptr;
    cm.Tick();
    CHECK(cm.config->HasProfilesLoaded(), "sys-clk loaded the config file");
    CHECK(cm.config->GetProfileCount(GAME) == 12, "performance profile: %u clock entries parsed (12 expected)", cm.config->GetProfileCount(GAME));
    CHECK(cm.config->GetProfileCount(BAL) == 9, "balanced profile: %u entries (9 expected)", cm.config->GetProfileCount(BAL));
    CHECK(cm.config->GetProfileCount(BAT) == 3, "battery profile: %u entries (3 expected)", cm.config->GetProfileCount(BAT));

    const SysClkProfile profs[] = { SysClkProfile_Handheld, SysClkProfile_HandheldChargingUSB, SysClkProfile_HandheldChargingOfficial, SysClkProfile_Docked };
    std::printf("\n  Clocks sys-clk applies (CPU MHz / GPU MHz / RAM MHz):\n");
    std::printf("  %-12s %-8s %-32s %s\n", "preset", "chip", "power state", "CPU / GPU / RAM");
    for(auto soc: { SysClkSocType_Erista, SysClkSocType_Mariko }) {
        const char *chip = soc == SysClkSocType_Erista ? "Erista" : "Mariko";
        for(auto p: profs) {
            auto c = run(cm, GAME, p, soc);
            std::printf("  %-12s %-8s %-32s %u / %u.%u / %u.%u\n", "performance", chip, pn(p), c.cpu, c.gpu / 10, c.gpu % 10, c.mem / 10, c.mem % 10);
            const unsigned gpu_cap = p == SysClkProfile_Handheld ? (soc == SysClkSocType_Mariko ? 6144 : 4608) : p == SysClkProfile_HandheldChargingUSB ? 7680 : 9216;
            CHECK(c.cpu == 1785 && c.mem == 16000, "performance: CPU 1785 + RAM 1600 in %s (%s)", pn(p), chip);
            CHECK(c.gpu == gpu_cap, "performance: GPU %u.%u = sys-clk's safe cap for %s on %s", gpu_cap / 10, gpu_cap % 10, pn(p), chip);
        }
    }
    auto b = run(cm, BAL, SysClkProfile_Docked, SysClkSocType_Erista);
    CHECK(b.cpu == 1428 && b.gpu == 8448 && b.mem == 16000, "balanced docked: %u / %u.%u / %u", b.cpu, b.gpu / 10, b.gpu % 10, b.mem / 10);
    b = run(cm, BAL, SysClkProfile_Handheld, SysClkSocType_Erista);
    CHECK(b.cpu == 1224 && b.gpu == 4608, "balanced handheld: %u / %u.%u", b.cpu, b.gpu / 10, b.gpu % 10);
    auto t = run(cm, BAT, SysClkProfile_Handheld, SysClkSocType_Mariko);
    CHECK(t.cpu == 816 && t.gpu == 3072 && t.mem == 10656, "battery handheld underclock: %u / %u.%u / %u.%u", t.cpu, t.gpu / 10, t.gpu % 10, t.mem / 10, t.mem % 10);

    // Leaving a game: sys-clk must go back to stock for titles without a profile (home menu, others)
    run(cm, GAME, SysClkProfile_Docked, SysClkSocType_Erista);
    auto h = run(cm, 0x0100000000001000ULL, SysClkProfile_Docked, SysClkSocType_Erista);
    CHECK(h.cpu == 1020 && h.gpu == 7680, "back on the home menu: stock docked clocks %u / %u.%u restored", h.cpu, h.gpu / 10, h.gpu % 10);
    run(cm, GAME, SysClkProfile_Handheld, SysClkSocType_Erista);
    auto n = run(cm, NOPROF, SysClkProfile_Handheld, SysClkSocType_Erista);
    CHECK(n.cpu == 1020 && n.gpu == 3840, "game without a profile runs at stock handheld clocks %u / %u.%u", n.cpu, n.gpu / 10, n.gpu % 10);
    // Docking mid-game switches profile on the next tick
    run(cm, GAME, SysClkProfile_Handheld, SysClkSocType_Mariko);
    auto d = run(cm, GAME, SysClkProfile_Docked, SysClkSocType_Mariko);
    CHECK(d.gpu == 9216, "docking mid-game raises GPU to %u.%u on the next tick", d.gpu / 10, d.gpu % 10);
    std::printf("\n%s (%d failure%s)\n", fails ? "FAILED" : "ALL PASSED", fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
