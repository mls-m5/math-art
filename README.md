# blumme

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
```

Run in a graphical desktop session with OpenGL 2.1 support. No additional
geometry libraries or shader loader are needed.

## Controls

- **R:** generate a new random arrangement.
- **Space:** pause/resume repulsion.
- **V:** toggle the original Voronoi polygon outlines (off by default).
- **Esc:** quit.

Resizing preserves the simulation domain and the circles' aspect ratio.

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
