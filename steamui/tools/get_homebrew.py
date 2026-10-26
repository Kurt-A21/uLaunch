#!/usr/bin/env python3
"""
get_homebrew.py - downloads the latest official releases of the essential Switch homebrew straight
from each project's GitHub "Releases" page and puts them in the right place on your SD card.

  python3 get_homebrew.py /Volumes/SWITCH                 # core + recommended apps
  python3 get_homebrew.py /Volumes/SWITCH --dry-run       # show what would be done
  python3 get_homebrew.py /Volumes/SWITCH --only jksv,ftpd
  python3 get_homebrew.py /Volumes/SWITCH --with-missioncontrol
  python3 get_homebrew.py --list
  python3 get_homebrew.py /Volumes/SWITCH --only atmosphere --pin atmosphere=1.11.2   # a specific release

Nothing is deleted: files are added or replaced. Back up the SD card first anyway.
No extra Python packages needed (standard library only).
"""
import argparse, io, json, os, re, sys, urllib.parse, urllib.request, zipfile
from pathlib import Path, PurePosixPath

API = os.environ.get("STEAMUI_GITHUB_API", "https://api.github.com")
WEB = os.environ.get("STEAMUI_GITHUB_WEB", "https://github.com")

# kind: "nro" -> copied to dest; "zip_sd" -> zip extracted onto the SD root (dest, if given, is where a bare .nro
#       at the zip's top level goes); "ovl" -> copied to switch/.overlays; "payload" -> copied to bootloader/payloads
# first_only: stop after the first matching download. any_release: if the latest release lacks the file, use the
# newest release that has it. newest: use the newest release that has the file, even one marked pre-release (for
# pairs that must match, like SaltyNX + FPSLocker). needs: other packages installed along with it.
# source: (branch, folder, dest) -> that folder of the repo's source zip is copied to dest.
PACKAGES = [
    # --- core (CFW itself) ---
    dict(key="atmosphere", group="core", repo="Atmosphere-NX/Atmosphere", desc="Atmosphere CFW",
         files=[(r"^atmosphere-.*\.zip$", "zip_sd", None), (r"^fusee\.bin$", "payload", "bootloader/payloads/fusee.bin")]),
    dict(key="hekate", group="core", repo="CTCaer/hekate", desc="hekate bootloader",
         files=[(r"^hekate_ctcaer_.*\.zip$", "zip_sd", None)]),
    dict(key="sysclk", group="core", repo="retronx-team/sys-clk", desc="sys-clk overclocking (+ overlay)",
         files=[(r"^sys-clk.*\.zip$", "zip_sd", None)]),
    # --- recommended apps ---
    dict(key="jksv", group="apps", repo="J-D-K/JKSV", desc="JKSV: save data backup/restore",
         files=[(r"^JKSV\.nro$", "nro", "switch/JKSV/JKSV.nro")]),
    dict(key="nxshell", group="apps", repo="joel16/NX-Shell", desc="NX-Shell: file manager",
         files=[(r"^NX-Shell\.nro$", "nro", "switch/NX-Shell/NX-Shell.nro")]),
    dict(key="ftpd", group="apps", repo="mtheall/ftpd", desc="ftpd: wireless file transfer (FTP)",
         files=[(r"^ftpd\.nro$", "nro", "switch/ftpd/ftpd.nro")]),
    dict(key="moonlight", group="apps", repo="XITRIX/Moonlight-Switch", desc="Moonlight: stream games from your PC",
         files=[(r"^Moonlight-Switch\.nro$", "nro", "switch/Moonlight-Switch/Moonlight-Switch.nro")]),
    dict(key="appstore", group="apps", repo="fortheusers/hb-appstore", desc="Homebrew App Store",
         files=[(r"^switch-extracttosd\.zip$", "zip_sd", None)]),
    dict(key="ultrahand", group="apps", repo="ppkantorski/Ultrahand-Overlay", desc="Ultrahand: overlay menu (L+DDown+RS)",
         files=[(r"^(sdout|Ultrahand.*)\.zip$", "zip_sd", None)]),
    dict(key="statusmonitor", group="apps", repo="masagrator/Status-Monitor-Overlay", desc="Status Monitor: temps/clocks overlay",
         files=[(r"^Status-Monitor-Overlay.*\.zip$", "zip_sd", None), (r"^Status-Monitor-Overlay\.ovl$", "ovl", None)]),
    dict(key="sphaira", group="apps", repo="ITotalJustice/sphaira", desc="sphaira: homebrew menu (+ Themezer theme browser, file browser)", first_only=True,
         files=[(r"^sphaira.*\.zip$", "zip_sd", "switch/sphaira/sphaira.nro"), (r"^sphaira\.nro$", "nro", "switch/sphaira/sphaira.nro")]),
    dict(key="nxthemes", group="apps", repo="exelix11/SwitchThemeInjector", desc="NXThemes Installer: themes for Nintendo's HOME menu", any_release=True,
         files=[(r"^NXThemesInstaller.*\.nro$", "nro", "switch/NXThemesInstaller/NXThemesInstaller.nro")]),
    dict(key="themepatches", group="apps", repo="exelix11/theme-patches", desc="Theme patches for NXThemes (it can't download them on an offline emuMMC)",
         source=("master", "systemPatches", "themes/systemPatches")),
    dict(key="saltynx", group="apps", repo="masagrator/SaltyNX", desc="SaltyNX: game plugin loader (needed by FPSLocker)", newest=True,
         files=[(r"^SaltyNX\.zip$", "zip_sd", None)]),
    dict(key="fpslocker", group="apps", repo="masagrator/FPSLocker", desc="FPSLocker: custom FPS / refresh rate per game (overlay)", needs=["saltynx"],
         files=[(r"^FPSLocker\.ovl$", "ovl", None)]),
    dict(key="fancontrol", group="apps", repo="Zathawo/NX-FanControl", desc="NX-FanControl: custom fan curve (sysmodule + overlay)",
         files=[(r"^NX-?FanControl.*\.zip$", "zip_sd", None)]),
    dict(key="tico", group="apps", repo="ticohq/tico", desc="tico: emulator frontend (N64, PS1, Saturn, Dreamcast, GameCube/Wii, 3DS...)", first_only=True,
         files=[(r"^tico\.nro$", "nro", "switch/tico/tico.nro"), (r"^tico.*\.zip$", "zip_sd", "switch/tico/tico.nro")]),
    # --- optional ---
    dict(key="missioncontrol", group="optional", repo="ndeadly/MissionControl", desc="MissionControl: PS4/PS5/Xbox controllers over Bluetooth",
         files=[(r"^MissionControl-.*\.zip$", "zip_sd", None)]),
]

