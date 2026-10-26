// "Press the same button three times" rule + the dynamic prompt's behaviour.
#include <ul/menu/ui/ui_UnlockPrompt.hpp>
#include <cstdio>
using namespace ul::menu::ui;
namespace pu::sdl2 { void DestroyTexture(Texture t){ delete t; } }
namespace pu::ui::render { static int g_id = 2000; sdl2::Texture RenderText(const std::string &, const std::string &, const Color, const u32, const u32){ return new sdl2::TextureObj{ g_id++ }; }
    s32 GetTextureWidth(sdl2::Texture){ return 200; } s32 GetTextureHeight(sdl2::Texture){ return 40; } sdl2::Texture ConvertToTexture(SDL_Surface *){ return nullptr; } }
static int fails = 0;
#define CHECK(c, ...) do { if(c) std::printf("  ok    "); else { std::printf("  FAIL  "); fails++; } std::printf(__VA_ARGS__); std::printf("\n"); } while(0)
using R = PressCounter::Result;
int main(){
    std::printf("== Unlock rule ==\n");
    PressCounter pc(3);
    CHECK(pc.Feed(PressCounter::PickButton(0)) == R::Ignored, "nothing pressed: ignored");
    CHECK(PressCounter::PickButton(HidNpadButton_StickLLeft | HidNpadButton_StickRRight) == 0, "stick tilts don't count as presses");
    CHECK(PressCounter::PickButton(HidNpadButton_A | HidNpadButton_ZR) == HidNpadButton_A, "two buttons in one frame count as one press");
    CHECK(pc.Feed(HidNpadButton_A) == R::Counted && pc.Feed(HidNpadButton_A) == R::Counted, "A, A: counting");
    CHECK(pc.Feed(HidNpadButton_A) == R::Completed, "A x3 unlocks");
    pc.Reset();
    pc.Feed(HidNpadButton_A); pc.Feed(HidNpadButton_A);
    CHECK(pc.Feed(HidNpadButton_B) == R::Restarted && pc.GetCount() == 1 && pc.GetKey() == HidNpadButton_B, "A, A, B: restarts at 1 with B");
    CHECK(pc.Feed(HidNpadButton_B) == R::Counted && pc.Feed(HidNpadButton_B) == R::Completed, "then B x2 more unlocks");
    pc.Reset();
    pc.Feed(PressCounter::TouchKey); pc.Feed(PressCounter::TouchKey);
    CHECK(pc.Feed(PressCounter::TouchKey) == R::Completed, "three taps on the screen unlock too");
    CHECK(std::string(PressCounter::GetButtonLabel(HidNpadButton_ZR)) == "ZR" && std::string(PressCounter::GetButtonLabel(PressCounter::TouchKey)) == "Touch", "badge labels");

    std::printf("== Prompt ==\n");
    std::vector<R> presses; int done = 0;
    auto p = UnlockPrompt::New(470, 3, pu::ui::Color{ 232, 237, 242, 255 }, pu::ui::Color{ 26, 159, 255, 255 }, [&](R r){ presses.push_back(r); }, [&](){ done++; });
    auto rd = pu::ui::render::Renderer::New();
    auto frames = [&](int n){ for(int i = 0; i < n; i++){ rd->calls.clear(); p->OnRender(rd, 0, 0); } };
    auto filled_dots = [&](){ int n = 0; for(auto &c: rd->calls) if(c.tex_id == -5 && c.alpha == 255 && c.w >= 17 && c.w <= 19) n++; return n; };
    pu::ui::TouchPoint none, tap{ 900, 500 };
    p->Reset("Skyforge", pu::sdl2::TextureHandle::New(new pu::sdl2::TextureObj{ 5 }));
    frames(30);
    bool icon = false; for(auto &c: rd->calls) if(c.tex_id == 5) icon = true;
    CHECK(filled_dots() == 0 && icon, "idle: no dots filled, paused game icon shown at the bottom");
    p->OnInput(HidNpadButton_X, 0, 0, none); frames(1); p->OnInput(HidNpadButton_X, 0, 0, none); frames(40);
    CHECK(filled_dots() == 2 && done == 0, "X, X: two dots filled, still locked");
    p->OnInput(HidNpadButton_Y, 0, 0, none); frames(40);
    CHECK(filled_dots() == 1 && presses.back() == R::Restarted, "Y: shakes and restarts at one dot");
    p->OnInput(0, 0, 0, tap); p->OnInput(0, 0, 0, tap); p->OnInput(0, 0, 0, tap);
    CHECK(p->GetCounter().GetCount() == 1 && p->GetCounter().GetKey() == PressCounter::TouchKey, "holding a finger counts as one tap (and restarts from Y)");
    p->OnInput(0, 0, 0, none); p->OnInput(0, 0, 0, tap); p->OnInput(0, 0, 0, none); p->OnInput(0, 0, 0, tap);
    CHECK(done == 1 && p->IsCompleted(), "three taps: unlocked once");
    int rings = 0; frames(6); for(auto &c: rd->calls) if(c.tex_id == -4 && c.w > 60) rings++;
    CHECK(rings > 0, "unlock burst ring drawn");
    p->OnInput(HidNpadButton_A, 0, 0, none); p->OnInput(HidNpadButton_A, 0, 0, none); p->OnInput(HidNpadButton_A, 0, 0, none);
    CHECK(done == 1, "presses after unlocking are ignored (no double resume)");
    p->Reset("", nullptr); frames(20);
    icon = false; for(auto &c: rd->calls) if(c.tex_id == 5) icon = true;
    CHECK(!p->IsCompleted() && p->GetCounter().GetCount() == 0 && !icon, "next sleep: fresh prompt; no game -> no paused line");
    std::printf("\n%s (%d failure%s)\n", fails ? "FAILED" : "ALL PASSED", fails, fails == 1 ? "" : "s"); return fails;
}
