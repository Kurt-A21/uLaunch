
#pragma once
#include <pu/Plutonium>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace ul::menu::ui {

    // steamui: console-style profile picker (theme opt-in: startup_menu.user_carousel.enabled).
    // Big avatars in a horizontal row; the focused one springs up with the same glow ring as the game row,
    // the row glides to keep it centred, names fade in underneath. Last tile = "Add user".
    class UserCarousel : public pu::ui::elm::Element {
        public:
            using SelectCallback = std::function<void()>;
            using MoveCallback = std::function<void()>;

            static constexpr double FocusScale = 1.18;
            static constexpr double SpringStiffness = 0.20;
            static constexpr double SpringDamping = 0.68;
            static constexpr double ScrollEase = 0.22;
            static constexpr u8 UnfocusedAlpha = 140;
            static constexpr s64 StickRepeatMs = 200;
            static constexpr u32 FadeInStep = 17;

            struct Item {
                pu::sdl2::TextureHandle::Ref avatar;    // null = "Add user" tile
                pu::sdl2::TextureHandle::Ref name_tex;
                SelectCallback on_select;
                double scale;
                double scale_vel;
            };

        private:
            s32 y;
            s32 avatar_size;
            s32 spacing;
            std::vector<Item> items;
            u32 focus_idx;
            double scroll;
            u32 fade_alpha;
            pu::sdl2::TextureHandle::Ref ring_tex;
            pu::ui::Color text_clr;
            pu::ui::Color tile_clr;
            MoveCallback on_move;
            std::chrono::steady_clock::time_point last_stick_move;
            bool touch_active;

            inline s32 ItemCenterX(const u32 idx) const {
                return static_cast<s32>(pu::ui::render::ScreenWidth) / 2 + static_cast<s32>((static_cast<double>(idx) - this->scroll) * this->spacing);
            }

            void MoveFocus(const s32 delta);

        public:
            UserCarousel(const s32 y, const s32 avatar_size, const s32 spacing, pu::sdl2::TextureHandle::Ref ring_tex, const pu::ui::Color text_clr, MoveCallback on_move);
            PU_SMART_CTOR(UserCarousel)

            inline s32 GetX() override {
                return 0;
            }

            inline s32 GetY() override {
                return this->y - this->avatar_size;
            }

            inline s32 GetWidth() override {
                return pu::ui::render::ScreenWidth;
            }

            inline s32 GetHeight() override {
                return this->avatar_size * 3;
            }

            void Clear();
            void AddUser(pu::sdl2::TextureHandle::Ref avatar, const std::string &name, SelectCallback on_select);
            void AddCreateUser(const std::string &label, SelectCallback on_select);

            inline u32 GetFocusedIndex() const {
                return this->focus_idx;
            }

            inline size_t GetItemCount() const {
                return this->items.size();
            }

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
    };

}
