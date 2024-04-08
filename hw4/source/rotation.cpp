#include "rotation.h"

Rotation::Rotation(float x, float y, float z, float w): v(Point(x, y, z)), w(w) {}

Rotation::Rotation(Point v, float w): v(v), w(w) {}

Rotation Rotation::operator+ (const Rotation &r) const {
    return {v + r.v, w + r.w};
}

Rotation Rotation::operator* (const Rotation &r) const {
    float n_x = v.z * r.v.y - v.y * r.v.z;
    float n_y = v.x * r.v.z - v.z * r.v.x;
    float n_z = v.y * r.v.x - v.x * r.v.y;

    return {w * r.v + r.w * v + Point(n_x, n_y, n_z), w * r.w - v * r.v};
}

Point Rotation::transform(const Point &p) const {
    return ((*this) * Rotation(p.x, p.y, p.z, 0.0) * Rotation(-1.0 * v, w)).v;
}
