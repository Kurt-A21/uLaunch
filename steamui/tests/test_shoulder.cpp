// Burger menu on ZL/ZR: a clean tap opens it; Ultrahand's ZL + ZR + D-pad Down combo doesn't.
#include <ul/menu/ui/ui_ShoulderTap.hpp>
#include <cstdio>
#include <vector>
using namespace ul::menu::ui;
static int fails = 0;
#define CHECK(c, ...) do { if(c) std::printf("  ok    "); else { std::printf("  FAIL  "); fails++; } std::printf(__VA_ARGS__); std::printf("\n"); } while(0)
// Plays frames of "buttons held"; returns how many times the menu would toggle
static int Play(const std::vector<u64> &frames) {
    ShoulderTap t; u64 prev = 0; int n = 0;
    for(auto held: frames) { n += t.Feed(held & ~prev, prev & ~held, held); prev = held; }
    return n;
}
int main() {
    const u64 ZL = HidNpadButton_ZL, ZR = HidNpadButton_ZR, DD = HidNpadButton_Down, A = HidNpadButton_A, SL = HidNpadButton_StickLLeft;
    CHECK(Play({ZL, ZL, 0}) == 1, "tap ZL: opens (on release)");
    CHECK(Play({ZR, 0}) == 1, "tap ZR: opens");
    CHECK(Play({ZL, 0, ZL, 0}) == 2, "two taps: open, then close");
    CHECK(Play({ZL, ZL|ZR, ZL|ZR|DD, ZL|ZR, 0}) == 0, "Ultrahand combo ZL, +ZR, +Down: menu stays shut");
    CHECK(Play({ZL|ZR|DD, 0}) == 0, "the whole combo in one frame: stays shut");
    CHECK(Play({ZR, ZR|ZL, ZR|ZL|DD, 0}) == 0, "combo starting with ZR: stays shut");
    CHECK(Play({ZL|ZR, 0}) == 0, "ZL + ZR together: stays shut");
    CHECK(Play({DD, DD|ZL, DD, 0}) == 0, "ZL while D-pad is held: stays shut");
    CHECK(Play({ZL, ZL|A, ZL, 0}) == 0, "another button during the tap: stays shut");
    CHECK(Play({ZL, ZL|SL, ZL, 0}) == 1, "stick tilt during the tap still counts");
    CHECK(Play({ZL, ZL|ZR, ZR, 0, ZR, 0}) == 1, "after a chord, the next clean tap works again");
    std::printf(fails ? "FAILED: %d\n" : "ALL PASSED\n", fails);
    return fails ? 1 : 0;
}
