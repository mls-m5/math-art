# Mathematical art — development

Build commands below are run from the repository root. The live gallery is at
[mls-m5.github.io/math-art](https://mls-m5.github.io/math-art/).

Animated C++17 SDL2 / OpenGL flowers built from Voronoi cells. Eighty random
points repel nearby neighbors until they spread out. Each flower gets a fixed,
random repulsion strength between 0.8 and 1.5. Log-uniform samples from 0.4 to
2.5 are compressed toward 1.0, reducing the original strength variation to one
third. Stronger flowers
push neighbors harder and over a longer range, tending to claim larger cells;
weaker ones fit into smaller spaces. **R** randomizes both positions and strengths.
Each cell is split into
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
./build/space_travel
```

Run in a graphical desktop session with OpenGL 2.1 support. CMake FetchContent
fetches header-only GLM 1.0.1 for the space scene (pinned archive with SHA-256),
so the first configure needs network access. No manual GLM install is needed.

## Web build with Podman

The Podman build compiles all four apps with **Emscripten 3.1.74**, runs them
in headless Chromium, and generates real PNG thumbnails for the gallery.
Only Podman is needed on the host (the `Dockerfile` is Podman-compatible):

```sh
podman build --layers -t localhost/living-forms .
podman run --rm -p 8080:80 localhost/living-forms
```

Open **http://localhost:8080**. The index is a responsive gallery; each artwork
uses the same `web/shell.html`: edge-to-edge canvas, gallery link, pause,
randomize, app-specific toggle, and fullscreen controls. The original keyboard
shortcuts still work; browser Escape exits fullscreen instead of quitting.
Controls fade out after 2.5 seconds of inactivity and return on pointer movement,
touch, or keyboard input. Focused controls remain visible for keyboard navigation.
The first build downloads the SDK and browser images and can take a few minutes.

### Export a static site

```sh
podman build --layers --target site --output type=local,dest=dist .
python3 -m http.server 8080 --directory dist
```

`dist/` contains `index.html`, CSS, the four `.html`/`.js`/`.wasm` applications,
and `thumbnails/*.png`. Deploy this directory unchanged to a static host.
Relative URLs support GitHub Pages repository subpaths. No server-side code,
threads, cross-origin isolation headers, CDN assets, or runtime npm packages
are required. Serve over HTTP(S), not `file://`. The export command requires
local Podman on Linux; for a remote Podman machine, build the normal server
image, then use `podman create` and `podman cp` to copy
`/usr/share/nginx/html/.` into `dist/` without starting the container.

### GitHub Pages (when ready)

`.github/workflows/pages.yml` uses Podman to build the same `site` target, generates
thumbnails, uploads the site artifact, and deploys it automatically on pushes
to `main`, including merged pull requests. Manual runs are also supported.

1. Push the repository to GitHub.
2. Under **Settings → Pages → Build and deployment**, choose **GitHub Actions**.
3. Under **Actions → Build and publish art gallery**, choose **Run workflow**.

The deployment job's output links to the published site. Subsequent pushes or
pull request merges into `main` automatically rebuild and publish the gallery.

### Without a container / regenerating thumbnails

With Emscripten 3.1.74 activated, CMake 3.23+, and Node 18+:

```sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web --parallel 2
cd tools
npm ci
npx playwright install --with-deps chromium
cd ..
node tools/thumbnails.mjs build-web/site
python3 -m http.server 8080 --directory build-web/site
```

The thumbnail tool starts its own temporary HTTP server, opens each compiled
app with `?capture=1&seed=42`, advances exactly 180 fixed simulation steps,
pauses, and captures the canvas without UI. It also checks JavaScript errors,
nonblank rendering, gallery links, and canvas resizing. Output images are
720×480, reproducible for a given browser/toolchain. They are generated build
artifacts, not placeholders or hand-maintained screenshots. Playwright's npm
and container image versions are pinned together. Geometry tests remain native.

## Programs and controls

**F** toggles fullscreen in every app, on desktop and web. On desktop, **Esc**
first leaves fullscreen; from windowed mode it quits. In the browser, Escape
belongs to fullscreen rather than application shutdown.

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
cells. Each cell has nested rectangular frames: a black border, a white inner
frame, and a black center. Neighboring cells invert those colors in a checkerboard
pattern. Local shader coordinates still run from 0 to 1 within every cell.
Three vertical and three horizontal sine curves with overtones divide the
screen into additional inversion regions. A pixel flips color if the total
number of vertical curves to its left and horizontal curves above it is odd.
The curves have randomized amplitudes, frequencies, and phases, and move slowly.
Their boundaries are antialiased; the curves themselves are not drawn as lines.
Press **W** to toggle wave inversion (on by default), **Space** to pause all motion,
**R** to randomize the grid and curves, and **Esc** to quit.
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

`space_travel` flies a real 3D camera through 700 stationary world-space stars.
GLM `lookAt` and perspective matrices project current and previous-exposure
positions; apparent trails are not a 2D radial simulation. Only the final capsule
expansion is screen-space, so the ends remain round. An antialiased capsule SDF
shader and additive blending draw all trails in one batch.

- **Space:** pause/resume flight.
- **B / Boost button:** smoothly ramp between 56 and 150 world units/second
  (frame-rate-independent easing with a 0.65-second time constant).
- **F:** toggle fullscreen.
- **R:** regenerate the starfield and reset the camera.
- **Esc:** quit on desktop (exit fullscreen in the browser).

The camera follows a fixed normalized 3D direction. Stars fade in from 440 to
280 units ahead and recycle after passing 12 units behind the camera. Recycled
stars spawn invisibly 440–455 units ahead. Spawning excludes a cylinder of radius
4 world units around the flight axis (camera-local XY). Because the camera's
heading is fixed, stars stay outside this corridor for their entire lifetimes,
preventing head-on collisions. Perspective, near-plane/screen clipping,
and a small near-camera fade prevent flashes as stars pass. `src/space.h` exposes
the CPU-only camera, starfield, projection, and capsule-mesh code. Tests cover
true camera translation, perspective expansion, distance fade, recycling,
safety clearance, frame-independent boost easing, and finite capsule geometry,
including portrait viewports.

## Reusable graphics library

The `graphics` CMake target lives in `lib/graphics/`. It handles SDL/OpenGL
initialization and cleanup, event polling, frame timing, high-DPI viewports,
aspect-preserving projection, clearing, and presentation. Drawing helpers provide
lines, circle outlines, filled disks, polygon outlines, and convex triangle fans.
All four programs use this library without direct OpenGL calls. The triangle
mesh helper also supplies per-vertex local coordinates, a scalar variant, and
two extra parameters to shaders. `drawableSize()` exposes real pixel dimensions;
`additiveBlending()` supports luminous effects without app-level OpenGL calls.
Windows request 4× MSAA for geometry edges, automatically retrying without
multisampling if unavailable.

For another program, link `graphics` (and `SDL2::SDL2main` where available),
include `graphics/graphics.h`, and create a `graphics::Window`. Poll its events,
call `beginFrame` with minimum world-space half-extents, draw, then `present`.
`view()` exposes the actual visible half-extents for edge-aligned artwork.
Use one window at a time. Initialization failures throw `std::runtime_error`.
`graphics::run` drives native loops or browser requestAnimationFrame callbacks.
Capture application state by value (the examples use `shared_ptr<App>`), never
references to stack variables that disappear when browser `main()` returns.

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
x/y coordinates; `uTime` is available for future shader animation. `vVariant`
is checkerboard parity, used to invert colors on neighboring cells. Distance to
the nearest UV edge defines the frames: 0–0.12 is the outer border, 0.12–0.28 is
the inner frame, and the rest is the center. Frame edges use `fwidth` and
`smoothstep` for approximately one-pixel antialiasing as cells deform. WebGL 1
uses `OES_standard_derivatives` when available, with a fixed-width fallback.
One shader
program is reused across all cells, with separate 0-to-1 coordinates per cell.
`vScreen` supplies whole-screen coordinates for the six inversion curves.
`uWaveAmplitude`, `uWaveFrequency`, and `uWavePhase` arrays configure their
fundamentals and two overtones. Curves are centered at 25%, 50%, and 75% on each
axis; bounded amplitudes keep them ordered and inside the screen. Crossing
parity is combined with checkerboard parity, so two inversions cancel.

Each convex quadrilateral is drawn as two triangles; coordinates are linearly
interpolated within each triangle. `src/grid.h` stores the CPU-only animated
lattice and generates the shared-position mesh. Geometry tests check edge
constraints, convexity, screen coverage, UV mapping, and checkerboard parity over
many time samples.

### Desktop and WebGL backend

The shared renderer uses vertex buffers and explicit shader attributes on both
desktop OpenGL and WebGL 1; there is no immediate-mode emulation. All shaders
use GLSL ES 1.00-compatible syntax. Browser canvases track viewport size and
pixel density (capped at 2×). The shared frame loop retains application state
across browser callbacks and preserves the native applications' controls and
simulation behavior.

## Geometry for later use

`src/voronoi.h` exposes `Cell`: a generating `center` and counterclockwise
`polygon` vertices. `buildVoronoi` returns cells in site order, clipping each
polygon to the rectangular `Bounds` using perpendicular-bisector half-planes.
Inputs must be distinct sites within the domain. Vertices are stored per cell,
not as a shared edge/vertex graph. `relaxSites` moves sites with short-range
repulsion and wall forces; the application rebuilds polygons after movement.
Its optional fourth argument is a positive, finite strength vector in site order;
omitting it preserves the original uniform-force behavior. Strength scales emitted
force and its square root scales interaction range and wall clearance. The app
stores strengths alongside sites, unchanged during simulation. Cell boundaries
are still ordinary Voronoi bisectors, not weighted/power-diagram boundaries, so
size is an emergent tendency rather than an exact requested area.
`src/flower.h` exposes `buildPetal(center, a, b)`, returning a sampled contour
for each fan triangle. Quadratic Bezier curves cut inward at the outer corners;
narrowed fan wedges separate petals, and a 6% radial inset separates neighboring
flowers. The original cell geometry is
unchanged. The contour starts at the center and can be drawn as a triangle fan.
The geometry library is independent of SDL/OpenGL and has CTest coverage,
including petal containment and reduced-area checks.
