
#pragma once
#include <switch.h>
#include <chrono>

namespace ul::menu::ui {

    // steamui: short HD Rumble "ticks" for a tactile, console-style feel (cursor moves, menu open/close).
    // Sent to handheld Joy-Con, player-1 Joy-Con pair and player-1 Pro Controller; unconnected ones are ignored.
    class Haptics {
        public:
            enum class Kind {
                Tick,   // cursor moves: crisp and light
                Thump,  // opening/closing the burger menu
                Press   // launching a game
            };

        private:
            static constexpr size_t MaxHandles = 5;
            HidVibrationDeviceHandle handles[MaxHandles];
            size_t handle_count;
            bool enabled;
            bool active;
            std::chrono::steady_clock::time_point stop_tp;

            void Send(const HidVibrationValue &value);

        public:
            Haptics() : handles(), handle_count(0), enabled(false), active(false), stop_tp() {}

            void Initialize(const bool enable);
            void Play(const Kind kind);

            // Call once per frame: stops the vibration once the pulse ends
            void Update();
            void Stop();
    };

}
