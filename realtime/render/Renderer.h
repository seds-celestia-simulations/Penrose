#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>

#include "core/Shader.h"
#include "scene/Camera.h"
#include "scene/ParticleBuffer.h"

class Renderer {
private:
    unsigned int dummyVAO;
    unsigned int computeOutputTexture;
    ParticleBuffer particleBuffer;

public:
    // Requires initial dimensions to allocate the compute image texture
    Renderer(int width, int height);
    ~Renderer();

    // Now takes both the compute shader for physics and the screen shader for blitting
    void draw(Shader& computeShader, Shader& screenShader, Camera& camera, unsigned int skyboxTexture, 
              float currentFrame, int renderWidth, int renderHeight, bool highQualityPass);

    void updateParticles(const std::vector<Particle>& particles);
    void bindParticleBuffer(unsigned int bindingPoint);
    size_t getParticleCount() const;
    
    bool captureFrame(const std::string& filePath, GLFWwindow* window);
    void bindComputeImage(unsigned int unit) {
    glBindImageTexture(unit, computeOutputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
}
    void blitToScreen();
};