SAFE_TOP_LEVEL = {"atmosphere", "bootloader", "switch", "config", "ulaunch", "themes", "SaltySD", "tico", "payload.bin", "hbmenu.nro", "boot.dat", "boot.ini"}


def http_json(url):
    req = urllib.request.Request(url, headers={"Accept": "application/vnd.github+json", "User-Agent": "steamui-get-homebrew"})
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.load(r)


def http_bytes(url):
    req = urllib.request.Request(url, headers={"User-Agent": "steamui-get-homebrew", "Accept": "application/octet-stream"})
    with urllib.request.urlopen(req, timeout=120) as r:
        return r.read()


def safe_extract(data, sd: Path, dry, nro_dest=None):
    """Extract a release zip onto the SD root, refusing anything outside the usual SD folders."""
    z = zipfile.ZipFile(io.BytesIO(data))
    names = [n for n in z.namelist() if not n.endswith("/")]
    # A zip holding just the app (e.g. "sphaira.nro" at the top) -> put it where the package says
    if nro_dest and all(len(PurePosixPath(n).parts) == 1 for n in names):
        nros = [n for n in names if n.lower().endswith(".nro")]
        if len(nros) == 1:
            if not dry:
                (sd / nro_dest).parent.mkdir(parents=True, exist_ok=True)
                (sd / nro_dest).write_bytes(z.read(nros[0]))
            return [sd / nro_dest]
    tops = {PurePosixPath(n).parts[0] for n in names}
    strip = 0
    if len(tops) == 1 and not (tops & SAFE_TOP_LEVEL):
        inner = {PurePosixPath(n).parts[1] for n in names if len(PurePosixPath(n).parts) > 1}
        if inner & SAFE_TOP_LEVEL:
            strip = 1
    written = []
    for n in names:
        parts = PurePosixPath(n).parts[strip:]
        if not parts or ".." in parts or parts[0].startswith("/") or "__MACOSX" in parts or parts[-1] == ".DS_Store":
            continue
        if parts[0] not in SAFE_TOP_LEVEL and not parts[0].endswith(".bin"):
            print(f"      skipped unexpected file in zip: {n}")
            continue
        if parts == ("bootloader", "hekate_ipl.ini") and (sd / "bootloader/hekate_ipl.ini").exists():
            print("      kept your existing bootloader/hekate_ipl.ini")
            continue
        if parts[0] == "config" and sd.joinpath(*parts).exists():
            # e.g. config/sys-clk/config.ini: never overwrite your settings/overclock profiles
            print(f"      kept your existing {'/'.join(parts)}")
            continue
        dst = sd.joinpath(*parts)
        written.append(dst)
        if not dry:
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.write_bytes(z.read(n))
    return written


