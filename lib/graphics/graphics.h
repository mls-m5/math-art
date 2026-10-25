#pragma once

#include "point.h"
#include <SDL.h>
#include <functional>
#include <cstdint>
#include <memory>
#include <vector>

namespace graphics {

struct Color
{
    float r, g, b;
};

struct View
{
    double halfWidth, halfHeight;
};

// Owns SDL video and a desktop OpenGL 2.1 or WebGL 1 context.
// Use one Window at a time.
// Construction throws std::runtime_error on failure; destruction cleans up.
class Window
{
public:
    explicit Window(const char *title, int width = 960, int height = 640);
    ~Window();
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    bool pollEvent(SDL_Event &event);
    double elapsed(); // Seconds since the previous call, clamped to 0.1.
    // Expand the view to fit minimum extents, preserving round circles.
    // Returns false while the drawable is zero-sized.
    bool beginFrame(View minimum, Color background = {0, 0, 0});
    View view() const { return view_; }
    Point drawableSize() const;
    void present();

private:
    SDL_Window *window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    bool vsync_ = false;
    Uint64 previous_ = 0;
    View view_{};
    struct Renderer;
    std::unique_ptr<Renderer> renderer_;
};

// Calls frame until it returns false. Browser callbacks retain their captures;
// capture application state by value (e.g. shared_ptr), never stack references.
void run(std::function<bool()> frame);
// Browser ?seed=... can make captures reproducible; native uses random_device.
std::uint32_t randomSeed();

// Contours must be convex and ordered. No closing duplicate is required.
void fillFan(const std::vector<Point> &contour, Color color);
// Draw independent triangles with local coordinates and an optional variant.
// Use with Shader (aPosition at 0, aCoordinate at 1, aVariant at 2,
// aParameters at 3).
void triangles(const std::vector<Vertex> &vertices);
// Additive blending is useful for luminous art. Reset to false after drawing.
void additiveBlending(bool enabled);
void outline(const std::vector<Point> &contour, Color color);
void line(Point start, Point end, Color color);
void disk(Point center, double radius, Color color);
void circle(Point center, double radius, Color color);

} // namespace graphics
