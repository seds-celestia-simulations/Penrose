#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include "core/Shader.h"

enum class MetricType {
    SCHWARZSCHILD_REDUCED,
    SCHWARZSCHILD_FULL
};

class ShaderManager {
public:
    ShaderManager() = default;
    ~ShaderManager() = default;

    void loadMetric(MetricType type, const std::string& vertexPath, const std::string& fragmentPath);
    void setMetric(MetricType type);
    Shader* getActive() const;
    MetricType activeMetric() const { return m_activeMetric; }
    void reloadAll();

    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setBool(const std::string& name, bool value) const;

private:
    struct ShaderPaths {
        std::string vertexPath;
        std::string fragmentPath;
    };

    std::unordered_map<MetricType, ShaderPaths> m_paths;
    std::unordered_map<MetricType, std::unique_ptr<Shader>> m_cache;
    MetricType m_activeMetric = MetricType::SCHWARZSCHILD_REDUCED;

    Shader* compileOrRetrieve(MetricType type);
};
