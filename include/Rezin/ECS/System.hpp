#pragma once

namespace rezin
{

class World;

// Systems contain behavior and transform component data once per frame. This
// mirrors Unity's OnCreate/OnUpdate/OnDestroy lifecycle and ezEngine world
// modules, while keeping components free of virtual functions and behavior.
class System
{
public:
    virtual ~System() = default;

    System(const System&) = delete;
    System& operator=(const System&) = delete;
    System(System&&) = delete;
    System& operator=(System&&) = delete;

    [[nodiscard]] bool isEnabled() const noexcept
    {
        return enabled_;
    }

    void setEnabled(bool enabled) noexcept
    {
        enabled_ = enabled;
    }

protected:
    System() = default;

    virtual void onCreate(World& world)
    {
        static_cast<void>(world);
    }

    virtual void onUpdate(World& world, float deltaSeconds) = 0;

    virtual void onDestroy(World& world) noexcept
    {
        static_cast<void>(world);
    }

private:
    friend class World;

    bool enabled_ = true;
};

}
