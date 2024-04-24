#pragma once

#include "point.h"
#include "figure.h"
#include "rotation.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>

class Node {
public:
    AABB aabb;
    uint32_t left = 0, right = 0, first, last;

    Node() {}
    Node(uint32_t first, uint32_t last): first(first), last(last) {}
};

class BVH {
public:
    std::vector<Node> nodes;
    uint32_t root;

    BVH() {}
    BVH(std::vector<Figure> &figures, uint32_t n) {
        root = build(figures, 0, n);
    }

    std::optional<std::pair<Intersection, int>> intersect(const std::vector<Figure> &figures, const Ray &ray,
                                                          std::optional<float> curBest) const {
        return intersectInner(figures, root, ray, curBest);
    }

    std::pair<float, uint32_t> bestSplit(std::vector<Figure> &figures, uint32_t first, uint32_t last) const;

    static void half(std::vector<Figure> &figures, uint32_t first, uint32_t last, int axis) ;

    uint32_t build(std::vector<Figure> &figures, uint32_t first, uint32_t last);

    std::optional<std::pair<Intersection, int>> intersectInner(const std::vector<Figure> &figures, uint32_t pos,
                                                               const Ray &ray, std::optional<float> curBest) const;
};
