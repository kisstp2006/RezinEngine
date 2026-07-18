#include <Rezin/ECS/EntityManager.hpp>

#include <limits>
#include <stdexcept>

namespace
{

rezin::Entity::Generation nextGeneration(
    rezin::Entity::Generation current
) noexcept
{
    // Generation zero is reserved for invalid/default handles.
    return current == std::numeric_limits<rezin::Entity::Generation>::max()
        ? 1
        : current + 1;
}

}

namespace rezin
{

EntityManager::EntityManager(Entity::WorldId worldId)
    : worldId_(worldId)
{
    if (worldId_ == 0)
    {
        throw std::invalid_argument(
            "EntityManager requires a non-zero World identifier."
        );
    }
}

Entity EntityManager::createEntity()
{
    ensureStructuralChangesAllowed();

    Entity::Index index = Entity::invalidIndex;

    if (!freeIndices_.empty())
    {
        index = freeIndices_.back();
        freeIndices_.pop_back();
        alive_[index] = 1;
    }
    else
    {
        if (generations_.size() >= Entity::invalidIndex)
        {
            throw std::overflow_error(
                "EntityManager exhausted all available 32-bit entity indices."
            );
        }

        index = static_cast<Entity::Index>(generations_.size());
        generations_.push_back(1);
        alive_.push_back(1);
    }

    ++entityCount_;
    return makeEntity(index);
}

void EntityManager::destroyEntity(Entity entity)
{
    ensureStructuralChangesAllowed();
    validate(entity);

    for (auto& [type, pool] : componentPools_)
    {
        static_cast<void>(type);
        pool->remove(entity.index());
    }

    alive_[entity.index()] = 0;
    generations_[entity.index()] = nextGeneration(
        generations_[entity.index()]
    );
    freeIndices_.push_back(entity.index());
    --entityCount_;
}

bool EntityManager::exists(Entity entity) const noexcept
{
    return entity.worldId() == worldId_
        && entity.index() < generations_.size()
        && alive_[entity.index()] != 0
        && generations_[entity.index()] == entity.generation();
}

std::size_t EntityManager::entityCount() const noexcept
{
    return entityCount_;
}

void EntityManager::clear()
{
    ensureStructuralChangesAllowed();

    for (auto& [type, pool] : componentPools_)
    {
        static_cast<void>(type);
        pool->clear();
    }

    freeIndices_.clear();
    freeIndices_.reserve(generations_.size());

    for (std::size_t index = 0; index < generations_.size(); ++index)
    {
        generations_[index] = nextGeneration(generations_[index]);
        alive_[index] = 0;
        freeIndices_.push_back(static_cast<Entity::Index>(index));
    }

    entityCount_ = 0;
}

void EntityManager::validate(Entity entity) const
{
    if (!exists(entity))
    {
        throw std::invalid_argument(
            "Entity handle is invalid, belongs to another World, or refers to an entity that was already destroyed."
        );
    }
}

void EntityManager::ensureStructuralChangesAllowed() const
{
    if (queryDepth_ != 0)
    {
        throw std::logic_error(
            "Structural ECS changes are not allowed during a query because compact component storage may move. Perform them after the query returns."
        );
    }
}

Entity EntityManager::makeEntity(Entity::Index entityIndex) const noexcept
{
    return Entity(
        entityIndex,
        generations_[entityIndex],
        worldId_
    );
}

}
