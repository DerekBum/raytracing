#include "scene.h"
#include <string>
#include <sstream>
#include <cmath>
#include <random>
#include <thread>
#include <mutex>

static std::minstd_rand rnd;
static std::uniform_real_distribution<float> u01(0.0, 1.0);
static std::normal_distribution<float> n01(0.0, 1.0);

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
            } else if (command == "EMISSION") {
                auto last_f = scene.figures.back();
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

            if (fabsf(sinTheta) > 1.0) {
                return {intersectedObject->emission.r + reflectedColor.r, intersectedObject->emission.g + reflectedColor.g,
                        intersectedObject->emission.b + reflectedColor.b};
            }

            float reflectivityCoefficient = pow((eta1 - eta2) / (eta1 + eta2), 2.0);
            float reflectivity = reflectivityCoefficient + (1.0 - reflectivityCoefficient) * pow(1.0 - (normal * incidentDirection), 5.0);

            if (u01(rnd) < reflectivity) {
                return {intersectedObject->emission.r + reflectedColor.r, intersectedObject->emission.g + reflectedColor.g,
                        intersectedObject->emission.b + reflectedColor.b};
            }

            float cosTheta = sqrt(1.0 - sinTheta * sinTheta);
            reflectionDirection = eta1 / eta2 * (-1.0 * incidentDirection) + (eta1 / eta2 * (normal * incidentDirection) - cosTheta) * normal;
            reflectionRay = Ray(ray.o + point * ray.d + 0.0001 * reflectionDirection, reflectionDirection);
            reflectedColor = getPixelColor(reflectionRay, bounceNum - 1);

            if (!insideObject) {
                reflectedColor = reflectedColor * intersectedObject->color;
            }

            return {intersectedObject->emission.r + reflectedColor.r, intersectedObject->emission.g + reflectedColor.g,
                    intersectedObject->emission.b + reflectedColor.b};
        }

        auto rec_color = intersectedObject->color * reflectedColor;
        return {intersectedObject->emission.r + rec_color.r, intersectedObject->emission.g + rec_color.g,
                intersectedObject->emission.b + rec_color.b};
    } else {
        Point w = Point(n01(rnd), n01(rnd), n01(rnd)).normalize();
        if (w * normal < 0) {
            w = -1.0 * w;
        }

        Point p = ray.o + point * ray.d;
        Ray wR = Ray(p + 0.0001 * w, w);

        auto rec_color = 2 * (w * normal) * intersectedObject->color * getPixelColor(wR, bounceNum - 1);
        return {intersectedObject->emission.r + rec_color.r, intersectedObject->emission.g + rec_color.g,
                     intersectedObject->emission.b + rec_color.b};
    }
}

void Scene::render(std::ostream &out) const {
    out << "P6\n";
    out << width << " " << height << '\n';
    out << 255 << '\n';

    int fast_num = std::thread::hardware_concurrency();

    int num_threads = std::min(samples, fast_num);
    int chunk_size = samples / num_threads;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            Color pixel {0, 0, 0};

            std::vector <std::thread> threads;
            std::mutex mutex;
            for (int i = 0; i < samples; i+=chunk_size) {
                threads.emplace_back([&, i]{
                    Color in_pixel;

                    for (int j = i; j < std::min(i + chunk_size, samples); j++) {
                        float nx = x + u01(rnd);
                        float ny = y + u01(rnd);

                        float tan_x = std::tan(cameraFovX / 2);
                        float tan_y = tan_x * float(height) / float(width);

                        float cx = 2.0 * (nx + 0.5) / width - 1.0;
                        float cy = 2.0 * (ny + 0.5) / height - 1.0;

                        float real_x = tan_x * cx;
                        float real_y = tan_y * cy;

                        Ray real_ray = Ray(camPos, real_x * camRight - real_y * camUp + camForward);

                        auto from_figures = getPixelColor(real_ray, rayDepth);

                        in_pixel = Color(in_pixel.r + from_figures.r, in_pixel.g + from_figures.g, in_pixel.b + from_figures.b);
                    }

                    mutex.lock();
                    pixel = Color(pixel.r + in_pixel.r, pixel.g + in_pixel.g, pixel.b + in_pixel.b);
                    mutex.unlock();
                });
            }
            for (auto& el : threads)
                el.join();

            pixel = (1.0 / samples) * pixel;

            pixel = gamma(aces(pixel));

            char res_pixel[3] = {char(std::round(255 * pixel.r)), char(std::round(255 * pixel.g)), char(std::round(255 * pixel.b))};
            out.write((char *) res_pixel, 3);
        }
    }
}
