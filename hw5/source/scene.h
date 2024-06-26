#ifndef HW1_SCENE_H
#define HW1_SCENE_H

#include <iostream>
#include <memory>
#include <vector>
#include "color.h"
#include "point.h"
#include "figure.h"
#include "distribution.h"
#include "bvh.h"

class Scene {
public:
    int width{}, height{};
    float cameraFovX{};
    Color bgColor;
    Point camPos{}, camRight{}, camUp{}, camForward{};
    std::vector <Figure> figures;

    int rayDepth{};

    int samples{};

    Scene() = default;

    void render(std::ostream &out) const;
    Color getPixelColor(std::uniform_real_distribution<float> u01, std::normal_distribution<float> n01, rng_type &rng, Ray ray, int bounceNum) const;

    Mix distribution;

    BVH bvh;
    int bvhble;

    std::optional<std::pair<Intersection, int>> findIntersection(Ray ray) const;
};

Scene loadSceneFromFile(std::istream &in);

#endif //HW1_SCENE_H
