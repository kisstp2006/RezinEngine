#pragma once

#include <Rezin/ECS/Detail/TypeId.hpp>
#include <Rezin/ECS/EntityManager.hpp>
#include <Rezin/ECS/System.hpp>

#include <concepts>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rezin
{

template<typename Type>
concept SystemType = std::derived_from<Type, System>;

// A World owns an isolated set of entities, component pools, and systems. This
// allows a game world, editor preview, and tests to exist without sharing data.
class World final
{
public:
    explicit World(std::string name = "World");
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

    [[nodiscard]] std::string_view name() const noexcept;

    [[nodiscard]] EntityManager& entityManager() noexcept;
    [[nodiscard]] const EntityManager& entityManager() const noexcept;

    // Like ezWorld::GetOrCreateModule, each system type has at most one
    // instance in a World. onCreate() runs immediately after construction.
    template<SystemType SystemClass, typename... Arguments>
    SystemClass& getOrCreateSystem(Arguments&&... arguments)
    {
        const detail::TypeId type = detail::typeId<
            SystemClass,
            detail::SystemTypeFamily
        >();
        const auto existing = systemLookup_.find(type);

        if (existing != systemLookup_.end())
            return *static_cast<SystemClass*>(existing->second);

        if (updating_)
        {
            throw std::logic_error(
                "Systems cannot be created while the World is updating."
            );
        }

        auto system = std::make_unique<SystemClass>(
            std::forward<Arguments>(arguments)...
        );
        auto* result = system.get();

        systems_.push_back(std::move(system));
        systemLookup_.emplace(type, result);

        try
        {
            // Call through the base class so user overrides may stay protected.
            // Virtual dispatch still invokes SystemClass::onCreate().
            static_cast<System*>(result)->onCreate(*this);
        }
        catch (...)
        {
            systemLookup_.erase(type);
            systems_.pop_back();
            throw;
        }

        return *result;
    }

    template<SystemType SystemClass>
    [[nodiscard]] SystemClass* tryGetSystem() noexcept
    {
        const auto found = systemLookup_.find(
            detail::typeId<SystemClass, detail::SystemTypeFamily>()
        );
        return found != systemLookup_.end()
            ? static_cast<SystemClass*>(found->second)
            : nullptr;
    }

    template<SystemType SystemClass>
    [[nodiscard]] const SystemClass* tryGetSystem() const noexcept
    {
        const auto found = systemLookup_.find(
            detail::typeId<SystemClass, detail::SystemTypeFamily>()
        );
        return found != systemLookup_.end()
            ? static_cast<const SystemClass*>(found->second)
            : nullptr;
    }

    // Calls enabled systems in creation order. A later scheduler can add named
    // update phases and parallel execution without changing component APIs.
    void update(float deltaSeconds);

    // Calls onDestroy() in reverse creation order and removes all systems.
    void shutdown() noexcept;

private:
    static Entity::WorldId allocateWorldId();

    std::string name_;
    EntityManager entityManager_;
    std::vector<std::unique_ptr<System>> systems_;
    std::unordered_map<detail::TypeId, System*> systemLookup_;
    bool updating_ = false;
};

}
