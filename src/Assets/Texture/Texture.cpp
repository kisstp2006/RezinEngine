#include <Rezin/Assets/Texture/Texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <bit>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

struct PixelFormat
{
    GLenum internalFormat;
    GLenum dataFormat;
};

GLenum toOpenGlWrap(rezin::TextureWrap wrap)
{
    switch (wrap)
    {
        case rezin::TextureWrap::Repeat:
            return GL_REPEAT;
        case rezin::TextureWrap::MirroredRepeat:
            return GL_MIRRORED_REPEAT;
        case rezin::TextureWrap::ClampToEdge:
            return GL_CLAMP_TO_EDGE;
    }

    return GL_REPEAT;
}

GLenum toOpenGlMagFilter(rezin::TextureFilter filter)
{
    return filter == rezin::TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
}

GLenum toOpenGlMinFilter(
    rezin::TextureFilter filter,
    bool generateMipmaps
)
{
    if (!generateMipmaps)
        return toOpenGlMagFilter(filter);

    return filter == rezin::TextureFilter::Nearest
        ? GL_NEAREST_MIPMAP_NEAREST
        : GL_LINEAR_MIPMAP_LINEAR;
}

PixelFormat selectPixelFormat(int channels, bool srgb)
{
    switch (channels)
    {
        case 1:
            return {GL_R8, GL_RED};
        case 2:
            return {GL_RG8, GL_RG};
        case 3:
            return {
                static_cast<GLenum>(srgb ? GL_SRGB8 : GL_RGB8),
                GL_RGB
            };
        case 4:
            return {
                static_cast<GLenum>(srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8),
                GL_RGBA
            };
        default:
            throw std::runtime_error(
                "Unsupported texture channel count: "
                + std::to_string(channels)
                + ". Expected 1, 2, 3, or 4 color channels."
            );
    }
}

std::vector<stbi_uc> readBinaryFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        throw std::runtime_error(
            "Failed to open texture file: " + path.string()
        );
    }

    const std::streampos endPosition = file.tellg();
    if (endPosition <= 0)
    {
        throw std::runtime_error(
            "Texture file is empty, or its size could not be read: " + path.string()
        );
    }

    const auto fileSize = static_cast<std::uintmax_t>(endPosition);
    if (fileSize > static_cast<std::uintmax_t>(std::numeric_limits<int>::max()))
    {
        throw std::runtime_error(
            "Texture file is too large for stb_image to decode: " + path.string()
        );
    }

    std::vector<stbi_uc> encoded(static_cast<std::size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    file.read(
        reinterpret_cast<char*>(encoded.data()),
        static_cast<std::streamsize>(encoded.size())
    );

    if (!file)
    {
        throw std::runtime_error(
            "Failed to read the complete texture file: " + path.string()
        );
    }

    return encoded;
}

std::mutex& stbMutex()
{
    static std::mutex mutex;
    return mutex;
}

}

