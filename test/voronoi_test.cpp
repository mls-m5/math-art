#include "flower.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>

namespace {
void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

double squaredDistance(blumme::Point a, blumme::Point b)
{
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

void check(const std::vector<blumme::Point> &sites, blumme::Bounds bounds)
{
    const auto cells = blumme::buildVoronoi(sites, bounds);
    require(cells.size() == sites.size(), "Cell count differs from site count");
    double totalArea = 0.0;
    for (std::size_t i = 0; i < cells.size(); ++i) {
        const auto &cell = cells[i];
        require(squaredDistance(cell.center, sites[i]) == 0.0, "Site order changed");
        require(cell.polygon.size() >= 3, "Degenerate polygon");
        double twiceArea = 0.0;
        for (std::size_t k = 0; k < cell.polygon.size(); ++k) {
            const auto a = cell.polygon[k];
            const auto b = cell.polygon[(k + 1) % cell.polygon.size()];
            require(std::isfinite(a.x) && std::isfinite(a.y), "Nonfinite vertex");
            require(std::abs(a.x) <= bounds.halfWidth + 1e-9 &&
                        std::abs(a.y) <= bounds.halfHeight + 1e-9, "Vertex outside bounds");
            for (auto site : sites)
                require(squaredDistance(a, cell.center) <= squaredDistance(a, site) + 1e-9,
                        "Vertex is closer to another site");
            require((b.x - a.x) * (cell.center.y - a.y) -
                        (b.y - a.y) * (cell.center.x - a.x) >= -1e-9,
                    "Center is outside polygon");
            twiceArea += a.x * b.y - b.x * a.y;

            const auto petal = blumme::buildPetal(cell.center, a, b);
            require(petal.size() > 4, "Petal curve has too few samples");
            require(squaredDistance(petal.front(), cell.center) == 0.0,
                    "Petal lost its center tip");
            const auto cross = [](blumme::Point p, blumme::Point q, blumme::Point r) {
                return (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x);
            };
            double petalArea = 0.0;
            for (std::size_t n = 0; n < petal.size(); ++n) {
                const auto p = petal[n];
                const auto q = petal[(n + 1) % petal.size()];
                require(std::isfinite(p.x) && std::isfinite(p.y), "Nonfinite petal vertex");
                require(cross(cell.center, a, p) >= -1e-9 &&
                            cross(a, b, p) >= -1e-9 && cross(b, cell.center, p) >= -1e-9,
                        "Petal escapes its fan triangle");
                petalArea += cross(cell.center, p, q);
            }
            const double triangleArea = cross(cell.center, a, b);
            require(petalArea >= -1e-9 && petalArea <= triangleArea + 1e-9,
                    "Invalid rounded petal area");
            if (triangleArea > 1e-9)
                require(petalArea > 0.0 && petalArea < triangleArea * 0.9,
                        "Petal did not leave background visible");
        }
        require(twiceArea > 0.0, "Polygon is not counterclockwise");
        totalArea += twiceArea * 0.5;
    }
    require(std::abs(totalArea - 4.0 * bounds.halfWidth * bounds.halfHeight) < 1e-8,
            "Cells do not cover domain area");
}
} // namespace

int main()
{
    const blumme::Bounds bounds;
    require(blumme::buildVoronoi({}, bounds).empty(), "Empty input failed");
    check({{0.0, 0.0}}, bounds);
    check({{-0.5, 0.0}, {0.5, 0.0}}, bounds);
    check({{-0.5, 0.0}, {0.0, 0.0}, {0.5, 0.0}}, bounds);
    check({{-0.5, -0.5}, {0.5, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, bounds);

    std::mt19937 random(42);
    std::uniform_real_distribution<double> x(-bounds.halfWidth, bounds.halfWidth);
    std::uniform_real_distribution<double> y(-bounds.halfHeight, bounds.halfHeight);
    std::vector<blumme::Point> sites;
    for (int i = 0; i < 80; ++i)
        sites.push_back({x(random), y(random)});
    check(sites, bounds);
    for (int i = 0; i < 1200; ++i)
        blumme::relaxSites(sites, bounds, 1.0 / 120.0);
    check(sites, bounds);

    std::vector<blumme::Point> coincident{{0.0, 0.0}, {0.0, 0.0}};
    blumme::relaxSites(coincident, bounds, 1.0 / 120.0);
    require(squaredDistance(coincident[0], coincident[1]) > 0.0,
            "Repulsion did not separate coincident sites");
    check(coincident, bounds);
    std::cout << "Voronoi geometry tests passed\n";
}
