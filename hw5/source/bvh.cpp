#include "bvh.h"

std::pair<float, uint32_t> BVH::bestSplit(std::vector<Figure> &figures, uint32_t first, uint32_t last) const {
    std::vector<float> all(last - first, 0);
    AABB pref(figures[first]);
    for (size_t i = 1; i < last - first; i++) {
        all[i] = pref.area() * i;
        pref.extend(figures[first + i]);
    }

    AABB suff(figures[last - 1]);
    for (size_t i = last - first - 1; i >= 1; i--) {
        all[i] += suff.area() * ((last - first) - i);
        suff.extend(figures[first + i - 1]);
    }

    std::pair<float, uint32_t> ans = {all[1], first + 1};
    for (size_t i = 2; i < last - first; i++) {
        if (all[i] < ans.first) {
            ans = {all[i], first + i};
        }
    }
    return ans;
}

void BVH::half(std::vector<Figure> &figures, uint32_t first, uint32_t last, int axis) {
    auto cmp = axis == 0 ? [](const Figure &lhs, const Figure &rhs) { return lhs.position.x < rhs.position.x; } :
               (axis == 1 ? [](const Figure &lhs, const Figure &rhs) { return lhs.position.y < rhs.position.y; } :
                [](const Figure &lhs, const Figure &rhs) { return lhs.position.z < rhs.position.z; });
    std::sort(figures.begin() + first, figures.begin() + last, cmp);
}

uint32_t BVH::build(std::vector<Figure> &figures, uint32_t first, uint32_t last) {
    Node cur = Node(first, last);
    AABB aabb;
    if (first < last) {
        aabb = AABB(figures[first]);
        for (uint32_t i = first + 1; i < last; i++) {
            aabb.extend(figures[i]);
        }
    }
    cur.aabb = aabb;
    uint32_t thisPos = nodes.size();
    nodes.push_back(cur);
    if (last - first <= 1) {
        return thisPos;
    }

    for (int axis = 0; axis < 3; ++axis) {
        half(figures, first, last, axis);
        auto split = bestSplit(figures, first, last);
        float bestResult = split.first;
        if (bestResult >= aabb.area() * (last - first)) {
            continue;
        }
        uint32_t mid = split.second;
        nodes[thisPos].left = build(figures, first, mid);
        nodes[thisPos].right = build(figures, mid, last);
        return thisPos;
    }
    return thisPos;
}

std::optional<std::pair<Intersection, int>> BVH::intersectInner(const std::vector<Figure> &figures, uint32_t pos, const Ray &ray,
                                                                std::optional<float> curBest) const {
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
                bestIntersection = {curIntersection.value(), static_cast<int>(i)};
            }
        }
        return bestIntersection;
    }

    auto leftIntersection = intersectInner(figures, cur.left, ray, curBest);
    if (leftIntersection.has_value() && (!curBest.has_value() || leftIntersection.value().first.t < curBest.value())) {
        bestIntersection = leftIntersection;
        curBest = leftIntersection.value().first.t;
    }

    auto rightIntersection = intersectInner(figures, cur.right, ray, curBest);
    if (rightIntersection.has_value() && (!bestIntersection.has_value() || rightIntersection.value().first.t < bestIntersection.value().first.t)) {
        bestIntersection = rightIntersection;
    }

    return bestIntersection;
}
