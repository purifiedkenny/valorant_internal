# MAXIWINDOWSXP

External ESP/visuals overlay for VALORANT, built against the 13.10 offset set.
The overlay runs in its own transparent window (OwnOverlay). This build is
configured as read-only ESP: the aimbot, triggerbot, and mouse-injection paths
are disabled.

`DiscordHookDll` is included as reference code. DLL injection into Discord is
disabled in this build.

## Requirements

- Windows 10/11 x64
- Visual Studio 2026 (v145 toolset) with the MASM compiler
- Windows SDK 10.0

## Build

```
build.bat          # clean + Debug x64 build
build_main.bat     # Debug x64 build + randomly named EXE
build_test.bat     # Debug x64 build named MAXITEST
```

Manual build:

```
msbuild MAXIWINDOWSXP.sln /t:Build /p:Configuration=Debug /p:Platform=x64
```

Outputs: `x64\Debug\MAXIWINDOWSXP.exe` and `x64\Debug\DiscordHookDll.dll`.

## Running

- `MAXIWINDOWSXP.exe` opens the configured driver, attaches to the game, and
  starts the overlay. A kernel driver is required; this package does not
  include one.
- `MAXIWINDOWSXP.exe --gui-only` skips driver setup. Without a game target it
  opens a standalone preview window after the usual 120-second wait.
- INSERT toggles the menu, ESC closes it.
- The Discord HTML overlay server starts on `127.0.0.1:47777` and patches
  Discord's `host.js` to load the local overlay page. Rendering remains on
  OwnOverlay; the HTML-only path is disabled in `src\render\render.cpp`.
- Menu sections: Combat, Visuals, Lineups, Misc Features, Configuration.
  Combat controls are present, but the aimbot/triggerbot paths are disabled.

## Project structure

| Path | Content |
|---|---|
| `main.cpp` | Entry point |
| `src\core` | Application lifecycle and main loop |
| `src\game` | Memory map, engine, and model layer |
| `src\render` | ImGui interface and drawing |
| `src\driver` | Driver communication layer (IOCTL + loader) |
| `src\discord_hook` | DLL injector (runtime injection disabled) |
| `src\discord_overlay` | HTML overlay server for Discord |
| `include\` | Header files |
| `3rd\` | ImGui, FreeType, nlohmann |
| `offsets\` | Offset documentation (`offsets\README.md`) |
| `assets\` | `maxi_overlay.html`, `navigation.json` |
| `lineups.json` | Lineup dataset (map coordinates; not wired into the project) |
| `resources\` | Embedded font/icon resources |
| `DiscordHookDll\` | Swapchain hook DLL project (reference; not injected) |

## Driver backend

This package does not include a compiled driver. Driver communication lives
under `src\driver\`:

- `driver_impl.cpp` - driver open/close and memory read/write interface
- `driver_loader.cpp` - driver loading flow
- `include\driver\backend.h` - backend communication declarations

The backend is selected at build time and lives in `include\driver\backend.h`.
To use a different driver, update the IOCTLs and request layout in that header
and the calls in `driver_impl.cpp`.

No compiled driver or driver image resource is included.

## Offsets

Offset set and table reference: `offsets\README.md`.
Code definitions: `src\game\memory_map.cpp`, `include\game\memory_map.hpp`.

## Lineups (`lineups.json`)

`lineups.json` is the lineup dataset (map, agent, and throw-position
coordinates). It is not wired into the project; loading, filtering, and
menu/ESP display still need to be implemented.

- `Lineups` array; each record has `Agent`, `Map`, `Weapon`,
  `PositionX/Y/Z`, `RotationX/Y/Z`, `Distance`, and `Tips`.
- Reference loader: `src\game\navigation.cpp` reads `assets\navigation.json`
  in the same format. `lineups.json` is not referenced directly.

## Optional support

Paid work is available for:

- Completing the Discord overlay (the current implementation is roughly 80%
  done).
- Integrating the previous Discord overlay version.

Ask for scope and pricing.
