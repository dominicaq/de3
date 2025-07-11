#define GLM_FORCE_INLINE
#define GLM_FORCE_SSE2

#include "TransformSystem.h"

TransformSystem::TransformSystem(entt::registry& registry)
    : m_registry(registry) {}

void TransformSystem::updateTransformComponents() {
    // Start from root entities (those without a Parent)
    const auto& view = m_registry.view<ModelMatrix>(entt::exclude<Parent>);
    for (const auto& entity : view) {
        updateTransformRecursive(entity, glm::mat4(1.0f));
    }
}

void TransformSystem::updateTransformRecursive(const entt::entity& entity, const glm::mat4& parentMatrix) {
    auto& entStatus = m_registry.get<EntityStatus>(entity).status;
    if (!entStatus.test(EntityStatus::DIRTY_MODEL_MATRIX)) {
        return;
    }

    const auto& position = m_registry.get<Position>(entity).position;
    const auto& rotation = m_registry.get<Rotation>(entity).quaternion;
    const auto& scale = m_registry.get<Scale>(entity).scale;
    auto& modelMatrix = m_registry.get<ModelMatrix>(entity);

    glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), position);
    localMatrix *= glm::mat4_cast(rotation);
    localMatrix = glm::scale(localMatrix, scale);

    modelMatrix.matrix = parentMatrix * localMatrix;
    entStatus.reset(EntityStatus::DIRTY_MODEL_MATRIX);

    if (m_registry.all_of<Children>(entity)) {
        const auto& children = m_registry.get<Children>(entity).children;
        for (auto& child : children) {
            updateTransformRecursive(child, modelMatrix.matrix);
        }
    }
}
