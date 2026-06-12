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
        Sphere
    };

    struct SceneObject
    {
        std::string name;
        MeshPrimitive primitive = MeshPrimitive::Cube;
        Transform transform{};
        Material material{};
        bool castsShadow = true;
    };

    class Scene
    {
    public:
        SceneObject& addCube(std::string name, const Transform& transform, const Material& material, bool castsShadow = true);
        SceneObject& addSphere(std::string name, const Transform& transform, const Material& material, bool castsShadow = true);

        void clear();
        void render(Renderer& renderer, float groundY = 0.0f) const;

        [[nodiscard]] std::vector<SceneObject>& objects();
        [[nodiscard]] const std::vector<SceneObject>& objects() const;

    private:
        SceneObject& addObject(std::string name, MeshPrimitive primitive, const Transform& transform, const Material& material, bool castsShadow);

        std::vector<SceneObject> m_objects;
    };
}
