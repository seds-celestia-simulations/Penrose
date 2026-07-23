#include "core/ShaderManager.h"
#include <iostream>

void ShaderManager::loadMetric(MetricType type, const std::string& vertexPath, const std::string& fragmentPath) {
    m_paths[type] = {vertexPath, fragmentPath};
}

void ShaderManager::setMetric(MetricType type) {
    if (m_paths.find(type) == m_paths.end()) {
        std::cerr << "ShaderManager: metric not loaded, loading on set\n";
    }
    m_activeMetric = type;
    compileOrRetrieve(type);
}

Shader* ShaderManager::getActive() const {
    auto it = m_cache.find(m_activeMetric);
    if (it == m_cache.end()) return nullptr;
    return it->second.get();
}

void ShaderManager::reloadAll() {
    m_cache.clear();
    for (auto& [type, paths] : m_paths) {
        m_cache[type] = std::make_unique<Shader>(paths.vertexPath.c_str(), paths.fragmentPath.c_str());
    }
}

Shader* ShaderManager::compileOrRetrieve(MetricType type) {
    auto it = m_cache.find(type);
    if (it != m_cache.end()) return it->second.get();

    auto pathsIt = m_paths.find(type);
    if (pathsIt == m_paths.end()) {
        std::cerr << "ShaderManager: no paths registered for metric\n";
        return nullptr;
    }

    auto shader = std::make_unique<Shader>(
        pathsIt->second.vertexPath.c_str(),
        pathsIt->second.fragmentPath.c_str()
    );
    Shader* ptr = shader.get();
    m_cache[type] = std::move(shader);
    return ptr;
}

void ShaderManager::setInt(const std::string& name, int value) const {
    Shader* s = getActive();
    if (s) s->setInt(name, value);
}

void ShaderManager::setFloat(const std::string& name, float value) const {
    Shader* s = getActive();
    if (s) s->setFloat(name, value);
}

void ShaderManager::setVec2(const std::string& name, const glm::vec2& value) const {
    Shader* s = getActive();
    if (s) s->setVec2(name, value);
}

void ShaderManager::setVec3(const std::string& name, const glm::vec3& value) const {
    Shader* s = getActive();
    if (s) s->setVec3(name, value);
}

void ShaderManager::setBool(const std::string& name, bool value) const {
    Shader* s = getActive();
    if (s) s->setBool(name, value);
}
