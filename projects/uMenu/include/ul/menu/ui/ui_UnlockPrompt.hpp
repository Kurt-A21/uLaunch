
#pragma once
#include <pu/Plutonium>
#include <ul/menu/ui/ui_PressCounter.hpp>
#include <functional>
#include <string>

namespace ul::menu::ui {

    // steamui: the Switch's original lock-screen prompt ("Press the same button three times"), made dynamic:
    //  - the button you're pressing pops up in a badge; each press fills a dot with a springy pop + glow
    //  - pressing a different button shakes the dots and starts over from that button
    //  - on the third press a ring bursts out and the suspended game resumes (or the home screen opens)
    //  - idle: the prompt and empty dots breathe; a small "<game> · paused" line sits at the bottom
    // Theme opt-in: lockscreen_menu.unlock_prompt.enabled.
    class UnlockPrompt : public pu::ui::elm::Element {
        public:
            using PressCallback = std::function<void(const PressCounter::Result)>;
            using DoneCallback = std::function<void()>;

            static constexpr s32 DotRadius = 18;
            static constexpr s32 DotSpacing = 64;
            static constexpr s32 BadgeHeight = 64;
            static constexpr s32 PausedIconSize = 56;
            static constexpr double SpringStiffness = 0.22;
            static constexpr double SpringDamping = 0.62;
            static constexpr u32 ShakeFrames = 18;
            static constexpr u32 BurstFrames = 26;
            static constexpr u32 FadeStep = 17;

        private:
            s32 y;
            PressCounter counter;
            pu::ui::Color text_clr;
            pu::ui::Color accent_clr;
            PressCallback on_press;
            DoneCallback on_done;
            pu::sdl2::TextureHandle::Ref prompt_tex;
            pu::sdl2::TextureHandle::Ref badge_tex;
            pu::sdl2::TextureHandle::Ref paused_tex;
            pu::sdl2::TextureHandle::Ref paused_icon;
            double dot_scale[8];
            double dot_vel[8];
            double badge_scale;
            double badge_vel;
            u32 shake_frame;
            u32 burst_frame;
            bool completed;
            bool touch_active;
            u32 frame;
            u32 alpha;

            void SetBadge(const u64 key);
            void Bump(const u32 dot_idx);

        public:
            UnlockPrompt(const s32 y, const u32 presses, const pu::ui::Color text_clr, const pu::ui::Color accent_clr, PressCallback on_press, DoneCallback on_done);
            PU_SMART_CTOR(UnlockPrompt)

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

            // Called each time the lock screen appears. paused_name empty = no game waiting (unlocks to home).
            void Reset(const std::string &paused_name, pu::sdl2::TextureHandle::Ref paused_icon);

            inline const PressCounter &GetCounter() const {
                return this->counter;
            }

            inline bool IsCompleted() const {
                return this->completed;
            }

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
    };

}
