#pragma once

#include <SDL.h>
#include <glm/glm.hpp>

namespace arc
{
    class Window;

    class FreeCamera
    {
    public:
        void OnEvent(const SDL_Event& event, Window& window);
        void Update(float deltaSeconds);

        const glm::vec3& GetPosition() const { return m_Position; }
        glm::vec3 GetForward() const;
        glm::vec3 GetRight() const;
        glm::vec3 GetUp() const;

        float GetFovDegrees() const { return m_FovDegrees; }
        float GetNearPlane() const { return m_NearPlane; }
        float GetFarPlane() const { return m_FarPlane; }

    private:
        void UpdateDirection();

    private:
        glm::vec3 m_Position = {0.0f, 1.2f, 5.0f};
        glm::vec3 m_Forward = {0.0f, 0.0f, -1.0f};
        glm::vec3 m_Right = {1.0f, 0.0f, 0.0f};
        glm::vec3 m_Up = {0.0f, 1.0f, 0.0f};

        float m_Yaw = -90.0f;
        float m_Pitch = -8.0f;
        float m_MouseSensitivity = 0.12f;
        float m_MoveSpeed = 5.0f;
        float m_FastMultiplier = 4.0f;
        float m_FovDegrees = 60.0f;
        float m_NearPlane = 0.05f;
        float m_FarPlane = 1000.0f;

        bool m_MouseCaptured = false;
    };
}
