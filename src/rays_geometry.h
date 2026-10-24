#pragma once

#include "graphics/point.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace rays {
inline constexpr double tau = 6.28318530717958647692;

inline graphics::Point edgePoint(double angle, double halfWidth, double halfHeight)
{
    const double dx = std::cos(angle);
    const double dy = std::sin(angle);
    const double infinity = std::numeric_limits<double>::infinity();
    const double tx = std::abs(dx) > 1e-12 ? halfWidth / std::abs(dx) : infinity;
    const double ty = std::abs(dy) > 1e-12 ? halfHeight / std::abs(dy) : infinity;
    const double distance = std::min(tx, ty);
    return {distance * dx, distance * dy};
}

// A triangle fan clipped to the visible rectangle. Include rectangle corners
// between the rays: simply joining their endpoints would leave corner holes.
// start is in [0, tau); end > start and may exceed tau for the wrapping wedge.
inline std::vector<graphics::Point> wedgeContour(double start, double end,
                                                double halfWidth, double halfHeight)
{
    std::vector<double> angles{start, end};
    for (auto corner : std::vector<graphics::Point>{{halfWidth, halfHeight},
                                                   {-halfWidth, halfHeight},
                                                   {-halfWidth, -halfHeight},
                                                   {halfWidth, -halfHeight}}) {
        double angle = std::atan2(corner.y, corner.x);
        if (angle < 0.0)
            angle += tau;
        if (angle <= start)
            angle += tau;
        if (angle < end)
            angles.push_back(angle);
    }
    std::sort(angles.begin(), angles.end());
    std::vector<graphics::Point> contour{{0, 0}};
    for (double angle : angles)
        contour.push_back(edgePoint(angle, halfWidth, halfHeight));
    return contour;
}
} // namespace rays
