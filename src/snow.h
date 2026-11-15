#pragma once

#include "graphics/point.h"
#include <cstdint>
#include <random>
#include <vector>

namespace snow {
struct Branch
{
    graphics::Point start, end; // Unit-radius flake coordinates.
    double width;
};
using Flake = std::vector<Branch>;
Flake generateFlake(std::uint32_t seed);

struct Particle
{
    double x, y; // Normalized viewport coordinates, bottom-left origin.
    double depth, angle, spin, phase;
    std::size_t variant;
};

class Snowfall
{
public:
    explicit Snowfall(std::uint32_t seed, std::size_t variants = 9, std::size_t count = 150);
    void update(double dt);
    const std::vector<Particle> &particles() const { return particles_; }
private:
    void spawn(Particle &particle, bool initial);
    std::mt19937 random_;
    std::size_t variants_;
    double time_ = 0;
    std::vector<Particle> particles_;
};

// Emit pixel-sized, antialiased capsule geometry for one flake. Screen positions
// are normalized to [-1,1]; local capsule coordinates remain in real pixels.
void appendFlake(std::vector<graphics::Vertex> &mesh, const Flake &flake,
                 graphics::Point center, double radius, double angle,
                 double brightness, int width, int height);
} // namespace snow
