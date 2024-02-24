#ifndef HW1_COLOR_H
#define HW1_COLOR_H

#include <cstdint>

class Color {
public:
    uint8_t r{}, g{}, b{};
    Color(float r, float g, float b);

    Color() = default;
};

#endif //HW1_COLOR_H
