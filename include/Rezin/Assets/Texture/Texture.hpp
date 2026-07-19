#pragma once

#include <glad/glad.h>

#include <cstdint>
#include <filesystem>
#include <span>

namespace rezin
{

enum class TextureWrap
{
    Repeat,
    MirroredRepeat,
    ClampToEdge
};

enum class TextureFilter
{
    Nearest,
    Linear
};

struct TextureSpecification
{
    TextureWrap wrapS{TextureWrap::Repeat};
    TextureWrap wrapT{TextureWrap::Repeat};
    TextureFilter minFilter{TextureFilter::Linear};
    TextureFilter magFilter{TextureFilter::Linear};
    bool generateMipmaps{true};
    bool flipVertically{true};
    bool srgb{false};
};

class Texture2D final
{
public:
    explicit Texture2D(
        const std::filesystem::path& path,
        TextureSpecification specification = {}
    );

    // Creates a texture from a complete encoded image already held in memory,
    // such as a PNG or JPG embedded inside an FBX model.
    Texture2D(
        std::span<const std::uint8_t> encodedImage,
        std::filesystem::path sourceName,
        TextureSpecification specification = {}
    );

    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    void bind(std::uint32_t slot = 0) const noexcept;
    static void unbind(std::uint32_t slot = 0) noexcept;

    [[nodiscard]] GLuint id() const noexcept;
    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] int channels() const noexcept;
    [[nodiscard]] const std::filesystem::path& path() const noexcept;
    [[nodiscard]] const TextureSpecification& specification() const noexcept;

private:
    GLuint textureId_{0};
    int width_{0};
    int height_{0};
    int channels_{0};
    std::filesystem::path path_;
    TextureSpecification specification_;

    void loadFromEncodedImage(
        std::span<const std::uint8_t> encodedImage
    );
};

}
