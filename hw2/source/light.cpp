#include <cmath>
#include "light.h"

Light::Light(Color intensity): intensity(intensity) {}

IndirectLight::IndirectLight(Color intensity, Point position, Point attenuation): Light(intensity), position(position), attenuation(attenuation) {}

lightInfo IndirectLight::getLight(Point point) const {
    Point direction = position - point;
    float r = std::sqrt(direction.len_square());

    float coeff = 1.0 / (attenuation.x + attenuation.y * r + attenuation.z * r * r);

    Color c = Color(intensity.r * coeff, intensity.g * coeff, intensity.b * coeff);
    return lightInfo{direction.normalize(), c, r};
}

DirectedLight::DirectedLight(Color intensity, Point direction): Light(intensity), direction(direction) {}

lightInfo DirectedLight::getLight(Point point) const {
    return lightInfo{direction.normalize(), intensity, 1. / 0.001};
}
