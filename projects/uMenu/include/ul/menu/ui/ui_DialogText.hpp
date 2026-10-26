
#pragma once
#include <algorithm>
#include <string>

namespace ul::menu::ui {

    // steamui: text layout helpers for GameDialog. The measuring functions are passed in
    // (Plutonium's GetTextWidth/GetTextHeight on the console) so these can be unit tested.

    // Word-wraps every line of the text to max_width (Plutonium itself only truncates). A single word
    // longer than the line stays whole on its own line.
    template<typename WidthFn>
    std::string WrapText(WidthFn text_width, const std::string &text, const int max_width) {
        std::string out;
        size_t line_start = 0;
        while(line_start <= text.size()) {
            const auto line_end = std::min(text.find('\n', line_start), text.size());
            const auto line = text.substr(line_start, line_end - line_start);
            std::string cur;
            size_t i = 0;
            while(i < line.size()) {
                auto j = line.find(' ', i);
                if(j == std::string::npos) {
                    j = line.size();
                }
                const auto word = line.substr(i, j - i);
                const auto candidate = cur.empty() ? word : (cur + " " + word);
                if(!cur.empty() && (text_width(candidate) > max_width)) {
                    out += cur + "\n";
                    cur = word;
                }
                else {
                    cur = candidate;
                }
                i = j + 1;
            }
            out += cur;
            if(line_end >= text.size()) {
                break;
            }
            out += "\n";
            line_start = line_end + 1;
        }
        return out;
    }

    // Keeps at most as many lines as fit in max_height; the last kept line then ends in "..."
    template<typename HeightFn>
    std::string LimitLines(HeightFn text_height, const std::string &text, const int max_height) {
        const int one = text_height("Ay");
        const int step = std::max(1, static_cast<int>(text_height("Ay\nAy")) - one);
        const auto max_lines = static_cast<size_t>(std::max(1, 1 + (max_height - one) / step));
        size_t lines = 1, pos = 0;
        while((pos = text.find('\n', pos)) != std::string::npos) {
            if(lines == max_lines) {
                return text.substr(0, pos) + "...";
            }
            lines++;
            pos++;
        }
        return text;
    }

}
