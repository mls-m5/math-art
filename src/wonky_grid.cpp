#include "graphics/graphics.h"
#include "graphics/shader.h"
#include "grid.h"
#include "grid_shaders.h"

#include <exception>
#include <iostream>
#include <random>

int main(int, char **) try
{
    graphics::Window window("Wonky grid - R: randomize | Space: pause | Esc: quit");
    graphics::Shader shader(wonky::vertexSource, wonky::fragmentSource);
    std::mt19937 random(std::random_device{}());
    constexpr int columns = 16;
    constexpr int rows = 10;
    wonky::Grid grid(columns, rows, random());
    bool running = true;
    bool paused = false;
    double time = 0.0;
    while (running) {
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
                case SDLK_r:
                    grid = wonky::Grid(columns, rows, random());
                    time = 0.0;
                    break;
                default:
                    break;
                }
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
        auto mesh = grid.mesh(time);
        for (auto &vertex : mesh) {
            vertex.position.x = (2.0 * vertex.position.x - 1.0) * view.halfWidth;
            vertex.position.y = (2.0 * vertex.position.y - 1.0) * view.halfHeight;
        }
        shader.use();
        shader.setVec2("uView", static_cast<float>(view.halfWidth), static_cast<float>(view.halfHeight));
        shader.setFloat("uTime", static_cast<float>(time));
        graphics::triangles(mesh);
        shader.stop();
        window.present();
    }
    return 0;
}
catch (const std::exception &error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
