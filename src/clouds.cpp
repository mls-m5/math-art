#include "graphics/graphics.h"
#include "graphics/shader.h"
#include "graphics/texture.h"
#include "simplex_texture.h"
#include "clouds_shaders.h"

#include <array>
#include <exception>
#include <iostream>
#include <random>

namespace {
struct App
{
    graphics::Window window{"Clouds - W: wind | R: randomize | Space: pause | F: fullscreen"};
    graphics::Shader shader{clouds::vertexSource, clouds::fragmentSource};
    std::mt19937 random{graphics::randomSeed()};
    std::array<std::unique_ptr<graphics::Texture2D>, 3> textures;
    bool running = true, paused = false, wind = true;
    double time = 0.0, windTime = 0.0;

    App() { regenerate(); }
    void regenerate()
    {
        for (auto &texture : textures) {
            const noiseart::SimplexTexture noise(static_cast<std::uint32_t>(random()));
            texture = std::make_unique<graphics::Texture2D>(256, 256, noise.rgba());
        }
        time = windTime = 0.0;
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
                case SDLK_w: wind = !wind; break;
                case SDLK_r: regenerate(); break;
                default: break;
                }
            }
        }
        if (!running) return false;
        const double dt = window.elapsed(0.5);
        if (!paused) {
            time += dt;
            if (wind) windTime += dt;
        }
        if (!window.beginFrame({1.4, 0.9})) return true;
        const auto view = window.view();
        shader.use();
        for (unsigned int i = 0; i < textures.size(); ++i) textures[i]->bind(i);
        shader.setInt("uNoise0", 0);
        shader.setInt("uNoise1", 1);
        shader.setInt("uNoise2", 2);
        shader.setFloat("uTime", static_cast<float>(time));
        shader.setFloat("uWindTime", static_cast<float>(windTime));
        shader.setFloat("uAspect", static_cast<float>(view.halfWidth / view.halfHeight));
        const double x = view.halfWidth, y = view.halfHeight;
        graphics::triangles({
            {{-x,-y},{0,0}}, {{x,-y},{1,0}}, {{x,y},{1,1}},
            {{-x,-y},{0,0}}, {{x,y},{1,1}}, {{-x,y},{0,1}}});
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
