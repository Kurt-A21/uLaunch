#!/usr/bin/env python3
"""
steamui_media.py - fills the SD card with hover art and hover sounds for the steamui home menu.

  Art:    downloads each installed game's official eShop banner (via the public titledb
          database) to  <SD>/ulaunch/steamui/bg/<APPID>.jpg  (1280x720 baseline JPEG: fast to decode, light on memory)
  Sounds: converts soundtrack clips you provide (audio files, or Switch video captures of a game's
          title screen music) to
          <SD>/ulaunch/steamui/sound/<APPID>.wav  (44100 Hz stereo, max 30 s, faded out)

Your installed games are found from the home menu's game list on the SD card
(ulaunch/emu_<user>/ for emuMMC, ulaunch/sys_<user>/ for sysMMC), so boot the menu once after
installing games. The downloads happen on your computer, so emuMMC being offline doesn't matter.

Usage (SD card mounted on your computer, e.g. /Volumes/SWITCH):
  python3 steamui_media.py art    /Volumes/SWITCH
  python3 steamui_media.py sounds /Volumes/SWITCH ~/Desktop/game_clips
  python3 steamui_media.py sync   /Volumes/SWITCH      <- run this after adding games
  python3 steamui_media.py list   /Volumes/SWITCH
  python3 steamui_media.py images /Volumes/SWITCH ~/Desktop/my_art

Homebrew apps (sdmc:/switch/**.nro) use their NRO file name as key, e.g. art for
switch/nfscx/nfscx-vulkan.nro goes in ulaunch/steamui/bg/nfscx-vulkan.jpg. The `images` and
`sounds` commands match your file names against both games and homebrew names.

Sound clip names: either the title ID (0100ABCD12340000.mp3) or the game's name
("Mario Kart 8 Deluxe.m4a"); names are matched loosely against your installed games.

Requires: Python 3.9+, `pip3 install pillow requests`, and ffmpeg for sounds (`brew install ffmpeg`).
"""
import difflib, io, json, re, subprocess, sys, time
from pathlib import Path

TITLEDB_URL = "https://raw.githubusercontent.com/blawar/titledb/master/{region}.json"
CACHE_DIR = Path.home() / ".cache" / "steamui"
MAX_SOUND_SECONDS = 30


def menu_entry_files(sd: Path):
    """uLaunch keeps one menu per console side and user: ulaunch/emu_<user>/ (emuMMC) and
    ulaunch/sys_<user>/ (sysMMC), with subfolders for menu folders. ulaunch/menu/ is the older layout."""
    root = sd / "ulaunch"
    files = []
    for pattern in ("emu_*/**/*.m.json", "sys_*/**/*.m.json", "menu/**/*.m.json", "menu_*/**/*.m.json"):
        files += root.glob(pattern)
    return files


def installed_app_ids(sd: Path):
    ids = set()
    for f in menu_entry_files(sd):
        try:
            j = json.loads(f.read_text(encoding="utf-8"))
        except Exception:
            continue
        app_id = j.get("application_id")
        if isinstance(app_id, int) and app_id > 0:
            ids.add(f"{app_id:016X}")
        elif isinstance(app_id, str) and re.fullmatch(r"(0x)?[0-9a-fA-F]{16}", app_id):
            ids.add(app_id[-16:].upper())
    return sorted(ids)


def homebrew_keys(sd: Path):
    """NRO stem -> display name, for sdmc:/switch/*.nro and sdmc:/switch/*/*.nro"""
    out = {}
    root = sd / "switch"
    for nro in list(root.glob("*.nro")) + list(root.glob("*/*.nro")):
        if not any(part.startswith(".") for part in nro.relative_to(root).parts):
            out[nro.stem] = f"{nro.stem} (homebrew)"
    return out


def match_key(stem, names):
    """names: key -> display name. Accepts the exact key or a loose name match."""
    if stem in names:
        return stem
    if re.fullmatch(r"[0-9a-fA-F]{16}", stem) and stem.upper() in names:
        return stem.upper()
    def norm(x):
        return re.sub(r"[^a-z0-9]+", " ", x.lower().replace("(homebrew)", "")).strip()
    normed = {norm(v): k for k, v in names.items()}
    hit = difflib.get_close_matches(norm(stem), list(normed), n=1, cutoff=0.55)
    return normed[hit[0]] if hit else None


