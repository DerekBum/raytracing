#pragma once

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

    Rotation doth() const;
};

inline Rotation::Rotation(float x, float y, float z, float w): v(Point(x, y, z)), w(w) {}

inline Rotation::Rotation(Point v, float w): v(v), w(w) {}

inline Rotation Rotation::operator+ (const Rotation &r) const {
    return {v + r.v, w + r.w};
}

inline Rotation Rotation::doth() const {
    return {-1.0 * v, w};
}

inline Rotation Rotation::operator* (const Rotation &r) const {
    return {w * r.v + r.w * v + v.inter(r.v), w * r.w - v.dot(r.v)};
}

inline Point Rotation::transform(const Point &p) const {
    return ((*this) * Rotation(p.x, p.y, p.z, 0.0) * doth()).v;
}
