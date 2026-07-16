#include "DisplayBlit.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace viz {

namespace {

GLuint compile_shader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Shader compile error: " << log << "\n";
    }
    return shader;
}

} // namespace

DisplayBlit::DisplayBlit() = default;

DisplayBlit::~DisplayBlit() { shutdown(); }

bool DisplayBlit::compile_shaders() {
    static const char* vert = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTex;
out vec2 vTex;
void main(){ vTex=aTex; gl_Position=vec4(aPos,0.0,1.0);} )";

    static const char* frag = R"(#version 330 core
in vec2 vTex; out vec4 FragColor; uniform sampler2D uTexture;
void main(){ FragColor = texture(uTexture, vTex);} )";

    const GLuint vs = compile_shader(GL_VERTEX_SHADER, vert);
    const GLuint fs = compile_shader(GL_FRAGMENT_SHADER, frag);
    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    return linked == GL_TRUE;
}

bool DisplayBlit::initialize() {
    if (!compile_shaders()) {
        return false;
    }

    const float quad[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, //
        1.0f,  -1.0f, 1.0f, 0.0f, //
        1.0f,  1.0f,  1.0f, 1.0f, //
        -1.0f, -1.0f, 0.0f, 0.0f, //
        1.0f,  1.0f,  1.0f, 1.0f, //
        -1.0f, 1.0f,  0.0f, 1.0f, //
    };

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);

    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void DisplayBlit::shutdown() {
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

void DisplayBlit::present(const Framebuffer& framebuffer, int drawable_width, int drawable_height) {
    const int drawable_w = std::max(1, drawable_width);
    const int drawable_h = std::max(1, drawable_height);
    glViewport(0, 0, drawable_w, drawable_h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (framebuffer.width() != texture_width_ || framebuffer.height() != texture_height_) {
        texture_width_ = framebuffer.width();
        texture_height_ = framebuffer.height();
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_width_, texture_height_, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     nullptr);
    }

    std::vector<unsigned char> rgb(static_cast<std::size_t>(framebuffer.width() * framebuffer.height() * 3));
    const auto& pixels = framebuffer.color_buffer();
    for (int i = 0; i < framebuffer.width() * framebuffer.height(); ++i) {
        rgb[static_cast<std::size_t>(i * 3 + 0)] = pixels[static_cast<std::size_t>(i)].r;
        rgb[static_cast<std::size_t>(i * 3 + 1)] = pixels[static_cast<std::size_t>(i)].g;
        rgb[static_cast<std::size_t>(i * 3 + 2)] = pixels[static_cast<std::size_t>(i)].b;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, framebuffer.width(), framebuffer.height(), GL_RGB,
                    GL_UNSIGNED_BYTE, rgb.data());

    glDisable(GL_DEPTH_TEST);
    glUseProgram(program_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glUniform1i(glGetUniformLocation(program_, "uTexture"), 0);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

GLFWwindow* create_viewer_window(int width, int height, const char* title) {
    if (!glfwInit()) {
        return nullptr;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }
    glfwSwapInterval(1);
    return window;
}

} // namespace viz
