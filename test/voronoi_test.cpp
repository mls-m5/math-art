#include "flower.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
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
            if (triangleArea > 1e-9) {
                for (std::size_t n = 1; n < petal.size(); ++n)
                    require(cross(cell.center, a, petal[n]) > 0.0 &&
                                cross(b, cell.center, petal[n]) > 0.0,
                            "Petal touches radial sides instead of leaving a gap");
            }
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
    // Omitting strengths must remain identical to explicit uniform strengths.
    auto uniform = sites;
    auto defaults = sites;
    blumme::relaxSites(uniform, bounds, 1.0 / 120.0, std::vector<double>(sites.size(), 1.0));
    blumme::relaxSites(defaults, bounds, 1.0 / 120.0);
    for (std::size_t i = 0; i < sites.size(); ++i)
        require(squaredDistance(uniform[i], defaults[i]) == 0.0, "Uniform behavior changed");

    std::vector<blumme::Point> pair{{-0.1, 0.0}, {0.1, 0.0}};
    blumme::relaxSites(pair, bounds, 1.0 / 120.0, {3.0, 0.3});
    require(pair[1].x - 0.1 > -0.1 - pair[0].x,
            "Stronger flower did not displace the weaker flower more");
    pair = {{-0.6, 0.0}, {0.6, 0.0}};
    blumme::relaxSites(pair, bounds, 1.0 / 120.0, {1.5, 0.2});
    require(pair[0].x == -0.6 && pair[1].x > 0.6,
            "Repulsion range did not depend on source strength");

    for (const auto &invalid : std::vector<std::vector<double>>{
             {1.0}, {1.0, 0.0}, {1.0, -1.0},
             {1.0, std::numeric_limits<double>::infinity()},
             {1.0, std::numeric_limits<double>::quiet_NaN()}}) {
        bool rejected = false;
        try { blumme::relaxSites(pair, bounds, 1.0 / 120.0, invalid); }
        catch (const std::invalid_argument &) { rejected = true; }
        require(rejected, "Invalid repulsion strengths were accepted");
    }

    // Mixed strengths should produce larger average cells for strong flowers,
    // without changing Voronoi containment, coverage, or petal validity.
    std::uniform_real_distribution<double> logStrength(std::log(0.4), std::log(2.5));
    std::vector<double> strengths;
    for (auto &site : sites) {
        site = {x(random), y(random)};
        strengths.push_back(std::exp(logStrength(random)));
    }
    const auto originalStrengths = strengths;
    for (int i = 0; i < 2400; ++i)
        blumme::relaxSites(sites, bounds, 1.0 / 120.0, strengths);
    require(strengths == originalStrengths, "Simulation modified persistent strengths");
    check(sites, bounds);
    double largeArea = 0.0, smallArea = 0.0;
    int largeCount = 0, smallCount = 0;
    const auto variedCells = blumme::buildVoronoi(sites, bounds);
    for (std::size_t i = 0; i < variedCells.size(); ++i) {
        const auto &polygon = variedCells[i].polygon;
        double area = 0.0;
        for (std::size_t j = 0; j < polygon.size(); ++j) {
            const auto a = polygon[j], b = polygon[(j + 1) % polygon.size()];
            area += 0.5 * (a.x * b.y - b.x * a.y);
        }
        if (strengths[i] > 1.5) { largeArea += area; ++largeCount; }
        if (strengths[i] < 0.7) { smallArea += area; ++smallCount; }
    }
    require(largeCount > 0 && smallCount > 0, "Missing strength groups in size test");
    require(largeArea / largeCount > smallArea / smallCount * 1.2,
            "Stronger flowers did not develop larger average cells");
    std::cout << "Voronoi geometry and variable repulsion tests passed\n";
}
