#pragma once

#include <memory>

namespace graphics {

// GLSL program for the current Window. Destroy before destroying the Window.
// Compilation/link errors throw std::runtime_error with the driver's log.
class Shader
{
public:
    Shader(const char *vertexSource, const char *fragmentSource);
    ~Shader();
    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

    void use() const;
    void setFloat(const char *name, float value) const; // Call after use().
    void setInt(const char *name, int value) const; // Includes texture sampler units.
    void setVec2(const char *name, float x, float y) const;
    void setVec3(const char *name, float x, float y, float z) const;
    static const Shader *active();
    void stop() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace graphics