def install_source_folder(pkg, sd: Path, dry):
    """Copies one folder of a repo's source zip (e.g. theme-patches' systemPatches) to the SD card."""
    branch, folder, dest = pkg["source"]
    print(f"  {pkg['desc']}  [{branch}]")
    if dry:
        print(f"      would copy {folder}/ from {pkg['repo']} -> {dest}/")
        return True
    z = zipfile.ZipFile(io.BytesIO(http_bytes(f"{WEB}/{pkg['repo']}/archive/refs/heads/{branch}.zip")))
    n = 0
    for name in z.namelist():
        parts = PurePosixPath(name).parts
        if name.endswith("/") or len(parts) < 3 or parts[1] != folder or ".." in parts:
            continue
        dst = sd.joinpath(*PurePosixPath(dest).parts, *parts[2:])
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_bytes(z.read(name))
        n += 1
    print(f"      copied {n} files -> {dest}/")
    return n > 0


PINS = {}  # key -> release tag (from --pin)


def pick_assets(pkg):
    """[(pattern, kind, dest, asset, tag)] for each file the package wants, from the latest release
    (or, with any_release, the newest release that has it)."""
    picks = []
    if pkg["key"] in PINS:
        rel = http_json(f"{API}/repos/{pkg['repo']}/releases/tags/{urllib.parse.quote(PINS[pkg['key']], safe='')}")
        for pattern, kind, dest in pkg["files"]:
            asset = next((a for a in rel.get("assets", []) if re.match(pattern, a["name"], re.I)), None)
            if asset is not None:
                picks.append((pattern, kind, dest, asset, rel.get("tag_name", "?") + " (pinned)"))
        return picks
    if pkg.get("newest"):
        for rel in http_json(f"{API}/repos/{pkg['repo']}/releases?per_page=20"):
            if rel.get("draft"):
                continue
            for pattern, kind, dest in pkg["files"]:
                asset = next((a for a in rel.get("assets", []) if re.match(pattern, a["name"], re.I)), None)
                if asset is not None:
                    tag = rel.get("tag_name", "?") + (" (marked pre-release/experimental by its author)" if rel.get("prerelease") else "")
                    picks.append((pattern, kind, dest, asset, tag))
            if picks:
                return picks
        return picks
    latest = http_json(f"{API}/repos/{pkg['repo']}/releases/latest")
    for pattern, kind, dest in pkg["files"]:
        asset = next((a for a in latest.get("assets", []) if re.match(pattern, a["name"], re.I)), None)
        tag = latest.get("tag_name", "?")
        if asset is None and pkg.get("any_release"):
            for rel in http_json(f"{API}/repos/{pkg['repo']}/releases?per_page=20"):
                if rel.get("draft") or rel.get("prerelease"):
                    continue
                asset = next((a for a in rel.get("assets", []) if re.match(pattern, a["name"], re.I)), None)
                if asset is not None:
                    tag = rel.get("tag_name", "?")
                    break
        if asset is not None:
            picks.append((pattern, kind, dest, asset, tag))
    return picks


