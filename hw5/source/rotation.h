#ifndef HW1_ROTATION_H
#define HW1_ROTATION_H

#include "point.h"

class Rotation {
public:
    Point v;
    float w;

    Rotation(float x, float y, float z, float w);
    Rotation(Point v, float w);
    Rotation() : v({}), w(1) {};

    Rotation operator+ (const Rotation &r) const;
    Rotation operator* (const Rotation &r) const;
    Point transform(const Point &p) const;
};

inline Rotation::Rotation(float x, float y, float z, float w): v(Point(x, y, z)), w(w) {}

inline Rotation::Rotation(Point v, float w): v(v), w(w) {}

inline Rotation Rotation::operator+ (const Rotation &r) const {
    return {v + r.v, w + r.w};
}

inline Rotation Rotation::operator* (const Rotation &r) const {
    float n_x = v.z * r.v.y - v.y * r.v.z;
    float n_y = v.x * r.v.z - v.z * r.v.x;
    float n_z = v.y * r.v.x - v.x * r.v.y;

    return {w * r.v + r.w * v + Point(n_x, n_y, n_z), w * r.w - v * r.v};
}

inline Point Rotation::transform(const Point &p) const {
    return ((*this) * Rotation(p.x, p.y, p.z, 0.0) * Rotation(-1.0 * v, w)).v;
}

#endif //HW1_ROTATION_H
