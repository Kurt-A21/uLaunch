"""Builds the 'Deck Night' uLaunch theme (.ultheme): SteamOS-inspired dark UI for the steamui uMenu fork.
All artwork and audio are generated here (original)."""
import json, math, os, shutil, subprocess, sys, wave, zipfile
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

OUT = sys.argv[1]
ROOT = os.path.join(OUT, "DeckNight")
shutil.rmtree(ROOT, ignore_errors=True)
SS = 4  # supersampling for smooth edges

TXT = (232, 237, 242)
BLUE = (26, 159, 255)


def p(*parts):
    path = os.path.join(ROOT, *parts)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    return path


def canvas(w, h):
    return Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))


def down(img, w, h):
    return img.resize((w, h), Image.LANCZOS)


def save(img, *parts):
    img.save(p(*parts))


def vgrad(w, h, stops):
    """stops: list of (t, (r,g,b,a))"""
    arr = np.zeros((h, w, 4), dtype=np.float32)
    ts = np.linspace(0, 1, h)
    for c in range(4):
        arr[:, :, c] = np.interp(ts, [s[0] for s in stops], [s[1][c] for s in stops])[:, None]
    return Image.fromarray(arr.clip(0, 255).astype(np.uint8), "RGBA")


# ---------------- backgrounds / shades ----------------
W, H = 1920, 1080
bg = np.zeros((H, W, 3), dtype=np.float32)
yy, xx = np.mgrid[0:H, 0:W]
d = np.sqrt(((xx - W * .3) / W) ** 2 + ((yy - H * .1) / H) ** 2)
for c, (a, b) in enumerate(zip((27, 40, 56), (9, 12, 16))):
    bg[..., c] = a + (b - a) * np.clip(d / 0.95, 0, 1)
rng = np.random.default_rng(3)
bg += rng.normal(0, 1.2, bg.shape)  # tiny grain against banding
save(Image.fromarray(bg.clip(0, 255).astype(np.uint8)).convert("RGBA"), "ui", "Background.png")

top = vgrad(W, 240, [(0, (0, 0, 0, 165)), (1, (0, 0, 0, 0))])
for n in ("Default", "Application", "Folder", "Homebrew"):
    save(top, "ui", "Main", "TopMenuBackground", f"{n}.png")

save(vgrad(W, 640, [(0, (0, 0, 0, 0)), (.45, (0, 0, 0, 170)), (1, (0, 0, 0, 235))]), "ui", "Main", "EntryMenuBackground.png")
for n in ("EntryMenuLeftIcon", "EntryMenuRightIcon", "InputBarBackground"):
    save(Image.new("RGBA", (4, 4), (0, 0, 0, 0)), "ui", "Main", f"{n}.png")
save(Image.new("RGBA", (1797, 60), (0, 0, 0, 0)), "ui", "Settings", "InputBarBackground.png")

# ---------------- tiles ----------------
T = 384
R = 22


def rrect_mask(size, inset, radius):
    m = canvas(size, size); dd = ImageDraw.Draw(m)
    dd.rounded_rectangle((inset * SS, inset * SS, (size - inset) * SS - 1, (size - inset) * SS - 1), radius * SS, fill=(255, 255, 255, 255))
    return m


# Empty slot: faint rounded square
e = canvas(T, T); ImageDraw.Draw(e).rounded_rectangle((6 * SS, 6 * SS, (T - 6) * SS, (T - 6) * SS), R * SS, fill=(255, 255, 255, 14), outline=(255, 255, 255, 22), width=2 * SS)
save(down(e, T, T), "ui", "Main", "EntryIcon", "Empty.png")

# Border (448 canvas, tile = inner 384): masks icon corners to look rounded + hairline
B = 448; off = (B - T) // 2
b = canvas(B, B); bd = ImageDraw.Draw(b)
# corner masks: paint the area between square tile corner and rounded corner in near-black
corner = Image.new("RGBA", (B * SS, B * SS), (0, 0, 0, 0))
cm = ImageDraw.Draw(corner)
cm.rectangle((off * SS, off * SS, (off + T) * SS - 1, (off + T) * SS - 1), fill=(10, 13, 18, 255))
cm.rounded_rectangle((off * SS, off * SS, (off + T) * SS - 1, (off + T) * SS - 1), R * SS, fill=(0, 0, 0, 0))
b = Image.alpha_composite(b, corner)
ImageDraw.Draw(b).rounded_rectangle((off * SS, off * SS, (off + T) * SS - 1, (off + T) * SS - 1), R * SS, outline=(255, 255, 255, 34), width=2 * SS)
save(down(b, B, B), "ui", "Main", "OverIcon", "Border.png")

