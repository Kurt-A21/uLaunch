"""Builds the Deck Night look for the Ultrahand overlay (the ZL + ZR + D-pad Down menu):
   config/ultrahand/themes/deck-night.ini      selectable theme
   config/ultrahand/theme.ini                  same, active by default
   config/ultrahand/wallpapers/deck-night.rgba selectable wallpaper (448x720 raw RGBA8888)
   config/ultrahand/wallpaper.rgba             same, active by default
   overlay_preview.png                         approximate preview
usage: build_overlay_theme.py <Ultrahand repo themes/ultra.ini> <out dir> [keys.txt]
keys.txt = theme keys libultrahand reads (one per line); used to reject typos."""
import configparser, math, os, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont

base_ini, out = sys.argv[1], sys.argv[2]
cfg = configparser.ConfigParser(); cfg.optionxform = str
cfg.read(base_ini)
t = dict(cfg["theme"])

BG, PANEL, TEXT, SUB, DIM, BLUE, BLUE_LIGHT, LINE = "0E141B", "171D25", "E8EDF2", "9AA7B4", "5C6B7A", "1A9FFF", "66C0FF", "2A3441"
OK, WARN, BAD = "4CD964", "FFB547", "FF5A5A"
over = {
    # backgrounds (alpha values are 0-15)
    "bg_color": BG, "bg_alpha": "14", "table_bg_color": PANEL, "table_bg_alpha": "14",
    "widget_backdrop_color": "000000", "widget_backdrop_alpha": "12",
    "selection_bg_color": BLUE, "selection_bg_alpha": "4",
    # text
    "text_color": TEXT, "header_text_color": TEXT, "bottom_text_color": TEXT, "clock_color": TEXT,
    "temperature_color": SUB, "banner_version_text_color": SUB, "notification_title_color": TEXT,
    "notification_text_color": TEXT, "notification_time_color": SUB, "overlay_text_color": TEXT,
    "package_text_color": TEXT, "overlay_version_text_color": SUB, "package_version_text_color": SUB,
    "ult_overlay_text_color": BLUE_LIGHT, "ult_overlay_version_text_color": BLUE, "ult_package_text_color": BLUE_LIGHT,
    "ult_package_version_text_color": BLUE, "selection_text_color": "FFFFFF", "selection_value_text_color": BLUE_LIGHT,
    "table_info_text_color": BLUE_LIGHT, "table_section_text_color": TEXT, "off_text_color": SUB, "on_text_color": BLUE_LIGHT,
    "unfocused_color": DIM, "default_overlay_color": TEXT, "default_package_color": TEXT,
    "inprogress_text_color": WARN, "invalid_text_color": BAD, "warning_text_color": BAD,
    "healthy_ram_text_color": OK, "neutral_ram_text_color": WARN, "bad_ram_text_color": BAD,
    # lines / separators
    "separator_color": LINE, "separator_alpha": "15", "text_separator_color": LINE, "top_separator_color": LINE,
    "header_separator_color": LINE, "bottom_separator_color": LINE, "table_border_color": LINE, "widget_border_color": LINE,
    "bottom_button_color": TEXT, "scrollbar_color": "3A4654", "scrollbar_wall_color": "5C6B7A",
    # focus / interaction (Steam-blue accents)
    "highlight_color_1": BLUE, "highlight_color_2": BLUE_LIGHT, "highlight_color_3": "FFFFFF", "highlight_color_4": BLUE,
    "click_color": BLUE, "click_alpha": "7", "click_text_color": "FFFFFF", "progress_color": BLUE, "progress_alpha": "9",
    "trackbar_full_color": BLUE, "trackbar_empty_color": LINE, "trackbar_slider_color": "E8EDF2",
    "trackbar_slider_border_color": BLUE, "trackbar_slider_malleable_color": BLUE_LIGHT,
    "star_color": BLUE_LIGHT, "selection_star_color": "FFFFFF",
    "battery_color": TEXT, "battery_charging_color": OK, "battery_low_color": BAD,
    # logo
    "logo_color_1": TEXT, "logo_color_2": BLUE, "dynamic_logo_color_1": BLUE, "dynamic_logo_color_2": BLUE_LIGHT,
    # "s2" widget style
    "s2_toggle_on_color": BLUE, "s2_toggle_off_color": "3A4654", "s2_toggle_circle_color": "FFFFFF",
    "s2_radio_selected_color": BLUE, "s2_radio_ring_color": "5C6B7A", "s2_radio_inner_color": "FFFFFF",
    "s2_highlight_color_1": BLUE, "s2_highlight_color_2": BLUE_LIGHT,
    "s2_table_border_color_1": BLUE, "s2_table_border_color_2": BLUE_LIGHT,
    "s2_widget_border_color_1": BLUE, "s2_widget_border_color_2": BLUE_LIGHT,
    # Newer Ultrahand (2.5.x) animates these between shades: keep them in the Deck Night blues
    # (otherwise Ultrahand's default white/pink/grey-purple shows through)
    "s2_highlight_color_3": "CDEBFF", "s2_highlight_color_3_deep": "8FD0FF",
    "s2_highlight_color_4": BLUE_LIGHT, "s2_highlight_color_4_deep": BLUE,
    "s2_table_border_color_3": DIM, "s2_table_border_color_3_deep": "3A4654",
    "s2_table_border_color_4": BLUE_LIGHT, "s2_table_border_color_4_deep": BLUE,
    "s2_widget_border_color_3": DIM, "s2_widget_border_color_3_deep": "3A4654",
    "s2_widget_border_color_4": BLUE_LIGHT, "s2_widget_border_color_4_deep": BLUE,
}
allowed = set(t) | (set(open(sys.argv[3]).read().split()) if len(sys.argv) > 3 else set())
unknown = [k for k in over if k not in allowed]
if unknown:
    sys.exit(f"keys not in Ultrahand's theme format: {unknown}")
