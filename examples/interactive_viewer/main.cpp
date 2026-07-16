/**
 * Interactive viewer example: Scientific Engine -> Scene -> visualization_viewer loop.
 *
 * Build target: visualization_viewer_example
 * Run: ./build/visualization_viewer_example
 */

#include "geodesics/GeodesicDynamics.h"
#include "metrics/SchwarzschildMetric.h"
#include "simulation/TerminationPolicy.h"
#include "simulation/TrajectorySolver.h"
#include "state/State.h"

#include "Apps/DisplayBlit.h"
#include "Camera/Camera.h"
#include "Presentation/PresentationDefaults.h"
#include "Presentation/PresentationPipeline.h"
#include "Presentation/PresentationProfile.h"
#include "Renderer/Framebuffer.h"
#include "Scene/Scene.h"
#include "Trajectory/TrajectoryAdapter.h"

#include <GLFW/glfw3.h>

#include <chrono>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

struct ViewerState {
    viz::Scene scene;
    viz::Camera camera;
    viz::Framebuffer framebuffer;
    viz::PresentationPipeline pipeline;
    viz::PresentationProfile profile = viz::PresentationProfile::interactive_default();
    viz::DisplayBlit display;
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
        state->dragging = action == GLFW_PRESS;
        if (state->dragging) {
            glfwGetCursorPos(window, &state->last_x, &state->last_y);
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
        playback.time += static_cast<double>(dt) * static_cast<double>(playback.speed);
        if (playback.time > playback.duration) {
            playback.time = 0.0;
        }
    }
}

viz::Scene build_orbital_scene() {
    constexpr double rs = 1.25;
    constexpr double r0 = 6.0;
    constexpr double vph = 0.06;
    constexpr double dt = 0.01;
    constexpr int max_steps = 100000;

    const double f = 1.0 - rs / r0;
    const double inner = (1.0 + r0 * r0 * vph * vph) / f;
    const double vt = std::sqrt(inner);

    State initial(Eigen::Vector4d(0.0, r0, M_PI / 2.0, 0.0), Eigen::Vector4d(vt, 0.0, 0.0, vph));

    Spacetime::SchwarzschildMetric metric(rs);
    Dynamics::GeodesicDynamics dynamics(metric);
    Simulation::HorizonTermination policy(rs, 1.0);

    const std::vector<State> history =
        Simulation::TrajectorySolver::solve(initial, dynamics, policy, dt, max_steps);

    std::cout << "Integrated " << history.size() << " states for interactive viewer\n";

    viz::Scene scene(viz::cinematic_scene_settings(static_cast<float>(rs)));
    viz::TrajectoryAdapterOptions options;
    options.name = "orbital";
    scene.add_trajectory(viz::adapt_states(history, options));
    scene.playback().time = 0.0;
    scene.playback().playing = true;
    return scene;
}

} // namespace

int main() {
    constexpr int width = 1280;
    constexpr int height = 720;

    GLFWwindow* window =
        viz::create_viewer_window(width, height, "Penrose Interactive Viewer Example");
    if (window == nullptr) {
        std::cerr << "Failed to create viewer window\n";
        return 1;
    }

    ViewerState state;
    state.scene = build_orbital_scene();
    viz::apply_cinematic_camera(state.camera);

    int fbw = width;
    int fbh = height;
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

    std::cout << "Controls: drag orbit, middle pan, scroll zoom, Space play/pause, Escape quit\n";

    viz::RenderOptions render_options{};
    auto last = std::chrono::steady_clock::now();

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

        state.pipeline.render(state.scene, state.camera, state.framebuffer, render_options, state.profile);
        state.display.present(state.framebuffer, fbw, fbh);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    state.display.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
