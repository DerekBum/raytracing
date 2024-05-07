#include <fstream>
#include "scene.h"

using namespace std;

int main(int argc, const char *argv[]) {
    (void) argc;

    Scene scene = loadSceneFromFile(argv[1]);

    scene.width = strtol(argv[2], nullptr, 10);
    scene.height = strtol(argv[3], nullptr, 10);
    scene.samples = strtol(argv[4], nullptr, 10);

    scene.render(argv[5]);
    return 0;
}
