#include <ul/menu/ui/ui_UserCarousel.hpp>
#include <algorithm>
#include <cmath>

namespace ul::menu::ui {

    UserCarousel::UserCarousel(const s32 y, const s32 avatar_size, const s32 spacing, pu::sdl2::TextureHandle::Ref ring_tex, const pu::ui::Color text_clr, MoveCallback on_move) : Element(), y(y), avatar_size(avatar_size), spacing(spacing), items(), focus_idx(0), scroll(0.0), fade_alpha(0), ring_tex(ring_tex), text_clr(text_clr), tile_clr({ 255, 255, 255, 22 }), on_move(on_move), last_stick_move(), touch_active(false) {}

    void UserCarousel::Clear() {
        this->items.clear();
        this->focus_idx = 0;
        this->scroll = 0.0;
        this->fade_alpha = 0;
    }

    void UserCarousel::AddUser(pu::sdl2::TextureHandle::Ref avatar, const std::string &name, SelectCallback on_select) {
        auto name_tex = pu::sdl2::TextureHandle::New(pu::ui::render::RenderText(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::MediumLarge), name, this->text_clr));
        this->items.push_back({ avatar, name_tex, on_select, 1.0, 0.0 });
    }

    void UserCarousel::AddCreateUser(const std::string &label, SelectCallback on_select) {
        auto name_tex = pu::sdl2::TextureHandle::New(pu::ui::render::RenderText(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::MediumLarge), label, this->text_clr));
        this->items.push_back({ nullptr, name_tex, on_select, 1.0, 0.0 });
    }

    void UserCarousel::MoveFocus(const s32 delta) {
        const auto target = static_cast<s32>(this->focus_idx) + delta;
        if(this->items.empty() || (target < 0) || (target >= static_cast<s32>(this->items.size()))) {
            return;
        }
        this->focus_idx = static_cast<u32>(target);
        if(this->on_move) {
            this->on_move();
        }
    }

    void UserCarousel::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        if(this->items.empty()) {
            return;
        }

        if(this->fade_alpha < 0xFF) {
            this->fade_alpha = std::min<u32>(0xFF, this->fade_alpha + FadeInStep);
        }
        this->scroll += (static_cast<double>(this->focus_idx) - this->scroll) * ScrollEase;

        // Draw unfocused first, focused last (on top)
        for(u32 pass = 0; pass < 2; pass++) {
            for(u32 i = 0; i < this->items.size(); i++) {
                const auto is_focus = i == this->focus_idx;
                if(is_focus != (pass == 1)) {
                    continue;
                }

                // Each item is visited in exactly one pass, so its spring steps once per frame
                auto &item = this->items.at(i);
                const auto target = is_focus ? FocusScale : 1.0;
                item.scale_vel += (target - item.scale) * SpringStiffness;
                item.scale_vel *= SpringDamping;
                item.scale += item.scale_vel;

                const auto cx = this->ItemCenterX(i);
                const auto size = static_cast<s32>(this->avatar_size * item.scale);
                if((cx + size < 0) || (cx - size > static_cast<s32>(pu::ui::render::ScreenWidth))) {
                    continue;
                }
                const auto ax = cx - size / 2;
                const auto ay = this->y - size / 2;
                const auto alpha = static_cast<u8>((is_focus ? 0xFF : UnfocusedAlpha) * this->fade_alpha / 0xFF);

                if(item.avatar != nullptr) {
                    drawer->RenderTexture(item.avatar->Get(), ax, ay, pu::ui::render::TextureRenderOptions(alpha, size, size, {}, {}, {}));
                }
                else {
                    // "Add user": soft tile with a plus
                    auto clr = this->tile_clr;
                    clr.a = static_cast<u8>(clr.a * alpha / 0xFF * 2);
                    drawer->RenderRoundedRectangleFill(clr, ax, ay, size, size, size / 10);
                    auto plus = this->text_clr;
                    plus.a = alpha;
                    const auto bar_len = size / 3;
                    const auto bar_w = std::max(6, size / 28);
                    drawer->RenderRectangleFill(plus, cx - bar_len / 2, this->y - bar_w / 2, bar_len, bar_w);
                    drawer->RenderRectangleFill(plus, cx - bar_w / 2, this->y - bar_len / 2, bar_w, bar_len);
                }

                if(is_focus && (this->ring_tex != nullptr) && (this->ring_tex->Get() != nullptr)) {
                    const auto ring_size = static_cast<s32>(size * 448.0 / 384.0);
                    drawer->RenderTexture(this->ring_tex->Get(), cx - ring_size / 2, this->y - ring_size / 2, pu::ui::render::TextureRenderOptions(static_cast<u8>(this->fade_alpha), ring_size, ring_size, {}, {}, {}));
                }

                if(item.name_tex != nullptr) {
                    const auto tw = pu::ui::render::GetTextureWidth(item.name_tex->Get());
                    const auto name_y = this->y + static_cast<s32>(this->avatar_size * FocusScale / 2) + 36;
                    drawer->RenderTexture(item.name_tex->Get(), cx - tw / 2, name_y, pu::ui::render::TextureRenderOptions(static_cast<u8>((is_focus ? 0xFF : 110) * this->fade_alpha / 0xFF), {}, {}, {}, {}, {}));
                }
            }
        }
    }

    void UserCarousel::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(this->items.empty()) {
            return;
        }

        if(keys_down & HidNpadButton_Left) {
            this->MoveFocus(-1);
        }
        else if(keys_down & HidNpadButton_Right) {
            this->MoveFocus(1);
        }
        else if(keys_held & (HidNpadButton_StickLLeft | HidNpadButton_StickLRight | HidNpadButton_StickRLeft | HidNpadButton_StickRRight)) {
            const auto now = std::chrono::steady_clock::now();
            if(std::chrono::duration_cast<std::chrono::milliseconds>(now - this->last_stick_move).count() >= StickRepeatMs) {
                this->last_stick_move = now;
                this->MoveFocus((keys_held & (HidNpadButton_StickLLeft | HidNpadButton_StickRLeft)) ? -1 : 1);
            }
        }
        else if(keys_down & HidNpadButton_A) {
            auto &item = this->items.at(this->focus_idx);
            if(item.on_select) {
                item.on_select();
            }
            return;
        }

        // Touch: act once per tap (the touch point repeats every frame while the finger is down)
        const auto touching = !touch_pos.IsEmpty();
        const auto tap_started = touching && !this->touch_active;
        this->touch_active = touching;
        if(tap_started) {
            for(u32 i = 0; i < this->items.size(); i++) {
                const auto cx = this->ItemCenterX(i);
                const auto half = static_cast<s32>(this->avatar_size * this->items.at(i).scale / 2);
                if(touch_pos.HitsRegion(cx - half, this->y - half, half * 2, half * 2)) {
                    if(i == this->focus_idx) {
                        auto &item = this->items.at(i);
                        if(item.on_select) {
                            item.on_select();
                        }
                    }
                    else {
                        this->MoveFocus(static_cast<s32>(i) - static_cast<s32>(this->focus_idx));
                    }
                    break;
                }
            }
        }
    }

}
