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
- **Profile picker.** Big avatars in a row with the same glow and spring as the game row; the name fades in under the focused one and "Add user" is the last tile. D-pad/stick or touch: tap to focus, tap again to pick.
- **Back into your game after sleep.**
  - Sleeping mid-game: waking goes straight back into the game.
  - Sleeping on the home screen with a game suspended: you get the Switch's own lock screen, with time and date top-left, status top-right, and "Press the same button three times to resume" in the middle, made more dynamic:
    - The button you press pops up in a badge, and each press fills a dot with a springy pop, a click and a rumble tick.
    - A different button shakes the dots and starts over.
    - On the third press, a ring bursts out and the game's frozen frame fades up into the game.
    - Three taps on the screen work too. HOME goes to the home screen instead.
    - Behind it, the paused game's art and frame are dimmed, with "<game> · paused" at the bottom.
  - This works even if uLaunch's own lock screen setting is off.
- **Console-style dialogs.** Every popup (Performance, power, game options, errors...) uses one look instead of the stock Switch dialog:
  - a dark card that springs up over a dimmed screen
  - options as a vertical list, with the blue highlight gliding between rows and popping on each move (click + rumble tick)
  - a short detail on the right of an option where it helps (e.g. what each overclock preset does)
  - A Select / B Back hints at the bottom; long lists scroll
  - tap an option to pick it, tap outside the card to go back
- **Burger menu (top right).** Tap it, or press ZL/ZR, and the old nav bar (Power, Controllers, Album, Web, User, Themes, Settings, Mii, amiibo) slides down as a list.

Theme: **Deck Night** (`ulaunch/themes/DeckNight.ultheme`).

## Requirements

- Atmosphere 1.12.1 (or anything recent that supports your firmware).
- Firmware 21.2.0. Anything up to 22.5.0 works; **23.x is not supported by uLaunch yet.**
- **steamui replaces Nintendo's HOME menu while it's active.** NXThemes theme Nintendo's HOME menu, so they don't show inside steamui. They can live side by side, and you can switch between the two whenever you like (see *Themes* below).

## Install

1. Back up your SD card.
2. Copy the contents of `SdOut/` to the SD card root (merge).
3. Boot CFW, then pick your user. Go to **burger menu → Themes** and choose **Deck Night**.
4. Add hover art and sounds (optional, see below).

**To uninstall:** delete `atmosphere/contents/0100000000001000/exefs.nsp` (only that file, so an installed NXTheme stays), or use uManager → Deactivate.

## Themes

There are two kinds, and you can switch between them whenever you want:

| | Where | How to change |
|---|---|---|
| **steamui themes** (Deck Night and other `.ultheme` files) | `ulaunch/themes/` | Burger menu → **Themes** → pick one. The menu restarts with it. |
| **NXThemes**, for Nintendo's own HOME menu | `themes/` (install with **NXThemes Installer**) | Install in NXThemes Installer, then switch to Nintendo's HOME menu (below). |

