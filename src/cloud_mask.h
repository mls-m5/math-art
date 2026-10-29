#pragma once

#include <cstdint>
#include <vector>

namespace noiseart {
struct CloudCircle { double x, y, radius; };

// Randomly scattered groups of differently sized, overlapping soft circles.
// Baked once to a repeating mask texture; no per-circle shader loops required.
class CloudMask
{
public:
    static constexpr double periodX = 2.8;
    static constexpr double periodY = 1.9;
    explicit CloudMask(std::uint32_t seed);
    const std::vector<CloudCircle> &circles() const { return circles_; }
    double sample(double x, double y) const;
    std::vector<std::uint8_t> rgba(int size = 512) const;
private:
    std::vector<CloudCircle> circles_;
};
} // namespace noiseart
