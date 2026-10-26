#include <ul/menu/ui/ui_StartupMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/acc/acc_Accounts.hpp>
#include <ul/menu/smi/smi_Commands.hpp>

extern ul::menu::ui::GlobalSettings g_GlobalSettings;
extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

namespace ul::menu::ui {

    void StartupMenuLayout::user_DefaultKey(const AccountUid uid) {
        // Note: menu loading is invoked below instead of here so that the main menu doesn't also register the button input which caused this action...
        this->load_menu = true;
        pu::audio::PlaySfx(this->user_select_sfx);
        g_GlobalSettings.SetSelectedUser(uid);

        auto &main_menu_lyt = g_MenuApplication->GetMainMenuLayout();
        if(main_menu_lyt != nullptr) {
            main_menu_lyt->NotifyNextReloadUserChanged();
        }
        g_MenuApplication->LoadMenu(MenuType::Main);
    }

    void StartupMenuLayout::create_DefaultKey() {
        pu::audio::PlaySfx(this->user_create_sfx);

        g_MenuApplication->FadeOutToLibraryApplet(AppletId_LibraryAppletMyPage);
        UL_RC_ASSERT(smi::OpenAddUser());
        g_MenuApplication->Finalize();
    }

    StartupMenuLayout::StartupMenuLayout() : IMenuLayout() {
        this->load_menu = false;

        this->user_create_sfx = nullptr;
        this->user_select_sfx = nullptr;

        this->info_text = pu::ui::elm::TextBlock::New(0, 0, GetLanguageString("startup_welcome_info"));
        this->info_text->SetColor(g_MenuApplication->GetTextColor());
        g_GlobalSettings.ApplyConfigForElement("startup_menu", "info_text", this->info_text);
        this->Add(this->info_text);

        this->users_menu = pu::ui::elm::Menu::New(0, 0, UsersMenuWidth, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), UsersMenuItemSize, UsersMenuItemsToShow);
        g_GlobalSettings.ApplyConfigForElement("startup_menu", "users_menu", this->users_menu);
        this->Add(this->users_menu);

        // steamui: console-style profile picker, if the theme asks for it
        this->user_move_sfx = nullptr;
        util::JSON carousel_json;
        if(TryGetUiElement("startup_menu", "user_carousel", carousel_json) && carousel_json.value("enabled", false)) {
            this->user_carousel = UserCarousel::New(carousel_json.value("y", 520), carousel_json.value("avatar_size", 256), carousel_json.value("spacing", 340), TryFindLoadImageHandle("ui/Main/OverIcon/Cursor"), g_MenuApplication->GetTextColor(), [&]() {
                pu::audio::PlaySfx(this->user_move_sfx);
                this->haptics.Play(Haptics::Kind::Tick);
            });
            this->Add(this->user_carousel);
            this->users_menu->SetVisible(false);

            util::JSON haptics_json;
            this->haptics.Initialize(TryGetUiElement("main_menu", "haptics", haptics_json) && haptics_json.value("enabled", false));
        }

        this->Add(GetScreenCaptureBackground());
    }

    void StartupMenuLayout::LoadSfx() {
        this->user_create_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Startup/UserCreate.wav"));
        this->user_select_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Startup/UserSelect.wav"));
        this->user_move_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/CursorMove.wav"));
    }
    
    void StartupMenuLayout::DisposeSfx() {
        pu::audio::DestroySfx(this->user_create_sfx);
        pu::audio::DestroySfx(this->user_select_sfx);
        pu::audio::DestroySfx(this->user_move_sfx);
        this->haptics.Stop();
    }

    void StartupMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        UpdateScreenCaptureBackground(false);
        this->haptics.Update();
    }

    bool StartupMenuLayout::OnHomeButtonPress() {
        // ...
        return true;
    }

    void StartupMenuLayout::ReloadMenu() {
        this->users_menu->ClearItems();
        if(this->user_carousel != nullptr) {
            this->user_carousel->Clear();
        }

        std::vector<AccountUid> user_ids;
        UL_RC_ASSERT(acc::ListAccounts(user_ids));
        for(const auto &user_id: user_ids) {
            std::string name;
            if(R_SUCCEEDED(acc::GetAccountName(user_id, name))) {
                auto user_item = pu::ui::elm::MenuItem::New(name);

                u8 *user_icon_buf = nullptr;
                size_t user_icon_size = 0;
                UL_RC_ASSERT(acc::LoadAccountImage(user_id, user_icon_buf, user_icon_size));
                auto user_icon = pu::sdl2::TextureHandle::New(pu::ui::render::LoadImageFromBuffer(user_icon_buf, user_icon_size));
                delete[] user_icon_buf;
                user_item->SetIcon(user_icon);
                if(this->user_carousel != nullptr) {
                    this->user_carousel->AddUser(user_icon, name, std::bind(&StartupMenuLayout::user_DefaultKey, this, user_id));
                }

                user_item->AddOnKey(std::bind(&StartupMenuLayout::user_DefaultKey, this, user_id));
                user_item->SetColor(g_MenuApplication->GetTextColor());
                this->users_menu->AddItem(user_item);
            }
        }

        auto create_user_item = pu::ui::elm::MenuItem::New(GetLanguageString("startup_add_user"));
        create_user_item->SetColor(g_MenuApplication->GetTextColor());
        create_user_item->AddOnKey(std::bind(&StartupMenuLayout::create_DefaultKey, this));
        this->users_menu->AddItem(create_user_item);
        if(this->user_carousel != nullptr) {
            this->user_carousel->AddCreateUser(GetLanguageString("startup_add_user"), std::bind(&StartupMenuLayout::create_DefaultKey, this));
        }

        this->users_menu->SetSelectedIndex(0);
    }

}
