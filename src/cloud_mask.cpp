#include "cloud_mask.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace noiseart {
namespace {
double wrapped(double x, double period)
{
    return x - std::floor(x / period + 0.5) * period;
}
} // namespace

CloudMask::CloudMask(std::uint32_t seed)
{
    std::mt19937 random(seed);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::uniform_int_distribution<int> groupCount(8, 11), lobeCount(4, 9);
    std::vector<CloudCircle> roots;
    for (int group = 0, count = groupCount(random); group < count; ++group) {
        CloudCircle root{};
        // Random centers with separation, rather than rows or equal spacing.
        for (int attempt = 0; attempt < 500; ++attempt) {
            const double size = unit(random);
            root = {unit(random) * periodX, unit(random) * periodY, 0.10 + size * size * 0.14};
            bool clear = true;
            for (const auto &other : roots) {
                const double dx = wrapped(root.x - other.x, periodX);
                const double dy = wrapped(root.y - other.y, periodY);
                if (std::hypot(dx, dy) < 0.35 + 0.45 * (root.radius + other.radius))
                    clear = false;
            }
            if (clear) break;
        }
        roots.push_back(root);
        circles_.push_back(root);
        const double tilt = (unit(random) - 0.5) * 1.2;
        const int lobes = lobeCount(random);
        const double longAxis = root.radius * std::min(1.4 + unit(random) * 1.2, (lobes - 2) * 0.65);
        const double bend = unit(random) * 6.283185307179586;
        // Spread overlapping lobes along a bent, tilted spine instead of
        // packing them into a round envelope. Larger groups become long banks.
        for (int i = 1; i < lobes; ++i) {
            const double t = -1.0 + 2.0 * (i - 1) / (lobes - 2);
            const double along = t * longAxis + (unit(random) - 0.5) * root.radius * 0.15;
            const double across = root.radius *
                (0.30 * std::sin(t * 2.0 + bend) + (unit(random) - 0.5) * 0.25);
            const double dx = along * std::cos(tilt) - across * std::sin(tilt);
            const double dy = along * std::sin(tilt) + across * std::cos(tilt);
            const double radius = root.radius * (0.75 + unit(random) * 0.35) * (1.0 - 0.15 * std::abs(t));
            circles_.push_back({root.x + dx, root.y + dy, std::clamp(radius, 0.06, 0.25)});
        }
    }
}

double CloudMask::sample(double x, double y) const
{
    double field = 0.0;
    for (const auto &circle : circles_) {
        const double dx = wrapped(x - circle.x, periodX);
        const double dy = wrapped(y - circle.y, periodY);
        const double distance = (dx * dx + dy * dy) / (circle.radius * circle.radius);
        const double t = std::clamp((distance - 0.25) / 1.35, 0.0, 1.0);
        field += 1.0 - t * t * (3.0 - 2.0 * t);
    }
    return field;
}

std::vector<std::uint8_t> CloudMask::rgba(int size) const
{
    if (size < 2 || size > 2048)
        throw std::invalid_argument("Cloud mask size must be between 2 and 2048");
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(size) * size * 4);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const auto value = static_cast<std::uint8_t>(std::lround(255.0 * std::clamp(
                sample(x * periodX / size, y * periodY / size), 0.0, 1.0)));
            const auto index = (static_cast<std::size_t>(y) * size + x) * 4;
            pixels[index] = pixels[index + 1] = pixels[index + 2] = value;
            pixels[index + 3] = 255;
        }
    }
    return pixels;
}
} // namespace noiseart
