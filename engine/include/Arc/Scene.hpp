#pragma once

#include "Arc/Math.hpp"
#include "Arc/Renderer.hpp"

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

    struct SceneObject
    {
        std::string name;
        MeshPrimitive primitive = MeshPrimitive::Cube;
        MeshHandle mesh{};
        Transform transform{};
        Material material{};
    };

    class Scene
    {
    public:
        SceneObject& addCube(std::string name, const Transform& transform, const Material& material);
        SceneObject& addSphere(std::string name, const Transform& transform, const Material& material);
        SceneObject& addMesh(std::string name, MeshHandle mesh, const Transform& transform, const Material& material);

        void clear();
        void render(Renderer& renderer, float groundY = 0.0f) const;

        [[nodiscard]] std::vector<SceneObject>& objects();
        [[nodiscard]] const std::vector<SceneObject>& objects() const;

    private:
        SceneObject& addObject(std::string name, MeshPrimitive primitive, MeshHandle mesh, const Transform& transform, const Material& material);

        std::vector<SceneObject> m_objects;
    };
}
