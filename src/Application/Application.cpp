#include <Rezin/Application/Application.hpp>
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
        // its a C callback so dont export an exception
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
            "Az alkalmazas neve nem lehet ures."
            );
        }
        if(specification_.width == 0 || specification_.height == 0)
        {
            throw std::invalid_argument(
            "Az alkalmazas merete nem lehet nulla."
            );
        }

    }

Application::~Application()
{
    // if run() cant finish properly
    shutdownPlatform();
}

int Application::run()
{
    if (glfwInitialized_ || window_ != nullptr)
    {
        Log::Error(
            "run() can only be called once per object"
        );

        return -1;
    }

    try
    {
        initializePlatform();

        running_ = true;

        //true even before startup because then after a failure in initization we can call Shutdown()
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

            //So the game doesnt get a large delta time after a debugger stop or long resize
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
            "Unknown error during run."
        );
    }

    //Free the Renderer resources while the context still active
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
            "Can't initialise GLFW."
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
            "Cant create GLFW window."
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
            "Cant load in OpenGL with glad."
        );
    }

    glfwSwapInterval(specification_.vsync ? 1 : 0);

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(
        window_,
        framebufferSizeCallback
    );

    int framebufferWidth = 0;
    int framebufferHeight = 0;

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

    if (glfwWindowShouldClose(window_) == GLFW_TRUE)
    {
        running_ = false;
        return;
    }


    if (specification_.width == 0 || specification_.height == 0)
    {
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
