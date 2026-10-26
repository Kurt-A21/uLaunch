#!/usr/bin/env python3
"""
get_homebrew.py - downloads the latest official releases of the essential Switch homebrew straight
from each project's GitHub "Releases" page and puts them in the right place on your SD card.

  python3 get_homebrew.py /Volumes/SWITCH                 # core + recommended apps
  python3 get_homebrew.py /Volumes/SWITCH --dry-run       # show what would be done
  python3 get_homebrew.py /Volumes/SWITCH --only jksv,ftpd
  python3 get_homebrew.py /Volumes/SWITCH --with-missioncontrol
  python3 get_homebrew.py --list

Nothing is deleted: files are added or replaced. Back up the SD card first anyway.
No extra Python packages needed (standard library only).
"""
import argparse, io, json, os, re, sys, urllib.request, zipfile
from pathlib import Path, PurePosixPath

API = os.environ.get("STEAMUI_GITHUB_API", "https://api.github.com")

# kind: "nro" -> copied to dest; "zip_sd" -> zip extracted onto the SD root;
#       "ovl" -> copied to switch/.overlays; "payload" -> copied to bootloader/payloads
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
    # --- optional ---
    dict(key="missioncontrol", group="optional", repo="ndeadly/MissionControl", desc="MissionControl: PS4/PS5/Xbox controllers over Bluetooth",
         files=[(r"^MissionControl-.*\.zip$", "zip_sd", None)]),
]

SAFE_TOP_LEVEL = {"atmosphere", "bootloader", "switch", "config", "ulaunch", "themes", "payload.bin", "hbmenu.nro", "boot.dat", "boot.ini"}


def http_json(url):
    req = urllib.request.Request(url, headers={"Accept": "application/vnd.github+json", "User-Agent": "steamui-get-homebrew"})
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.load(r)


def http_bytes(url):
    req = urllib.request.Request(url, headers={"User-Agent": "steamui-get-homebrew", "Accept": "application/octet-stream"})
    with urllib.request.urlopen(req, timeout=120) as r:
        return r.read()


def safe_extract(data, sd: Path, dry):
    """Extract a release zip onto the SD root, refusing anything outside the usual SD folders."""
    z = zipfile.ZipFile(io.BytesIO(data))
    names = [n for n in z.namelist() if not n.endswith("/")]
    # Some zips wrap everything in one folder (e.g. "SdOut/"): strip it if the next level looks like an SD root
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


def install(pkg, sd: Path, dry):
    rel = http_json(f"{API}/repos/{pkg['repo']}/releases/latest")
    assets = rel.get("assets", [])
    print(f"  {pkg['desc']}  [{rel.get('tag_name', '?')}]")
    done = 0
    for pattern, kind, dest in pkg["files"]:
        asset = next((a for a in assets if re.match(pattern, a["name"], re.I)), None)
        if asset is None:
            continue
        data = b"" if dry else http_bytes(asset["browser_download_url"])
        if kind == "zip_sd":
            if dry:
                print(f"      would extract {asset['name']} onto the SD card")
            else:
                n = len(safe_extract(data, sd, dry))
                print(f"      extracted {asset['name']} ({n} files)")
        else:
            if kind == "ovl":
                dest = f"switch/.overlays/{asset['name']}"
            target = sd / dest
            print(f"      {'would copy' if dry else 'copied'} {asset['name']} -> {dest}")
            if not dry:
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
        done += 1
        if kind in ("zip_sd",) and pkg["key"] == "statusmonitor":
            break  # zip already contains the .ovl
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
    a = ap.parse_args()

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
    if any(p["key"] == "missioncontrol" for p in pkgs):
        print("MissionControl: if the console won't boot after an Atmosphere update, delete atmosphere/contents/010000000000BD00")


if __name__ == "__main__":
    main()
