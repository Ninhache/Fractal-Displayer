# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
mkdir -p build && cd build
cmake ..
make
./a
```

The shader uses a relative path `../shaders/shader.frag`, so always run the binary from within the `build/` directory.

## Architecture

Real-time GPU-rendered Mandelbrot set explorer. The CPU side manages state and UI; all fractal computation happens in a GLSL fragment shader.

**Data flow:**
1. `main.cpp` owns the SFML window, ImGui panel, and event loop
2. User input (arrow keys = pan, `+`/`-` = iterations, `A`/`Z` = zoom) updates uniforms
3. Uniforms are pushed to `shaders/shader.frag` each frame via `sf::Shader::setUniform`
4. The shader computes Mandelbrot iterations per pixel and colors using the active palette

**Key files:**
- `src/main.cpp` — entry point, event loop, ImGui UI, uniform dispatch
- `shaders/shader.frag` — all fractal math; uniforms: `iterations`, `scale`, `x_offset`, `y_offset`, `pallet[10]`, `colors_nb`, `smoth` (typo, keep it), `background_color`, `resolution`
- `src/ColorPalet.hpp/cpp` — 4 built-in palettes (original, RGB, Black and White, Fire); global singleton `GLOBAL_PALLET`; `palletToArray()` converts to C-array for shader
- `src/Fractal.hpp/cpp` — data model for fractal params (mostly unused by shader directly)
- `src/FractalHandler.hpp/cpp` — thin wrapper around `Fractal` (minimal, work in progress)

**Dependencies** (auto-fetched via CMake FetchContent):
- SFML 2.5.1 — window, graphics, events
- ImGui + ImGui-SFML 2.3 — settings panel

## Known Quirks

- `palletToArray()` allocates with `new` and never frees — known leak
- Julia set is in the `Fractal::Type` enum but not wired up in the UI
- `smoth` typo in shader uniform is intentional (matches the C++ side — don't fix one without the other)
- `FractalHandler` is incomplete scaffolding
