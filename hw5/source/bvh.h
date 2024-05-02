#pragma once

#include "figure.h"
#include "rotation.h"
#include <cassert>
#include <iostream>

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

    std::optional<std::pair<Intersection, int>> intersect(const std::vector<Figure> &figures, const Ray &ray, std::optional<float> curBest) const {
        return intersect_(figures, root, ray, curBest);
    }

private:
    std::pair<float, uint32_t> bestSplit(std::vector<Figure> &figures, uint32_t first, uint32_t last) const {
        std::vector<float> scores(last - first, 0);
        AABB prefixAABB(figures[first]);
        for (size_t i = 1; i < last - first; i++) {
            scores[i] = prefixAABB.area() * i;
            prefixAABB.extend(figures[first + i]);
        }

        AABB suffixAABB(figures[last - 1]);
        for (size_t i = last - first - 1; i >= 1; i--) {
            scores[i] += suffixAABB.area() * ((last - first) - i);
            suffixAABB.extend(figures[first + i - 1]);
        }
        std::pair<float, uint32_t> ans = {scores[1], first + 1};
        for (size_t i = 2; i < last - first; i++) {
            if (scores[i] < ans.first) {
                ans = {scores[i], first + i};
            }
        }
        return ans;
    }

    void half(std::vector<Figure> &figures, uint32_t first, uint32_t last, short axis) const {
        auto cmp = axis == 0 ? [](const Figure &lhs, const Figure &rhs) { return lhs.position.x < rhs.position.x; } :
                   (axis == 1 ? [](const Figure &lhs, const Figure &rhs) { return lhs.position.y < rhs.position.y; } :
                    [](const Figure &lhs, const Figure &rhs) { return lhs.position.z < rhs.position.z; });
        std::sort(figures.begin() + first, figures.begin() + last, cmp);
    }

    uint32_t build(std::vector<Figure> &figures, uint32_t first, uint32_t last) {
        Node cur = Node(first, last);
        AABB aabb{};
        if (first < last) {
            aabb = AABB(figures[first]);
        }
        for (uint32_t i = first + 1; i < last; i++) {
            aabb.extend(figures[i]);
        }
        cur.aabb = aabb;
        uint32_t thisPos = nodes.size();
        nodes.push_back(cur);
        if (last - first <= 1) {
            return thisPos;
        }

        half(figures, first, last, 0);
        auto splitX = bestSplit(figures, first, last);
        half(figures, first, last, 1);
        auto splitY = bestSplit(figures, first, last);
        half(figures, first, last, 2);
        auto splitZ = bestSplit(figures, first, last);

        float bestResult = std::min(splitX.first, std::min(splitY.first, splitZ.first));
        if (bestResult >= aabb.area() * (last - first)) {
            return thisPos;
        }

        uint32_t mid;
        if (bestResult == splitX.first) {
            mid = splitX.second;
            half(figures, first, last, 0);
        } else if (bestResult == splitY.first) {
            mid = splitY.second;
            half(figures, first, last, 1);
        } else {
            mid = splitZ.second;
            half(figures, first, last, 2);
        }
        nodes[thisPos].left = build(figures, first, mid);
        nodes[thisPos].right = build(figures, mid, last);
        return thisPos;
    }

    std::optional<std::pair<Intersection, int>> intersect_(const std::vector<Figure> &figures, uint32_t pos, const Ray &ray, std::optional<float> curBest) const {
        const Node &cur = nodes[pos];
        auto intersection = cur.aabb.intersect(ray);
        if (!intersection.has_value()) {
            return {};
        }
        auto [t, _, isInside] = intersection.value();
        if (curBest.has_value() && curBest.value() < t && !isInside) {
            return {};
        }

        std::optional<std::pair<Intersection, int>> bestIntersection = {};
        if (cur.left == 0) {
            for (uint32_t i = cur.first; i < cur.last; i++) {
                auto curIntersection = figures[i].intersect(ray);
                if (curIntersection.has_value() && (!bestIntersection.has_value() || curIntersection.value().t < bestIntersection.value().first.t)) {
                    bestIntersection = {curIntersection.value(), i};
                }
            }
            return bestIntersection;
        }
        auto leftIntersection = intersect_(figures, cur.left, ray, curBest);
        bestIntersection = leftIntersection;
        if (leftIntersection.has_value() && (!curBest.has_value() || leftIntersection.value().first.t < curBest.value())) {
            curBest = leftIntersection.value().first.t;
        }
        auto rightIntersection = intersect_(figures, cur.right, ray, curBest);
        if (rightIntersection.has_value() && (!bestIntersection.has_value() || rightIntersection.value().first.t < bestIntersection.value().first.t)) {
            bestIntersection = rightIntersection;
        }
        return bestIntersection;
    }
};
