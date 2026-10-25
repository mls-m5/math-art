#include "graphics.h"
#include "shader.h"

#include <SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#ifdef __EMSCRIPTEN__
extern "C" EMSCRIPTEN_KEEPALIVE void art_key(int key)
{
    SDL_Event event{};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = static_cast<SDL_Keycode>(key);
    SDL_PushEvent(&event);
}
#endif

namespace graphics {
std::uint32_t randomSeed()
{
#ifdef __EMSCRIPTEN__
    if (EM_ASM_INT({ return Number.isInteger(window.artSeed); }))
        return static_cast<std::uint32_t>(EM_ASM_INT({ return window.artSeed | 0; }));
#endif
    return static_cast<std::uint32_t>(std::random_device{}());
}

namespace {
constexpr double pi = 3.14159265358979323846;
View currentView{};
Shader *defaultShader = nullptr;
GLuint vertexBuffer = 0;
template <typename T> T load(const char *name)
{
    const auto function = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
    if (!function)
        throw std::runtime_error(std::string("Missing OpenGL function: ") + name);
    return function;
}

const char *vertexSource = R"GLSL(
attribute vec2 aPosition;
uniform vec2 uView;
void main() { gl_Position = vec4(aPosition / uView, 0.0, 1.0); }
)GLSL";
const char *fragmentSource = R"GLSL(
#ifdef GL_ES
precision mediump float;
#endif
uniform vec3 uColor;
void main() { gl_FragColor = vec4(uColor, 1.0); }
)GLSL";
} // namespace

struct Window::Renderer
{
    Shader shader{vertexSource, fragmentSource};
    PFNGLGENBUFFERSPROC genBuffers = load<PFNGLGENBUFFERSPROC>("glGenBuffers");
    PFNGLDELETEBUFFERSPROC deleteBuffers = load<PFNGLDELETEBUFFERSPROC>("glDeleteBuffers");
    Renderer()
    {
        genBuffers(1, &vertexBuffer);
        defaultShader = &shader;
    }
    ~Renderer()
    {
        deleteBuffers(1, &vertexBuffer);
        vertexBuffer = 0;
        defaultShader = nullptr;
    }
};

Window::Window(const char *title, int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error(std::string("SDL initialization failed: ") + SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    const auto create = [&] {
        window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                                     SDL_WINDOW_ALLOW_HIGHDPI);
        if (window_)
            context_ = SDL_GL_CreateContext(window_);
        return window_ && context_;
    };
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    if (!create()) {
        if (window_)
            SDL_DestroyWindow(window_);
        window_ = nullptr;
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
        create();
    }
    if (!window_ || !context_) {
        const std::string error = std::string("Graphics initialization failed: ") + SDL_GetError();
        if (window_)
            SDL_DestroyWindow(window_);
        SDL_Quit();
        throw std::runtime_error(error);
    }
    try {
        renderer_ = std::make_unique<Renderer>();
    } catch (...) {
        SDL_GL_DeleteContext(context_);
        SDL_DestroyWindow(window_);
        SDL_Quit();
        throw;
    }
#ifndef __EMSCRIPTEN__
    glEnable(GL_MULTISAMPLE);
#endif
    vsync_ = SDL_GL_SetSwapInterval(1) == 0;
    previous_ = SDL_GetPerformanceCounter();
}

Window::~Window()
{
    renderer_.reset();
    SDL_GL_DeleteContext(context_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool Window::pollEvent(SDL_Event &event)
{
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN && !event.key.repeat &&
            event.key.keysym.sym == SDLK_f &&
            !(event.key.keysym.mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI))) {
#ifndef __EMSCRIPTEN__
            const bool fullscreen = SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN;
            if (SDL_SetWindowFullscreen(window_, fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
                std::cerr << "Fullscreen unavailable: " << SDL_GetError() << '\n';
#endif
            // Browser F is handled synchronously by the shell's user gesture.
            continue;
        }
#ifndef __EMSCRIPTEN__
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE &&
            (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN)) {
            SDL_SetWindowFullscreen(window_, 0);
            continue;
        }
#endif
#ifdef __EMSCRIPTEN__
        // Escape belongs to browser fullscreen, not application shutdown.
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            continue;
#endif
        return true;
    }
    return false;
}

double Window::elapsed()
{
    const Uint64 now = SDL_GetPerformanceCounter();
    const double seconds = static_cast<double>(now - previous_) / SDL_GetPerformanceFrequency();
    previous_ = now;
#ifdef __EMSCRIPTEN__
    if (EM_ASM_INT({ return !!window.artCapture; }))
        return 1.0 / 60.0;
#endif
    return std::min(seconds, 0.1);
}

bool Window::beginFrame(View minimum, Color background)
{
#ifdef __EMSCRIPTEN__
    double cssWidth = 0, cssHeight = 0;
    emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight);
    const double ratio = std::min(emscripten_get_device_pixel_ratio(), 2.0);
    const int targetWidth = std::max(1, static_cast<int>(cssWidth * ratio));
    const int targetHeight = std::max(1, static_cast<int>(cssHeight * ratio));
    int oldWidth, oldHeight;
    SDL_GetWindowSize(window_, &oldWidth, &oldHeight);
    if (oldWidth != targetWidth || oldHeight != targetHeight)
        SDL_SetWindowSize(window_, targetWidth, targetHeight);
