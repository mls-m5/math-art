#include "flower.h"

namespace blumme {
namespace {
Point mix(Point a, Point b, double t)
{
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

void appendCurve(std::vector<Point> &contour, Point start, Point control, Point end)
{
    constexpr int segments = 12;
    for (int i = 1; i <= segments; ++i) {
        const double t = static_cast<double>(i) / segments;
        // De Casteljau interpolation keeps the curve inside its control triangle.
        contour.push_back(mix(mix(start, control, t), mix(control, end, t), t));
    }
}
} // namespace

std::vector<Point> buildPetal(Point center, Point a, Point b)
{
    a = mix(center, a, 0.94);
    b = mix(center, b, 0.94);
    constexpr double rounding = 0.42;
    const Point aSide = mix(a, center, rounding);
    const Point aEdge = mix(a, b, rounding);
    const Point bEdge = mix(b, a, rounding);
    const Point bSide = mix(b, center, rounding);

    std::vector<Point> contour{center, aSide};
    appendCurve(contour, aSide, a, aEdge);
    contour.push_back(bEdge);
    appendCurve(contour, bEdge, b, bSide);
    return contour;
}

} // namespace blumme
