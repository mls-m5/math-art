#include "graphics/graphics.h"
#include "graphics/shader.h"
#include "snow.h"
#include "space_shaders.h" // Shared pixel-space capsule shader for rounded branches.

#include <algorithm>
#include <exception>
#include <iostream>
#include <random>

namespace {
struct App
{
    graphics::Window window{"Snowflakes - V: snowfall | R: new flakes | Space: pause | F: fullscreen"};
    graphics::Shader shader{spaceart::vertexSource, spaceart::fragmentSource};
    std::mt19937 random{graphics::randomSeed()};
    std::vector<snow::Flake> flakes;
    snow::Snowfall snowfall{static_cast<std::uint32_t>(random())};
    bool running = true, paused = false, falling = true;
    double time = 0;
    App() { regenerate(); }

    void regenerate()
    {
        flakes.clear();
        for (int i = 0; i < 9; ++i) flakes.push_back(snow::generateFlake(static_cast<std::uint32_t>(random())));
        snowfall = snow::Snowfall(static_cast<std::uint32_t>(random()), flakes.size());
        time = 0;
    }

    bool frame()
    {
        SDL_Event event;
        while (window.pollEvent(event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE: running = false; break;
                case SDLK_SPACE: paused = !paused; break;
                case SDLK_v: falling = !falling; break;
                case SDLK_r: regenerate(); break;
                default: break;
                }
            }
        }
        if (!running) return false;
        const double dt = window.elapsed();
        if (!paused) {
            time += dt;
            snowfall.update(dt);
        }
        if (!window.beginFrame({1.4, 0.9}, {0.018f, 0.035f, 0.065f})) return true;
        const auto size = window.drawableSize();
        const auto view = window.view();
        const int width = static_cast<int>(size.x), height = static_cast<int>(size.y);
        std::vector<graphics::Vertex> mesh;
        mesh.reserve(falling ? 180000 : 12000);
        if (falling) {
            for (const auto &p : snowfall.particles()) {
                const double radius = std::min(width, height) * (0.007 + 0.024 * p.depth);
                snow::appendFlake(mesh, flakes[p.variant], {p.x*width, p.y*height},
                                  radius, p.angle, 0.30 + 0.65*p.depth, width, height);
                // Duplicate near the seam so flakes wrap without popping.
                if (p.x * width < radius)
                    snow::appendFlake(mesh, flakes[p.variant], {p.x*width+width, p.y*height},
                                      radius, p.angle, 0.30 + 0.65*p.depth, width, height);
                if ((1-p.x)*width < radius)
                    snow::appendFlake(mesh, flakes[p.variant], {p.x*width-width, p.y*height},
                                      radius, p.angle, 0.30 + 0.65*p.depth, width, height);
            }
        } else {
            const double cellWidth = width / 3.0, cellHeight = height / 3.0;
            const double radius = std::min(cellWidth, cellHeight) * 0.35;
            for (std::size_t i = 0; i < flakes.size(); ++i)
                snow::appendFlake(mesh, flakes[i], {(i%3 + 0.5)*cellWidth, (2-i/3 + 0.5)*cellHeight},
                                  radius, time * 0.035, 0.9, width, height);
        }
        for (auto &vertex : mesh) {
            vertex.position.x *= view.halfWidth;
            vertex.position.y *= view.halfHeight;
        }
        shader.use();
        graphics::additiveBlending(true);
        graphics::triangles(mesh);
        graphics::additiveBlending(false);
        shader.stop();
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
