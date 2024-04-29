#ifndef HW1_POINT_H
#define HW1_POINT_H

#include <valarray>

class Point {
public:
    float x, y, z;

    Point(float x, float y, float z);

    Point() = default;

    Point operator+ (const Point &p) const;
    Point operator- (const Point &p) const;
    float operator* (const Point &p) const;
    float len_square() const;

    Point normalize() const;

    Point inter(const Point &p) const;
};

Point operator* (float k, const Point &p);

inline Point::Point(float x, float y, float z): x(x), y(y), z(z) {}

inline Point Point::operator+ (const Point &p) const {
    return {x + p.x, y + p.y, z + p.z};
}

inline Point Point::operator- (const Point &p) const {
    return {x - p.x, y - p.y, z - p.z};
}

inline Point operator* (float k, const Point &p) {
    return {k * p.x, k * p.y, k * p.z};
}

inline float Point::operator* (const Point &p) const {
    return x * p.x + y * p.y + z * p.z;
}

inline float Point::len_square() const {
    return (*this) * (*this);
}

inline Point Point::normalize() const {
    return std::sqrt(1.0 / len_square()) * (*this);
}

inline Point Point::inter(const Point &p) const {
    return {z * p.y - y * p.z, x * p.z - z * p.x, y * p.x - x * p.y};
}

#endif //HW1_POINT_H
