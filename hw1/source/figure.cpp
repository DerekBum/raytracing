#include "figure.h"
#include <cmath>

Ray::Ray(Point o, Point d): o(o), d(d) {}

Ray Ray::operator+ (const Point &p) const {
    return {o + p, d};
}

Ray Ray::operator- (const Point &p) const {
    return {o - p, d};
}

Ray Ray::rotate(const Rotation &r) const {
    return {r.transform(o), r.transform(d)};
}

std::optional <std::pair <float, Color>> Figure::intersect(const Ray &ray) {
    Ray real_ray = (ray - position).rotate(rotation);
    auto result = intersectImpl(real_ray);
    if (result.has_value()) {
        return std::make_pair(result.value(), color);
    }
    return {};
}

Plane::Plane(Point n): n(n) {}

std::optional <float> Plane::intersectImpl(const Ray &ray) const {
    float x = -(ray.o * n) / (ray.d * n);
    if (x > 0) {
        return x;
    }
    return {};
}

Ellipsoid::Ellipsoid(Point r): r(r) {}

std::optional <float> Ellipsoid::intersectImpl(const Ray &ray) const {
    Point ray_o_div = Point(ray.o.x / r.x, ray.o.y / r.y, ray.o.z / r.z);
    Point ray_d_div = Point(ray.d.x / r.x, ray.d.y / r.y, ray.d.z / r.z);

    float a = ray_d_div.len_square();
    float b = 2 * ray_o_div * ray_d_div;
    float c = ray_o_div.len_square() - 1;

    float disc = b * b - 4 * a * c;
    if (disc <= 0) {
        return {};
    }

    float res1 = (-b - std::sqrt(disc)) / (2 * a);
    float res2 = (-b + std::sqrt(disc)) / (2 * a);
    if (res1 > res2) {
        std::swap(res1, res2);
    }

    if (res2 < 0) {
        return {};
    } else if (res1 < 0) {
        return res2;
    } else {
        return res1;
    }
}

Box::Box(Point s): s(s) {}

std::optional <float> Box::intersectImpl(const Ray &ray) const {
    Point ray_o = (-1.0 * s - ray.o);
    Point ray_o_z = (s - ray.o);

    Point int1 = Point(ray_o.x / ray.d.x, ray_o.y / ray.d.y, ray_o.z / ray.d.z);
    Point int2 = Point(ray_o_z.x / ray.d.x, ray_o_z.y / ray.d.y, ray_o_z.z / ray.d.z);

    float d1x = std::min(int1.x, int2.x), d2x = std::max(int1.x, int2.x);
    float d1y = std::min(int1.y, int2.y), d2y = std::max(int1.y, int2.y);
    float d1z = std::min(int1.z, int2.z), d2z = std::max(int1.z, int2.z);

    float res1 = std::max(std::max(d1x, d1y), d1z);
    float res2 = std::min(std::min(d2x, d2y), d2z);

    if (res1 > res2 || res2 < 0) {
        return {};
    } else if (res1 < 0) {
        return res2;
    } else {
        return res1;
    }
}
