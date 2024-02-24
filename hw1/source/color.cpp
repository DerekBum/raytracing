#include "color.h"
#include <cmath>

Color::Color(float red, float green, float blue) {
    r = std::round(255 * red);
    g = std::round(255 * green);
    b = std::round(255 * blue);
}
