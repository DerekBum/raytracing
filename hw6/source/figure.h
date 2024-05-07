#pragma once
#include <optional>
#include <cassert>
#include "point.h"
#include "color.h"
#include "rotation.h"
#include "gltf.h"

const float eps = 1e-4;

class Ray {
public:
    const Point o{}, d{};

    Ray() = default;
    Ray(Point o, Point d);

    Ray operator +(const Point &r) const;
    Ray operator -(const Point &r) const;
    Ray rotate(const Rotation &r) const;
};

inline Ray::Ray(Point o, Point d): o(o), d(d) {}

inline Ray Ray::operator+ (const Point &r) const {
    return {o + r, d};
}

inline Ray Ray::operator- (const Point &r) const {
    return {o - r, d};
}

inline Ray Ray::rotate(const Rotation &r) const {
    return {r.transform(o), r.transform(d)};
}

struct Intersection {
    float t;
    Point norma;
    bool is_inside;
};

enum class FigureType {
    ELLIPSOID, PLANE, BOX, TRIANGLE
};

class Figure {
private:
    std::optional<Intersection> intersectAsEllipsoid(const Ray &ray) const;
    std::optional<Intersection> intersectAsPlane(const Ray &ray) const;
    std::optional<Intersection> intersectAsBox(const Ray &ray) const;
    std::optional<Intersection> intersectAsTriangle(const Ray &ray) const;

public:
    Point position{};
    Rotation rotation{};
    GltfMaterial material;

    FigureType type;
    Point data{};
    Point data2{};
    Point data3{};

    Figure() {}
    Figure(FigureType type, Point data) : type(type), data(data) {}
    Figure(FigureType type, Point data, Point data2, Point data3) : type(type), data(data), data2(data2), data3(data3) {}

    std::optional<Intersection> intersect(const Ray &ray) const;
};

class AABB {
public:
    Point min;
    Point max;

    AABB() = default;
    AABB(const Figure &fig);

    void extend(const Point &p);
    void extend(const AABB &aabb);
    float area() const;

    std::optional<Intersection> intersect(const Ray &ray) const;
};
