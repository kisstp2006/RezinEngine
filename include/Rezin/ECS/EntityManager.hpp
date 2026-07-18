#pragma once

#include <Rezin/ECS/Component.hpp>
#include <Rezin/ECS/Detail/ComponentPool.hpp>
#include <Rezin/ECS/Detail/TypeId.hpp>
#include <Rezin/ECS/Entity.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rezin
{

class World;

// Unity-style entry point for entity and component operations. A World owns
// exactly one EntityManager, keeping multiple worlds completely independent.
class EntityManager final
{
public:
    ~EntityManager() = default;

    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;
    EntityManager(EntityManager&&) = delete;
    EntityManager& operator=(EntityManager&&) = delete;

    [[nodiscard]] Entity createEntity();
    void destroyEntity(Entity entity);

    [[nodiscard]] bool exists(Entity entity) const noexcept;
    [[nodiscard]] std::size_t entityCount() const noexcept;

    // Adding or removing a component is a structural change. Do it outside a
    // query, because compact storage may move component objects in memory.
    template<ComponentData Component>
    Component& addComponentData(
        Entity entity,
        Component component = {}
    )
    {
        ensureStructuralChangesAllowed();
        validate(entity);

        return getOrCreatePool<Component>().add(
            entity.index(),
            std::move(component)
        );
    }

    // Convenient Unity-style tag/default-component operation.
    template<ComponentData Component>
    Component& addComponent(Entity entity)
    {
        return addComponentData<Component>(entity, Component{});
    }

    template<ComponentData Component>
    [[nodiscard]] bool hasComponent(Entity entity) const
    {
        validate(entity);

        const auto* pool = findPool<Component>();
        return pool != nullptr && pool->contains(entity.index());
    }

    template<ComponentData Component>
    [[nodiscard]] Component& getComponentData(Entity entity)
    {
        validate(entity);

        auto* component = tryGetComponentData<Component>(entity);
        if (component == nullptr)
        {
            throw std::out_of_range(
                "Entity does not contain the requested component type."
            );
        }

        return *component;
    }

    template<ComponentData Component>
    [[nodiscard]] const Component& getComponentData(Entity entity) const
    {
        validate(entity);

        const auto* component = tryGetComponentData<Component>(entity);
        if (component == nullptr)
        {
            throw std::out_of_range(
                "Entity does not contain the requested component type."
            );
        }

        return *component;
    }

    template<ComponentData Component>
    [[nodiscard]] Component* tryGetComponentData(Entity entity) noexcept
    {
        if (!exists(entity))
            return nullptr;

        auto* pool = findPool<Component>();
        return pool != nullptr ? pool->tryGet(entity.index()) : nullptr;
    }

    template<ComponentData Component>
    [[nodiscard]] const Component* tryGetComponentData(Entity entity) const noexcept
    {
        if (!exists(entity))
            return nullptr;

        const auto* pool = findPool<Component>();
        return pool != nullptr ? pool->tryGet(entity.index()) : nullptr;
    }

    template<ComponentData Component>
    void setComponentData(Entity entity, Component component)
    {
        Component& stored = getComponentData<Component>(entity);
        stored = std::move(component);
    }

    template<ComponentData Component>
    bool removeComponent(Entity entity)
    {
        ensureStructuralChangesAllowed();
        validate(entity);

        auto* pool = findPool<Component>();
        if (pool == nullptr || !pool->contains(entity.index()))
            return false;

        pool->remove(entity.index());
        return true;
    }

    // Query calls the function once for every entity containing all requested
    // component types. The smallest pool drives iteration to reduce checks.
    // Do not create/destroy entities or add/remove components inside a query;
    // queue those operations and perform them after the query returns.
    template<ComponentData... Components, typename Function>
        requires (sizeof...(Components) > 0)
    void query(Function&& function)
    {
        // Resolve the type-index map once. The inner loop then uses direct pool
        // pointers instead of performing hash lookups for every entity.
        auto typedPools = std::tuple{
            findPool<Components>()...
        };

        const auto pools = std::apply(
            [](auto*... pool)
            {
                return std::array<
                    detail::IComponentPool*,
                    sizeof...(Components)
                >{pool...};
            },
            typedPools
        );

        if (std::ranges::any_of(
            pools,
            [](const detail::IComponentPool* pool)
            {
                return pool == nullptr;
            }
        ))
        {
            return;
        }

        detail::IComponentPool* driver = *std::ranges::min_element(
            pools,
            {},
            [](const detail::IComponentPool* pool)
            {
                return pool->size();
            }
        );

        QueryScope queryScope(*this);

        for (const Entity::Index entityIndex : driver->entityIndices())
        {
            const bool containsEveryComponent = std::apply(
                [entityIndex](auto*... pool)
                {
                    return (pool->contains(entityIndex) && ...);
                },
                typedPools
            );

            if (!containsEveryComponent)
                continue;

            std::apply(
                [this, entityIndex, &function](auto*... pool)
                {
                    std::invoke(
                        function,
                        makeEntity(entityIndex),
                        pool->get(entityIndex)...
                    );
                },
                typedPools
            );
        }
    }

    void clear();

private:
    friend class World;

    explicit EntityManager(Entity::WorldId worldId);

    class QueryScope final
    {
    public:
        explicit QueryScope(EntityManager& manager) noexcept
            : manager_(manager)
        {
            ++manager_.queryDepth_;
        }

        ~QueryScope()
        {
            --manager_.queryDepth_;
        }

        QueryScope(const QueryScope&) = delete;
        QueryScope& operator=(const QueryScope&) = delete;

    private:
        EntityManager& manager_;
    };

    void validate(Entity entity) const;
    void ensureStructuralChangesAllowed() const;
    [[nodiscard]] Entity makeEntity(Entity::Index entityIndex) const noexcept;

    template<ComponentData Component>
    detail::ComponentPool<Component>& getOrCreatePool()
    {
        const detail::TypeId type = detail::typeId<
            Component,
            detail::ComponentTypeFamily
        >();
        const auto existing = componentPools_.find(type);

        if (existing != componentPools_.end())
        {
            return *static_cast<detail::ComponentPool<Component>*>(
                existing->second.get()
            );
        }

        auto pool = std::make_unique<detail::ComponentPool<Component>>();
        auto* result = pool.get();
        componentPools_.emplace(type, std::move(pool));
        return *result;
    }

    template<ComponentData Component>
    [[nodiscard]] detail::ComponentPool<Component>* findPool() noexcept
    {
        const auto found = componentPools_.find(
            detail::typeId<Component, detail::ComponentTypeFamily>()
        );
        return found != componentPools_.end()
            ? static_cast<detail::ComponentPool<Component>*>(found->second.get())
            : nullptr;
    }

    template<ComponentData Component>
    [[nodiscard]] const detail::ComponentPool<Component>* findPool() const noexcept
    {
        const auto found = componentPools_.find(
            detail::typeId<Component, detail::ComponentTypeFamily>()
        );
        return found != componentPools_.end()
            ? static_cast<const detail::ComponentPool<Component>*>(found->second.get())
            : nullptr;
    }

    Entity::WorldId worldId_ = 0;
    std::vector<Entity::Generation> generations_;
    std::vector<std::uint8_t> alive_;
    std::vector<Entity::Index> freeIndices_;
    std::unordered_map<detail::TypeId, std::unique_ptr<detail::IComponentPool>> componentPools_;
    std::size_t entityCount_ = 0;
    std::size_t queryDepth_ = 0;
};

}
