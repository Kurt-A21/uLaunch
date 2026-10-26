#include <ul/menu/ui/ui_QuickMenu.hpp>
#include <algorithm>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/cfg/cfg_Config.hpp>
#include <ul/acc/acc_Accounts.hpp>

extern ul::menu::ui::GlobalSettings g_GlobalSettings;
extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

namespace ul::menu::ui {

    namespace {

        std::atomic_bool g_HomePressed = false;

    }

    void QuickMenu::OnHomeButtonDetection(const smi::MenuMessageContext &_) {
        g_HomePressed = true;
    }

    QuickMenu::QuickMenu() {
        this->on = false;
        this->bg_alpha = 0;
        this->bg_alpha_max = BackgroundAlphaMax;
        this->slide_down = false;
        this->slide_h = 0;

        // steamui: the theme can size the list and turn it into a slide-down dropdown
        s32 menu_width = MenuWidth;
        s32 item_height = MenuItemHeight;
        s32 items_to_show = MenuItemsToShow;
        util::JSON qm_json;
        if(TryGetUiElement("quick_menu", "quick_menu", qm_json)) {
            menu_width = qm_json.value("width", menu_width);
            item_height = qm_json.value("item_height", item_height);
            items_to_show = qm_json.value("items_to_show", items_to_show);
            this->slide_down = qm_json.value("slide_down", false);
            this->bg_alpha_max = std::clamp<s32>(qm_json.value("dim_alpha", this->bg_alpha_max), 0, 0xFF);
        }

        this->options_menu = pu::ui::elm::Menu::New(MenuX, MenuY, menu_width, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), item_height, items_to_show);
        g_GlobalSettings.ApplyConfigForElement("quick_menu", "quick_menu", this->options_menu);

