
#pragma once
#include <pu/Plutonium>
#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <chrono>
#include <vector>

namespace ul::menu::ui {

    // SteamOS-style hover media (steamui fork):
    //  - full-screen per-game background art, crossfaded in with a slow zoom-out
    //  - per-game audio preview, started once the cursor settles; the menu BGM is ducked meanwhile
    //
    // Both are optional and user-supplied on the SD card (APPID = 16 uppercase hex digits):
    //   sdmc:/ulaunch/steamui/bg/<APPID>.jpg  (or .png)  - 1920x1080 recommended
    //   sdmc:/ulaunch/steamui/sound/<APPID>.wav          - 44100 Hz WAV, keep it short (<= 30 s)
    class HoverMedia : public pu::ui::elm::Element {
        public:
            static constexpr const char *BackgroundDirectory = "sdmc:/ulaunch/steamui/bg";
            static constexpr const char *SoundDirectory = "sdmc:/ulaunch/steamui/sound";

            // Waiting a bit avoids decoding images/sounds while scrolling quickly
            static constexpr s64 BackgroundLoadDelayMs = 180;
            static constexpr s64 SoundStartDelayMs = 650;

            static constexpr s32 AlphaStep = 16; // ~16 frames to fully fade in
            static constexpr double ZoomStart = 1.06;
            static constexpr double ZoomEaseFactor = 0.02;

            static constexpr s32 SoundFadeInMs = 600;
            static constexpr s32 SoundFadeOutMs = 300;
            static constexpr s32 DuckedMusicVolume = MIX_MAX_VOLUME / 5;

        private:
            struct FadingSound {
                s32 channel;
                Mix_Chunk *chunk;
            };

            // Requested target (0 = none)
            u64 target_app_id;
            std::chrono::steady_clock::time_point target_tp;
            bool bg_pending;
            bool sound_pending;

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

            void LoadBackground(const u64 app_id);
            void StartSound(const u64 app_id);
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

            // Call whenever the focused entry changes (0 = not a game: fade art out, stop sound)
            void SetTarget(const u64 app_id);

            // Immediately stops the preview sound (e.g. when launching a game or leaving the menu)
            void StopSound();

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override {}
    };

}
