#pragma once

#include <cstdint>
#include <vector>

namespace graphics {
// RGBA8 texture for the current Window; destroy before the Window. Repeating
// textures require power-of-two dimensions for WebGL 1 compatibility.
class Texture2D
{
public:
    Texture2D(int width, int height, const std::vector<std::uint8_t> &rgba, bool repeat = true);
    ~Texture2D();
    Texture2D(const Texture2D &) = delete;
    Texture2D &operator=(const Texture2D &) = delete;
    void bind(unsigned int unit) const;
private:
    unsigned int id_ = 0;
};
} // namespace graphics
