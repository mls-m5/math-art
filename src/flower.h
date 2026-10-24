#pragma once

#include "voronoi.h"

namespace blumme {

// Turn a counterclockwise fan triangle (center, a, b) into a filled petal.
// The center stays sharp. Outer corners are rounded inward with quadratic
// Beziers, and the outer edge is inset 6% to leave a gap between flowers.
// Returns a counterclockwise contour starting at center, without a closing copy.
std::vector<Point> buildPetal(Point center, Point a, Point b);

} // namespace blumme
