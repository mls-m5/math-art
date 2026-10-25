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

    require(!spaceart::projectStar(starAt(220, 0), camera, 960, 640, 28), "Spawned star did not start invisible");
    const auto faint = spaceart::projectStar(starAt(205, 0), camera, 960, 640, 28);
    const auto bright = spaceart::projectStar(starAt(150, 0), camera, 960, 640, 28);
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
    for (const auto &star : field.stars()) {
        const double depth = glm::dot(star.position - field.camera().position, field.camera().forward);
        require(depth >= 220 - 1e-8 && depth <= 235 + 1e-8, "Star was not respawned far ahead");
        require(!spaceart::projectStar(star, field.camera(), 960, 640, 28), "Recycled star popped in visibly");
    }
    for (int i = 0; i < 900; ++i)
        field.update(1.0 / 60.0, 75, 1.5);
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
    std::cout << "3D camera, perspective, star lifecycle, and capsule mesh tests passed\n";
}
