#!/usr/bin/env bash
# Runs sys-clk's REAL config parser and clock manager (from its GitHub source) against the profiles
# oc_profile.py writes, with the hardware layer simulated, for Erista and Mariko in every power state.
set -euo pipefail
T="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT="$T/../../.."
W="$(mktemp -d)"; trap 'rm -rf "$W"' EXIT
git clone -q --depth 1 https://github.com/retronx-team/sys-clk.git "$W/sys-clk"
git clone -q --depth 1 https://github.com/switchbrew/libnx.git "$W/libnx"
SC="$W/sys-clk"
I=(-I"$SC/sysmodule/src" -I"$SC/common/include" -I"$SC/sysmodule/lib/minIni/include" -I"$SC/sysmodule/lib/nxExt/include" -I"$T/nxstub" -I"$W/libnx/nx/include" -D__SWITCH__ -DTARGET='"sys-clk"')
# x86 has no ARM tick counter: that call only times optional logging, so it is zeroed in a copy
sed 's/armTicksToNs(armGetSystemTick())/(std::uint64_t)0/' "$SC/sysmodule/src/clock_manager.cpp" > "$W/clock_manager.cpp"
for f in "$W/clock_manager.cpp" "$SC/sysmodule/src/config.cpp" "$SC/sysmodule/src/errors.cpp" "$T/stubs.cpp" "$T/test_oc.cpp"; do
  g++ -std=gnu++20 -w -c "${I[@]}" "$f" -o "$W/$(basename "$f").o"
done
gcc -c -I"$SC/sysmodule/lib/minIni/include" "$SC/sysmodule/lib/minIni/dev/minIni.c" -o "$W/minIni.o"
g++ "$W"/*.o -o "$W/test_oc"
mkdir -p "$W/sd"
python3 "$ROOT/steamui/tools/oc_profile.py" add "$W/sd" 0100152000022000 performance
python3 "$ROOT/steamui/tools/oc_profile.py" add "$W/sd" 01007EF00011E000 balanced
python3 "$ROOT/steamui/tools/oc_profile.py" add "$W/sd" 0100BA0003EEA000 battery
python3 "$ROOT/steamui/tools/oc_profile.py" add "$W/sd" 0100152000022000 performance >/dev/null   # re-adding must not duplicate
[ "$(grep -c '^\[0100152000022000\]' "$W/sd/config/sys-clk/config.ini")" = 1 ] || { echo "duplicate section"; exit 1; }
"$W/test_oc" "$W/sd/config/sys-clk/config.ini"
