#include "render/Renderer.h"
#include <iostream>
#include <fstream>

Renderer::Renderer(int width, int height) {

    glGenTextures(1, &computeOutputTexture);
    glBindTexture(GL_TEXTURE_2D, computeOutputTexture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glGenVertexArrays(1, &dummyVAO);
}

Renderer::~Renderer() {
    glDeleteTextures(1, &computeOutputTexture);
    glDeleteVertexArrays(1, &dummyVAO);
}

void Renderer::draw(
    Shader& computeShader, Shader& screenShader, Camera& camera, unsigned int skyboxTexture, 
    float currentFrame, int renderWidth, int renderHeight, bool highQualityPass
) {
    computeShader.use();
    
    // Set all your existing uniforms
    computeShader.setBool("uHighQualityPass", highQualityPass);
    computeShader.setFloat("uHighQualityRadius", 0.45f);
    computeShader.setFloat("uTime", currentFrame);
    computeShader.setVec2("uResolution", glm::vec2(renderWidth, renderHeight));
    computeShader.setVec3("uCameraPos", camera.Position);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), float(renderWidth) / float(renderHeight), 0.1f, 100.0f);
    glm::mat4 invProjView = glm::inverse(projection * view);

    glUniformMatrix4fv(glGetUniformLocation(computeShader.ID, "uInvProjView"), 1, GL_FALSE, &invProjView[0][0]);

    particleBuffer.bind(0);
    computeShader.setInt("uParticleCount", int(particleBuffer.getParticleCount()));
    
    glBindImageTexture(0, computeOutputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    unsigned int numGroupsX = (renderWidth + 15) / 16;
    unsigned int numGroupsY = (renderHeight + 15) / 16;
    
    glDispatchCompute(numGroupsX, numGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glViewport(0, 0, renderWidth, renderHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    screenShader.use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, computeOutputTexture);
    
    glBindVertexArray(dummyVAO);
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

void Renderer::blitToScreen() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, computeOutputTexture); // Bind the texture for reading
    
    glBindVertexArray(dummyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}