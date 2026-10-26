
#pragma once
#include <ul/menu/ui/ui_IMenuLayout.hpp>
#include <ul/menu/ui/ui_HoverMedia.hpp>
#include <ul/menu/ui/ui_UnlockPrompt.hpp>
#include <ul/menu/ui/ui_Haptics.hpp>

namespace ul::menu::ui {

    class LockscreenMenuLayout : public IMenuLayout {
        private:
            pu::ui::elm::TextBlock::Ref info_text;
            pu::ui::elm::Image::Ref connection_top_icon;
            MultiTextBlock::Ref time_mtext;
            pu::ui::elm::TextBlock::Ref date_text;
            pu::ui::elm::TextBlock::Ref battery_text;
            pu::ui::elm::Image::Ref battery_top_icon;
            pu::ui::elm::Image::Ref battery_charging_top_icon;
            // steamui: show the suspended game and resume it straight from the lock screen
            bool resume_mode;
            bool resuming;
            HoverMedia::Ref game_art;
            UnlockPrompt::Ref unlock_prompt;
            Haptics haptics;
            pu::audio::Sfx resume_sfx;
            pu::audio::Sfx press_sfx;
            bool has_game;
            void RefreshPrompt();
            void OnUnlocked();
            void StartResume();

        public:
            LockscreenMenuLayout();
            PU_SMART_CTOR(LockscreenMenuLayout)
            // Called when the lock screen is about to be shown (picks up the current suspended game)
            void Prepare();
            
            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            bool OnHomeButtonPress() override;
            void LoadSfx() override;
            void DisposeSfx() override;
    };

}