        this->power_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_power_options"));
        this->power_menu_item->SetIcon(TryFindLoadImageHandle("ui/Main/QuickIcon/Power"));
        this->power_menu_item->AddOnKey(&ShowPowerDialog);
        this->power_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->power_menu_item);

        this->controller_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_controller_options"));
        this->controller_menu_item->SetIcon(TryFindLoadImageHandle("ui/Main/QuickIcon/Controllers"));
        this->controller_menu_item->AddOnKey(&ShowController);
        this->controller_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->controller_menu_item);

        this->album_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_album"));
        this->album_menu_item->SetIcon(TryFindLoadImageHandle("ui/Main/QuickIcon/Album"));
        this->album_menu_item->AddOnKey(&ShowAlbum);
        this->album_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->album_menu_item);

        this->web_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_web_page"));
        this->web_menu_item->SetIcon(TryFindLoadImageHandle("ui/Main/QuickIcon/WebBrowser"));
        this->web_menu_item->AddOnKey(&ShowWebPage);
        this->web_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->web_menu_item);

        this->user_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_user_menu"));
        this->user_menu_item->SetIcon(nullptr); // Will be later set, when a user is actually selected
        this->user_menu_item->AddOnKey(&ShowUserPage);
        this->user_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->user_menu_item);

        this->themes_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_themes_menu"));
        this->themes_menu_item->SetIcon(TryFindLoadImageHandle("ui/Main/QuickIcon/Themes"));
        this->themes_menu_item->AddOnKey(&ShowThemesMenu);
        this->themes_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->themes_menu_item);

        this->settings_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_settings_menu"));
        this->settings_menu_item->SetIcon(TryFindLoadImageHandle("ui/Main/QuickIcon/Settings"));
        this->settings_menu_item->AddOnKey(&ShowSettingsMenu);
        this->settings_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->settings_menu_item);

        this->mii_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_mii_edit"));
        this->mii_menu_item->SetIcon(TryFindLoadImageHandle("ui/Main/QuickIcon/MiiEdit"));
        this->mii_menu_item->AddOnKey(&ShowMiiEdit);
        this->mii_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->mii_menu_item);

        this->amiibo_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_amiibo"));
        this->amiibo_menu_item->SetIcon(TryFindLoadImageHandle("ui/Main/QuickIcon/Amiibo"));
        this->amiibo_menu_item->AddOnKey(&ShowCabinet);
        this->amiibo_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->amiibo_menu_item);
    }

    void QuickMenu::Toggle() {
        this->on = !this->on;

        if(this->slide_down) {
            if(this->on) {
                this->options_menu->SetSelectedIndex(0);
                this->slide_h_incr.Start(SlideSteps, this->slide_h, this->options_menu->GetHeight() - this->slide_h);
            }
            else {
                this->slide_h_incr.Start(SlideSteps, this->slide_h, -this->slide_h);
            }
        }
    }

    void QuickMenu::RenderSlideDown(pu::ui::render::Renderer::Ref &drawer) {
        this->slide_h_incr.Increment(this->slide_h);
        if(this->slide_h <= 0) {
            return;
        }

        const auto menu_x = this->options_menu->GetProcessedX();
        const auto menu_y = this->options_menu->GetProcessedY();
        const auto menu_w = this->options_menu->GetWidth();
        const auto menu_h = this->options_menu->GetHeight();
        const auto visible_h = std::min(this->slide_h, menu_h);

        drawer->RenderRoundedRectangleFill(PanelColor, menu_x - PanelPadding, menu_y - PanelPadding, menu_w + 2 * PanelPadding, visible_h + 2 * PanelPadding, PanelRadius);

        // The list slides down from under the burger: only the revealed part is drawn
        const SDL_Rect clip = { menu_x, menu_y, menu_w, visible_h };
        SDL_RenderSetClipRect(pu::ui::render::GetMainRenderer(), &clip);
        this->options_menu->OnRender(drawer, menu_x, menu_y - (menu_h - visible_h));
        SDL_RenderSetClipRect(pu::ui::render::GetMainRenderer(), nullptr);
    }

    void QuickMenu::UpdateItems() {
        this->user_menu_item->SetIcon(GetSelectedUserIconTexture());
    }

    void QuickMenu::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        if(!this->on) {
            if(this->bg_alpha > 0) {
                this->bg_alpha -= BackgroundAlphaIncrement;
                if(this->bg_alpha < 0) {
                    this->bg_alpha = 0;
                }
            }
        }
        else {
            if(this->bg_alpha < this->bg_alpha_max) {
                this->bg_alpha += BackgroundAlphaIncrement;
                if(this->bg_alpha > this->bg_alpha_max) {
                    this->bg_alpha = this->bg_alpha_max;
                }
            }
        }
        this->options_menu->SetVisible(this->bg_alpha != 0);

        if(this->slide_down) {
            drawer->RenderRectangleFill(MakeDimColor(static_cast<u8>(this->bg_alpha)), this->GetX(), this->GetY(), this->GetWidth(), this->GetHeight());
            this->RenderSlideDown(drawer);
            return;
        }

        drawer->RenderRectangleFill(MakeBackgroundColor(static_cast<u8>(this->bg_alpha)), this->GetX(), this->GetY(), this->GetWidth(), this->GetHeight());

        if(this->bg_alpha > 0) {
            if(this->bg_alpha < this->bg_alpha_max) {
                drawer->SetBaseRenderAlpha(static_cast<u8>(this->bg_alpha));
            }
            this->options_menu->OnRender(drawer, this->options_menu->GetProcessedX(), this->options_menu->GetProcessedY());
            if(this->bg_alpha < this->bg_alpha_max) {
                drawer->ResetBaseRenderAlpha();
            }
        }
    }

    void QuickMenu::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(this->on && this->slide_down && !touch_pos.IsEmpty()) {
            // steamui: tapping below the burger but outside the list closes it
            const auto panel_x = this->options_menu->GetProcessedX() - PanelPadding;
            const auto panel_y = this->options_menu->GetProcessedY() - PanelPadding;
            const auto panel_w = this->options_menu->GetWidth() + 2 * PanelPadding;
            const auto panel_h = this->options_menu->GetHeight() + 2 * PanelPadding;
            if((touch_pos.y >= panel_y) && !touch_pos.HitsRegion(panel_x, panel_y, panel_w, panel_h)) {
                this->Toggle();
                return;
            }
        }

        if(this->on) {
            this->options_menu->OnInput(keys_down, keys_up, keys_held, touch_pos);
        }

        if((keys_down & HidNpadButton_ZL) || (keys_down & HidNpadButton_ZR)) {
            this->Toggle();
        }
        else if((keys_down & HidNpadButton_B) || (keys_down & HidNpadButton_A)) {
            // B only valid for toggling off
            // A = something selected in the menu
            if(this->on) {
                this->Toggle();
            }
        }
        else {
            if(g_HomePressed) {
                if(this->on) {
                    this->Toggle();
                }
                
                g_HomePressed = false;
            }
        }
    }

}
