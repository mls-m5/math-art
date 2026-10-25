#include "terrain.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}
} // namespace

int main()
{
    landscape::Terrain terrain(42), same(42), other(91);
    const auto camera = terrain.camera(0, 1.5);
    const auto offset = camera.eye - camera.target;
    require(std::abs(glm::degrees(std::atan2(offset.y, offset.x)) - 30.0) < 1e-10,
            "Camera azimuth is not 30 degrees around Z");
    require(std::abs(glm::degrees(std::atan2(offset.z, glm::length(glm::dvec2(offset)))) - 30.0) < 1e-10,
            "Camera elevation is not 30 degrees");
    const auto later = terrain.camera(100, 1.5);
    require(later.eye.z == camera.eye.z && later.target.z == 0,
            "Camera altitude changed while panning");
    const auto firstPixel = camera.projection * camera.view * glm::dvec4(0, 0, 6, 1);
    const auto laterPixel = later.projection * later.view * glm::dvec4(0, 0, 6, 1);
    require(laterPixel.y > firstPixel.y, "Landscape does not scroll upward");
    const auto oneSecond = terrain.camera(1, 1.5);
    const auto movingPixel = oneSecond.projection * oneSecond.view * glm::dvec4(0, 0, 6, 1);
    require(movingPixel.y - firstPixel.y > 0.15, "Upward pan is too subtle to see");
    require(std::abs(movingPixel.x - firstPixel.x) < 1e-10, "Pan drifts sideways on screen");
    for (double depth : {-20.0, -100.0, -200.0}) {
        const auto a = camera.projection * glm::dvec4(0, 0, depth, 1);
        const auto b = camera.projection * glm::dvec4(1, 0, depth, 1);
        require(a.w == 1 && b.w == 1, "Projection is not orthographic");
        require(std::abs((b.x - a.x) - 1.0 / camera.halfSize.x) < 1e-10,
                "Apparent scale varies with depth");
    }
    require(terrain.height(-12.5, 8.5, 3.0) == same.height(-12.5, 8.5, 3.0),
            "Seeded terrain is not reproducible");
    require(std::abs(terrain.height(0.5, 0.5, 0) - other.height(0.5, 0.5, 0)) > 1e-4,
            "Random seed did not change the terrain");
    bool animated = false;
    for (int i = -50; i <= 50; ++i) {
        const double h = terrain.height(i + 0.5, i * 0.7, i * 0.1);
        require(h >= landscape::minimumHeight && h <= landscape::maximumHeight,
                "Noise exceeded terrain height bounds");
        require(std::abs(terrain.height(i + 0.5, i * 0.7, i * 0.1 + 0.0001) - h) < 0.01,
                "Noise jumps instead of changing smoothly over time");
        if (std::abs(terrain.height(i + 0.5, i * 0.7, i * 0.1 + 1) - h) > 0.01)
            animated = true;
    }
    require(animated, "Terrain does not animate");
    int clamped = 0;
    for (int y = -50; y < 50; ++y) {
        for (int x = -50; x < 50; ++x) {
            const double h = terrain.height(x + 0.5, y + 0.5, 3.0);
            if (h == landscape::minimumHeight || h == landscape::maximumHeight)
                ++clamped;
        }
    }
    require(clamped < 10, "Noise artifacts created clipped, flat plateaus");
    for (double aspect : {0.5, 1.5, 2.5}) {
        const auto view = terrain.camera(35, aspect);
        const auto bounds = terrain.visibleTiles(view);
        const auto inverse = glm::inverse(view.projection * view.view);
        for (double x : {-1.0, 0.0, 1.0}) {
            for (double y : {-1.0, 0.0, 1.0}) {
                const auto near = inverse * glm::dvec4(x, y, -1, 1);
                const auto far = inverse * glm::dvec4(x, y, 1, 1);
                for (double z : {landscape::minimumHeight, landscape::maximumHeight}) {
                    const auto p = glm::dvec3(near + (far - near) * ((z - near.z) / (far.z - near.z)));
                    require(p.x >= bounds.minX && p.x <= bounds.maxX &&
                                p.y >= bounds.minY && p.y <= bounds.maxY,
                            "Generated tiles do not cover the viewport");
                }
            }
        }
        const auto mesh = terrain.mesh(4, 35, aspect);
        const auto columns = static_cast<std::size_t>(bounds.maxX - bounds.minX + 1) *
            (bounds.maxY - bounds.minY + 1);
        require(!mesh.empty() && mesh.size() % 6 == 0 && mesh.size() < columns * 18,
                "Hidden side faces were not culled");
        std::size_t topVertices = 0;
        bool darkSide = false, lightSide = false;
        const float topLight = mesh.front().variant;
        for (const auto &vertex : mesh) {
            if (vertex.variant == topLight) ++topVertices;
            else if (vertex.variant == 0.25f) darkSide = true;
            else if (vertex.variant > 0.25f && vertex.variant < topLight) lightSide = true;
            require(std::isfinite(vertex.position.x) && std::isfinite(vertex.position.y) &&
                        vertex.parameters.x > -1 && vertex.parameters.x < 1,
                    "Terrain vertex has invalid projected depth");
            require(vertex.variant >= 0.25f && vertex.variant <= 1.0f, "Invalid sun illumination");
            if (vertex.coordinate.x == landscape::columnBottom && std::abs(vertex.position.x) <= 1)
                require(vertex.position.y < -1, "A column's bottom end is visible");
        }
        require(topVertices == columns * 6, "Column tops are missing");
        require(darkSide && lightSide, "Sunlight does not shade side normals differently");
        const auto a = inverse * glm::dvec4(mesh[0].position.x, mesh[0].position.y, mesh[0].parameters.x, 1);
        const auto b = inverse * glm::dvec4(mesh[1].position.x, mesh[1].position.y, mesh[1].parameters.x, 1);
        require(std::abs(glm::length(glm::dvec3(b - a)) - 1.0) < 1e-8,
                "Columns are not one XY unit wide");
    }
    std::cout << "Terrain angles, orthographic projection, noise, lighting, panning, and coverage tests passed\n";
}
