#ifndef HW1_COLOR_H
#define HW1_COLOR_H

#include <cstdint>

class Color {
public:
    float r{}, g{}, b{};
    Color(float r, float g, float b);

    Color operator* (const Color &c) const;
    Color operator/ (const Color &c) const;
    Color operator+ (float k) const;

    Color() = default;
};

Color gamma(const Color &x);
Color aces(const Color &x);

Color operator* (float k, const Color &p);

#endif //HW1_COLOR_H
