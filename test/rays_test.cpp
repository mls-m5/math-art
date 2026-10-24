#include "rays_geometry.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

void check(double width, double height)
{
    // Nonuniform spacing and a wrapping final wedge exercise corner insertion.
    const std::vector<double> angles{0.1, 0.2, 0.8, 1.9, 2.0, 3.0, 4.5, 5.9};
    double area = 0.0;
    for (std::size_t i = 0; i < angles.size(); ++i) {
        const double end = i + 1 < angles.size() ? angles[i + 1] : angles[0] + rays::tau;
        const auto contour = rays::wedgeContour(angles[i], end, width, height);
        for (std::size_t j = 1; j < contour.size(); ++j) {
            const auto p = contour[j];
            if (!std::isfinite(p.x) || !std::isfinite(p.y) ||
                std::abs(p.x) > width + 1e-9 || std::abs(p.y) > height + 1e-9 ||
                (std::abs(std::abs(p.x) - width) > 1e-9 &&
                 std::abs(std::abs(p.y) - height) > 1e-9)) {
                std::cerr << "Wedge vertex is not on a window edge\n";
                std::exit(1);
            }
            if (j + 1 < contour.size()) {
                const auto q = contour[j + 1];
                const double triangleArea = 0.5 * (p.x * q.y - q.x * p.y);
                if (triangleArea < -1e-9) {
                    std::cerr << "Wedge winding is incorrect\n";
                    std::exit(1);
                }
                area += triangleArea;
            }
        }
    }
    if (std::abs(area - 4.0 * width * height) > 1e-8) {
        std::cerr << "Wedges do not cover the visible rectangle\n";
        std::exit(1);
    }
}

int main()
{
    check(1.4, 0.9);
    check(0.9, 1.4);
    check(4.0, 0.5);
    check(1.0, 1.0);
    std::cout << "Ray wedge tests passed\n";
}
