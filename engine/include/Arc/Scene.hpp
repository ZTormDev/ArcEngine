#pragma once

#include "Arc/Math.hpp"
#include "Arc/Renderer.hpp"
#include "Arc/ECS.hpp"

#include <string>
#include <vector>

namespace Arc
{
    enum class MeshPrimitive
    {
        Cube,
        Sphere,
        StaticMesh
    };

    // Components
    struct TagComponent
    {
        std::string name;
    };

    struct TransformComponent
    {
        Vec3 position{};
        Vec3 rotation{};
        Vec3 scale{ 1.0f, 1.0f, 1.0f };

        // Parent-child relationships
        Entity parent = NullEntity;
        std::vector<Entity> children;

        // Calculated model matrices
        float globalMatrix[16];
        float prevGlobalMatrix[16];

        TransformComponent()
        {
            // Identity matrix
            for(int i = 0; i < 16; ++i)
            {
                globalMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
                prevGlobalMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
            }
        }
    };

    struct MeshComponent
    {
        MeshPrimitive primitive = MeshPrimitive::Cube;
        MeshHandle mesh{};
        Material material{};
    };

    class Scene
    {
    public:
        Scene();
        ~Scene() = default;

        // Factory methods returning Entities for backward-compatibility or quick construction
        Entity addCube(std::string name, const Transform& transform, const Material& material);
        Entity addSphere(std::string name, const Transform& transform, const Material& material);
        Entity addMesh(std::string name, MeshHandle mesh, const Transform& transform, const Material& material);

        // Hierarchical scene graph functions
        void setParent(Entity child, Entity parent);
        void updateTransforms();

        void clear();
        void render(Renderer& renderer) const;

        [[nodiscard]] Registry& registry() { return m_registry; }
        [[nodiscard]] const Registry& registry() const { return m_registry; }

    private:
        void updateGlobalMatricesRecursive(Entity entity, const float* parentGlobalMtx);

        Registry m_registry;
    };
}
