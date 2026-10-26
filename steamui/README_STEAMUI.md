# steamui: a SteamOS-style home menu for uLaunch

This is a fork of [uLaunch](https://github.com/XorTroll/uLaunch) 1.2.5. It replaces the Switch home menu with:

- **The game row along the bottom.** The bottom nav bar and button hints are gone.
- **Hover art.** The hovered game's art fills the screen, crossfading in with a slow zoom.
- **Hover sound.** The hovered game's sound plays once you stop on it, and the menu music ducks underneath.
- **Console-style feel.**
  - The focused tile pops up on a spring, with a slight overshoot like the PS5/Xbox menus.
  - The title slides up as it fades in.
  - Launching a game gives the tile a press dip.
  - Every move gets a light HD Rumble tick, and opening the menu or launching a game gives a stronger one. Turn this off in UI.json with `main_menu.haptics.enabled`.
- **Smooth loading.** Art and sounds load on a background thread, so scrolling never stutters on SD card reads. Recently viewed art is cached, so going back and forth is instant.
- **Homebrew on the home screen.** Every app in `switch/` (and its direct subfolders) is added to the row automatically, next to your games.
- **Burger menu (top right).** Tap it, or press ZL/ZR, and the old nav bar (Power, Controllers, Album, Web, User, Themes, Settings, Mii, amiibo) slides down as a list.

Theme: **Deck Night** (`ulaunch/themes/DeckNight.ultheme`).

## Requirements

- Atmosphere 1.12.1 (or anything recent that supports your firmware).
- Firmware 21.2.0. Anything up to 22.5.0 works; **23.x is not supported by uLaunch yet.**
- **The steamui build replaces the stock home menu (and NXThemes).** uLaunch lives in `atmosphere/contents/0100000000001000`, the same folder NXThemes uses. So:
  - remove any installed NXTheme before installing uLaunch
  - don't use NXThemes Installer while uLaunch is installed

## Install

1. Back up your SD card.
2. Copy the contents of `SdOut/` to the SD card root (merge).
3. Boot CFW, then pick your user. Go to **burger menu → Themes** and choose **Deck Night**.
4. Add hover art and sounds (optional, see below).

**To uninstall:** delete `atmosphere/contents/0100000000001000`. This gives you the stock home menu back.

## Homebrew apps and ports

New `.nro` files in `switch/` or `switch/<folder>/` are added to the home screen the next time the menu loads.

- **Removing apps.** If you remove one from the menu, it stays removed. The list of apps already added is kept in `ulaunch/steamui/hb_known.txt`; delete that file to add everything again.
- **Full memory for big ports.** Large ports (NFS Carbon, the N64 ports, etc.) need full memory. To set that up once:
  1. Pick any installed game you don't mind lending.
  2. Press **X** on it, then choose **Select as homebrew donor/take-over application**.
  3. From then on, homebrew starts in place of that game, with full memory (like holding R in hbmenu).
- **Art and sound for homebrew.** These use the NRO file name instead of a title ID. For example, `switch/nfscx/nfscx-vulkan.nro` gets its art from `ulaunch/steamui/bg/nfscx-vulkan.jpg`.

## Hover art and sounds

Files go on the SD card. `APPID` is the game's 16-digit title ID.

| What | Where | Format |
|---|---|---|
| Background art | `ulaunch/steamui/bg/<APPID>.jpg` (or `.png`) | 1280×720 baseline JPEG (1920×1080 also works but uses more memory) |
| Soundtrack clip | `ulaunch/steamui/sound/<APPID>.wav` | 44.1 kHz WAV, ≤30 s |

**After adding games, run one command:** `python3 steamui_media.py sync /Volumes/SWITCH`. It:

1. Downloads official eShop art for every game that doesn't have a wallpaper yet. New games are detected automatically from the home menu's game list on the SD card (`ulaunch/emu_<user>/` on emuMMC), so boot the menu once after installing games. The download runs on your computer, so it works even though emuMMC is kept offline.
2. Lists your new title-screen video captures (sysMMC and emuMMC albums) one by one and asks which game each is. It turns the audio into that game's soundtrack and remembers what's done, so captures are never asked about twice.
3. Shows which games still lack art or sound.

Individual commands, if you prefer:

```
pip3 install pillow requests        # once
brew install ffmpeg                 # once, only needed for sounds

python3 steamui_media.py list   /Volumes/SWITCH            # what's installed / missing
python3 steamui_media.py art    /Volumes/SWITCH            # official eShop art for every installed game
python3 steamui_media.py sounds /Volumes/SWITCH ~/clips    # converts your clips (named by game or title ID)
python3 steamui_media.py images /Volumes/SWITCH ~/art      # your own images, e.g. for homebrew (named by game/app)
```

### Getting each game's main theme

The clips come from your own games:

1. Start a game and wait on its title screen, where its main theme plays.
2. Hold the **Capture** button to save the last 30 seconds as a video.
3. Repeat for each game.
4. Copy the videos from `Nintendo/Album` on the SD card into a folder, and rename each one after its game, e.g. `Mario Kart 8 Deluxe.mp4`.
5. Run `steamui_media.py sounds`. It takes the audio from each video, evens out the loudness, and fades out the end.

Some games block video capture. For those, use an audio file of the game's soundtrack from an album you own.

## Overclocking (sys-clk)

sys-clk sets the CPU, GPU and RAM clocks for each game, by title ID, in `config/sys-clk/config.ini`. There are two ways to set them:

- **On the console:** open the Ultrahand overlay (L + D-pad Down + right stick), go to **sys-clk**, and change clocks for the running game.
- **From your computer:** use `oc_profile.py` with a preset:

```
python3 oc_profile.py add /Volumes/SWITCH "Mario Kart 8 Deluxe" performance   # or balanced / battery
python3 oc_profile.py show /Volumes/SWITCH
```

| Preset | Docked | Handheld |
|---|---|---|
| performance | CPU 1785, GPU 921, RAM 1600 MHz | CPU 1785, GPU at the safe max, RAM 1600 MHz |
| balanced | CPU 1428, GPU 844, RAM 1600 MHz | CPU 1224, GPU 460, RAM 1600 MHz |
| battery | stock | CPU 816, GPU 307, RAM 1065 MHz |

The handheld GPU safe max is 460 MHz on original Switches and 614 MHz on newer models (V2, Lite, OLED). sys-clk applies these limits by itself, and also limits the GPU to 768 MHz on non-official chargers.

**Ports and homebrew:** they run as your take-over game, so put the profile on that game's title ID. The NFS Carbon port deliberately ignores its own clock settings while sys-clk is running.

**Checking it works on your console:**
- Open Status Monitor (in Ultrahand) while a game is running; its clocks should match the profile.
- Or set `csv_write_interval_ms=1000` in `config.ini`, play for a minute, and look at `config/sys-clk/context.csv`.

**Tested:** `steamui/tests/overclock/run_overclock_test.sh` runs sys-clk's actual config parser and clock manager against these profiles. It checks every power state (handheld, USB charger, official charger, docked) on both chip types, plus leaving a game and docking mid-game.

## Theme options (UI.json)

These keys are new in this fork:

- `main_menu.entry_menu.fixed_row_count`: locks the number of rows (Deck Night uses 1). +/- resizing is disabled while it's set.
- `main_menu.burger_icon`: position of the burger button. Its image is `ui/Main/BurgerIcon.png`. Themes without that image show no button.
- `main_menu.haptics.enabled`: HD Rumble ticks on/off.
- `quick_menu.quick_menu.slide_down`: `true` turns the quick menu into a dropdown. Related keys: `width`, `item_height`, `items_to_show`, `dim_alpha`.

## Essential homebrew apps

`get_homebrew.py` downloads the latest official releases straight from each project's GitHub page and puts them on the SD card. It never deletes anything, and it keeps your `hekate_ipl.ini`.

```
python3 get_homebrew.py --list
python3 get_homebrew.py /Volumes/SWITCH --dry-run       # preview
python3 get_homebrew.py /Volumes/SWITCH                 # Atmosphere, hekate, sys-clk + the apps below
python3 get_homebrew.py /Volumes/SWITCH --with-missioncontrol
```

| App | What it's for |
|---|---|
| JKSV | Back up and restore save data. Do this before anything risky. |
| NX-Shell | File manager |
| ftpd | Copy files over Wi-Fi |
| Moonlight | Stream games from your PC (e.g. your PC copy of Metal Gear Rising) |
| Homebrew App Store | Browse and update homebrew |
| Ultrahand | Overlay menu (L + D-pad Down + right stick), hosting the sys-clk and Status Monitor overlays |
| Status Monitor | Live temperatures and clocks while overclocking |
| MissionControl (optional) | Use PS4/PS5/Xbox controllers over Bluetooth |

All of them show up on the home screen automatically.

## Tests

`steamui/tests/run_tests.sh` runs on any Linux or Mac with g++/clang and Python + Pillow. GitHub runs it on every push. It covers:
- The hover art/sound logic, with memory and thread sanitizers.
- Homebrew auto-adding.
- `validate_theme.py`, which checks the theme against the menu code and checks the screen layout.

## Building

GitHub Actions builds everything on each push, using `.github/workflows/main.yml`. Download the `SdOut` artifact from the run.

To build locally, follow upstream uLaunch's instructions (devkitPro + `make`).
