#include "voronoi.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

int main(int, char **)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window *window = SDL_CreateWindow(
        "Blumme - Voronoi cells | R: randomize | Space: pause | Esc: quit",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 640,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        std::cerr << "OpenGL context creation failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    const bool vsync = SDL_GL_SetSwapInterval(1) == 0;

    const blumme::Bounds bounds;
    std::mt19937 random(std::random_device{}());
    std::vector<blumme::Point> sites;
    const auto randomize = [&] {
        std::uniform_real_distribution<double> x(-bounds.halfWidth * 0.95, bounds.halfWidth * 0.95);
        std::uniform_real_distribution<double> y(-bounds.halfHeight * 0.95, bounds.halfHeight * 0.95);
        sites.clear();
        while (sites.size() < 80) {
            const blumme::Point candidate{x(random), y(random)};
            // Avoid duplicate or nearly coincident generating points.
            const bool tooClose = std::any_of(sites.begin(), sites.end(), [&](blumme::Point p) {
                return std::hypot(p.x - candidate.x, p.y - candidate.y) < 0.001;
            });
            if (!tooClose)
                sites.push_back(candidate);
        }
    };
    randomize();
    auto cells = blumme::buildVoronoi(sites, bounds);
    bool running = true;
    bool paused = false;
    double accumulator = 0.0;
    constexpr double step = 1.0 / 120.0;
    Uint64 previous = SDL_GetPerformanceCounter();
    const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());

    while (running) {
        bool dirty = false;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_SPACE:
                    paused = !paused;
                    accumulator = 0.0;
                    break;
                case SDLK_r:
                    randomize();
                    dirty = true;
                    break;
                default:
                    break;
                }
            }
        }
        if (!running)
            break;

        const Uint64 now = SDL_GetPerformanceCounter();
        const double elapsed = static_cast<double>(now - previous) / frequency;
        previous = now;
        if (!paused) {
            accumulator += std::min(elapsed, 0.1);
            while (accumulator >= step) {
                blumme::relaxSites(sites, bounds, step);
                accumulator -= step;
                dirty = true;
            }
        }
        if (dirty)
            cells = blumme::buildVoronoi(sites, bounds);

        int width, height;
        SDL_GL_GetDrawableSize(window, &width, &height);
        if (width <= 0 || height <= 0) {
            SDL_Delay(10);
            continue;
        }
        // Keep the domain fixed and circles round even when resizing.
        const double aspect = static_cast<double>(width) / height;
        const double viewHeight = std::max(bounds.halfHeight + 0.08,
                                           (bounds.halfWidth + 0.08) / aspect);
        const double viewWidth = viewHeight * aspect;
        glViewport(0, 0, width, height);
        glClearColor(0.035f, 0.045f, 0.075f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-viewWidth, viewWidth, -viewHeight, viewHeight, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glColor3f(0.45f, 0.75f, 0.85f);
        for (const auto &cell : cells) {
            glBegin(GL_LINE_LOOP);
            for (const auto &vertex : cell.polygon)
                glVertex2d(vertex.x, vertex.y);
            glEnd();
        }

        glColor3f(1.0f, 0.78f, 0.30f);
        constexpr double pi = 3.14159265358979323846;
        constexpr double radius = 0.018;
        for (const auto &cell : cells) {
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 24; ++i) {
                const double angle = 2.0 * pi * i / 24;
                glVertex2d(cell.center.x + radius * std::cos(angle),
                           cell.center.y + radius * std::sin(angle));
            }
            glEnd();
        }

        SDL_GL_SwapWindow(window);
        if (!vsync)
            SDL_Delay(16);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
