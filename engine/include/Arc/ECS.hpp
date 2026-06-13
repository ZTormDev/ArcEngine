#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <typeindex>
#include <cassert>

namespace Arc
{
    using Entity = std::uint32_t;
    constexpr Entity NullEntity = 0;

    class IComponentPool
    {
    public:
        virtual ~IComponentPool() = default;
        virtual void remove(Entity entity) = 0;
        virtual bool has(Entity entity) const = 0;
    };

    template<typename T>
    class ComponentPool final : public IComponentPool
    {
    public:
        T& get(Entity entity)
        {
            assert(has(entity) && "Entity does not have component!");
            return m_components.at(entity);
        }

        const T& get(Entity entity) const
        {
            assert(has(entity) && "Entity does not have component!");
            return m_components.at(entity);
        }

        template<typename... Args>
        T& emplace(Entity entity, Args&&... args)
        {
            m_components.insert_or_assign(entity, T(std::forward<Args>(args)...));
            return m_components.at(entity);
        }

        void remove(Entity entity) override
        {
            m_components.erase(entity);
        }

        bool has(Entity entity) const override
        {
            return m_components.find(entity) != m_components.end();
        }

        std::unordered_map<Entity, T>& data() { return m_components; }
        const std::unordered_map<Entity, T>& data() const { return m_components; }

    private:
        std::unordered_map<Entity, T> m_components;
    };

    class Registry;

    template<typename... Components>
    class RegistryView final
    {
    public:
        explicit RegistryView(Registry& registry)
            : m_registry(registry)
        {
        }

        template<typename Func>
        void each(Func&& func)
        {
            auto entitiesCopy = m_registry.entities();
            for (Entity entity : entitiesCopy)
            {
                if (m_registry.isValid(entity) && (m_registry.template has<Components>(entity) && ...))
                {
                    func(entity, m_registry.template get<Components>(entity)...);
                }
            }
        }

    private:
        Registry& m_registry;
    };

    class Registry final
    {
    public:
        Registry() = default;
        ~Registry() = default;

        Registry(const Registry&) = delete;
        Registry& operator=(const Registry&) = delete;
        Registry(Registry&&) = delete;
        Registry& operator=(Registry&&) = delete;

        Entity create()
        {
            Entity entity = m_nextEntity++;
            m_entities.push_back(entity);
            return entity;
        }

        void destroy(Entity entity)
        {
            if (!isValid(entity))
            {
                return;
            }

            for (auto& [_, pool] : m_pools)
            {
                pool->remove(entity);
            }

            m_entities.erase(std::remove(m_entities.begin(), m_entities.end(), entity), m_entities.end());
        }

        bool isValid(Entity entity) const
        {
            return entity != NullEntity && std::find(m_entities.begin(), m_entities.end(), entity) != m_entities.end();
        }

        template<typename T, typename... Args>
        T& emplace(Entity entity, Args&&... args)
        {
            assert(isValid(entity) && "Entity is not valid!");
            return getPool<T>().emplace(entity, std::forward<Args>(args)...);
        }

        template<typename T>
        void remove(Entity entity)
        {
            assert(isValid(entity) && "Entity is not valid!");
            getPool<T>().remove(entity);
        }

        template<typename T>
        [[nodiscard]] T& get(Entity entity)
        {
            assert(isValid(entity) && "Entity is not valid!");
            return getPool<T>().get(entity);
        }

        template<typename T>
        [[nodiscard]] const T& get(Entity entity) const
        {
            assert(isValid(entity) && "Entity is not valid!");
            return getPool<T>().get(entity);
        }

        template<typename T>
        [[nodiscard]] bool has(Entity entity) const
        {
            if (!isValid(entity))
            {
                return false;
            }
            auto it = m_pools.find(std::type_index(typeid(T)));
            if (it == m_pools.end())
            {
                return false;
            }
            return it->second->has(entity);
        }

        template<typename... Components>
        [[nodiscard]] auto view()
        {
            return RegistryView<Components...>(*this);
        }

        [[nodiscard]] const std::vector<Entity>& entities() const { return m_entities; }

        void clear()
        {
            m_entities.clear();
            m_pools.clear();
            m_nextEntity = 1;
        }

    private:
        friend class RegistryView<>;

        template<typename... Components>
        friend class RegistryView;

        template<typename T>
        ComponentPool<T>& getPool()
        {
            auto typeKey = std::type_index(typeid(T));
            auto it = m_pools.find(typeKey);
            if (it == m_pools.end())
            {
                m_pools[typeKey] = std::make_unique<ComponentPool<T>>();
            }
            return *static_cast<ComponentPool<T>*>(m_pools[typeKey].get());
        }

        template<typename T>
        const ComponentPool<T>& getPool() const
        {
            auto typeKey = std::type_index(typeid(T));
            auto it = m_pools.find(typeKey);
            assert(it != m_pools.end() && "Pool has not been initialized for this type");
            return *static_cast<const ComponentPool<T>*>(it->second.get());
        }

        Entity m_nextEntity = 1;
        std::vector<Entity> m_entities;
        std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_pools;
    };
}
