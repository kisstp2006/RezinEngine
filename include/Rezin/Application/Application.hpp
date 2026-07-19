#pragma once

#include <Rezin/Graphics/Renderer.hpp>

#include <cstdint>
#include <string>

struct GLFWwindow;

namespace rezin
{

struct ApplicationSpecification
{
    std::string name = "RezinEngine";
    std::uint32_t width = 1280;
    std::uint32_t height = 720;
    bool resizable = true;
    bool vsync = true;
    RendererSpecification renderer;
};

class Application
{
public:
    explicit Application(ApplicationSpecification specification);
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    [[nodiscard]] int run();

    void requestQuit(int exitCode = 0) noexcept;

    [[nodiscard]] bool isRunning() const noexcept;
    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;

protected:
    virtual void onStartup() {}
    virtual void onUpdate(float deltaSeconds) = 0;
    virtual void onRender() = 0;
    virtual void onShutdown() noexcept {}

    virtual void onFramebufferResize(
        std::uint32_t width,
        std::uint32_t height
    );

    [[nodiscard]] GLFWwindow* nativeWindow() const noexcept;

private:
    void initializePlatform();
    void shutdownPlatform() noexcept;
    void runOneFrame(float deltaSeconds);

    static void framebufferSizeCallback(
        GLFWwindow* window,
        int width,
        int height
    );

    ApplicationSpecification specification_;

    GLFWwindow* window_ = nullptr;

    bool glfwInitialized_ = false;
    bool running_ = false;
    bool started_ = false;

    int exitCode_ = 0;
};

}
