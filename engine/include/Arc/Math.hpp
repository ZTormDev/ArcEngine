#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Arc
{
    struct Vec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct Color
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    struct Transform
    {
        Vec3 position{};
        Vec3 rotation{};
        Vec3 scale{ 1.0f, 1.0f, 1.0f };
    };

    constexpr float Pi = 3.14159265358979323846f;

    [[nodiscard]] constexpr float radians(float degrees)
    {
        return degrees * Pi / 180.0f;
    }

    [[nodiscard]] inline Vec3 operator+(Vec3 left, Vec3 right)
    {
        return { left.x + right.x, left.y + right.y, left.z + right.z };
    }

    [[nodiscard]] inline Vec3 operator-(Vec3 left, Vec3 right)
    {
        return { left.x - right.x, left.y - right.y, left.z - right.z };
    }

    [[nodiscard]] inline Vec3 operator*(Vec3 value, float scalar)
    {
        return { value.x * scalar, value.y * scalar, value.z * scalar };
    }

    [[nodiscard]] inline Vec3 operator*(float scalar, Vec3 value)
    {
        return value * scalar;
    }

    inline Vec3& operator+=(Vec3& left, Vec3 right)
    {
        left = left + right;
        return left;
    }

    [[nodiscard]] inline float dot(Vec3 left, Vec3 right)
    {
        return left.x * right.x + left.y * right.y + left.z * right.z;
    }

    [[nodiscard]] inline Vec3 cross(Vec3 left, Vec3 right)
    {
        return {
            left.y * right.z - left.z * right.y,
            left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x
        };
    }

    [[nodiscard]] inline float length(Vec3 value)
    {
        return std::sqrt(dot(value, value));
    }

    [[nodiscard]] inline Vec3 normalize(Vec3 value)
    {
        const float valueLength = length(value);
        if(valueLength <= 0.00001f)
        {
            return {};
        }

        return value * (1.0f / valueLength);
    }

    [[nodiscard]] inline float clamp(float value, float minValue, float maxValue)
    {
        return std::clamp(value, minValue, maxValue);
    }
}
