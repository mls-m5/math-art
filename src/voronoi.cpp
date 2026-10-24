#include "voronoi.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace blumme {

std::vector<Cell> buildVoronoi(const std::vector<Point> &sites, Bounds bounds)
{
    std::vector<Cell> cells;
    cells.reserve(sites.size());
    for (std::size_t i = 0; i < sites.size(); ++i) {
        Cell cell{sites[i], {{-bounds.halfWidth, -bounds.halfHeight},
                             {bounds.halfWidth, -bounds.halfHeight},
                             {bounds.halfWidth, bounds.halfHeight},
                             {-bounds.halfWidth, bounds.halfHeight}}};
        for (std::size_t j = 0; j < sites.size() && !cell.polygon.empty(); ++j) {
            if (i == j)
                continue;
            const Point normal{sites[j].x - sites[i].x, sites[j].y - sites[i].y};
            const Point midpoint{(sites[i].x + sites[j].x) * 0.5,
                                 (sites[i].y + sites[j].y) * 0.5};
            const auto distance = [&](Point p) {
                return (p.x - midpoint.x) * normal.x + (p.y - midpoint.y) * normal.y;
            };

            // Clip against the half-plane on this site's side of the bisector.
            std::vector<Point> clipped;
            Point previous = cell.polygon.back();
            double previousDistance = distance(previous);
            for (Point current : cell.polygon) {
                const double currentDistance = distance(current);
                if ((previousDistance <= 0.0) != (currentDistance <= 0.0)) {
                    const double t = previousDistance / (previousDistance - currentDistance);
                    clipped.push_back({previous.x + t * (current.x - previous.x),
                                       previous.y + t * (current.y - previous.y)});
                }
                if (currentDistance <= 0.0)
                    clipped.push_back(current);
                previous = current;
                previousDistance = currentDistance;
            }
            cell.polygon = std::move(clipped);
        }
        cells.push_back(std::move(cell));
    }
    return cells;
}

void relaxSites(std::vector<Point> &sites, Bounds bounds, double dt)
{
    if (sites.empty())
        return;
    const double spacing = std::sqrt(4.0 * bounds.halfWidth * bounds.halfHeight / sites.size());
    const double radius = spacing * 1.25;
    const double margin = spacing * 0.35;
    std::vector<Point> forces(sites.size());
    for (std::size_t i = 0; i < sites.size(); ++i) {
        for (std::size_t j = i + 1; j < sites.size(); ++j) {
            double dx = sites[i].x - sites[j].x;
            double dy = sites[i].y - sites[j].y;
            double distance = std::hypot(dx, dy);
            if (distance >= radius)
                continue;
            if (distance < 1e-10) {
                // Deterministically separate even accidentally coincident sites.
                dx = 1.0;
                dy = 0.0;
                distance = 0.0;
            } else {
                dx /= distance;
                dy /= distance;
            }
            const double strength = (radius - distance) * 2.0;
            forces[i].x += dx * strength;
            forces[i].y += dy * strength;
            forces[j].x -= dx * strength;
            forces[j].y -= dy * strength;
        }
    }
    for (std::size_t i = 0; i < sites.size(); ++i) {
        // Soft wall forces plus a hard boundary keep sites in the domain.
        const double xLimit = std::max(0.0, bounds.halfWidth - margin);
        const double yLimit = std::max(0.0, bounds.halfHeight - margin);
        forces[i].x -= 4.0 * (sites[i].x - std::clamp(sites[i].x, -xLimit, xLimit));
        forces[i].y -= 4.0 * (sites[i].y - std::clamp(sites[i].y, -yLimit, yLimit));
        const double speed = std::hypot(forces[i].x, forces[i].y);
        const double scale = speed > spacing ? spacing / speed : 1.0;
        sites[i].x = std::clamp(sites[i].x + forces[i].x * scale * dt,
                                -bounds.halfWidth, bounds.halfWidth);
        sites[i].y = std::clamp(sites[i].y + forces[i].y * scale * dt,
                                -bounds.halfHeight, bounds.halfHeight);
    }
}

} // namespace blumme
