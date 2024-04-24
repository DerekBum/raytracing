#pragma once
#include <random>
#include <cmath>
#include <memory>
#include <variant>
#include "point.h"
#include "figure.h"
#include "bvh.h"

typedef std::minstd_rand rng_type;

class Uniform {
public:
    Uniform() = default;

    Point sample(std::normal_distribution<float> &n01, rng_type &rng, Point x, Point n);

    float pdf(Point x, Point n, Point d) const;
};


class Cosine {
public:
    Cosine() {}

    Point sample(std::normal_distribution<float> &n01, rng_type &rng, Point x, Point n);

    float pdf(Point x, Point n, Point d) const;
};

class BoxLight {
public:
    float sTotal, sx, sy, sz, wx, wy, wz;
    const Figure figure;

    float pdfOne(Point x, Point d, Point y, Point yn) const;

    BoxLight(const Figure &box): figure(box) {
        sx = box.data.x;
        sy = box.data.y;
        sz = box.data.z;
        sTotal = 8 * (sy * sz + sx * sz + sx * sy);
        wx = sy * sz;
        wy = sx * sz;
        wz = sx * sy;
    }

    Point sample(std::uniform_real_distribution<float> &u01, rng_type &rng, Point x, Point n);
};

class TriangleLight {
public:
    float pointProb;
    const Figure figure;

    float pdfOne(Point x, Point d, Point y, Point yn) const;

    TriangleLight(const Figure &ellipsoid): figure(ellipsoid) {
        const Point &a = figure.data3;
        const Point &b = figure.data - a;
        const Point &c = figure.data2 - a;
        Point n = b.inter(c);
        pointProb = 1.0 / (0.5 * sqrt(n.len_square()));
    }

    Point sample(std::uniform_real_distribution<float> &u01, rng_type &rng, Point x, Point n);
};

class EllipsoidLight {
public:
    const Figure figure;

    float pdfOne(Point x, Point d, Point y, Point yn) const;

    EllipsoidLight(const Figure &ellipsoid): figure(ellipsoid) {}

    Point sample(std::normal_distribution<float> &n01, rng_type &rng, Point x, Point n);
};

class FiguresMix {
public:
    std::vector<std::variant<BoxLight, EllipsoidLight, TriangleLight>> figures_;
    BVH bvh;

    FiguresMix(std::vector<Figure> figures) {
        size_t n = std::partition(figures.begin(), figures.end(), [](const auto &fig) {
            if (fig.emission.r == 0 && fig.emission.g == 0 && fig.emission.b == 0) {
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

    Point sample(std::uniform_real_distribution<float> &u01, std::normal_distribution<float> &n01, rng_type &rng,
                 Point x, Point n);

    float pdf(Point x, Point n, Point d) const;

    bool isEmpty() const;

    float pdfLight(const std::variant<BoxLight, EllipsoidLight, TriangleLight> &figureLight,
                            Point x, Point n, Point d) const;

    float getTotalPdf(uint32_t pos, const Point &x, const Point &n, const Point &d) const;
};

class Mix {
public:
    std::vector<std::variant<Cosine, FiguresMix>> components;

    Mix() {}
    Mix(const std::vector<std::variant<Cosine, FiguresMix>> &components): components(components) {}

    Point sample(std::uniform_real_distribution<float> &u01, std::normal_distribution<float> &n01, rng_type &rng,
                 Point x, Point n);

    float pdf(Point x, Point n, Point d) const;
};