#include "graphics/graphics.h"
#include "graphics/shader.h"
#include "rays_geometry.h"
#include "rays_shaders.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <random>
#include <vector>

int main(int, char **) try
{
    graphics::Window window("Rays - R: randomize | Space: pause | Esc: quit");
    graphics::Shader shader(rays::vertexSource, rays::fragmentSource);
    std::mt19937 random(std::random_device{}());
    constexpr double radius = 0.22;
    constexpr int rayCount = 64;
    static_assert(rayCount >= 2 && rayCount % 2 == 0,
                  "Alternating wedges require a positive even ray count");
    std::vector<double> angles;
    const auto randomize = [&] {
        std::uniform_real_distribution<double> jitter(-0.42, 0.42);
        std::uniform_real_distribution<double> rotation(0.0, rays::tau);
        const double offset = rotation(random);
        angles.clear();
        for (int i = 0; i < rayCount; ++i) {
            double angle = std::fmod(offset + (i + jitter(random)) * rays::tau / rayCount, rays::tau);
            if (angle < 0.0)
                angle += rays::tau;
            angles.push_back(angle);
        }
        std::sort(angles.begin(), angles.end());
    };
    randomize();
    bool running = true;
    bool paused = false;
    double time = 0.0;
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
                if (event.key.keysym.sym == SDLK_SPACE)
                    paused = !paused;
            }
        }
        if (!running)
            break;
        const double elapsed = window.elapsed();
        if (!paused)
            time += elapsed;
        if (!window.beginFrame({1.4, 0.9}))
            continue;
        const auto view = window.view();
        shader.use();
        shader.setVec2("uView", static_cast<float>(view.halfWidth), static_cast<float>(view.halfHeight));
        // The pattern repeats every 0.20 / 0.18 seconds. Wrap time to avoid
        // losing floating-point precision during long-running installations.
        shader.setFloat("uTime", static_cast<float>(std::fmod(time, 0.20 / 0.18)));
        shader.setFloat("uBendAmount", 0.055f);
        for (std::size_t i = 0; i < angles.size(); ++i) {
            const double start = angles[i];
            const double end = i + 1 < angles.size() ? angles[i + 1] : angles[0] + rays::tau;
            shader.setFloat("uAngleStart", static_cast<float>(start));
            shader.setFloat("uAngleSpan", static_cast<float>(end - start));
            shader.setFloat("uBandOffset", static_cast<float>(i % 2));
            graphics::fillFan(rays::wedgeContour(start, end, view.halfWidth, view.halfHeight), {1, 1, 1});
        }
        shader.stop();
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
