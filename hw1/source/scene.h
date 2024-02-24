#ifndef HW1_SCENE_H
#define HW1_SCENE_H

#include <iostream>
#include <memory>
#include <vector>
#include "color.h"
#include "point.h"
#include "figure.h"

class Scene {
public:
    int width{}, height{};
    float cameraFovX{};
    Color bgColor;
    Point camPos{}, camRight{}, camUp{}, camForward{};
    std::vector <Figure*> figures;

    Scene() = default;

    void render(std::ostream &out) const;
};

Scene loadSceneFromFile(std::istream &in);

#endif //HW1_SCENE_H
