#include <Rezin/Application/Application.hpp>
#include <Rezin/Input/Input.hpp>
#include <Rezin/Utilities/Log.hpp>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;
using namespace rezin;


namespace
{

void glfwErrorCallback(int errorCode, const char* description) noexcept
{
    try
    {
        Log::Error(
            "GLFW error ("
            + std::to_string(errorCode)
            + "): "
            + (description != nullptr ? description : "unknown error")
        );
    }
    catch (...)
    {
        // GLFW calls this through a C API. A C++ exception must never escape a
        // C callback boundary, so logging failures are deliberately ignored.
    }
}

}


namespace rezin
{
    Application::Application(ApplicationSpecification specification) : specification_(std::move(specification))
    {
        if(specification_.name.empty())
        {
            throw std::invalid_argument(
            "Application name cannot be empty. Give the window a visible title."
            );
        }
        if(specification_.width == 0 || specification_.height == 0)
        {
            throw std::invalid_argument(
            "Initial application width and height must both be greater than zero."
            );
        }

    }

Application::~Application()
{
    // This is a final safety net for partial initialization or an early exit.
    // shutdownPlatform() is safe to call more than once.
    shutdownPlatform();
}

int Application::run()
{
    if (glfwInitialized_ || window_ != nullptr)
    {
        Log::Error(
            "Application::run() cannot start while this Application instance already owns a GLFW window."
        );

        return -1;
    }

    try
    {
        initializePlatform();

        running_ = true;

        // Mark startup as entered before calling user code. If onStartup()
        // throws, onShutdown() can still release resources created before it.
        started_ = true;
        onStartup();

        using Clock = std::chrono::steady_clock;
        auto previousFrameTime = Clock::now();

        while (
            running_
            && glfwWindowShouldClose(window_) == GLFW_FALSE
        )
        {
            const auto currentFrameTime = Clock::now();

            float deltaSeconds =
                std::chrono::duration<float>(
                    currentFrameTime - previousFrameTime
                ).count();

            previousFrameTime = currentFrameTime;

            // Limit unusually long frames. This prevents one large simulation
            // jump after a debugger pause, window resize, or temporary stall.
            deltaSeconds = std::min(deltaSeconds, 0.25f);

            runOneFrame(deltaSeconds);
        }

        running_ = false;
    }
    catch (const std::exception& error)
    {
        running_ = false;
        exitCode_ = -1;

        Log::Error(
            std::string("Application error: ") + error.what()
        );
    }
    catch (...)
    {
        running_ = false;
        exitCode_ = -1;

        Log::Error(
            "Application stopped because an unknown non-standard exception was thrown during run()."
        );
    }

    // Release renderer resources before destroying the window because OpenGL
    // deletion functions require a valid current context.
    if (started_)
    {
        onShutdown();
        started_ = false;
    }

    shutdownPlatform();

    return exitCode_;
}

void Application::requestQuit(int exitCode) noexcept
{
    exitCode_ = exitCode;
    running_ = false;

    if (window_ != nullptr)
    {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }
}

bool Application::isRunning() const noexcept
{
    return running_;
}

std::uint32_t Application::width() const noexcept
{
    return specification_.width;
}

std::uint32_t Application::height() const noexcept
{
    return specification_.height;
}

GLFWwindow* Application::nativeWindow() const noexcept
{
    return window_;
}

void Application::initializePlatform()
{
    glfwSetErrorCallback(glfwErrorCallback);

    if (glfwInit() != GLFW_TRUE)
    {
        throw std::runtime_error(
            "Failed to initialize GLFW. Check that its runtime dependencies are available."
        );
    }

    glfwInitialized_ = true;

    glfwDefaultWindowHints();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );
    glfwWindowHint(
        GLFW_OPENGL_FORWARD_COMPAT,
        GLFW_TRUE
    );
    glfwWindowHint(
        GLFW_RESIZABLE,
        specification_.resizable ? GLFW_TRUE : GLFW_FALSE
    );

