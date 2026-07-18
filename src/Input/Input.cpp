#include <Rezin/Input/Input.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <array>
#include <cstddef>
#include <stdexcept>

namespace
{

constexpr std::size_t keyCount = static_cast<std::size_t>(rezin::KeyCode::Count);
constexpr std::size_t mouseButtonCount =
    static_cast<std::size_t>(rezin::MouseButton::Count);

struct InputState
{
    GLFWwindow* window = nullptr;
    std::array<bool, keyCount> currentKeys{};
    std::array<bool, keyCount> previousKeys{};
    std::array<bool, mouseButtonCount> currentMouseButtons{};
    std::array<bool, mouseButtonCount> previousMouseButtons{};
    glm::vec2 mousePosition{0.0f};
    glm::vec2 mouseDelta{0.0f};
    glm::vec2 scrollDelta{0.0f};
    glm::vec2 pendingScrollDelta{0.0f};
};

InputState& inputState() noexcept
{
    static InputState state;
    return state;
}

std::size_t keyIndex(rezin::KeyCode key) noexcept
{
    return static_cast<std::size_t>(key);
}

std::size_t mouseButtonIndex(rezin::MouseButton button) noexcept
{
    return static_cast<std::size_t>(button);
}

int toGlfwKey(rezin::KeyCode key) noexcept
{
    using enum rezin::KeyCode;

    switch (key)
    {
        case Space:        return GLFW_KEY_SPACE;
        case Enter:        return GLFW_KEY_ENTER;
        case Escape:       return GLFW_KEY_ESCAPE;
        case Tab:          return GLFW_KEY_TAB;
        case Backspace:    return GLFW_KEY_BACKSPACE;

        case LeftArrow:    return GLFW_KEY_LEFT;
        case RightArrow:   return GLFW_KEY_RIGHT;
        case UpArrow:      return GLFW_KEY_UP;
        case DownArrow:    return GLFW_KEY_DOWN;

        case A:            return GLFW_KEY_A;
        case B:            return GLFW_KEY_B;
        case C:            return GLFW_KEY_C;
        case D:            return GLFW_KEY_D;
        case E:            return GLFW_KEY_E;
        case F:            return GLFW_KEY_F;
        case G:            return GLFW_KEY_G;
        case H:            return GLFW_KEY_H;
        case I:            return GLFW_KEY_I;
        case J:            return GLFW_KEY_J;
        case K:            return GLFW_KEY_K;
        case L:            return GLFW_KEY_L;
        case M:            return GLFW_KEY_M;
        case N:            return GLFW_KEY_N;
        case O:            return GLFW_KEY_O;
        case P:            return GLFW_KEY_P;
        case Q:            return GLFW_KEY_Q;
        case R:            return GLFW_KEY_R;
        case S:            return GLFW_KEY_S;
        case T:            return GLFW_KEY_T;
        case U:            return GLFW_KEY_U;
        case V:            return GLFW_KEY_V;
        case W:            return GLFW_KEY_W;
        case X:            return GLFW_KEY_X;
        case Y:            return GLFW_KEY_Y;
        case Z:            return GLFW_KEY_Z;

        case Alpha0:       return GLFW_KEY_0;
        case Alpha1:       return GLFW_KEY_1;
        case Alpha2:       return GLFW_KEY_2;
        case Alpha3:       return GLFW_KEY_3;
        case Alpha4:       return GLFW_KEY_4;
        case Alpha5:       return GLFW_KEY_5;
        case Alpha6:       return GLFW_KEY_6;
        case Alpha7:       return GLFW_KEY_7;
        case Alpha8:       return GLFW_KEY_8;
        case Alpha9:       return GLFW_KEY_9;

        case LeftShift:    return GLFW_KEY_LEFT_SHIFT;
        case RightShift:   return GLFW_KEY_RIGHT_SHIFT;
        case LeftControl:  return GLFW_KEY_LEFT_CONTROL;
        case RightControl: return GLFW_KEY_RIGHT_CONTROL;
        case LeftAlt:      return GLFW_KEY_LEFT_ALT;
        case RightAlt:     return GLFW_KEY_RIGHT_ALT;

        case None:
        case Count:
            return GLFW_KEY_UNKNOWN;
    }

    return GLFW_KEY_UNKNOWN;
}

bool isValidKey(rezin::KeyCode key) noexcept
{
    return key != rezin::KeyCode::None && keyIndex(key) < keyCount;
}

int toGlfwMouseButton(rezin::MouseButton button) noexcept
{
    using enum rezin::MouseButton;

    switch (button)
    {
        case Left:    return GLFW_MOUSE_BUTTON_LEFT;
        case Right:   return GLFW_MOUSE_BUTTON_RIGHT;
        case Middle:  return GLFW_MOUSE_BUTTON_MIDDLE;
        case Button4: return GLFW_MOUSE_BUTTON_4;
        case Button5: return GLFW_MOUSE_BUTTON_5;
        case Button6: return GLFW_MOUSE_BUTTON_6;
        case Button7: return GLFW_MOUSE_BUTTON_7;
        case Button8: return GLFW_MOUSE_BUTTON_8;
        case Count:   return -1;
    }

    return -1;
}

bool isValidMouseButton(rezin::MouseButton button) noexcept
{
    return mouseButtonIndex(button) < mouseButtonCount;
}

glm::vec2 readMousePosition(GLFWwindow* window) noexcept
{
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window, &x, &y);

