#include <fstream>
#include "scene.h"

using namespace std;

int main(int argc, const char *argv[]) {
    ifstream in(argv[1]);
    ofstream out(argv[2]);

    Scene scene = loadSceneFromFile(in);
    scene.render(out);
    return 0;
}
