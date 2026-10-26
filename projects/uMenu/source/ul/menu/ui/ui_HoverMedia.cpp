#include <ul/menu/ui/ui_HoverMedia.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/ul_Result.hpp>
#include <SDL2/SDL_image.h>
#include <cstdio>

namespace ul::menu::ui {

    namespace {

        std::string MakeMediaPath(const char *dir, const u64 app_id, const char *ext) {
            char path[FS_MAX_PATH] = {};
            std::snprintf(path, sizeof(path), "%s/%016llX.%s", dir, static_cast<unsigned long long>(app_id), ext);
            return path;
        }

        // Both run on the worker thread (or inline if it couldn't be created)

        SDL_Surface *LoadBackgroundSurface(const u64 app_id) {
            for(const auto ext: { "jpg", "png" }) {
                const auto path = MakeMediaPath(HoverMedia::BackgroundDirectory, app_id, ext);
                if(fs::ExistsFile(path)) {
                    auto surface = IMG_Load(path.c_str());
                    if(surface != nullptr) {
                        return surface;
                    }
                }
            }
            return nullptr;
        }

        Mix_Chunk *LoadSoundChunk(const u64 app_id) {
            const auto path = MakeMediaPath(HoverMedia::SoundDirectory, app_id, "wav");
            if(!fs::ExistsFile(path)) {
                return nullptr;
            }
            return Mix_LoadWAV(path.c_str());
        }

        inline bool IsChunkPlayingOn(const s32 channel, Mix_Chunk *chunk) {
            return (channel >= 0) && Mix_Playing(channel) && (Mix_GetChunk(channel) == chunk);
        }

    }

    HoverMedia::HoverMedia() : Element(), worker_thread(), worker_running(false), worker_lock(), worker_cv(), worker_exit(false), req_bg_app_id(0), req_sound_app_id(0), res_bg_ready(false), res_bg_app_id(0), res_bg_surface(nullptr), res_sound_ready(false), res_sound_app_id(0), res_sound_chunk(nullptr), target_app_id(0), target_tp(), bg_pending(false), bg_waiting(false), sound_pending(false), sound_waiting(false), bg_cache(), cur_bg(), cur_bg_alpha(0), cur_bg_zoom(1.0), prev_bg(), prev_bg_alpha(0), prev_bg_zoom(1.0), cur_sound(nullptr), cur_sound_channel(-1), fading_sounds(), music_ducked(false), saved_music_volume(MIX_MAX_VOLUME) {
        mutexInit(&this->worker_lock);
        condvarInit(&this->worker_cv);

        if(R_SUCCEEDED(threadCreate(&this->worker_thread, &HoverMedia::WorkerMain, this, nullptr, WorkerStackSize, WorkerPriority, -2))) {
            if(R_SUCCEEDED(threadStart(&this->worker_thread))) {
                this->worker_running = true;
            }
            else {
                threadClose(&this->worker_thread);
            }
        }
        if(!this->worker_running) {
            UL_LOG_WARN("HoverMedia: worker thread unavailable, loading media inline");
        }
    }

    HoverMedia::~HoverMedia() {
        if(this->worker_running) {
            mutexLock(&this->worker_lock);
            this->worker_exit = true;
            condvarWakeAll(&this->worker_cv);
            mutexUnlock(&this->worker_lock);
            threadWaitForExit(&this->worker_thread);
            threadClose(&this->worker_thread);
            this->worker_running = false;
        }

        if(this->res_bg_surface != nullptr) {
            SDL_FreeSurface(this->res_bg_surface);
            this->res_bg_surface = nullptr;
        }
        if(this->res_sound_chunk != nullptr) {
            Mix_FreeChunk(this->res_sound_chunk);
            this->res_sound_chunk = nullptr;
        }

        this->StopSound();
        this->bg_cache.clear();
        this->cur_bg = {};
        this->prev_bg = {};
    }

    void HoverMedia::WorkerMain(void *arg) {
        reinterpret_cast<HoverMedia*>(arg)->WorkerLoop();
    }

