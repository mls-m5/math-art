#include "simplex_texture.h"

#include <algorithm>
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
    noiseart::SimplexTexture noise(42), same(42), different(43);
    require(noise.sample(.23,.71) == same.sample(.23,.71), "Seed is not reproducible");
    require(noise.sample(.23,.71) != different.sample(.23,.71), "Seeds produce identical noise");
    for (int i = -20; i <= 20; ++i) {
        const double u = i * .071, v = i * .039;
        const double value = noise.sample(u,v);
        require(value >= 0 && value <= 1, "Noise outside normalized range");
        require(std::abs(value - noise.sample(u+1,v-2)) < 1e-5, "Noise is not periodic");
        require(std::abs(value - noise.sample(u+.00001,v)) < .01, "Noise is discontinuous");
    }
    require(std::abs(noise.sample(-.000001,.4) - noise.sample(.000001,.4)) < .01,
            "Texture has a seam at its repeat boundary");
    const auto image = noise.rgba(64);
    require(image.size() == 64*64*4, "Incorrect RGBA image size");
    require(image == same.rgba(64), "Image is not reproducible");
    int low=255, high=0;
    for (std::size_t i=0; i<image.size(); i+=4) {
        require(image[i] == image[i+1] && image[i] == image[i+2] && image[i+3] == 255,
                "Invalid grayscale RGBA pixels");
        low=std::min(low,int(image[i])); high=std::max(high,int(image[i]));
    }
    require(high-low > 100, "Noise texture lacks contrast");
    bool rejected=false;
    try { noise.rgba(0); } catch(const std::invalid_argument &) { rejected=true; }
    require(rejected, "Invalid texture size was accepted");
    std::cout << "Seamless simplex texture tests passed\n";
}
