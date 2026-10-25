#include "space.h"

#include <glm/gtc/matrix_transform.hpp>
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
    spaceart::Starfield field(42);
    const auto before = field.stars();
    const auto camera = field.camera();
    const auto checkCorridor = [](const spaceart::Starfield &scene) {
        for (const auto &star : scene.stars()) {
            const auto relative = star.position - scene.camera().position;
            const auto lateral = relative - scene.camera().forward * glm::dot(relative, scene.camera().forward);
            require(glm::length(lateral) >= spaceart::safetyRadius - 1e-7,
                    "Star entered the camera's safety corridor");
        }
    };
    checkCorridor(field);
    require(spaceart::spawnDistance > 220, "Spawn plane did not move farther away");
    field.update(0.1, 28.0, 1.5);
    require(glm::length(field.camera().position - camera.forward * 2.8) < 1e-10,
            "Camera did not move through 3D world space");
    for (std::size_t i = 0; i < before.size(); ++i)
        require(field.stars()[i].position == before[i].position, "World-space star moved before recycling");

    const auto right = glm::normalize(glm::cross(camera.forward, glm::dvec3(0, 1, 0)));
    const auto starAt = [&](double depth, double offset) {
        return spaceart::Star{camera.position + camera.forward * depth + right * offset, 1.0, 0.05};
    };
    const auto center = spaceart::projectStar(starAt(80, 0), camera, 960, 640, 28);
    require(center.has_value(), "Center star is not visible");
    require(glm::length(center->start - glm::dvec2(480, 320)) < 1e-8,
            "3D view/projection does not center the forward axis");
    const auto distant = spaceart::projectStar(starAt(80, 5), camera, 960, 640, 28);
    const auto close = spaceart::projectStar(starAt(40, 5), camera, 960, 640, 28);
    require(distant && close, "Off-axis test star is not visible");
    require(close->start.x > distant->start.x && distant->start.x > 480,
            "Perspective did not expand approaching stars outward");
    require(glm::length(close->end - close->start) > glm::length(distant->end - distant->start),
            "Approaching star did not acquire a longer exposure trail");
    const auto still = spaceart::projectStar(starAt(40, 5), camera, 960, 640, 0);
    require(still && glm::length(still->end - still->start) < 1e-8, "Zero speed did not produce a round star");

    auto movedCamera = camera;
    movedCamera.position = {17.0, -9.0, 28.0};
    auto movedStar = starAt(80, 5);
    movedStar.position += movedCamera.position;
    const auto translated = spaceart::projectStar(movedStar, movedCamera, 960, 640, 28);
    require(translated && glm::length(translated->start - distant->start) < 1e-8,
            "Camera translation is not a real view transform");

    require(!spaceart::projectStar(starAt(spaceart::spawnDistance, 0), camera, 960, 640, 28), "Spawned star did not start invisible");
    const auto faint = spaceart::projectStar(starAt(spaceart::spawnDistance - 15, 0), camera, 960, 640, 28);
    const auto bright = spaceart::projectStar(starAt(spaceart::fadeStartDistance + 10, 0), camera, 960, 640, 28);
    require(faint && bright && faint->brightness < bright->brightness, "Distance fade-in is not gradual");
    require(!spaceart::projectStar(starAt(-5, 0), camera, 960, 640, 28), "Behind-camera star was rendered");
    require(!spaceart::projectStar(starAt(20, 0), camera, 0, 0, 28), "Zero-sized drawable was projected");
    for (double depth : {0.19, 0.2, 0.21, 0.5}) {
        const auto near = spaceart::projectStar(starAt(depth, 0.01), camera, 960, 640, 28);
        if (near)
            require(std::isfinite(near->start.x) && std::isfinite(near->end.y), "Near-plane projection was nonfinite");
    }

    field.update(100, 28, 1.5); // Every original star must now be recycled ahead.
    require(field.stars().size() == before.size(), "Recycling changed the star count");
    checkCorridor(field);
    for (const auto &star : field.stars()) {
        const double depth = glm::dot(star.position - field.camera().position, field.camera().forward);
        require(depth >= spaceart::spawnDistance - 1e-8 && depth <= spaceart::spawnDistance + 15 + 1e-8, "Star was not respawned far ahead");
        require(!spaceart::projectStar(star, field.camera(), 960, 640, 28), "Recycled star popped in visibly");
    }
    for (int i = 0; i < 900; ++i) {
        field.update(1.0 / 60.0, spaceart::boostedSpeed, 1.5);
        if (i % 30 == 0)
            checkCorridor(field);
    }
    for (const auto &star : field.stars())
        require(glm::dot(star.position - field.camera().position, field.camera().forward) >= -spaceart::behindDistance - 1e-8,
                "Star survived beyond the despawn plane");
    for (const auto size : {glm::ivec2(960, 640), glm::ivec2(400, 900)}) {
        const auto mesh = spaceart::buildStarMesh(field, size.x, size.y, 75);
        require(!mesh.empty() && mesh.size() % 6 == 0, "Capsule mesh is empty or incomplete");
        for (const auto &vertex : mesh)
            require(std::isfinite(vertex.position.x) && std::isfinite(vertex.position.y) &&
                        vertex.variant > 0 && vertex.variant <= 1 && vertex.parameters.y > 0,
                    "Capsule vertex is invalid");
    }
    const double firstStep = spaceart::approachFlightSpeed(spaceart::normalSpeed, true, 1.0 / 60.0);
    require(firstStep > spaceart::normalSpeed && firstStep < spaceart::boostedSpeed,
            "Boost switched speed instantly instead of ramping");
    const double whole = spaceart::approachFlightSpeed(spaceart::normalSpeed, true, 0.1);
    const double halves = spaceart::approachFlightSpeed(
        spaceart::approachFlightSpeed(spaceart::normalSpeed, true, 0.05), true, 0.05);
    require(std::abs(whole - halves) < 1e-10, "Boost ramp depends on frame rate");
    double speed = spaceart::normalSpeed;
    for (int i = 0; i < 600; ++i)
        speed = spaceart::approachFlightSpeed(speed, true, 1.0 / 60.0);
    require(std::abs(speed - spaceart::boostedSpeed) < 0.0001, "Boost never approached cruising speed");
    const double slowing = spaceart::approachFlightSpeed(speed, false, 1.0 / 60.0);
    require(slowing < speed && slowing > spaceart::normalSpeed, "Slowdown was not smooth");
    require(spaceart::approachFlightSpeed(speed, false, 0.0) == speed, "Paused ramp changed speed");
    std::cout << "3D projection, lifecycle, safety corridor, and smooth boost tests passed\n";
}