namespace rezin
{

Texture2D::Texture2D(
    const std::filesystem::path& path,
    TextureSpecification specification
)
    : path_(path),
      specification_(specification)
{
    const std::vector<stbi_uc> encoded = readBinaryFile(path_);

    using PixelPointer = std::unique_ptr<stbi_uc, decltype(&stbi_image_free)>;
    PixelPointer pixels(nullptr, &stbi_image_free);
    std::string failureReason;

    {
        // stb_image's vertical-flip option is global state. The mutex prevents
        // two loading threads from changing that setting at the same time.
        const std::scoped_lock lock(stbMutex());
        stbi_set_flip_vertically_on_load(specification_.flipVertically ? 1 : 0);

        pixels.reset(stbi_load_from_memory(
            encoded.data(),
            static_cast<int>(encoded.size()),
            &width_,
            &height_,
            &channels_,
            0
        ));

        if (!pixels)
        {
            const char* reason = stbi_failure_reason();
            failureReason = reason != nullptr ? reason : "unknown stb_image error";
        }

        stbi_set_flip_vertically_on_load(0);
    }

    if (!pixels)
    {
        throw std::runtime_error(
            "Failed to decode texture file: " + path_.string()
            + " (stb_image reason: " + failureReason + ")"
        );
    }

    GLint maximumTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
    if (width_ > maximumTextureSize || height_ > maximumTextureSize)
    {
        throw std::runtime_error(
            "Texture dimensions exceed the maximum size supported by this GPU: "
            + path_.string()
        );
    }

    // The file's channel count determines how incoming bytes are interpreted.
    // The internal format determines how OpenGL stores those pixels on the GPU.
    const PixelFormat format = selectPixelFormat(channels_, specification_.srgb);
    const int largestDimension = std::max(width_, height_);
    const GLsizei mipLevels = specification_.generateMipmaps
        ? static_cast<GLsizei>(std::bit_width(static_cast<unsigned int>(largestDimension)))
        : 1;

    glCreateTextures(GL_TEXTURE_2D, 1, &textureId_);
    if (textureId_ == 0)
        throw std::runtime_error(
            "OpenGL failed to create the Texture2D object."
        );

    glTextureParameteri(
        textureId_,
        GL_TEXTURE_WRAP_S,
        static_cast<GLint>(toOpenGlWrap(specification_.wrapS))
    );
    glTextureParameteri(
        textureId_,
        GL_TEXTURE_WRAP_T,
        static_cast<GLint>(toOpenGlWrap(specification_.wrapT))
    );
    glTextureParameteri(
        textureId_,
        GL_TEXTURE_MIN_FILTER,
        static_cast<GLint>(toOpenGlMinFilter(
            specification_.minFilter,
            specification_.generateMipmaps
        ))
    );
    glTextureParameteri(
        textureId_,
        GL_TEXTURE_MAG_FILTER,
        static_cast<GLint>(toOpenGlMagFilter(specification_.magFilter))
    );

    glTextureStorage2D(
        textureId_,
        mipLevels,
        format.internalFormat,
        width_,
        height_
    );

    // One-byte unpack alignment works for every supported channel count and
    // prevents OpenGL from assuming extra padding at the end of image rows.
    GLint previousUnpackAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTextureSubImage2D(
        textureId_,
        0,
        0,
        0,
        width_,
        height_,
        format.dataFormat,
        GL_UNSIGNED_BYTE,
        pixels.get()
    );

    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);

    if (specification_.generateMipmaps)
        glGenerateTextureMipmap(textureId_);
}

Texture2D::~Texture2D()
{
    if (textureId_ != 0)
        glDeleteTextures(1, &textureId_);
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : textureId_(std::exchange(other.textureId_, 0)),
      width_(std::exchange(other.width_, 0)),
      height_(std::exchange(other.height_, 0)),
      channels_(std::exchange(other.channels_, 0)),
      path_(std::move(other.path_)),
      specification_(other.specification_)
{
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
{
    if (this == &other)
        return *this;

    if (textureId_ != 0)
        glDeleteTextures(1, &textureId_);

    textureId_ = std::exchange(other.textureId_, 0);
    width_ = std::exchange(other.width_, 0);
    height_ = std::exchange(other.height_, 0);
    channels_ = std::exchange(other.channels_, 0);
    path_ = std::move(other.path_);
    specification_ = other.specification_;
    return *this;
}

void Texture2D::bind(std::uint32_t slot) const noexcept
{
    glBindTextureUnit(slot, textureId_);
}

void Texture2D::unbind(std::uint32_t slot) noexcept
{
    glBindTextureUnit(slot, 0);
}

GLuint Texture2D::id() const noexcept
{
    return textureId_;
}

int Texture2D::width() const noexcept
{
    return width_;
}

int Texture2D::height() const noexcept
{
    return height_;
}

int Texture2D::channels() const noexcept
{
    return channels_;
}

const std::filesystem::path& Texture2D::path() const noexcept
{
    return path_;
}

const TextureSpecification& Texture2D::specification() const noexcept
{
    return specification_;
}

}
