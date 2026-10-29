#include "texture.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <stdexcept>

namespace graphics {
Texture2D::Texture2D(int width, int height, const std::vector<std::uint8_t> &rgba, bool repeat)
{
    if (width <= 0 || height <= 0 || rgba.size() != static_cast<std::size_t>(width) * height * 4)
        throw std::invalid_argument("Texture size does not match RGBA data");
    if (repeat && ((width & (width - 1)) || (height & (height - 1))))
        throw std::invalid_argument("Repeating textures must have power-of-two dimensions");
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
}
Texture2D::~Texture2D() { glDeleteTextures(1, &id_); }
void Texture2D::bind(unsigned int unit) const
{
    const auto active = reinterpret_cast<PFNGLACTIVETEXTUREPROC>(SDL_GL_GetProcAddress("glActiveTexture"));
    if (!active)
        throw std::runtime_error("Missing OpenGL function: glActiveTexture");
    active(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}
} // namespace graphics
