#include "render/Renderer.h"
#include <iostream>
#include <fstream>

Renderer::Renderer() {
     float quadVertices[] = {
         -1.0f, -1.0f,    0.0f, 0.0f,
         3.0f, -1.0f,    2.0f, 0.0f,
         -1.0f,  3.0f,    0.0f, 2.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
}

void Renderer::draw(
    Shader& shader, Camera& camera, unsigned int texture, float currentFrame,
    int renderWidth, int renderHeight, bool blackHolePass, bool highQualityPass
) {
    
glViewport(0, 0, renderWidth, renderHeight);
shader.use();

if (blackHolePass) {
    shader.setBool("uHighQualityPass", highQualityPass);
    shader.setFloat("uHighQualityRadius", 0.45f);
    shader.setFloat("uTime", currentFrame);
    shader.setVec2("uResolution", glm::vec2(renderWidth, renderHeight));
    shader.setVec3("uCameraPos", camera.Position);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        float(renderWidth) / float(renderHeight),
        0.1f, 100.0f
    );

    glm::mat4 invProjView = glm::inverse(projection * view);

    glUniformMatrix4fv(
        glGetUniformLocation(shader.ID, "uInvProjView"),
        1, GL_FALSE, &invProjView[0][0]
    );

    particleBuffer.bind(0);
    shader.setInt("uParticleCount", int(particleBuffer.getParticleCount()));
}

glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, texture);

glBindVertexArray(quadVAO);
glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::updateParticles(const std::vector<Particle>& particles) {
    particleBuffer.uploadParticles(particles);
}

void Renderer::bindParticleBuffer(unsigned int bindingPoint) {
    particleBuffer.bind(bindingPoint);
}

size_t Renderer::getParticleCount() const {
    return particleBuffer.getParticleCount();
}

void Renderer::drawQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

bool Renderer::captureFrame(const std::string& filePath, GLFWwindow* window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    unsigned char* pixels = new unsigned char[width * height * 3];
    
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        delete[] pixels;
        return false;
    }
    
    file << "P6\n";
    file << width << " " << height << "\n";
    file << "255\n";
    
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 3;
            file.write((const char*)(pixels + idx), 3);
        }
    }
    
    file.close();
    delete[] pixels;
    
    return true;
}
