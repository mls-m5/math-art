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
    float variant = 0.0f; // Optional per-cell shader value; constant across a cell.
    Point parameters; // Optional shader parameters (attribute 3).
};
} // namespace graphics
