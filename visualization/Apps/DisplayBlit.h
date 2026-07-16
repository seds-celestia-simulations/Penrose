#pragma once

#include "../Renderer/Framebuffer.h"

struct GLFWwindow;

namespace viz {

class DisplayBlit {
public:
    DisplayBlit();
    ~DisplayBlit();

    DisplayBlit(const DisplayBlit&) = delete;
    DisplayBlit& operator=(const DisplayBlit&) = delete;

    bool initialize();
    void shutdown();
    void present(const Framebuffer& framebuffer, int drawable_width, int drawable_height);

private:
    bool compile_shaders();
    unsigned int program_ = 0;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int texture_ = 0;
    int texture_width_ = 0;
    int texture_height_ = 0;
};

GLFWwindow* create_viewer_window(int width, int height, const char* title);

} // namespace viz
