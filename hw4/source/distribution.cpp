#include "distribution.h"

Point Cosine::sample(Point x, Point n) {
    Point d = Point{n01(rng), n01(rng), n01(rng)}.normalize();
    d = d + n;
    float len = sqrt(d.len_square());
    if (len <= 1e-7 || d * n <= 1e-7) {
        return n;
    }
    return 1.0 / len * d;
}

float Cosine::pdf(Point x, Point n, Point d) {
    return std::max(0.0f, d * n / float(acos(-1)));
}

float FigureLight::pdf(Point x, Point n, Point d) {
    auto firstIntersection = figure.intersect(Ray(x, d));
    if (!firstIntersection.has_value()) {
        return 0.;
    }
    auto val1 = firstIntersection.value();
    Point y = x + val1.pt * d;
    float ans = runPdf(x, d, y, val1.norm);

    auto secondIntersection = figure.intersect(Ray(x + (val1.pt + 0.0001) * d, d));
    if (!secondIntersection.has_value()) {
        return ans;
    }
    auto val2 = secondIntersection.value();
    Point y2 = x + (val1.pt + 0.0001 + val2.pt) * d;
    return ans + runPdf(x, d, y2, val2.norm);
}

BoxLight::BoxLight(std::minstd_rand &rng, Figure &box): FigureLight(box), rng(rng) {
    float sx = box.inner.x, sy = box.inner.y, sz = box.inner.z;
    result = 8 * (sy * sz + sx * sz + sx * sy);
}

float BoxLight::runPdf(Point x, Point d, Point y, Point yn) {
    return (x - y).len_square() / (result * fabs(d * yn));
}

Point BoxLight::sample(Point x, Point n) {
    float figureInnerX = figure.inner.x;
    float figureInnerY = figure.inner.y;
    float figureInnerZ = figure.inner.z;

    float wx = figureInnerY * figureInnerZ;
    float wy = figureInnerX * figureInnerZ;
    float wz = figureInnerX * figureInnerY;

    while (true) {
        float u = u01(rng) * (wx + wy + wz);
        float sign = u01(rng) > 0.5 ? 1 : -1;
        Point point;

        if (u < wx) {
            point = Point(sign * figureInnerX, (2 * u01(rng) - 1) * figureInnerY, (2 * u01(rng) - 1) * figureInnerZ);
        } else if (u < wx + wy) {
            point = Point((2 * u01(rng) - 1) * figureInnerX, sign * figureInnerY, (2 * u01(rng) - 1) * figureInnerZ);
        } else {
            point = Point((2 * u01(rng) - 1) * figureInnerX, (2 * u01(rng) - 1) * figureInnerY, sign * figureInnerZ);
        }

        auto rotated = Rotation(-1.0 * figure.rotation.v, figure.rotation.w);

        Point actual = rotated.transform(point) + figure.position;
        if (figure.intersect(Ray(x, (actual - x).normalize())).has_value()) {
            return (actual - x).normalize();
        }
    }
}

float EllipsoidLight::runPdf(Point x, Point d, Point y, Point yn) {
    Point r = figure.inner;

    auto transformed = figure.rotation.transform(y - figure.position);

    Point n = Point(transformed.x / r.x, transformed.y / r.y, transformed.z / r.z);
    float prob = 1.0 / (4 * acos(-1) * sqrt(Point{n.x * r.y * r.z, r.x * n.y * r.z, r.x * r.y * n.z}.len_square()));
    return prob * (x - y).len_square() / fabs(d * yn);
}

Point EllipsoidLight::sample(Point x, Point n) {
    Point r = figure.inner;

    for (int i = 0; i < 100; i++) {
        auto normal = Point{n01(rng), n01(rng), n01(rng)}.normalize();

        Point point = Point(r.x * normal.x, r.y * normal.y, r.z * normal.z);

        auto rotated = Rotation(-1.0 * figure.rotation.v, figure.rotation.w);

        Point actual = rotated.transform(point) + figure.position;
        if (figure.intersect(Ray(x, (actual - x).normalize())).has_value()) {
            return (actual - x).normalize();
        }
    }
    // TODO -- this is wrong :/
    return x.normalize();
}

Point Mix::sample(Point x, Point n) {
    int distNum = u01(rng) * components.size();
    return components[distNum]->sample(x, n);
}

float Mix::pdf(Point x, Point n, Point d) {
    float ans = 0;
    for (auto& component : components) {
        ans += component->pdf(x, n, d);
    }
    return ans / float(components.size());
}
