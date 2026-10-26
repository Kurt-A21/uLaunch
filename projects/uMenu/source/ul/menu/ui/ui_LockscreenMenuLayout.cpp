#include <ul/menu/ui/ui_LockscreenMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/menu/ui/ui_BackgroundScreenCapture.hpp>
#include <ul/menu/smi/smi_Commands.hpp>

extern ul::menu::ui::GlobalSettings g_GlobalSettings;
extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

namespace ul::menu::ui {

    namespace {

        inline void UnlockScreen() {
            if(accountUidIsValid(&g_GlobalSettings.system_status.selected_user)) {
                g_MenuApplication->LoadMenu(MenuType::Main);
            }
            else {
                g_MenuApplication->LoadMenu(MenuType::Startup);
            }
        }

    }

    LockscreenMenuLayout::LockscreenMenuLayout() : IMenuLayout(), resume_mode(false), resuming(false), resume_sfx(nullptr) {
        // steamui: resume card mode (theme opt-in). Art + frozen game frame go below everything else.
        util::JSON card_json;
        if(TryGetUiElement("lockscreen_menu", "resume_card", card_json) && card_json.value("enabled", false)) {
            this->resume_mode = true;
            this->game_art = HoverMedia::New();
            this->game_art->SetSoundEnabled(false);
            this->Add(this->game_art);
            this->Add(GetScreenCaptureBackground());
            this->resume_card = ResumeCard::New(card_json.value("x", (static_cast<s32>(pu::ui::render::ScreenWidth) - ResumeCard::Width) / 2), card_json.value("y", 760));
            util::JSON haptics_json;
            this->haptics.Initialize(TryGetUiElement("main_menu", "haptics", haptics_json) && haptics_json.value("enabled", false));
        }

        this->info_text = pu::ui::elm::TextBlock::New(0, 0, GetLanguageString("lockscreen_info"));
        this->info_text->SetColor(g_MenuApplication->GetTextColor());
        g_GlobalSettings.ApplyConfigForElement("lockscreen_menu", "info_text", this->info_text);
        this->Add(this->info_text);

        this->connection_top_icon = pu::ui::elm::Image::New(0, 0, TryFindLoadImageHandle("ui/Main/TopIcon/Connection/None"));
        g_GlobalSettings.ApplyConfigForElement("lockscreen_menu", "connection_top_icon", this->connection_top_icon);
        this->Add(this->connection_top_icon);

        this->InitializeTimeText(this->time_mtext, "lockscreen_menu", "time_text");

        this->date_text = pu::ui::elm::TextBlock::New(0, 0, "...");
        this->date_text->SetColor(g_MenuApplication->GetTextColor());
        g_GlobalSettings.ApplyConfigForElement("lockscreen_menu", "date_text", this->date_text);
        this->Add(this->date_text);

        this->battery_text = pu::ui::elm::TextBlock::New(0, 0, "...");
        this->battery_text->SetColor(g_MenuApplication->GetTextColor());
        g_GlobalSettings.ApplyConfigForElement("lockscreen_menu", "battery_text", this->battery_text);
        this->Add(this->battery_text);

        this->battery_top_icon = pu::ui::elm::Image::New(0, 0, TryFindLoadImageHandle("ui/Main/TopIcon/Battery/100"));
        this->battery_charging_top_icon = pu::ui::elm::Image::New(0, 0, TryFindLoadImageHandle("ui/Main/TopIcon/Battery/Charging"));
        this->battery_charging_top_icon->SetVisible(false);
        g_GlobalSettings.ApplyConfigForElement("lockscreen_menu", "battery_top_icon", this->battery_top_icon);
        g_GlobalSettings.ApplyConfigForElement("lockscreen_menu", "battery_top_icon", this->battery_charging_top_icon);
        this->Add(this->battery_top_icon);
        this->Add(this->battery_charging_top_icon);

        if(this->resume_mode) {
            this->Add(this->resume_card);
        }
    }

    void LockscreenMenuLayout::Prepare() {
        this->resuming = false;
        if(this->resume_mode) {
            this->RefreshResumeCard();
        }
    }

    void LockscreenMenuLayout::RefreshResumeCard() {
        // Which game was running when the console went to sleep?
        std::string name;
        std::string art_key;
        pu::sdl2::TextureHandle::Ref icon;
        if(g_GlobalSettings.IsTitleSuspended()) {
            const auto app_id = g_GlobalSettings.system_status.suspended_app_id;
            name = LoadApplicationName(app_id);
            if(name.empty()) {
                name = "your game";
            }
            icon = LoadApplicationIconTexture(app_id);
            art_key = HoverMedia::MakeApplicationKey(app_id);
        }
        else if(g_GlobalSettings.IsHomebrewSuspended()) {
            art_key = HoverMedia::MakeHomebrewKey(g_GlobalSettings.system_status.suspended_hb_target_ipt.nro_path);
            name = art_key;
            icon = TryFindLoadImageHandle("ui/Main/EntryIcon/DefaultHomebrew");
        }

        this->resume_card->SetGame(icon, name, g_MenuApplication->GetTextColor());
        this->game_art->SetTarget(art_key);
        // Without a suspended game, keep the classic "press any key" text
        this->info_text->SetVisible(!this->resume_card->HasGame());
    }

    void LockscreenMenuLayout::StartResume() {
        this->resuming = true;
        this->resume_card->Hide();
        this->game_art->StopSound();
        this->haptics.Play(Haptics::Kind::Press);
        pu::audio::PlaySfx(this->resume_sfx);
        // Same transition the home menu uses: the frozen game frame fades to full, then the game resumes
        RequestResumeScreenCaptureBackground();
    }

    void LockscreenMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(this->resume_mode) {
            this->haptics.Update();
            // Keep the suspended game's frame dimmed behind the card (or fading in while resuming)
            UpdateScreenCaptureBackground(true);
            if(this->resuming) {
                return;
            }
            if((keys_down != 0) || !touch_pos.IsEmpty()) {
                if(this->resume_card->HasGame() && g_GlobalSettings.IsSuspended()) {
                    this->StartResume();
                }
                else {
                    UnlockScreen();
                }
                return;
            }
        }
        else if(keys_down != 0) {
            UnlockScreen();
        }

        this->UpdateConnectionTopIcon(this->connection_top_icon);
        this->UpdateTimeText(this->time_mtext);
        this->UpdateDateText(this->date_text);
        this->UpdateBatteryTextAndTopIcons(this->battery_text, this->battery_top_icon, this->battery_charging_top_icon);
    }

    bool LockscreenMenuLayout::OnHomeButtonPress() {
        // HOME always goes to the home screen (the game stays suspended there)
        if(!this->resuming) {
            UnlockScreen();
        }
        return true;
    }

    void LockscreenMenuLayout::LoadSfx() {
        this->resume_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/ResumeApplication.wav"));
    }

    void LockscreenMenuLayout::DisposeSfx() {
        pu::audio::DestroySfx(this->resume_sfx);
        if(this->game_art != nullptr) {
            this->game_art->StopSound();
        }
        this->haptics.Stop();
    }

}
