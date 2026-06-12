#pragma once

#include "Arc/Input.hpp"
#include "Arc/Math.hpp"

namespace Arc
{
    struct Camera
    {
        Vec3 position{ 0.0f, 1.6f, 6.0f };
        float yaw = 0.0f;
        float pitch = 0.0f;
        float verticalFovDegrees = 70.0f;
        float nearPlane = 0.01f;
        float farPlane = 4000.0f;

        [[nodiscard]] Vec3 forward() const;
        [[nodiscard]] Vec3 right() const;
        [[nodiscard]] Vec3 up() const;
    };

    class FreeCamera
    {
    public:
        [[nodiscard]] Camera& camera();
        [[nodiscard]] const Camera& camera() const;

        void update(const Input& input, float deltaSeconds);

        float moveSpeed = 5.0f;
        float fastMoveMultiplier = 4.0f;
        float mouseSensitivity = 0.0025f;

    private:
        Camera m_camera{};
    };
}
