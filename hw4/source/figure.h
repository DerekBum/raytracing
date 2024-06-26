#ifndef HW1_FIGURE_H
#define HW1_FIGURE_H

#include <optional>
#include "point.h"
#include "color.h"
#include "rotation.h"

class Ray { // to trace.
public:
    Point o, d;

    Ray(Point o, Point d);
    Ray() = default;

    Ray operator+ (const Point &p) const;
    Ray operator- (const Point &p) const;
    Ray rotate(const Rotation &r) const;
};

enum class Material {
    DEFAULT, METALLIC, DIELECTRIC // DEFAULT == DIFFUSE.
};
enum class Figures {
    PLANE, BOX, ELLIPSOID
};

struct intersectPoint {
    Point norm;
    bool inner;
    float pt;
};

class Figure {
public:
    Point position{};
    Rotation rotation = {0, 0, 0, 1};
    Color color;

    Material material = Material::DEFAULT;
    float ior{};
    Color emission;

    Point inner;
    Figures type;

    Figure() = default;
    Figure(Point inner): inner(inner) {}

    std::optional <intersectPoint> intersect(const Ray &ray);
    virtual std::optional <intersectPoint> intersectImpl(const Ray &ray) const = 0;
};

class Plane : public Figure {
public:
    Point n = inner;

    Plane(Point n);
    Plane() = default;

    std::optional <intersectPoint> intersectImpl(const Ray &ray) const override;
};

class Ellipsoid : public Figure {
public:
    Point r = inner;

    Ellipsoid(Point r);
    Ellipsoid() = default;

    std::optional <intersectPoint> intersectImpl(const Ray &ray) const override;
};

class Box : public Figure {
public:
    Point s = inner;

    Box(Point s);
    Box() = default;

    std::optional <intersectPoint> intersectImpl(const Ray &ray) const override;
};

#endif //HW1_FIGURE_H