    window_ = glfwCreateWindow(
        static_cast<int>(specification_.width),
        static_cast<int>(specification_.height),
        specification_.name.c_str(),
        nullptr,
        nullptr
    );

    if (window_ == nullptr)
    {
        throw std::runtime_error(
            "Failed to create the GLFW window or its OpenGL 4.6 context. The requested OpenGL version may not be supported."
        );
    }

    glfwMakeContextCurrent(window_);

    if (
        !gladLoadGLLoader(
            reinterpret_cast<GLADloadproc>(
                glfwGetProcAddress
            )
        )
    )
    {
        throw std::runtime_error(
            "Failed to load OpenGL functions with GLAD after creating the context."
        );
    }

    glfwSwapInterval(specification_.vsync ? 1 : 0);

    // GLFW callbacks only receive a GLFWwindow pointer. The user pointer lets
    // the static callback find the correct C++ Application instance again.
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(
        window_,
        framebufferSizeCallback
    );

    Input::initialize(window_);

    int framebufferWidth = 0;
    int framebufferHeight = 0;

    // Framebuffer pixels may differ from logical window units on high-DPI
    // displays, so rendering and glViewport use the framebuffer dimensions.
    glfwGetFramebufferSize(
        window_,
        &framebufferWidth,
        &framebufferHeight
    );

    specification_.width = static_cast<std::uint32_t>(
        std::max(framebufferWidth, 0)
    );

    specification_.height = static_cast<std::uint32_t>(
        std::max(framebufferHeight, 0)
    );

    glViewport(
        0,
        0,
        framebufferWidth,
        framebufferHeight
    );

    Log::Info(
        "Application started: "
        + specification_.name
        + " ("
        + std::to_string(specification_.width)
        + "x"
        + std::to_string(specification_.height)
        + ")"
    );
}

void Application::shutdownPlatform() noexcept
{
    if (window_ != nullptr)
    {
        Input::shutdown();

        glfwSetFramebufferSizeCallback(window_, nullptr);
        glfwSetWindowUserPointer(window_, nullptr);

        if (glfwGetCurrentContext() == window_)
        {
            glfwMakeContextCurrent(nullptr);
        }

        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    if (glfwInitialized_)
    {
        glfwTerminate();
        glfwInitialized_ = false;
    }

    glfwSetErrorCallback(nullptr);

    running_ = false;
}

void Application::runOneFrame(float deltaSeconds)
{
    glfwPollEvents();
    Input::update();

    if (glfwWindowShouldClose(window_) == GLFW_TRUE)
    {
        running_ = false;
        return;
    }


    if (specification_.width == 0 || specification_.height == 0)
    {
        // A minimized window can have a zero-sized framebuffer. Wait briefly
        // instead of rendering invalid frames or consuming an entire CPU core.
        glfwWaitEventsTimeout(0.05);
        return;
    }

    onUpdate(deltaSeconds);
    onRender();

    glfwSwapBuffers(window_);
}

void Application::framebufferSizeCallback(
    GLFWwindow* window,
    int width,
    int height
)
{
    auto* application = static_cast<Application*>(
        glfwGetWindowUserPointer(window)
    );

    if (application == nullptr)
        return;

    const int safeWidth = std::max(width, 0);
    const int safeHeight = std::max(height, 0);

    application->specification_.width =
        static_cast<std::uint32_t>(safeWidth);

    application->specification_.height =
        static_cast<std::uint32_t>(safeHeight);

    glViewport(
        0,
        0,
        safeWidth,
        safeHeight
    );

    application->onFramebufferResize(
        application->specification_.width,
        application->specification_.height
    );
}

void Application::onFramebufferResize(
    std::uint32_t width,
    std::uint32_t height
)
{
    static_cast<void>(width);
    static_cast<void>(height);
}

}
