#include "core/ShaderManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

std::string ShaderManager::readFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "ShaderManager: failed to open " << filePath << "\n";
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string ShaderManager::resolveIncludes(const std::string& filePath, std::set<std::string>& visited) {
    if (visited.count(filePath)) {
        std::cerr << "ShaderManager: circular include detected: " << filePath << "\n";
        return "";
    }
    visited.insert(filePath);

    std::string content = readFile(filePath);
    if (content.empty()) return "";

    std::string result;
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));

        if (trimmed.rfind("#include", 0) == 0) {
            size_t firstQuote = trimmed.find('"');
            size_t lastQuote = trimmed.rfind('"');
            if (firstQuote == std::string::npos || lastQuote == firstQuote) {
                std::cerr << "ShaderManager: malformed #include: " << line << "\n";
                result += line + "\n";
                continue;
            }

            std::string includePath = trimmed.substr(firstQuote + 1, lastQuote - firstQuote - 1);

            std::string dir;
            size_t lastSlash = filePath.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                dir = filePath.substr(0, lastSlash + 1);
<<<<<<< Updated upstream
            }
=======
                      }
>>>>>>> Stashed changes

            std::string resolved = dir + includePath;
            result += resolveIncludes(resolved, visited);
        } else {
            result += line + "\n";
        }
    }
<<<<<<< Updated upstream

    return result;
}

void ShaderManager::loadMetric(MetricType type, const std::string& vertexPath,
                               const std::string& headerPath, const std::string& metricPath,
                               const std::string& mainPath) {
    m_paths[type] = {vertexPath, headerPath, metricPath, mainPath};
=======
       return result;
}

void ShaderManager::loadMetricCompute(MetricType type, const std::string& computePath) {
    
    m_paths[type].computePath = computePath;
    
    m_cache[type] = std::make_unique<Shader>(computePath.c_str());
>>>>>>> Stashed changes
}

void ShaderManager::setMetric(MetricType type) {
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
        compileOrRetrieve(type);
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

    auto& p = pathsIt->second;

    std::set<std::string> visited;
    std::string header = resolveIncludes(p.headerPath, visited);
    visited.clear();
    std::string metric = resolveIncludes(p.metricPath, visited);
    visited.clear();
    std::string main = resolveIncludes(p.mainPath, visited);

    std::string fragmentSource = header + "\n" + metric + "\n" + main + "\n";

    auto shader = std::make_unique<Shader>(p.vertexPath.c_str(), fragmentSource.c_str());
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
