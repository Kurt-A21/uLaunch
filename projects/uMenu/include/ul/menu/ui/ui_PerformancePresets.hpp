
#pragma once
#include <ul/menu/sysclk_Include.hpp>
#include <string>

namespace ul::menu::ui::perf {

    // steamui: overclock presets applied through sys-clk. Values in MHz, 0 = stock.
    // Must stay identical to PRESETS in steamui/tools/oc_profile.py (checked by the tests).
    enum class Preset : u32 {
        Stock,
        Battery,
        Balanced,
        Performance,
        Count
    };

    constexpr s32 CustomPreset = -1;

    const char *GetPresetName(const Preset preset);
    const char *GetPresetDescription(const Preset preset);
    void FillProfiles(const Preset preset, SysClkTitleProfileList &out_profiles);

    // Which preset a game's current sys-clk profile corresponds to (CustomPreset if hand-tuned)
    s32 DetectPreset(const SysClkTitleProfileList &profiles);

    // e.g. "CPU 1020 · GPU 384 · RAM 1331 MHz\nSoC 45.2°C · Board 38.0°C · Skin 35.1°C · Docked"
    std::string FormatStatus(const SysClkContext &context);

}
