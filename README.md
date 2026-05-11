# Install to Separator

Install to Separator adds a separator picker to Mod Organizer 2 install dialogs.
When a new mod is installed, the plugin can place it at the bottom of the chosen
separator group.

End-user documentation and downloads are hosted on
[Nexus Mods](https://www.nexusmods.com/games/skyrimspecialedition/mods/179707).
This repository is mainly for source, issue tracking, and reproducible release
builds.

## Building

This project is built inside the Mod Organizer 2
[`mob`](https://github.com/ModOrganizer2/mob) workspace. Configure both
supported MO2 targets:

```powershell
cmake --preset ninja-multi-mo2-2.5.2
cmake --preset ninja-multi-mo2-2.5.3beta11
```

Build release ZIPs:

```powershell
cmake --build --preset package-ninja-multi-mo2-2.5.2-relwithdebinfo
cmake --build --preset package-ninja-multi-mo2-2.5.3beta11-relwithdebinfo
```

Expected outputs:

- `dist/Install.To.Separator.MO2-2.5.2.zip`
- `dist/Install.To.Separator.MO2-2.5.3beta11.zip`

Use the ZIP that matches the target MO2 version. The two builds use different
plugin ABI snapshots.
