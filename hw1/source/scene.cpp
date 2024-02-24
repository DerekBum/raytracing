#include "scene.h"
#include <string>
#include <sstream>
#include <cmath>

Scene loadSceneFromFile(std::istream &in) {
    Scene scene;

    std::string line;
    while (getline(in, line)) {
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

            Figure* figure;

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
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
        }
    }

    return scene;
}

void Scene::render(std::ostream &out) const {
    out << "P6\n";
    out << width << " " << height << '\n';
    out << 255 << '\n';
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            Color pixel = bgColor;
            float dir = -1;
            for (auto &figure : figures) {
                float tan_x = std::tan(cameraFovX / 2);
                float tan_y = tan_x * float(height) / float(width);

                float cx = 2.0 * (x + 0.5) / width - 1.0;
                float cy = 2.0 * (y + 0.5) / height - 1.0;

                float real_x = tan_x * cx;
                float real_y = tan_y * cy;

                auto intersection = figure->intersect({camPos, real_x * camRight - real_y * camUp + camForward});
                if (intersection.has_value()) {
                    if (dir == -1 || intersection.value().first < dir) {
                        dir = intersection.value().first;
                        pixel = intersection.value().second;
                    }
                }
            }

            char res_pixel[3] = {char(pixel.r), char(pixel.g), char(pixel.b)};
            out.write((char *) res_pixel, 3);
        }
    }
}
