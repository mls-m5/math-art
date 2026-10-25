#pragma once

#include "graphics/point.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace landscape {
inline constexpr double azimuthDegrees = 30.0;
inline constexpr double elevationDegrees = 30.0;
inline constexpr double minimumHeight = 2.0;
inline constexpr double maximumHeight = 10.0;
inline constexpr double columnBottom = -128.0;
inline constexpr double panSpeed = 4.0; // Ground units per second.

struct Camera
{
    glm::dvec3 eye, target;
    glm::dmat4 view, projection;
    glm::dvec2 halfSize;
};

struct TileBounds { int minX, maxX, minY, maxY; }; // Inclusive tile coordinates.

// Infinite XY terrain, Z up. Sampling uses absolute world coordinates so
// scrolling never regenerates a different height for an existing tile.
class Terrain
{
public:
    explicit Terrain(std::uint32_t seed);
    double height(double x, double y, double time) const;
    Camera camera(double panTime, double aspect) const;
    TileBounds visibleTiles(const Camera &camera) const;
    // Positions are NDC XY; parameters.x carries NDC depth for the depth buffer.
    std::vector<graphics::Vertex> mesh(double waveTime, double panTime, double aspect) const;

private:
    glm::dvec2 offset_;
};
} // namespace landscape
