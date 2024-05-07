#include "scene.h"
#include "distribution.h"
#include "istreamwrapper.h"
#include "gltf.h"
#include <string>
#include <sstream>
#include <cmath>
#include <random>
#include <thread>
#include <fstream>
#include <filesystem>

void loadBuffers(std::string_view gltfFilename, const rapidjson::Document &gltfScene, Scene &scene) {
    const auto &bufferSpecs = gltfScene["buffers"].GetArray();
    for (const auto &bufSpec : bufferSpecs) {
        size_t sz = bufSpec["byteLength"].GetUint();
        Buffer buf(sz);

        const auto gltfFilePath = std::filesystem::path(gltfFilename);
        const auto bufferFilePath = gltfFilePath.parent_path().append(bufSpec["uri"].GetString());
        std::ifstream bufferStream(bufferFilePath, std::ios::binary);
        bufferStream.read(buf.data(), sz);
        scene.buffers.push_back(buf);
    }
}

void loadBufferViews(const rapidjson::Document &gltfScene, Scene &scene) {
    const auto &bufferViewSpecs = gltfScene["bufferViews"].GetArray();
    for (const auto &bufferViewSpec : bufferViewSpecs) {
        scene.bufferViews.push_back(BufferView{
                bufferViewSpec["buffer"].GetUint(),
                bufferViewSpec["byteLength"].GetUint(),
                bufferViewSpec["byteOffset"].GetUint(),
        });
    }
}

void loadNodes(const rapidjson::Document &gltfScene, Scene &scene) {
    const auto &nodes = gltfScene["nodes"].GetArray();
    for (const auto &node : nodes) {
        Node curNode;
        if (node.HasMember("mesh")) {
            curNode.mesh = node["mesh"].GetUint();
        }
        if (node.HasMember("camera")) {
            curNode.camera = node["camera"].GetUint();
        }
        if (node.HasMember("rotation")) {
            auto rotation = node["rotation"].GetArray();
            curNode.rotation = {rotation[0].GetFloat(), rotation[1].GetFloat(), rotation[2].GetFloat(), rotation[3].GetFloat()};
        }
        if (node.HasMember("translation")) {
            auto translation = node["translation"].GetArray();
            curNode.translation = {translation[0].GetFloat(), translation[1].GetFloat(), translation[2].GetFloat()};
        }
        if (node.HasMember("scale")) {
            auto scale = node["scale"].GetArray();
            curNode.scale = {scale[0].GetFloat(), scale[1].GetFloat(), scale[2].GetFloat()};
        }
        if (node.HasMember("children")) {
            auto children = node["children"].GetArray();
            for (const auto &child : children) {
                curNode.children.push_back(child.GetUint());
            }
        }
        if (node.HasMember("matrix")) {
            auto matrix = node["matrix"].GetArray();
            float transition[4][4];
            for (size_t i = 0; i < 16; i++) {
                transition[i % 4][i / 4] = matrix[i].GetFloat();
            }
            curNode.transform = Transform(transition);
        }
        if (!curNode.transform.has_value()) {
            curNode.transform = Transform(curNode.translation, curNode.rotation, curNode.scale);
        }
        scene.nodes.push_back(curNode);
    }
}

void loadMeshes(const rapidjson::Document &gltfScene, Scene &scene) {
    const auto &meshes = gltfScene["meshes"].GetArray();
    for (const auto &mesh : meshes) {
        Mesh curMesh;
        for (const auto &primitive : mesh["primitives"].GetArray()) {
            curMesh.primitives.push_back(Primitive{
                    primitive["attributes"]["POSITION"].GetUint(),
                    primitive["indices"].GetUint(),
                    primitive["material"].GetUint()
            });
        }
        scene.meshes.push_back(curMesh);
    }
}

void loadAccessors(const rapidjson::Document &gltfScene, Scene &scene) {
    const auto &accessors = gltfScene["accessors"].GetArray();
    for (const auto &accessor : accessors) {
        auto curAccessor = Accessor{
                .bufferView = accessor["bufferView"].GetUint(),
                .count = accessor["count"].GetUint(),
                .componentType = accessor["componentType"].GetUint(),
                .type = accessor["type"].GetString(),
                .byteOffset = 0
        };
        if (accessor.HasMember("byteOffset")) {
            curAccessor.byteOffset = accessor["byteOffset"].GetFloat();
        }
        scene.accessors.push_back(curAccessor);
    }
}

