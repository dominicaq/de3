#pragma once

#include <string>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "components/GameObject.h"
#include "SceneData.h"

class SceneUtils {
public:
    // =========================================================================
    // Component Helpers
    // =========================================================================

    /**
     * Adds a GameObject component to the specified entity in the registry.
     * Associates the provided scene data with the entity, including transform and meta data.
     * @param registry - The registry to add the component to.
     * @param entity - The entity to which the component will be added.
     * @param data - The scene data to associate with the entity.
     * @return The newly added GameObject component.
     */
    static GameObject* addGameObjectComponent(entt::registry& registry, entt::entity entity, const SceneData& data);

    // =========================================================================
    // GameObject Helpers
    // =========================================================================
    /**
     * Creates an empty GameObject in the registry and associates it with the provided scene data.
     * @param registry - The registry to create the GameObject in.
     * @param data - The scene data to associate with the GameObject.
     */
    static void createEmptyGameObject(entt::registry& registry, const SceneData& data);
};
