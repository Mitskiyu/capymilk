
# Capymilk

Real-time spiral galaxy renderer built from scratch in C

https://github.com/user-attachments/assets/c8d23190-90cb-46a2-afc5-27a9631d5a25

## Build
```
# from a shell with msvc attached
build.bat

build\main.exe # make sure to run from root dir
```

## Controls

| Key | Action |
|-----|--------|
| Q/A | angular offset |
| W/S | inner eccentricity |
| E/D | outer eccentricity |
| R/F | time scale |
| U/J | core radius |
| I/K | galaxy radius |
| T/G | star count |
| Y/H | dust count |

## Layers
- `base`
    - types, macros, math, arena allocator
- `platform` (`plat_`)
    - files, time, memory reservation; os-dependent
- `window_manager` (`wm_`)
    - window creation, input, message loop; os-dependent
- `render` (`r_`)
    - gpu resources and drawing; implemented per-backend
- `galaxy` (`glx_`)
    - particle simulation + renderer
- `ui`
    - quad drawing (stub)
- `capymilk` (`cm_`)
    - app state and frame loop

## Credit
[beltoforion's article](https://beltoforion.de/en/spiral_galaxy_renderer/) was used for implementing the simulation model