void restoreNodeParents(Scene &scene) {
    for (size_t i = 0; i < scene.nodes.size(); i++) {
        for (const auto &child : scene.nodes[i].children) {
            scene.nodes[child].parentNode = i;
        }
    }
}

void calculateTransitions(Scene &scene) {
    for (auto &node : scene.nodes) {
        node.totalTransform = node.transform.value();
        auto par = node.parentNode;
        while (par.has_value()) {
            node.totalTransform = scene.nodes[par.value()].transform.value().compose(node.totalTransform);
            par = scene.nodes[par.value()].parentNode;
        }
    }
}

std::vector<Point> loadPositions(size_t positionsIndex, Scene &scene) {
    std::vector<Point> positions;
    const auto &accessor = scene.accessors[positionsIndex];
    const auto &bufferView = scene.bufferViews[accessor.bufferView];
    const auto &buffer = scene.buffers[bufferView.buffer];
    if (accessor.type != "VEC3") {
        std::cerr << "Load positions accessor: " << accessor.type << std::endl;
    }
    size_t byteOffset = bufferView.byteOffset + accessor.byteOffset;
    for (size_t i = 0; i < accessor.count; i++) {
        Point position;
        position.x = *(reinterpret_cast<const float*>(buffer.data() + byteOffset + 12 * i));
        position.y = *(reinterpret_cast<const float*>(buffer.data() + byteOffset + 12 * i + 4));
        position.z = *(reinterpret_cast<const float*>(buffer.data() + byteOffset + 12 * i + 8));
        positions.push_back(position);
    }
    return positions;
}

void loadMaterials(const rapidjson::Document &gltfScene, Scene &scene) {
    const auto &materials = gltfScene["materials"].GetArray();
    for (const auto &material : materials) {
        GltfMaterial curMaterial;
        if (material.HasMember("pbrMetallicRoughness")) {
            if (material["pbrMetallicRoughness"].HasMember("baseColorFactor")) {
                auto color = material["pbrMetallicRoughness"]["baseColorFactor"].GetArray();
                curMaterial.color = Color{
                        color[0].GetFloat(),
                        color[1].GetFloat(),
                        color[2].GetFloat()
                };
                curMaterial.alpha = color[3].GetFloat();
            }
            if (material["pbrMetallicRoughness"].HasMember("metallicFactor")) {
                curMaterial.metallicFactor = material["pbrMetallicRoughness"]["metallicFactor"].GetFloat();
            }
        }
        if (material.HasMember("emissiveFactor")) {
            const auto emission = material["emissiveFactor"].GetArray();
            curMaterial.emission = Color{
                    emission[0].GetFloat(),
                    emission[1].GetFloat(),
                    emission[2].GetFloat()
            };
        }
        if (material.HasMember("extensions") && material["extensions"].HasMember("KHR_materials_emissive_strength")) {
            float emissionFactor = material["extensions"]["KHR_materials_emissive_strength"]["emissiveStrength"].GetFloat();
            curMaterial.emission = emissionFactor * curMaterial.emission;
        }
        if (curMaterial.alpha < 1) {
            curMaterial.material = Material::DIELECTRIC;
        } else if (curMaterial.metallicFactor > 0) {
            curMaterial.material = Material::METALLIC;
        }
        scene.materials.push_back(curMaterial);
    }
}

void loadFigures(size_t indicesIndex, const Transform &transition, size_t material, const std::vector<Point> &positions, Scene &scene) {
    std::vector<Figure> figures;
    const auto &accessor = scene.accessors[indicesIndex];
    const auto &bufferView = scene.bufferViews[accessor.bufferView];
    const auto &buffer = scene.buffers[bufferView.buffer];
    if (accessor.type != "SCALAR") {
        std::cerr << "Load figures accessor: " << accessor.type << std::endl;
    }
    auto materialValue = scene.materials[material];
    for (size_t i = 0; i < accessor.count; i += 3) {
        size_t pos1, pos2, pos3;
        if (accessor.componentType == 5123) {
            pos1 = *(reinterpret_cast<const uint16_t*>(buffer.data() + bufferView.byteOffset + 2 * i));
            pos2 = *(reinterpret_cast<const uint16_t*>(buffer.data() + bufferView.byteOffset + 2 * (i + 1)));
            pos3 = *(reinterpret_cast<const uint16_t*>(buffer.data() + bufferView.byteOffset + 2 * (i + 2)));
        } else {
            if (accessor.componentType != 5125) {
                std::cerr << "Unexpected accessor component type: " << accessor.componentType << std::endl;
            }
            pos1 = *(reinterpret_cast<const uint32_t*>(buffer.data() + bufferView.byteOffset + 4 * i));
            pos2 = *(reinterpret_cast<const uint32_t*>(buffer.data() + bufferView.byteOffset + 4 * (i + 1)));
            pos3 = *(reinterpret_cast<const uint32_t*>(buffer.data() + bufferView.byteOffset + 4 * (i + 2)));
        }
        Point p1 = transition.apply(positions[pos1]);
        Point p2 = transition.apply(positions[pos2]);
        Point p3 = transition.apply(positions[pos3]);
        Figure fig(FigureType::TRIANGLE, p1, p3, p2);
        fig.material = materialValue;
        scene.figures.push_back(fig);
    }
}

