#include "graphics/graphics.h"
#include "graphics/shader.h"
#include "grid.h"
#include "grid_shaders.h"

#include <array>
#include <exception>
#include <iostream>
#include <random>
#include <string>

namespace {
struct App
{
    graphics::Window window{"Wonky grid - W: waves | R: randomize | Space: pause | Esc: quit"};
    graphics::Shader shader{wonky::vertexSource, wonky::fragmentSource};
    std::mt19937 random{graphics::randomSeed()};
    static constexpr int columns = 16;
    static constexpr int rows = 10;
    wonky::Grid grid{columns, rows, static_cast<std::uint32_t>(random())};
    struct Wave { float amplitude, frequency, phase; };
    std::array<Wave, 6> waves;
    void randomizeWaves() {
        // Keep curves ordered and inside the screen, even with overtones.
        std::uniform_real_distribution<float> amplitude(0.025f, 0.045f);
        std::uniform_real_distribution<float> frequency(0.7f, 1.4f);
        std::uniform_real_distribution<float> phase(0.0f, 6.2831853f);
        for (auto &wave : waves)
            wave = {amplitude(random), frequency(random), phase(random)};
    }
    App() { randomizeWaves(); }
    bool running = true;
    bool paused = false;
    bool showWaves = true;
    double time = 0.0;
    bool frame() {
        SDL_Event event;
        while (window.pollEvent(event)) {
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_SPACE:
                    paused = !paused;
                    break;
                case SDLK_w:
                    showWaves = !showWaves;
                    break;
                case SDLK_r:
                    grid = wonky::Grid(columns, rows, random());
                    randomizeWaves();
                    time = 0.0;
                    break;
                default:
                    break;
                }
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
        auto mesh = grid.mesh(time);
        for (auto &vertex : mesh) {
            vertex.position.x = (2.0 * vertex.position.x - 1.0) * view.halfWidth;
            vertex.position.y = (2.0 * vertex.position.y - 1.0) * view.halfHeight;
        }
        shader.use();
        shader.setVec2("uView", static_cast<float>(view.halfWidth), static_cast<float>(view.halfHeight));
        shader.setFloat("uTime", static_cast<float>(time));
        shader.setFloat("uWavesEnabled", showWaves ? 1.0f : 0.0f);
        for (std::size_t i = 0; i < waves.size(); ++i) {
            const std::string index = "[" + std::to_string(i) + "]";
            shader.setFloat(("uWaveAmplitude" + index).c_str(), waves[i].amplitude);
            shader.setFloat(("uWaveFrequency" + index).c_str(), waves[i].frequency);
            shader.setFloat(("uWavePhase" + index).c_str(), waves[i].phase);
        }
        graphics::triangles(mesh);
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