    void HoverMedia::WorkerLoop() {
        mutexLock(&this->worker_lock);
        while(true) {
            while(!this->worker_exit && (this->req_bg_app_id == 0) && (this->req_sound_app_id == 0)) {
                condvarWait(&this->worker_cv, &this->worker_lock);
            }
            if(this->worker_exit) {
                break;
            }

            // Art first: it's what the user notices first
            if(this->req_bg_app_id != 0) {
                const auto app_id = this->req_bg_app_id;
                this->req_bg_app_id = 0;
                mutexUnlock(&this->worker_lock);
                auto surface = LoadBackgroundSurface(app_id);
                mutexLock(&this->worker_lock);

                if(this->res_bg_surface != nullptr) {
                    // Previous result was never picked up (superseded)
                    SDL_FreeSurface(this->res_bg_surface);
                }
                this->res_bg_surface = surface;
                this->res_bg_app_id = app_id;
                this->res_bg_ready = true;
                continue;
            }

            if(this->req_sound_app_id != 0) {
                const auto app_id = this->req_sound_app_id;
                this->req_sound_app_id = 0;
                mutexUnlock(&this->worker_lock);
                auto chunk = LoadSoundChunk(app_id);
                mutexLock(&this->worker_lock);

                if(this->res_sound_chunk != nullptr) {
                    Mix_FreeChunk(this->res_sound_chunk);
                }
                this->res_sound_chunk = chunk;
                this->res_sound_app_id = app_id;
                this->res_sound_ready = true;
            }
        }
        mutexUnlock(&this->worker_lock);
    }

    void HoverMedia::SetTarget(const u64 app_id) {
        if(app_id == this->target_app_id) {
            return;
        }

        this->target_app_id = app_id;
        this->target_tp = std::chrono::steady_clock::now();
        this->bg_pending = true;
        this->bg_waiting = false;
        this->sound_pending = app_id != 0;
        this->sound_waiting = false;

        // The previous game's sound always fades out right away
        this->FadeOutCurrentSound(SoundFadeOutMs);
    }

    bool HoverMedia::TryGetCachedBackground(const u64 app_id, pu::sdl2::TextureHandle::Ref &out_tex) {
        for(auto it = this->bg_cache.begin(); it != this->bg_cache.end(); it++) {
            if(it->app_id == app_id) {
                // Move to the back (most recently used)
                auto entry = *it;
                this->bg_cache.erase(it);
                this->bg_cache.push_back(entry);
                out_tex = entry.tex;
                return true;
            }
        }
        return false;
    }

    void HoverMedia::CacheBackground(const u64 app_id, pu::sdl2::TextureHandle::Ref tex) {
        pu::sdl2::TextureHandle::Ref existing;
        if(this->TryGetCachedBackground(app_id, existing)) {
            this->bg_cache.back().tex = tex;
            return;
        }

        if(this->bg_cache.size() >= BackgroundCacheSize) {
            // Evict the least recently used one (the texture stays alive while still on screen)
            this->bg_cache.erase(this->bg_cache.begin());
        }
        this->bg_cache.push_back({ app_id, tex });
    }

    void HoverMedia::ShowBackground(pu::sdl2::TextureHandle::Ref tex) {
        if((tex != nullptr) && (tex->Get() == nullptr)) {
            tex = {};
        }

        if(tex == this->cur_bg) {
            return;
        }
        if((tex == nullptr) && (this->cur_bg == nullptr)) {
            return;
        }

        // Current layer becomes the one being faded out
        if(this->cur_bg != nullptr) {
            this->prev_bg = this->cur_bg;
            this->prev_bg_alpha = this->cur_bg_alpha;
            this->prev_bg_zoom = this->cur_bg_zoom;
        }

        this->cur_bg = tex;
        this->cur_bg_alpha = 0;
        this->cur_bg_zoom = ZoomStart;
    }

