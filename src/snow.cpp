#include "snow.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace snow {
namespace {
constexpr double tau = 6.283185307179586;
graphics::Point rotate(graphics::Point p, double angle)
{
    return {p.x * std::cos(angle) - p.y * std::sin(angle),
            p.x * std::sin(angle) + p.y * std::cos(angle)};
}
} // namespace

Flake generateFlake(std::uint32_t seed)
{
    std::mt19937 random(seed);
    std::uniform_real_distribution<double> unit(0, 1);
    const int style = static_cast<int>(random() % 3);
    const int levels = 3 + static_cast<int>(random() % 3);
    const double width = 0.013 + unit(random) * 0.009;
    Flake arm{{{0, 0}, {1, 0}, width}};
    for (int i = 0; i < levels; ++i) {
        const double t = 0.22 + (i + 0.25 * unit(random)) * 0.64 / levels;
        const double length = std::min(0.18 + 0.20 * unit(random), (1.0 - t) * 0.75);
        const double angle = 0.70 + unit(random) * 0.30;
        for (double side : {-1.0, 1.0}) {
            const graphics::Point end{t + length * std::cos(angle), side * length * std::sin(angle)};
            arm.push_back({{t, 0}, end, width * 0.72});
            // Feathery branches on some variants, simpler needles on others.
            if (style == 0 && i < levels - 1) {
                const graphics::Point middle{t + length * 0.55 * std::cos(angle),
                                            side * length * 0.55 * std::sin(angle)};
                arm.push_back({middle, {middle.x + length * 0.32,
                                       middle.y + side * length * 0.10}, width * 0.5});
            }
        }
    }
    if (style != 1) {
        // Six rotated copies close into a small hexagonal crystal at the core.
        const double radius = 0.12 + unit(random) * 0.09;
        arm.push_back({{radius, 0}, {radius * 0.5, radius * std::sqrt(3.0) * 0.5}, width * 0.8});
    }
    Flake flake;
    flake.reserve(arm.size() * 6);
    for (int i = 0; i < 6; ++i)
        for (const auto &branch : arm)
            flake.push_back({rotate(branch.start, i * tau / 6), rotate(branch.end, i * tau / 6), branch.width});
    return flake;
}

Snowfall::Snowfall(std::uint32_t seed, std::size_t variants, std::size_t count)
    : random_(seed), variants_(variants), particles_(count)
{
    if (variants == 0) throw std::invalid_argument("Snowfall needs at least one variant");
    for (auto &particle : particles_) spawn(particle, true);
}

void Snowfall::spawn(Particle &p, bool initial)
{
    std::uniform_real_distribution<double> unit(0, 1);
    p = {unit(random_), initial ? unit(random_) * 1.2 - 0.1 : 1.10 + unit(random_) * 0.1,
         unit(random_), unit(random_) * tau, (unit(random_) - 0.5) * 0.7,
         unit(random_) * tau, random_() % variants_};
}

void Snowfall::update(double dt)
{
    if (!std::isfinite(dt) || dt < 0) throw std::invalid_argument("Invalid snowfall time step");
    time_ += dt;
    for (auto &p : particles_) {
        p.y -= (0.035 + 0.075 * p.depth) * dt;
        p.x += (0.018 + 0.018 * std::sin(time_ * 0.6 + p.phase)) * dt;
        p.x -= std::floor(p.x); // Seamless horizontal wrapping, including long steps.
        p.angle = std::remainder(p.angle + p.spin * dt, tau);
        if (p.y < -0.12) spawn(p, false);
    }
}

void appendFlake(std::vector<graphics::Vertex> &mesh, const Flake &flake,
                 graphics::Point center, double radius, double angle,
                 double brightness, int width, int height)
{
    if (width <= 0 || height <= 0) return;
    for (const auto &branch : flake) {
        const auto a = rotate(branch.start, angle), b = rotate(branch.end, angle);
        const graphics::Point delta{(b.x - a.x) * radius, (b.y - a.y) * radius};
        const double length = std::hypot(delta.x, delta.y);
        const graphics::Point direction = length > 1e-8 ? graphics::Point{delta.x/length, delta.y/length}
                                                       : graphics::Point{1,0};
        const graphics::Point midpoint{center.x + (a.x + b.x) * radius * 0.5,
                                       center.y + (a.y + b.y) * radius * 0.5};
        const double stroke = std::max(0.55, branch.width * radius);
        const double half = length * 0.5, pad = stroke + 1.0;
        const graphics::Point corners[]{{-half-pad,-pad},{half+pad,-pad},{half+pad,pad},{-half-pad,pad}};
        for (int index : {0,1,2,0,2,3}) {
            const auto local = corners[index];
            const double x = midpoint.x + direction.x * local.x - direction.y * local.y;
            const double y = midpoint.y + direction.y * local.x + direction.x * local.y;
            mesh.push_back({{2*x/width-1, 2*y/height-1}, local,
                            static_cast<float>(brightness), {half, stroke}});
        }
    }
}
} // namespace snow
