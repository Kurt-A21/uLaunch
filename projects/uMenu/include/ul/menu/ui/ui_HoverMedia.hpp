
#pragma once
#include <pu/Plutonium>
#include <SDL2/SDL_mixer.h>
#include <switch.h>
#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

namespace ul::menu::ui {

    // SteamOS-style hover media (steamui fork):
    //  - full-screen per-game background art, crossfaded in with a slow zoom-out
    //  - per-game soundtrack preview, started once the cursor settles; the menu BGM is ducked meanwhile
    //
    // Both are optional and user-supplied on the SD card. KEY is the game's title ID (16 uppercase hex digits),
    // or for homebrew the NRO file name without ".nro" (e.g. "nfscx-vulkan"):
    //   sdmc:/ulaunch/steamui/bg/<KEY>.jpg  (or .png)  - 1280x720 recommended (1920x1080 works, uses more memory)
    //   sdmc:/ulaunch/steamui/sound/<KEY>.wav          - 44100 Hz WAV, keep it short (<= 30 s)
    //
    // Performance: decoding (JPEG/PNG and WAV) happens on a low-priority worker thread, so scrolling never
    // stalls on SD reads; only the texture upload happens on the render thread. Recently shown art is kept
    // in a small cache so moving back and forth between neighbours is instant.
    class HoverMedia : public pu::ui::elm::Element {
        public:
            static constexpr const char *BackgroundDirectory = "sdmc:/ulaunch/steamui/bg";
            static constexpr const char *SoundDirectory = "sdmc:/ulaunch/steamui/sound";

            // Waiting a bit avoids loading anything while scrolling quickly
            static constexpr s64 BackgroundLoadDelayMs = 120;
            static constexpr s64 SoundStartDelayMs = 600;

            static constexpr s32 AlphaStep = 20; // ~13 frames to fully fade in
            static constexpr double ZoomStart = 1.05;
            static constexpr double ZoomEaseFactor = 0.02;

            static constexpr s32 SoundFadeInMs = 700;
            static constexpr s32 SoundFadeOutMs = 300;
            static constexpr s32 DuckedMusicVolume = MIX_MAX_VOLUME / 6;

            static constexpr size_t BackgroundCacheSize = 4;
            static constexpr size_t WorkerStackSize = 0x20000;
            static constexpr int WorkerPriority = 0x30; // Below the UI thread

        private:
            struct FadingSound {
                s32 channel;
                Mix_Chunk *chunk;
            };

            struct CachedBackground {
                std::string key;
                pu::sdl2::TextureHandle::Ref tex; // null texture = "this game has no art"
            };

            // Worker thread state (guarded by worker_lock)
            Thread worker_thread;
            bool worker_running;
            Mutex worker_lock;
            CondVar worker_cv;
            bool worker_exit;
            std::string req_bg_key;     // empty = none
            std::string req_sound_key;  // empty = none
            bool res_bg_ready;
            std::string res_bg_key;
            SDL_Surface *res_bg_surface;
            bool res_sound_ready;
            std::string res_sound_key;
            Mix_Chunk *res_sound_chunk;

            // Requested target (empty = none)
            std::string target_key;
            std::chrono::steady_clock::time_point target_tp;
            bool bg_pending;
            bool bg_waiting;
            bool sound_pending;
            bool sound_waiting;

            std::vector<CachedBackground> bg_cache;

            // Background layers: "cur" fades in over "prev"
            pu::sdl2::TextureHandle::Ref cur_bg;
            s32 cur_bg_alpha;
            double cur_bg_zoom;
            pu::sdl2::TextureHandle::Ref prev_bg;
            s32 prev_bg_alpha;
            double prev_bg_zoom;

            // Sound preview
            Mix_Chunk *cur_sound;
            s32 cur_sound_channel;
            std::vector<FadingSound> fading_sounds;
            bool music_ducked;
            s32 saved_music_volume;

            static void WorkerMain(void *arg);
            void WorkerLoop();

            bool TryGetCachedBackground(const std::string &key, pu::sdl2::TextureHandle::Ref &out_tex);
            void CacheBackground(const std::string &key, pu::sdl2::TextureHandle::Ref tex);
            void ShowBackground(pu::sdl2::TextureHandle::Ref tex);
            void PollWorkerResults();
            void PlaySoundChunk(Mix_Chunk *chunk);
            void FadeOutCurrentSound(const s32 fade_ms);
            void UpdateSounds();
            void RenderLayer(pu::ui::render::Renderer::Ref &drawer, pu::sdl2::TextureHandle::Ref &tex, const s32 alpha, const double zoom);

        public:
            HoverMedia();
            PU_SMART_CTOR(HoverMedia)
            ~HoverMedia();

            inline s32 GetX() override {
                return 0;
            }

            inline s32 GetY() override {
                return 0;
            }

            inline s32 GetWidth() override {
                return pu::ui::render::ScreenWidth;
            }

            inline s32 GetHeight() override {
                return pu::ui::render::ScreenHeight;
            }

            // Call whenever the focused entry changes (empty = nothing to show: fade art out, stop sound)
            void SetTarget(const std::string &key);

            static std::string MakeApplicationKey(const u64 app_id);
            static std::string MakeHomebrewKey(const std::string &nro_path);

            // Immediately stops the preview sound (e.g. when launching a game or leaving the menu)
            void StopSound();

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override {}
    };

}