#endif
    int width, height;
    SDL_GL_GetDrawableSize(window_, &width, &height);
    if (width <= 0 || height <= 0) {
#ifndef __EMSCRIPTEN__
        SDL_Delay(10);
#endif
        return false;
    }
    const double aspect = static_cast<double>(width) / height;
    view_.halfHeight = std::max(minimum.halfHeight, minimum.halfWidth / aspect);
    view_.halfWidth = view_.halfHeight * aspect;
    currentView = view_;
    glViewport(0, 0, width, height);
    glClearColor(background.r, background.g, background.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    return true;
}

Point Window::drawableSize() const
{
    int width, height;
    SDL_GL_GetDrawableSize(window_, &width, &height);
    return {static_cast<double>(width), static_cast<double>(height)};
}

void Window::present()
{
    SDL_GL_SwapWindow(window_);
#ifdef __EMSCRIPTEN__
    EM_ASM({
        window.artReady = true;
        window.artFrames = (window.artFrames || 0) + 1;
        if (window.artCapture && window.artFrames === 180)
            Module._art_key(32);
    });
#else
    if (!vsync_)
        SDL_Delay(16);
#endif
}

void run(std::function<bool()> frame)
{
#ifdef __EMSCRIPTEN__
    auto *callback = new std::function<bool()>(std::move(frame));
    emscripten_set_main_loop_arg([](void *pointer) {
        auto *frame = static_cast<std::function<bool()> *>(pointer);
        bool running = false;
        try {
            running = (*frame)();
        } catch (const std::exception &error) {
            std::cerr << error.what() << '\n';
            EM_ASM({ window.artError = true; });
        }
        if (!running) {
            emscripten_cancel_main_loop();
            delete frame;
        }
    }, callback, 0, false);
#else
    while (frame()) {}
#endif
}

namespace {
void draw(const std::vector<Vertex> &vertices, GLenum mode, Color color)
{
    if (vertices.empty())
        return;
    const auto bindBuffer = load<PFNGLBINDBUFFERPROC>("glBindBuffer");
    const auto bufferData = load<PFNGLBUFFERDATAPROC>("glBufferData");
    const auto enable = load<PFNGLENABLEVERTEXATTRIBARRAYPROC>("glEnableVertexAttribArray");
    const auto attribute = load<PFNGLVERTEXATTRIBPOINTERPROC>("glVertexAttribPointer");
    const Shader *shader = Shader::active();
    const bool useDefault = !shader;
    if (useDefault) {
        shader = defaultShader;
        shader->use();
    }
    shader->setVec2("uView", static_cast<float>(currentView.halfWidth),
                    static_cast<float>(currentView.halfHeight));
    shader->setVec3("uColor", color.r, color.g, color.b);
    std::vector<float> data;
    data.reserve(vertices.size() * 7);
    for (const auto &vertex : vertices) {
        data.insert(data.end(), {static_cast<float>(vertex.position.x), static_cast<float>(vertex.position.y),
            static_cast<float>(vertex.coordinate.x), static_cast<float>(vertex.coordinate.y), vertex.variant,
            static_cast<float>(vertex.parameters.x), static_cast<float>(vertex.parameters.y)});
    }
    bindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    bufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(float)), data.data(), GL_STREAM_DRAW);
    constexpr GLuint offsets[]{0, 2, 4, 5};
    for (GLuint i = 0; i < 4; ++i) {
        enable(i);
        attribute(i, i == 2 ? 1 : 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                  reinterpret_cast<const void *>(static_cast<std::uintptr_t>(offsets[i] * sizeof(float))));
    }
    glDrawArrays(mode, 0, static_cast<GLsizei>(vertices.size()));
    if (useDefault)
        shader->stop();
}

std::vector<Vertex> contourVertices(const std::vector<Point> &contour)
{
    std::vector<Vertex> vertices;
    vertices.reserve(contour.size());
    for (auto p : contour)
        vertices.push_back({p, {0, 0}});
    return vertices;
}
std::vector<Vertex> circleVertices(Point center, double radius)
{
    std::vector<Vertex> vertices;
    for (int i = 0; i <= 32; ++i) {
        const double angle = 2.0 * pi * i / 32;
        vertices.push_back({{center.x + radius * std::cos(angle), center.y + radius * std::sin(angle)}, {0, 0}});
    }
    return vertices;
}
} // namespace

void fillFan(const std::vector<Point> &contour, Color color)
{
    draw(contourVertices(contour), GL_TRIANGLE_FAN, color);
}
void triangles(const std::vector<Vertex> &vertices) { draw(vertices, GL_TRIANGLES, {1, 1, 1}); }
void additiveBlending(bool enabled)
{
    if (enabled) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
    } else {
        glDisable(GL_BLEND);
    }
}
void outline(const std::vector<Point> &contour, Color color) { draw(contourVertices(contour), GL_LINE_LOOP, color); }
void line(Point start, Point end, Color color) { draw({{start, {0, 0}}, {end, {0, 0}}}, GL_LINES, color); }
void disk(Point center, double radius, Color color)
{
    auto vertices = circleVertices(center, radius);
    vertices.insert(vertices.begin(), {center, {0, 0}});
    draw(vertices, GL_TRIANGLE_FAN, color);
}
void circle(Point center, double radius, Color color) { draw(circleVertices(center, radius), GL_LINE_LOOP, color); }
} // namespace graphics
