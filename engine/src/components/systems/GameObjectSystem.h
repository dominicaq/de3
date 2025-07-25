#pragma once

#include "../GameObject.h"
#include <entt/entt.hpp>
#include <vector>

class GameObjectSystem {
public:
    GameObjectSystem(entt::registry& registry);
    ~GameObjectSystem();

    // Start() and Update() calls
    void startAll();
    void updateAll(const float& currentTime, const float& deltaTime);

    // Access all gameobjects
    std::vector<GameObject*> getActiveGameObjects() const;

private:
    entt::registry& m_registry;
};
