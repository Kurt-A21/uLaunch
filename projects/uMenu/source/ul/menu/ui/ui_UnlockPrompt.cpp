#include <ul/menu/ui/ui_UnlockPrompt.hpp>
#include <algorithm>
#include <cmath>

namespace ul::menu::ui {

    namespace {

        inline pu::ui::Color WithAlpha(pu::ui::Color clr, const double a) {
            clr.a = static_cast<u8>(std::clamp(a, 0.0, 255.0));
            return clr;
        }

        inline pu::sdl2::TextureHandle::Ref MakeText(const pu::ui::DefaultFontSize size, const std::string &text, const pu::ui::Color clr) {
            return pu::sdl2::TextureHandle::New(pu::ui::render::RenderText(pu::ui::GetDefaultFont(size), text, clr));
        }

        constexpr s32 ScreenCenterX = static_cast<s32>(pu::ui::render::ScreenWidth) / 2;

    }

    UnlockPrompt::UnlockPrompt(const s32 y, const u32 presses, const pu::ui::Color text_clr, const pu::ui::Color accent_clr, PressCallback on_press, DoneCallback on_done) : Element(), y(y), counter(std::clamp<u32>(presses, 1, 8)), text_clr(text_clr), accent_clr(accent_clr), on_press(on_press), on_done(on_done), prompt_tex(), badge_tex(), paused_tex(), paused_icon(), dot_scale(), dot_vel(), badge_scale(0.0), badge_vel(0.0), shake_frame(ShakeFrames), burst_frame(BurstFrames), completed(false), touch_active(false), frame(0), alpha(0) {}

    void UnlockPrompt::Reset(const std::string &paused_name, pu::sdl2::TextureHandle::Ref paused_icon) {
        this->counter.Reset();
        for(u32 i = 0; i < 8; i++) {
            this->dot_scale[i] = 0.0;
            this->dot_vel[i] = 0.0;
        }
        this->badge_scale = 0.0;
        this->badge_vel = 0.0;
        this->badge_tex = {};
        this->shake_frame = ShakeFrames;
        this->burst_frame = BurstFrames;
        this->completed = false;
        this->touch_active = false;
        this->frame = 0;
        this->alpha = 0;

        const auto has_game = !paused_name.empty();
        const auto n = this->counter.GetTarget();
        const std::string times = (n == 3) ? "three times" : (std::to_string(n) + " times");
        this->prompt_tex = MakeText(pu::ui::DefaultFontSize::Large, "Press the same button " + times + (has_game ? " to resume" : ""), this->text_clr);
        this->paused_icon = has_game ? paused_icon : nullptr;
        this->paused_tex = has_game ? MakeText(pu::ui::DefaultFontSize::Medium, paused_name + "  ·  paused", WithAlpha(this->text_clr, 210)) : nullptr;
    }

    void UnlockPrompt::SetBadge(const u64 key) {
        this->badge_tex = MakeText(pu::ui::DefaultFontSize::MediumLarge, PressCounter::GetButtonLabel(key), this->text_clr);
    }

    void UnlockPrompt::Bump(const u32 dot_idx) {
        if(dot_idx < 8) {
            this->dot_vel[dot_idx] += 0.55;  // pops past full size, then settles
        }
        this->badge_vel += 0.35;
    }

    void UnlockPrompt::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        if(this->prompt_tex == nullptr) {
            return;
        }
        this->frame++;
        if(this->alpha < 0xFF) {
            this->alpha = std::min<u32>(0xFF, this->alpha + FadeStep);
        }
        const auto fa = this->alpha / 255.0;
        const auto n = this->counter.GetTarget();
        const auto count = this->counter.GetCount();

        // Springs: filled dots head to 1.0, empty to 0; the badge shows once a button was pressed
        for(u32 i = 0; i < n; i++) {
            const auto target = (i < count) ? 1.0 : 0.0;
            this->dot_vel[i] += (target - this->dot_scale[i]) * SpringStiffness;
            this->dot_vel[i] *= SpringDamping;
            this->dot_scale[i] += this->dot_vel[i];
        }
        const auto badge_target = (this->badge_tex != nullptr) ? 1.0 : 0.0;
        this->badge_vel += (badge_target - this->badge_scale) * SpringStiffness;
        this->badge_vel *= SpringDamping;
        this->badge_scale += this->badge_vel;

        // Wrong button: decaying horizontal shake
        s32 shake = 0;
        if(this->shake_frame < ShakeFrames) {
            const auto t = static_cast<double>(this->shake_frame) / ShakeFrames;
            shake = static_cast<s32>(16.0 * std::sin(t * 6.0 * M_PI) * (1.0 - t));
            this->shake_frame++;
        }

        // Prompt (breathes gently while idle)
        const auto breathe = (count == 0) ? (0.78 + 0.22 * (0.5 + 0.5 * std::sin(this->frame / 60.0 * 2.0 * M_PI * 0.5))) : 1.0;
        const auto pw = pu::ui::render::GetTextureWidth(this->prompt_tex->Get());
        drawer->RenderTexture(this->prompt_tex->Get(), ScreenCenterX - pw / 2, this->y, pu::ui::render::TextureRenderOptions(static_cast<u8>(255 * fa * breathe), {}, {}, {}, {}, {}));

