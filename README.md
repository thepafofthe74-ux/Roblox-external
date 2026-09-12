# Roblox-external

External cheat for the **normal Roblox client** (`RobloxPlayerBeta.exe`). Updated for version `version-e7d81637d42c4b23`.

Single-file project: everything lives in `runtime.cpp`.

## Features

- **ESP**: boxes, names, distances, lines to players.
- **Fly**: free flight, configurable speed.
- **No Clip**: wall and collision bypass.
- **Freecam**: camera detached from the character.
- **Overlay menu**: in-game menu opened with the INSERT key, with toggles, sliders and parameter selection.

## Requirements

- Windows 10/11 x64
- Microsoft Visual C++ Redistributable 2015-2022 (x64)
- Roblox client updated to `version-e7d81637d42c4b23`
- Administrator privileges (required for process memory access)

## Build

    git clone https://github.com/user/Roblox-external.git
    cd Roblox-external
    cl /std:c++17 /O2 /EHsc runtime.cpp /link user32.lib gdi32.lib psapi.lib

Output binary: `runtime.exe`.

## Usage

1. Launch the Roblox client and join an experience.
2. Run `runtime.exe` as administrator.
3. Wait for attachment to the `RobloxPlayerBeta.exe` process.
4. Press **INSERT** to open the overlay menu.
5. Toggle and configure ESP, Fly, No Clip, Freecam inside the menu.

## Menu

Press **INSERT** to open/close the overlay menu. Inside the menu:

- **ESP section**: toggle boxes, names, distances, lines; pick colors.
- **Fly section**: toggle fly; adjust speed slider.
- **No Clip section**: toggle no clip.
- **Freecam section**: toggle freecam; adjust speed slider.

## Default Hotkeys

| Key | Action |
|-----|--------|
| INSERT | Open/close menu |
| F2 | ESP on/off |
| F3 | Fly on/off |
| F4 | No Clip on/off |
| F5 | Freecam on/off |
| END | Unload cheat |

## Settings

All settings are stored in memory and edited through the INSERT menu. Values persist for the current session only.

## Offsets

All offsets are defined in `runtime.cpp` inside the `Offsets` namespace, tied to `version-e7d81637d42c4b23`. After any Roblox client update, recalculate:

- `DataModel`
- `Workspace`
- `LocalPlayer`
- `Camera`
- `Humanoid`

## Troubleshooting

- **Attachment failed**: run as administrator, verify `RobloxPlayerBeta.exe` is running.
- **Features not working**: offsets outdated after a client update, recalculate.
- **Crash on inject**: verify VCRedist x64 is installed.
- **Menu not showing**: press INSERT, check overlay init logs.

## Update

Target version: `version-e7d81637d42c4b23`. After a Roblox client update, recalculate offsets in `runtime.cpp`.

## Disclaimer

Provided as-is, no guarantee of functionality after a client update.

167% vibe coded
