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

std::optional <intersectPoint> Figure::intersect(const Ray &ray) {
    Ray real_ray = (ray - position).rotate(rotation);
    auto result = intersectImpl(real_ray);
    if (result.has_value()) {
        auto res = result.value();
        res.norm = Rotation(-1.0 * rotation.v, rotation.w).transform(res.norm);
        res.norm = res.norm.normalize();
        return res;
    }
    return {};
}

Plane::Plane(Point n): n(n) {
    inner = n;
    type = Figures::PLANE;
}

std::optional <intersectPoint> Plane::intersectImpl(const Ray &ray) const {
    float x = -(ray.o * n) / (ray.d * n);
    if (x > 0) {
        if (ray.d * n > 0) {
            return intersectPoint{-1.0 * n, false, x};
        }
        return intersectPoint{n, false, x};
    }
    return {};
}

Ellipsoid::Ellipsoid(Point r): r(r) {
    inner = r;
    type = Figures::ELLIPSOID;
}

std::optional<intersectPoint> Ellipsoid::intersectImpl(const Ray& ray) const {
    Point rayOriginDivided = Point(ray.o.x / r.x, ray.o.y / r.y, ray.o.z / r.z);
    Point rayDirectionDivided = Point(ray.d.x / r.x, ray.d.y / r.y, ray.d.z / r.z);

    float a = rayDirectionDivided.len_square();
    float b = 2 * (rayOriginDivided * rayDirectionDivided);
    float c = rayOriginDivided.len_square() - 1;

    float discriminant = b * b - 4 * a * c;
    if (discriminant <= 0) {
        return {};
    }

    float root1 = (-b - std::sqrt(discriminant)) / (2 * a);
    float root2 = (-b + std::sqrt(discriminant)) / (2 * a);
    if (root1 > root2) {
        std::swap(root1, root2);
    }

    if (root2 < 0) {
        return {};
    } else if (root1 < 0) {
        Point intersectionPoint = ray.o + root2 * ray.d;
        Point normalizedNormal = Point(intersectionPoint.x / (r.x * r.x), intersectionPoint.y / (r.y * r.y), intersectionPoint.z / (r.z * r.z));
        normalizedNormal = -1.0 * normalizedNormal;
        normalizedNormal = normalizedNormal.normalize();
        return intersectPoint{normalizedNormal, true, root2};
    } else {
        Point intersectionPoint = ray.o + root1 * ray.d;
        Point normalizedNormal = Point(intersectionPoint.x / (r.x * r.x), intersectionPoint.y / (r.y * r.y), intersectionPoint.z / (r.z * r.z));
        normalizedNormal = normalizedNormal.normalize();
        return intersectPoint{normalizedNormal, false, root1};
    }
}

Box::Box(Point s): s(s) {
    inner = s;
    type = Figures::BOX;
}

std::optional<intersectPoint> Box::intersectImpl(const Ray& ray) const {
    Point rayOffset = -1.0 * s - ray.o;
    Point intersection1 = Point(rayOffset.x / ray.d.x, rayOffset.y / ray.d.y, rayOffset.z / ray.d.z);
    Point diff = s - ray.o;
    Point intersection2 = Point(diff.x / ray.d.x, diff.y / ray.d.y, diff.z / ray.d.z);

    float minX = std::min(intersection1.x, intersection2.x);
    float minY = std::min(intersection1.y, intersection2.y);
    float minZ = std::min(intersection1.z, intersection2.z);
    float maxX = std::max(intersection1.x, intersection2.x);
    float maxY = std::max(intersection1.y, intersection2.y);
    float maxZ = std::max(intersection1.z, intersection2.z);

    float tNear = std::max(minX, std::max(minY, minZ));
    float tFar = std::min(maxX, std::min(maxY, maxZ));

    if (tNear > tFar || tFar < 0) {
        return {};
    }

    float t = (tNear < 0) ? tFar : tNear;
    bool inside = (tNear < 0);

    Point intersectionPoint = ray.o + t * ray.d;
    Point normalizedNorm = Point(intersectionPoint.x / s.x, intersectionPoint.y / s.y, intersectionPoint.z / s.z);

    if (inside) {
        normalizedNorm = -1.0 * normalizedNorm;
    }

    float maxComponent = std::max(std::abs(normalizedNorm.x), std::max(std::abs(normalizedNorm.y), std::abs(normalizedNorm.z)));
    if (maxComponent != std::abs(normalizedNorm.x)) {
        normalizedNorm.x = 0.0;
    }
    if (maxComponent != std::abs(normalizedNorm.y)) {
        normalizedNorm.y = 0.0;
    }
    if (maxComponent != std::abs(normalizedNorm.z)) {
        normalizedNorm.z = 0.0;
    }

    return intersectPoint{normalizedNorm, inside, t};
}
