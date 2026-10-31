<p align="center">
  <img width="420" alt="Capymilk" src="https://github.com/user-attachments/assets/3de32db1-afdc-46a8-95b3-dab84185999a" />
</p>
<h1 align="center">Capymilk</h1>
<p align="center">
  Real-time spiral galaxy renderer in C and OpenGL, with no engine or libraries<br>
</p>

## Build
```
# from a shell with MSVC attached
build.bat release
build\main.exe   # run from root
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
| Space | reset |

## Layers

| Layer | Prefix | Purpose |
|-------|--------|---------|
| `base` | | types, macros, math, arena allocator |
| `platform` | `plat_` | files, time, memory reservation |
| `window_manager` | `wm_` | window creation, input, message loop |
| `render` | `r_` | GPU resources and drawing, per backend |
| `galaxy` | `glx_` | particle simulation and renderer |
| `capymilk` | `cm_` | app state and frame loop |

## Credit

[Beltoforion's](https://beltoforion.de/en/spiral_galaxy_renderer/) article was the basis for the simulation model
