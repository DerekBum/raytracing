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
    Color operator+ (const Color &c) const;

    Color() = default;
};

Color gamma(const Color &x);
Color aces(const Color &x);

Color operator* (float k, const Color &c);

inline Color::Color(float red, float green, float blue): r(red), g(green), b(blue) {}

inline Color Color::operator* (const Color &c) const {
    return {r * c.r, g * c.g, b * c.b};
}

inline Color Color::operator/ (const Color &c) const {
    return {r / c.r, g / c.g, b / c.b};
}

inline Color operator* (float k, const Color &c) {
    return {k * c.r, k * c.g, k * c.b};
}

inline Color Color::operator+ (float k) const {
    return {k + r, k + g, k + b};
}

inline Color Color::operator+(const Color &c) const {
    return {r + c.r, g + c.g, b + c.b};
}

#endif //HW1_COLOR_H
