#include "ViewerApp.h"

#include "DisplayBlit.h"
#include "../Presentation/PresentationPipeline.h"
#include "../Renderer/Framebuffer.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <iostream>

namespace viz {
namespace {

struct ViewerState {
    Scene scene;
    Camera camera;
    Framebuffer framebuffer;
    PresentationPipeline pipeline;
    PresentationProfile profile{};
    RenderOptions render_options{};
    DisplayBlit display;
    bool dragging = false;
    bool panning = false;
    double last_x = 0.0;
    double last_y = 0.0;
};

void scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    auto* state = static_cast<ViewerState*>(glfwGetWindowUserPointer(window));
    if (state == nullptr) {
        return;
    }
    state->camera.zoom(static_cast<float>(yoffset * 0.1));
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/) {
    auto* state = static_cast<ViewerState*>(glfwGetWindowUserPointer(window));
    if (state == nullptr) {
        return;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            state->dragging = true;
            glfwGetCursorPos(window, &state->last_x, &state->last_y);
        } else if (action == GLFW_RELEASE) {
            state->dragging = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        state->panning = action == GLFW_PRESS;
        if (state->panning) {
            glfwGetCursorPos(window, &state->last_x, &state->last_y);
        }
    }
}

void cursor_callback(GLFWwindow* window, double xpos, double ypos) {
    auto* state = static_cast<ViewerState*>(glfwGetWindowUserPointer(window));
    if (state == nullptr) {
        return;
    }

    const double dx = xpos - state->last_x;
    const double dy = ypos - state->last_y;
    state->last_x = xpos;
    state->last_y = ypos;

    if (state->dragging) {
        state->camera.orbit(static_cast<float>(dx * 0.005), static_cast<float>(-dy * 0.005));
    } else if (state->panning) {
        state->camera.pan(static_cast<float>(-dx * 0.01), static_cast<float>(dy * 0.01));
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    auto* state = static_cast<ViewerState*>(glfwGetWindowUserPointer(window));
    if (state == nullptr) {
        return;
    }
    state->framebuffer.resize(width, height);
}

void process_keyboard(GLFWwindow* window, ViewerState& state, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    auto& playback = state.scene.playback();
    static bool space_was_down = false;
    const bool space_down = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (space_down && !space_was_down) {
        playback.playing = !playback.playing;
    }
    space_was_down = space_down;

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        playback.time = std::max(0.0, playback.time - playback.duration * 0.01);
        playback.playing = false;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        playback.time = std::min(playback.duration, playback.time + playback.duration * 0.01);
        playback.playing = false;
    }
    if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS) {
        playback.time = 0.0;
    }
    if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS) {
        playback.time = playback.duration;
    }

    if (playback.playing && !state.scene.trajectories().empty() && playback.duration > 0.0) {
        const float clamped_dt = std::min(dt, 0.05f);
        playback.time += static_cast<double>(clamped_dt) * static_cast<double>(playback.speed);
        if (playback.time > playback.duration) {
            playback.time = 0.0;
        }
    }
}

void print_controls() {
    std::cout << "Controls:\n"
              << "  Left drag     orbit\n"
              << "  Middle drag   pan\n"
              << "  Scroll        zoom\n"
              << "  Space         play/pause\n"
              << "  Left/Right    scrub\n"
              << "  Home/End      jump to start/end\n"
              << "  1-9           select trajectory highlight\n"
              << "  Escape        quit\n";
}

} // namespace

int run_interactive_viewer(Scene scene, Camera camera, const VisualizationConfig& config) {
    GLFWwindow* window = create_viewer_window(config.width, config.height, config.title);
    if (window == nullptr) {
        std::cerr << "Failed to create viewer window\n";
        return 1;
    }

    ViewerState state;
    state.scene = std::move(scene);
    state.camera = camera;
    state.profile = config.presentation;
    state.render_options = config.render;
    state.framebuffer.resize(config.width, config.height);

    if (!state.scene.trajectories().empty()) {
        state.scene.playback().time = 0.0;
        state.scene.playback().playing = true;
        state.scene.playback().speed = std::max(0.01f, config.playback_speed);
    }

    int fbw = config.width;
    int fbh = config.height;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    state.framebuffer.resize(fbw, fbh);

    glfwSetWindowUserPointer(window, &state);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!state.display.initialize()) {
        std::cerr << "Failed to initialize display blit\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    print_controls();

    auto last = std::chrono::steady_clock::now();
    auto fps_window_start = last;
    int frame_count = 0;
    double render_ms_accum = 0.0;

    while (!glfwWindowShouldClose(window)) {
        const auto now = std::chrono::steady_clock::now();
        const float dt = std::chrono::duration<float>(now - last).count();
        last = now;

        process_keyboard(window, state, dt);

        glfwGetFramebufferSize(window, &fbw, &fbh);
        if (fbw > 0 && fbh > 0 &&
            (fbw != state.framebuffer.width() || fbh != state.framebuffer.height())) {
            state.framebuffer.resize(fbw, fbh);
        }

        for (int key = GLFW_KEY_1; key <= GLFW_KEY_9; ++key) {
            if (glfwGetKey(window, key) == GLFW_PRESS) {
                state.scene.playback().selected_trajectory = key - GLFW_KEY_1;
            }
        }

        const auto render_start = std::chrono::steady_clock::now();
        state.pipeline.render(state.scene, state.camera, state.framebuffer, state.render_options,
                              state.profile);
        const auto render_end = std::chrono::steady_clock::now();
        const double render_ms =
            std::chrono::duration<double, std::milli>(render_end - render_start).count();
        render_ms_accum += render_ms;
        ++frame_count;

        state.display.present(state.framebuffer, fbw, fbh);
        glfwSwapBuffers(window);
        glfwPollEvents();

        const double window_s = std::chrono::duration<double>(now - fps_window_start).count();
        if (window_s >= 1.0) {
            const double fps = static_cast<double>(frame_count) / window_s;
            const double avg_render_ms = render_ms_accum / static_cast<double>(frame_count);
            std::cout << "[viz] " << state.framebuffer.width() << "x" << state.framebuffer.height()
                      << "  fps=" << static_cast<int>(fps + 0.5) << "  render="
                      << static_cast<int>(avg_render_ms + 0.5) << " ms/frame"
                      << (state.profile.enabled && state.profile.lensing_strength > 0.0f ? " (lensing)"
                                                                                        : "")
                      << "\n";
            fps_window_start = now;
            frame_count = 0;
            render_ms_accum = 0.0;
        }
    }

    state.display.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

} // namespace viz
