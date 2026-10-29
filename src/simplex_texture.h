#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace noiseart {
// Seamless 2D noise via a torus embedded in four-dimensional simplex noise.
// u/v repeat every 1.0; suitable for linearly filtered repeating GPU textures.
class SimplexTexture
{
public:
    explicit SimplexTexture(std::uint32_t seed);
    double sample(double u, double v, float scale = 1.6f) const;
    std::vector<std::uint8_t> rgba(int size = 256, float scale = 1.6f) const;
private:
    glm::vec4 offset_;
};
} // namespace noiseart
