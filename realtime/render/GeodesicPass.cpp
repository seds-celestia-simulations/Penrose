#include "render/GeodesicPass.h"
#include "render/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GeodesicPass::GeodesicPass(Renderer& renderer, ShaderManager& shaderManager)
    : m_renderer(renderer), m_shaderManager(shaderManager) {}

void GeodesicPass::execute(const PassContext& ctx) {
    Shader* shader = m_shaderManager.getActive();
    if (!shader) return;

    glViewport(0, 0, ctx.width, ctx.height);
    shader->use();

    shader->setBool("uHighQualityPass", false);
    shader->setFloat("uHighQualityRadius", 0.45f);
    shader->setFloat("uTime", ctx.time);
    shader->setVec2("uResolution", glm::vec2(ctx.width, ctx.height));
    shader->setVec3("uCameraPos", ctx.camera.Position);

    glm::mat4 view = ctx.camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        float(ctx.width) / float(ctx.height),
        0.1f, 100.0f
    );
    glm::mat4 invProjView = glm::inverse(projection * view);
    glUniformMatrix4fv(
        glGetUniformLocation(shader->ID, "uInvProjView"),
        1, GL_FALSE, &invProjView[0][0]
    );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ctx.skyboxTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ctx.geodesicLUT);

    m_renderer.bindParticleBuffer(0);
    shader->setInt("uParticleCount", int(m_renderer.getParticleCount()));

    m_renderer.drawQuad();
}
