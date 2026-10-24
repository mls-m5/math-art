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

namespace {
struct App
{
    graphics::Window window{"Rays - C: center disk | R: randomize | Space: pause | Esc: quit"};
    graphics::Shader shader{rays::vertexSource, rays::fragmentSource};
    std::mt19937 random{graphics::randomSeed()};
    static constexpr float innerRadius = 0.11f;
    static constexpr float bendAmount = 0.055f;
    static constexpr int rayCount = 22;
    static_assert(rayCount >= 2 && rayCount % 2 == 0,
                  "Alternating wedges require a positive even ray count");
    std::vector<double> angles;
    void randomize() {
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
    }
    App() { randomize(); }
    bool running = true;
    bool paused = false;
    bool showCenterDisk = true;
    double time = 0.0;
    bool frame() {
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
                if (event.key.keysym.sym == SDLK_c)
                    showCenterDisk = !showCenterDisk;
            }
        }
        if (!running)
            return false;
        const double elapsed = window.elapsed();
        if (!paused)
            time += elapsed;
        if (!window.beginFrame({1.4, 0.9}))
            return true;
        const auto view = window.view();
        shader.use();
        shader.setVec2("uView", static_cast<float>(view.halfWidth), static_cast<float>(view.halfHeight));
        // The pattern repeats every 0.20 / 0.18 seconds. Wrap time to avoid
        // losing floating-point precision during long-running installations.
        shader.setFloat("uTime", static_cast<float>(std::fmod(time, 0.20 / 0.18)));
        shader.setFloat("uBendAmount", bendAmount);
        shader.setFloat("uInnerRadius", innerRadius);
        for (std::size_t i = 0; i < angles.size(); ++i) {
            const double start = angles[i];
            const double end = i + 1 < angles.size() ? angles[i + 1] : angles[0] + rays::tau;
            shader.setFloat("uAngleStart", static_cast<float>(start));
            shader.setFloat("uAngleSpan", static_cast<float>(end - start));
            shader.setFloat("uBandOffset", static_cast<float>(i % 2));
            graphics::fillFan(rays::wedgeContour(start, end, view.halfWidth, view.halfHeight), {1, 1, 1});
        }
        shader.stop();
        if (showCenterDisk) {
            // Cover the largest outward cutoff, with a little extra room for
            // the disk's polygonal approximation and rasterization.
            const double radius = (innerRadius + std::abs(bendAmount)) * 1.02;
            graphics::disk({0, 0}, radius, {0, 0, 0});
        }
        window.present();
        return true;
    }
};
} // namespace

int main(int, char **) try
{
    auto app = std::make_shared<App>();
    graphics::run([app] { return app->frame(); });
    return 0;
}
catch (const std::exception &error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
