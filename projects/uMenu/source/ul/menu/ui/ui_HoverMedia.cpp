#include <ul/menu/ui/ui_HoverMedia.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/ul_Result.hpp>
#include <cstdio>

namespace ul::menu::ui {

    namespace {

        std::string MakeMediaPath(const char *dir, const u64 app_id, const char *ext) {
            char path[FS_MAX_PATH] = {};
            std::snprintf(path, sizeof(path), "%s/%016llX.%s", dir, static_cast<unsigned long long>(app_id), ext);
            return path;
        }

    }

    HoverMedia::HoverMedia() : Element(), target_app_id(0), target_tp(), bg_pending(false), sound_pending(false), cur_bg(), cur_bg_alpha(0), cur_bg_zoom(1.0), prev_bg(), prev_bg_alpha(0), prev_bg_zoom(1.0), cur_sound(nullptr), cur_sound_channel(-1), fading_sounds(), music_ducked(false), saved_music_volume(MIX_MAX_VOLUME) {}

    HoverMedia::~HoverMedia() {
        this->StopSound();
        for(auto &snd: this->fading_sounds) {
            Mix_HaltChannel(snd.channel);
            Mix_FreeChunk(snd.chunk);
        }
        this->fading_sounds.clear();
        this->cur_bg = {};
        this->prev_bg = {};
    }

    void HoverMedia::SetTarget(const u64 app_id) {
        if(app_id == this->target_app_id) {
            return;
        }

        this->target_app_id = app_id;
        this->target_tp = std::chrono::steady_clock::now();
        this->bg_pending = true;
        this->sound_pending = app_id != 0;

        // The previous game's sound always fades out right away
        this->FadeOutCurrentSound(SoundFadeOutMs);
    }

    void HoverMedia::LoadBackground(const u64 app_id) {
        pu::sdl2::TextureHandle::Ref new_bg = {};
        if(app_id != 0) {
            for(const auto ext: { "jpg", "png" }) {
                const auto path = MakeMediaPath(BackgroundDirectory, app_id, ext);
                if(fs::ExistsFile(path)) {
                    auto tex = pu::ui::render::LoadImageFromFile(path);
                    if(tex != nullptr) {
                        new_bg = pu::sdl2::TextureHandle::New(tex);
                        break;
                    }
                }
            }
        }

        if((new_bg == nullptr) && (this->cur_bg == nullptr)) {
            // Nothing shown before and nothing to show now
            return;
        }

        // Current layer becomes the one being faded out
        if(this->cur_bg != nullptr) {
            this->prev_bg = this->cur_bg;
            this->prev_bg_alpha = this->cur_bg_alpha;
            this->prev_bg_zoom = this->cur_bg_zoom;
        }

        this->cur_bg = new_bg;
        this->cur_bg_alpha = 0;
        this->cur_bg_zoom = ZoomStart;
    }

    void HoverMedia::StartSound(const u64 app_id) {
        const auto path = MakeMediaPath(SoundDirectory, app_id, "wav");
        if(!fs::ExistsFile(path)) {
            return;
        }

        auto chunk = Mix_LoadWAV(path.c_str());
        if(chunk == nullptr) {
            UL_LOG_INFO("HoverMedia: unable to load '%s': %s", path.c_str(), Mix_GetError());
            return;
        }

        const auto channel = Mix_FadeInChannel(-1, chunk, 0, SoundFadeInMs);
        if(channel < 0) {
            Mix_FreeChunk(chunk);
            return;
        }

        this->cur_sound = chunk;
        this->cur_sound_channel = channel;

        if(!this->music_ducked) {
            this->saved_music_volume = Mix_VolumeMusic(-1);
            Mix_VolumeMusic(std::min(this->saved_music_volume, DuckedMusicVolume));
            this->music_ducked = true;
        }
    }

    void HoverMedia::FadeOutCurrentSound(const s32 fade_ms) {
        if(this->cur_sound == nullptr) {
            return;
        }

        if((fade_ms > 0) && Mix_Playing(this->cur_sound_channel) && (Mix_GetChunk(this->cur_sound_channel) == this->cur_sound)) {
            Mix_FadeOutChannel(this->cur_sound_channel, fade_ms);
            // Freed once the channel stops playing it (see UpdateSounds)
            this->fading_sounds.push_back({ this->cur_sound_channel, this->cur_sound });
        }
        else {
            if(Mix_GetChunk(this->cur_sound_channel) == this->cur_sound) {
                Mix_HaltChannel(this->cur_sound_channel);
            }
            Mix_FreeChunk(this->cur_sound);
        }

        this->cur_sound = nullptr;
        this->cur_sound_channel = -1;
    }

