#!/usr/bin/env python3
"""Runs get_homebrew.py against a fake GitHub (local HTTP server) and checks where sphaira, NXThemes Installer
and the theme patches land, plus that existing NXThemes/config files are left alone."""
import io, json, os, subprocess, sys, tempfile, threading, zipfile
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

TOOL = Path(__file__).resolve().parent.parent / "tools/get_homebrew.py"
fails = 0


def check(cond, msg):
    global fails
    print(("  ok    " if cond else "  FAIL  ") + msg)
    fails += 0 if cond else 1


def mkzip(files):
    b = io.BytesIO()
    with zipfile.ZipFile(b, "w") as z:
        for n, d in files.items():
            z.writestr(n, d)
    return b.getvalue()


ROUTES = {}


class H(BaseHTTPRequestHandler):
    def log_message(self, *a): pass

    def do_GET(self):
        body = ROUTES.get(self.path)
        if body is None:
            self.send_response(404); self.end_headers(); return
        if not isinstance(body, bytes):
            body = json.dumps(body).encode()
        self.send_response(200); self.end_headers(); self.wfile.write(body)


def asset(name):
    return {"name": name, "browser_download_url": f"{BASE}/dl/{name}"}


srv = ThreadingHTTPServer(("127.0.0.1", 0), H)
BASE = f"http://127.0.0.1:{srv.server_address[1]}"
threading.Thread(target=srv.serve_forever, daemon=True).start()

ROUTES.update({
    # sphaira: SD-layout zip (+ its own hbmenu.nro) and a bare nro; only one should be used
    "/repos/ITotalJustice/sphaira/releases/latest": {"tag_name": "1.0.8", "assets": [asset("sphaira.zip"), asset("sphaira.nro")]},
    "/dl/sphaira.zip": mkzip({"switch/sphaira/sphaira.nro": b"SPHAIRA-ZIP", "hbmenu.nro": b"SPHAIRA-HBMENU"}),
    "/dl/sphaira.nro": b"SPHAIRA-BARE",
    # NXThemes: latest release is the PC injector (no .nro) -> must fall back to the newest installer release
    "/repos/exelix11/SwitchThemeInjector/releases/latest": {"tag_name": "v4.8.1", "assets": [asset("SwitchThemes.zip")]},
    "/repos/exelix11/SwitchThemeInjector/releases?per_page=20": [
        {"tag_name": "v4.8.1", "assets": [asset("SwitchThemes.zip")]},
        {"tag_name": "nxt-3.1-beta", "prerelease": True, "assets": [asset("NXThemesInstaller.nro")]},
        {"tag_name": "nxt-3.0.2", "assets": [asset("NXThemesInstaller.nro")]}],
    "/dl/NXThemesInstaller.nro": b"NXTHEMES",
    "/exelix11/theme-patches/archive/refs/heads/master.zip": mkzip({
        "theme-patches-master/README.md": b"readme",
        "theme-patches-master/systemPatches/ResidentMenu/21.2.0.ips": b"PATCH",
        "theme-patches-master/systemPatches/Entrance/21.2.0.ips": b"PATCH2"}),
})

env = dict(os.environ, STEAMUI_GITHUB_API=BASE, STEAMUI_GITHUB_WEB=BASE)
with tempfile.TemporaryDirectory() as d:
    sd = Path(d)
    lyt = sd / "atmosphere/contents/0100000000001000/romfs/lyt/ResidentMenu.szs"
    lyt.parent.mkdir(parents=True); lyt.write_bytes(b"MY-NXTHEME")
    (sd / "atmosphere/contents/0100000000001000/exefs.nsp").write_bytes(b"ULAUNCH")
    (sd / "config/sphaira").mkdir(parents=True); (sd / "config/sphaira/config.ini").write_bytes(b"MINE")
    (sd / "atmosphere/hbmenu.nro").write_bytes(b"x")
    out = subprocess.run([sys.executable, "-I", str(TOOL), str(sd), "--only", "sphaira,nxthemes,themepatches"], env=env, capture_output=True, text=True)
    print(out.stdout.rstrip())
    print("== Results ==")
    check(out.returncode == 0 and "need attention" not in out.stdout, "all three installed without errors")
    check((sd / "switch/sphaira/sphaira.nro").read_bytes() == b"SPHAIRA-ZIP", "sphaira: release zip used, app in switch/sphaira/ (shows on the steamui home row)")
    check((sd / "hbmenu.nro").read_bytes() == b"SPHAIRA-HBMENU", "sphaira: its hbmenu.nro replaces the stock homebrew menu")
    check((sd / "switch/NXThemesInstaller/NXThemesInstaller.nro").read_bytes() == b"NXTHEMES", "NXThemes Installer: found in the newest installer release (not the PC injector, not a beta)")
    check("nxt-3.0.2" in out.stdout, "NXThemes: reports the installer release it used")
    check((sd / "themes/systemPatches/ResidentMenu/21.2.0.ips").read_bytes() == b"PATCH" and (sd / "themes/systemPatches/Entrance/21.2.0.ips").exists(), "theme patches in themes/systemPatches (works offline on emuMMC)")
    check(not (sd / "themes/systemPatches/README.md").exists() and not (sd / "themes/README.md").exists(), "only the systemPatches folder copied")
    check(lyt.read_bytes() == b"MY-NXTHEME", "an installed NXTheme is left alone")
    check((sd / "atmosphere/contents/0100000000001000/exefs.nsp").read_bytes() == b"ULAUNCH", "steamui (uLaunch) stays active")
    check((sd / "config/sphaira/config.ini").read_bytes() == b"MINE", "existing sphaira settings kept")

    # A release that ships only a zip with the bare app inside
    ROUTES["/repos/ITotalJustice/sphaira/releases/latest"] = {"tag_name": "1.0.9", "assets": [asset("sphaira-1.0.9.zip")]}
    ROUTES["/dl/sphaira-1.0.9.zip"] = mkzip({"sphaira.nro": b"SPHAIRA-109"})
    out = subprocess.run([sys.executable, "-I", str(TOOL), str(sd), "--only", "sphaira"], env=env, capture_output=True, text=True)
    check((sd / "switch/sphaira/sphaira.nro").read_bytes() == b"SPHAIRA-109", "sphaira: a zip with just sphaira.nro still lands in switch/sphaira/")

    # Dry run touches nothing
    before = sorted(p.relative_to(sd).as_posix() for p in sd.rglob("*"))
    subprocess.run([sys.executable, "-I", str(TOOL), str(sd), "--only", "sphaira,nxthemes,themepatches", "--dry-run"], env=env, capture_output=True, text=True)
    check(before == sorted(p.relative_to(sd).as_posix() for p in sd.rglob("*")), "--dry-run changes nothing")

