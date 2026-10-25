#include "terrain.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

namespace landscape {
namespace {
const glm::dvec3 sun = glm::normalize(glm::dvec3(-0.45, 0.65, 1.0));
constexpr double halfViewHeight = 11.0;
constexpr double cameraDistance = 100.0;

double illumination(glm::dvec3 normal)
{
    return 0.25 + 0.75 * std::max(0.0, glm::dot(normal, sun));
}
} // namespace

Terrain::Terrain(std::uint32_t seed)
{
    std::mt19937 random(seed);
    std::uniform_real_distribution<double> offset(-1000.0, 1000.0);
    offset_ = {offset(random), offset(random)};
}

double Terrain::height(double x, double y, double time) const
{
    // GLM's simplex gradient/hash constants target float precision. Keep world
    // coordinates double, but sample its reference float implementation to
    // avoid double-precision rounding artifacts in gradient selection.
    const glm::vec2 p((glm::dvec2(x, y) + offset_) * 0.065);
    const float t = static_cast<float>(time);
    const double value = (glm::simplex(glm::vec3(p, t * 0.12f)) +
        0.5 * glm::simplex(glm::vec3(p * 2.0f, t * 0.16f + 31.0f)) +
        0.25 * glm::simplex(glm::vec3(p * 4.0f, t * 0.21f + 73.0f))) / 1.75;
    return std::clamp(6.0 + 4.0 * value, minimumHeight, maximumHeight);
}

Camera Terrain::camera(double panTime, double aspect) const
{
    if (!std::isfinite(aspect) || aspect <= 0 || !std::isfinite(panTime))
        throw std::invalid_argument("Terrain camera needs finite time and positive aspect");
    const double azimuth = glm::radians(azimuthDegrees);
    const double elevation = glm::radians(elevationDegrees);
    const glm::dvec3 groundDirection(std::cos(azimuth), std::sin(azimuth), 0);
    // Translate along the ground, toward the eye's XY direction. Stationary
    // terrain consequently drifts upward on screen; altitude stays at 50.
    const glm::dvec3 target = groundDirection * panTime * panSpeed;
    const glm::dvec3 eye = target + cameraDistance *
        (groundDirection * std::cos(elevation) + glm::dvec3(0, 0, std::sin(elevation)));
    Camera result;
    result.eye = eye;
    result.target = target;
    result.halfSize = {halfViewHeight * aspect, halfViewHeight};
    result.view = glm::lookAt(eye, target, glm::dvec3(0, 0, 1));
    result.projection = glm::ortho(-result.halfSize.x, result.halfSize.x,
        -result.halfSize.y, result.halfSize.y, 0.1, 512.0);
    return result;
}

TileBounds Terrain::visibleTiles(const Camera &camera) const
{
    const glm::dmat4 inverse = glm::inverse(camera.projection * camera.view);
    glm::dvec2 low(std::numeric_limits<double>::max());
    glm::dvec2 high(std::numeric_limits<double>::lowest());
    // Intersect orthographic corner rays with both extreme surface heights.
    // Their XY bounds cover every potentially visible top and exposed side.
    for (double x : {-1.0, 1.0}) {
        for (double y : {-1.0, 1.0}) {
            const auto near = inverse * glm::dvec4(x, y, -1, 1);
            const auto far = inverse * glm::dvec4(x, y, 1, 1);
            const glm::dvec3 a = glm::dvec3(near) / near.w;
            const glm::dvec3 b = glm::dvec3(far) / far.w;
            for (double z : {minimumHeight, maximumHeight}) {
                const double t = (z - a.z) / (b.z - a.z);
                const glm::dvec2 p = glm::dvec2(a + (b - a) * t);
                low = glm::min(low, p);
                high = glm::max(high, p);
            }
        }
    }
    constexpr int overscan = 3;
    return {static_cast<int>(std::floor(low.x)) - overscan,
            static_cast<int>(std::ceil(high.x)) + overscan,
            static_cast<int>(std::floor(low.y)) - overscan,
            static_cast<int>(std::ceil(high.y)) + overscan};
}

std::vector<graphics::Vertex> Terrain::mesh(double waveTime, double panTime, double aspect) const
{
    const auto view = camera(panTime, aspect);
    const auto bounds = visibleTiles(view);
    const glm::dmat4 transform = view.projection * view.view;
    // Sample each tile once, including a +X/+Y neighbor halo. Interior wall
    // sections below neighboring tops cannot be seen and must not be drawn.
    const int stride = bounds.maxX - bounds.minX + 2;
    std::vector<double> heights;
    heights.reserve(static_cast<std::size_t>(stride) * (bounds.maxY - bounds.minY + 2));
    for (int y = bounds.minY; y <= bounds.maxY + 1; ++y)
        for (int x = bounds.minX; x <= bounds.maxX + 1; ++x)
            heights.push_back(height(x + 0.5, y + 0.5, waveTime));
    const auto sampledHeight = [&](int x, int y) {
        return heights[static_cast<std::size_t>(y - bounds.minY) * stride + x - bounds.minX];
    };
    std::vector<graphics::Vertex> vertices;
    vertices.reserve(static_cast<std::size_t>(bounds.maxX - bounds.minX + 1) *
                     (bounds.maxY - bounds.minY + 1) * 18);
    for (int y = bounds.minY; y <= bounds.maxY; ++y) {
        for (int x = bounds.minX; x <= bounds.maxX; ++x) {
            const double top = sampledHeight(x, y);
            const double xBottom = x == bounds.maxX ? columnBottom : sampledHeight(x + 1, y);
            const double yBottom = y == bounds.maxY ? columnBottom : sampledHeight(x, y + 1);
            const glm::dvec3 points[]{
                {x, y, top}, {x + 1, y, top}, {x + 1, y + 1, top}, {x, y + 1, top},
                {x + 1, y, xBottom}, {x + 1, y + 1, xBottom},
                {x + 1, y + 1, yBottom}, {x, y + 1, yBottom}};
            graphics::Vertex projected[8];
            for (int i = 0; i < 8; ++i) {
                const glm::dvec4 clip = transform * glm::dvec4(points[i], 1);
                const glm::dvec3 ndc = glm::dvec3(clip) / clip.w;
                projected[i] = {{ndc.x, ndc.y}, {points[i].z, 0}, 1.0f, {ndc.z, 0}};
            }
            const auto face = [&](int a, int b, int c, int d, glm::dvec3 normal) {
                const float shade = static_cast<float>(illumination(normal));
                for (int index : {a, b, c, a, c, d}) {
                    auto vertex = projected[index];
                    vertex.variant = shade;
                    vertices.push_back(vertex);
                }
            };
            face(0, 1, 2, 3, {0, 0, 1});
            // Fixed 30-degree azimuth sees only +X/+Y sides; omit back-facing
            // walls. Depth testing handles occlusion between adjacent columns.
            if (top > xBottom)
                face(1, 4, 5, 2, {1, 0, 0});
            if (top > yBottom)
                face(2, 6, 7, 3, {0, 1, 0});
        }
    }
    return vertices;
}
} // namespace landscape
