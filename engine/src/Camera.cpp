#include "Arc/Camera.hpp"

namespace Arc
{
    Vec3 Camera::forward() const
    {
        const float cosPitch = std::cos(pitch);

        return normalize({
            cosPitch * std::sin(yaw),
            std::sin(pitch),
            -cosPitch * std::cos(yaw)
        });
    }

    Vec3 Camera::right() const
    {
        return normalize(cross(forward(), { 0.0f, 1.0f, 0.0f }));
    }

    Vec3 Camera::up() const
    {
        return normalize(cross(right(), forward()));
    }

    Camera& FreeCamera::camera()
    {
        return m_camera;
    }

    const Camera& FreeCamera::camera() const
    {
        return m_camera;
    }

    void FreeCamera::update(const Input& input, float deltaSeconds)
    {
        if(input.isMouseButtonDown(MouseButton::Right))
        {
            m_camera.yaw += static_cast<float>(input.mouseDeltaX()) * mouseSensitivity;
            m_camera.pitch -= static_cast<float>(input.mouseDeltaY()) * mouseSensitivity;
            m_camera.pitch = clamp(m_camera.pitch, radians(-88.0f), radians(88.0f));
        }

        Vec3 movement{};

        if(input.isKeyDown(Key::W))
        {
            movement += m_camera.forward();
        }

        if(input.isKeyDown(Key::S))
        {
            movement += m_camera.forward() * -1.0f;
        }

        if(input.isKeyDown(Key::D))
        {
            movement += m_camera.right();
        }

        if(input.isKeyDown(Key::A))
        {
            movement += m_camera.right() * -1.0f;
        }

        if(input.isKeyDown(Key::E))
        {
            movement += Vec3{ 0.0f, 1.0f, 0.0f };
        }

        if(input.isKeyDown(Key::Q))
        {
            movement += Vec3{ 0.0f, -1.0f, 0.0f };
        }

        if(length(movement) > 0.00001f)
        {
            float speed = moveSpeed;
            if(input.isKeyDown(Key::LeftShift))
            {
                speed *= fastMoveMultiplier;
            }

            m_camera.position += normalize(movement) * speed * deltaSeconds;
        }
    }
}