void loadFiguresFromNodes(Scene &scene) {
    for (const auto &node : scene.nodes) {
        if (!node.mesh.has_value()) {
            continue;
        }
        const auto &mesh = node.mesh.value();
        for (const auto &primitive : scene.meshes[mesh].primitives) {
            auto positions = loadPositions(primitive.positions, scene);
            loadFigures(primitive.indices, node.totalTransform, primitive.material, positions, scene);
        }
    }
}

void loadCameraPosition(const rapidjson::Document &gltfScene, Scene &scene) {
    scene.camUp = {0, 1, 0};
    scene.camForward = {0, 0, -1};
    scene.camRight = {1, 0, 0};

    for (const auto &node : scene.nodes) {
        if (node.camera.has_value()) {
            std::cerr << "Detected camera" << std::endl;
            scene.cameraFovX = gltfScene["cameras"].GetArray()[node.camera.value()]["perspective"]["yfov"].GetFloat();
            scene.camPos = node.totalTransform.apply({0, 0, 0});
            scene.camUp = node.totalTransform.apply(scene.camUp) - scene.camPos;
            scene.camRight = node.totalTransform.apply(scene.camRight) - scene.camPos;
            scene.camForward = node.totalTransform.apply(scene.camForward) - scene.camPos;
        }
    }
}

Scene loadSceneFromFile(std::string_view gltfFile) {
    Scene scene;

    std::ifstream in(gltfFile.data(), std::ios_base::binary);

    rapidjson::IStreamWrapper isw(in);
    rapidjson::Document gltfScene;
    gltfScene.ParseStream(isw);

    loadBuffers(gltfFile, gltfScene, scene);
    loadBufferViews(gltfScene, scene);
    loadNodes(gltfScene, scene);
    restoreNodeParents(scene);
    calculateTransitions(scene);
    loadMeshes(gltfScene, scene);
    loadAccessors(gltfScene, scene);
    loadMaterials(gltfScene, scene);
    loadFiguresFromNodes(scene);
    loadCameraPosition(gltfScene, scene);

    /*std::string line;
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
    }*/

    scene.bvhble = std::partition(scene.figures.begin(), scene.figures.end(), [](const auto &elem) {
        return elem.type != FigureType::PLANE;
    }) - scene.figures.begin();
    scene.bvh = BVH(scene.figures, scene.bvhble);

    auto lightDistribution = FiguresMix(scene.figures);
    std::vector<std::variant<Cosine, FiguresMix>> finalDistributions;
    finalDistributions.push_back(Cosine());
    if (!lightDistribution.isEmpty()) {
        finalDistributions.push_back(lightDistribution);
    }
    scene.distribution = Mix(finalDistributions);

    return scene;
}