        // Badge with the button being pressed
        const auto badge_y = this->y + 78;
        if((this->badge_tex != nullptr) && (this->badge_scale > 0.02)) {
            const auto tw = pu::ui::render::GetTextureWidth(this->badge_tex->Get());
            const auto th = pu::ui::render::GetTextureHeight(this->badge_tex->Get());
            const auto bw = static_cast<s32>((std::max(tw + 48, BadgeHeight)) * this->badge_scale);
            const auto bh = static_cast<s32>(BadgeHeight * this->badge_scale);
            const auto bx = ScreenCenterX - bw / 2 + shake;
            const auto by = badge_y + (BadgeHeight - bh) / 2;
            drawer->RenderRoundedRectangleFill(WithAlpha(this->accent_clr, 70 * fa), bx - 6, by - 6, bw + 12, bh + 12, (bh + 12) / 2);
            drawer->RenderRoundedRectangleFill(WithAlpha({ 23, 29, 37, 255 }, 240 * fa), bx, by, bw, bh, bh / 2);
            if(this->badge_scale > 0.6) {
                drawer->RenderTexture(this->badge_tex->Get(), ScreenCenterX - tw / 2 + shake, badge_y + (BadgeHeight - th) / 2, pu::ui::render::TextureRenderOptions(static_cast<u8>(255 * fa), {}, {}, {}, {}, {}));
            }
        }

        // Dots
        const auto dots_y = badge_y + BadgeHeight + 56;
        const auto first_x = ScreenCenterX - static_cast<s32>((n - 1) * DotSpacing) / 2 + shake;
        const auto burst_t = (this->burst_frame < BurstFrames) ? static_cast<double>(this->burst_frame) / BurstFrames : 1.0;
        for(u32 i = 0; i < n; i++) {
            const auto cx = first_x + static_cast<s32>(i * DotSpacing);
            const auto ring_a = (count == 0) ? 110 * breathe : 110.0;
            drawer->RenderCircle(WithAlpha(this->text_clr, ring_a * fa), cx, dots_y, DotRadius);
            drawer->RenderCircle(WithAlpha(this->text_clr, ring_a * fa), cx, dots_y, DotRadius - 1);
            const auto s = this->dot_scale[i];
            if(s > 0.02) {
                const auto r = static_cast<s32>(DotRadius * s);
                drawer->RenderCircleFill(WithAlpha(this->accent_clr, 60 * fa), cx, dots_y, r + 10);
                const auto flash = (this->completed && burst_t < 1.0) ? (1.0 - burst_t) : 0.0;
                pu::ui::Color fill = this->accent_clr;
                fill.r = static_cast<u8>(fill.r + (255 - fill.r) * flash);
                fill.g = static_cast<u8>(fill.g + (255 - fill.g) * flash);
                fill.b = static_cast<u8>(fill.b + (255 - fill.b) * flash);
                drawer->RenderCircleFill(WithAlpha(fill, 255 * fa), cx, dots_y, std::max(1, r));
            }
        }

        // Unlock burst: rings expanding from the dots
        if(this->burst_frame < BurstFrames) {
            const auto eased = 1.0 - (1.0 - burst_t) * (1.0 - burst_t);
            const auto radius = static_cast<s32>(40 + 300 * eased);
            const auto a = 200.0 * (1.0 - burst_t);
            for(s32 k = 0; k < 4; k++) {
                drawer->RenderCircle(WithAlpha(this->accent_clr, a * fa), ScreenCenterX, dots_y, radius - k);
            }
            drawer->RenderCircle(WithAlpha(this->text_clr, a * 0.6 * fa), ScreenCenterX, dots_y, static_cast<s32>(radius * 0.7));
            this->burst_frame++;
        }

        // "<game> · paused" at the bottom
        if(this->paused_tex != nullptr) {
            const auto tw = pu::ui::render::GetTextureWidth(this->paused_tex->Get());
            const auto th = pu::ui::render::GetTextureHeight(this->paused_tex->Get());
            const auto has_icon = (this->paused_icon != nullptr) && (this->paused_icon->Get() != nullptr);
            const auto total_w = tw + (has_icon ? PausedIconSize + 18 : 0);
            const auto line_y = static_cast<s32>(pu::ui::render::ScreenHeight) - 120;
            auto px = ScreenCenterX - total_w / 2;
            if(has_icon) {
                drawer->RenderTexture(this->paused_icon->Get(), px, line_y, pu::ui::render::TextureRenderOptions(static_cast<u8>(230 * fa), PausedIconSize, PausedIconSize, {}, {}, {}));
                px += PausedIconSize + 18;
            }
            drawer->RenderTexture(this->paused_tex->Get(), px, line_y + (PausedIconSize - th) / 2, pu::ui::render::TextureRenderOptions(static_cast<u8>(255 * fa), {}, {}, {}, {}, {}));
        }
    }

    void UnlockPrompt::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        // A touch counts once per tap (the point repeats every frame while the finger is down)
        const auto touching = !touch_pos.IsEmpty();
        const auto tap_started = touching && !this->touch_active;
        this->touch_active = touching;

        if(this->completed || (this->prompt_tex == nullptr)) {
            return;
        }

        auto key = PressCounter::PickButton(keys_down);
        if((key == 0) && tap_started) {
            key = PressCounter::TouchKey;
        }
        const auto res = this->counter.Feed(key);
        if(res == PressCounter::Result::Ignored) {
            return;
        }

        this->SetBadge(key);
        if(res == PressCounter::Result::Restarted) {
            this->shake_frame = 0;
            for(u32 i = 0; i < 8; i++) {
                this->dot_vel[i] = 0.0;
                this->dot_scale[i] = 0.0;
            }
        }
        const auto filled = this->counter.GetCount();
        this->Bump(filled - 1);

        if(this->on_press) {
            this->on_press(res);
        }
        if(res == PressCounter::Result::Completed) {
            this->completed = true;
            this->burst_frame = 0;
            if(this->on_done) {
                this->on_done();
            }
        }
    }

}
