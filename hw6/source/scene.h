#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include "color.h"
#include "point.h"
#include "figure.h"
#include "distribution.h"
#include "bvh.h"
#include "document.h"
#include "gltf.h"

class Scene {
public:
    int width{}, height{};
    float cameraFovX{};
    Color bgColor{};
    Point camPos{}, camRight{}, camUp{}, camForward{};
    std::vector <Figure> figures;

    int rayDepth = 6;

    int samples{};

    Scene() = default;

    void render(std::string_view outFile) const;
    Color getColor(std::uniform_real_distribution<float> &u01, std::normal_distribution<float> &n01, rng_type &rng, const Ray &ray, int bounceNum) const;

    Mix distribution;

    BVH bvh;
    int bvhble{};

    std::optional<std::pair<Intersection, int>> intersect(const Ray &ray) const;

    Color getPixel(rng_type &rng, int x, int y) const;

    Ray getRay(float x, float y) const;

    std::vector <Buffer> buffers;
    std::vector <BufferView> bufferViews;
    std::vector <Node> nodes;
    std::vector <Mesh> meshes;
    std::vector <Accessor> accessors;
    std::vector <GltfMaterial> materials;
};

Scene loadSceneFromFile(std::string_view gltfFile);
