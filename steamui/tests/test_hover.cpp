#include <ul/menu/ui/ui_HoverMedia.hpp>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
#include <sys/stat.h>
#include <fstream>
using namespace ul::menu::ui;
extern std::atomic<int> g_SurfAlloc, g_SurfFree, g_TexAlloc, g_TexFree, g_ChunkAlloc, g_ChunkFree, g_Errors, g_ImgLoads, g_WavLoads, g_LoadDelayMs;
extern int g_ThreadCreateFail;
extern std::vector<std::string> g_LoadLog; extern std::mutex g_LoadLog_lock;
int SimMusicVolume(); int SimPlayingChunkId(); int SimIdForPath(const std::string &p);
static int fails = 0;
#define CHECK(cond, msg) do { if(cond) std::printf("  ok   %s\n", msg); else { std::printf("  FAIL %s\n", msg); fails++; } } while(0)

static pu::ui::render::Renderer::Ref R = pu::ui::render::Renderer::New();
static void frames(HoverMedia::Ref &m, int ms){ auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    while(std::chrono::steady_clock::now() < end){ R->calls.clear(); m->OnRender(R, 0, 0); std::this_thread::sleep_for(std::chrono::milliseconds(16)); } }
static int top_id(){ return R->calls.empty() ? 0 : R->calls.back().tex_id; }
static int top_alpha(){ return R->calls.empty() ? 0 : R->calls.back().alpha; }
static int bg_id(const char *key){ return SimIdForPath(std::string("sdmc:/ulaunch/steamui/bg/") + key + ".jpg"); }
static int loads_of(const char *key){ std::lock_guard l(g_LoadLog_lock); int n = 0; for(auto &s: g_LoadLog) if(s.find(std::string("/") + key + ".") != std::string::npos) n++; return n; }
static void touch(const std::string &p){ std::ofstream(p) << "x"; }

static void scenario(bool threaded){
    std::printf("\n== HoverMedia (%s) ==\n", threaded ? "worker thread" : "inline fallback");
    g_ThreadCreateFail = threaded ? 0 : 1;
    { std::lock_guard l(g_LoadLog_lock); g_LoadLog.clear(); }
    auto m = HoverMedia::New();
    const int vol0 = SimMusicVolume();

    m->SetTarget("GAMEA"); frames(m, 60);
    CHECK(R->calls.empty(), "no art drawn before the load delay");
    frames(m, 500);
    CHECK(top_id() == bg_id("GAMEA") && top_alpha() == 255, "game A art fully faded in");
    CHECK(R->calls.back().w > 1920 || R->calls.back().w == 1920, "art covers the screen (zoom >= 1)");
    frames(m, 400);
    CHECK(SimPlayingChunkId() != 0, "game A soundtrack playing after settling");
    CHECK(SimMusicVolume() < vol0, "menu music ducked during preview");

    // Fast scrolling: B, C quickly, then stop on D
    m->SetTarget("GAMEB"); frames(m, 40); m->SetTarget("GAMEC"); frames(m, 40); m->SetTarget("GAMED");
    frames(m, 350);
    CHECK(SimPlayingChunkId() == 0, "A's sound fades out as soon as the cursor moves");
    frames(m, 900);
    CHECK(loads_of("GAMEB") == 0 && loads_of("GAMEC") == 0, "skipped games B/C were never loaded");
    CHECK(top_id() == bg_id("GAMED") && top_alpha() == 255, "game D art shown");
    CHECK(SimPlayingChunkId() != 0, "game D soundtrack playing");

    // Game without art/sound: art fades away, music restored
    m->SetTarget("GAMEE"); frames(m, 900);
    CHECK(R->calls.empty(), "no art for game E: background fades out");
    CHECK(SimPlayingChunkId() == 0 && SimMusicVolume() == vol0, "sound stopped and menu music restored");

    // Back to A: cache hit
    const int a_loads = loads_of("GAMEA");
    m->SetTarget("GAMEA"); frames(m, 400);
    CHECK(loads_of("GAMEA") == a_loads, "returning to A uses the cache (no SD read)");
    CHECK(top_id() == bg_id("GAMEA"), "A art shown again");

    // Stress: random hopping, including during loads
    const char *keys[] = { "GAMEA", "GAMEB", "GAMEC", "GAMED", "GAMEE", "GAMEF", "", "hb-app" };
    for(int i = 0; i < 300; i++){ m->SetTarget(keys[(i * 7 + i / 3) % 8]); frames(m, (i % 5) * 9); }
    frames(m, 1200);
    m->SetTarget("GAMEF"); frames(m, 300); m->StopSound();
    CHECK(SimMusicVolume() == vol0, "StopSound restores music volume");
    m.reset();
    R->calls.clear();
}

int main(){
    mkdir("sdmc:", 0777); mkdir("sdmc:/ulaunch", 0777); mkdir("sdmc:/ulaunch/steamui", 0777);
    mkdir("sdmc:/ulaunch/steamui/bg", 0777); mkdir("sdmc:/ulaunch/steamui/sound", 0777);
    for(auto k: { "GAMEA", "GAMEB", "GAMEC", "GAMED", "GAMEF", "hb-app" }) touch(std::string("sdmc:/ulaunch/steamui/bg/") + k + ".jpg");
    for(auto k: { "GAMEA", "GAMED", "GAMEF", "hb-app" }) touch(std::string("sdmc:/ulaunch/steamui/sound/") + k + ".wav");
    scenario(true);
    scenario(false);
    std::printf("\n== Resources ==\n");
    CHECK(g_SurfAlloc == g_SurfFree, ("image surfaces freed: " + std::to_string(g_SurfAlloc) + "/" + std::to_string(g_SurfFree)).c_str());
    CHECK(g_TexAlloc == g_TexFree, ("textures destroyed: " + std::to_string(g_TexAlloc) + "/" + std::to_string(g_TexFree)).c_str());
    CHECK(g_ChunkAlloc == g_ChunkFree, ("sound chunks freed: " + std::to_string(g_ChunkAlloc) + "/" + std::to_string(g_ChunkFree)).c_str());
    CHECK(g_Errors == 0, "no audio misuse (playing/freeing freed or still-playing chunks)");
    CHECK(HoverMedia::MakeApplicationKey(0x0100ABCD12340000ULL) == "0100ABCD12340000", "title ID key format");
    CHECK(HoverMedia::MakeHomebrewKey("sdmc:/switch/nfscx/nfscx-vulkan.nro") == "nfscx-vulkan", "homebrew key from NRO path");
    std::printf("\n%s (%d failure%s)\n", fails ? "FAILED" : "ALL PASSED", fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
