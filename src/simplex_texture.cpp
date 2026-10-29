#include "simplex_texture.h"

#include <glm/gtc/noise.hpp>
#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace noiseart {
SimplexTexture::SimplexTexture(std::uint32_t seed)
{
    std::mt19937 random(seed);
    std::uniform_real_distribution<float> offset(-64.0f, 64.0f);
    offset_ = {offset(random), offset(random), offset(random), offset(random)};
}

double SimplexTexture::sample(double u, double v, float scale) const
{
    if (!std::isfinite(u) || !std::isfinite(v) || !std::isfinite(scale) || scale <= 0)
        throw std::invalid_argument("Noise coordinates must be finite and scale positive");
    constexpr double tau = 6.283185307179586;
    const double x = tau * (u - std::floor(u));
    const double y = tau * (v - std::floor(v));
    const glm::vec4 torus(std::cos(x), std::sin(x), std::cos(y), std::sin(y));
    return std::clamp(0.5 + 0.5 * glm::simplex(torus * scale + offset_), 0.0, 1.0);
}

std::vector<std::uint8_t> SimplexTexture::rgba(int size, float scale) const
{
    if (size < 2 || size > 2048)
        throw std::invalid_argument("Noise texture size must be between 2 and 2048");
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(size) * size * 4);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const auto value = static_cast<std::uint8_t>(std::lround(
                sample(static_cast<double>(x) / size, static_cast<double>(y) / size, scale) * 255.0));
            const auto i = (static_cast<std::size_t>(y) * size + x) * 4;
            pixels[i] = pixels[i + 1] = pixels[i + 2] = value;
            pixels[i + 3] = 255;
        }
    }
    return pixels;
}
} // namespace noiseart
