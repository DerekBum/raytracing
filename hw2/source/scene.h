#ifndef HW1_SCENE_H
#define HW1_SCENE_H

#include <iostream>
#include <memory>
#include <vector>
#include "color.h"
#include "point.h"
#include "figure.h"
#include "light.h"

class Scene {
public:
    int width{}, height{};
    float cameraFovX{};
    Color bgColor;
    Point camPos{}, camRight{}, camUp{}, camForward{};
    std::vector <Figure*> figures;

    int rayDepth{};
    Color ambientLight;
    std::vector <Light*> lights;

    Scene() = default;

    void render(std::ostream &out) const;
    Color getPixelColor(Ray ray, int bounceNum) const;

    std::pair<intersectPoint, int> findIntersection(Ray ray, float lower_bound) const;
};

Scene loadSceneFromFile(std::istream &in);

#endif //HW1_SCENE_H
