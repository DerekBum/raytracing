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

Figure::Figure() = default;

Figure::Figure(FigureType type, Point data): type(type), data(data) {};

Figure::Figure(FigureType type, Point data, Point data2, Point data3): type(type), data(data), data2(data2), data3(data3) {};

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
    auto rot = Rotation(-1.0 * rotation.v, rotation.w);
    norma = rot.transform(norma).normalize();
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
    auto ro = Point(ray.o.x / r.x, ray.o.y / r.y, ray.o.z / r.z);
    auto rd = Point(ray.d.x / r.x, ray.d.y / r.y, ray.d.z / r.z);
    float c = ro.len_square() - 1;
    float b = 2.0 * ro * rd;
    float a = rd.len_square();

    auto opt_t = smallestPositiveRootOfQuadraticEquation(a, b, c);
    if (!opt_t.has_value()) {
        return {};
    }

    auto [t, is_inside] = opt_t.value();
    Point point = ray.o + t * ray.d;
    Point norma = (1.0 / (r * r)) * point;
    if (is_inside) {
        norma = -1. * norma;
    }
    return {Intersection {t, norma.normalize(), is_inside}};
}

std::optional<Intersection> intersectPlaneAndRay(const Point &n, const Ray &ray) {
    float t = -(ray.o * n) / (ray.d * n);
    if (t > 0 && t < 1e4) {
        if (ray.d * n > 0) {
            return {Intersection {t, -1.0 * n, true}};
        }
        return {Intersection {t, n, false}};
    }
    return {};
}

std::optional<Intersection> Figure::intersectAsPlane(const Ray &ray) const {
    return intersectPlaneAndRay(data, ray);
}

std::optional<Intersection> intersectBoxAndRay(const Point& s, const Ray& ray, bool require_normal = true) {
    auto calculateInterval = [&](float start, float end, float dir) {
        float t1 = (start) / dir;
        float t2 = (end) / dir;
        if (t1 > t2)
            std::swap(t1, t2);
        return std::make_pair(t1, t2);
    };

    auto mi = (-1.0 * s - ray.o);
    auto ma = (s - ray.o);
    auto tsX = calculateInterval(mi.x, ma.x, ray.d.x);
    auto tsY = calculateInterval(mi.y, ma.y, ray.d.y);
    auto tsZ = calculateInterval(mi.z, ma.z, ray.d.z);

    float t1 = std::max(tsX.first, std::max(tsY.first, tsZ.first));
    float t2 = std::min(tsX.second, std::min(tsY.second, tsZ.second));

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

    if (!require_normal) {
        return {Intersection {t, {}, is_inside}};
    }

    Point p = ray.o + t * ray.d;
    Point normal = Point(p.x / s.x, p.y / s.y, p.z / s.z);
    float maxComponent = std::max(std::fabs(normal.x), std::max(std::fabs(normal.y), std::fabs(normal.z)));
    if (std::fabs(normal.x) != maxComponent)
        normal.x = 0;
    if (std::fabs(normal.y) != maxComponent)
        normal.y = 0;
    if (std::fabs(normal.z) != maxComponent)
        normal.z = 0;

    if (is_inside) {
        normal = -1.0f * normal;
    }

    return {Intersection {t, normal, is_inside}};
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
    if (b.inter(p) * n < 0) {
        return {};
    }
    if (p.inter(c) * n < 0) {
        return {};
    }
    if ((c - b).inter(p - b) * n < 0) {
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
    auto rotation = Rotation(-1.0 * fig.rotation.v, fig.rotation.w);
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