    void HoverMedia::StopSound() {
        this->sound_pending = false;
        this->FadeOutCurrentSound(0);
        for(auto &snd: this->fading_sounds) {
            if(Mix_GetChunk(snd.channel) == snd.chunk) {
                Mix_HaltChannel(snd.channel);
            }
            Mix_FreeChunk(snd.chunk);
        }
        this->fading_sounds.clear();

        if(this->music_ducked) {
            Mix_VolumeMusic(this->saved_music_volume);
            this->music_ducked = false;
        }
    }

    void HoverMedia::UpdateSounds() {
        // Free chunks whose fade-out finished
        for(auto it = this->fading_sounds.begin(); it != this->fading_sounds.end();) {
            const auto still_ours = Mix_Playing(it->channel) && (Mix_GetChunk(it->channel) == it->chunk);
            if(!still_ours) {
                Mix_FreeChunk(it->chunk);
                it = this->fading_sounds.erase(it);
            }
            else {
                it++;
            }
        }

        // Current preview finished on its own
        if((this->cur_sound != nullptr) && !(Mix_Playing(this->cur_sound_channel) && (Mix_GetChunk(this->cur_sound_channel) == this->cur_sound))) {
            Mix_FreeChunk(this->cur_sound);
            this->cur_sound = nullptr;
            this->cur_sound_channel = -1;
        }

        // Restore the menu music once nothing is previewing
        if(this->music_ducked && (this->cur_sound == nullptr) && this->fading_sounds.empty() && !this->sound_pending) {
            Mix_VolumeMusic(this->saved_music_volume);
            this->music_ducked = false;
        }
    }

    void HoverMedia::RenderLayer(pu::ui::render::Renderer::Ref &drawer, pu::sdl2::TextureHandle::Ref &tex, const s32 alpha, const double zoom) {
        if((tex == nullptr) || (alpha <= 0)) {
            return;
        }

        const auto w = static_cast<s32>(pu::ui::render::ScreenWidth * zoom);
        const auto h = static_cast<s32>(pu::ui::render::ScreenHeight * zoom);
        const auto x = (pu::ui::render::ScreenWidth - w) / 2;
        const auto y = (pu::ui::render::ScreenHeight - h) / 2;
        drawer->RenderTexture(tex->Get(), x, y, pu::ui::render::TextureRenderOptions(static_cast<u8>(std::min(alpha, 255)), w, h, {}, {}, {}));
    }

    void HoverMedia::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - this->target_tp).count();

        if(this->bg_pending && (elapsed_ms >= BackgroundLoadDelayMs)) {
            this->bg_pending = false;
            this->LoadBackground(this->target_app_id);
        }
        if(this->sound_pending && (elapsed_ms >= SoundStartDelayMs)) {
            this->sound_pending = false;
            this->StartSound(this->target_app_id);
        }
        this->UpdateSounds();

        // Fade: new art fades in over the old one; with no new art the old one fades out
        if(this->cur_bg != nullptr) {
            if(this->cur_bg_alpha < 255) {
                this->cur_bg_alpha = std::min(255, this->cur_bg_alpha + AlphaStep);
            }
            if(this->cur_bg_alpha >= 255) {
                this->prev_bg = {};
                this->prev_bg_alpha = 0;
            }
        }
        else if(this->prev_bg != nullptr) {
            this->prev_bg_alpha -= AlphaStep;
            if(this->prev_bg_alpha <= 0) {
                this->prev_bg = {};
                this->prev_bg_alpha = 0;
            }
        }

        // Slow zoom-out (Ken Burns) on the visible art
        this->cur_bg_zoom += (1.0 - this->cur_bg_zoom) * ZoomEaseFactor;

        this->RenderLayer(drawer, this->prev_bg, this->prev_bg_alpha, this->prev_bg_zoom);
        this->RenderLayer(drawer, this->cur_bg, this->cur_bg_alpha, this->cur_bg_zoom);
    }

}
