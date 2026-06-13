#include "Arc/Scene.hpp"

#include <bx/math.h>
#include <algorithm>
#include <utility>

namespace Arc
{
    Scene::Scene()
    {
    }

    Entity Scene::addCube(std::string name, const Transform& transform, const Material& material)
    {
        Entity entity = m_registry.create();
        m_registry.emplace<TagComponent>(entity, std::move(name));

        auto& tc = m_registry.emplace<TransformComponent>(entity);
        tc.position = transform.position;
        tc.rotation = transform.rotation;
        tc.scale = transform.scale;

        // Compute initial matrices
        bx::mtxSRT(
            tc.globalMatrix,
            tc.scale.x, tc.scale.y, tc.scale.z,
            tc.rotation.x, tc.rotation.y, tc.rotation.z,
            tc.position.x, tc.position.y, tc.position.z
        );
        std::copy(std::begin(tc.globalMatrix), std::end(tc.globalMatrix), std::begin(tc.prevGlobalMatrix));

        m_registry.emplace<MeshComponent>(entity, MeshPrimitive::Cube, MeshHandle{}, material);
        return entity;
    }

    Entity Scene::addSphere(std::string name, const Transform& transform, const Material& material)
    {
        Entity entity = m_registry.create();
        m_registry.emplace<TagComponent>(entity, std::move(name));

        auto& tc = m_registry.emplace<TransformComponent>(entity);
        tc.position = transform.position;
        tc.rotation = transform.rotation;
        tc.scale = transform.scale;

        // Compute initial matrices
        bx::mtxSRT(
            tc.globalMatrix,
            tc.scale.x, tc.scale.y, tc.scale.z,
            tc.rotation.x, tc.rotation.y, tc.rotation.z,
            tc.position.x, tc.position.y, tc.position.z
        );
        std::copy(std::begin(tc.globalMatrix), std::end(tc.globalMatrix), std::begin(tc.prevGlobalMatrix));

        m_registry.emplace<MeshComponent>(entity, MeshPrimitive::Sphere, MeshHandle{}, material);
        return entity;
    }

    Entity Scene::addMesh(std::string name, MeshHandle mesh, const Transform& transform, const Material& material)
    {
        Entity entity = m_registry.create();
        m_registry.emplace<TagComponent>(entity, std::move(name));

        auto& tc = m_registry.emplace<TransformComponent>(entity);
        tc.position = transform.position;
        tc.rotation = transform.rotation;
        tc.scale = transform.scale;

        // Compute initial matrices
        bx::mtxSRT(
            tc.globalMatrix,
            tc.scale.x, tc.scale.y, tc.scale.z,
            tc.rotation.x, tc.rotation.y, tc.rotation.z,
            tc.position.x, tc.position.y, tc.position.z
        );
        std::copy(std::begin(tc.globalMatrix), std::end(tc.globalMatrix), std::begin(tc.prevGlobalMatrix));

        m_registry.emplace<MeshComponent>(entity, MeshPrimitive::StaticMesh, mesh, material);
        return entity;
    }

    void Scene::setParent(Entity child, Entity parent)
    {
        if (!m_registry.isValid(child))
        {
            return;
        }

        auto& childTransform = m_registry.get<TransformComponent>(child);

        // Remove from existing parent if any
        if (childTransform.parent != NullEntity)
        {
            if (m_registry.isValid(childTransform.parent))
            {
                auto& prevParent = m_registry.get<TransformComponent>(childTransform.parent);
                prevParent.children.erase(
                    std::remove(prevParent.children.begin(), prevParent.children.end(), child),
                    prevParent.children.end()
                );
            }
        }

        childTransform.parent = parent;

        // Link to new parent
        if (parent != NullEntity && m_registry.isValid(parent))
        {
            auto& parentTransform = m_registry.get<TransformComponent>(parent);
            if (std::find(parentTransform.children.begin(), parentTransform.children.end(), child) == parentTransform.children.end())
            {
                parentTransform.children.push_back(child);
            }
        }
    }

    void Scene::updateTransforms()
    {
        // 1. Copy current global matrices to previous for motion vector rendering
        m_registry.view<TransformComponent>().each([](Entity, TransformComponent& tc) {
            std::copy(std::begin(tc.globalMatrix), std::end(tc.globalMatrix), std::begin(tc.prevGlobalMatrix));
        });

        // 2. Traverses hierarchy starting from root nodes (no parents)
        m_registry.view<TransformComponent>().each([this](Entity entity, TransformComponent& tc) {
            if (tc.parent == NullEntity)
            {
                float identity[16];
                bx::mtxIdentity(identity);
                updateGlobalMatricesRecursive(entity, identity);
            }
        });
    }

    void Scene::updateGlobalMatricesRecursive(Entity entity, const float* parentGlobalMtx)
    {
        auto& tc = m_registry.get<TransformComponent>(entity);

        // Compute local model matrix
        float localMtx[16];
        bx::mtxSRT(
            localMtx,
            tc.scale.x, tc.scale.y, tc.scale.z,
            tc.rotation.x, tc.rotation.y, tc.rotation.z,
            tc.position.x, tc.position.y, tc.position.z
        );

        // child_global = child_local * parent_global
        bx::mtxMul(tc.globalMatrix, localMtx, parentGlobalMtx);

        // Propagate to all child transforms recursively
        for (Entity child : tc.children)
        {
            if (m_registry.isValid(child))
            {
                updateGlobalMatricesRecursive(child, tc.globalMatrix);
            }
        }
    }

    void Scene::clear()
    {
        m_registry.clear();
    }

    void Scene::render(Renderer& renderer) const
    {
        auto& registryNonConst = const_cast<Registry&>(m_registry);

        registryNonConst.view<TransformComponent, MeshComponent>().each([&renderer](Entity, TransformComponent& tc, MeshComponent& mc) {
            switch(mc.primitive)
            {
            case MeshPrimitive::Cube:
                renderer.drawCube(tc.globalMatrix, tc.prevGlobalMatrix, mc.material);
                break;
            case MeshPrimitive::Sphere:
                renderer.drawSphere(tc.globalMatrix, tc.prevGlobalMatrix, mc.material);
                break;
            case MeshPrimitive::StaticMesh:
                renderer.drawMesh(mc.mesh, tc.globalMatrix, tc.prevGlobalMatrix, mc.material);
                break;
            }
        });
    }
}
