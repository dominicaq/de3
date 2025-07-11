#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "../Transform.h"
#include "../MetaData.h"

class TransformSystem {
public:
    TransformSystem(entt::registry& registry);
    ~TransformSystem() = default;

    void updateTransformComponents();

private:
    void updateTransformRecursive(const entt::entity& entity, const glm::mat4& parentMatrix);

    entt::registry& m_registry;
};
