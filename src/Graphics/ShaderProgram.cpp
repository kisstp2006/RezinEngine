#include <Rezin/Graphics/ShaderProgram.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{

std::string shaderStageName(GLenum stage)
{
    switch (stage)
    {
        case GL_VERTEX_SHADER:
            return "vertex";
        case GL_FRAGMENT_SHADER:
            return "fragment";
        case GL_GEOMETRY_SHADER:
            return "geometry";
        case GL_TESS_CONTROL_SHADER:
            return "tessellation control";
        case GL_TESS_EVALUATION_SHADER:
            return "tessellation evaluation";
        case GL_COMPUTE_SHADER:
            return "compute";
        default:
            return "unknown";
    }
}

std::string shaderInfoLog(GLuint shader)
{
    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    if (logLength <= 1)
        return {};

    std::vector<GLchar> buffer(static_cast<std::size_t>(logLength));
    GLsizei written = 0;
    glGetShaderInfoLog(shader, logLength, &written, buffer.data());

    return std::string(buffer.data(), static_cast<std::size_t>(written));
}

std::string programInfoLog(GLuint program)
{
    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

    if (logLength <= 1)
        return {};

    std::vector<GLchar> buffer(static_cast<std::size_t>(logLength));
    GLsizei written = 0;
    glGetProgramInfoLog(program, logLength, &written, buffer.data());

    return std::string(buffer.data(), static_cast<std::size_t>(written));
}

}

namespace rezin
{

ShaderProgram::ShaderProgram(
    const std::filesystem::path& vertexPath,
    const std::filesystem::path& fragmentPath
)
{
    const std::string vertexSource = readFile(vertexPath);
    const std::string fragmentSource = readFile(fragmentPath);

    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;

    try
    {
        vertexShader = compileStage(GL_VERTEX_SHADER, vertexSource, vertexPath);
        fragmentShader = compileStage(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);
        programId_ = linkProgram(vertexShader, fragmentShader);

        glDetachShader(programId_, vertexShader);
        glDetachShader(programId_, fragmentShader);
    }
    catch (...)
    {
        if (vertexShader != 0)
            glDeleteShader(vertexShader);
        if (fragmentShader != 0)
            glDeleteShader(fragmentShader);
        throw;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

ShaderProgram::~ShaderProgram()
{
    if (programId_ != 0)
        glDeleteProgram(programId_);
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : programId_(std::exchange(other.programId_, 0)),
      uniformLocations_(std::move(other.uniformLocations_))
{
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
    if (this == &other)
        return *this;

    if (programId_ != 0)
        glDeleteProgram(programId_);

    programId_ = std::exchange(other.programId_, 0);
    uniformLocations_ = std::move(other.uniformLocations_);
    return *this;
}

void ShaderProgram::use() const noexcept
{
    glUseProgram(programId_);
}

void ShaderProgram::setBool(std::string_view name, bool value) const
{
    glProgramUniform1i(programId_, findUniform(name), value ? GL_TRUE : GL_FALSE);
}

void ShaderProgram::setInt(std::string_view name, int value) const
{
    glProgramUniform1i(programId_, findUniform(name), value);
}

void ShaderProgram::setFloat(std::string_view name, float value) const
{
    glProgramUniform1f(programId_, findUniform(name), value);
}

std::string ShaderProgram::readFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error(
            "Nem sikerult megnyitni a shader fajlt: "
            + path.string()
        );
    }

    std::ostringstream contents;
    contents << file.rdbuf();

    if (file.bad())
    {
        throw std::runtime_error(
            "Hiba tortent a shader fajl olvasasa kozben: "
            + path.string()
        );
    }

    return contents.str();
}

GLuint ShaderProgram::compileStage(
    GLenum stage,
    const std::string& source,
    const std::filesystem::path& sourcePath
)
{
    const GLuint shader = glCreateShader(stage);
    if (shader == 0)
    {
        throw std::runtime_error(
            "Nem sikerult letrehozni a(z) " + shaderStageName(stage)
            + " shader objektumot: " + sourcePath.string()
        );
    }

    const GLchar* sourcePointer = source.data();
    const GLint sourceLength = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &sourcePointer, &sourceLength);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE)
        return shader;

    const std::string log = shaderInfoLog(shader);
    glDeleteShader(shader);

    throw std::runtime_error(
        "Nem sikerult leforditani a(z) " + shaderStageName(stage)
        + " shadert: " + sourcePath.string() + "\n" + log
    );
}

GLuint ShaderProgram::linkProgram(GLuint vertexShader, GLuint fragmentShader)
{
    const GLuint program = glCreateProgram();
    if (program == 0)
        throw std::runtime_error("Nem sikerult letrehozni az OpenGL shader programot.");

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE)
        return program;

    const std::string log = programInfoLog(program);
    glDeleteProgram(program);

    throw std::runtime_error(
        "Nem sikerult linkelni az OpenGL shader programot.\n" + log
    );
}

GLint ShaderProgram::findUniform(std::string_view name) const
{
    const std::string key(name);

    const auto cached = uniformLocations_.find(key);
    if (cached != uniformLocations_.end())
        return cached->second;

    const GLint location = glGetUniformLocation(programId_, key.c_str());
    if (location == -1)
    {
        throw std::runtime_error(
            "A uniform nem talalhato vagy nincs hasznalatban: " + key
        );
    }

    uniformLocations_.emplace(key, location);
    return location;
}

}
