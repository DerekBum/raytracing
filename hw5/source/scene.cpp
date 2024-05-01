#include "scene.h"
#include "distribution.h"
#include <string>
#include <sstream>
#include <cmath>
#include <random>
#include <thread>
#include <mutex>

static std::minstd_rand rnd;

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

                Figure figure = Figure();

                if (name == "PLANE") {
                    float x, y, z;
                    ss2 >> x >> y >> z;
                    Point n(x, y, z);
                    figure.data = n;
                    figure.type = FigureType::PLANE;
                } else if (name == "ELLIPSOID") {
                    float x, y, z;
                    ss2 >> x >> y >> z;
                    Point r(x, y, z);
                    figure.data = r;
                    figure.type = FigureType::ELLIPSOID;
                } else if (name == "BOX") {
                    float x, y, z;
                    ss2 >> x >> y >> z;
                    Point s(x, y, z);
                    figure.data = s;
                    figure.type = FigureType::BOX;
                } else if (name == "TRIANGLE") {
                    float x, y, z;
                    ss2 >> x >> y >> z;
                    Point p1(x, y, z);
                    ss2 >> x >> y >> z;
                    Point p2(x, y, z);
                    ss2 >> x >> y >> z;
                    Point p3(x, y, z);
                    figure.data = p3;
                    figure.data2 = p2;
                    figure.data3 = p1;
                    figure.type = FigureType::TRIANGLE;
                } else {
                    std::cerr << "Unknown figure: " << name << std::endl;
                }

                scene.figures.push_back(figure);
            } else if (command == "POSITION") {
                auto last_f = &scene.figures.back();
                float x, y, z;
                ss >> x >> y >> z;
                last_f->position = Point(x, y, z);
            } else if (command == "ROTATION") {
                auto last_f = &scene.figures.back();
                float x, y, z, w;
                ss >> x >> y >> z >> w;
                last_f->rotation = Rotation(x, y, z, w);
            } else if (command == "COLOR") {
                auto last_f = &scene.figures.back();
                float r, g, b;
                ss >> r >> g >> b;
                last_f->color = Color(r, g, b);
            } else if (command == "METALLIC") {
                auto last_f = &scene.figures.back();
                last_f->material = Material::METALLIC;
            } else if (command == "DIELECTRIC") {
                auto last_f = &scene.figures.back();
                last_f->material = Material::DIELECTRIC;
            } else if (command == "IOR") {
                auto last_f = &scene.figures.back();
                float ior;
                ss >> ior;
                last_f->ior = ior;
            } else if (command == "EMISSION") {
                auto last_f = &scene.figures.back();
                float r, g, b;
                ss >> r >> g >> b;
                last_f->emission = Color(r, g, b);
            } else if (command == "RAY_DEPTH") {
                ss >> scene.rayDepth;
            } else if (command == "SAMPLES") {
                ss >> scene.samples;
            } else {
                std::cerr << "Unknown command: " << command << std::endl;
            }
            break;
        }
    }

    scene.bvhble = std::partition(scene.figures.begin(), scene.figures.end(), [](const auto &elem) {
        return elem.type != FigureType::PLANE;
    }) - scene.figures.begin();
    scene.bvh = BVH(scene.figures, scene.bvhble);

    auto lightDistribution = FiguresMix(scene.figures);
    std::vector<std::variant<Cosine, FiguresMix>> finalDistributions;
    finalDistributions.emplace_back(Cosine());
    if (!lightDistribution.isEmpty()) {
        finalDistributions.emplace_back(lightDistribution);
    }
    scene.distribution = Mix(finalDistributions);

    return scene;
}

std::optional<std::pair<Intersection, int>> Scene::findIntersection(Ray ray) const {
    std::optional<std::pair<Intersection, int>> bestIntersection = {};
    for (int i = bvhble; i < (int) figures.size(); i++) {
        auto intersection_ = figures[i].intersect(ray);
        if (intersection_.has_value()) {
            auto intersection = intersection_.value();
            if (!bestIntersection.has_value() || intersection.t < bestIntersection.value().first.t) {
                bestIntersection = {intersection, i};
            }
        }
    }
    std::optional<float> curBest = {};
    if (bestIntersection.has_value()) {
        curBest = bestIntersection.value().first.t;
    }
    auto bvhIntersection = bvh.intersect(figures, ray, curBest);
    if (bvhIntersection.has_value() && (!bestIntersection.has_value() || bvhIntersection.value().first.t < bestIntersection.value().first.t)) {
        bestIntersection = bvhIntersection;
    }
    return bestIntersection;
}

