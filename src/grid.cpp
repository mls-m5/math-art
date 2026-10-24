#include "grid.h"

#include <cmath>
#include <random>
#include <stdexcept>

namespace wonky {
namespace {
graphics::Point largeScaleField(graphics::Point p, double time)
{
    constexpr double tau = 6.283185307179586;
    constexpr double amplitude = 0.08; // Fraction of the whole screen, not a cell.
    const auto envelope = [](graphics::Point q) {
        // Zero on every edge, strongest at the center. Near an edge this
        // fades proportionally to distance, so points cannot escape.
        return 16.0 * q.x * (1.0 - q.x) * q.y * (1.0 - q.y);
    };
    // Compose two bounded, monotonic axis warps instead of adding an arbitrary
    // vector field. Each axis derivative stays positive (1 - 4*amplitude > 0),
    // making the continuous deformation invertible.
    p.x += amplitude * envelope(p) * std::sin(tau * p.y + 0.27 * time);
    p.y += amplitude * envelope(p) * std::sin(tau * p.x - 0.21 * time + 1.3);
    return p;
}
} // namespace

double Grid::Wave::at(double time) const
{
    return amplitude1 * std::sin(frequency1 * time + phase1) +
           amplitude2 * std::sin(frequency2 * time + phase2);
}

Grid::Grid(int columns, int rows, std::uint32_t seed) : columns_(columns), rows_(rows)
{
    if (columns < 1 || rows < 1)
        throw std::invalid_argument("Grid dimensions must be positive");
    std::mt19937 random(seed);
    // Double the original amplitudes: up to 0.32 grid spacings per axis.
    // points() reduces displacement only if needed to keep cells convex.
    std::uniform_real_distribution<double> amplitude(0.07, 0.16);
    std::uniform_real_distribution<double> frequency(0.3, 1.2);
    std::uniform_real_distribution<double> phase(0.0, 6.283185307179586);
    const auto wave = [&]() -> Wave {
        return {amplitude(random), amplitude(random), frequency(random), frequency(random),
                phase(random), phase(random)};
    };
    for (int y = 0; y <= rows_; ++y)
        for (int x = 0; x <= columns_; ++x)
            motions_.push_back({wave(), wave()});
}

std::vector<graphics::Point> Grid::points(double time) const
{
    std::vector<graphics::Point> points;
    points.reserve(motions_.size());
    for (int y = 0; y <= rows_; ++y) {
        for (int x = 0; x <= columns_; ++x) {
            const auto &motion = motions_[static_cast<std::size_t>(y) * (columns_ + 1) + x];
            // Edge vertices may slide tangentially, never leave their edge.
            const double dx = x == 0 || x == columns_ ? 0.0 : motion.x.at(time);
            const double dy = y == 0 || y == rows_ ? 0.0 : motion.y.at(time);
            points.push_back(largeScaleField({(x + dx) / columns_, (y + dy) / rows_}, time));
        }
    }
    // Large offsets can occasionally make a quad concave. Find the largest
    // safe global displacement scale, preserving shared vertices and edges.
    const auto atScale = [&](int x, int y, double scale) {
        const graphics::Point base{static_cast<double>(x) / columns_,
                                   static_cast<double>(y) / rows_};
        const auto p = points[static_cast<std::size_t>(y) * (columns_ + 1) + x];
        return graphics::Point{base.x + (p.x - base.x) * scale,
                               base.y + (p.y - base.y) * scale};
    };
    const auto convex = [&](double scale) {
        for (int y = 0; y < rows_; ++y) {
            for (int x = 0; x < columns_; ++x) {
                const graphics::Point q[]{atScale(x, y, scale), atScale(x + 1, y, scale),
                    atScale(x + 1, y + 1, scale), atScale(x, y + 1, scale)};
                for (int i = 0; i < 4; ++i) {
                    const auto a = q[i], b = q[(i + 1) % 4], c = q[(i + 2) % 4];
                    const double area = (b.x - a.x) * (c.y - a.y) -
                                        (b.y - a.y) * (c.x - a.x);
                    if (area <= 0.001 / (static_cast<double>(columns_) * rows_))
                        return false;
                }
            }
        }
        return true;
    };
    if (!convex(1.0)) {
        double low = 0.0, high = 1.0;
        for (int i = 0; i < 24; ++i) {
            const double middle = (low + high) * 0.5;
            if (convex(middle))
                low = middle;
            else
                high = middle;
        }
        // atScale only reads the corresponding vertex, so in-place is safe.
        for (int y = 0; y <= rows_; ++y)
            for (int x = 0; x <= columns_; ++x)
                points[static_cast<std::size_t>(y) * (columns_ + 1) + x] = atScale(x, y, low);
    }
    return points;
}

std::vector<graphics::Vertex> Grid::mesh(double time) const
{
    const auto positions = points(time);
    std::vector<graphics::Vertex> vertices;
    vertices.reserve(static_cast<std::size_t>(columns_) * rows_ * 6);
    for (int y = 0; y < rows_; ++y) {
        for (int x = 0; x < columns_; ++x) {
            const std::size_t a = static_cast<std::size_t>(y) * (columns_ + 1) + x;
            const std::size_t b = a + 1;
            const std::size_t d = a + columns_ + 1;
            const std::size_t c = d + 1;
            // Every cell resets UVs; shared positions keep the grid watertight.
            vertices.insert(vertices.end(), {
                {positions[a], {0, 0}}, {positions[b], {1, 0}}, {positions[c], {1, 1}},
                {positions[a], {0, 0}}, {positions[c], {1, 1}}, {positions[d], {0, 1}}});
        }
    }
    return vertices;
}

} // namespace wonky
