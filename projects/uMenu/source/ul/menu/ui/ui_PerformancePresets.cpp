#include <ul/menu/ui/ui_PerformancePresets.hpp>
#include <cstdio>
#include <cstring>

namespace ul::menu::ui::perf {

    namespace {

        struct Clocks {
            u32 cpu;
            u32 gpu;
            u32 mem;
        };

        struct PresetDef {
            const char *name;
            const char *desc;
            // indexed by SysClkProfile: Handheld, HandheldCharging, HandheldChargingUSB, HandheldChargingOfficial, Docked
            Clocks clocks[SysClkProfile_EnumMax];
        };

        constexpr PresetDef Presets[static_cast<u32>(Preset::Count)] = {
            { "Stock", "Nintendo's normal clocks", {} },
            { "Battery", "Underclocked in handheld for longer battery", {
                { 816, 307, 1065 }, {}, {}, {}, {}
            } },
            { "Balanced", "Faster than stock, cooler than max", {
                { 1224, 460, 1600 }, { 1428, 768, 1600 }, {}, {}, { 1428, 844, 1600 }
            } },
            { "Performance", "Maximum safe clocks (more heat, less battery)", {
                { 1785, 614, 1600 }, { 1785, 768, 1600 }, {}, { 1785, 921, 1600 }, { 1785, 921, 1600 }
            } },
        };

        inline void ToProfiles(const PresetDef &def, SysClkTitleProfileList &out) {
            std::memset(&out, 0, sizeof(out));
            for(u32 p = 0; p < SysClkProfile_EnumMax; p++) {
                out.mhzMap[p][SysClkModule_CPU] = def.clocks[p].cpu;
                out.mhzMap[p][SysClkModule_GPU] = def.clocks[p].gpu;
                out.mhzMap[p][SysClkModule_MEM] = def.clocks[p].mem;
            }
        }

        inline const char *ProfileLabel(const SysClkProfile profile) {
            switch(profile) {
                case SysClkProfile_Handheld: return "Handheld";
                case SysClkProfile_HandheldCharging:
                case SysClkProfile_HandheldChargingUSB: return "Handheld, charging";
                case SysClkProfile_HandheldChargingOfficial: return "Handheld, official charger";
                case SysClkProfile_Docked: return "Docked";
                default: return "";
            }
        }

    }

    const char *GetPresetName(const Preset preset) {
        return (preset < Preset::Count) ? Presets[static_cast<u32>(preset)].name : "Custom";
    }

    const char *GetPresetDescription(const Preset preset) {
        return (preset < Preset::Count) ? Presets[static_cast<u32>(preset)].desc : "";
    }

    void FillProfiles(const Preset preset, SysClkTitleProfileList &out_profiles) {
        ToProfiles(Presets[static_cast<u32>((preset < Preset::Count) ? preset : Preset::Stock)], out_profiles);
    }

    s32 DetectPreset(const SysClkTitleProfileList &profiles) {
        for(u32 i = 0; i < static_cast<u32>(Preset::Count); i++) {
            SysClkTitleProfileList candidate;
            ToProfiles(Presets[i], candidate);
            if(std::memcmp(&candidate, &profiles, sizeof(candidate)) == 0) {
                return static_cast<s32>(i);
            }
        }
        return CustomPreset;
    }

    std::string FormatStatus(const SysClkContext &context) {
        char buf[256] = {};
        const auto t = context.temps;
        std::snprintf(buf, sizeof(buf), "Now: CPU %u · GPU %u · RAM %u MHz\n%u.%u°C SoC · %u.%u°C board · %s",
            context.freqs[SysClkModule_CPU] / 1000000, context.freqs[SysClkModule_GPU] / 1000000, context.freqs[SysClkModule_MEM] / 1000000,
            t[SysClkThermalSensor_SOC] / 1000, (t[SysClkThermalSensor_SOC] % 1000) / 100,
            t[SysClkThermalSensor_PCB] / 1000, (t[SysClkThermalSensor_PCB] % 1000) / 100,
            ProfileLabel(context.profile));
        return buf;
    }

}