# Cursor: white outline + blue glow around the focused tile
c = Image.new("RGBA", (B, B), (0, 0, 0, 0))
glow = Image.new("RGBA", (B, B), (0, 0, 0, 0))
ImageDraw.Draw(glow).rounded_rectangle((off - 6, off - 6, off + T + 6, off + T + 6), R + 6, outline=BLUE + (230,), width=14)
glow = glow.filter(ImageFilter.GaussianBlur(12))
ring = canvas(B, B)
ImageDraw.Draw(ring).rounded_rectangle(((off - 5) * SS, (off - 5) * SS, (off + T + 5) * SS, (off + T + 5) * SS), (R + 5) * SS, outline=(255, 255, 255, 255), width=6 * SS)
c = Image.alpha_composite(glow, down(ring, B, B))
save(c, "ui", "Main", "OverIcon", "Cursor.png")

# Selected / Suspended (416 like default): blue pill badge on top-left
for name, label_col in (("Selected", BLUE), ("Suspended", (255, 255, 255))):
    S = 416; o = (S - T) // 2
    s = canvas(S, S); sd = ImageDraw.Draw(s)
    sd.rounded_rectangle(((o + 16) * SS, (o + 16) * SS, (o + 96) * SS, (o + 56) * SS), 20 * SS, fill=label_col + (235,))
    if name == "Suspended":
        sd.polygon([((o + 46) * SS, (o + 25) * SS), ((o + 46) * SS, (o + 47) * SS), ((o + 66) * SS, (o + 36) * SS)], fill=(20, 24, 30, 255))
    else:
        sd.line([((o + 40) * SS, (o + 36) * SS), ((o + 52) * SS, (o + 47) * SS), ((o + 74) * SS, (o + 26) * SS)], fill=(255, 255, 255, 255), width=6 * SS)
    save(down(s, S, S), "ui", "Main", "OverIcon", f"{name}.png")

# ---------------- line icons (quick menu, status, burger) ----------------
def icon(draw_fn, size=100, stroke=7):
    im = canvas(size, size); draw_fn(ImageDraw.Draw(im), SS, stroke * SS)
    return down(im, size, size)


def ic_power(d, k, w):
    d.arc((22 * k, 24 * k, 78 * k, 80 * k), 300, 240, fill=TXT, width=w); d.line((50 * k, 14 * k, 50 * k, 48 * k), fill=TXT, width=w)
def ic_pad(d, k, w):
    d.rounded_rectangle((10 * k, 30 * k, 90 * k, 72 * k), 21 * k, outline=TXT, width=w)
    d.line((30 * k, 42 * k, 30 * k, 60 * k), fill=TXT, width=w); d.line((21 * k, 51 * k, 39 * k, 51 * k), fill=TXT, width=w)
    d.ellipse((62 * k, 40 * k, 72 * k, 50 * k), fill=TXT); d.ellipse((70 * k, 52 * k, 80 * k, 62 * k), fill=TXT)
def ic_album(d, k, w):
    d.rounded_rectangle((12 * k, 20 * k, 88 * k, 80 * k), 8 * k, outline=TXT, width=w)
    d.line([(20 * k, 70 * k), (42 * k, 46 * k), (58 * k, 62 * k), (66 * k, 54 * k), (80 * k, 68 * k)], fill=TXT, width=w, joint="curve")
    d.ellipse((60 * k, 30 * k, 72 * k, 42 * k), fill=TXT)
def ic_web(d, k, w):
    d.ellipse((12 * k, 12 * k, 88 * k, 88 * k), outline=TXT, width=w); d.line((12 * k, 50 * k, 88 * k, 50 * k), fill=TXT, width=w - k)
    d.ellipse((34 * k, 12 * k, 66 * k, 88 * k), outline=TXT, width=w - k)
def ic_settings(d, k, w):
    for a in range(0, 360, 45):
        r = math.radians(a); d.line((50 * k + 26 * k * math.cos(r), 50 * k + 26 * k * math.sin(r), 50 * k + 40 * k * math.cos(r), 50 * k + 40 * k * math.sin(r)), fill=TXT, width=w + 2 * k)
    d.ellipse((24 * k, 24 * k, 76 * k, 76 * k), outline=TXT, width=w); d.ellipse((40 * k, 40 * k, 60 * k, 60 * k), outline=TXT, width=w - k)
