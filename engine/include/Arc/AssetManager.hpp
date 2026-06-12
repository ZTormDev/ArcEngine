#pragma once

#include "Arc/Mesh.hpp"

#include <string>

namespace Arc
{
    class AssetManager
    {
    public:
        [[nodiscard]] ModelData loadGltfModel(const std::string& path) const;
    };
}