def install(pkg, sd: Path, dry):
    if "source" in pkg:
        return install_source_folder(pkg, sd, dry)
    picks = pick_assets(pkg)
    print(f"  {pkg['desc']}  [{', '.join(sorted({p[4] for p in picks})) or '?'}]")
    done = 0
    for pattern, kind, dest, asset, tag in picks:
        data = b"" if dry else http_bytes(asset["browser_download_url"])
        if kind == "zip_sd":
            if dry:
                print(f"      would extract {asset['name']} onto the SD card")
            else:
                written = safe_extract(data, sd, dry, nro_dest=dest)
                print(f"      extracted {asset['name']} ({len(written)} files)")
                # Name any background sysmodules it added, so they're easy to turn off if needed
                for tid in sorted({p.relative_to(sd).parts[2] for p in written
                                   if p.relative_to(sd).parts[:2] == ("atmosphere", "contents") and p.name == "boot2.flag"}):
                    print(f"      sysmodule {tid}: starts at boot (to turn it off, delete atmosphere/contents/{tid}/flags/boot2.flag)")
        else:
            if kind == "ovl":
                dest = f"switch/.overlays/{asset['name']}"
            target = sd / dest
            print(f"      {'would copy' if dry else 'copied'} {asset['name']} -> {dest}")
            if not dry:
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
        done += 1
        if (kind == "zip_sd" and pkg["key"] == "statusmonitor") or pkg.get("first_only"):
            break  # e.g. the zip already contains the .ovl / the app
    if done == 0:
        print(f"      !! no matching download found - get it manually from https://github.com/{pkg['repo']}/releases")
    return done > 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("sd", nargs="?", help="SD card root, e.g. /Volumes/SWITCH")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--only", help="comma-separated keys (see --list)")
    ap.add_argument("--skip-core", action="store_true", help="don't touch Atmosphere/hekate/sys-clk")
    ap.add_argument("--with-missioncontrol", action="store_true")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--pin", action="append", default=[], metavar="KEY=TAG", help="install that release instead of the latest, e.g. atmosphere=1.11.2")
    a = ap.parse_args()
    for pin in a.pin:
        key, _, tag = pin.partition("=")
        if not tag or key not in {p["key"] for p in PACKAGES}:
            ap.error(f"--pin needs KEY=TAG with a key from --list, got '{pin}'")
        PINS[key] = tag

    if a.list:
        for p in PACKAGES:
            print(f"{p['key']:15} {p['group']:9} {p['desc']}")
        return
    if not a.sd:
        ap.error("give the SD card path")
    sd = Path(a.sd).expanduser()
    if not sd.is_dir():
        sys.exit(f"{sd} is not a folder")
    if not (sd / "atmosphere").is_dir() and not a.dry_run:
        print(f"note: {sd} has no atmosphere folder yet - continuing (fresh card?)")

    if a.only:
        wanted = {k.strip() for k in a.only.split(",")}
        for p in PACKAGES:
            if p["key"] in wanted:
                wanted.update(p.get("needs", []))
        pkgs = [p for p in PACKAGES if p["key"] in wanted]
    else:
        pkgs = [p for p in PACKAGES if p["group"] == "apps" or (p["group"] == "core" and not a.skip_core)
                or (p["key"] == "missioncontrol" and a.with_missioncontrol)]

    failed = []
    for p in pkgs:
        try:
            if not install(p, sd, a.dry_run):
                failed.append(p["key"])
        except Exception as e:
            print(f"      !! {p['key']} failed: {e}")
            failed.append(p["key"])
    print()
    print("Done." if not failed else f"Done, but these need attention: {', '.join(failed)}")
    if any(p["key"] in ("nxthemes", "themepatches") for p in pkgs):
        print("NXThemes: themes go in themes/ on the SD card. They show on Nintendo's HOME menu, i.e. after switching")
        print("  steamui off (burger menu -> Themes -> Nintendo HOME menu). uManager -> Activate brings steamui back.")
    if any(p["key"] == "fpslocker" for p in pkgs):
        print("FPSLocker: Ultrahand -> FPSLocker. Some games need a per-game config: use 'Check/download config file' there.")
    if any(p["key"] == "fancontrol" for p in pkgs):
        print("NX-FanControl: set the curve in Ultrahand -> NX-FanControl. Keep Status Monitor up to date (old versions")
        print("  that read the fan directly can make the console reboot while NX-FanControl runs).")
    if any(p["key"] == "tico" for p in pkgs):
        print("tico: start it once from the home screen to finish setup and download emulator cores (needs Wi-Fi).")
        print("  Put your own game dumps in tico/roms/<system>/ (e.g. tico/roms/n64, tico/roms/gc, tico/roms/wii).")
    if any(p["key"] == "missioncontrol" for p in pkgs):
        print("MissionControl: if the console won't boot after an Atmosphere update, delete atmosphere/contents/010000000000BD00")


if __name__ == "__main__":
    main()
