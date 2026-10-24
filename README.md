# blumme and rays

Animated C++17 SDL2 / OpenGL flowers built from Voronoi cells. Eighty random
points repel nearby neighbors until they spread out. Each cell is split into
fan triangles, then filled as petals with the two outer corners rounded inward.
Black background shows through the cutouts, between petals, and between flowers.
Gold disks with black borders mark the generating sites (not necessarily polygon
centroids).

## Build and run

Requires CMake 3.23+, a C++17 compiler, SDL2 development files, and desktop
OpenGL development files/drivers. On Debian/Ubuntu:

```sh
sudo apt install build-essential cmake libsdl2-dev libgl1-mesa-dev
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/blumme
./build/rays
```

Run in a graphical desktop session with OpenGL 2.1 support. No additional
geometry libraries or shader loader are needed.

## Programs and controls

`blumme` retains the animated Voronoi flowers:

- **R:** generate a new random arrangement.
- **Space:** pause/resume repulsion.
- **V:** toggle the original Voronoi polygon outlines (off by default).
- **Esc:** quit.

`rays` displays a central circle with 64 straight rays at jittered random
angles. The wedges between rays are shader-filled with alternating black and
white radial bands moving inward. Rays reach the window edges, even after
resizing. Press **R** for a new arrangement, **Space** to pause/resume the bands,
and **Esc** to quit.

Resizing preserves the simulation domain and the circles' aspect ratio.

## Reusable graphics library

The `graphics` CMake target lives in `lib/graphics/`. It handles SDL/OpenGL
initialization and cleanup, event polling, frame timing, high-DPI viewports,
aspect-preserving projection, clearing, and presentation. Drawing helpers provide
lines, circle outlines, filled disks, polygon outlines, and convex triangle fans.
Both programs use this library; neither contains direct OpenGL calls.

For another program, link `graphics` (and `SDL2::SDL2main` where available),
include `graphics/graphics.h`, and create a `graphics::Window`. Poll its events,
call `beginFrame` with minimum world-space half-extents, draw, then `present`.
`view()` exposes the actual visible half-extents for edge-aligned artwork.
Use one window at a time. Initialization failures throw `std::runtime_error`.

`graphics/point.h` is a lightweight shared point type; the `graphics_types`
interface target lets CPU geometry use it without linking SDL or OpenGL.

### Ray field shader

Edit `src/shaders/rays.frag` and rebuild. CMake embeds the GLSL sources, so the
executable does not depend on runtime shader file paths. `graphics::Shader`
provides compilation/link diagnostics and float/vec2 uniforms.

The fragment shader's `fieldCoordinates()` returns:
- **x:** normalized angular position within the current wedge (0 to 1).
- **y:** actual distance from the origin in world units.

`uTime` drives inward motion; `uAngleStart`/`uAngleSpan` define each wedge.
The first pattern uses only y and time; x is ready for angular effects later.
The bands are 0.10 units wide and move inward at 0.18 units/second. Time wraps
at a full pattern period to maintain precision. Rectangle corners are included
in wedge contours, avoiding holes at window corners.

### Future Emscripten / WebGL support

The new shaders use explicit position attributes and GLSL ES 1.00-compatible
syntax (including fragment precision), rather than fixed-function built-ins.
This is preparation, **not a working web build yet**. The graphics backend still
needs vertex buffers instead of immediate-mode OpenGL, a GLES/WebGL context,
and Emscripten CMake/SDL setup. Both applications also need a browser-compatible
frame callback instead of their blocking loops. These changes can stay behind
the shared graphics API without changing the flower geometry.

## Geometry for later use

`src/voronoi.h` exposes `Cell`: a generating `center` and counterclockwise
`polygon` vertices. `buildVoronoi` returns cells in site order, clipping each
polygon to the rectangular `Bounds` using perpendicular-bisector half-planes.
Inputs must be distinct sites within the domain. Vertices are stored per cell,
not as a shared edge/vertex graph. `relaxSites` moves sites with short-range
repulsion and wall forces; the application rebuilds polygons after movement.
`src/flower.h` exposes `buildPetal(center, a, b)`, returning a sampled contour
for each fan triangle. Quadratic Bezier curves cut inward at the outer corners;
narrowed fan wedges separate petals, and a 6% radial inset separates neighboring
flowers. The original cell geometry is
unchanged. The contour starts at the center and can be drawn as a triangle fan.
The geometry library is independent of SDL/OpenGL and has CTest coverage,
including petal containment and reduced-area checks.
