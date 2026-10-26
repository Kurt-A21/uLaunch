#!/usr/bin/env python3
"""
oc_profile.py - adds sys-clk overclock profiles for your games to the SD card's /config/sys-clk/config.ini.

  python3 oc_profile.py presets
  python3 oc_profile.py add    /Volumes/SWITCH "Mario Kart 8 Deluxe" performance
  python3 oc_profile.py add    /Volumes/SWITCH 0100152000022000 balanced
  python3 oc_profile.py remove /Volumes/SWITCH 0100152000022000
  python3 oc_profile.py show   /Volumes/SWITCH

Notes
  * sys-clk applies a profile by the RUNNING GAME's title ID. Homebrew/ports started through the
    steamui home screen run as the "take-over" game, so give THAT game's title ID a profile to
    overclock ports such as NFS Carbon (the port ignores its own NFSU2_*_MHZ lines while sys-clk runs).
  * The home menu itself is 0100000000001000.
  * sys-clk rounds to the nearest real clock and caps GPU in handheld (460 MHz, 614 MHz on
    Mariko/OLED/Lite) and when charging from a non-official charger (768 MHz).
  * You can also change clocks live on the console: Ultrahand overlay (ZL + ZR + D-pad Down) > sys-clk.
"""
import re, sys
from pathlib import Path

HOME_MENU_TID = "0100000000001000"

PRESETS = {
    # Max clocks sys-clk allows (Nintendo's boost-mode limits): smoothest, hottest, shortest battery.
    "performance": {
        "docked_cpu": 1785, "docked_gpu": 921, "docked_mem": 1600,
        "handheld_charging_official_cpu": 1785, "handheld_charging_official_gpu": 921, "handheld_charging_official_mem": 1600,
        "handheld_charging_cpu": 1785, "handheld_charging_gpu": 768, "handheld_charging_mem": 1600,
        "handheld_cpu": 1785, "handheld_gpu": 614, "handheld_mem": 1600,
    },
    # Noticeably faster than stock without running flat out.
    "balanced": {
        "docked_cpu": 1428, "docked_gpu": 844, "docked_mem": 1600,
        "handheld_charging_cpu": 1428, "handheld_charging_gpu": 768, "handheld_charging_mem": 1600,
        "handheld_cpu": 1224, "handheld_gpu": 460, "handheld_mem": 1600,
    },
    # Underclock for light games, longer handheld battery.
    "battery": {
        "handheld_cpu": 816, "handheld_gpu": 307, "handheld_mem": 1065,
    },
}

HEADER = """; sys-clk configuration (managed in part by steamui oc_profile.py)
; One [TITLEID] section per game. Values in MHz; missing/0 = stock clocks.
; Live changes on the console: Ultrahand overlay (ZL + ZR + D-pad Down) > sys-clk.

[values]
; how often sys-clk re-checks the running game and applies its profile
poll_interval_ms=300
; set to e.g. 1000 to log temperatures/clocks to /config/sys-clk/context.csv (for checking it works)
csv_write_interval_ms=0
temp_log_interval_ms=0
"""


def config_path(sd: Path) -> Path:
    return sd / "config" / "sys-clk" / "config.ini"


def read_sections(text):
    """Returns ordered list of (section, [lines]) keeping comments."""
    sections, cur, lines = [], None, []
    for line in text.splitlines():
        m = re.match(r"^\s*\[([^\]]+)\]\s*$", line)
        if m:
            sections.append((cur, lines))
            cur, lines = m.group(1).strip(), []
        else:
            lines.append(line)
    sections.append((cur, lines))
    return sections


def write_sections(sections):
    out = []
    for name, lines in sections:
        if name is not None:
            out.append(f"[{name}]")
        out.extend(lines)
    return "\n".join(out).rstrip() + "\n"


def resolve_title(sd: Path, game: str) -> tuple:
    if re.fullmatch(r"[0-9a-fA-F]{16}", game):
        return game.upper(), game.upper()
    if game.lower() in ("home", "home menu", "menu"):
        return HOME_MENU_TID, "Home menu"
    sys.path.insert(0, str(Path(__file__).parent))
    from steamui_media import load_titledb, installed_app_ids, match_key  # same folder
    db = load_titledb()
    names = {tid: (db.get(tid) or {}).get("name") or tid for tid in installed_app_ids(sd)}
    tid = match_key(game, names)
    if tid is None:
        sys.exit(f"No installed game matches '{game}'. Use its 16-digit title ID instead.")
    return tid, names[tid]


def cmd_add(sd: Path, game: str, preset: str):
    if preset not in PRESETS:
        sys.exit(f"Unknown preset '{preset}'. Choose: {', '.join(PRESETS)}")
    tid, name = resolve_title(sd, game)
    path = config_path(sd)
    path.parent.mkdir(parents=True, exist_ok=True)
    text = path.read_text() if path.exists() else HEADER
    sections = [s for s in read_sections(text) if s[0] is None or s[0].upper() != tid]
    body = [f"; {name} - {preset}"] + [f"{k}={v}" for k, v in PRESETS[preset].items()] + [""]
    sections.append((tid, body))
    path.write_text(write_sections(sections))
    print(f"Added '{preset}' profile for {name} [{tid}] to {path}")


def cmd_remove(sd: Path, game: str):
    tid, name = resolve_title(sd, game)
    path = config_path(sd)
    if not path.exists():
        sys.exit("No sys-clk config on this SD card.")
    sections = read_sections(path.read_text())
    kept = [s for s in sections if s[0] is None or s[0].upper() != tid]
    path.write_text(write_sections(kept))
    print(f"{'Removed' if len(kept) != len(sections) else 'No profile for'} {name} [{tid}]")


def cmd_show(sd: Path):
    path = config_path(sd)
    print(path.read_text() if path.exists() else "No sys-clk config on this SD card yet.")


def main():
    a = sys.argv[1:]
    if not a or a[0] not in ("add", "remove", "show", "presets"):
        print(__doc__); sys.exit(1)
    if a[0] == "presets":
        for k, v in PRESETS.items():
            print(f"{k}:"); [print(f"  {kk}={vv}") for kk, vv in v.items()]
        return
    sd = Path(a[1]).expanduser()
    if a[0] == "show":
        cmd_show(sd)
    elif a[0] == "add":
        if len(a) < 4: sys.exit("usage: oc_profile.py add <SD> <game or title ID> <preset>")
        cmd_add(sd, a[2], a[3])
    else:
        if len(a) < 3: sys.exit("usage: oc_profile.py remove <SD> <game or title ID>")
        cmd_remove(sd, a[2])


if __name__ == "__main__":
    main()