def save_cover(img, dst):
    from PIL import Image
    img = img.convert("RGB")
    scale = max(1280 / img.width, 720 / img.height)
    img = img.resize((round(img.width * scale), round(img.height * scale)), Image.LANCZOS)
    left, top = (img.width - 1280) // 2, (img.height - 720) // 2
    img.crop((left, top, left + 1280, top + 720)).save(dst, "JPEG", quality=88, progressive=False, optimize=True)


def all_names(sd: Path, db):
    names = {tid: (db.get(tid) or {}).get("name") or tid for tid in installed_app_ids(sd)}
    names.update(homebrew_keys(sd))
    return names


def cmd_images(sd: Path, folder: Path):
    from PIL import Image
    names = all_names(sd, load_titledb())
    out = sd / "ulaunch/steamui/bg"
    out.mkdir(parents=True, exist_ok=True)
    for f in sorted(p for p in folder.iterdir() if p.is_file() and not p.name.startswith(".")):
        key = match_key(f.stem, names)
        if key is None:
            print(f"skip  {f.name}: no game or homebrew matches this name")
            continue
        try:
            save_cover(Image.open(f), out / f"{key}.jpg")
            print(f"ok    {f.name} -> {names[key]}")
        except Exception as e:
            print(f"fail  {f.name}: {e}")


def album_videos(sd: Path):
    """Video captures on sysMMC and on emuMMC (whose Nintendo folder lives under emuMMC/...)."""
    roots = [sd / "Nintendo" / "Album"] + [p / "Album" for p in sd.glob("emuMMC/*/Nintendo")]
    vids = []
    for r in roots:
        if r.is_dir():
            vids += [v for v in r.rglob("*.mp4") if not v.name.startswith(".")]
    return sorted(vids, key=lambda v: v.name, reverse=True)  # file names start with the capture time


def capture_time(v: Path):
    m = re.match(r"(\d{4})(\d{2})(\d{2})(\d{2})(\d{2})(\d{2})", v.name)
    return f"{m[1]}-{m[2]}-{m[3]} {m[4]}:{m[5]}" if m else v.name


def convert_clip(src: Path, dst: Path):
    fade_start = MAX_SOUND_SECONDS - 2
    cmd = ["ffmpeg", "-y", "-loglevel", "error", "-i", str(src), "-vn", "-t", str(MAX_SOUND_SECONDS),
           "-af", f"loudnorm=I=-18:TP=-2,afade=t=out:st={fade_start}:d=2", "-ar", "44100", "-ac", "2", "-c:a", "pcm_s16le", str(dst)]
    try:
        subprocess.run(cmd, check=True)
        return True
    except FileNotFoundError:
        sys.exit("ffmpeg is not installed (macOS: brew install ffmpeg)")
    except subprocess.CalledProcessError:
        return False


def cmd_sync(sd: Path, ask=input):
    """After adding games: fetch missing art, then match new title-screen captures to games missing a soundtrack."""
    print("== 1/3 Wallpapers: official art for games that don't have one yet ==")
    cmd_art(sd)

    print()
    print("== 2/3 Soundtracks from your video captures ==")
    db = load_titledb()
    names = all_names(sd, db)
    sound_dir = sd / "ulaunch/steamui/sound"
    sound_dir.mkdir(parents=True, exist_ok=True)
    done_file = sd / "ulaunch/steamui/processed_captures.txt"
    done = set(done_file.read_text().split()) if done_file.exists() else set()
    def save_done():
        # saved after every answer, so an interrupted run never redoes work
        done_file.write_text("\n".join(sorted(done)) + ("\n" if done else ""))

    new_vids = [v for v in album_videos(sd) if v.name not in done]
    missing = [k for k in names if not (sound_dir / f"{k}.wav").exists()]
    if not new_vids:
        print("  No new video captures. On a game's title screen, hold Capture to save the last 30 s, then run sync again.")
    for v in new_vids:
        if not missing:
            print("  Every game already has a soundtrack.")
            break
        print(f"\n  Capture {v.name}  ({capture_time(v)})")
        for i, k in enumerate(missing, 1):
            print(f"    {i:2}. {names[k]}")
        try:
            ans = ask("  Which game is this? number, s = skip, i = not a title screen (ignore forever): ").strip().lower()
        except (EOFError, KeyboardInterrupt):
            print("\n  Stopped. Run sync again to continue where you left off.")
            break
        if ans == "i":
            done.add(v.name)
            save_done()
            continue
        if not ans.isdigit() or not (1 <= int(ans) <= len(missing)):
            continue
        key = missing[int(ans) - 1]
        if convert_clip(v, sound_dir / f"{key}.wav"):
            print(f"  ok    {names[key]} now plays this soundtrack")
            missing.remove(key)
            done.add(v.name)
            save_done()
        else:
            print("  fail  ffmpeg could not read this video")
    print()
    print("== 3/3 Status ==")
    cmd_list(sd)


