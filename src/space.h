#pragma once

#include "graphics/point.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

namespace spaceart {

inline constexpr double nearPlane = 0.2;
inline constexpr double spawnDistance = 440.0;
inline constexpr double fadeStartDistance = 280.0;
inline constexpr double behindDistance = 12.0;
// Clear cylinder around the flight axis, measured in camera-local XY.
inline constexpr double safetyRadius = 4.0;
inline constexpr double normalSpeed = 56.0;
inline constexpr double boostedSpeed = 150.0;

// Frame-rate-independent exponential ramp, with a 0.65-second time constant.
double approachFlightSpeed(double current, bool boosted, double dt);

struct Camera
{
    glm::dvec3 position{0.0};
    glm::dvec3 forward = glm::normalize(glm::dvec3(0.15, 0.04, -1.0));
    glm::dmat4 view() const;
    glm::dmat4 projection(double aspect) const;
};

struct Star
{
    glm::dvec3 position;
    double luminance;
    double size;
};

class Starfield
{
public:
    explicit Starfield(std::uint32_t seed, double aspect = 1.5, std::size_t count = 700);
    // Stars remain fixed in world space until recycled. Only the camera moves.
    void update(double dt, double speed, double aspect);
    const Camera &camera() const { return camera_; }
    const std::vector<Star> &stars() const { return stars_; }

private:
    void spawn(Star &star, bool initial);
    Camera camera_;
    std::mt19937 random_;
    double aspect_;
    std::vector<Star> stars_;
};

struct Capsule
{
    glm::dvec2 start, end; // Pixel coordinates, bottom-left origin.
    double radius, brightness;
};

// Project a world-space exposure trail through actual view/perspective matrices.
// Near-plane and screen clipping keep capsules finite as stars pass the camera.
std::optional<Capsule> projectStar(const Star &star, const Camera &camera,
                                 int width, int height, double speed);
std::vector<graphics::Vertex> buildStarMesh(const Starfield &field,
                                         int width, int height, double speed);

} // namespace spaceart
