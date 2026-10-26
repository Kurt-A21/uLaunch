#!/usr/bin/env python3
"""
validate_theme.py - checks a uLaunch .ultheme against what the steamui uMenu code actually loads.

  python3 validate_theme.py <theme.ultheme> <uLaunch repo root>

Checks: manifest, every image/sound path the menu code requests (theme or default-theme fallback),
image formats, WAV sample rates, UI.json keys/types, and the on-screen layout geometry of the
main menu (tiles, focus overshoot, cursor glow, dropdown, burger, texts) for overlaps/off-screen.
Exit code 1 if anything fails. Needs Pillow.
"""
import io, json, re, sys, wave, zipfile
from pathlib import Path
from PIL import Image

W, H = 1920, 1080
fails, warns = [], []


def ok(m): print(f"  ok    {m}")
def fail(m): print(f"  FAIL  {m}"); fails.append(m)
def warn(m): print(f"  warn  {m}"); warns.append(m)


def main():
    theme_path, repo = Path(sys.argv[1]), Path(sys.argv[2])
    z = zipfile.ZipFile(theme_path)
    names = set(z.namelist())
    default = repo / "default-theme"
    src = "\n".join(p.read_text(errors="ignore") for p in (repo / "projects/uMenu/source").rglob("*.cpp"))

    print("== Manifest ==")
    man = json.loads(z.read("theme/Manifest.json"))
    (ok if man.get("format_version") == 3 else fail)(f"format_version = {man.get('format_version')} (uLaunch 1.2.x expects 3)")
    for k in ("name", "author", "release", "description"):
        (ok if man.get(k) else fail)(f"manifest has '{k}'")

    print("== Resources requested by the menu code ==")
    img_refs = sorted(set(re.findall(r'TryFindLoadImage(?:Handle)?\("([^"]+)"\)', src)))
    for ref in img_refs:
        in_theme = [f"{ref}.{e}" for e in ("png", "jpg", "jpeg", "webp") if f"{ref}.{e}" in names]
        in_default = [p for p in default.glob(f"{ref}.*")]
        if in_theme:
            ok(f"{ref}  (theme)")
        elif in_default:
            ok(f"{ref}  (falls back to default theme)")
        elif ref == "ui/Main/BurgerIcon":
            fail(f"{ref} missing: no burger button")
        else:
            warn(f"{ref} missing everywhere (element shows nothing)")
    # dynamic paths built in code (battery levels, connection)
    for n in [f"ui/Main/TopIcon/Battery/{p}.png" for p in range(10, 101, 10)] + ["ui/Main/TopIcon/Battery/Charging.png"] + \
             [f"ui/Main/TopIcon/Connection/{c}.png" for c in ("0", "1", "2", "3", "None")]:
        (ok if n in names or (default / n).exists() else fail)(f"{n}")
    snd_refs = sorted(set(re.findall(r'TryGetActiveThemeResource\("(sound/[^"]+)"\)', src)))
    for ref in snd_refs:
        if ref in names:
            data = z.read(ref)
            if ref.endswith(".wav"):
                w = wave.open(io.BytesIO(data))
                (ok if w.getframerate() == 44100 else fail)(f"{ref}: {w.getframerate()} Hz, {w.getnchannels()} ch")
    if "sound/Main/Bgm.mp3" in names:
        b = z.read("sound/Main/Bgm.mp3")
        (ok if b[:3] == b"ID3" or (b[0] == 0xFF and (b[1] & 0xE0) == 0xE0) else fail)("sound/Main/Bgm.mp3 is an MP3")
        bgm = json.loads(z.read("sound/BGM.json")) if "sound/BGM.json" in names else {}
        (ok if bgm.get("main_menu", {}).get("bgm_loop") else warn)("main menu BGM loops")

    print("== Images ==")
    for n in sorted(x for x in names if x.endswith((".png", ".jpg"))):
        im = Image.open(io.BytesIO(z.read(n)))
        if n.endswith(".jpg") and im.info.get("progressive"):
            fail(f"{n}: progressive JPEG (SDL_image on Switch may not load it)")
        if n.startswith("ui/Main/OverIcon/") and im.size[0] != im.size[1]:
            fail(f"{n}: over icons must be square, got {im.size}")
    bg = Image.open(io.BytesIO(z.read("ui/Background.png")))
    (ok if bg.size == (W, H) else fail)(f"ui/Background.png is {bg.size}")

    print("== UI.json ==")
    ui = json.loads(z.read("ui/UI.json"))
    for c in ("text_color", "menu_focus_color", "menu_bg_color", "dialog_color", "dialog_over_color", "toast_base_color", "toast_text_color", "dialog_title_color", "dialog_cnt_color", "dialog_opt_color"):
        (ok if re.fullmatch(r"#[0-9a-fA-F]{8}", ui.get(c, "")) else fail)(f"{c} = {ui.get(c)}")
    dui = json.loads((default / "ui/UI.json").read_text())
    for menu, elems in dui.items():
        if isinstance(elems, dict):
            for e in elems:
                if menu in ui and e not in ui[menu]:
                    warn(f"{menu}.{e} not set: uses the default theme's position")
    mm, qm = ui["main_menu"], ui["quick_menu"]["quick_menu"]
    em = mm["entry_menu"]
    (ok if isinstance(em.get("fixed_row_count"), int) and em["fixed_row_count"] >= 1 else fail)(f"fixed_row_count = {em.get('fixed_row_count')}")
    (ok if mm.get("input_bar", {}).get("visible") is False else fail)("input bar (nav hints) hidden")
    (ok if qm.get("slide_down") is True else fail)("quick menu is a slide-down dropdown")
    (ok if isinstance(mm.get("haptics", {}).get("enabled"), bool) else warn)("haptics flag present")

    print("== Layout geometry (mirrors EntryMenu::ComputeSizes) ==")
    rows = em["fixed_row_count"]; ey = em["y"]; eh = H - ey
    HMARGIN, VMARGIN, GAP, MAXSZ = 60, 35, 32, 384
    size = int((eh - 2 * VMARGIN - (rows - 1) * GAP) / rows); size = min(size, MAXSZ)
    vmargin = int((eh - rows * size - (rows - 1) * GAP) / 2)
    count = int((W - 2 * HMARGIN + GAP) / (size + GAP))
    hgap = int((W - 2 * HMARGIN - count * size) / (count - 1))
    ok(f"{rows} row x {count} tiles of {size}px, {hgap}px apart, top at y={ey + vmargin}")
    cur = Image.open(io.BytesIO(z.read("ui/Main/OverIcon/Cursor.png"))).size[0]
    factor = cur / MAXSZ if cur >= MAXSZ else cur / 256
    peak = 1.12  # spring overshoot of the 1.10 focus scale
    tile_peak = size * peak; cursor_peak = size * factor * peak
    grow = (tile_peak - size) / 2
    (ok if grow < hgap / 2 else fail)(f"focused tile at peak overshoot grows {grow:.0f}px per side (< half the {hgap}px gap)")
    top = ey + vmargin + size / 2 - cursor_peak / 2; bottom = ey + vmargin + size / 2 + cursor_peak / 2
    (ok if top >= 0 and bottom <= H + 8 else fail)(f"cursor glow spans y {top:.0f}..{bottom:.0f} (screen 0..{H})")
    left = HMARGIN + size / 2 - cursor_peak / 2
    (ok if left >= -8 else fail)(f"first tile's glow starts at x={left:.0f}")
    def text_bottom(e, px):
        return mm[e]["y"] + px
    title_b = text_bottom("cur_entry_sub_text", 40)
    (ok if title_b < ey + vmargin - (cursor_peak - size) / 2 else fail)(f"game title/author end at y~{title_b} above the focused tile glow (y~{ey + vmargin - (cursor_peak - size) / 2:.0f})")
    (ok if mm["cur_entry_main_text"]["y"] + 60 <= mm["cur_entry_sub_text"]["y"] + 5 else warn)("title and author lines don't overlap")
    bx, by = mm["burger_icon"]["x"], mm["burger_icon"]["y"]
    bw, bh = Image.open(io.BytesIO(z.read("ui/Main/BurgerIcon.png"))).size
    (ok if bx + bw <= W and by >= 0 else fail)(f"burger at ({bx},{by}) size {bw}x{bh} is on screen")
    for e in ("time_text", "battery_text", "date_text", "connection_top_icon", "battery_top_icon"):
        (ok if mm[e]["x"] + 40 < bx else fail)(f"{e} at x={mm[e]['x']} stays left of the burger")
    qx, qy, qw = qm["x"], qm["y"], qm["width"]; qh = qm["item_height"] * qm["items_to_show"]
    (ok if qx + qw + 12 <= W and qy + qh + 12 <= H else fail)(f"dropdown {qw}x{qh} at ({qx},{qy}) fits on screen")
    (ok if qy - 12 >= by + bh else fail)(f"dropdown opens below the burger (panel top {qy - 12} >= {by + bh})")
    (ok if abs((qx + qw) - (bx + bw)) <= 20 else warn)("dropdown is right-aligned with the burger")
    n_items = len(re.findall(r"options_menu->AddItem\(", (repo / "projects/uMenu/source/ul/menu/ui/ui_QuickMenu.cpp").read_text()))
    (ok if qm["items_to_show"] >= n_items else warn)(f"all {n_items} quick menu items visible without scrolling")

    sm = ui.get("startup_menu", {})
    uc = sm.get("user_carousel")
    if uc and uc.get("enabled"):
        print("== Profile picker geometry (mirrors UserCarousel) ==")
        peak = uc["avatar_size"] * 1.18 * 1.02  # focus scale + spring overshoot
        ring = peak * factor
        top, bottom = uc["y"] - ring / 2, uc["y"] + ring / 2
        name_y = uc["y"] + uc["avatar_size"] * 1.18 / 2 + 36
        (ok if top > sm["info_text"]["y"] + 60 else fail)(f"welcome text (y={sm['info_text']['y']}) clears the focused avatar's glow (top {top:.0f})")
        (ok if name_y + 50 < H else fail)(f"profile name at y~{name_y:.0f} stays on screen")
        (ok if uc["spacing"] > peak else fail)(f"neighbouring avatars don't overlap the focused one ({uc['spacing']}px apart vs {peak:.0f}px wide)")
        (ok if (W - uc["spacing"]) / 2 > 0 else fail)("at least 3 profiles visible at once")

    lm = ui.get("lockscreen_menu", {})
    up = lm.get("unlock_prompt")
    if up and up.get("enabled"):
        print("== Lock screen geometry (mirrors UnlockPrompt) ==")
        n = up.get("presses", 3)
        (ok if 1 <= n <= 8 else fail)(f"{n} presses to unlock (1-8)")
        py = up["y"]; dots_y = py + 78 + 64 + 56
        burst_bottom = dots_y + 340
        top_items = [lm[k]["y"] for k in ("time_text", "date_text", "connection_top_icon", "battery_top_icon") if k in lm and "y" in lm[k]]
        (ok if top_items and max(top_items) + 80 < py else fail)(f"top bar (to y~{max(top_items) + 80 if top_items else '?'}) stays above the prompt (y={py})")
        (ok if dots_y + 40 < H - 120 else fail)(f"dots (y~{dots_y}) clear the '<game> · paused' line (y={H - 120})")
        (ok if (n - 1) * 64 + 2 * 28 < W else fail)("dots fit across the screen")
        (ok if "sound/Main/ResumeApplication.wav" in names or (default / "sound/Main/ResumeApplication.wav").exists() else warn)("a resume sound exists (theme or built-in default)")
        (ok if "resume_card" not in lm else warn)("no leftover resume_card settings")

    gd = ui.get("dialog", {}).get("game_dialog")
    if gd and gd.get("enabled"):
        print("== Console-style dialog ==")
        cc = ui["dialog_color"]; oc = ui["dialog_over_color"]
        def lum(h):
            r, g, b = (int(h[i:i + 2], 16) / 255 for i in (1, 3, 5))
            f = lambda c: c / 12.92 if c <= 0.03928 else ((c + 0.055) / 1.055) ** 2.4
            return 0.2126 * f(r) + 0.7152 * f(g) + 0.0722 * f(b)
        def contrast(a, b):
            la, lb = sorted((lum(a), lum(b)), reverse=True); return (la + 0.05) / (lb + 0.05)
        (ok if contrast(ui["dialog_opt_color"], cc) >= 4.5 else fail)(f"option text on the card: contrast {contrast(ui['dialog_opt_color'], cc):.1f}:1 (>= 4.5)")
        (ok if contrast("#0b0f14ff", oc) >= 4.5 else fail)(f"dark text on the highlight: contrast {contrast('#0b0f14ff', oc):.1f}:1 (>= 4.5)")
        (ok if contrast(oc, cc) >= 3 else fail)(f"highlight stands out from the card: {contrast(oc, cc):.1f}:1 (>= 3)")
        hdr = (repo / "projects/uMenu/include/ul/menu/ui/ui_GameDialog.hpp").read_text()
        g = {k: int(v) for k, v in re.findall(r"static constexpr s32 (\w+) = (\d+);", hdr)}
        rows_fit = (H - 120 - (g["Padding"] + 45 + 34) - g["FooterHeight"] - 24 + g["RowGap"]) // (g["RowHeight"] + g["RowGap"])
        (ok if rows_fit >= 6 else fail)(f"{rows_fit} option rows fit on screen before the list scrolls")
        (ok if g["RowHeight"] >= 64 else fail)(f"rows are {g['RowHeight']}px tall (>= 64, easy to tap in handheld)")

    print()
    print(f"{'PASSED' if not fails else 'FAILED'}: {len(fails)} failure(s), {len(warns)} warning(s)")
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
