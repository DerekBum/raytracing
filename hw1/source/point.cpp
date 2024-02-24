#include "point.h"

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
