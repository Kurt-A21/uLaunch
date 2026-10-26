
#pragma once
#include <pu/Plutonium>
#include <string>

namespace ul::menu::ui {

    // steamui: lock-screen card for the game that was running when the console went to sleep
    // ("Resume <game>", its icon, and a gently pulsing "Press any button" hint). Theme opt-in:
    // lockscreen_menu.resume_card.enabled. It fades out when resuming.
    class ResumeCard : public pu::ui::elm::Element {
        public:
            static constexpr s32 Width = 820;
            static constexpr s32 Height = 160;
            static constexpr s32 IconSize = 120;
            static constexpr s32 Padding = 20;
            static constexpr pu::ui::Color PanelColor = { 23, 29, 37, 225 };
            static constexpr pu::ui::Color AccentColor = { 26, 159, 255, 255 };
            static constexpr pu::ui::Color SubColor = { 154, 167, 180, 255 };
            static constexpr u32 FadeStep = 20;
            static constexpr double PulseHz = 0.8;

        private:
            s32 x;
            s32 y;
            pu::sdl2::TextureHandle::Ref icon;
            pu::sdl2::TextureHandle::Ref label_tex;
            pu::sdl2::TextureHandle::Ref name_tex;
            pu::sdl2::TextureHandle::Ref hint_tex;
            s32 alpha;
            bool hiding;
            u32 frame;

        public:
            ResumeCard(const s32 x, const s32 y);
            PU_SMART_CTOR(ResumeCard)

            inline s32 GetX() override {
                return this->x;
            }

            inline s32 GetY() override {
                return this->y;
            }

            inline s32 GetWidth() override {
                return Width;
            }

            inline s32 GetHeight() override {
                return Height;
            }

            // name empty = nothing to resume (card hidden)
            void SetGame(pu::sdl2::TextureHandle::Ref icon, const std::string &name, const pu::ui::Color text_clr);
            inline void Hide() {
                this->hiding = true;
            }

            inline bool HasGame() const {
                return this->name_tex != nullptr;
            }

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override {}
    };

    // Name of an installed game from its control data ("" if unavailable)
    std::string LoadApplicationName(const u64 app_id);

}
