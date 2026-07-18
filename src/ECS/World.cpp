#include <Rezin/ECS/World.hpp>

#include <atomic>
#include <limits>
#include <stdexcept>
#include <utility>

namespace rezin
{

World::World(std::string name)
    : name_(std::move(name)),
      entityManager_(allocateWorldId())
{
    if (name_.empty())
        name_ = "World";
}

World::~World()
{
    shutdown();
}

std::string_view World::name() const noexcept
{
    return name_;
}

EntityManager& World::entityManager() noexcept
{
    return entityManager_;
}

const EntityManager& World::entityManager() const noexcept
{
    return entityManager_;
}

void World::update(float deltaSeconds)
{
    if (deltaSeconds < 0.0f)
    {
        throw std::invalid_argument(
            "World update delta time cannot be negative."
        );
    }

    if (updating_)
    {
        throw std::logic_error(
            "World::update() cannot be called recursively."
        );
    }

    updating_ = true;

    try
    {
        for (const auto& system : systems_)
        {
            if (system->isEnabled())
                system->onUpdate(*this, deltaSeconds);
        }
    }
    catch (...)
    {
        updating_ = false;
        throw;
    }

    updating_ = false;
}

void World::shutdown() noexcept
{
    if (updating_)
        return;

    for (auto system = systems_.rbegin(); system != systems_.rend(); ++system)
        (*system)->onDestroy(*this);

    systemLookup_.clear();
    systems_.clear();
}

Entity::WorldId World::allocateWorldId()
{
    static std::atomic<Entity::WorldId> nextId{1};

    const Entity::WorldId id = nextId.fetch_add(
        1,
        std::memory_order_relaxed
    );

    if (id == 0 || id == std::numeric_limits<Entity::WorldId>::max())
    {
        throw std::overflow_error(
            "No more unique ECS World identifiers are available."
        );
    }

    return id;
}

}