# ---- Performance + emulator apps ----
ROUTES.update({
    # SaltyNX: GitHub's "latest" is still 1.9.2, but FPSLocker 3.5 needs 2.x, which is marked experimental
    "/repos/masagrator/SaltyNX/releases/latest": {"tag_name": "1.9.2", "assets": [asset("SaltyNX.zip")]},
    "/repos/masagrator/SaltyNX/releases?per_page=20": [
        {"tag_name": "2.1.0", "prerelease": True, "assets": [asset("SaltyNX_Debug.zip"), asset("SaltyNX-2.1.0.zip")]},
        {"tag_name": "1.9.2", "assets": [asset("SaltyNX.zip")]}],
    "/dl/SaltyNX-2.1.0.zip": b"",
    "/repos/masagrator/FPSLocker/releases/latest": {"tag_name": "3.5.0", "assets": [asset("Debug.zip"), asset("FPSLocker.ovl")]},
    "/dl/FPSLocker.ovl": b"FPSLOCKER",
    "/repos/Zathawo/NX-FanControl/releases/latest": {"tag_name": "1.0.3", "assets": [asset("NXFanControl1.0.3.zip")]},
    "/dl/NXFanControl1.0.3.zip": mkzip({
        "atmosphere/contents/00FF0000B378D640/exefs.nsp": b"FAN-SYS", "atmosphere/contents/00FF0000B378D640/flags/boot2.flag": b"",
        "atmosphere/contents/00FF0000B378D640/toolbox.json": b"{}",
        "switch/.overlays/NX-FanControl.ovl": b"FAN-OVL", "config/NX-FanControl/config.dat": b"DEFAULT-CURVE"}),
    "/repos/ticohq/tico/releases/latest": {"tag_name": "0.7.8", "assets": [asset("tico.nro")]},
    "/dl/tico.nro": b"TICO",
})
# The 2.1.0 zip: SaltySD folder + its sysmodule
ROUTES["/repos/masagrator/SaltyNX/releases?per_page=20"][0]["assets"][1] = asset("SaltyNX.zip")
ROUTES["/dl/SaltyNX.zip"] = mkzip({"SaltySD/saltysd_core.elf": b"CORE-2.1.0", "SaltySD/plugins/README.md": b"",
                                   "atmosphere/contents/0000000000534C56/exefs.nsp": b"SALTY-SYS",
                                   "atmosphere/contents/0000000000534C56/flags/boot2.flag": b""})
with tempfile.TemporaryDirectory() as d:
    sd = Path(d)
    (sd / "config/NX-FanControl").mkdir(parents=True); (sd / "config/NX-FanControl/config.dat").write_bytes(b"MY-CURVE")
    out = subprocess.run([sys.executable, "-I", str(TOOL), str(sd), "--only", "fpslocker,fancontrol,tico"], env=env, capture_output=True, text=True)
    print(out.stdout.rstrip())
    print("== Results ==")
    check(out.returncode == 0 and "need attention" not in out.stdout, "FPSLocker, NX-FanControl, tico installed without errors")
    check((sd / "SaltySD/saltysd_core.elf").read_bytes() == b"CORE-2.1.0", "--only fpslocker also installs SaltyNX, and the 2.x it needs (not GitHub's 'latest' 1.9.2)")
    check("experimental" in out.stdout, "says SaltyNX 2.x is marked experimental by its author")
    check((sd / "atmosphere/contents/0000000000534C56/flags/boot2.flag").exists(), "SaltyNX sysmodule installed and set to start at boot")
    check((sd / "switch/.overlays/FPSLocker.ovl").read_bytes() == b"FPSLOCKER", "FPSLocker overlay in switch/.overlays (shows in Ultrahand)")
    check((sd / "atmosphere/contents/00FF0000B378D640/exefs.nsp").exists() and (sd / "switch/.overlays/NX-FanControl.ovl").exists(), "NX-FanControl sysmodule + overlay installed")
    check((sd / "config/NX-FanControl/config.dat").read_bytes() == b"MY-CURVE", "an existing fan curve is kept")
    check("delete atmosphere/contents/00FF0000B378D640/flags/boot2.flag" in out.stdout and "0000000000534C56" in out.stdout, "tells you how to turn each new sysmodule off")
    check((sd / "switch/tico/tico.nro").read_bytes() == b"TICO", "tico in switch/tico/tico.nro (the path it requires; shows on the home row)")

srv.shutdown()
print("ALL PASSED" if not fails else f"FAILED: {fails}")
sys.exit(1 if fails else 0)
