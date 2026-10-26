#include <ul/menu/ui/ui_UserCarousel.hpp>
#include <cstdio>
#include <cmath>
using namespace ul::menu::ui;
namespace pu::sdl2 { void DestroyTexture(Texture t){ delete t; } }
namespace pu::ui::render { static int g_id = 1000; sdl2::Texture RenderText(const std::string &, const std::string &, const Color, const u32, const u32){ return new sdl2::TextureObj{ g_id++ }; }
    s32 GetTextureWidth(sdl2::Texture){ return 120; } sdl2::Texture ConvertToTexture(SDL_Surface *){ return nullptr; } }
static int fails = 0;
#define CHECK(c, ...) do { if(c) std::printf("  ok    "); else { std::printf("  FAIL  "); fails++; } std::printf(__VA_ARGS__); std::printf("\n"); } while(0)
int main(){
    int selected = -1, moves = 0;
    auto ring = pu::sdl2::TextureHandle::New(new pu::sdl2::TextureObj{ 7 });
    auto c = UserCarousel::New(520, 256, 340, ring, pu::ui::Color{ 232, 237, 242, 255 }, [&](){ moves++; });
    for(int i = 0; i < 3; i++) c->AddUser(pu::sdl2::TextureHandle::New(new pu::sdl2::TextureObj{ 10 + i }), "User" + std::to_string(i), [&, i](){ selected = i; });
    c->AddCreateUser("Add user", [&](){ selected = 99; });
    auto R = pu::ui::render::Renderer::New();
    auto frames = [&](int n){ for(int i = 0; i < n; i++){ R->calls.clear(); c->OnRender(R, 0, 0); } };
    pu::ui::TouchPoint none;
    frames(30);
    // focused avatar is centred and drawn biggest + last (on top), with the ring
    int focus_w = 0, other_w = 0; bool ring_drawn = false;
    for(auto &d: R->calls){ if(d.tex_id == 10) focus_w = d.w; if(d.tex_id == 11) other_w = d.w; if(d.tex_id == 7) ring_drawn = true; }
    CHECK(std::abs(focus_w - (int)(256 * 1.18)) <= 3 && other_w == 256, "focused avatar springs to 118%% (%d px), others stay 256 px", focus_w);
    CHECK(ring_drawn, "glow ring drawn around the focused avatar");
    c->OnInput(HidNpadButton_Left, 0, 0, none);
    CHECK(c->GetFocusedIndex() == 0 && moves == 0, "can't move left past the first profile");
    c->OnInput(HidNpadButton_Right, 0, 0, none); c->OnInput(HidNpadButton_Right, 0, 0, none); c->OnInput(HidNpadButton_Right, 0, 0, none); c->OnInput(HidNpadButton_Right, 0, 0, none);
    CHECK(c->GetFocusedIndex() == 3 && moves == 3, "moves right to the Add-user tile and stops there (3 moves)");
    frames(40);
    bool tile = false; for(auto &d: R->calls) if(d.tex_id == -2 && std::abs(d.x + d.w / 2 - 960) <= 4) tile = true;
    CHECK(tile, "row glided so the Add-user tile is centred");
    c->OnInput(HidNpadButton_A, 0, 0, none);
    CHECK(selected == 99, "A on Add-user opens account creation");
    // tap on profile 2 (left of centre): first tap focuses, second tap selects; held finger doesn't repeat
    pu::ui::TouchPoint tap{ 960 - 340, 520 };
    c->OnInput(0, 0, 0, tap); c->OnInput(0, 0, 0, tap); c->OnInput(0, 0, 0, tap);
    CHECK(c->GetFocusedIndex() == 2 && selected == 99, "tap focuses profile; holding the finger doesn't select");
    frames(40);
    c->OnInput(0, 0, 0, none); c->OnInput(0, 0, 0, pu::ui::TouchPoint{ 960, 520 });
    CHECK(selected == 2, "second tap on the focused profile selects it");
    c->Clear(); R->calls.clear(); c->OnRender(R, 0, 0); c->OnInput(HidNpadButton_A | HidNpadButton_Right, 0, 0, none);
    CHECK(R->calls.empty(), "empty carousel draws nothing and ignores input");
    std::printf("\n%s (%d failure%s)\n", fails ? "FAILED" : "ALL PASSED", fails, fails == 1 ? "" : "s"); return fails;
}
