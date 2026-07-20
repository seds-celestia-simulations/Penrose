#include "core/Engine.h"
#include <cstdlib>

#ifdef _WIN32
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

int main() {
#ifndef _WIN32
    setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 1);
    setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 1);
#endif

    Engine app(1920, 1080, "Penrose: RK4 Black Hole");
    app.run();

    return 0;
}
