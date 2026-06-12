#include "Arc/Input.hpp"

namespace Arc
{
    bool Input::isKeyDown(Key key) const
    {
        return m_keys[static_cast<std::size_t>(key)];
    }

    bool Input::isMouseButtonDown(MouseButton button) const
    {
        return m_mouseButtons[static_cast<std::size_t>(button)];
    }

    int Input::mouseDeltaX() const
    {
        return m_mouseDeltaX;
    }

    int Input::mouseDeltaY() const
    {
        return m_mouseDeltaY;
    }

    void Input::beginFrame()
    {
        m_mouseDeltaX = 0;
        m_mouseDeltaY = 0;
    }

    void Input::setKey(Key key, bool pressed)
    {
        m_keys[static_cast<std::size_t>(key)] = pressed;
    }

    void Input::setMouseButton(MouseButton button, bool pressed)
    {
        m_mouseButtons[static_cast<std::size_t>(button)] = pressed;
    }

    void Input::addMouseMotion(int deltaX, int deltaY)
    {
        m_mouseDeltaX += deltaX;
        m_mouseDeltaY += deltaY;
    }
}
