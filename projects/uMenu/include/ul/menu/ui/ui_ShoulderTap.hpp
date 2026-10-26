
#pragma once
#include <switch.h>

namespace ul::menu::ui {

    // steamui: ZL/ZR open the burger menu, but Ultrahand's overlay combo is ZL + ZR + D-pad Down.
    // So a shoulder button only counts as a "tap" when it's released without any other button having been
    // pressed while it was down (a chord such as the overlay combo never opens the menu).
    class ShoulderTap {
        public:
            static constexpr u64 TapButtons = HidNpadButton_ZL | HidNpadButton_ZR;
            // Real buttons only (A..Minus, D-pad, and the right Joy-Con's SL/SR etc.); stick tilts don't spoil a tap
            static constexpr u64 Buttons = 0xFFFFull | BITL(24) | BITL(25) | BITL(26) | BITL(27);

        private:
            bool armed;

        public:
            constexpr ShoulderTap() : armed(false) {}

            // Returns true on the frame a clean tap completes
            inline bool Feed(const u64 keys_down, const u64 keys_up, const u64 keys_held) {
                const auto tap_down = keys_down & TapButtons;
                const auto tap_held = keys_held & TapButtons;
                const auto other_down = keys_down & Buttons & ~TapButtons;
                const auto other_held = keys_held & Buttons & ~TapButtons;

                if(tap_down) {
                    // A second shoulder button joining (ZL then ZR), or anything else already held = a chord
                    const auto first_press = (tap_held & ~tap_down) == 0;
                    this->armed = first_press && (tap_down != TapButtons) && !other_held && !other_down;
                }
                else if(other_down && tap_held) {
                    this->armed = false;
                }

                if((keys_up & TapButtons) && (tap_held == 0)) {
                    const auto fire = this->armed;
                    this->armed = false;
                    return fire;
                }
                return false;
            }
    };

}