    void HoverMedia::PollWorkerResults() {
        SDL_Surface *bg_surface = nullptr;
        u64 bg_app_id = 0;
        bool got_bg = false;
        Mix_Chunk *sound_chunk = nullptr;
        u64 sound_app_id = 0;
        bool got_sound = false;

        mutexLock(&this->worker_lock);
        if(this->res_bg_ready) {
            got_bg = true;
            bg_surface = this->res_bg_surface;
            bg_app_id = this->res_bg_app_id;
            this->res_bg_surface = nullptr;
            this->res_bg_ready = false;
        }
        if(this->res_sound_ready) {
            got_sound = true;
            sound_chunk = this->res_sound_chunk;
            sound_app_id = this->res_sound_app_id;
            this->res_sound_chunk = nullptr;
            this->res_sound_ready = false;
        }
        mutexUnlock(&this->worker_lock);

        if(got_bg) {
            // Only the GPU upload happens here (fast); decoding already happened on the worker
            auto tex = pu::sdl2::TextureHandle::New(pu::ui::render::ConvertToTexture(bg_surface));
            this->CacheBackground(bg_app_id, tex);
            if(this->bg_waiting && (bg_app_id == this->target_app_id)) {
                this->bg_waiting = false;
                this->ShowBackground(tex);
            }
        }

        if(got_sound) {
            if(this->sound_waiting && (sound_app_id == this->target_app_id) && (sound_chunk != nullptr)) {
                this->sound_waiting = false;
                this->PlaySoundChunk(sound_chunk);
            }
            else if(sound_chunk != nullptr) {
                Mix_FreeChunk(sound_chunk);
            }
        }
    }

    void HoverMedia::PlaySoundChunk(Mix_Chunk *chunk) {
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

        if((fade_ms > 0) && IsChunkPlayingOn(this->cur_sound_channel, this->cur_sound)) {
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
        this->sound_waiting = false;
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
            if(!IsChunkPlayingOn(it->channel, it->chunk)) {
                Mix_FreeChunk(it->chunk);
                it = this->fading_sounds.erase(it);
            }
            else {
                it++;
            }
        }

        // Current preview finished on its own
        if((this->cur_sound != nullptr) && !IsChunkPlayingOn(this->cur_sound_channel, this->cur_sound)) {
            Mix_FreeChunk(this->cur_sound);
            this->cur_sound = nullptr;
            this->cur_sound_channel = -1;
        }

        // Restore the menu music once nothing is previewing
        if(this->music_ducked && (this->cur_sound == nullptr) && this->fading_sounds.empty() && !this->sound_pending && !this->sound_waiting) {
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
            pu::sdl2::TextureHandle::Ref cached;
            if(this->target_app_id == 0) {
                this->ShowBackground({});
            }
            else if(this->TryGetCachedBackground(this->target_app_id, cached)) {
                this->ShowBackground(cached);
            }
            else if(this->worker_running) {
                mutexLock(&this->worker_lock);
                this->req_bg_app_id = this->target_app_id;
                condvarWakeAll(&this->worker_cv);
                mutexUnlock(&this->worker_lock);
                this->bg_waiting = true;
            }
            else {
                auto tex = pu::sdl2::TextureHandle::New(pu::ui::render::ConvertToTexture(LoadBackgroundSurface(this->target_app_id)));
                this->CacheBackground(this->target_app_id, tex);
                this->ShowBackground(tex);
            }
        }

        if(this->sound_pending && (elapsed_ms >= SoundStartDelayMs)) {
            this->sound_pending = false;
            if(this->worker_running) {
                mutexLock(&this->worker_lock);
                this->req_sound_app_id = this->target_app_id;
                condvarWakeAll(&this->worker_cv);
                mutexUnlock(&this->worker_lock);
                this->sound_waiting = true;
            }
            else {
                auto chunk = LoadSoundChunk(this->target_app_id);
                if(chunk != nullptr) {
                    this->PlaySoundChunk(chunk);
                }
            }
        }

        if(this->worker_running) {
            this->PollWorkerResults();
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
