#include <ul/menu/ui/ui_PerformanceMenu.hpp>
#include <ul/menu/ui/ui_PerformancePresets.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

namespace ul::menu::ui {

    namespace {

        u64 g_TargetAppId = 0;
        std::string g_TargetName;
        bool g_TargetViaTakeover = false;

        constexpr const char Title[] = "Performance";

        // RAII session with the sys-clk sysmodule
        class SysClkSession {
            private:
                bool ok;
            public:
                SysClkSession() : ok(false) {
                    if(!sysclkIpcRunning()) {
                        return;
                    }
                    if(R_FAILED(sysclkIpcInitialize())) {
                        return;
                    }
                    u32 api_ver = 0;
                    if(R_FAILED(sysclkIpcGetAPIVersion(&api_ver)) || (api_ver != SYSCLK_IPC_API_VERSION)) {
                        sysclkIpcExit();
                        return;
                    }
                    this->ok = true;
                }

                ~SysClkSession() {
                    if(this->ok) {
                        sysclkIpcExit();
                    }
                }

                inline bool IsOk() {
                    return this->ok;
                }
        };

        std::string CurrentPresetLabel() {
            SysClkTitleProfileList profiles = {};
            if(R_FAILED(sysclkIpcGetProfiles(g_TargetAppId, &profiles))) {
                return "unknown";
            }
            const auto preset = perf::DetectPreset(profiles);
            return (preset == perf::CustomPreset) ? "Custom (set in the sys-clk overlay)" : perf::GetPresetName(static_cast<perf::Preset>(preset));
        }

        void ShowPresetPicker() {
            std::vector<DialogOption> opts;
            const auto current = CurrentPresetLabel();
            const std::string content = g_TargetName + (g_TargetViaTakeover ? " (runs as your take-over game)" : "") + "\nNow: " + current;
            for(u32 i = 0; i < static_cast<u32>(perf::Preset::Count); i++) {
                const auto preset = static_cast<perf::Preset>(i);
                opts.push_back({ perf::GetPresetName(preset), perf::GetPresetDescription(preset) });
            }
            opts.push_back({ "Cancel", "" });

            const auto option = g_MenuApplication->DisplayOptionsDialog(Title, content, opts, true);
            if((option < 0) || (option >= static_cast<s32>(perf::Preset::Count))) {
                return;
            }

            const auto preset = static_cast<perf::Preset>(option);
            SysClkTitleProfileList profiles;
            perf::FillProfiles(preset, profiles);
            if(R_SUCCEEDED(sysclkIpcSetProfiles(g_TargetAppId, &profiles))) {
                g_MenuApplication->ShowNotification(std::string(perf::GetPresetName(preset)) + " set for " + g_TargetName);
            }
            else {
                g_MenuApplication->ShowNotification("sys-clk couldn't save the profile");
            }
        }

    }

    void SetPerformanceTarget(const u64 app_id, const std::string &name, const bool via_takeover) {
        g_TargetAppId = app_id;
        g_TargetName = name.empty() ? "this game" : name;
        g_TargetViaTakeover = via_takeover;
    }

    void ShowPerformanceMenu() {
        SysClkSession session;
        if(!session.IsOk()) {
            g_MenuApplication->DisplayDialog(Title, "sys-clk isn't running (or is an incompatible version).\nInstall sys-clk 2.0.x and restart the console.", { "OK" }, true);
            return;
        }

        SysClkContext context = {};
        const auto has_context = R_SUCCEEDED(sysclkIpcGetCurrentContext(&context));
        const auto enabled = has_context && context.enabled;

        std::string content;
        if(g_TargetAppId != 0) {
            content = g_TargetName + ": " + CurrentPresetLabel() + "\n\n";
        }
        else if(g_TargetViaTakeover) {
            // A homebrew app is highlighted, but there's no take-over game for it to run as (and so no profile to set)
            content = g_TargetName + " runs as your take-over game, but none is set yet.\nPress X on a game you don't mind lending > Select as homebrew donor/take-over application.\n\n";
        }
        else {
            content = "Highlight a game on the home screen to set its overclock.\n\n";
        }
        if(has_context) {
            content += perf::FormatStatus(context);
        }
        if(!enabled) {
            content += "\nOverclocking is paused.";
        }

        std::vector<DialogOption> opts;
        s32 set_opt = -1;
        if(g_TargetAppId != 0) {
            set_opt = static_cast<s32>(opts.size());
            opts.push_back({ "Set preset", "For " + g_TargetName });
        }
        const auto toggle_opt = static_cast<s32>(opts.size());
        opts.push_back({ enabled ? "Pause overclocking" : "Resume overclocking", "All games" });
        opts.push_back({ "Close", "" });

        const auto option = g_MenuApplication->DisplayOptionsDialog(Title, content, opts, true);
        if(option == set_opt) {
            ShowPresetPicker();
        }
        else if(option == toggle_opt) {
            if(R_SUCCEEDED(sysclkIpcSetEnabled(!enabled))) {
                g_MenuApplication->ShowNotification(enabled ? "Overclocking paused" : "Overclocking resumed");
            }
        }
    }

}
