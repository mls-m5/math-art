#include "graphics/graphics.h"
#include "graphics/shader.h"
#include "space.h"
#include "space_shaders.h"

#include <exception>
#include <iostream>

namespace {
struct App
{
    graphics::Window window{"Space travel - B: boost | R: randomize | Space: pause | Esc: quit"};
    graphics::Shader shader{spaceart::vertexSource, spaceart::fragmentSource};
    spaceart::Starfield field{graphics::randomSeed()};
    bool running = true;
    bool paused = false;
    bool boost = false;

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
                case SDLK_b: boost = !boost; break;
                case SDLK_r:
                    field = spaceart::Starfield(graphics::randomSeed(),
                        window.view().halfHeight > 0 ? window.view().halfWidth / window.view().halfHeight : 1.5);
                    break;
                default: break;
                }
            }
        }
        if (!running)
            return false;
        const double dt = window.elapsed();
        if (!window.beginFrame({1.4, 0.9}))
            return true;
        const auto view = window.view();
        const auto size = window.drawableSize();
        const double speed = boost ? 150.0 : 56.0;
        if (!paused)
            field.update(dt, speed, view.halfWidth / view.halfHeight);
        auto mesh = spaceart::buildStarMesh(field, static_cast<int>(size.x), static_cast<int>(size.y), speed);
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
