#pragma once

#include <Rezin/ECS/Component.hpp>
#include <Rezin/ECS/Entity.hpp>

#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace rezin::detail
{

// Type-erased interface used by EntityManager to own differently typed pools
// in one container. Gameplay code never needs to use this interface directly.
class IComponentPool
{
public:
    virtual ~IComponentPool() = default;

    virtual void remove(Entity::Index entityIndex) noexcept = 0;
    [[nodiscard]] virtual bool contains(Entity::Index entityIndex) const noexcept = 0;
    [[nodiscard]] virtual std::size_t size() const noexcept = 0;
    [[nodiscard]] virtual std::span<const Entity::Index> entityIndices() const noexcept = 0;
    virtual void clear() noexcept = 0;
};

// Each component type gets one pool, similar to an ezEngine component manager.
// Components are dense, so systems iterate contiguous memory. The sparse array
// maps an entity index to its location in that dense component array.
template<ComponentData Component>
class ComponentPool final : public IComponentPool
{
public:
    ComponentPool()
    {
        static_assert(
            std::is_nothrow_move_assignable_v<Component>,
            "ECS components must be nothrow move-assignable so compact removal remains safe."
        );
    }

    Component& add(Entity::Index entityIndex, Component component)
    {
        if (contains(entityIndex))
        {
            throw std::logic_error(
                "Cannot add the same component type to an entity more than once."
            );
        }

        ensureSparseSize(entityIndex);

        const std::size_t denseIndex = components_.size();
        components_.push_back(std::move(component));

        try
        {
            entities_.push_back(entityIndex);
        }
        catch (...)
        {
            components_.pop_back();
            throw;
        }

        sparse_[entityIndex] = denseIndex;
        return components_.back();
    }

    void remove(Entity::Index entityIndex) noexcept override
    {
        if (!contains(entityIndex))
            return;

        // Compact removal fills the erased slot with the final component. This
        // is the same idea as ezEngine's compact component storage: iteration
        // stays dense, but pointers may move after structural changes.
        const std::size_t removedIndex = sparse_[entityIndex];
        const std::size_t lastIndex = components_.size() - 1;

        if (removedIndex != lastIndex)
        {
            components_[removedIndex] = std::move(components_[lastIndex]);

            const Entity::Index movedEntity = entities_[lastIndex];
            entities_[removedIndex] = movedEntity;
            sparse_[movedEntity] = removedIndex;
        }

        components_.pop_back();
        entities_.pop_back();
        sparse_[entityIndex] = invalidDenseIndex;
    }

    [[nodiscard]] bool contains(Entity::Index entityIndex) const noexcept override
    {
        return entityIndex < sparse_.size()
            && sparse_[entityIndex] != invalidDenseIndex;
    }

    [[nodiscard]] Component* tryGet(Entity::Index entityIndex) noexcept
    {
        return contains(entityIndex)
            ? &components_[sparse_[entityIndex]]
            : nullptr;
    }

    [[nodiscard]] const Component* tryGet(Entity::Index entityIndex) const noexcept
    {
        return contains(entityIndex)
            ? &components_[sparse_[entityIndex]]
            : nullptr;
    }

    [[nodiscard]] Component& get(Entity::Index entityIndex) noexcept
    {
        return components_[sparse_[entityIndex]];
    }

    [[nodiscard]] const Component& get(Entity::Index entityIndex) const noexcept
    {
        return components_[sparse_[entityIndex]];
    }

    [[nodiscard]] std::size_t size() const noexcept override
    {
        return components_.size();
    }

    [[nodiscard]] std::span<const Entity::Index> entityIndices() const noexcept override
    {
        return entities_;
    }

    void clear() noexcept override
    {
        components_.clear();
        entities_.clear();
        sparse_.clear();
    }

private:
    static constexpr std::size_t invalidDenseIndex =
        std::numeric_limits<std::size_t>::max();

    void ensureSparseSize(Entity::Index entityIndex)
    {
        const std::size_t requiredSize =
            static_cast<std::size_t>(entityIndex) + 1;

        if (sparse_.size() < requiredSize)
            sparse_.resize(requiredSize, invalidDenseIndex);
    }

    // Structure-of-arrays at the component-type level: entities and each
    // component type live in separate contiguous arrays.
    std::vector<Component> components_;
    std::vector<Entity::Index> entities_;
    std::vector<std::size_t> sparse_;
};

}
