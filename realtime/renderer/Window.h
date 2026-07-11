#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "FrameCapture.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow *window, Camera& camera, FrameCapture& frameCapture);
void setupMouseControls(GLFWwindow* window, Camera& camera);