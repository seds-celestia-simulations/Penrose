#include "Window.h"
#include "../Constants.h"

// Prevent repeated toggles while a key is held.
static bool pKeyPressed = false;
static bool tabKeyPressed = false;

// Mouse state
static bool firstMouse = true;
static float lastMouseX = 400.0f; // initial window center: 800 / 2
static float lastMouseY = 300.0f; // initial window center: 600 / 2
static bool mouseCaptured = true;

// Pointer to the camera used by GLFW's C-style callback.
static Camera* activeCamera = nullptr;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (activeCamera == nullptr || !mouseCaptured)
        return;

    if (firstMouse) {
        lastMouseX = static_cast<float>(xpos);
        lastMouseY = static_cast<float>(ypos);
        firstMouse = false;
        return;
    }

    float xoffset = static_cast<float>(xpos) - lastMouseX;
    float yoffset = lastMouseY - static_cast<float>(ypos);

    lastMouseX = static_cast<float>(xpos);
    lastMouseY = static_cast<float>(ypos);

    activeCamera->ProcessMouseMovement(xoffset, yoffset);
}

void setupMouseControls(GLFWwindow* window, Camera& camera)
{
    activeCamera = &camera;

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    mouseCaptured = true;
    firstMouse = true;
}

void processInput(GLFWwindow* window, Camera& camera, FrameCapture& frameCapture)
{
    float deltaTime = dt;
    bool sprinting =
    glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
    glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

if (sprinting) {
    deltaTime *= 10.0f;
}

    // Escape quits.
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Tab toggles cursor capture.
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tabKeyPressed) {
            mouseCaptured = !mouseCaptured;

            glfwSetInputMode(
                window,
                GLFW_CURSOR,
                mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL
            );

            // Ignore the first mouse event after switching modes.
            firstMouse = true;
            tabKeyPressed = true;
        }
    } else {
        tabKeyPressed = false;
    }

    // P toggles frame capture.
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        if (!pKeyPressed) {
            frameCapture.toggleCapture();
            pKeyPressed = true;
        }
    } else {
        pKeyPressed = false;
    }

    // WASD movement.
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(SHIFT, deltaTime);

    // Optional: keep arrow-key rotation as a backup.
    float xoffset = 0.0f;
    float yoffset = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    yoffset =  1.0f;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  yoffset = -1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  xoffset = -1.0f;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) xoffset =  1.0f;
   // if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) velocity =  1.0f;

    if (xoffset != 0.0f || yoffset != 0.0f) {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}