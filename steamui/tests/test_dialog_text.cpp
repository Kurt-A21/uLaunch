// GameDialog text layout: word wrap + cutting text that wouldn't fit (monospace fake font: 10 px per char, 30 px lines + 6 px gap).
#include <ul/menu/ui/ui_DialogText.hpp>
#include <cstdio>
#include <string>
using namespace ul::menu::ui;
static int fails = 0;
#define CHECK(c, ...) do { if(c) std::printf("  ok    "); else { std::printf("  FAIL  "); fails++; } std::printf(__VA_ARGS__); std::printf("\n"); } while(0)
static int W(const std::string &t) { size_t best = 0, cur = 0; for(char c: t) { if(c == '\n') { best = std::max(best, cur); cur = 0; } else cur++; } return (int)std::max(best, cur) * 10; }
static int H(const std::string &t) { int n = 1; for(char c: t) n += (c == '\n'); return n * 30 + (n - 1) * 6; }
static int Lines(const std::string &t) { int n = 1; for(char c: t) n += (c == '\n'); return n; }
int main() {
    std::printf("== Word wrap ==\n");
    CHECK(WrapText(W, "short", 200) == "short", "short text unchanged");
    const auto w = WrapText(W, "the quick brown fox jumps over the lazy dog", 100);
    bool fits = true; size_t s = 0; while(s <= w.size()) { auto e = std::min(w.find('\n', s), w.size()); if(W(w.substr(s, e - s)) > 100) fits = false; s = e + 1; }
    CHECK(fits && Lines(w) == 5, "every wrapped line fits the width (%d lines)", Lines(w));
    std::string joined = w; for(auto &c: joined) if(c == '\n') c = ' ';
    CHECK(joined == "the quick brown fox jumps over the lazy dog", "no words lost or split");
    CHECK(WrapText(W, "line one\nline two", 1000) == "line one\nline two", "existing line breaks kept");
    CHECK(WrapText(W, "a\n\nb", 1000) == "a\n\nb", "blank lines kept");
    CHECK(WrapText(W, "supercalifragilistic x", 50) == "supercalifragilistic\nx", "a word longer than the line stays whole");
    CHECK(WrapText(W, "", 100).empty(), "empty text");
    std::printf("== Cutting text that doesn't fit ==\n");
    std::string ten; for(int i = 0; i < 10; i++) ten += "line" + std::to_string(i) + (i < 9 ? "\n" : "");
    CHECK(LimitLines(H, ten, 1000) == ten, "fits: unchanged");
    const auto cut = LimitLines(H, ten, H("a\nb\nc\nd"));
    CHECK(Lines(cut) == 4 && H(cut) <= H("a\nb\nc\nd"), "cut to the 4 lines that fit");
    CHECK(cut.size() >= 3 && cut.substr(cut.size() - 3) == "..." && cut.find("line3...") != std::string::npos, "last kept line ends in '...'");
    CHECK(Lines(LimitLines(H, ten, 5)) == 1, "always keeps at least one line");
    std::printf(fails ? "FAILED: %d\n" : "ALL PASSED\n", fails);
    return fails ? 1 : 0;
}
