#include <ul/menu/ui/ui_GameDialog.hpp>
#include <ul/menu/ui/ui_Common.hpp>
#include <ul/menu/ui/ui_Haptics.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>

namespace ul::menu::ui {

    namespace {

        using pu::ui::render::GetTextureWidth;
        using pu::ui::render::GetTextureHeight;

        constexpr s32 ScreenW = static_cast<s32>(pu::ui::render::ScreenWidth);
        constexpr s32 ScreenH = static_cast<s32>(pu::ui::render::ScreenHeight);
        constexpr s32 ScreenMargin = 60;
        constexpr s32 HeaderContentGap = 18;
        constexpr s32 ContentRowsGap = 34;
        constexpr s32 RowTextMargin = 30;
        constexpr s32 HintGlyphRadius = 19;
        constexpr s32 DetailGap = 60;

        inline pu::ui::Color WithAlpha(pu::ui::Color clr, const double a) {
            clr.a = static_cast<u8>(std::clamp(a, 0.0, 255.0));
            return clr;
        }

        inline pu::ui::Color Mix(const pu::ui::Color a, const pu::ui::Color b, const double t) {
            return {
                static_cast<u8>(a.r + (b.r - a.r) * t),
                static_cast<u8>(a.g + (b.g - a.g) * t),
                static_cast<u8>(a.b + (b.b - a.b) * t),
                static_cast<u8>(a.a + (b.a - a.a) * t)
            };
        }

        inline pu::ui::render::TextureRenderOptions Alpha(const double a) {
            return pu::ui::render::TextureRenderOptions(static_cast<u8>(std::clamp(a, 0.0, 255.0)), {}, {}, {}, {}, {});
        }

        inline s64 NowMs() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        }

        // Owns the textures created for one dialog
        struct Texts {
            std::vector<pu::sdl2::Texture> all;

            pu::sdl2::Texture Make(const pu::ui::DefaultFontSize size, const std::string &text, const pu::ui::Color clr, const u32 max_width = 0) {
                if(text.empty()) {
                    return nullptr;
                }
                auto tex = pu::ui::render::RenderText(pu::ui::GetDefaultFont(size), text, clr, max_width);
                this->all.push_back(tex);
                return tex;
            }

            ~Texts() {
                for(auto &tex: this->all) {
                    pu::ui::render::DeleteTexture(tex);
                }
            }
        };

        // Word-wraps every line of the text to max_width (Plutonium only truncates)
        std::string WrapText(const std::string &font, const std::string &text, const s32 max_width) {
            std::string out;
            size_t line_start = 0;
            while(line_start <= text.size()) {
                const auto line_end = std::min(text.find('\n', line_start), text.size());
                const auto line = text.substr(line_start, line_end - line_start);
                std::string cur;
                size_t i = 0;
                while(i < line.size()) {
                    auto j = line.find(' ', i);
                    if(j == std::string::npos) {
                        j = line.size();
                    }
                    const auto word = line.substr(i, j - i);
                    const auto candidate = cur.empty() ? word : (cur + " " + word);
                    if(!cur.empty() && (pu::ui::render::GetTextWidth(font, candidate) > max_width)) {
                        out += cur + "\n";
                        cur = word;
                    }
                    else {
                        cur = candidate;
                    }
                    i = j + 1;
                }
                out += cur;
                if(line_end >= text.size()) {
                    break;
                }
                out += "\n";
                line_start = line_end + 1;
            }
            return out;
        }

