#include "shader.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace graphics {
namespace {
template <typename T> T load(const char *name)
{
    const auto function = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
    if (!function)
        throw std::runtime_error(std::string("Missing OpenGL function: ") + name);
    return function;
}
} // namespace

struct Shader::Impl
{
    PFNGLCREATESHADERPROC createShader = load<PFNGLCREATESHADERPROC>("glCreateShader");
    PFNGLSHADERSOURCEPROC shaderSource = load<PFNGLSHADERSOURCEPROC>("glShaderSource");
    PFNGLCOMPILESHADERPROC compileShader = load<PFNGLCOMPILESHADERPROC>("glCompileShader");
    PFNGLGETSHADERIVPROC getShaderiv = load<PFNGLGETSHADERIVPROC>("glGetShaderiv");
    PFNGLGETSHADERINFOLOGPROC shaderLog = load<PFNGLGETSHADERINFOLOGPROC>("glGetShaderInfoLog");
    PFNGLDELETESHADERPROC deleteShader = load<PFNGLDELETESHADERPROC>("glDeleteShader");
    PFNGLCREATEPROGRAMPROC createProgram = load<PFNGLCREATEPROGRAMPROC>("glCreateProgram");
    PFNGLATTACHSHADERPROC attachShader = load<PFNGLATTACHSHADERPROC>("glAttachShader");
    PFNGLBINDATTRIBLOCATIONPROC bindAttribute = load<PFNGLBINDATTRIBLOCATIONPROC>("glBindAttribLocation");
    PFNGLLINKPROGRAMPROC linkProgram = load<PFNGLLINKPROGRAMPROC>("glLinkProgram");
    PFNGLGETPROGRAMIVPROC getProgramiv = load<PFNGLGETPROGRAMIVPROC>("glGetProgramiv");
    PFNGLGETPROGRAMINFOLOGPROC programLog = load<PFNGLGETPROGRAMINFOLOGPROC>("glGetProgramInfoLog");
    PFNGLDELETEPROGRAMPROC deleteProgram = load<PFNGLDELETEPROGRAMPROC>("glDeleteProgram");
    PFNGLUSEPROGRAMPROC useProgram = load<PFNGLUSEPROGRAMPROC>("glUseProgram");
    PFNGLGETUNIFORMLOCATIONPROC uniformLocation = load<PFNGLGETUNIFORMLOCATIONPROC>("glGetUniformLocation");
    PFNGLUNIFORM1FPROC uniform1f = load<PFNGLUNIFORM1FPROC>("glUniform1f");
    PFNGLUNIFORM2FPROC uniform2f = load<PFNGLUNIFORM2FPROC>("glUniform2f");
    GLuint program = 0;

    ~Impl()
    {
        if (program)
            deleteProgram(program);
    }

    GLuint compile(GLenum type, const char *source)
    {
        const GLuint shader = createShader(type);
        shaderSource(shader, 1, &source, nullptr);
        compileShader(shader);
        GLint success = 0;
        getShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLint size = 0;
            getShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
            std::vector<char> log(size > 0 ? size : 1, '\0');
            shaderLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
            deleteShader(shader);
            throw std::runtime_error(std::string("Shader compilation failed: ") + log.data());
        }
        return shader;
    }
};

Shader::Shader(const char *vertexSource, const char *fragmentSource) : impl_(std::make_unique<Impl>())
{
    const GLuint vertex = impl_->compile(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = 0;
    try {
        fragment = impl_->compile(GL_FRAGMENT_SHADER, fragmentSource);
        impl_->program = impl_->createProgram();
        impl_->attachShader(impl_->program, vertex);
        impl_->attachShader(impl_->program, fragment);
        // Attribute 0 is also supplied by the current compatibility renderer's
        // glVertex calls. A future WebGL backend can feed it from a vertex buffer.
        impl_->bindAttribute(impl_->program, 0, "aPosition");
        impl_->bindAttribute(impl_->program, 1, "aCoordinate");
        impl_->bindAttribute(impl_->program, 2, "aVariant");
        impl_->linkProgram(impl_->program);
        GLint success = 0;
        impl_->getProgramiv(impl_->program, GL_LINK_STATUS, &success);
        if (!success) {
            GLint size = 0;
            impl_->getProgramiv(impl_->program, GL_INFO_LOG_LENGTH, &size);
            std::vector<char> log(size > 0 ? size : 1, '\0');
            impl_->programLog(impl_->program, static_cast<GLsizei>(log.size()), nullptr, log.data());
            throw std::runtime_error(std::string("Shader linking failed: ") + log.data());
        }
    } catch (...) {
        impl_->deleteShader(vertex);
        if (fragment)
            impl_->deleteShader(fragment);
        throw;
    }
    impl_->deleteShader(vertex);
    impl_->deleteShader(fragment);
}

Shader::~Shader() = default;
void Shader::use() const { impl_->useProgram(impl_->program); }
void Shader::stop() const { impl_->useProgram(0); }
void Shader::setFloat(const char *name, float value) const
{
    impl_->uniform1f(impl_->uniformLocation(impl_->program, name), value);
}
void Shader::setVec2(const char *name, float x, float y) const
{
    impl_->uniform2f(impl_->uniformLocation(impl_->program, name), x, y);
}
} // namespace graphics
