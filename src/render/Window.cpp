#include "Window.h"
#include "../Constants.h"

// Track key state to prevent multiple toggles on single press
static bool pKeyPressed = false;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window, Camera& camera, FrameCapture& frameCapture) {
    float deltaTime = dt;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Handle 'P' key for frame capture toggle
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        if (!pKeyPressed) {
            frameCapture.toggleCapture();
            pKeyPressed = true;
        }
    } else {
        pKeyPressed = false;
    }

    // Movement (WASD)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // Rotation (Arrow Keys)
    float xoffset = 0.0f;
    float yoffset = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    yoffset =  1.0f;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  yoffset = -1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  xoffset = -1.0f;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) xoffset =  1.0f;

    if (xoffset != 0.0f || yoffset != 0.0f) {
        camera.ProcessRotation(xoffset, yoffset, deltaTime);
    }
}