**Getting NXThemes:**
- Browse and download them from [Themezer](https://themezer.net) on your computer, or with sphaira's Themezer browser on the console.
- Put the `.nxtheme` files in `themes/<theme name>/` and install them with NXThemes Installer (it's added to your home screen automatically).
- The theme patches NXThemes needs for your firmware are already in `themes/systemPatches` (via `get_homebrew.py`), because an offline emuMMC can't download them. Themes work on firmware up to 22.x; NXThemes doesn't support 23.0 yet.

**Switching between steamui and Nintendo's HOME menu:**
- **To Nintendo's HOME menu (your NXTheme):** burger menu → Themes → **Nintendo HOME menu · NXThemes** → *Switch and restart*. Only steamui's `exefs.nsp` is removed; its backup stays in `ulaunch/bin/uSystem/`.
- **Back to steamui:** in Nintendo's HOME menu, open the Album (the homebrew menu: sphaira, or hbmenu if sphaira isn't set as it), run **uManager**, choose **Activate**, and reboot.
- Neither direction touches your NXTheme, games, saves or settings. (Stock uManager's Deactivate deleted the whole folder, NXTheme included; this build only removes that one file.)

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
2. Turns your title-screen video captures (sysMMC and emuMMC albums) into soundtracks, **automatically matched to the right game**. The home menu writes down every launch (`ulaunch/steamui/launches.txt`), so a capture belongs to the game launched just before it. The first capture of a game that has no soundtrack yet becomes its soundtrack. It only asks about captures it can't match, such as ones taken before the menu was installed. Captures are never processed twice.
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

1. Launch a new game from the steamui home screen, wait on its title screen, and hold **Capture** to save the last 30 seconds as a video.
2. Next time the SD card is in your computer, run `steamui_media.py sync`. The capture is matched to the game and converted automatically: the loudness is evened out and the end fades.

To do it by hand instead, copy captures into a folder, name them after their games (e.g. `Mario Kart 8 Deluxe.mp4`) and run `steamui_media.py sounds`.

Some games block video capture. For those, use an audio file of the game's soundtrack from an album you own.

## Overclocking (sys-clk)

sys-clk sets the CPU, GPU and RAM clocks for each game, by title ID, in `config/sys-clk/config.ini`. There are three ways to set them:

- **In the home menu (easiest):** highlight a game, open the **burger menu**, and choose **Performance**.
  - It shows the game's current preset, plus live clocks, temperatures and dock state.
  - **Set preset** offers Stock, Battery, Balanced and Performance; your choice is saved in sys-clk for that game.
  - **Pause/Resume overclocking** turns sys-clk off or on for everything.
  - For homebrew and ports, the preset goes on the take-over game they run as.
- **During a game:** open the Ultrahand overlay (L + D-pad Down + right stick), go to **sys-clk**, and change clocks live.
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

## Overlay look (Ultrahand)

`config/ultrahand/theme.ini` and `config/ultrahand/wallpaper.rgba` restyle the Ultrahand overlay to match Deck Night: a dark blue-grey background, Steam-blue highlights, toggles and sliders, and a wallpaper with a faint grid. Both are also in `config/ultrahand/themes/` and `wallpapers/`, so you can switch back in Ultrahand's settings.

**Limitation:** the theme applies to Ultrahand and to overlays built on its library. sys-clk's and Status Monitor's own overlays use the older Tesla library, which has no theme support, so they keep their stock look. The Performance option in the home menu covers the overclock settings in Deck Night style.

## Theme options (UI.json)

These keys are new in this fork:

- `main_menu.entry_menu.fixed_row_count`: locks the number of rows (Deck Night uses 1). +/- resizing is disabled while it's set.
- `main_menu.burger_icon`: position of the burger button. Its image is `ui/Main/BurgerIcon.png`. Themes without that image show no button.
- `main_menu.haptics.enabled`: HD Rumble ticks on/off.
- `startup_menu.user_carousel`: `{"enabled": true, "y": 540, "avatar_size": 256, "spacing": 340}` turns the profile list into the avatar row. Without it, uLaunch's original list is used.
- `lockscreen_menu.unlock_prompt`: `{"enabled": true, "y": 470, "presses": 3}` gives the dynamic press-to-unlock prompt, shown after sleep whenever a game is suspended.
- `dialog.game_dialog`: `{"enabled": true}` turns on the console-style dialogs. Colors come from `dialog_color` (card), `dialog_over_color` (highlight) and `dialog_opt_color` (text). Without it, the stock dialog is used.
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
| sphaira | Homebrew menu (replaces hbmenu): app list, file browser, Themezer theme browser, game installer |
| NXThemes Installer | Installs NXThemes on Nintendo's HOME menu (theme patches for your firmware included) |
| MissionControl (optional) | Use PS4/PS5/Xbox controllers over Bluetooth |

All of them show up on the home screen automatically.

## Tests

`steamui/tests/run_tests.sh` runs on any Linux or Mac with g++/clang and Python + Pillow. GitHub runs it on every push. It covers:
- The hover art/sound logic, with memory and thread sanitizers.
- Homebrew auto-adding.
- `validate_theme.py`, which checks the theme against the menu code and checks the screen layout.
- Switching between steamui and Nintendo's HOME menu on a fake SD card (an NXTheme must survive every round trip).
- `get_homebrew.py` against a fake GitHub: where sphaira, NXThemes Installer and the theme patches end up.

## Building

GitHub Actions builds everything on each push, using `.github/workflows/main.yml`. Download the `SdOut` artifact from the run.

To build locally, follow upstream uLaunch's instructions (devkitPro + `make`).
