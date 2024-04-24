#ifndef HW1_POINT_H
#define HW1_POINT_H

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

#endif //HW1_POINT_H
