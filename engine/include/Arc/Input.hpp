#pragma once

#include <array>
#include <cstddef>

namespace Arc
{
    enum class Key : std::size_t
    {
        W,
        A,
        S,
        D,
        Q,
        E,
        LeftShift,
        Escape,
        Num1,
        Num2,
        Num3,
        I,
        O,
        K,
        L,
        U,
        J,
        Count
    };

    enum class MouseButton : std::size_t
    {
        Left,
        Right,
        Middle,
        Count
    };

    class Input
    {
    public:
        [[nodiscard]] bool isKeyDown(Key key) const;
        [[nodiscard]] bool isMouseButtonDown(MouseButton button) const;
        [[nodiscard]] int mouseDeltaX() const;
        [[nodiscard]] int mouseDeltaY() const;

    private:
        friend class Application;

        void beginFrame();
        void setKey(Key key, bool pressed);
        void setMouseButton(MouseButton button, bool pressed);
        void addMouseMotion(int deltaX, int deltaY);

        std::array<bool, static_cast<std::size_t>(Key::Count)> m_keys{};
        std::array<bool, static_cast<std::size_t>(MouseButton::Count)> m_mouseButtons{};
        int m_mouseDeltaX = 0;
        int m_mouseDeltaY = 0;
    };
}
