#pragma once

#include <glad/glad.h>

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
    void setFloat(std::string_view name, float value) const;

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
