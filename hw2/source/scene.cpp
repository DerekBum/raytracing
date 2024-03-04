#include "scene.h"
#include <string>
#include <sstream>
#include <cmath>

Scene loadSceneFromFile(std::istream &in) {
    Scene scene;

    std::string line;
    while (getline(in, line)) {
        while (true) {
            std::string command;

            std::stringstream ss;
            ss << line;
            ss >> command;

            if (command == "DIMENSIONS") {
                ss >> scene.width >> scene.height;
            } else if (command == "BG_COLOR") {
                float r, g, b;
                ss >> r >> g >> b;
                scene.bgColor = Color(r, g, b);
            } else if (command == "CAMERA_POSITION") {
                float x, y, z;
                ss >> x >> y >> z;
                scene.camPos = Point(x, y, z);
            } else if (command == "CAMERA_RIGHT") {
                float x, y, z;
                ss >> x >> y >> z;
                scene.camRight = Point(x, y, z);
            } else if (command == "CAMERA_UP") {
                float x, y, z;
                ss >> x >> y >> z;
                scene.camUp = Point(x, y, z);
            } else if (command == "CAMERA_FORWARD") {
                float x, y, z;
                ss >> x >> y >> z;
                scene.camForward = Point(x, y, z);
            } else if (command == "CAMERA_FOV_X") {
                ss >> scene.cameraFovX;
            } else if (command == "NEW_PRIMITIVE") {
                getline(in, line);

                std::stringstream ss2;
                ss2 << line;
                std::string name;
                ss2 >> name;

                Figure *figure;

                if (name == "PLANE") {
                    float x, y, z;
                    ss2 >> x >> y >> z;
                    Point n(x, y, z);
                    figure = new Plane(n);
                } else if (name == "ELLIPSOID") {
                    float x, y, z;
                    ss2 >> x >> y >> z;
                    Point r(x, y, z);
                    figure = new Ellipsoid(r);
                } else if (name == "BOX") {
                    float x, y, z;
                    ss2 >> x >> y >> z;
                    Point s(x, y, z);
                    figure = new Box(s);
                } else {
                    std::cerr << "Unknown figure: " << name << std::endl;
                }

                scene.figures.push_back(figure);
            } else if (command == "POSITION") {
                auto last_f = scene.figures.back();
                float x, y, z;
                ss >> x >> y >> z;
                last_f->position = Point(x, y, z);
            } else if (command == "ROTATION") {
                auto last_f = scene.figures.back();
                float x, y, z, w;
                ss >> x >> y >> z >> w;
                last_f->rotation = Rotation(x, y, z, w);
            } else if (command == "COLOR") {
                auto last_f = scene.figures.back();
                float r, g, b;
                ss >> r >> g >> b;
                last_f->color = Color(r, g, b);
            } else if (command == "METALLIC") {
                auto last_f = scene.figures.back();
                last_f->material = Material::METALLIC;
            } else if (command == "DIELECTRIC") {
                auto last_f = scene.figures.back();
                last_f->material = Material::DIELECTRIC;
            } else if (command == "IOR") {
                auto last_f = scene.figures.back();
                float ior;
                ss >> ior;
                last_f->ior = ior;
            } else if (command == "RAY_DEPTH") {
                ss >> scene.rayDepth;
            } else if (command == "AMBIENT_LIGHT") {
                float r, g, b;
                ss >> r >> g >> b;
                scene.ambientLight = Color(r, g, b);
            } else if (command == "NEW_LIGHT") {
                Light *light;
                Color intensity;
                Point dir{}, pos{}, att{};
                bool directedLight = false;

                while (getline(in, line)) {

                    std::stringstream ss2;
                    ss2 << line;
                    std::string com;
                    ss2 >> com;

                    if (com == "LIGHT_INTENSITY") {
                        float r, g, b;
                        ss2 >> r >> g >> b;
                        intensity = Color(r, g, b);
                    } else if (com == "LIGHT_DIRECTION") {
                        float x, y, z;
                        ss2 >> x >> y >> z;
                        dir = Point(x, y, z);
                        directedLight = true;
                    } else if (com == "LIGHT_POSITION") {
                        float x, y, z;
                        ss2 >> x >> y >> z;
                        pos = Point(x, y, z);
                    } else if (com == "LIGHT_ATTENUATION") {
                        float x, y, z;
                        ss2 >> x >> y >> z;
                        att = Point(x, y, z);
                    } else {
                        break;
                    }
                }

                if (directedLight) {
                    light = new DirectedLight(intensity, dir);
                } else {
                    light = new IndirectLight(intensity, pos, att);
                }

                scene.lights.push_back(light);
                continue;
            } else {
                std::cerr << "Unknown command: " << command << std::endl;
            }
            break;
        }
    }

    return scene;
}

