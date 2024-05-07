#pragma once
#include <random>
#include <cmath>
#include <memory>
#include <variant>
#include "point.h"
#include "figure.h"
#include "bvh.h"

typedef std::minstd_rand rng_type;
const float PI = acos(-1);

class Uniform {
public:
    Uniform() {}

    Point sample(std::normal_distribution<float> &n01, rng_type &rng, Point x, Point n) const {
        (void) x;
        Point d = Point {n01(rng), n01(rng), n01(rng)}.normalize();
        if (d.dot(n) < 0) {
            d = -1. * d;
        }
        return d;
    }

    float pdf(Point x, Point n, Point d) const {
        (void) x;
        if (d.dot(n) < 0) {
            return 0;
        }
        return 1. / (2. * PI);
    }
};


class Cosine {
private:
    static constexpr float eps = 1e-9;

public:
    Cosine() {}

    Point sample(std::normal_distribution<float> &n01, rng_type &rng, Point x, Point n) const {
        (void) x;
        Point d = Point {n01(rng), n01(rng), n01(rng)}.normalize();
        d = d + n;
        float len = sqrt(d.len_square());
        if (len <= eps || d.dot(n) <= eps || std::isnan(len)) {
            // Can happen due to precision errors
            return n;
        }
        return 1. / len * d;
    }

    float pdf(Point x, Point n, Point d) const {
        (void) x;
        return std::max(0.f, d.dot(n) / PI);
    }
};

class BoxLight {
private:
    float sTotal, sx, sy, sz, wx, wy, wz;

public:
    const Figure figure;

public:
    float pdfOne(Point x, Point d, Point y, Point yn) const {
        return (x - y).len_square() / (sTotal * fabs(d.dot(yn)));
    }

public:
    BoxLight(const Figure &box): figure(box) {
        sx = box.data.x;
        sy = box.data.y;
        sz = box.data.z;
        sTotal = 8 * (sy * sz + sx * sz + sx * sy);
        wx = sy * sz;
        wy = sx * sz;
        wz = sx * sy;
    }

    Point sample(std::uniform_real_distribution<float> &u01, rng_type &rng, Point x, Point n) const {
        (void) n;

        while (true) {
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
    }
};

class TriangleLight {
private:
    float pointProb;

public:
    const Figure figure;

public:
    float pdfOne(Point x, Point d, Point y, Point yn) const {
        return pointProb * (x - y).len_square() / fabs(d.dot(yn));
    }

public:
    TriangleLight(const Figure &ellipsoid): figure(ellipsoid) {
        const Point &a = figure.data3;
        const Point &b = figure.data - a;
        const Point &c = figure.data2 - a;
        Point n = b.inter(c);
        pointProb = 1.0 / (0.5 * sqrt(n.len_square()));
    }

    Point sample(std::uniform_real_distribution<float> &u01, rng_type &rng, Point x, Point n) const {
        (void) n;
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
};

class EllipsoidLight {
public:
    const Figure figure;

public:
    float pdfOne(Point x, Point d, Point y, Point yn) const {
        Point r = figure.data;
        Point n = figure.rotation.transform(y - figure.position) / r;
        float pointProb = 1. / (4 * PI * sqrt(Point{n.x * r.y * r.z, r.x * n.y * r.z, r.x * r.y * n.z}.len_square()));
        return pointProb * (x - y).len_square() / fabs(d.dot(yn));
    }

public:
    EllipsoidLight(const Figure &ellipsoid): figure(ellipsoid) {}

    Point sample(std::normal_distribution<float> &n01, rng_type &rng, Point x, Point n) const {
        (void) n;
        Point r = figure.data;

        while (true) {
            Point point = r * Point{n01(rng), n01(rng), n01(rng)}.normalize();
            Point actualPoint = figure.rotation.doth().transform(point) + figure.position;
            if (figure.intersect(Ray(x, (actualPoint - x).normalize())).has_value()) {
                return (actualPoint - x).normalize();
            }
        }
    }
};

class FiguresMix {
private:
    std::vector<std::variant<BoxLight, EllipsoidLight, TriangleLight>> figures_;
    BVH bvh;

public:
    FiguresMix(std::vector<Figure> figures) {
        size_t n = std::partition(figures.begin(), figures.end(), [](const auto &fig) {
            if (fig.material.emission.x == 0 && fig.material.emission.y == 0 && fig.material.emission.z == 0) {
                return false;
            }
            return fig.type == FigureType::BOX || fig.type == FigureType::ELLIPSOID || fig.type == FigureType::TRIANGLE;
        }) - figures.begin();

        bvh = BVH(figures, n);
        for (size_t i = 0; i < n; i++) {
            if (figures[i].type == FigureType::BOX) {
                figures_.push_back(BoxLight(figures[i]));
            } else if (figures[i].type == FigureType::ELLIPSOID) {
                figures_.push_back(EllipsoidLight(figures[i]));
            } else {
                figures_.push_back(TriangleLight(figures[i]));
            }
        }
    }

