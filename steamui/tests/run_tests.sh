#!/usr/bin/env bash
# Host-side tests for the steamui uMenu additions (no Switch needed):
#  - HoverMedia: art/sound timing, fast-scroll skipping, cache, music ducking, leaks, thread races (ASan/UBSan/TSan)
#  - SyncHomebrewEntries: auto-adding homebrew to the home screen
#  - Deck Night theme validation against the menu code + layout geometry
set -euo pipefail
T="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT="$T/../.."; SRC="$ROOT/projects/uMenu"
OUT="$(mktemp -d)"; trap 'rm -rf "$OUT"' EXIT
CXX="${CXX:-g++}"; F=(-std=gnu++23 -O1 -g -Wall -I"$T/stub" -I"$SRC/include")

echo "### HoverMedia (address + undefined-behaviour sanitizers)"
$CXX "${F[@]}" -Werror -fsanitize=address,undefined -c "$SRC/source/ul/menu/ui/ui_HoverMedia.cpp" -o "$OUT/hm.o"
$CXX "${F[@]}" -Wno-misleading-indentation -fsanitize=address,undefined "$T/test_hover.cpp" "$T/sim.cpp" "$OUT/hm.o" -o "$OUT/test_hover" -lpthread
(mkdir -p "$OUT/r1" && cd "$OUT/r1" && "$OUT/test_hover")

echo "### HoverMedia (thread sanitizer)"
$CXX "${F[@]}" -Wno-misleading-indentation -fsanitize=thread "$T/test_hover.cpp" "$T/sim.cpp" "$SRC/source/ul/menu/ui/ui_HoverMedia.cpp" -o "$OUT/test_hover_tsan" -lpthread
(mkdir -p "$OUT/r2" && cd "$OUT/r2" && TSAN_OPTIONS=halt_on_error=1 "$OUT/test_hover_tsan" | tail -1)

echo "### Homebrew auto-add"
$CXX "${F[@]}" -Werror -fsanitize=address,undefined -include algorithm "$T/test_hb.cpp" "$SRC/source/ul/menu/menu_HomebrewSync.cpp" -o "$OUT/test_hb"
(mkdir -p "$OUT/r3" && cd "$OUT/r3" && "$OUT/test_hb")

echo "### Theme"
python3 "$ROOT/steamui/tools/validate_theme.py" "$ROOT/steamui/DeckNight.ultheme" "$ROOT" | tail -1