std::pair<intersectPoint, int> Scene::findIntersection(Ray ray, float lower_bound) const {
    intersectPoint resInt{};
    int dir = -1;
    for (int i = 0; i < figures.size(); i++) {
        auto intersection = figures[i]->intersect(ray);
        if (intersection.has_value()) {
            if (intersection.value().pt <= lower_bound && (dir == -1 || intersection.value().pt < resInt.pt)) {
                resInt = intersection.value();
                dir = i;
            }
        }
    }
    return {resInt, dir};
}

Color Scene::getPixelColor(Ray ray, int bounceNum) const {
    if (bounceNum == 0)
        return {};

    auto intersectionResult = findIntersection(ray, 1e9);
    auto intersection = intersectionResult.first;
    auto intersectedObjectIndex = intersectionResult.second;

    if (intersectedObjectIndex == -1)
        return bgColor;

    auto normal = intersection.norm;
    auto point = intersection.pt;
    auto insideObject = intersection.inner;
    auto intersectedObject = figures[intersectedObjectIndex];

    if (intersectedObject->material == Material::METALLIC || intersectedObject->material == Material::DIELECTRIC) {
        Point reflectionDirection = ray.d.normalize() - 2.0 * (normal * ray.d.normalize()) * normal;
        Ray reflectionRay(ray.o + point * ray.d + 0.0001 * reflectionDirection, reflectionDirection);
        Color reflectedColor = getPixelColor(reflectionRay, bounceNum - 1);

        if (intersectedObject->material == Material::DIELECTRIC) {
            float eta1 = 1.0, eta2 = intersectedObject->ior;
            if (insideObject)
                std::swap(eta1, eta2);

            Point incidentDirection = -1.0 * ray.d.normalize();
            float sinTheta = eta1 / eta2 * sqrt(1.0 - (normal * incidentDirection) * (normal * incidentDirection));

            if (fabsf(sinTheta) > 1.0)
                return reflectedColor;

            float cosTheta = sqrt(1.0 - sinTheta * sinTheta);
            reflectionDirection = eta1 / eta2 * (-1.0 * incidentDirection) + (eta1 / eta2 * (normal * incidentDirection) - cosTheta) * normal;
            reflectionRay = Ray(ray.o + point * ray.d + 0.0001 * reflectionDirection, reflectionDirection);
            reflectedColor = getPixelColor(reflectionRay, bounceNum - 1);

            if (!insideObject)
                reflectedColor = reflectedColor * intersectedObject->color;

            float reflectivityCoefficient = pow((eta1 - eta2) / (eta1 + eta2), 2.0);
            float reflectivity = reflectivityCoefficient + (1.0 - reflectivityCoefficient) * pow(1.0 - (normal * incidentDirection), 5.0);

            auto fir = reflectivity * reflectedColor;
            auto sec = (1.0 - reflectivity) * reflectedColor;

            return {fir.r + sec.r, fir.g + sec.g, fir.b + sec.b};
        }

        return intersectedObject->color * reflectedColor;
    } else {
        Color color = ambientLight;
        for (auto& light : lights) {
            Point p = ray.o + point * ray.d;
            auto lightResult = light->getLight(p);
            float lightReflection = lightResult.point * normal;
            auto intensity = lightReflection * lightResult.color;

            auto secondaryIntersection = findIntersection(Ray(p + 0.0001 * lightResult.point, lightResult.point), lightResult.r);
            if (lightReflection >= 0 && secondaryIntersection.second == -1) {
                color = Color(color.r + intensity.r, color.g + intensity.g, color.b + intensity.b);
            }
        }
        return intersectedObject->color * color;
    }
}

void Scene::render(std::ostream &out) const {
    out << "P6\n";
    out << width << " " << height << '\n';
    out << 255 << '\n';
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            float tan_x = std::tan(cameraFovX / 2);
            float tan_y = tan_x * float(height) / float(width);

            float cx = 2.0 * (x + 0.5) / width - 1.0;
            float cy = 2.0 * (y + 0.5) / height - 1.0;

            float real_x = tan_x * cx;
            float real_y = tan_y * cy;

            Ray real_ray = Ray(camPos, real_x * camRight - real_y * camUp + camForward);

            auto pixel = getPixelColor(real_ray, rayDepth);

            pixel = gamma(aces(pixel));

            char res_pixel[3] = {char(std::round(255 * pixel.r)), char(std::round(255 * pixel.g)), char(std::round(255 * pixel.b))};
            out.write((char *) res_pixel, 3);
        }
    }
}
