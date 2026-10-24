#pragma once

#include "graphics/point.h"
#include <vector>

namespace blumme {

using Point = graphics::Point;

struct Bounds
{
    double halfWidth = 1.4;
    double halfHeight = 0.9;
};

struct Cell
{
    // The generating site, not necessarily the polygon's geometric centroid.
    Point center;
    // Counterclockwise vertices, clipped to the rectangular domain.
    std::vector<Point> polygon;
};

// Sites must be distinct and inside bounds. Output preserves input order.
std::vector<Cell> buildVoronoi(const std::vector<Point> &sites, Bounds bounds);

// A small, overdamped repulsion step. Supply a small fixed dt (e.g. 1/120 s).
// Optional persistent strengths are positive, finite values in site order.
// Stronger sites emit stronger, longer-range repulsion, tending to claim larger
// cells. Empty strengths preserve uniform repulsion (1.0 for every site).
void relaxSites(std::vector<Point> &sites, Bounds bounds, double dt,
                const std::vector<double> &strengths = {});

} // namespace blumme
