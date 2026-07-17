#pragma once

#include <glad/glad.h>
#include <glm/fwd.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace rezin
{

class ShaderProgram final
{
public:
    ShaderProgram(
        const std::filesystem::path& vertexPath,
        const std::filesystem::path& fragmentPath
    );

    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    void use() const noexcept;

    void setBool(std::string_view name, bool value) const;
    void setInt(std::string_view name, int value) const;
    void setUInt(std::string_view name, std::uint32_t value) const;
    void setFloat(std::string_view name, float value) const;

    void setVec2(std::string_view name, const glm::vec2& value) const;
    void setVec3(std::string_view name, const glm::vec3& value) const;
    void setVec4(std::string_view name, const glm::vec4& value) const;

    void setIVec2(std::string_view name, const glm::ivec2& value) const;
    void setIVec3(std::string_view name, const glm::ivec3& value) const;
    void setIVec4(std::string_view name, const glm::ivec4& value) const;

    void setUVec2(std::string_view name, const glm::uvec2& value) const;
    void setUVec3(std::string_view name, const glm::uvec3& value) const;
    void setUVec4(std::string_view name, const glm::uvec4& value) const;

    void setMat2(std::string_view name, const glm::mat2& value) const;
    void setMat3(std::string_view name, const glm::mat3& value) const;
    void setMat4(std::string_view name, const glm::mat4& value) const;

private:
    GLuint programId_{0};

    static std::string readFile(const std::filesystem::path& path);

    static GLuint compileStage(
        GLenum stage,
        const std::string& source,
        const std::filesystem::path& sourcePath
    );

    static GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader);

    GLint findUniform(std::string_view name) const;

    mutable std::unordered_map<std::string, GLint> uniformLocations_;
};

}
