#include "distribution.h"

Point Uniform::sample(std::normal_distribution<float> &n01, rng_type &rng, Point x, Point n) const {
    Point d = Point {n01(rng), n01(rng), n01(rng)}.normalize();
    if (d.dot(n) < 0) {
        d = -1.0 * d;
    }
    return d;
}

float Uniform::pdf(Point x, Point n, Point d) const {
    if (d.dot(n) < 0) {
        return 0;
    }
    return 1.0 / (2.0 * PI);
}

Point Cosine::sample(std::normal_distribution<float> &n01, rng_type &rng, Point x, Point n) const {
    Point d = Point {n01(rng), n01(rng), n01(rng)}.normalize();
    d = d + n;
    float len = sqrt(d.len_square());
    if (len <= 1e-4 || d.dot(n) <= 1e-4 || std::isnan(len)) {
        return n;
    }
    return 1.0 / len * d;
}

float Cosine::pdf(Point x, Point n, Point d) const {
    return std::max(0.f, float(d.dot(n) / PI));
}


float BoxLight::pdfOne(Point x, Point d, Point y, Point yn) const {
    return (x - y).len_square() / (sTotal * fabs(d.dot(yn)));
}

Point BoxLight::sample(std::uniform_real_distribution<float> &u01, rng_type &rng, Point x, Point n) const {
    for (int _ = 0; _ < 1000; _++) {
        float u = u01(rng) * (wx + wy + wz);
        float flipSign = u01(rng) > 0.5 ? 1 : -1;
        Point point;

        if (u < wx) {
            point = Point(flipSign * sx, (2 * u01(rng) - 1) * sy, (2 * u01(rng) - 1) * sz);
        } else if (u < wx + wy) {
            point = Point((2 * u01(rng) - 1) * sx, flipSign * sy, (2 * u01(rng) - 1) * sz);
        } else {
            point = Point((2 * u01(rng) - 1) * sx, (2 * u01(rng) - 1) * sy, flipSign * sz);
        }

        Point actualPoint = figure.rotation.doth().transform(point) + figure.position;
        if (figure.intersect(Ray(x, (actualPoint - x).normalize())).has_value()) {
            return (actualPoint - x).normalize();
        }
    }
    return x.normalize();
}


float TriangleLight::pdfOne(Point x, Point d, Point y, Point yn) const {
    return pointProb * (x - y).len_square() / fabs(d.dot(yn));
}

Point TriangleLight::sample(std::uniform_real_distribution<float> &u01, rng_type &rng, Point x, Point n) const {
    const Point &a = figure.data3;
    const Point &b = figure.data - a;
    const Point &c = figure.data2 - a;
    float u = u01(rng);
    float v = u01(rng);
    if (u + v > 1.) {
        u = 1 - u;
        v = 1 - v;
    }
    Point point = figure.position + figure.rotation.doth().transform(a + u * b + v * c);
    return (point - x).normalize();
}


float EllipsoidLight::pdfOne(Point x, Point d, Point y, Point yn) const {
    Point r = figure.data;
    auto transf = figure.rotation.transform(y - figure.position);
    Point n = Point(transf.x / r.x, transf.y / r.y, transf.z / r.z);
    float pointProb = 1. / (4 * PI * sqrt(Point{n.x * r.y * r.z, r.x * n.y * r.z, r.x * r.y * n.z}.len_square()));
    return pointProb * (x - y).len_square() / fabs(d.dot(yn));
}

Point EllipsoidLight::sample(std::normal_distribution<float> &n01, rng_type &rng, Point x, Point n) const {
    Point r = figure.data;

    for (int i = 0; i < 1000; i++) {
        auto norm = Point{n01(rng), n01(rng), n01(rng)}.normalize();
        Point point = r * norm;
        Point actualPoint = figure.rotation.doth().transform(point) + figure.position;
        if (figure.intersect(Ray(x, (actualPoint - x).normalize())).has_value()) {
            return (actualPoint - x).normalize();
        }
    }
    return x.normalize();
}

Point
FiguresMix::sample(std::uniform_real_distribution<float> &u01, std::normal_distribution<float> &n01, rng_type &rng,
                   Point x, Point n) const {

    int distNum = u01(rng) * figures_.size();
    if (std::holds_alternative<BoxLight>(figures_[distNum])) {
        return std::get<BoxLight>(figures_[distNum]).sample(u01, rng, x, n);
    } else if (std::holds_alternative<EllipsoidLight>(figures_[distNum])) {
        return std::get<EllipsoidLight>(figures_[distNum]).sample(n01, rng, x, n);
    } else {
        return std::get<TriangleLight>(figures_[distNum]).sample(u01, rng, x, n);
    }
}

float FiguresMix::pdf(Point x, Point n, Point d) const {
    return getTotalPdf(0, x, n, d) / figures_.size();
}

bool FiguresMix::isEmpty() const {
    return figures_.empty();
}

float
FiguresMix::pdfLight(const std::variant<BoxLight, EllipsoidLight, TriangleLight> &figureLight, Point x, Point n,
                              Point d) const {

    const Figure &figure = std::visit([](const auto& light) { return light.figure; }, figureLight);

    auto firstIntersection = figure.intersect(Ray(x, d));
    if (!firstIntersection.has_value()) {
        return 0.;
    }
    auto [t, yn, _] = firstIntersection.value();

    Point y = x + t * d;
    float ans = std::visit([&](const auto& light) { return light.pdfOne(x, d, y, yn); }, figureLight);

    if (figure.type == FigureType::TRIANGLE) {
        return ans;
    }

    auto secondIntersection = figure.intersect(Ray(x + (t + 1e-4) * d, d));
    if (!secondIntersection.has_value()) {
        return ans;
    }
    auto [t2, yn2, __] = secondIntersection.value();
    Point y2 = x + (t + 1e-4 + t2) * d;
    return ans + std::visit([&](const auto& light) { return light.pdfOne(x, d, y2, yn2); }, figureLight);
}

float FiguresMix::getTotalPdf(uint32_t pos, const Point &x, const Point &n, const Point &d) const {
    Ray ray(x, d);
    const Node &cur = bvh.nodes[pos];
    auto intersection = cur.aabb.intersect(ray);
    if (!intersection.has_value()) {
        return 0;
    }

    if (cur.left == 0) {
        float result = 0;
        for (uint32_t i = cur.first; i < cur.last; i++) {
            result += pdfLight(figures_[i], x, n, d);
        }
        return result;
    }

    return getTotalPdf(cur.left, x, n, d) + getTotalPdf(cur.right, x, n, d);
}

Point
Mix::sample(std::uniform_real_distribution<float> &u01, std::normal_distribution<float> &n01, rng_type &rng, Point x,
            Point n) const {

    int distNum = u01(rng) * components.size();
    if (std::holds_alternative<Cosine>(components[distNum])) {
        return std::get<Cosine>(components[distNum]).sample(n01, rng, x, n);
    } else {
        return std::get<FiguresMix>(components[distNum]).sample(u01, n01, rng, x, n);
    }
}

float Mix::pdf(Point x, Point n, Point d) const {
    float ans = 0;
    for (const auto &component : components) {
        std::visit([&](auto&& comp) {
            ans +=  comp.pdf(x, n, d);
        }, component);
    }
    return ans / float(components.size());
}
