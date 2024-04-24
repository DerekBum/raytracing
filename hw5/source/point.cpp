#include "point.h"
#include <cmath>

Point::Point(float x, float y, float z): x(x), y(y), z(z) {}

Point Point::operator+ (const Point &p) const {
    return {x + p.x, y + p.y, z + p.z};
}

Point Point::operator- (const Point &p) const {
    return {x - p.x, y - p.y, z - p.z};
}

Point operator* (float k, const Point &p) {
    return {k * p.x, k * p.y, k * p.z};
}

float Point::operator* (const Point &p) const {
    return x * p.x + y * p.y + z * p.z;
}

float Point::len_square() const {
    return (*this) * (*this);
}

Point Point::normalize() const {
    return std::sqrt(1. / len_square()) * (*this);
}

Point Point::inter(const Point &p) const {
    return {z * p.y - y * p.z, x * p.z - z * p.x, y * p.x - x * p.y};
}
