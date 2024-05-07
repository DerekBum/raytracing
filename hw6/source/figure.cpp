#include "figure.h"
#include <cmath>

static const float T_MAX = 1e4;

std::optional<Intersection> Figure::intersect(const Ray &ray) const {
    Ray transformed = (ray - position).rotate(rotation);

    std::optional<Intersection> result;
    if (type == FigureType::ELLIPSOID) {
        result = intersectAsEllipsoid(transformed);
    } else if (type == FigureType::PLANE) {
        result = intersectAsPlane(transformed);
    } else if (type == FigureType::BOX) {
        result = intersectAsBox(transformed);
    } else {
        result = intersectAsTriangle(transformed);
    }

    if (!result.has_value()) {
        return {};
    }
    auto [t, norma, is_inside] = result.value();
    norma = rotation.doth().transform(norma).normalize();
    return {Intersection {t, norma, is_inside}};
}

std::optional<std::pair<float, bool>> smallestPositiveRootOfQuadraticEquation(float a, float b, float c) {
    float d = b * b - 4 * a * c;
    if (d <= 0) {
        return {};
    }

    float x1 = (-b - sqrt(d)) / (2 * a);
    float x2 = (-b + sqrt(d)) / (2 * a);
    if (x1 > x2) {
        std::swap(x1, x2);
    }

    if (x2 < 0) {
        return {};
    } else if (x1 < 0) {
        return {std::make_pair(x2, true)};
    } else {
        return {std::make_pair(x1, false)};
    }
}

std::optional<Intersection> Figure::intersectAsEllipsoid(const Ray &ray) const {
    Point r = data;
    float c = (ray.o / r).len_square() - 1;
    float b = 2. * (ray.o / r).dot(ray.d / r);
    float a = (ray.d / r).len_square();

    auto opt_t = smallestPositiveRootOfQuadraticEquation(a, b, c);
    if (!opt_t.has_value()) {
        return {};
    }

    auto [t, is_inside] = opt_t.value();
    Point point = ray.o + t * ray.d;
    Point norma = point / (r * r);
    if (is_inside) {
        norma = -1. * norma;
    }
    return {Intersection {t, norma.normalize(), is_inside}};
}

std::optional<Intersection> intersectPlaneAndRay(const Point &n, const Ray &ray) {
    float t = -ray.o.dot(n) / ray.d.dot(n);
    if (t > 0 && t < T_MAX) {
        if (ray.d.dot(n) > 0) {
            return {Intersection {t, -1. * n, true}};
        }
        return {Intersection {t, n, false}};
    }
    return {};
}

std::optional<Intersection> Figure::intersectAsPlane(const Ray &ray) const {
    return intersectPlaneAndRay(data, ray);
}

std::optional<Intersection> intersectBoxAndRay(const Point &s, const Ray &ray, bool require_norma = true) {
    Point ts1 = (-1. * s - ray.o) / ray.d;
    Point ts2 = (s - ray.o) / ray.d;
    float t1x = std::min(ts1.x, ts2.x), t2x = std::max(ts1.x, ts2.x);
    float t1y = std::min(ts1.y, ts2.y), t2y = std::max(ts1.y, ts2.y);
    float t1z = std::min(ts1.z, ts2.z), t2z = std::max(ts1.z, ts2.z);
    float t1 = std::max(std::max(t1x, t1y), t1z);
    float t2 = std::min(std::min(t2x, t2y), t2z);
    if (t1 > t2 || t2 < 0) {
        return {};
    }

    float t;
    bool is_inside;
    if (t1 < 0) {
        is_inside = true;
        t = t2;
    } else {
        is_inside = false;
        t = t1;
    }

    if (!require_norma) {
        return {Intersection {t, {}, is_inside}};
    }

    Point p = ray.o + t * ray.d;
    Point norma = p / s;
    float mx = std::max(std::max(fabs(norma.x), fabs(norma.y)), fabs(norma.z));

    if (fabs(norma.x) != mx) {
        norma.x = 0;
    }
    if (fabs(norma.y) != mx) {
        norma.y = 0;
    }
    if (fabs(norma.z) != mx) {
        norma.z = 0;
    }

    if (is_inside) {
        norma = -1. * norma;
    }

    return {Intersection {t, norma, is_inside}};
}

std::optional<Intersection> Figure::intersectAsBox(const Ray &ray) const {
    return intersectBoxAndRay(data, ray);
}

std::optional<Intersection> Figure::intersectAsTriangle(const Ray &ray) const {
    const Point &a = data3;
    const Point &b = data - a;
    const Point &c = data2 - a;
    Point n = b.inter(c);
    auto intersection = intersectPlaneAndRay(n, ray - a);
    if (!intersection.has_value()) {
        return {};
    }
    auto t = intersection.value().t;
    Point p = ray.o - a + t * ray.d;
    if (b.inter(p).dot(n) < 0) {
        return {};
    }
    if (p.inter(c).dot(n) < 0) {
        return {};
    }
    if ((c - b).inter(p - b).dot(n) < 0) {
        return {};
    }
    return intersection;
}

AABB::AABB(const Figure &fig) {
    if (fig.type == FigureType::BOX || fig.type == FigureType::ELLIPSOID) {
        min = (-1.) * fig.data;
        max = fig.data;
    } else if (fig.type == FigureType::TRIANGLE) {
        min = Point(
                std::min(fig.data3.x, std::min(fig.data.x, fig.data2.x)),
                std::min(fig.data3.y, std::min(fig.data.y, fig.data2.y)),
                std::min(fig.data3.z, std::min(fig.data.z, fig.data2.z))
        );
        max = Point(
                std::max(fig.data3.x, std::max(fig.data.x, fig.data2.x)),
                std::max(fig.data3.y, std::max(fig.data.y, fig.data2.y)),
                std::max(fig.data3.z, std::max(fig.data.z, fig.data2.z))
        );
    }
    auto rotation = fig.rotation.doth();
    AABB unbiased = *this;
    min = max = rotation.transform(unbiased.min);
    extend(rotation.transform(Point(unbiased.min.x, unbiased.min.y, unbiased.max.z)));
    extend(rotation.transform(Point(unbiased.min.x, unbiased.max.y, unbiased.min.z)));
    extend(rotation.transform(Point(unbiased.min.x, unbiased.max.y, unbiased.max.z)));
    extend(rotation.transform(Point(unbiased.max.x, unbiased.min.y, unbiased.min.z)));
    extend(rotation.transform(Point(unbiased.max.x, unbiased.min.y, unbiased.max.z)));
    extend(rotation.transform(Point(unbiased.max.x, unbiased.max.y, unbiased.min.z)));
    extend(rotation.transform(Point(unbiased.max.x, unbiased.max.y, unbiased.max.z)));
    min = min + fig.position;
    max = max + fig.position;
}


void AABB::extend(const Point &p) {
    max.x = std::max(max.x, p.x);
    max.y = std::max(max.y, p.y);
    max.z = std::max(max.z, p.z);
    min.x = std::min(min.x, p.x);
    min.y = std::min(min.y, p.y);
    min.z = std::min(min.z, p.z);
}

void AABB::extend(const AABB &aabb) {
    extend(aabb.min);
    extend(aabb.max);
}

float AABB::area() const {
    Point d = max - min;
    return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
}

std::optional<Intersection> AABB::intersect(const Ray &ray) const {
    return intersectBoxAndRay(0.5 * (max - min), ray - 0.5 * (min + max), false);
}