def load_titledb(region="US.en"):
    import requests
    CACHE_DIR.mkdir(parents=True, exist_ok=True)
    cache = CACHE_DIR / f"{region}.json"
    if not cache.exists() or time.time() - cache.stat().st_mtime > 14 * 86400:
        print(f"Downloading game database ({region}), this is large and only happens every 2 weeks...")
        with requests.get(TITLEDB_URL.format(region=region), stream=True, timeout=60) as r:
            r.raise_for_status()
            with open(cache, "wb") as f:
                for chunk in r.iter_content(1 << 20):
                    f.write(chunk)
    raw = json.loads(cache.read_text(encoding="utf-8"))
    by_id = {}
    for entry in raw.values():
        tid = (entry.get("id") or "").upper()
        if len(tid) == 16:
            by_id[tid] = entry
    return by_id


def cmd_list(sd: Path):
    names = all_names(sd, load_titledb())
    for tid, name in names.items():
        art = "art" if (sd / "ulaunch/steamui/bg" / f"{tid}.jpg").exists() else "-"
        snd = "sound" if (sd / "ulaunch/steamui/sound" / f"{tid}.wav").exists() else "-"
        print(f"{tid}  {art:5} {snd:6} {name}")


def cmd_art(sd: Path, force=False):
    import requests
    from PIL import Image
    db = load_titledb()
    out = sd / "ulaunch/steamui/bg"
    out.mkdir(parents=True, exist_ok=True)
    ids = installed_app_ids(sd)
    if not ids:
        print("No games found yet. Boot the steamui home menu once (on emuMMC) so it lists your games")
        print("on the SD card, then run this again.")
        return
    for tid in ids:
        dst = out / f"{tid}.jpg"
        entry = db.get(tid) or db.get(tid[:-3] + "000") or {}
        name = entry.get("name") or tid
        if dst.exists() and not force:
            print(f"skip  {name} (already has art)")
            continue
        url = entry.get("bannerUrl") or (entry.get("screenshots") or [None])[0]
        if not url:
            print(f"none  {name}: no official art in the database")
            continue
        try:
            img = Image.open(io.BytesIO(requests.get(url, timeout=30).content)).convert("RGB")
        except Exception as e:
            print(f"fail  {name}: {e}")
            continue
        # cover-fit to 1280x720 (the menu scales it up smoothly; half the memory/decode time of 1080p)
        save_cover(img, dst)
        print(f"ok    {name}")


def cmd_sounds(sd: Path, clips: Path):
    names = all_names(sd, load_titledb())
    out = sd / "ulaunch/steamui/sound"
    out.mkdir(parents=True, exist_ok=True)

    for clip in sorted(p for p in clips.iterdir() if p.is_file() and not p.name.startswith(".")):
        tid = match_key(clip.stem, names)
        if tid is None:
            print(f"skip  {clip.name}: no game or homebrew matches this name (rename it to the title ID)")
            continue
        dst = out / f"{tid}.wav"
        if convert_clip(clip, dst):
            print(f"ok    {clip.name} -> {names.get(tid, tid)}")
        else:
            print(f"fail  {clip.name}: ffmpeg could not convert it")


def main():
    if len(sys.argv) < 3 or sys.argv[1] not in ("art", "sounds", "list", "images", "sync"):
        print(__doc__)
        sys.exit(1)
    sd = Path(sys.argv[2]).expanduser()
    if not (sd / "ulaunch").is_dir():
        sys.exit(f"{sd} doesn't look like your SD card (no ulaunch folder). Install uLaunch first.")
    if sys.argv[1] == "sync":
        cmd_sync(sd)
    elif sys.argv[1] == "list":
        cmd_list(sd)
    elif sys.argv[1] == "images":
        if len(sys.argv) < 4:
            sys.exit("Give the folder with your images, e.g. steamui_media.py images /Volumes/SWITCH ~/Desktop/art")
        cmd_images(sd, Path(sys.argv[3]).expanduser())
    elif sys.argv[1] == "art":
        cmd_art(sd, force="--force" in sys.argv)
    else:
        if len(sys.argv) < 4:
            sys.exit("Give the folder with your sound clips, e.g. steamui_media.py sounds /Volumes/SWITCH ~/Desktop/clips")
        cmd_sounds(sd, Path(sys.argv[3]).expanduser())


if __name__ == "__main__":
    main()
