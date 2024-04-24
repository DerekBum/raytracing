#pragma once
#include <optional>
#include <cassert>
#include "point.h"
#include "color.h"
#include "rotation.h"

enum class Material {
    DIFFUSE, METALLIC, DIELECTRIC
};

class Ray {
public:
    const Point o{}, d{};

    Ray() = default;
    Ray(Point o, Point d);

    Ray operator +(const Point &other) const;
    Ray operator -(const Point &other) const;
    Ray rotate(const Rotation &rotation) const;
};

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
    Material material = Material::DIFFUSE;
    Color color{};
    Color emission{};
    float ior;

    FigureType type;
    Point data{};
    Point data2{};
    Point data3{};

    Figure();
    Figure(FigureType type, Point data);
    Figure(FigureType type, Point data, Point data2, Point data3);

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