    int windowHeight = 0;
    glfwGetWindowSize(window, nullptr, &windowHeight);

    // GLFW starts at the top-left. Flip Y to expose Unity-style coordinates
    // whose origin is at the bottom-left of the application window.
    return {
        static_cast<float>(x),
        static_cast<float>(windowHeight) - static_cast<float>(y)
    };
}

void scrollCallback(
    GLFWwindow* window,
    double xOffset,
    double yOffset
) noexcept
{
    InputState& state = inputState();
    if (state.window != window)
        return;

    state.pendingScrollDelta += glm::vec2(
        static_cast<float>(xOffset),
        static_cast<float>(yOffset)
    );
}

}

namespace rezin
{

bool Input::getKey(KeyCode key) noexcept
{
    if (!isValidKey(key))
        return false;

    return inputState().currentKeys[keyIndex(key)];
}

bool Input::getKeyDown(KeyCode key) noexcept
{
    if (!isValidKey(key))
        return false;

    const InputState& state = inputState();
    const std::size_t index = keyIndex(key);
    return state.currentKeys[index] && !state.previousKeys[index];
}

bool Input::getKeyUp(KeyCode key) noexcept
{
    if (!isValidKey(key))
        return false;

    const InputState& state = inputState();
    const std::size_t index = keyIndex(key);
    return !state.currentKeys[index] && state.previousKeys[index];
}

bool Input::getMouseButton(MouseButton button) noexcept
{
    if (!isValidMouseButton(button))
        return false;

    return inputState().currentMouseButtons[mouseButtonIndex(button)];
}

bool Input::getMouseButtonDown(MouseButton button) noexcept
{
    if (!isValidMouseButton(button))
        return false;

    const InputState& state = inputState();
    const std::size_t index = mouseButtonIndex(button);
    return state.currentMouseButtons[index]
        && !state.previousMouseButtons[index];
}

bool Input::getMouseButtonUp(MouseButton button) noexcept
{
    if (!isValidMouseButton(button))
        return false;

    const InputState& state = inputState();
    const std::size_t index = mouseButtonIndex(button);
    return !state.currentMouseButtons[index]
        && state.previousMouseButtons[index];
}

glm::vec2 Input::mousePosition() noexcept
{
    return inputState().mousePosition;
}

glm::vec2 Input::mouseDelta() noexcept
{
    return inputState().mouseDelta;
}

glm::vec2 Input::scrollDelta() noexcept
{
    return inputState().scrollDelta;
}

void Input::initialize(void* nativeWindow)
{
    if (nativeWindow == nullptr)
    {
        throw std::invalid_argument(
            "Input cannot initialize without a valid native window."
        );
    }

    InputState& state = inputState();
    state = {};
    state.window = static_cast<GLFWwindow*>(nativeWindow);
    state.mousePosition = readMousePosition(state.window);

    glfwSetScrollCallback(state.window, scrollCallback);

    // Read the initial state twice so controls already held during startup do
    // not incorrectly appear as newly pressed on the first application frame.
    update();
    state.previousKeys = state.currentKeys;
    state.previousMouseButtons = state.currentMouseButtons;
    state.mouseDelta = glm::vec2(0.0f);
}

void Input::update() noexcept
{
    InputState& state = inputState();
    if (state.window == nullptr)
        return;

    state.previousKeys = state.currentKeys;

    for (std::size_t index = 0; index < keyCount; ++index)
    {
        const KeyCode key = static_cast<KeyCode>(index);
        const int glfwKey = toGlfwKey(key);

        state.currentKeys[index] = glfwKey != GLFW_KEY_UNKNOWN
            && glfwGetKey(state.window, glfwKey) == GLFW_PRESS;
    }

    state.previousMouseButtons = state.currentMouseButtons;

    for (std::size_t index = 0; index < mouseButtonCount; ++index)
    {
        const MouseButton button = static_cast<MouseButton>(index);
        const int glfwButton = toGlfwMouseButton(button);

        state.currentMouseButtons[index] = glfwButton >= 0
            && glfwGetMouseButton(state.window, glfwButton) == GLFW_PRESS;
    }

    const glm::vec2 newMousePosition = readMousePosition(state.window);
    state.mouseDelta = newMousePosition - state.mousePosition;
    state.mousePosition = newMousePosition;

    // Scroll arrives through a GLFW callback during glfwPollEvents(). Move the
    // accumulated value into this frame, then clear it for the next frame.
    state.scrollDelta = state.pendingScrollDelta;
    state.pendingScrollDelta = glm::vec2(0.0f);
}

void Input::shutdown() noexcept
{
    InputState& state = inputState();
    if (state.window != nullptr)
        glfwSetScrollCallback(state.window, nullptr);

    inputState() = {};
}

}
