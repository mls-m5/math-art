#include "space.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace spaceart {
namespace {
constexpr double fieldOfView = 58.0;
constexpr double exposure = 0.10; // Seconds of camera travel represented by a trail.

double smooth(double low, double high, double value)
{
    const double t = std::clamp((value - low) / (high - low), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

bool clipScreen(glm::dvec2 &a, glm::dvec2 &b, double width, double height, double padding)
{
    const glm::dvec2 delta = b - a;
    double low = 0.0, high = 1.0;
    for (int axis = 0; axis < 2; ++axis) {
        const double minimum = -padding;
        const double maximum = (axis == 0 ? width : height) + padding;
        if (std::abs(delta[axis]) < 1e-12) {
            if (a[axis] < minimum || a[axis] > maximum)
                return false;
        } else {
            double enter = (minimum - a[axis]) / delta[axis];
            double leave = (maximum - a[axis]) / delta[axis];
            if (enter > leave)
                std::swap(enter, leave);
            low = std::max(low, enter);
            high = std::min(high, leave);
            if (low > high)
                return false;
        }
    }
    b = a + delta * high;
    a += delta * low;
    return true;
}
} // namespace

double approachFlightSpeed(double current, bool boosted, double dt)
{
    if (!std::isfinite(current) || current < 0 || !std::isfinite(dt) || dt < 0)
        throw std::invalid_argument("Flight speed and step must be nonnegative and finite");
    const double target = boosted ? boostedSpeed : normalSpeed;
    return current + (target - current) * -std::expm1(-dt / 0.65);
}

glm::dmat4 Camera::view() const
{
    return glm::lookAt(position, position + forward, glm::dvec3(0, 1, 0));
}

glm::dmat4 Camera::projection(double aspect) const
{
    if (!std::isfinite(aspect) || aspect <= 0)
        throw std::invalid_argument("Camera aspect must be positive and finite");
    return glm::perspective(glm::radians(fieldOfView), aspect, nearPlane, spawnDistance + 60.0);
}

Starfield::Starfield(std::uint32_t seed, double aspect, std::size_t count)
    : random_(seed), aspect_(aspect), stars_(count)
{
    camera_.projection(aspect); // Validate before spawning.
    for (auto &star : stars_)
        spawn(star, true);
}

void Starfield::spawn(Star &star, bool initial)
{
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    const double depth = initial ? unit(random_) * (spawnDistance + 15.0)
                                 : spawnDistance + 15.0 * unit(random_);
    const glm::dvec3 right = glm::normalize(glm::cross(camera_.forward, glm::dvec3(0, 1, 0)));
    const glm::dvec3 up = glm::cross(right, camera_.forward);
    const double spread = spawnDistance * std::tan(glm::radians(fieldOfView * 0.5));
    double x, y;
    // Reject the small central corridor. With a fixed travel direction, a
    // star's lateral distance remains constant throughout its entire lifetime.
    // For extreme portrait aspects, retain enough width to sample safely.
    const double halfWidth = std::max(spread * aspect_, safetyRadius);
    do {
        x = (2.0 * unit(random_) - 1.0) * halfWidth;
        y = (2.0 * unit(random_) - 1.0) * spread;
    } while (x * x + y * y < safetyRadius * safetyRadius);
    star.position = camera_.position + camera_.forward * depth + right * x + up * y;
    star.luminance = 0.45 + 0.55 * unit(random_);
    star.size = 0.035 + 0.035 * unit(random_);
}

void Starfield::update(double dt, double speed, double aspect)
{
    if (!std::isfinite(dt) || dt < 0 || !std::isfinite(speed) || speed < 0)
        throw std::invalid_argument("Flight step and speed must be nonnegative and finite");
    camera_.projection(aspect);
    aspect_ = aspect;
    camera_.position += camera_.forward * speed * dt;
    for (auto &star : stars_) {
        const double depth = glm::dot(star.position - camera_.position, camera_.forward);
        if (depth < -behindDistance)
            spawn(star, false);
    }
}

std::optional<Capsule> projectStar(const Star &star, const Camera &camera,
                                 int width, int height, double speed)
{
    if (width <= 0 || height <= 0)
        return std::nullopt;
    const glm::dmat4 view = camera.view();
    const glm::dmat4 projection = camera.projection(static_cast<double>(width) / height);
    glm::dvec3 head = glm::dvec3(view * glm::dvec4(star.position, 1.0));
    // This is the same stationary star viewed from the previous camera position,
    // expressed in the current view. It lies farther away, toward the vanishing point.
    glm::dvec3 tail = glm::dvec3(view * glm::dvec4(
        star.position + camera.forward * speed * exposure, 1.0));
    const double depth = -head.z;
    const double brightness = star.luminance *
        (1.0 - smooth(fadeStartDistance, spawnDistance, depth)) * smooth(nearPlane, 1.0, depth);
    if (brightness <= 0.0001 || (head.z > -nearPlane && tail.z > -nearPlane))
        return std::nullopt;
    if (head.z > -nearPlane)
        head = glm::mix(head, tail, (-nearPlane - head.z) / (tail.z - head.z));
    if (tail.z > -nearPlane)
        tail = glm::mix(tail, head, (-nearPlane - tail.z) / (head.z - tail.z));
    const auto pixel = [&](glm::dvec3 p) {
        const glm::dvec4 clip = projection * glm::dvec4(p, 1.0);
        const glm::dvec2 ndc = glm::dvec2(clip) / clip.w;
        return (ndc + glm::dvec2(1.0)) * glm::dvec2(width, height) * 0.5;
    };
    glm::dvec2 start = pixel(head), end = pixel(tail);
    const double focalLength = height / (2.0 * std::tan(glm::radians(fieldOfView * 0.5)));
    const double radius = std::clamp(star.size * focalLength / std::max(depth, nearPlane), 0.55, 2.6);
    if (!clipScreen(start, end, width, height, radius + 1.0))
        return std::nullopt;
    return Capsule{start, end, radius, brightness};
}

std::vector<graphics::Vertex> buildStarMesh(const Starfield &field, int width, int height, double speed)
{
    std::vector<graphics::Vertex> mesh;
    if (width <= 0 || height <= 0)
        return mesh;
    mesh.reserve(field.stars().size() * 6);
    for (const auto &star : field.stars()) {
        const auto capsule = projectStar(star, field.camera(), width, height, speed);
        if (!capsule)
            continue;
        const glm::dvec2 delta = capsule->end - capsule->start;
        const double length = glm::length(delta);
        const glm::dvec2 direction = length > 1e-8 ? delta / length : glm::dvec2(1, 0);
        const glm::dvec2 normal(-direction.y, direction.x);
        const glm::dvec2 center = (capsule->start + capsule->end) * 0.5;
        const double halfLength = length * 0.5;
        const double padding = capsule->radius + 1.0;
        const double x = halfLength + padding;
        const double y = padding;
        const glm::dvec2 corners[]{{-x, -y}, {x, -y}, {x, y}, {-x, y}};
        for (int index : {0, 1, 2, 0, 2, 3}) {
            const auto local = corners[index];
            const auto p = center + direction * local.x + normal * local.y;
            mesh.push_back({{2.0 * p.x / width - 1.0, 2.0 * p.y / height - 1.0},
                {local.x, local.y}, static_cast<float>(capsule->brightness),
                {halfLength, capsule->radius}});
        }
    }
    return mesh;
}
} // namespace spaceart
