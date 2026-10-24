# blumme

An animated C++17 SDL2 / OpenGL Voronoi diagram. Eighty random generating
points repel nearby neighbors until they spread out. Cyan polygon outlines
mark the area closest to each point; gold circles mark the generating sites
(not necessarily the geometric centroids of their polygons).

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
- **Esc:** quit.

Resizing preserves the simulation domain and the circles' aspect ratio.

## Geometry for later use

`src/voronoi.h` exposes `Cell`: a generating `center` and counterclockwise
`polygon` vertices. `buildVoronoi` returns cells in site order, clipping each
polygon to the rectangular `Bounds` using perpendicular-bisector half-planes.
Inputs must be distinct sites within the domain. Vertices are stored per cell,
not as a shared edge/vertex graph. `relaxSites` moves sites with short-range
repulsion and wall forces; the application rebuilds polygons after movement.
The geometry library is independent of SDL/OpenGL and has CTest coverage.
