#ifndef HW2_LIGHT_H
#define HW2_LIGHT_H

#include <tuple>
#include "color.h"
#include "point.h"

struct lightInfo {
    Point point{};
    Color color{};
    float r{};
};

class Light {
public:
    Color intensity;

    virtual lightInfo getLight(Point point) const = 0;

    Light() = default;
    Light(Color intensity);
};

class IndirectLight : public Light {
public:
    Point position, attenuation;

    virtual lightInfo getLight(Point point) const override;

    IndirectLight() = default;
    IndirectLight(Color intensity, Point position, Point attenuation);
};

class DirectedLight : public Light {
public:
    Point direction;

    virtual lightInfo getLight(Point point) const override;

    DirectedLight() = default;
    DirectedLight(Color intensity, Point direction);
};

#endif //HW2_LIGHT_H
