# Podman-compatible multi-stage build (also works with Docker).
FROM docker.io/emscripten/emsdk:3.1.74 AS compile
# The SDK image's distribution CMake is older than this project's minimum.
RUN python3 -m pip install --no-cache-dir cmake==3.31.6
WORKDIR /src
COPY CMakeLists.txt ./
COPY lib ./lib
COPY src ./src
COPY web ./web
RUN emcmake cmake -S . -B /build -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=OFF -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=/site \
    && cmake --build /build --parallel 2

# The Playwright package and image versions must match.
FROM mcr.microsoft.com/playwright:v1.56.1-noble AS previews
WORKDIR /tools
COPY tools/package.json tools/package-lock.json ./
RUN npm ci --ignore-scripts
COPY tools/thumbnails.mjs ./
COPY --from=compile /site /site
RUN node thumbnails.mjs /site

# Export a standalone static site, e.g. for GitHub Pages.
FROM scratch AS site
COPY --from=previews /site /

# Default target: a local web server with the same site.
FROM docker.io/library/nginx:1.28-alpine AS serve
COPY --from=previews /site /usr/share/nginx/html
EXPOSE 80
