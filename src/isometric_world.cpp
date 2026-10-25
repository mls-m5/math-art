#include "graphics/graphics.h"
#include "graphics/shader.h"
#include "terrain.h"
#include "terrain_shaders.h"

#include <exception>
#include <iostream>
#include <random>

namespace {
struct App
{
    graphics::Window window{"Isometric world - P: pan | R: randomize | Space: pause | F: fullscreen"};
    graphics::Shader shader{landscape::vertexSource, landscape::fragmentSource};
    std::mt19937 random{graphics::randomSeed()};
    landscape::Terrain terrain{static_cast<std::uint32_t>(random())};
    bool running = true;
    bool paused = false;
    bool pan = true;
    double waveTime = 0.0;
    double panTime = 0.0;
    bool meshDirty = true;
    double meshAspect = 0.0;
    std::vector<graphics::Vertex> mesh;

    bool frame()
    {
        SDL_Event event;
        while (window.pollEvent(event)) {
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE: running = false; break;
                case SDLK_SPACE: paused = !paused; break;
                case SDLK_p: pan = !pan; break;
                case SDLK_r:
                    terrain = landscape::Terrain(static_cast<std::uint32_t>(random()));
                    waveTime = panTime = 0.0;
                    meshDirty = true;
                    break;
                default: break;
                }
            }
        }
        if (!running)
            return false;
        // This is a visual animation, not a stiff physics simulation: let
        // slow frames advance the pan at real-time speed (up to a short pause).
        const double dt = window.elapsed(0.5);
        if (!paused) {
            meshDirty = true;
            waveTime += dt;
            if (pan)
                panTime += dt;
        }
        if (!window.beginFrame({1.4, 0.9}, {0.07f, 0.16f, 0.10f}))
            return true;
        const auto view = window.view();
        const double aspect = view.halfWidth / view.halfHeight;
        if (meshDirty || meshAspect != aspect) {
            mesh = terrain.mesh(waveTime, panTime, aspect);
            for (auto &vertex : mesh) {
                vertex.position.x *= view.halfWidth;
                vertex.position.y *= view.halfHeight;
            }
            meshDirty = false;
            meshAspect = aspect;
        }
        shader.use();
        graphics::depthTesting(true);
        graphics::triangles(mesh);
        graphics::depthTesting(false);
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
