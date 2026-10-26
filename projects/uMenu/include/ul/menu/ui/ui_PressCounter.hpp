
#pragma once
#include <switch.h>

namespace ul::menu::ui {

    // steamui: the Switch's "press the same button three times" unlock rule.
    // Physical buttons only (A/B/X/Y, sticks clicked, L/R/ZL/ZR, +/-, D-pad, SL/SR); stick tilts don't count.
    // A touch counts as its own "button". Pressing a different button starts over from that one.
    class PressCounter {
        public:
            static constexpr u64 TouchKey = BITL(63);
            static constexpr u64 CountedButtons = 0xFFFFull | BITL(24) | BITL(25) | BITL(26) | BITL(27);

            enum class Result {
                Ignored,    // nothing that counts was pressed
                Counted,    // same button again (or first press)
                Restarted,  // different button: count restarted at 1 with it
                Completed   // reached the target: unlock
            };

        private:
            u32 target;
            u32 count;
            u64 key;

        public:
            constexpr PressCounter(const u32 target = 3) : target(target), count(0), key(0) {}

            // Lowest counted button in keys_down (one press per frame is plenty)
            static constexpr u64 PickButton(const u64 keys_down) {
                const auto pressed = keys_down & CountedButtons;
                return pressed & (~pressed + 1);
            }

            inline Result Feed(const u64 pressed_key) {
                if(pressed_key == 0) {
                    return Result::Ignored;
                }
                auto res = Result::Counted;
                if((this->count > 0) && (pressed_key != this->key)) {
                    this->count = 0;
                    res = Result::Restarted;
                }
                this->key = pressed_key;
                this->count++;
                if(this->count >= this->target) {
                    return Result::Completed;
                }
                return res;
            }

            inline void Reset() {
                this->count = 0;
                this->key = 0;
            }

            inline u32 GetCount() const {
                return this->count;
            }

            inline u32 GetTarget() const {
                return this->target;
            }

            inline u64 GetKey() const {
                return this->key;
            }

            // Short label for the button being pressed, shown in the badge
            static constexpr const char *GetButtonLabel(const u64 key) {
                switch(key) {
                    case HidNpadButton_A: return "A";
                    case HidNpadButton_B: return "B";
                    case HidNpadButton_X: return "X";
                    case HidNpadButton_Y: return "Y";
                    case HidNpadButton_StickL: return "L-Stick";
                    case HidNpadButton_StickR: return "R-Stick";
                    case HidNpadButton_L: return "L";
                    case HidNpadButton_R: return "R";
                    case HidNpadButton_ZL: return "ZL";
                    case HidNpadButton_ZR: return "ZR";
                    case HidNpadButton_Plus: return "+";
                    case HidNpadButton_Minus: return "-";
                    case HidNpadButton_Left: return "Left";
                    case HidNpadButton_Up: return "Up";
                    case HidNpadButton_Right: return "Right";
                    case HidNpadButton_Down: return "Down";
                    case TouchKey: return "Touch";
                    default: return "SL/SR";
                }
            }
    };

}
