#include "core/Window.h"

static const float dt = 0.05f;
static bool pKeyPressed = false;
static bool tabKeyPressed = false;

static bool firstMouse = true;
static float lastMouseX = 400.0f;
static float lastMouseY = 300.0f;
static bool mouseCaptured = true;

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

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tabKeyPressed) {
            mouseCaptured = !mouseCaptured;

            glfwSetInputMode(
                window,
                GLFW_CURSOR,
                mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL
            );

            firstMouse = true;
            tabKeyPressed = true;
        }
    } else {
        tabKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        if (!pKeyPressed) {
            frameCapture.toggleCapture();
            pKeyPressed = true;
        }
    } else {
        pKeyPressed = false;
    }

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

    float xoffset = 0.0f;
    float yoffset = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    yoffset =  1.0f;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  yoffset = -1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  xoffset = -1.0f;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) xoffset =  1.0f;

    if (xoffset != 0.0f || yoffset != 0.0f) {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}