    Point sample(std::uniform_real_distribution<float> &u01, std::normal_distribution<float> &n01, rng_type &rng, Point x, Point n) const {
        int distNum = u01(rng) * figures_.size();
        if (std::holds_alternative<BoxLight>(figures_[distNum])) {
            return std::get<BoxLight>(figures_[distNum]).sample(u01, rng, x, n);
        } else if (std::holds_alternative<EllipsoidLight>(figures_[distNum])) {
            return std::get<EllipsoidLight>(figures_[distNum]).sample(n01, rng, x, n);
        } else {
            return std::get<TriangleLight>(figures_[distNum]).sample(u01, rng, x, n);
        }
    }

    float pdf(Point x, Point n, Point d) const {
        return getTotalPdf(0, x, n, d) / figures_.size();
    }

    bool isEmpty() const {
        return figures_.empty();
    }

private:
    float pdfOneFigureLight(const std::variant<BoxLight, EllipsoidLight, TriangleLight> &figureLight, Point x, Point n, Point d) const {
        (void) n;

        const Figure &figure = std::holds_alternative<BoxLight>(figureLight) ? std::get<BoxLight>(figureLight).figure :
                               (std::holds_alternative<EllipsoidLight>(figureLight) ? std::get<EllipsoidLight>(figureLight).figure :
                                std::get<TriangleLight>(figureLight).figure);

        auto firstIntersection = figure.intersect(Ray(x, d));
        if (!firstIntersection.has_value()) {
            return 0.;
        }
        auto [t, yn, _] = firstIntersection.value();
        if (std::isnan(t)) { // Shouldn't happen actually...
            return INFINITY;
        }
        Point y = x + t * d;
        float ans = std::holds_alternative<BoxLight>(figureLight) ? std::get<BoxLight>(figureLight).pdfOne(x, d, y, yn) :
                    (std::holds_alternative<EllipsoidLight>(figureLight) ? std::get<EllipsoidLight>(figureLight).pdfOne(x, d, y, yn) :
                     std::get<TriangleLight>(figureLight).pdfOne(x, d, y, yn));

        if (figure.type == FigureType::TRIANGLE) {
            return ans;
        }

        auto secondIntersection = figure.intersect(Ray(x + (t + eps) * d, d));
        if (!secondIntersection.has_value()) {
            return ans;
        }
        auto [t2, yn2, __] = secondIntersection.value();
        Point y2 = x + (t + eps + t2) * d;
        return ans + (std::holds_alternative<BoxLight>(figureLight) ? std::get<BoxLight>(figureLight).pdfOne(x, d, y2, yn2) :
                      (std::holds_alternative<EllipsoidLight>(figureLight) ? std::get<EllipsoidLight>(figureLight).pdfOne(x, d, y2, yn2) :
                       std::get<TriangleLight>(figureLight).pdfOne(x, d, y2, yn2)));
    }

    float getTotalPdf(uint32_t pos, const Point &x, const Point &n, const Point &d) const {
        Ray ray(x, d);
        const BvhNode &cur = bvh.nodes[pos];
        auto intersection = cur.aabb.intersect(ray);
        if (!intersection.has_value()) {
            return 0;
        }

        if (cur.left == 0) {
            float result = 0;
            for (uint32_t i = cur.first; i < cur.last; i++) {
                result += pdfOneFigureLight(figures_[i], x, n, d);
            }
            return result;
        }

        return getTotalPdf(cur.left, x, n, d) + getTotalPdf(cur.right, x, n, d);
    }
};

class Mix {
private:
    std::vector<std::variant<Cosine, FiguresMix>> components;

public:
    Mix() {}
    Mix(const std::vector<std::variant<Cosine, FiguresMix>> &components): components(components) {}

    Point sample(std::uniform_real_distribution<float> &u01, std::normal_distribution<float> &n01, rng_type &rng, Point x, Point n) const {
        int distNum = u01(rng) * components.size();
        if (std::holds_alternative<Cosine>(components[distNum])) {
            return std::get<Cosine>(components[distNum]).sample(n01, rng, x, n);
        } else {
            return std::get<FiguresMix>(components[distNum]).sample(u01, n01, rng, x, n);
        }
    }

    float pdf(Point x, Point n, Point d) const {
        float ans = 0;
        for (const auto &component : components) {
            if (std::holds_alternative<Cosine>(component)) {
                ans += std::get<Cosine>(component).pdf(x, n, d);
            } else {
                ans += std::get<FiguresMix>(component).pdf(x, n, d);
            }
        }
        return ans / components.size();
    }
};