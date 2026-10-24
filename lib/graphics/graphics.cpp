#include "graphics.h"

#include <SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace graphics {
namespace {
constexpr double pi = 3.14159265358979323846;
void setColor(Color color) { glColor3f(color.r, color.g, color.b); }
void circleVertices(Point center, double radius)
{
    for (int i = 0; i <= 32; ++i) {
        const double angle = 2.0 * pi * i / 32;
        glVertex2d(center.x + radius * std::cos(angle), center.y + radius * std::sin(angle));
    }
}
} // namespace

Window::Window(const char *title, int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error(std::string("SDL initialization failed: ") + SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
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
        // Some drivers/display formats cannot provide multisampling. Retry
        // without it rather than making antialiasing a startup requirement.
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
    glEnable(GL_MULTISAMPLE);
    vsync_ = SDL_GL_SetSwapInterval(1) == 0;
    previous_ = SDL_GetPerformanceCounter();
}

Window::~Window()
{
    SDL_GL_DeleteContext(context_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool Window::pollEvent(SDL_Event &event) { return SDL_PollEvent(&event) != 0; }

double Window::elapsed()
{
    const Uint64 now = SDL_GetPerformanceCounter();
    const double seconds = static_cast<double>(now - previous_) / SDL_GetPerformanceFrequency();
    previous_ = now;
    return std::min(seconds, 0.1);
}

bool Window::beginFrame(View minimum, Color background)
{
    int width, height;
    SDL_GL_GetDrawableSize(window_, &width, &height);
    if (width <= 0 || height <= 0) {
        SDL_Delay(10);
        return false;
    }
    const double aspect = static_cast<double>(width) / height;
    view_.halfHeight = std::max(minimum.halfHeight, minimum.halfWidth / aspect);
    view_.halfWidth = view_.halfHeight * aspect;
    glViewport(0, 0, width, height);
    glClearColor(background.r, background.g, background.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-view_.halfWidth, view_.halfWidth, -view_.halfHeight, view_.halfHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    return true;
}

void Window::present()
{
    SDL_GL_SwapWindow(window_);
    if (!vsync_)
        SDL_Delay(16);
}

void fillFan(const std::vector<Point> &contour, Color color)
{
    setColor(color);
    glBegin(GL_TRIANGLE_FAN);
    for (auto p : contour)
        glVertex2d(p.x, p.y);
    glEnd();
}

void triangles(const std::vector<Vertex> &vertices)
{
    const auto coordinate = reinterpret_cast<PFNGLVERTEXATTRIB2FPROC>(
        SDL_GL_GetProcAddress("glVertexAttrib2f"));
    const auto variant = reinterpret_cast<PFNGLVERTEXATTRIB1FPROC>(
        SDL_GL_GetProcAddress("glVertexAttrib1f"));
    if (!coordinate || !variant)
        throw std::runtime_error("Missing OpenGL vertex attribute functions");
    glBegin(GL_TRIANGLES);
    for (const auto &vertex : vertices) {
        coordinate(1, static_cast<float>(vertex.coordinate.x), static_cast<float>(vertex.coordinate.y));
        variant(2, vertex.variant);
        glVertex2d(vertex.position.x, vertex.position.y);
    }
    glEnd();
}

void outline(const std::vector<Point> &contour, Color color)
{
    setColor(color);
    glBegin(GL_LINE_LOOP);
    for (auto p : contour)
        glVertex2d(p.x, p.y);
    glEnd();
}

void line(Point start, Point end, Color color)
{
    setColor(color);
    glBegin(GL_LINES);
    glVertex2d(start.x, start.y);
    glVertex2d(end.x, end.y);
    glEnd();
}

void disk(Point center, double radius, Color color)
{
    setColor(color);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(center.x, center.y);
    circleVertices(center, radius);
    glEnd();
}

void circle(Point center, double radius, Color color)
{
    setColor(color);
    glBegin(GL_LINE_LOOP);
    circleVertices(center, radius);
    glEnd();
}
} // namespace graphics
