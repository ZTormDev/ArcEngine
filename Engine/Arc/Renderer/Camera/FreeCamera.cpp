#include "FreeCamera.h"

#include "../../Platform/Window.h"

#include <algorithm>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace arc
{
    static float DegToRad(float degrees)
    {
        return degrees * glm::pi<float>() / 180.0f;
    }

    void FreeCamera::OnEvent(const SDL_Event& event, Window& window)
    {
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT)
        {
            m_MouseCaptured = true;
            window.CaptureMouse(true);
        }

        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT)
        {
            m_MouseCaptured = false;
            window.CaptureMouse(false);
        }

        if (event.type == SDL_MOUSEMOTION && m_MouseCaptured)
        {
            m_Yaw += static_cast<float>(event.motion.xrel) * m_MouseSensitivity;
            m_Pitch -= static_cast<float>(event.motion.yrel) * m_MouseSensitivity;
            m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);
            UpdateDirection();
        }

        if (event.type == SDL_MOUSEWHEEL)
        {
            m_FovDegrees -= static_cast<float>(event.wheel.y) * 2.0f;
            m_FovDegrees = std::clamp(m_FovDegrees, 35.0f, 90.0f);
        }
    }

    void FreeCamera::Update(float deltaSeconds)
    {
        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        float speed = m_MoveSpeed;
        if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT])
            speed *= m_FastMultiplier;

        const float amount = speed * deltaSeconds;

        if (keys[SDL_SCANCODE_W]) m_Position += m_Forward * amount;
        if (keys[SDL_SCANCODE_S]) m_Position -= m_Forward * amount;
        if (keys[SDL_SCANCODE_D]) m_Position += m_Right * amount;
        if (keys[SDL_SCANCODE_A]) m_Position -= m_Right * amount;
        if (keys[SDL_SCANCODE_E]) m_Position += glm::vec3(0.0f, 1.0f, 0.0f) * amount;
        if (keys[SDL_SCANCODE_Q]) m_Position -= glm::vec3(0.0f, 1.0f, 0.0f) * amount;
    }

    glm::vec3 FreeCamera::GetForward() const
    {
        return m_Forward;
    }

    glm::vec3 FreeCamera::GetRight() const
    {
        return m_Right;
    }

    glm::vec3 FreeCamera::GetUp() const
    {
        return m_Up;
    }

    void FreeCamera::UpdateDirection()
    {
        glm::vec3 direction;
        direction.x = cos(DegToRad(m_Yaw)) * cos(DegToRad(m_Pitch));
        direction.y = sin(DegToRad(m_Pitch));
        direction.z = sin(DegToRad(m_Yaw)) * cos(DegToRad(m_Pitch));

        m_Forward = glm::normalize(direction);
        m_Right = glm::normalize(glm::cross(m_Forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        m_Up = glm::normalize(glm::cross(m_Right, m_Forward));
    }
}
