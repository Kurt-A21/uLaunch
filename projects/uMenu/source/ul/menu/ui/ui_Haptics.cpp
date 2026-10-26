#include <ul/menu/ui/ui_Haptics.hpp>

namespace ul::menu::ui {

    namespace {

        struct HapticPulse {
            HidVibrationValue value;
            s64 duration_ms;
        };

        constexpr HidVibrationValue StopValue = { 0.0f, 160.0f, 0.0f, 320.0f };

        constexpr HapticPulse GetPulse(const Haptics::Kind kind) {
            switch(kind) {
                case Haptics::Kind::Tick:
                    return { { 0.10f, 170.0f, 0.32f, 340.0f }, 18 };
                case Haptics::Kind::Thump:
                    return { { 0.35f, 120.0f, 0.20f, 240.0f }, 40 };
                case Haptics::Kind::Press:
                    return { { 0.45f, 140.0f, 0.40f, 300.0f }, 70 };
            }
            return { StopValue, 0 };
        }

    }

    void Haptics::Initialize(const bool enable) {
        this->enabled = false;
        this->handle_count = 0;
        if(!enable) {
            return;
        }

        // Handheld (2 Joy-Con), player 1 Joy-Con pair (2), player 1 Pro Controller (1)
        if(R_SUCCEEDED(hidInitializeVibrationDevices(this->handles + this->handle_count, 2, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld))) {
            this->handle_count += 2;
        }
        if(R_SUCCEEDED(hidInitializeVibrationDevices(this->handles + this->handle_count, 2, HidNpadIdType_No1, HidNpadStyleTag_NpadJoyDual))) {
            this->handle_count += 2;
        }
        if(R_SUCCEEDED(hidInitializeVibrationDevices(this->handles + this->handle_count, 1, HidNpadIdType_No1, HidNpadStyleTag_NpadFullKey))) {
            this->handle_count += 1;
        }

        this->enabled = this->handle_count > 0;
    }

    void Haptics::Send(const HidVibrationValue &value) {
        HidVibrationValue values[MaxHandles];
        for(size_t i = 0; i < this->handle_count; i++) {
            values[i] = value;
        }
        // Disconnected controllers just fail; nothing to handle
        hidSendVibrationValues(this->handles, values, static_cast<s32>(this->handle_count));
    }

    void Haptics::Play(const Kind kind) {
        if(!this->enabled) {
            return;
        }

        const auto pulse = GetPulse(kind);
        this->Send(pulse.value);
        this->active = true;
        this->stop_tp = std::chrono::steady_clock::now() + std::chrono::milliseconds(pulse.duration_ms);
    }

    void Haptics::Update() {
        if(this->active && (std::chrono::steady_clock::now() >= this->stop_tp)) {
            this->Stop();
        }
    }

    void Haptics::Stop() {
        if(this->enabled && this->active) {
            this->Send(StopValue);
        }
        this->active = false;
    }

}
