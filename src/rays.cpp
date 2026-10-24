#include "graphics/graphics.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

int main(int, char **) try
{
    graphics::Window window("Rays - R: randomize | Esc: quit");
    std::mt19937 random(std::random_device{}());
    constexpr double pi = 3.14159265358979323846;
    constexpr double radius = 0.22;
    constexpr int rayCount = 64;
    std::vector<double> angles;
    const auto randomize = [&] {
        // Jitter evenly spaced angles for irregular spacing without large clumps.
        std::uniform_real_distribution<double> jitter(-0.42, 0.42);
        std::uniform_real_distribution<double> rotation(0.0, 2.0 * pi);
        const double offset = rotation(random);
        angles.clear();
        for (int i = 0; i < rayCount; ++i)
            angles.push_back(offset + (i + jitter(random)) * 2.0 * pi / rayCount);
    };
    randomize();
    bool running = true;
    while (running) {
        SDL_Event event;
        while (window.pollEvent(event)) {
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    running = false;
                if (event.key.keysym.sym == SDLK_r)
                    randomize();
            }
        }
        if (!running)
            break;
        if (!window.beginFrame({1.4, 0.9}))
            continue;
        const auto view = window.view();
        for (double angle : angles) {
            const double dx = std::cos(angle);
            const double dy = std::sin(angle);
            // Intersect each ray with the current visible rectangle so it ends
            // exactly at a window edge, including after resizing.
            const double infinity = std::numeric_limits<double>::infinity();
            const double tx = std::abs(dx) > 1e-12 ? view.halfWidth / std::abs(dx) : infinity;
            const double ty = std::abs(dy) > 1e-12 ? view.halfHeight / std::abs(dy) : infinity;
            const double distance = std::min(tx, ty);
            graphics::line({radius * dx, radius * dy}, {distance * dx, distance * dy},
                           {0.80f, 0.85f, 0.94f});
        }
        graphics::disk({0, 0}, radius, {0.03f, 0.03f, 0.04f});
        graphics::circle({0, 0}, radius, {1.0f, 0.78f, 0.30f});
        window.present();
    }
    return 0;
}
catch (const std::exception &error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
