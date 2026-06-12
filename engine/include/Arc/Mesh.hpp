#pragma once

#include "Arc/Math.hpp"
#include "Arc/Renderer.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Arc
{
    struct MeshVertex
    {
        Vec3 position{};
        Vec3 normal{ 0.0f, 1.0f, 0.0f };
        Vec2 uv{};
        Vec4 tangent{ 1.0f, 0.0f, 0.0f, 1.0f };
    };

    struct MeshData
    {
        std::string name;
        std::vector<MeshVertex> vertices;
        std::vector<std::uint32_t> indices;
        Material material{};
    };

    struct ModelData
    {
        std::string name;
        std::vector<MeshData> meshes;
    };
}
