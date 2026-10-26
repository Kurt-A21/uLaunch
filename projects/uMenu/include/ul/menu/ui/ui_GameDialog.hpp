
#pragma once
#include <pu/Plutonium>
#include <ul/menu/ui/ui_DialogNav.hpp>
#include <string>
#include <vector>

namespace ul::menu::ui {

    // steamui: console-style replacement for Plutonium's dialog (enabled by the theme: "dialog": {"game_dialog": {"enabled": true}}).
    //  - dark card that springs up over a dimmed screen
    //  - options as a vertical list; the highlight glides between rows and pops on every move
    //  - each option can carry a short detail shown on its right (e.g. what a preset does)
    //  - A/B hints at the bottom, HD Rumble tick per move, tap a row to pick it, tap outside to go back
    // Return values match pu::ui::Application::CreateShowDialog: the option index, -1 = backed out with B, -2 = the cancel option.
    struct DialogOption {
        std::string label;
        std::string detail;
    };

    struct GameDialogStyle {
        pu::ui::Color card_clr;
        pu::ui::Color accent_clr;
        pu::ui::Color title_clr;
        pu::ui::Color text_clr;
        bool haptics;
    };

    class GameDialog {
        public:
            static constexpr s32 MinWidth = 860;
            static constexpr s32 MaxWidth = 1480;
            static constexpr s32 Padding = 56;
            static constexpr s32 Radius = 22;
            static constexpr s32 IconSize = 96;
            static constexpr s32 RowHeight = 76;
            static constexpr s32 RowGap = 10;
            static constexpr s32 RowRadius = 14;
            static constexpr s32 FooterHeight = 64;
            static constexpr s32 SlideDistance = 56;
            static constexpr u8 DimAlpha = 185;

            static constexpr double OpenStiffness = 0.16;
            static constexpr double OpenDamping = 0.70;
            static constexpr double CloseStep = 0.14;
            static constexpr double MoveStiffness = 0.24;
            static constexpr double MoveDamping = 0.64;

            static bool IsEnabledByTheme();

            static s32 Show(pu::ui::Application *app, const GameDialogStyle &style, const std::string &title, const std::string &content, const std::vector<DialogOption> &opts, const bool use_last_opt_as_cancel, pu::sdl2::TextureHandle::Ref icon = {});
    };

}
