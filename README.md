# blumme, rays, and wonky_grid

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
./build/wonky_grid
```

Run in a graphical desktop session with OpenGL 2.1 support. No additional
geometry libraries or shader loader are needed.

## Programs and controls

`blumme` retains the animated Voronoi flowers:

- **R:** generate a new random arrangement.
- **Space:** pause/resume repulsion.
- **V:** toggle the original Voronoi polygon outlines (off by default).
- **Esc:** quit.

`rays` displays a black center disk surrounded by 22 wedges at jittered random
angles. Press **C** to toggle the disk (on by default), revealing or hiding the
sine-shaped shader cutoff underneath. Their boundaries reach the window edges but have no drawn outlines.
Each wedge has black and white radial bands moving inward; neighboring wedges
are offset by one band width, making their colors opposite. A half-sine radial
bend bows alternate wedges inward and outward, with zero offset at their edges. The ray count is
enforced to be even so alternation continues across the last/first boundary. Press **R** for a new arrangement, **Space** to pause/resume the bands,
and **Esc** to quit.

`wonky_grid` fills the screen with a moving 16-by-10 grid of almost-rectangular
cells. Each cell has local shader coordinates from 0 to 1: x is red, y is blue.
Press **Space** to pause, **R** to randomize the motion, and **Esc** to quit.
The grid stretches to fill the window on resize.

Vertices share positions between cells, so there are no gaps. Their offsets
combine two sine waves per axis with random amplitudes, frequencies, and phases.
Edge vertices slide along their edges; corners stay fixed. Offsets are bounded
to 32% of grid spacing per axis (twice the original amplitudes). A slower,
shared sine field also moves whole regions by up to 8% of the screen per axis.
Its envelope, `16*x*(1-x)*y*(1-y)`, fades proportionally to distance near any
edge and is zero on the boundary. Two monotonic axis warps keep the continuous
field invertible. If needed, the combined displacements are scaled down just
enough to keep every rendered cell convex, with no folds or overlaps.

Resizing preserves the flower/ray domains and the circles' aspect ratio.

## Reusable graphics library

The `graphics` CMake target lives in `lib/graphics/`. It handles SDL/OpenGL
initialization and cleanup, event polling, frame timing, high-DPI viewports,
aspect-preserving projection, clearing, and presentation. Drawing helpers provide
lines, circle outlines, filled disks, polygon outlines, and convex triangle fans.
All three programs use this library without direct OpenGL calls. The triangle
mesh helper also supplies per-vertex local coordinates to shaders.

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
`uBandOffset` alternates between 0 and 1 in sorted angular order.
`uBendAmount` controls the radial half-sine amplitude (currently 0.055 world
units). The effective distance is `y + sign * uBendAmount * sin(pi * x)`, with
alternating signs, so each wedge has zero bend at its edges and maximum bend
at its angular midpoint. `uInnerRadius` (0.11) cuts out the center using this
same warped distance, producing a stationary wavy boundary. The bands keep
moving into that boundary. By default, a black disk covers the entire cutoff;
its radius is `(innerRadius + abs(bendAmount)) * 1.02` to include a small margin.
The bands are 0.10 units wide and move inward at 0.18 units/second. Time wraps
at a full pattern period to maintain precision. Rectangle corners are included
in wedge contours, avoiding holes at window corners.

### Grid field shader

Edit `src/shaders/grid.frag` and rebuild. `vCoordinate` contains the cell-local
x/y coordinates; `uTime` is available for future shader animation. One shader
program is reused across all cells, with separate 0-to-1 coordinates per cell.
Each convex quadrilateral is drawn as two triangles; coordinates are linearly
interpolated within each triangle. `src/grid.h` stores the CPU-only animated
lattice and generates the shared-position mesh. Geometry tests check edge
constraints, convexity, screen coverage, and UV mapping over many time samples.

### Future Emscripten / WebGL support

The new shaders use explicit position attributes and GLSL ES 1.00-compatible
syntax (including fragment precision), rather than fixed-function built-ins.
This is preparation, **not a working web build yet**. The graphics backend still
needs vertex buffers instead of immediate-mode OpenGL, a GLES/WebGL context,
and Emscripten CMake/SDL setup. All applications also need a browser-compatible
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
