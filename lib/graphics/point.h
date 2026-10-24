#pragma once

namespace graphics {
struct Point
{
    double x = 0.0;
    double y = 0.0;
};

struct Vertex
{
    Point position;
    Point coordinate; // Local shader coordinates (u, v).
};
} // namespace graphics
