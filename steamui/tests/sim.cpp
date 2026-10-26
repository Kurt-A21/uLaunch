// Simulated SDL_mixer / SDL_image / texture backends with leak + misuse detection
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <pu/Plutonium>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <set>
#include <string>
#include <sys/stat.h>
#include <thread>

int g_ThreadCreateFail = 0;
std::atomic<int> g_SurfAlloc{0}, g_SurfFree{0}, g_TexAlloc{0}, g_TexFree{0}, g_ChunkAlloc{0}, g_ChunkFree{0}, g_Errors{0};
std::atomic<int> g_ImgLoads{0}, g_WavLoads{0};
std::mutex g_LoadLog_lock; std::vector<std::string> g_LoadLog;
std::atomic<int> g_LoadDelayMs{60};  // simulated SD decode time
static int g_MusicVol = 100;
using clk = std::chrono::steady_clock;
struct Chan { Mix_Chunk *c = nullptr; clk::time_point end; bool active = false; };
static Chan g_Ch[8];
static std::set<Mix_Chunk*> g_LiveChunks;
static std::mutex g_MixLock;

void ERR(const char *m){ std::printf("  !! ERROR: %s\n", m); g_Errors++; }
static bool exists(const char *p){ struct stat st; return stat(p, &st) == 0; }

#include <map>
static std::mutex g_IdLock; static std::map<std::string,int> g_Ids;
int SimIdForPath(const std::string &p){ std::lock_guard l(g_IdLock); auto it = g_Ids.find(p); if(it != g_Ids.end()) return it->second; int id = 100 + (int)g_Ids.size(); g_Ids[p] = id; return id; }
SDL_Surface *IMG_Load(const char *path){
    std::this_thread::sleep_for(std::chrono::milliseconds(g_LoadDelayMs));
    g_ImgLoads++; { std::lock_guard l(g_LoadLog_lock); g_LoadLog.push_back(std::string("img:") + path); }
    if(!exists(path)) return nullptr;
    g_SurfAlloc++; return new SDL_Surface{ SimIdForPath(path) };
}
void SDL_FreeSurface(SDL_Surface *s){ if(s){ g_SurfFree++; delete s; } }
namespace pu::sdl2 { void DestroyTexture(Texture t){ g_TexFree++; delete t; } }
namespace pu::ui::render { sdl2::Texture ConvertToTexture(SDL_Surface *s){ if(!s) return nullptr; auto t = new sdl2::TextureObj{ s->id }; g_TexAlloc++; SDL_FreeSurface(s); return t; } }

Mix_Chunk *Mix_LoadWAV(const char *path){
    std::this_thread::sleep_for(std::chrono::milliseconds(g_LoadDelayMs));
    g_WavLoads++; { std::lock_guard l(g_LoadLog_lock); g_LoadLog.push_back(std::string("wav:") + path); }
    if(!exists(path)) return nullptr;
    static std::atomic<int> id{1}; g_ChunkAlloc++; auto c = new Mix_Chunk{ id++, 3000 };
    std::lock_guard l(g_MixLock); g_LiveChunks.insert(c); return c;
}
static void upd(){ auto now = clk::now(); for(auto &ch: g_Ch) if(ch.active && now >= ch.end) ch.active = false; }
int Mix_FadeInChannel(int, Mix_Chunk *c, int, int){ std::lock_guard l(g_MixLock); upd(); if(!g_LiveChunks.count(c)) ERR("playing freed chunk");
    for(int i = 0; i < 8; i++) if(!g_Ch[i].active){ g_Ch[i] = { c, clk::now() + std::chrono::milliseconds(c->len_ms), true }; return i; } return -1; }
int Mix_FadeOutChannel(int ch, int ms){ std::lock_guard l(g_MixLock); upd(); if(ch >= 0 && g_Ch[ch].active){ auto e = clk::now() + std::chrono::milliseconds(ms); if(e < g_Ch[ch].end) g_Ch[ch].end = e; } return 1; }
int Mix_HaltChannel(int ch){ std::lock_guard l(g_MixLock); if(ch >= 0) g_Ch[ch].active = false; return 0; }
int Mix_Playing(int ch){ std::lock_guard l(g_MixLock); upd(); return (ch >= 0 && ch < 8 && g_Ch[ch].active) ? 1 : 0; }
Mix_Chunk *Mix_GetChunk(int ch){ std::lock_guard l(g_MixLock); return (ch >= 0 && ch < 8) ? g_Ch[ch].c : nullptr; }
void Mix_FreeChunk(Mix_Chunk *c){ if(!c) return; std::lock_guard l(g_MixLock); upd();
    for(auto &ch: g_Ch) if(ch.active && ch.c == c) ERR("freeing a chunk that is still playing");
    if(!g_LiveChunks.erase(c)) ERR("double free of chunk"); g_ChunkFree++; delete c; }
int Mix_VolumeMusic(int v){ int old = g_MusicVol; if(v >= 0) g_MusicVol = v; return old; }
const char *Mix_GetError(){ return "sim"; }
int SimMusicVolume(){ return g_MusicVol; }
int SimPlayingChunkId(){ std::lock_guard l(g_MixLock); upd(); for(auto &ch: g_Ch) if(ch.active) return ch.c->id; return 0; }