t.update(over)

ini = "[theme]\n" + "".join(f"{k}={v}\n" for k, v in sorted(t.items()))
for rel in ("config/ultrahand/themes/deck-night.ini", "config/ultrahand/theme.ini"):
    p = os.path.join(out, rel); os.makedirs(os.path.dirname(p), exist_ok=True)
    open(p, "w").write(ini)

# Wallpaper: dark blue-grey gradient, soft Steam-blue glow at the top, faint grid at the bottom
W, H = 448, 720
yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
tt = yy / H
img = np.zeros((H, W, 3), np.float32)
for c, (a, b) in enumerate(zip((27, 38, 54), (8, 11, 15))):
    img[..., c] = a + (b - a) * tt
glow = np.exp(-(((xx - W * 0.7) / (W * 0.6)) ** 2 + ((yy + 40) / (H * 0.35)) ** 2))
img += glow[..., None] * np.array([26, 159, 255]) * 0.28
grid = np.zeros((H, W), np.float32)
horizon = 560
for gy in range(horizon, H):
    z = (gy - horizon + 8)
    if int(z * 0.9) % 22 == 0 or (gy - horizon) % max(3, int(60 / z * 6)) == 0:
        pass
for k in range(1, 8):
    gy = horizon + int(160 * (1 - 1 / (1 + k * 0.35)))
    if gy < H: grid[gy, :] = 1
for k in range(-9, 10):
    x0, x1 = W / 2 + k * 6, W / 2 + k * 70
    for gy in range(horizon, H):
        f = (gy - horizon) / (H - horizon)
        x = int(x0 + (x1 - x0) * f)
        if 0 <= x < W: grid[gy, x] = 1
fade = np.clip((yy - horizon) / (H - horizon), 0, 1)
img += (grid * fade * 0.55)[..., None] * np.array([26, 159, 255]) * 0.6
rgba = np.dstack([img.clip(0, 255), np.full((H, W), 255.0)]).astype(np.uint8)
raw = rgba.tobytes()
assert len(raw) == 448 * 720 * 4
for rel in ("config/ultrahand/wallpapers/deck-night.rgba", "config/ultrahand/wallpaper.rgba"):
    p = os.path.join(out, rel); os.makedirs(os.path.dirname(p), exist_ok=True)
    open(p, "wb").write(raw)

# Approximate preview of the overlay with this theme (not a screenshot of the real overlay)
def col(h, a=255): return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4)) + (a,)
F = "/usr/share/fonts/opentype/inter/InterDisplay-Medium.otf"
f_big, f, f_s = ImageFont.truetype(F, 30), ImageFont.truetype(F, 19), ImageFont.truetype(F, 15)
pv = Image.fromarray(rgba, "RGBA")
ov = Image.new("RGBA", pv.size, col(BG, int(255 * 14 / 15) // 3)); pv = Image.alpha_composite(pv, ov)
d = ImageDraw.Draw(pv)
d.text((22, 26), "Ultra", font=f_big, fill=col(TEXT)); d.text((22 + d.textlength("Ultra", font=f_big), 26), "hand", font=f_big, fill=col(BLUE))
d.text((330, 22), "10:32", font=f, fill=col(TEXT)); d.text((330, 46), "48°C  82%", font=f_s, fill=col(SUB))
d.line((18, 92, W - 18, 92), fill=col(LINE), width=1)
rows = [("sys-clk", "2.0.1", None), ("Status Monitor", "1.4.1", None), ("Example toggle", None, True), ("Example toggle", None, False), ("Example slider", None, "slider")]
y = 110
for i, (name, ver, kind) in enumerate(rows):
    sel = i == 2
    if sel:
        d.rounded_rectangle((14, y - 6, W - 14, y + 58), 10, fill=col(BLUE, 60), outline=col(BLUE), width=3)
    d.text((32, y + 12), name, font=f, fill=col("FFFFFF" if sel else TEXT))
    if ver: d.text((W - 32 - d.textlength(ver, font=f_s), y + 16), ver, font=f_s, fill=col(SUB))
    if kind is True or kind is False:
        x = W - 92; on = kind
        d.rounded_rectangle((x, y + 14, x + 54, y + 42), 14, fill=col(BLUE if on else "3A4654"))
        cx = x + (40 if on else 14); d.ellipse((cx - 11, y + 17, cx + 11, y + 39), fill=col("FFFFFF"))
    if kind == "slider":
        d.rounded_rectangle((32, y + 50, W - 32, y + 56), 3, fill=col(LINE)); d.rounded_rectangle((32, y + 50, 300, y + 56), 3, fill=col(BLUE))
        d.ellipse((290, y + 43, 310, y + 63), fill=col(TEXT), outline=col(BLUE), width=2)
        d.text((W - 80, y + 12), "70%", font=f_s, fill=col(BLUE_LIGHT))
    d.line((26, y + 66, W - 26, y + 66), fill=col(LINE), width=1)
    y += 84 if kind == "slider" else 72
d.line((18, H - 64, W - 18, H - 64), fill=col(LINE), width=1)
d.text((26, H - 46), "Ⓑ Back    Ⓐ OK", font=f_s, fill=col(TEXT))
pv.convert("RGB").save(os.path.join(out, "overlay_preview.png"))
print("ok", len(t), "theme keys")
