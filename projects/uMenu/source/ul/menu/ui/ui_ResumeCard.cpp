#include <ul/menu/ui/ui_ResumeCard.hpp>
#include <ul/menu/ui/ui_Common.hpp>
#include <ul/ul_Result.hpp>
#include <switch.h>
#include <cmath>
#include <memory>

namespace ul::menu::ui {

    ResumeCard::ResumeCard(const s32 x, const s32 y) : Element(), x(x), y(y), icon(), label_tex(), name_tex(), hint_tex(), alpha(0), hiding(false), frame(0) {}

    void ResumeCard::SetGame(pu::sdl2::TextureHandle::Ref icon, const std::string &name, const pu::ui::Color text_clr) {
        this->icon = icon;
        this->alpha = 0;
        this->hiding = false;
        this->frame = 0;
        if(name.empty()) {
            this->label_tex = {};
            this->name_tex = {};
            this->hint_tex = {};
            return;
        }
        this->label_tex = pu::sdl2::TextureHandle::New(pu::ui::render::RenderText(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small), "RESUME", AccentColor));
        this->name_tex = pu::sdl2::TextureHandle::New(pu::ui::render::RenderText(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large), name, text_clr, Width - IconSize - 3 * Padding));
        this->hint_tex = pu::sdl2::TextureHandle::New(pu::ui::render::RenderText(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small), "Press any button to jump back in  ·  HOME for the home screen", SubColor));
    }

    void ResumeCard::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        if(!this->HasGame()) {
            return;
        }

        if(this->hiding) {
            this->alpha = std::max<s32>(0, this->alpha - static_cast<s32>(FadeStep));
        }
        else if(this->alpha < 0xFF) {
            this->alpha = std::min<s32>(0xFF, this->alpha + static_cast<s32>(FadeStep));
        }
        if(this->alpha <= 0) {
            return;
        }
        this->frame++;

        // Slide up slightly while fading in
        const auto rise = static_cast<s32>(24.0 * (1.0 - this->alpha / 255.0));
        const auto cy = y + rise;
        auto panel = PanelColor;
        panel.a = static_cast<u8>(panel.a * this->alpha / 0xFF);
        drawer->RenderRoundedRectangleFill(panel, x, cy, Width, Height, 22);

        const auto a = static_cast<u8>(this->alpha);
        const auto icon_y = cy + (Height - IconSize) / 2;
        if((this->icon != nullptr) && (this->icon->Get() != nullptr)) {
            drawer->RenderTexture(this->icon->Get(), x + Padding, icon_y, pu::ui::render::TextureRenderOptions(a, IconSize, IconSize, {}, {}, {}));
        }

        const auto text_x = x + Padding * 2 + IconSize;
        drawer->RenderTexture(this->label_tex->Get(), text_x, cy + 26, pu::ui::render::TextureRenderOptions(a, {}, {}, {}, {}, {}));
        drawer->RenderTexture(this->name_tex->Get(), text_x, cy + 52, pu::ui::render::TextureRenderOptions(a, {}, {}, {}, {}, {}));

        // Breathing hint under the card
        const auto pulse = 0.55 + 0.45 * (0.5 + 0.5 * std::sin(this->frame / 60.0 * 2.0 * M_PI * PulseHz));
        const auto hint_w = pu::ui::render::GetTextureWidth(this->hint_tex->Get());
        drawer->RenderTexture(this->hint_tex->Get(), x + (Width - hint_w) / 2, cy + Height + 26, pu::ui::render::TextureRenderOptions(static_cast<u8>(a * pulse), {}, {}, {}, {}, {}));
    }

    std::string LoadApplicationName(const u64 app_id) {
        // Control data is large (~150 KB): allocate it instead of using the stack
        auto control = std::make_unique<NsApplicationControlData>();
        u64 size = 0;
        if(R_FAILED(nsextGetApplicationControlData(NsApplicationControlSource_Storage, app_id, control.get(), sizeof(NsApplicationControlData), &size))) {
            return "";
        }
        NacpLanguageEntry *lang_entry = nullptr;
        if(R_FAILED(nacpGetLanguageEntry(&control->nacp, &lang_entry)) || (lang_entry == nullptr)) {
            return "";
        }
        return lang_entry->name;
    }

}
