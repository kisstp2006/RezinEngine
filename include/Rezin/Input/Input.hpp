#pragma once

#include <glm/vec2.hpp>

#include <cstdint>

namespace rezin
{

// Engine-owned key names keep gameplay code independent from GLFW constants.
// More keys can be added later without changing how callers query input.
enum class KeyCode : std::uint8_t
{
    None = 0,

    Space,
    Enter,
    Escape,
    Tab,
    Backspace,

    LeftArrow,
    RightArrow,
    UpArrow,
    DownArrow,

    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    Alpha0,
    Alpha1,
    Alpha2,
    Alpha3,
    Alpha4,
    Alpha5,
    Alpha6,
    Alpha7,
    Alpha8,
    Alpha9,

    LeftShift,
    RightShift,
    LeftControl,
    RightControl,
    LeftAlt,
    RightAlt,

    Count
};

// Strongly typed equivalent of Unity's numbered mouse buttons.
enum class MouseButton : std::uint8_t
{
    Left = 0,
    Right,
    Middle,
    Button4,
    Button5,
    Button6,
    Button7,
    Button8,

    Count
};

// Unity-style static input API. The Application updates it once per frame, so
// game code only needs to include this header and does not need GLFW.
class Input final
{
public:
    Input() = delete;

    // True during every frame in which the key is held down.
    [[nodiscard]] static bool getKey(KeyCode key) noexcept;

    // True only during the frame in which the key changes from up to down.
    [[nodiscard]] static bool getKeyDown(KeyCode key) noexcept;

    // True only during the frame in which the key changes from down to up.
    [[nodiscard]] static bool getKeyUp(KeyCode key) noexcept;

    // True during every frame in which the mouse button is held down.
    [[nodiscard]] static bool getMouseButton(MouseButton button) noexcept;

    // True only during the frame in which the mouse button is pressed.
    [[nodiscard]] static bool getMouseButtonDown(MouseButton button) noexcept;

    // True only during the frame in which the mouse button is released.
    [[nodiscard]] static bool getMouseButtonUp(MouseButton button) noexcept;

    // Cursor position inside the window. Like Unity, the origin is at the
    // bottom-left corner and positive Y points upward.
    [[nodiscard]] static glm::vec2 mousePosition() noexcept;

    // Cursor movement since the previous frame, measured in window units.
    [[nodiscard]] static glm::vec2 mouseDelta() noexcept;

    // Scroll movement received during this frame. Usually only Y is used.
    [[nodiscard]] static glm::vec2 scrollDelta() noexcept;

private:
    friend class Application;

    // These functions belong to the engine lifecycle, not to gameplay code.
    // void* keeps GLFW types out of the public Input header.
    static void initialize(void* nativeWindow);
    static void update() noexcept;
    static void shutdown() noexcept;
};

}