        // Round button glyph with a letter, like the console's own hints; returns its width
        s32 RenderHint(pu::ui::render::Renderer::Ref &drawer, pu::sdl2::Texture letter, pu::sdl2::Texture label, const s32 right_x, const s32 cy, const pu::ui::Color clr, const double a) {
            const auto lw = GetTextureWidth(label);
            const auto lh = GetTextureHeight(label);
            const auto total = 2 * HintGlyphRadius + 12 + lw;
            const auto x = right_x - total;
            drawer->RenderCircleFill(WithAlpha(clr, 235 * a / 255.0), x + HintGlyphRadius, cy, HintGlyphRadius);
            drawer->RenderTexture(letter, x + HintGlyphRadius - GetTextureWidth(letter) / 2, cy - GetTextureHeight(letter) / 2, Alpha(a));
            drawer->RenderTexture(label, x + 2 * HintGlyphRadius + 12, cy - lh / 2, Alpha(a * 0.85));
            return total;
        }

    }

    bool GameDialog::IsEnabledByTheme() {
        util::JSON json;
        return TryGetUiElement("dialog", "game_dialog", json) && json.value("enabled", false);
    }

    s32 GameDialog::Show(pu::ui::Application *app, const GameDialogStyle &style, const std::string &title, const std::string &content, const std::vector<DialogOption> &opts, const bool use_last_opt_as_cancel, pu::sdl2::TextureHandle::Ref icon) {
        if(opts.empty()) {
            return -1;
        }

        Texts texts;
        const auto has_icon = (icon != nullptr) && (icon->Get() != nullptr);
        const auto dark_text = pu::ui::Color { 0x0B, 0x0F, 0x14, 0xFF };

        // ---- Measure ----
        const auto content_font = pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium);
        const auto label_font = pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::MediumLarge);
        const auto detail_font = pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small);
        const auto max_inner = MaxWidth - 2 * Padding;

        s32 want_inner = 0;
        const auto header_text_x_off = has_icon ? (IconSize + 28) : 0;
        want_inner = std::max(want_inner, header_text_x_off + pu::ui::render::GetTextWidth(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large), title));
        for(const auto &opt: opts) {
            const auto dw = opt.detail.empty() ? 0 : (pu::ui::render::GetTextWidth(detail_font, opt.detail) + DetailGap);
            want_inner = std::max(want_inner, 2 * RowTextMargin + pu::ui::render::GetTextWidth(label_font, opt.label) + dw);
        }
        const auto wrapped = WrapText(content_font, content, max_inner);
        auto cnt_tex = texts.Make(pu::ui::DefaultFontSize::Medium, wrapped, WithAlpha(style.text_clr, 0xFF));
        if(cnt_tex != nullptr) {
            want_inner = std::max(want_inner, GetTextureWidth(cnt_tex));
        }
        const auto card_w = std::clamp(want_inner + 2 * Padding, MinWidth, MaxWidth);
        const auto inner_w = card_w - 2 * Padding;

        auto title_tex = texts.Make(pu::ui::DefaultFontSize::Large, title, style.title_clr, inner_w - header_text_x_off);
        const auto header_h = std::max(has_icon ? IconSize : 0, (title_tex != nullptr) ? GetTextureHeight(title_tex) : 0);
        const auto cnt_h = (cnt_tex != nullptr) ? GetTextureHeight(cnt_tex) : 0;

        struct RowTexts {
            pu::sdl2::Texture label;
            pu::sdl2::Texture label_focus;
            pu::sdl2::Texture detail;
            pu::sdl2::Texture detail_focus;
        };
        std::vector<RowTexts> rows;
        for(const auto &opt: opts) {
            RowTexts r = {};
            // The label keeps its full width (up to half the row); the detail gets the rest, with a clear gap
            const auto label_w = std::min<s32>(pu::ui::render::GetTextWidth(label_font, opt.label), inner_w / 2);
            const auto detail_max = std::max<s32>(0, inner_w - 2 * RowTextMargin - label_w - DetailGap);
            const auto detail_w = opt.detail.empty() ? 0 : std::min<s32>(pu::ui::render::GetTextWidth(detail_font, opt.detail), detail_max);
            const auto label_max = inner_w - 2 * RowTextMargin - ((detail_w > 0) ? (detail_w + DetailGap) : 0);
            r.label = texts.Make(pu::ui::DefaultFontSize::MediumLarge, opt.label, style.text_clr, label_max);
            r.label_focus = texts.Make(pu::ui::DefaultFontSize::MediumLarge, opt.label, dark_text, label_max);
            if(detail_w > 0) {
                r.detail = texts.Make(pu::ui::DefaultFontSize::Small, opt.detail, WithAlpha(style.text_clr, 0xFF), detail_max);
                r.detail_focus = texts.Make(pu::ui::DefaultFontSize::Small, opt.detail, dark_text, detail_max);
            }
            rows.push_back(r);
        }

        auto hint_a = texts.Make(pu::ui::DefaultFontSize::Small, "A", dark_text);
        auto hint_b = texts.Make(pu::ui::DefaultFontSize::Small, "B", dark_text);
        auto hint_select = texts.Make(pu::ui::DefaultFontSize::Small, "Select", style.text_clr);
        auto hint_back = texts.Make(pu::ui::DefaultFontSize::Small, "Back", style.text_clr);

        // ---- Layout ----
        const auto top_part = Padding + header_h + ((cnt_h > 0) ? (HeaderContentGap + cnt_h) : 0) + ContentRowsGap;
        const auto row_step = RowHeight + RowGap;
        const auto avail_rows_h = (ScreenH - 2 * ScreenMargin) - top_part - FooterHeight - 24;
        const auto max_visible = static_cast<u32>(std::max<s32>(2, (avail_rows_h + RowGap) / row_step));

        DialogNav nav;
        nav.Reset(static_cast<u32>(opts.size()), max_visible, 0);
        const auto visible = static_cast<s32>(nav.GetVisible());
        const auto rows_h = visible * RowHeight + (visible - 1) * RowGap;
        const auto card_h = std::min(ScreenH - 2 * ScreenMargin, top_part + rows_h + 24 + FooterHeight);
        const auto card_x = (ScreenW - card_w) / 2;
        const auto base_card_y = (ScreenH - card_h) / 2;
        const auto rows_x = card_x + Padding - 16;
        const auto rows_w = inner_w + 32;

        // ---- Feedback ----
        Haptics haptics;
        haptics.Initialize(style.haptics);
        auto move_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/CursorMove.wav"));

        // ---- Animation state ----
        double appear = 0.0, appear_vel = 0.0;
        bool closing = false;
        double hl_row = 0.0, hl_vel = 0.0;       // highlight position, in rows relative to the first visible row
        double pop = 0.0, pop_vel = 0.0;         // highlight pop on every move (px)
        double scroll = 0.0, scroll_vel = 0.0;   // list scroll, in rows
        HoldRepeat repeat;
        bool touch_active = false;
        s32 result = 0;
        bool backed_out = false;

        const auto opt_count = static_cast<s32>(opts.size());
        const auto finish = [&](const s32 idx, const bool back) {
            if(closing) {
                return;
            }
            closing = true;
            backed_out = back;
            result = idx;
            pop_vel -= 5.0;
            haptics.Play(Haptics::Kind::Press);
        };

        while(true) {
            const auto ok = app->CallForRenderWithRenderOver([&](pu::ui::render::Renderer::Ref &drawer) -> bool {
                haptics.Update();

                // ---- Input ----
                if(!closing) {
                    const auto down = app->GetButtonsDown();
                    const auto held = app->GetButtonsHeld();
                    const auto to_dir = [](const u64 keys) -> s32 {
                        if(keys & (HidNpadButton_AnyUp | HidNpadButton_AnyLeft)) {
                            return -1;
                        }
                        if(keys & (HidNpadButton_AnyDown | HidNpadButton_AnyRight)) {
                            return 1;
                        }
                        return 0;
                    };
                    const auto dir = repeat.Feed(to_dir(down), to_dir(held), NowMs());
                    if((dir != 0) && nav.Move(dir)) {
                        pop_vel += 3.2;
                        haptics.Play(Haptics::Kind::Tick);
                        pu::audio::PlaySfx(move_sfx);
                    }

                    if(down & HidNpadButton_A) {
                        finish(static_cast<s32>(nav.GetSelected()), false);
                    }
                    else if(down & HidNpadButton_B) {
                        finish(use_last_opt_as_cancel ? (opt_count - 1) : static_cast<s32>(nav.GetSelected()), true);
                    }

                    // Touch: tap a row to pick it, tap outside the card to go back
                    const auto tch = app->GetTouchState();
                    const auto touching = tch.count > 0;
                    if(touching && !touch_active) {
                        const auto tx = static_cast<s32>(tch.touches[0].x * pu::ui::render::ScreenFactor);
                        const auto ty = static_cast<s32>(tch.touches[0].y * pu::ui::render::ScreenFactor);
                        const auto rows_y = base_card_y + top_part;
                        if((tx < card_x) || (tx > card_x + card_w) || (ty < base_card_y) || (ty > base_card_y + card_h)) {
                            finish(use_last_opt_as_cancel ? (opt_count - 1) : static_cast<s32>(nav.GetSelected()), true);
                        }
                        else if((tx >= rows_x) && (tx <= rows_x + rows_w) && (ty >= rows_y) && (ty < rows_y + rows_h)) {
                            const auto rel = (ty - rows_y) / row_step;
                            const auto idx = static_cast<s32>(nav.GetFirstVisible()) + rel;
                            if((idx < opt_count) && (((ty - rows_y) % row_step) < RowHeight)) {
                                nav.Select(static_cast<u32>(idx));
                                finish(idx, false);
                            }
                        }
                    }
                    touch_active = touching;
                }

                // ---- Springs ----
                if(closing) {
                    appear = std::max(0.0, appear - CloseStep);
                }
                else {
                    appear_vel += (1.0 - appear) * OpenStiffness;
                    appear_vel *= OpenDamping;
                    appear += appear_vel;
                }
                const auto scroll_target = static_cast<double>(nav.GetFirstVisible());
                scroll_vel += (scroll_target - scroll) * MoveStiffness;
                scroll_vel *= MoveDamping;
                scroll += scroll_vel;
                const auto hl_target = static_cast<double>(nav.GetSelected());
                hl_vel += (hl_target - hl_row) * MoveStiffness;
                hl_vel *= MoveDamping;
                hl_row += hl_vel;
                pop_vel += -pop * 0.22;
                pop_vel *= 0.62;
                pop += pop_vel;

                const auto a = std::clamp(appear, 0.0, 1.0);
                const auto fa = 255.0 * a;
                const auto card_y = base_card_y + static_cast<s32>((1.0 - std::min(appear, 1.04)) * SlideDistance);

                // ---- Dim + card ----
                drawer->RenderRectangleFill({ 0, 0, 0, static_cast<u8>(DimAlpha * a) }, 0, 0, ScreenW, ScreenH);
                drawer->RenderRoundedRectangleFill(WithAlpha({ 0, 0, 0, 0xFF }, 90 * a), card_x - 6, card_y + 4, card_w + 12, card_h + 14, Radius + 6);
                drawer->RenderRoundedRectangleFill(WithAlpha(style.card_clr, 250 * a), card_x, card_y, card_w, card_h, Radius);
                drawer->RenderRoundedRectangle(WithAlpha({ 0xFF, 0xFF, 0xFF, 0xFF }, 16 * a), card_x, card_y, card_w, card_h, Radius);
                // Accent strip along the top edge
                drawer->RenderRoundedRectangleFill(WithAlpha(style.accent_clr, fa), card_x + Padding, card_y, 132, 5, 2);

                // ---- Header ----
                auto hx = card_x + Padding;
                const auto hy = card_y + Padding;
                if(has_icon) {
                    drawer->RenderTexture(icon->Get(), hx, hy, pu::ui::render::TextureRenderOptions(static_cast<u8>(fa), IconSize, IconSize, {}, {}, {}));
                    hx += IconSize + 28;
                }
                if(title_tex != nullptr) {
                    drawer->RenderTexture(title_tex, hx, hy + (header_h - GetTextureHeight(title_tex)) / 2, Alpha(fa));
                }
                if(cnt_tex != nullptr) {
                    drawer->RenderTexture(cnt_tex, card_x + Padding, hy + header_h + HeaderContentGap, Alpha(fa * 0.74));
                }

                // ---- Rows ----
                const auto rows_y = card_y + top_part;
                const auto first = static_cast<s32>(std::floor(scroll));
                for(s32 i = std::max(0, first - 1); i < std::min(opt_count, first + visible + 1); i++) {
                    const auto ry = rows_y + static_cast<s32>((i - scroll) * row_step);
                    // Fade rows that scroll past the list edges
                    double edge = 1.0;
                    if(ry < rows_y) {
                        edge = 1.0 - static_cast<double>(rows_y - ry) / RowHeight;
                    }
                    else if(ry + RowHeight > rows_y + rows_h) {
                        edge = 1.0 - static_cast<double>(ry + RowHeight - (rows_y + rows_h)) / RowHeight;
                    }
                    edge = std::clamp(edge, 0.0, 1.0);
                    if(edge <= 0.01) {
                        continue;
                    }
                    drawer->RenderRoundedRectangleFill(WithAlpha({ 0xFF, 0xFF, 0xFF, 0xFF }, 11 * a * edge), rows_x, ry, rows_w, RowHeight, RowRadius);
                }

                // Highlight: glides between rows, pops on each move
                const auto hl_y = rows_y + static_cast<s32>((hl_row - scroll) * row_step);
                const auto grow = static_cast<s32>(pop);
                if((hl_y + RowHeight > rows_y - 8) && (hl_y < rows_y + rows_h + 8)) {
                    drawer->RenderRoundedRectangleFill(WithAlpha(style.accent_clr, 55 * a), rows_x - 6 - grow, hl_y - 6 - grow, rows_w + 12 + 2 * grow, RowHeight + 12 + 2 * grow, RowRadius + 6);
                    drawer->RenderRoundedRectangleFill(WithAlpha(style.accent_clr, fa), rows_x - grow, hl_y - grow, rows_w + 2 * grow, RowHeight + 2 * grow, RowRadius);
                }

                const auto selected = static_cast<s32>(nav.GetSelected());
                for(s32 i = std::max(0, first - 1); i < std::min(opt_count, first + visible + 1); i++) {
                    const auto ry = rows_y + static_cast<s32>((i - scroll) * row_step);
                    double edge = 1.0;
                    if(ry < rows_y) {
                        edge = 1.0 - static_cast<double>(rows_y - ry) / RowHeight;
                    }
                    else if(ry + RowHeight > rows_y + rows_h) {
                        edge = 1.0 - static_cast<double>(ry + RowHeight - (rows_y + rows_h)) / RowHeight;
                    }
                    edge = std::clamp(edge, 0.0, 1.0);
                    if(edge <= 0.01) {
                        continue;
                    }
                    // Text switches to dark once the highlight covers most of the row
                    const auto covered = (i == selected) && (std::abs(hl_row - i) < 0.5);
                    const auto &r = rows.at(i);
                    auto label = covered ? r.label_focus : r.label;
                    const auto lx = rows_x + RowTextMargin + ((i == selected) ? 6 : 0);
                    drawer->RenderTexture(label, lx, ry + (RowHeight - GetTextureHeight(label)) / 2, Alpha(fa * edge));
                    auto detail = covered ? r.detail_focus : r.detail;
                    if(detail != nullptr) {
                        drawer->RenderTexture(detail, rows_x + rows_w - RowTextMargin - GetTextureWidth(detail), ry + (RowHeight - GetTextureHeight(detail)) / 2, Alpha(fa * edge * (covered ? 0.85 : 0.6)));
                    }
                }

                // Scrollbar when the list is longer than the card
                if(nav.IsScrollable()) {
                    const auto track_x = card_x + card_w - 22;
                    drawer->RenderRoundedRectangleFill(WithAlpha({ 0xFF, 0xFF, 0xFF, 0xFF }, 22 * a), track_x, rows_y, 5, rows_h, 2);
                    const auto thumb_h = std::max(36, rows_h * visible / opt_count);
                    const auto max_first = std::max(1, opt_count - visible);
                    const auto thumb_y = rows_y + static_cast<s32>((rows_h - thumb_h) * std::clamp(scroll / max_first, 0.0, 1.0));
                    drawer->RenderRoundedRectangleFill(WithAlpha(style.accent_clr, fa), track_x, thumb_y, 5, thumb_h, 2);
                }

                // ---- Footer hints ----
                const auto foot_cy = card_y + card_h - FooterHeight / 2 - 6;
                drawer->RenderRectangleFill(WithAlpha({ 0xFF, 0xFF, 0xFF, 0xFF }, 14 * a), card_x + Padding - 16, card_y + card_h - FooterHeight - 12, card_w - 2 * (Padding - 16), 1);
                auto hint_right = card_x + card_w - Padding;
                hint_right -= RenderHint(drawer, hint_b, hint_back, hint_right, foot_cy, style.text_clr, fa) + 34;
                RenderHint(drawer, hint_a, hint_select, hint_right, foot_cy, style.text_clr, fa);

                return !(closing && (appear <= 0.0));
            });

            if(!ok) {
                app->CallForRenderWithRenderOver([](pu::ui::render::Renderer::Ref&) -> bool {
                    return false;
                });
                break;
            }
        }

        haptics.Stop();
        pu::audio::DestroySfx(move_sfx);

        // Same meaning as pu::ui::Application::CreateShowDialog
        if(backed_out) {
            return -1;
        }
        if(use_last_opt_as_cancel && (result == opt_count - 1)) {
            return -2;
        }
        return result;
    }

}