Color Scene::getPixelColor(std::uniform_real_distribution<float> u01, std::normal_distribution<float> n01, rng_type &rng, Ray ray, int bounceNum) const {
    if (bounceNum == 0)
        return {};

    auto intersectionResult = findIntersection(ray);

    if (!intersectionResult.has_value())
        return bgColor;

    auto [intersection, intersectedObjectIndex] = intersectionResult.value();

    auto normal = intersection.norma;
    auto point = intersection.t;
    auto insideObject = intersection.is_inside;
    auto intersectedObject = figures[intersectedObjectIndex];

    if (intersectedObject.material == Material::METALLIC || intersectedObject.material == Material::DIELECTRIC) {
        Point reflectionDirection = ray.d.normalize() - 2.0 * (normal * ray.d.normalize()) * normal;
        Ray reflectionRay(ray.o + point * ray.d + 0.0001 * reflectionDirection, reflectionDirection);
        Color reflectedColor = getPixelColor(u01, n01, rng, reflectionRay, bounceNum - 1);

        if (intersectedObject.material == Material::DIELECTRIC) {
            float eta1 = 1.0, eta2 = intersectedObject.ior;
            if (insideObject)
                std::swap(eta1, eta2);

            Point incidentDirection = -1.0 * ray.d.normalize();
            float sinTheta = eta1 / eta2 * sqrt(1.0 - (normal * incidentDirection) * (normal * incidentDirection));

            if (fabsf(sinTheta) > 1.0) {
                return intersectedObject.emission + reflectedColor;
            }

            float reflectivityCoefficient = pow((eta1 - eta2) / (eta1 + eta2), 2.0);
            float reflectivity = reflectivityCoefficient + (1.0 - reflectivityCoefficient) * pow(1.0 - (normal * incidentDirection), 5.0);

            if (u01(rnd) < reflectivity) {
                return intersectedObject.emission + reflectedColor;
            }

            float cosTheta = sqrt(1.0 - sinTheta * sinTheta);
            reflectionDirection = eta1 / eta2 * (-1.0 * incidentDirection) + (eta1 / eta2 * (normal * incidentDirection) - cosTheta) * normal;
            auto reflection = Ray(ray.o + point * ray.d + 0.0001 * reflectionDirection, reflectionDirection);
            reflectedColor = getPixelColor(u01, n01, rng, reflection, bounceNum - 1);

            if (!insideObject) {
                reflectedColor = reflectedColor * intersectedObject.color;
            }

            return intersectedObject.emission + reflectedColor;
        }

        auto rec_color = intersectedObject.color * reflectedColor;
        return intersectedObject.emission + rec_color;
    } else {
        Point p = ray.o + point * ray.d;

        Point w = distribution.sample(u01, n01, rng, p + 0.0001 * normal, normal);
        if (w * normal < 0) {
            return intersectedObject.emission;
        }

        float pdf = distribution.pdf(p + 0.0001 * normal, normal, w);
        Ray wR = Ray(p + 0.0001 * w, w);

        auto rec_color = 1.0 / (PI * pdf) * (w * normal) * intersectedObject.color * getPixelColor(u01, n01, rng, wR, bounceNum - 1);
        return intersectedObject.emission + rec_color;
    }
}

void Scene::render(std::ostream &out) const {
    out << "P6\n";
    out << width << " " << height << '\n';
    out << 255 << '\n';

    std::uniform_real_distribution<float> u01(0.0, 1.0);
    std::normal_distribution<float> n01(0.0, 1.0);

    std::array<char, 3> ans[height][width];

#pragma omp parallel for schedule(dynamic,8)
    for (int iter = 0; iter < width * height; iter++) {
        int y = iter / width;
        int x = iter % width;

        rng_type rng(iter);

        Color pixel{0, 0, 0};

        for (int i = 0; i < samples; i++) {
            float nx = x + u01(rnd);
            float ny = y + u01(rnd);

            float tan_x = std::tan(cameraFovX / 2);
            float tan_y = tan_x * float(height) / float(width);

            float cx = 2.0 * nx / width - 1.0;
            float cy = 2.0 * ny / height - 1.0;

            float real_x = tan_x * cx;
            float real_y = tan_y * cy;

            Ray real_ray = Ray(camPos, real_x * camRight - real_y * camUp + camForward);

            auto from_figures = getPixelColor(u01, n01, rng, real_ray, rayDepth);

            pixel = pixel + from_figures;
        }

        pixel = (1.0 / samples) * pixel;

        pixel = gamma(aces(pixel));

        ans[y][x] = {char(std::round(255 * pixel.r)), char(std::round(255 * pixel.g)),
                               char(std::round(255 * pixel.b))};
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            out.write((char *) ans[y][x].data(), 3);
        }
    }
}
