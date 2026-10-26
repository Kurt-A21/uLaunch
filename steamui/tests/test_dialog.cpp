// Console-style dialog: selection, scrolling and held-direction repeat (DialogNav / HoldRepeat).
#include <ul/menu/ui/ui_DialogNav.hpp>
#include <cstdio>
using namespace ul::menu::ui;
static int fails = 0;
#define CHECK(c, ...) do { if(c) std::printf("  ok    "); else { std::printf("  FAIL  "); fails++; } std::printf(__VA_ARGS__); std::printf("\n"); } while(0)
int main(){
    std::printf("== Selection ==\n");
    DialogNav n; n.Reset(5, 8, 0);
    CHECK(n.GetSelected() == 0 && !n.IsScrollable() && n.GetVisible() == 5, "5 options, all visible, first selected");
    CHECK(!n.Move(-1) && n.GetSelected() == 0, "up on the first option stays (no wrap)");
    CHECK(n.Move(1) && n.Move(1) && n.GetSelected() == 2, "down twice selects the third");
    n.Move(10); CHECK(n.GetSelected() == 4 && !n.Move(1), "can't go past the last (Cancel)");
    CHECK(n.Select(1) && n.GetSelected() == 1 && !n.Select(1) && !n.Select(9), "tap selects; same/out of range ignored");
    n.Reset(3, 8, 7); CHECK(n.GetSelected() == 2, "initial index clamped");
    n.Reset(0, 4); CHECK(!n.Move(1) && n.GetSelected() == 0, "no options: nothing moves");

    std::printf("== Scrolling (more options than fit) ==\n");
    n.Reset(9, 4, 0);
    CHECK(n.IsScrollable() && n.GetVisible() == 4 && n.GetFirstVisible() == 0, "9 options, 4 rows shown");
    for(int i = 0; i < 3; i++) n.Move(1);
    CHECK(n.GetFirstVisible() == 0, "moving within the window doesn't scroll");
    n.Move(1); CHECK(n.GetSelected() == 4 && n.GetFirstVisible() == 1, "stepping past the bottom scrolls by one");
    n.Move(10); CHECK(n.GetSelected() == 8 && n.GetFirstVisible() == 5, "last option: window ends at the list end");
    n.Move(-3); CHECK(n.GetFirstVisible() == 5, "moving up inside the window doesn't scroll");
    n.Move(-1); CHECK(n.GetSelected() == 4 && n.GetFirstVisible() == 4, "stepping past the top scrolls up");
    bool always_visible = true; n.Reset(9, 4, 0);
    for(int s = 0; s < 40; s++){ n.Move((s * 7 % 5) - 2); if(n.GetSelected() < n.GetFirstVisible() || n.GetSelected() >= n.GetFirstVisible() + 4) always_visible = false; }
    CHECK(always_visible, "selection always inside the visible window (random walk)");

    std::printf("== Held direction repeat ==\n");
    HoldRepeat r; int fired = 0;
    CHECK(r.Feed(1, 1, 0) == 1, "press fires immediately");
    for(int t = 16; t < HoldRepeat::FirstDelayMs; t += 16) if(r.Feed(0, 1, t)) fired++;
    CHECK(fired == 0, "nothing more until the first delay (%lld ms)", (long long)HoldRepeat::FirstDelayMs);
    fired = 0; for(int t = HoldRepeat::FirstDelayMs; t < HoldRepeat::FirstDelayMs + 1000; t += 16) if(r.Feed(0, 1, t) == 1) fired++;
    CHECK(fired >= 10 && fired <= 13, "then about every %lld ms while held (%d in 1 s)", (long long)HoldRepeat::RepeatMs, fired);
    CHECK(r.Feed(0, 0, 5000) == 0 && r.Feed(0, 1, 5016) == 0, "release stops it; holding again without a new press doesn't fire");
    CHECK(r.Feed(-1, -1, 6000) == -1 && r.Feed(0, 1, 6400) == 0, "switching direction mid-hold needs a new press");
    std::printf(fails ? "FAILED: %d\n" : "ALL PASSED\n", fails);
    return fails ? 1 : 0;
}
