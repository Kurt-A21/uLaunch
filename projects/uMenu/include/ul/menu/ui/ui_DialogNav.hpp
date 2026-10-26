
#pragma once
#include <switch/types.h>
#include <algorithm>

namespace ul::menu::ui {

    // steamui: selection + scrolling logic of GameDialog (kept free of rendering so it can be unit tested).
    // The list doesn't wrap around, like the PS5/Xbox/SteamOS menus.
    class DialogNav {
        private:
            u32 count;
            u32 visible;
            u32 selected;
            u32 first;

            inline void ClampFirst() {
                if(this->selected < this->first) {
                    this->first = this->selected;
                }
                else if(this->selected >= this->first + this->visible) {
                    this->first = this->selected - this->visible + 1;
                }
                const auto max_first = (this->count > this->visible) ? (this->count - this->visible) : 0;
                this->first = std::min(this->first, max_first);
            }

        public:
            DialogNav() : count(0), visible(1), selected(0), first(0) {}

            inline void Reset(const u32 count, const u32 visible, const u32 initial = 0) {
                this->count = count;
                this->visible = std::max<u32>(1, visible);
                this->selected = (count > 0) ? std::min(initial, count - 1) : 0;
                this->first = 0;
                this->ClampFirst();
            }

            // Returns whether the selection changed
            inline bool Move(const s32 delta) {
                if(this->count == 0) {
                    return false;
                }
                const auto target = std::clamp<s64>(static_cast<s64>(this->selected) + delta, 0, static_cast<s64>(this->count) - 1);
                if(static_cast<u32>(target) == this->selected) {
                    return false;
                }
                this->selected = static_cast<u32>(target);
                this->ClampFirst();
                return true;
            }

            inline bool Select(const u32 idx) {
                if((idx >= this->count) || (idx == this->selected)) {
                    return false;
                }
                this->selected = idx;
                this->ClampFirst();
                return true;
            }

            inline u32 GetCount() const { return this->count; }
            inline u32 GetVisible() const { return std::min(this->visible, this->count); }
            inline u32 GetSelected() const { return this->selected; }
            inline u32 GetFirstVisible() const { return this->first; }
            inline bool IsScrollable() const { return this->count > this->visible; }
    };

    // Held-direction auto repeat: fires on press, then after FirstDelayMs every RepeatMs
    class HoldRepeat {
        public:
            static constexpr s64 FirstDelayMs = 320;
            static constexpr s64 RepeatMs = 85;

        private:
            s32 dir;
            s64 next_ms;

        public:
            HoldRepeat() : dir(0), next_ms(0) {}

            // pressed_dir: direction newly pressed this frame (0 = none); held_dir: direction currently held (0 = none)
            inline s32 Feed(const s32 pressed_dir, const s32 held_dir, const s64 now_ms) {
                if(pressed_dir != 0) {
                    this->dir = pressed_dir;
                    this->next_ms = now_ms + FirstDelayMs;
                    return pressed_dir;
                }
                if((held_dir == 0) || (held_dir != this->dir)) {
                    this->dir = 0;
                    return 0;
                }
                if(now_ms >= this->next_ms) {
                    this->next_ms = now_ms + RepeatMs;
                    return this->dir;
                }
                return 0;
            }
    };

}