std::optional<std::pair<Intersection, int>> Scene::intersect(const Ray &ray) const {
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

Color Scene::getColor(std::uniform_real_distribution<float> &u01, std::normal_distribution<float> &n01, rng_type &rng, const Ray &ray, int recLimit) const {
    if (recLimit == 0) {
        return {0., 0., 0.};
    }

    auto intersection_ = intersect(ray);
    if (!intersection_.has_value()) {
        return bgColor;
    }

    auto [intersection, figurePos] = intersection_.value();
    auto [t, norma, is_inside] = intersection;
    auto figurePtr = figures.begin() + figurePos;
    auto x = ray.o + t * ray.d;

    if (figurePtr->material.material == Material::DIFFUSE) {
        Point d = distribution.sample(u01, n01, rng, x + eps * norma, norma);
        if (d.dot(norma) < 0) {
            return figurePtr->material.emission;
        }
        float pdf = distribution.pdf(x + eps * norma, norma, d);
        Ray dRay = Ray(x + eps * d, d);
        return figurePtr->material.emission + 1. / (PI * pdf) * d.dot(norma) * figurePtr->material.color * getColor(u01, n01, rng, dRay, recLimit - 1);
    } else if (figurePtr->material.material == Material::METALLIC) {
        Point reflectedDir = ray.d.normalize() - 2. * norma.dot(ray.d.normalize()) * norma;
        Ray reflected = Ray(ray.o + t * ray.d + eps * reflectedDir, reflectedDir);
        return figurePtr->material.emission + figurePtr->material.color * getColor(u01, n01, rng, reflected, recLimit - 1);
    } else {
        Point reflectedDir = ray.d.normalize() - 2. * norma.dot(ray.d.normalize()) * norma;
        Ray reflected = Ray(ray.o + t * ray.d + eps * reflectedDir, reflectedDir);
        Color reflectedColor = getColor(u01, n01, rng, reflected, recLimit - 1);

        float eta1 = 1., eta2 = figurePtr->material.ior;
        if (is_inside) {
            std::swap(eta1, eta2);
        }

        Point l = -1. * ray.d.normalize();
        float sinTheta2 = eta1 / eta2 * sqrt(1 - norma.dot(l) * norma.dot(l));
        if (fabs(sinTheta2) > 1.) {
            return figurePtr->material.emission + reflectedColor;
        }

        float r0 = pow((eta1 - eta2) / (eta1 + eta2), 2.);
        float r = r0 + (1 - r0) * pow(1 - norma.dot(l), 5.);
        if (u01(rng) < r) {
            return figurePtr->material.emission + reflectedColor;
        }

        float cosTheta2 = sqrt(1 - sinTheta2 * sinTheta2);
        Point refractedDir = eta1 / eta2 * (-1. * l) + (eta1 / eta2 * norma.dot(l) - cosTheta2) * norma;
        Ray refracted = Ray(ray.o + t * ray.d + eps * refractedDir, refractedDir);
        Color refractedColor = getColor(u01, n01, rng, refracted, recLimit - 1);
        if (!is_inside) {
            refractedColor = refractedColor * figurePtr->material.color;
        }
        return figurePtr->material.emission + refractedColor;
    }
}

Ray Scene::getRay(float x, float y) const {
    float tan_y = tan(cameraFovX / 2);
    float tan_x = tan_y * width / height;

    float cx = 2.0 * x / width - 1.0;
    float cy = 2.0 * y / height - 1.0;

    float real_x = tan_x * cx;
    float real_y = tan_y * cy;

    return {camPos, real_x * camRight - real_y * camUp + camForward};
}

Color Scene::getPixel(rng_type &rng, int x, int y) const {
    std::uniform_real_distribution<float> u01(0.0, 1.0);
    std::normal_distribution<float> n01(0.0, 1.0);

    Color pixel{0, 0, 0};

    for (int i = 0; i < samples; i++) {
        float nx = x + u01(rng);
        float ny = y + u01(rng);

        pixel = pixel + getColor(u01, n01, rng, getRay(nx, ny), rayDepth);
    }

    return (1.0 / samples) * pixel;
}

void Scene::render(std::string_view outFile) const {
    std::ofstream out(outFile.data(), std::ios::binary);

    out << "P6\n";
    out << width << ' ' << height << '\n';
    out << 255 << '\n';

    std::uniform_real_distribution<float> u01(0.0, 1.0);
    std::normal_distribution<float> n01(0.0, 1.0);

    std::array<uint8_t, 3> ans[height][width];

#pragma omp parallel for schedule(dynamic, 8)
    for (int iter = 0; iter < width * height; iter++) {
        int y = iter / width;
        int x = iter % width;

        rng_type rng(iter);

        auto pixel = gamma(aces(getPixel(rng, x, y)));

        ans[y][x] = {uint8_t(std::round(255 * pixel.x)), uint8_t(std::round(255 * pixel.y)),
                     uint8_t(std::round(255 * pixel.z))};
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            out.write((char *) ans[y][x].data(), 3);
        }
    }
}
