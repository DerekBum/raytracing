#ifndef HW4_DISTRIBUTION_H
#define HW4_DISTRIBUTION_H

#include <random>
#include "point.h"
#include "figure.h"

class Distribution {
public:
    virtual Point sample(Point x, Point n) = 0;
    virtual float pdf(Point x, Point n, Point d) = 0;
};

class Cosine : public Distribution {
public:
    std::normal_distribution<float> n01{0.f, 1.f};
    std::minstd_rand &rng;
    explicit Cosine(std::minstd_rand &rng): rng(rng) {}

    Point sample(Point x, Point n) override;
    float pdf(Point x, Point n, Point d) override;
};

class FigureLight : public Distribution {
public:
    virtual float runPdf(Point x, Point d, Point y, Point yn) = 0;

    Figure &figure;
    explicit FigureLight(Figure &figure): figure(figure) {}

    float pdf(Point x, Point n, Point d) override;
};

class BoxLight : public FigureLight {
public:
    std::uniform_real_distribution<float> u01{0.0, 1.0};
    std::minstd_rand &rng;
    float result;

    BoxLight(std::minstd_rand &rng, Figure &box);

    float runPdf(Point x, Point d, Point y, Point yn) override;
    Point sample(Point x, Point n) override;
};

class EllipsoidLight : public FigureLight {
public:
    std::normal_distribution<float> n01{0.f, 1.f};
    std::minstd_rand &rng;

    EllipsoidLight(std::minstd_rand &rng, Figure &ellipsoid): FigureLight(ellipsoid), rng(rng) {}

    float runPdf(Point x, Point d, Point y, Point yn) override;
    Point sample(Point x, Point n) override;
};


class Mix : public Distribution {
public:
    std::uniform_real_distribution<float> u01{0.0, 1.0};
    std::minstd_rand &rng;
    std::vector<Distribution*> components;

    Mix(std::minstd_rand &rng, std::vector<Distribution*> &&components): rng(rng), components(std::move(components)) {}

    Point sample(Point x, Point n) override;
    float pdf(Point x, Point n, Point d) override;
};

#endif //HW4_DISTRIBUTION_H
