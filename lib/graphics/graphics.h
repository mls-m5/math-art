#pragma once

#include "point.h"
#include <SDL.h>
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

// Owns SDL video and an OpenGL 2.1 context. Use one Window at a time.
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
    void present();

private:
    SDL_Window *window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    bool vsync_ = false;
    Uint64 previous_ = 0;
    View view_{};
};

// Contours must be convex and ordered. No closing duplicate is required.
void fillFan(const std::vector<Point> &contour, Color color);
void outline(const std::vector<Point> &contour, Color color);
void line(Point start, Point end, Color color);
void disk(Point center, double radius, Color color);
void circle(Point center, double radius, Color color);

} // namespace graphics
