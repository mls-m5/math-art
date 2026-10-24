# blumme

A tiny C++17 SDL2 / OpenGL demo: a rotating, color-interpolated triangle,
outlined edges, a background grid, and interactive line drawing.
Uses OpenGL 2.1 immediate mode for simplicity (no shader loader needed).

## Build and run

Requires CMake 3.23+, a C++17 compiler, SDL2 development files, and desktop
OpenGL development files/drivers. On Debian/Ubuntu:

```sh
sudo apt install build-essential cmake libsdl2-dev libgl1-mesa-dev
cmake -S . -B build
cmake --build build
./build/blumme
```

Run in a graphical desktop session with OpenGL support.

## Controls

- **Left mouse drag:** draw a yellow line (preview while dragging).
- **C:** clear drawn lines.
- **Space:** pause/resume triangle rotation.
- **Esc:** quit.

The window is resizable and supports high-DPI displays.
