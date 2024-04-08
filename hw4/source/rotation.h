#ifndef HW1_ROTATION_H
#define HW1_ROTATION_H

#include "point.h"

class Rotation {
public:
    Point v;
    float w;

    Rotation(float x, float y, float z, float w);
    Rotation(Point v, float w);
    Rotation() = default;

    Rotation operator+ (const Rotation &r) const;
    Rotation operator* (const Rotation &r) const;
    Point transform(const Point &p) const;
};

#endif //HW1_ROTATION_H
