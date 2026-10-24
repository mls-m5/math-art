#pragma once

#include "graphics/point.h"
#include <cstdint>
#include <vector>

namespace wonky {

// Geometry stays normalized to the unit square, independent of SDL/OpenGL.
class Grid
{
public:
    Grid(int columns, int rows, std::uint32_t seed);
    std::vector<graphics::Point> points(double time) const;
    std::vector<graphics::Vertex> mesh(double time) const;
    int columns() const { return columns_; }
    int rows() const { return rows_; }

private:
    struct Wave
    {
        double amplitude1, amplitude2;
        double frequency1, frequency2;
        double phase1, phase2;
        double at(double time) const;
    };
    struct Motion { Wave x, y; };
    int columns_, rows_;
    std::vector<Motion> motions_;
};

} // namespace wonky