def ic_themes(d, k, w):
    d.chord((12 * k, 12 * k, 88 * k, 88 * k), 0, 360, outline=TXT, width=w)
    for x, y in ((32, 42), (46, 28), (64, 32), (32, 62)): d.ellipse(((x - 6) * k, (y - 6) * k, (x + 6) * k, (y + 6) * k), fill=TXT)
    d.ellipse((56 * k, 56 * k, 76 * k, 76 * k), fill=(0, 0, 0, 0), outline=TXT, width=w - k)
def ic_mii(d, k, w):
    d.ellipse((12 * k, 12 * k, 88 * k, 88 * k), outline=TXT, width=w)
    d.ellipse((34 * k, 36 * k, 44 * k, 46 * k), fill=TXT); d.ellipse((56 * k, 36 * k, 66 * k, 46 * k), fill=TXT)
    d.arc((30 * k, 40 * k, 70 * k, 72 * k), 20, 160, fill=TXT, width=w)
def ic_amiibo(d, k, w):
    d.ellipse((12 * k, 12 * k, 88 * k, 88 * k), outline=TXT, width=w); d.ellipse((32 * k, 32 * k, 68 * k, 68 * k), outline=TXT, width=w)

def ic_perf(d, k, w):
    # speedometer: arc, ticks and a needle pointing high
    d.arc((12 * k, 18 * k, 88 * k, 94 * k), 150, 30, fill=TXT, width=w)
    for a in (150, 195, 240, 285, 330, 15):
        r = math.radians(a)
        d.line((50 * k + 30 * k * math.cos(r), 56 * k + 30 * k * math.sin(r), 50 * k + 38 * k * math.cos(r), 56 * k + 38 * k * math.sin(r)), fill=TXT, width=max(k, w // 2))
    r = math.radians(320)
    d.line((50 * k, 56 * k, 50 * k + 30 * k * math.cos(r), 56 * k + 30 * k * math.sin(r)), fill=BLUE, width=w)
    d.ellipse((44 * k, 50 * k, 56 * k, 62 * k), fill=TXT)

for name, fn in (("Performance", ic_perf), ("Power", ic_power), ("Controllers", ic_pad), ("Album", ic_album), ("WebBrowser", ic_web),
                 ("Settings", ic_settings), ("Themes", ic_themes), ("MiiEdit", ic_mii), ("Amiibo", ic_amiibo)):
    save(icon(fn), "ui", "Main", "QuickIcon", f"{name}.png")

# Burger button (78x78)
bur = canvas(78, 78); bd = ImageDraw.Draw(bur)
bd.rounded_rectangle((0, 0, 78 * SS - 1, 78 * SS - 1), 18 * SS, fill=(255, 255, 255, 30))
for y in (26, 39, 52):
    bd.rounded_rectangle((22 * SS, (y - 2.5) * SS, 56 * SS, (y + 2.5) * SS), 3 * SS, fill=TXT + (255,))
save(down(bur, 78, 78), "ui", "Main", "BurgerIcon.png")

# Status icons (56x56): wifi bars + battery
def wifi(level):
    def f(d, k, w):
        cx, cy = 50, 78
        for i, r in enumerate((14, 30, 46)):
            col = TXT + (255,) if level > i else TXT + (70,)
            d.arc(((cx - r) * k, (cy - r) * k, (cx + r) * k, (cy + r) * k), 225, 315, fill=col, width=w)
        d.ellipse(((cx - 5) * k, (cy - 5) * k, (cx + 5) * k, (cy + 5) * k), fill=TXT + (255,) if level > 0 else TXT + (70,))
        if level < 0:
            d.line((22 * k, 22 * k, 78 * k, 78 * k), fill=(255, 90, 90, 255), width=w)
    return down(icon(f, 100, 8), 56, 56)

for n, lv in (("0", 0), ("1", 1), ("2", 2), ("3", 3), ("None", -1)):
    save(wifi(lv), "ui", "Main", "TopIcon", "Connection", f"{n}.png")


def battery(pct, charging=False):
    def f(d, k, w):
        d.rounded_rectangle((8 * k, 32 * k, 84 * k, 68 * k), 7 * k, outline=TXT, width=w - 2 * k)
        d.rectangle((86 * k, 43 * k, 92 * k, 57 * k), fill=TXT)
        fill_w = int(66 * pct / 100)
        col = (255, 90, 90) if pct <= 20 else TXT
        if fill_w > 0: d.rounded_rectangle((14 * k, 38 * k, (14 + fill_w) * k, 62 * k), 3 * k, fill=col)
        if charging:
            d.polygon([(52 * k, 24 * k), (36 * k, 52 * k), (50 * k, 52 * k), (44 * k, 76 * k), (64 * k, 46 * k), (50 * k, 46 * k)], fill=(255, 210, 60, 255), outline=(20, 20, 20, 255))
    return down(icon(f, 100, 8), 56, 56)

for pct in range(10, 101, 10):
    save(battery(pct), "ui", "Main", "TopIcon", "Battery", f"{pct}.png")
save(battery(100, True), "ui", "Main", "TopIcon", "Battery", "Charging.png")

# ---------------- UI.json ----------------
default_ui = json.load(open(sys.argv[2]))
ui = json.loads(json.dumps(default_ui))
ui.update({
    "text_color": "#e8edf2ff", "toast_base_color": "#171d25ff", "toast_text_color": "#e8edf2ff",
    "menu_focus_color": "#1a9fffff", "menu_bg_color": "#171d25ff",
    "dialog_title_color": "#e8edf2ff", "dialog_cnt_color": "#c7d0d9ff", "dialog_opt_color": "#e8edf2ff",
    "dialog_color": "#171d25ff", "dialog_over_color": "#1a9fffff",
})
ui["main_menu"] = {
    "top_menu_bg": {"x": 0, "y": 0},
    "logo_top_icon": {"x": 60, "y": 40},
    "date_text": {"x": 1180, "y": 58, "font_size": "small"},
    "connection_top_icon": {"x": 1392, "y": 48},
    "battery_top_icon": {"x": 1462, "y": 48},
    "battery_text": {"x": 1528, "y": 58, "font_size": "small"},
    "time_text": {"x": 1636, "y": 54, "font_size": "medium"},
    "burger_icon": {"x": 1782, "y": 37},
    "input_bar": {"visible": False},
    "cur_path_text": {"y": 150, "h_align": "center", "font_size": "small", "clamp_width": 900, "clamp_speed": 3, "clamp_delay": 50},
    "cur_entry_main_text": {"x": 90, "y": 610, "font_size": "large", "clamp_width": 1300, "clamp_speed": 4, "clamp_delay": 50},
    "cur_entry_sub_text": {"x": 92, "y": 678, "font_size": "medium", "clamp_width": 1300, "clamp_speed": 4, "clamp_delay": 50},
    "entry_menu_bg": {"x": 0, "y": 440},
    "entry_menu_left_icon": {"x": 0, "y": 760},
    "entry_menu_right_icon": {"x": 1916, "y": 760},
    "entry_menu": {"x": 0, "y": 760, "fixed_row_count": 1},
    "haptics": {"enabled": True},
}
# Profile picker: big avatars in a row (steamui UserCarousel); the old list stays configured as fallback
ui["startup_menu"]["info_text"] = {"y": 190, "h_align": "center", "font_size": "large"}
ui["startup_menu"]["user_carousel"] = {"enabled": True, "y": 540, "avatar_size": 256, "spacing": 340}
# Lock screen: the original layout (time/date top-left, status top-right, prompt in the middle),
# with the dynamic "press the same button three times" prompt over the paused game
ui["lockscreen_menu"] = json.loads(json.dumps(default_ui["lockscreen_menu"]))
ui["lockscreen_menu"]["unlock_prompt"] = {"enabled": True, "y": 470, "presses": 3}
# Same spots as the original, adjusted for Deck Night's 56 px status icons (percentage beside, not on, the battery)
ui["lockscreen_menu"]["battery_top_icon"] = {"x": 1566, "y": 48}
ui["lockscreen_menu"]["battery_text"] = {"x": 1632, "y": 58, "font_size": "small"}
ui["quick_menu"] = {"quick_menu": {"x": 1480, "y": 140, "width": 380, "item_height": 80, "items_to_show": 10, "slide_down": True, "dim_alpha": 110}}
json.dump(ui, open(p("ui", "UI.json"), "w"), indent=4)

json.dump({"name": "Deck Night", "format_version": 3, "release": "1.2.5", "description": "SteamOS-inspired dark theme for the steamui uMenu build: game row at the bottom, hover art and sound, burger menu.", "author": "Kurt"}, open(p("theme", "Manifest.json"), "w"), indent=4)
ic = Image.new("RGBA", (256, 256), (0, 0, 0, 0)); ImageDraw.Draw(ic).rounded_rectangle((0, 0, 255, 255), 48, fill=(23, 29, 37, 255))
ImageDraw.Draw(ic).rounded_rectangle((60, 150, 196, 196), 12, fill=BLUE + (255,))
ImageDraw.Draw(ic).ellipse((90, 50, 166, 126), outline=TXT + (255,), width=12)
ic.save(p("theme", "Icon.png"))

# ---------------- sounds (44100 Hz, original) ----------------
SR = 44100


def wav(path, x):
    x = np.clip(x, -1, 1); pcm = (np.stack([x, x], 1) * 32767).astype(np.int16)
    with wave.open(path, "wb") as w:
        w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR); w.writeframes(pcm.tobytes())


def tone(f, dur, decay, amp=.3, kind="sin"):
    t = np.arange(int(dur * SR)) / SR
    ph = 2 * np.pi * (np.cumsum(f if np.ndim(f) else np.full(len(t), f)) / SR)
    s = np.sin(ph) if kind == "sin" else np.sign(np.sin(ph)) * .4
    return s * np.exp(-t * decay) * amp

cm = tone(1700, .06, 80, .22); cm[:int(.04 * SR)] += tone(3400, .04, 120, .05)
wav(p("sound", "Main", "CursorMove.wav"), cm)
wav(p("sound", "Main", "PageMove.wav"), tone(np.linspace(900, 1500, int(.12 * SR)), .12, 25, .2))
wav(p("sound", "Main", "OpenQuickMenu.wav"), tone(np.linspace(500, 1100, int(.22 * SR)), .22, 12, .25))
wav(p("sound", "Main", "CloseQuickMenu.wav"), tone(np.linspace(1100, 500, int(.22 * SR)), .22, 12, .25))
la = sum(tone(f, .7, 5, .16) for f in (523.25, 659.25, 783.99)); hi = sum(tone(f, .62, 5, .14) for f in (1046.5, 1318.5)); la[int(.08 * SR):int(.08 * SR) + len(hi)] += hi[:len(la) - int(.08 * SR)]
wav(p("sound", "Main", "LaunchApplication.wav"), la)
# Resume (lock screen / home): quick rising two-note chime
rs = tone(659.25, .45, 7, .16); hi2 = tone(987.77, .38, 7, .15); rs[int(.09 * SR):int(.09 * SR) + len(hi2)] += hi2[:len(rs) - int(.09 * SR)]
wav(p("sound", "Main", "ResumeApplication.wav"), rs)

# Ambient menu music (gets ducked while a game preview plays)
dur = 48.0; n = int(dur * SR); t = np.arange(n) / SR
mus = np.zeros(n)
chords = [[48, 55, 64, 71], [45, 52, 60, 67], [41, 48, 57, 64], [43, 50, 59, 62]]
for k in range(int(dur / 6)):
    for m in chords[k % 4]:
        f = 440 * 2 ** ((m - 69) / 12); i = int(k * 6 * SR); j = min(n, i + int(7 * SR)); tt = np.arange(j - i) / SR
        env = np.minimum(1, tt / 1.8) * np.minimum(1, (7 - tt) / 2.2)
        mus[i:j] += (np.sin(2 * np.pi * f * tt) + .25 * np.sin(2 * np.pi * 2 * f * tt + .3)) * env * .045
mus = mus * (0.9 + 0.1 * np.sin(2 * np.pi * t / 12))
tmpw = os.path.join(OUT, "bgm_tmp.wav"); wav(tmpw, mus)
subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", tmpw, "-ar", "44100", "-b:a", "128k", p("sound", "Main", "Bgm.mp3")], check=True)
os.remove(tmpw)
json.dump({"main_menu": {"bgm_loop": True, "bgm_fade_in_ms": 1500, "bgm_fade_out_ms": 500}}, open(p("sound", "BGM.json"), "w"), indent=4)

# ---------------- pack ----------------
ult = os.path.join(OUT, "DeckNight.ultheme")
with zipfile.ZipFile(ult, "w", zipfile.ZIP_DEFLATED) as z:
    for base, _, files in os.walk(ROOT):
        for f in files:
            full = os.path.join(base, f); z.write(full, os.path.relpath(full, ROOT))
print("wrote", ult, os.path.getsize(ult))
