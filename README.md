<div align="center">

<img src="docs/images/preview.gif" width="88">

<h1>TinyCat</h1>

<p>
  A tiny pixel-art cat that lives on your desktop.
</p>

<br>

<p>
  <img src="https://img.shields.io/badge/Windows-10%20%7C%2011-informational?style=flat-square">
  <img src="https://img.shields.io/badge/C%2B%2B-Win32-informational?style=flat-square">
  <img src="https://img.shields.io/badge/Pixel%20Art-Desktop%20Pet-informational?style=flat-square"> <br>
  Walks on desktop windows · Grab and throw · Cursor interaction
</p>

<img src="docs/images/gameplay.gif" width="600">

</div>

## System tray

Right-click the Tiny Cat icon in the system tray to open its menu.

- **Add Cat** creates another independent pet.
- **Exit** closes all cats and quits the application.

## Project layout

- `src/`: C++ source files and headers.
- `resources/`: Windows resource script, resource IDs, and icons.
- `assets/sprites/`: runtime sprite sheets.
- `assets/cursors/`: runtime cursor files.
- `docs/images/`: README previews and gameplay recordings.
- `build/bin/<platform>/<configuration>/`: executables and runtime assets.
- `build/obj/<platform>/<configuration>/`: compiler and linker intermediate files.
- `build/previous/`: preserved output folders from before the layout change (local only).

## Build

Open `TinyCat.vcxproj` in Visual Studio with the v145 C++ toolset and Windows SDK installed, or run this from a Developer PowerShell in the project root:

```powershell
msbuild .\TinyCat.vcxproj /p:Configuration=Release /p:Platform=x64
```

Launch `build/bin/x64/Release/TinyCat.exe`. Use `Debug` or `Win32` for the other configurations. Sprite sheets and cursor files are copied next to the executable automatically; keep those files with the executable when distributing the app.