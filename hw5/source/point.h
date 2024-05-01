#pragma once

#include <valarray>

class Point {
public:
    float x, y, z;

    Point(float x, float y, float z);

    Point() = default;

    Point operator+ (const Point &p) const;
    Point operator- (const Point &p) const;
    Point operator* (const Point &p) const;
    float dot(const Point &p) const;
    float len_square() const;

    Point normalize() const;

    Point inter(const Point &p) const;

    Point operator+ (float k) const;
    Point operator/ (const Point &p) const;
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

inline float Point::dot(const Point &p) const {
    return x * p.x + y * p.y + z * p.z;
}

inline Point Point::operator* (const Point &p) const {
    return {x * p.x, y * p.y, z * p.z};
}

inline float Point::len_square() const {
    return this->dot(*this);
}

inline Point Point::normalize() const {
    return std::sqrt(1.0 / len_square()) * (*this);
}

inline Point Point::inter(const Point &p) const {
    return {z * p.y - y * p.z, x * p.z - z * p.x, y * p.x - x * p.y};
}

inline Point Point::operator+ (float k) const {
    return {k + x, k + y, k + z};
}

inline Point Point::operator/ (const Point &p) const {
    return {x / p.x, y / p.y, z / p.z};
}
