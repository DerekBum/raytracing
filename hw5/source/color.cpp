#include <algorithm>
#include <cmath>
#include "color.h"

Color gamma(const Color &x) {
    float gamma = 1.0 / 2.2;
    return Color(pow(x.r, gamma), pow(x.g, gamma), pow(x.b, gamma));
}

Color aces(const Color &x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;

    Color res = (x * (a * x + b)) / (x * (c * x + d) + e);

    float r = std::min(1.f, std::max(0.f, res.r));
    float g = std::min(1.f, std::max(0.f, res.g));
    float blue = std::min(1.f, std::max(0.f, res.b));
    return {r, g, blue};
}
