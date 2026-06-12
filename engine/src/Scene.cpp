#include "Arc/Scene.hpp"

#include <utility>

namespace Arc
{
    SceneObject& Scene::addCube(std::string name, const Transform& transform, const Material& material, bool castsShadow)
    {
        return addObject(std::move(name), MeshPrimitive::Cube, {}, transform, material, castsShadow);
    }

    SceneObject& Scene::addSphere(std::string name, const Transform& transform, const Material& material, bool castsShadow)
    {
        return addObject(std::move(name), MeshPrimitive::Sphere, {}, transform, material, castsShadow);
    }

    SceneObject& Scene::addMesh(std::string name, MeshHandle mesh, const Transform& transform, const Material& material, bool castsShadow)
    {
        return addObject(std::move(name), MeshPrimitive::StaticMesh, mesh, transform, material, castsShadow);
    }

    void Scene::clear()
    {
        m_objects.clear();
    }

    void Scene::render(Renderer& renderer, float groundY) const
    {
        for(const SceneObject& object : m_objects)
        {
            if(object.castsShadow)
            {
                renderer.drawBlobShadow(object.transform, groundY, object.primitive == MeshPrimitive::Sphere ? 0.24f : 0.36f);
            }

            switch(object.primitive)
            {
            case MeshPrimitive::Cube:
                renderer.drawCube(object.transform, object.material);
                break;
            case MeshPrimitive::Sphere:
                renderer.drawSphere(object.transform, object.material);
                break;
            case MeshPrimitive::StaticMesh:
                renderer.drawMesh(object.mesh, object.transform, object.material);
                break;
            }
        }
    }

    std::vector<SceneObject>& Scene::objects()
    {
        return m_objects;
    }

    const std::vector<SceneObject>& Scene::objects() const
    {
        return m_objects;
    }

    SceneObject& Scene::addObject(std::string name, MeshPrimitive primitive, MeshHandle mesh, const Transform& transform, const Material& material, bool castsShadow)
    {
        m_objects.push_back({ std::move(name), primitive, mesh, transform, material, castsShadow });
        return m_objects.back();
    }
}
