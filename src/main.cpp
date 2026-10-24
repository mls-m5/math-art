#include "flower.h"
#include "graphics/graphics.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <random>
#include <vector>

namespace {
struct App
{
    graphics::Window window{
        "Blumme - flowers | V: polygons | R: randomize | Space: pause | Esc: quit"};
    const blumme::Bounds bounds;
    std::mt19937 random{graphics::randomSeed()};
    std::vector<blumme::Point> sites;
    void randomize() {
        std::uniform_real_distribution<double> x(-bounds.halfWidth * 0.95, bounds.halfWidth * 0.95);
        std::uniform_real_distribution<double> y(-bounds.halfHeight * 0.95, bounds.halfHeight * 0.95);
        sites.clear();
        while (sites.size() < 80) {
            const blumme::Point candidate{x(random), y(random)};
            const bool tooClose = std::any_of(sites.begin(), sites.end(), [&](blumme::Point p) {
                return std::hypot(p.x - candidate.x, p.y - candidate.y) < 0.001;
            });
            if (!tooClose)
                sites.push_back(candidate);
        }
    }
    std::vector<blumme::Cell> cells;
    App() { randomize(); cells = blumme::buildVoronoi(sites, bounds); }
    bool running = true;
    bool paused = false;
    bool showPolygons = false;
    double accumulator = 0.0;
    static constexpr double step = 1.0 / 120.0;

    bool frame() {
        bool dirty = false;
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
                    accumulator = 0.0;
                    break;
                case SDLK_v:
                    showPolygons = !showPolygons;
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
            return false;

        const double elapsed = window.elapsed();
        if (!paused) {
            accumulator += elapsed;
            while (accumulator >= step) {
                blumme::relaxSites(sites, bounds, step);
                accumulator -= step;
                dirty = true;
            }
        }
        if (dirty)
            cells = blumme::buildVoronoi(sites, bounds);
        if (!window.beginFrame({bounds.halfWidth + 0.08, bounds.halfHeight + 0.08}))
            return true;

        constexpr graphics::Color palette[] = {
            {0.92f, 0.35f, 0.52f}, {0.68f, 0.42f, 0.88f},
            {0.95f, 0.56f, 0.36f}, {0.38f, 0.68f, 0.88f},
            {0.91f, 0.46f, 0.70f}, {0.42f, 0.77f, 0.65f}};
        for (std::size_t i = 0; i < cells.size(); ++i) {
            const auto &cell = cells[i];
            const auto color = palette[i % 6];
            for (std::size_t j = 0; j < cell.polygon.size(); ++j) {
                const auto petal = blumme::buildPetal(
                    cell.center, cell.polygon[j], cell.polygon[(j + 1) % cell.polygon.size()]);
                const float shade = j % 2 == 0 ? 1.0f : 0.88f;
                graphics::fillFan(petal, {color.r * shade, color.g * shade, color.b * shade});
            }
        }
        if (showPolygons) {
            for (const auto &cell : cells)
                graphics::outline(cell.polygon, {0.45f, 0.75f, 0.85f});
        }
        for (const auto &cell : cells) {
            graphics::disk(cell.center, 0.026, {0, 0, 0});
            graphics::disk(cell.center, 0.018, {1.0f, 0.78f, 0.30f});
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
