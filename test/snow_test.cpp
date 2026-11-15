#include "snow.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

void require(bool condition, const char *message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

int main()
{
    constexpr double angle = 6.283185307179586 / 6;
    const auto rotate = [](graphics::Point p) {
        return graphics::Point{p.x*std::cos(angle)-p.y*std::sin(angle),
                               p.x*std::sin(angle)+p.y*std::cos(angle)};
    };
    const auto near = [](graphics::Point a, graphics::Point b) { return std::hypot(a.x-b.x,a.y-b.y)<1e-10; };
    for (unsigned seed=0; seed<20; ++seed) {
        const auto flake = snow::generateFlake(seed);
        const auto same = snow::generateFlake(seed);
        require(!flake.empty() && flake.size()%6==0, "Missing sixfold arms");
        require(flake.size()==same.size(), "Flakes are not reproducible");
        const auto arm = flake.size()/6;
        for (std::size_t i=0; i<flake.size(); ++i) {
            const auto &a = flake[i], &b = flake[(i+arm)%flake.size()];
            require(near(rotate(a.start),b.start) && near(rotate(a.end),b.end), "Broken sixfold symmetry");
            require(near(a.end,same[i].end) && a.width==same[i].width, "Seeded geometry changed");
            require(std::hypot(a.end.x,a.end.y)<=1.00001 && a.width>0, "Invalid branch dimensions");
        }
        std::vector<graphics::Vertex> mesh;
        snow::appendFlake(mesh,flake,{240,160},80,0.4,0.8,480,320);
        require(mesh.size()==flake.size()*6, "Incomplete capsule mesh");
        for (const auto &v:mesh)
            require(std::isfinite(v.position.x) && std::isfinite(v.position.y) &&
                    v.parameters.x>=0 && v.parameters.y>=0.55 && v.variant==0.8f, "Invalid capsule vertex");
    }
    const auto a=snow::generateFlake(1), b=snow::generateFlake(2);
    require(a.size()!=b.size() || !near(a[1].end,b[1].end), "Variants do not differ");
    snow::Snowfall field(42), same(42);
    const auto before=field.particles();
    field.update(0);
    require(field.particles()[0].y==before[0].y, "Zero timestep changed position");
    field.update(.1);
    same.update(.1);
    require(field.particles()[0].y<before[0].y, "Snow does not fall downward");
    require(field.particles()[0].angle!=before[0].angle, "Snow does not rotate");
    require(field.particles()[0].x==same.particles()[0].x, "Snowfall is not reproducible");
    field.update(1000);
    require(field.particles().size()==150, "Recycling changed particle count");
    for (const auto &p:field.particles())
        require(p.x>=0 && p.x<1 && p.y>1 && p.y<=1.2 && p.variant<9 &&
                p.depth>=0 && p.depth<=1 && std::isfinite(p.angle), "Invalid respawned snowflake");
    bool rejected=false;
    try { field.update(-1); } catch(const std::invalid_argument &) { rejected=true; }
    require(rejected, "Invalid timestep accepted");
    std::cout << "Snowflake symmetry, variation, capsule geometry, drift, and recycling tests passed\n";
}
