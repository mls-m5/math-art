#include "grid.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

double cross(graphics::Point a, graphics::Point b, graphics::Point c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}
} // namespace

int main()
{
    bool sawLargeScaleMotion = false;
    for (unsigned seed = 0; seed < 5; ++seed) {
        wonky::Grid grid(16, 10, seed);
        const auto initial = grid.points(0.0);
        const auto later = grid.points(1.0);
        require(initial[20].x != later[20].x, "Interior vertex did not move");
        require(initial[1].x != later[1].x, "Edge vertex did not slide");
        for (int step = 0; step < 100; ++step) {
            const double time = step * 1.371;
            const auto points = grid.points(time);
            const int stride = grid.columns() + 1;
            require(points.size() == static_cast<std::size_t>(stride * (grid.rows() + 1)),
                    "Incorrect lattice size");
            for (int y = 0; y <= grid.rows(); ++y) {
                for (int x = 0; x <= grid.columns(); ++x) {
                    const auto p = points[y * stride + x];
                    const double baseX = static_cast<double>(x) / grid.columns();
                    const double baseY = static_cast<double>(y) / grid.rows();
                    if (std::abs(p.x - baseX) > 0.32 / grid.columns() ||
                        std::abs(p.y - baseY) > 0.32 / grid.rows())
                        sawLargeScaleMotion = true;
                    require(p.x >= 0 && p.x <= 1 && p.y >= 0 && p.y <= 1,
                            "Vertex escaped unit square");
                    if (x == 0 || x == grid.columns())
                        require(p.x == static_cast<double>(x) / grid.columns(), "Edge x drifted");
                    if (y == 0 || y == grid.rows())
                        require(p.y == static_cast<double>(y) / grid.rows(), "Edge y drifted");
                    if (x < grid.columns() && y < grid.rows()) {
                        const graphics::Point quad[]{p, points[y * stride + x + 1],
                            points[(y + 1) * stride + x + 1], points[(y + 1) * stride + x]};
                        for (int n = 0; n < 4; ++n)
                            require(cross(quad[n], quad[(n + 1) % 4], quad[(n + 2) % 4]) > 0,
                                    "Cell folded or became nonconvex");
                    }
                }
            }
            const auto mesh = grid.mesh(time);
            require(mesh.size() == 16 * 10 * 6, "Incorrect mesh size");
            double twiceArea = 0.0;
            const graphics::Point uv[]{{0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1}};
            for (std::size_t i = 0; i < mesh.size(); ++i) {
                require(mesh[i].coordinate.x == uv[i % 6].x &&
                            mesh[i].coordinate.y == uv[i % 6].y, "Cell UV mapping changed");
                const std::size_t cell = i / 6;
                const float expectedVariant = static_cast<float>(
                    (cell % grid.columns() + cell / grid.columns()) % 2);
                require(mesh[i].variant == expectedVariant, "Checkerboard inversion changed");
                if (i % 3 == 0)
                    twiceArea += cross(mesh[i].position, mesh[i + 1].position, mesh[i + 2].position);
            }
            require(std::abs(twiceArea - 2.0) < 1e-10, "Grid does not cover screen area");
        }
    }
    require(sawLargeScaleMotion, "Shared field never exceeded cell-local motion");
    wonky::Grid single(1, 1, 42);
    require(single.points(12)[0].x == 0 && single.points(12)[3].y == 1,
            "Single cell corners moved");
    std::cout << "Animated grid geometry tests passed\n";
}
